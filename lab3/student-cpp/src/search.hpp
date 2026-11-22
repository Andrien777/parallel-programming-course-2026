#pragma once
#include <cstdint>
#include <vector>

#include "timetable.hpp"

enum class Mode { Sequential, Static, Chunked };

struct Route {
    int32_t price = 0;
    int32_t duration = 0;
    uint32_t ids[3] = {};
    uint8_t n = 0;
};

struct Answer {
    uint64_t count = 0;
    std::vector<Route> top;
    std::vector<uint64_t> work;
};

Answer search(const Timetable& timetable, int32_t from, int32_t to, int day, Mode mode, int threads, int threshold);
