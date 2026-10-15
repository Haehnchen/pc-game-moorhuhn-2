#include "world/tree.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool finite_pose(float world_x, float world_y, float screen_x, float screen_y, float velocity_x, float velocity_y, float animation_time) {
    return std::isfinite(world_x) && std::isfinite(world_y) && std::isfinite(screen_x) && std::isfinite(screen_y) && std::isfinite(velocity_x) && std::isfinite(velocity_y)
           && std::isfinite(animation_time);
}

bool crown(int frame) {
    return frame == 0 || frame == 2;
}

bool valid_tree(const Tree& tree) {
    return (tree.type == -1 || tree.type == 26 || tree.type == 66) && tree.frame >= 0 && tree.frame <= 3 && tree.animation_time == 0 && tree.animation_rate == 0
           && tree.frame_count == 1 && finite_pose(tree.world_x, tree.world_y, tree.screen_x, tree.screen_y, tree.velocity_x, tree.velocity_y, tree.animation_time)
           && (crown(tree.frame) ? tree.points >= 0 && tree.points <= 10 : tree.points <= 0) && (tree.type != 26 || (tree.velocity_x == 0 && tree.velocity_y == 0))
           && (tree.type != 66 || (crown(tree.frame) && tree.points == 0));
}

bool valid_fragment(const TreeFragment& fragment) {
    if (!finite_pose(fragment.world_x, fragment.world_y, fragment.screen_x, fragment.screen_y, fragment.velocity_x, fragment.velocity_y, fragment.animation_time)
        || fragment.animation_time < 0 || (fragment.depth != 2 && fragment.depth != 3)) {
        return false;
    }

    switch (fragment.type) {
        case -1:
            return true;
        case 27:
            return fragment.points == 1 && fragment.animation_rate == 4 && fragment.frame_count == 19 && fragment.frame >= 0 && fragment.frame < 19;
        case 67:
            return fragment.points == 1 && fragment.animation_rate == 5 && fragment.frame_count == 25 && fragment.frame >= 0 && fragment.frame < 25;
        case 28:
            return fragment.frame == 0 && fragment.animation_time == 0 && fragment.animation_rate == 3 && fragment.frame_count == 1
                   && ((fragment.points == 1 && fragment.depth == 2) || (fragment.points == 2 && fragment.depth == 3));
        case 30:
            return fragment.depth == 2 && (fragment.points == 0 || fragment.points == 1) && fragment.frame == 0 && fragment.animation_time == 0 && fragment.animation_rate == 0
                   && fragment.frame_count == 0 && fragment.velocity_x == 0;
        default:
            return false;
    }
}

void validate_update(float delta, float camera, int recoil) {
    if (!detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid tree time, camera or recoil");
    }
}

template <std::size_t Count> void validate_spawn(float camera, int pointer_x, const std::array<std::uint32_t, Count>& random) {
    validate_update(0, camera, 0);

    if (pointer_x < -16 || pointer_x > 639) {
        throw std::invalid_argument("Invalid tree spawn pointer");
    }

    for (const auto draw : random) {
        if (draw > 32767) {
            throw std::invalid_argument("Tree spawn requires original CRT draws");
        }
    }
}

void pose(TreeFragment& fragment, float camera, int recoil, float camera_scale, float recoil_scale) {
    const auto offset = detail::camera_offset(camera, camera_scale);
    fragment.screen_x = stored(static_cast<long double>(fragment.world_x) - offset);
    fragment.screen_y = stored(static_cast<long double>(fragment.world_y) + recoil * static_cast<long double>(recoil_scale));
}

} // namespace

std::array<Tree, 4> initial_trees() {
    return {Tree{}, Tree{.world_x = 147, .world_y = -13, .screen_x = 0, .screen_y = 0, .frame = 0, .points = 10}, Tree{.world_x = 2073, .screen_x = 0, .screen_y = 0, .frame = 3},
        Tree{.world_x = 2067, .world_y = -13, .screen_x = 0, .screen_y = 0, .frame = 2, .points = 10}};
}

TreeUpdate update_tree(Tree& tree, float delta, float camera, int recoil) {
    validate_update(delta, camera, recoil);

    if (!valid_tree(tree)) {
        throw std::invalid_argument("Invalid tree state");
    }

    if (tree.type == -1) {
        return {};
    }

    Tree next = tree;
    const long double elapsed = delta;

    if (tree.type == 66) {
        next.velocity_x = stored(static_cast<long double>(tree.velocity_x) + (tree.velocity_x > 0 ? 1 : -1) * elapsed * 0.01F);
        next.velocity_y = stored(static_cast<long double>(tree.velocity_y) + elapsed);
        next.world_x = stored(static_cast<long double>(tree.world_x) + next.velocity_x * elapsed * 100.0F);
        next.world_y = stored(static_cast<long double>(tree.world_y) + next.velocity_y * elapsed * 100.0F);
    }

    const auto offset = detail::camera_offset(camera, 1.6F);
    next.screen_x = stored(static_cast<long double>(next.world_x) - offset);
    next.screen_y = stored(static_cast<long double>(next.world_y) + recoil);
    const TreeUpdate result{tree.type == 66 && next.world_y > 480.0F};

    if (result.ground_sound) {
        next.type = -1;
    }

    if (!valid_tree(next)) {
        throw std::invalid_argument("Tree update exceeds original finite storage");
    }

    tree = next;

    return result;
}

