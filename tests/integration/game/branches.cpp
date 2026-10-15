#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "render/round_scene.hpp"
#include "support.hpp"
#include "ui/screens.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <bit>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::event;
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::require;
using moorhuhn2::game::test::step;

std::vector<contracts::SpriteDraw> scene(const game::Round& round, const world::Presentation& landscape) {
    return render::round_scene(round, landscape);
}

void shoot(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape, contracts::ScreenPoint pointer) {
    game::test::advance(round, round.state().previous_sample, {.pointer = pointer, .fire_down = true});
    auto draws = scene(round, landscape);
    render::Framebuffer frame;
    frame.draw(images, draws);
    round.resolve_actions([&](const auto& sprite, auto point) {
        return frame.matches_sprite_pixel(images, sprite, point);
    });
}

void ordinary(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    const auto random = state.random_draws;
    const auto leaf_draws = game::test::leaf_draw_count(state, 1);
    shoot(round, images, landscape, {114, 20});
    require(
        state.selected_slot == 20 && state.branches[0].type == 62 && state.branches_hit == 1 && state.hits == 1 && state.misses == 0 && state.score == 0 && state.ammo_index == 9,
        "Source-visible branch1 hit must spend one shot and credit accuracy without score");
    require(state.random_draws == random + 33 + leaf_draws, "Frog and persistent leaves precede ten three-draw particles and two branch velocity draws");
    require(event(round, game::RoundEventKind::branch_hit) && !event(round, game::RoundEventKind::branch_ground), "Impact and ground sounds must be distinct events");
    require(std::ranges::count_if(state.particles,
                [](const auto& particle) {
                    return particle.active && particle.type == 3 && particle.depth == 2 && particle.frame >= 1 && particle.frame <= 3 && particle.world_x == 130
                           && particle.world_y == 36;
                })
                == 10,
        "Ten branch wood fragments must use the impact point and original cells 1..3");
    require(std::ranges::none_of(state.particles,
                [](const auto& particle) {
                    return particle.active && particle.type == 2;
                }),
        "Zero-point branch impact must not allocate a score popup");
    shoot(round, images, landscape, {114, 20});
    require(state.hits == 1 && state.misses == 1 && state.branches_hit == 1 && !event(round, game::RoundEventKind::branch_hit),
        "Falling branch must reject an immediate repeat even while its original art is visible");
    require(state.multi_hit.write_index == 0, "Reaction62 must not register as a qualifying multiple hit");
    step(round, .125F);
    require(state.branches[0].world_y > 5 && state.branches[0].world_x != 87, "Hit branch must move using its stored velocities");
    require(state.particles[0].screen_y == static_cast<float>(static_cast<long double>(state.particles[0].world_y) + 8 * static_cast<long double>(0.8F)),
        "Depth2 wood debris must use the original fixed 0.8 recoil projection");
    const auto before_render = round.state();
    const auto draws = scene(round, landscape);
    require(round.state() == before_render, "Branch composition must preserve core state");
    require(std::ranges::count_if(draws,
                [](const auto& draw) {
                    return draw.image == "gfx/branch1";
                })
                == 2,
        "Falling branch replaces its static presentation record rather than duplicating it");
    int impacts = 0;
    for (int i = 0; i < 40; ++i) {
        step(round, .125F);
        impacts += event(round, game::RoundEventKind::branch_ground) ? 1 : 0;
    }
    require(impacts == 1 && state.branches[0].type == -1 && state.score == 0 && state.branches_hit == 1, "Ground impact must happen exactly once with no delayed award");
    require(std::ranges::none_of(state.particles,
                [](const auto& particle) {
                    return particle.active;
                }),
        "Fallen wood particles must leave the shared pool");
    const auto removed = scene(round, landscape);
    require(std::ranges::count_if(removed,
                [](const auto& draw) {
                    return draw.image == "gfx/branch1";
                })
                == 1,
        "Removed branch must not reappear through static prop fallback");
}

