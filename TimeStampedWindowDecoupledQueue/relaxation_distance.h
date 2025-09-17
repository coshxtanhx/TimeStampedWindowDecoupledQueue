#ifndef RELAXATION_DISTANCE_H
#define RELAXATION_DISTANCE_H

#include <list>
#include <mutex>
#include <deque>
#include <utility>
#include <limits>

namespace benchmark {
	struct RelaxationDistanceLog {
		RelaxationDistanceLog(uint64_t begin, uint64_t end, void* ptr)
			: begin{ begin }, end{ end }, ptr{ ptr } {}

		uint64_t begin;
		uint64_t end;
		void* ptr;
	};

	class RelaxationDistanceManager {
	public:
		RelaxationDistanceManager() = default;

		void CheckRelaxationDistance() {
			checks_relaxation_distance_ = true;
		}

		uint64_t StartEnq() {
			if (not checks_relaxation_distance_) {
				return 0;
			}

			return order_.fetch_add(1);
		}

		void EndEnq(void* ptr, uint64_t begin) {
			if (checks_relaxation_distance_) {
				mx_enq_.lock();
				auto end = order_.fetch_add(1);
				enq_logs_.emplace_back(begin, end, ptr);
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

		void Deq(void* ptr) {
			if (checks_relaxation_distance_) {
				deq_elements_.push_back(ptr);
			}
		}

		auto GetRelaxationDistance() {
			if (not checks_relaxation_distance_) {
				return std::make_tuple(uint64_t{}, uint64_t{}, uint64_t{});
			}

			uint64_t sum_rd{};
			uint64_t max_rd{};

			std::vector<RelaxationDistanceLog> logs;
			logs.reserve(3000);

			for (auto i = deq_elements_.cbegin(); i != deq_elements_.cend(); ++i) {
				logs.clear();
				for (auto j = enq_logs_.cbegin(); j != enq_logs_.cend(); ++j) {
					if (*i == j->ptr) {
						/*std::print("trg - ({}, {})\n", j->begin, j->end);

						for (const auto& l : logs) {
							std::print("({}, {}), ", l.begin, l.end);
						}*/

						uint64_t cnt_skip = std::count_if(logs.begin(), logs.end(), [j](const RelaxationDistanceLog& log) {
							return log.end < j->begin;
							});
						//std::print("\ncnt: {}\n", cnt_skip);

						enq_logs_.erase(j);

						sum_rd += cnt_skip;
						if (cnt_skip > max_rd) {
							max_rd = cnt_skip;
						}
						break;
					}

					logs.push_back(*j);
				}
			}

			return std::make_tuple(deq_elements_.size(), sum_rd, max_rd);
		}

	private:
		bool checks_relaxation_distance_{};
		std::atomic<uint64_t> order_;
		std::mutex mx_enq_;
		std::mutex mx_deq_;
		std::deque<void*> deq_elements_;
		std::list<RelaxationDistanceLog> enq_logs_;
	};
}

#endif
