#include <iostream>
#include "benchmark_tester.h"

int main()
{
	benchmark::Tester tester;

	bool is_terminated{};
	while (not is_terminated) {
		std::cout << "Input Command: ";
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

			while (num_repeat--) {
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
			is_terminated = true;
			break;
		default:
			break;
		}
	}
}