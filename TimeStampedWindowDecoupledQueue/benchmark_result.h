#ifndef BENCHMARK_RESULT_H
#define BENCHMARK_RESULT_H

#include <map>
#include <numeric>
#include "print.h"

namespace benchmark {
	struct Result {
		double elapsed_sec;
		double avg_relaxation_distance;
		int shortest_distance;
		int num_repeat;
	};

	struct ResultMap : public std::map<int, Result>{
		void PrintMicrobenchmarkResult(bool checks_relaxation_distance,
			bool scales_with_depth, int num_op) const {

			for (auto i = cbegin(); i != cend(); ++i) {
				if (scales_with_depth) {
					std::print("k-relaxation: {:5}", i->first);
				} else {
					std::print("threads: {:2}", i->first);
				}
				std::print("  |  ");
				
				if (checks_relaxation_distance) {
					std::print("avg dist: {:7.2f}", i->second.avg_relaxation_distance / i->second.num_repeat);
				} else {
					std::print("avg throughput: {:5.2f} MOp/s", num_op / 1e6 / i->second.elapsed_sec * i->second.num_repeat);
				}
				std::print("\n");
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
				std::print("  |  avg elapsed time: {:5.2f} sec", i->second.elapsed_sec / i->second.num_repeat);
				
				auto avg_error = (static_cast<double>(i->second.shortest_distance) / i->second.num_repeat
					- shortest_distance) / shortest_distance * 100.0;
				std::print("  |  avg error: {:.4f}%\n", avg_error);
			}
			std::print("\n");
		}
	};
}

#endif