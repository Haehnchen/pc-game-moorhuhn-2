#pragma once

#include "contracts/random.hpp"

namespace moorhuhn2::ui {

struct ResultsRandom {
    std::uint32_t state{};
    std::uint64_t draws{};

    [[nodiscard]] std::uint32_t next() noexcept {
        ++draws;
        return contracts::next_random(state);
    }

    bool operator==(const ResultsRandom&) const = default;
};

} // namespace moorhuhn2::ui
