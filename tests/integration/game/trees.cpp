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
    if (round.state().ammo_index == 16) {
        game::test::advance(round, round.state().previous_sample, {.reload_down = true});
        round.resolve_actions({});
    }
    game::test::advance(round, round.state().previous_sample, {.pointer = pointer, .fire_down = true});
    auto draws = scene(round, landscape);
    render::Framebuffer frame;
    frame.draw(images, draws);
    round.resolve_actions([&](const auto& sprite, auto point) {
        return frame.matches_sprite_pixel(images, sprite, point);
    });
}

void detach_branches(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape) {
    auto& state = fixture(round);
    for (const int index : {0, 2, 1, 3}) {
        state.camera = index % 2 == 0 ? 0 : 1200;
        state.recoil = 0;
        shoot(round, images, landscape, index < 2 ? contracts::ScreenPoint{114, 20} : contracts::ScreenPoint{244, 20});
    }
    require(state.branches_hit == 4, "Four original branch aims must establish the actual prerequisite");
}

void crown(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape, int index) {
    auto& state = fixture(round);
    state.camera = index == 1 ? 0 : 1200;
    state.recoil = 0;
    shoot(round, images, landscape, {175, 16});
    require(state.selected_slot == 15 + index, "Source-matched crown aim must select the crown, not its earlier trunk");
}

void full_tree_route(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    crown(round, images, landscape, 1);
    require(state.trees[1].points == 10 && state.score == 0 && state.hits == 1 && event(round, game::RoundEventKind::tree_hit),
        "Ungated crown hit credits accuracy/sound without damage or points");
    state.recoil = 0;
    shoot(round, images, landscape, {189, 164});
    require(state.selected_slot == 15 && state.trees[0].points == 0 && state.hits == 2 && state.score == 0, "Ungated trunk hit must preserve zero stored points");
    require(std::ranges::any_of(state.tree_fragments,
                [](const auto& item) {
                    return item.type == 27;
                }),
        "Ordinary trunk hits must create shootable tree leaves");
    detach_branches(round, images, landscape);
    for (const int index : {1, 3}) {
        for (int hit = 1; hit <= 10; ++hit) {
            crown(round, images, landscape, index);
            require(state.trees[static_cast<std::size_t>(index)].points == 10 - hit, "Exactly four branch hits unlock each distinct crown damage step");
            require(event(round, game::RoundEventKind::tree_destroyed) == (hit == 10), "Only the tenth gated crown hit destroys it");
        }
        require(state.trees[static_cast<std::size_t>(index)].type == 66 && state.trees_destroyed[static_cast<std::size_t>(index / 2)],
            "Destroyed crown must latch the corresponding source flag");
        const auto destruction = std::ranges::find_if(round.events(), [](const auto& value) {
            return value.kind == game::RoundEventKind::tree_destroyed;
        });
        require(destruction != round.events().end() && std::next(destruction)->kind == game::RoundEventKind::tree_hit, "Break sound must precede ordinary wood impact");
    }
    require(state.score == 0 && state.hits == 26 && state.misses == 0 && state.multi_hit.write_index == 0, "Tree and branch reactions must not score or register multiple hits");
    require(std::ranges::count_if(state.tree_fragments,
                [](const auto& item) {
                    return item.type == 28;
                }) == 8
                && std::ranges::count_if(state.tree_fragments,
                       [](const auto& item) {
                           return item.type == 30;
                       })
                       == 2,
        "Two crowns request four released branches and one highfly each");
    require(std::ranges::count_if(state.tree_fragments,
                [](const auto& item) {
                    return item.type == 27;
                })
                >= 32,
        "Both crowns create their sixteen-leaf bursts");
    const auto before_render = round.state();
    const auto draws = scene(round, landscape);
    require(round.state() == before_render, "Tree drawing must not advance simulation state");
    require(std::ranges::count_if(draws,
                [](const auto& draw) {
                    return draw.image == "gfx/tree1_01";
                })
                == 2,
        "Falling crowns replace initial static records");
    int crown_ground = 0;
    int highfly_sounds = 0;
    for (int i = 0; i < 50; ++i) {
        step(round, .125F);
        for (const auto& value : round.events()) {
            crown_ground += value.kind == game::RoundEventKind::tree_ground && (value.slot == 16 || value.slot == 18) ? 1 : 0;
            highfly_sounds += value.kind == game::RoundEventKind::tree_highfly ? 1 : 0;
        }
    }
    require(state.trees[1].type == -1 && state.trees[3].type == -1 && crown_ground == 2 && highfly_sounds == 2, "Both crowns ground once and both highfly sounds fire once");
    const auto removed = scene(round, landscape);
    require(std::ranges::none_of(removed,
                [](const auto& draw) {
                    return draw.image == "gfx/tree1_01";
                }),
        "Removed crowns must not reappear as static art");
    state.camera = 0;
    state.recoil = 0;
    shoot(round, images, landscape, {189, 164});
    require(
        state.trees[0].points == -1 && state.trees[0].type == 26 && state.trees_destroyed[0], "Gated trunk points can go negative without destroying or clearing the crown flag");
    game::Round reset(1, 0);
    require(reset.state().trees == world::initial_trees() && reset.state().trees_destroyed == std::array{false, false} && reset.state().leaves_hit == 0
                && std::ranges::none_of(reset.state().tree_fragments,
                    [](const auto& item) {
                        return item.type >= 0;
                    }),
        "New round must restore all trees and clear flags/effects/counters");
}

