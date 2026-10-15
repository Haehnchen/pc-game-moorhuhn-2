#include "ui/title.hpp"

#include "contracts/angle.hpp"
#include "contracts/time.hpp"
#include "ui/hotspot.hpp"
#include "ui/screens.hpp"
#include "ui/tiles.hpp"

#include <cmath>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {
constexpr Hotspot computer_link{28, 391, 55, 419};
constexpr Hotspot disclaimer_link{285, 450, 324, 462};
constexpr Hotspot extras_link{480, 375, 620, 425};

TitleAction hotspot(contracts::ScreenPoint point, bool introducing) {
    if (computer_link.contains(point)) {
        return TitleAction::extra;
    }

    if (disclaimer_link.contains(point)) {
        return TitleAction::disclaimer;
    }

    if (!introducing && extras_link.contains(point)) {
        return TitleAction::extras_menu;
    }

    return TitleAction::none;
}
} // namespace

Title::Title() {
    reenter();
}

void Title::reenter() {
    // Entry retains settled records and the last computed logo X.
    if (introducing_) {
        for (std::size_t i = 0; i < tiles_.size(); ++i) {
            tiles_[i] = {0.0F, 0.0F, static_cast<float>(i % 32 * 20), static_cast<float>(i / 32 * 20), 2.0F, 0.0F, 0, static_cast<std::int32_t>(i),
                640.0F + 10.0F * static_cast<float>(i / 32), 640.0F};
        }
    }

    logo_time_ = 1.0F;
}

TitleAction Title::update(float delta, bool space, bool cancel, bool left_release, contracts::ScreenPoint pointer) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Title delta must be finite and nonnegative");
    }

    auto result = cancel ? TitleAction::quit : TitleAction::none;
    const auto hovered = hotspot(pointer, introducing_);
    if (left_release && hovered != TitleAction::none) {
        result = hovered;
        left_release = false;
    }

    const long double elapsed = delta;
    if (introducing_) {
        for (auto& tile : tiles_) {
            const long double sine = std::sin(static_cast<long double>(tile.motion_x) * contracts::degrees_per_radian);
            tile.motion_x = static_cast<float>(tile.motion_x + elapsed * 0.014F);
            tile.x = static_cast<float>(sine * tile.amplitude + tile.target_x);
            tile.y = tile.target_y;
            const long double amplitude = tile.amplitude - elapsed * 180.0F;
            tile.amplitude = static_cast<float>(amplitude);
            if (amplitude <= 0) {
                tile.amplitude = 0;
            }
        }

        if (tiles_.back().x == 620.0F) {
            introducing_ = false;
        }

        if (space || left_release) {
            for (std::size_t i = 0; i < tiles_.size(); ++i) {
                tiles_[i].x = static_cast<float>(i % 32 * 20);
                tiles_[i].y = static_cast<float>(i / 32 * 20);
                tiles_[i].frame = static_cast<std::int32_t>(i);
            }

            introducing_ = false;
        }
    } else {
        if (space || left_release) {
            result = TitleAction::start;
        }
    }

    if (!introducing_) {
        // FST stores the tween, but the polynomial and zero test retain ST(0).
        const long double time = logo_time_ - elapsed;
        logo_time_ = static_cast<float>(time);
        const long double smooth = time < 0 ? 0 : time > 1 ? 1 : (3.0F - (time + time)) * time * time;
        logo_x_ = static_cast<float>(smooth * 100.0F + 585.0F);
        if (time <= 0) {
            logo_time_ = 0;
        }
    }

    return result;
}

void Title::update_introduction(float delta) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Introduction delta must be finite and nonnegative");
    }
    // Hunting entry reverses the retained title tween, including an early start
    // while the brand is still entering. FST leaves the sum for the polynomial.
    const long double time = logo_time_ + static_cast<long double>(delta);
    logo_time_ = static_cast<float>(time);
    const long double smooth = time < 0 ? 0 : time > 1 ? 1 : (3.0F - (time + time)) * time * time;
    logo_x_ = static_cast<float>(smooth * 100.0F + 585.0F);
    if (time >= 1) {
        logo_time_ = 1;
    }
}

std::vector<contracts::SpriteDraw> Title::introduction_draws() const {
    std::vector<contracts::SpriteDraw> result;
    result.reserve(769);
    // Unlike the interactive title, hunting entry paints records forward and
    // has no cursor or menu overlays (EXE 0x40bd02--0x40bd83).
    for (const auto& tile : tiles_) {
        const auto frame = tile.frame % 768;
        result.push_back(tile_draw("gfx/mainscreen", frame, {static_cast<int>(tile.x), static_cast<int>(tile.y)}));
    }

    result.push_back({"gfx/Witansmall", {0, 0, 80, 81}, {static_cast<int>(logo_x_), 426}});
    return result;
}

std::vector<contracts::SpriteDraw> Title::draws(contracts::ScreenPoint pointer) const {
    std::vector<contracts::SpriteDraw> result;
    result.reserve(771);
    if (introducing_) {
        result.push_back({"gfx/loading", {0, 0, 267, 200}, {187, 100}});
    }

    for (std::size_t reverse = tiles_.size(); reverse != 0; --reverse) {
        const auto i = reverse - 1;
        const auto& tile = tiles_[i];
        const auto frame = tile.frame % 768;
        result.push_back(tile_draw("gfx/mainscreen", frame, {static_cast<int>(tile.x), static_cast<int>(tile.y)}));
    }

    if (!introducing_) {
        result.push_back({"gfx/Witansmall", {0, 0, 80, 81}, {static_cast<int>(logo_x_), 426}});
        text(result, "Extras", 480, 375);
        cursor(result, pointer, hotspot(pointer, introducing_) != TitleAction::none ? 1 : 0);
    }

    return result;
}

OutroTiles Title::outro_tiles() const {
    OutroTiles result{};
    for (std::size_t i = 0; i < tiles_.size(); ++i) {
        result[i] = {tiles_[i].x, tiles_[i].y, tiles_[i].motion_x, tiles_[i].motion_y, tiles_[i].state, tiles_[i].frame};
    }

    return result;
}

} // namespace moorhuhn2::ui
