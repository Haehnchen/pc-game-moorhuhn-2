#pragma once

#include "game/round.hpp"

namespace moorhuhn2::game::test {

// Accepted local object fixtures give their notice accumulator as the current
// sample, independently of the global application's clock history.
inline void advance(Round& round, float current, const RoundInput& input) {
    const float delta = static_cast<float>(static_cast<double>(current) - static_cast<double>(round.state().previous_sample));
    round.advance({current, delta, current}, input);
}

} // namespace moorhuhn2::game::test
