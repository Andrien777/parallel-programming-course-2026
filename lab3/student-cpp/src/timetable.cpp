#include "timetable.hpp"

#include <algorithm>

namespace {
    uint32_t lower_dep(const std::vector<uint32_t> &index, uint32_t begin, uint32_t end, const int32_t value,
                       const std::vector<int32_t> &dep) {
        while (begin < end) {
            if (const uint32_t mid = begin + (end - begin) / 2; dep[index[mid]] < value)
                begin = mid + 1;
            else
                end = mid;
        }
        return begin;
    }

    uint32_t upper_dep(const std::vector<uint32_t> &index, uint32_t begin, uint32_t end, const int32_t value,
                       const std::vector<int32_t> &dep) {
        while (begin < end) {
            if (const uint32_t mid = begin + (end - begin) / 2; dep[index[mid]] <= value)
                begin = mid + 1;
            else
                end = mid;
        }
        return begin;
    }
} // namespace

Timetable::Timetable(const lab3::schedule &source) : n_airports(source.n_airports) {
    for (int32_t id = 0; id < n_airports; id++) airport_ids_.emplace(source.airport_code(id), id);

    const int32_t n = source.n_flights;
    dep.resize(n);
    arr.resize(n);
    price.resize(n);
    from.resize(n);
    to.resize(n);
    for (int32_t id = 0; id < n; id++) {
        const lab3::flight flight = source.get(id);
        dep[id] = flight.dep;
        arr[id] = flight.arr;
        price[id] = flight.base_price;
        from[id] = static_cast<uint16_t>(airport_id(flight.from));
        to[id] = static_cast<uint16_t>(airport_id(flight.to));
    }

    const std::size_t pairs = static_cast<std::size_t>(n_airports) * n_airports;
    by_from_offsets.assign(static_cast<std::size_t>(n_airports) + 1, 0);
    by_pair_offsets.assign(pairs + 1, 0);
    for (int32_t id = 0; id < n; id++) {
        by_from_offsets[from[id] + 1]++;
        by_pair_offsets[static_cast<std::size_t>(from[id]) * n_airports + to[id] + 1]++;
    }
    for (std::size_t i = 1; i < by_from_offsets.size(); i++)
        by_from_offsets[i] += by_from_offsets[i - 1];
    for (std::size_t i = 1; i < by_pair_offsets.size(); i++)
        by_pair_offsets[i] += by_pair_offsets[i - 1];

    by_from.resize(n);
    by_pair.resize(n);
    std::vector fill_from(by_from_offsets.begin(), by_from_offsets.end() - 1);
    std::vector fill_pair(by_pair_offsets.begin(), by_pair_offsets.end() - 1);
    for (int32_t id = 0; id < n; id++) {
        by_from[fill_from[from[id]]++] = id;
        by_pair[fill_pair[static_cast<std::size_t>(from[id]) * n_airports + to[id]]++] = id;
    }

    const auto by_dep = [this](const uint32_t x, const uint32_t y) {
        return dep[x] != dep[y] ? dep[x] < dep[y] : x < y;
    };
    for (int32_t a = 0; a < n_airports; a++)
        std::sort(by_from.begin() + by_from_offsets[a], by_from.begin() + by_from_offsets[a + 1], by_dep);
    for (std::size_t p = 0; p < pairs; p++)
        if (by_pair_offsets[p + 1] - by_pair_offsets[p] > 1)
            std::sort(by_pair.begin() + by_pair_offsets[p], by_pair.begin() + by_pair_offsets[p + 1], by_dep);
}

int32_t Timetable::airport_id(const std::string &code) const {
    return airport_ids_.at(code);
}

std::pair<uint32_t, uint32_t> Timetable::from_range(int32_t airport, int32_t dep_lo, int32_t dep_hi) const {
    const uint32_t begin = by_from_offsets[airport], end = by_from_offsets[airport + 1];
    const uint32_t lo = lower_dep(by_from, begin, end, dep_lo, dep);
    return {lo, upper_dep(by_from, lo, end, dep_hi, dep)};
}

std::pair<uint32_t, uint32_t> Timetable::pair_range(const int32_t from, const int32_t to, const int32_t dep_lo,
                                                    const int32_t dep_hi) const {
    const std::size_t p = static_cast<std::size_t>(from) * n_airports + to;
    const uint32_t begin = by_pair_offsets[p], end = by_pair_offsets[p + 1];
    const uint32_t lo = lower_dep(by_pair, begin, end, dep_lo, dep);
    return {lo, upper_dep(by_pair, lo, end, dep_hi, dep)};
}