void leaf_and_default_collision(const render::ImageBank& images) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    // Independent scene boundaries: a source-initialized leaf occupies a free
    // slot; actual framebuffer equality determines its selection.
    state.tree_fragments[34] = world::make_tree_leaf(0, 400, {0, 100, 0, 0, 10, 0});
    auto& leaf = state.tree_fragments[34];
    leaf.world_x = 350;
    leaf.world_y = 100;
    state.chickens[34].type = -4;
    auto draw = world::tree_fragment_draw(leaf);
    contracts::ScreenPoint local{};
    bool found = false;
    for (int y = 1; y < 60 && !found; ++y) {
        for (int x = 1; x < 60 && !found; ++x) {
            if (images.rgb565_at("gfx/Leaf", x, y) != 0xf81f) {
                local = {x, y};
                found = true;
            }
        }
    }
    require(found, "Original leaf frame0 must expose a visible test word");
    game::test::advance(round, 0, {.pointer = {334 + local.x, 84 + local.y}, .fire_down = true});
    draw = world::tree_fragment_draw(leaf);
    render::Framebuffer frame;
    frame.draw(images, std::span(&draw, 1));
    round.resolve_actions([&](const auto& candidate, auto point) {
        return candidate.image == "gfx/Leaf" && frame.matches_sprite_pixel(images, candidate, point);
    });
    require(state.selected_slot == 34 && leaf.type == 67 && state.leaves_hit == 1 && state.score == 1 && state.hits == 1,
        "Visible source leaf must award one point and increment its counter once");
    require(event(round, game::RoundEventKind::leaf_hit) && round.events().size() == 2, "Leaf impact must emit no extra audio event");
    step(round, .125F);
    require(state.multi_hit.write_index == 0 && leaf.frame == 3 && leaf.type == 67, "Reaction67 animates but remains outside the multiple-hit range");
    for (int i = 0; i < 7; ++i) {
        step(round, .125F);
    }
    require(leaf.type == -1 && state.chickens[34].type == -1, "Leaf death must release its original pool slot");

    for (const int type : {28, 30}) {
        for (const std::uint16_t final_word : {std::uint16_t{0}, std::uint16_t{1}}) {
            game::Round probe(1, 0);
            auto& value = fixture(probe);
            value.camera = 0;
            auto& item = value.tree_fragments[34];
            item = type == 28 ? world::make_tree_branch(0, 400, {0, 0, 0, 5, 0}) : world::make_tree_highfly(0, 400);
            item.world_x = 350;
            item.world_y = 100;
            value.chickens[34].type = -4;
            game::test::advance(probe, 0, {.pointer = {344, 94}, .fire_down = true});
            const auto before = item;
            render::Framebuffer background;
            background.clear(final_word);
            probe.resolve_actions([&](const auto& candidate, auto point) {
                return background.matches_sprite_pixel(images, candidate, point);
            });
            require(value.selected_slot == (final_word == 0 ? 34 : -1) && value.hits == (final_word == 0 ? 1 : 0) && item == before && value.score == 0,
                "Default tree-effect collision compares black scratch but impact changes only accuracy");
        }
    }
}

struct EffectExpectation {
    std::size_t slot;
    bool scene;
    int type, depth, points, frame;
    std::array<std::uint32_t, 4> words;
};

