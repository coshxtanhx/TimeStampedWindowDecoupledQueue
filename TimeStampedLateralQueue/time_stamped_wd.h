#ifndef TIME_STAMPED_WD_H
#define TIME_STAMPED_WD_H

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

		Node* volatile next{ nullptr };
		uint64_t retire_epoch{};
		uint64_t time_stamp{};
		int v{};
		int thread_id{ MyThread::GetID() };
	};

	struct alignas(std::hardware_destructive_interference_size) Window {
		Window() = default;

		bool CAS(uint64_t expected, uint64_t desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&max),
				&expected, desired);
		}

		volatile uint64_t max{};
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

		void Enq(Node* node, uint64_t put_max, benchmark::RelaxationDistanceManager& rdm) {
			auto tail_ts = tail_->time_stamp;
			node->time_stamp = (tail_ts <= put_max) ? (put_max + 1) : (tail_ts + 1);

			rdm.LockEnq();
			tail_->next = node;
			rdm.Enq(node, node->thread_id);
			rdm.UnlockEnq();
			tail_ = node;
		}

		std::pair<std::optional<int>, Node*> TryDeq(EBR<Node>& ebr, int depth,
			uint64_t get_max, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_head = head_;
				auto first = loc_head->next;
				if (nullptr == first) {
					return std::make_pair(std::nullopt, loc_head); // pq is empty
				}
				if (first->time_stamp > get_max + depth) {
					return std::make_pair(std::nullopt, nullptr); // retry required
				}
				auto value = first->v;
				rdm.LockDeq();
				if (true == CAS(head_, loc_head, first)) {
					rdm.Deq(first, first->thread_id);
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

		alignas(std::hardware_destructive_interference_size) Node* volatile tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	class TimeStampedWd {
	public:
		TimeStampedWd(int num_thread, int depth)
			: depth_{ depth }, queues_(num_thread), ebr_{ num_thread } {
		}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			ebr_.StartOp();
			auto node = new Node{ v };
			auto put_max = window_put_.max;
			auto& pq = queues_[MyThread::GetID()];

			if (pq.GetTailTimeStamp() < put_max + depth_) {
				pq.Enq(node, put_max, rdm_);
			}
			else {
				window_put_.CAS(put_max, put_max + depth_);
				pq.Enq(node, put_max, rdm_);
			}

			ebr_.EndOp();
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_heads(queues_.size());
			size_t id = MyThread::GetID();
			ebr_.StartOp();
			while (true) {
				int cnt_empty{};
				auto get_max = window_get_.max;
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& pq = queues_[id];
					auto [value, old_head] = pq.TryDeq(ebr_, depth_, get_max, rdm_);
					if (nullptr != old_head) {
						old_heads[id] = old_head;
						cnt_empty += 1;
					}
					else if (value.has_value()) {
						ebr_.EndOp();
						return value;
					}
					id = (id + 1) % queues_.size();
				}

				auto put_max = window_put_.max;
				if (get_max < put_max) {
					window_get_.CAS(get_max, get_max + depth_);
				}

				if (queues_.size() == cnt_empty) {
					bool is_empty{ true };
					for (size_t i = 1; i < queues_.size(); ++i) {
						id = (i + MyThread::GetID()) % queues_.size();
						if (nullptr != old_heads[id]->next) {
							is_empty = false;
							break;
						}
					}
					if (is_empty) {
						ebr_.EndOp();
						return std::nullopt;
					}
				}
				else {
					id = MyThread::GetID();
				}
			}
		}
	private:
		int depth_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		Window window_get_;
		Window window_put_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif