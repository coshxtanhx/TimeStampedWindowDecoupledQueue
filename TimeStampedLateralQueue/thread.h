#ifndef THREAD_H
#define THREAD_H

#include <iostream>

namespace thread {
	inline constexpr int kUndefinedThreadID{ -1 };

	inline int ID(int id_to_register = kUndefinedThreadID) noexcept {
		static thread_local const int id{ id_to_register };

		if (kUndefinedThreadID == id) {
			std::cout << "[Error] Must Register Thread ID First.\n";
			exit(getchar());
		}

		return id;
	}
}

#endif