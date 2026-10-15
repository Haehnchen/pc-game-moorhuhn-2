#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <vector>

namespace moorhuhn2::bonus {

class MakingOf {
public:
    // The original alternate title artwork selects the logo for an exact Witan name.
    explicit MakingOf(bool witan_logo = false);
    void update(float delta);

    [[nodiscard]] bool witan_logo() const noexcept {
        return witan_logo_;
    }

    [[nodiscard]] std::array<float, 2> logo_phases() const noexcept {
        return {x_phase_, y_phase_};
    }

    [[nodiscard]] contracts::ScreenPoint logo_position() const noexcept {
        return logo_position_;
    }

    [[nodiscard]] static int portrait_frame(contracts::ScreenPoint pointer) noexcept;

    // Pointer coordinates use the original stored cursor position, as on Title.
    [[nodiscard]] std::vector<contracts::SpriteDraw> draws(contracts::ScreenPoint pointer) const;

private:
    bool witan_logo_{};
    float x_phase_{}, y_phase_{};
    contracts::ScreenPoint logo_position_{220, 340};
};

} // namespace moorhuhn2::bonus
