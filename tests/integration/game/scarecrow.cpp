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
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::count_event;
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::require;

void step(game::Round& round, float delta = 0) {
    // Separate the scene rule from ordinary timed spawning in these unit cases.
    fixture(round).spawn_accumulator = 0;
    game::test::step(round, delta);
}

std::vector<contracts::SpriteDraw> scene(const game::Round& round, const world::Presentation& landscape) {
    return render::round_scene(round, landscape);
}

void shoot(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape, int frame, std::optional<std::uint32_t> hit_seed = {}) {
    // Literal source-visible aims from A02.scarecrow at camera1280/recoil0.
    constexpr std::array pointers{contracts::ScreenPoint{153, 217}, contracts::ScreenPoint{179, 217}, contracts::ScreenPoint{158, 237}, contracts::ScreenPoint{175, 237},
        contracts::ScreenPoint{166, 218}, contracts::ScreenPoint{164, 200}, contracts::ScreenPoint{170, 184}, contracts::ScreenPoint{167, 247}, contracts::ScreenPoint{166, 206}};
    auto& state = fixture(round);
    state.camera = 1280;
    state.recoil = 0;
    const auto pointer = pointers[static_cast<std::size_t>(frame)];
    game::test::advance(round, state.previous_sample, {.pointer = pointer, .fire_down = true});
    if (hit_seed) {
        state.random_state = *hit_seed;
        state.random_draws = 0;
    }
    auto draws = scene(round, landscape);
    render::Framebuffer framebuffer;
    framebuffer.draw(images, draws);
    round.resolve_actions([&](const auto& sprite, auto point) {
        return framebuffer.matches_sprite_pixel(images, sprite, point);
    });
}

void settle(game::Round& round) {
    for (int i = 0; i < 32; ++i) {
        step(round, .125F);
    }
}

void ordered(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    shoot(round, images, landscape, 7);
    require(
        state.selected_slot == -1 && state.hits == 0 && state.misses == 1 && state.scarecrow_hits == 0, "Opaque nonblack pole pixel must miss its default cleared scratch surface");
    constexpr std::array totals{5, 15, 30, 50, 75, 205};
    for (int frame = 0; frame < 6; ++frame) {
        const auto draws_before = state.random_draws;
        const auto leaf_draws = game::test::leaf_draw_count(state, 1);
        shoot(round, images, landscape, frame);
        require(state.selected_slot == 7 + frame && state.score == totals[static_cast<std::size_t>(frame)] && state.hits == frame + 1 && state.misses == 1,
            "Source-composed body input must select its original slot and award the source score");
        require(state.scarecrow[static_cast<std::size_t>(frame + 1)].type == 57 && state.scarecrow_hits == (frame == 5 ? 7 : frame + 1) && state.scarecrow_ordered,
            "Body hit must enter falling57 and preserve the ordered counter");
        require(state.random_draws == draws_before + leaf_draws + (frame == 5 ? 5 : 3), "Frog and persistent leaf draws precede two velocity draws per released body piece");
        require(count_event(round, game::RoundEventKind::scarecrow_hit) == 1 && !state.scarecrow_complete, "One direct impact sound precedes deferred completion");
        if (frame == 5) {
            require(state.scarecrow[7].type == 57, "Frame5 must release the adjacent live hat6");
            require(std::ranges::count_if(state.particles,
                        [](const auto& p) {
                            return p.active && p.type == 2 && p.points == 30 && p.world_x == 1954 && p.world_y == 216 && p.depth == 3;
                        })
                        == 2,
                "Automatic hat requests two selected-head 30 popups at the head position");
            require(std::ranges::none_of(state.particles,
                        [](const auto& p) {
                            return p.active && p.points == 50;
                        }),
                "Automatic hat's double 50 score must not invent a 50 popup");
            const auto before_bonus = state.random_draws;
            const auto bonus_leaves = game::test::leaf_draw_count(state, 1);
            step(round);
            require(state.scarecrow_complete && count_event(round, game::RoundEventKind::scarecrow_balloons) == 1 && state.random_draws == before_bonus + 121 + bonus_leaves,
                "Next update must request 24 balloons after the frog draw and play magic once");
            require(std::ranges::count(state.balloons, 21, &world::Balloon::type) == 24 && state.score == 205 && !state.special_ammo,
                "Ordered completion adds 24 balloons without extra score or ammunition");
        }
        settle(round);
        require(state.scarecrow[static_cast<std::size_t>(frame + 1)].type == 500 && state.chickens[static_cast<std::size_t>(frame + 7)].type == -2,
            "Scarecrow landing must become invisible type 500 without freeing its scene slot");
        require(state.multi_hit.write_index == 0 && state.score == totals[static_cast<std::size_t>(frame)], "Reaction57 must not enter the multi-hit tracker");
    }
    const auto before_render = round.state();
    const auto finished = scene(round, landscape);
    require(before_render == round.state()
                && std::ranges::none_of(finished,
                    [](const auto& draw) {
                        return draw.image == "gfx/scheuche";
                    }),
        "Fallen pieces stay absent and composition cannot mutate state");
    require(std::ranges::count(finished, std::string_view{"gfx/pole"}, &contracts::SpriteDraw::image) == 1, "The pole remains visible after completion");
    require(state.ammo_index == 15, "Six direct body shots plus pole miss must spend seven cartridges");
    step(round);
    require(!count_event(round, game::RoundEventKind::scarecrow_balloons), "Completion cannot repeat");
    game::Round reset(1, 0);
    require(reset.state().scarecrow == world::initial_scarecrow() && reset.state().scarecrow_hits == 0 && reset.state().scarecrow_ordered && !reset.state().scarecrow_complete,
        "New round restores every body, the ordered latch and the completion latch");
}

