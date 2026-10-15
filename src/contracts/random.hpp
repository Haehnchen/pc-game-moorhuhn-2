#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>

namespace moorhuhn2::contracts {

// Original MSVCRT stream: unsigned 32-bit wraparound and a 15-bit result.
[[nodiscard]] constexpr std::uint32_t next_random(std::uint32_t& state) noexcept {
    state = state * 214013U + 2531011U;
    return (state >> 16U) & 0x7fffU;
}

// Validate injected draws without owning or advancing an implicit RNG stream.
[[nodiscard]] inline std::uint32_t draw_random(const std::function<std::uint32_t()>& random) {
    if (!random) {
        throw std::invalid_argument("An ordered CRT random source is required");
    }
    const auto value = random();
    if (value > 32767) {
        throw std::invalid_argument("CRT draws must be in 0..32767");
    }
    return value;
}

} // namespace moorhuhn2::contracts
