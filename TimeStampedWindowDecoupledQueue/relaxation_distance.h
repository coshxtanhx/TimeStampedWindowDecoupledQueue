#ifndef RELAXATION_DISTANCE_H
#define RELAXATION_DISTANCE_H

#include <list>
#include <mutex>
#include <deque>
#include <utility>
#include <limits>

namespace benchmark {
	class RelaxationDistanceManager {
	public:
		RelaxationDistanceManager() = default;

		void CheckRelaxationDistance() {
			checks_relaxation_distance_ = true;
		}

		void LockEnq() {
			if (checks_relaxation_distance_) {
				mx_enq_.lock();
			}
		}

		void UnlockEnq() {
			if (checks_relaxation_distance_) {
				mx_enq_.unlock();
			}
		}

		void LockDeq() {
			if (checks_relaxation_distance_) {
				mx_deq_.lock();
			}
		}

		void UnlockDeq() {
			if (checks_relaxation_distance_) {
				mx_deq_.unlock();
			}
		}

		void Enq(void* node) {
			if (checks_relaxation_distance_) {
				enq_elements_.push_back(node);
			}
		}

		void Deq(void* node) {
			if (checks_relaxation_distance_) {
				deq_elements_.push_back(node);
			}
		}

		auto GetRelaxationDistance() {
			if (not checks_relaxation_distance_) {
				return std::make_tuple(uint64_t{}, uint64_t{}, uint64_t{});
			}

			uint64_t sum_rd{};
			uint64_t max_rd{};

			for (auto i = deq_elements_.cbegin(); i != deq_elements_.cend(); ++i) {
				uint64_t cnt_skip{};
				for (auto j = enq_elements_.cbegin(); j != enq_elements_.cend(); ++j, ++cnt_skip) {
					if (*i == *j) {
						enq_elements_.erase(j);
						sum_rd += cnt_skip;
						if (cnt_skip > max_rd) {
							max_rd = cnt_skip;
						}
						break;
					}
				}
			}

			return std::make_tuple(deq_elements_.size(), sum_rd, max_rd);
		}

	private:
		bool checks_relaxation_distance_{};
		std::mutex mx_enq_;
		std::mutex mx_deq_;
		std::deque<void*> deq_elements_;
		std::list<void*> enq_elements_;
	};
}

#endif
