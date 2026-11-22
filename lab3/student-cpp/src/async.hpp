#pragma once
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/experimental/channel.hpp>

#include "plan.hpp"

namespace stage2 {
    class AsyncAggregator : public lab3::aggregator {
    public:
        explicit AsyncAggregator(lab3::kit &kit);

        ~AsyncAggregator() override;

        void search(std::shared_ptr<const lab3::search> s, std::function<void(lab3::answer)> done) override;

    private:
        using Channel = boost::asio::experimental::channel<void(boost::system::error_code, int)>;

        struct Search {
            std::shared_ptr<const lab3::search> s;
            std::function<void(lab3::answer)> done;
            SearchPlan plan;
            std::vector<KeyState> keys;
            int pending = 0;
            bool finished = false;
        };

        void start(const std::shared_ptr<Search> &search);

        boost::asio::awaitable<void> run_key(std::shared_ptr<Search> search, int index);

        boost::asio::awaitable<bool> acquire(Channel &gate, lab3::clock::duration wait);

        static void finish(const std::shared_ptr<Search> &search);

        lab3::kit &kit_;
        boost::asio::io_context io_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
        std::thread thread_;
        std::map<std::string, std::unique_ptr<Channel> > gates_;
    };
} // namespace stage2
