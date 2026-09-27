//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_SINGLEMUTEXCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_SINGLEMUTEXCOLLECTOR_H
#include <mutex>

#include "MetricsCollector.h"


class SingleMutexCollector : public MetricsCollector {
protected:
    std::mutex mutex;
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_SINGLEMUTEXCOLLECTOR_H
