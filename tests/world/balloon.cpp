#include "world/balloon.hpp"

#include "support/numeric.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace moorhuhn2;
using moorhuhn2::world::Balloon;

namespace {

void check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

using moorhuhn2::test::binary32;

struct SpawnCase {
    std::array<std::uint32_t, 5> random_values;
    float camera;
    world::Balloon after;
};

struct SpawnExpectation {
    std::array<std::uint32_t, 5> random_values;
    world::Balloon after;
};

struct SpawnBatch {
    float camera;
    std::vector<SpawnExpectation> balloons;
};

struct UpdateCase {
    world::Balloon before;
    float delta_seconds;
    float camera;
    int recoil;
    int drift;
    world::Balloon after;
};

struct UpdateStep {
    float delta_seconds;
    int drift;
    world::Balloon after;
};

struct SequenceCase {
    world::Balloon before;
    float camera;
    int recoil;
    std::vector<UpdateStep> steps;
};

struct ImpactCase {
    world::Balloon before;
    std::array<std::uint32_t, 2> random_values;
    world::Balloon after;
};

const std::array<SpawnCase, 8> initializers_cases = {{SpawnCase{{{0U, 0U, 0U, 0U, 1U}}, 0.0F, world::Balloon{21, 0.0F, 340.0F, -1024.0F, -1024.0F, -8.0F, 20.0F, 0, 1}},
    SpawnCase{{{639U, 1U, 31U, 59U, 99U}}, 100.625F, world::Balloon{21, binary32(0x4442f800U), 340.0F, -1024.0F, -1024.0F, 23.0F, 79.0F, 1, 2}},
    SpawnCase{{{640U, 2U, 32U, 60U, 101U}}, 640.0F, world::Balloon{21, 896.0F, 340.0F, -1024.0F, -1024.0F, -8.0F, 20.0F, 2, 3}},
    SpawnCase{{{32767U, 3U, 32767U, 32767U, 32767U}}, 1280.0F, world::Balloon{21, 1919.0F, 340.0F, -1024.0F, -1024.0F, 23.0F, 27.0F, 3, 4}},
    SpawnCase{{{639U, 3U, 8U, 0U, 0U}}, 640.0F, world::Balloon{21, 1535.0F, 340.0F, -1024.0F, -1024.0F, 0.0F, 20.0F, 4, 100}},
    SpawnCase{{{12345U, 1U, 17U, 42U, 100U}}, 100.625F, world::Balloon{21, binary32(0x43a2f000U), 340.0F, -1024.0F, -1024.0F, 9.0F, 62.0F, 4, 100}},
    SpawnCase{{{0U, 0U, 0U, 0U, 1U}}, 99.99998474121094F, world::Balloon{21, binary32(0x430bfffeU), 340.0F, -1024.0F, -1024.0F, -8.0F, 20.0F, 0, 1}},
    SpawnCase{{{0U, 0U, 0U, 0U, 1U}}, 100.0F, world::Balloon{21, 140.0F, 340.0F, -1024.0F, -1024.0F, -8.0F, 20.0F, 0, 1}}}};

const std::array<SpawnBatch, 4> spawn_batches_cases = {
    {SpawnBatch{0.0F, {SpawnExpectation{{{41U, 18467U, 6334U, 26500U, 19169U}}, world::Balloon{21, 41.0F, 340.0F, -1024.0F, -1024.0F, 22.0F, 60.0F, 3, 4}},
                          SpawnExpectation{{{15724U, 11478U, 29358U, 26962U, 24464U}}, world::Balloon{21, 364.0F, 340.0F, -1024.0F, -1024.0F, 6.0F, 42.0F, 2, 3}},
                          SpawnExpectation{{{5705U, 28145U, 23281U, 16827U, 9961U}}, world::Balloon{21, 585.0F, 340.0F, -1024.0F, -1024.0F, 9.0F, 47.0F, 1, 2}},
                          SpawnExpectation{{{491U, 2995U, 11942U, 4827U, 5436U}}, world::Balloon{21, 491.0F, 340.0F, -1024.0F, -1024.0F, -2.0F, 47.0F, 3, 4}},
                          SpawnExpectation{{{32391U, 14604U, 3902U, 153U, 292U}}, world::Balloon{21, 391.0F, 340.0F, -1024.0F, -1024.0F, 22.0F, 53.0F, 0, 1}},
                          SpawnExpectation{{{12382U, 17421U, 18716U, 19718U, 19895U}}, world::Balloon{21, 222.0F, 340.0F, -1024.0F, -1024.0F, 20.0F, 58.0F, 1, 2}},
                          SpawnExpectation{{{5447U, 21726U, 14771U, 11538U, 1869U}}, world::Balloon{21, 327.0F, 340.0F, -1024.0F, -1024.0F, 11.0F, 38.0F, 2, 3}},
                          SpawnExpectation{{{19912U, 25667U, 26299U, 17035U, 9894U}}, world::Balloon{21, 72.0F, 340.0F, -1024.0F, -1024.0F, 19.0F, 75.0F, 3, 4}},
                          SpawnExpectation{{{28703U, 23811U, 31322U, 30333U, 17673U}}, world::Balloon{21, 543.0F, 340.0F, -1024.0F, -1024.0F, 18.0F, 53.0F, 3, 4}},
                          SpawnExpectation{{{4664U, 15141U, 7711U, 28253U, 6868U}}, world::Balloon{21, 184.0F, 340.0F, -1024.0F, -1024.0F, 23.0F, 73.0F, 1, 2}},
                          SpawnExpectation{{{25547U, 27644U, 32662U, 32757U, 20037U}}, world::Balloon{21, 587.0F, 340.0F, -1024.0F, -1024.0F, 14.0F, 77.0F, 0, 1}},
                          SpawnExpectation{{{12859U, 8723U, 9741U, 27529U, 778U}}, world::Balloon{21, 59.0F, 340.0F, -1024.0F, -1024.0F, 5.0F, 69.0F, 3, 4}},
                          SpawnExpectation{{{12316U, 3035U, 22190U, 1842U, 288U}}, world::Balloon{21, 156.0F, 340.0F, -1024.0F, -1024.0F, 6.0F, 62.0F, 3, 4}},
                          SpawnExpectation{{{30106U, 9040U, 8942U, 19264U, 22648U}}, world::Balloon{21, 26.0F, 340.0F, -1024.0F, -1024.0F, 6.0F, 24.0F, 0, 1}},
                          SpawnExpectation{{{27446U, 23805U, 15890U, 6729U, 24370U}}, world::Balloon{21, 566.0F, 340.0F, -1024.0F, -1024.0F, 10.0F, 29.0F, 1, 2}},
                          SpawnExpectation{{{15350U, 15006U, 31101U, 24393U, 3548U}}, world::Balloon{21, 630.0F, 340.0F, -1024.0F, -1024.0F, 21.0F, 53.0F, 2, 3}}}},
        SpawnBatch{640.0F, {SpawnExpectation{{{175U, 400U, 17869U, 30056U, 16083U}}, world::Balloon{21, 1071.0F, 340.0F, -1024.0F, -1024.0F, 5.0F, 76.0F, 0, 1}},
                               SpawnExpectation{{{12879U, 8016U, 7644U, 15809U, 1769U}}, world::Balloon{21, 975.0F, 340.0F, -1024.0F, -1024.0F, 20.0F, 49.0F, 0, 1}},
                               SpawnExpectation{{{32409U, 29950U, 13471U, 7099U, 2336U}}, world::Balloon{21, 1305.0F, 340.0F, -1024.0F, -1024.0F, 23.0F, 39.0F, 2, 3}},
                               SpawnExpectation{{{23225U, 21188U, 13712U, 14351U, 19676U}}, world::Balloon{21, 1081.0F, 340.0F, -1024.0F, -1024.0F, 8.0F, 31.0F, 0, 1}},
                               SpawnExpectation{{{8907U, 26730U, 29686U, 25607U, 9190U}}, world::Balloon{21, 1483.0F, 340.0F, -1024.0F, -1024.0F, 14.0F, 67.0F, 2, 3}},
                               SpawnExpectation{{{7040U, 6336U, 6091U, 3445U, 3715U}}, world::Balloon{21, 896.0F, 340.0F, -1024.0F, -1024.0F, 3.0F, 45.0F, 0, 1}},
                               SpawnExpectation{{{30182U, 3597U, 28888U, 17445U, 7906U}}, world::Balloon{21, 998.0F, 340.0F, -1024.0F, -1024.0F, 16.0F, 65.0F, 1, 2}},
                               SpawnExpectation{{{31147U, 11649U, 30172U, 8167U, 19555U}}, world::Balloon{21, 1323.0F, 340.0F, -1024.0F, -1024.0F, 20.0F, 27.0F, 1, 2}},
                               SpawnExpectation{{{27469U, 29923U, 27343U, 16368U, 3860U}}, world::Balloon{21, 1485.0F, 340.0F, -1024.0F, -1024.0F, 7.0F, 68.0F, 3, 4}},
                               SpawnExpectation{{{7299U, 4181U, 31349U, 23935U, 14736U}}, world::Balloon{21, 1155.0F, 340.0F, -1024.0F, -1024.0F, 13.0F, 75.0F, 1, 2}},
                               SpawnExpectation{{{6532U, 21023U, 16409U, 2870U, 5487U}}, world::Balloon{21, 1028.0F, 340.0F, -1024.0F, -1024.0F, 17.0F, 70.0F, 3, 4}},
                               SpawnExpectation{{{29702U, 24745U, 16630U, 12067U, 29269U}}, world::Balloon{21, 1158.0F, 340.0F, -1024.0F, -1024.0F, 14.0F, 27.0F, 1, 2}},
                               SpawnExpectation{{{3776U, 6635U, 24538U, 32171U, 9102U}}, world::Balloon{21, 1472.0F, 340.0F, -1024.0F, -1024.0F, 18.0F, 31.0F, 3, 4}},
                               SpawnExpectation{{{2386U, 4465U, 19905U, 22521U, 1097U}}, world::Balloon{21, 1362.0F, 340.0F, -1024.0F, -1024.0F, -7.0F, 41.0F, 1, 2}},
                               SpawnExpectation{{{17692U, 32076U, 16625U, 23767U, 4697U}}, world::Balloon{21, 1308.0F, 340.0F, -1024.0F, -1024.0F, 9.0F, 27.0F, 0, 1}},
                               SpawnExpectation{{{23845U, 26109U, 20013U, 4642U, 679U}}, world::Balloon{21, 1061.0F, 340.0F, -1024.0F, -1024.0F, 5.0F, 42.0F, 1, 2}}}},
        SpawnBatch{
            100.625F, {SpawnExpectation{{{7584U, 19164U, 25795U, 22125U, 5828U}}, world::Balloon{21, binary32(0x442b3800U), 340.0F, -1024.0F, -1024.0F, -5.0F, 65.0F, 0, 1}},
                          SpawnExpectation{{{23405U, 27477U, 5413U, 29072U, 23404U}}, world::Balloon{21, binary32(0x43fcf000U), 340.0F, -1024.0F, -1024.0F, -3.0F, 52.0F, 1, 2}},
                          SpawnExpectation{{{14959U, 14800U, 20443U, 21732U, 9534U}}, world::Balloon{21, binary32(0x43bdf000U), 340.0F, -1024.0F, -1024.0F, 19.0F, 32.0F, 0, 1}},
                          SpawnExpectation{{{14076U, 21966U, 10216U, 26075U, 3949U}}, world::Balloon{21, binary32(0x44423800U), 340.0F, -1024.0F, -1024.0F, 0.0F, 55.0F, 2, 3}},
                          SpawnExpectation{{{10218U, 8325U, 11118U, 15284U, 2636U}}, world::Balloon{21, binary32(0x443db800U), 340.0F, -1024.0F, -1024.0F, 6.0F, 64.0F, 1, 2}}}},
        SpawnBatch{640.0F, {}}}};

const std::array<UpdateCase, 22> updates_cases = {
    {UpdateCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.25F, 640.0F, 0, 0,
         world::Balloon{21, binary32(0x44954800U), binary32(0x43a02000U), binary32(0x4395a000U), binary32(0x43a02000U), 23.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, -8.0F, 79.0F, 3, 4}, 0.25F, 640.0F, 0, 0,
            world::Balloon{21, 1202.0F, binary32(0x43a02000U), 307.0F, binary32(0x43a02000U), -8.0F, 79.0F, 3, 4}},
        UpdateCase{
            world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.0F, 640.0F, 0, 0, world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 0.0F, 79.0F, 3, 4}, 0.125F, 640.0F, 0, 0,
            world::Balloon{21, 1200.0F, binary32(0x43a51000U), 305.0F, binary32(0x43a51000U), 0.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.125F, 640.0F, 8, 0,
            world::Balloon{21, binary32(0x4495a400U), binary32(0x43a51000U), binary32(0x43971000U), binary32(0x43a84333U), 23.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.125F, 100.625F, 0, 0,
            world::Balloon{21, binary32(0x4495a400U), binary32(0x43a51000U), binary32(0x44842400U), binary32(0x43a51000U), 23.0F, 79.0F, 3, 4}},
        UpdateCase{
            world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.0F, 640.0F, 0, 0, world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}},
        UpdateCase{
            world::Balloon{21, 1200.0F, -42.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.0F, 640.0F, 0, 0, world::Balloon{21, 1200.0F, -42.0F, 305.0F, -42.0F, 23.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, -42.000003814697266F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 0.0F, 640.0F, 0, 0,
            world::Balloon{-1, 1200.0F, binary32(0xc2280001U), 305.0F, binary32(0xc2280001U), 23.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, -40.0F, 305.0F, 340.0F, 23.0F, 20.0F, 3, 4}, 0.125F, 640.0F, 0, 0,
            world::Balloon{-1, binary32(0x4495a400U), binary32(0xc22a0000U), binary32(0x43971000U), binary32(0xc22a0000U), 23.0F, 20.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, -42.0F, 305.0F, 340.0F, 23.0F, 20.0F, 3, 4}, 2.9802322387695312e-08F, 640.0F, 0, 0,
            world::Balloon{21, 1200.0F, -42.0F, 305.0F, -42.0F, 23.0F, 20.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, -42.0F, 305.0F, 340.0F, 23.0F, 20.0F, 3, 4}, 1.1920928955078125e-07F, 640.0F, 0, 0,
            world::Balloon{-1, 1200.0F, binary32(0xc2280001U), 305.0F, binary32(0xc2280001U), 23.0F, 20.0F, 3, 4}},
        UpdateCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 4, 100}, 1.0F, 640.0F, 0, 0,
            world::Balloon{21, 1177.0F, 261.0F, 282.0F, 261.0F, 23.0F, 79.0F, 4, 100}},
        UpdateCase{world::Balloon{21, 895.5F, 340.0F, 305.0F, 340.0F, 0.0F, 79.0F, 3, 4}, 0.125F, 640.0F, 0, 0,
            world::Balloon{21, binary32(0x445fe000U), binary32(0x43a51000U), binary32(0x3f000000U), binary32(0x43a51000U), 0.0F, 79.0F, 3, 4}},
        UpdateCase{world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 8, 4}, 0.25F, 640.0F, 0, 0,
            world::Balloon{61, 1200.0F, binary32(0x43ad2000U), 305.0F, binary32(0x43ad2000U), 23.0F, 79.0F, 8, 4}},
        UpdateCase{
            world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 8, 4}, 0.0F, 640.0F, 0, -1, world::Balloon{61, 1199.0F, 340.0F, 304.0F, 340.0F, 23.0F, 79.0F, 8, 4}},
        UpdateCase{
            world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 8, 4}, 0.0F, 640.0F, 0, 0, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 8, 4}},
        UpdateCase{
            world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 8, 4}, 0.0F, 640.0F, 0, 1, world::Balloon{61, 1201.0F, 340.0F, 306.0F, 340.0F, 23.0F, 79.0F, 8, 4}},
        UpdateCase{world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 9, 100}, 0.25F, 640.0F, 0, -1,
            world::Balloon{61, 1199.0F, binary32(0x43ad2000U), 304.0F, binary32(0x43ad2000U), 23.0F, 79.0F, 9, 100}},
        UpdateCase{world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 9.0F, 19.0F, 5, 4}, 0.25F, 640.0F, 0, 0,
            world::Balloon{61, 1200.0F, binary32(0x43ad2000U), 305.0F, binary32(0x43ad2000U), 9.0F, 19.0F, 5, 4}},
        UpdateCase{
            world::Balloon{61, 1200.0F, 480.0F, 305.0F, 340.0F, 23.0F, 79.0F, 5, 4}, 0.0F, 640.0F, 0, 0, world::Balloon{61, 1200.0F, 480.0F, 305.0F, 480.0F, 23.0F, 79.0F, 5, 4}},
        UpdateCase{world::Balloon{61, 1200.0F, 500.0F, 305.0F, 340.0F, 23.0F, 79.0F, 9, 100}, 90.0F, 640.0F, 8, 0,
            world::Balloon{61, 1200.0F, 2750.0F, 305.0F, binary32(0x452c4666U), 23.0F, 79.0F, 9, 100}}}};

