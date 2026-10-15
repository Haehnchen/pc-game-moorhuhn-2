#include "platform/viewport.hpp"

#include <algorithm>
#include <cmath>

namespace moorhuhn2::platform {
namespace {

std::optional<Coordinate> window_to_output(const Viewport& viewport, Coordinate point) {
    if (!viewport.drawable() || !std::isfinite(point.x) || !std::isfinite(point.y) || point.x < 0.0 || point.y < 0.0 || point.x >= viewport.window_width
        || point.y >= viewport.window_height) {
        return std::nullopt;
    }

    return Coordinate{point.x * viewport.pixel_width / viewport.window_width, point.y * viewport.pixel_height / viewport.window_height};
}

} // namespace

Viewport Viewport::fit(int window_width, int window_height, int pixel_width, int pixel_height) {
    Viewport result{window_width, window_height, pixel_width, pixel_height};

    if (window_width <= 0 || window_height <= 0 || pixel_width <= 0 || pixel_height <= 0) {
        return result;
    }

    const double available = std::min(static_cast<double>(pixel_width) / contracts::logical_width, static_cast<double>(pixel_height) / contracts::logical_height);
    result.scale = available >= 1.0 ? std::floor(available) : available;
    result.width = contracts::logical_width * result.scale;
    result.height = contracts::logical_height * result.scale;
    result.x = (pixel_width - result.width) / 2.0;
    result.y = (pixel_height - result.height) / 2.0;

    return result;
}

bool Viewport::drawable() const noexcept {
    return scale > 0.0;
}

std::optional<contracts::ScreenPoint> Viewport::window_to_logical(Coordinate point) const {
    const auto output = window_to_output(*this, point);
    return output ? output_to_logical(*output) : std::nullopt;
}

std::optional<contracts::ScreenPoint> Viewport::window_to_logical_clamped(Coordinate point) const {
    const auto output = window_to_output(*this, point);
    if (!output) {
        return std::nullopt;
    }

    const auto logical_x = std::clamp((output->x - x) / scale, 0.0, static_cast<double>(contracts::logical_width - 1));
    const auto logical_y = std::clamp((output->y - y) / scale, 0.0, static_cast<double>(contracts::logical_height - 1));
    return contracts::ScreenPoint{static_cast<int>(logical_x), static_cast<int>(logical_y)};
}

std::optional<contracts::ScreenPoint> Viewport::output_to_logical(Coordinate point) const {
    if (!drawable() || !std::isfinite(point.x) || !std::isfinite(point.y) || point.x < x || point.y < y || point.x >= x + width || point.y >= y + height) {
        return std::nullopt;
    }

    const auto logical_x = static_cast<int>(std::floor((point.x - x) / scale));
    const auto logical_y = static_cast<int>(std::floor((point.y - y) / scale));

    if (logical_x < 0 || logical_y < 0 || logical_x >= contracts::logical_width || logical_y >= contracts::logical_height) {
        return std::nullopt;
    }

    return contracts::ScreenPoint{logical_x, logical_y};
}

std::optional<Coordinate> Viewport::logical_to_window(contracts::ScreenPoint point) const {
    if (!drawable() || point.x < 0 || point.y < 0 || point.x >= contracts::logical_width || point.y >= contracts::logical_height) {
        return std::nullopt;
    }

    return Coordinate{(x + (point.x + 0.5) * scale) * window_width / pixel_width, (y + (point.y + 0.5) * scale) * window_height / pixel_height};
}

} // namespace moorhuhn2::platform