void all_branches_and_reset(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    for (const int index : {0, 2, 1, 3}) {
        state.camera = index % 2 == 0 ? 0 : 1200;
        state.recoil = 0;
        shoot(round, images, landscape, index < 2 ? contracts::ScreenPoint{114, 20} : contracts::ScreenPoint{244, 20});
        require(state.selected_slot == 20 + index && state.branches[static_cast<std::size_t>(index)].type == (index < 2 ? 62 : 63),
            "Each of the four original branch slots must be selectable through actual composed art");
    }
    require(state.branches_hit == 4 && state.hits == 4 && state.misses == 0 && state.score == 0, "Four distinct branches must set the shared prerequisite counter once each");
    step(round, .125F);
    require(state.multi_hit.write_index == 0 && state.score == 0, "Four branches must not trigger a multiple-hit award");
    game::Round reset(1, 0);
    require(reset.state().branches == world::initial_branches() && reset.state().branches_hit == 0 && reset.state().hits == 0
                && std::ranges::none_of(reset.state().particles,
                    [](const auto& particle) {
                        return particle.active;
                    }),
        "New round restores all four live branches, zero counter and empty temporary particles");

    game::Round special(1, 0);
    auto& with_special = fixture(special);
    with_special.camera = 0;
    with_special.special_ammo = true;
    game::Round normal(1, 0);
    fixture(normal).camera = 0;
    shoot(special, images, landscape, {114, 20});
    shoot(normal, images, landscape, {114, 20});
    require(with_special.branches == normal.state().branches && with_special.particles == normal.state().particles && with_special.random_state == normal.state().random_state
                && with_special.score == 0 && with_special.branches_hit == 1,
        "Special ammunition must preserve branch physics, particles, RNG and zero score");
    with_special.recoil = 0;
    const auto before_empty = with_special.branches;
    with_special.ammo_index = 16;
    shoot(special, images, landscape, {244, 20});
    require(with_special.branches == before_empty && with_special.branches_hit == 1 && event(special, game::RoundEventKind::empty), "Empty fire must not detach another branch");
}

void released_slot(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    shoot(round, images, landscape, {114, 20});
    // Unit boundary: the branch crosses the ground on the same update as the
    // existing spider grant. Allocation must observe the newly vacant slot.
    state.branches[0].world_y = 480;
    state.branches[0].velocity_y = 1;
    state.spider_hits = 3;
    step(round, .125F);
    require(state.branches[0].type == -1 && state.balloons[20].type == 21 && state.chickens[20].type == -3,
        "A grounded branch must release its original scene slot before later same-update grants");
    require(event(round, game::RoundEventKind::branch_ground) && event(round, game::RoundEventKind::spider_balloons), "Ground and later grant events must both survive");
    // Place the already allocated balloon in clear sky to isolate reused-slot
    // dispatch from motion/occlusion; no branch-specific handler may consume it.
    state.balloons[20].world_x = 350;
    state.balloons[20].world_y = 100;
    state.recoil = 0;
    game::test::advance(round, state.previous_sample, {.pointer = {345, 105}, .fire_down = true});
    round.resolve_actions([](const auto& draw, auto) {
        return draw.image == "gfx/ballon";
    });
    require(state.selected_slot == 20 && state.balloons[20].type == 61 && state.branches_hit == 1 && event(round, game::RoundEventKind::balloon_hit),
        "Reused branch slot must route a selected live balloon to balloon impact");
}

struct SelectionCase {
    int type;
    std::string_view image, label;
    contracts::ScreenPoint local;
    std::uint16_t scene_word;
    bool selected;
};

