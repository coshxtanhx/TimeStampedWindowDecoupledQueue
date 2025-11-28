#ifndef MY_THREAD_ID_H
#define MY_THREAD_ID_H

#include <limits>
#include "print.h"

class MyThreadID {
public:
	static void Set(int id) {
		if (kUndefinedThreadID != id_) [[unlikely]] {
			PRINT("[Error] Thread ID has already been assigned.\n");
			std::exit(getchar());
		}
		id_ = id;
	}
	static int Get() {
		if (kUndefinedThreadID == id_) [[unlikely]] {
			PRINT("[Error] Must register thread ID first.\n");
			exit(getchar());
		}
		return id_;
	}

	static constexpr int kMainThreadID{ -1 };
private:
	static constexpr int kUndefinedThreadID{ std::numeric_limits<int>::min() };
	static thread_local int id_;
};

inline thread_local int MyThreadID::id_{ MyThreadID::kUndefinedThreadID };

#endif