#pragma once
#include <tbb/concurrent_queue.h>
#include <optional>
#include "relaxation_distance.h"

struct TBBQueue {
	void CheckRelaxationDistance() {
		// Does not support
	}

	auto GetRelaxationDistance() {
		return rdm.GetRelaxationDistance();
	}

	void Enq(int v) {
		queue.push(v);
	}

	std::optional<int> Deq() {
		int r;
		if (queue.try_pop(r)) {
			return r;
		}
		return std::nullopt;
	}

	tbb::concurrent_queue<int> queue;
	benchmark::RelaxationDistanceManager rdm;
};