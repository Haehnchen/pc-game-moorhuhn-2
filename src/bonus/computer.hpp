#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace moorhuhn2::bonus {

struct ComputerChicken {
    float x{}, y{}, speed{}, animation_time{};
    int frame{}, direction{};

    bool operator==(const ComputerChicken&) const = default;
};

class Computer {
public:
    // The screen continues the session's CRT stream without reseeding it.
    explicit Computer(std::uint32_t& random_state);
    void update(float delta, std::uint32_t& random_state);

    [[nodiscard]] std::span<const ComputerChicken, 8> records() const {
        return chickens_;
    }

    [[nodiscard]] std::array<contracts::SpriteDraw, 9> draws() const;

private:
    std::array<ComputerChicken, 8> chickens_{};
};

} // namespace moorhuhn2::bonus