bool damage_tree(Tree& tree, int branch_hits) {
    if (!valid_tree(tree) || tree.type != 26 || branch_hits < 0) {
        throw std::invalid_argument("Tree damage requires a live tree and branch count");
    }

    if (branch_hits != 4) {
        return false;
    }

    if (tree.points == std::numeric_limits<int>::min() || (crown(tree.frame) && tree.points == 0)) {
        throw std::invalid_argument("Tree damage exceeds the supported source lifetime");
    }

    --tree.points;

    return tree.points == 0;
}

void start_tree_fall(Tree& tree, std::uint32_t random_x, std::uint32_t random_y) {
    if (!valid_tree(tree) || tree.type != 26 || !crown(tree.frame) || tree.points != 0 || random_x > 32767 || random_y > 32767) {
        throw std::invalid_argument("Tree fall requires a destroyed crown and two CRT draws");
    }

    tree.type = 66;
    tree.velocity_x = stored(static_cast<long double>(static_cast<int>(random_x % 10) - 5) * 0.1F);
    tree.velocity_y = stored(static_cast<long double>(random_y % 20) * 0.1F);
}

contracts::SpriteDraw tree_draw(const Tree& tree) {
    if (!valid_tree(tree) || tree.type == -1 || !detail::in_int_range(tree.screen_x) || !detail::in_int_range(tree.screen_y)) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported tree");
    }

    const auto destination = contracts::ScreenPoint{static_cast<int>(tree.screen_x), static_cast<int>(tree.screen_y)};

    if (tree.type == 66 || crown(tree.frame)) {
        return {"gfx/tree1_01", {0, 0, 110, 128}, destination, true};
    }

    return {"gfx/tree1_02", {0, 0, 97, 352}, destination, true};
}

TreeFragment make_tree_leaf(float camera, int pointer_x, const std::array<std::uint32_t, 6>& random) {
    validate_spawn(camera, pointer_x, random);
    const bool nearer = random[0] % 2 == 0;
    const long double camera_scale = nearer ? 1.6F : 1.4F;

    return {.type = 27,
        .depth = nearer ? 2 : 3,
        .world_x = stored(static_cast<long double>(camera) * camera_scale + pointer_x + 16 + random[1] % (nearer ? 320U : 256U) - (nearer ? 220 : 156)),
        .world_y = stored(-60 - static_cast<int>(random[2] % (nearer ? 150U : 100U))),
        .velocity_x = stored(random[4] % (nearer ? 60U : 50U) + (nearer ? 10 : 5)),
        .velocity_y = stored(random[5] % (nearer ? 40U : 35U) + (nearer ? 35 : 30)),
        .animation_rate = 4,
        .frame_count = 19,
        .frame = static_cast<int>(random[3] % 19),
        .points = 1};
}

TreeFragment make_tree_branch(float camera, int pointer_x, const std::array<std::uint32_t, 5>& random) {
    validate_spawn(camera, pointer_x, random);
    const bool nearer = random[0] % 2 == 0;

    return {.type = 28,
        .depth = nearer ? 2 : 3,
        .world_x = stored(static_cast<long double>(camera) * (nearer ? 1.6F : 1.4F) + pointer_x + 16 + random[1] % (nearer ? 320U : 256U) - (nearer ? 220 : 156)),
        .world_y = stored(-60 - static_cast<int>(random[2] % (nearer ? 150U : 100U))),
        .velocity_x = stored(static_cast<long double>(static_cast<int>(random[3] % 10) - 5) * 0.1F),
        .velocity_y = stored(static_cast<long double>(random[4] % 20) * 0.1F),
        .animation_rate = 3,
        .frame_count = 1,
        .points = nearer ? 1 : 2};
}

TreeFragment make_tree_highfly(float camera, int pointer_x) {
    validate_spawn(camera, pointer_x, std::array<std::uint32_t, 0>{});

    return {.type = 30, .world_x = stored(static_cast<long double>(camera) * 1.6F + pointer_x + 16 - 120), .world_y = -480, .velocity_y = 4};
}

