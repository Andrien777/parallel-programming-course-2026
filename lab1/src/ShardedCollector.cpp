//
// Created by andre on 26.09.2026.
//

#include "ShardedCollector.h"

void ShardedCollector::record(const uint64_t value) {
    const int b = std::min(static_cast<int>(value / 4), 255);
    mutex_arr[b % 16].lock();
    ++buckets[b];
    mutex_arr[b % 16].unlock();
    ++atomic_count;
    atomic_sum += value;
    uint64_t cas_old = atomic_min.load();
    if (value < cas_old) {
        while (!atomic_min.compare_exchange_weak(cas_old, std::min(cas_old, value)));
    }
    cas_old = atomic_max.load();
    if (value > cas_old) {
        while (!atomic_max.compare_exchange_weak(cas_old, std::max(cas_old, value)));
    }
}

Snapshot ShardedCollector::snapshot() {
    Snapshot snapshot = {};
    for (int i = 0; i < 16; i++) {
        mutex_arr[i].lock();
        for (int b = i; b < 256; b += 16) {
            snapshot.buckets[b] = buckets[b];
        }
        mutex_arr[i].unlock();
    }
    snapshot.count = atomic_count.load();
    snapshot.sum = atomic_sum.load();
    snapshot.min = atomic_min.load();
    snapshot.max = atomic_max.load();
    snapshot.p50 = get_percentile(snapshot.buckets, 50, atomic_count.load());
    snapshot.p99 = get_percentile(snapshot.buckets, 99, atomic_count.load());
    return snapshot;
}
