#include "ui/results_details.hpp"

#include "contracts/time.hpp"
#include "ui/hotspot.hpp"

#include <stdexcept>

namespace moorhuhn2::ui {

ResultsDetails::ResultsDetails(bool name_entry_at_entry) : personal_score_allowed_(!name_entry_at_entry) {}

void ResultsDetails::update(float delta) {
    if (!contracts::valid_delta(delta)) {
        throw std::invalid_argument("Results delta must be finite and nonnegative");
    }

    const auto elapsed = static_cast<long double>(elapsed_) + delta;
    const auto tick = elapsed * 15;
    if (tick >= 2147483648.0L) {
        throw std::invalid_argument("Results elapsed time must fit a signed 32-bit blink conversion");
    }
    // FST stores binary32 elapsed but leaves the extended sum for _ftol.
    elapsed_ = static_cast<float>(elapsed);
    blink_tick_ = static_cast<int>(tick);
}

bool results_website_hover(contracts::ScreenPoint pointer) noexcept {
    // Strict hotspot bounds 231 < x < 418 and 433 < y < 457, less the +16 offset.
    return Hotspot{216, 418, 402, 441}.contains(pointer);
}

std::array<ResultsWatermarkPixel, 80> results_watermark(std::span<const ScoreEntry, 5> entries) noexcept {
    std::array<ResultsWatermarkPixel, 80> pixels;
    for (std::size_t row = 0; row < entries.size(); ++row) {
        for (unsigned bit = 0; bit < 16; ++bit) {
            pixels[row * 16 + bit] = {{639 - static_cast<int>(bit), 75 + static_cast<int>(row)}, static_cast<std::uint16_t>((entries[row].score >> bit) & 1U)};
        }
    }

    return pixels;
}

} // namespace moorhuhn2::ui
