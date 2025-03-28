#ifndef TWODD_H
#define TWODD_H

#include <utility>
#include <optional>
#include <vector>
#include <chrono>
#include "ebr.h"
#include "random.h"
#include "relaxation_distance.h"

namespace lf::twodd {
	struct Node{
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{ nullptr };
		uint64_t retire_epoch{};
		uint64_t cnt{};
		int v{};
	};

	struct alignas(std::hardware_destructive_interference_size) PaddedPtr {
		Node* volatile ptr{};
	};

	struct alignas(std::hardware_destructive_interference_size) Window {
		Window() = default;
		Window(int depth) : max{ static_cast<uint64_t>(depth) } {}

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
			: depth_{ depth }, width_{ num_queue }, heads_(num_queue), tails_(num_queue)
			, ebr_{ num_thread }, window_get{ depth }, window_put{ depth } {
			for (int i = 0; i < num_queue; ++i) {
				tails_[i].ptr = new Node;
				heads_[i].ptr = tails_[i].ptr;
			}
		}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}
		
		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			bool has_contented{ false };
			ebr_.StartOp();
			auto node = new Node{ v };
			Node* tail;
			while (true) {
				tail = GetTail(has_contented);
				node->cnt = tail->cnt + 1;
				if (nullptr == tail->next) {
					rdm_.LockEnq();
					if (true == CAS(tail->next, nullptr, node)) {
						rdm_.Enq(node);
						rdm_.UnlockEnq();
						if (false == CAS(tails_[index_].ptr, tail, node)) {
							has_contented = true;
						}
						ebr_.EndOp();

						//std::cout << index_ << ' ' << window_put.max << '\n';

						return;
					}
					rdm_.UnlockEnq();
					has_contented = true;
				}
			}
		}

		std::optional<int> Deq() {
			bool has_contented{ false };
			ebr_.StartOp();
			while (true) {
				auto head = GetHead(has_contented);

				auto tail = tails_[index_].ptr;
				auto first = head->next;
				if (head == tail) {
					if (nullptr == first) {
						ebr_.EndOp();
						return std::nullopt;
					}
					else {
						if (false == CAS(tails_[index_].ptr, head, first)) {
							has_contented = true;
						}
					}
				}
				else {
					rdm_.LockDeq();
					if (true == CAS(heads_[index_].ptr, head, first)) {
						rdm_.Deq(first);
						rdm_.UnlockDeq();
						ebr_.Retire(head);
						ebr_.EndOp();
						return first->v;
					}
					rdm_.UnlockDeq();
					has_contented = true;
				}
			}
		}

	private:
		Node* GetHead(bool& has_contented) {
			bool is_empty{ true };
			uint64_t hops{}, random{}, put_cnt{};

			uint64_t loc_max_get{};
			max_get_ = window_get.max;

			if (has_contented) {
				index_ = rng.Get(0, width_ - 1);
				has_contented = false;
			}
			if (max_get_ != window_get.max) {
				max_get_ = window_get.max;
				is_empty = true;
			}

			while (true) {
				auto head = heads_[index_].ptr;
				put_cnt = tails_[index_].ptr->cnt;

				loc_max_get = window_get.max;
				if (loc_max_get != max_get_) {
					max_get_ = loc_max_get;
					hops = 0;
					is_empty = true;
				}
				else if (head->cnt < loc_max_get and put_cnt != head->cnt) {
					return head;
				}
				else if (hops != width_) {
					if (is_empty and (put_cnt != head->cnt)) {
						is_empty = false;
					}
					Hop(random, hops);
				}
				else if (not is_empty) {
					if (max_get_ == window_get.max) {
						window_get.CAS(max_get_, max_get_ + depth_);
					}

					max_get_ = window_get.max;
					hops = 0;
					is_empty = true;
				}
				else {
					return head;
				}
			}
		}

		Node* GetTail(bool& has_contented) {
			uint64_t hops{}, random{};
			uint64_t loc_max_put{};

			if (has_contented) {
				index_ = rng.Get(0, width_ - 1);
				has_contented = false;
			}

			if (max_put_ != window_put.max) {
				max_put_ = window_put.max;
			}

			while (true) {
				auto tail = tails_[index_].ptr;
				loc_max_put = window_put.max;
				if (loc_max_put != max_put_) {
					max_put_ = loc_max_put;
					hops = 0;
				}
				else if (tail->cnt < loc_max_put) {
					return tail;
				}
				else if (hops != width_) {
					Hop(random, hops);
				}
				else {
					if (max_put_ == window_put.max) {
						window_put.CAS(max_put_, max_put_ + depth_);
					}

					max_put_ = window_put.max;
					hops = 0;
				}
			}
		}

		void Hop(uint64_t& random, uint64_t& hops) const {
			if (random < 2) {
				random += 1;
				index_ = rng.Get(0, width_ - 1);
			}
			else {
				hops += 1;
				index_ = (index_ + 1) % width_;
			}
		}

		bool CAS(Node* volatile& trg, Node* expected, Node* desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&trg),
				reinterpret_cast<uint64_t*>(&expected),
				reinterpret_cast<uint64_t>(desired));
		}

		static thread_local int index_;
		static thread_local uint64_t max_get_;
		static thread_local uint64_t max_put_;
		int depth_, width_;
		std::vector<PaddedPtr> heads_;
		std::vector<PaddedPtr> tails_;
		Window window_get;
		Window window_put;
		EBR<Node> ebr_;
		benchmark::RelaxationDistanceManager rdm_;
	};

	thread_local int TwoDd::index_{};
	thread_local uint64_t TwoDd::max_get_{};
	thread_local uint64_t TwoDd::max_put_{};
}

#endif