#ifndef TS_ATOMIC_H
#define TS_ATOMIC_H

#include <utility>
#include <optional>
#include <vector>
#include <chrono>
#include <limits>
#include "ebr.h"
#include "relaxation_distance.h"
#include "stopwatch.h"

namespace lf::ts_atomic {
	struct Node {
		Node() = default;
		Node(int v, uint64_t time_stamp) : v{ v }, time_stamp{ time_stamp } {}

		Node* volatile next{};
		uint64_t retire_epoch{};
		uint64_t time_stamp{};
		int v{};
	};

	class PartialQueue {
	public:
		PartialQueue() : tail_{ new Node }, head_{ tail_ } {}
		~PartialQueue() {
			while (nullptr != head_->next) {
				Node* t = head_;
				head_ = head_->next;
				delete t;
			}
			delete head_;
		}

		void Enq(int v, uint64_t time_stamp) {
			auto node = new Node{ v, time_stamp };
			tail_->next = node;
			tail_ = node;
		}

		std::optional<int> TryDeq(EBR<Node>& ebr, Node* first) {
			auto loc_head = head_;
			if (loc_head->next != first) {
				return std::nullopt;
			}
			if (false == CAS(head_, loc_head, first)) {
				return std::nullopt;
			}
			ebr.Retire(loc_head);
			return first->v;
		}

		const auto GetHead() const {
			return head_;
		}

	private:
		bool CAS(Node* volatile& trg, Node* expected, Node* desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&trg),
				reinterpret_cast<uint64_t*>(&expected),
				reinterpret_cast<uint64_t>(desired));
		}

		alignas(std::hardware_destructive_interference_size) Node* volatile tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	class TSAtomic {
	public:
		TSAtomic(int num_thread) : queues_(num_thread) , ebr_{ num_thread } {}

		void CheckRelaxationDistance() {
			// Does not support
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance(); // Returns (0, 0, 0)
		}

		void Enq(int v) {
			queues_[MyThreadID::Get()].Enq(v, cnt_.fetch_add(1));
		}

		std::optional<int> Deq() {
			ebr_.StartOp();
			size_t id = MyThreadID::Get();
			while (true) {
				auto min_time_stamp{ std::numeric_limits<uint64_t>::max() };
				Node* youngest{};
				PartialQueue* trg{};
				std::vector<Node*> old_heads(queues_.size());

				for (size_t i = 0; i < queues_.size(); ++i) {
					auto head = queues_[id].GetHead();
					auto first = head->next;
					if (nullptr == first) {
						old_heads[id] = head;
					} else if (first->time_stamp < min_time_stamp) {
						min_time_stamp = first->time_stamp;
						youngest = first;
						trg = &queues_[id];
					}
					id = (id + 1) % queues_.size();
				}

				if (nullptr == youngest) {
					for (size_t i = 0; i < old_heads.size(); ++i) {
						if (nullptr != old_heads[i]->next) {
							id = i;
							break;
						}
						if (i == old_heads.size() - 1) {
							ebr_.EndOp();
							return std::nullopt;
						}
					}
				} else {
					auto value = trg->TryDeq(ebr_, youngest);
					if (value.has_value()) {
						ebr_.EndOp();
						return value;
					}
				}
			}
		}
	private:
		std::atomic<uint64_t> cnt_{ 1 };
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif