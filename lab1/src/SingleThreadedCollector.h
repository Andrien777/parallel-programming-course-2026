//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_SINGLETHREADEDCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_SINGLETHREADEDCOLLECTOR_H
#include "MetricsCollector.h"

class SingleThreadedCollector: public MetricsCollector {
public:
    void record(uint64_t value) override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_SINGLETHREADEDCOLLECTOR_H
