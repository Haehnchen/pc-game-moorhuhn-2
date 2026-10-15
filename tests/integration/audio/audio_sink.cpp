#include "app/audio_sink.hpp"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
namespace audio = moorhuhn2::audio;
using moorhuhn2::app::AudioSink;

void check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

void fades_and_commands(audio::Engine& engine, AudioSink& sink) {
    sink.select_stream(audio::Stream::title);
    sink.fade_in(1.5F);
    check(sink.stream_gain() == 0.75F, "Title fade must retain its unquantized gain");
    sink.set_gain(0.0F);
    check(engine.state().stream_gain == 0.0F && sink.stream_gain() == 0.75F, "Boss muting must preserve the fade gain");
    sink.fade_in(0.5F);
    check(engine.state().stream_gain == 1.0F && sink.stream_gain() == 1.0F, "Resume must continue the cached fade");
    sink.select_stream(audio::Stream::birds);
    check(sink.stream_gain() == 0.0F && engine.state().stream == audio::Stream::birds, "Stream selection must restart the fade");
    sink.fade_in(4.0F);
    check(sink.stream_gain() == 1.0F, "Fade in must clamp to full gain");
    sink.fade_out(0.5F);
    check(sink.stream_gain() == 0.75F, "Fade out must retain partial gain");
    sink.fade_round_exit(0.5F);
    check(sink.stream_gain() == 0.5F, "Round exit must continue the current fade");
    sink.fade_out(2.0F);
    check(sink.stream_gain() == 0.0F, "Fade out must clamp to silence");
    sink.play(audio::Sample::shot);
    sink.command(audio::Action::start_loop, audio::Stream::none, audio::Sample::airplane_motor);
    sink.command(audio::Action::start_loop, audio::Stream::none, audio::Sample::airplane_failure);
    sink.set_airplane_gain(0.0F);
    const auto state = engine.state();
    check(state.commands_consumed == 14 && state.last_sequence == 14, "All helpers must share one increasing command sequence");
    check(state.samples_started == 1 && state.active_loops == 2, "Muting must preserve both airplane loops");

    // At this halfway case, rounding the product first would yield one ULP,
    // while the original extended subtraction rounds the result to two ULPs.
    sink.fade_in(std::bit_cast<float>(std::uint32_t{6}));
    sink.fade_round_exit(std::bit_cast<float>(std::uint32_t{3}));
    check(std::bit_cast<std::uint32_t>(sink.stream_gain()) == 2, "Round-exit fade must keep its extended intermediate");
}

void fractional_pumping(audio::Engine& engine, AudioSink& sink) {
    const double quarter_frame = 0.25 / engine.mix_sample_rate();
    for (int i = 0; i < 3; ++i) {
        sink.pump(quarter_frame);
    }
    check(engine.state().frames_rendered == 0, "Partial audio frames must be retained");
    sink.pump(quarter_frame);
    check(engine.state().frames_rendered == 1, "Fractional requests must accumulate to a complete audio frame");
    sink.pump(3.5 / engine.mix_sample_rate());
    check(engine.state().frames_rendered == 4, "Pumping must retain the next fractional remainder");
    sink.pump(0.5 / engine.mix_sample_rate());
    check(engine.state().frames_rendered == 5, "The retained remainder must survive successive pumps");
    sink.pump(24.0 * 60 * 60);
    check(engine.state().frames_rendered == 5 + static_cast<std::uint64_t>(engine.mix_sample_rate()) / 4, "Standby must render at most 250 ms of offline audio");
    const auto before = engine.state().frames_rendered;
    for (const double elapsed : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        bool rejected = false;
        try {
            sink.pump(elapsed);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && engine.state().frames_rendered == before, "Invalid time must leave offline audio unchanged");
    }
    sink.pump(1.0 / engine.mix_sample_rate());
    check(engine.state().frames_rendered == before + 1, "Offline pumping must recover after invalid time");
}
} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Usage: moorhuhn2_audio_sink_tests ASSETS");
        const auto assets = moorhuhn2::assets::AssetStore::load_directory(argv[1]);
        audio::Engine engine(assets, audio::OutputMode::offline);
        AudioSink sink(engine);
        fades_and_commands(engine, sink);
        fractional_pumping(engine, sink);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "audio sink: " << error.what() << '\n';
        return 1;
    }
}
