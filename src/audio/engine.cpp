#include "audio/engine.hpp"

#include "audio/mixing.hpp"
#include "audio/resampling.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace moorhuhn2::audio {
namespace {

void require(bool result, std::string_view operation) {
    if (!result) {
        throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
}

struct AudioDeviceLifetime {
    bool initialized{};

    ~AudioDeviceLifetime() {
        if (initialized) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
    }
};

struct MixerLifetime {
    MixerLifetime() {
        require(MIX_Init(), "initialize SDL_mixer");
    }

    ~MixerLifetime() {
        MIX_Quit();
    }
};

struct MixerDeleter {
    void operator()(MIX_Mixer* value) const {
        MIX_DestroyMixer(value);
    }
};

struct DecoderDeleter {
    void operator()(MIX_AudioDecoder* value) const {
        MIX_DestroyAudioDecoder(value);
    }
};

struct AudioStreamDeleter {
    void operator()(SDL_AudioStream* value) const {
        SDL_DestroyAudioStream(value);
    }
};

using Mixer = std::unique_ptr<MIX_Mixer, MixerDeleter>;
using Decoder = std::unique_ptr<MIX_AudioDecoder, DecoderDeleter>;
using AudioStream = std::unique_ptr<SDL_AudioStream, AudioStreamDeleter>;

struct PreparedAudio {
    std::vector<std::int16_t> pcm;
    int channels{};
};

std::vector<std::int16_t> decode_pcm(const assets::AudioView& source, SDL_AudioSpec& format) {
    auto* io = SDL_IOFromConstMem(source.encoded.data(), source.encoded.size());
    require(io != nullptr, "open encoded audio");
    const Decoder decoder(MIX_CreateAudioDecoder_IO(io, true, 0));
    require(decoder != nullptr, "create audio decoder");
    require(MIX_GetAudioDecoderFormat(decoder.get(), &format), "query source audio format");
    require(format.freq == static_cast<int>(source.sample_rate) && format.channels == static_cast<int>(source.channels), "verify source audio format");
    format.format = SDL_AUDIO_S16;
    std::vector<std::int16_t> pcm;
    std::array<std::int16_t, 8192> buffer{};

    while (true) {
        const auto bytes = MIX_DecodeAudio(decoder.get(), buffer.data(), sizeof(buffer), &format);
        require(bytes >= 0 && bytes % SDL_AUDIO_FRAMESIZE(format) == 0, "decode complete source PCM frames");

        if (bytes == 0) {
            break;
        }

        pcm.insert(pcm.end(), buffer.begin(), buffer.begin() + bytes / sizeof(std::int16_t));
        require(pcm.size() / static_cast<std::size_t>(format.channels) <= source.decoded_frames, "bound decoded source PCM");
    }

    require(!pcm.empty(), "decode nonempty source PCM");

    return pcm;
}

struct MixerLock {
    MIX_Mixer* mixer;

    explicit MixerLock(MIX_Mixer* value) : mixer(value) {
        MIX_LockMixer(mixer);
    }

    ~MixerLock() {
        MIX_UnlockMixer(mixer);
    }
};

void validate_loop_sample(Sample sample) {
    if (sample != Sample::airplane_motor && sample != Sample::airplane_failure) {
        throw std::invalid_argument("audio sample does not support retained looping");
    }
}

int gain_volume(float gain) {
    if (!std::isfinite(gain)) {
        throw std::invalid_argument("audio gain must be finite");
    }

    return static_cast<int>(static_cast<double>(std::clamp(gain, 0.0F, 1.0F)) * 255.0);
}

} // namespace

std::string_view asset_id(Stream stream) {
    switch (stream) {
        case Stream::none:
            return {};
        case Stream::title:
            return "sfx/Moorhuhn2_Title_Music";
        case Stream::birds:
            return "sfx/sfx_background_birds_STEREO_LOOP";
        case Stream::highscore:
            return "sfx/Moorhuhn2_Highscore_Music";
    }

    throw std::invalid_argument("unknown audio stream");
}

std::string_view asset_id(Sample sample) {
    switch (sample) {
        case Sample::shot:
            return "sfx/sfx_shotgun_blast";
        case Sample::empty_fire:
            return "sfx/sfx_empty_magazine";
        case Sample::reload:
            return "sfx/sfx_gun_reload";
        case Sample::get_ready:
            return "sfx/sfx_get_ready_after_typing_name";
        case Sample::game_over:
            return "sfx/sfx_game_over";
        case Sample::time_running_out:
            return "sfx/sfx_time_running_out";
        case Sample::chicken_hit:
            return "sfx/chickenhit2";
        case Sample::chicken_far:
            return "sfx/sfx_chicken_shot_FAR";
        case Sample::chicken_mid:
            return "sfx/sfx_chicken_shot_MID";
        case Sample::chicken_close:
            return "sfx/sfx_chicken_shot_CLOSE";
        case Sample::typing_click:
            return "sfx/sfx_typename_click";
        case Sample::boat_impact:
            return "sfx/sfx_tree_shot";
        case Sample::stone_impact:
            return "sfx/sfx_stone_shot";
        case Sample::magic_balloons:
            return "sfx/sfx_magic_balloons_appear";
        case Sample::balloon_hit:
            return "sfx/sfx_balloon_shot";
        case Sample::boat_sinking:
            return "sfx/sfx_boat_sinking";
        case Sample::rare_balloon:
            return "sfx/sfx_50_point_score";
        case Sample::church_bell:
            return "sfx/sfx_churchbell";
        case Sample::church_bell_second:
            return "sfx/sfx_churchbell_2";
        case Sample::house_hit:
            return "sfx/sfx_house_shot";
        case Sample::frog_croak:
            return "sfx/sfx_frog";
        case Sample::frog_hit:
            return "sfx/sfx_frog_shot";
        case Sample::spider_swing:
            return "sfx/sfx_spider_swings";
        case Sample::tree_break:
            return "sfx/sfx_tree_breaks_down";
        case Sample::tree_highfly:
            return "sfx/chickenhit2";
        case Sample::branch_ground:
            return "sfx/sfx_treebranch_hits_ground";
        case Sample::scarecrow_hit:
            return "sfx/sfx_scarecrow_shot";
        case Sample::hunting_stand_ladder:
            return "sfx/sfx_guardhouse_ladder_shot";
        case Sample::hunting_stand_broken:
            return "sfx/sfx_guardhouse_breaks_down";
        case Sample::big_chicken_appear:
            return "sfx/sfx_big_chicken_pops_up";
        case Sample::airplane_motor:
            return "sfx/sfx_airplane_loop";
        case Sample::airplane_failure:
            return "sfx/sfx_airplane_motor_failure_loop";
        case Sample::airplane_hit:
            return "sfx/sfx_airplane_shot";
        case Sample::airplane_crash:
            return "sfx/sfx_airplane_crash";
        case Sample::blimp_hit:
            return "sfx/sfx_zeppelin_shot";
    }

    throw std::invalid_argument("unknown audio sample");
}

int quantize_pan(float pan) {
    if (!std::isfinite(pan)) {
        throw std::invalid_argument("nonfinite audio pan");
    }

    // FModPlugin's channel +0x2c uses trunc(pan*128+128); FMOD clamps to 255.
    // The volatile binary64 store rounds to 53 bits before truncation, even
    // under x87 excess precision; pans just below center depend on that tie.
    static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);
    const volatile double value = static_cast<double>(std::clamp(pan, -1.0F, 1.0F)) * 128 + 128;

