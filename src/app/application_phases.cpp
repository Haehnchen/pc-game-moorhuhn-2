#include "app/application_state.hpp"

#include <algorithm>

namespace moorhuhn2::app {
void Application::update_title(const FrameControls& controls) {
    sound.fade_in(controls.time.delta);
    const auto action = title.update(controls.time.delta, controls.pressed(key_code::space), controls.pressed(key_code::cancel), controls.release, pointer);

    draws = title.draws(pointer);
    if (controls.pressed(key_code::cancel)) {
        sound.play(audio::Sample::game_over);
    }

    if (action == ui::TitleAction::quit) {
        // The original persists the in-memory table at dispatcher exit, before the outro.
        save();
        outro.emplace(title.outro_tiles(), session_random_state);
        transition(Phase::outro);
    } else if (action == ui::TitleAction::start) {
        sound.play(audio::Sample::get_ready);
        // The source keeps this application-level countdown cache
        // across rounds, including an early Escape at seconds 9..1.
        const float seed_time = counter_clock.sample(counter());
        round.emplace(seed_time, seed_time, countdown_second);

        house88_results.reset();
        results_celebration.reset();
        introduction = ui::RoundIntroduction{};
        transition(Phase::introduction);
    } else if (action == ui::TitleAction::disclaimer) {
        transition(Phase::disclaimer);
    } else if (action == ui::TitleAction::extras_menu) {
        extras_menu = ui::ExtrasMenu{};
        transition(Phase::extras_menu);
    } else if (action == ui::TitleAction::extra) {
        computer.emplace(session_random_state);
        transition(Phase::computer);
    }
}

void Application::update_disclaimer(const FrameControls& controls) {
    draws.push_back(ui::screen("gfx/disclaimer"));
    if (controls.pressed(key_code::cancel) || controls.pressed(key_code::space) || controls.release) {
        return_to_title();
    }
}

void Application::update_computer(const FrameControls& controls) {
    const bool returning = controls.pressed(key_code::cancel) || controls.pressed(key_code::space) || controls.release;
    computer->update(controls.time.delta, session_random_state);

    const auto commands = computer->draws();
    draws.assign(commands.begin(), commands.end());

    if (returning) {
        return_to_title();
    }
}

void Application::update_extras_menu(const FrameControls& controls) {
    const int direction = controls.pressed(key_code::down) ? 1 : controls.pressed(key_code::up) ? -1 : 0;
    const auto action =
        extras_menu.update(direction, controls.pressed(key_code::accept) || controls.pressed(key_code::space), controls.pressed(key_code::cancel), controls.release, pointer);
    draws = extras_menu.draws(pointer);

    if (action == ui::ExtrasAction::back) {
        return_to_title();
    } else if (action == ui::ExtrasAction::snake) {
        snake.emplace();
        transition(Phase::snake);
    } else if (action == ui::ExtrasAction::making_of) {
        const bool witan = std::ranges::any_of(scores.entries(), [](const auto& entry) {
            return entry.name == "Witan";
        });
        making_of.emplace(witan);
        transition(Phase::making_of);
    }
}

void Application::update_making_of(const FrameControls& controls) {
    making_of->update(controls.time.delta);

    draws = making_of->draws(pointer);
    ui::cursor(draws, pointer);

    if (controls.pressed(key_code::cancel) || controls.pressed(key_code::space) || controls.release) {
        transition(Phase::extras_menu);
    }
}

void Application::update_snake(const FrameControls& controls) {
    const bonus::SnakeInput snake_input{
        controls.pressed(key_code::space), controls.pressed(key_code::cancel), controls.held_up, controls.held_right, controls.held_down, controls.held_left};
    const auto update = snake->update(controls.time.delta, snake_input, session_random_state);
    for (std::size_t i = 0; i < update.sound_count; ++i) {
        for (const auto sample : {audio::Sample::typing_click, audio::Sample::rare_balloon, audio::Sample::airplane_crash}) {
            if (update.sounds[i] == audio::asset_id(sample)) {
                sound.play(sample);
            }
        }
    }

    draws = snake->draws();

    if (update.exit_to_title) {
        transition(Phase::extras_menu);
    }
}

void Application::update_outro(const FrameControls& controls, FrameDispatch& dispatch) {
    dispatch.outro_dispatch = true;
    sound.fade_out(controls.time.delta);
    running = !outro->update(controls.time.delta, controls.pressed(key_code::cancel) || controls.pressed(key_code::space) || controls.release);

    const auto commands = outro->draws();
    draws.assign(commands.begin(), commands.end());
}

void Application::update_introduction(const FrameControls& controls) {
    sound.fade_out(controls.time.delta);
    const bool introduction_complete = introduction.update(controls.time.delta);
    title.update_introduction(controls.time.delta);

    draws = title.introduction_draws();
    const auto landscape_draws = landscape.commands(round->state().camera, {}, introduction.offsets());
    draws.insert(draws.end(), landscape_draws.begin(), landscape_draws.end());

    if (introduction_complete) {
        sound.select_stream(audio::Stream::birds);
        transition(Phase::hunting);
    }
}

void Application::update_boss() {
    draws.push_back(ui::screen("gfx/Bossscreen"));
    ui::cursor(draws, pointer, 2);
}

} // namespace moorhuhn2::app
