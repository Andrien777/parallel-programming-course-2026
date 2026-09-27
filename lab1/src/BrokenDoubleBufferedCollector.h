//
// Created by andre on 27.09.2026.
//

#ifndef PARALLEL_PROGRAMMING_COURSE_2026_BROKENDOUBLEBUFFEREDCOLLECTOR_H
#define PARALLEL_PROGRAMMING_COURSE_2026_BROKENDOUBLEBUFFEREDCOLLECTOR_H
#include "DoubleBufferedCollector.h"


class BrokenDoubleBufferedCollector : public DoubleBufferedCollector {
public:
    void record(uint64_t value) override;
};


#endif //PARALLEL_PROGRAMMING_COURSE_2026_BROKENDOUBLEBUFFEREDCOLLECTOR_H
