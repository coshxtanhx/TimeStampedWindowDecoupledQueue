#ifndef BENCHMARK_TESTER_H
#define BENCHMARK_TESTER_H

#include <array>
#include <vector>
#include <thread>
#include <memory>
#include <map>
#include "stopwatch.h"
#include "graph.h"
#include "benchmark_result.h"
#include "microbenchmark_thread_func.h"
#include "macrobenchmark_thread_func.h"
#include "subject_type.h"

namespace benchmark {

	class Tester {
	public:
		Tester() {
			auto max_thread = std::thread::hardware_concurrency();
			switch (max_thread) {
				case 8: {
					fixed_num_thread_ = 6;
					num_threads_ = decltype(num_threads_){ 2, 4, 6, 8 };
					break;
				}
				case 16: {
					fixed_num_thread_ = 11;
					num_threads_ = decltype(num_threads_){ 4, 8, 12, 16 };
					break;
				}
				case 40: {
					fixed_num_thread_ = 33;
					num_threads_ = decltype(num_threads_){ 10, 20, 30, 40 };
					break;
				}
				default: {
					fixed_num_thread_ = 41;
					num_threads_ = decltype(num_threads_){ 12, 24, 48, 72 };
					break;
				}
			}
		}

		void Run();
		void RunMicroBenchmark();
		void RunMacroBenchmark();
	private:
		template<class Subject>
		using MicrobenchmarkFuncT = void(*)(int, int, float, float, Subject&);

		template<class Subject>
		using MacrobenchmarkFuncT = void(*)(int, int, Subject&, Graph&, int&);

		template<class Subject>
		using PrefillFuncT = void(*)(int, int, Subject&);

		bool RunMicroBenchmarkScalingWithThread();
		bool RunMicroBenchmarkScalingWithDepth();
		bool RunMacroBenchmarkScalingWithThread();
		bool RunMacroBenchmarkScalingWithDepth();
		void SetSubject();
		void SetParameter();
		void SetEnqRate();
		void SetWidth();
		void SetDelay();
		void CheckRelaxationDistance();
		void ScaleWithDepth();
		void GenerateGraph();
		void LoadGraph();
		void PrintHelp() const;

		template<class T> requires std::floating_point<T> or std::integral<T>
		T InputNumber() const {
			T input;
			std::cin >> input;
			if (std::cin.fail()) {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				return T{};
			} else {
				std::cin.ignore();
				return input;
			}
		}

		template<class Subject>
		void Measure(MicrobenchmarkFuncT<Subject> thread_func, int32_t key, Subject& subject) {
			Stopwatch stopwatch;
			auto num_thread = scales_with_depth_ ? fixed_num_thread_ : key;

			results.try_emplace(key, std::vector<Result>{});

			if (checks_relaxation_distance_) {
				subject.CheckRelaxationDistance();
			}

			CreateThreads(Prefill, num_thread, subject);

			stopwatch.Start();
			CreateThreads(MicrobenchmarkFunc, num_thread, subject);
			auto elapsed_sec = stopwatch.GetDuration();
			auto [num_element, sum_rd, max_rd] = subject.GetRelaxationDistance();

			results[key].emplace_back(elapsed_sec, num_element, sum_rd, max_rd);

			compat::Print("     threads: {}\n", num_thread);
			if (scales_with_depth_) {
				compat::Print("k-relaxation: {}\n", key);
			}
			if (checks_relaxation_distance_) {
				compat::Print("    avg dist: {:.2f}\n", static_cast<double>(sum_rd) / num_element);
				compat::Print("    max dist: {}\n", max_rd);
			} else {
				compat::Print("elapsed time: {:.2f} sec\n", elapsed_sec);
				auto throughput = kTotalNumOp / elapsed_sec / 1e6;
				compat::Print("  throughput: {:.2f} MOp/s\n", throughput);
			}
			compat::Print("\n");
		}

		template<class Subject>
		void Measure(MacrobenchmarkFuncT<Subject> thread_func, int32_t key, Subject& subject) {
			graph_->Reset();

			Stopwatch stopwatch;
			auto num_thread = scales_with_depth_ ? fixed_num_thread_ : key;
			std::vector<int32_t> distances(num_thread, std::numeric_limits<int>::max());
			
			results.try_emplace(key, std::vector<Result>{});

			stopwatch.Start();
			CreateThreads(MacrobenchmarkFunc, num_thread, subject, distances);
			
			auto distance = *std::min_element(distances.begin(), distances.end());
			auto elapsed_sec = stopwatch.GetDuration();

			compat::Print("     threads: {}\n", num_thread);
			if (scales_with_depth_) {
				compat::Print("k-relaxation: {}\n", key);
			}
			compat::Print("elapsed time: {:.2f} sec\n", elapsed_sec);
			compat::Print("    distance: {}\n", distance);
			compat::Print("\n");

			results[key].emplace_back(elapsed_sec, distance);
		}

		template<class Subject>
		void CreateThreads(MicrobenchmarkFuncT<Subject> thread_func, int num_thread, Subject& subject) {
			std::vector<std::thread> threads;
			threads.reserve(num_thread);
			
			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads.emplace_back(thread_func, thread_id, num_thread, enq_rate_,
					checks_relaxation_distance_ ? 0.0f : delay_, std::ref(subject));
			}

			for (auto& t : threads) {
				t.join();
			}
		}

		template<class Subject>
		void CreateThreads(MacrobenchmarkFuncT<Subject> thread_func,
			int num_thread, Subject& subject, std::vector<int32_t>& distances) {
			std::vector<std::thread> threads;
			threads.reserve(num_thread);

			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads.emplace_back(thread_func, thread_id, num_thread,
					std::ref(subject), std::ref(*graph_), std::ref(distances[thread_id]));
			}

			for (auto& t : threads) {
				t.join();
			}
		}

		template<class Subject>
		void CreateThreads(PrefillFuncT<Subject> thread_func, int num_thread, Subject& subject) {
			std::vector<std::thread> threads;
			threads.reserve(num_thread);

			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads.emplace_back(thread_func, thread_id, num_thread, std::ref(subject));
			}

			for (auto& t : threads) {
				t.join();
			}
		}

		bool HasValidParameter() const;

		std::unique_ptr<Graph> graph_{};
		int parameter_{};
		int width_{};
		Subject subject_{};
		ResultMap results;
		bool checks_relaxation_distance_{};
		bool scales_with_depth_{};
		float enq_rate_{ 50.0f };
		float delay_{ 1.2f };

		int fixed_num_thread_{};
		std::array<int, 4> num_threads_{};
	};
}

#endif