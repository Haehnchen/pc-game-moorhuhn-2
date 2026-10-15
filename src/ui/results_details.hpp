#pragma once

#include "contracts/display.hpp"
#include "ui/scores.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace moorhuhn2::ui {

class ResultsDetails {
public:
    // Pass the qualified name-entry state at results entry. Confirming a name
    // later does not enable the personal-score display.
    explicit ResultsDetails(bool name_entry_at_entry = false);
    void update(float delta);

    [[nodiscard]] float elapsed() const noexcept {
        return elapsed_;
    }

    [[nodiscard]] int blink_tick() const noexcept {
        return blink_tick_;
    }

    // This is the blink phase; the application also requires a current editing row.
    [[nodiscard]] bool caret_visible() const noexcept {
        return blink_tick_ % 2 != 0;
    }

    [[nodiscard]] bool personal_score_visible() const noexcept {
        return personal_score_allowed_ && blink_tick_ % 4 != 0;
    }

private:
    float elapsed_{};
    int blink_tick_{};
    bool personal_score_allowed_{};
};

inline constexpr std::string_view results_website_url = "www.computerchannel.de";

// The original cursor's logical point is its top-left; its hotspot is +16,+16.
[[nodiscard]] bool results_website_hover(contracts::ScreenPoint pointer) noexcept;

struct ResultsWatermarkPixel {
    contracts::ScreenPoint point;
    std::uint16_t word{};

    bool operator==(const ResultsWatermarkPixel&) const = default;
};

// Write these raw RGB565 words after the primary score background and before
// decorations/rows. The common results-exit loop does not write a watermark.
[[nodiscard]] std::array<ResultsWatermarkPixel, 80> results_watermark(std::span<const ScoreEntry, 5> entries) noexcept;

} // namespace moorhuhn2::ui
