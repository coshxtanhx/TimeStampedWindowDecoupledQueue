#ifndef GRAPH_H
#define GRAPH_H

#include <atomic>
#include <fstream>
#include <vector>
#include <random>
#include <array>
#include <optional>
#include <format>
#include <queue>
#include <thread>
#include "print.h"

class Graph {
public:
	enum class Type : uint8_t {
		kNone, kAlpha, kBeta, kGamma, kDelta, kEpsilon, kZeta,
	};

	enum class Option {
		kGenerate, kLoad
	};

	Graph(Type type, Option option) : type_{ type } {
		if (Option::kGenerate == option) {
			Generate();
		} else {
			Load();
		}
	}

	template<class QueueT>
	int32_t RelaxedBFS(int num_thread, QueueT& queue) {
		auto dst = num_vertex_ - 1;

		while (not has_ended_) {
			std::optional<int> curr = queue.Deq();

			if (not curr.has_value()) {
				if (1 == num_thread) {
					return distances_[dst];
				}
				continue;
			}

			auto dist = distances_[curr.value()];

			for (auto adj : adjs_[curr.value()]) {
				if (adj == dst) {
					has_ended_ = true;
					return dist + 1;
				}

				auto expected_dist = distances_[adj];

				if (std::numeric_limits<int32_t>::max() == expected_dist) {
					if (true == CAS(adj, expected_dist, dist + 1)) {
						queue.Enq(adj);
					}
				}
			}
		}
		return std::numeric_limits<int32_t>::max();
	}

	void Reset() {
		for (auto& i : distances_) {
			i = std::numeric_limits<int32_t>::max();
		}
		distances_.front() = 0;
		has_ended_ = false;
	}

	void Save();

	bool IsValid() const {
		return num_vertex_ != 0;
	}

	void PrintStatus() const {
		compat::Print("     vertices: {}\n", num_vertex_);
		compat::Print("        edges: {}\n", num_edge_);
		compat::Print("shortest dist: {}\n\n", shortest_distance_);
	}

	auto GetShortestDistance() const {
		return shortest_distance_;
	}

	auto GetType() const {
		return type_;
	}

	static std::string GetName(Type type) {
		constexpr std::array<const char*, 7> names{
			"None", "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta"
		};

		return names[static_cast<int>(type)];
	}

private:
	void Generate();
	void Load();
	int32_t SingleThreadBFS();

	bool CAS(int32_t node, int32_t expected_cost, int32_t desired_cost) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<int32_t>*>(&distances_[node]),
			&expected_cost, desired_cost);
	}

	std::vector<std::vector<int32_t>> adjs_;
	std::vector<int32_t> distances_;
	int32_t num_vertex_{};
	uint32_t num_edge_{};
	int32_t shortest_distance_{};
	volatile bool has_ended_{};
	Type type_{};
};

#endif
