#pragma once
#include <cstdint>
#include <vector>

#include "aggregator.hpp"

namespace stage2 {

struct KeyState {
    bool answered = false;
    lab3::price_result result;
};

class SearchPlan {
public:
    static SearchPlan build(const lab3::search &s);
    lab3::answer assemble(const lab3::search &s, const std::vector<KeyState> &keys) const;

    std::vector<lab3::price_key> keys;
    std::vector<std::vector<int>> route_keys;
};

} // namespace stage2