    return std::clamp(static_cast<int>(value), 0, 255);
}

float quantize_gain(float gain) {
    // Plugin volume getter: integer volume scaled back to the public gain.
    return static_cast<float>(gain_volume(gain)) / 255.0F;
}

struct Engine::Impl {
    enum class Kind {
        sample,
        loop,
        stream
    };

    struct Channel {
        Kind kind{Kind::sample};
        bool allocated{}, reserved{};
        int volume{255}, pan{128};
    };

    AudioDeviceLifetime device_lifetime;
    MixerLifetime lifetime;
    Mixer mixer;
    std::map<std::string, PreparedAudio, std::less<>> audio;
    std::vector<AudioInfo> inventory;
    std::array<Channel, 32> channels;
    std::array<PcmVoice, 32> voices;
    std::size_t next_channel{};
    std::optional<std::size_t> stream_channel;
    std::map<Sample, std::optional<std::size_t>> loop_channels;
    OutputStatus output{OutputStatus::offline};
    std::string device_error;
    int mix_rate{};
    State state;
    AudioStream output_stream;
    AudioStream device_stream;

    Impl(const assets::AssetStore& assets, OutputMode mode, int rate, const LoadObserver& observer) : mix_rate(rate) {
        if (rate < 8000 || rate > 192000) {
            throw std::invalid_argument("mix sample rate must be between 8000 and 192000");
        }

        const SDL_AudioSpec spec{SDL_AUDIO_F32, 2, original_mix_rate};

        mixer.reset(MIX_CreateMixer(&spec));
        require(mixer != nullptr, "create fixed-format mixer");

        if (mode == OutputMode::device) {
            device_lifetime.initialized = SDL_InitSubSystem(SDL_INIT_AUDIO);
            if (device_lifetime.initialized) {
                device_stream.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed_device, this));
            }
            if (device_stream) {
                output = OutputStatus::device;
                mix_rate = original_mix_rate;
            } else {
                device_error = SDL_GetError();
                output = OutputStatus::unavailable_device;
            }
        }

        if (!device_stream && rate != original_mix_rate) {
            const SDL_AudioSpec destination{SDL_AUDIO_F32, 2, rate};
            output_stream.reset(SDL_CreateAudioStream(&spec, &destination));
            require(output_stream != nullptr, "create final output conversion");
        }

        std::vector<std::string_view> order;

        for (const auto id : observer.order) {
            if (std::ranges::find(assets.audio_ids(), id) == assets.audio_ids().end() || std::ranges::find(order, id) != order.end()) {
                throw std::invalid_argument("Audio load order contains an unknown or duplicate asset");
            }

            order.push_back(id);
        }

        for (const auto& id : assets.audio_ids()) {
            if (std::ranges::find(order, id) == order.end()) {
                order.push_back(id);
            }
        }

        for (const auto asset : order) {
            const std::string id(asset);
            const auto source = assets.audio(id);
            SDL_AudioSpec format{};
            const auto source_pcm = decode_pcm(source, format);
            const auto frames = source_pcm.size() / static_cast<std::size_t>(format.channels);
            inventory.push_back({id, format.freq, format.channels, static_cast<std::int64_t>(frames)});
            auto prepared = resample_pcm(source_pcm, format.channels, format.freq);
            audio.emplace(id, PreparedAudio{std::move(prepared), format.channels});

            if (observer.loaded) {
                observer.loaded(id);
            }
        }

        std::ranges::sort(inventory, {}, &AudioInfo::id);
        require(MIX_SetPostMixCallback(mixer.get(), mix_output, this), "set original integer mixer");
        if (device_stream) {
            require(SDL_ResumeAudioStreamDevice(device_stream.get()), "resume audio device");
        }
    }

    ~Impl() {
        device_stream.reset();
        // The setter takes the mixer lock and waits for any active callback.
        // Detach before the borrowed voice PCM and state are destroyed.
        MIX_SetPostMixCallback(mixer.get(), nullptr, nullptr);
    }

    static void SDLCALL feed_device(void* userdata, SDL_AudioStream* stream, int additional_bytes, int) {
        auto& self = *static_cast<Impl*>(userdata);
        std::array<float, 4096> buffer{};
        while (additional_bytes > 0) {
            const auto bytes = std::min(additional_bytes, static_cast<int>(sizeof(buffer)));
            if (MIX_Generate(self.mixer.get(), buffer.data(), bytes) < 0 || !SDL_PutAudioStreamData(stream, buffer.data(), bytes)) {
                return;
            }
            additional_bytes -= bytes;
        }
    }

    static void SDLCALL mix_output(void* userdata, MIX_Mixer*, const SDL_AudioSpec*, float* pcm, int samples) {
        auto& self = *static_cast<Impl*>(userdata);
        const std::span output(pcm, static_cast<std::size_t>(samples));

        if (self.state.paused) {
            std::ranges::fill(output, 0);
            return;
        }

        std::array<std::int16_t, 4096> buffer{};

        for (std::size_t offset = 0; offset < output.size();) {
            const auto count = std::min(buffer.size(), output.size() - offset);
            const auto block = std::span(buffer).first(count);
            mix_pcm(self.voices, block);

            for (std::size_t index = 0; index < count; ++index) {
                output[offset + index] = static_cast<float>(block[index]) * (1.0F / 32768.0F);
            }

            offset += count;
        }
    }

    void prepare_output(int bytes) {
        std::array<float, 4096> buffer{};

        while (true) {
            const auto available = SDL_GetAudioStreamAvailable(output_stream.get());
            require(available >= 0, "query final output conversion");

            if (available >= bytes) {
                return;
            }

            // Availability includes the resampler's lookahead. Feed until the
            // actual request is ready, keeping history across calls and loops.
            const auto missing_frames = (bytes - available) / (2 * sizeof(float));
            const auto source_frames = (missing_frames * original_mix_rate + mix_rate - 1) / mix_rate;
            const auto frames = std::clamp<std::size_t>(source_frames, 1, buffer.size() / 2);
            const auto source_bytes = static_cast<int>(frames * 2 * sizeof(float));
            require(MIX_Generate(mixer.get(), buffer.data(), source_bytes) >= 0, "generate original-rate PCM");
            require(SDL_PutAudioStreamData(output_stream.get(), buffer.data(), source_bytes), "feed final output conversion");
        }
    }

    bool active(std::size_t index) const {
        return voices[index].playing;
    }

    void stop(std::size_t index) {
        voices[index].playing = false;
        voices[index].position = 0;
    }

    std::size_t allocate(Kind kind) {
        std::optional<std::size_t> selected;
        // FMOD searches circularly for a free unreserved channel. All game
        // samples have priority 255, so the first busy candidate wins a tie.
        for (std::size_t offset = 0; offset < channels.size(); ++offset) {
            const auto index = (next_channel + offset) % channels.size();
            const auto& channel = channels[index];

            if (channel.reserved) {
                continue;
            }

            if (!selected) {
                selected = index;
            }

            if (!active(index)) {
                selected = index;
                break;
            }
        }

        if (!selected) {
            throw std::runtime_error("all original audio channels are reserved");
        }

        auto& channel = channels[*selected];

        if (active(*selected)) {
            ++state.channels_stolen;
        }

        stop(*selected);
        channel.allocated = true;
        channel.kind = kind;
        channel.reserved = kind == Kind::stream;
        next_channel = (*selected + 1) % channels.size();
        state.last_channel = static_cast<int>(*selected);

        return *selected;
    }

    const PreparedAudio& find(std::string_view id) const {
        const auto found = audio.find(id);

        if (found == audio.end()) {
            throw std::runtime_error("missing mapped audio asset: " + std::string(id));
        }

        return found->second;
    }

    static int default_pan(const Channel& channel) {
        // Defaults come from plugin load flags, not decoded channel count:
        // samples use 0x2b (even stereo game_over), streams use 0x4042.
        return channel.kind == Kind::stream ? -1 : 128;
    }

    void update_mix(std::size_t index) {
        const auto& channel = channels[index];
        // FMOD first truncates the two volume/pan products to channel bytes.
        // Music streams default to the -1 sentinel, preserving both channels.
        const int left = channel.pan == -1 ? channel.volume : channel.volume * (255 - channel.pan) / 255;
        const int right = channel.pan == -1 ? channel.volume : channel.volume * channel.pan / 255;
        voices[index].left_gain = static_cast<std::uint8_t>(left);
        voices[index].right_gain = static_cast<std::uint8_t>(right);
    }

    void assign(std::size_t index, std::string_view id, float gain, std::optional<float> pan) {
        auto& channel = channels[index];
        const auto& source = find(id);
        voices[index] = {source.pcm, source.channels, 0, 0, 0, channel.kind != Kind::sample, true};
        channel.volume = gain_volume(gain);
        channel.pan = pan ? quantize_pan(*pan) : default_pan(channel);
        update_mix(index);
    }

    void set_gain(std::size_t index, float gain) {
        auto& channel = channels[index];
        channel.volume = gain_volume(gain);
        update_mix(index);
    }

    void select(Stream stream) {
        if (stream == state.stream) {
            return;
        }

        if (stream_channel) {
            stop(*stream_channel);
            channels[*stream_channel].reserved = false;
            stream_channel.reset();
        }

        state.stream = Stream::none;
        state.stream_gain = 0;

        if (stream == Stream::none) {
            return;
        }

        const auto index = allocate(Kind::stream);
        assign(index, asset_id(stream), 0, std::nullopt);
        stream_channel = index;
        state.stream = stream;
        ++state.streams_started;
    }

    void play(Sample sample, float gain, std::optional<float> pan) {
        const auto index = allocate(Kind::sample);
        assign(index, asset_id(sample), gain, pan);
        ++state.samples_started;
    }

    void start_loop(Sample sample, float gain, std::optional<float> pan) {
        const auto found = loop_channels.find(sample);

        if (found != loop_channels.end() && found->second) {
            stop(*found->second);
        }

        const auto index = allocate(Kind::loop);
        loop_channels[sample] = index;
        assign(index, asset_id(sample), gain, pan);
        ++state.loops_started;
    }

    void set_loop_pan(Sample sample, std::optional<float> pan) {
        const auto found = loop_channels.find(sample);

        if (found != loop_channels.end() && found->second) {
            auto& channel = channels[*found->second];
            channel.pan = pan ? quantize_pan(*pan) : default_pan(channel);
            update_mix(*found->second);
        }
    }

    void set_loop_gain(Sample sample, float gain) {
        const auto found = loop_channels.find(sample);

        if (found != loop_channels.end() && found->second) {
            set_gain(*found->second, gain);
        }
    }

    void stop_loop(Sample sample) {
        const auto found = loop_channels.find(sample);

        if (found != loop_channels.end() && found->second) {
            // The plugin discards FMOD's generation, so this can stop a stolen
            // occupant. The EXE clears its retained pointer after the stop.
            stop(*found->second);
            found->second.reset();
        }
    }

    void validate_command(const Command& command) const {
        switch (command.action) {
            case Action::select_stream:
                if (command.stream != Stream::none) {
                    static_cast<void>(find(asset_id(command.stream)));
                }

                break;
            case Action::set_stream_gain:
                static_cast<void>(quantize_gain(command.gain));
                break;
            case Action::play_sample:
                static_cast<void>(find(asset_id(command.sample)));
                static_cast<void>(quantize_gain(command.gain));

                if (command.pan) {
                    static_cast<void>(quantize_pan(*command.pan));
                }

                break;
            case Action::start_loop:
                validate_loop_sample(command.sample);
                static_cast<void>(find(asset_id(command.sample)));
                static_cast<void>(quantize_gain(command.gain));

                if (command.pan) {
                    static_cast<void>(quantize_pan(*command.pan));
                }

                break;
            case Action::set_loop_pan:
                validate_loop_sample(command.sample);

                if (command.pan) {
                    static_cast<void>(quantize_pan(*command.pan));
                }

                break;
            case Action::stop_loop:
                validate_loop_sample(command.sample);
                break;
            case Action::set_loop_gain:
                validate_loop_sample(command.sample);
                static_cast<void>(quantize_gain(command.gain));
                break;
            case Action::stop_stream:
            case Action::stop_samples:
            case Action::pause:
            case Action::resume:
                break;
            default:
                throw std::invalid_argument("unknown audio action");
        }
    }

    void apply_command(const Command& command) {
        switch (command.action) {
            case Action::select_stream:
                select(command.stream);
                break;
            case Action::set_stream_gain:
                if (stream_channel) {
                    set_gain(*stream_channel, command.gain);
                }

                state.stream_gain = quantize_gain(command.gain);
                break;
            case Action::stop_stream:
                select(Stream::none);
                break;
            case Action::play_sample:
                play(command.sample, command.gain, command.pan);
                break;
            case Action::start_loop:
                start_loop(command.sample, command.gain, command.pan);
                break;
            case Action::set_loop_pan:
                set_loop_pan(command.sample, command.pan);
                break;
            case Action::stop_loop:
                stop_loop(command.sample);
                break;
            case Action::set_loop_gain:
                set_loop_gain(command.sample, command.gain);
                break;
            case Action::stop_samples:
                for (std::size_t index = 0; index < channels.size(); ++index) {
                    if (channels[index].kind != Kind::stream) {
                        stop(index);
                    }
                }

                for (auto& channel : std::views::values(loop_channels)) {
                    channel.reset();
                }

                break;
            case Action::pause:
                state.paused = true;
                break;
            case Action::resume:
                state.paused = false;
                break;
        }
        state.last_sequence = command.sequence;
        ++state.commands_consumed;
    }
};

