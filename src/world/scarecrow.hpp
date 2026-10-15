#pragma once

#include "contracts/draw.hpp"

#include <array>
#include <cstdint>

namespace moorhuhn2::world {

struct ScarecrowPiece {
    int type{17};
    float world_x{1944};
    float world_y{217};
    float screen_x{-1024};
    float screen_y{-1024};
    float velocity_x{};
    float velocity_y{};
    float animation_time{};
    float animation_rate{};
    int frame_count{1};
    int frame{};
    int points{5};

    bool operator==(const ScarecrowPiece&) const = default;
};

struct ScarecrowImpact {
    bool release_hat{};
    float popup_x{};
    float popup_y{};
    int popup_points{};

    bool operator==(const ScarecrowImpact&) const = default;
};

struct ScarecrowCompletion {
    int balloon_count{};
    int chicken_count{};
    bool magic_sound{};

    bool operator==(const ScarecrowCompletion&) const = default;
};

// Original slots 6..13: permanent pole, then body frames 0..6. Only the pole and
// first body have initializer screen writes; fresh partner storage starts at0.
[[nodiscard]] std::array<ScarecrowPiece, 8> initial_scarecrow();
// Type 57 becomes invisible type 500 at stored y>480, retaining its scene slot.
// All updates consume no RNG and produce no sound.
void update_scarecrow_piece(ScarecrowPiece& piece, float delta, float camera, int recoil);
// Every popup reads the selected slot, including the second request when frame5
// releases the adjacent live frame6. The caller adds that hat's points twice.
[[nodiscard]] ScarecrowImpact prepare_scarecrow_hit(const ScarecrowPiece& selected, const ScarecrowPiece* adjacent = nullptr);
void start_scarecrow_fall(ScarecrowPiece& piece, std::uint32_t random_x, std::uint32_t random_y);
void record_scarecrow_frame(int& hits, bool& ordered, int frame);
// Run after stone/spider grants and before the frog time bonus, composition and
// firing. Failed scene allocations retain the completed latch and effect counts.
[[nodiscard]] ScarecrowCompletion apply_scarecrow_completion(int hits, bool& completed, bool ordered);
[[nodiscard]] contracts::SpriteDraw scarecrow_draw(const ScarecrowPiece& piece);

} // namespace moorhuhn2::world
