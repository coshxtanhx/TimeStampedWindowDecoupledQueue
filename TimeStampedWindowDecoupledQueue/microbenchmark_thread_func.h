#ifndef MICROBENCHMARK_THREAD_FUNC_H
#define MICROBENCHMARK_THREAD_FUNC_H

#include "random.h"
#include "my_thread_id.h"

namespace benchmark {
	inline const int kTotalNumOp{ (std::thread::hardware_concurrency() <= 8) ? 360'000 : 3600'0000 };
	inline constexpr int kNumPrefill{ 100'000 };

	template<class QueueT>
	void MicrobenchmarkFunc(int thread_id, int num_thread, float enq_rate, QueueT& queue)
	{
		MyThreadID::Set(thread_id);
		auto num_op = kTotalNumOp / num_thread;

		for (int i = 0; i < num_op; ++i) {
			auto op = Random::Get(0.0f, 100.0f);

			if (op <= enq_rate) {
				queue.Enq(Random::Get(0, 65535));
			} else {
				auto p = queue.Deq();
			}
		}
	}

	template<class QueueT>
	void Prefill(int thread_id, int num_thread, QueueT& queue)
	{
		MyThreadID::Set(thread_id);
		auto num_op = kNumPrefill / num_thread;

		for (int i = 0; i < num_op; ++i) {
			queue.Enq(Random::Get(0, 65535));
		}
	}
}

#endif