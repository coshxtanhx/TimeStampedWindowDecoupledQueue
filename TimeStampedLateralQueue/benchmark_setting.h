#ifndef BENCHMARK_SETTING_H
#define BENCHMARK_SETTING_H

#include <fstream>
#include <array>
#include <stdio.h>

namespace benchmark{
	enum class Subject {
		kNone, kLRU, kRR, kRA, kTSInterval, k2Dd, kTSL
	};

    struct MicrobenchmarkSetting{
		MicrobenchmarkSetting() noexcept {
			std::ifstream setting{ "micro.txt" };
			if (!setting){
				printf("[Error] \"micro.txt\" does not exist.\n");
				return;
			}

			while (true){
				std::string line;
				setting >> line;
				if (line.front() == '#'){
					break;
				}
				if (line == "contention"){
					setting >> contention;
				}
				else if (line == "num_op"){
					setting >> num_op;
				}
				else if (line == "subject"){
					int subject_id;
					setting >> subject_id;
					subject = static_cast<Subject>(subject_id);
				}
			}
			printf("contention: %d / num_op: %d / subject: %s\n\n", contention, num_op, subject_names[static_cast<int>(subject)]);
		}

		static constexpr const char* subject_names[]{ "NONE", "LRU", "RR", "RA", "TS-interval", "2Dd", "TSL" };
		int contention{};
        int num_op{};
		Subject subject{};
	};
}

#endif