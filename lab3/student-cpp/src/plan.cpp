#include "plan.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>

namespace stage2 {

SearchPlan SearchPlan::build(const lab3::search &s) {
    SearchPlan plan;
    std::unordered_map<lab3::price_key, int> index;
    plan.route_keys.resize(s.routes.size());
    for (std::size_t i = 0; i < s.routes.size(); i++) {
        const auto &flights = s.routes[i].flights;
        plan.route_keys[i].resize(flights.size());
        for (std::size_t j = 0; j < flights.size(); j++) {
            const auto key = flights[j].key();
            const auto [it, inserted] = index.try_emplace(key, static_cast<int>(plan.keys.size()));
            if (inserted)
                plan.keys.push_back(key);
            plan.route_keys[i][j] = it->second;
        }
    }
    return plan;
}

lab3::answer SearchPlan::assemble(const lab3::search &s, const std::vector<KeyState> &keys) const {
    std::vector<lab3::answer::route_result> results;
    results.reserve(s.routes.size());
    for (std::size_t i = 0; i < s.routes.size(); i++) {
        const auto &flights = s.routes[i].flights;
        bool all_answered = true, no_seats = false;
        int32_t sum = 0;
        for (std::size_t j = 0; j < flights.size(); j++) {
            const KeyState &state = keys[route_keys[i][j]];
            if (!state.answered) {
                all_answered = false;
                continue;
            }
            const int32_t price = state.result.price_of(flights[j].id);
            if (price < 0)
                no_seats = true;
            else
                sum += price;
        }
        if (no_seats)
            results.push_back(lab3::answer::route_result::no_seats());
        else if (all_answered)
            results.push_back(lab3::answer::route_result::priced(sum));
        else
            results.push_back(lab3::answer::route_result::unpriced());
    }
    return lab3::answer::of(s, std::move(results));
}

} // namespace stage2
