#ifndef RELAXATION_DISTANCE_H
#define RELAXATION_DISTANCE_H

#include <list>
#include <mutex>
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

		void Enq(void* node, int thread_id) {
			if (checks_relaxation_distance_) {
				enq_elements_.push_back(std::make_pair(node, thread_id));
			}
		}

		void Deq(void* node, int thread_id) {
			if (checks_relaxation_distance_) {
				deq_elements_.push_back(std::make_pair(node, thread_id));
			}
		}

		auto GetRelaxationDistance() {
			uint64_t sum_rd{};
			auto num_elements = deq_elements_.size();

			for (auto i = deq_elements_.begin(); i != deq_elements_.end(); ++i) {
				uint64_t cnt_skip{};
				for (auto j = enq_elements_.begin(); j != enq_elements_.end(); ++j, ++cnt_skip) {
					if (i->first == j->first) {
						enq_elements_.erase(j);
						sum_rd += cnt_skip;
						break;
					}
				}
			}

			return sum_rd / static_cast<double>(num_elements);
		}

	private:
		bool checks_relaxation_distance_{};
		std::mutex mx_enq_;
		std::mutex mx_deq_;
		std::list<std::pair<void*, int>> deq_elements_;
		std::list<std::pair<void*, int>> enq_elements_;
	};
}

#endif
