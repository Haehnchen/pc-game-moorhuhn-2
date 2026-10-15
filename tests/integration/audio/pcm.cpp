#include "audio/engine.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace audio = moorhuhn2::audio;
namespace assets = moorhuhn2::assets;

struct EncodedSource {
    std::string_view id;
    std::size_t complete_pcm_frames;
};

constexpr std::array encoded_sources{EncodedSource{"sfx/Moorhuhn2_Highscore_Music", 807552}, EncodedSource{"sfx/Moorhuhn2_Title_Music", 3044736},
    EncodedSource{"sfx/chickenhit2", 27072}, EncodedSource{"sfx/sfx_airplane_loop", 433152}, EncodedSource{"sfx/sfx_airplane_motor_failure_loop", 324864},
    EncodedSource{"sfx/sfx_background_birds_STEREO_LOOP", 346176}};

constexpr int original_mix_rate = 44100;
constexpr std::array<std::size_t, 5> requests{1, 577, 4093, 8191, 1153};

void check(bool value, const std::string& message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

void check_sdl(bool value, const std::string& message) {
    if (!value) {
        throw std::runtime_error(message + ": " + SDL_GetError());
    }
}

struct MixerLifetime {
    MixerLifetime() {
        check_sdl(MIX_Init(), "Initialize offline decoder");
    }

    ~MixerLifetime() {
        MIX_Quit();
    }
};

struct DecoderDeleter {
    void operator()(MIX_AudioDecoder* value) const {
        MIX_DestroyAudioDecoder(value);
    }
};

struct StreamDeleter {
    void operator()(SDL_AudioStream* value) const {
        SDL_DestroyAudioStream(value);
    }
};

struct Source {
    int rate{}, channels{};
    std::vector<std::int16_t> pcm;

    [[nodiscard]] std::size_t frames() const {
        return pcm.size() / static_cast<std::size_t>(channels);
    }

    [[nodiscard]] std::size_t mix_frames() const {
        return frames() * static_cast<std::size_t>(original_mix_rate / rate);
    }
};

// Decode native signed-16 frames to real EOF, without Engine, its source
// converter, a playback-rate resampler or the estimated manifest duration.
Source decode(const assets::AssetStore& store, std::string_view id, std::span<const EncodedSource> inventory) {
    const auto asset = store.audio(id);
    auto* io = SDL_IOFromConstMem(asset.encoded.data(), asset.encoded.size());
    check_sdl(io != nullptr, "Open source MP3");
    const std::unique_ptr<MIX_AudioDecoder, DecoderDeleter> decoder(MIX_CreateAudioDecoder_IO(io, true, 0));
    check_sdl(decoder != nullptr, "Create source decoder");
    SDL_AudioSpec spec{};
    check_sdl(MIX_GetAudioDecoderFormat(decoder.get(), &spec), "Query native source format");
    check(spec.freq == static_cast<int>(asset.sample_rate) && spec.channels == static_cast<int>(asset.channels), "Decoder changed native source format");
    check((spec.freq == 22050 || spec.freq == original_mix_rate) && (spec.channels == 1 || spec.channels == 2), "Unsupported reference source format");
    spec.format = SDL_AUDIO_S16;
    Source source{spec.freq, spec.channels, {}};
    std::array<std::int16_t, 8192> block{};

    while (true) {
        const auto bytes = MIX_DecodeAudio(decoder.get(), block.data(), sizeof(block), &spec);
        check_sdl(bytes >= 0, "Decode native signed-16 frames");
        check(bytes % static_cast<int>(spec.channels * sizeof(std::int16_t)) == 0, "Decode complete native signed-16 frames");

        if (bytes == 0) {
            break;
        }

        source.pcm.insert(source.pcm.end(), block.begin(), block.begin() + bytes / sizeof(std::int16_t));
        check(source.frames() <= 160 * asset.sample_rate, "Bound source duration");
    }

    check(!source.pcm.empty(), "Source decode is empty");
    const auto record = std::ranges::find_if(inventory, [&](const auto& row) {
        return row.id == id;
    });
    check(record != inventory.end(), "Missing encoded-frame source evidence");
    check(source.frames() == record->complete_pcm_frames, std::string(id) + " source-derived complete-frame length");

    return source;
}

void apply(audio::Engine& engine, audio::Command command) {
    engine.apply(std::span(&command, 1));
}

struct Case {
    const char* name;
    audio::Stream stream;
    audio::Sample sample;
};

constexpr std::array cases{
    Case{"airplane", audio::Stream::none, audio::Sample::airplane_motor},
    Case{"airplane-failure", audio::Stream::none, audio::Sample::airplane_failure},
    Case{"title", audio::Stream::title, audio::Sample::shot},
    Case{"highscore", audio::Stream::highscore, audio::Sample::shot},
    Case{"birds", audio::Stream::birds, audio::Sample::shot},
};

std::string_view asset_id(const Case& item) {
    return item.stream == audio::Stream::none ? audio::asset_id(item.sample) : audio::asset_id(item.stream);
}

void start(audio::Engine& engine, const Case& item) {
    if (item.stream != audio::Stream::none) {
        apply(engine, {1, audio::Action::select_stream, item.stream});
        apply(engine, {2, audio::Action::set_stream_gain});
    } else {
        apply(engine, {1, audio::Action::start_loop, audio::Stream::none, item.sample});
    }
}

// Reference source and mixer outcomes: stereo holds the current sample;
// mono floors the adjacent-sample mean, including the frame-zero tail guard,
// with the same integer formula as production. The MMX side gain then floors
// sample*byte/256 in floating point before normalized output.
std::vector<float> reference(const Source& source, std::size_t first, std::size_t frames, bool loop, bool stream) {
    std::vector<float> pcm(frames * 2);
    const std::size_t repeat = source.rate == 22050 ? 2 : 1;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        const auto position = first + frame;

        if (!loop && position >= source.mix_frames()) {
            break;
        }

        const auto source_frame = (position / repeat) % source.frames();

        for (std::size_t channel = 0; channel < 2; ++channel) {
            const auto source_channel = source.channels == 1 ? 0 : channel;
            int value = source.pcm[source_frame * static_cast<std::size_t>(source.channels) + source_channel];

            if (source.channels == 1 && repeat == 2 && position % 2 != 0) {
                const int sum = value + source.pcm[(source_frame + 1) % source.frames()];
                value = sum / 2 - (sum < 0 && sum % 2 != 0);
            }

            const int side_byte = stream ? 255 : 127 + static_cast<int>(channel);
            const auto scaled = static_cast<int>(std::floor(static_cast<double>(value) * side_byte / 256.0));
            pcm[frame * 2 + channel] = static_cast<float>(scaled) * (1.0F / 32768.0F);
        }
    }

    return pcm;
}

