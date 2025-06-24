#ifndef MACROBENCHMARK_THREAD_FUNC_H
#define MACROBENCHMARK_THREAD_FUNC_H

#include "graph.h"

namespace benchmark {
	template<class QueueT>
	void MacrobenchmarkFunc(int thread_id, int num_thread, QueueT& queue, Graph& graph, int& shortest_dist)
	{
		MyThreadID::Set(thread_id);
		if (0 == thread_id) {
			queue.Enq(0);
		}

		shortest_dist = graph.RelaxedBFS(num_thread, queue);
	}
}

#endif