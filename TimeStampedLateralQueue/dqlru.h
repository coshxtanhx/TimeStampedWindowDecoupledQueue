#ifndef DQLRU_H
#define DQLRU_H

#include <utility>
#include <vector>
#include <chrono>
#include <limits>
#include <optional>
#include "random.h"
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::dqlru {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{ nullptr };
		uint64_t retire_epoch{};
		uint64_t stamp{};
		int v{};
		int thread_id{ MyThread::GetID() };
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

		bool TryEnq(Node* node, Node* expected_tail, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto next = expected_tail->next;

				if (tail_ != expected_tail) {
					return false;
				}

				if (nullptr == next) {
					rdm.LockEnq();
					if (true == CAS(expected_tail->next, nullptr, node)) {
						rdm.Enq(node, node->thread_id);
						rdm.UnlockEnq();
						CAS(tail_, expected_tail, node);
						return true;
					}
					rdm.UnlockEnq();
					return false;
				}
				else {
					CAS(tail_, expected_tail, next);
					return false;
				}
			}
		}

		std::pair<int, Node*> TryDeq(EBR<Node>& ebr, Node* expected_head, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_tail = tail_;
				auto first = expected_head->next;

				if (nullptr == first) {
					ebr.EndOp();
					return std::make_pair(0, loc_tail);
				}
				if (expected_head == loc_tail) {
					CAS(tail_, loc_tail, first);
					continue;
				}
				auto value = first->v;
				rdm.LockDeq();
				if (false == CAS(head_, expected_head, first)) {
					rdm.UnlockDeq();
					return std::make_pair(0, expected_head);
				}
				rdm.Deq(first, first->thread_id);
				rdm.UnlockDeq();
				ebr.Retire(expected_head);
				return std::make_pair(value, nullptr);
			}
		}

		auto GetHead() const {
			return head_;
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

		alignas(std::hardware_destructive_interference_size) Node* volatile tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	class DQLRU {
	public:
		DQLRU(int num_queue, int num_thread) : queues_(num_queue), ebr_{ num_thread } {}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			auto node = new Node{ v };
			auto start = rng.Get(0, queues_.size() - 1);
			ebr_.StartOp();
			while (true) {
				auto [id, stamp] = GetLowestTail(start);
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& queue = queues_[id];
					auto tail = queue.GetTail();
					node->stamp = stamp + 1;
					if (tail->stamp == stamp and true == queue.TryEnq(node, tail, rdm_)) {
						ebr_.EndOp();
						return;
					}
					id = (id + 1) % queues_.size();
				}
			}
		}

		std::optional<int> Deq() {
			auto start = rng.Get(0, queues_.size() - 1);
			std::vector<Node*> old_tails(queues_.size());

			while (true) {
				auto [id, stamp] = GetLowestHead(start);
				int cnt_empty{};

				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& queue = queues_[id];
					auto head = queue.GetHead();
					if (head->stamp == stamp) {
						auto [value, old_tail] = queue.TryDeq(ebr_, head, rdm_);

						if (nullptr != old_tail) {
							if (queue.GetTail() == old_tail) {
								old_tails[id] = old_tail;
								cnt_empty += 1;
							}
						}

						ebr_.EndOp();
						return value;
					}
					id = (id + 1) % queues_.size();
				}

				if (cnt_empty == queues_.size()) {
					for (size_t i = 0; i < old_tails.size(); ++i) {
						if (old_tails[i] != queues_[i].GetTail()) {
							start = static_cast<int>(i);
							break;
						}
						if (i == old_tails.size() - 1) {
							ebr_.EndOp();
							return std::nullopt;
						}
					}
				}
			}
		}
	private:
		std::pair<size_t, uint64_t> GetLowestTail(int start) const {
			auto tail_stamp = queues_[start].GetTail()->stamp;

			for (size_t i = 1; i < queues_.size(); ++i) {
				auto other_id = (start + i) % queues_.size();
				auto other_tail_stamp = queues_[other_id].GetTail()->stamp;
				if (tail_stamp > other_tail_stamp) {
					return std::make_pair(other_id, other_tail_stamp);
				}
				else if (tail_stamp < other_tail_stamp) {
					return std::make_pair(start, tail_stamp);
				}
			}

			return std::make_pair(start, tail_stamp);
		}

		std::pair<size_t, uint64_t> GetLowestHead(int start) const {
			auto head_stamp = queues_[start].GetHead()->stamp;

			for (int i = 1; i < queues_.size(); ++i) {
				auto other_id = (start + i) % queues_.size();
				auto other_head_stamp = queues_[other_id].GetHead()->stamp;

				if (head_stamp > other_head_stamp) {
					return std::make_pair(other_id, other_head_stamp);
				}
				else if (head_stamp < other_head_stamp) {
					return std::make_pair(start, head_stamp);
				}
			}

			return std::make_pair(start, head_stamp);
		}

		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif