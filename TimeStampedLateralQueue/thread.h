#ifndef THREAD_H
#define THREAD_H

#include <iostream>
#include <limits>

class MyThread {
public:
	static void SetID(int id) {
		if (kUndefinedThreadID != id_) {
			std::cout << "[Error] Thread ID has already been assigned.\n";
			std::exit(getchar());
		}
		id_ = id;
	}
	static int GetID() {
		if (kUndefinedThreadID == id_) {
			std::cout << "[Error] Must register thread ID first.\n";
			exit(getchar());
		}
		return id_;
	}
private:
	static constexpr int kUndefinedThreadID{ std::numeric_limits<int>::min() };
	static thread_local int id_;
};

thread_local int MyThread::id_{ MyThread::kUndefinedThreadID };

#endif