const std::array<SequenceCase, 4> sequences_cases = {{SequenceCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 640.0F, 0,
                                                          {UpdateStep{1.0F, 0, world::Balloon{21, 1177.0F, 261.0F, 282.0F, 261.0F, 23.0F, 79.0F, 3, 4}}}},
    SequenceCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 640.0F, 0,
        {UpdateStep{0.5F, 0, world::Balloon{21, binary32(0x44949000U), binary32(0x43964000U), binary32(0x4392c000U), binary32(0x43964000U), 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, 1177.0F, 261.0F, 282.0F, 261.0F, 23.0F, 79.0F, 3, 4}}}},
    SequenceCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, 640.0F, 0,
        {UpdateStep{0.5F, 0, world::Balloon{21, binary32(0x44949000U), binary32(0x43964000U), binary32(0x4392c000U), binary32(0x43964000U), 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, 1177.0F, 261.0F, 282.0F, 261.0F, 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, binary32(0x4491b000U), binary32(0x435d8000U), binary32(0x43874000U), binary32(0x435d8000U), 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, 1154.0F, 182.0F, 259.0F, 182.0F, 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, binary32(0x448ed000U), binary32(0x430e8000U), binary32(0x43778000U), binary32(0x430e8000U), 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, 1131.0F, 103.0F, 236.0F, 103.0F, 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, binary32(0x448bf000U), binary32(0x427e0000U), binary32(0x43608000U), binary32(0x427e0000U), 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, 1108.0F, 24.0F, 213.0F, 24.0F, 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{21, binary32(0x44891000U), binary32(0xc1780000U), binary32(0x43498000U), binary32(0xc1780000U), 23.0F, 79.0F, 3, 4}},
            UpdateStep{0.5F, 0, world::Balloon{-1, 1085.0F, -55.0F, 190.0F, -55.0F, 23.0F, 79.0F, 3, 4}}}},
    SequenceCase{world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 8, 4}, 640.0F, 0,
        {UpdateStep{5.0F, 0, world::Balloon{61, 1200.0F, 465.0F, 305.0F, 465.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, -1, world::Balloon{61, 1199.0F, 590.0F, 304.0F, 590.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, 0, world::Balloon{61, 1199.0F, 715.0F, 304.0F, 715.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, 1, world::Balloon{61, 1200.0F, 840.0F, 305.0F, 840.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, 0, world::Balloon{61, 1200.0F, 965.0F, 305.0F, 965.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, 0, world::Balloon{61, 1200.0F, 1090.0F, 305.0F, 1090.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, 0, world::Balloon{61, 1200.0F, 1215.0F, 305.0F, 1215.0F, 23.0F, 79.0F, 8, 4}},
            UpdateStep{5.0F, 0, world::Balloon{61, 1200.0F, 1340.0F, 305.0F, 1340.0F, 23.0F, 79.0F, 8, 4}}}}}};

const std::array<ImpactCase, 10> impacts_cases = {{ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 0, 1}, {{0U, 0U}},
                                                       world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 0.0F, 0.0F, 5, 1}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 0, 1}, {{32767U, 32767U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 7.0F, 7.0F, 5, 1}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 1, 2}, {{0U, 0U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 0.0F, 0.0F, 6, 2}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 1, 2}, {{32767U, 32767U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 7.0F, 7.0F, 6, 2}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 2, 3}, {{0U, 0U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 0.0F, 0.0F, 7, 3}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 2, 3}, {{32767U, 32767U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 7.0F, 7.0F, 7, 3}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, {{0U, 0U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 0.0F, 0.0F, 8, 4}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 3, 4}, {{32767U, 32767U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 7.0F, 7.0F, 8, 4}},
    ImpactCase{world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 4, 100}, {{0U, 0U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 0.0F, 0.0F, 9, 100}},
    ImpactCase{
        world::Balloon{21, 1200.0F, 340.0F, 305.0F, 340.0F, 23.0F, 79.0F, 4, 100}, {{32767U, 32767U}}, world::Balloon{61, 1200.0F, 340.0F, 305.0F, 340.0F, 7.0F, 7.0F, 9, 100}}}};

void check_state(const Balloon& actual, const Balloon& expected, std::string_view context) {
    check(actual == expected, std::string(context) + ": original state differs");
    check(std::bit_cast<std::uint32_t>(actual.world_x) == std::bit_cast<std::uint32_t>(expected.world_x)
              && std::bit_cast<std::uint32_t>(actual.world_y) == std::bit_cast<std::uint32_t>(expected.world_y)
              && std::bit_cast<std::uint32_t>(actual.screen_x) == std::bit_cast<std::uint32_t>(expected.screen_x)
              && std::bit_cast<std::uint32_t>(actual.screen_y) == std::bit_cast<std::uint32_t>(expected.screen_y)
              && std::bit_cast<std::uint32_t>(actual.velocity_x) == std::bit_cast<std::uint32_t>(expected.velocity_x)
              && std::bit_cast<std::uint32_t>(actual.velocity_y) == std::bit_cast<std::uint32_t>(expected.velocity_y),
        std::string(context) + ": original binary32 words differ");
}

template <typename Function> void rejects(Function function, std::string_view message) {
    bool rejected = false;
    try {
        function();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, message);
}

} // namespace

