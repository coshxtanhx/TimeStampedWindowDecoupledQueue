#ifndef BENCHMARK_TESTER_HH
#define BENCHMARK_TESTER_HH

#include <vector>
#include <thread>
#include <stdio.h>
#include "stopwatch.hh"
#include "benchmark_setting.hh"

namespace benchmark {
	class Tester;
	template<class T>
	using ThreadFunc = void(*)(Tester*, int, int, T*);
	using NoSubject = void*;

	class Tester{
	public:
		Tester() noexcept = default;

		template<class Subject, class... Args>
		void StartMicroBenchmark(ThreadFunc<Subject> thread_func, Args... args) {
			constexpr auto kMaxThread{ 72 };
			threads_.reserve(kMaxThread);

			Stopwatch stopwatch;

			for (int num_thread = kMaxThread / 8; num_thread <= kMaxThread; num_thread *= 2){
				threads_.clear();

				stopwatch.Start();

				auto subject = new Subject{ args... };
				for (int thread_id = 0; thread_id < num_thread; ++thread_id){
					threads_.emplace_back(thread_func, this, thread_id, num_thread, subject);
				}
				delete subject;

				
			}
		}
	private:
		std::vector<std::thread> threads_;
		int contention_;
		int num_op_;
	};
}

#endif