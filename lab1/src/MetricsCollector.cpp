//
// Created by andre on 26.09.2026.
//

#include "MetricsCollector.h"

uint64_t get_percentile(const std::array<uint64_t, 256> &buckets, const int percentile, const uint64_t count) {
    const uint64_t threshold = count * (percentile / 100.0);
    uint64_t sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += buckets[i];
        if (sum >= threshold) {
            return i * 4;
        }
    }
    return count;
}

Snapshot MetricsCollector::snapshot() {
    Snapshot snapshot = {};
    std::copy(this->buckets.begin(), this->buckets.end(), snapshot.buckets.begin());
    snapshot.count = this->count;
    snapshot.max = this->max;
    snapshot.min = this->min;
    snapshot.sum = this->sum;
    snapshot.p50 = get_percentile(snapshot.buckets, 50, snapshot.count);
    snapshot.p99 = get_percentile(snapshot.buckets, 99, snapshot.count);
    return snapshot;
}