constexpr std::array crown_ungated_effects{
    EffectExpectation{32, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x3cf5c28fU, 0x3fa66667U}},
    EffectExpectation{33, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0x3c23d70aU, 0x3e99999aU}},
    EffectExpectation{34, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0x3e0f5c29U, 0x3dcccccdU}},
};

constexpr std::array trunk_random_loop_effects{
    EffectExpectation{32, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0xbca3d70aU, 0x3f666667U}},
    EffectExpectation{33, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe9eb852U, 0x3f333333U}},
    EffectExpectation{34, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0x3d8f5c29U, 0x3fd9999aU}},
    EffectExpectation{34, true, 27, 3, 1, 6, {0xc11dc28fU, 0xc31b0000U, 0x42380000U, 0x42000000U}},
    EffectExpectation{35, true, 27, 3, 1, 3, {0x41023d71U, 0xc2b20000U, 0x42140000U, 0x42080000U}},
    EffectExpectation{36, true, 27, 3, 1, 1, {0x430523d7U, 0xc3090000U, 0x40a00000U, 0x42480000U}},
    EffectExpectation{37, true, 27, 2, 1, 10, {0xc24b5c29U, 0xc2d20000U, 0x42840000U, 0x42500000U}},
};

constexpr std::array trunk_destroyedflag_skips_loop_effects{
    EffectExpectation{32, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0xbca3d70aU, 0x3f666667U}},
    EffectExpectation{33, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe9eb852U, 0x3f333333U}},
};

