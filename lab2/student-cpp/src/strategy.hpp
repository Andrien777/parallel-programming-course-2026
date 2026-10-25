#pragma once
#include <vector>

namespace sched {

// Decides who runs next. Multi-run strategies (DFS) also say whether another run is needed.
struct Strategy {
    virtual ~Strategy() = default;
    virtual char pick(const std::vector<char>& enabled, char last) = 0;
    virtual void begin_run() {}
    virtual bool end_run() { return false; }   // true = run again
};

}  // namespace sched
