#pragma once

#include "audio/engine.hpp"

namespace moorhuhn2::app {

class AudioSink {
public:
    explicit AudioSink(audio::Engine& engine);
    AudioSink(const AudioSink&) = delete;
    AudioSink& operator=(const AudioSink&) = delete;

    void command(
        audio::Action action, audio::Stream stream = audio::Stream::none, audio::Sample sample = audio::Sample::shot, float gain = 1.0F, std::optional<float> pan = std::nullopt);
    void play(audio::Sample sample);
    void select_stream(audio::Stream stream);
    // Temporary muting (boss screen) must not change the application's fade gain.
    void set_gain(float gain);
    void set_airplane_gain(float gain);
    void fade_in(float delta);
    void fade_out(float delta);
    void fade_round_exit(float delta);
    float stream_gain() const;
    void pump(double elapsed);

private:
    audio::Engine& engine_;
    std::uint64_t sequence_{};
    float stream_gain_{};
    double fraction_{};
    std::vector<float> offline_pcm_;
};

} // namespace moorhuhn2::app
