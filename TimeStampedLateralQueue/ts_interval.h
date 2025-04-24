#ifndef TS_INTERVAL_H
#define TS_INTERVAL_H

#include <utility>
#include <optional>
#include <vector>
#include <chrono>
#include <limits>
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::ts {
	class TimeStamp {
	public:
		TimeStamp() = default;
		TimeStamp(uint64_t t1, uint64_t t2) : t1_{ t1 }, t2_{ t2 } {}
		TimeStamp(int delay) {
			Set(delay);
		}

		void Set(int delay) {
			using namespace std::chrono;
			t1_ = duration_cast<microseconds>(steady_clock::now() - tp_base_).count();
			for (volatile int i = 0; i < delay; ++i) {}
			t2_ = duration_cast<microseconds>(steady_clock::now() - tp_base_).count();

		}

		bool operator<(const TimeStamp& rhs) const {
			return t2_ < rhs.t1_;
		}
	private:
		static std::chrono::steady_clock::time_point tp_base_;
		uint64_t t1_;
		uint64_t t2_;
	};

	std::chrono::steady_clock::time_point TimeStamp::tp_base_{ std::chrono::steady_clock::now() };

	struct Node {
		Node() = default;
		Node(int v, int delay) : v{ v }, time_stamp{ delay } {}

		Node* volatile next{};
		uint64_t retire_epoch{};
		TimeStamp time_stamp{};
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

		void Enq(int v, int delay, benchmark::RelaxationDistanceManager& rdm) {
			auto node = new Node{ v, delay };
			rdm.LockEnq();
			tail_->next = node;
			rdm.Enq(node);
			rdm.UnlockEnq();
			tail_ = node;
		}

		std::optional<int> TryDeq(EBR<Node>& ebr, Node* first, benchmark::RelaxationDistanceManager& rdm) {
			auto loc_head = head_;
			if (loc_head->next != first) {
				return std::nullopt;
			}
			rdm.LockDeq();
			if (false == CAS(head_, loc_head, first)) {
				rdm.UnlockDeq();
				return std::nullopt;
			}
			rdm.Deq(first);
			rdm.UnlockDeq();
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

	class TSInterval {
	public:
		TSInterval(int num_thread, int delay) : delay_{ delay }, queues_(num_thread) , ebr_{ num_thread } {}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			queues_[MyThread::GetID()].Enq(v, delay_, rdm_);
		}

		std::optional<int> Deq() {
			ebr_.StartOp();
			size_t id = MyThread::GetID();
			while (true) {
				TimeStamp min_time_stamp{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max() };
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
					auto value = trg->TryDeq(ebr_, youngest, rdm_);
					if (value.has_value()) {
						ebr_.EndOp();
						return value;
					}
				}
			}
		}
	private:
		int delay_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif