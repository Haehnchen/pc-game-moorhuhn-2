#include "platform/viewport.hpp"

#include "support/require.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using moorhuhn2::test::require;

bool near(double actual, double expected) {
    return std::abs(actual - expected) < 1e-9;
}

struct Case {
    int window_width;
    int window_height;
    int pixel_width;
    int pixel_height;
    double scale;
    double x;
    double y;
};

void check(const Case& data) {
    using moorhuhn2::contracts::ScreenPoint;
    const auto viewport = moorhuhn2::platform::Viewport::fit(data.window_width, data.window_height, data.pixel_width, data.pixel_height);
    require(near(viewport.scale, data.scale), "Wrong scale");
    require(near(viewport.x, data.x) && near(viewport.y, data.y), "Wrong centering");
    require(near(viewport.width / viewport.height, 4.0 / 3.0), "Aspect ratio changed");
    require(viewport.output_to_logical({viewport.x, viewport.y}) == ScreenPoint{0, 0}, "Top-left edge missing");
    require(!viewport.output_to_logical({viewport.x - 0.001, viewport.y}), "Left letterbox accepted");
    require(!viewport.output_to_logical({viewport.x, viewport.y - 0.001}), "Top letterbox accepted");
    require(!viewport.output_to_logical({viewport.x + viewport.width, viewport.y}), "Right edge accepted");
    require(!viewport.output_to_logical({viewport.x, viewport.y + viewport.height}), "Bottom edge accepted");
    require(viewport.output_to_logical({viewport.x + viewport.width - 0.00001, viewport.y + viewport.height - 0.00001}) == ScreenPoint{639, 479}, "Bottom-right interior missing");
    require(!viewport.window_to_logical({-0.01, 0}) && !viewport.window_to_logical({static_cast<double>(data.window_width), 0}), "Window boundary accepted");
    require(!viewport.logical_to_window({640, 0}) && !viewport.logical_to_window({0, 480}) && !viewport.logical_to_window({-1, 0}), "Invalid logical point accepted");

    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            const ScreenPoint logical{x, y};
            const auto window = viewport.logical_to_window(logical);
            require(window && viewport.window_to_logical(*window) == logical, "Round trip changed a pixel");
            require(viewport.window_to_logical_clamped(*window) == logical, "Clamping changed a playfield pixel");
        }
    }
}

void check_clamped_margins() {
    using moorhuhn2::contracts::ScreenPoint;
    using moorhuhn2::platform::Coordinate;
    using moorhuhn2::platform::Viewport;
    // Same output with native, high-DPI and unequal window/output scales.
    for (const auto window : {Coordinate{1920, 1080}, Coordinate{960, 540}, Coordinate{960, 1080}}) {
        const auto viewport = Viewport::fit(static_cast<int>(window.x), static_cast<int>(window.y), 1920, 1080);
        const auto map = [&](double x, double y) {
            return viewport.window_to_logical_clamped({x * window.x / 1920, y * window.y / 1080});
        };
        require(map(319, 540) == ScreenPoint{0, 240} && map(1600, 540) == ScreenPoint{639, 240}, "Side margins did not clamp to the playfield edge");
        require(map(0, 540) == ScreenPoint{0, 240} && map(1919, 540) == ScreenPoint{639, 240}, "Outer window edges lost the scroll position");
        require(map(960, 0) == ScreenPoint{320, 0} && map(960, 1079) == ScreenPoint{320, 479}, "Top/bottom margins changed horizontal position");
        require(map(0, 0) == ScreenPoint{0, 0} && map(1919, 1079) == ScreenPoint{639, 479}, "Corner margins lost the scroll position");
        for (const auto point : {Coordinate{-1, 540}, Coordinate{1920, 540}, Coordinate{960, -1}, Coordinate{960, 1080}, Coordinate{std::numeric_limits<double>::infinity(), 0},
                 Coordinate{0, std::numeric_limits<double>::quiet_NaN()}}) {
            require(!map(point.x, point.y), "Clamping accepted input outside the window");
        }
    }
}

} // namespace

int main() {
    try {
        const std::vector<Case> cases{
            {640, 480, 640, 480, 1, 0, 0},
            {960, 640, 960, 640, 1, 160, 80},
            {1280, 960, 1280, 960, 2, 0, 0},
            {1920, 1080, 1920, 1080, 2, 320, 60},
            {320, 240, 320, 240, 0.5, 0, 0},
            {320, 200, 320, 200, 200.0 / 480, (320 - 640 * (200.0 / 480)) / 2, 0},
            {100, 480, 100, 480, 100.0 / 640, 0, (480 - 480 * (100.0 / 640)) / 2},
            {1, 1, 1, 1, 1.0 / 640, 0, 0.125},
            {800, 600, 1600, 1200, 2, 160, 120},
            {640, 480, 1280, 960, 2, 0, 0},
            {320, 240, 640, 480, 1, 0, 0},
            {801, 601, 1202, 902, 1, 281, 211},
            {640, 480, 1280, 480, 1, 320, 0},
        };

        for (const auto& data : cases) {
            check(data);
        }
        check_clamped_margins();

        for (const auto& dimensions : std::vector<Case>{{0, 480, 640, 480, 0, 0, 0}, {640, 0, 640, 480, 0, 0, 0}, {640, 480, 0, 0, 0, 0, 0}, {-1, 480, 640, 480, 0, 0, 0}}) {
            const auto viewport = moorhuhn2::platform::Viewport::fit(dimensions.window_width, dimensions.window_height, dimensions.pixel_width, dimensions.pixel_height);
            require(!viewport.drawable() && !viewport.window_to_logical({0, 0}) && !viewport.logical_to_window({0, 0}), "Zero-sized viewport accepted input");
            require(!viewport.window_to_logical_clamped({0, 0}), "Clamping accepted a zero-sized viewport");
        }

        const auto viewport = moorhuhn2::platform::Viewport::fit(640, 480, 640, 480);
        require(!viewport.window_to_logical({std::numeric_limits<double>::infinity(), 0}) && !viewport.output_to_logical({0, std::numeric_limits<double>::quiet_NaN()}),
            "Nonfinite coordinate accepted");
        std::cout << "13 viewport cases; all logical pixel centers round trip; window/output scale, letterbox edges, clamped margins, invalid dimensions passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
