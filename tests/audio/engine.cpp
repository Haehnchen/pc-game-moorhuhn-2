#include "audio/engine.hpp"

#include "audio/fixtures.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace audio = moorhuhn2::audio;
namespace assets = moorhuhn2::assets;
using audio::Action;
using audio::Command;
using audio::Sample;
using audio::Stream;

float real(std::uint32_t value) {
    return std::bit_cast<float>(value);
}

struct PanGain {
    std::string id;
    std::string kind;
    int channels{}, pan_value{}, volume{}, left{}, right{};
    float gain{};
    std::optional<float> pan;
};

struct GainCase {
    std::string_view id;
    std::uint32_t input_bits;
    std::uint32_t normalized_bits;
    int volume;
};

struct FadeStep {
    int index;
    std::uint32_t stored_gain_bits;
    int volume;
};

struct FadeCase {
    std::string id;
    std::uint32_t initial_gain_bits;
    std::vector<FadeStep> steps;
};

struct AllocationStep {
    Command command;
    bool finish_slots;
    std::size_t active_channels;
    std::uint64_t channels_stolen;
    std::optional<int> selected_slot;
};

struct AllocationTrace {
    std::string id;
    std::vector<AllocationStep> steps;
};

const std::array pan_vectors{PanGain{"mono-left-full", "sample", 1, 0, 255, 255, 0, real(0x3f800000U), real(0xbf800000U)},
    PanGain{"mono-left-half", "sample", 1, 0, 127, 127, 0, real(0x3f000000U), real(0xbf800000U)},
    PanGain{"mono-left-one-byte", "sample", 1, 0, 1, 1, 0, real(0x3b808081U), real(0xbf800000U)},
    PanGain{"mono-left-mute", "sample", 1, 0, 0, 0, 0, real(0x00000000U), real(0xbf800000U)},
    PanGain{"mono-left-quarter-full", "sample", 1, 64, 255, 191, 64, real(0x3f800000U), real(0xbf000000U)},
    PanGain{"mono-left-quarter-half", "sample", 1, 64, 127, 95, 31, real(0x3f000000U), real(0xbf000000U)},
    PanGain{"mono-left-quarter-one-byte", "sample", 1, 64, 1, 0, 0, real(0x3b808081U), real(0xbf000000U)},
    PanGain{"mono-left-quarter-mute", "sample", 1, 64, 0, 0, 0, real(0x00000000U), real(0xbf000000U)},
    PanGain{"mono-center-minus-byte-full", "sample", 1, 127, 255, 128, 127, real(0x3f800000U), real(0xbbffffffU)},
    PanGain{"mono-center-minus-byte-half", "sample", 1, 127, 127, 63, 63, real(0x3f000000U), real(0xbbffffffU)},
    PanGain{"mono-center-minus-byte-one-byte", "sample", 1, 127, 1, 0, 0, real(0x3b808081U), real(0xbbffffffU)},
    PanGain{"mono-center-minus-byte-mute", "sample", 1, 127, 0, 0, 0, real(0x00000000U), real(0xbbffffffU)},
    PanGain{"mono-center-negative-normal-full", "sample", 1, 127, 255, 128, 127, real(0x3f800000U), real(0xb3800000U)},
    PanGain{"mono-center-negative-normal-half", "sample", 1, 127, 127, 63, 63, real(0x3f000000U), real(0xb3800000U)},
    PanGain{"mono-center-negative-normal-one-byte", "sample", 1, 127, 1, 0, 0, real(0x3b808081U), real(0xb3800000U)},
    PanGain{"mono-center-negative-normal-mute", "sample", 1, 127, 0, 0, 0, real(0x00000000U), real(0xb3800000U)},
    PanGain{"mono-center-round53-below-tie-full", "sample", 1, 127, 255, 128, 127, real(0x3f800000U), real(0xa4800001U)},
    PanGain{"mono-center-round53-below-tie-half", "sample", 1, 127, 127, 63, 63, real(0x3f000000U), real(0xa4800001U)},
    PanGain{"mono-center-round53-below-tie-one-byte", "sample", 1, 127, 1, 0, 0, real(0x3b808081U), real(0xa4800001U)},
    PanGain{"mono-center-round53-below-tie-mute", "sample", 1, 127, 0, 0, 0, real(0x00000000U), real(0xa4800001U)},
    PanGain{"mono-center-round53-tie-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), real(0xa4800000U)},
    PanGain{"mono-center-round53-tie-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), real(0xa4800000U)},
    PanGain{"mono-center-round53-tie-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), real(0xa4800000U)},
    PanGain{"mono-center-round53-tie-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), real(0xa4800000U)},
    PanGain{"mono-center-round53-above-tie-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), real(0xa47fffffU)},
    PanGain{"mono-center-round53-above-tie-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), real(0xa47fffffU)},
    PanGain{"mono-center-round53-above-tie-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), real(0xa47fffffU)},
    PanGain{"mono-center-round53-above-tie-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), real(0xa47fffffU)},
    PanGain{"mono-center-negative-tiny-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), real(0xa0000000U)},
    PanGain{"mono-center-negative-tiny-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), real(0xa0000000U)},
    PanGain{"mono-center-negative-tiny-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), real(0xa0000000U)},
    PanGain{"mono-center-negative-tiny-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), real(0xa0000000U)},
    PanGain{"mono-center-negative-subnormal-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), real(0x80000001U)},
    PanGain{"mono-center-negative-subnormal-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), real(0x80000001U)},
    PanGain{"mono-center-negative-subnormal-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), real(0x80000001U)},
    PanGain{"mono-center-negative-subnormal-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), real(0x80000001U)},
    PanGain{"mono-center-negative-zero-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), real(0x80000000U)},
    PanGain{"mono-center-negative-zero-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), real(0x80000000U)},
    PanGain{"mono-center-negative-zero-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), real(0x80000000U)},
    PanGain{"mono-center-negative-zero-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), real(0x80000000U)},
    PanGain{"mono-center-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), real(0x00000000U)},
    PanGain{"mono-center-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), real(0x00000000U)},
    PanGain{"mono-center-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), real(0x00000000U)},
    PanGain{"mono-center-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), real(0x00000000U)},
    PanGain{"mono-right-quarter-full", "sample", 1, 192, 255, 63, 192, real(0x3f800000U), real(0x3f000000U)},
    PanGain{"mono-right-quarter-half", "sample", 1, 192, 127, 31, 95, real(0x3f000000U), real(0x3f000000U)},
    PanGain{"mono-right-quarter-one-byte", "sample", 1, 192, 1, 0, 0, real(0x3b808081U), real(0x3f000000U)},
    PanGain{"mono-right-quarter-mute", "sample", 1, 192, 0, 0, 0, real(0x00000000U), real(0x3f000000U)},
    PanGain{"mono-right-byte255-full", "sample", 1, 255, 255, 0, 255, real(0x3f800000U), real(0x3f7e0000U)},
    PanGain{"mono-right-byte255-half", "sample", 1, 255, 127, 0, 127, real(0x3f000000U), real(0x3f7e0000U)},
    PanGain{"mono-right-byte255-one-byte", "sample", 1, 255, 1, 0, 1, real(0x3b808081U), real(0x3f7e0000U)},
    PanGain{"mono-right-byte255-mute", "sample", 1, 255, 0, 0, 0, real(0x00000000U), real(0x3f7e0000U)},
    PanGain{"mono-right-full", "sample", 1, 255, 255, 0, 255, real(0x3f800000U), real(0x3f800000U)},
    PanGain{"mono-right-half", "sample", 1, 255, 127, 0, 127, real(0x3f000000U), real(0x3f800000U)},
    PanGain{"mono-right-one-byte", "sample", 1, 255, 1, 0, 1, real(0x3b808081U), real(0x3f800000U)},
    PanGain{"mono-right-mute", "sample", 1, 255, 0, 0, 0, real(0x00000000U), real(0x3f800000U)},
    PanGain{"mono-sample-default-full", "sample", 1, 128, 255, 127, 128, real(0x3f800000U), std::nullopt},
    PanGain{"mono-sample-default-half", "sample", 1, 128, 127, 63, 63, real(0x3f000000U), std::nullopt},
    PanGain{"mono-sample-default-one-byte", "sample", 1, 128, 1, 0, 0, real(0x3b808081U), std::nullopt},
    PanGain{"mono-sample-default-mute", "sample", 1, 128, 0, 0, 0, real(0x00000000U), std::nullopt},
    PanGain{"mono-sample-default-quarter", "sample", 1, 128, 63, 31, 31, real(0x3e800000U), std::nullopt},
    PanGain{"mono-sample-default-three-quarter", "sample", 1, 128, 191, 95, 95, real(0x3f400000U), std::nullopt},
    PanGain{"stereo-sample-default-full", "sample", 2, 128, 255, 127, 128, real(0x3f800000U), std::nullopt},
    PanGain{"stereo-sample-default-half", "sample", 2, 128, 127, 63, 63, real(0x3f000000U), std::nullopt},
    PanGain{"stereo-sample-default-one-byte", "sample", 2, 128, 1, 0, 0, real(0x3b808081U), std::nullopt},
    PanGain{"stereo-sample-default-mute", "sample", 2, 128, 0, 0, 0, real(0x00000000U), std::nullopt},
    PanGain{"stereo-sample-default-quarter", "sample", 2, 128, 63, 31, 31, real(0x3e800000U), std::nullopt},
    PanGain{"stereo-sample-default-three-quarter", "sample", 2, 128, 191, 95, 95, real(0x3f400000U), std::nullopt},
    PanGain{"stereo-stream-default-full", "stream", 2, -1, 255, 255, 255, real(0x3f800000U), std::nullopt},
    PanGain{"stereo-stream-default-half", "stream", 2, -1, 127, 127, 127, real(0x3f000000U), std::nullopt},
    PanGain{"stereo-stream-default-one-byte", "stream", 2, -1, 1, 1, 1, real(0x3b808081U), std::nullopt},
    PanGain{"stereo-stream-default-mute", "stream", 2, -1, 0, 0, 0, real(0x00000000U), std::nullopt},
    PanGain{"stereo-stream-default-quarter", "stream", 2, -1, 63, 63, 63, real(0x3e800000U), std::nullopt},
    PanGain{"stereo-stream-default-three-quarter", "stream", 2, -1, 191, 191, 191, real(0x3f400000U), std::nullopt}};

constexpr std::array gain_cases{GainCase{"byte-1-below", 0x3b808080U, 0x00000000U, 0}, GainCase{"byte-1-at", 0x3b808081U, 0x3b808081U, 1},
    GainCase{"byte-1-above", 0x3b808082U, 0x3b808081U, 1}, GainCase{"byte-2-below", 0x3c008080U, 0x3b808081U, 1}, GainCase{"byte-2-at", 0x3c008081U, 0x3c008081U, 2},
    GainCase{"byte-2-above", 0x3c008082U, 0x3c008081U, 2}, GainCase{"byte-63-below", 0x3e7cfcfcU, 0x3e78f8f9U, 62}, GainCase{"byte-63-at", 0x3e7cfcfdU, 0x3e7cfcfdU, 63},
    GainCase{"byte-63-above", 0x3e7cfcfeU, 0x3e7cfcfdU, 63}, GainCase{"byte-64-below", 0x3e808080U, 0x3e7cfcfdU, 63}, GainCase{"byte-64-at", 0x3e808081U, 0x3e808081U, 64},
    GainCase{"byte-64-above", 0x3e808082U, 0x3e808081U, 64}, GainCase{"byte-127-below", 0x3efefefeU, 0x3efcfcfdU, 126}, GainCase{"byte-127-at", 0x3efefeffU, 0x3efefeffU, 127},
    GainCase{"byte-127-above", 0x3efeff00U, 0x3efefeffU, 127}, GainCase{"byte-128-below", 0x3f008080U, 0x3efefeffU, 127}, GainCase{"byte-128-at", 0x3f008081U, 0x3f008081U, 128},
    GainCase{"byte-128-above", 0x3f008082U, 0x3f008081U, 128}, GainCase{"byte-191-below", 0x3f3fbfbfU, 0x3f3ebebfU, 190}, GainCase{"byte-191-at", 0x3f3fbfc0U, 0x3f3fbfc0U, 191},
    GainCase{"byte-191-above", 0x3f3fbfc1U, 0x3f3fbfc0U, 191}, GainCase{"byte-192-below", 0x3f40c0c0U, 0x3f3fbfc0U, 191}, GainCase{"byte-192-at", 0x3f40c0c1U, 0x3f40c0c1U, 192},
    GainCase{"byte-192-above", 0x3f40c0c2U, 0x3f40c0c1U, 192}, GainCase{"byte-254-below", 0x3f7efefeU, 0x3f7dfdfeU, 253}, GainCase{"byte-254-at", 0x3f7efeffU, 0x3f7efeffU, 254},
    GainCase{"byte-254-above", 0x3f7eff00U, 0x3f7efeffU, 254}, GainCase{"byte-255-below", 0x3f7fffffU, 0x3f7efeffU, 254}, GainCase{"byte-255-at", 0x3f800000U, 0x3f800000U, 255},
    GainCase{"byte-255-above", 0x3f800001U, 0x3f800000U, 255}, GainCase{"mute-negative", 0x80000001U, 0x00000000U, 0}, GainCase{"mute-negative-zero", 0x80000000U, 0x00000000U, 0},
    GainCase{"mute", 0x00000000U, 0x00000000U, 0}, GainCase{"mute-positive-subnormal", 0x00000001U, 0x00000000U, 0}, GainCase{"mute-negative-one", 0xbf800000U, 0x00000000U, 0},
    GainCase{"full-above", 0x3f800001U, 0x3f800000U, 255}};

constexpr std::array<std::uint32_t, 256> normalization_bits{0x00000000U, 0x3b808081U, 0x3c008081U, 0x3c40c0c1U, 0x3c808081U, 0x3ca0a0a1U, 0x3cc0c0c1U, 0x3ce0e0e1U, 0x3d008081U,
    0x3d109091U, 0x3d20a0a1U, 0x3d30b0b1U, 0x3d40c0c1U, 0x3d50d0d1U, 0x3d60e0e1U, 0x3d70f0f1U, 0x3d808081U, 0x3d888889U, 0x3d909091U, 0x3d989899U, 0x3da0a0a1U, 0x3da8a8a9U,
    0x3db0b0b1U, 0x3db8b8b9U, 0x3dc0c0c1U, 0x3dc8c8c9U, 0x3dd0d0d1U, 0x3dd8d8d9U, 0x3de0e0e1U, 0x3de8e8e9U, 0x3df0f0f1U, 0x3df8f8f9U, 0x3e008081U, 0x3e048485U, 0x3e088889U,
    0x3e0c8c8dU, 0x3e109091U, 0x3e149495U, 0x3e189899U, 0x3e1c9c9dU, 0x3e20a0a1U, 0x3e24a4a5U, 0x3e28a8a9U, 0x3e2cacadU, 0x3e30b0b1U, 0x3e34b4b5U, 0x3e38b8b9U, 0x3e3cbcbdU,
    0x3e40c0c1U, 0x3e44c4c5U, 0x3e48c8c9U, 0x3e4ccccdU, 0x3e50d0d1U, 0x3e54d4d5U, 0x3e58d8d9U, 0x3e5cdcddU, 0x3e60e0e1U, 0x3e64e4e5U, 0x3e68e8e9U, 0x3e6cecedU, 0x3e70f0f1U,
    0x3e74f4f5U, 0x3e78f8f9U, 0x3e7cfcfdU, 0x3e808081U, 0x3e828283U, 0x3e848485U, 0x3e868687U, 0x3e888889U, 0x3e8a8a8bU, 0x3e8c8c8dU, 0x3e8e8e8fU, 0x3e909091U, 0x3e929293U,
    0x3e949495U, 0x3e969697U, 0x3e989899U, 0x3e9a9a9bU, 0x3e9c9c9dU, 0x3e9e9e9fU, 0x3ea0a0a1U, 0x3ea2a2a3U, 0x3ea4a4a5U, 0x3ea6a6a7U, 0x3ea8a8a9U, 0x3eaaaaabU, 0x3eacacadU,
    0x3eaeaeafU, 0x3eb0b0b1U, 0x3eb2b2b3U, 0x3eb4b4b5U, 0x3eb6b6b7U, 0x3eb8b8b9U, 0x3ebababbU, 0x3ebcbcbdU, 0x3ebebebfU, 0x3ec0c0c1U, 0x3ec2c2c3U, 0x3ec4c4c5U, 0x3ec6c6c7U,
    0x3ec8c8c9U, 0x3ecacacbU, 0x3ecccccdU, 0x3ecececfU, 0x3ed0d0d1U, 0x3ed2d2d3U, 0x3ed4d4d5U, 0x3ed6d6d7U, 0x3ed8d8d9U, 0x3edadadbU, 0x3edcdcddU, 0x3edededfU, 0x3ee0e0e1U,
    0x3ee2e2e3U, 0x3ee4e4e5U, 0x3ee6e6e7U, 0x3ee8e8e9U, 0x3eeaeaebU, 0x3eececedU, 0x3eeeeeefU, 0x3ef0f0f1U, 0x3ef2f2f3U, 0x3ef4f4f5U, 0x3ef6f6f7U, 0x3ef8f8f9U, 0x3efafafbU,
    0x3efcfcfdU, 0x3efefeffU, 0x3f008081U, 0x3f018182U, 0x3f028283U, 0x3f038384U, 0x3f048485U, 0x3f058586U, 0x3f068687U, 0x3f078788U, 0x3f088889U, 0x3f09898aU, 0x3f0a8a8bU,
    0x3f0b8b8cU, 0x3f0c8c8dU, 0x3f0d8d8eU, 0x3f0e8e8fU, 0x3f0f8f90U, 0x3f109091U, 0x3f119192U, 0x3f129293U, 0x3f139394U, 0x3f149495U, 0x3f159596U, 0x3f169697U, 0x3f179798U,
    0x3f189899U, 0x3f19999aU, 0x3f1a9a9bU, 0x3f1b9b9cU, 0x3f1c9c9dU, 0x3f1d9d9eU, 0x3f1e9e9fU, 0x3f1f9fa0U, 0x3f20a0a1U, 0x3f21a1a2U, 0x3f22a2a3U, 0x3f23a3a4U, 0x3f24a4a5U,
    0x3f25a5a6U, 0x3f26a6a7U, 0x3f27a7a8U, 0x3f28a8a9U, 0x3f29a9aaU, 0x3f2aaaabU, 0x3f2babacU, 0x3f2cacadU, 0x3f2dadaeU, 0x3f2eaeafU, 0x3f2fafb0U, 0x3f30b0b1U, 0x3f31b1b2U,
    0x3f32b2b3U, 0x3f33b3b4U, 0x3f34b4b5U, 0x3f35b5b6U, 0x3f36b6b7U, 0x3f37b7b8U, 0x3f38b8b9U, 0x3f39b9baU, 0x3f3ababbU, 0x3f3bbbbcU, 0x3f3cbcbdU, 0x3f3dbdbeU, 0x3f3ebebfU,
    0x3f3fbfc0U, 0x3f40c0c1U, 0x3f41c1c2U, 0x3f42c2c3U, 0x3f43c3c4U, 0x3f44c4c5U, 0x3f45c5c6U, 0x3f46c6c7U, 0x3f47c7c8U, 0x3f48c8c9U, 0x3f49c9caU, 0x3f4acacbU, 0x3f4bcbccU,
    0x3f4ccccdU, 0x3f4dcdceU, 0x3f4ececfU, 0x3f4fcfd0U, 0x3f50d0d1U, 0x3f51d1d2U, 0x3f52d2d3U, 0x3f53d3d4U, 0x3f54d4d5U, 0x3f55d5d6U, 0x3f56d6d7U, 0x3f57d7d8U, 0x3f58d8d9U,
    0x3f59d9daU, 0x3f5adadbU, 0x3f5bdbdcU, 0x3f5cdcddU, 0x3f5ddddeU, 0x3f5ededfU, 0x3f5fdfe0U, 0x3f60e0e1U, 0x3f61e1e2U, 0x3f62e2e3U, 0x3f63e3e4U, 0x3f64e4e5U, 0x3f65e5e6U,
    0x3f66e6e7U, 0x3f67e7e8U, 0x3f68e8e9U, 0x3f69e9eaU, 0x3f6aeaebU, 0x3f6bebecU, 0x3f6cecedU, 0x3f6dedeeU, 0x3f6eeeefU, 0x3f6feff0U, 0x3f70f0f1U, 0x3f71f1f2U, 0x3f72f2f3U,
    0x3f73f3f4U, 0x3f74f4f5U, 0x3f75f5f6U, 0x3f76f6f7U, 0x3f77f7f8U, 0x3f78f8f9U, 0x3f79f9faU, 0x3f7afafbU, 0x3f7bfbfcU, 0x3f7cfcfdU, 0x3f7dfdfeU, 0x3f7efeffU, 0x3f800000U};

const std::array fade_cases{FadeCase{"title-in-quarter-second", 0x00000000U,
                                {{0, 0x3e000000U, 31}, {1, 0x3e800000U, 63}, {2, 0x3ec00000U, 95}, {3, 0x3f000000U, 127}, {4, 0x3f200000U, 159}, {5, 0x3f400000U, 191},
                                    {6, 0x3f600000U, 223}, {7, 0x3f800000U, 255}, {8, 0x3f800000U, 255}}},
    FadeCase{"round-out-quarter-second", 0x3f800000U,
        {{0, 0x3f600000U, 223}, {1, 0x3f400000U, 191}, {2, 0x3f200000U, 159}, {3, 0x3f000000U, 127}, {4, 0x3ec00000U, 95}, {5, 0x3e800000U, 63}, {6, 0x3e000000U, 31},
            {7, 0x00000000U, 0}, {8, 0x00000000U, 0}}},
    FadeCase{"title-in-variable-samples", 0x00000000U,
        {{0, 0x00000000U, 0}, {1, 0x3c088889U, 2}, {2, 0x3d6eeeefU, 14}, {3, 0x3e9ddddeU, 78}, {4, 0x3f4eeeefU, 206}, {5, 0x3f800000U, 255}}},
    FadeCase{"round-out-variable-samples", 0x3f800000U,
        {{0, 0x3f800000U, 255}, {1, 0x3f7ddddeU, 252}, {2, 0x3f711111U, 240}, {3, 0x3f311111U, 176}, {4, 0x3e444444U, 48}, {5, 0x00000000U, 0}}},
    FadeCase{"title-in-retained-half-ulp", 0x3f7fffffU, {{0, 0x3f800000U, 255}, {1, 0x3f800000U, 255}, {2, 0x3f800000U, 255}}}};

const std::array allocation_traces{AllocationTrace{"one-shot-saturation", {{{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, 0},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 2, 0, 1},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 3, 0, 2},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 4, 0, 3},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 5, 0, 4},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 6, 0, 5},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 7, 0, 6},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 8, 0, 7},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 9, 0, 8},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 10, 0, 9},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 11, 0, 10},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 12, 0, 11},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 13, 0, 12},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 14, 0, 13},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 15, 0, 14},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 16, 0, 15},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 17, 0, 16},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 18, 0, 17},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 19, 0, 18},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 20, 0, 19},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 21, 0, 20},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 22, 0, 21},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 23, 0, 22},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 24, 0, 23},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 25, 0, 24},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 26, 0, 25},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 27, 0, 26},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 28, 0, 27},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 29, 0, 28},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 30, 0, 29},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 31, 0, 30},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 0, 31},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 0},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 2, 1},
                                                                              {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 3, 2}}},
    AllocationTrace{"music-and-stale-loop-wrappers", {{{0, Action::select_stream, Stream::title, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, 0},
                                                         {{0, Action::start_loop, Stream::none, Sample::airplane_motor, 1.0F, std::nullopt}, false, 2, 0, 1},
                                                         {{0, Action::start_loop, Stream::none, Sample::airplane_failure, 1.0F, std::nullopt}, false, 3, 0, 2},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 4, 0, 3},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 5, 0, 4},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 6, 0, 5},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 7, 0, 6},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 8, 0, 7},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 9, 0, 8},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 10, 0, 9},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 11, 0, 10},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 12, 0, 11},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 13, 0, 12},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 14, 0, 13},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 15, 0, 14},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 16, 0, 15},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 17, 0, 16},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 18, 0, 17},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 19, 0, 18},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 20, 0, 19},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 21, 0, 20},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 22, 0, 21},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 23, 0, 22},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 24, 0, 23},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 25, 0, 24},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 26, 0, 25},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 27, 0, 26},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 28, 0, 27},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 29, 0, 28},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 30, 0, 29},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 31, 0, 30},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 0, 31},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 1},
                                                         {{0, Action::set_loop_pan, Stream::none, Sample::airplane_motor, 1.0F, 1.0F}, false, 32, 1, std::nullopt},
                                                         {{0, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0.0F, std::nullopt}, false, 32, 1, std::nullopt},
                                                         {{0, Action::stop_loop, Stream::none, Sample::airplane_motor, 1.0F, std::nullopt}, false, 31, 1, std::nullopt},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 1},
                                                         {{0, Action::stop_loop, Stream::none, Sample::airplane_failure, 1.0F, std::nullopt}, false, 31, 1, std::nullopt},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 2}}},
    AllocationTrace{"muted-paused-saturation", {{{0, Action::select_stream, Stream::title, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, 0},
                                                   {{0, Action::start_loop, Stream::none, Sample::airplane_motor, 1.0F, std::nullopt}, false, 2, 0, 1},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 3, 0, 2},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 4, 0, 3},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 5, 0, 4},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 6, 0, 5},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 7, 0, 6},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 8, 0, 7},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 9, 0, 8},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 10, 0, 9},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 11, 0, 10},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 12, 0, 11},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 13, 0, 12},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 14, 0, 13},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 15, 0, 14},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 16, 0, 15},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 17, 0, 16},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 18, 0, 17},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 19, 0, 18},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 20, 0, 19},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 21, 0, 20},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 22, 0, 21},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 23, 0, 22},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 24, 0, 23},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 25, 0, 24},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 26, 0, 25},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 27, 0, 26},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 28, 0, 27},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 29, 0, 28},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 30, 0, 29},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 31, 0, 30},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 0, 31},
                                                   {{0, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0.0F, std::nullopt}, false, 32, 0, std::nullopt},
                                                   {{0, Action::pause, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 0, std::nullopt},
                                                   {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 1},
                                                   {{0, Action::resume, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, std::nullopt}}},
    AllocationTrace{"finished-holes-circular-reuse", {{{0, Action::select_stream, Stream::title, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, 0},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 2, 0, 1},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 3, 0, 2},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 4, 0, 3},
                                                         {{0, Action::play_sample, Stream::none, Sample::typing_click, 0.0F, std::nullopt}, false, 5, 0, 4},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 6, 0, 5},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 7, 0, 6},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 8, 0, 7},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 9, 0, 8},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 10, 0, 9},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 11, 0, 10},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 12, 0, 11},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 13, 0, 12},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 14, 0, 13},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 15, 0, 14},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 16, 0, 15},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 17, 0, 16},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 18, 0, 17},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 19, 0, 18},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 20, 0, 19},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 21, 0, 20},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 22, 0, 21},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 23, 0, 22},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 24, 0, 23},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 25, 0, 24},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 26, 0, 25},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 27, 0, 26},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 28, 0, 27},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 29, 0, 28},
                                                         {{0, Action::play_sample, Stream::none, Sample::typing_click, 0.0F, std::nullopt}, false, 30, 0, 29},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 31, 0, 30},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 0.0F, std::nullopt}, false, 32, 0, 31},
                                                         {{0, Action::stop_samples, Stream::none, Sample::shot, 1.0F, std::nullopt}, true, 30, 0, std::nullopt},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 31, 0, 4},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 0, 29},
                                                         {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 30}}},
    AllocationTrace{"stream-stop-releases-reservation", {{{0, Action::select_stream, Stream::title, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, 0},
                                                            {{0, Action::start_loop, Stream::none, Sample::airplane_motor, 1.0F, std::nullopt}, false, 2, 0, 1},
                                                            {{0, Action::stop_stream, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, std::nullopt},
                                                            {{0, Action::select_stream, Stream::highscore, Sample::shot, 1.0F, std::nullopt}, false, 2, 0, 2},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 3, 0, 3},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 4, 0, 4},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 5, 0, 5},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 6, 0, 6},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 7, 0, 7},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 8, 0, 8},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 9, 0, 9},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 10, 0, 10},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 11, 0, 11},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 12, 0, 12},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 13, 0, 13},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 14, 0, 14},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 15, 0, 15},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 16, 0, 16},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 17, 0, 17},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 18, 0, 18},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 19, 0, 19},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 20, 0, 20},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 21, 0, 21},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 22, 0, 22},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 23, 0, 23},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 24, 0, 24},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 25, 0, 25},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 26, 0, 26},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 27, 0, 27},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 28, 0, 28},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 29, 0, 29},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 30, 0, 30},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 31, 0, 31},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 0, 0},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 1},
                                                            {{0, Action::stop_stream, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 31, 1, std::nullopt},
                                                            {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 32, 1, 2}}},
    AllocationTrace{"same-sample-overlap", {{{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 1, 0, 0},
                                               {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 2, 0, 1},
                                               {{0, Action::play_sample, Stream::none, Sample::shot, 1.0F, std::nullopt}, false, 3, 0, 2}}}};

