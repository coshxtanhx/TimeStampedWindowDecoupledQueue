#include <iostream>
#include "stopwatch.hh"
#include "random.hh"

int main()
{
	for (int i = 0; i < 5; ++i){
		std::cout << rng.Get(0, 5) << '\n';
	}

	Stopwatch sw;
	sw.Start();

	while (sw.GetDuration() <= 3.0){}

	std::cout << sw.GetDuration() << '\n';

	getchar();
}