#ifndef EBR_H
#define EBR_H

#include <queue>
#include <vector>
#include <atomic>
#include <iostream>
#include <limits>
#include "my_thread.h"

namespace lf {
	struct Reservation {
		Reservation() = default;

		void StartOP(std::atomic<uint64_t>& base_epoch) {
			epoch = base_epoch.fetch_add(1, std::memory_order_relaxed);
		}

		void EndOp() {
			epoch = std::numeric_limits<uint64_t>::max();
		}

		volatile uint64_t epoch{ std::numeric_limits<uint64_t>::max() };
	};

	template<class T>
	class EBR {
	public:
		EBR() = delete;
		EBR(int num_thread) noexcept
			: num_thread_{ num_thread }, reservations_(num_thread), retired_(num_thread) {
		}
		~EBR() noexcept {
			for (auto& q : retired_) {
				while (not q.empty()) {
					auto f = q.front();
					q.pop();
					delete f;
				}
			}
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
			reservations_[thread::ID()].StartOP(epoch_);
		}

		void EndOp() noexcept {
			reservations_[thread::ID()].EndOp();
		}

	private:
		struct alignas(std::hardware_destructive_interference_size) RetiredPtrQueue : std::queue<T*> {};

		uint64_t GetCapacity() const noexcept {
			return static_cast<uint64_t>(num_thread_ * 60);
		}

		uint64_t GetMinReservation() const noexcept {
			return std::min_element(reservations_.begin(), reservations_.end(),
				[](const Reservation& a, const Reservation& b) {
					return a.epoch < b.epoch;
				})->epoch;
		}

		void Clear() noexcept {
			auto max_safe_epoch = GetMinReservation();

			while (false == retired_[thread::ID()].empty()) {
				auto f = retired_[thread::ID()].front();
				if (f->retire_epoch >= max_safe_epoch) {
					break;
				}
				retired_[thread::ID()].pop();

				delete f;
			}
		}

		int num_thread_;
		std::vector<Reservation> reservations_;
		std::vector<RetiredPtrQueue> retired_;
		std::atomic<uint64_t> epoch_;
	};
}

#endif