TreeFragmentUpdate update_tree_fragment(TreeFragment& fragment, float delta, float camera, int recoil) {
    validate_update(delta, camera, recoil);

    if (!valid_fragment(fragment)) {
        throw std::invalid_argument("Invalid tree fragment state");
    }

    if (fragment.type == -1) {
        return {};
    }

    TreeFragment next = fragment;
    TreeFragmentUpdate result;
    const long double elapsed = delta;

    if (fragment.type == 27 || fragment.type == 67) {
        next.world_x = stored(static_cast<long double>(fragment.world_x) + fragment.velocity_x * elapsed);
        next.world_y = stored(static_cast<long double>(fragment.world_y) + fragment.velocity_y * elapsed);
        pose(next, camera, recoil, fragment.depth == 2 ? 1.6F : 1.4F, fragment.depth == 2 ? 1.0F : 0.8F);

        if (next.world_y > 480.0F) {
            next.type = -1;
        }

        const long double animation = fragment.animation_time + fragment.animation_rate * elapsed * 5.0F;

        if (!detail::in_int_range(animation)) {
            throw std::invalid_argument("Tree leaf animation exceeds original integer storage");
        }

        next.animation_time = stored(animation);
        // The source stores the accumulator but truncates its retained x87 sum.
        next.frame = static_cast<int>(animation);

        if (next.frame >= next.frame_count) {
            if (fragment.type == 27) {
                next.frame = 0;
                next.animation_time = 0;
            } else {
                next.type = -1;
            }
        }
    } else if (fragment.type == 28) {
        next.velocity_x = stored(static_cast<long double>(fragment.velocity_x) + (fragment.velocity_x > 0 ? 1 : -1) * elapsed * (fragment.points == 1 ? 0.01F : 0.005F));
        next.velocity_y = stored(static_cast<long double>(fragment.velocity_y) + elapsed * (fragment.points == 1 ? 1.0F : 0.9F));
        // These released branches add velocity once per consumed update, without
        // multiplying position by delta; they differ from the original branches.
        next.world_x = stored(static_cast<long double>(fragment.world_x) + next.velocity_x);
        next.world_y = stored(static_cast<long double>(fragment.world_y) + next.velocity_y);
        pose(next, camera, recoil, fragment.points == 1 ? 1.6F : 1.4F, 1.0F);
        result.ground_sound = next.world_y > 480.0F;

        if (result.ground_sound) {
            next.type = -1;
        }
    } else {
        const long double velocity = static_cast<long double>(fragment.velocity_y) + elapsed;
        next.velocity_y = stored(velocity);
        // Unlike crown motion, the highfly expression retains vy through FST.
        next.world_y = stored(static_cast<long double>(fragment.world_y) + velocity * elapsed * 100.0F);
        pose(next, camera, recoil, 1.6F, 1.0F);

        if (next.world_y > 480.0F) {
            next.type = -1;
        }

        if (next.world_y > -50.0F && next.points == 0) {
            result.highfly_sound = true;
            next.points = 1;
        }
    }

    if (!valid_fragment(next)) {
        throw std::invalid_argument("Tree fragment update exceeds original finite storage");
    }

    fragment = next;

    return result;
}

TreeLeafImpact hit_tree_leaf(TreeFragment& leaf) {
    if (!valid_fragment(leaf) || leaf.type != 27) {
        throw std::invalid_argument("Leaf impact requires a live tree leaf");
    }

    const TreeLeafImpact impact{leaf.points, stored(static_cast<long double>(leaf.world_x) + 20), stored(static_cast<long double>(leaf.world_y) + 30)};

    if (!std::isfinite(impact.popup_x) || !std::isfinite(impact.popup_y)) {
        throw std::invalid_argument("Leaf popup exceeds original finite storage");
    }

    leaf.type = 67;
    leaf.animation_time = 0;
    leaf.frame = 0;
    leaf.frame_count = 25;
    leaf.animation_rate = 5;

    return impact;
}

contracts::SpriteDraw tree_fragment_draw(const TreeFragment& fragment) {
    if (!valid_fragment(fragment) || fragment.type == -1 || !detail::in_int_range(fragment.screen_x) || !detail::in_int_range(fragment.screen_y)) {
        throw std::invalid_argument("Cannot draw an inactive or unsupported tree fragment");
    }

    const auto destination = contracts::ScreenPoint{static_cast<int>(fragment.screen_x), static_cast<int>(fragment.screen_y)};

    switch (fragment.type) {
        case 27:
            return {"gfx/Leaf", {0, fragment.frame * 60, 60, 60}, destination, true};
        case 67:
            return {"gfx/Leaf2", {0, fragment.frame * 80, 80, 80}, destination, true};
        case 28:
            return fragment.points == 1 ? contracts::SpriteDraw{"gfx/branch1", {0, 0, 78, 60}, destination, true}
                                        : contracts::SpriteDraw{"gfx/branch2", {0, 0, 118, 58}, destination, true};
        default:
            return {"gfx/highfly", {0, 0, 206, 210}, destination, true};
    }
}

} // namespace moorhuhn2::world
