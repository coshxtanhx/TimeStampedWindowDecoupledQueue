#ifndef MY_THREAD_H
#define MY_THREAD_H

#include <limits>
#include "print.h"

class MyThread {
public:
	static constexpr int kMainThreadID{ -1 };

	static void SetID(int id) {
		if (kUndefinedThreadID != id_) {
			std::print("[Error] Thread ID has already been assigned.\n");
			std::exit(getchar());
		}
		id_ = id;
	}
	static int GetID() {
		if (kUndefinedThreadID == id_) {
			std::print("[Error] Must register thread ID first.\n");
			exit(getchar());
		}
		return id_;
	}
private:
	static constexpr int kUndefinedThreadID{ std::numeric_limits<int>::min() };
	static thread_local int id_;
};

inline thread_local int MyThread::id_{ MyThread::kUndefinedThreadID };

#endif