#ifndef QUEUE_NODE_H
#define QUEUE_NODE_H

#include <chrono>

namespace lf {
	struct QueueNode {
		QueueNode(int v) : v{ v } {}
		void SetEnqTime() {
			enq_time = std::chrono::steady_clock::now();
		}

		QueueNode* volatile next{};
		uint64_t retire_epoch{};
		std::chrono::steady_clock::time_point enq_time{};
		int v{};
	};
}

#endif