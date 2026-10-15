#pragma once

#include "contracts/draw.hpp"
#include "ui/outro.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace moorhuhn2::ui {

// Normal startup uses the animated title; Extras exposes the restored bonus screens.
enum class TitleAction {
    none,
    start,
    quit,
    disclaimer,
    extra,
    extras_menu
};

struct TitleTile {
    float x{}, y{}, target_x{}, target_y{}, motion_x{}, motion_y{};
    std::int32_t state{}, frame{};
    float amplitude{}, initial_amplitude{};
};

class Title {
public:
    Title();
    void reenter();
    TitleAction update(float delta, bool space, bool cancel, bool left_release, contracts::ScreenPoint pointer);
    void update_introduction(float delta);

    [[nodiscard]] bool introducing() const noexcept {
        return introducing_;
    }

    [[nodiscard]] std::span<const TitleTile, 768> records() const noexcept {
        return tiles_;
    }

    [[nodiscard]] float logo_x() const noexcept {
        return logo_x_;
    }

    [[nodiscard]] std::vector<contracts::SpriteDraw> draws(contracts::ScreenPoint pointer) const;
    [[nodiscard]] std::vector<contracts::SpriteDraw> introduction_draws() const;
    [[nodiscard]] OutroTiles outro_tiles() const;

private:
    std::array<TitleTile, 768> tiles_{};
    bool introducing_{true};
    float logo_time_{1.0F}, logo_x_{};
};

} // namespace moorhuhn2::ui
