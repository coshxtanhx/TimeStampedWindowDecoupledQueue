#ifndef BENCHMARK_SETTING_HH
#define BENCHMARK_SETTING_HH

#include <fstream>
#include <stdio.h>

namespace benchmark{
    struct MicroBenchmarkSetting{
		MicroBenchmarkSetting() noexcept {
			std::ifstream setting{ "../mibm.txt" };
			if (!setting){

			}
		}

		int contention;
        int num_op;
		int subject;
	};
}

#endif