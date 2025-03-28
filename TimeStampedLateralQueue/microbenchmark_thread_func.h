#ifndef MICROBENCHMARK_THREAD_FUNC_H
#define MICROBENCHMARK_THREAD_FUNC_H

#include "random.h"
#include "thread.h"

namespace benchmark {
	double ComputePi(int n)
	{
		double pi{};
		int sign = +1;
		for (int i = 0; i < n; ++i) {
			pi += sign * 1.0 / (i * 2 + 1) * 4;
			sign *= -1;
		}
		return pi;
	}

	void Idle(int contention)
	{
		volatile double pi;
		for (int i = 0; i < contention; ++i) {
			pi = ComputePi(560);
		}
	}

	inline constexpr int kTotalNumOp{ 3600'0000 };

	template<class QueueT>
	void MicrobenchmarkFunc(int thread_id, int num_thread, int contention, QueueT& queue)
	{
		thread::ID(thread_id);
		auto num_op = kTotalNumOp / num_thread;

		for (int i = 0; i < num_op; ++i) {
			auto op = rng.Get(0, 1);

			if (op == 0 or i < num_op / 1000) {
				queue.Enq(rng.Get(0, 65535));
			} 
			else {
				auto v = queue.Deq();
			}

			Idle(contention);
		}
	}
}

#endif