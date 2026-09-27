//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_DOUBLEBUFFEREDCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_DOUBLEBUFFEREDCOLLECTOR_H
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

#include "inline_helpers.h"
#include "MetricsCollector.h"

struct Buf {
    std::array<uint64_t, 256> buckets{}; // обычные uint64_t
    uint64_t count = 0, sum = 0, min = UINT64_MAX, max = 0;
    void clear() {
        buckets.fill(0);
        count = 0; sum = 0; min = UINT64_MAX; max = 0;
    }
};

struct alignas(64) ThreadBuffers {
    std::atomic<int> inside{-1}; // -1 = нигде, 0 = буфер 0, 1 = буфер 1
    Buf buf[2];
};

class DoubleBufferedCollector : public MetricsCollector {
protected:
    const uint64_t id_ = next_collector_id();
    std::atomic<int> active{0};
    std::mutex list_lock_;
    std::mutex snap_lock_;
    std::vector<std::shared_ptr<ThreadBuffers>> all_bufs_;
    ThreadBuffers* get_my_bufs();
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_DOUBLEBUFFEREDCOLLECTOR_H
