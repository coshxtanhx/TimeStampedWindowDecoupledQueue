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
		uint64_t stamp{ std::numeric_limits<uint64_t>::max() };
		int v{};
	};

	struct Descriptor {
		int item{};
		uint64_t cnt_get{};
		uint64_t cnt_put{};
	};

	inline void PutWindow(size_t& index) {}
	inline void GetWindow(size_t& index) {}

	struct Window {
		Window(bool puts, size_t& index, bool& has_contented, size_t queue_size) {
			if (has_contented) {
				index = rng.Get(size_t{}, queue_size - 1);
				has_contented = false;
			}
			puts ? PutWindow(index) : GetWindow(index);
		}

		uint64_t max{};
	};

	class TwoDd {
	public:
	private:
	};
}

#endif