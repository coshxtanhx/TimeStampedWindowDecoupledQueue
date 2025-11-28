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
			PRINT("Command ('h' for help): ");
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
				case 'w': {
					SetWidth();
					break;
				}
				case 'd': {
					SetDelay();
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
		if (not HasValidParameter()) {
			return;
		}

		PRINT("Input the number of times to repeat: ");
		auto num_repeat{ InputNumber<int>() };
		if (0 == num_repeat) {
			return;
		}

		results.clear();

		for (int i = 1; i <= num_repeat; ++i) {
			PRINT("---------- {}/{} ----------\n", i, num_repeat);
			if (scales_with_depth_) {
				if (false == RunMicroBenchmarkScalingWithDepth()) {
					return;
				}
			} else {
				if (false == RunMicroBenchmarkScalingWithThread()) {
					return;
				}
			}
		}
		results.PrintResult(checks_relaxation_distance_, scales_with_depth_, kTotalNumOp);
		results.Save(checks_relaxation_distance_, scales_with_depth_, enq_rate_, subject_, parameter_, width_);
	}

	void Tester::RunMacroBenchmark()
	{
		if (not HasValidParameter()) {
			return;
		}

		if (nullptr == graph_) {
			PRINT("[Error] Generate or load graph first.\n\n");
			return;
		}
		
		PRINT("Input the number of times to repeat: ");
		auto num_repeat{ InputNumber<int>() };
		if (0 == num_repeat) {
			return;
		}

		results.clear();
		graph_->PrintStatus();

		for (int i = 1; i <= num_repeat; ++i) {
			PRINT("---------- {}/{} ----------\n", i, num_repeat);
			if (scales_with_depth_) {
				if (false == RunMacroBenchmarkScalingWithDepth()) {
					return;
				}
			} else {
				if (false == RunMacroBenchmarkScalingWithThread()) {
					return;
				}
			}
		}
		results.PrintResult(scales_with_depth_, graph_->GetShortestDistance());
		results.Save(scales_with_depth_, graph_->GetType(), subject_, parameter_, width_);
	}

	bool Tester::RunMicroBenchmarkScalingWithThread()
	{
		for (auto num_thread : kNumThreads) {
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
					auto width = 0 == width_ ? num_thread : width_;
					lf::cbo::CBO subject{ width, num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::k2Dd: {
					auto width = 0 == width_ ? num_thread : width_;
					lf::twodd::TwoDd subject{ width, num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSWD: {
					lf::tswd::TSWD subject{ num_thread, parameter_ };
					Measure(MicrobenchmarkFunc, num_thread, subject);
					break;
				}
				default: {
					PRINT("[Error] Invalid subject.\n\n");
					return false;
				}
			}
		}
		return true;
	}

	bool Tester::RunMicroBenchmarkScalingWithDepth()
	{
		constexpr int32_t kMinRelaxationBound{ 320 };
		constexpr int32_t kMaxRelaxationBound{ kMinRelaxationBound << 5 };

		for (auto rb = kMinRelaxationBound; rb <= kMaxRelaxationBound; rb *= 2) {
			switch (subject_) {
				case Subject::k2Dd: {
					auto width = 0 == width_ ? kFixedNumThread : width_;
					auto depth = rb / (width - 1);
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
					PRINT("[Error] Invalid subject. 'Scaling with depth' mode is only for 2Dd or TSWD.\n\n");
					return false;
				}
			}
		}
		return true;
	}

	bool Tester::RunMacroBenchmarkScalingWithThread()
	{
		for (auto num_thread : kNumThreads) {
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
					auto width = 0 == width_ ? num_thread : width_;
					lf::cbo::CBO subject{ width, num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::k2Dd: {
					auto width = 0 == width_ ? num_thread : width_;
					lf::twodd::TwoDd subject{ width, num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				case Subject::kTSWD: {
					lf::tswd::TSWD subject{ num_thread, parameter_ };
					Measure(MacrobenchmarkFunc, num_thread, subject);
					break;
				}
				default: {
					PRINT("[Error] Invalid subject.\n");
					return false;
				}
			}
		}
		return true;
	}

	bool Tester::RunMacroBenchmarkScalingWithDepth()
	{
		constexpr int32_t kMinRelaxationBound{ 640 };
		constexpr int32_t kMaxRelaxationBound{ kMinRelaxationBound << 6 };

		for (auto rb = kMinRelaxationBound; rb <= kMaxRelaxationBound; rb *= 2) {
			graph_->Reset();

			switch (subject_) {
				case Subject::k2Dd: {
					auto width = 0 == width_ ? kFixedNumThread : width_;
					auto depth = rb / (width - 1);
					lf::twodd::TwoDd subject{ width, kFixedNumThread, depth };
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
					PRINT("[Error] Invalid subject. 'Scaling with depth' mode is only for 2Dd or TSWD.\n\n");
					return false;
				}
			}
		}
		return true;
	}

	void Tester::SetSubject()
	{
		//kNone, kTSCAS, kTSStutter, kTSAtomic, kTSInterval, kCBO, k2Dd, kTSWD

		PRINT("\n--- List ---\n");
		PRINT("1: TS-CAS, 2: TS-stutter, 3: TS-atomic, 4: TS-interval,\n");
		PRINT("5: d-CBO, 6: 2Dd, 7: TSWD\n");
		PRINT("Subject: ");
		subject_ = static_cast<Subject>(InputNumber<int>());
	}

	void Tester::SetParameter()
	{
		PRINT("\n--- List ---\n");;
		PRINT("     TS-cas: [parameter] = delay (microsec)\n");
		PRINT("TS-interval: [parameter] = delay (microsec)\n");
		PRINT("      d-CBO: [parameter] = d\n");
		PRINT("        2Dd: [parameter] = depth\n");
		PRINT("       TSWD: [parameter] = depth\n");
		PRINT("Parameter: ");
		parameter_ = InputNumber<int>();
	}

	void Tester::SetEnqRate()
	{
		PRINT("Enqueue rate(%): ");
		enq_rate_ = InputNumber<float>();
	}

	void Tester::SetWidth()
	{
		PRINT("Input width (0 = use nbr thread): ");
		width_ = InputNumber<int>();
	}

	void Tester::SetDelay()
	{
		PRINT("Set delay (microsec): ");
		delay_ = InputNumber<float>();
	}

	void Tester::CheckRelaxationDistance()
	{
		checks_relaxation_distance_ ^= true;
		if (checks_relaxation_distance_) {
			PRINT("Checks relaxation distance.\n");
		} else {
			PRINT("Checks throughput.\n");
		}
	}

	void Tester::ScaleWithDepth()
	{
		scales_with_depth_ ^= true;
		if (scales_with_depth_) {
			PRINT("Scales with depth.\n");
		} else {
			PRINT("Scales with thread.\n");
		}
	}

	void Tester::GenerateGraph()
	{
		PRINT("\n--- List ---\n");
		PRINT("1: Alpha, 2: Beta, 3: Gamma, 4: Delta, 5: Epsilon, 6: Zeta\n");
		PRINT("Graph Type: ");

		auto graph_type{ InputNumber<int>() };

		if (graph_type < static_cast<int>(Graph::Type::kAlpha)
			or graph_type > static_cast<int>(Graph::Type::kZeta)) {
			PRINT("[Error] Invalid graph type.\n");
			return;
		}

		PRINT("Generating the graph. Please wait a moment.\n");

		graph_ = std::make_unique<Graph>(static_cast<Graph::Type>(graph_type), Graph::Option::kGenerate);
		graph_->Save();
	}

	void Tester::LoadGraph()
	{
		PRINT("\n--- List ---\n");
		PRINT("1: Alpha, 2: Beta, 3: Gamma, 4: Delta, 5: Epsilon, 6: Zeta\n");
		PRINT("Graph Type: ");

		auto graph_type{ InputNumber<int>() };

		if (graph_type < static_cast<int>(Graph::Type::kAlpha)
			or graph_type > static_cast<int>(Graph::Type::kZeta)) {
			PRINT("[Error] Invalid graph type.\n");
			return;
		}

		graph_ = std::make_unique<Graph>(static_cast<Graph::Type>(graph_type), Graph::Option::kLoad);
		
		if (not graph_->IsValid()) {
			graph_ = nullptr;
		}
	}

	void Tester::PrintHelp() const
	{
		PRINT("e: Set enqueue rate\n");
		PRINT("m: Toggle microbenchmark mode (throughput/relaxation)\n");
		PRINT("c: Toggle scaling mode (thread/depth)\n");
		PRINT("s: Set subject\n");
		PRINT("p: Set parameter\n");
		PRINT("w: Set width\n");
		PRINT("d: Set delay\n");
		PRINT("l: Load graph\n");
		PRINT("g: Generate graph\n");
		PRINT("i: Microbenchmark\n");
		PRINT("a: Macrobenchmark\n");
		PRINT("h: Help\n");
		PRINT("q: Quit\n");
		PRINT("\n");
	}

	bool Tester::HasValidParameter() const
	{
		if (width_ < 0) {
			PRINT("[Error] Invalid width.\n");
			return false;
		}

		switch (subject_)
		{
			case Subject::kCBO: {
				auto min_width{ width_ };
				if (0 == width_) {
					if (scales_with_depth_) {
						min_width = kFixedNumThread;
					} else {
						min_width = kNumThreads.front();
					}
				}

				if (parameter_ <= 0 or parameter_ > min_width) {
					PRINT("[Error] Invalid d.\n");
					return false;
				}
				break;
			}
			case Subject::k2Dd: {
				if (parameter_ <= 0 and not scales_with_depth_) {
					PRINT("[Error] Invalid depth.\n");
					return false;
				}
				break;
			}
			case Subject::kTSWD: {
				if (parameter_ <= 0 and not scales_with_depth_) {
					PRINT("[Error] Invalid depth.\n");
					return false;
				}
				break;
			}
		}

		return true;
	}
}