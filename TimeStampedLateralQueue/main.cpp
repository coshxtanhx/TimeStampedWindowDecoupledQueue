#include <iostream>
#include "benchmark_tester.h"

int main()
{
	benchmark::Tester tester;

	while (true) {
		std::cout << "Input Command: ";
		char cmd;
		std::cin >> cmd;
		std::cin.ignore();

		switch (cmd)
		{
		case 'p':
			tester.SetParameter();
			break;
		case 'i':
			tester.StartMicroBenchmark();
			break;
		case 'a':
			tester.StartMacroBenchmark();
			break;
		case 'g':
			tester.GenerateGraph();
			break;
		case 'l':
			tester.LoadGraph();
			break;
		default:
			break;
		}
	}
}