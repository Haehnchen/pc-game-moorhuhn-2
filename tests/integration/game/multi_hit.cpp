#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
using moorhuhn2::game::test::fixture;
using namespace moorhuhn2;

using moorhuhn2::test::require;

void step(game::Round& round, float time, game::RoundInput input = {}) {
    game::test::advance(round, time, input);
    round.resolve_actions({});
}

bool bonus(const game::Round& round) {
    return std::ranges::any_of(round.events(), [](const auto& event) {
        return event.kind == game::RoundEventKind::multi_hit_bonus && event.points == 5;
    });
}

void stone(game::Round& round, int index, float time) {
    auto& target = fixture(round).stones[static_cast<std::size_t>(index)];
    target.world_x = static_cast<float>(1100 + index * 150);
    target.world_y = 340;
    const int x = static_cast<int>(target.world_x) - 1024;
    const int y = 340 + round.state().recoil;
    game::test::advance(round, time, {.pointer = {x + 73 - 16, y + 55 - 16}, .fire_down = true});
    round.resolve_actions([&](const auto& draw, auto) {
        return draw.image == "gfx/stone" && draw.destination.x == x && draw.destination.y == y;
    });
    require(round.state().selected_slot == game::first_stone_scene_slot + index && target.type == 50, "Unit input must select a distinct live stone and retain its slot");
}

void triple(game::Round& round) {
    stone(round, 0, 0);
    require(round.state().multi_hit.write_index == 0, "Current hit must wait until the next pre-input update");
    stone(round, 1, .125F);
    require(round.state().multi_hit.write_index == 1 && round.state().multi_hit.timers[0] == 1.875F, "Previous selected reaction registers then loses the current delta");
    stone(round, 2, .25F);
    require(round.state().score == 6 && round.state().multi_hit.write_index == 2, "Three impact awards precede the deferred triple bonus");
}

void delayed_bonus_and_misses() {
    game::Round round(1, 0);
    triple(round);
    step(round, .375F, {.pointer = {320, 240}, .fire_down = true});
    require(round.state().misses == 1 && round.state().selected_slot == -1 && round.state().multi_hit.write_index == 3,
        "A miss clears selection without cancelling the prior three timers");
    step(round, 1);
    require(round.state().score == 6 && !bonus(round) && round.state().multi_hit.write_index == 3, "A retained corpse must not register again on no-fire updates");
    // Keep the unrelated chicken-spawn accumulator below its strict threshold.
    fixture(round).spawn_accumulator = 0;
    const auto random = round.state().random_draws;
    const auto leaf_draws = game::test::leaf_draw_count(round.state(), 1);
    step(round, 2);
    require(round.state().score == 11 && round.state().multi_hit.write_index == 0 && bonus(round), "Oldest expiry resolves three surviving events into exactly five bonus points");
    require(round.state().random_draws == random + 1 + leaf_draws && round.state().hits == 3 && round.state().misses == 1,
        "Tracker/notice consume no RNG or accuracy; live frog uses one draw");
    const auto& notice = round.state().notices[0];
    require(notice.type == 3 && notice.active && notice.age == 1 && notice.y == 120, "New notice ages and moves in the same composition update");
    step(round, 2.25F);
    const auto label = world::notice_text(round.state().notices[0]);
    require(label && label->text == "TRIPLE HIT +5" && label->x == 200 && label->y == 110, "Production shared notice selects source triple text and motion");
    const auto before_render = round.state();
    static_cast<void>(world::notice_text(round.state().notices[0]));
    static_cast<void>(round.target_draws(2));
    require(before_render == round.state() && !bonus(round) && round.state().score == 11, "Rendering and later frames cannot repeat the award");

    game::Round empty(1, 0);
    triple(empty);
    fixture(empty).ammo_index = 16;
    step(empty, .375F, {.fire_down = true});
    require(empty.state().multi_hit.write_index == 3 && empty.state().selected_slot == -1 && empty.state().misses == 0, "Empty fire must preserve timers without a new hit");
    step(empty, .5F, {.reload_down = true});
    require(empty.state().ammo_index == 8 && empty.state().multi_hit.write_index == 3, "Reload must preserve the current group");
    step(empty, 2);
    require(empty.state().score == 11 && bonus(empty), "Empty fire and reload cannot cancel a pending triple");

    game::Round expired(1, 0);
    triple(expired);
    step(expired, .375F);
    step(expired, 3);
    require(
        expired.state().score == 6 && expired.state().multi_hit.write_index == 0 && !bonus(expired), "A large update expiring every tail timer must not synthesize a triple award");
}