constexpr std::array selection_cases{
    SelectionCase{22, "gfx/branch1", "opaque equality", {43, 31}, 27041, true},
    SelectionCase{22, "gfx/branch1", "different low byte occluder", {43, 31}, 27040, false},
    SelectionCase{22, "gfx/branch1", "same low byte different high byte occluder", {43, 31}, 26785, false},
    SelectionCase{22, "gfx/branch1", "equal whole word occluder", {43, 31}, 27041, true},
    SelectionCase{22, "gfx/branch1", "keyed scratch matches black final", {1, 1}, 0, true},
    SelectionCase{22, "gfx/branch1", "keyed scratch differs nonblack final", {1, 1}, 1, false},
    SelectionCase{22, "gfx/branch1", "strict left", {0, 31}, 0, false},
    SelectionCase{22, "gfx/branch1", "strict top", {43, 0}, 0, false},
    SelectionCase{22, "gfx/branch1", "strict right", {78, 31}, 0, false},
    SelectionCase{22, "gfx/branch1", "strict bottom", {43, 60}, 0, false},
    SelectionCase{22, "gfx/branch1", "outside left", {-1, 31}, 0, false},
    SelectionCase{22, "gfx/branch1", "outside bottom", {43, 61}, 0, false},
    SelectionCase{62, "gfx/branch1", "reaction excluded before equality", {43, 31}, 27041, false},
    SelectionCase{23, "gfx/branch2", "opaque equality", {45, 34}, 6338, true},
    SelectionCase{23, "gfx/branch2", "different low byte occluder", {45, 34}, 6339, false},
    SelectionCase{23, "gfx/branch2", "same low byte different high byte occluder", {45, 34}, 6594, false},
    SelectionCase{23, "gfx/branch2", "equal whole word occluder", {45, 34}, 6338, true},
    SelectionCase{23, "gfx/branch2", "keyed scratch matches black final", {1, 1}, 0, true},
    SelectionCase{23, "gfx/branch2", "keyed scratch differs nonblack final", {1, 1}, 1, false},
    SelectionCase{23, "gfx/branch2", "strict left", {0, 34}, 0, false},
    SelectionCase{23, "gfx/branch2", "strict top", {45, 0}, 0, false},
    SelectionCase{23, "gfx/branch2", "strict right", {118, 34}, 0, false},
    SelectionCase{23, "gfx/branch2", "strict bottom", {45, 58}, 0, false},
    SelectionCase{23, "gfx/branch2", "outside left", {-1, 34}, 0, false},
    SelectionCase{23, "gfx/branch2", "outside bottom", {45, 59}, 0, false},
    SelectionCase{63, "gfx/branch2", "reaction excluded before equality", {45, 34}, 6338, false},
};

struct ParticleExpectation {
    int type, depth, frame, points;
    std::array<std::optional<std::uint32_t>, 6> words;
    std::optional<bool> active;
};

struct Allocation {
    std::size_t index;
    ParticleExpectation particle;
};

struct CapacityCase {
    float camera;
    contracts::ScreenPoint pointer;
    std::uint32_t seed;
    int score_before, branch_hits_before, score_after, branch_hits_after, accuracy_delta;
    std::uint64_t total_draws;
    std::uint32_t random_state;
    int branch_type;
    std::array<std::uint32_t, 2> branch_velocity;
    std::vector<std::size_t> free_slots;
    std::vector<Allocation> allocations;
};

