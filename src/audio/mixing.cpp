#include "audio/mixing.hpp"

#include <algorithm>
#include <cassert>

namespace moorhuhn2::audio {
namespace {

int scale_sample(std::int16_t sample, std::uint8_t gain) noexcept {
    const auto product = static_cast<std::int32_t>(sample) * gain;
    // PMULHW and its sign correction round down, including negative fractions.
    return product >> 8;
}

} // namespace

void mix_pcm(std::span<PcmVoice> voices, std::span<std::int16_t> output) noexcept {
    assert(output.size() % 2 == 0);
    std::ranges::fill(output, 0);

    for (auto& voice : voices) {
        if (!voice.playing) {
            continue;
        }

        assert(voice.channels == 1 || voice.channels == 2);
        const auto channels = static_cast<std::size_t>(voice.channels);
        assert(!voice.pcm.empty() && voice.pcm.size() % channels == 0);
        const auto period = voice.pcm.size() / channels;
        assert(voice.position < period);

        for (std::size_t frame = 0; frame < output.size() / 2; ++frame) {
            const auto source = voice.position * channels;
            const auto left = scale_sample(voice.pcm[source], voice.left_gain);
            const auto right = scale_sample(voice.pcm[source + channels - 1], voice.right_gain);
            output[frame * 2] = static_cast<std::int16_t>(std::clamp(output[frame * 2] + left, -32768, 32767));
            output[frame * 2 + 1] = static_cast<std::int16_t>(std::clamp(output[frame * 2 + 1] + right, -32768, 32767));

            if (++voice.position == period) {
                voice.position = 0;

                if (!voice.loop) {
                    voice.playing = false;
                    break;
                }
            }
        }
    }
}

} // namespace moorhuhn2::audio
