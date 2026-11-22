#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <vector>

#include "plan.hpp"

namespace stage2 {
    class BlockingAggregator : public lab3::aggregator {
    public:
        explicit BlockingAggregator(lab3::kit &kit);

        ~BlockingAggregator() override;

        void search(std::shared_ptr<const lab3::search> s, std::function<void(lab3::answer)> done) override;

    private:
        struct Gate {
            explicit Gate(int value) : limit(value) {
            }

            int limit;
            int used = 0;
            std::mutex mutex;
            std::condition_variable cv;
        };

        struct Slot {
            enum Status { Idle, Running, Done } status = Idle;

            KeyState value;
        };

        struct Search {
            std::shared_ptr<const lab3::search> s;
            std::function<void(lab3::answer)> done;
            SearchPlan plan;
            std::vector<Slot> slots;
            std::mutex mutex;
            std::stop_source stop;
            int pending = 0;
            bool finished = false;
        };

        struct Job {
            std::shared_ptr<Search> search;
            int key = 0;
        };

        struct Deadline {
            lab3::clock::time_point at;
            std::weak_ptr<Search> search;

            bool operator>(const Deadline &other) const { return at > other.at; }
        };

        void worker_loop();

        void watchdog_loop();

        void process(const Job &job);

        static void expire(const std::shared_ptr<Search> &search);

        static void deliver(const std::shared_ptr<Search> &search);

        static bool acquire(Gate &gate, const std::stop_token &stop, lab3::clock::time_point last);

        static void release(Gate &gate);

        lab3::kit &kit_;
        int pool_size_;
        std::map<std::string, Gate> gates_;

        std::deque<Job> queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        std::vector<std::thread> workers_;
        std::atomic<bool> stopping_{false};

        std::thread watchdog_;
        std::mutex deadline_mutex_;
        std::condition_variable deadline_cv_;
        std::priority_queue<Deadline, std::vector<Deadline>, std::greater<Deadline> > deadlines_;
    };
} // namespace stage2
