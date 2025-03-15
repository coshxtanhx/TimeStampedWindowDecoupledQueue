#ifndef MICROBENCHMARK_THREAD_FUNC_H
#define MICROBENCHMARK_THREAD_FUNC_H

#include "benchmark_setting.h"
#include "random.h"

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

	template<class QueueT>
	void MicrobenchmarkFunc(MicrobenchmarkSetting setting, int thread_id, int num_thread, QueueT* queue)
	{
		auto num_op = setting.num_op / num_thread;

		for (int i = 0; i < num_op; ++i) {
			auto v = rng.Get(0, 65535);
			auto op = rng.Get(0, 1);
			//auto op = 0;

			if (op == 0 or i < num_op / 10000) {
				//queue->Enq(v);
			}
			else {
				//v = queue->Deq();
			}

			Idle(setting.contention);
		}
	}
}

#endif