#ifndef BENCHMARK_TESTER_H
#define BENCHMARK_TESTER_H

#include <vector>
#include <thread>
#include <map>
#include "stopwatch.h"
#include "microbenchmark_thread_func.h"
#include "macrobenchmark_thread_func.h"
#include "dqrr.h"
#include "cbo.h"
#include "dqlru.h"
#include "ts_interval.h"
#include "twodd.h"
#include "time_stamped_wd.h"

namespace benchmark {
	enum class Subject : uint8_t {
		kNone, kLRU, kRR, kCBO, kTSInterval, k2Dd, kTSWD
	};

	class Tester {
	public:
		template<class Subject>
		using MicrobenchmarkFuncT = void(*)(int, int, int, Subject&);

		template<class Subject>
		using MacrobenchmarkFuncT = void(*)(int, int, Subject&, Graph&, int&);

		Tester() noexcept = default;
		~Tester() noexcept {
			delete graph_;
		}

		void StartMicroBenchmark() {
			constexpr auto kMaxThread{ 72 };
			threads_.reserve(kMaxThread);
			Stopwatch stopwatch;

			std::map<int, double> results{};

			std::cout << "Input the number of times to repeat: ";
			int num_repeat;
			std::cin >> num_repeat;
			for (int i = 1; i <= num_repeat; ++i) {
				printf("------ %d/%d ------\n", i, num_repeat);
				for (int num_thread = 9; num_thread <= kMaxThread; num_thread *= 2) {
					threads_.clear();
					std::pair<double, uint64_t> rd{};
					double elapsed_sec{};
					stopwatch.Start();

					switch (subject_) {
					case Subject::kLRU: {
						lf::dqlru::DQLRU subject{ num_thread * parameter_ / 9, num_thread };
						RunMicrobenchmark(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kRR: {
						lf::dqrr::DQRR subject{ num_thread * parameter_ / 9, num_thread, 1 };
						RunMicrobenchmark(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kCBO: {
						lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
						RunMicrobenchmark(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSInterval: {
						lf::ts::TSInterval subject{ num_thread, parameter_ };
						RunMicrobenchmark(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::k2Dd: {
						lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
						RunMicrobenchmark(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSWD: {
						lf::tswd::TSWD subject{ num_thread, parameter_ };
						RunMicrobenchmark(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					default:
						break;
					}

					//std::this_thread::sleep_for(std::chrono::seconds(1));

					auto throughput = kTotalNumOp / elapsed_sec / 1e6;

					printf("    threads: %d\n", num_thread);
					printf("elapsed sec: %.2lf s\n", elapsed_sec);
					printf(" throughput: %.2lf MOp/s\n", throughput);
					printf("   avg dist: %.2lf\n", rd.first);
					printf("   max dist: %zu\n", rd.second);
					printf("\n");

					results[num_thread] += checks_relaxation_distance_ ? rd.first : throughput;
				}
			}

			for (auto& [num_thread, sum] : results) {
				printf("threads: %2d, avg: %.2lf\n", num_thread, sum / num_repeat);
			}
			printf("\n");
		}
		
		void StartMacroBenchmark() {
			constexpr auto kMaxThread{ 72 };
			threads_.reserve(kMaxThread);

			Stopwatch stopwatch;

			std::map<int, double> results{};

			std::cout << "Input the number of times to repeat: ";
			int num_repeat;
			std::cin >> num_repeat;
			for (int i = 1; i <= num_repeat; ++i) {
				printf("------ %d/%d ------\n", i, num_repeat);

				for (int num_thread = 9; num_thread <= kMaxThread; num_thread *= 2) {
					threads_.clear();
					std::vector<int> shortest_dists(num_thread);
					graph_->Reset();

					stopwatch.Start();

					switch (subject_) {
					case Subject::kLRU: {
						lf::dqlru::DQLRU subject{ num_thread * parameter_ / 9, num_thread };
						RunMacrobenchmark(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kRR: {
						lf::dqrr::DQRR subject{ num_thread * parameter_ / 9, num_thread, num_thread };
						RunMacrobenchmark(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kCBO: {
						lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
						RunMacrobenchmark(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSInterval: {
						lf::ts::TSInterval subject{ num_thread, parameter_ };
						RunMacrobenchmark(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::k2Dd: {
						lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
						RunMacrobenchmark(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSWD: {
						lf::tswd::TSWD subject{ num_thread, parameter_ };
						RunMacrobenchmark(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					default:
						break;
					}

					double elapsed_sec{ stopwatch.GetDuration() };
					auto shortest_distance = *std::min_element(shortest_dists.begin(), shortest_dists.end());

					printf("          threads: %d\n", num_thread);
					printf("      elapsed sec: %.2lf s\n", elapsed_sec);
					printf("shortest distance: %d\n", shortest_distance);
					printf("\n");

					results[num_thread] += elapsed_sec;
				}
			}

			for (auto& [num_thread, sum] : results) {
				printf("threads: %2d, avg: %.2lf\n", num_thread, sum / num_repeat);
			}
			printf("\n");
		}

		void SetSubject() {
			std::cout << "1: LRU, 2: RR, 3: CBO, 4: TS-interval, 5: 2Dd, 6: TSWd\n";
			std::cout << "Input subject: ";
			int subject_id;
			std::cin >> subject_id;
			subject_ = static_cast<Subject>(subject_id);
		}

		void SetParameter() {
			std::cout << "Input parameter: ";
			std::cin >> parameter_;
		}

		void SetEnqRate() {
			std::cout << "Input enq rate(%): ";
			std::cin >> enq_rate_;
		}

		void CheckRelaxationDistance() {
			checks_relaxation_distance_ ^= true;
			if (checks_relaxation_distance_) {
				std::cout << "Checks relaxation distance.\n";
			}
			else {
				std::cout << "Does not check relaxation distance.\n";
			}
		}

		void GenerateGraph() {
			if (nullptr != graph_) {
				std::cout << "[Error] Graph has already been generated!\n";
				return;
			}
			graph_ = new Graph{ 18'000'000 };
			graph_->Save("graph.bin");
		}

		void LoadGraph() {
			if (nullptr != graph_) {
				std::cout << "[Error] Graph has already been generated!\n";
				return;
			}
			graph_ = new Graph{ "graph.bin" };
		}

	private:
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
		Graph* graph_{};
		int parameter_{};
		Subject subject_{};
		bool checks_relaxation_distance_{};
		int enq_rate_{ 50 };
	};
}

#endif