void wrong_order(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round round(1, 0);
    auto& state = fixture(round);
    int total = 0;
    constexpr std::array order{6, 0, 1, 2, 3, 4, 5};
    constexpr std::array values{50, 5, 10, 15, 20, 25, 30};
    for (std::size_t i = 0; i < order.size(); ++i) {
        shoot(round, images, landscape, order[i]);
        total += values[i];
        require(state.selected_slot == 7 + order[i] && state.score == total && state.scarecrow_hits == static_cast<int>(i + 1) && !state.scarecrow_ordered,
            "Hat-first order must use seven direct awards and latch the wrong order");
        if (i != order.size() - 1) {
            settle(round);
        }
    }
    require(state.score == 155 && state.hits == 7 && !state.scarecrow_complete, "A previously shot hat suppresses both automatic 50 additions");
    step(round);
    require(state.scarecrow_complete && count_event(round, game::RoundEventKind::spawn) == 10 && count_event(round, game::RoundEventKind::scarecrow_chickens) == 1
                && !count_event(round, game::RoundEventKind::scarecrow_balloons),
        "Wrong-order completion requests ten ordinary horizontal chickens with no magic sample");
    require(std::ranges::count_if(state.chickens,
                [](const auto& c) {
                    return c.type == 2 || c.type == 3;
                }) == 10
                && state.score == 155 && state.hits == 7,
        "Wrong-order reward must be actual shootable production chickens without a direct bonus");
    require(std::ranges::all_of(state.chickens.begin() + 6, state.chickens.begin() + 14,
                [](const auto& c) {
                    return c.type == -2;
                }),
        "All pole/body slots remain reserved during completion allocations");
    auto resumed = round.state();
    resumed.previous_sample += 100;
    resumed.delta = 0;
    round.reset_sample(resumed.previous_sample);
    require(round.state() == resumed, "Clock resumption preserves the completed scene state");
}

void capacity_and_selection(const render::ImageBank& images, const world::Presentation& landscape) {
    game::Round actual_black(1, 0);
    shoot(actual_black, images, landscape, 8);
    require(actual_black.state().selected_slot == 6 && actual_black.state().hits == 1 && actual_black.state().score == 0 && actual_black.state().scarecrow_hits == 0,
        "A source-black head pixel inside the pole rectangle must select the earlier default-scratch pole");
    for (const bool correct : {false, true}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        for (auto& c : state.chickens) {
            c.type = -2;
        }
        state.scarecrow_hits = 7;
        state.scarecrow_ordered = correct;
        const auto before = state.random_draws;
        const auto leaf_draws = game::test::leaf_draw_count(state, 1);
        step(round);
        require(state.random_draws == before + 1 + leaf_draws && state.scarecrow_complete && state.score == 0,
            "Full scene consumes frog and persistent leaf RNG while completion still latches");
        state.chickens[34].type = -1;
        step(round);
        require(state.chickens[34].type == -1, "Later free capacity cannot retry a completed grant");
    }
    game::Round black(1, 0);
    auto& state = fixture(black);
    state.camera = 1280;
    game::test::advance(black, 0, {.pointer = {167, 247}, .fire_down = true});
    black.resolve_actions([](const auto& draw, auto) {
        return draw.image.empty();
    });
    require(state.selected_slot == 6 && state.hits == 1 && state.score == 0 && state.scarecrow_hits == 0 && state.scarecrow[0].type == 16
                && !count_event(black, game::RoundEventKind::scarecrow_hit),
        "Default pole scratch can match black and credit accuracy without reaction, points or sample");
}

