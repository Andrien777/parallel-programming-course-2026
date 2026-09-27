//
// Created by andre on 26.09.2026.
//

#include "EmptyLockCollector.h"

void EmptyLockCollector::record(uint64_t value) {
    std::lock_guard lock(mutex);
}