constexpr std::array crown_death_full_pools_effects{
    EffectExpectation{32, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x3cf5c28fU, 0x3fa66667U}},
    EffectExpectation{33, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0x3c23d70aU, 0x3e99999aU}},
    EffectExpectation{34, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0x3e0f5c29U, 0x3dcccccdU}},
    EffectExpectation{34, true, 28, 3, 2, 0, {0x436023d7U, 0xc30d0000U, 0x3e4ccccdU, 0x3dcccccdU}},
    EffectExpectation{35, true, 28, 3, 2, 0, {0x432223d7U, 0xc2cc0000U, 0x3e4ccccdU, 0x3fcccccdU}},
    EffectExpectation{36, true, 28, 3, 2, 0, {0xc09b851fU, 0xc2780000U, 0xbe4ccccdU, 0x3f99999aU}},
    EffectExpectation{37, true, 28, 2, 1, 0, {0x4270a3d7U, 0xc3300000U, 0x3e99999aU, 0x3fc00000U}},
    EffectExpectation{38, true, 27, 3, 1, 5, {0x434d23d7U, 0xc3030000U, 0x41c00000U, 0x42780000U}},
    EffectExpectation{39, true, 27, 3, 1, 14, {0x432a23d7U, 0xc2be0000U, 0x41000000U, 0x42240000U}},
    EffectExpectation{40, true, 27, 2, 1, 9, {0x432c28f6U, 0xc3370000U, 0x41f80000U, 0x42840000U}},
    EffectExpectation{41, true, 27, 3, 1, 18, {0x434323d7U, 0xc2d60000U, 0x41880000U, 0x42780000U}},
    EffectExpectation{42, true, 27, 3, 1, 13, {0x42288f5cU, 0xc2a60000U, 0x42080000U, 0x42180000U}},
    EffectExpectation{43, true, 27, 2, 1, 18, {0x429451ecU, 0xc3480000U, 0x42680000U, 0x42740000U}},
    EffectExpectation{44, true, 27, 2, 1, 0, {0x435d28f6U, 0xc2f80000U, 0x42100000U, 0x42200000U}},
    EffectExpectation{45, true, 27, 2, 1, 17, {0xc28fae14U, 0xc3020000U, 0x41800000U, 0x42600000U}},
    EffectExpectation{46, true, 27, 3, 1, 7, {0x434b23d7U, 0xc2b20000U, 0x421c0000U, 0x42080000U}},
    EffectExpectation{47, true, 27, 2, 1, 4, {0xc2a1ae14U, 0xc2980000U, 0x41a80000U, 0x427c0000U}},
    EffectExpectation{48, true, 27, 2, 1, 18, {0x4218a3d7U, 0xc2ac0000U, 0x41d80000U, 0x42540000U}},
    EffectExpectation{49, true, 27, 2, 1, 11, {0x42a251ecU, 0xc33d0000U, 0x422c0000U, 0x428c0000U}},
    EffectExpectation{50, true, 27, 3, 1, 12, {0x434923d7U, 0xc2800000U, 0x42000000U, 0x42740000U}},
    EffectExpectation{51, true, 27, 3, 1, 17, {0x42c247aeU, 0xc2a20000U, 0x41e80000U, 0x42780000U}},
    EffectExpectation{52, true, 27, 2, 1, 12, {0xc13d70a4U, 0xc3090000U, 0x42860000U, 0x42860000U}},
    EffectExpectation{53, true, 27, 2, 1, 16, {0x42b251ecU, 0xc28e0000U, 0x42340000U, 0x42280000U}},
    EffectExpectation{35, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbdccccccU, 0x3f800000U}},
    EffectExpectation{36, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0xbdccccccU, 0x3f666667U}},
    EffectExpectation{37, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbd23d70aU, 0x3f000000U}},
    EffectExpectation{38, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0xbe947ae1U, 0x3f800000U}},
    EffectExpectation{39, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0xbe6b851eU, 0x3fcccccdU}},
    EffectExpectation{40, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x3db851ebU, 0x3e4ccccdU}},
    EffectExpectation{41, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe428f5cU, 0x3f4ccccdU}},
    EffectExpectation{42, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbcf5c28fU, 0x3f333333U}},
    EffectExpectation{43, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x3e3851ebU, 0x3f000000U}},
    EffectExpectation{44, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0xbcf5c28fU, 0x3f333333U}},
    EffectExpectation{45, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbe2e147bU, 0x3dcccccdU}},
    EffectExpectation{46, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe999999U, 0x3fe66667U}},
    EffectExpectation{47, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0x3e800000U, 0x3f333333U}},
    EffectExpectation{48, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbe570a3dU, 0x00000000U}},
    EffectExpectation{49, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbca3d70aU, 0x3f800000U}},
    EffectExpectation{50, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbca3d70aU, 0xbdcccccdU}},
    EffectExpectation{51, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbde147aeU, 0x3f333333U}},
    EffectExpectation{52, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbdf5c28fU, 0x3e99999aU}},
    EffectExpectation{53, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0x3e999999U, 0x3f99999aU}},
    EffectExpectation{54, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0x3e3851ebU, 0x3fd9999aU}},
    EffectExpectation{55, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbe4cccccU, 0x3f19999aU}},
    EffectExpectation{56, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe23d70aU, 0x3f99999aU}},
    EffectExpectation{57, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbe051eb8U, 0x3f19999aU}},
    EffectExpectation{58, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0x3d75c28fU, 0x3fcccccdU}},
    EffectExpectation{59, false, 3, 2, 0, 2, {0x430b28f6U, 0x42600000U, 0x3dccccccU, 0x3f4ccccdU}},
    EffectExpectation{60, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x3e6b851eU, 0x3f800000U}},
    EffectExpectation{61, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe9eb852U, 0x3fe66667U}},
    EffectExpectation{62, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe3851ebU, 0x3e4ccccdU}},
    EffectExpectation{63, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbea3d70aU, 0x3e99999aU}},
    EffectExpectation{64, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbe428f5cU, 0x3dcccccdU}},
    EffectExpectation{65, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbe8a3d70U, 0x3f99999aU}},
    EffectExpectation{66, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0x3e8a3d70U, 0x3f19999aU}},
    EffectExpectation{54, true, 30, 2, 0, 0, {0x419947aeU, 0xc3f00000U, 0x00000000U, 0x40800000U}},
};

constexpr std::array<EffectExpectation, 0> crown_death_exhausted_pools_effects{};

constexpr std::array crown_death_one_scene_hole_effects{
    EffectExpectation{20, true, 28, 2, 1, 0, {0xc285ae14U, 0xc3170000U, 0xbecccccdU, 0x3fd9999aU}},
};