int main() {
    try {
        for (const auto& vector : initializers_cases) {
            const auto draws = vector.random_values;
            const auto actual = moorhuhn2::world::make_balloon(vector.camera, draws);
            check_state(actual, vector.after, "Balloon initializer");
        }
        for (const auto& batch : spawn_batches_cases) {
            for (const auto& vector : batch.balloons) {
                const auto draws = vector.random_values;
                const auto actual = moorhuhn2::world::make_balloon(batch.camera, draws);
                check_state(actual, vector.after, "Balloon spawn batch");
            }
        }
        for (const auto& vector : updates_cases) {
            auto actual = vector.before;
            moorhuhn2::world::update_balloon(actual, vector.delta_seconds, vector.camera, vector.recoil, vector.drift);
            check_state(actual, vector.after, "Balloon update");
        }
        for (const auto& sequence : sequences_cases) {
            auto actual = sequence.before;
            for (const auto& step : sequence.steps) {
                moorhuhn2::world::update_balloon(actual, step.delta_seconds, sequence.camera, sequence.recoil, step.drift);
                check_state(actual, step.after, "Balloon update sequence");
            }
        }
        for (const auto& vector : impacts_cases) {
            auto actual = vector.before;
            const auto draws = vector.random_values;
            moorhuhn2::world::hit_balloon(actual, draws[0], draws[1]);
            check_state(actual, vector.after, "Balloon impact");
            const auto before_repeated_hit = actual;
            rejects(
                [&] {
                    moorhuhn2::world::hit_balloon(actual, 0, 0);
                },
                "Dying balloon accepted a repeated impact");
            check(actual == before_repeated_hit, "Rejected impact mutated the dying balloon");
        }

        for (int frame = 0; frame < 10; ++frame) {
            Balloon balloon;
            balloon.type = frame < 5 ? 21 : 61;
            balloon.frame = frame;
            balloon.screen_x = -0.75F;
            balloon.screen_y = -1.75F;
            const auto draw = moorhuhn2::world::balloon_draw(balloon);
            check(draw.image == "gfx/ballon" && draw.source.x == frame % 5 * 22 && draw.source.y == frame / 5 * 42 && draw.source.width == 22 && draw.source.height == 42
                      && draw.destination.x == 0 && draw.destination.y == -1 && draw.color_key && !draw.mirror_x,
                "Original balloon source cell or truncated pose differs");
        }
        Balloon inactive;
        rejects(
            [&] {
                moorhuhn2::world::update_balloon(inactive, 0, 0, 0);
            },
            "Inactive balloon accepted an update");
        rejects(
            [&] {
                static_cast<void>(moorhuhn2::world::balloon_draw(inactive));
            },
            "Inactive balloon accepted a draw");
        rejects(
            [] {
                static_cast<void>(moorhuhn2::world::make_balloon(0, {0, 0, 0, 0, 32768}));
            },
            "Invalid CRT draw accepted");
        std::cout << "Original balloon initialization, stored motion, strict removal, impacts, retained death and all10 sprite cells passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
