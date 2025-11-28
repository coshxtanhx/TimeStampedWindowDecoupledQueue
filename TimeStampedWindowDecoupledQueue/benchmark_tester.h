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
		Tester() = default;

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
			auto num_thread = scales_with_depth_ ? kFixedNumThread : key;

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

			PRINT("     threads: {}\n", num_thread);
			if (scales_with_depth_) {
				PRINT("k-relaxation: {}\n", key);
			}
			if (checks_relaxation_distance_) {
				PRINT("    avg dist: {:.2f}\n", static_cast<double>(sum_rd) / num_element);
				PRINT("    max dist: {}\n", max_rd);
			} else {
				PRINT("elapsed time: {:.2f} sec\n", elapsed_sec);
				auto throughput = kTotalNumOp / elapsed_sec / 1e6;
				PRINT("  throughput: {:.2f} MOp/s\n", throughput);
			}
			PRINT("\n");
		}

		template<class Subject>
		void Measure(MacrobenchmarkFuncT<Subject> thread_func, int32_t key, Subject& subject) {
			graph_->Reset();

			Stopwatch stopwatch;
			auto num_thread = scales_with_depth_ ? kFixedNumThread : key;
			std::vector<int32_t> distances(num_thread, std::numeric_limits<int>::max());
			
			results.try_emplace(key, std::vector<Result>{});

			stopwatch.Start();
			CreateThreads(MacrobenchmarkFunc, num_thread, subject, distances);
			
			auto distance = *std::min_element(distances.begin(), distances.end());
			auto elapsed_sec = stopwatch.GetDuration();

			PRINT("     threads: {}\n", num_thread);
			if (scales_with_depth_) {
				PRINT("k-relaxation: {}\n", key);
			}
			PRINT("elapsed time: {:.2f} sec\n", elapsed_sec);
			PRINT("    distance: {}\n", distance);
			PRINT("\n");

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

		static constexpr auto kFixedNumThread{ 41 };
		static constexpr std::array<int, 4> kNumThreads{ 12, 24, 48, 72 };

		std::unique_ptr<Graph> graph_{};
		int parameter_{};
		int width_{};
		Subject subject_{};
		ResultMap results;
		bool checks_relaxation_distance_{};
		bool scales_with_depth_{};
		float enq_rate_{ 50.0f };
		float delay_{ 1.2f };
	};
}

#endif