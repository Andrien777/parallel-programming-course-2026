//
// Created by andre on 26.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_EMPTYLOCKCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_EMPTYLOCKCOLLECTOR_H
#include "SingleMutexCollector.h"


class EmptyLockCollector : public SingleMutexCollector {
public:
    void record(uint64_t value) override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_EMPTYLOCKCOLLECTOR_H
