#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <chrono>

class Stopwatch {
public:
	Stopwatch() noexcept : time_point_{ Clock::now() } {}
	void Start() noexcept {
		time_point_ = Clock::now();
	}
	double GetDuration() const noexcept {
		auto duration = Clock::now() - time_point_;
		return std::chrono::duration_cast<Resolution>(duration).count() / static_cast<double>(Resolution::period::den);
	}
private:
	using Clock = std::chrono::high_resolution_clock;
	using Resolution = std::chrono::nanoseconds;
	Clock::time_point time_point_;
};

#endif