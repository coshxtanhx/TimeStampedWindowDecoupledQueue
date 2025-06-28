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
	int RelaxedBFS(int num_thread, QueueT& queue) {
		int dst = num_vertex_ - 1;

		while (not has_ended_) {
			std::optional<int> curr = queue.Deq();

			if (not curr.has_value()) {
				if (1 == num_thread) {
					return distances_[dst];
				}
				continue;
			}

			auto dist = distances_[curr.value()];

			for (int adj : adjs_[curr.value()]) {
				if (adj == dst) {
					has_ended_ = true;
					return dist + 1;
				}

				auto expected_dist = distances_[adj];

				if (std::numeric_limits<int>::max() == expected_dist) {
					if (true == CAS(adj, expected_dist, dist + 1)) {
						queue.Enq(adj);
					}
				}
			}
		}
		return std::numeric_limits<int>::max();
	}

	void Reset() {
		for (int& i : distances_) {
			i = std::numeric_limits<int>::max();
		}
		distances_.front() = 0;
		has_ended_ = false;
	}

	void Save();

	bool IsValid() const {
		return num_vertex_ != 0;
	}

	void PrintStatus() const {
		std::print("     vertices: {}\n", num_vertex_);
		std::print("        edges: {}\n", num_edge_);
		std::print("shortest dist: {}\n\n", shortest_distance_);
	}

	auto GetShortestDistance() const {
		return shortest_distance_;
	}

	auto GetType() const {
		return type_;
	}

	static std::string GetName(Type type) {
		constexpr std::array<const char*, 7> names{
			"None", "Alpha", "Beta", "Gamma, Delta", "Epsilon", "Zeta"
		};

		return names[static_cast<int>(type)];
	}

private:
	void Generate();
	void Load();
	int SingleThreadBFS();

	bool CAS(int node, int expected_cost, int desired_cost) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<int>*>(&distances_[node]),
			&expected_cost, desired_cost);
	}

	std::vector<std::vector<int>> adjs_;
	std::vector<int> distances_;
	int num_vertex_{};
	unsigned num_edge_{};
	int shortest_distance_{};
	volatile bool has_ended_{};
	Type type_{};
};

#endif
