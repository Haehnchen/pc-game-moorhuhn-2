#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace moorhuhn2::world {

// These values are the original startup scene slots, ahead of boat slot 3.
enum class VillageTarget {
    first_church,
    second_church,
    house
};
enum class VillageSound {
    churchbell,
    churchbell_2,
    house_shot
};

struct Village {
    bool first_church_hit{};
    bool second_church_hit{};
    bool house_hit{};
    int house_points{25};

    bool operator==(const Village&) const = default;
};

using VillagePopup = ScorePopup;

struct VillageImpact {
    int points{};
    float extra_seconds{};
    std::optional<VillagePopup> popup;
    std::array<VillageSound, 3> sounds{};
    std::size_t sound_count{};
    bool spawn_balloon{};

    bool operator==(const VillageImpact&) const = default;
};

// Every selected impact credits one accuracy hit. The caller applies score/time
// and popup allocation, then allocates the requested balloon using its own RNG.
[[nodiscard]] VillageImpact hit_village(Village& village, VillageTarget target, int frog_hits);
// Original types/frames remain 31/0, 32/1 and 33/1 after every impact.
[[nodiscard]] contracts::SpriteDraw village_draw(VillageTarget target, float camera, int recoil);

} // namespace moorhuhn2::world
