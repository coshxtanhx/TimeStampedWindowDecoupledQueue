#ifndef RELAXATION_DISTANCE_H
#define RELAXATION_DISTANCE_H

#include <list>
#include <mutex>
#include <utility>
#include <memory>

#include <fstream>

namespace benchmark {
	inline std::ofstream out{ "log.txt" };

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
			uint64_t sum{};
			uint64_t max{};
			auto num_elements = deq_elements_.size();

			for (auto i = deq_elements_.begin(); i != deq_elements_.end(); ++i) {
				uint64_t cnt_skip{};
				for (auto j = enq_elements_.begin(); j != enq_elements_.end(); ++j, ++cnt_skip) {
					if (*i == *j) {
						enq_elements_.erase(j);
						sum += cnt_skip;
						if (max < cnt_skip) {
							max = cnt_skip;
						}
						break;
					}
				}
			}

			return std::make_pair(max, sum / static_cast<double>(num_elements));
		}

	private:
		bool checks_relaxation_distance_{};
		std::mutex mx_enq_;
		std::mutex mx_deq_;
		std::list<void*> deq_elements_;
		std::list<void*> enq_elements_;
	};
}

#endif
