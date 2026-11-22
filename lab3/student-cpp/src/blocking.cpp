#include "blocking.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace stage2 {
    namespace {
        constexpr auto kMargin = std::chrono::milliseconds(30);
    }

    BlockingAggregator::BlockingAggregator(lab3::kit &kit) : kit_(kit), pool_size_(kit.int_option("pool", 128)) {
        for (const auto &[name, limit]: kit_.limits)
            gates_.try_emplace(name, limit);
        for (int i = 0; i < pool_size_; i++)
            workers_.emplace_back([this] { worker_loop(); });
        watchdog_ = std::thread([this] { watchdog_loop(); });
    }

    BlockingAggregator::~BlockingAggregator() {
        stopping_ = true;
        queue_cv_.notify_all();
        deadline_cv_.notify_all();
        for (auto &worker: workers_)
            worker.join();
        watchdog_.join();
    }

    void BlockingAggregator::search(std::shared_ptr<const lab3::search> s, std::function<void(lab3::answer)> done) {
        const auto search = std::make_shared<Search>();
        search->s = std::move(s);
        search->done = std::move(done);
        search->plan = SearchPlan::build(*search->s);
        search->slots.resize(search->plan.keys.size());
        search->pending = static_cast<int>(search->slots.size());
        if (search->pending == 0) {
            search->finished = true;
            deliver(search);
            return;
        }
        {
            std::lock_guard lock(queue_mutex_);
            for (int i = 0; i < search->pending; i++)
                queue_.push_back({.search = search, .key = i});
        }
        queue_cv_.notify_all();
        {
            std::lock_guard lock(deadline_mutex_);
            deadlines_.push({.at = search->s->deadline - kMargin, .search = search});
        }
        deadline_cv_.notify_one();
    }

    void BlockingAggregator::worker_loop() {
        std::unique_lock lock(queue_mutex_);
        while (true) {
            queue_cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
            if (stopping_)
                return;
            Job job = std::move(queue_.front());
            queue_.pop_front();
            lock.unlock();
            process(job);
            lock.lock();
        }
    }

    void BlockingAggregator::watchdog_loop() {
        std::unique_lock lock(deadline_mutex_);
        while (!stopping_) {
            if (deadlines_.empty()) {
                deadline_cv_.wait(lock);
                continue;
            }
            if (const auto at = deadlines_.top().at; lab3::clock::now() < at) {
                deadline_cv_.wait_until(lock, at);
                continue;
            }
            auto [at, search] = deadlines_.top();
            deadlines_.pop();
            auto search_ptr = search.lock();
            if (!search_ptr)
                continue;
            lock.unlock();
            expire(search_ptr);
            lock.lock();
        }
    }

    void BlockingAggregator::process(const Job &job) {
        const auto search = job.search;
        {
            std::lock_guard lock(search->mutex);
            auto &[status, value] = search->slots[job.key];
            if (search->finished || search->stop.stop_requested() || status != Slot::Idle)
                return;
            status = Slot::Running;
        }

        const auto &key = search->plan.keys[job.key];
        Gate &gate = gates_.at(key.airline);
        const auto last = search->s->deadline - kMargin;
        KeyState value;
        while (!(search->stop.stop_requested() || lab3::clock::now() >= last)) {
            if (!acquire(gate, search->stop.get_token(), last))
                break;
            const auto remaining = last - lab3::clock::now();
            try {
                value.result = kit_.client.get(key, search->s->id, remaining, search->stop.get_token());
            } catch (...) {
                value = {};
            }
            value.answered = value.result.ok();
            release(gate);
            if (value.answered || value.result.k != lab3::price_result::kind::err)
                break;
            value = {};
        }

        bool complete = false;
        {
            std::lock_guard lock(search->mutex);
            if (auto &[status, key_state] = search->slots[job.key]; status == Slot::Running) {
                status = Slot::Done;
                key_state = std::move(value);
                if (--search->pending == 0 && !search->finished) {
                    search->finished = true;
                    complete = true;
                }
            }
        }
        if (complete)
            deliver(search);
    }

    void BlockingAggregator::expire(const std::shared_ptr<Search> &search) {
        // ReSharper disable once CppExpressionWithoutSideEffects
        search->stop.request_stop();
        bool complete = false;
        {
            std::lock_guard lock(search->mutex);
            if (search->finished)
                return;
            for (auto &[status, value]: search->slots)
                if (status == Slot::Idle) {
                    status = Slot::Done;
                    --search->pending;
                }
            if (search->pending == 0) {
                search->finished = true;
                complete = true;
            }
        }
        if (complete)
            deliver(search);
    }

    void BlockingAggregator::deliver(const std::shared_ptr<Search> &search) {
        std::vector<KeyState> keys(search->slots.size());
        for (std::size_t i = 0; i < search->slots.size(); i++)
            keys[i] = search->slots[i].value;
        search->done(search->plan.assemble(*search->s, keys));
    }

    bool BlockingAggregator::acquire(Gate &gate, const std::stop_token &stop, const lab3::clock::time_point last) {
        std::unique_lock lock(gate.mutex);
        while (gate.used >= gate.limit) {
            if (stop.stop_requested() || lab3::clock::now() >= last)
                return false;
            gate.cv.wait_until(lock, last);
        }
        gate.used++;
        return true;
    }

    void BlockingAggregator::release(Gate &gate) {
        {
            std::lock_guard lock(gate.mutex);
            gate.used--;
        }
        gate.cv.notify_one();
    }
} // namespace stage2

LAB3_REGISTER("blocking", stage2::BlockingAggregator);
