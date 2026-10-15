#include "audio/mixing.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GainCase {
    std::string id{};
    int channels{};
    std::vector<std::int16_t> pcm16{};
    std::array<std::uint8_t, 2> side_bytes{};
    std::vector<int> expected_pcm16{};
};

struct VoiceCase {
    std::size_t slot{};
    int channels{};
    std::vector<std::int16_t> pcm16{};
    std::array<std::uint8_t, 2> side_bytes{};
};

struct AccumulationCase {
    std::string id{};
    std::size_t frames{};
    std::vector<VoiceCase> voices{};
    std::vector<int> expected_voice_order{};
    std::vector<std::vector<int>> expected_after_voice{};
    std::vector<int> expected_pcm16{};
};

const std::array<GainCase, 25> gain_vectors = {{{"zero_gain_signed_extrema", 2, {-32768, 32767}, {0, 0}, {0, 0}}, {"byte1_signed_extrema", 2, {-32768, 32767}, {1, 1}, {-128, 127}},
    {"byte1_negative_fraction", 2, {-257, 257}, {1, 1}, {-2, 1}}, {"byte127_signed_extrema", 2, {-32768, 32767}, {127, 127}, {-16256, 16255}},
    {"byte128_signed_coefficient_transition", 2, {-32768, 32767}, {128, 128}, {-16384, 16383}},
    {"byte129_signed_coefficient_correction", 2, {-32768, 32767}, {129, 129}, {-16512, 16511}}, {"byte254_signed_extrema", 2, {-32768, 32767}, {254, 254}, {-32512, 32511}},
    {"byte255_signed_extrema", 2, {-32768, 32767}, {255, 255}, {-32640, 32639}}, {"positive1_byte255_counterexample", 2, {1, 1}, {255, 255}, {0, 0}},
    {"negative1_byte255_floor", 2, {-1, -1}, {255, 255}, {-1, -1}}, {"byte255_integer_product", 2, {-256, 256}, {255, 255}, {-255, 255}},
    {"byte255_negative_fraction", 2, {-255, 255}, {255, 255}, {-255, 254}}, {"byte255_beyond_exact_product", 2, {-257, 257}, {255, 255}, {-256, 255}},
    {"byte128_odd_fraction", 2, {-3, 3}, {128, 128}, {-2, 1}}, {"independent_side_extreme_bytes", 2, {-32768, 32767}, {255, 1}, {-32640, 127}},
    {"side_zero_preserves_other_side", 2, {32767, -32768}, {0, 255}, {0, -32640}}, {"mono_zero_signed_min", 1, {-32768}, {0, 0}, {0, 0}},
    {"mono_byte1_negative_fraction", 1, {-257}, {1, 255}, {-2, -256}}, {"mono_byte127_128_transition", 1, {-32767}, {127, 128}, {-16256, -16384}},
    {"mono_byte129_254_transition", 1, {32767}, {129, 254}, {16511, 32511}}, {"mono_signed_min", 1, {-32768}, {255, 255}, {-32640, -32640}},
    {"mono_signed_max", 1, {32767}, {255, 255}, {32639, 32639}}, {"mono_positive1_byte255_counterexample", 1, {1}, {255, 255}, {0, 0}},
    {"mono_negative1_floor", 1, {-1}, {1, 255}, {-1, -1}}, {"zero_pcm_all_sign_masks", 2, {0, 0}, {127, 128}, {0, 0}}}};

