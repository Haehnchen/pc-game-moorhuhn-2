#pragma once

#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <vector>

namespace moorhuhn2::game::test {

using moorhuhn2::test::require;

inline RoundState& fixture(Round& round) {
    return const_cast<RoundState&>(round.state());
}

inline void step(Round& round, float delta = 0) {
    advance(round, round.state().previous_sample + delta, {});
    round.resolve_actions({});
}

inline int count_event(const Round& round, RoundEventKind kind) {
    return static_cast<int>(std::ranges::count(round.events(), kind, &RoundEvent::kind));
}

inline bool event(const Round& round, RoundEventKind kind) {
    return count_event(round, kind) != 0;
}

inline std::vector<contracts::DepthDraw> target_draws(const Round& round) {
    std::vector<contracts::DepthDraw> result;
    for (int depth = 4; depth >= 0; --depth) {
        round.append_target_draws(depth, result);
    }
    return result;
}

} // namespace moorhuhn2::game::test
