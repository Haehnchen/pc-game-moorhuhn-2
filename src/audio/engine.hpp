#pragma once

#include "assets/asset_store.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace moorhuhn2::audio {

enum class OutputMode {
    device,
    offline
};
enum class OutputStatus {
    device,
    offline,
    unavailable_device
};
enum class Stream {
    none,
    title,
    birds,
    highscore
};
enum class Sample {
    shot,
    empty_fire,
    reload,
    get_ready,
    game_over,
    time_running_out,
    chicken_hit,
    chicken_far,
    chicken_mid,
    chicken_close,
    typing_click,
    boat_impact,
    stone_impact,
    magic_balloons,
    balloon_hit,
    boat_sinking,
    rare_balloon,
    church_bell,
    church_bell_second,
    house_hit,
    frog_croak,
    frog_hit,
    spider_swing,
    branch_ground,
    tree_break,
    tree_highfly,
    scarecrow_hit,
    hunting_stand_ladder,
    hunting_stand_broken,
    big_chicken_appear,
    airplane_motor,
    airplane_failure,
    airplane_hit,
    airplane_crash,
    blimp_hit
};
enum class Action {
    select_stream,
    set_stream_gain,
    stop_stream,
    play_sample,
    stop_samples,
    pause,
    resume,
    start_loop,
    set_loop_pan,
    stop_loop,
    set_loop_gain
};

struct Command {
    std::uint64_t sequence{};
    Action action{};
    Stream stream{Stream::none};
    Sample sample{Sample::shot};
    float gain{1.0F};
    std::optional<float> pan{};
};

struct AudioInfo {
    std::string id;
    int sample_rate{}, channels{};
    std::int64_t decoded_frames{};
};

struct LoadObserver {
    std::span<const std::string_view> order{};
    std::function<void(std::string_view)> loaded{};
};

struct State {
    Stream stream{Stream::none};
    float stream_gain{};
    bool paused{};
    std::uint64_t last_sequence{};
    std::uint64_t commands_consumed{};
    std::uint64_t samples_started{};
    std::uint64_t streams_started{};
    std::uint64_t frames_rendered{};
    std::uint64_t loops_started{};
    std::uint64_t channels_stolen{};
    std::size_t active_samples{};
    std::size_t loop_tracks{};
    std::size_t active_loops{};
    std::size_t channel_tracks{};
    std::size_t active_channels{};
    int last_channel{-1};
    std::int64_t stream_position{};
};

[[nodiscard]] std::string_view asset_id(Stream stream);
[[nodiscard]] std::string_view asset_id(Sample sample);
[[nodiscard]] float quantize_gain(float gain);
[[nodiscard]] int quantize_pan(float pan);

// Main-thread adapter. Prepared 44100-Hz PCM uses original integer gain and
// per-channel saturated accumulation; SDL handles decoding and final output.
// Inventory and reported stream position retain native source units.
// Other offline rates convert the final mix, including normal resampler history.
// Device playback uses the same 44100-Hz mix before SDL's hardware conversion.
class Engine final {
public:
    explicit Engine(const assets::AssetStore& assets, OutputMode mode = OutputMode::device, int mix_sample_rate = 44100, const LoadObserver& observer = {});
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    // Sequence IDs begin at 1, increase across the lifetime, and identify an
    // event once. Duplicate commands at or below last_sequence are ignored.
    // Music, one-shots and loops share the original 32 software channels; music
    // is reserved. Effect handles retain a channel index, including after theft.
    // Starting replaces its playback; setting pan or gain preserves position.
    // stop_samples clears effects and their handles. stop_loop stops the current
    // channel occupant, then releases that sample's retained handle.
    void apply(std::span<const Command> commands);
    // Offline and device modes use the same integer kernel through SDL_mixer.
    // Offline only: device mode is pumped by SDL's callback and rejects calls.
    [[nodiscard]] std::vector<float> render_frames(std::size_t frames);
    // Reuse storage for interleaved stereo samples (two floats per frame).
    void render_frames(std::span<float> pcm);
    [[nodiscard]] OutputStatus status() const;
    [[nodiscard]] bool needs_pumping() const;
    [[nodiscard]] int mix_sample_rate() const;
    [[nodiscard]] const std::string& device_error() const;
    [[nodiscard]] State state() const;
    [[nodiscard]] std::span<const AudioInfo> inventory() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace moorhuhn2::audio
