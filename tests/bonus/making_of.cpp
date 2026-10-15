#include "bonus/making_of.hpp"

#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

using moorhuhn2::bonus::MakingOf;
using moorhuhn2::contracts::ScreenPoint;
using moorhuhn2::contracts::SpriteDraw;

using moorhuhn2::test::require;

void check_sprite(const SpriteDraw& actual, std::string_view image, moorhuhn2::contracts::ImageRect source, ScreenPoint destination) {
    require(actual.image == image && actual.source.x == source.x && actual.source.y == source.y && actual.source.width == source.width && actual.source.height == source.height
                && actual.destination == destination && actual.color_key && !actual.mirror_x,
        "Alternate title art has the wrong source rectangle, anchor or drawing flags");
}

void making_artwork() {
    MakingOf screen;
    require(!screen.witan_logo(), "Ordinary Making-of must be the default branch");

    struct PortraitCase {
        ScreenPoint pointer;
        int cell;
    };

    // Integer outcomes from 0040b464–0040b4ab, including both quotient thresholds.
    constexpr std::array cases{PortraitCase{{0, 0}, 0}, PortraitCase{{212, 159}, 0}, PortraitCase{{213, 159}, 0}, PortraitCase{{212, 160}, 0}, PortraitCase{{213, 160}, 1},
        PortraitCase{{425, 319}, 1}, PortraitCase{{426, 319}, 1}, PortraitCase{{425, 320}, 1}, PortraitCase{{426, 320}, 2}, PortraitCase{{638, 159}, 1},
        PortraitCase{{639, 159}, 1}, PortraitCase{{638, 160}, 1}, PortraitCase{{639, 160}, 2}, PortraitCase{{0, 320}, 1}, PortraitCase{{639, 479}, 2}, PortraitCase{{-16, -16}, 0}};
    for (const auto& sample : cases) {
        require(MakingOf::portrait_frame(sample.pointer) == sample.cell, "Making-of portrait diagnostic differs from its source cell");
        screen.update(30.0F);
        const auto phase_before = screen.logo_phases();
        for (int repeat = 0; repeat < 2; ++repeat) {
            const auto draws = screen.draws(sample.pointer);
            require(draws.size() == 3, "Making-of must draw background, portrait and head in source order");
            check_sprite(draws[0], "gfx/Making", {0, 0, 640, 480}, {0, 0});
            check_sprite(draws[1], "gfx/jennie", {0, sample.cell * 100, 125, 100}, {515, 327});
            check_sprite(draws[2], "gfx/kopf", {0, 0, 46, 29}, {594, 399});
        }

        require(screen.logo_phases() == phase_before && phase_before == std::array<float, 2>{0, 0}, "Making-of portrait drawing/time must not advance logo phases");
    }
}

void logo_animation() {
    MakingOf screen(true);
    require(screen.witan_logo() && screen.logo_position() == ScreenPoint{220, 340}, "Witan logo entry must start with zero phases");

    struct LogoCase {
        float delta;
        ScreenPoint position;
        std::array<std::uint32_t, 2> phase_bits;
    };

    // Source phase stores are binary32; positions truncate the old x87 trigonometric result.
    constexpr std::array cases{LogoCase{0.0F, {220, 340}, {0x00000000, 0x00000000}}, LogoCase{1.0F, {220, 340}, {0x3c03126f, 0x3be56042}},
        LogoCase{1.0F, {308, 324}, {0x3c83126f, 0x3c656042}}, LogoCase{2.0F, {378, 279}, {0x3d03126f, 0x3ce56042}}, LogoCase{5.0F, {413, 133}, {0x3d9374bd, 0x3d810625}},
        LogoCase{30.0F, {53, -38}, {0x3e9fbe77, 0x3e8bc6a8}}, LogoCase{0.5F, {54, -59}, {0x3ea1cac1, 0x3e8d9169}}, LogoCase{0.25F, {84, -58}, {0x3ea2d0e6, 0x3e8e76c9}},
        LogoCase{0.125F, {102, -54}, {0x3ea353f8, 0x3e8ee979}}};
    for (const auto& sample : cases) {
        screen.update(sample.delta);
        const auto phases = screen.logo_phases();
        require(screen.logo_position() == sample.position && std::bit_cast<std::uint32_t>(phases[0]) == sample.phase_bits[0]
                    && std::bit_cast<std::uint32_t>(phases[1]) == sample.phase_bits[1],
            "Witan logo old-phase pose, distinct rates or binary32 accumulation differs");
        for (const auto pointer : {ScreenPoint{0, 0}, ScreenPoint{639, 479}}) {
            const auto draws = screen.draws(pointer);
            require(draws.size() == 1, "Witan variant must draw only its source logo");
            check_sprite(draws[0], "gfx/WitanLogo", {0, 0, 200, 203}, sample.position);
        }

        require(screen.logo_phases() == phases && screen.logo_position() == sample.position, "Logo drawing must not advance animation");
    }

    const MakingOf reentered(true);
    require(reentered.logo_phases() == std::array<float, 2>{0, 0} && reentered.logo_position() == ScreenPoint{220, 340}, "Reentering must reset only the new screen instance");
    require(screen.logo_position() == ScreenPoint{102, -54}, "New Making-of instance changed the previous state");
}

void invalid_time() {
    MakingOf screen(true);
    screen.update(1.0F);
    const auto phases = screen.logo_phases();
    const auto position = screen.logo_position();
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        bool rejected = false;
        try {
            screen.update(delta);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected && screen.logo_phases() == phases && screen.logo_position() == position, "Invalid delta accepted or partially advanced Making-of");
    }

    screen.update(0);
    require(screen.logo_position() == ScreenPoint{308, 324} && screen.logo_phases() == phases, "Zero-delta update evaluates the retained old phases without advancing them");
}

} // namespace

int main() {
    try {
        making_artwork();
        logo_animation();
        invalid_time();
        std::cout << "PASS: Making-of source layout, 16 pointer cells, nine Witan animation samples, draw purity, reset and invalid-time atomicity\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
