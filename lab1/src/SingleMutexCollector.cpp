//
// Created by andre on 26.09.2026.
//

#include "SingleMutexCollector.h"

void SingleMutexCollector::record(uint64_t value) {
    std::lock_guard lock(mutex);
    sum += value;
    ++count;
    if (value < min) min = value;
    if (value > max) max = value;
    const uint64_t bucket = std::min(value / 4, 255ULL);
    ++buckets[bucket];
}

Snapshot SingleMutexCollector::snapshot() {
    std::lock_guard lock(mutex);
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
