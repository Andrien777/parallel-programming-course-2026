#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "schedule.hpp"

class Timetable {
public:
    explicit Timetable(const lab3::schedule &source);

    int32_t airport_id(const std::string &code) const;
    // SELECT id FROM by_from WHERE airport = :airport AND dep BETWEEN :dep_lo AND :dep_hi
    std::pair<uint32_t, uint32_t> from_range(int32_t airport, int32_t dep_lo, int32_t dep_hi) const;
    // SELECT id FROM by_pair WHERE from = :from AND to = :to AND dep BETWEEN :dep_lo AND :dep_hi
    std::pair<uint32_t, uint32_t> pair_range(int32_t from, int32_t to, int32_t dep_lo, int32_t dep_hi) const;

    int32_t n_airports = 0;
    std::vector<int32_t> dep, arr, price;
    std::vector<uint16_t> from, to;
    // 2D vectors, but flattened, *_offsets are row offsets
    std::vector<uint32_t> by_from_offsets, by_from;
    std::vector<uint32_t> by_pair_offsets, by_pair;

private:
    std::unordered_map<std::string, int32_t> airport_ids_;
};
