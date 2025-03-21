#ifndef TIME_STAMPED_LATERAL_QUEUE
#define TIME_STAMPED_LATERAL_QUEUE

#include <utility>
#include <vector>
#include <chrono>
#include <limits>
#include <optional>
#include "random.h"
#include "ebr.h"
#include "relaxation_distance.h"

#include <format>

namespace lf::tsl {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{ nullptr };
		uint64_t retire_epoch{};
		uint64_t time_stamp{};
		int v{};
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
				if (true == CAS(loc_tail->next, nullptr, node)) {

					//benchmark::out << std::format("eL {} {}\n", (uint64_t)(node), node->time_stamp);

					rdm.Enq(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(node) + 1));
					CAS(tail_, loc_tail, node);
					return true;
				}
				return false;
			}
			CAS(tail_, loc_tail, next);
			return false;
		}

		std::pair<std::optional<int>, bool> TryDeq(EBR<Node>& ebr, uint64_t lq_ts, benchmark::RelaxationDistanceManager& rdm) {
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
				if (first->time_stamp > lq_ts) {
					return std::make_pair(std::nullopt, false); // retry required
				}

				auto value = first->v;
				rdm.LockDeq();
				if (true == CAS(head_, loc_head, first)) {

					//benchmark::out << std::format("dL {} {}\n", (uint64_t)(first), first->time_stamp);

					rdm.Deq(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(first) + 1));
					rdm.UnlockDeq();
					ebr.Retire(loc_head);
					return std::make_pair(value, false);
				}
				rdm.UnlockDeq();
				return std::make_pair(std::nullopt, (nullptr == first->next));
			}
		}

		auto GetHeadAndFirstTimeStamp() const {
			auto loc_head = head_;
			auto first = loc_head->next;
			auto first_time_stamp = (nullptr == first) ? std::numeric_limits<uint64_t>::max() : first->time_stamp;

			return std::make_pair(loc_head->time_stamp, first_time_stamp);
		}

		auto GetHeadTimeStamp() const {
			return head_->time_stamp;
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

			//benchmark::out << std::format("e{} {} {}\n", thread::ID(), (uint64_t)(node), node->time_stamp);

			tail_->next = node;
			rdm.Enq(node);
			tail_ = node;

		}

		std::pair<std::optional<int>, Node*> TryDeq(EBR<Node>& ebr, /*uint64_t lq_head_ts,*/
			uint64_t lq_first_ts, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_head = head_;
				auto first = loc_head->next;
				if (nullptr == first) {
					return std::make_pair(std::nullopt, loc_head); // pq is empty
				}
				if (first->time_stamp > lq_first_ts) {
					return std::make_pair(std::nullopt, nullptr); // retry required
				}
				auto value = first->v;
				rdm.LockDeq();
				if (true == CAS(head_, loc_head, first)) {

					//benchmark::out << std::format("d{} {} {}\n", thread::ID(), (uint64_t)(first), first->time_stamp);

					rdm.Deq(first);
					rdm.UnlockDeq();
					ebr.Retire(loc_head);
					return std::make_pair(value, nullptr); 
				}
				rdm.UnlockDeq();
			}
		}

		auto GetFirstTimeStamp() const {
			auto first = head_->next;
			return (nullptr == first) ? std::numeric_limits<uint64_t>::max() : first->time_stamp;
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
			rdm_.LockEnq();
			auto lq_ts = lateral_queue_.GetTailTimeStamp();
			auto& pq = queues_[thread::ID()];

			if (pq.GetTailTimeStamp() < lq_ts + depth_) {
				pq.Enq(node, lq_ts, rdm_);
			}
			else if (lateral_queue_.TryEnq(node, lq_ts, depth_, rdm_) == false) {
				pq.Enq(node, lq_ts, rdm_);
			}
			rdm_.UnlockEnq();
			ebr_.EndOp();
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_heads(queues_.size());
			size_t id = thread::ID();
			ebr_.StartOp();
			while (true) {
				int cnt_empty{};
				auto lq_ts = lateral_queue_.GetHeadTimeStamp() + depth_;
				//auto [lq_head_ts, lq_first_ts] = lateral_queue_.GetHeadAndFirstTimeStamp();

				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& pq = queues_[id];
					if (1) {
						//auto [value, old_head] = pq.TryDeq(ebr_, lq_head_ts, lq_first_ts, rdm_);
						auto [value, old_head] = pq.TryDeq(ebr_, lq_ts, rdm_);
						if (nullptr != old_head) {
							old_heads[id] = old_head;
							cnt_empty += 1;
						}
						else if (value.has_value()) {
							ebr_.EndOp();
							return value;
						}
					}
					id = (id + 1) % queues_.size();
				}
				//auto [value, is_lq_empty] = lateral_queue_.TryDeq(ebr_, lq_first_ts, rdm_);
				auto [value, is_lq_empty] = lateral_queue_.TryDeq(ebr_, lq_ts, rdm_);
				if (is_lq_empty) {
					if (queues_.size() == cnt_empty) {
						for (size_t i = 1; i < queues_.size(); ++i) {
							id = (i + thread::ID()) % queues_.size();
							if (nullptr != old_heads[id]->next) {
								break;
							}
							if (queues_.size() - 1 == i) {
								ebr_.EndOp();
								return std::nullopt;
							}
						}
					}
				}
				else if (value.has_value()) {
					ebr_.EndOp();
					return value;
				}
				else {
					id = thread::ID();
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