#include "world/branch.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool live(int type) {
    return type == 22 || type == 23;
}

bool valid_state(const Branch& branch) {
    return (branch.type == -1 || live(branch.type) || branch.type == 62 || branch.type == 63) && std::isfinite(branch.world_x) && std::isfinite(branch.world_y)
           && std::isfinite(branch.screen_x) && std::isfinite(branch.screen_y) && std::isfinite(branch.velocity_x) && std::isfinite(branch.velocity_y) && branch.animation_time == 0
           && branch.animation_rate == 3 && branch.frame_count == 1 && branch.frame == 0 && branch.points == 0
           && (!live(branch.type) || (branch.velocity_x == 0 && branch.velocity_y == 0));
}

} // namespace

std::array<Branch, 4> initial_branches() {
    return {Branch{}, Branch{.world_x = 2007, .screen_x = 0, .screen_y = 0}, Branch{.type = 23, .world_x = 215, .world_y = 2},
        Branch{.type = 23, .world_x = 2135, .world_y = 2, .screen_x = 0, .screen_y = 0}};
}

BranchUpdate update_branch(Branch& branch, float delta, float camera, int recoil) {
    if (!valid_state(branch) || !detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid branch update state, time, camera or recoil");
    }

    if (branch.type == -1) {
        return {};
    }

    Branch next = branch;
    const long double elapsed = delta;

    if (!live(branch.type)) {
        next.velocity_x = stored(static_cast<long double>(branch.velocity_x) + (branch.velocity_x > 0 ? 1 : -1) * elapsed * 0.01F);
        next.velocity_y = stored(static_cast<long double>(branch.velocity_y) + elapsed);
        // Each velocity is stored before the position expression reloads it.
        next.world_x = stored(static_cast<long double>(branch.world_x) + static_cast<long double>(next.velocity_x) * elapsed * 100.0F);
        next.world_y = stored(static_cast<long double>(branch.world_y) + static_cast<long double>(next.velocity_y) * elapsed * 100.0F);
    }

    const auto offset = detail::camera_offset(camera, 1.6F);
    next.screen_x = stored(static_cast<long double>(next.world_x) - offset);
    next.screen_y = stored(static_cast<long double>(next.world_y) + recoil);
    const BranchUpdate result{!live(branch.type) && next.world_y > 480.0F};

    if (result.ground_sound) {
        next.type = -1;
    }

    if (!valid_state(next)) {
        throw std::invalid_argument("Branch update exceeds finite original storage");
    }

    branch = next;

    return result;
}

void hit_branch(Branch& branch, std::uint32_t random_x, std::uint32_t random_y) {
    if (!valid_state(branch) || !live(branch.type) || random_x > 32767 || random_y > 32767) {
        throw std::invalid_argument("Branch impact requires a live branch and two CRT draws");
    }

    branch.type += 40;
    branch.velocity_x = stored(static_cast<long double>(static_cast<int>(random_x % 10) - 5) * 0.1F);
    branch.velocity_y = stored(static_cast<long double>(random_y % 20) * 0.1F);
}

contracts::SpriteDraw branch_draw(const Branch& branch) {
    if (!valid_state(branch) || branch.type == -1 || !detail::in_int_range(branch.screen_x) || !detail::in_int_range(branch.screen_y)) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported branch pose");
    }

    if (branch.type == 22 || branch.type == 62) {
        return {"gfx/branch1", {0, 0, 78, 60}, {static_cast<int>(branch.screen_x), static_cast<int>(branch.screen_y)}, true};
    }

    return {"gfx/branch2", {0, 0, 118, 58}, {static_cast<int>(branch.screen_x), static_cast<int>(branch.screen_y)}, true};
}

} // namespace moorhuhn2::world
