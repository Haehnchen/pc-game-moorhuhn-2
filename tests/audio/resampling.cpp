#include "audio/resampling.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct ResamplingCase {
    std::string id{};
    int source_rate{};
    int channels{};
    std::vector<std::int16_t> source_pcm16{};
    std::vector<std::int16_t> expected_pcm16{};
};

const std::array<ResamplingCase, 13> resampling_cases = {{{"empty_mono_1x", 44100, 1, {}, {}}, {"empty_stereo_2x", 22050, 2, {}, {}},
    {"mono_1x_signed_extrema", 44100, 1, {-32768, -32767, -2, -1, 0, 1, 2, 32767}, {-32768, -32767, -2, -1, 0, 1, 2, 32767}}, {"mono_1x_one_frame", 44100, 1, {-1}, {-1}},
    {"mono_2x_one_frame_min", 22050, 1, {-32768}, {-32768, -32768}}, {"mono_2x_one_frame_max", 22050, 1, {32767}, {32767, 32767}},
    {"mono_2x_odd_ramp_tail_first", 22050, 1, {0, 1, 2, 3, 4}, {0, 0, 1, 1, 2, 2, 3, 3, 4, 2}},
    {"mono_2x_negative_odd_floor", 22050, 1, {-3, -2, -1, 0, 1}, {-3, -3, -2, -2, -1, -1, 0, 0, 1, -1}},
    {"mono_2x_extrema_tail_first", 22050, 1, {-32768, 32767, 0, -1, 1}, {-32768, -1, 32767, 16383, 0, -1, -1, 0, 1, -16384}},
    {"mono_2x_even_tail_first", 22050, 1, {9, -4}, {9, 2, -4, 2}}, {"stereo_1x_odd_interleaved", 44100, 2, {-32768, 32767, -1, 1, 17, -18}, {-32768, 32767, -1, 1, 17, -18}},
    {"stereo_2x_one_frame", 22050, 2, {-32768, 32767}, {-32768, 32767, -32768, 32767}},
    {"stereo_2x_odd_interleaved", 22050, 2, {-32768, 32767, -1, 1, 1, -1, 32767, -32768, 17, -18},
        {-32768, 32767, -32768, 32767, -1, 1, -1, 1, 1, -1, 1, -1, 32767, -32768, 32767, -32768, 17, -18, 17, -18}}}};

} // namespace

int main() {
    try {
        for (const auto& item : resampling_cases) {
            const auto actual = moorhuhn2::audio::resample_pcm(item.source_pcm16, item.channels, item.source_rate);

            if (actual != item.expected_pcm16) {
                throw std::runtime_error("original PCM vector differs: " + item.id);
            }
        }

        const std::array<std::int16_t, 3> incomplete_frame{1, -2, 3};

        for (const auto [channels, rate] : std::array<std::array<int, 2>, 6>{{{0, 44100}, {-1, 44100}, {3, 44100}, {1, 48000}, {2, 44100}, {2, 22050}}}) {
            bool rejected = false;
            try {
                static_cast<void>(moorhuhn2::audio::resample_pcm(incomplete_frame, channels, rate));
            } catch (const std::invalid_argument&) {
                rejected = true;
            }

            if (!rejected) {
                throw std::runtime_error("unsupported or incomplete PCM format accepted");
            }
        }

        std::cout << resampling_cases.size() << " original PCM vectors and invalid formats passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