const std::array particle_capacity_cases{
    CapacityCase{0.0F, {114, 20}, 1U, 27, 0, 27, 1, 1, 2, 0xc823f683U, 62, {0xbecccccdU, 0x3f333333U}, {}, {}},
    CapacityCase{0.0F, {114, 20}, 0U, 27, 0, 27, 1, 1, 5, 0xa2974c77U, 62, {0x3e4ccccdU, 0x3fc00000U}, {67},
        {{67, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3d8f5c29U, 0x3fd9999aU}, std::nullopt}}}},
    CapacityCase{0.0F, {114, 20}, 6U, 27, 0, 27, 1, 1, 11, 0x93662c5bU, 62, {0xbe99999aU, 0x3f19999aU}, {67, 0, 18},
        {{0, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe199999U, 0x3fe66667U}, std::nullopt}},
            {18, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbdccccccU, 0x3f4ccccdU}, std::nullopt}},
            {67, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe999999U, 0x3fc00000U}, std::nullopt}}}},
    CapacityCase{0.0F, {114, 20}, 10U, 27, 0, 27, 1, 1, 29, 0x2c572831U, 62, {0x3e4ccccdU, 0x3f8ccccdU}, {0, 1, 2, 3, 4, 5, 6, 7, 8},
        {{0, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe947ae1U, 0x3f8ccccdU}, std::nullopt}},
            {1, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe9eb852U, 0x3fc00000U}, std::nullopt}},
            {2, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe8f5c29U, 0x3f000000U}, std::nullopt}},
            {3, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbdccccccU, 0x00000000U}, std::nullopt}},
            {4, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e23d70aU, 0x3e4ccccdU}, std::nullopt}},
            {5, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe8a3d70U, 0x3f4ccccdU}, std::nullopt}},
            {6, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe851eb8U, 0x3fe66667U}, std::nullopt}},
            {7, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbd4cccccU, 0x3fc00000U}, std::nullopt}},
            {8, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3d8f5c29U, 0x3f666667U}, std::nullopt}}}},
    CapacityCase{0.0F, {114, 20}, 12345U, 27, 0, 27, 1, 1, 32, 0x8840b3d9U, 62, {0x3e99999aU, 0x3f99999aU}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        {{0, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbd23d70aU, 0x3fb33333U}, std::nullopt}},
            {1, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe8f5c29U, 0x3ecccccdU}, std::nullopt}},
            {2, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3d4cccccU, 0x3f8ccccdU}, std::nullopt}},
            {3, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e199999U, 0xbdcccccdU}, std::nullopt}},
            {4, {3, 2, 2, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3d23d70aU, 0x3fa66667U}, std::nullopt}},
            {5, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe3851ebU, 0x3fc00000U}, std::nullopt}},
            {6, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e051eb8U, 0x3fd9999aU}, std::nullopt}},
            {7, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e0f5c29U, 0x3e99999aU}, std::nullopt}},
            {8, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e570a3dU, 0x00000000U}, std::nullopt}},
            {9, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e23d70aU, 0x3f4ccccdU}, std::nullopt}}}},
    CapacityCase{0.10000000149011612F, {114, 20}, 1U, 27, 0, 27, 1, 1, 32, 0x54ded7a1U, 62, {0x3e4ccccdU, 0x3f19999aU},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
            45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67},
        {{0, {3, 2, 3, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3cf5c28fU, 0x3fa66667U}, std::nullopt}},
            {1, {3, 2, 2, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3c23d70aU, 0x3e99999aU}, std::nullopt}},
            {2, {3, 2, 1, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3e0f5c29U, 0x3dcccccdU}, std::nullopt}},
            {3, {3, 2, 3, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0xbe6b851eU, 0x3ecccccdU}, std::nullopt}},
            {4, {3, 2, 2, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3e8a3d70U, 0x00000000U}, std::nullopt}},
            {5, {3, 2, 3, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3e428f5cU, 0x3dcccccdU}, std::nullopt}},
            {6, {3, 2, 1, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3e8f5c29U, 0x3f800000U}, std::nullopt}},
            {7, {3, 2, 1, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0x3e999999U, 0x3f99999aU}, std::nullopt}},
            {8, {3, 2, 2, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0xbca3d70aU, 0x00000000U}, std::nullopt}},
            {9, {3, 2, 3, 0, {0x430228f6U, 0x42100000U, std::nullopt, std::nullopt, 0xbe851eb8U, 0x3fb33333U}, std::nullopt}}}},
    CapacityCase{0.0F, {114, 20}, 4294967295U, 27, 0, 27, 1, 1, 14, 0x4077096dU, 62, {0xbecccccdU, 0x3e99999aU}, {1, 66, 35, 9},
        {{1, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3de147aeU, 0x3fa66667U}, std::nullopt}},
            {9, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe4cccccU, 0x3f666667U}, std::nullopt}},
            {35, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0xbe428f5cU, 0x3f000000U}, std::nullopt}},
            {66, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e2e147bU, 0x3fc00000U}, std::nullopt}}}},
    CapacityCase{0.0F, {114, 20}, 123U, 27, 0, 27, 1, 1, 11, 0xcfcd26a4U, 62, {0xbdcccccdU, 0x3f666667U}, {0, 1, 2},
        {{0, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3e051eb8U, 0x3fb33333U}, std::nullopt}},
            {1, {3, 2, 1, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3de147aeU, 0x3ecccccdU}, std::nullopt}},
            {2, {3, 2, 3, 0, {0x43020000U, 0x42100000U, std::nullopt, std::nullopt, 0x3d75c28fU, 0xbdcccccdU}, std::nullopt}}}},
};

