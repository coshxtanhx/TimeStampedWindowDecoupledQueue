#ifndef MACROBENCHMARK_THREAD_FUNC_H
#define MACROBENCHMARK_THREAD_FUNC_H

#include "benchmark_setting.h"
#include "graph.h"

namespace benchmark {
	template<class QueueT>
	void MacrobenchmarkFunc(MicrobenchmarkSetting setting, int thread_id, int num_thread,
		QueueT& queue, Graph& graph, int& result)
	{
		thread::ID(thread_id);
		if (0 == thread_id) {
			queue.Enq(0);
		}

		result = graph.BFS(num_thread, queue);
	}
}

#endif