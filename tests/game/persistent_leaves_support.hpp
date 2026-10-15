#pragma once

#include "game/round.hpp"

#include <cstdint>

namespace moorhuhn2::game::test {

// Independent scalar MSVCRT stream and the source particle case 1 draw gate.
// This predicts the shared RNG boundary without calling the leaf implementation.
struct SourceRandom {
    std::uint32_t state{};
    std::uint64_t draws{};

    std::uint32_t next() {
        state = state * 214013U + 2531011U;
        ++draws;
        return (state >> 16U) & 0x7fffU;
    }

    void discard(std::uint64_t count) {
        for (std::uint64_t index = 0; index < count; ++index) {
            static_cast<void>(next());
        }
    }

    void leaves() {
        for (int slot = 0; slot < 32; ++slot) {
            if (next() % 64 == 2) {
                static_cast<void>(next());
            }
        }
    }
};

inline SourceRandom snapshot(const RoundState& state) {
    return {state.random_state, state.random_draws};
}

inline SourceRandom after_leaves(SourceRandom random, std::uint64_t preceding_draws = 0) {
    random.discard(preceding_draws);
    random.leaves();
    return random;
}

inline std::uint64_t leaf_draw_count(const RoundState& state, std::uint64_t preceding_draws = 0) {
    auto random = snapshot(state);
    random.discard(preceding_draws);
    const auto before = random.draws;
    random.leaves();
    return random.draws - before;
}

} // namespace moorhuhn2::game::test

namespace moorhuhn2::game {

// Accepted literal grant fixtures describe a local source function, rather than
// all preceding scene/particle consumers. Only those tests use these wrappers.
struct RoundTestAccess {
    static void update_balloons(Round& round) {
        for (std::size_t index = 0; index < round.state_.scene_capacity; ++index) {
            round.update_balloon(index);
        }
    }

    static void grant_scarecrow(Round& round) {
        round.grant_scarecrow_bonus();
    }

    static void grant_balloon_hero(Round& round) {
        round.grant_balloon_bonus();
    }
};

} // namespace moorhuhn2::game
