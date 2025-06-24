#ifndef TSWD_H
#define TSWD_H

#include <utility>
#include <vector>
#include <chrono>
#include <limits>
#include <optional>
#include "random.h"
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::tswd {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{};
		uint64_t retire_epoch{};
		uint64_t time_stamp{};
		int v{};
	};

	struct alignas(std::hardware_destructive_interference_size) Window {
		Window() = default;

		bool CAS(uint64_t expected, uint64_t desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&time_stamp),
				&expected, desired);
		}

		volatile uint64_t time_stamp{};
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

		void Enq(Node* node, uint64_t put_ts) {
			auto tail_ts = tail_->time_stamp;
			node->time_stamp = std::max(put_ts, tail_ts) + 1;
			tail_->next = node;
			tail_ = node;
		}

		std::pair<std::optional<int>, Node*> TryDeq(EBR<Node>& ebr, int depth,
			uint64_t get_ts, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_head = head_;
				auto first = loc_head->next;
				if (nullptr == first) {
					return std::make_pair(std::nullopt, loc_head); // pq is empty
				}
				if (first->time_stamp > get_ts + depth) {
					return std::make_pair(std::nullopt, nullptr); // retry required
				}
				auto value = first->v;
				rdm.LockDeq();
				if (true == CAS(head_, loc_head, first)) {
					rdm.Deq(first);
					rdm.UnlockDeq();
					ebr.Retire(loc_head);
					return std::make_pair(value, nullptr);
				}
				rdm.UnlockDeq();
			}
		}

		auto GetTailTimeStamp() const {
			return tail_->time_stamp;
		}
	private:
		bool CAS(Node* volatile& trg, Node* expected, Node* desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&trg),
				reinterpret_cast<uint64_t*>(&expected),
				reinterpret_cast<uint64_t>(desired));
		}

		alignas(std::hardware_destructive_interference_size) Node* tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	class TSWD {
	public:
		TSWD(int num_thread, int depth)
			: depth_{ depth }, queues_(num_thread), ebr_{ num_thread }
			, num_delay_op_{ 3 * GetDelayOpPerUs() } {
		}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			auto node = new Node{ v };

			// Unless a dequeue occurs when the queue is empty,
			// using the moment of reading the time-stamp of window put
			// as the linearization point does not affect linearizability.
			rdm_.LockEnq();
			auto put_ts = window_put_.time_stamp;
			rdm_.Enq(node);
			rdm_.UnlockEnq();

			auto& pq = queues_[MyThreadID::Get()];

			if (pq.GetTailTimeStamp() >= put_ts + depth_) {

				window_put_.CAS(put_ts, put_ts + depth_);
				put_ts += depth_;
			}
			pq.Enq(node, put_ts);
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_heads(queues_.size());
			size_t id = MyThreadID::Get();

			ebr_.StartOp();
			while (true) {
				int cnt_empty{};
				auto put_ts = window_put_.time_stamp;
				auto get_ts = window_get_.time_stamp;
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& pq = queues_[id];
					auto [value, old_head] = pq.TryDeq(ebr_, depth_, get_ts, rdm_);
					if (nullptr != old_head) {
						old_heads[id] = old_head;
						cnt_empty += 1;
					} else if (value.has_value()) {
						ebr_.EndOp();
						return value;
					}
					id = (id + 1) % queues_.size();
				}

				if (queues_.size() == cnt_empty) {
					bool is_empty{ true };
					for (size_t i = 1; i < queues_.size(); ++i) {
						id = (i + MyThreadID::Get()) % queues_.size();
						auto next = old_heads[id]->next;
						if (nullptr != next) {
							is_empty = false;
							break;
						}
					}
					if (is_empty) {
						ebr_.EndOp();
						return std::nullopt;
					}
				} else {
					id = MyThreadID::Get();
				}

				window_get_.CAS(get_ts, get_ts + depth_);
			}
		}
	private:
		int GetDelayOpPerUs() {
			if (kUndefinedDelay != delay_per_us_) {
				return delay_per_us_;
			}

			Stopwatch sw;
			sw.Start();
			constexpr int kLoop = 1'000'000'000;
			for (volatile int i = 0; i < kLoop; ++i) {}
			auto us = sw.GetDuration() * 1.0e6;

			delay_per_us_ = static_cast<int>(kLoop / us);

			return delay_per_us_;
		}

		static constexpr int kUndefinedDelay{ -1 };
		static int delay_per_us_;
		int num_delay_op_;

		int depth_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		Window window_get_;
		Window window_put_;
		benchmark::RelaxationDistanceManager rdm_;
	};

	int TSWD::delay_per_us_{ TSWD::kUndefinedDelay };
}

#endif