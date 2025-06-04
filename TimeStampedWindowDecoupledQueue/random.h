#ifndef RANDOM_H
#define RANDOM_H

#include <random>

class Random {
public:
	Random() noexcept = default;

	template<class T> requires std::floating_point<T>
	static T Get(T min, T max) noexcept {
		auto rand_num = static_cast<double>(uid_(dre_));
		rand_num /= std::numeric_limits<long long>::max();

		return min + static_cast<T>((max - min) * (rand_num));
	}

	template<class T1, class T2> requires std::integral<T1> and std::integral<T2>
	static T1 Get(T1 min, T2 max) noexcept {
		auto rand_num = uid_(dre_);

		return min + static_cast<T1>(rand_num % (max - min + 1));
	}
private:
	static thread_local std::random_device rd_;
	static thread_local std::default_random_engine dre_;
	static thread_local std::uniform_int_distribution<long long> uid_;
};

inline thread_local std::random_device Random::rd_;
inline thread_local std::default_random_engine Random::dre_{ Random::rd_() };
inline thread_local std::uniform_int_distribution<long long> Random::uid_;

#endif