const std::array<AccumulationCase, 9> accumulation_vectors = {{{"empty_accumulator", 1, {}, {}, {}, {0, 0}},
    {"positive_saturation_three_voices", 1, {{0, 1, {32767}, {255, 255}}, {1, 1, {32767}, {255, 255}}, {2, 1, {32767}, {255, 255}}}, {0, 1, 2},
        {{32639, 32639}, {32767, 32767}, {32767, 32767}}, {32767, 32767}},
    {"negative_saturation_then_positive", 1, {{0, 1, {-32768}, {255, 255}}, {1, 1, {-32768}, {255, 255}}, {2, 1, {32767}, {255, 255}}}, {0, 1, 2},
        {{-32640, -32640}, {-32768, -32768}, {-129, -129}}, {-129, -129}},
    {"ascending_slots_reserved_stereo_middle", 1, {{9, 1, {-32768}, {255, 255}}, {1, 1, {32767}, {255, 255}}, {4, 2, {32767, -32768}, {255, 255}}}, {1, 4, 9},
        {{32639, 32639}, {32767, -1}, {127, -32641}}, {127, -32641}},
    {"negative_first_changes_saturated_result", 1, {{9, 2, {32767, -32768}, {255, 255}}, {1, 1, {-32768}, {255, 255}}, {4, 1, {32767}, {255, 255}}}, {1, 4, 9},
        {{-32640, -32640}, {-1, -1}, {32638, -32641}}, {32638, -32641}},
    {"per_voice_negative_fraction_rounding", 1, {{0, 1, {-1}, {1, 1}}, {31, 2, {-1, -1}, {1, 1}}}, {0, 31}, {{-1, -1}, {-2, -2}}, {-2, -2}},
    {"muted_voice_between_active_extremes", 1, {{31, 1, {32767}, {255, 255}}, {0, 2, {-32768, 32767}, {255, 255}}, {16, 1, {-32768}, {0, 0}}}, {0, 16, 31},
        {{-32640, 32639}, {-32640, 32639}, {-1, 32767}}, {-1, 32767}},
    {"stereo_mono_three_frame_tail_overlap", 3, {{31, 1, {1, 32767, -257}, {255, 254}}, {0, 2, {-1, 1, 32767, -32768, 256, -256}, {255, 128}}}, {0, 31},
        {{-1, 0, 32639, -16384, 255, -128}, {-1, 0, 32767, 16127, -1, -383}}, {-1, 0, 32767, 16127, -1, -383}},
    {"stereo_mono_four_frame_vector_overlap", 4, {{31, 1, {1, 32767, -257, -32768}, {255, 254}}, {0, 2, {-1, 1, 32767, -32768, 256, -256, -32768, 32767}, {255, 128}}}, {0, 31},
        {{-1, 0, 32639, -16384, 255, -128, -32640, 16383}, {-1, 0, 32767, 16127, -1, -383, -32768, -16129}}, {-1, 0, 32767, 16127, -1, -383, -32768, -16129}}}};

namespace audio = moorhuhn2::audio;

void check(bool value, const std::string& message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

void source_vectors() {
    for (const auto& item : gain_vectors) {
        const auto input = item.pcm16;
        const auto gains = item.side_bytes;
        std::array voices{audio::PcmVoice{input, item.channels, 0, gains[0], gains[1], false, true}};
        std::array<std::int16_t, 2> output{};
        audio::mix_pcm(voices, output);
        check(std::ranges::equal(output, item.expected_pcm16), "Original gain vector: " + item.id);
    }

    for (const auto& item : accumulation_vectors) {
        std::array<std::vector<std::int16_t>, 32> inputs;
        std::array<audio::PcmVoice, 32> voices{};

        for (const auto& entry : item.voices) {
            const auto slot = entry.slot;
            const auto gains = entry.side_bytes;
            inputs.at(slot) = entry.pcm16;
            voices.at(slot) = {inputs.at(slot), entry.channels, 0, gains[0], gains[1], true, false};
        }

        std::vector<std::int16_t> output(item.frames * 2, 32767);
        audio::mix_pcm(voices, output);
        std::size_t step{};

        for (const auto& slot : item.expected_voice_order) {
            voices.at(slot).playing = true;
            audio::mix_pcm(voices, output);
            check(std::ranges::equal(output, item.expected_after_voice.at(step++)), "Original accumulation stage: " + item.id);
        }

        check(std::ranges::equal(output, item.expected_pcm16), "Original final accumulation: " + item.id);
    }
}

void playback_boundaries() {
    const std::array<std::int16_t, 2> source{256, -257};
    std::array voices{audio::PcmVoice{source, 1, 0, 128, 255, false, true}};
    std::array<std::int16_t, 6> output{};
    audio::mix_pcm(voices, output);
    check(output == std::array<std::int16_t, 6>{128, 255, -129, -256, 0, 0}, "One-shot PCM and exact EOF silence");
    check(!voices[0].playing && voices[0].position == 0, "One-shot did not release/reset its position at EOF");
    voices[0] = {source, 1, 0, 128, 255, true, true};
    audio::mix_pcm(voices, {});
    check(voices[0].playing && voices[0].position == 0, "Empty output consumed a frame");
    std::array<std::int16_t, 2> first{};
    audio::mix_pcm(voices, first);
    check(first == std::array<std::int16_t, 2>{128, 255} && voices[0].position == 1, "Single-frame request/position");
    audio::mix_pcm(voices, output);
    check(output == std::array<std::int16_t, 6>{-129, -256, 128, 255, -129, -256}, "Loop lost its phase across output requests");
    check(voices[0].playing && voices[0].position == 0, "Loop did not wrap exactly");
    voices[0] = {source, 1, 0, 0, 0, false, true};
    audio::mix_pcm(voices, first);
    check(first == std::array<std::int16_t, 2>{} && voices[0].playing && voices[0].position == 1, "Muted voice did not consume its frame");
    audio::mix_pcm(voices, first);
    check(first == std::array<std::int16_t, 2>{} && !voices[0].playing, "Muted one-shot did not reach EOF");
}
} // namespace

int main() {
    try {
        source_vectors();
        playback_boundaries();
        std::cout << "Original gain/saturation/order vectors and playback boundaries passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