Engine::Engine(const assets::AssetStore& assets, OutputMode mode, int mix_sample_rate, const LoadObserver& observer)
    : impl_(std::make_unique<Impl>(assets, mode, mix_sample_rate, observer)) {}

Engine::~Engine() = default;
Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;

void Engine::apply(std::span<const Command> commands) {
    auto previous = impl_->state.last_sequence;

    // Validate the batch before consuming any one-shot or stealing a channel.
    for (const auto& command : commands) {
        if (command.sequence <= impl_->state.last_sequence) {
            continue;
        }

        if (command.sequence < previous) {
            throw std::invalid_argument("audio command sequence must increase within a batch");
        }

        if (command.sequence == previous) {
            continue;
        }

        previous = command.sequence;
        impl_->validate_command(command);
    }

    MixerLock lock(impl_->mixer.get());

    for (const auto& command : commands) {
        if (command.sequence > impl_->state.last_sequence) {
            impl_->apply_command(command);
        }
    }
}

std::vector<float> Engine::render_frames(std::size_t frames) {
    if (!needs_pumping()) {
        throw std::logic_error("device mixer is pumped by SDL");
    }

    if (frames > static_cast<std::size_t>(std::numeric_limits<int>::max()) / (2 * sizeof(float))) {
        throw std::length_error("offline audio request exceeds SDL buffer limit");
    }

    std::vector<float> pcm(frames * 2);
    render_frames(pcm);

    return pcm;
}

