#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace moorhuhn2::ui {

struct RoundExitTile {
    float x{}, y{}, speed{}, target_x{};
    int state{}, frame{};

    bool operator==(const RoundExitTile&) const = default;
};

class RoundExit {
public:
    RoundExit();

    // A skip finishes after this update; present its final frame before results.
    bool update(float delta, bool skip = false);

    [[nodiscard]] std::span<const RoundExitTile, 768> records() const {
        return tiles_;
    }

    // Scorescreen tiles follow the frozen world in ascending record order.
    [[nodiscard]] std::array<contracts::SpriteDraw, 768> draws() const;

    [[nodiscard]] std::size_t next_tile() const noexcept {
        return next_tile_;
    }

    [[nodiscard]] float reveal_time() const noexcept {
        return reveal_time_;
    }

    [[nodiscard]] bool finished() const noexcept {
        return finished_;
    }

private:
    std::array<RoundExitTile, 768> tiles_{};
    std::size_t next_tile_{};
    float reveal_time_{};
    bool finished_{};
};

} // namespace moorhuhn2::ui
