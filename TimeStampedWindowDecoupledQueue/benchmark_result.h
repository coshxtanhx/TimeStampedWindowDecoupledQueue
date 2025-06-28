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
		Result(double elapsed_sec, uint64_t num_element, uint64_t sum_rd, uint64_t max_rd)
			: elapsed_sec{ elapsed_sec }, num_element{ num_element }
			, sum_relaxation_distance{ sum_rd }, max_relaxation_distance{ max_rd } {}

		Result(double elapsed_sec, int shortest_distance)
			: elapsed_sec{ elapsed_sec }, shortest_distance{ shortest_distance } {}

		double elapsed_sec{};
		uint64_t num_element{};
		uint64_t sum_relaxation_distance{};
		uint64_t max_relaxation_distance{};
		int shortest_distance{};
	};

	class ResultMap : public std::map<int, std::vector<Result>>{
	public:
		ResultMap() = default;
		void PrintResult(bool checks_relaxation_distance,
			bool scales_with_depth, int num_op) const;
		void PrintResult(bool scales_with_depth, int shortest_distance) const;

		void Save(bool checks_relaxation_distance, bool scales_with_depth,
			float enq_rate, Subject subject, int parameter, int width);

		void Save(bool scales_with_depth, Graph::Type graph, Subject subject, int parameter, int width);

	private:
		std::ofstream file_{ "log.txt", std::ios::app };
	};
}

#endif