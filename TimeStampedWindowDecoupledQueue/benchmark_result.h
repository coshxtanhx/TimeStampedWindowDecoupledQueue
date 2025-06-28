#ifndef BENCHMARK_RESULT_H
#define BENCHMARK_RESULT_H

#include <map>
#include <vector>
#include <numeric>
#include "print.h"

namespace benchmark {
	struct Result {
		Result() = default;
		Result(double elapsed_sec, double avg_relaxation_distance)
			: elapsed_sec{ elapsed_sec }, avg_relaxation_distance{ avg_relaxation_distance } {}

		Result(double avg_relaxation_distance, int shortest_distance) 
			: avg_relaxation_distance{ avg_relaxation_distance }, shortest_distance{ shortest_distance } {}

		double elapsed_sec{};
		double avg_relaxation_distance{};
		int shortest_distance{};
	};

	struct ResultMap : public std::map<int, std::vector<Result>>{
		void PrintMicrobenchmarkResult(bool checks_relaxation_distance,
			bool scales_with_depth, int num_op) const {

			for (auto i = cbegin(); i != cend(); ++i) {
				if (scales_with_depth) {
					std::print("k-relaxation: {:5}", i->first);
				} else {
					std::print("threads: {:2}", i->first);
				}
				std::print("  |  ");

				auto& results = i->second;
				
				if (checks_relaxation_distance) {
					auto avg_rd = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
						return acc + r.avg_relaxation_distance;
						}) / results.size();
					std::print("avg dist: {:7.2f}\n", avg_rd);

					std::print("[ ");
					for (auto& r : results) {
						std::print("{:7.2f}, ", r.avg_relaxation_distance);
					}
					std::print("]\n");
				} else {
					auto avg_sec = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
						return acc + r.elapsed_sec;
						}) / results.size();
					std::print("avg throughput: {:5.2f} MOp/s\n", num_op / 1e6 / avg_sec);

					std::print("[ ");
					for (auto& r : results) {
						std::print("{:7.2f}, ", num_op / 1e6 / r.elapsed_sec);
					}
					std::print("]\n");
				}
			}
			std::print("\n");
		}

		void PrintMacrobenchmarkResult(bool scales_with_depth, int shortest_distance) const {
			for (auto i = cbegin(); i != cend(); ++i) {
				if (scales_with_depth) {
					std::print("k-relaxation: {:5}", i->first);
				} else {
					std::print("threads: {:2}", i->first);
				}

				auto& results = i->second;

				auto avg_sec = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
					return acc + r.elapsed_sec;
					}) / results.size();
				std::print("  |  avg elapsed time: {:5.2f} sec", avg_sec);
				
				auto sum_dist = std::accumulate(results.begin(), results.end(), 0, [](int acc, const Result& r) {
					return acc + r.shortest_distance;
					});

				auto avg_error = (static_cast<double>(sum_dist) / results.size()
					- shortest_distance) / shortest_distance * 100.0;
				std::print("  |  avg error: {:.4f}%\n", avg_error);

				std::print("seconds: [ ");
				for (auto& r : results) {
					std::print("{:5.2f}, ", r.elapsed_sec);
				}
				std::print("]\n");

				std::print("dists: [ ");
				for (auto& r : results) {
					std::print("{:3}, ", r.shortest_distance);
				}
				std::print("]\n");
			}
			std::print("\n");
		}
	};
}

#endif