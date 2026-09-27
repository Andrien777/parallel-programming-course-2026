//
// Created by andre on 26.09.2026.
//

#include "ThreadLocalCollector.h"

ThreadState *ThreadLocalCollector::get_my_state() {
    struct TLSSlot { uint64_t id = 0; ThreadState* state = nullptr; };
    thread_local TLSSlot slot;
    if (slot.id != id_) {
        auto s = std::make_shared<ThreadState>();
        ThreadState* raw = s.get();
        {
            std::lock_guard g(list_lock_);
            all_states_.push_back(std::move(s));
        }
        slot.id = id_;
        slot.state = raw;
    }
    return slot.state;
}

void ThreadLocalCollector::record(uint64_t value) {
    ThreadState* s = get_my_state();
    const uint64_t b = std::min(value / 4, static_cast<uint64_t>(255));
    relaxed_add(s->buckets[b], 1);
    relaxed_add(s->count, 1);
    relaxed_add(s->sum, value);
    if (value < s->min.load(std::memory_order_relaxed))
        s->min.store(value, std::memory_order_relaxed);
    if (value > s->max.load(std::memory_order_relaxed))
        s->max.store(value, std::memory_order_relaxed);
}

Snapshot ThreadLocalCollector::snapshot() {
    Snapshot snapshot = {};
    list_lock_.lock();
    const std::vector all_states_copy(all_states_);
    list_lock_.unlock();
    for (const auto& s : all_states_copy) {
        for (int i = 0; i < 256; i++)
            snapshot.buckets[i] += s->buckets[i].load(std::memory_order_relaxed);
        snapshot.count += s->count.load(std::memory_order_relaxed);
        snapshot.sum += s->sum.load(std::memory_order_relaxed);
        snapshot.min = std::min(snapshot.min, s->min.load(std::memory_order_relaxed));
        snapshot.max = std::max(snapshot.max, s->max.load(std::memory_order_relaxed));
    }
    snapshot.p50 = get_percentile(snapshot.buckets, 50, snapshot.count);
    snapshot.p99 = get_percentile(snapshot.buckets, 99, snapshot.count);
    return snapshot;
}