constexpr std::size_t pcm_frames = 4096;

void check(bool value, std::string_view message) {
    if (!value) {
        throw std::runtime_error(std::string(message));
    }
}

void rejects(const std::function<void()>& operation, std::string_view message) {
    bool rejected{};
    try {
        operation();
    } catch (const std::exception&) {
        rejected = true;
    }

    check(rejected, message);
}

double energy(std::span<const float> pcm) {
    double total{};

    for (const auto value : pcm) {
        check(std::isfinite(value), "PCM is not finite");
        total += static_cast<double>(value) * value;
    }

    return total;
}

void apply(audio::Engine& engine, Command command) {
    engine.apply(std::span(&command, 1));
}

void matching_pcm(std::span<const float> actual, std::span<const float> expected, std::string_view message) {
    check(actual.size() == expected.size(), message);
    check(std::equal(actual.begin(), actual.end(), expected.begin(),
              [](float left, float right) {
                  return std::isfinite(left) && std::isfinite(right) && std::abs(left - right) <= 0.000001F;
              }),
        message);
}

void matching_mono_pan(std::span<const float> actual, std::span<const float> reference, float pan) {
    check(actual.size() == reference.size(), "Effect pan changed the PCM frame count");
    // A reference at the same pan preserves exact integer quantization; a
    // centered signal cannot be proportionally scaled into an endpoint signal.
    for (std::size_t index = 0; index < actual.size(); index += 2) {
        check(actual[index] == (pan > 0 ? 0 : reference[index]) && actual[index + 1] == (pan < 0 ? 0 : reference[index + 1]),
            "Effect pan restarted playback or selected the wrong speaker");
    }

    check(energy(actual) > 0, "Panned effect loop is silent");
}

