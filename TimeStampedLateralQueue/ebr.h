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
		EBR(int num_thread) noexcept : reservations_{ new RetiredEpoch[num_thread] },
			num_thread_{ num_thread }, epoch_{} {
			for (int i = 0; i < num_thread; ++i) {
				reservations_[i] = std::numeric_limits<RetiredEpoch>::max();
				retired_.emplace_back();
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
			retired_[thread::ID()].push(ptr);
			if (retired_[thread::ID()].size() >= GetCapacity()) {
				Clear();
			}
		}
		void StartOp() noexcept {
			reservations_[thread::ID()] = epoch_.fetch_add(1, std::memory_order_relaxed);
		}
		void EndOp() noexcept {
			reservations_[thread::ID()] = std::numeric_limits<RetiredEpoch>::max();
		}

	private:
		using RetiredEpoch = uint64_t;
		using RetiredNodeQueue = std::queue<T*>;

		constexpr RetiredEpoch GetCapacity() const noexcept {
			return (RetiredEpoch)(3 * num_thread_ * 2 * 10);
		}

		RetiredEpoch GetMinReservation() const noexcept {
			RetiredEpoch min_re = std::numeric_limits<RetiredEpoch>::max();
			for (int i = 0; i < num_thread_; ++i) {
				min_re = std::min(min_re, (RetiredEpoch)reservations_[i]);
			}
			return min_re;
		}

		void Clear() noexcept {
			RetiredEpoch max_safe_epoch = GetMinReservation();

			while (false == retired_[thread::ID()].empty()) {
				auto f = retired_[thread::ID()].front();
				if (f->retire_epoch >= max_safe_epoch)
					break;
				retired_[thread::ID()].pop();

				delete f;
			}
		}

		RetiredEpoch* volatile reservations_;
		std::vector<RetiredNodeQueue> retired_;
		int num_thread_;
		std::atomic<RetiredEpoch> epoch_;
	};
}

#endif