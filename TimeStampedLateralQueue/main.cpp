#include <iostream>
#include <format>
#include "benchmark_tester.h"

int main()
{
	benchmark::Tester tester;

	bool has_terminated{};
	while (not has_terminated) {
		std::cout << "Input command: ";
		char cmd;
		std::cin >> cmd;
		std::cin.ignore();

		switch (cmd)
		{
		case 's':
			tester.SetSubject();
			break;
		case 'p':
			tester.SetParameter();
			break;
		case 'i':
			tester.StartMicroBenchmark();
			break;
		case 'a': {
			std::cout << "Input the number of times to repeat: ";
			int num_repeat;
			std::cin >> num_repeat;

			for (int i = 1; i <= num_repeat; ++i) {
				std::cout << std::format("--- {}/{} ---\n", i, num_repeat);
				tester.StartMacroBenchmark();
			}
			break;
		}
		case 'g':
			tester.GenerateGraph();
			break;
		case 'l':
			tester.LoadGraph();
			break;
		case 'q':
			has_terminated = true;
			break;
		default:
			break;
		}
	}
}