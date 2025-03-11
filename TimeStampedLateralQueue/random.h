#ifndef RANDOM_H
#define RANDOM_H

#include <random>

class Random {
public:
	Random() noexcept : rd_{}, dre_{ rd_() }, uid_{} {}

	template<class T1, class T2>
	T1 Get(T1 min, T2 max) noexcept {
		auto rand_num = uid_(dre_);

		return min + static_cast<T1>(rand_num % (max - min + 1));
	}
private:
	std::random_device rd_;
	std::default_random_engine dre_;
	std::uniform_int_distribution<long long> uid_;
};

thread_local Random rng;

#endif