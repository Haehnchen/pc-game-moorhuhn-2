#pragma once

#include "app/audio_sink.hpp"
#include "app/loading.hpp"
#include "bonus/computer.hpp"
#include "bonus/making_of.hpp"
#include "bonus/snake.hpp"
#include "game/round.hpp"
#include "platform/clock.hpp"
#include "platform/input.hpp"
#include "render/frame_catalog.hpp"
#include "render/round_scene.hpp"
#include "ui/house88_results.hpp"
#include "ui/results_celebration.hpp"
#include "ui/results_details.hpp"
#include "ui/results_transition.hpp"
#include "ui/round_exit.hpp"
#include "ui/scores.hpp"
#include "ui/screens.hpp"
#include "ui/title.hpp"
#include "world/presentation.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace moorhuhn2::app {

namespace key_code = platform::key_code;

inline constexpr contracts::ScreenPoint screen_center{contracts::logical_width / 2, contracts::logical_height / 2};

// Owns session state; bootstrap resources outlive this controller in app::run().
class Application {
public:
    Application(const platform::AppPaths& paths, platform::Display& display, render::ImageBank& images, render::Framebuffer& framebuffer, LoadedResources& resources);
    void execute();

private:
    enum class Phase {
        title,
        introduction,
        hunting,
        round_exit,
        boss,
        results,
        results_exit,
        disclaimer,
        computer,
        extras_menu,
        snake,
        making_of,
        outro
    };

    struct FrameControls {
        contracts::FrameTime time{};
        std::uint16_t key{};
        bool fire{}, reload{}, release{};
        bool held_left{}, held_right{}, held_up{}, held_down{};
        contracts::ScreenPoint round_pointer{};

        bool pressed(std::uint16_t expected) const {
            return key == expected;
        }
    };

    // These effects belong to the dispatched frame even when its update already
    // changed phase. Some transitions must wait until its image is presented.
    struct FrameDispatch {
        bool hunting_dispatch{};
        bool round_exit_finished{};
        bool outro_dispatch{};
        bool results_exit_requested{}, results_returning{};
        std::size_t results_background_count{};
    };

    void initialize();
    void save();
    std::int64_t counter() const;
    void reset_clock();
    std::optional<contracts::FrameTime> sample_time(bool frame_focused);
    void collect_mouse(const contracts::InputFrame& frame, FrameControls& controls);
    void read_keys(FrameControls& controls);

    void update_frame(const FrameControls& controls, double elapsed);
    void update_phase(const FrameControls& controls, FrameDispatch& dispatch);
    void draw_frame(const FrameDispatch& dispatch);
    void finish_frame(const FrameDispatch& dispatch);
    void transition(Phase next);
    void return_to_title();

    void update_title(const FrameControls& controls);
    void update_disclaimer(const FrameControls& controls);
    void update_computer(const FrameControls& controls);
    void update_extras_menu(const FrameControls& controls);
    void update_making_of(const FrameControls& controls);
    void update_snake(const FrameControls& controls);
    void update_outro(const FrameControls& controls, FrameDispatch& dispatch);
    void update_introduction(const FrameControls& controls);
    void update_boss();
    void update_hunting(const FrameControls& controls, FrameDispatch& dispatch);
    void update_round_exit(const FrameControls& controls, FrameDispatch& dispatch);
    void update_results(const FrameControls& controls, FrameDispatch& dispatch);
    void update_results_exit(const FrameControls& controls, FrameDispatch& dispatch);

    void draw_hunting();
    void resolve_hunting();
    void enter_round_exit();
    void enter_results();
    void draw_result_rows(bool show_caret);
    void edit_result_name(std::uint16_t key);
    void open_results_website();

    platform::Display& display;
    render::ImageBank& images;
    render::Framebuffer& framebuffer;
    audio::Engine& audio;
    AudioSink sound;
    const render::FrameCatalog catalog;
    const world::Presentation landscape;

    ui::Title title;
    contracts::ScreenPoint pointer{screen_center};
    platform::InputAdapter input;
    ui::Scores scores;
    const std::filesystem::path save_path;

    Phase phase{Phase::title};
    ui::RoundIntroduction introduction;
    ui::RoundExit round_exit;
    std::vector<contracts::SpriteDraw> round_exit_world;
    int round_scene_recoil{};
    std::optional<game::Round> round;
    int countdown_second{};

    std::optional<std::size_t> editing;
    std::optional<ui::House88Results> house88_results;
    std::optional<ui::ResultsCelebration> results_celebration;
    std::optional<bonus::Computer> computer;
    ui::ExtrasMenu extras_menu;
    std::optional<bonus::Snake> snake;
    std::optional<bonus::MakingOf> making_of;
    std::optional<ui::Outro> outro;
    ui::ResultsRandom results_random;
    ui::ResultsTransition results_transition;
    ui::ResultsDetails results_details;
    bool results_hover{};
    std::uint32_t session_random_state{1};
    int results_score{};
    bool running{true};

    std::vector<contracts::SpriteDraw> draws;
    render::RoundScene round_scene;

    std::uint64_t counter_origin{};
    platform::CounterClock counter_clock;
    platform::SampleClock sample_clock;
    std::uint64_t previous_wall{};
};

} // namespace moorhuhn2::app