void loading(const assets::AssetStore& assets) {
    const std::array order{audio::asset_id(Stream::title), audio::asset_id(Stream::highscore), audio::asset_id(Stream::birds)};
    std::vector<std::string> completed;
    audio::Engine engine(assets, audio::OutputMode::offline, 44100, {order, [&](std::string_view id) {
                                                                         completed.emplace_back(id);
                                                                     }});
    check(completed.size() == assets.audio_ids().size() && std::equal(order.begin(), order.end(), completed.begin()), "Source music loads did not precede remaining audio");
    auto sorted = completed;
    std::ranges::sort(sorted);
    check(std::ranges::equal(sorted, assets.audio_ids()), "Audio preparation repeated or omitted an asset");

    for (std::size_t index = 0; index < sorted.size(); ++index) {
        check(engine.inventory()[index].id == sorted[index], "Load ordering changed the public canonical inventory");
    }

    check(engine.state().streams_started == 0 && engine.state().samples_started == 0 && energy(engine.render_frames(32)) == 0,
        "Resource preparation played audio before title entry");

    std::array<float, 8> silence;
    silence.fill(1);
    const auto rendered = engine.state().frames_rendered;
    rejects(
        [&] {
            engine.render_frames(std::span(silence).first(3));
        },
        "Incomplete stereo frame accepted");
    engine.render_frames(std::span<float>{});
    check(engine.state().frames_rendered == rendered, "Invalid or empty PCM storage advanced playback");
    engine.render_frames(silence);
    check(std::ranges::all_of(silence,
              [](float sample) {
                  return sample == 0;
              })
              && engine.state().frames_rendered == rendered + 4,
        "Caller-owned PCM storage was not completely filled or counted in stereo frames");

    for (const auto count : {std::size_t{1}, assets.audio_ids().size()}) {
        std::size_t calls{};
        rejects(
            [&] {
                audio::Engine cancelled(assets, audio::OutputMode::offline, 44100, {order, [&](std::string_view) {
                                                                                        if (++calls == count) {
                                                                                            throw std::runtime_error("cancel preparation");
                                                                                        }
                                                                                    }});
            },
            "Early/late preparation cancellation was ignored");
        check(calls == count, "Audio preparation continued after cancellation");
    }

    for (const auto invalid : {std::array{order[0], order[0]}, std::array{order[0], std::string_view{"sfx/missing"}}}) {
        rejects(
            [&] {
                audio::Engine rejected(assets, audio::OutputMode::offline, 44100, {invalid, {}});
            },
            "Unknown/duplicate audio preparation order was accepted");
    }

    apply(engine, {1, Action::play_sample, Stream::none, Sample::get_ready});
    check(energy(engine.render_frames(44100)) > 0, "Cancelled other preparations invalidated the live mixer");
    std::cout << "audio loading: source order, canonical inventory, silent preparation and early/late cancellation passed\n";
}

