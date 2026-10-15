#include "app/application_state.hpp"
#include "app/round_audio.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <string>

namespace moorhuhn2::app {
void Application::draw_result_rows(bool show_caret) {
    const long double offset = results_transition.row_offset();
    std::array<char, 48> buffer{};
    for (std::size_t i = 0; i < scores.entries().size(); ++i) {
        auto name = scores.entries()[i].name;
        if (show_caret && editing == i && results_details.caret_visible()) {
            name += 'I';
        }

        ui::text(draws, name, static_cast<int>(offset + 220.0L), 150 + static_cast<int>(i) * 40);
        std::snprintf(buffer.data(), buffer.size(), "%5u", scores.entries()[i].score);
        ui::text(draws, buffer.data(), static_cast<int>(offset + 400.0L), 150 + static_cast<int>(i) * 40);
    }
}

void Application::update_hunting(const FrameControls& controls, FrameDispatch& dispatch) {
    const game::RoundInput round_input{controls.round_pointer, controls.fire, controls.reload, controls.held_left, controls.held_right, controls.pressed(key_code::cancel)};
    sound.fade_in(controls.time.delta);
    round->advance(controls.time, round_input);
    round_scene_recoil = round->state().recoil;

    draw_hunting();
    dispatch.hunting_dispatch = true;
}

void Application::update_round_exit(const FrameControls& controls, FrameDispatch& dispatch) {
    sound.fade_round_exit(controls.time.delta);
    const bool round_exit_skipped = controls.pressed(key_code::cancel) || controls.pressed(key_code::space);
    dispatch.round_exit_finished = round_exit.update(controls.time.delta, round_exit_skipped);

    draws = round_exit_world;
    const auto tiles = round_exit.draws();
    draws.insert(draws.end(), tiles.begin(), tiles.end());
}

void Application::update_results(const FrameControls& controls, FrameDispatch& dispatch) {
    sound.fade_in(controls.time.delta);
    results_details.update(controls.time.delta);
    results_transition.update_entry(controls.time.delta);

    edit_result_name(controls.key);
    results_hover = ui::results_website_hover(pointer);
    if (results_hover && controls.release) {
        open_results_website();
    }

    draws = results_transition.background_draws();
    dispatch.results_background_count = draws.size();
    if (house88_results) {
        house88_results->update(controls.time.delta, results_random);
        const auto chickens = house88_results->draws();
        draws.insert(draws.end(), chickens.begin(), chickens.end());
    } else if (results_celebration) {
        results_celebration->update(controls.time.delta, results_random);
        const auto decorations = results_celebration->draws();
        draws.insert(draws.end(), decorations.begin(), decorations.end());
    }

    std::array<char, 48> buffer{};
    draw_result_rows(true);
    if (results_details.personal_score_visible()) {
        std::snprintf(buffer.data(), buffer.size(), "%i", results_score);
        ui::text(draws, buffer.data(), results_score < 10 ? 305 : results_score < 100 ? 290 : results_score < 1000 ? 275 : 260, 380);
    }
    ui::cursor(draws, pointer, results_hover ? 1 : 0);

    if (controls.pressed(key_code::cancel) || controls.release) {
        editing.reset();
        dispatch.results_exit_requested = true;
    }
}

void Application::update_results_exit(const FrameControls& controls, FrameDispatch& dispatch) {
    sound.fade_out(controls.time.delta);
    dispatch.results_returning = results_transition.update_exit(controls.time.delta, controls.pressed(key_code::cancel) || controls.pressed(key_code::space) || controls.release);

    draws = results_transition.background_draws();
    draw_result_rows(false);
    ui::cursor(draws, pointer, results_hover ? 1 : 0);
}

void Application::draw_hunting() {
    round_scene.append(draws, *round, landscape, pointer);
}

void Application::edit_result_name(std::uint16_t key) {
    if (editing && key) {
        bool changed = false;
        const auto character = key & 0x1ff;
        if (character == key_code::backspace) {
            changed = scores.backspace(*editing);
        } else if (character == key_code::accept || character == key_code::cancel) {
            editing.reset();
        } else if (character < 128) {
            changed = scores.type(*editing, static_cast<char>(character));
        }

        if (changed) {
            sound.play(audio::Sample::typing_click);
        }
    }
}

void Application::open_results_website() {
    const auto url = "http://" + std::string(ui::results_website_url);
    const bool opened = SDL_OpenURL(url.c_str());
    if (!opened) {
        std::cerr << "moorhuhn2: could not open results website: " << SDL_GetError() << '\n';
    }
}

void Application::resolve_hunting() {
    round->resolve_actions([&](const auto& draw, const auto point) {
        return framebuffer.matches_sprite_pixel(images, draw, point);
    });
    countdown_second = round->state().countdown_second;

    for (const auto& event : round->events()) {
        if (const auto cue = round_audio(event)) {
            sound.command(cue->action, cue->stream, cue->sample, cue->gain, cue->pan);
        }
    }
}

void Application::enter_round_exit() {
    round_exit = ui::RoundExit{};
    std::vector<contracts::DepthDraw> exit_targets;
    for (int depth = 4; depth >= 0; --depth) {
        round->append_target_draws(depth, exit_targets);
    }

    const auto& state = round->state();
    round_exit_world = landscape.commands(
        state.camera, exit_targets, {}, state.recoil, state.boat, state.branches, state.trees, state.scarecrow, &state.hunting_stand, &state.rose, round_scene_recoil);
    // Stage 1 is the next iteration of the same original loop. Preserve
    // later queued keys; only the current hunting key was consumed.
    transition(Phase::round_exit);
}

void Application::enter_results() {
    if (round->state().airplane_state.engine_loop) {
        sound.command(audio::Action::stop_loop, audio::Stream::none, audio::Sample::airplane_motor);
    }

    if (round->state().airplane_state.failure_loop) {
        sound.command(audio::Action::stop_loop, audio::Stream::none, audio::Sample::airplane_failure);
    }

    sound.command(audio::Action::stop_stream);
    sound.select_stream(audio::Stream::highscore);
    results_transition = ui::ResultsTransition{};
    results_hover = false;
    results_score = std::max(0, round->state().score);
    results_random = {round->state().random_state, round->state().random_draws};
    if (round->state().house88_mode) {
        results_celebration.reset();
        house88_results.emplace(results_score, results_random);
        results_score = 0;
        editing.reset();
    } else {
        house88_results.reset();
        editing = scores.insert(results_score, round->state().hits, round->state().misses);
        results_celebration.emplace(results_score, editing == 0, results_random);
    }

    results_details = ui::ResultsDetails(editing.has_value());
    transition(Phase::results);
}

} // namespace moorhuhn2::app