constexpr std::array crown_death_scene_holes_wood_partial_effects{
    EffectExpectation{32, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbca3d70aU, 0x3e4ccccdU}},
    EffectExpectation{34, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x3d8f5c29U, 0x3fd9999aU}},
    EffectExpectation{39, false, 3, 2, 0, 1, {0x430b28f6U, 0x42600000U, 0xbe851eb8U, 0x3fb33333U}},
    EffectExpectation{20, true, 28, 2, 1, 0, {0x42ac51ecU, 0xc27c0000U, 0xbecccccdU, 0x3fb33333U}},
    EffectExpectation{23, true, 28, 2, 1, 0, {0x430f28f6U, 0xc3030000U, 0xbe4ccccdU, 0x3dcccccdU}},
    EffectExpectation{34, true, 28, 2, 1, 0, {0xc293ae14U, 0xc29c0000U, 0x3e99999aU, 0x3ecccccdU}},
    EffectExpectation{37, true, 28, 3, 2, 0, {0xbf5c28f6U, 0xc3070000U, 0x00000000U, 0x3e4ccccdU}},
    EffectExpectation{45, true, 27, 3, 1, 3, {0x436623d7U, 0xc2940000U, 0x40c00000U, 0x42100000U}},
    EffectExpectation{76, true, 27, 3, 1, 3, {0xc0770a3dU, 0xc2c20000U, 0x41500000U, 0x42140000U}},
    EffectExpectation{52, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0x00000000U, 0x3fcccccdU}},
};

constexpr std::array crown_exact_branchcount_effects{
    EffectExpectation{32, false, 3, 2, 0, 3, {0x430b28f6U, 0x42600000U, 0xbca3d70aU, 0x3e4ccccdU}},
};

struct TreeExpectation {
    int type, points;
    std::array<std::uint32_t, 2> velocity;
};

struct CapacityCase {
    std::string_view name;
    float camera;
    contracts::ScreenPoint pointer;
    std::uint32_t seed;
    int branch_hits;
    bool special_ammo;
    std::array<bool, 2> flags_before, flags_after;
    std::vector<std::size_t> scene_free, particle_free;
    int frame, points_before;
    TreeExpectation tree;
    std::uint32_t random_state;
    std::uint64_t random_draws;
    std::span<const EffectExpectation> effects;
    std::vector<int> scene_free_after, particle_free_after;
};

const std::array tree_capacity_cases{
    CapacityCase{"crown-ungated", 0.1F, {123, 40}, 1U, 3, false, {false, false}, {false, false}, {34, 35, 36}, {32, 33, 34}, 0, 10, {26, 10, {0x00000000U, 0x00000000U}},
        3750785579U, 10, crown_ungated_effects, {34, 35, 36}, {}},
    CapacityCase{"trunk-random-loop", 0.1F, {123, 40}, 234U, 0, false, {false, false}, {false, false}, {34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49},
        {32, 33, 34, 35, 36, 37, 38, 39, 40, 41}, 1, 0, {26, 0, {0x00000000U, 0x00000000U}}, 2328143064U, 38, trunk_random_loop_effects,
        {38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49}, {35, 36, 37, 38, 39, 40, 41}},
    CapacityCase{"trunk-destroyedflag-skips-loop", 0.1F, {123, 40}, 234U, 4, false, {true, false}, {true, false}, {34, 35}, {32, 33}, 1, 0, {26, -1, {0x00000000U, 0x00000000U}},
        861675448U, 6, trunk_destroyedflag_skips_loop_effects, {34, 35}, {}},
    CapacityCase{"crown-death-full-pools", 0.1F, {123, 40}, 1U, 4, false, {false, false}, {true, false},
        {34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
            77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
            115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127},
        {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
            75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99},
        0, 1, {66, 0, {0xbdcccccdU, 0x3e99999aU}}, 2770318689U, 224, crown_death_full_pools_effects,
        {55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
            98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127},
        {67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99}},
    CapacityCase{"crown-death-exhausted-pools", 0.1F, {123, 40}, 3U, 4, false, {true, false}, {true, true}, {}, {}, 2, 1, {66, 0, {0x3dcccccdU, 0x3fb33333U}}, 2756632324U, 3,
        crown_death_exhausted_pools_effects, {}, {}},
    CapacityCase{"crown-death-one-scene-hole", 0.1F, {123, 40}, 3U, 4, false, {false, false}, {true, false}, {20}, {}, 0, 1, {66, 0, {0xbe99999aU, 0x3e99999aU}}, 1540352939U, 8,
        crown_death_one_scene_hole_effects, {}, {}},
    CapacityCase{"crown-death-scene-holes-wood-partial", 0.1F, {123, 40}, 56U, 4, false, {true, false}, {true, true}, {20, 23, 34, 37, 45, 76}, {32, 34, 39, 52}, 2, 1,
        {66, 0, {0xbe4ccccdU, 0x3f19999aU}}, 814388297U, 47, crown_death_scene_holes_wood_partial_effects, {}, {}},
    CapacityCase{"crown-exact-branchcount", 0.1F, {123, 40}, 56U, 5, false, {true, false}, {true, false}, {34}, {32}, 2, 1, {26, 1, {0x00000000U, 0x00000000U}}, 2573120124U, 4,
        crown_exact_branchcount_effects, {34}, {}},
    CapacityCase{"crown-special-identical", 0.1F, {123, 40}, 1U, 4, true, {false, false}, {true, false},
        {34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
            77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
            115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127},
        {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
            75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99},
        0, 1, {66, 0, {0xbdcccccdU, 0x3e99999aU}}, 2770318689U, 224, crown_death_full_pools_effects,
        {55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
            98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127},
        {67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99}},
};

