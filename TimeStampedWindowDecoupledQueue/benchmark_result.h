#ifndef BENCHMARK_RESULT_H
#define BENCHMARK_RESULT_H

#include <map>
#include <vector>
#include <fstream>
#include <numeric>
#include "subject_type.h"
#include "graph.h"
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

	class ResultMap : public std::map<int, std::vector<Result>>{
	public:
		ResultMap() = default;
		void PrintMicrobenchmarkResult(bool checks_relaxation_distance,
			bool scales_with_depth, int num_op, float enq_rate, Subject subject,
			int parameter, int width);
		void PrintMacrobenchmarkResult(bool scales_with_depth, int shortest_distance,
			Graph::Type graph, Subject subject, int parameter, int width);

	private:
		std::ofstream file_{ "log.txt", std::ios::app };
	};
}

#endif