struct UpdateCase {
    float camera, delta;
    int recoil;
    ParticleExpectation before, after;
};

const std::array particle_updates{
    UpdateCase{0.0F, 0.0F, 8, {3, 2, 1, 0, {0x43020000U, 0x42100000U, 0x43020000U, 0x42100000U, 0x00000000U, 0x00000000U}, true},
        {3, 2, 1, 0, {0x43020000U, 0x42100000U, 0x43020000U, 0x4229999aU, 0x00000000U, 0x00000000U}, true}},
    UpdateCase{640.6249389648438F, 0.125F, 8, {3, 2, 2, 0, {0x43020000U, 0x42100000U, 0x43020000U, 0x42100000U, 0xbea3d70aU, 0xbdcccccdU}, true},
        {3, 2, 2, 0, {0x42fbf800U, 0x420d8000U, 0xc4608100U, 0x4227199aU, 0xbea47ae1U, 0xbd4ccccdU}, true}},
    UpdateCase{0.0F, 1.125F, 8, {3, 2, 3, 0, {0x43020000U, 0x42100000U, 0x43020000U, 0x42100000U, 0x3e9eb852U, 0x3fe66666U}, true},
        {3, 2, 3, 0, {0x43262400U, 0x43909000U, 0x43262400U, 0x4393c333U, 0x3ea47ae1U, 0x40100000U}, true}},
    UpdateCase{0.0F, 0.0F, 8, {3, 2, 1, 0, {0x43020000U, 0x43f00000U, 0x43020000U, 0x43f00000U, 0x00000000U, 0x00000000U}, true},
        {3, 2, 1, 0, {0x43020000U, 0x43f00000U, 0x43020000U, 0x43f33333U, 0x00000000U, 0x00000000U}, true}},
    UpdateCase{0.0F, 9.999999747378752e-06F, 8, {3, 2, 2, 0, {0x43020000U, 0x43f00000U, 0x43020000U, 0x43f00000U, 0x00000000U, 0x00000000U}, true},
        {3, 2, 2, 0, {0x43020000U, 0x43f00000U, 0x43020000U, 0x43f33333U, 0xb3d6bf94U, 0x368637bdU}, true}},
    UpdateCase{0.0F, 0.0010000000474974513F, 8, {3, 2, 3, 0, {0x43020000U, 0x43f00000U, 0x43020000U, 0x43f00000U, 0x00000000U, 0x00000000U}, true},
        {3, 2, 3, 0, {0x43020000U, 0x43f00001U, 0x43020000U, 0x43f33334U, 0xb727c5adU, 0x39d1b718U}, false}},
    UpdateCase{1280.0F, 0.0F, 1, {3, 2, 1, 0, {0x43020000U, 0x43f00001U, 0x43020000U, 0x43f00001U, 0xbea3d70aU, 0x3fe66666U}, true},
        {3, 2, 1, 0, {0x43020000U, 0x43f00001U, 0xc4efc000U, 0x43f06667U, 0xbea3d70aU, 0x3fe66666U}, false}},
    UpdateCase{0.0F, 1.0F, 8, {3, 2, 1, 0, {0x43020000U, 0x43f00001U, 0x43020000U, 0x43f00001U, 0xbea3d70aU, 0x3fe66666U}, false},
        {3, 2, 1, 0, {0x43020000U, 0x43f00001U, 0x43020000U, 0x43f00001U, 0xbea3d70aU, 0x3fe66666U}, false}},
};

