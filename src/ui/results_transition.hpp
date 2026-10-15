#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace moorhuhn2::ui {

struct ResultsTile {
    float x{}, y{}, velocity_y{};
    int state{}, frame{};

    bool operator==(const ResultsTile&) const = default;
};

class ResultsTransition {
public:
    ResultsTransition();

    void update_entry(float delta);
    // Retain row motion; reset the falling score grid and reveal accumulator.
    void begin_exit();
    // A skip finishes after this update; present its final frame before returning.
    bool update_exit(float delta, bool skip = false);

    [[nodiscard]] float row_offset() const noexcept {
        return row_offset_;
    }

    [[nodiscard]] float row_speed() const noexcept {
        return row_speed_;
    }

    [[nodiscard]] std::size_t next_column() const noexcept {
        return next_column_;
    }

    [[nodiscard]] float reveal_time() const noexcept {
        return reveal_time_;
    }

    [[nodiscard]] bool exiting() const noexcept {
        return exiting_;
    }

    [[nodiscard]] bool finished() const noexcept {
        return finished_;
    }

    [[nodiscard]] std::span<const ResultsTile, 768> records() const {
        return tiles_;
    }

    // Entry: reverse score tiles. Exit: reverse title tiles, then score tiles.
    [[nodiscard]] std::vector<contracts::SpriteDraw> background_draws() const;

private:
    std::array<ResultsTile, 768> tiles_{};
    float row_offset_{960.0F}, row_speed_{10.0F}, reveal_time_{};
    std::size_t next_column_{};
    bool exiting_{}, finished_{};
};

} // namespace moorhuhn2::ui
