#include "ui/results_details.hpp"

#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
using moorhuhn2::contracts::ScreenPoint;
using moorhuhn2::ui::ResultsDetails;
using moorhuhn2::ui::ScoreEntry;

using moorhuhn2::test::require;

void check_blink() {
    struct Outcome {
        std::uint32_t delta_bits;
        int tick;
        bool caret;
        bool personal;
    };

    // Fixed outcomes from binary32 inputs and the source's retained sum * 15.
    // Neighbors straddle the first four tick boundaries without decimal parsing.
    constexpr std::array<Outcome, 14> outcomes{{
        {0x00000000U, 0, false, false},
        {0x3d888888U, 0, false, false},
        {0x3d888889U, 1, true, true},
        {0x3e088888U, 1, true, true},
        {0x3e088889U, 2, false, true},
        {0x3e4cccccU, 2, false, true},
        {0x3e4ccccdU, 3, true, true},
        {0x3e888888U, 3, true, true},
        {0x3e888889U, 4, false, false},
        {0x3eaaaaaaU, 4, false, false},
        {0x3eaaaaabU, 5, true, true},
        {0x3ecccccdU, 6, false, true},
        {0x3f7fffffU, 14, false, true},
        {0x3f800000U, 15, true, true},
    }};
    for (const auto& expected : outcomes) {
        for (const bool name_entry : {false, true}) {
            ResultsDetails screen(name_entry);
            require(screen.elapsed() == 0 && screen.blink_tick() == 0 && !screen.caret_visible() && !screen.personal_score_visible(), "Entry starts at hidden tick zero");
            screen.update(std::bit_cast<float>(expected.delta_bits));
            require(std::bit_cast<std::uint32_t>(screen.elapsed()) == expected.delta_bits && screen.blink_tick() == expected.tick, "Original stored elapsed/tick differs");
            require(screen.caret_visible() == expected.caret, "Caret must follow odd 15 Hz ticks");
            require(screen.personal_score_visible() == (!name_entry && expected.personal), "Personal score must follow the immutable entry qualification gate");
        }
    }

    ResultsDetails unranked;
    ResultsDetails ranked(true);
    for (auto* screen : {&unranked, &ranked}) {
        screen->update(std::bit_cast<float>(0x3c74aaf1U));
        screen->update(std::bit_cast<float>(0x3e80e331U));
        // Exact sum 286331153/1073741824 is below 4/15, but rounds above it.
        require(std::bit_cast<std::uint32_t>(screen->elapsed()) == 0x3e888889U && screen->blink_tick() == 3 && screen->caret_visible(),
            "FST must retain the pre-store sum for blink conversion");
    }

    require(unranked.personal_score_visible() && !ranked.personal_score_visible(), "Ranked and unranked personal gates differ at the same tick");
    unranked.update(0);
    ranked.update(0);
    require(unranked.blink_tick() == 4 && ranked.blink_tick() == 4 && !unranked.caret_visible() && !unranked.personal_score_visible() && !ranked.personal_score_visible(),
        "The following update must reload the stored binary32 elapsed");

    ranked.update(1);
    require(ranked.caret_visible() && !ranked.personal_score_visible(), "Advancing the clock must preserve the entry name latch");
    const auto elapsed_before = ranked.elapsed();
    const auto tick_before = ranked.blink_tick();
    for (const auto delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max(), 2147483648.0F}) {
        bool rejected = false;
        try {
            ranked.update(delta);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && ranked.elapsed() == elapsed_before && ranked.blink_tick() == tick_before && !ranked.personal_score_visible(),
            "Invalid deltas must leave clock and entry latch unchanged");
    }
}

void check_website() {
    struct Outcome {
        ScreenPoint point;
        bool hover;
    };

    constexpr std::array<Outcome, 16> outcomes{{
        {{215, 430}, false},
        {{216, 430}, true},
        {{401, 430}, true},
        {{402, 430}, false},
        {{300, 417}, false},
        {{300, 418}, true},
        {{300, 440}, true},
        {{300, 441}, false},
        {{216, 418}, true},
        {{401, 440}, true},
        {{215, 417}, false},
        {{402, 441}, false},
        {{0, 0}, false},
        {{639, 479}, false},
        {{std::numeric_limits<int>::min(), 430}, false},
        {{std::numeric_limits<int>::max(), 430}, false},
    }};
    for (const auto& expected : outcomes) {
        require(moorhuhn2::ui::results_website_hover(expected.point) == expected.hover, "Website must use strict bounds around the cursor's +16,+16 hotspot");
    }
}

void check_watermark() {
    const std::array<ScoreEntry, 5> entries{{{"low", 1, 100}, {"high", 0x8000, 1}, {"alternating", 0xaaaa, 37}, {"upper bits", 0x12345678, 0}, {"full", 0xffffffff, 42}}};
    // Expected raw words in screen order x624..639, rows 75..79. The source
    // writes the opposite X order, placing the least significant bit at x639.
    constexpr std::array<std::string_view, 5> expected{{"0000000000000001", "1000000000000000", "1010101010101010", "0101011001111000", "1111111111111111"}};
    const auto pixels = moorhuhn2::ui::results_watermark(entries);
    for (std::size_t row = 0; row < expected.size(); ++row) {
        for (std::size_t bit = 0; bit < 16; ++bit) {
            const auto& pixel = pixels[row * 16 + bit];
            require(pixel.point == ScreenPoint{639 - static_cast<int>(bit), 75 + static_cast<int>(row)}, "Watermark coordinates/ascending table order differ");
            require(pixel.word == static_cast<std::uint16_t>(expected[row][15 - bit] - '0'), "Watermark must encode raw low-16 score bits, not accuracy or RGB color values");
        }
    }

    require(moorhuhn2::ui::results_watermark(entries) == pixels, "Repeated watermark drawing must be identical");
    auto alternate_accuracy = entries;
    for (auto& entry : alternate_accuracy) {
        entry.accuracy = 99;
    }

    require(moorhuhn2::ui::results_watermark(alternate_accuracy) == pixels, "Changing accuracy must not change the score watermark");
}
} // namespace

int main() {
    try {
        check_blink();
        check_website();
        check_watermark();
        std::cout << "Results details source outcomes passed: blink/latch/pre-store tick, strict website bounds and raw score watermark\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
