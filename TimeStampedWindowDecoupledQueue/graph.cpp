#include "graph.h"

void Graph::Save()
{	
	std::ofstream out{ std::format("graph{}.bin", static_cast<int>(type_)), std::ios::binary };

	int size = static_cast<int>(distances_.size());
	out.write(reinterpret_cast<const char*>(&size), sizeof(size));

	for (const auto& adj : adjs_) {
		size = static_cast<int>(adj.size());
		out.write(reinterpret_cast<const char*>(&size), sizeof(size));
		out.write(reinterpret_cast<const char*>(adj.data()), size * sizeof(*adj.data()));
		num_edge_ += size;
	}

	Reset();
	shortest_distance_ = SingleThreadBFS();

	out.write(reinterpret_cast<const char*>(&shortest_distance_), sizeof(shortest_distance_));

	std::print("Graph has been generated.\n");
	PrintStatus();
}

void Graph::Generate()
{
	int max_adj{};
	switch (type_) {
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

	distances_.resize(num_vertex_, std::numeric_limits<int>::max());
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

void Graph::Load()
{
	std::ifstream in{ std::format("graph{}.bin", static_cast<int>(type_)), std::ios::binary };

	if (in.fail()) {
		std::print("[Error] File does not exist.\n");
		return;
	}

	in.read(reinterpret_cast<char*>(&num_vertex_), sizeof(num_vertex_));

	distances_.resize(num_vertex_, std::numeric_limits<int>::max());
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

int Graph::SingleThreadBFS()
{
	int dst = num_vertex_ - 1;
	std::queue<int> queue;
	queue.push(0);

	while (not queue.empty()) {
		auto p = queue.front();
		queue.pop();

		if (p == dst) {
			break;
		}

		int cost = distances_[p] + 1;

		for (auto adj : adjs_[p]) {
			if (cost < distances_[adj]) {
				distances_[adj] = cost;
				queue.push(adj);
			}
		}
	}

	return distances_[dst];
}