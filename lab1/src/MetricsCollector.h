//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_METRICSCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_METRICSCOLLECTOR_H


#include <array>
#include <cstdint>

struct Snapshot {
    std::array<uint64_t, 256> buckets;
    uint64_t count;
    uint64_t sum;
    uint64_t min;
    uint64_t max;
    uint64_t p50;
    uint64_t p99;
};

class MetricsCollector {
public:
    virtual ~MetricsCollector() = default;
    virtual void record(uint64_t value) = 0;
    virtual Snapshot snapshot();
protected:
    std::array<uint64_t, 256> buckets = {};
    uint64_t count = 0;
    uint64_t sum = 0;
    uint64_t min = UINT64_MAX;
    uint64_t max = 0;
};

uint64_t get_percentile(const std::array<uint64_t, 256> &buckets, int percentile, uint64_t count);
#endif //PARALLEL_PROGRAMMING_COURSE_2026_METRICSCOLLECTOR_H
