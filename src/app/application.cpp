#include "app/application.hpp"

#include "app/application_state.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <iostream>
#include <span>
#include <stdexcept>
#include <utility>

namespace moorhuhn2::app {
namespace {
using contracts::InputControl;
using contracts::InputEdge;
using contracts::SpriteDraw;

ui::Title present_initial_title(platform::Display& display, const render::ImageBank& images, render::Framebuffer& framebuffer) {
    ui::Title title;
    const contracts::ScreenPoint pointer{screen_center};
    // Wayland maps a window only after its first buffer is presented. Waiting
    // for focus before this frame would prevent the window from gaining focus.
    static_cast<void>(title.update(0, false, false, false, pointer));
    const auto initial_title = title.draws(pointer);
    framebuffer.clear();
    framebuffer.draw(images, initial_title);
    display.present(framebuffer.bytes());
    return title;
}
} // namespace

Application::Application(const platform::AppPaths& paths, platform::Display& display, render::ImageBank& images, render::Framebuffer& framebuffer, LoadedResources& resources)
    : display(display), images(images), framebuffer(framebuffer), audio(resources.audio), sound(audio), catalog(std::move(resources.catalog)), landscape(catalog),
      title(present_initial_title(display, images, framebuffer)), input(display), save_path(paths.user_file("moorhuhn2.sav")),
      counter_clock(static_cast<std::int64_t>(SDL_GetPerformanceFrequency()), 0) {}

void Application::execute() {
    initialize();

    while (running) {
        const auto frame_started = SDL_GetTicksNS();
        const auto frame = input.poll();
        const auto wall = SDL_GetTicksNS();
        const double elapsed = static_cast<double>(wall - previous_wall) / 1.0e9;
        previous_wall = wall;
        if (frame.quit_requested) {
            break;
        }

        FrameControls controls;
        collect_mouse(frame, controls);
        const auto time = sample_time(frame.focused);
        if (!time) {
            continue;
        }

        controls.time = *time;
        read_keys(controls);
        if (!frame.pointer) {
            controls.fire = controls.reload = controls.release = false;
        }

        controls.round_pointer = frame.clamped_pointer ? pointer : screen_center;
        update_frame(controls, elapsed);
        constexpr std::uint64_t frame_interval = 1'000'000'000 / 60;
        const auto frame_elapsed = SDL_GetTicksNS() - frame_started;
        if (frame_elapsed < frame_interval) {
            SDL_DelayNS(frame_interval - frame_elapsed);
        }
    }

    save();
}

void Application::initialize() {
    if (!audio.device_error().empty()) {
        std::cerr << "moorhuhn2: audio unavailable, continuing with offline mixing: " << audio.device_error() << '\n';
    }

    try {
        scores.load(save_path);
    } catch (const std::exception& error) {
        std::cerr << "moorhuhn2: using default highscores: " << error.what() << '\n';
        // Keep the unreadable file so saving the defaults cannot destroy it.
        auto backup = save_path;
        backup += ".bad";
        std::error_code ignored;
        std::filesystem::rename(save_path, backup, ignored);
    }

    sound.select_stream(audio::Stream::title);

    // SDL supplies a relative counter origin; WTN's reciprocal, pause and
    // binary32 stores then operate on that explicit native counter domain.
    counter_origin = SDL_GetPerformanceCounter();
    input.clear_keys();
    reset_clock();
    previous_wall = SDL_GetTicksNS();
}

void Application::save() {
    try {
        scores.save(save_path);
    } catch (const std::exception& error) {
        std::cerr << "moorhuhn2: could not save highscores: " << error.what() << '\n';
    }
}

std::int64_t Application::counter() const {
    const auto value = SDL_GetPerformanceCounter() - counter_origin;
    if (value > (std::uint64_t{1} << 53)) {
        throw std::out_of_range("Native performance counter exceeds exact clock domain");
    }

    return static_cast<std::int64_t>(value);
}

void Application::reset_clock() {
    sample_clock.reset_previous(counter_clock.sample(counter()));
}

void Application::transition(Phase next) {
    phase = next;
    switch (next) {
        case Phase::title:
        case Phase::results:
        case Phase::outro:
        case Phase::computer:
        case Phase::disclaimer:
        case Phase::snake:
            input.clear_keys();
            reset_clock();
            break;
        case Phase::introduction:
            // Round entry samples a new baseline without draining keys.
            reset_clock();
            break;
        case Phase::results_exit:
        case Phase::extras_menu:
        case Phase::making_of:
            input.clear_keys();
            break;
        case Phase::hunting:
        case Phase::round_exit:
        case Phase::boss:
            break;
    }
}

void Application::return_to_title() {
    title.reenter();
    if (title.introducing()) {
        pointer = screen_center;
        if (const auto window_point = display.viewport().logical_to_window(pointer)) {
            SDL_WarpMouseInWindow(display.window(), static_cast<float>(window_point->x), static_cast<float>(window_point->y));
        }
    }

    transition(Phase::title);
}

void Application::draw_frame(const FrameDispatch& dispatch) {
    if (dispatch.results_background_count != 0) {
        const std::span<const SpriteDraw> commands(draws);
        framebuffer.draw(images, commands.first(dispatch.results_background_count));
        // The original writes score bits to the destination surface before
        // drawing decorations and text; they are absent from the exit tiles.
        for (const auto& pixel : ui::results_watermark(scores.entries())) {
            framebuffer.set_rgb565_at(pixel.point.x, pixel.point.y, pixel.word);
        }

        framebuffer.draw(images, commands.subspan(dispatch.results_background_count));
    } else {
        framebuffer.draw(images, draws);
    }
}

void Application::finish_frame(const FrameDispatch& dispatch) {
    if (dispatch.hunting_dispatch && round->state().exit_requested) {
        enter_round_exit();
    }

    if (dispatch.round_exit_finished) {
        enter_results();
    }

    if (dispatch.results_exit_requested) {
        save();
        results_transition.begin_exit();
        transition(Phase::results_exit);
    }

    if (dispatch.results_returning) {
        sound.command(audio::Action::stop_stream);
        sound.select_stream(audio::Stream::title);
        session_random_state = results_random.state;
        return_to_title();
    }

    if (dispatch.outro_dispatch && !running) {
        sound.command(audio::Action::stop_stream);
    }
}

void Application::update_frame(const FrameControls& controls, double elapsed) {
    // The original toggles B before dispatch: returning runs hunting with
    // this sample and its mouse edges, rather than discarding another frame.
    if ((phase == Phase::hunting || phase == Phase::boss) && controls.pressed(key_code::boss)) {
        const bool resume = phase == Phase::boss;
        sound.set_gain(resume ? 1.0F : 0.0F);
        sound.set_airplane_gain(resume ? 1.0F : 0.0F);
        transition(resume ? Phase::hunting : Phase::boss);
    }

    draws.clear();
    framebuffer.clear();

    FrameDispatch dispatch;
    update_phase(controls, dispatch);
    draw_frame(dispatch);
    if (dispatch.hunting_dispatch) {
        resolve_hunting();
    }

    display.present(framebuffer.bytes());
    finish_frame(dispatch);
    sound.pump(elapsed);
}

void Application::update_phase(const FrameControls& controls, FrameDispatch& dispatch) {
    switch (phase) {
        case Phase::title:
            update_title(controls);
            break;
        case Phase::disclaimer:
            update_disclaimer(controls);
            break;
        case Phase::computer:
            update_computer(controls);
            break;
        case Phase::extras_menu:
            update_extras_menu(controls);
            break;
        case Phase::making_of:
            update_making_of(controls);
            break;
        case Phase::snake:
            update_snake(controls);
            break;
        case Phase::outro:
            update_outro(controls, dispatch);
            break;
        case Phase::introduction:
            update_introduction(controls);
            break;
        case Phase::boss:
            update_boss();
            break;
        case Phase::hunting:
            update_hunting(controls, dispatch);
            break;
        case Phase::round_exit:
            update_round_exit(controls, dispatch);
            break;
        case Phase::results:
            update_results(controls, dispatch);
            break;
        case Phase::results_exit:
            update_results_exit(controls, dispatch);
            break;
    }
}

std::optional<contracts::FrameTime> Application::sample_time(bool frame_focused) {
    if (counter_clock.state().paused) {
        if (!frame_focused) {
            SDL_Delay(16);
            return std::nullopt;
        }

        sound.command(audio::Action::resume);
        counter_clock.resume(counter());
        input.clear_keys();

        // Resume clears only queued keys and skips the blocked body. SDL's
        // focus-loss key releases already updated the logical held state.
        display.present(framebuffer.bytes());
        SDL_Delay(1);
        return std::nullopt;
    }

    const auto time = sample_clock.sample(counter_clock.sample(counter()));
    if (!frame_focused) {
        counter_clock.pause(counter());
        sound.command(audio::Action::pause);

        display.present(framebuffer.bytes());
        return std::nullopt;
    }

    return time;
}

void Application::collect_mouse(const contracts::InputFrame& frame, FrameControls& controls) {
    if (frame.clamped_pointer) {
        pointer = {std::clamp(frame.clamped_pointer->x, 0, 630), std::clamp(frame.clamped_pointer->y, 0, 470)};
    }

    for (const auto& action : frame.actions) {
        if (action.control == InputControl::primary) {
            controls.fire = controls.fire || action.edge == InputEdge::pressed;
            controls.release = controls.release || action.edge == InputEdge::released;
        } else if (action.control == InputControl::secondary) {
            controls.reload = controls.reload || action.edge == InputEdge::pressed;
        }
    }
}

void Application::read_keys(FrameControls& controls) {
    controls.key = input.take_key();
    controls.held_left = input.key_held(key_code::left);
    controls.held_right = input.key_held(key_code::right);
    controls.held_up = input.key_held(key_code::up);
    controls.held_down = input.key_held(key_code::down);
}

int run() {
    platform::SdlRuntime runtime;
    const auto paths = platform::AppPaths::discover();
    platform::Display display;
    SDL_HideCursor();
    render::ImageBank images;
    render::Framebuffer framebuffer;
    auto loaded = load_resources(paths, display, images, framebuffer);
    if (!loaded) {
        return 0;
    }

    Application application(paths, display, images, framebuffer, *loaded);
    application.execute();
    return 0;
}

} // namespace moorhuhn2::app
