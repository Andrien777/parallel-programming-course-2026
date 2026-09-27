//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_THREADLOCALCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_THREADLOCALCOLLECTOR_H
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include <mutex>

#include "DoubleBufferedCollector.h"
#include "MetricsCollector.h"

struct ThreadState {
    std::array<std::atomic<uint64_t>, 256> buckets{};
    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> sum{0};
    std::atomic<uint64_t> min{UINT64_MAX};
    std::atomic<uint64_t> max{0};
};

class ThreadLocalCollector : public MetricsCollector  {
private:
    const uint64_t id_ = next_collector_id();
    std::mutex list_lock_;
    std::vector<std::shared_ptr<ThreadState>> all_states_;
    ThreadState* get_my_state();
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_THREADLOCALCOLLECTOR_H