struct SpawnExpectation {
    std::size_t slot;
    int type, depth, frame, points;
    float world_x, world_y, screen_x, screen_y, velocity_x, velocity_y, phase, animation;
};

struct CompletionCase {
    bool ordered;
    int hits;
    std::vector<std::size_t> scene_holes;
    std::uint32_t seed, random_state;
    float camera;
    std::uint64_t random_draws;
    std::vector<SpawnExpectation> born;
};

const std::array completion_cases{
    CompletionCase{false, 7, {}, 1U, 1U, 640.0F, 0, {}},
    CompletionCase{false, 7, {}, 12345U, 12345U, 640.0F, 0, {}},
    CompletionCase{false, 7, {34, 47, 127}, 1U, 3750785579U, 640.0F, 10,
        {{34, 3, 2, 0, 5, 1694.0F, 66.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {47, 3, 2, 0, 5, 1694.0F, 148.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {127, 2, 3, 0, 10, 727.0F, 112.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}}},
    CompletionCase{false, 7, {34, 47, 127}, 12345U, 980411738U, 640.0F, 11,
        {{34, 2, 2, 0, 5, 871.0F, 147.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {47, 3, 2, 0, 5, 1694.0F, 171.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {127, 2, 2, 0, 5, 830.0F, 145.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}}},
    CompletionCase{false, 7, {34, 35, 42, 50, 62, 78, 89, 95, 110, 127}, 1U, 756189987U, 640.0F, 34,
        {{34, 3, 2, 0, 5, 1694.0F, 66.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {35, 3, 2, 0, 5, 1694.0F, 148.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {42, 2, 3, 0, 10, 727.0F, 112.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {50, 3, 2, 0, 5, 1694.0F, 15.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {62, 3, 4, 0, 25, 1438.0F, 21.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {78, 2, 4, 0, 25, 591.0F, 68.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {89, 2, 4, 0, 25, 556.0F, 103.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {95, 2, 2, 0, 5, 861.0F, 228.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {110, 3, 3, 0, 10, 1565.0F, 57.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {127, 3, 3, 0, 10, 1565.0F, 110.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}}},
    CompletionCase{false, 7, {34, 35, 42, 50, 62, 78, 89, 95, 110, 127}, 12345U, 2082081051U, 640.0F, 34,
        {{34, 2, 2, 0, 5, 871.0F, 147.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {35, 3, 2, 0, 5, 1694.0F, 171.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {42, 2, 2, 0, 5, 830.0F, 145.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {50, 3, 2, 0, 5, 1694.0F, 28.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {62, 2, 3, 0, 10, 731.0F, 24.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {78, 3, 4, 0, 25, 1438.0F, 22.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {89, 2, 2, 0, 5, 822.0F, 180.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {95, 3, 2, 0, 5, 1694.0F, 252.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F},
            {110, 3, 2, 0, 5, 1694.0F, 236.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}, {127, 3, 2, 0, 5, 1694.0F, 230.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F}}},
    CompletionCase{true, 7, {}, 1U, 1U, 640.0F, 0, {}},
    CompletionCase{true, 7, {}, 12345U, 12345U, 640.0F, 0, {}},
    CompletionCase{true, 7, {34, 47, 127}, 1U, 2800339366U, 640.0F, 15,
        {{34, 21, 0, 3, 4, 937.0F, 340.0F, -1024.0F, -1024.0F, 22.0F, 60.0F, 0.0F, 0.0F}, {47, 21, 0, 2, 3, 1260.0F, 340.0F, -1024.0F, -1024.0F, 6.0F, 42.0F, 0.0F, 0.0F},
            {127, 21, 0, 1, 2, 1481.0F, 340.0F, -1024.0F, -1024.0F, 9.0F, 47.0F, 0.0F, 0.0F}}},
    CompletionCase{true, 7, {34, 47, 127}, 12345U, 2772357694U, 640.0F, 15,
        {{34, 21, 0, 0, 1, 1440.0F, 340.0F, -1024.0F, -1024.0F, -5.0F, 65.0F, 0.0F, 0.0F}, {47, 21, 0, 1, 2, 1261.0F, 340.0F, -1024.0F, -1024.0F, -3.0F, 52.0F, 0.0F, 0.0F},
            {127, 21, 0, 0, 1, 1135.0F, 340.0F, -1024.0F, -1024.0F, 19.0F, 32.0F, 0.0F, 0.0F}}},
    CompletionCase{true, 7, {34, 35, 42, 50, 62, 78, 89, 95, 110, 127}, 1U, 2597632883U, 640.0F, 50,
        {{34, 21, 0, 3, 4, 937.0F, 340.0F, -1024.0F, -1024.0F, 22.0F, 60.0F, 0.0F, 0.0F}, {35, 21, 0, 2, 3, 1260.0F, 340.0F, -1024.0F, -1024.0F, 6.0F, 42.0F, 0.0F, 0.0F},
            {42, 21, 0, 1, 2, 1481.0F, 340.0F, -1024.0F, -1024.0F, 9.0F, 47.0F, 0.0F, 0.0F}, {50, 21, 0, 3, 4, 1387.0F, 340.0F, -1024.0F, -1024.0F, -2.0F, 47.0F, 0.0F, 0.0F},
            {62, 21, 0, 0, 1, 1287.0F, 340.0F, -1024.0F, -1024.0F, 22.0F, 53.0F, 0.0F, 0.0F}, {78, 21, 0, 1, 2, 1118.0F, 340.0F, -1024.0F, -1024.0F, 20.0F, 58.0F, 0.0F, 0.0F},
            {89, 21, 0, 2, 3, 1223.0F, 340.0F, -1024.0F, -1024.0F, 11.0F, 38.0F, 0.0F, 0.0F}, {95, 21, 0, 3, 4, 968.0F, 340.0F, -1024.0F, -1024.0F, 19.0F, 75.0F, 0.0F, 0.0F},
            {110, 21, 0, 3, 4, 1439.0F, 340.0F, -1024.0F, -1024.0F, 18.0F, 53.0F, 0.0F, 0.0F}, {127, 21, 0, 1, 2, 1080.0F, 340.0F, -1024.0F, -1024.0F, 23.0F, 73.0F, 0.0F, 0.0F}}},
    CompletionCase{true, 7, {34, 35, 42, 50, 62, 78, 89, 95, 110, 127}, 12345U, 3325886315U, 640.0F, 50,
        {{34, 21, 0, 0, 1, 1440.0F, 340.0F, -1024.0F, -1024.0F, -5.0F, 65.0F, 0.0F, 0.0F}, {35, 21, 0, 1, 2, 1261.0F, 340.0F, -1024.0F, -1024.0F, -3.0F, 52.0F, 0.0F, 0.0F},
            {42, 21, 0, 0, 1, 1135.0F, 340.0F, -1024.0F, -1024.0F, 19.0F, 32.0F, 0.0F, 0.0F}, {50, 21, 0, 2, 3, 1532.0F, 340.0F, -1024.0F, -1024.0F, 0.0F, 55.0F, 0.0F, 0.0F},
            {62, 21, 0, 1, 2, 1514.0F, 340.0F, -1024.0F, -1024.0F, 6.0F, 64.0F, 0.0F, 0.0F}, {78, 21, 0, 1, 2, 1141.0F, 340.0F, -1024.0F, -1024.0F, -4.0F, 28.0F, 0.0F, 0.0F},
            {89, 21, 0, 4, 100, 1044.0F, 340.0F, -1024.0F, -1024.0F, 13.0F, 50.0F, 0.0F, 0.0F}, {95, 21, 0, 0, 1, 1112.0F, 340.0F, -1024.0F, -1024.0F, 23.0F, 22.0F, 0.0F, 0.0F},
            {110, 21, 0, 1, 2, 1250.0F, 340.0F, -1024.0F, -1024.0F, 1.0F, 44.0F, 0.0F, 0.0F}, {127, 21, 0, 0, 1, 1103.0F, 340.0F, -1024.0F, -1024.0F, 9.0F, 58.0F, 0.0F, 0.0F}}},
};

struct HatCase {
    std::vector<int> holes;
};

const std::array hat_cases{HatCase{{}}, HatCase{{32}}, HatCase{{32, 99}}};

struct FallExpectation {
    std::size_t index;
    int type;
    std::uint32_t velocity_x, velocity_y;
};

constexpr std::array hat_falls{
    FallExpectation{6, 57, 0xbe75c28fU, 0xbee66667U},
    FallExpectation{7, 57, 0x3e4cccccU, 0xbf000000U},
};

void source_capacities(const render::ImageBank& images, const world::Presentation& landscape) {
    for (const auto& row : completion_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        for (auto& chicken : state.chickens) {
            chicken.type = -2;
        }
        for (const int slot : row.scene_holes) {
            state.chickens[static_cast<std::size_t>(slot)].type = -1;
        }
        for (std::size_t index = 1; index < state.scarecrow.size(); ++index) {
            state.scarecrow[index].type = 500;
        }
        state.frog.type = -1;
        state.camera = row.camera;
        state.scarecrow_hits = row.hits;
        state.scarecrow_ordered = row.ordered;
        state.random_state = row.seed;
        state.random_draws = 0;
        // Literal completion vectors start at the grant boundary. Normal update
        // ordering and its leaf consumers are covered by integration.game.persistent_leaves.
        game::RoundTestAccess::grant_scarecrow(round);
        require(state.random_state == row.random_state && state.random_draws == row.random_draws && state.scarecrow_complete && state.score == 0 && state.hits == 0
                    && state.ammo_index == 8,
            "Original completion capacity vector must match exact CRT state/draw count and non-score effects");
        for (const auto& entry : row.born) {
            const auto index = entry.slot;
            const auto& value = entry;
            if (state.scarecrow_ordered) {
                const auto& actual = state.balloons[index];
                require(state.chickens[index].type == -3 && actual.type == value.type && actual.world_x == value.world_x && actual.world_y == value.world_y
                            && actual.screen_x == value.screen_x && actual.screen_y == value.screen_y && actual.velocity_x == value.velocity_x
                            && actual.velocity_y == value.velocity_y && actual.frame == value.frame && actual.points == value.points,
                    "Original balloon capacity vector must match every born slot/state");
            } else {
                const auto& actual = state.chickens[index];
                require(actual.type == value.type && actual.depth == value.depth && actual.world_x == value.world_x && actual.world_y == value.world_y
                            && actual.screen_x == value.screen_x && actual.screen_y == value.screen_y && actual.phase == value.phase && actual.animation_time == value.animation
                            && actual.frame == value.frame && actual.points == value.points,
                    "Original forced-type2 factory vector must match slot, direction, depth, position and score");
            }
        }
        require(count_event(round, game::RoundEventKind::scarecrow_balloons) == (state.scarecrow_ordered ? 1 : 0),
            "Source completion sound remains independent of successful allocation count");
    }
    for (const auto& row : hat_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.frog.type = -1;
        state.scarecrow_hits = 5;
        state.scarecrow_ordered = true;
        for (std::size_t index = 1; index < 6; ++index) {
            state.scarecrow[index].type = 500;
        }
        for (auto& particle : state.particles) {
            particle.active = true;
        }
        for (const int slot : row.holes) {
            state.particles[static_cast<std::size_t>(slot - 32)].active = false;
        }
        state.random_state = 12345;
        state.random_draws = 0;
        shoot(round, images, landscape, 5, 12345);
        require(state.selected_slot == 12 && state.score == 130 && state.hits == 1 && state.scarecrow_hits == 7 && state.scarecrow_ordered && state.random_state == 1449997005U
                    && state.random_draws == 4,
            "Particle exhaustion cannot alter source double-hat score, accuracy, order or four velocity draws");
        for (const auto& expected : hat_falls) {
            const auto& piece = state.scarecrow[expected.index];
            require(piece.type == expected.type && std::bit_cast<std::uint32_t>(piece.velocity_x) == expected.velocity_x
                        && std::bit_cast<std::uint32_t>(piece.velocity_y) == expected.velocity_y,
                "Particle capacity vectors must preserve exact selected/adjacent fall velocities");
        }
        require(std::ranges::count(state.particles, 30, &game::Particle::points) == static_cast<int>(row.holes.size()),
            "Only available particle holes receive the selected-head 30 popup");
        for (const auto slot : row.holes) {
            const auto& popup = state.particles[static_cast<std::size_t>(slot - 32)];
            require(popup.active && popup.points == 30 && popup.world_x == 1954 && popup.world_y == 216 && popup.depth == 3,
                "Duplicate popup must occupy the original first-free temporary particle slot");
        }
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected runtime assets directory");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        ordered(images, landscape);
        wrong_order(images, landscape);
        capacity_and_selection(images, landscape);
        source_capacities(images, landscape);
        std::cout << "pass: scarecrow ordered/wrong-order routes, source composition, points, effects, pool, pole and reset\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
