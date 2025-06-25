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

namespace benchmark {
	enum class Subject : uint8_t {
		kNone, kTSCAS, kTSStutter, kTSAtomic, kTSInterval, kCBO, k2Dd, kTSWD
	};

	class Tester {
	public:
		Tester() = default;

		void Run();
		void RunMicroBenchmark();
		void RunMacroBenchmark();
	private:
		template<class Subject>
		using MicrobenchmarkFuncT = void(*)(int, int, float, Subject&);

		template<class Subject>
		using MacrobenchmarkFuncT = void(*)(int, int, Subject&, Graph&, int&);

		template<class Subject>
		using PrefillFuncT = void(*)(int, int, Subject&);

		void RunMicroBenchmarkScalingWithThread();
		void RunMicroBenchmarkScalingWithDepth();
		void RunMacroBenchmarkScalingWithThread();
		void RunMacroBenchmarkScalingWithDepth();
		void SetSubject();
		void SetParameter();
		void SetEnqRate();
		void SetWidth();
		void CheckRelaxationDistance();
		void ScaleWithDepth();
		void GenerateGraph();
		void LoadGraph();
		void PrintHelp() const;

		template<class T> requires std::floating_point<T> or std::integral<T>
		T InputNumber(const std::string& msg) const {
			std::cout << msg;

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
		void Measure(MicrobenchmarkFuncT<Subject> thread_func, int key, Subject& subject) {
			Stopwatch stopwatch;
			int num_thread = scales_with_depth_ ? kFixedNumThread : key;

			results.try_emplace(key, Result{});

			if (checks_relaxation_distance_) {
				subject.CheckRelaxationDistance();
			}

			CreateThreads(Prefill, num_thread, subject);

			stopwatch.Start();
			CreateThreads(MicrobenchmarkFunc, num_thread, subject);
			auto elapsed_sec = stopwatch.GetDuration();
			auto rd = subject.GetRelaxationDistance();

			results[key].elapsed_sec += elapsed_sec;
			results[key].avg_relaxation_distance += rd.first;
			results[key].num_repeat += 1;

			std::print("     threads: {}\n", num_thread);
			std::print("elapsed time: {:.2f} sec\n", elapsed_sec);
			if (scales_with_depth_) {
				std::print("k-relaxation: {}\n", key);
			}
			if (checks_relaxation_distance_) {
				std::print("    avg dist: {:.2f}\n", rd.first);
				std::print("    max dist: {}\n", rd.second);
			} else {
				auto throughput = kTotalNumOp / elapsed_sec / 1e6;
				std::print("  throughput: {:.2f} MOp/s\n", throughput);
			}
			std::print("\n");
		}

		template<class Subject>
		void Measure(MacrobenchmarkFuncT<Subject> thread_func, int key, Subject& subject) {
			graph_->Reset();
			
			Stopwatch stopwatch;
			int num_thread = scales_with_depth_ ? kFixedNumThread : key;
			std::vector<int> shortest_distances(num_thread, std::numeric_limits<int>::max());
			
			results.try_emplace(key, Result{});

			stopwatch.Start();
			CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_distances);
			
			auto shortest_distance = *std::min_element(shortest_distances.begin(), shortest_distances.end());
			auto elapsed_sec = stopwatch.GetDuration();

			std::print("          threads: {}\n", num_thread);
			if (scales_with_depth_) {
				std::print("     k-relaxation: {}\n", key);
			}
			std::print("     elapsed time: {:.2f} sec\n", elapsed_sec);
			std::print("shortest distance: {}\n", shortest_distance);
			std::print("\n");

			results[key].elapsed_sec += elapsed_sec;
			results[key].shortest_distance += shortest_distance;
			results[key].num_repeat += 1;
		}

		template<class Subject>
		void CreateThreads(MicrobenchmarkFuncT<Subject> thread_func, int num_thread, Subject& subject) {
			std::vector<std::thread> threads;
			threads.reserve(num_thread);
			
			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads.emplace_back(thread_func, thread_id, num_thread, enq_rate_, std::ref(subject));
			}

			for (auto& t : threads) {
				t.join();
			}
		}

		template<class Subject>
		void CreateThreads(MacrobenchmarkFuncT<Subject> thread_func,
			int num_thread, Subject& subject, std::vector<int>& shortest_dists) {
			std::vector<std::thread> threads;
			threads.reserve(num_thread);

			for (int thread_id = 0; thread_id < num_thread; ++thread_id) {
				threads.emplace_back(thread_func, thread_id, num_thread,
					std::ref(subject), std::ref(*graph_), std::ref(shortest_dists[thread_id]));
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

		static constexpr auto kFixedNumThread{ 41 };

		std::unique_ptr<Graph> graph_{};
		int parameter_{};
		int width_{};
		Subject subject_{};
		ResultMap results;
		bool checks_relaxation_distance_{};
		bool scales_with_depth_{};
		float enq_rate_{ 50.0f };
	};
}

#endif