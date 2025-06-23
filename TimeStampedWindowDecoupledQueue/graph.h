#ifndef GRAPH_H
#define GRAPH_H

#include <atomic>
#include <fstream>
#include <vector>
#include <random>
#include <optional>
#include <queue>
#include <thread>
#include "print.h"

class Graph {
public:
	enum class Type {
		kAlpha,
		kBeta,
		kGamma,
		kDelta,
		kEpsilon,
		kZeta,
		kSize
	};

	Graph(Type type) {
		int max_adj{};
		switch (type) {
			case Type::kAlpha: {
				max_adj = 12;
				num_vertex_ = 4'500'000;
				break;
			}
			case Type::kBeta: {
				max_adj = 72;
				num_vertex_ = 4'500'000;
				break;
			}
			case Type::kGamma: {
				max_adj = 18;
				num_vertex_ = 12'000'000;
				break;
			}
			case Type::kDelta: {
				max_adj = 72;
				num_vertex_ = 18'000'000;
				break;
			}
			case Type::kEpsilon: {
				max_adj = 84;
				num_vertex_ = 21'000'000;
				break;
			}
			case Type::kZeta: {
				max_adj = 100;
				num_vertex_ = 25'000'000;
				break;
			}
		}

		has_visited_.resize(num_vertex_, std::numeric_limits<int>::max());
		adjs_.resize(num_vertex_);

		for (auto& adj : adjs_) {
			adj.reserve(max_adj);
		}

		std::mt19937 re{ 2025 };
		std::uniform_int_distribution<int> uid{ 0, num_vertex_ };

		for (int i = 0; i < num_vertex_ - 1; ++i) {
			adjs_[i].push_back(i + 1);
			adjs_[i + 1].push_back(i);

			auto step = uid(re) % 100;
			if (step <= 1) {
				continue;
			}

			for (int j = 1; ; ++j) {
				int next = i + step * j;
				if (next >= num_vertex_ or max_adj == adjs_[i].size()) {
					break;
				}
				if (max_adj > adjs_[next].size() and max_adj > adjs_[i].size() and uid(re) % 100 < 5) {
					adjs_[i].push_back(next);
					adjs_[next].push_back(i);
				}
			}

			for (int j = static_cast<int>(adjs_[i].size() - 1); j > 0; --j) {
				int r = uid(re) % j;
				std::swap(adjs_[i][j], adjs_[i][r]);
			}
		}
	}

	Graph(const std::string& file_name) {
		std::ifstream in{ file_name, std::ios::binary };

		if (in.fail()) {
			std::print("[Error] File does not exist.\n");
			return;
		}

		in.read(reinterpret_cast<char*>(&num_vertex_), sizeof(num_vertex_));

		has_visited_.resize(num_vertex_, std::numeric_limits<int>::max());
		adjs_.resize(num_vertex_);

		int num_adj{};
		for (auto& adj : adjs_) {
			in.read(reinterpret_cast<char*>(&num_adj), sizeof(num_adj));
			num_edge_ += num_adj;
			adj.resize(num_adj);

			in.read(reinterpret_cast<char*>(adj.data()), num_adj * sizeof(*adj.data()));
		}

		in.read(reinterpret_cast<char*>(&shortest_distance_), sizeof(shortest_distance_));

		std::print("Graph has been loaded.\n");
		PrintStatus();
	}

	template<class QueueT>
	int RelaxedBFS(int num_thread, QueueT& queue) {
		int dst = num_vertex_ - 1;

		while (not has_ended_) {
			std::optional<int> prev = queue.Deq();

			if (not prev.has_value()) {
				if (1 == num_thread) {
					return has_visited_[dst];
				}
				continue;
			}

			auto cost = has_visited_[prev.value()];

			for (int adj : adjs_[prev.value()]) {
				if (adj == dst) {
					has_ended_ = true;
					return cost + 1;
				}
				while (true) {
					auto expected_cost = has_visited_[adj];

					if (expected_cost > cost + 1) {
						if (true == CAS(adj, expected_cost, cost + 1)) {
							queue.Enq(adj);
							break;
						}
					} else {
						break;
					}
				}
			}
		}
		return std::numeric_limits<int>::max();
	}

	void Reset() {
		for (int& i : has_visited_) {
			i = std::numeric_limits<int>::max();
		}
		has_visited_.front() = 0;
		has_ended_ = false;
	}

	void Save(const std::string& file_name) {
		std::ofstream out{ file_name, std::ios::binary };
		int size = static_cast<int>(has_visited_.size());
		out.write(reinterpret_cast<const char*>(&size), sizeof(size));

		for (const auto& adj : adjs_) {
			size = static_cast<int>(adj.size());
			out.write(reinterpret_cast<const char*>(&size), sizeof(size));
			out.write(reinterpret_cast<const char*>(adj.data()), size * sizeof(*adj.data()));
			num_edge_ += size;
		}

		Reset();
		shortest_distance_ = UnsafeBFS();

		out.write(reinterpret_cast<const char*>(&shortest_distance_), sizeof(shortest_distance_));

		std::print("Graph has been generated.\n");
		PrintStatus();
	}

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

private:
	bool CAS(int node, int expected_cost, int desired_cost) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<int>*>(&has_visited_[node]),
			&expected_cost, desired_cost);
	}

	int UnsafeBFS() {
		int dst = num_vertex_ - 1;
		std::queue<int> queue;
		queue.push(0);

		while (not queue.empty()) {
			auto p = queue.front();
			queue.pop();

			if (p == dst) {
				break;
			}

			int cost = has_visited_[p] + 1;

			for (auto adj : adjs_[p]) {
				if (cost < has_visited_[adj]) {
					has_visited_[adj] = cost;
					queue.push(adj);
				}
			}
		}

		return has_visited_[dst];
	}

	std::vector<std::vector<int>> adjs_;
	std::vector<int> has_visited_;
	int num_vertex_{};
	unsigned num_edge_{};
	int shortest_distance_{};
	volatile bool has_ended_{};
};

#endif
