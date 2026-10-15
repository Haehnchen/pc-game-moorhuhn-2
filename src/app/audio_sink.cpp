#include "app/audio_sink.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::app {

AudioSink::AudioSink(audio::Engine& engine) : engine_(engine) {}

void AudioSink::command(audio::Action action, audio::Stream stream, audio::Sample sample, float gain, std::optional<float> pan) {
    const std::array commands{audio::Command{++sequence_, action, stream, sample, gain, pan}};
    engine_.apply(commands);
}

void AudioSink::play(audio::Sample sample) {
    command(audio::Action::play_sample, audio::Stream::none, sample);
}

void AudioSink::select_stream(audio::Stream stream) {
    command(audio::Action::select_stream, stream);
    stream_gain_ = 0.0F;
}

void AudioSink::set_gain(float gain) {
    command(audio::Action::set_stream_gain, audio::Stream::none, audio::Sample::shot, gain);
}

void AudioSink::set_airplane_gain(float gain) {
    command(audio::Action::set_loop_gain, audio::Stream::none, audio::Sample::airplane_motor, gain);
    command(audio::Action::set_loop_gain, audio::Stream::none, audio::Sample::airplane_failure, gain);
}

void AudioSink::fade_in(float delta) {
    stream_gain_ = std::min(1.0F, stream_gain_ + delta * 0.5F);
    set_gain(stream_gain_);
}

void AudioSink::fade_out(float delta) {
    stream_gain_ = std::max(0.0F, stream_gain_ - delta * 0.5F);
    set_gain(stream_gain_);
}

void AudioSink::fade_round_exit(float delta) {
    // This original fade stores only after the extended-precision subtraction.
    stream_gain_ = std::max(0.0F, static_cast<float>(static_cast<long double>(stream_gain_) - static_cast<long double>(delta) * 0.5F));
    set_gain(stream_gain_);
}

float AudioSink::stream_gain() const {
    return stream_gain_;
}

void AudioSink::pump(double elapsed) {
    if (!engine_.needs_pumping()) {
        return;
    }

    if (!std::isfinite(elapsed) || elapsed < 0) {
        throw std::invalid_argument("Audio elapsed time must be finite and nonnegative");
    }

    fraction_ += std::min(elapsed, 0.25) * static_cast<double>(engine_.mix_sample_rate());
    const auto count = static_cast<std::size_t>(fraction_);
    fraction_ -= static_cast<double>(count);
    if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()) / (2 * sizeof(float))) {
        throw std::length_error("offline audio request exceeds SDL buffer limit");
    }

    offline_pcm_.resize(count * 2);
    engine_.render_frames(offline_pcm_);
}

} // namespace moorhuhn2::app