void source_selection(const render::ImageBank& images) {
    for (const auto& row : selection_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = 0;
        // Relocate one branch to keep the strict-edge vectors on screen.
        // A uniform final word isolates the original scratch comparison.
        state.branches[0].type = row.type;
        state.branches[0].world_x = 350;
        state.branches[0].world_y = 100;
        game::test::advance(round, 0, {.pointer = {334 + row.local.x, 84 + row.local.y}, .fire_down = true});
        render::Framebuffer frame;
        frame.clear(row.scene_word);
        round.resolve_actions([&](const auto& draw, auto point) {
            return draw.image == row.image && frame.matches_sprite_pixel(images, draw, point);
        });
        require((state.selected_slot == 20) == row.selected, row.label);
    }
}

void particle_matches(const game::Particle& particle, const ParticleExpectation& expected) {
    require(particle.type == expected.type && particle.depth == expected.depth && particle.frame == expected.frame && particle.points == expected.points,
        "Original particle identity/cell differs");
    const std::array actual{particle.world_x, particle.world_y, particle.screen_x, particle.screen_y, particle.velocity_x, particle.velocity_y};
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (expected.words[index]) {
            require(std::bit_cast<std::uint32_t>(actual[index]) == *expected.words[index], "Original particle float store differs");
        }
    }
    require(particle.age == 0, "Wood debris must not advance popup age");
    if (expected.active) {
        require(particle.active == *expected.active, "Original particle lifetime differs");
    }
}

void source_particles() {
    for (const auto& row : particle_capacity_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = row.camera;
        game::test::advance(round, 0, {.pointer = row.pointer, .fire_down = true});
        // These vectors begin at impact, after ordinary world RNG.
        state.random_state = row.seed;
        state.random_draws = 0;
        state.score = row.score_before;
        state.branches_hit = row.branch_hits_before;
        for (auto& particle : state.particles) {
            particle.active = true;
            particle.points = 777; // Existing occupied entries must survive untouched.
        }
        for (const auto index : row.free_slots) {
            state.particles.at(index).active = false;
        }
        const auto before = state.particles;
        round.resolve_actions([](const auto& draw, auto) {
            return draw.image == "gfx/branch1";
        });
        require(state.selected_slot == 20 && state.hits == row.accuracy_delta && state.score == row.score_after && state.branches_hit == row.branch_hits_after,
            "Capacity vector must traverse real branch selection/impact");
        require(state.random_draws == row.total_draws && state.random_state == row.random_state, "Full/free/fragmented pool CRT order differs");
        require(state.branches[0].type == row.branch_type && std::bit_cast<std::uint32_t>(state.branches[0].velocity_x) == row.branch_velocity[0]
                    && std::bit_cast<std::uint32_t>(state.branches[0].velocity_y) == row.branch_velocity[1],
            "Branch must consume its two CRT draws after successful debris allocations");
        std::array<bool, 68> allocated{};
        for (const auto& allocation : row.allocations) {
            allocated.at(allocation.index) = true;
            require(state.particles.at(allocation.index).active, "Original first-free particle allocation differs");
            particle_matches(state.particles.at(allocation.index), allocation.particle);
        }
        for (std::size_t index = 0; index < state.particles.size(); ++index) {
            require(allocated[index] || state.particles[index] == before[index], "Particle allocation changed an occupied or unused entry");
        }
    }
    for (const auto& row : particle_updates) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        const auto& before = row.before;
        state.camera = row.camera;
        state.recoil = row.recoil;
        state.particles[0] = {.active = *before.active,
            .type = before.type,
            .depth = before.depth,
            .world_x = std::bit_cast<float>(*before.words[0]),
            .world_y = std::bit_cast<float>(*before.words[1]),
            .screen_x = std::bit_cast<float>(*before.words[2]),
            .screen_y = std::bit_cast<float>(*before.words[3]),
            .velocity_x = std::bit_cast<float>(*before.words[4]),
            .velocity_y = std::bit_cast<float>(*before.words[5]),
            .points = before.points,
            .frame = before.frame};
        particle_matches(state.particles[0], before);
        step(round, row.delta);
        particle_matches(state.particles[0], row.after);
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error("Expected assets directory");
        }
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        source_particles();
        source_selection(images);
        ordinary(images, landscape);
        all_branches_and_reset(images, landscape);
        released_slot(images, landscape);
        std::cout << "Branch composed hits, falling/removal, debris, counter, sounds, reused slots and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