void offline(const assets::AssetStore& assets) {
    audio::Engine engine(assets, audio::OutputMode::offline, 44100);
    check(engine.status() == audio::OutputStatus::offline && engine.needs_pumping(), "offline mode selected a device");
    check(engine.inventory().size() == assets.audio_ids().size(), "missing source audio inventory");

    for (const auto& info : engine.inventory()) {
        const auto source = assets.audio(info.id);
        check(info.sample_rate == static_cast<int>(source.sample_rate), "native source sample rate changed");
        check(info.channels == static_cast<int>(source.channels), "native source channels changed");
        check(info.decoded_frames > 0, "missing source duration");
    }

    check(engine.render_frames(0).empty(), "zero-frame render is not empty");
    check(energy(engine.render_frames(44100)) == 0, "empty mixer is not silent");
    apply(engine, {1, Action::select_stream, Stream::title});
    check(engine.state().stream_gain == 0, "title did not start at gain zero");
    check(energy(engine.render_frames(44100)) == 0, "title gain zero is audible");
    apply(engine, {2, Action::set_stream_gain, Stream::none, Sample::shot, 0.5F});
    check(engine.state().stream_gain == 127.0F / 255.0F, "plugin gain quantization was lost");
    const auto title = engine.render_frames(44100);
    check(energy(title) > 0, "actual title MP3 did not decode to PCM");
    bool stereo{};

    for (std::size_t i = 0; i < title.size(); i += 2) {
        stereo = stereo || title[i] != title[i + 1];
    }

    check(stereo, "title stereo was collapsed");
    const auto position = engine.state().stream_position;
    apply(engine, {3, Action::select_stream, Stream::title});
    check(engine.state().streams_started == 1 && engine.state().stream_position == position, "same stream restarted");
    apply(engine, {4, Action::stop_stream});
    check(engine.state().stream == Stream::none && energy(engine.render_frames(44100)) == 0, "stop leaves old stream audio");

    apply(engine, {5, Action::select_stream, Stream::birds});
    check(engine.state().stream_gain == 0, "birds did not start at zero gain");
    apply(engine, {6, Action::set_stream_gain});
    check(energy(engine.render_frames(44100)) > 0, "birds stream is silent");
    apply(engine, {7, Action::select_stream, Stream::highscore});
    check(engine.state().stream_gain == 0 && engine.state().streams_started == 3, "transition did not replace birds with highscore");
    apply(engine, {8, Action::stop_stream});

    const std::array events{Command{9, Action::play_sample, Stream::none, Sample::shot}, Command{9, Action::play_sample, Stream::none, Sample::shot},
        Command{10, Action::play_sample, Stream::none, Sample::chicken_far}, Command{11, Action::play_sample, Stream::none, Sample::reload}};
    engine.apply(events);
    check(engine.state().samples_started == 3 && engine.state().active_samples == 3, "ordered overlapping samples lost an event");
    engine.apply(events);
    check(engine.state().samples_started == 3 && engine.state().commands_consumed == 11, "duplicate one-shot commands played twice");
    check(energy(engine.render_frames(220500)) > 0, "sample MP3 output is silent");
    check(engine.state().active_samples == 0, "one-shot samples looped");
    apply(engine, {12, Action::play_sample, Stream::none, Sample::empty_fire});
    check(engine.state().active_samples == 1, "New sample did not become active");
    apply(engine, {13, Action::stop_samples});
    check(energy(engine.render_frames(44100)) == 0 && engine.state().active_samples == 0, "sample stop leaves audible data");
    const std::array unordered{Command{15, Action::play_sample}, Command{14, Action::play_sample}};
    rejects(
        [&] {
            engine.apply(unordered);
        },
        "unordered fresh commands accepted");
    check(engine.state().last_sequence == 13, "invalid batch partly consumed");
    check(audio::asset_id(Sample::typing_click) == "sfx/sfx_typename_click", "name-entry click mapping changed");
    rejects(
        [&] {
            apply(engine, {14, Action::set_stream_gain, Stream::none, Sample::shot, std::numeric_limits<float>::quiet_NaN()});
        },
        "nonfinite gain accepted");

    apply(engine, {14, Action::select_stream, Stream::title});
    apply(engine, {15, Action::set_stream_gain});
    check(energy(engine.render_frames(44100)) > 0, "restarted title is silent");
    apply(engine, {16, Action::pause});
    const auto paused_position = engine.state().stream_position;
    check(engine.state().paused && energy(engine.render_frames(44100)) == 0, "paused offline mixer is audible");
    check(engine.state().stream_position == paused_position, "paused stream advanced");
    apply(engine, {17, Action::play_sample, Stream::none, Sample::typing_click});
    check(engine.state().active_samples == 1 && energy(engine.render_frames(44100)) == 0, "new sample did not preserve paused state");
    apply(engine, {18, Action::resume});
    check(!engine.state().paused && energy(engine.render_frames(44100)) > 0, "resume did not restore stream/sample playback");
    apply(engine, {19, Action::stop_stream});
    apply(engine, {20, Action::stop_samples});
    check(audio::asset_id(Sample::boat_impact) == "sfx/sfx_tree_shot", "Normal boat impact must use the original wood sample");
    const auto started = engine.state().samples_started;
    const Command impact{21, Action::play_sample, Stream::none, Sample::boat_impact};
    apply(engine, impact);
    apply(engine, impact);
    check(engine.state().samples_started == started + 1, "One boat event must play once");
    check(energy(engine.render_frames(220500)) > 0 && engine.state().active_samples == 0, "Boat impact must produce finite one-shot PCM");
    check(audio::asset_id(Sample::stone_impact) == "sfx/sfx_stone_shot", "Stone impact must use the original stone sample");
    const Command stone{22, Action::play_sample, Stream::none, Sample::stone_impact};
    apply(engine, stone);
    apply(engine, stone);
    check(engine.state().samples_started == started + 2, "One stone event must play once");
    check(energy(engine.render_frames(220500)) > 0 && engine.state().active_samples == 0, "Stone impact must produce finite one-shot PCM");
    std::uint64_t sequence = 23;

    for (const auto sample : {Sample::magic_balloons, Sample::balloon_hit, Sample::boat_sinking, Sample::rare_balloon, Sample::church_bell, Sample::church_bell_second,
             Sample::house_hit, Sample::frog_croak, Sample::frog_hit, Sample::spider_swing, Sample::branch_ground, Sample::tree_break, Sample::tree_highfly, Sample::scarecrow_hit,
             Sample::hunting_stand_ladder, Sample::hunting_stand_broken, Sample::big_chicken_appear}) {
        const auto before = engine.state().samples_started;
        const Command command{sequence++, Action::play_sample, Stream::none, sample};
        apply(engine, command);
        apply(engine, command);
        check(engine.state().samples_started == before + 1, "Each chain event must play its sample once");
        check(energy(engine.render_frames(220500)) > 0 && engine.state().active_samples == 0, "Chain sample must produce finite, nonzero one-shot PCM");
    }

    check(audio::asset_id(Sample::branch_ground) == "sfx/sfx_treebranch_hits_ground", "Fallen branch must use the original ground-impact sample");

    for (const auto pan : {std::optional{-1.0F}, std::optional{1.0F}, std::optional<float>{}}) {
        apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::frog_croak, 1, pan});
        const auto pcm = engine.render_frames(441000);
        double left = 0, right = 0;

        for (std::size_t index = 0; index < pcm.size(); index += 2) {
            left += std::abs(pcm[index]);
            right += std::abs(pcm[index + 1]);
        }

        check(pan ? (*pan < 0 ? left > 0 && right == 0 : left == 0 && right > 0) : left > 0 && right > left,
            "Frog pan must reach either speaker, and unpanned mono must restore both sides with the stronger right byte");
        check(engine.state().active_samples == 0, "Frog croak must finish as a one-shot");
    }

    rejects(
        [&] {
            apply(engine, {sequence, Action::play_sample, Stream::none, Sample::frog_croak, 1, std::numeric_limits<float>::quiet_NaN()});
        },
        "Nonfinite sample pan accepted");
    std::cout << "decoded " << assets.audio_ids().size() << " MP3s; stereo/native formats, gain, pan, loops, stops, sequence dedup/order and reuse passed\n";
}

