#ifndef TS_INTERVAL_H
#define TS_INTERVAL_H

#include <utility>
#include <optional>
#include <vector>
#include <chrono>
#include <limits>
#include "ebr.h"
#include "idle.h"
#include "relaxation_distance.h"
#include "stopwatch.h"

namespace lf::ts_interval {
	class TimeStamp {
	public:
		TimeStamp() = default;
		TimeStamp(uint64_t t1, uint64_t t2) : t1_{ t1 }, t2_{ t2 } {}
		TimeStamp(int delay) {
			Set(delay);
		}

		void Set(int delay) {
			t1_ = std::chrono::duration_cast<Resolution>(Clock::now() - tp_base_).count();
			idle.Do(static_cast<float>(delay));
			t2_ = std::chrono::duration_cast<Resolution>(Clock::now() - tp_base_).count();
		}

		bool operator<(const TimeStamp& rhs) const {
			return t2_ < rhs.t1_;
		}
	private:
		using Clock = std::chrono::steady_clock;
		using Resolution = std::chrono::microseconds;

		static Clock::time_point tp_base_;
		uint64_t t1_;
		uint64_t t2_;
	};

	TimeStamp::Clock::time_point TimeStamp::tp_base_{ std::chrono::steady_clock::now() };

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

		void Enq(int v, int delay) {
			auto node = new Node{ v, delay };
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
				reinterpret_cast<volatile std::atomic<Node*>*>(&trg),
				&expected, desired);
		}

		alignas(std::hardware_destructive_interference_size) Node* volatile tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	class TSInterval {
	public:
		TSInterval(int num_thread, int delay_microsec_) : delay_microsec_{ delay_microsec_ }
			, queues_(num_thread) , ebr_{ num_thread } {}

		void CheckRelaxationDistance() {
			// Does not support
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance(); // Returns (0, 0, 0)
		}

		void Enq(int v) {
			queues_[MyThreadID::Get()].Enq(v, delay_microsec_);
		}

		std::optional<int> Deq() {
			ebr_.StartOp();
			size_t id = MyThreadID::Get();
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
					auto value = trg->TryDeq(ebr_, youngest);
					if (value.has_value()) {
						ebr_.EndOp();
						return value;
					}
				}
			}
		}
	private:
		int delay_microsec_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif