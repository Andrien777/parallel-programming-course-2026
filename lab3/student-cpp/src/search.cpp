#include "search.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <latch>
#include <thread>

namespace {
    bool better(const Route &a, const Route &b) {
        if (a.price != b.price)
            return a.price < b.price;
        if (a.duration != b.duration)
            return a.duration < b.duration;
        const int n = std::min(a.n, b.n);
        for (int i = 0; i < n; i++)
            if (a.ids[i] != b.ids[i])
                return a.ids[i] < b.ids[i];
        return a.n < b.n;
    }

    void remember(Route *top, int &size, const Route &route) {
        if (size == 10 && !better(route, top[9]))
            return;
        int pos = size;
        while (pos > 0 && better(route, top[pos - 1]))
            pos--;
        if (size < 10)
            size++;
        for (int i = size - 1; i > pos; i--)
            top[i] = top[i - 1];
        top[pos] = route;
    }

    // SELECT * FROM by_pair WHERE from = :middle AND to = :to AND dep BETWEEN arr[second]+60 AND arr[second]+360
    void scan_second(const Timetable &timetable, const int32_t from, const int32_t to, const uint32_t first,
                     const uint32_t begin, const uint32_t end,
                     Route *top, int &size, uint64_t &count, uint64_t &work) {
        const uint16_t destination = timetable.to[first];
        const int32_t first_price = timetable.price[first], first_dep = timetable.dep[first];
        for (uint32_t k = begin; k < end; k++) {
            work++;
            const uint32_t second = timetable.by_from[k];
            const uint16_t middle = timetable.to[second];
            if (middle == from || middle == destination)
                continue;
            const int32_t price = first_price + timetable.price[second];
            if (middle == to) {
                count++;
                remember(top, size, {
                             .price = price, .duration = timetable.arr[second] - first_dep,
                             .ids = {first, second, 0}, .n = 2
                         });
                continue;
            }
            const auto [range_fst, range_snd] = timetable.pair_range(
                middle, to, timetable.arr[second] + 60, timetable.arr[second] + 360);
            count += range_snd - range_fst;
            work += range_snd - range_fst;
            if (size == 10 && price > top[9].price)
                continue;
            for (uint32_t j = range_fst; j < range_snd; j++) {
                const uint32_t third = timetable.by_pair[j];
                remember(top, size, {
                             .price = price + timetable.price[third],
                             .duration = timetable.arr[third] - first_dep,
                             .ids = {first, second, third}, .n = 3
                         });
            }
        }
    }

    // SELECT * FROM by_from WHERE airport = :to[first] AND dep BETWEEN arr[first]+60 AND arr[first]+360
    void scan_first(const Timetable &timetable, const int32_t from, const int32_t to, const uint32_t first, Route *top,
                    int &size,
                    uint64_t &count, uint64_t &work) {
        const uint16_t destination = timetable.to[first];
        if (destination == from)
            return;
        if (destination == to) {
            count++;
            remember(top, size, {
                         .price = timetable.price[first], .duration = timetable.arr[first] - timetable.dep[first],
                         .ids = {first, 0, 0}, .n = 1
                     });
            return;
        }
        const auto [range_fst, range_snd] = timetable.from_range(
            destination, timetable.arr[first] + 60, timetable.arr[first] + 360);
        scan_second(timetable, from, to, first, range_fst, range_snd, top, size, count, work);
    }
} // namespace

Answer search(const Timetable &timetable, const int32_t from, const int32_t to, const int day, const Mode mode,
              const int threads, const int threshold) {
    Answer answer;
    // SELECT * FROM by_from WHERE airport = :from AND dep BETWEEN :day*1440 AND :day*1440+1439
    const auto [first_begin, first_end] = timetable.from_range(from, day * 1440, day * 1440 + 1439);
    const std::vector first(timetable.by_from.begin() + first_begin,
                            timetable.by_from.begin() + first_end);
    const int n = mode == Mode::Sequential ? 1 : threads;
    std::vector<std::array<Route, 10> > tops(n);
    std::vector sizes(n, 0);
    std::vector<uint64_t> counts(n, 0), works(n, 0);
    const auto step = static_cast<uint32_t>(threshold);

    if (mode == Mode::Sequential) {
        for (const uint32_t f: first)
            scan_first(timetable, from, to, f, tops[0].data(), sizes[0], counts[0], works[0]);
    } else if (mode == Mode::Static) {
        std::latch start{1};
        std::vector<std::thread> workers;
        for (int i = 0; i < n; i++) {
            workers.emplace_back([&, i] {
                start.wait();
                const int begin = static_cast<int>(static_cast<int64_t>(first.size()) * i / n);
                const int end = static_cast<int>(static_cast<int64_t>(first.size()) * (i + 1) / n);
                for (int k = begin; k < end; k++)
                    scan_first(timetable, from, to, first[k], tops[i].data(), sizes[i], counts[i], works[i]);
            });
        }
        start.count_down();
        for (auto &worker: workers)
            worker.join();
    } else {
        struct Task {
            uint32_t first, begin, end;
        };
        std::vector<Task> tasks;
        for (const uint32_t f: first) {
            if (const uint16_t destination = timetable.to[f]; destination == to) {
                counts[0]++;
                remember(tops[0].data(), sizes[0], {
                             .price = timetable.price[f], .duration = timetable.arr[f] - timetable.dep[f],
                             .ids = {f, 0, 0}, .n = 1
                         });
            } else if (destination != from) {
                // SELECT * FROM by_from WHERE airport = :to[first] AND dep BETWEEN arr[first]+60 AND arr[first]+360
                const auto [range_fst, range_snd] = timetable.from_range(
                    destination, timetable.arr[f] + 60, timetable.arr[f] + 360);
                for (uint32_t begin = range_fst; begin < range_snd; begin += step)
                    tasks.push_back({.first = f, .begin = begin, .end = std::min(begin + step, range_snd)});
            }
        }

        std::atomic<uint32_t> next{0};
        std::latch start{1};
        std::vector<std::thread> workers;
        for (int i = 0; i < n; i++) {
            workers.emplace_back([&, i] {
                start.wait();
                while (true) {
                    const uint32_t k = next.fetch_add(1, std::memory_order_relaxed);
                    if (k >= tasks.size())
                        break;
                    const auto &[task_first, begin, end] = tasks[k];
                    scan_second(timetable, from, to, task_first, begin, end, tops[i].data(), sizes[i],
                                counts[i], works[i]);
                }
            });
        }
        start.count_down();
        for (auto &worker: workers)
            worker.join();
    }

    Route merged[10];
    int merged_size = 0;
    for (int i = 0; i < n; i++) {
        answer.count += counts[i];
        for (int j = 0; j < sizes[i]; j++)
            remember(merged, merged_size, tops[i][j]);
    }
    answer.top.assign(merged, merged + merged_size);
    answer.work = std::move(works);
    return answer;
}
