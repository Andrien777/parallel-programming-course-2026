//
// Created by andre on 26.09.2026.
//

#include "SingleThreadedCollector.h"

void SingleThreadedCollector::record(const uint64_t value) {
    sum += value;
    ++count;
    if (value < min) min = value;
    if (value > max) max = value;
    const uint64_t bucket = std::min(value / 4, 255ULL);
    ++buckets[bucket];
}