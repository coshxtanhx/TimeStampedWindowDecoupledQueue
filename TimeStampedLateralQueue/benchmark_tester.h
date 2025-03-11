#ifndef BENCHMARK_TESTER_H
#define BENCHMARK_TESTER_H

#include <vector>
#include <thread>
#include <stdio.h>
#include "stopwatch.h"
#include "benchmark_setting.h"

namespace benchmark {
	class Tester;
	template<class T>
	using ThreadFunc = void(*)(Tester*, int, int, T*);

	class Tester{
	public:
		Tester() noexcept = default;

		void StartMicroBenchmark() {
			const auto kMaxThread{ static_cast<int>(std::thread::hardware_concurrency()) };
			threads_.reserve(kMaxThread);

			Stopwatch stopwatch;

			for (int num_thread = kMaxThread / 8; num_thread <= kMaxThread; num_thread *= 2){
				if (num_thread == 0) {
					num_thread = 1;
				}
				
				threads_.clear();

				stopwatch.Start();

				switch (microbenchmark_setting_.subject)
				{
				case Subject::kLRU:
					//AddThread(tf, num_thread, subject);
					break;
				case Subject::kRR:
					break;
				case Subject::kRA:
					break;
				case Subject::kTSCAS:
					break;
				case Subject::kTSInterval:
					break;
				case Subject::k2Dd:
					break;
				case Subject::kTSL:
					break;
				default:
					break;
				}

				//std::this_thread::sleep_for(std::chrono::seconds(1));

				auto elapsed_sec = stopwatch.GetDuration();
				auto throughput = microbenchmark_setting_.num_op / elapsed_sec;

				std::printf("   threads: %d\n", num_thread);
				std::printf("throughput: %.2lf MOp/s\n", throughput / 1e6);
				std::printf("\n");
			}
		}
	private:
		template<class SubjectT>
		void AddThread(ThreadFunc<SubjectT> thread_func, int num_thread) {
			auto subject = new Subject{};
			
			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads_.emplace_back(thread_func, this, thread_id, num_thread, subject);
			}

			for (auto& t : threads_) {
				t.join();
			}
		}

		std::vector<std::thread> threads_;
		benchmark::MicrobenchmarkSetting microbenchmark_setting_;
	};
}

#endif