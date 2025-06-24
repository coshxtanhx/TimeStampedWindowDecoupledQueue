#include <iostream>
#include "benchmark_tester.h"
#include "print.h"
#include "my_thread_id.h"

int main()
{
	MyThreadID::Set(MyThreadID::kMainThreadID);

	benchmark::Tester tester;
	tester.Run();
}