#include <format>
#include "benchmark_result.h"

namespace benchmark {
	void ResultMap::PrintResult(bool checks_relaxation_distance,
		bool scales_with_depth, int32_t num_op) const
	{
		for (auto i = cbegin(); i != cend(); ++i) {
			if (scales_with_depth) {
				compat::Print("k-relaxation: {:5}", i->first);
			} else {
				compat::Print("threads: {:2}", i->first);
			}
			compat::Print("  |  ");

			auto& results = i->second;

			if (checks_relaxation_distance) {
				auto total_element = std::accumulate(results.begin(), results.end(), uint64_t{}, [](uint64_t acc, const Result& r) {
					return acc + r.num_element;
					});

				auto sum_rd = std::accumulate(results.begin(), results.end(), uint64_t{}, [](uint64_t acc, const Result& r) {
					return acc + r.sum_relaxation_distance;
					});
				auto avg_dist = static_cast<double>(sum_rd) / total_element;
				compat::Print("avg dist: {:7.2f}\n", avg_dist);
			} else {
				auto avg_sec = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
					return acc + r.elapsed_sec;
					}) / results.size();
				auto throughput = num_op / 1e6 / avg_sec;
				compat::Print("avg throughput: {:5.2f} MOp/s\n", throughput);
			}
		}
		compat::Print("\n");
	}

	void ResultMap::PrintResult(bool scales_with_depth, int32_t distance) const
	{
		for (auto i = cbegin(); i != cend(); ++i) {
			if (scales_with_depth) {
				compat::Print("k-relaxation: {:5}", i->first);
				
			} else {
				compat::Print("threads: {:2}", i->first);
			}

			auto& results = i->second;

			auto avg_sec = std::accumulate(results.begin(), results.end(), 0.0, [](double acc, const Result& r) {
				return acc + r.elapsed_sec;
				}) / results.size();
			compat::Print("  |  avg elapsed time: {:5.2f} sec", avg_sec);

			auto sum_dist = std::accumulate(results.begin(), results.end(), 0, [](int32_t acc, const Result& r) {
				return acc + r.distance;
				});

			auto avg_error = (static_cast<double>(sum_dist) / results.size()
				- distance) / distance * 100.0;
			compat::Print("  |  avg error: {:.4f}%\n", avg_error);
		}
		compat::Print("\n");
	}

	void ResultMap::Save(bool checks_relaxation_distance, bool scales_with_depth,
		float enq_rate, Subject subject, int parameter, int width)
	{
		file_ << std::format("subject: {}, ", GetSubjectName(subject));
		if ((Subject::k2Dd == subject or Subject::kCBO == subject) and width != 0) {
			file_ << std::format("width: {}, ", width);

		} else {
			file_ << std::format("width: nbr thread, ");
		}

		if (scales_with_depth) {
			file_ << "k-relaxation: ";
		} else {
			file_ << std::format("parameter: {}, ", parameter);
			file_ << "threads: ";
		}
		file_ << std::format("enq rate: {}\n", enq_rate);

		for (auto& [key, results] : *this) {
			file_ << std::format("{}|", key);
		}
		file_ << '\n';

		if (checks_relaxation_distance) {
			file_ << "dequeued elements|sum dist|max dist|\n";
		} else {
			file_ << "elapsed sec\n";
		}

		for (auto& [key, results] : *this) {
			if (checks_relaxation_distance) {
				for (auto& result : results) {
					file_ << std::format("{}|", result.num_element);
				}
				for (auto& result : results) {
					file_ << std::format("{}|", result.sum_relaxation_distance);
				}
				for (auto& result : results) {
					file_ << std::format("{}|", result.max_relaxation_distance);
				}
			} else {
				for (auto& result : results) {
					file_ << std::format("{:.6f}|", result.elapsed_sec);
				}
			}
			file_ << '\n';
		}

		file_ << std::format("\n\n");
	}

	void ResultMap::Save(bool scales_with_depth, Graph::Type graph,
		Subject subject, int parameter, int width)
	{
		file_ << std::format("subject: {}, ", GetSubjectName(subject));
		if ((Subject::k2Dd == subject or Subject::kCBO == subject) and width != 0) {
			file_ << std::format("width: {}, ", width);

		} else {
			file_ << std::format("width: nbr thread, ");
		}

		if (scales_with_depth) {
			file_ << "k-relaxation: ";
		} else {
			file_ << std::format("parameter: {}, ", parameter);
			file_ << "threads: ";
		}
		file_ << std::format("graph: {}\n", Graph::GetName(graph));

		for (auto& [key, results] : *this) {
			file_ << std::format("{}|", key);
		}
		file_ << '\n';

		for (auto& [key, results] : *this) {
			for (auto& result : results) {
				file_ << std::format("{:.6f}|", result.elapsed_sec);
			}
			for (auto& result : results) {
				file_ << std::format("{}|", result.distance);
			}
			file_ << '\n';
		}
		file_ << "\n\n";
	}
}