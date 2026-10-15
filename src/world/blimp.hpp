#pragma once

#include "contracts/draw.hpp"
#include "world/score_popup.hpp"

#include <cstdint>
#include <functional>
#include <optional>

namespace moorhuhn2::world {

struct Blimp {
    int type{-1};
    int depth{4};
    float world_x{};
    float world_y{};
    float screen_x{-1024};
    float screen_y{-1024};
    float animation_time{};
    int frame{};
    int frame_count{1};
    float animation_rate{5};
    int points{-35};
    float hit_offset_x{};
    float hit_offset_y{};

    bool operator==(const Blimp&) const = default;
};

struct BlimpProgress {
    bool active{};
    bool alternate{};
    int hits{};

    bool operator==(const BlimpProgress&) const = default;
};

using BlimpPopup = ScorePopup;

struct BlimpImpact {
    int points{};
    BlimpPopup popup;

    bool operator==(const BlimpImpact&) const = default;
};

// Call only after finding a free scene slot. While progress.active marks a live
// blimp this returns nullopt without RNG; otherwise direction 24 takes four
// ordered CRT draws and direction 25 takes three.
[[nodiscard]] std::optional<Blimp> initial_blimp(float camera, BlimpProgress& progress, const std::function<std::uint32_t()>& random);
// Returns one shared type4 smoke request, even if this update releases the slot.
// The caller allocates the particle before drawing its two CRT values.
[[nodiscard]] bool update_blimp(Blimp& blimp, float delta, float camera, int recoil, BlimpProgress& progress);
// Both ammo kinds agree. Caller orders score,popup,reaction,sfx_zeppelin_shot
// and one accuracy hit; the separate balloon-hit counter does not increase.
[[nodiscard]] BlimpImpact hit_blimp(Blimp& blimp, contracts::ScreenPoint pointer, BlimpProgress& progress);
// Run with other grants before firing. Exact hit count 2 sets the alternate
// latch and requests notice 102 at y180, including when the notice pool is full.
[[nodiscard]] bool update_blimp_progress(BlimpProgress& progress);
[[nodiscard]] std::optional<contracts::SpriteDraw> blimp_draw(const Blimp& blimp);
[[nodiscard]] std::optional<contracts::SpriteDraw> blimp_collision(const Blimp& blimp);

} // namespace moorhuhn2::world
