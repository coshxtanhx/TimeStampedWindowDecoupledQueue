#ifndef DQRR_H
#define DQRR_H

#include <utility>
#include <optional>
#include <vector>
#include <chrono>
#include <limits>
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::dqrr {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{};
		uint64_t retire_epoch{};
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

		void Enq(int v, benchmark::RelaxationDistanceManager& rdm) {
			auto node = new Node{ v };

			while (true) {
				auto loc_tail = tail_;
				auto next = loc_tail->next;

				if (loc_tail != tail_) {
					continue;
				}

				if (nullptr == next) {
					rdm.LockEnq();
					if (true == CAS(loc_tail->next, nullptr, node)) {
						rdm.Enq(node);
						rdm.UnlockEnq();
						CAS(tail_, loc_tail, node);
						return;
					}
					rdm.UnlockEnq();
				} else {
					CAS(tail_, loc_tail, next);
				}
			}
		}

		std::pair<int, Node*> TryDeq(EBR<Node>& ebr, benchmark::RelaxationDistanceManager& rdm) {
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
				rdm.LockDeq();
				if (false == CAS(head_, loc_head, first)) {
					rdm.UnlockDeq();
					continue;
				}
				rdm.Deq(first);
				rdm.UnlockDeq();
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

		alignas(std::hardware_destructive_interference_size) Node* volatile tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	struct alignas(std::hardware_destructive_interference_size) RRCounter {
		uint64_t value;
	};

	class DQRR {
	public:
		DQRR(int num_queue, int num_thread, int b)
			: b_{ b }, queues_(num_queue), enq_rrs_(b), deq_rrs_(b), ebr_{ num_thread } {
			for (uint64_t i = 0; i < b; ++i) {
				enq_rrs_[i].value = i * num_queue / b;
				deq_rrs_[i].value = i * num_queue / b;
			}
		}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			ebr_.StartOp();
			queues_[GetEnqueuerIndex()].Enq(v, rdm_);
			ebr_.EndOp();
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_tails(queues_.size());
			ebr_.StartOp();

			auto start = GetDequeuerIndex();
			while (true) {
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto id = (start + i) % queues_.size();
					auto [value, old_tail] = queues_[id].TryDeq(ebr_, rdm_);

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
		size_t GetEnqueuerIndex() {
			auto enq_rr = enq_rrs_[MyThread::GetID() % b_].value++;
			return enq_rr % queues_.size();
		}

		size_t GetDequeuerIndex() {
			auto deq_rr = deq_rrs_[MyThread::GetID() % b_].value++;
			return deq_rr % queues_.size();
		}

		int b_;
		std::vector<PartialQueue> queues_;
		std::vector<RRCounter> enq_rrs_;
		std::vector<RRCounter> deq_rrs_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif