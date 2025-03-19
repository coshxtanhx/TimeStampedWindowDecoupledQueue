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
		uint64_t stamp{};
		int v{};
	};

	class alignas(std::hardware_destructive_interference_size) PartialQueue {
	public:
		PartialQueue() : tail_{ new Node }, head_{ tail_ } {}

	private:
		Node* volatile head_;
		Node* volatile tail_;
	};

	inline thread_local bool has_contented;

	class TwoDd {
	public:
		TwoDd(int num_queue, int num_thread, int depth)
			: depth_{ depth }, queues_(num_queue), ebr_{ num_thread } {}

		void Enq(int v) {
			has_contented = false;
			while (true) {

			}
		}

		std::optional<int> Deq() {

		}

	private:
		int depth_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
	};
}

#endif