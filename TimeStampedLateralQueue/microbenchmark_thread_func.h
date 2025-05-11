#ifndef MICROBENCHMARK_THREAD_FUNC_H
#define MICROBENCHMARK_THREAD_FUNC_H

#include "random.h"
#include "my_thread.h"

namespace benchmark {
	inline const int kTotalNumOp{ (std::thread::hardware_concurrency() <= 8) ? 360'000 : 3600'0000 };

	void Idle()
	{
		for (volatile int i = 0; i < 30; ++i) {}
	}

	template<class QueueT>
	void MicrobenchmarkFunc(int thread_id, int num_thread, float enq_rate, 
		bool checks_relaxation_distance, QueueT& queue)
	{
		MyThread::SetID(thread_id);
		auto num_op = kTotalNumOp / num_thread;

		for (int i = 0; i < num_op; ++i) {
			auto op = Random::Get(0.0f, 100.0f);

			if (op <= enq_rate or i < num_op / 1000) {
				queue.Enq(Random::Get(0, 65535));
			} else {
				queue.Deq();
			}

			if (not checks_relaxation_distance) {
				Idle();
			}
		}
	}
}

#endif