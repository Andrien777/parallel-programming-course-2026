//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_SHARDEDCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_SHARDEDCOLLECTOR_H
#include <atomic>
#include <mutex>

#include "MetricsCollector.h"


class ShardedCollector : public MetricsCollector {
private:
    std::atomic<uint64_t> atomic_count{0};
    std::atomic<uint64_t> atomic_sum{0};
    std::atomic<uint64_t> atomic_min{UINT64_MAX};
    std::atomic<uint64_t> atomic_max{0};
    std::array<std::mutex, 16> mutex_arr{};
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_SHARDEDCOLLECTOR_H