void capacity_cases() {
    for (const auto& expected : tree_capacity_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = expected.camera;
        game::test::advance(round, 0, {.pointer = expected.pointer, .fire_down = true});
        state.random_state = expected.seed;
        state.random_draws = 0;
        state.branches_hit = expected.branch_hits;
        state.special_ammo = expected.special_ammo;
        state.trees_destroyed = expected.flags_before;
        for (auto& chicken : state.chickens) {
            chicken.type = -2;
        }
        for (auto& branch : state.branches) {
            branch.type = -1;
        }
        for (const auto slot : expected.scene_free) {
            state.chickens[slot].type = -1;
        }
        for (auto& particle : state.particles) {
            particle.active = true;
            particle.type = 3;
        }
        for (const auto slot : expected.particle_free) {
            state.particles[slot - 32].active = false;
        }
        const std::size_t index = expected.frame == 0 ? 1 : expected.frame == 1 ? 0 : expected.frame == 2 ? 3 : 2;
        auto& tree = state.trees[index];
        tree.points = expected.points_before;
        // Isolate hit-local source vectors from earlier update RNG and pose.
        // Actual artwork/camera selection is covered by the full normal route.
        tree.screen_x = 100;
        tree.screen_y = 0;
        const auto image = world::tree_draw(tree).image;
        round.resolve_actions([&](const auto& draw, auto) {
            return draw.image == image;
        });
        const auto prefix = std::string(expected.name) + ": ";
        require(state.selected_slot == 15 + static_cast<int>(index), prefix + "hit-local target selection");
        require(state.random_state == expected.random_state && state.random_draws == expected.random_draws, prefix + "ordered allocator/loop RNG");
        require(tree.type == expected.tree.type && tree.points == expected.tree.points && state.trees_destroyed == expected.flags_after && state.score == 0 && state.hits == 1,
            prefix + "tree flags, damage, score and accuracy");
        require(std::array{std::bit_cast<std::uint32_t>(tree.velocity_x), std::bit_cast<std::uint32_t>(tree.velocity_y)} == expected.tree.velocity, prefix + "tree fall velocity");
        for (const auto& action : expected.effects) {
            std::array<std::uint32_t, 4> words{};
            if (action.scene) {
                const auto& actual = state.tree_fragments[action.slot];
                require(actual.type == action.type && actual.depth == action.depth && actual.points == action.points && actual.frame == action.frame
                            && state.chickens[action.slot].type == -4,
                    prefix + "first-free fragment state");
                words = {std::bit_cast<std::uint32_t>(actual.world_x), std::bit_cast<std::uint32_t>(actual.world_y), std::bit_cast<std::uint32_t>(actual.velocity_x),
                    std::bit_cast<std::uint32_t>(actual.velocity_y)};
            } else {
                const auto& actual = state.particles[action.slot - 32];
                require(actual.active && actual.type == 3 && actual.frame == action.frame, prefix + "wood particle state");
                words = {std::bit_cast<std::uint32_t>(actual.world_x), std::bit_cast<std::uint32_t>(actual.world_y), std::bit_cast<std::uint32_t>(actual.velocity_x),
                    std::bit_cast<std::uint32_t>(actual.velocity_y)};
            }
            require(words == action.words, prefix + "allocated effect float stores");
        }
        std::vector<int> scene_free, particle_free;
        for (std::size_t slot = 0; slot < state.chickens.size(); ++slot) {
            if (state.chickens[slot].type == -1) {
                scene_free.push_back(static_cast<int>(slot));
            }
        }
        for (std::size_t slot = 0; slot < state.particles.size(); ++slot) {
            if (!state.particles[slot].active) {
                particle_free.push_back(static_cast<int>(slot) + 32);
            }
        }
        require(scene_free == expected.scene_free_after && particle_free == expected.particle_free_after, prefix + "remaining allocator holes");
    }
}