void flying_audio(const assets::AssetStore& assets) {
    audio::Engine engine(assets, audio::OutputMode::offline, 44100);
    audio::Engine reference(assets, audio::OutputMode::offline, 44100);
    std::uint64_t sequence = 1, reference_sequence = 1;

    for (const auto& [sample, id] : std::array{std::pair{Sample::airplane_motor, "sfx/sfx_airplane_loop"},
             std::pair{Sample::airplane_failure, "sfx/sfx_airplane_motor_failure_loop"}, std::pair{Sample::airplane_hit, "sfx/sfx_airplane_shot"},
             std::pair{Sample::airplane_crash, "sfx/sfx_airplane_crash"}, std::pair{Sample::blimp_hit, "sfx/sfx_zeppelin_shot"}}) {
        check(audio::asset_id(sample) == id, "Flying-target sample mapping differs from the original loader");
    }

    for (const auto sample : {Sample::airplane_hit, Sample::airplane_crash, Sample::blimp_hit}) {
        const auto started = engine.state().samples_started;
        const Command hit{sequence++, Action::play_sample, Stream::none, sample};
        apply(engine, hit);
        apply(engine, hit);
        const auto frames = static_cast<std::size_t>(assets.audio(audio::asset_id(sample)).decoded_frames) + 44100;
        check(energy(engine.render_frames(frames)) > 0 && engine.state().active_samples == 0, "Flying-target impact must finish as an audible one-shot");
        check(engine.state().samples_started == started + 1 && engine.state().loop_tracks == 0, "Flying-target duplicate impact command started another track");
    }

    apply(engine, {sequence++, Action::stop_loop, Stream::none, Sample::airplane_motor});
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_failure, 1, -1});
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_failure, 0});
    check(engine.state().loop_tracks == 0 && engine.state().active_loops == 0, "Absent loop commands allocated a track");

    for (const auto sample : {Sample::airplane_motor, Sample::airplane_failure}) {
        const auto before = engine.state();
        const Command start{sequence++, Action::start_loop, Stream::none, sample};
        apply(engine, start);
        apply(reference, {reference_sequence++, Action::start_loop, Stream::none, sample});
        const auto initial = engine.render_frames(4410);
        matching_pcm(initial, reference.render_frames(4410), "Initially unpanned effect loop changed the source");
        matching_mono_pan(initial, initial, 0);
        apply(engine, start);
        check(engine.state().loops_started == before.loops_started + 1 && engine.state().loop_tracks == before.loop_tracks + 1 && engine.state().active_loops == 1,
            "A repeated effect start restarted or allocated another loop");

        const auto rendered = engine.state().frames_rendered;
        apply(engine, {sequence++, Action::set_loop_pan, Stream::none, sample, 1, -1});
        apply(reference, {reference_sequence++, Action::set_loop_pan, Stream::none, sample, 1, -1});
        check(engine.state().frames_rendered == rendered && engine.state().loops_started == before.loops_started + 1, "Setting loop pan advanced audio state");
        matching_mono_pan(engine.render_frames(4410), reference.render_frames(4410), -1);
        apply(engine, {sequence++, Action::set_loop_pan, Stream::none, sample, 1, 1});
        apply(reference, {reference_sequence++, Action::set_loop_pan, Stream::none, sample, 1, 1});
        const auto frames = static_cast<std::size_t>(assets.audio(audio::asset_id(sample)).decoded_frames) + 4410;
        const auto repeated = engine.render_frames(frames);
        matching_mono_pan(repeated, reference.render_frames(frames), 1);
        check(energy(std::span(repeated).last(8820)) > 0 && engine.state().active_loops == 1, "Effect loop stopped at source EOF");
        apply(engine, {sequence++, Action::set_loop_pan, Stream::none, sample});
        apply(reference, {reference_sequence++, Action::set_loop_pan, Stream::none, sample});
        matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Clearing loop pan retained an old speaker balance");

        apply(engine, {sequence++, Action::set_loop_gain, Stream::none, sample, 0.5F});
        apply(reference, {reference_sequence++, Action::set_loop_gain, Stream::none, sample, 0.5F});
        matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Effect gain did not preserve its playback position");
        apply(engine, {sequence++, Action::set_loop_gain, Stream::none, sample, 0});
        check(energy(engine.render_frames(4410)) == 0, "Gain-zero effect loop is audible");
        check(energy(reference.render_frames(4410)) > 0, "Reference effect loop is silent");
        apply(engine, {sequence++, Action::set_loop_gain, Stream::none, sample, 0.5F});
        matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Muted effect loop failed to advance or restarted when gain returned");
        check(engine.state().loops_started == before.loops_started + 1, "Pan or gain updates restarted the loop");

        apply(engine, {sequence++, Action::start_loop, Stream::none, sample});
        apply(reference, {reference_sequence++, Action::start_loop, Stream::none, sample});
        matching_pcm(engine.render_frames(4410), initial, "Replacing the same effect loop did not reset gain, pan and playback");
        matching_pcm(reference.render_frames(4410), initial, "Reference effect restart changed the source");
        check(engine.state().loops_started == before.loops_started + 2 && engine.state().loop_tracks == before.loop_tracks + 1,
            "Replacing the same loop allocated another retained track");
        apply(engine, {sequence++, Action::stop_loop, Stream::none, sample});
        apply(reference, {reference_sequence++, Action::stop_loop, Stream::none, sample});
        check(energy(engine.render_frames(4410)) == 0 && engine.state().active_loops == 0, "Stopped effect loop left audible PCM");
    }

    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor, 1, -1});
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_failure, 1, 1});
    apply(reference, {reference_sequence++, Action::start_loop, Stream::none, Sample::airplane_failure, 1, 1});
    const auto independent = engine.render_frames(4410);
    matching_mono_pan(reference.render_frames(4410), independent, 1);
    check(engine.state().active_loops == 2 && engine.state().loop_tracks == 2, "Airplane effect loops did not retain independent tracks");
    apply(engine, {sequence++, Action::select_stream, Stream::birds});
    apply(engine, {sequence++, Action::set_stream_gain});
    apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::airplane_crash});
    apply(reference, {reference_sequence++, Action::select_stream, Stream::birds});
    apply(reference, {reference_sequence++, Action::set_stream_gain});
    apply(reference, {reference_sequence++, Action::play_sample, Stream::none, Sample::airplane_crash});
    apply(engine, {sequence++, Action::stop_loop, Stream::none, Sample::airplane_motor});
    check(
        engine.state().active_loops == 1 && engine.state().active_samples == 1 && engine.state().stream == Stream::birds, "Stopping the motor interrupted another effect or music");
    matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Stopping one loop changed independent failure, crash or music PCM");
    apply(engine, {sequence++, Action::stop_loop, Stream::none, Sample::airplane_failure});
    apply(reference, {reference_sequence++, Action::stop_loop, Stream::none, Sample::airplane_failure});
    matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Stopping the remaining loop interrupted crash or music PCM");
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor});
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_failure});
    apply(engine, {sequence++, Action::stop_samples});
    apply(reference, {reference_sequence++, Action::stop_samples});
    check(engine.state().active_loops == 0 && engine.state().active_samples == 0 && engine.state().stream == Stream::birds,
        "Stopping samples did not clear loops while preserving music");
    matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Stopping all effects interrupted music PCM");
    apply(engine, {sequence++, Action::stop_stream});
    apply(reference, {reference_sequence++, Action::stop_stream});
    check(energy(engine.render_frames(4410)) == 0, "Effect and music cleanup is not silent");

    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor});
    apply(reference, {reference_sequence++, Action::start_loop, Stream::none, Sample::airplane_motor});
    matching_pcm(engine.render_frames(4410), reference.render_frames(4410), "Reused motor track changed playback");
    apply(engine, {sequence++, Action::pause});
    apply(reference, {reference_sequence++, Action::pause});
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_failure});
    apply(reference, {reference_sequence++, Action::start_loop, Stream::none, Sample::airplane_failure});
    check(engine.state().paused && engine.state().active_loops == 2 && energy(engine.render_frames(44100)) == 0, "Existing or newly started effect loop ignored pause");
    apply(engine, {sequence++, Action::resume});
    apply(reference, {reference_sequence++, Action::resume});
    const auto resumed = engine.render_frames(4410);
    check(energy(resumed) > 0, "Paused effect loops did not resume");
    matching_pcm(resumed, reference.render_frames(4410), "Paused effect loop consumed source frames");
    apply(engine, {sequence++, Action::stop_samples});
    check(energy(engine.render_frames(4410)) == 0 && engine.state().active_loops == 0 && engine.state().loop_tracks == 2, "Reused loop tracks leaked after cleanup");

    for (const auto bad : {Command{0, Action::start_loop, Stream::none, Sample::shot}, Command{0, Action::set_loop_pan, Stream::none, Sample::airplane_hit, 1, 0},
             Command{0, Action::stop_loop, Stream::none, Sample::blimp_hit}, Command{0, Action::set_loop_gain, Stream::none, Sample::airplane_crash},
             Command{0, Action::start_loop, Stream::none, Sample::airplane_motor, std::numeric_limits<float>::quiet_NaN()},
             Command{0, Action::start_loop, Stream::none, Sample::airplane_failure, 1, std::numeric_limits<float>::infinity()},
             Command{0, Action::set_loop_pan, Stream::none, Sample::airplane_failure, 1, std::numeric_limits<float>::quiet_NaN()},
             Command{0, Action::set_loop_gain, Stream::none, Sample::airplane_motor, std::numeric_limits<float>::infinity()}}) {
        auto invalid = bad;
        invalid.sequence = sequence + 1;
        const std::array batch{Command{sequence, Action::start_loop, Stream::none, Sample::airplane_motor}, invalid};
        const auto before = engine.state();
        rejects(
            [&] {
                engine.apply(batch);
            },
            "Malformed effect-loop batch accepted");
        check(engine.state().last_sequence == before.last_sequence && engine.state().commands_consumed == before.commands_consumed
                  && engine.state().loops_started == before.loops_started && engine.state().active_loops == 0 && energy(engine.render_frames(4410)) == 0,
            "Malformed effect-loop batch partly changed playback");
    }

    std::cout << "flying effects: retained loops across EOF, pan/gain continuity, restart/reuse/dedup, independent music/impacts, pause and atomic rejection passed\n";
}

struct SurroundDevice {
    SDL_AudioDeviceID device{};

    SurroundDevice() {
        if (SDL_getenv("SDL_AUDIO_CHANNELS")) {
            check(SDL_InitSubSystem(SDL_INIT_AUDIO), "Initialize surround device");
            const SDL_AudioSpec format{SDL_AUDIO_S16, 6, 48000};
            device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format);
            if (!device) {
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
                throw std::runtime_error("Open surround device");
            }
        }
    }

    ~SurroundDevice() {
        if (device) {
            SDL_CloseAudioDevice(device);
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
    }
};

bool advances_past(const audio::Engine& engine, std::int64_t position) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

    while (engine.state().stream_position <= position) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }

        SDL_Delay(5);
    }

    return true;
}

