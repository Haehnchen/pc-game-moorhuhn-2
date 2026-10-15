#include "world/scarecrow.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool valid_state(const ScarecrowPiece& piece) {
    const bool finite = std::isfinite(piece.world_x) && std::isfinite(piece.world_y) && std::isfinite(piece.screen_x) && std::isfinite(piece.screen_y)
                        && std::isfinite(piece.velocity_x) && std::isfinite(piece.velocity_y);
    const bool stationary = piece.velocity_x == 0 && piece.velocity_y == 0;

    if (!finite || piece.animation_time != 0 || piece.animation_rate != 0 || piece.frame_count != 1) {
        return false;
    }

    if (piece.type == 16) {
        return stationary && piece.frame == 0 && piece.points == 0;
    }

    constexpr std::array<int, 7> values{5, 10, 15, 20, 25, 30, 50};

    return (piece.type == 17 || piece.type == 57 || piece.type == 500) && piece.frame >= 0 && piece.frame < 7 && piece.points == values[piece.frame]
           && (piece.type != 17 || stationary);
}

} // namespace

std::array<ScarecrowPiece, 8> initial_scarecrow() {
    return {ScarecrowPiece{.type = 16, .world_x = 1950, .world_y = 220, .points = 0}, ScarecrowPiece{},
        ScarecrowPiece{.world_x = 1970, .screen_x = 0, .screen_y = 0, .frame = 1, .points = 10},
        ScarecrowPiece{.world_x = 1949, .world_y = 237, .screen_x = 0, .screen_y = 0, .frame = 2, .points = 15},
        ScarecrowPiece{.world_x = 1966, .world_y = 237, .screen_x = 0, .screen_y = 0, .frame = 3, .points = 20},
        ScarecrowPiece{.world_x = 1957, .world_y = 218, .screen_x = 0, .screen_y = 0, .frame = 4, .points = 25},
        ScarecrowPiece{.world_x = 1958, .world_y = 200, .screen_x = 0, .screen_y = 0, .frame = 5, .points = 30},
        ScarecrowPiece{.world_x = 1958, .world_y = 184, .screen_x = 0, .screen_y = 0, .frame = 6, .points = 50}};
}

void update_scarecrow_piece(ScarecrowPiece& piece, float delta, float camera, int recoil) {
    if (!valid_state(piece) || !detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid scarecrow state, time, camera or recoil");
    }

    if (piece.type == 500) {
        return;
    }

    ScarecrowPiece next = piece;
    const long double elapsed = delta;

    if (piece.type == 57) {
        next.velocity_x = stored(static_cast<long double>(piece.velocity_x) + (piece.velocity_x > 0 ? 1 : -1) * elapsed * 0.06F);
        next.velocity_y = stored(static_cast<long double>(piece.velocity_y) + elapsed * 1.6F);
        // The source stores each velocity before reloading it for movement.
        next.world_x = stored(static_cast<long double>(piece.world_x) + static_cast<long double>(next.velocity_x) * elapsed * 100.0F);
        next.world_y = stored(static_cast<long double>(piece.world_y) + static_cast<long double>(next.velocity_y) * elapsed * 100.0F);
    }

    const auto offset = detail::camera_offset(camera, 1.4F);
    next.screen_x = stored(static_cast<long double>(next.world_x) - offset);
    next.screen_y = stored(static_cast<long double>(next.world_y) + recoil * static_cast<long double>(0.8F));

    if (piece.type == 57 && next.world_y > 480.0F) {
        next.type = 500;
    }

    if (!valid_state(next)) {
        throw std::invalid_argument("Scarecrow update exceeds finite original storage");
    }

    piece = next;
}

ScarecrowImpact prepare_scarecrow_hit(const ScarecrowPiece& selected, const ScarecrowPiece* adjacent) {
    if (!valid_state(selected) || selected.type != 17 || (adjacent != nullptr && !valid_state(*adjacent))) {
        throw std::invalid_argument("Scarecrow impact requires a live body and valid adjacent state");
    }

    return {.release_hat = selected.frame == 5 && adjacent != nullptr && adjacent->frame == 6 && adjacent->type == 17,
        .popup_x = stored(static_cast<long double>(selected.world_x) + 16 - (selected.points < 10 ? 10 : 20)),
        .popup_y = stored(static_cast<long double>(selected.world_y) + 16),
        .popup_points = selected.points};
}

void start_scarecrow_fall(ScarecrowPiece& piece, std::uint32_t random_x, std::uint32_t random_y) {
    if (!valid_state(piece) || piece.type != 17 || random_x > 32767 || random_y > 32767) {
        throw std::invalid_argument("Scarecrow fall requires a live body and two CRT draws");
    }

    piece.type = 57;
    piece.velocity_x = stored(static_cast<long double>(static_cast<int>(random_x % 20) - 10) * 0.04F);
    piece.velocity_y = stored(static_cast<long double>(-5 - static_cast<int>(random_y % 20)) * 0.05F);
}

void record_scarecrow_frame(int& hits, bool& ordered, int frame) {
    if (hits < 0 || hits == std::numeric_limits<int>::max() || frame < 0 || frame > 6) {
        throw std::invalid_argument("Invalid scarecrow hit count or frame");
    }

    if (hits != frame) {
        ordered = false;
    }

    ++hits;
}

ScarecrowCompletion apply_scarecrow_completion(int hits, bool& completed, bool ordered) {
    if (hits < 0) {
        throw std::invalid_argument("Invalid scarecrow completion count");
    }

    if (hits != 7 || completed) {
        return {};
    }

    completed = true;

    return ordered ? ScarecrowCompletion{24, 0, true} : ScarecrowCompletion{0, 10, false};
}

contracts::SpriteDraw scarecrow_draw(const ScarecrowPiece& piece) {
    if (!valid_state(piece) || piece.type == 500 || !detail::in_int_range(piece.screen_x) || !detail::in_int_range(piece.screen_y)) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported scarecrow pose");
    }

    const auto destination = contracts::ScreenPoint{static_cast<int>(piece.screen_x), static_cast<int>(piece.screen_y)};

    if (piece.type == 16) {
        return {"gfx/pole", {0, 0, 48, 48}, destination, true};
    }

    return {"gfx/scheuche", {0, piece.frame * 32, 32, 32}, destination, true};
}

} // namespace moorhuhn2::world
