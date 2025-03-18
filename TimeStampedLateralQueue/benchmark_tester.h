#ifndef BENCHMARK_TESTER_H
#define BENCHMARK_TESTER_H

#include <vector>
#include <thread>
#include "stopwatch.h"
#include "benchmark_setting.h"
#include "microbenchmark_thread_func.h"
#include "dqrr.h"
#include "dqra.h"
#include "dqlru.h"
#include "ts_interval.h"
#include "time_stamped_lateral_queue.h"

namespace benchmark {
	template<class BenchmarkSetting, class Subject>
	using ThreadFunc = void(*)(BenchmarkSetting, int, int, Subject*);

	class Tester{
	public:
		Tester() noexcept = default;

		void StartMicroBenchmark() {
			const auto kMaxThread{ static_cast<int>(std::thread::hardware_concurrency()) };
			threads_.reserve(kMaxThread);

			Stopwatch stopwatch;

			for (int num_thread = kMaxThread / 8; num_thread <= kMaxThread; num_thread *= 2){
				if (num_thread < 4) {
					num_thread = kMaxThread / 2;
				}
				
				threads_.clear();

				stopwatch.Start();

				switch (microbenchmark_setting_.subject) {
				case Subject::kLRU: {
					auto subject = new lf::dqlru::DQLRU{ num_thread * parameter_ / 9, num_thread };
					AddThread(MicrobenchmarkFunc, num_thread, subject);
					delete subject;
					break;
				}
				case Subject::kRR: {
					auto subject = new lf::dqrr::DQRR{ num_thread * parameter_ / 9, num_thread, num_thread };
					AddThread(MicrobenchmarkFunc, num_thread, subject);
					delete subject;
					break;
				}
				case Subject::kRA: {
					auto subject = new lf::dqra::DQRA{ num_thread, num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, subject);
					delete subject;
					break;
				}
				case Subject::kTSInterval: {
					auto subject = new lf::ts::TSInterval{ num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, subject);
					delete subject;
					break;
				}
				case Subject::k2Dd: {
					break;
				}
				case Subject::kTSL: {
					auto subject = new lf::tsl::TimeStampedLateralQueue{ num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, subject);
					delete subject;
					break;
				}
				default:
					break;
				}

				//std::this_thread::sleep_for(std::chrono::seconds(1));

				auto elapsed_sec = stopwatch.GetDuration();
				auto throughput = microbenchmark_setting_.num_op / elapsed_sec;

				printf("   threads: %d\n", num_thread);
				printf("elapsedsec: %.2lf s\n", elapsed_sec);
				printf("throughput: %.2lf MOp/s\n", throughput / 1e6);
				printf("\n");
			}
		}
		
		void SetParameter() {
			std::cout << "input parameter: ";
			std::cin >> parameter_;
		}

	private:
		template<class Subject>
		void AddThread(ThreadFunc<MicrobenchmarkSetting, Subject> thread_func, int num_thread, Subject* subject) {
			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads_.emplace_back(thread_func, microbenchmark_setting_, thread_id, num_thread, subject);
			}

			for (auto& t : threads_) {
				t.join();
			}
		}

		std::vector<std::thread> threads_;
		int parameter_{};
		benchmark::MicrobenchmarkSetting microbenchmark_setting_;
	};
}

#endif