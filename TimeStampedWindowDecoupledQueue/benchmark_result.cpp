#include <format>
#include "benchmark_result.h"

namespace benchmark {
	void ResultMap::PrintMicrobenchmarkResult(bool checks_relaxation_distance,
		bool scales_with_depth, int num_op, float enq_rate, Subject subject,
		int parameter, int width)
	{
		file_ << std::format("subject: {}, parameter: {}\n", GetSubjectName(subject), parameter);
		if (0 != width and Subject::k2Dd == subject and Subject::kCBO == subject) {
			file_ << std::format("width: {}\n", width);
		} else {
			file_ << std::format("width: nbr thread\n");
		}
		file_ << std::format("enqueue rate : {}%\n", enq_rate);

		for (auto i = cbegin(); i != cend(); ++i) {
			if (scales_with_depth) {
				file_ << std::format("k-relaxation: {}, ", i->first);
				std::print("k-relaxation: {:5}", i->first);
			} else {
				file_ << std::format("thread: {}, ", i->first);
				std::print("threads: {:2}", i->first);
			}
			std::print("  |  ");

			auto& results = i->second;

			if (checks_relaxation_distance) {
				auto avg_rd = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
					return acc + r.avg_relaxation_distance;
					}) / results.size();
				std::print("avg dist: {:7.2f}\n", avg_rd);

				file_ << std::format("avg dist: {:.2f}\n", avg_rd);

				std::print("[ ");
				file_ << "[ ";
				for (auto& r : results) {
					file_ << std::format("{:.2f}, ", r.avg_relaxation_distance);
					std::print("{:7.2f}, ", r.avg_relaxation_distance);
				}
				std::print("]\n");
				file_ << " ]\n";
			} else {
				auto avg_sec = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
					return acc + r.elapsed_sec;
					}) / results.size();
				auto throughput = num_op / 1e6 / avg_sec;
				std::print("avg throughput: {:5.2f} MOp/s\n", throughput);

				file_ << std::format("throughput: {:.2f}\n", throughput);

				std::print("[ ");
				file_ << "[ ";
				for (auto& r : results) {
					file_ << std::format("{:.2f}, ", num_op / 1e6 / r.elapsed_sec);
					std::print("{:7.2f}, ", num_op / 1e6 / r.elapsed_sec);
				}
				std::print("]\n");
				file_ << " ]\n";
			}
		}
		std::print("\n");
		file_ << "\n";
	}

	void ResultMap::PrintMacrobenchmarkResult(bool scales_with_depth, int shortest_distance,
		Graph::Type graph, Subject subject, int parameter, int width)
	{
		file_ << std::format("subject: {}, parameter: {}\n", GetSubjectName(subject), parameter);
		if (0 != width and Subject::k2Dd == subject and Subject::kCBO == subject) {
			file_ << std::format("width: {}\n", width);
		} else {
			file_ << std::format("width: nbr thread\n");
		}

		file_ << std::format("graph: {}\n", Graph::GetName(graph));

		for (auto i = cbegin(); i != cend(); ++i) {
			if (scales_with_depth) {
				file_ << std::format("k-relaxation: {}, ", i->first);
				std::print("k-relaxation: {:5}", i->first);
				
			} else {
				file_ << std::format("thread: {}, ", i->first);
				std::print("threads: {:2}", i->first);
			}

			auto& results = i->second;

			auto avg_sec = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
				return acc + r.elapsed_sec;
				}) / results.size();
			std::print("  |  avg elapsed time: {:5.2f} sec", avg_sec);
			file_ << std::format("avg elapsed time: {:.2f} sec, ", avg_sec);

			auto sum_dist = std::accumulate(results.begin(), results.end(), 0, [](int acc, const Result& r) {
				return acc + r.shortest_distance;
				});

			auto avg_error = (static_cast<double>(sum_dist) / results.size()
				- shortest_distance) / shortest_distance * 100.0;
			std::print("  |  avg error: {:.4f}%\n", avg_error);
			file_ << std::format("avg error: {:.4f}%\n", avg_error);

			std::print("seconds: [ ");
			file_ << std::format("seconds: [ ");
			for (auto& r : results) {
				std::print("{:5.2f}, ", r.elapsed_sec);
				file_ << std::format("{:.2f}, ", r.elapsed_sec);
			}
			std::print("]\n");
			file_ << "]\n";

			std::print("dists: [ ");
			file_ << std::format("dists: [ ");
			for (auto& r : results) {
				std::print("{:3}, ", r.shortest_distance);
				file_ << std::format("{}, ", r.shortest_distance);
			}
			std::print("]\n");
			file_ << "]\n";
		}
		std::print("\n");
		file_ << "\n";
	}
}