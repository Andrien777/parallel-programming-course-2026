//
// Created by andre on 27.09.2026.
//

#include "BrokenDoubleBufferedCollector.h"

void BrokenDoubleBufferedCollector::record(uint64_t value) {
    ThreadBuffers* s = get_my_bufs();
    const int b = active.load();
    s->inside.store(b);
    const uint64_t bucket = std::min(value / 4, static_cast<uint64_t>(255));
    s->buf[b].buckets[bucket]++;
    s->buf[b].count++;
    s->buf[b].sum += value;
    if (s->buf[b].min > value) s->buf[b].min = value;
    if (s->buf[b].max < value) s->buf[b].max = value;
    s->inside.store(-1);
}