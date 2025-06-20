#ifndef FIXED_RANDOM_H
#define FIXED_RANDOM_H

#include <stdint.h>

class FixedRandom {
public:
	FixedRandom() noexcept = default;
	auto Get() {
		uint32_t r{};
		switch (a_ % 3) {
			case 0: {
				r = a_++ - b_ * ++c_ + d_;
				b_ += ++c_;
				c_ += d_++ * 2;
				r += a_ * b_ * c_ * d_;
				break;
			}

			case 1: {
				r = ++a_ - b_ * c_++ + d_;
				b_ += ++d_;
				c_ += b_++ * (a_ % 4 + 1);
				r -= a_ * b_ * c_ * d_;
				break;
			}

			case 2: {
				r = a_++ - d_ * ++b_ + c_;
				d_ += ++c_;
				c_ += ++b_ * (a_ - d_);
				r -= a_ * c_ + b_ * d_;
				break;
			}
		}

		return r;
	}
	
private:
	uint32_t a_{ 12345678 }, b_{ 12108642 }, c_{ 12481632 }, d_{ 13972181 };
};

#endif