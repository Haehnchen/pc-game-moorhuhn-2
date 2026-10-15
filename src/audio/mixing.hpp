#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace moorhuhn2::audio {

struct PcmVoice {
    std::span<const std::int16_t> pcm;
    int channels{};
    std::size_t position{};
    std::uint8_t left_gain{}, right_gain{};
    bool loop{}, playing{};
};

// Prepared 44100-Hz PCM16, in original software-slot order. Each active voice
// has complete mono/stereo frames and a position inside its nonempty period.
// Output is interleaved stereo; each voice addition saturates independently.
void mix_pcm(std::span<PcmVoice> voices, std::span<std::int16_t> output) noexcept;

} // namespace moorhuhn2::audio
