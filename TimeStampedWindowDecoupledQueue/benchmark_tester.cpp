#include "benchmark_tester.h"
#include "microbenchmark_thread_func.h"
#include "macrobenchmark_thread_func.h"
#include "stopwatch.h"
#include "cbo.h"
#include "ts_interval.h"
#include "ts_cas.h"
#include "ts_atomic.h"
#include "ts_stutter.h"
#include "twodd.h"
#include "tswd.h"

namespace benchmark {
	struct BFSResult {
		int shortest_distance;
		double elapsed_sec;
	};

	void Tester::Run()
	{
		while (true) {
			std::print("Command ('h' for help): ");
			std::string cmd;
			std::getline(std::cin, cmd);

			if (cmd.size() > 1) {
				continue;
			}
			if (std::cin.eof()) {
				return;
			}

			switch (cmd.front()) {
				case 'e': {
					SetEnqRate();
					break;
				}
				case 'm': {
					CheckRelaxationDistance();
					break;
				}
				case 'c': {
					ScaleWithDepth();
					break;
				}
				case 's': {
					SetSubject();
					break;
				}
				case 'p': {
					SetParameter();
					break;
				}
				case 'i': {
					RunMicroBenchmark();
					break;
				}
				case 'a': {
					RunMacroBenchmark();
					break;
				}
				case 'g': {
					GenerateGraph();
					break;
				}
				case 'l': {
					LoadGraph();
					break;
				}
				case 'h': {
					PrintHelp();
					break;
				}
				case 'q': {
					return;
				}
				default: {
					break;
				}
			}
		}
	}

	void Tester::RunMicroBenchmark()
	{
		if (0 == parameter_ and not scales_with_depth_) {
			std::print("[Error] Set parameter first.\n\n");
			return;
		}

		std::print("Input the number of times to repeat: ");
		int num_repeat;
		std::cin >> num_repeat;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}