// SDL performs only the final output conversion. Its input is independently
// constructed 44100-Hz mixed PCM, so this cannot hide a production source-rate
// conversion, gain or seam error.
class OutputReference {
public:
    OutputReference(const Source& source, int rate, bool loop, bool stream) : source_(source), loop_(loop), stream_(stream) {
        const SDL_AudioSpec input{SDL_AUDIO_F32, 2, original_mix_rate};
        const SDL_AudioSpec output{SDL_AUDIO_F32, 2, rate};
        output_.reset(SDL_CreateAudioStream(&input, &output));
        check_sdl(output_ != nullptr, "Create independent final-rate conversion");
    }

    std::vector<float> render(std::size_t frames) {
        const auto bytes = static_cast<int>(frames * 2 * sizeof(float));
        int available = SDL_GetAudioStreamAvailable(output_.get());
        check_sdl(available >= 0, "Query independent converted PCM");

        while (available < bytes) {
            constexpr std::size_t block_frames = 4096;
            const auto block = reference(source_, input_position_, block_frames, loop_, stream_);
            check_sdl(SDL_PutAudioStreamData(output_.get(), block.data(), static_cast<int>(block.size() * sizeof(float))), "Feed independent 44100-Hz PCM");
            input_position_ += block_frames;
            available = SDL_GetAudioStreamAvailable(output_.get());
            check_sdl(available >= 0, "Query independent converted PCM");
        }

        std::vector<float> pcm(frames * 2);
        check_sdl(SDL_GetAudioStreamData(output_.get(), pcm.data(), bytes) == bytes, "Read exact independent output frame count");

        return pcm;
    }

private:
    const Source& source_;
    bool loop_{}, stream_{};
    std::size_t input_position_{};
    std::unique_ptr<SDL_AudioStream, StreamDeleter> output_;
};

