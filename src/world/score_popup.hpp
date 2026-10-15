#pragma once

namespace moorhuhn2::world {
struct ScorePopup {
    float world_x{};
    float world_y{};
    int points{};
    bool operator==(const ScorePopup&) const = default;
};
} // namespace moorhuhn2::world