void current_slot_type() {
    game::Round returned(1, 0);
    auto& state = fixture(returned);
    state.selected_slot = game::spider_scene_slot;
    state.spider.type = 60;
    state.spider.mode = 0;
    state.spider.countdown = 0;
    state.spider.frame = 24;
    state.spider.animation_time = 3;
    step(returned, .125F);
    require(state.spider.type == 20 && state.multi_hit.write_index == 0, "A reaction returning before registration must not qualify from its old impact type");

    game::Round reused(1, 0);
    auto& replacement = fixture(reused);
    replacement.selected_slot = game::spider_scene_slot;
    replacement.spider.type = 60;
    replacement.spider.countdown = 1;
    replacement.spider.frame = 7;
    replacement.spider.animation_time = 3;
    replacement.spider_hits = 3;
    step(reused, .125F);
    require(replacement.spider.type == -1 && replacement.balloons[5].type == 21 && replacement.multi_hit.write_index == 0,
        "Same-update slot reuse by a live balloon must be read after special allocations");

    game::Round boat(1, 0);
    fixture(boat).selected_slot = 3;
    fixture(boat).boat.sinking = true;
    step(boat, .125F);
    require(boat.state().multi_hit.write_index == 0, "Sinking boat is original type52, not qualifying special-chicken type44");

    for (const int slot : {5, 19, 24, 34, 35}) {
        game::Round qualifying(1, 0);
        auto& q = fixture(qualifying);
        q.selected_slot = slot;
        if (slot == 5) {
            q.spider.type = 60;
            q.spider.frame = 0;
            q.spider.countdown = 0;
        } else if (slot == 19) {
            static_cast<void>(world::hit_frog(q.frog));
        } else if (slot == 24) {
            q.stones[0].type = 50;
        } else if (slot == 34) {
            q.chickens[34] = {.type = 42, .depth = 2};
        } else {
            q.balloons[35] = {.type = 61, .world_x = 1200, .world_y = 100, .frame = 5};
            q.chickens[35].type = -3;
        }
        step(qualifying, .125F);
        require(q.multi_hit.write_index == 1 && q.multi_hit.timers[0] == 1.875F, "Every integrated qualifying reaction maps to its current original pool type");
    }
}

void pool_order_and_clamp() {
    const auto prime = [](game::RoundState& state) {
        state.multi_hit.timers[0] = .125F;
        state.multi_hit.timers[1] = 1;
        state.multi_hit.timers[2] = 1;
        state.multi_hit.write_index = 3;
    };
    game::Round clamped(1, 0);
    prime(fixture(clamped));
    fixture(clamped).score = -3;
    step(clamped, .125F);
    require(clamped.state().score == 2 && bonus(clamped), "Score clamp must run after the multiple-hit award, not before it");

    game::Round wrapped(1, 0);
    prime(fixture(wrapped));
    fixture(wrapped).score = std::numeric_limits<std::int32_t>::max() - 2;
    step(wrapped, .125F);
    require(wrapped.state().score == 0 && bonus(wrapped), "Original 32-bit bonus addition wraps before signed-negative clamping");

    game::Round full(1, 0);
    auto& state = fixture(full);
    prime(state);
    for (auto& notice : state.notices) {
        notice = {true, false, 3, 200, 101};
    }
    step(full, .125F);
    require(state.score == 0 && !bonus(full) && state.multi_hit.write_index == 0 && !state.notices[0].active,
        "Full pool suppresses bonus and sound even when those notices expire during later composition");
    step(full, .25F);
    require(state.score == 0 && !bonus(full), "Suppressed group is consumed, not deferred until a free notice slot");

    game::Round hero(1, 0);
    auto& both = fixture(hero);
    prime(both);
    both.spider_hits = 26;
    for (std::size_t i = 0; i < 7; ++i) {
        both.notices[i] = {true, false, 0, 200, 3};
    }
    step(hero, .125F);
    require(both.spider_hero && both.score == 100 && both.notices[7].type == 101 && both.notices[7].y == 175 && !bonus(hero),
        "Earlier spider hero reserves the last shared notice slot and retains its distinct unconditional score policy");

    game::Round reset(1, 0);
    require(reset.state().selected_slot == -1 && reset.state().multi_hit == game::specials::MultiHitState{}
                && std::ranges::none_of(reset.state().notices,
                    [](const auto& notice) {
                        return notice.active || notice.visible;
                    }),
        "New rounds clear pending selection, tracker and every shared notice");
}
} // namespace

int main() {
    try {
        delayed_bonus_and_misses();
        current_slot_type();
        pool_order_and_clamp();
        std::cout << "Shared multiple-hit registration, expiry, pool ordering, score, reset and notice integration passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
