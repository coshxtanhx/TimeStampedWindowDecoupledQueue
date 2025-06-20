#include <iostream>
#include "benchmark_tester.h"
#include "print.h"
#include "my_thread.h"

#include "fixed_random.h"

int main()
{
	MyThread::SetID(MyThread::kMainThreadID);

	benchmark::Tester tester;
	tester.Run();
}