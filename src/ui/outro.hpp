#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace moorhuhn2::ui {

struct OutroTile {
    float x{}, y{}, velocity_x{}, velocity_y{};
    int state{}, frame{};

    bool operator==(const OutroTile&) const = default;
};

using OutroTiles = std::array<OutroTile, 768>;

class Outro {
public:
    // Retain the title's float poses/frames and continue the session CRT stream.
    Outro(const OutroTiles& tiles, std::uint32_t& random_state);

    // A skip finishes after this update; its final draw must still be presented.
    bool update(float delta, bool skip = false);

    [[nodiscard]] std::span<const OutroTile, 768> records() const {
        return tiles_;
    }

    [[nodiscard]] std::array<contracts::SpriteDraw, 769> draws() const;

    [[nodiscard]] bool holding() const noexcept {
        return holding_;
    }

    [[nodiscard]] std::size_t next_tile() const noexcept {
        return next_tile_;
    }

    [[nodiscard]] float phase_time() const noexcept {
        return phase_time_;
    }

    [[nodiscard]] float logo_time() const noexcept {
        return logo_time_;
    }

    [[nodiscard]] float logo_x() const noexcept {
        return logo_x_;
    }

    [[nodiscard]] bool finished() const noexcept {
        return finished_;
    }

private:
    OutroTiles tiles_{};
    std::size_t next_tile_{};
    float phase_time_{}, logo_time_{}, logo_x_{585.0F};
    bool holding_{}, finished_{};
};

} // namespace moorhuhn2::ui
