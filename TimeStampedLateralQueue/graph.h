#ifndef GRAPH_H

#include <atomic>
#include <fstream>
#include <vector>
#include <random>
#include <optional>
#include <stdio.h>

class Graph {
public:
	Graph(int num_vertex)
		: num_vertex_{ num_vertex }, has_visited_(num_vertex, std::numeric_limits<int>::max()), has_ended_{ false } {
		adjs_.resize(num_vertex);

		for (auto& adj : adjs_) {
			adj.reserve(18);
		}

		std::mt19937 re;
		std::uniform_int_distribution uid;

		for (int i = 0; i < num_vertex - 1; ++i) {
			adjs_[i].push_back(i + 1);
			adjs_[i + 1].push_back(i);

			auto step = uid(re) % 100;
			if (step <= 1) {
				continue;
			}

			for (int j = 1; ; ++j) {
				int next = i + step * j;
				if (next >= num_vertex or adjs_[i].capacity() == adjs_[i].size()) {
					break;
				}
				if (adjs_[next].capacity() > adjs_[next].size() and uid(re) % 100 < 5) {
					adjs_[i].push_back(next);
					adjs_[next].push_back(i);
				}
			}

			for (int j = static_cast<int>(adjs_[i].size() - 1); j > 0; --j) {
				int r = uid(re) % 100 % j;
				std::swap(adjs_[i][j], adjs_[i][r]);
			}
		}
	}

	Graph(const std::string& file_name) : has_ended_{ false } {
		std::ifstream in{ file_name, std::ios::binary };

		int size{};
		in.read(reinterpret_cast<char*>(&size), sizeof(size));

		has_visited_.resize(size, std::numeric_limits<int>::max());
		adjs_.resize(size);

		for (auto& adj : adjs_) {
			in.read(reinterpret_cast<char*>(&size), sizeof(size));
			adj.resize(size);

			in.read(reinterpret_cast<char*>(adj.data()), size * sizeof(*adj.data()));
		}
	}

	template<class QueueT>
	int BFS(int num_thread, QueueT& queue) {
		int dst = num_vertex_ - 1;

		while (not has_ended_) {
			std::optional<int> node = queue.Deq();

			if (not node.has_value()) {
				if (1 == num_thread) {
					return has_visited_[dst];
				}
				continue;
			}

			auto cost = has_visited_[node];

			for (int adj : adjs_[node]) {
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
					}
					else {
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

	void Save(const std::string& file_name) const {
		std::ofstream out{ file_name, std::ios::binary };
		int size = static_cast<int>(has_visited_.size());
		out.write(reinterpret_cast<const char*>(&size), sizeof(size));

		for (const auto& adj : adjs_) {
			size = static_cast<int>(adj.size());
			out.write(reinterpret_cast<const char*>(&size), sizeof(size));
			out.write(reinterpret_cast<const char*>(adj.data()), size * sizeof(*adj.data()));
		}
	}

private:
	bool CAS(int node, int expected_cost, int desired_cost) {
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic<int>*>(&has_visited_[node]),
			&expected_cost, desired_cost);
	}

	int num_vertex_{};
	std::vector<std::vector<int>> adjs_;
	std::vector<int> has_visited_;
	volatile bool has_ended_;
};

#endif
