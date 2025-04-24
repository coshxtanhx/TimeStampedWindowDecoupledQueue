#ifndef BENCHMARK_TESTER_H
#define BENCHMARK_TESTER_H

#include <vector>
#include <thread>
#include <memory>
#include <map>
#include "graph.h"
#include "print.h"

namespace benchmark {
	enum class Subject : uint8_t {
		kNone, kLRU, kRR, kCBO, kTSInterval, k2Dd, kTSWD
	};

	class Tester {
	public:
		Tester() noexcept = default;

		void Run();
		void StartMicroBenchmark();
		void StartMacroBenchmark();
	private:
		template<class Subject>
		using MicrobenchmarkFuncT = void(*)(int, int, int, Subject&);

		template<class Subject>
		using MacrobenchmarkFuncT = void(*)(int, int, Subject&, Graph&, int&);

		void SetSubject();
		void SetParameter();
		void SetEnqRate();
		void CheckRelaxationDistance();
		void GenerateGraph();
		void LoadGraph();

		template<class Subject>
		void RunMicrobenchmark(MicrobenchmarkFuncT<Subject> thread_func, int num_thread, Subject& subject) {
			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				if (checks_relaxation_distance_) {
					subject.CheckRelaxationDistance();
				}
				threads_.emplace_back(thread_func, thread_id, num_thread, enq_rate_, std::ref(subject));
			}

			for (auto& t : threads_) {
				t.join();
			}
		}

		template<class Subject>
		void RunMacrobenchmark(MacrobenchmarkFuncT<Subject> thread_func,
			int num_thread, Subject& subject, std::vector<int>& shortest_dists) {
			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads_.emplace_back(thread_func, thread_id, num_thread,
					std::ref(subject), std::ref(*graph_), std::ref(shortest_dists[thread_id]));
			}

			for (auto& t : threads_) {
				t.join();
			}
		}

		std::vector<std::thread> threads_;
		std::unique_ptr<Graph> graph_{};
		int parameter_{};
		Subject subject_{};
		bool checks_relaxation_distance_{};
		int enq_rate_{ 50 };
	};
}

#endif