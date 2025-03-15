#ifndef DQRR_H
#define DQRR_H

#include <utility>
#include <optional>
#include "queue_node.h"
#include "ebr.h"

namespace lf::dqrr {
	using Node = QueueNode;

	class PartialQueue {
	public:
		void Enq(int v) {
			auto node = new Node{ v };

			while (true) {
				auto loc_tail = tail_;
				auto next = loc_tail->next;

				if (loc_tail != tail_) {
					continue;
				}

				if (nullptr == next) {
					node->SetEnqTime();
					if (true == CAS(loc_tail->next, nullptr, node)) {
						CAS(tail_, loc_tail, node);
						return;
					}
				}
				else {
					CAS(tail_, loc_tail, next);
				}
			}
		}

		std::pair<int, Node*> Deq(EBR<Node>& ebr) {
			while (true) {
				auto loc_head = head_;
				auto loc_tail = tail_;
				auto first = loc_head->next;
				if (loc_head != head_) {
					continue;
				}
				if (nullptr == first) {
					ebr.EndOp();
					return std::make_pair(0, loc_tail);
				}
				if (loc_head == loc_tail) {
					CAS(tail_, loc_tail, first);
					continue;
				}
				auto value = first->v;
				if (false == CAS(head_, loc_head, first)) {
					continue;
				}
				ebr.Retire(loc_head);
				return std::make_pair(value, nullptr);
			}
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

		Node* volatile head_;
		Node* volatile tail_;
	};

	class DQRR {
	public:
		DQRR(int num_queue, int num_thread, int b)
			: num_thread_{ num_thread }, b_{ b }, queues_(num_queue)
			, enq_rrs_(b), deq_rrs_(b), ebr_{ num_thread } {}

		void Enq(int v) {
			ebr_.StartOp();
			queues_[GetEnqueuerIndex()].Enq(v);
			ebr_.EndOp();
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_tails(queues_.size());
			ebr_.StartOp();

			auto start = GetDequeuerIndex();
			while (true) {
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto id = (start + i) % queues_.size();
					auto [value, old_tail] = queues_[id].Deq(ebr_);

					if (nullptr == old_tail) {
						ebr_.EndOp();
						return value;
					}

					old_tails[id] = old_tail;
				}

				for (size_t i = 0; i < old_tails.size(); ++i) {
					if (old_tails[i] != queues_[i].GetTail()) {
						start = i;
						break;
					}
					if (i == old_tails.size() - 1) {
						ebr_.EndOp();
						return std::nullopt;
					}
				}
			}
		}

	private:
		uint64_t GetEnqueuerIndex() {
			int rr_id = thread::ID() % b_;
			auto enq_rr = enq_rrs_[rr_id].fetch_add(1);
			return enq_rr % queues_.size();
		}

		uint64_t GetDequeuerIndex() {
			int rr_id = thread::ID() % b_;
			auto deq_rr = deq_rrs_[rr_id].fetch_add(1);
			return deq_rr % queues_.size();
		}

		int num_thread_, b_;
		std::vector<PartialQueue> queues_;
		std::vector<std::atomic<uint64_t>> enq_rrs_;
		std::vector<std::atomic<uint64_t>> deq_rrs_;
		EBR<Node> ebr_;
	};
}

#endif