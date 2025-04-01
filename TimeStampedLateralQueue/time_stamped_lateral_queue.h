#ifndef TIME_STAMPED_LATERAL_QUEUE_H
#define TIME_STAMPED_LATERAL_QUEUE_H

#include <utility>
#include <vector>
#include <chrono>
#include <limits>
#include <optional>
#include "random.h"
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::tsl {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{ nullptr };
		uint64_t retire_epoch{};
		uint64_t time_stamp{};
		int v{};
		int thread_id{ MyThread::GetID() };
	};

	class alignas(std::hardware_destructive_interference_size) LateralQueue {
	public:
		LateralQueue() : tail_{ new Node }, head_{ tail_ } {}
		~LateralQueue() {
			while (nullptr != head_->next) {
				Node* t = head_;
				head_ = head_->next;
				delete t;
			}
			delete head_;
		}

		bool TryEnq(Node* node, uint64_t lq_ts, int depth, benchmark::RelaxationDistanceManager& rdm) {
			node->time_stamp = lq_ts + depth;
			auto loc_tail = tail_;
			auto next = loc_tail->next;
			if (tail_ != loc_tail) {
				return false;
			}
			if (tail_->time_stamp != lq_ts) {
				return false;
			}

			if (nullptr == next) {
				rdm.LockEnq();
				if (true == CAS(loc_tail->next, nullptr, node)) {
					rdm.Enq(node, node->thread_id);
					
					rdm.UnlockEnq();
					CAS(tail_, loc_tail, node);
					return true;
				}
				rdm.UnlockEnq();
				return false;
			}
			CAS(tail_, loc_tail, next);
			return false;
		}

		std::pair<std::optional<int>, bool> TryDeq(EBR<Node>& ebr, int depth, uint64_t lq_ts, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_head = head_;
				auto first = loc_head->next;
				auto loc_tail = tail_;
				if (nullptr == first) {
					return std::make_pair(std::nullopt, true); // lq is empty
				}
				if (loc_head == loc_tail) {
					CAS(tail_, loc_tail, first);
					continue;
				}
				if (first->time_stamp > lq_ts + depth) {
					return std::make_pair(std::nullopt, false); // retry required
				}

				auto value = first->v;
				rdm.LockDeq();
				if (true == CAS(head_, loc_head, first)) {
					rdm.Deq(first, first->thread_id);
					rdm.UnlockDeq();
					ebr.Retire(loc_head);
					return std::make_pair(value, false);
				}
				rdm.UnlockDeq();
				return std::make_pair(std::nullopt, (nullptr == first->next));
			}
		}

		auto GetHeadTimeStamp() const {
			return head_->time_stamp;
		}

		auto GetTail() const {
			return tail_;
		}
	private:
		bool CAS(Node* volatile& trg, Node* expected, Node* desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&trg),
				reinterpret_cast<uint64_t*>(&expected),
				reinterpret_cast<uint64_t>(desired));
		}

		Node* volatile tail_;
		Node* volatile head_;
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

		void Enq(Node* node, uint64_t lq_ts, benchmark::RelaxationDistanceManager& rdm) {
			auto tail_ts = tail_->time_stamp;
			node->time_stamp = (tail_ts <= lq_ts) ? (lq_ts + 1) : (tail_ts + 1);

			rdm.LockEnq();
			tail_->next = node;
			rdm.Enq(node, node->thread_id);
			rdm.UnlockEnq();
			tail_ = node;
		}

		std::pair<std::optional<int>, Node*> TryDeq(EBR<Node>& ebr, int depth,
			uint64_t lq_ts, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_head = head_;
				auto first = loc_head->next;
				if (nullptr == first) {
					return std::make_pair(std::nullopt, loc_head); // pq is empty
				}
				if (first->time_stamp > lq_ts + depth) {
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

	class TimeStampedLateralQueue {
	public:
		TimeStampedLateralQueue(int num_thread, int depth)
			: depth_{ depth }, queues_(num_thread), ebr_ { num_thread } {}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			ebr_.StartOp();
			auto node = new Node{ v };
			auto lq_tail = lateral_queue_.GetTail();
			auto lq_ts = lq_tail->time_stamp;
			auto& pq = queues_[MyThread::GetID()];


			if (pq.GetTailTimeStamp() < lq_ts + depth_) {
				pq.Enq(node, lq_ts, rdm_);
			}
			else if (lateral_queue_.TryEnq(node, lq_ts, depth_, rdm_) == false) {
				pq.Enq(node, lq_ts, rdm_);
			}

			ebr_.EndOp();
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_heads(queues_.size());
			size_t id = MyThread::GetID();
			ebr_.StartOp();
			while (true) {
				int cnt_empty{};
				auto lq_ts = lateral_queue_.GetHeadTimeStamp();
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& pq = queues_[id];
					auto [value, old_head] = pq.TryDeq(ebr_, depth_, lq_ts, rdm_);
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
				auto [value, is_lq_empty] = lateral_queue_.TryDeq(ebr_, depth_, lq_ts, rdm_);
				if (is_lq_empty) {
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
				}
				else if (value.has_value()) {
					ebr_.EndOp();
					return value;
				}
				else {
					id = MyThread::GetID();
				}
			}
		}
	private:
		int depth_;
		LateralQueue lateral_queue_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif