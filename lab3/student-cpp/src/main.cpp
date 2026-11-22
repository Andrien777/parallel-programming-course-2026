#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "schedule.hpp"
#include "search.hpp"
#include "timetable.hpp"

namespace {
    double elapsed_ms(const std::chrono::steady_clock::time_point start) {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    }

    Mode parse_mode(const std::string &name) {
        if (name == "static")
            return Mode::Static;
        if (name == "chunked")
            return Mode::Chunked;
        return Mode::Sequential;
    }

    std::filesystem::path exe_dir(const char *argv0) {
        return std::filesystem::absolute(argv0).parent_path();
    }

    void print_answer(std::ofstream &out, const int index, const std::string &from, const std::string &to, const int day,
                      const Answer &answer, const double ms) {
        out << 'Q' << index << ' ' << from << ' ' << to << ' ' << day << " count=" << answer.count << " ms="
            << static_cast<int>(ms + 0.5) << '\n';
        for (const auto &[price, duration, ids, n]: answer.top) {
            out << "  " << price << ' ' << duration << ' ';
            for (int i = 0; i < n; i++)
                out << ids[i] << (i + 1 < n ? "-" : "");
            out << '\n';
        }
    }
} // namespace

int main(int argc, char **argv) {
    const Mode mode = argc > 1 ? parse_mode(argv[1]) : Mode::Sequential;
    const int threads = argc > 2 ? std::atoi(argv[2]) : static_cast<int>(std::thread::hardware_concurrency());
    const int threshold = argc > 3 ? std::atoi(argv[3]) : 64;

    const auto exe = exe_dir(argv[0]);
    const auto data = (exe / ".." / ".." / "data").lexically_normal();
    const auto results_path = exe / "stage1.txt";
    std::ofstream results(results_path);

    const auto load_start = std::chrono::steady_clock::now();
    const auto schedule = lab3::schedule::open((data / "schedule.bin").string());
    const Timetable timetable(schedule);
    std::printf("# threads=%d threshold=%d load=%.0f ms\n", threads, threshold, elapsed_ms(load_start));
    std::printf("# results=%s\n", results_path.string().c_str());

    std::ifstream in(data / "queries.txt");
    int index = 1;
    for (std::string from, to; in >> from >> to;) {
        int day;
        in >> day;
        const int32_t from_id = timetable.airport_id(from), to_id = timetable.airport_id(to);
        search(timetable, from_id, to_id, day, mode, threads, threshold);

        Answer answer;
        std::vector<double> times;
        times.reserve(5);
        for (int i = 0; i < 5; i++) {
            const auto start = std::chrono::steady_clock::now();
            answer = search(timetable, from_id, to_id, day, mode, threads, threshold);
            times.push_back(elapsed_ms(start));
        }
        for (std::size_t tid = 0; tid < answer.work.size(); tid++)
            std::printf("# work Q%d %zu routes=%llu\n", index, tid,
                        static_cast<unsigned long long>(answer.work[tid]));
        std::ranges::sort(times);
        print_answer(results, index, from, to, day, answer, times[times.size() / 2]);
        index++;
    }
    return 0;
}
