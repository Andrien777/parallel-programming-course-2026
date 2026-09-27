//
// Created by andre on 26.09.2026.
//

#include "DoubleBufferedCollector.h"

#include <thread>

ThreadBuffers* DoubleBufferedCollector::get_my_bufs() {
    struct BufSlot { uint64_t id = 0; ThreadBuffers* bufs = nullptr; };
    thread_local BufSlot slot;
    if (slot.id != id_) {
        auto s = std::make_shared<ThreadBuffers>();
        ThreadBuffers* raw = s.get();
        {
            std::lock_guard g(list_lock_);
            all_bufs_.push_back(std::move(s));
        }
        slot.id = id_;
        slot.bufs = raw;
    }
    return slot.bufs;
}

void DoubleBufferedCollector::record(uint64_t value) {
    ThreadBuffers* s = get_my_bufs();
    int b;
    while (true) {
        b = active.load();
        s->inside.store(b);
        if (b == active.load()) {
            break;
        }
        s->inside.store(-1);
    }
    const uint64_t bucket = std::min(value / 4, static_cast<uint64_t>(255));
    s->buf[b].buckets[bucket]++;
    s->buf[b].count++;
    s->buf[b].sum += value;
    if (s->buf[b].min > value) s->buf[b].min = value;
    if (s->buf[b].max < value) s->buf[b].max = value;
    s->inside.store(-1);
}

Snapshot DoubleBufferedCollector::snapshot() {
    Snapshot snapshot = {};
    std::lock_guard g(snap_lock_);
    const int old = active.load();
    active.store(1 - old);
    for (const auto& buf : all_bufs_) {
        while (buf->inside.load() == old) std::this_thread::yield();
        count += buf->buf[old].count;
        sum += buf->buf[old].sum;
        for (int i = 0; i < 256; i++) {
            buckets[i] += buf->buf[old].buckets[i];
        }
        if (buf->buf[old].max > max) max = buf->buf[old].max;
        if (buf->buf[old].min < min) min = buf->buf[old].min;
        buf->buf[old].clear();
    }
    std::ranges::copy(buckets, snapshot.buckets.begin());
    snapshot.min = min;
    snapshot.max = max;
    snapshot.count = count;
    snapshot.sum = sum;
    snapshot.p50 = get_percentile(buckets, 50, snapshot.count);
    snapshot.p99 = get_percentile(buckets, 99, snapshot.count);
    return snapshot;
}
