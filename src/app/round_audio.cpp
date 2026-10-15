#include "app/round_audio.hpp"

namespace moorhuhn2::app {
namespace {
audio::Command play(audio::Sample sample, std::optional<float> pan = {}) {
    return {0, audio::Action::play_sample, audio::Stream::none, sample, 1.0F, pan};
}
} // namespace

std::optional<audio::Command> round_audio(const game::RoundEvent& event) {
    switch (event.kind) {
        case game::RoundEventKind::shot:
            return play(audio::Sample::shot);
        case game::RoundEventKind::empty:
            return play(audio::Sample::empty_fire);
        case game::RoundEventKind::reload:
            return play(audio::Sample::reload);
        case game::RoundEventKind::countdown:
            return play(audio::Sample::time_running_out);
        case game::RoundEventKind::game_over:
            return play(audio::Sample::game_over);
        case game::RoundEventKind::chicken_hit:
            return play(event.depth == 2 ? audio::Sample::chicken_close : event.depth == 3 ? audio::Sample::chicken_mid : audio::Sample::chicken_far);
        case game::RoundEventKind::big_chicken_appear:
            return play(audio::Sample::big_chicken_appear, event.pan);
        case game::RoundEventKind::special_chicken_hit:
            return play(audio::Sample::chicken_hit);
        case game::RoundEventKind::airplane_engine_start:
            return audio::Command{0, audio::Action::start_loop, audio::Stream::none, audio::Sample::airplane_motor};
        case game::RoundEventKind::airplane_engine_pan:
            return audio::Command{0, audio::Action::set_loop_pan, audio::Stream::none, audio::Sample::airplane_motor, 1.0F, event.pan};
        case game::RoundEventKind::airplane_engine_stop:
            return audio::Command{0, audio::Action::stop_loop, audio::Stream::none, audio::Sample::airplane_motor};
        case game::RoundEventKind::airplane_failure_start:
            return audio::Command{0, audio::Action::start_loop, audio::Stream::none, audio::Sample::airplane_failure};
        case game::RoundEventKind::airplane_failure_pan:
            return audio::Command{0, audio::Action::set_loop_pan, audio::Stream::none, audio::Sample::airplane_failure, 1.0F, event.pan};
        case game::RoundEventKind::airplane_failure_stop:
            return audio::Command{0, audio::Action::stop_loop, audio::Stream::none, audio::Sample::airplane_failure};
        case game::RoundEventKind::airplane_hit:
            return play(audio::Sample::airplane_hit);
        case game::RoundEventKind::airplane_crash:
            return play(audio::Sample::airplane_crash);
        case game::RoundEventKind::blimp_hit:
            return play(audio::Sample::blimp_hit);
        case game::RoundEventKind::boat_hit:
        case game::RoundEventKind::branch_hit:
        case game::RoundEventKind::tree_hit:
        case game::RoundEventKind::hunting_stand_hit:
            return play(audio::Sample::boat_impact);
        case game::RoundEventKind::hunting_stand_chicken:
        case game::RoundEventKind::hidden_chicken_far:
            return play(audio::Sample::chicken_far);
        case game::RoundEventKind::hidden_chicken_close:
            return play(audio::Sample::chicken_close);
        case game::RoundEventKind::hidden_chicken_mid:
            return play(audio::Sample::chicken_mid);
        case game::RoundEventKind::hunting_stand_ladder:
            return play(audio::Sample::hunting_stand_ladder);
        case game::RoundEventKind::hunting_stand_broken:
            return play(audio::Sample::hunting_stand_broken);
        case game::RoundEventKind::branch_ground:
        case game::RoundEventKind::tree_ground:
            return play(audio::Sample::branch_ground);
        case game::RoundEventKind::tree_destroyed:
            return play(audio::Sample::tree_break);
        case game::RoundEventKind::tree_highfly:
            return play(audio::Sample::tree_highfly);
        case game::RoundEventKind::scarecrow_hit:
            return play(audio::Sample::scarecrow_hit);
        case game::RoundEventKind::stone_hit:
            return play(audio::Sample::stone_impact);
        case game::RoundEventKind::special_ammo:
        case game::RoundEventKind::hidden_chicken_balloons:
        case game::RoundEventKind::spider_balloons:
        case game::RoundEventKind::balloon_hero:
        case game::RoundEventKind::frog_tree_bonus:
        case game::RoundEventKind::scarecrow_balloons:
            return play(audio::Sample::magic_balloons);
        case game::RoundEventKind::balloon_hit:
            return play(audio::Sample::balloon_hit);
        case game::RoundEventKind::boat_sunk:
            return play(audio::Sample::boat_sinking);
        case game::RoundEventKind::rare_balloon:
        case game::RoundEventKind::hidden_leaf_hit:
        case game::RoundEventKind::frog_time_bonus:
        case game::RoundEventKind::spider_hero:
        case game::RoundEventKind::multi_hit_bonus:
            return play(audio::Sample::rare_balloon);
        case game::RoundEventKind::church_bell:
            return play(audio::Sample::church_bell);
        case game::RoundEventKind::church_bell_second:
            return play(audio::Sample::church_bell_second);
        case game::RoundEventKind::house_hit:
            return play(audio::Sample::house_hit);
        case game::RoundEventKind::frog_croak:
            return play(audio::Sample::frog_croak, event.pan);
        case game::RoundEventKind::frog_hit:
            return play(audio::Sample::frog_hit);
        case game::RoundEventKind::spider_hit:
            return play(audio::Sample::spider_swing);
        case game::RoundEventKind::leaf_hit:
        case game::RoundEventKind::blimp_killer:
        case game::RoundEventKind::scarecrow_chickens:
        case game::RoundEventKind::spawn:
            return std::nullopt;
    }

    return std::nullopt;
}

} // namespace moorhuhn2::app
