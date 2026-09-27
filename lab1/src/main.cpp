//
// Created by andre on 26.09.2026.
//
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <latch>
#include <thread>
#include <vector>

#include "BrokenDoubleBufferedCollector.h"
#include "DoubleBufferedCollector.h"
#include "EmptyLockCollector.h"
#include "MetricsCollector.h"
#include "ShardedCollector.h"
#include "SingleMutexCollector.h"
#include "SingleThreadedCollector.h"
#include "ThreadLocalCollector.h"

std::array<uint64_t, 1 << 20> values;

void generate_values(uint64_t seed) {
    srand(seed);
    std::array<double, 1023> probabilities = {1, 0};
    for (int i = 1; i < 1023; i++) {
        probabilities[i] = probabilities[i - 1] + 1.0 / std::pow(i + 1, 1.15);
    }
    for (int i = 0; i < 1023; i++) {
        probabilities[i] /= probabilities[1022];
    }
    for (uint64_t &value: values) {
        const double num = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
        for (int j = 0; j < 1023; j++) {
            if (num <= probabilities[j]) {
                value = j + 1;
                break;
            }
        }
    }
}

double run(MetricsCollector *collector, uint8_t threads, uint64_t seconds) {
    std::latch start{1};
    std::atomic stop{false};
    auto *ops = new uint64_t[threads];
    auto *thread_arr = new std::thread[threads];
    auto job = [&](const int k) {
        uint64_t local_count = 0;
        int i = k * 1000;
        start.wait();
        while (!stop.load()) {
            collector->record(values[i]);
            ++local_count;
            ++i;
            if (i == values.size()) i = 0;
        }
        ops[k] = local_count;
    };
    for (int i = 0; i < threads; i++) {
        thread_arr[i] = std::thread(job, i);
    }
    const auto t0 = std::clock();
    start.count_down(1);
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    const auto t1 = std::clock();
    for (int i = 0; i < threads; i++) {
        thread_arr[i].join();
    }
    uint64_t sum = 0;
    for (int i = 0; i < threads; i++) {
        sum += ops[i];
    }
    delete[] ops;
    delete[] thread_arr;
    return sum / (static_cast<double>(t1 - t0) / CLOCKS_PER_SEC);
}

double measure_point(MetricsCollector *collector, uint8_t threads) {
    run(collector, threads, 5);
    std::vector<double> results;
    for (int i = 0; i < 5; i++)
        results.push_back(run(collector, threads, 5));
    std::ranges::sort(results);
    return results[2];
}

void inconsistency_test(MetricsCollector *collector) {
    std::latch start{1};
    std::atomic stop{false};
    std::thread threads[4];
    uint64_t ops[4] = {};
    auto job = [&](const int k) {
        int i = k * 1000;
        uint64_t local_count = 0;
        start.wait();
        while (!stop.load()) {
            collector->record(values[i]);
            ++i;
            ++local_count;
            if (i == values.size()) i = 0;
        }
        ops[k] = local_count;
    };
    for (int i = 0; i < 4; i++) {
        threads[i] = std::thread(job, i);
    }
    int error = 0, err_less = 0, err_more = 0;
    start.count_down(1);
    for (int i = 0; i < 10000; i++) {
        Snapshot snap = collector->snapshot();
        uint64_t sum = 0;
        for (int j = 0; j < 256; j++) sum += snap.buckets[j];
        if (sum > snap.count) {
            ++error;
            ++err_less;
        } else if (sum < snap.count) {
            ++error;
            ++err_more;
        }
        std::cout << sum << std::endl; // In stage 3 `clang++ -O2` eliminates this loop without cout
    }
    stop.store(true);
    for (auto &thread: threads) {
        thread.join();
    }
    uint64_t true_count = 0;
    for (const uint64_t op : ops) true_count += op;
    std::cout << "Errors: " << error / 10000.0 * 100 << "%" << std::endl;
    std::cout << "Under: " << err_less << " (" << err_less / 10000.0 * 100 << "%)" << std::endl;
    std::cout << "Over: " << err_more << " (" << err_more / 10000.0 * 100 << "%)" << std::endl;
    std::cout << "Total count: " << collector->snapshot().count << " (true value " << true_count << ")" << std::endl;
}

int main() {
    generate_values(0);
    // auto collector = SingleThreadedCollector();
    // std::cout << measure_point(&collector, 1);

    // for (const std::array<uint64_t, 6> thread_nums = {1, 2, 4, 8, 16, 20}; const auto i : thread_nums) {
    //     auto collector = SingleMutexCollector();
    //     std::cout << i << '|' << measure_point(&collector, i) << std::endl;
    // }
    // for (const std::array<uint64_t, 6> thread_nums = {1, 2, 4, 8, 16, 20}; const auto i : thread_nums) {
    //     auto collector = EmptyLockCollector();
    //     std::cout << i << '|' << measure_point(&collector, i) << std::endl;
    // }

    // for (const std::array<uint64_t, 6> thread_nums = {1, 2, 4, 8, 16, 20}; const auto i: thread_nums) {
    //     auto collector = ShardedCollector();
    //     std::cout << i << '|' << measure_point(&collector, i) << std::endl;
    // }
    // auto collector = ShardedCollector();
    // inconsistency_test(&collector);

    // for (const std::array<uint64_t, 6> thread_nums = {1, 2, 4, 8, 16, 20}; const auto i: thread_nums) {
    //     auto collector = ThreadLocalCollector();
    //     std::cout << i << '|' << measure_point(&collector, i) << std::endl;
    // }
    // auto collector = ThreadLocalCollector();
    // inconsistency_test(&collector);

    // for (const std::array<uint64_t, 6> thread_nums = {1, 2, 4, 8, 16, 20}; const auto i: thread_nums) {
    //     auto collector = DoubleBufferedCollector();
    //     std::cout << i << '|' << measure_point(&collector, i) << std::endl;
    // }
    // auto collector = DoubleBufferedCollector();
    // inconsistency_test(&collector);

    auto collector = BrokenDoubleBufferedCollector();
    inconsistency_test(&collector);
}
