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
#include "twodd.h"
#include "time_stamped_lateral_queue.h"

namespace benchmark {
	template<class BenchmarkSetting, class Subject>
	using ThreadFunc = void(*)(BenchmarkSetting, int, int, Subject*);

	class Tester {
	public:
		Tester() noexcept = default;

		void StartMicroBenchmark() {
			constexpr auto kMaxThread{ 72 };
			threads_.reserve(kMaxThread);

			Stopwatch stopwatch;

			for (int num_thread = 9; num_thread <= kMaxThread; num_thread *= 2){
				threads_.clear();
				double average_relaxation_distance{};
				double elapsed_sec{};
				stopwatch.Start();

				switch (microbenchmark_setting_.subject) {
				case Subject::kLRU: {
					lf::dqlru::DQLRU subject{ num_thread * parameter_ / 9, num_thread };
					AddThread(MicrobenchmarkFunc, num_thread, &subject);
					elapsed_sec = stopwatch.GetDuration();
					average_relaxation_distance = subject.GetRelaxationDistance();
					break;
				}
				case Subject::kRR: {
					lf::dqrr::DQRR subject{ num_thread * parameter_ / 9, num_thread, num_thread };
					AddThread(MicrobenchmarkFunc, num_thread, &subject);
					elapsed_sec = stopwatch.GetDuration();
					average_relaxation_distance = subject.GetRelaxationDistance();
					break;
				}
				case Subject::kRA: {
					lf::dqra::DQRA subject{ num_thread, num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, &subject);
					elapsed_sec = stopwatch.GetDuration();
					average_relaxation_distance = subject.GetRelaxationDistance();
					break;
				}
				case Subject::kTSInterval: {
					lf::ts::TSInterval subject{ num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, &subject);
					elapsed_sec = stopwatch.GetDuration();
					average_relaxation_distance = subject.GetRelaxationDistance();
					break;
				}
				case Subject::k2Dd: {
					lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, &subject);
					elapsed_sec = stopwatch.GetDuration();
					average_relaxation_distance = subject.GetRelaxationDistance();
					break;
				}
				case Subject::kTSL: {
					lf::tsl::TimeStampedLateralQueue subject{ num_thread, parameter_ };
					AddThread(MicrobenchmarkFunc, num_thread, &subject);
					elapsed_sec = stopwatch.GetDuration();
					average_relaxation_distance = subject.GetRelaxationDistance();
					break;
				}
				default:
					break;
				}

				//std::this_thread::sleep_for(std::chrono::seconds(1));

				auto throughput = microbenchmark_setting_.num_op / elapsed_sec;

				printf("    threads: %d\n", num_thread);
				printf("elapsed sec: %.2lf s\n", elapsed_sec);
				printf(" throughput: %.2lf MOp/s\n", throughput / 1e6);
				printf("   avg dist: %.2lf\n", average_relaxation_distance);
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
				//subject->CheckRelaxationDistance();
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