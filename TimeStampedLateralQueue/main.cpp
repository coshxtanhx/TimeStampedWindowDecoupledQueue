#include <iostream>
#include <format>
#include "benchmark_tester.h"

int main()
{
	MyThread::SetID(-1);

	benchmark::Tester tester;

	while (true) {
		std::cout << "Input command: ";
		char cmd;
		std::cin >> cmd;
		std::cin.ignore();

		switch (cmd)
		{
		case 'c':
			tester.SetContention();
			break;
		case 'e':
			tester.SetEnqRate();
			break;
		case 'r':
			tester.CheckRelaxationDistance();
			break;
		case 's':
			tester.SetSubject();
			break;
		case 'p':
			tester.SetParameter();
			break;
		case 'i':
			std::cout << "Input the number of times to repeat: ";
			int num_repeat;
			std::cin >> num_repeat;
			for (int i = 1; i <= num_repeat; ++i) {
				std::cout << std::format("------ {}/{} ------\n", i, num_repeat);
				tester.StartMicroBenchmark();
			}
			break;
		case 'a': {
			std::cout << "Input the number of times to repeat: ";
			int num_repeat;
			std::cin >> num_repeat;
			for (int i = 1; i <= num_repeat; ++i) {
				std::cout << std::format("------ {}/{} ------\n", i, num_repeat);
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
			return 0;
		default:
			break;
		}
	}
}