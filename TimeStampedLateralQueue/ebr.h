#ifndef EBR_H
#define EBR_H

#include <queue>
#include <vector>
#include <atomic>
#include <iostream>
#include <limits>
#include "thread.h"

namespace lf {
	template<class T>
	class EBR {
	public:
		EBR() = delete;
		EBR(int num_thread) noexcept
			: num_thread_{ num_thread }, reservations_{ new RetiredEpoch[num_thread] }
			, retired_(num_thread) {
			for (int i = 0; i < num_thread; ++i) {
				reservations_[i] = std::numeric_limits<RetiredEpoch>::max();
			}
		}
		~EBR() noexcept {
			for (auto& q : retired_) {
				while (not q.empty()) {
					auto f = q.front();
					q.pop();
					delete f;
				}
			}
			delete[] reservations_;
		}
		EBR(const EBR&) = delete;
		EBR(EBR&&) = delete;
		EBR& operator=(const EBR&) = delete;
		EBR& operator=(EBR&&) = delete;
		void Retire(T* ptr) noexcept {
			ptr->retire_epoch = epoch_.load(std::memory_order_relaxed);
			retired_[MyThread::GetID()].push(ptr);
			if (retired_[MyThread::GetID()].size() >= GetCapacity()) {
				Clear();
			}
		}
		void StartOp() noexcept {
			reservations_[MyThread::GetID()] = epoch_.fetch_add(1, std::memory_order_relaxed);
		}
		void EndOp() noexcept {
			reservations_[MyThread::GetID()] = std::numeric_limits<RetiredEpoch>::max();
		}

	private:
		using RetiredEpoch = uint64_t;
		using RetiredNodeQueue = std::queue<T*>;

		constexpr RetiredEpoch GetCapacity() const noexcept {
			return static_cast<RetiredEpoch>(num_thread_ * 60);
		}

		RetiredEpoch GetMinReservation() const noexcept {
			auto min_re = std::numeric_limits<RetiredEpoch>::max();
			for (int i = 0; i < num_thread_; ++i) {
				min_re = std::min(min_re, reservations_[i]);
			}
			return min_re;
		}

		void Clear() noexcept {
			RetiredEpoch max_safe_epoch = GetMinReservation();

			while (false == retired_[MyThread::GetID()].empty()) {
				auto f = retired_[MyThread::GetID()].front();
				if (f->retire_epoch >= max_safe_epoch)
					break;
				retired_[MyThread::GetID()].pop();

				delete f;
			}
		}

		int num_thread_;
		RetiredEpoch* volatile reservations_;
		std::vector<RetiredNodeQueue> retired_;
		std::atomic<RetiredEpoch> epoch_;
	};
}

#endif