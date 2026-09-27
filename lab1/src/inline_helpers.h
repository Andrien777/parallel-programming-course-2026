//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_INLINE_HELPERS_H
#define PARALLEL_PROGRAMMING_COURSE_2026_INLINE_HELPERS_H
#include <atomic>
#include <cstdint>

inline uint64_t next_collector_id() {
    static std::atomic<uint64_t> counter{1};
    return counter.fetch_add(1);
}

inline void relaxed_add(std::atomic<uint64_t>& c, uint64_t delta) {
    c.store(c.load(std::memory_order_relaxed) + delta, std::memory_order_relaxed);
}

#endif //PARALLEL_PROGRAMMING_COURSE_2026_INLINE_HELPERS_H
