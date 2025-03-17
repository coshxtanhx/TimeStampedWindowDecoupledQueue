#ifndef DQRA_H
#define DQRA_H

#include <utility>
#include <optional>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>
#include "random.h"
#include "ebr.h"

namespace lf::dqra {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		void SetEnqTime() {
			enq_time = std::chrono::steady_clock::now();
		}

		Node* volatile next{};
		uint64_t retire_epoch{};
		std::chrono::steady_clock::time_point enq_time{};
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

		void Enq(int v) {
			auto node = new Node{ v };

			while (true) {
				auto loc_tail = tail_;
				auto next = loc_tail->next;

				if (loc_tail != tail_) {
					continue;
				}

				if (nullptr == next) {
					node->stamp = loc_tail->stamp + 1;
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

		std::pair<int, Node*> TryDeq(EBR<Node>& ebr) {
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

		auto GetSize() const {
			return tail_->stamp - head_->stamp;
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

	class DQRA {
	public:
		DQRA(int num_queue, int num_thread, int d)
			: num_thread_{ num_thread }, d_{ d }, indices_(num_thread)
			, queues_(num_queue), ebr_{ num_thread } {
			for (auto& indices : indices_) {
				indices.resize(num_queue);
				std::iota(indices.begin(), indices.end(), 0);
			}
		}

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
					auto [value, old_tail] = queues_[id].TryDeq(ebr_);

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
			ShuffleIndex();
			auto& indices = indices_[thread::ID()];
			return *std::min_element(indices.begin(), indices.begin() + d_, [this](size_t a, size_t b) {
				return queues_[a].GetSize() < queues_[b].GetSize();
				});
		}

		size_t GetDequeuerIndex() {
			ShuffleIndex();
			auto& indices = indices_[thread::ID()];
			return *std::max_element(indices.begin(), indices.begin() + d_, [this](size_t a, size_t b) {
				return queues_[a].GetSize() < queues_[b].GetSize();
				});
		}

		void ShuffleIndex() {
			auto& indices = indices_[thread::ID()];
			for (int i = 0; i < d_; ++i) {
				auto r = rng.Get(i, indices.size() - 1);
				std::swap(indices[i], indices[r]);
			}
		}

		int num_thread_;
		int d_;
		std::vector<std::vector<size_t>> indices_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
	};
}

#endif