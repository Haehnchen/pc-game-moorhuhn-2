#include "audio/resampling.hpp"

#include <stdexcept>

namespace moorhuhn2::audio {

std::vector<std::int16_t> resample_pcm(std::span<const std::int16_t> input, int channels, int source_rate) {
    if ((channels != 1 && channels != 2) || (source_rate != 22050 && source_rate != original_mix_rate) || input.size() % static_cast<std::size_t>(channels) != 0) {
        throw std::invalid_argument("unsupported original PCM format");
    }

    if (source_rate == original_mix_rate) {
        return {input.begin(), input.end()};
    }

    if (input.size() > std::vector<std::int16_t>().max_size() / 2) {
        throw std::length_error("original PCM conversion is too large");
    }

    std::vector<std::int16_t> output(input.size() * 2);

    if (channels == 2) {
        for (std::size_t frame = 0; frame < input.size() / 2; ++frame) {
            output[frame * 4] = output[frame * 4 + 2] = input[frame * 2];
            output[frame * 4 + 1] = output[frame * 4 + 3] = input[frame * 2 + 1];
        }
    } else {
        for (std::size_t frame = 0; frame < input.size(); ++frame) {
            const auto current = input[frame];
            const auto next = input[frame + 1 == input.size() ? 0 : frame + 1];
            const auto sum = static_cast<std::int32_t>(current) + next;
            // MMX PSRAD rounds negative odd sums down, unlike signed division.
            const auto midpoint = sum >> 1;
            output[frame * 2] = current;
            output[frame * 2 + 1] = static_cast<std::int16_t>(midpoint);
        }
    }

    return output;
}

} // namespace moorhuhn2::audio
