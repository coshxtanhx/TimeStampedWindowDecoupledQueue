#include <iostream>
#include "benchmark_tester.h"

int main()
{
	benchmark::Tester tester;
	tester.SetParameter();

	tester.StartMicroBenchmark();
}