void matching_pcm(std::span<const float> actual, std::span<const float> expected, const std::string& name, std::size_t first, int rate) {
    check(actual.size() == expected.size(), name + " exact output frame count");

    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (!std::isfinite(actual[index]) || actual[index] != expected[index]) {
            throw std::runtime_error(name + " exact " + std::to_string(rate) + "-Hz PCM at frame " + std::to_string(first + index / 2) + " channel " + std::to_string(index % 2));
        }
    }
}

void loop(const assets::AssetStore& store, const Case& item, std::span<const EncodedSource> inventory) {
    const auto source = decode(store, asset_id(item), inventory);
    const auto total_mix_frames = source.mix_frames() * 2 + 4096;

    for (const int rate : {original_mix_rate, 22050, 48000}) {
        audio::Engine engine(store, audio::OutputMode::offline, rate);
        check(engine.mix_sample_rate() == rate, "Diagnostic output rate changed");
        start(engine, item);
        OutputReference converted(source, rate, true, item.stream != audio::Stream::none);
        const auto total = total_mix_frames * static_cast<std::size_t>(rate) / original_mix_rate;
        std::size_t rendered{}, request{};
        std::vector<float> actual;

        while (rendered < total) {
            const auto count = std::min(requests[request++ % requests.size()], total - rendered);
            actual.resize(count * 2);
            std::ranges::fill(actual, std::numeric_limits<float>::quiet_NaN());
            engine.render_frames(actual);
            const auto expected = rate == original_mix_rate ? reference(source, rendered, count, true, item.stream != audio::Stream::none) : converted.render(count);
            matching_pcm(actual, expected, item.name, rendered, rate);
            rendered += count;
        }

        check(engine.state().frames_rendered == total, "Rendered-frame state uses the wrong output rate");

        if (item.stream != audio::Stream::none && rate == original_mix_rate) {
            const auto expected_position = (total / static_cast<std::size_t>(original_mix_rate / source.rate)) % source.frames();
            check(engine.state().stream_position == static_cast<std::int64_t>(expected_position), "Music playback position is not in native source frames");
        }

        std::cout << item.name << ": source_frames=" << source.frames() << " original_mix_frames=" << source.mix_frames() << " output_rate=" << rate << " checked_frames=" << total
                  << " exact repeated PCM through two wraps\n";
    }
}

void mono_one_shot(const assets::AssetStore& store, std::span<const EncodedSource> inventory) {
    check(audio::asset_id(audio::Sample::chicken_hit) == "sfx/chickenhit2", "22050-Hz mono sample mapping changed");
    const auto source = decode(store, audio::asset_id(audio::Sample::chicken_hit), inventory);
    check(source.channels == 1 && source.rate == 22050, "Mono interpolation regression uses the wrong source format");
    audio::Engine engine(store, audio::OutputMode::offline, original_mix_rate);
    apply(engine, {1, audio::Action::play_sample, audio::Stream::none, audio::Sample::chicken_hit});
    std::size_t rendered{}, request{};
    const auto end = source.mix_frames();

    while (rendered < end) {
        const auto count = std::min(requests[request++ % requests.size()], end - rendered);
        matching_pcm(engine.render_frames(count), reference(source, rendered, count, false, false), "chickenhit2", rendered, original_mix_rate);
        rendered += count;
    }

    const auto tail = engine.render_frames(1);
    check(engine.state().active_samples == 0 && tail == std::vector<float>(2), "Mono one-shot added an EOF frame or repeated after its guarded final midpoint");
    std::cout << "chickenhit2: " << source.frames() << " native mono frames, " << end << " original mixer frames; exact midpoint and one-shot EOF\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Usage: moorhuhn2_audio_pcm_tests ASSETS");
        const MixerLifetime lifetime;
        const auto store = assets::AssetStore::load_directory(argv[1]);

        for (const auto& item : cases) {
            loop(store, item, encoded_sources);
        }

        mono_one_shot(store, encoded_sources);

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "audio PCM: " << error.what() << '\n';

        return 1;
    }
}
