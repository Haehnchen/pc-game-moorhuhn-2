#pragma once
#include <bit>
#include <cstdint>

namespace moorhuhn2::test {
[[nodiscard]] constexpr float binary32(std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}
} // namespace moorhuhn2::test
