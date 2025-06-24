#include "benchmark_tester.h"
#include "cbo.h"
#include "ts_interval.h"
#include "ts_cas.h"
#include "ts_atomic.h"
#include "ts_stutter.h"
#include "twodd.h"
#include "tswd.h"

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

		results.clear();
		for (int i = 1; i <= num_repeat; ++i) {
			std::print("---------- {}/{} ----------\n", i, num_repeat);
			if (scales_with_depth_) {
				RunMicroBenchmarkScalingWithDepth();
			} else {
				RunMicroBenchmarkScalingWithThread();
			}
		}
		results.PrintMicrobenchmarkResult(checks_relaxation_distance_, scales_with_depth_, kTotalNumOp);
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

		results.clear();
		graph_->PrintStatus();

		for (int i = 1; i <= num_repeat; ++i) {
			std::print("---------- {}/{} ----------\n", i, num_repeat);
			if (scales_with_depth_) {
				RunMacroBenchmarkScalingWithDepth();
			} else {
				RunMacroBenchmarkScalingWithThread();
			}
		}
		results.PrintMacrobenchmarkResult(scales_with_depth_, graph_->GetShortestDistance());
	}

	void Tester::RunMicroBenchmarkScalingWithThread()
	{
		constexpr auto kMinThread{ 9 };
		constexpr auto kMaxThread{ 72 };

		for (int num_thread = kMinThread; num_thread <= kMaxThread; num_thread *= 2) {
			switch (subject_) {
				case Subject::kTSCAS: {
					lf::ts_cas::TSCAS subject{ num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSStutter: {
					lf::ts_atomic::TSAtomic subject{ num_thread };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSAtomic: {
					lf::ts_atomic::TSAtomic subject{ num_thread };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSInterval: {
					lf::ts_interval::TSInterval subject{ num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kCBO: {
					lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::k2Dd: {
					lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSWD: {
					lf::tswd::TSWD subject{ num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				default: {
					std::print("[Error] Invalid subject.\n\n");
					return;
				}
			}
		}
	}

	void Tester::RunMicroBenchmarkScalingWithDepth()
	{
		constexpr int kMinRelaxationBound{ 350 };
		constexpr int kMaxRelaxationBound{ 11200 };

		for (auto rb = kMinRelaxationBound; rb <= kMaxRelaxationBound; rb *= 2) {
			switch (subject_) {
				case Subject::k2Dd: {
					int depth, width;
					while (true) {
						int d1{ 10 }, w1{ 35 }, d2{ 14 }, w2{ 50 };
						if (d1 * w1 == rb) {
							depth = d1;
							width = w1;
							break;
						}
						if (d2 * w2 == rb) {
							depth = d2;
							width = w2;
							break;
						}
						d1 *= 2;
						d2 *= 2;
						w1 *= 2;
						w2 *= 2;
					}

					lf::twodd::TwoDd subject{ width, kFixedNumThread, depth };
					Measure(MicrobenchmarkFunc, rb, subject);
					break;
				}
				case Subject::kTSWD: {
					auto depth = rb / (kFixedNumThread - 1) - 1;
					lf::tswd::TSWD subject{ kFixedNumThread, depth };
					Measure(MicrobenchmarkFunc, rb, subject);
					break;
				}
				default: {
					std::print("[Error] Invalid subject. 'Scaling with depth' mode is only for 2Dd or TSWD.\n\n");
					return;
				}
			}
		}
	}

	void Tester::RunMacroBenchmarkScalingWithThread()
	{
		constexpr auto kMinThread{ 9 };
		constexpr auto kMaxThread{ 72 };

		for (int num_thread = kMinThread; num_thread <= kMaxThread; num_thread *= 2) {
			switch (subject_) {
				case Subject::kTSCAS: {
					lf::ts_cas::TSCAS subject{ num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSStutter: {
					lf::ts_stutter::TSStutter subject{ num_thread };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSAtomic: {
					lf::ts_atomic::TSAtomic subject{ num_thread };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSInterval: {
					lf::ts_interval::TSInterval subject{ num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kCBO: {
					lf::cbo::CBO subject{ num_thread, num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::k2Dd: {
					lf::twodd::TwoDd subject{ num_thread, num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSWD: {
					lf::tswd::TSWD subject{ num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				default: {
					std::print("[Error] Invalid subject.\n");
					return;
				}
			}
		}
	}

	void Tester::RunMacroBenchmarkScalingWithDepth()
	{
		constexpr int kMinRelaxationBound{ 560 };
		constexpr int kMaxRelaxationBound{ kMinRelaxationBound << 6 };

		for (auto rb = kMinRelaxationBound; rb <= kMaxRelaxationBound; rb *= 2) {
			graph_->Reset();

			switch (subject_) {
				case Subject::k2Dd: {
					int width, depth;
					while (true) {
						int d1{ 14 }, w1{ 40 }, d2{ 20 }, w2{ 56 };
						if (d1 * w1 == rb) {
							depth = d1;
							width = w1;
							break;
						}
						if (d2 * w2 == rb) {
							depth = d2;
							width = w2;
							break;
						}
						d1 *= 2;
						d2 *= 2;
						w1 *= 2;
						w2 *= 2;
					}
					lf::twodd::TwoDd subject{ kFixedNumThread, kFixedNumThread, depth };
					Measure(MacrobenchmarkFunc, rb, subject);
					break;
				}
				case Subject::kTSWD: {
					auto depth = rb / (kFixedNumThread - 1) - 1;
					lf::tswd::TSWD subject{ kFixedNumThread, depth };
					Measure(MacrobenchmarkFunc, rb, subject);
					break;
				}
				default: {
					std::print("[Error] Invalid subject. 'Scaling with depth' mode is only for 2Dd or TSWD.\n\n");
					return;
				}
			}
		}
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