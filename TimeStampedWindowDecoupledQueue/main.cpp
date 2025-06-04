#include <iostream>
#include "benchmark_tester.h"
#include "print.h"
#include "my_thread.h"

int main()
{
	MyThread::SetID(MyThread::kMainThreadID);

	benchmark::Tester tester;
	tester.Run();
}