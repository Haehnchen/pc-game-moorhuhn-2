#pragma once

#include "contracts/draw.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace moorhuhn2::ui {

struct LoadingMilestone {
    std::string_view asset;
    int progress{};
};

// Original resource completions, including repeated progress arguments. The
// bootstrap art and final cursor/font image loads have no redraw milestone.
[[nodiscard]] std::span<const LoadingMilestone> loading_milestones() noexcept;
[[nodiscard]] std::span<const std::string_view> loading_image_order() noexcept;
// The original initializer loads 38 sounds. Manifest-only sounds are not milestones.
[[nodiscard]] std::span<const std::string_view> loading_audio_order() noexcept;

// The source loader calls with 1, 2 and 5..19. Bound drawing to the valid
// remaining-row domain 1..19; there is no fabricated empty/full progress state.
[[nodiscard]] std::vector<contracts::SpriteDraw> loading_draws(int progress);

} // namespace moorhuhn2::ui
