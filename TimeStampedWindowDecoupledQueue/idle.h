#ifndef IDLE_H
#define IDLE_H

#include "stopwatch.h"

class Idle {
public:
	Idle() {
		GetDelayOpPerMicrosec();
	}

	int32_t GetDelayOpPerMicrosec() {
		if (kUndefinedDelay != delay_per_microsec_) [[likely]] {
			return delay_per_microsec_;
		}

		Stopwatch sw;
		sw.Start();
		constexpr auto kLoop = 1'000'000'000;
		for (volatile int32_t i = 0; i < kLoop; ++i) {}
		auto us = sw.GetDuration() * 1.0e6;

		delay_per_microsec_ = static_cast<int32_t>(kLoop / us);

		return delay_per_microsec_;
	}

	void Do(float microsec) const {
		if (0.0f == microsec) {
			return;
		}

		for (volatile int32_t i = 0; i < static_cast<int32_t>(delay_per_microsec_ * microsec); ++i) {}
	}
private:
	static constexpr int32_t kUndefinedDelay{ -1 };
	int32_t delay_per_microsec_{ kUndefinedDelay };
};

inline Idle idle;

#endif