void dummy(const assets::AssetStore& assets) {
    check(std::string_view(SDL_getenv("SDL_AUDIO_DRIVER")) == "dummy", "device test requires explicit dummy backend");
    const SurroundDevice surround;
    audio::Engine engine(assets, audio::OutputMode::device);
    check(engine.status() == audio::OutputStatus::device && !engine.needs_pumping(), "dummy device was not selected");
    if (SDL_getenv("SDL_AUDIO_CHANNELS")) {
        int count{};
        auto* devices = SDL_GetAudioPlaybackDevices(&count);
        SDL_AudioSpec format{};
        const bool queried = devices && count > 0 && SDL_GetAudioDeviceFormat(devices[0], &format, nullptr);
        SDL_free(devices);
        check(queried && format.freq == 48000 && format.channels == 6 && format.format == SDL_AUDIO_S16, "Surround test must use a 48 kHz six-channel S16 device");
    }
    apply(engine, {1, Action::select_stream, Stream::title});
    apply(engine, {2, Action::set_stream_gain});
    check(advances_past(engine, 0), "dummy callback did not advance playback");
    apply(engine, {3, Action::pause});
    const auto position = engine.state().stream_position;
    // Several dummy callbacks run in this interval.
    SDL_Delay(100);
    check(engine.state().stream_position == position, "paused dummy callback advanced playback");
    apply(engine, {4, Action::resume});
    check(advances_past(engine, position), "dummy callback did not resume playback");
    rejects(
        [&] {
            static_cast<void>(engine.render_frames(10));
        },
        "manual mixing accepted on device");
    std::cout << "SDL dummy callback advanced title playback\n";
}

void unavailable(const assets::AssetStore& assets) {
    check(std::string_view(SDL_getenv("SDL_AUDIO_DRIVER")) == "moorhuhn2-disabled", "failure test requires unavailable backend guard");
    audio::Engine engine(assets, audio::OutputMode::device);
    check(engine.status() == audio::OutputStatus::unavailable_device && engine.needs_pumping() && !engine.device_error().empty(), "device failure did not fall back");
    apply(engine, {1, Action::play_sample, Stream::none, Sample::get_ready});
    const auto pcm = engine.render_frames(44100);
    check(pcm.size() == 88200 && energy(pcm) > 0, "fallback mixer did not generate exact PCM frame count");
    std::cout << "unavailable-device fallback: " << engine.device_error() << '\n';
}

int side_sample(std::int16_t sample, int side_byte) {
    // The independent scalar oracle expresses the source's signed floor,
    // rather than reproducing the production integer/MMX implementation.
    return static_cast<int>(std::floor(static_cast<double>(sample) * side_byte / 256.0));
}

void matching_pcm(std::span<const float> actual, std::span<const std::int16_t> decoded, int left, int right, std::string_view message) {
    check(actual.size() == decoded.size(), message);

    for (std::size_t index = 0; index < actual.size(); index += 2) {
        check(std::isfinite(actual[index]) && std::isfinite(actual[index + 1]) && actual[index] == static_cast<float>(side_sample(decoded[index], left)) / 32768.0F
                  && actual[index + 1] == static_cast<float>(side_sample(decoded[index + 1], right)) / 32768.0F,
            message);
    }
}

struct MixerLifetime {
    MixerLifetime() {
        check(MIX_Init(), "Cannot initialize offline decoder");
    }

    ~MixerLifetime() {
        MIX_Quit();
    }
};

struct DecoderDeleter {
    void operator()(MIX_AudioDecoder* decoder) const {
        MIX_DestroyAudioDecoder(decoder);
    }
};

// Decode native signed-16 PCM without Engine. The original stereo mixer reads
// the current frame at 44100 Hz; a 22050-Hz frame therefore supplies two
// consecutive output frames. Mono midpoints restate the production formula.
class RawDecoder {
public:
    RawDecoder(const assets::AssetStore& store, std::string_view id) {
        const auto source = store.audio(id);
        auto* io = SDL_IOFromConstMem(source.encoded.data(), source.encoded.size());
        check(io != nullptr, "Cannot open raw source decoder");
        decoder_.reset(MIX_CreateAudioDecoder_IO(io, true, 0));
        check(decoder_ != nullptr, "Cannot create raw source decoder");
        check(MIX_GetAudioDecoderFormat(decoder_.get(), &format_), "Cannot query raw decoder format");
        check(format_.freq == static_cast<int>(source.sample_rate) && format_.channels == static_cast<int>(source.channels), "Raw decoder changed the native source format");
        format_.format = SDL_AUDIO_S16;
        check(format_.freq == 22050 || format_.freq == 44100, "Unsupported raw source rate");
        check(format_.channels == 1 || format_.channels == 2, "Unsupported raw source channels");
        decode_until(1);
        check(!pcm_.empty(), "Raw source decode is empty");
    }

    std::vector<std::int16_t> render(std::size_t frames) {
        std::vector<std::int16_t> pcm(frames * 2);
        const std::size_t repeat = format_.freq == 22050 ? 2 : 1;
        // One source frame of lookahead preserves mono interpolation and the
        // first-frame guard at EOF without decoding unused minutes of music.
        decode_until((position_ + frames) / repeat + 1);
        const auto source_frames = pcm_.size() / static_cast<std::size_t>(format_.channels);

        for (std::size_t frame = 0; frame < frames; ++frame, ++position_) {
            const auto source_frame = position_ / repeat;

            if (source_frame >= source_frames) {
                break;
            }

            for (std::size_t channel = 0; channel < 2; ++channel) {
                const auto source_channel = format_.channels == 1 ? 0 : channel;
                int value = pcm_[source_frame * static_cast<std::size_t>(format_.channels) + source_channel];

                if (format_.channels == 1 && repeat == 2 && position_ % 2 != 0) {
                    const int sum = value + pcm_[(source_frame + 1) % source_frames];
                    value = sum / 2 - (sum < 0 && sum % 2 != 0);
                }

                pcm[frame * 2 + channel] = static_cast<std::int16_t>(value);
            }
        }

        return pcm;
    }

private:
    void decode_until(std::size_t frames) {
        std::array<std::int16_t, 8192> block{};
        while (!eof_ && pcm_.size() / static_cast<std::size_t>(format_.channels) < frames) {
            const auto bytes = MIX_DecodeAudio(decoder_.get(), block.data(), sizeof(block), &format_);
            check(bytes >= 0 && bytes % static_cast<int>(format_.channels * sizeof(std::int16_t)) == 0, "Raw decoder returned an invalid PCM length");
            eof_ = bytes == 0;
            pcm_.insert(pcm_.end(), block.begin(), block.begin() + bytes / sizeof(std::int16_t));
        }
    }

    std::unique_ptr<MIX_AudioDecoder, DecoderDeleter> decoder_;
    SDL_AudioSpec format_{};
    std::vector<std::int16_t> pcm_;
    std::size_t position_{};
    bool eof_{};
};

const PanGain& vector_for(std::span<const PanGain> vectors, int channels, std::optional<float> pan, float gain) {
    const auto found = std::ranges::find_if(vectors, [&](const PanGain& value) {
        return value.channels == channels && value.pan == pan && value.gain == gain;
    });
    check(found != vectors.end(), "Required original pan/gain vector is absent");

    return *found;
}

void allocation_vectors(const std::filesystem::path& root) {
    std::size_t tested{};
    for (const auto& trace : allocation_traces) {
        std::set<std::string_view> selected;
        for (const auto& step : trace.steps) {
            const auto& command = step.command;
            if (step.finish_slots) {
                selected.insert(audio::asset_id(Sample::typing_click));
                selected.insert(audio::asset_id(Sample::shot));
            } else if (command.action == Action::select_stream && command.stream != Stream::none) {
                selected.insert(audio::asset_id(command.stream));
            } else if (command.action == Action::play_sample || command.action == Action::start_loop) {
                selected.insert(audio::asset_id(command.sample));
            }
        }
        const auto store = audio_test::load_assets(root, selected);
        audio::Engine engine(store, audio::OutputMode::offline, 44100);
        std::uint64_t sequence = 1;
        for (const auto& step : trace.steps) {
            const auto id = trace.id + "/" + std::to_string(sequence);
            if (step.finish_slots) {
                const auto click = store.audio(audio::asset_id(Sample::typing_click));
                check(click.sample_rate == 44100 && store.audio(audio::asset_id(Sample::shot)).decoded_frames > click.decoded_frames + 1024,
                    "Source EOF trace needs a short click before the neighboring shots finish");
                static_cast<void>(engine.render_frames(static_cast<std::size_t>(click.decoded_frames) + 1024));
            } else {
                auto command = step.command;
                command.sequence = sequence++;
                apply(engine, command);
            }
            const auto state = engine.state();
            check(state.active_channels == step.active_channels && state.channels_stolen == step.channels_stolen, "Source allocation outcome differs: " + id);
            if (step.selected_slot) {
                check(state.last_channel == *step.selected_slot, "Source circular allocation selected another slot: " + id);
            }
            ++tested;
        }
        apply(engine, {sequence++, Action::stop_samples});
        apply(engine, {sequence, Action::stop_stream});
        check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Capacity trace cleanup leaked audible channels");
    }
    check(tested > 100, "Source capacity traces lost their dense coverage");
    std::cout << "source allocation traces: " << tested << " outcomes passed\n";
}