void Engine::render_frames(std::span<float> pcm) {
    if (!needs_pumping()) {
        throw std::logic_error("device mixer is pumped by SDL");
    }

    if (pcm.size() % 2 != 0) {
        throw std::invalid_argument("offline PCM must contain complete stereo frames");
    }

    if (pcm.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) / sizeof(float)) {
        throw std::length_error("offline audio request exceeds SDL buffer limit");
    }

    if (!pcm.empty()) {
        const auto bytes = static_cast<int>(pcm.size() * sizeof(float));

        if (impl_->output_stream) {
            impl_->prepare_output(bytes);
            require(SDL_GetAudioStreamData(impl_->output_stream.get(), pcm.data(), bytes) == bytes, "convert complete offline PCM");
        } else {
            // MIX_Generate fills the entire buffer, but its return excludes silence.
            require(MIX_Generate(impl_->mixer.get(), pcm.data(), bytes) >= 0, "generate offline PCM");
        }
    }

    impl_->state.frames_rendered += pcm.size() / 2;
}

OutputStatus Engine::status() const {
    return impl_->output;
}

bool Engine::needs_pumping() const {
    return status() != OutputStatus::device;
}

int Engine::mix_sample_rate() const {
    return impl_->mix_rate;
}

const std::string& Engine::device_error() const {
    return impl_->device_error;
}

State Engine::state() const {
    MixerLock lock(impl_->mixer.get());
    auto state = impl_->state;
    state.loop_tracks = impl_->loop_channels.size();

    for (std::size_t index = 0; index < impl_->channels.size(); ++index) {
        const auto& channel = impl_->channels[index];
        state.channel_tracks += channel.allocated;

        if (impl_->active(index)) {
            ++state.active_channels;
            state.active_samples += channel.kind == Impl::Kind::sample;
            state.active_loops += channel.kind == Impl::Kind::loop;
        }
    }

    state.stream_position = impl_->stream_channel ? static_cast<std::int64_t>(impl_->voices[*impl_->stream_channel].position) : 0;

    if (impl_->stream_channel) {
        // All three original streams are 22050 Hz; prepared playback uses 44100.
        state.stream_position /= 2;
    }

    return state;
}

std::span<const AudioInfo> Engine::inventory() const {
    return impl_->inventory;
}

} // namespace moorhuhn2::audio