		if (scales_with_depth_) {
			RunMicroBenchmarkScalingWithDepth(num_repeat);
		} else {
			RunMicroBenchmarkScalingWithThread(num_repeat);
		}
	}

	void Tester::RunMacroBenchmark()
	{
		if (nullptr == graph_) {
			std::print("[Error] Generate or load graph first.\n\n");
			return;
		}
		if (0 == parameter_ and not scales_with_depth_) {
			std::print("[Error] Set parameter first.\n\n");
			return;
		}

		std::print("Input the number of times to repeat: ");
		int num_repeat;
		std::cin >> num_repeat;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}

		if (num_repeat <= 0) {
			return;
		}

		if (scales_with_depth_) {
			RunMacroBenchmarkScalingWithDepth(num_repeat);
		} else {
			RunMacroBenchmarkScalingWithThread(num_repeat);
		}

		
	}

	void Tester::RunMicroBenchmarkScalingWithThread(int num_repeat)
	{
		constexpr auto kMaxThread{ kNumThreads.back() };
		threads_.reserve(kMaxThread);
		Stopwatch stopwatch;

		std::map<int, double> results{};

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("------ {}/{} ------\n", i, num_repeat);
			for (int num_thread : kNumThreads) {
				threads_.clear();
				std::pair<double, uint64_t> rd{};
				double elapsed_sec{};

				switch (subject_) {
					case Subject::kTSCAS: {
						lf::ts_cas::TSCAS subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSStutter: {
						lf::ts_atomic::TSAtomic subject{ num_thread };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSAtomic: {
						lf::ts_atomic::TSAtomic subject{ num_thread };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSInterval: {
						lf::ts_interval::TSInterval subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kCBO: {
						lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::k2Dd: {
						lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSWD: {
						lf::tswd::TSWD subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					default: {
						std::print("[Error] Invalid subject.\n\n");
						return;
					}
				}

				auto throughput = kTotalNumOp / elapsed_sec / 1e6;

				std::print("    threads: {}\n", num_thread);
				std::print("elapsed sec: {:.2f}\n", elapsed_sec);

				if (checks_relaxation_distance_) {
					std::print("   avg dist: {:.2f}\n", rd.first);
					std::print("   max dist: {}\n", rd.second);
				} else {
					std::print(" throughput: {:.2f} MOp/s\n", throughput);
				}
				std::print("\n");

				results[num_thread] += checks_relaxation_distance_ ? rd.first : throughput;
			}
		}

		for (auto& [num_thread, sum] : results) {
			std::print("threads: {:2}, avg: {:.2f}\n", num_thread, sum / num_repeat);
		}
		std::print("\n");
	}

	void Tester::RunMicroBenchmarkScalingWithDepth(int num_repeat)
	{
		constexpr auto kNumThread{ 41 };
		threads_.reserve(kNumThread);
		Stopwatch stopwatch;

		std::map<int, double> results{};

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("------ {}/{} ------\n", i, num_repeat);
			
			std::pair<double, uint64_t> rd{};
			double elapsed_sec{};

			for (auto rb : kRelaxationBounds) {
				threads_.clear();
				switch (subject_) {
					case Subject::k2Dd: {
						auto depth = rb / (kNumThread - 1);
						lf::twodd::TwoDd subject{ kNumThread, kNumThread, depth };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, kNumThread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kTSWD: {
						auto depth = rb / (kNumThread - 1) - 1;
						lf::tswd::TSWD subject{ kNumThread, depth };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, kNumThread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					default: {
						std::print("[Error] Invalid subject. 'Scaling with depth' mode is only for 2Dd or TSWD.\n\n");
						return;
					}
				}

				auto throughput = kTotalNumOp / elapsed_sec / 1e6;

				std::print("     threads: {}\n", kNumThread);
				std::print("k-relaxation: {}\n", rb);
				std::print(" elapsed sec: {:.2f}\n", elapsed_sec);

				if (checks_relaxation_distance_) {
					std::print("    avg dist: {:.2f}\n", rd.first);
					std::print("    max dist: {}\n", rd.second);
				} else {
					std::print("  throughput: {:.2f} MOp/s\n", throughput);
				}
				std::print("\n");

				results[rb] += checks_relaxation_distance_ ? rd.first : throughput;
			}
		}

		for (auto& [rb, sum] : results) {
			std::print("k-relaxation: {:5}, avg: {:.2f}\n", rb, sum / num_repeat);
		}
		std::print("\n");
	}

	void Tester::RunMacroBenchmarkScalingWithThread(int num_repeat)
	{
		constexpr auto kMaxThread{ kNumThreads.back() };
		threads_.reserve(kMaxThread);

		Stopwatch stopwatch;

		std::map<int, BFSResult> results{};
		for (auto num_thread : kNumThreads) {
			results.insert(std::make_pair(num_thread, BFSResult{}));
		}

		graph_->PrintStatus();

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("------ {}/{} ------\n", i, num_repeat);

			for (auto num_thread : kNumThreads) {
				threads_.clear();
				std::vector<int> shortest_dists(num_thread);
				graph_->Reset();

				switch (subject_) {
					case Subject::kTSCAS: {
						lf::ts_cas::TSCAS subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSStutter: {
						lf::ts_stutter::TSStutter subject{ num_thread };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSAtomic: {
						lf::ts_atomic::TSAtomic subject{ num_thread };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSInterval: {
						lf::ts_interval::TSInterval subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kCBO: {
						lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::k2Dd: {
						lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSWD: {
						lf::tswd::TSWD subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					default: {
						std::print("[Error] Invalid subject.\n");
						return;
					}
				}

				auto shortest_distance = *std::min_element(shortest_dists.begin(), shortest_dists.end());
				double elapsed_sec{ stopwatch.GetDuration() };

				std::print("          threads: {}\n", num_thread);
				std::print("      elapsed sec: {:.2f}\n", elapsed_sec);
				std::print("shortest distance: {}\n", shortest_distance);
				std::print("\n");

				results[num_thread].elapsed_sec += elapsed_sec;
				results[num_thread].shortest_distance += shortest_distance;
			}
		}

		auto actual_shortest_distance = graph_->GetShortestDistance();
		for (auto& [num_thread, sum] : results) {
			auto avg_error = (sum.shortest_distance / static_cast<double>(num_repeat)
				- actual_shortest_distance) / actual_shortest_distance * 100.0;

			std::print("threads: {:2}, avg elapsed sec: {:.2f}, avg error: {:.4f}%\n",
				num_thread, sum.elapsed_sec / num_repeat, avg_error);
		}
		std::print("\n");
	}

	void Tester::RunMacroBenchmarkScalingWithDepth(int num_repeat)
	{
		constexpr auto kNumThread{ 41 };
		threads_.reserve(kNumThread);

		Stopwatch stopwatch;

		std::map<int, BFSResult> results{};
		for (auto rb : kRelaxationBounds) {
			results.insert(std::make_pair(rb, BFSResult{}));
		}

		graph_->PrintStatus();

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("------ {}/{} ------\n", i, num_repeat);

			for (auto rb : kRelaxationBounds) {
				threads_.clear();
				std::vector<int> shortest_dists(kNumThread);
				graph_->Reset();

				switch (subject_) {
					case Subject::k2Dd: {
						auto depth = rb / (kNumThread - 1);
						lf::twodd::TwoDd subject{ kNumThread, kNumThread, depth };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, kNumThread, subject, shortest_dists);
						break;
					}
					case Subject::kTSWD: {
						auto depth = rb / (kNumThread - 1) - 1;
						lf::tswd::TSWD subject{ kNumThread, depth };
						stopwatch.Start();
						CreateThreads(MacrobenchmarkFunc, kNumThread, subject, shortest_dists);
						break;
					}
					default: {
						std::print("[Error] Invalid subject. 'Scaling with depth' mode is only for 2Dd or TSWD.\n\n");
						return;
					}
				}

				auto shortest_distance = *std::min_element(shortest_dists.begin(), shortest_dists.end());
				double elapsed_sec{ stopwatch.GetDuration() };

				std::print("          threads: {}\n", kNumThread);
				std::print("      elapsed sec: {:.2f}\n", elapsed_sec);
				std::print("shortest distance: {}\n", shortest_distance);
				std::print("\n");

				results[rb].elapsed_sec += elapsed_sec;
				results[rb].shortest_distance += shortest_distance;
			}
		}

		auto actual_shortest_distance = graph_->GetShortestDistance();
		for (auto& [rb, sum] : results) {
			auto avg_error = (sum.shortest_distance / static_cast<double>(num_repeat)
				- actual_shortest_distance) / actual_shortest_distance * 100.0;

			std::print("k-relaxation: {:5}, avg elapsed sec: {:.2f}, avg error: {:.4f}%\n",
				rb, sum.elapsed_sec / num_repeat, avg_error);
		}
		std::print("\n");
	}

	void Tester::SetSubject()
	{
		//kNone, kTSCAS, kTSStutter, kTSAtomic, kTSInterval, kCBO, k2Dd, kTSWD

		std::print("\n--- List ---\n");
		std::print("1: TS-CAS, 2: TS-stutter, 3: TS-atomic, 4: TS-interval,\n");
		std::print("5: d-CBO, 6: 2Dd, 7: TSWD\n");
		std::print("Subject: ");
		int subject_id;
		std::cin >> subject_id;
		subject_ = static_cast<Subject>(subject_id);
		std::cin.ignore();
	}

	void Tester::SetParameter()
	{
		std::print("\n--- List ---\n");;
		std::print("     TS-cas: [parameter] = delay (microsec)\n");
		std::print("TS-interval: [parameter] = delay (microsec)\n");
		std::print("      d-CBO: [parameter] = d\n");
		std::print("        2Dd: [parameter] = depth\n");
		std::print("       TSWD: [parameter] = depth\n");
		std::print("Parameter: ");
		std::cin >> parameter_;

		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}
	}

	void Tester::SetEnqRate()
	{
		std::print("Enqueue rate(%): ");
		std::cin >> enq_rate_;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}
	}

	void Tester::CheckRelaxationDistance()
	{
		checks_relaxation_distance_ ^= true;
		if (checks_relaxation_distance_) {
			std::print("Checks relaxation distance.\n");
		} else {
			std::print("Checks throughput.\n");
		}
	}

	void Tester::ScaleWithDepth()
	{
		scales_with_depth_ ^= true;
		if (scales_with_depth_) {
			std::print("Scales with depth.\n");
		} else {
			std::print("Scales with thread.\n");
		}
	}

	void Tester::GenerateGraph()
	{
		std::print("\n--- List ---\n");
		std::print("0: Alpha, 1: Beta, 2: Gamma, 3: Delta, 4: Epsilon, 5: Zeta\n");
		std::print("Graph Type: ");

		int graph_type;
		std::cin >> graph_type;
		std::cin.ignore();

		if (graph_type < 0 or graph_type >= static_cast<int>(Graph::Type::kSize)) {
			std::print("[Error] Invalid graph type.\n");
			return;
		}

		std::print("Generating the graph. Please wait a moment.\n");

		graph_ = std::make_unique<Graph>(static_cast<Graph::Type>(graph_type));
		graph_->Save(std::format("graph{}.bin", graph_type));
	}

	void Tester::LoadGraph()
	{
		std::print("\n--- List ---\n");
		std::print("0: Alpha, 1: Beta, 2: Gamma, 3: Delta, 4: Epsilon, 5: Zeta\n");
		std::print("Graph Type: ");

		int graph_type;
		std::cin >> graph_type;
		std::cin.ignore();

		if (graph_type < 0 or graph_type >= static_cast<int>(Graph::Type::kSize)) {
			std::print("[Error] Invalid graph type.\n");
			return;
		}

		graph_ = std::make_unique<Graph>(std::format("graph{}.bin", graph_type));
		
		if (not graph_->IsValid()) {
			graph_ = nullptr;
		}
	}

	void Tester::PrintHelp() const
	{
		std::print("e: Set enqueue rate\n");
		std::print("m: Toggle microbenchmark mode (throughput/relaxation)\n");
		std::print("c: Toggle scaling mode (thread/depth)\n");
		std::print("s: Set subject\n");
		std::print("p: Set parameter\n");
		std::print("l: Load graph\n");
		std::print("g: Generate graph\n");
		std::print("i: Microbenchmark\n");
		std::print("a: Macrobenchmark\n");
		std::print("h: Help\n");
		std::print("q: Quit\n");
		std::print("\n");
	}
}