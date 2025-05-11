#include <sstream>
#include "benchmark_tester.h"
#include "microbenchmark_thread_func.h"
#include "macrobenchmark_thread_func.h"
#include "stopwatch.h"
#include "dqrr.h"
#include "cbo.h"
#include "dqlru.h"
#include "ts_interval.h"
#include "twodd.h"
#include "tswd.h"
#include "tswd2.h"

namespace benchmark {
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
				case 'r': {
					CheckRelaxationDistance();
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
					StartMicroBenchmark();
					break;
				}
				case 'a': {
					if (graph_) {
						StartMacroBenchmark();
					} else {
						std::print("[Error] Generate or load graph first.\n\n");
					}
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
					std::print("e: Set enq rate\n");
					std::print("r: Check/Do not check relaxation distance\n");
					std::print("s: Set subject\n");
					std::print("p: Set parameter\n");
					std::print("l: Load graph\n");
					std::print("g: Generate graph\n");
					std::print("i: Microbenchmark\n");
					std::print("a: Macrobenchmark\n");
					std::print("h: help\n");
					std::print("q: quit\n");
					std::print("\n");
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

	void Tester::StartMicroBenchmark()
	{
		constexpr auto kMaxThread{ 72 };
		threads_.reserve(kMaxThread);
		Stopwatch stopwatch;

		std::map<int, double> results{};

		std::print("Input the number of times to repeat: ");
		int num_repeat;
		std::cin >> num_repeat;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("------ {}/{} ------\n", i, num_repeat);
			for (int num_thread = 9; num_thread <= kMaxThread; num_thread *= 2) {
				threads_.clear();
				std::pair<double, uint64_t> rd{};
				double elapsed_sec{};

				switch (subject_) {
					case Subject::kLRU: {
						//lf::dqlru::DQLRU subject{ num_thread * parameter_, num_thread };
						lf::tswd2::TSWD subject{ num_thread, parameter_ };
						stopwatch.Start();
						CreateThreads(MicrobenchmarkFunc, num_thread, subject);
						elapsed_sec = stopwatch.GetDuration();
						rd = subject.GetRelaxationDistance();
						break;
					}
					case Subject::kRR: {
						lf::dqrr::DQRR subject{ num_thread * parameter_, num_thread, 1 };
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
					case Subject::kTSInterval: {
						lf::ts::TSInterval subject{ num_thread, parameter_ };
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

				//std::this_thread::sleep_for(std::chrono::seconds(1));

				auto throughput = kTotalNumOp / elapsed_sec / 1e6;

				std::print("    threads: {}\n", num_thread);
				std::print("elapsed sec: {:.2f} s\n", elapsed_sec);
				std::print(" throughput: {:.2f} MOp/s\n", throughput);
				std::print("   avg dist: {:.2f}\n", rd.first);
				std::print("   max dist: {}\n", rd.second);
				std::print("\n");

				results[num_thread] += checks_relaxation_distance_ ? rd.first : throughput;
			}
		}

		for (auto& [num_thread, sum] : results) {
			std::print("threads: {:2}, avg: {:.2f}\n", num_thread, sum / num_repeat);
		}
		std::print("\n");
	}

	void Tester::StartMacroBenchmark()
	{
		constexpr auto kMaxThread{ 72 };
		threads_.reserve(kMaxThread);

		Stopwatch stopwatch;

		std::map<int, double> results{};

		std::print("Input the number of times to repeat: ");
		int num_repeat;
		std::cin >> num_repeat;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("------ {}/{} ------\n", i, num_repeat);

			for (int num_thread = 9; num_thread <= kMaxThread; num_thread *= 2) {
				threads_.clear();
				std::vector<int> shortest_dists(num_thread);
				graph_->Reset();

				stopwatch.Start();

				switch (subject_) {
					case Subject::kLRU: {
						lf::dqlru::DQLRU subject{ num_thread * parameter_, num_thread };
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kRR: {
						lf::dqrr::DQRR subject{ num_thread * parameter_, num_thread, num_thread };
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kCBO: {
						lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSInterval: {
						lf::ts::TSInterval subject{ num_thread, parameter_ };
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::k2Dd: {
						lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					case Subject::kTSWD: {
						lf::tswd::TSWD subject{ num_thread, parameter_ };
						CreateThreads(MacrobenchmarkFunc, num_thread, subject, shortest_dists);
						break;
					}
					default: {
						std::print("[Error] Invalid subject.\n");
						return;
					}
				}

				double elapsed_sec{ stopwatch.GetDuration() };
				auto shortest_distance = *std::min_element(shortest_dists.begin(), shortest_dists.end());

				std::print("          threads: {}\n", num_thread);
				std::print("      elapsed sec: {:.2f} s\n", elapsed_sec);
				std::print("shortest distance: {}\n", shortest_distance);
				std::print("\n");

				results[num_thread] += elapsed_sec;
			}
		}

		for (auto& [num_thread, sum] : results) {
			std::print("threads: {:2}, avg: {:.2f}\n", num_thread, sum / num_repeat);
		}
		std::print("\n");
	}

	void Tester::SetSubject() {
		std::print("1: LRU, 2: TL-RR, 3: d-CBO, 4: TS-interval, 5: 2Dd, 6: TSWd\n");
		std::print("Subject: ");
		int subject_id;
		std::cin >> subject_id;
		subject_ = static_cast<Subject>(subject_id);
		std::cin.ignore();
	}

	void Tester::SetParameter() {
		std::print("\n--- List ---\n");
		std::print("        LRU: [parameter] = nbr_queue / nbr_thread\n");
		std::print("      TL-RR: [parameter] = nbr_queue / nbr_thread\n");
		std::print("      d-CBO: [parameter] = d\n");
		std::print("TS-interval: [parameter] = delay\n");
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

	void Tester::SetEnqRate() {
		std::print("Enq rate(%): ");
		std::cin >> enq_rate_;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		} else {
			std::cin.ignore();
		}
	}

	void Tester::CheckRelaxationDistance() {
		checks_relaxation_distance_ ^= true;
		if (checks_relaxation_distance_) {
			std::print("Checks relaxation distance.\n");
		} else {
			std::print("Does not check relaxation distance.\n");
		}
	}

	void Tester::GenerateGraph() {
		if (nullptr != graph_) {
			std::print("[Error] Graph has already been generated!\n");
			return;
		}
		graph_ = std::make_unique<Graph>(18'000'000);
		graph_->Save("graph.bin");
	}

	void Tester::LoadGraph() {
		if (nullptr != graph_) {
			std::print("[Error] Graph has already been generated!\n");
			return;
		}
		graph_ = std::make_unique<Graph>("graph.bin");
	}
}