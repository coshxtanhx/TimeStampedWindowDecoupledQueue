#ifndef TSWD_H
#define TSWD_H

#include <utility>
#include <vector>
#include <chrono>
#include <limits>
#include <optional>
#include "random.h"
#include "ebr.h"
#include "relaxation_distance.h"

namespace lf::tswd {
	struct Node {
		Node() = default;
		Node(int v) : v{ v } {}

		Node* volatile next{};
		uint64_t retire_epoch{};
		uint64_t time_stamp{};
		int v{};
	};

	struct alignas(std::hardware_destructive_interference_size) Window {
		Window() = default;

		bool CAS(uint64_t expected, uint64_t desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&time_stamp),
				&expected, desired);
		}

		volatile uint64_t time_stamp{};
	};

	struct DeqResult {
		enum class Status {
			kFail, kSucc, kNeedsAdvance, kEmpty
		};

		DeqResult() = default;
		DeqResult(Status status, int value) : status{ status }, value{ value } {}
		DeqResult(Status status, Node* old_head) : status{ status }, old_head{ old_head } {}
		DeqResult(Status status) : status{ status } {}

		Status status{};
		int value{};
		Node* old_head{};
	};

	class PartialQueue {
	public:
		PartialQueue() : tail_{ new Node }, head_{ tail_ } {}
		~PartialQueue() {
			while (nullptr != head_->next) {
				Node* t = head_;
				head_ = head_->next;
				delete t;
			}
			delete head_;
		}

		void Enq(Node* node, uint64_t put_ts) {
			auto tail_ts = tail_->time_stamp;
			node->time_stamp = std::max(put_ts, tail_ts) + 1;
			tail_->next = node;
			tail_ = node;
		}

		DeqResult TryDeq(EBR<Node>& ebr, int depth,
			uint64_t get_ts, benchmark::RelaxationDistanceManager& rdm) {
			bool has_failed{};
			while (true) {
				auto loc_head = head_;
				auto first = loc_head->next;
				if (nullptr == first) {
					return { DeqResult::Status::kEmpty, loc_head };
				}
				if (first->time_stamp > get_ts + depth) {
					return { DeqResult::Status::kNeedsAdvance };
				}
				if (has_failed) {
					return { DeqResult::Status::kFail };
				}
				auto value = first->v;
				rdm.LockDeq();
				if (true == CAS(head_, loc_head, first)) {
					rdm.Deq(first);
					rdm.UnlockDeq();
					ebr.Retire(loc_head);
					return { DeqResult::Status::kSucc, value };
				}
				rdm.UnlockDeq();
				has_failed = true;
			}
		}

		auto GetTailTimeStamp() const {
			return tail_->time_stamp;
		}
	private:
		bool CAS(Node* volatile& trg, Node* expected, Node* desired) {
			return std::atomic_compare_exchange_strong(
				reinterpret_cast<volatile std::atomic<uint64_t>*>(&trg),
				reinterpret_cast<uint64_t*>(&expected),
				reinterpret_cast<uint64_t>(desired));
		}

		alignas(std::hardware_destructive_interference_size) Node* tail_;
		alignas(std::hardware_destructive_interference_size) Node* volatile head_;
	};

	class TSWD {
	public:
		TSWD(int num_thread, int depth)
			: depth_{ depth }, queues_(num_thread), ebr_{ num_thread } {
		}

		void CheckRelaxationDistance() {
			rdm_.CheckRelaxationDistance();
		}

		auto GetRelaxationDistance() {
			return rdm_.GetRelaxationDistance();
		}

		void Enq(int v) {
			auto node = new Node{ v };

			rdm_.LockEnq();
			auto put_ts = window_put_.time_stamp;
			rdm_.Enq(node);
			rdm_.UnlockEnq();

			auto& pq = queues_[MyThread::GetID()];

			if (pq.GetTailTimeStamp() >= put_ts + depth_) {
				window_put_.CAS(put_ts, put_ts + depth_);
				put_ts += depth_;
			}
			pq.Enq(node, put_ts);
		}

		std::optional<int> Deq() {
			std::vector<Node*> old_heads(queues_.size());
			size_t id = MyThread::GetID();

			constexpr std::array<int, 100> coprimes{ 1, 5, 7, 11, 13, 17, 19, 23, 25, 29, 31, 35, 37, 41, 43, 47, 49, 53, 55, 59, 61, 65, 67, 71, 73, 77, 79, 83, 85, 89, 91, 95, 97, 101, 103, 107, 109, 113, 115, 119, 121, 125, 127, 131, 133, 137, 139, 143, 145, 149, 151, 155, 157, 161, 163, 167, 169, 173, 175, 179, 181, 185, 187, 191, 193, 197, 199, 203, 205, 209, 211, 215, 217, 221, 223, 227, 229, 233, 235, 239, 241, 245, 247, 251, 253, 257, 259, 263, 265, 269, 271, 275, 277, 281, 283, 287, 289, 293, 295, 299 };
			auto dir = coprimes[Random::Get(0, coprimes.size() - 1)];

			ebr_.StartOp();
			while (true) {
				int cnt_empty{};
				int cnt_needs_advance{};

				auto put_ts = window_put_.time_stamp;
				auto get_ts = window_get_.time_stamp;
				for (size_t i = 0; i < queues_.size(); ++i) {
					auto& pq = queues_[id];
					auto result = pq.TryDeq(ebr_, depth_, get_ts, rdm_);

					switch (result.status) {
						case DeqResult::Status::kSucc: {
							ebr_.EndOp();
							return result.value;
						}
						break;
						case DeqResult::Status::kNeedsAdvance: {
							cnt_needs_advance += 1;
						}
						break;
						case DeqResult::Status::kEmpty: {
							old_heads[id] = result.old_head;
							cnt_empty += 1;
						}
						break;
					}

					id = (id + dir) % queues_.size();
				}

				if (queues_.size() == cnt_empty) {
					bool is_empty{ true };
					for (size_t i = 1; i < queues_.size(); ++i) {
						id = (i * dir + MyThread::GetID()) % queues_.size();
						auto next = old_heads[id]->next;
						if (nullptr != next) {
							is_empty = false;
							break;
						}
					}
					if (is_empty) {
						ebr_.EndOp();
						return std::nullopt;
					}
				} else {
					id = MyThread::GetID();
				}

				if (queues_.size() == cnt_needs_advance + cnt_empty) {
					window_get_.CAS(get_ts, get_ts + depth_);
				}
			}
		}
	private:
		int depth_;
		std::vector<PartialQueue> queues_;
		EBR<Node> ebr_;
		Window window_get_;
		Window window_put_;
		benchmark::RelaxationDistanceManager rdm_;
	};
}

#endif