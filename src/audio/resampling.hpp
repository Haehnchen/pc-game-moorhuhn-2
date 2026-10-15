#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace moorhuhn2::audio {

inline constexpr int original_mix_rate = 44100;

// Prepare the original 16-bit source PCM for the game's 44100-Hz mixer.
// MH2's sources use 22050/44100 Hz and one/two interleaved channels.
// Stereo repeats the current frame; mono interpolates with signed floor rounding.
// The mono neighbor beyond the last frame is the first frame, even for one-shots.
[[nodiscard]] std::vector<std::int16_t> resample_pcm(std::span<const std::int16_t> input, int channels, int source_rate);

} // namespace moorhuhn2::audio