void gain_and_pan_vectors(const assets::AssetStore& store, std::span<const PanGain> vectors) {
    RawDecoder mono_decoder(store, audio::asset_id(Sample::shot));
    const auto mono = mono_decoder.render(pcm_frames);
    RawDecoder stereo_decoder(store, audio::asset_id(Stream::title));
    const auto stereo = stereo_decoder.render(pcm_frames);
    RawDecoder stereo_sample_decoder(store, audio::asset_id(Sample::game_over));
    const auto stereo_sample = stereo_sample_decoder.render(pcm_frames);
    const auto audible = [](const auto& pcm) {
        return std::ranges::any_of(pcm, [](auto value) {
            return value != 0;
        });
    };
    check(audible(mono) && audible(stereo) && audible(stereo_sample), "Raw PCM fixtures contain only silence");
    audio::Engine mono_engine(store, audio::OutputMode::offline, 44100);
    audio::Engine stereo_engine(store, audio::OutputMode::offline, 44100);
    std::uint64_t mono_sequence = 1, stereo_sequence = 1;
    std::size_t checked{};

    for (const auto& vector : vectors) {
        if (vector.pan) {
            check(audio::quantize_pan(*vector.pan) == vector.pan_value, "Source pan quantization differs: " + vector.id);
        }

        if (vector.kind == "sample") {
            apply(mono_engine, {mono_sequence++, Action::stop_samples});
            apply(mono_engine, {mono_sequence++, Action::play_sample, Stream::none, vector.channels == 1 ? Sample::shot : Sample::game_over, vector.gain, vector.pan});
            matching_pcm(
                mono_engine.render_frames(pcm_frames), vector.channels == 1 ? mono : stereo_sample, vector.left, vector.right, "Source sample gain/pan PCM differs: " + vector.id);
        } else {
            apply(stereo_engine, {stereo_sequence++, Action::stop_stream});
            apply(stereo_engine, {stereo_sequence++, Action::select_stream, Stream::title});
            apply(stereo_engine, {stereo_sequence++, Action::set_stream_gain, Stream::none, Sample::shot, vector.gain});
            matching_pcm(stereo_engine.render_frames(pcm_frames), stereo, vector.left, vector.right, "Source stereo gain/sentinel PCM differs: " + vector.id);
        }

        ++checked;
    }

    std::size_t boundaries{};

    for (const auto& row : gain_cases) {
        const auto gain = real(row.input_bits);
        check(std::bit_cast<std::uint32_t>(audio::quantize_gain(gain)) == row.normalized_bits, "Source gain boundary differs: " + std::string(row.id));
        apply(stereo_engine, {stereo_sequence++, Action::stop_stream});
        apply(stereo_engine, {stereo_sequence++, Action::select_stream, Stream::title});
        apply(stereo_engine, {stereo_sequence++, Action::set_stream_gain, Stream::none, Sample::shot, gain});
        matching_pcm(stereo_engine.render_frames(pcm_frames), stereo, row.volume, row.volume, "Source gain boundary PCM differs: " + std::string(row.id));
        ++boundaries;
    }

    check(checked >= 60 && boundaries >= 30, "Source pan/gain tables lost boundary coverage");
    std::cout << "source pan/gain: " << checked << " raw PCM vectors and " << boundaries << " gain boundaries passed\n";
}

void gain_sequences(const assets::AssetStore& store) {
    audio::Engine engine(store, audio::OutputMode::offline, 44100);
    RawDecoder decoder(store, audio::asset_id(Stream::title));
    std::uint64_t sequence = 1;
    apply(engine, {sequence++, Action::select_stream, Stream::title});
    std::size_t normalizations{};

    for (std::size_t index = 0; index < normalization_bits.size(); ++index) {
        const int volume = static_cast<int>(index);
        const auto normalized = real(normalization_bits[index]);
        // Select a representable input above the independent byte/255 boundary.
        // This tests every output byte without reproducing the gain quantizer.
        const float gain = volume == 0 ? 0.0F : volume == 255 ? 1.0F : std::nextafter(normalized, std::numeric_limits<float>::infinity());
        apply(engine, {sequence++, Action::set_stream_gain, Stream::none, Sample::shot, gain});
        check(std::bit_cast<std::uint32_t>(engine.state().stream_gain) == normalization_bits[index],
            "Stream gain getter lost the source normalization byte " + std::to_string(volume));
        matching_pcm(engine.render_frames(64), decoder.render(64), volume, volume, "Source signed integer mixer PCM differs at byte " + std::to_string(volume));
        ++normalizations;
    }

    check(normalizations == 256 && engine.state().streams_started == 1, "Complete source normalization coverage restarted the stream");
    apply(engine, {sequence, Action::stop_stream});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(64)) == 0, "Normalization cleanup leaked PCM");

    std::size_t fade_steps{};
    audio::Engine fade(store, audio::OutputMode::offline, 44100);
    std::uint64_t fade_sequence = 1;

    for (const auto& trace : fade_cases) {
        const auto started_before = fade.state().streams_started;
        RawDecoder source(store, audio::asset_id(Stream::title));
        apply(fade, {fade_sequence++, Action::select_stream, Stream::title});
        apply(fade, {fade_sequence++, Action::set_stream_gain, Stream::none, Sample::shot, real(trace.initial_gain_bits)});

        for (const auto& step : trace.steps) {
            const auto id = trace.id + "/" + std::to_string(step.index);
            apply(fade, {fade_sequence++, Action::set_stream_gain, Stream::none, Sample::shot, real(step.stored_gain_bits)});
            const auto volume = step.volume;
            const auto expected_gain = normalization_bits.at(static_cast<std::size_t>(volume));
            check(std::bit_cast<std::uint32_t>(fade.state().stream_gain) == expected_gain, "Source sampled fade volume differs: " + id);
            matching_pcm(fade.render_frames(512), source.render(512), volume, volume, "Source sampled fade gain command changed PCM continuity: " + id);
            ++fade_steps;
        }

        check(fade.state().streams_started == started_before + 1, "Source sampled fade gain commands restarted playback");
        apply(fade, {fade_sequence++, Action::stop_stream});
        check(fade.state().active_channels == 0 && energy(fade.render_frames(64)) == 0, "Sampled fade cleanup leaked PCM");
    }

    check(fade_steps == 33, "Source sampled fade command vectors lost coverage");
    std::cout << "source stream gain: " << normalizations << " byte normalizations and " << fade_steps << " sampled fade command/continuity steps passed\n";
}

void loop_continuity(const assets::AssetStore& store) {
    audio::Engine engine(store, audio::OutputMode::offline, 44100);
    RawDecoder decoder(store, audio::asset_id(Sample::airplane_motor));
    std::uint64_t sequence = 1;
    auto compare = [&](const PanGain& expected, std::string_view message) {
        matching_pcm(engine.render_frames(pcm_frames), decoder.render(pcm_frames), expected.left, expected.right, message);
    };
    const auto& center = vector_for(pan_vectors, 1, {}, 1);
    const auto& left = vector_for(pan_vectors, 1, -1.0F, 1);
    const auto& left_half = vector_for(pan_vectors, 1, -1.0F, 0.5F);
    const auto& right_half = vector_for(pan_vectors, 1, 1.0F, 0.5F);
    const auto& center_half = vector_for(pan_vectors, 1, {}, 0.5F);
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor});
    compare(center, "Default mono loop PCM differs from the source center volumes");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor, 1, -1});
    compare(left, "Loop pan change restarted playback or changed the source left volume");
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0.5F});
    compare(left_half, "Loop gain change restarted playback or used another source volume");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor, 1, 1});
    compare(right_half, "Loop pan change lost its current gain or playback position");
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0});
    check(energy(engine.render_frames(pcm_frames)) == 0, "Muted effect loop remains audible");
    static_cast<void>(decoder.render(pcm_frames));
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0.5F});
    compare(right_half, "Muted effect loop failed to preserve source time");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor});
    compare(center_half, "Null loop pan failed to restore the source mono default");
    apply(engine, {sequence++, Action::pause});
    check(energy(engine.render_frames(pcm_frames)) == 0, "Paused effect loop remains audible");
    apply(engine, {sequence++, Action::resume});
    compare(center_half, "Paused effect loop consumed source frames");
    check(engine.state().loops_started == 1 && engine.state().active_loops == 1, "Changing loop gain, pan or pause allocated new playback");
    apply(engine, {sequence, Action::stop_loop, Stream::none, Sample::airplane_motor});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Loop continuity cleanup leaked PCM");
}

void retained_handle(const assets::AssetStore& store) {
    audio::Engine engine(store, audio::OutputMode::offline, 44100);
    RawDecoder replacement(store, audio::asset_id(Sample::shot));
    std::uint64_t sequence = 1;
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor, 0});

    for (int count = 0; count < 31; ++count) {
        apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 0});
    }

    const Command steal{sequence++, Action::play_sample, Stream::none, Sample::shot, 1, -1};
    apply(engine, steal);
    check(engine.state().last_channel == 0 && engine.state().active_loops == 0 && engine.state().active_samples == 32 && engine.state().channels_stolen == 1,
        "One-shot saturation did not replace the retained loop slot");
    const auto before_duplicate = engine.state();
    apply(engine, steal);
    check(engine.state().samples_started == before_duplicate.samples_started && engine.state().channels_stolen == before_duplicate.channels_stolen
              && engine.state().last_channel == before_duplicate.last_channel,
        "A repeated steal command changed the channel pool");
    const auto& left = vector_for(pan_vectors, 1, -1.0F, 1);
    matching_pcm(engine.render_frames(pcm_frames), replacement.render(pcm_frames), left.left, left.right, "Stolen channel did not play the replacement one-shot source");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor, 1, 1});
    const auto& right = vector_for(pan_vectors, 1, 1.0F, 1);
    matching_pcm(engine.render_frames(pcm_frames), replacement.render(pcm_frames), right.left, right.right, "Retained raw loop handle did not pan its replacement occupant");
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0.5F});
    const auto& right_half = vector_for(pan_vectors, 1, 1.0F, 0.5F);
    matching_pcm(engine.render_frames(pcm_frames), replacement.render(pcm_frames), right_half.left, right_half.right,
        "Retained raw loop handle did not change its replacement occupant gain");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor});
    const auto& center_half = vector_for(pan_vectors, 1, {}, 0.5F);
    matching_pcm(engine.render_frames(pcm_frames), replacement.render(pcm_frames), center_half.left, center_half.right,
        "Retained raw loop handle did not restore its replacement occupant mono pan");
    apply(engine, {sequence++, Action::stop_loop, Stream::none, Sample::airplane_motor});
    check(engine.state().active_samples == 31 && engine.state().active_loops == 0 && energy(engine.render_frames(pcm_frames)) == 0,
        "Retained raw loop stop did not stop its replacement one-shot");
    apply(engine, {sequence++, Action::stop_samples});
    check(engine.state().active_channels == 0 && engine.state().channel_tracks == 32 && engine.state().loop_tracks == 1, "Stolen handle cleanup changed the retained pool");
    // A released wrapper must no longer reach a later occupant of its old slot.
    for (int count = 0; count < 31; ++count) {
        apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 0});
    }

    apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 1, -1});
    check(engine.state().last_channel == 0 && engine.state().active_samples == 32, "Released raw handle test did not reuse its old slot");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor, 1, 1});
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0});
    apply(engine, {sequence++, Action::stop_loop, Stream::none, Sample::airplane_motor});
    RawDecoder later_occupant(store, audio::asset_id(Sample::shot));
    matching_pcm(
        engine.render_frames(pcm_frames), later_occupant.render(pcm_frames), left.left, left.right, "Released loop wrapper still changed a later occupant of its old raw slot");
    check(engine.state().active_samples == 32 && engine.state().active_loops == 0, "Released loop wrapper stopped a later occupant");
    apply(engine, {sequence++, Action::stop_samples});
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor});
    RawDecoder restarted(store, audio::asset_id(Sample::airplane_motor));
    const auto& center = vector_for(pan_vectors, 1, {}, 1);
    matching_pcm(engine.render_frames(pcm_frames), restarted.render(pcm_frames), center.left, center.right, "Restarting a stolen loop did not reset playback and defaults");
    check(engine.state().last_channel == 1 && engine.state().active_loops == 1 && engine.state().loop_tracks == 1 && engine.state().channels_stolen == 1,
        "Restarting a stolen loop did not follow the next free channel or retained wrapper policy");
    apply(engine, {sequence++, Action::stop_samples});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Restarted stale loop leaked PCM after cleanup");

    for (int count = 0; count < 31; ++count) {
        apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 0});
    }

    apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 1, -1});
    check(engine.state().last_channel == 1, "Effect cleanup handle test did not reuse the stopped loop slot");
    apply(engine, {sequence++, Action::set_loop_pan, Stream::none, Sample::airplane_motor, 1, 1});
    apply(engine, {sequence++, Action::set_loop_gain, Stream::none, Sample::airplane_motor, 0});
    apply(engine, {sequence++, Action::stop_loop, Stream::none, Sample::airplane_motor});
    RawDecoder after_cleanup(store, audio::asset_id(Sample::shot));
    matching_pcm(engine.render_frames(pcm_frames), after_cleanup.render(pcm_frames), left.left, left.right, "Stopping all effects failed to release a retained loop binding");
    check(engine.state().active_samples == 32 && engine.state().active_loops == 0, "Effect cleanup left a wrapper able to stop a later occupant");
    apply(engine, {sequence, Action::stop_samples});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Released wrapper cleanup leaked PCM");
}

