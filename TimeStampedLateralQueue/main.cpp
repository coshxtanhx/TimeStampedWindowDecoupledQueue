#include <iostream>
#include "benchmark_tester.h"
#include "print.h"

int main()
{
	MyThread::SetID(MyThread::kMainThreadID);

	benchmark::Tester tester;

	while (true) {
		std::print("Input command: ");
		std::string cmd;
		std::cin >> cmd;
		std::cin.ignore();

		switch (cmd.front())
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