void frog_pool_boundaries(const render::ImageBank& images, const world::Presentation& landscape) {
    for (const int available : {0, 1, 7, 20, 30}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = 0;
        state.trees_destroyed = {true, true};
        for (auto& chicken : state.chickens) {
            chicken.type = -2;
        }
        for (int i = 0; i < available; ++i) {
            state.chickens[static_cast<std::size_t>(34 + i)].type = -1;
        }
        const auto draws = state.random_draws;
        const auto leaf_draws = game::test::leaf_draw_count(state, 1);
        shoot(round, images, landscape, {506, 344});
        const int allocated = std::min(20, available);
        require(state.selected_slot == 19 && state.score == 150 && state.frog_hits == 1 && state.frog.type == 51 && state.hits == 1,
            "Tree-dependent frog bonus must survive full or partial scene capacity");
        require(std::ranges::count_if(state.balloons,
                    [](const auto& item) {
                        return item.type == 21;
                    }) == allocated
                    && state.random_draws == draws + 1 + leaf_draws + static_cast<unsigned>(allocated * 5),
            "Only successful first-free frog balloons consume five draws after the live frog update");
        for (int i = 0; i < allocated; ++i) {
            require(state.balloons[static_cast<std::size_t>(34 + i)].type == 21, "Frog balloons must fill ascending free slots");
        }
        const auto events = round.events();
        require(events.size() == 3 && events[1].kind == game::RoundEventKind::frog_tree_bonus && events[2].kind == game::RoundEventKind::frog_hit,
            "Frog bonus audio precedes ordinary frog impact even without room for balloons");
        require(std::ranges::any_of(state.particles,
                    [](const auto& particle) {
                        return particle.active && particle.type == 2 && particle.points == 150 && particle.world_x == 470 && particle.world_y == 370;
                    }),
            "Upgraded frog popup must use its distinct source origin");
        state.recoil = 0;
        shoot(round, images, landscape, {506, 344});
        require(state.score == 150 && state.frog_hits == 1 && !event(round, game::RoundEventKind::frog_tree_bonus), "Frog death cannot grant a second tree-dependent bonus");
    }
}

struct FrogCase {
    bool first_tree, second_tree;
    float remaining;
    int points, balloons;
    contracts::ScreenPoint popup_origin;
    int type;
};

constexpr std::array frog_cases{
    FrogCase{false, false, 90.0F, 5, 0, {490, 370}, 51},
    FrogCase{true, false, 90.0F, 5, 0, {490, 370}, 51},
    FrogCase{false, true, 90.0F, 5, 0, {490, 370}, 51},
    FrogCase{true, true, 80.0F, 5, 0, {490, 370}, 51},
    FrogCase{true, true, 80.00000762939453F, 150, 20, {470, 370}, 51},
    FrogCase{true, true, 79.99999237060547F, 5, 0, {490, 370}, 51},
    FrogCase{true, true, 90.0F, 150, 20, {470, 370}, 51},
};

void frog_dependency() {
    for (const auto& expected : frog_cases) {
        world::Frog frog;
        const auto impact = world::hit_frog(frog, expected.first_tree, expected.second_tree, expected.remaining);
        require(impact.points == expected.points && impact.balloon_count == expected.balloons && frog.points == impact.points && impact.popup.world_x == expected.popup_origin.x
                    && impact.popup.world_y == expected.popup_origin.y && frog.type == expected.type,
            "Independent tree-dependent frog boundary, points, balloons and popup must agree");
        static_cast<void>(world::update_frog(frog, .125F, 0, 0));
        require(frog.type == 51, "Upgraded frog must retain the ordinary death animation");
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected assets directory");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        full_tree_route(images, landscape);
        leaf_and_default_collision(images);
        frog_dependency();
        frog_pool_boundaries(images, landscape);
        capacity_cases();
        std::cout << "Trees: branch-gated crowns, ordered effects, leaves, default collision, sounds, frog dependency and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
