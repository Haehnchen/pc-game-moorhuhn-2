#pragma once

#include "contracts/draw.hpp"
#include "ui/results_random.hpp"

#include <array>
#include <span>

namespace moorhuhn2::ui {

struct House88Chicken {
    float x{}, y{}, speed{}, animation_time{};
    int frame{}, variant{};

    bool operator==(const House88Chicken&) const = default;
};

class House88Results {
public:
    // Continue the round's CRT random stream. The original runs its >=1500
    // initialization before overwriting those records for this results mode.
    House88Results(int actual_score, ResultsRandom& random);
    void update(float delta, ResultsRandom& random);

    [[nodiscard]] std::span<const House88Chicken, 16> records() const {
        return chickens_;
    }

    [[nodiscard]] std::array<contracts::SpriteDraw, 16> draws() const;

private:
    std::array<House88Chicken, 16> chickens_{};
};

} // namespace moorhuhn2::ui
