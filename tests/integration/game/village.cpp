#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "assets/asset_store.hpp"
#include "game/round.hpp"
#include "render/framebuffer.hpp"
#include "support.hpp"
#include "support/require.hpp"
#include "ui/screens.hpp"
#include "world/presentation.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;
using Target = world::VillageTarget;
using Event = game::RoundEventKind;

namespace {
using moorhuhn2::game::test::fixture;

using moorhuhn2::test::require;

// Unit prerequisites isolate pool pressure and absent scene groups. The native
// scenario reaches the ordinary sequence entirely through mouse/keyboard input.

void step(game::Round& round, float time, game::RoundInput input = {}) {
    game::test::advance(round, time, input);
    round.resolve_actions({});
}

game::Round visible_round() {
    game::Round round(1, 0);
    fixture(round).camera = 320;
    // Isolate the village RNG/pool contract from live frog croaks.
    fixture(round).frog.type = -1;
    return round;
}

contracts::ScreenPoint opaque_aim(const render::ImageBank& images, const contracts::SpriteDraw& draw) {
    for (int y = 1; y < draw.source.height; ++y) {
        for (int x = 1; x < draw.source.width; ++x) {
            const auto word = images.rgb565_at(draw.image, draw.source.x + x, draw.source.y + y);
            if (word != 0 && word != 0xf81f) {
                return {draw.destination.x + x, draw.destination.y + y};
            }
        }
    }
    throw std::runtime_error("Village sprite has no interior opaque aim");
}

game::test::SourceRandom shoot(game::Round& round, const render::ImageBank& images, Target target, bool occluded = false, bool reload = false) {
    step(round, round.state().previous_sample);
    const auto draw = world::village_draw(target, round.state().camera, 0);
    const auto aim = opaque_aim(images, draw);
    game::RoundInput input;
    input.pointer = {aim.x - 16, aim.y - 16};
    input.fire_down = true;
    input.reload_down = reload;
    game::test::advance(round, round.state().previous_sample, input);
    const auto action_random = game::test::snapshot(round.state());
    render::Framebuffer frame;
    if (!occluded) {
        frame.draw(images, std::span(&draw, 1));
    }
    round.resolve_actions([&](const auto& candidate, auto point) {
        return frame.matches_sprite_pixel(images, candidate, point);
    });
    return action_random;
}

int balloons(const game::Round& round) {
    return static_cast<int>(std::ranges::count_if(round.state().balloons, [](const auto& balloon) {
        return balloon.type == 21;
    }));
}

void ordinary_sequence(const render::ImageBank& images) {
    auto round = visible_round();
    shoot(round, images, Target::house);
    require(round.state().score == 25 && round.state().village.house_hit && balloons(round) == 0, "First house hit awards 25 without needing either church or spawning a balloon");
    shoot(round, images, Target::first_church);
    require(round.state().score == 0 && round.state().remaining == 100 && round.state().seconds == 30 && round.state().village.first_church_hit,
        "First church subtracts 25 and adds 10 seconds, leaving the already-composed HUD time unchanged");
    require(round.state().particles[1].points == -25 && round.state().particles[1].world_x == 720 && round.state().particles[1].world_y == 177,
        "Church penalty popup must retain its signed value and source origin");
    shoot(round, images, Target::first_church);
    require(round.state().score == 0 && round.state().remaining == 100 && round.state().seconds == 40 && round.state().hits == 3,
        "Repeated first-church hits retain accuracy and sound without repeating score/time");
    int total = 0;
    for (int index = 0; index < 5; ++index) {
        const int award = 20 - index * 5;
        auto random = shoot(round, images, Target::house);
        random.discard(5);
        total += award;
        require(round.state().score == total && round.state().village.house_points == award && balloons(round) == index + 1,
            "House chain must award 20/15/10/5/0 and spawn a balloon even for zero");
        require(round.state().random_draws == random.draws && round.state().random_state == random.state, "House balloon allocations consume exactly five draws each");
        require(!round.state().special_ammo && !round.state().stone_bonus_granted, "House balloons must not grant stone-chain ammunition");
        const auto events = round.events();
        require(events.size() == 4 && events[0].kind == Event::shot && events[1].kind == Event::house_hit && events[2].kind == Event::church_bell
                    && events[3].kind == Event::church_bell_second,
            "House chain must emit shotgun, house, first bell, second bell in original order");
    }
    require(round.state().ammo_index == 16 && round.state().hits == 8 && round.state().score == 50, "Eight normal shots complete the five-balloon house sequence");
    shoot(round, images, Target::house, false, true);
    require(round.events().size() == 2 && round.events()[0].kind == Event::empty && round.events()[1].kind == Event::reload && round.state().ammo_index == 8
                && round.state().hits == 8 && round.state().score == 50,
        "Empty fire then reload cannot impact a building");
    shoot(round, images, Target::house);
    require(round.state().score == 50 && balloons(round) == 5 && round.events().size() == 2 && round.events()[1].kind == Event::house_hit,
        "Exhausted house stays hittable with house sound but no additional award or balloon");
    shoot(round, images, Target::second_church);
    shoot(round, images, Target::second_church);
    require(round.state().score == 65 && round.state().hits == 11 && round.state().village.second_church_hit, "Second church awards 15 only once while repeats count as hits");

    auto penalty = visible_round();
    shoot(penalty, images, Target::first_church);
    require(penalty.state().score == -25, "First-church impact temporarily retains negative score before the next update");
    step(penalty, 0);
    require(penalty.state().score == 0 && penalty.state().particles[0].points == -25, "Pre-input bonus update clamps score without changing the penalty popup");
    step(penalty, .5F);
    require(penalty.state().particles[0].world_y == 170.75F && penalty.state().particles[0].screen_x == 336 && penalty.state().particles[0].screen_y == 170.75F,
        "Depth4 popup rises 12.5 px/s and uses camera 1.2 after recoil settles");
    step(penalty, 2);
    require(penalty.state().particles[0].active, "Village popup survives exact age 2");
    penalty.reset_sample(0);
    step(penalty, 0x1p-23F);
    require(!penalty.state().particles[0].active && penalty.state().particles[0].age == 2, "Village popup expiry compares the extended sum before binary32 rounding");
}

void prerequisites_and_pools(const render::ImageBank& images) {
    for (int missing = 0; missing < 4; ++missing) {
        auto round = visible_round();
        auto& state = fixture(round);
        state.village = {.first_church_hit = missing != 0, .second_church_hit = missing == 1, .house_hit = true};
        state.frog_hits = missing == 2 ? 1 : missing == 3 ? 2 : 0;
        const auto random = shoot(round, images, Target::house);
        require(state.hits == 1 && state.score == 0 && state.village.house_points == 25 && balloons(round) == 0 && state.random_draws == random.draws
                    && state.random_state == random.state,
            "House chain must reject missing first church, hit second church and either positive frog counter");
    }
    auto full = visible_round();
    auto& state = fixture(full);
    state.village = {.first_church_hit = true, .house_hit = true};
    for (auto& target : state.chickens) {
        target.type = -2;
    }
    for (auto& particle : state.particles) {
        particle.active = true;
    }
    const auto random = shoot(full, images, Target::house);
    require(
        state.score == 20 && state.village.house_points == 20 && state.hits == 1 && state.random_draws == random.draws && state.random_state == random.state && balloons(full) == 0,
        "Full scene/particle pools still consume the house award without allocating or drawing RNG");

    auto fresh = visible_round();
    require(fresh.state().village == world::Village{} && fresh.state().frog_hits == 0 && fresh.state().score == 0 && fresh.state().remaining == 90,
        "New round resets all building flags, house points and shared prerequisites");
    shoot(fresh, images, Target::house);
    require(fresh.state().score == 25, "House first-hit award is restored in a new round");
}

void pixels_and_selection(const render::ImageBank& images, const world::Presentation& landscape) {
    for (const auto target : {Target::first_church, Target::second_church, Target::house}) {
        auto round = visible_round();
        const auto draw = world::village_draw(target, 320, 0);
        const auto aim = opaque_aim(images, draw);
        render::Framebuffer frame;
        frame.draw(images, landscape.commands(320));
        require(frame.matches_sprite_pixel(images, draw, aim), "A source-visible building pixel must match the normal landscape composition");
        game::RoundInput input;
        input.pointer = {aim.x - 16, aim.y - 16};
        input.fire_down = true;
        game::test::advance(round, 0, input);
        round.resolve_actions([&](const auto& candidate, auto point) {
            return frame.matches_sprite_pixel(images, candidate, point);
        });
        require(round.state().hits == 1 && round.events().back().slot == static_cast<int>(target), "Normal composed source words must select startup village slots 0..2");
        auto occluded = visible_round();
        shoot(occluded, images, target, true);
        require(occluded.state().hits == 0 && occluded.state().misses == 1 && occluded.state().village == world::Village{},
            "Different composed words must reject an occluded building");
        for (const auto edge : {contracts::ScreenPoint{0, 10}, {draw.source.width, 10}, {5, 0}, {5, draw.source.height}}) {
            auto boundary = visible_round();
            input.pointer = {draw.destination.x + edge.x - 16, draw.destination.y + edge.y - 16};
            game::test::advance(boundary, 0, input);
            boundary.resolve_actions([&](const auto& candidate, auto) {
                return candidate.image == draw.image && candidate.destination == draw.destination;
            });
            require(boundary.state().hits == 0 && boundary.state().misses == 1, "Village rectangle excludes all four boundaries before pixel matching");
        }
    }
    // A later chicken can share the exact source word; original slot 0 still wins.
    auto overlap = visible_round();
    const auto draw = world::village_draw(Target::first_church, 320, 0);
    const auto aim = opaque_aim(images, draw);
    game::RoundInput input;
    input.pointer = {aim.x - 16, aim.y - 16};
    input.fire_down = true;
    game::test::advance(overlap, 0, input);
    auto& chicken = fixture(overlap).chickens[34];
    chicken.type = 0;
    chicken.depth = 4;
    chicken.screen_x = static_cast<float>(draw.destination.x);
    chicken.screen_y = static_cast<float>(draw.destination.y);
    overlap.resolve_actions([](const auto&, auto) {
        return true;
    });
    require(overlap.state().village.first_church_hit && chicken.type == 0 && overlap.state().score == -25, "Village startup slot wins before overlapping dynamic targets");
}

struct PopupCase {
    float camera, delta;
    int recoil;
    std::array<float, 3> before;
    std::array<std::uint32_t, 4> after;
    bool active;
};

constexpr std::array popup_cases{
    PopupCase{640.0F, 0.0F, 8, {720.0F, 177.0F, 0.0F}, {0x43310000U, 0xc2400000U, 0x4335cccdU, 0x00000000U}, true},
    PopupCase{624.1666259765625F, 0.5F, 8, {536.0F, 231.0F, 0.0F}, {0x4360c000U, 0xc3540000U, 0x43658ccdU, 0x3f000000U}, true},
    PopupCase{0.0F, 2.0F, 0, {546.0F, 231.0F, 0.0F}, {0x434e0000U, 0x44088000U, 0x434e0000U, 0x40000000U}, true},
    PopupCase{0.0F, 0.0F, 8, {546.0F, 206.0F, 2.0F}, {0x434e0000U, 0x44088000U, 0x4352cccdU, 0x40000000U}, true},
    PopupCase{640.0F, 2.9802322387695312e-08F, 0, {546.0F, 206.0F, 2.0F}, {0x434e0000U, 0xc35e0000U, 0x434e0000U, 0x40000000U}, false},
    PopupCase{640.0F, 1.1920928955078125e-07F, 0, {536.0F, 231.0F, 1.9999998807907104F}, {0x43670000U, 0xc3680000U, 0x43670000U, 0x40000000U}, true},
};

struct TextCase {
    int points;
    std::string_view text;
    std::vector<int> glyphs;
};

const std::array text_cases{
    TextCase{-25, "-25", {97, 100, 103}},
    TextCase{15, "15", {99, 103}},
    TextCase{25, "25", {100, 103}},
    TextCase{20, "20", {100, 98}},
    TextCase{10, "10", {99, 98}},
    TextCase{5, "5", {103}},
    TextCase{0, "0", {98}},
};

struct ComparisonCase {
    Target target;
    contracts::ScreenPoint local;
    std::uint16_t scene_word;
    bool selected;
};

constexpr std::array comparison_cases{
    ComparisonCase{static_cast<Target>(0), {11, 20}, 31626, true},
    ComparisonCase{static_cast<Target>(0), {11, 20}, 31627, false},
    ComparisonCase{static_cast<Target>(0), {11, 20}, 31626, true},
    ComparisonCase{static_cast<Target>(0), {1, 1}, 0, true},
    ComparisonCase{static_cast<Target>(0), {1, 1}, 1, false},
    ComparisonCase{static_cast<Target>(0), {0, 20}, 0, false},
    ComparisonCase{static_cast<Target>(0), {11, 0}, 0, false},
    ComparisonCase{static_cast<Target>(0), {20, 20}, 0, false},
    ComparisonCase{static_cast<Target>(0), {11, 40}, 0, false},
    ComparisonCase{static_cast<Target>(1), {10, 23}, 33607, true},
    ComparisonCase{static_cast<Target>(1), {10, 23}, 33606, false},
    ComparisonCase{static_cast<Target>(1), {10, 23}, 33607, true},
    ComparisonCase{static_cast<Target>(1), {1, 1}, 0, true},
    ComparisonCase{static_cast<Target>(1), {1, 1}, 1, false},
    ComparisonCase{static_cast<Target>(1), {0, 23}, 0, false},
    ComparisonCase{static_cast<Target>(1), {10, 0}, 0, false},
    ComparisonCase{static_cast<Target>(1), {20, 23}, 0, false},
    ComparisonCase{static_cast<Target>(1), {10, 40}, 0, false},
    ComparisonCase{static_cast<Target>(2), {6, 10}, 44141, true},
    ComparisonCase{static_cast<Target>(2), {6, 10}, 44140, false},
    ComparisonCase{static_cast<Target>(2), {6, 10}, 44141, true},
    ComparisonCase{static_cast<Target>(2), {1, 1}, 0, true},
    ComparisonCase{static_cast<Target>(2), {1, 1}, 1, false},
    ComparisonCase{static_cast<Target>(2), {0, 10}, 0, false},
    ComparisonCase{static_cast<Target>(2), {6, 0}, 0, false},
    ComparisonCase{static_cast<Target>(2), {13, 10}, 0, false},
    ComparisonCase{static_cast<Target>(2), {6, 20}, 0, false},
};

struct PixelExpectation {
    contracts::ScreenPoint local;
    std::uint16_t scene_word;
    bool matches;
};

struct CompositionCase {
    float camera;
    int recoil;
    Target target;
    contracts::ScreenPoint aim;
    std::optional<PixelExpectation> lower;
};

const std::array composition_cases{
    CompositionCase{0.0F, 0, static_cast<Target>(2), {556, 231}, PixelExpectation{{5, 16}, 46188, false}},
    CompositionCase{100.0F, 0, static_cast<Target>(0), {631, 177}, std::nullopt},
    CompositionCase{100.0F, 0, static_cast<Target>(1), {533, 185}, std::nullopt},
    CompositionCase{100.0F, 0, static_cast<Target>(2), {436, 231}, PixelExpectation{{5, 16}, 46188, false}},
    CompositionCase{200.0F, 0, static_cast<Target>(0), {511, 177}, std::nullopt},
    CompositionCase{200.0F, 0, static_cast<Target>(1), {413, 185}, std::nullopt},
    CompositionCase{200.0F, 0, static_cast<Target>(2), {316, 231}, PixelExpectation{{5, 16}, 41932, false}},
    CompositionCase{300.0F, 0, static_cast<Target>(0), {391, 177}, std::nullopt},
    CompositionCase{300.0F, 0, static_cast<Target>(1), {293, 185}, std::nullopt},
    CompositionCase{300.0F, 0, static_cast<Target>(2), {196, 231}, PixelExpectation{{5, 16}, 41932, false}},
    CompositionCase{400.0F, 0, static_cast<Target>(0), {271, 177}, std::nullopt},
    CompositionCase{400.0F, 0, static_cast<Target>(1), {173, 185}, std::nullopt},
    CompositionCase{400.0F, 0, static_cast<Target>(2), {76, 231}, PixelExpectation{{5, 16}, 41932, false}},
    CompositionCase{200.0F, 3, static_cast<Target>(0), {511, 178}, std::nullopt},
    CompositionCase{200.0F, 3, static_cast<Target>(1), {413, 186}, std::nullopt},
    CompositionCase{200.0F, 3, static_cast<Target>(2), {316, 232}, PixelExpectation{{5, 16}, 44042, false}},
    CompositionCase{200.0F, 5, static_cast<Target>(0), {511, 180}, std::nullopt},
    CompositionCase{200.0F, 5, static_cast<Target>(1), {413, 188}, std::nullopt},
    CompositionCase{200.0F, 5, static_cast<Target>(2), {316, 234}, PixelExpectation{{5, 16}, 44042, false}},
    CompositionCase{200.0F, 8, static_cast<Target>(0), {511, 181}, std::nullopt},
    CompositionCase{200.0F, 8, static_cast<Target>(1), {413, 189}, std::nullopt},
    CompositionCase{200.0F, 8, static_cast<Target>(2), {316, 235}, PixelExpectation{{5, 16}, 44141, true}},
};

void literal_vectors(const render::ImageBank& images, const world::Presentation& landscape) {
    for (const auto& row : popup_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = row.camera;
        state.recoil = row.recoil;
        auto& particle = state.particles[0];
        particle.active = true;
        particle.depth = 4;
        particle.world_x = row.before[0];
        particle.world_y = row.before[1];
        particle.age = row.before[2];
        step(round, row.delta);
        const std::array actual{std::bit_cast<std::uint32_t>(particle.world_y), std::bit_cast<std::uint32_t>(particle.screen_x), std::bit_cast<std::uint32_t>(particle.screen_y),
            std::bit_cast<std::uint32_t>(particle.age)};
        require(particle.active == row.active && actual == row.after, "Original village popup lifetime and float-store vectors differ");
    }
    for (const auto& row : text_cases) {
        std::vector<contracts::SpriteDraw> draws;
        const auto text = std::to_string(row.points);
        require(text == row.text, "Signed village popup text differs");
        ui::text(draws, text, 0, 0);
        require(draws.size() == row.glyphs.size(), "Village popup glyph count differs");
        for (std::size_t index = 0; index < draws.size(); ++index) {
            require(draws[index].source.y == row.glyphs[index] * 50 && draws[index].destination.x == static_cast<int>(index) * 20,
                "Village penalty/zero popup must use the original minus/digit glyphs and spacing");
        }
    }
    for (const auto& row : comparison_cases) {
        auto round = visible_round();
        const auto draw = world::village_draw(row.target, 320, 0);
        game::RoundInput input;
        input.fire_down = true;
        input.pointer = {draw.destination.x + row.local.x - 16, draw.destination.y + row.local.y - 16};
        game::test::advance(round, 0, input);
        render::Framebuffer frame;
        frame.clear(row.scene_word);
        round.resolve_actions([&](const auto& candidate, auto point) {
            return frame.matches_sprite_pixel(images, candidate, point);
        });
        require(round.state().hits == (row.selected ? 1 : 0), "Original strict bounds, occlusion and keyed-zero comparison differ");
    }
    for (const auto& row : composition_cases) {
        const auto draw = world::village_draw(row.target, row.camera, row.recoil);
        render::Framebuffer frame;
        frame.draw(images, landscape.commands(row.camera, {}, {}, row.recoil));
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                require(frame.matches_sprite_pixel(images, draw, {row.aim.x + dx, row.aim.y + dy}), "Original composed village aim patch differs");
            }
        }
        if (row.lower) {
            const auto& lower = *row.lower;
            const contracts::ScreenPoint point{draw.destination.x + lower.local.x, draw.destination.y + lower.local.y};
            require(frame.rgb565_at(point.x, point.y) == lower.scene_word && frame.matches_sprite_pixel(images, draw, point) == lower.matches,
                "Foreground landscape must occlude/reveal the lower house pixel at the original recoil boundary");
        }
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected adopted assets directory");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog catalog(assets);
        const world::Presentation landscape(catalog);
        ordinary_sequence(images);
        prerequisites_and_pools(images);
        pixels_and_selection(images, landscape);
        literal_vectors(images, landscape);
        std::cout << "Village score/time, repeated house balloons, source pixels, ordering, empty/reload, pool pressure and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
