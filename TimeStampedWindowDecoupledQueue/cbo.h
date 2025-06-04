#ifndef CBO_H
#define CBO_H

#include <utility>
#include <optional>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include "random.h"
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::cbo {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{};
		uint64_t retire_epoch{};
		uint64_t stamp{};
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
					node->stamp = loc_tail->stamp + 1;
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

		std::optional<int> TryDeq(EBR<Node>& ebr, benchmark::RelaxationDistanceManager& rdm) {
			while (true) {
				auto loc_head = head_;
				auto loc_tail = tail_;
				auto first = loc_head->next;
				if (loc_head != head_) {
					continue;
				}
				if (nullptr == first) {
					return std::nullopt;
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
				return value;
			}
		}

		auto GetTail() const {
			return tail_;
		}

		auto GetHead() const {
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

	class CBO {
	public:
		CBO(int num_queue, int num_thread, int d) : d_{ d }, indices_(num_thread)
			, queues_(num_queue), ebr_{ num_thread } {
			for (auto& indices : indices_) {
				indices.resize(num_queue);
				std::iota(indices.begin(), indices.end(), 0);
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

			auto optimal = GetDequeuerIndex();
			auto value = queues_[optimal].TryDeq(ebr_, rdm_);

			if (not value.has_value()) {
				value = DoubleCollect(optimal);
			}
			ebr_.EndOp();
			return value;
		}

	private:
		size_t GetEnqueuerIndex() {
			ShuffleIndex();
			auto& indices = indices_[MyThread::GetID()];
			
			return *std::min_element(indices.begin(), indices.begin() + d_, [this](size_t a, size_t b) {
				return queues_[a].GetTail()->stamp < queues_[b].GetTail()->stamp;
				});
		}

		size_t GetDequeuerIndex() {
			ShuffleIndex();
			auto& indices = indices_[MyThread::GetID()];

			return *std::min_element(indices.begin(), indices.begin() + d_, [this](size_t a, size_t b) {
				return queues_[a].GetHead()->stamp < queues_[b].GetHead()->stamp;
				});
		}

		std::optional<int> DoubleCollect(size_t start) {
			std::vector<Node*> versions(queues_.size());
			while (true) {
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto id = (start + i) % queues_.size();
					versions[i] = queues_[i].GetTail();
					auto value = queues_[i].TryDeq(ebr_, rdm_);
					if (value.has_value()) {
						return value;
					}
				}

				bool is_empty{ true };
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto id = (start + i) % queues_.size();
					if (versions[id] != queues_[id].GetTail()) {
						is_empty = false;
						start = id;
						break;
					}
				}

				if (is_empty) {
					return std::nullopt;
				}
			}
		}

		void ShuffleIndex() {
			auto& indices = indices_[MyThread::GetID()];
			for (int i = 0; i < d_; ++i) {
				auto r = Random::Get(i, indices.size() - 1);
				std::swap(indices[i], indices[r]);
			}
		}

		int d_;
		std::vector<std::vector<size_t>> indices_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif