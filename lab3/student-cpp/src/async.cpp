#include "async.hpp"

#include <chrono>
#include <optional>
#include <utility>
#include <variant>

#include <boost/asio/experimental/awaitable_operators.hpp>

namespace stage2 {
    namespace asio = boost::asio;
    using namespace asio::experimental::awaitable_operators;

    namespace {
        constexpr auto kMargin = std::chrono::milliseconds(25);
    }

    AsyncAggregator::AsyncAggregator(lab3::kit &kit) : kit_(kit), work_(asio::make_work_guard(io_)) {
        for (const auto &[name, limit]: kit_.limits) {
            auto gate = std::make_unique<Channel>(io_, limit);
            for (int i = 0; i < limit; i++)
                gate->try_send(boost::system::error_code{}, 0);
            gates_.emplace(name, std::move(gate));
        }
        thread_ = std::thread([this] { io_.run(); });
    }

    AsyncAggregator::~AsyncAggregator() {
        work_.reset();
        io_.stop();
        if (thread_.joinable())
            thread_.join();
    }

    void AsyncAggregator::search(std::shared_ptr<const lab3::search> s, std::function<void(lab3::answer)> done) {
        auto search = std::make_shared<Search>();
        search->s = std::move(s);
        search->done = std::move(done);
        search->plan = SearchPlan::build(*search->s);
        search->keys.resize(search->plan.keys.size());
        search->pending = static_cast<int>(search->keys.size());
        asio::post(io_, [this, search] { start(search); });
    }

    void AsyncAggregator::start(const std::shared_ptr<Search> &search) {
        if (search->pending == 0) {
            finish(search);
            return;
        }
        for (int i = 0; i < search->pending; i++)
            asio::co_spawn(io_, run_key(search, i), asio::detached);
    }

    asio::awaitable<bool> AsyncAggregator::acquire(Channel &gate, const lab3::clock::duration wait) {
        asio::steady_timer timer(io_, wait);
        try {
            const auto result = co_await (gate.async_receive(asio::use_awaitable) ||
                                          timer.async_wait(asio::use_awaitable));
            co_return result.index() == 0;
        } catch (...) {
            co_return false;
        }
    }

    asio::awaitable<void> AsyncAggregator::run_key(const std::shared_ptr<Search> search, const int index) {
        const auto key = search->plan.keys[index];
        const auto last = search->s->deadline - kMargin;
        KeyState value;
        while (true) {
            const auto now = lab3::clock::now();
            if (now >= last)
                break;
            Channel &gate = *gates_.at(key.airline);
            if (!co_await acquire(gate, last - now))
                break;
            std::optional<lab3::price_result> result;
            try {
                asio::steady_timer timer(io_, last - lab3::clock::now());
                const auto raced = co_await (kit_.client.async_get(key, search->s->id) ||
                                             timer.async_wait(asio::use_awaitable));
                if (raced.index() == 0)
                    result = std::get<lab3::price_result>(raced);
            } catch (...) {
            }
            gate.try_send(boost::system::error_code{}, 0);
            if (!result)
                break;
            value.result = std::move(*result);
            value.answered = value.result.ok();
            if (value.answered || value.result.k != lab3::price_result::kind::err)
                break;
            value = {};
        }
        search->keys[index] = std::move(value);
        if (--search->pending == 0)
            finish(search);
        co_return;
    }

    void AsyncAggregator::finish(const std::shared_ptr<Search> &search) {
        if (search->finished)
            return;
        search->finished = true;
        search->done(search->plan.assemble(*search->s, search->keys));
    }
} // namespace stage2

LAB3_REGISTER("async", stage2::AsyncAggregator);
