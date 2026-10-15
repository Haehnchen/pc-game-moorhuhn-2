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
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace moorhuhn2;

namespace {
using moorhuhn2::game::test::fixture;
using moorhuhn2::game::test::require;

std::vector<contracts::SpriteDraw> compose(const game::Round& round, const world::Presentation& landscape) {
    return render::round_scene(round, landscape);
}

void shoot(game::Round& round, const render::ImageBank& images, const world::Presentation& landscape, contracts::ScreenPoint aim = {507, 406}) {
    const contracts::ScreenPoint pointer{aim.x - 16, aim.y - 16};
    fixture(round).recoil = 0;
    game::test::advance(round, round.state().previous_sample, {.pointer = pointer, .fire_down = true});
    auto draws = compose(round, landscape);
    render::Framebuffer frame;
    frame.draw(images, draws);
    round.resolve_actions([&](const auto& sprite, auto point) {
        return frame.matches_sprite_pixel(images, sprite, point);
    });
}

void tick(game::Round& round, float delta = 0) {
    game::test::advance(round, round.state().previous_sample + delta, {});
    round.resolve_actions({});
}

void real_hit_and_time(const render::ImageBank& images, const world::Presentation& landscape) {
    struct Case {
        float remaining;
        int points;
        float after;
    };

    constexpr std::array cases{Case{90, 10, 95}, Case{85, 10, 90}, Case{84, 10, 84}, Case{65, 10, 65}, Case{6, 10, 6}, Case{5, 50, 5}, Case{2, 50, 2}};
    for (const bool special : {false, true}) {
        for (const auto& expected : cases) {
            game::Round round(1, 0);
            auto& state = fixture(round);
            state.camera = 0;
            state.remaining = expected.remaining;
            state.special_ammo = special;
            const auto random_before = state.random_draws;
            const auto leaf_draws = game::test::leaf_draw_count(state, 1);
            shoot(round, images, landscape);
            require(state.selected_slot == 32 && state.rose.type == 69 && state.rose.points == expected.points && state.score == expected.points && state.hits == 1
                        && state.misses == 0 && state.ammo_index == 9,
                "The actual rose blossom must win full-word selection, score once and enter reaction69 for either ammo kind");
            require(state.remaining == expected.after && state.random_draws == random_before + 1 + leaf_draws, "Rose impact consumes no RNG after the frog and persistent leaves");
            require(state.minutes * 60 + state.seconds == expected.remaining, "The displayed clock remains the pre-impact sample until the next update");
            require(state.particles[0].active && state.particles[0].type == 2 && state.particles[0].depth == 0 && state.particles[0].world_x == 486
                        && state.particles[0].world_y == 409 && state.particles[0].points == expected.points,
                "Rose popup must use the original x486,y409,depth0 for both point amounts");
            require(std::ranges::all_of(round.events(),
                        [](const auto& event) {
                            return event.kind == game::RoundEventKind::shot || event.kind == game::RoundEventKind::countdown;
                        }),
                "Rose has no extra impact sound or debris event");
            shoot(round, images, landscape);
            require(state.selected_slot == -1 && state.score == expected.points && state.hits == 1 && state.misses == 1 && state.remaining == expected.after,
                "The still-visible reaction rejects another hit and time grant");
            tick(round, .5F);
            require(state.rose.type == 69 && state.rose.world_y == 433.5F && state.rose.frame == 0, "Reaction69 falls 75 world pixels per second without frame animation");
            for (int i = 0; i < 8; ++i) {
                tick(round, .1F);
            }
            require(
                state.rose.type == -1 && state.chickens[32].type == -1 && state.multi_hit.write_index == 0, "Completed rose releases slot32 and does not register a multiple-hit");
            require(std::ranges::none_of(compose(round, landscape),
                        [](const auto& draw) {
                            return draw.image == "gfx/rose";
                        }),
                "The normal presentation must not restore a removed static rose");
            shoot(round, images, landscape);
            require(state.selected_slot == -1 && state.hits == 1 && state.score == expected.points, "The removed underlay cannot award the rose again");
        }
    }
}

void capacity_empty_and_reuse(const render::ImageBank& images, const world::Presentation& landscape) {
    for (const bool full : {false, true}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = 0;
        state.remaining = 87;
        for (auto& particle : state.particles) {
            particle.active = true;
            particle.points = 777;
        }
        if (!full) {
            state.particles.back().active = false;
        }
        shoot(round, images, landscape);
        require(state.score == 10 && state.remaining == 92 && state.rose.type == 69 && state.hits == 1, "A full popup pool cannot suppress rose points, time or reaction");
        require(std::ranges::count_if(state.particles,
                    [](const auto& p) {
                        return p.active && p.points == 10;
                    })
                    == (full ? 0 : 1),
            "Rose uses the first available shared popup slot without replacing active particles");
    }
    game::Round empty(1, 0);
    auto& state = fixture(empty);
    state.camera = 0;
    state.ammo_index = 16;
    shoot(empty, images, landscape);
    require(state.rose.type == 29 && state.score == 0 && state.hits == 0 && state.remaining == 90, "An empty input must leave the live rose and time untouched");
    game::test::advance(empty, 0, {.reload_down = true});
    empty.resolve_actions({});
    require(state.ammo_index == 8 && state.rose.type == 29, "Normal reload keeps the live rose");
    shoot(empty, images, landscape);
    for (int i = 0; i < 12; ++i) {
        tick(empty, .1F);
    }
    require(state.rose.type == -1, "Rose must finish before its slot is reused");
    state.balloons[32] = world::make_balloon(0, {0, 1, 2, 3, 4});
    state.chickens[32].type = -3;
    require(std::ranges::any_of(empty.target_draws(3),
                [](const auto& draw) {
                    return draw.scene_slot == 32 && draw.sprite.image == "gfx/ballon";
                }),
        "The freed rose slot participates in the existing scene allocator");
    const game::Round fresh(1, 0);
    require(fresh.state().rose == world::Rose{} && fresh.state().chickens[32].type == -2, "A new round restores the original rose and reserved slot");
}

void strict_edges() {
    for (const auto aim : {contracts::ScreenPoint{497, 406}, contracts::ScreenPoint{515, 406}, contracts::ScreenPoint{507, 396}, contracts::ScreenPoint{507, 422}}) {
        game::Round round(1, 0);
        fixture(round).camera = 0;
        bool queried = false;
        game::test::advance(round, 0, {.pointer = {aim.x - 16, aim.y - 16}, .fire_down = true});
        round.resolve_actions([&](const auto& sprite, auto) {
            queried |= sprite.image == "gfx/rose";
            return false;
        });
        require(!queried, "Rose collision uses strict 18x26 bounds before scratch comparison");
    }
}

struct PopupCase {
    std::array<std::uint32_t, 5> before, after;
    std::uint32_t delta, camera;
    int recoil;
    bool before_active, active;
};

constexpr std::array popup_cases{
    PopupCase{{0x43f30000U, 0x43cc8000U, 0x00000000U, 0x00000000U, 0x00000000U}, {0x43f30000U, 0x43c64000U, 0x43f30000U, 0x43c64000U, 0x3e800000U}, 0x3e800000U, 0x00000000U, 0,
        true, true},
    PopupCase{{0x43f30000U, 0x43cc8000U, 0x00000000U, 0x00000000U, 0x00000000U}, {0x43f30000U, 0x43c96000U, 0xc2b20000U, 0x43ce2ccdU, 0x3e000000U}, 0x3e000000U, 0x43a00000U, 8,
        true, true},
    PopupCase{{0x43f30000U, 0x43cc8000U, 0x00000000U, 0x00000000U, 0x00000000U}, {0x43f30000U, 0x439a8000U, 0x42140000U, 0x439a8000U, 0x40000000U}, 0x40000000U, 0x437a0000U, 0,
        true, true},
    PopupCase{{0x43f30000U, 0x43c00000U, 0x00000000U, 0x00000000U, 0x40000000U}, {0x43f30000U, 0x43c00000U, 0x43f30000U, 0x43c00000U, 0x40000000U}, 0x00000000U, 0x00000000U, 0,
        true, true},
    PopupCase{{0x43f30000U, 0x43c00000U, 0x00000000U, 0x00000000U, 0x40000000U}, {0x43f30000U, 0x43c00000U, 0x43f30000U, 0x43c00000U, 0x40000000U}, 0x33000000U, 0x00000000U, 0,
        true, false},
};

void source_popup_vectors() {
    for (const auto& row : popup_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = std::bit_cast<float>(row.camera);
        state.recoil = row.recoil;
        auto& popup = state.particles[0];
        popup.active = row.before_active;
        popup.depth = 0;
        popup.points = 10;
        popup.world_x = std::bit_cast<float>(row.before[0]);
        popup.world_y = std::bit_cast<float>(row.before[1]);
        popup.screen_x = std::bit_cast<float>(row.before[2]);
        popup.screen_y = std::bit_cast<float>(row.before[3]);
        popup.age = std::bit_cast<float>(row.before[4]);
        game::test::advance(round, std::bit_cast<float>(row.delta), {.pointer = {320, 240}});
        round.resolve_actions({});
        const std::array actual{std::bit_cast<std::uint32_t>(popup.world_x), std::bit_cast<std::uint32_t>(popup.world_y), std::bit_cast<std::uint32_t>(popup.screen_x),
            std::bit_cast<std::uint32_t>(popup.screen_y), std::bit_cast<std::uint32_t>(popup.age)};
        require(actual == row.after && popup.active == row.active, "Foreground popup must match independent source position, recoil and retained lifetime vectors");
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected adopted runtime assets directory");
        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation landscape(frames);
        real_hit_and_time(images, landscape);
        capacity_empty_and_reuse(images, landscape);
        strict_edges();
        source_popup_vectors();
        std::cout << "Rose real pixels, time variants, reaction/removal, pools, empty/reload, slot reuse and reset passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