void reserved_music(const assets::AssetStore& store) {
    audio::Engine engine(store, audio::OutputMode::offline, 44100);
    RawDecoder title(store, audio::asset_id(Stream::title));
    std::uint64_t sequence = 1;
    apply(engine, {sequence++, Action::select_stream, Stream::title});
    apply(engine, {sequence++, Action::set_stream_gain});

    for (int count = 0; count < 40; ++count) {
        apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 0});
    }

    check(engine.state().active_channels == 32 && engine.state().active_samples == 31 && engine.state().channels_stolen == 9,
        "Reserved music did not share and protect one of the thirty-two slots");
    matching_pcm(engine.render_frames(pcm_frames), title.render(pcm_frames), 255, 255, "Effect saturation stole or altered the reserved stereo stream");
    const auto position = engine.state().stream_position;
    apply(engine, {sequence++, Action::pause});
    apply(engine, {sequence++, Action::start_loop, Stream::none, Sample::airplane_motor, 0});
    check(engine.state().active_channels == 32 && engine.state().active_loops == 1 && engine.state().channels_stolen == 10,
        "Paused channels were treated as free during loop saturation");
    check(energy(engine.render_frames(pcm_frames)) == 0 && engine.state().stream_position == position, "Paused saturated music advanced or produced PCM");
    apply(engine, {sequence++, Action::resume});
    matching_pcm(engine.render_frames(pcm_frames), title.render(pcm_frames), 255, 255, "Resuming saturation did not preserve protected stream playback");
    apply(engine, {sequence++, Action::stop_samples});
    check(engine.state().active_channels == 1 && engine.state().active_samples == 0 && engine.state().active_loops == 0 && engine.state().stream == Stream::title,
        "Effect cleanup did not preserve the reserved stream");
    matching_pcm(engine.render_frames(pcm_frames), title.render(pcm_frames), 255, 255, "Effect cleanup changed reserved stereo PCM");
    apply(engine, {sequence, Action::stop_stream});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Stream cleanup left audible PCM");
}

void audible_overlap(const assets::AssetStore& store) {
    constexpr std::size_t frames = 16384;
    constexpr std::size_t first_slot = 13;
    audio::Engine engine(store, audio::OutputMode::offline, 44100);
    std::uint64_t sequence = 1;
    // Rotate the allocation cursor before filling all slots. Creation order
    // consequently differs from the permanent ascending mixer traversal.
    for (std::size_t slot = 0; slot < first_slot; ++slot) {
        apply(engine, {sequence++, Action::play_sample, Stream::none, Sample::shot, 0});
    }

    apply(engine, {sequence++, Action::stop_samples});
    RawDecoder title(store, audio::asset_id(Stream::title));
    RawDecoder shot(store, audio::asset_id(Sample::shot));
    RawDecoder game_over(store, audio::asset_id(Sample::game_over));
    RawDecoder chicken(store, audio::asset_id(Sample::chicken_hit));
    const std::array decoded{title.render(frames), shot.render(frames), game_over.render(frames), chicken.render(frames)};
    std::array<std::size_t, 32> source_for_slot{};
    std::array<std::array<int, 2>, 32> side_bytes{};
    apply(engine, {sequence++, Action::select_stream, Stream::title});
    apply(engine, {sequence++, Action::set_stream_gain});
    check(engine.state().last_channel == static_cast<int>(first_slot), "Overlap stream did not retain the rotated source slot");
    side_bytes[first_slot] = {255, 255};

    for (std::size_t voice = 1; voice < 32; ++voice) {
        const auto source = voice <= 15 ? std::size_t{1} : voice <= 23 ? std::size_t{2} : std::size_t{3};
        const auto sample = source == 1 ? Sample::shot : source == 2 ? Sample::game_over : Sample::chicken_hit;
        apply(engine, {sequence++, Action::play_sample, Stream::none, sample});
        const auto slot = (first_slot + voice) % source_for_slot.size();
        check(engine.state().last_channel == static_cast<int>(slot), "Overlap effect allocation changed the original circular slot order");
        source_for_slot[slot] = source;
        side_bytes[slot] = {127, 128};
    }

    check(engine.state().active_channels == 32 && engine.state().active_samples == 31 && engine.state().channels_stolen == 0,
        "Audible overlap did not fill the reserved music and thirty-one effect slots");
    std::vector<float> expected(frames * 2);
    std::size_t saturations{}, order_differences{}, final_clamp_differences{};

    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto channel = index % 2;
        int mixed{}, creation_order{}, unclamped{};

        for (std::size_t slot = 0; slot < source_for_slot.size(); ++slot) {
            const int contribution = side_sample(decoded[source_for_slot[slot]][index], side_bytes[slot][channel]);
            const int sum = mixed + contribution;
            mixed = std::clamp(sum, -32768, 32767);
            saturations += mixed != sum;
            unclamped += contribution;
            const auto created_slot = (first_slot + slot) % source_for_slot.size();
            creation_order = std::clamp(creation_order + side_sample(decoded[source_for_slot[created_slot]][index], side_bytes[created_slot][channel]), -32768, 32767);
        }

        order_differences += mixed != creation_order;
        final_clamp_differences += mixed != std::clamp(unclamped, -32768, 32767);
        expected[index] = static_cast<float>(mixed) / 32768.0F;
    }

    check(saturations > 0 && order_differences > 0 && final_clamp_differences > 0,
        "Decoded overlap corpus lost clipping or ceased to distinguish slot traversal and per-voice saturation");
    const auto actual = engine.render_frames(frames);
    check(actual.size() == expected.size() && std::ranges::equal(actual, expected), "Audible overlap PCM differs from independent ascending-slot signed saturated mixing");
    apply(engine, {sequence++, Action::stop_samples});
    apply(engine, {sequence, Action::stop_stream});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Audible overlap cleanup leaked PCM");
    std::cout << "audible overlap: " << saturations << " saturated additions, " << order_differences << " allocation-order differences and " << final_clamp_differences
              << " final-only-clamp differences; exact ascending-slot PCM passed\n";
}

void atomic_saturation(const assets::AssetStore& store) {
    audio::Engine engine(store, audio::OutputMode::offline, 44100);

    for (std::uint64_t sequence = 1; sequence <= 32; ++sequence) {
        apply(engine, {sequence, Action::play_sample, Stream::none, Sample::shot, 0});
    }

    const auto before = engine.state();
    const std::array invalid{Command{34, Action::play_sample}, Command{33, Action::play_sample}};
    bool rejected{};
    try {
        engine.apply(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    check(rejected && engine.state().last_sequence == before.last_sequence && engine.state().samples_started == before.samples_started
              && engine.state().channels_stolen == before.channels_stolen && engine.state().last_channel == before.last_channel && engine.state().active_channels == 32
              && energy(engine.render_frames(pcm_frames)) == 0,
        "Invalid batch partially stole from a saturated pool");
    const std::array duplicate{Command{33, Action::play_sample, Stream::none, Sample::shot, 0}, Command{33, Action::play_sample, Stream::none, Sample::shot, 1},
        Command{34, Action::play_sample, Stream::none, Sample::shot, 0}};
    engine.apply(duplicate);
    engine.apply(duplicate);
    check(engine.state().samples_started == 34 && engine.state().commands_consumed == 34 && engine.state().channels_stolen == 2 && engine.state().last_channel == 1
              && engine.state().active_channels == 32 && energy(engine.render_frames(pcm_frames)) == 0,
        "Duplicate sequence events changed saturated playback or cursor progression");
    apply(engine, {35, Action::stop_samples});
    check(engine.state().active_channels == 0 && energy(engine.render_frames(pcm_frames)) == 0, "Atomic saturation cleanup leaked PCM");
}

void policy(const std::filesystem::path& root) {
    MixerLifetime lifetime;
    const auto shot = audio::asset_id(Sample::shot), title = audio::asset_id(Stream::title), airplane = audio::asset_id(Sample::airplane_motor);
    allocation_vectors(root);
    gain_and_pan_vectors(audio_test::load_assets(root, {shot, title, audio::asset_id(Sample::game_over)}), pan_vectors);
    gain_sequences(audio_test::load_assets(root, {title}));
    loop_continuity(audio_test::load_assets(root, {airplane}));
    retained_handle(audio_test::load_assets(root, {shot, airplane}));
    reserved_music(audio_test::load_assets(root, {shot, title, airplane}));
    audible_overlap(audio_test::load_assets(root, {shot, title, audio::asset_id(Sample::game_over), audio::asset_id(Sample::chicken_hit)}));
    atomic_saturation(audio_test::load_assets(root, {shot}));
}

} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 3, "Usage: audio_tests ASSETS offline|dummy|unavailable|policy");
        const std::string_view mode(argv[2]);
        if (mode == "policy") {
            policy(argv[1]);
            return 0;
        }
        const auto store = assets::AssetStore::load_directory(argv[1]);
        if (mode == "offline") {
            loading(store);
            offline(store);
            flying_audio(store);
        } else if (mode == "dummy") {
            dummy(store);
        } else if (mode == "unavailable") {
            unavailable(store);
        } else {
            throw std::runtime_error("unknown test mode");
        }

        return 0;
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';

        return 1;
    }
}
