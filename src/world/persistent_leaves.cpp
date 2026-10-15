#include "world/persistent_leaves.hpp"

#include "contracts/angle.hpp"
#include "contracts/random.hpp"
#include "world/numeric.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

void check(const PersistentLeaf& leaf) {
    if (!detail::strictly_in_int_range(leaf.world_x) || !detail::strictly_in_int_range(leaf.world_y) || !detail::strictly_in_int_range(leaf.screen_x)
        || !detail::strictly_in_int_range(leaf.screen_y) || !std::isfinite(leaf.phase) || leaf.phase < 0 || !std::isfinite(leaf.amplitude) || leaf.amplitude < 0
        || leaf.amplitude > 7 || std::trunc(leaf.amplitude) != leaf.amplitude || leaf.depth != 3 || leaf.frame != 1) {
        throw std::invalid_argument("Invalid persistent leaf state");
    }
}

} // namespace

PersistentLeaf make_persistent_leaf(const std::array<std::uint32_t, 4>& random) {
    if (std::ranges::any_of(random, [](auto value) {
            return value > 32767;
        })) {
        throw std::invalid_argument("Persistent leaf creation requires four CRT draws");
    }

    PersistentLeaf leaf;
    leaf.world_x = static_cast<float>(700 + random[0] % 64);
    leaf.world_y = static_cast<float>(240 + random[1] % 64);
    leaf.phase = stored(static_cast<long double>(random[2] % 64) * 0.01F);
    leaf.amplitude = static_cast<float>(random[3] % 8);

    return leaf;
}

std::array<PersistentLeaf, persistent_leaf_count> make_persistent_leaves(const PersistentLeafRandom& random) {
    std::array<PersistentLeaf, persistent_leaf_count> leaves;

    for (auto& leaf : leaves) {
        leaf = make_persistent_leaf({contracts::draw_random(random), contracts::draw_random(random), contracts::draw_random(random), contracts::draw_random(random)});
    }

    return leaves;
}

void update_persistent_leaf(PersistentLeaf& leaf, float delta, float camera, int recoil, const PersistentLeafRandom& random) {
    check(leaf);

    if (!detail::valid_update(delta, camera, recoil) || !random) {
        throw std::invalid_argument("Invalid persistent leaf elapsed time, camera, recoil or randomness");
    }

    auto next = leaf;
    const int camera_offset = detail::camera_offset(camera, 1.4F);
    const long double angle = static_cast<long double>(leaf.phase) * contracts::degrees_per_radian;
    // FSIN is stored to binary32 before use; FCOS remains on the x87 stack.
    const float sine = stored(std::sin(angle));
    const long double cosine = std::cos(angle);
    next.phase = stored(static_cast<long double>(delta) * 0.045F + leaf.phase);
    next.screen_x = stored((static_cast<long double>(leaf.amplitude) + 15.0F) * sine + leaf.world_x - camera_offset);
    next.screen_y = stored(cosine * 15.0F + leaf.world_y + static_cast<long double>(recoil) * 0.8F);
    check(next);
    // Pose uses the old origin and amplitude; drift becomes visible next update.
    switch (contracts::draw_random(random) % 64) {
        case 0:
            next.world_x = stored(static_cast<long double>(next.world_x) + 1.0F);
            break;
        case 1:
            next.world_x = stored(static_cast<long double>(next.world_x) - 1.0F);
            break;
        case 2:
            next.amplitude = static_cast<float>(contracts::draw_random(random) % 8);
            break;
        default:
            break;
    }

    check(next);
    leaf = next;
}

contracts::SpriteDraw persistent_leaf_draw(const PersistentLeaf& leaf) {
    check(leaf);

    return {"gfx/particle", {0, 1, 8, 1}, {static_cast<int>(leaf.screen_x), static_cast<int>(leaf.screen_y)}, true};
}

} // namespace moorhuhn2::world
