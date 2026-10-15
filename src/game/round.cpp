#include "game/round.hpp"

#include "game/round_math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::game {
using detail::stored;
using detail::truncated;

namespace {
void validate_sample(float value) {
    // Bound _ftol-style conversions before converting public clock samples.
    if (!std::isfinite(value) || value < 0.0F || value >= 2147483648.0F) {
        throw std::invalid_argument("Round clock sample must fit nonnegative signed 32-bit seconds");
    }
}
} // namespace

Round::Round(float seed_time, float initial_sample, int countdown_second) {
    validate_sample(seed_time);
    validate_sample(initial_sample);
    state_.previous_sample = initial_sample;
    state_.countdown_second = countdown_second;
    state_.random_state = static_cast<std::uint32_t>(truncated(seed_time));

    for (std::size_t index = 0; index < state_.ammo.size(); ++index) {
        load_ammo(static_cast<int>(index), index >= initial_ammo_index ? 1 : 0);
    }

    state_.persistent_leaves = world::make_persistent_leaves([&] {
        return random();
    });

    for (auto& stone : state_.stones) {
        stone.world_x = static_cast<float>(random() % 2248);
        stone.world_y = static_cast<float>(340 + random() % 80);
        world::update_stone(stone, 0, state_.camera, 0);
    }

    for (int index = 0; index < initial_scene_slots; ++index) {
        state_.chickens[static_cast<std::size_t>(index)].type = inert_scene_marker;
    }

    // Original zero-filled slots 128..255 have type 0/depth0/zero size. They stay
    // invisible and nonvacant even when the bonus expands the scene scan.
    for (std::size_t index = initial_scene_capacity; index < state_.chickens.size(); ++index) {
        state_.chickens[index].type = inert_scene_marker;
    }
}

void Round::reset_sample(float application_time) {
    validate_sample(application_time);

    if (awaiting_actions_) {
        throw std::logic_error("Resolve the current round input before resetting the clock");
    }

    state_.previous_sample = application_time;
    state_.delta = 0;
}

void Round::advance(contracts::FrameTime time, const RoundInput& input) {
    validate_sample(time.current);
    validate_sample(time.delta);
    validate_sample(time.total);

    if (static_cast<double>(time.total) * 60 >= std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Round accumulated time exceeds notice clock range");
    }

    if (time.current < state_.previous_sample) {
        throw std::invalid_argument("Round samples must be monotonic; reset on clock-domain changes");
    }

    if (awaiting_actions_) {
        throw std::logic_error("Round update requires input resolution for the preceding frame");
    }

    // The app also samples while other stages run or a focus update is skipped.
    // Its current delta cannot be reconstructed from the last hunting timestamp.
    state_.delta = time.delta;
    state_.previous_sample = time.current;
    state_.accumulated_time = time.total;
    input_ = input;
    input_.pointer.x = std::clamp(input_.pointer.x, 0, 630);
    input_.pointer.y = std::clamp(input_.pointer.y, 0, 470);
    events_.clear();
    awaiting_actions_ = true;
    ++state_.updates;

    if (input_.cancel) {
        state_.exit_requested = true;
        events_.push_back({RoundEventKind::game_over});
    }

    update_camera();
    update_clock_and_spawn();
    update_ammo();
    // Ascending pool order also fixes the shared CRT stream order.
    for (std::size_t index = 0; index < state_.scene_capacity; ++index) {
        update_scene_object(index);
    }

    update_particles();
    update_grants();
}

void Round::update_grants() {
    grant_stone_bonus();
    grant_spider_bonus();
    grant_scarecrow_bonus();

    if (world::apply_frog_time_bonus(state_.frog_hits, state_.stones_hit, state_.remaining)) {
        events_.push_back({RoundEventKind::frog_time_bonus, frog_scene_slot, 2});
    }

    grant_balloon_bonus();

    if (state_.branches_hit == 4 && !state_.branch_ammo_granted) {
        state_.reload_start_index = 6;
        state_.branch_ammo_granted = true;
    }

    if (world::update_blimp_progress(state_.blimp_progress)) {
        static_cast<void>(world::allocate_notice(state_.notices, world::notice_type::zeppelin_killer, 180));
        events_.push_back({RoundEventKind::blimp_killer});
    }

    if (state_.leaves_hit >= 10 && state_.branches_hit == 0 && random() % 512 < 8) {
        spawn_tree_fragment(27);
    }

    update_multi_hit();
    // The original clamps after all pre-input bonuses, including multiple hits.
    state_.score = std::max(0, state_.score);
    // Original notice rendering advances even newborn entries in this update.
    // Keep this state change in the core; repeated draw submission stays pure.
    for (auto& notice : state_.notices) {
        world::update_notice(notice, state_.delta, state_.accumulated_time);
    }
}

void Round::update_camera() {
    const long double movement = static_cast<long double>(state_.delta) * 200.0F;

    if (input_.pointer.x < 32 && state_.camera > 0) {
        state_.camera = stored(state_.camera - movement);
    }

    if (input_.pointer.x >= 576 && state_.camera < 1280.0F) {
        state_.camera = stored(state_.camera + movement);
    }

    if (input_.held_left && state_.camera > 0) {
        state_.camera = stored(state_.camera - movement);
    }

    if (input_.held_right && state_.camera < 1280.0F) {
        state_.camera = stored(state_.camera + movement);
    }

    state_.camera = std::clamp(state_.camera, 0.0F, 1280.0F);
}

void Round::update_clock_and_spawn() {
    const long double remaining = static_cast<long double>(state_.remaining) - state_.delta;
    state_.remaining = stored(remaining);
    state_.minutes = truncated(remaining * 0.01666666753590107F);
    state_.seconds = truncated(remaining - static_cast<long double>(state_.minutes) * 60);
    // Expiry is a countdown-cache transition into the zero-second bucket.
    // A large delta can skip it; remaining <= 0 is not the original guard.
    if (state_.minutes < 1) {
        if (state_.seconds == 0 && state_.countdown_second != 0) {
            state_.countdown_second = 0;
            state_.exit_requested = true;
            events_.push_back({RoundEventKind::game_over});
        } else if (state_.seconds != 0 && state_.seconds < 10 && state_.seconds != state_.countdown_second) {
            state_.countdown_second = state_.seconds;
            events_.push_back({RoundEventKind::countdown});
        }
    }

    const long double spawn_time = state_.spawn_accumulator + static_cast<long double>(state_.delta) * 50.0F;
    state_.spawn_accumulator = stored(spawn_time);
    // One request per update discards overshoot and compares the retained sum.
    if (spawn_time > state_.spawn_threshold) {
        state_.spawn_accumulator = 0;
        spawn();
    }
}

const RoundState& Round::state() const noexcept {
    return state_;
}

std::span<const RoundEvent> Round::events() const noexcept {
    return events_;
}

} // namespace moorhuhn2::game
