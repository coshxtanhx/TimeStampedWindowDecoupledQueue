#ifndef TWODD_H
#define TWODD_H

#include <utility>
#include <optional>
#include <vector>
#include <chrono>
#include "ebr.h"
#include "random.h"

namespace lf::twodd {
	struct Node{
		Node() = default;
		Node(int v) : v{ v } {}

		void SetEnqTime() {
			enq_time = std::chrono::steady_clock::now();
		}

		Node* volatile next{ nullptr };
		uint64_t retire_epoch{};
		std::chrono::steady_clock::time_point enq_time{};
		uint64_t cnt{};
		int v{};
	};

	struct alignas(std::hardware_destructive_interference_size) PaddedPtr {
		Node* volatile ptr{};
	};

	inline thread_local bool has_contented;
	inline thread_local int index;
	inline thread_local bool is_empty{ true };

	struct alignas(std::hardware_destructive_interference_size) Window {
		Window() = default;
		bool CAS(uint64_t expected, uint64_t desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&max),
				&expected, desired);
		}

		volatile uint64_t max{};
	};

	class TwoDd {
	public:
		TwoDd(int num_queue, int num_thread, int depth)
			: depth_{ depth }, width_{ num_queue }, heads_(num_queue), tails_(num_queue), ebr_{ num_thread } {
			for (int i = 0; i < num_queue; ++i) {
				tails_[i].ptr = new Node;
				heads_[i].ptr = tails_[i].ptr;
			}
		}

		void Enq(int v) {
			has_contented = false;
			ebr_.StartOp();
			auto node = new Node{ v };
			Node* tail;
			while (true) {
				tail = GetTail();
				node->cnt = tail->cnt + 1;
				if (nullptr == tail->next) {
					node->SetEnqTime();
					if (CAS(tail->next, nullptr, node)) {
						break;
					}
					has_contented = true;
				}
			}
			if (false == CAS(tails_[index].ptr, tail, node)) {
				has_contented = true;
			}
			ebr_.EndOp();
		}

		std::optional<int> Deq() {
			has_contented = false;
			ebr_.StartOp();
			while (true) {
				auto head = GetHead();
				auto tail = tails_[index].ptr;
				auto first = head->next;
				if (head == tail) {
					if (nullptr == first) {
						ebr_.EndOp();
						return std::nullopt;
					}
					else {
						if (false == CAS(tails_[index].ptr, head, first)) {
							has_contented = true;
						}
					}
				}
				else {
					if (true == CAS(heads_[index].ptr, head, first)) {
						ebr_.Retire(head);
						ebr_.EndOp();
						return first->v;
					}
					has_contented = true;
				}
			}
		}

	private:
		Node* GetHead() {
			is_empty = true;
			int random{};
			int index_search{};
			auto loc_get_max = window_get.max;

			while (true) {
				if (index_search == width_) {
					if (loc_get_max == window_get.max) {
						window_get.CAS(loc_get_max, loc_get_max + depth_);
					}
					loc_get_max = window_get.max;
					is_empty = true;
					index_search = 0;
				}
				auto head = heads_[index].ptr;
				if (head != nullptr and head->cnt < window_get.max) {
					return head;
				}
				else if (loc_get_max == window_get.max) {
					if (head != nullptr) {
						is_empty = false;
					}
					// HOP
					if (random < 2) {
						index = rng.Get(0, width_ - 1);
						random += 1;
					}
					else {
						index_search += 1;
						if (index_search == width_ and is_empty == true) {
							return head;
						}
						index = (index + 1) % width_;
					}
				}
				else {
					loc_get_max = window_get.max;
					index_search = 0;
					is_empty = true;
				}
			}
		}

		Node* GetTail() {
			int random{};
			int index_search{};
			auto loc_put_max = window_put.max;

			while (true) {
				if (index_search == width_) {
					if (loc_put_max == window_put.max) {
						window_put.CAS(loc_put_max, loc_put_max + depth_);
					}
					loc_put_max = window_put.max;
					index_search = 0;
				}
				auto tail = tails_[index].ptr;
				if (tail->cnt < window_put.max) {
					return tail;
				}
				else if (loc_put_max == window_put.max) {
					// HOP
					if (random < 2) {
						index = rng.Get(0, width_ - 1);
						random += 1;
					}
					else {
						index_search += 1;
						if (index_search == width_ and is_empty == true) {
							return tail;
						}
						index = (index + 1) % width_;
					}
				}
				else {
					loc_put_max = window_put.max;
					index_search = 0;
				}
			}
		}

		bool CAS(Node* volatile& trg, Node* expected, Node* desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&trg),
				reinterpret_cast<uint64_t*>(&expected),
				reinterpret_cast<uint64_t>(desired));
		}

		int depth_, width_;
		std::vector<PaddedPtr> heads_;
		std::vector<PaddedPtr> tails_;
		Window window_get;
		Window window_put;
		EBR<Node> ebr_;
	};
}

#endif