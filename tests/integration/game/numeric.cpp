#include "support/numeric.hpp"

#include "game/round.hpp"
#include "world/airplane.hpp"
#include "world/notice.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace moorhuhn2;

void check(bool condition, std::string_view id, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(id) + ": " + std::string(message));
    }
}

using moorhuhn2::test::binary32;

struct TimerBoundary {
    std::string id;
    std::uint32_t delta_bits;
    bool active;
};

const std::array<TimerBoundary, 5> timer_boundaries = {{TimerBoundary{"zero", 0x00000000U, true}, TimerBoundary{"below-halfway", 0x25000000U, true},
    TimerBoundary{"halfway-even", 0x25800000U, true}, TimerBoundary{"above-halfway", 0x25800001U, false}, TimerBoundary{"binary32-store-halfway", 0x34000000U, false}}};

std::uint32_t word(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

world::Notice notice_case() {
    return {.active = true, .visible = false, .age = 3, .y = 160, .type = 3};
}

world::AircraftSmoke smoke_case() {
    return {.type = 5, .world_x = 0, .world_y = 0, .screen_x = 0, .screen_y = 0, .age = 2, .frame = 5};
}

void check_notice(const world::Notice& notice, const TimerBoundary& expected, std::string_view id, bool expiry_update) {
    check(word(notice.age) == 0x40400000U, id, "Notice stores the exact binary32 age word");
    const bool active = expected.active;
    check(notice.active == active, id, "Notice lifetime uses the retained 53-bit age");
    check(notice.y == 160 && notice.type == 3, id, "Tiny lifetime deltas preserve the stored notice pose and label");
    const bool visible = expiry_update || active;
    const auto text = world::notice_text(notice);
    check(notice.visible == visible && text.has_value() == visible, id, "An expiring notice draws this update, then its inactive successor clears visibility");
    if (text) {
        check(text->text == "TRIPLE HIT +5" && text->x == 200 && text->y == 160, id, "The source notice label and draw origin remain exact");
    }
}

void check_smoke(const world::AircraftSmoke& smoke, const TimerBoundary& expected, std::string_view id, contracts::ScreenPoint draw_position = {0, 0}) {
    check(word(smoke.age) == 0x40000000U, id, "Type 5 smoke stores the exact binary32 age word");
    const bool active = expected.active;
    check(smoke.type == (active ? 5 : -1) && smoke.frame == 5, id, "Smoke expiry tests the retained 53-bit age and preserves its source frame");
    const auto before = smoke;
    const auto draw = world::aircraft_smoke_draw(smoke);
    check(draw.has_value() == active && smoke == before, id, "Expired smoke disappears from pure draw submission in the same update");
    if (draw) {
        check(draw->image == "gfx/particle2" && draw->source.x == 0 && draw->source.y == 40 && draw->source.width == 8 && draw->source.height == 8
                  && draw->destination == draw_position,
            id, "The retained smoke uses its original particle cell and truncated position");
    }
}

void related_world_boundaries() {
    for (const auto& row : timer_boundaries) {
        const std::string id = "notice/" + row.id;
        auto notice = notice_case();
        world::update_notice(notice, binary32(row.delta_bits), 0);
        check_notice(notice, row, id, true);
        world::update_notice(notice, 0, 0);
        check_notice(notice, row, id, false);
    }
    for (const auto& row : timer_boundaries) {
        const std::string id = "smoke-five/" + row.id;
        auto smoke = smoke_case();
        world::update_aircraft_smoke(smoke, binary32(row.delta_bits), 0, 0);
        check_smoke(smoke, row, id);
        world::update_aircraft_smoke(smoke, 0, 0, 0);
        check_smoke(smoke, row, id);
    }
}

void arrange_timers(game::Round& round) {
    // Arrange only source boundary state; updates, inputs, RNG and action
    // resolution continue through the ordinary public Round route.
    auto& state = const_cast<game::RoundState&>(round.state());
    state.camera = 0;
    state.particles[0] = {.active = true, .type = 0, .depth = 2, .world_x = 100, .world_y = 200, .age = 2.0F, .points = 2};
    state.particles[1] = {.active = true, .type = 2, .depth = 2, .world_x = 200, .world_y = 300, .age = 2.0F, .points = 2};
    state.particles[2] = {.active = true, .type = 5, .depth = 3};
    state.aircraft_smoke[2] = smoke_case();
    state.notices[0] = notice_case();
}

void check_round_timers(const game::Round& round, const TimerBoundary& expected, std::string_view id, bool expiry_update) {
    for (std::size_t index = 0; index < 2; ++index) {
        const auto& particle = round.state().particles[index];
        check(word(particle.age) == 0x40000000U, id, "Both popup types store the exact binary32 age");
        check(
            particle.active == expected.active && particle.type == (index == 0 ? 0 : 2), id, "Popup types 0/2 compare retained 53-bit age even when the binary32 store remains 2");
    }
    check_notice(round.state().notices[0], expected, id, expiry_update);
    // The next composition retains the preceding shot's recoil 8: its source
    // smoke offset truncates to 4. The first composition starts at recoil 0.
    check_smoke(round.state().aircraft_smoke[2], expected, id, {0, expiry_update ? 0 : 4});
    check(round.state().particles[2].active == expected.active && round.state().particles[2].type == 5, id,
        "Normal Round smoke update releases the shared particle entry with the expired source smoke");
}

void normal_round_boundaries() {
    for (const auto& row : timer_boundaries) {
        const std::string id = "round/" + row.id;
        game::Round first(1, 0), control(1, 0);
        arrange_timers(first);
        const_cast<game::RoundState&>(control.state()).camera = 0;
        const game::RoundInput input{.fire_down = true};
        const contracts::FrameTime time{1, binary32(row.delta_bits), 0};
        first.advance(time, input);
        control.advance(time, input);
        check_round_timers(first, row, id, true);
        check(first.state().ammo_index == 8 && first.state().misses == 0 && std::ranges::count(first.events(), game::RoundEventKind::shot, &game::RoundEvent::kind) == 0, id,
            "Timer expiry precedes fire resolution");
        const auto before_draw = first.state();
        for (int depth = 0; depth <= 4; ++depth) {
            static_cast<void>(first.target_draws(depth));
        }
        static_cast<void>(first.ammo_draws());
        check(first.state() == before_draw, id, "Scene and ammo command submission preserve timer state");
        first.resolve_actions({});
        control.resolve_actions({});
        check(first.state().ammo_index == 9 && first.state().misses == 1 && first.state().hits == 0 && first.state().recoil == 8
                  && std::ranges::count(first.events(), game::RoundEventKind::shot, &game::RoundEvent::kind) == 1 && std::ranges::equal(first.events(), control.events())
                  && first.state().random_state == control.state().random_state && first.state().random_draws == control.state().random_draws,
            id, "Lifetime boundaries preserve ordinary charged-shot events, action order and RNG consumption");
        check(word(first.state().previous_sample) == 0x3f800000U && word(first.state().delta) == row.delta_bits && word(first.state().accumulated_time) == 0, id,
            "Timer boundaries use independent current, delta and total words");

        // A subsequent zero update reloads the stored age. Surviving timers
        // stay alive; inactive notices stop drawing and smoke stays removed.
        first.advance({2, 0, 0}, {});
        control.advance({2, 0, 0}, {});
        check_round_timers(first, row, id, false);
        first.resolve_actions({});
        control.resolve_actions({});
        check(std::ranges::equal(first.events(), control.events()), id, "Zero update preserves resolved event order after survival or expiry");
    }
}
} // namespace

int main() {
    try {
        normal_round_boundaries();
        related_world_boundaries();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Independent retained 53-bit popup/notice/smoke lifetimes, draw eligibility and ordered actions passed\n";
}
