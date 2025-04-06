#include <iostream>
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
		case 'q':
			return 0;
		default:
			break;
		}
	}
}