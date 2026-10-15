#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace moorhuhn2;

namespace {
using moorhuhn2::test::require;

struct InitialLeaf {
    std::size_t slot;
    float x;
    float y;
    std::uint32_t phase_bits;
    float amplitude;
};

struct SetupCase {
    std::uint32_t seed;
    std::uint64_t draws;
    std::uint32_t state;
    std::array<InitialLeaf, 32> leaves;
};

struct LeafDrift {
    std::size_t slot;
    float world_x;
    float amplitude;
};

struct DriftCase {
    std::uint32_t seed;
    std::uint64_t draws;
    std::uint32_t state;
    std::array<LeafDrift, 32> leaves;
};

struct SpawnExpectation {
    std::uint64_t first_draws;
    std::uint32_t first_state;
    int type;
    float x;
    float y;
    float wait;
    std::uint64_t draws;
    std::uint32_t state;
};

const std::array<SetupCase, 3> setup_cases = {
    {SetupCase{0U, 176ULL, 47163440U,
         {{InitialLeaf{0U, 708.0F, 267.0F, 0x3de147aeU, 5.0F}, InitialLeaf{1U, 759.0F, 249.0F, 0x3eccccccU, 3.0F}, InitialLeaf{2U, 729.0F, 270.0F, 0x3f2147aeU, 7.0F},
             InitialLeaf{3U, 745.0F, 280.0F, 0x3f051eb8U, 7.0F}, InitialLeaf{4U, 726.0F, 282.0F, 0x3e051eb8U, 4.0F}, InitialLeaf{5U, 755.0F, 241.0F, 0x3d23d70aU, 1.0F},
             InitialLeaf{6U, 741.0F, 248.0F, 0x3e8a3d70U, 0.0F}, InitialLeaf{7U, 734.0F, 296.0F, 0x3ef5c28fU, 6.0F}, InitialLeaf{8U, 720.0F, 260.0F, 0x3e2e147bU, 0.0F},
             InitialLeaf{9U, 730.0F, 244.0F, 0x3e2e147bU, 5.0F}, InitialLeaf{10U, 711.0F, 303.0F, 0x3e8f5c29U, 6.0F}, InitialLeaf{11U, 759.0F, 287.0F, 0x3d8f5c29U, 4.0F},
             InitialLeaf{12U, 733.0F, 256.0F, 0x3edc28f5U, 0.0F}, InitialLeaf{13U, 747.0F, 266.0F, 0x3f051eb8U, 1.0F}, InitialLeaf{14U, 715.0F, 271.0F, 0x3f07ae14U, 1.0F},
             InitialLeaf{15U, 720.0F, 294.0F, 0x3f2147aeU, 1.0F}, InitialLeaf{16U, 748.0F, 265.0F, 0x3efae147U, 6.0F}, InitialLeaf{17U, 739.0F, 252.0F, 0x3e2e147bU, 4.0F},
             InitialLeaf{18U, 703.0F, 303.0F, 0x3e8f5c29U, 3.0F}, InitialLeaf{19U, 710.0F, 294.0F, 0x3e8a3d70U, 6.0F}, InitialLeaf{20U, 745.0F, 279.0F, 0x3f0ccccdU, 3.0F},
             InitialLeaf{21U, 702.0F, 276.0F, 0x3e0f5c29U, 0.0F}, InitialLeaf{22U, 752.0F, 297.0F, 0x3d75c28fU, 3.0F}, InitialLeaf{23U, 758.0F, 264.0F, 0x3eae147bU, 6.0F},
             InitialLeaf{24U, 705.0F, 291.0F, 0x3e428f5cU, 0.0F}, InitialLeaf{25U, 762.0F, 282.0F, 0x3e23d70aU, 7.0F}, InitialLeaf{26U, 747.0F, 278.0F, 0x3eccccccU, 4.0F},
             InitialLeaf{27U, 700.0F, 245.0F, 0x3e6b851eU, 0.0F}, InitialLeaf{28U, 738.0F, 295.0F, 0x3e75c28fU, 6.0F}, InitialLeaf{29U, 723.0F, 280.0F, 0x3f170a3dU, 0.0F},
             InitialLeaf{30U, 701.0F, 271.0F, 0x3f0ccccdU, 7.0F}, InitialLeaf{31U, 759.0F, 278.0F, 0x3c23d70aU, 5.0F}}}},
        SetupCase{1U, 176ULL, 3721762801U,
            {{InitialLeaf{0U, 751.0F, 258.0F, 0x3e051eb8U, 0.0F}, InitialLeaf{1U, 703.0F, 299.0F, 0x3de147aeU, 6.0F}, InitialLeaf{2U, 731.0F, 243.0F, 0x3e851eb8U, 5.0F},
                InitialLeaf{3U, 709.0F, 296.0F, 0x3ebd70a4U, 7.0F}, InitialLeaf{4U, 729.0F, 260.0F, 0x3de147aeU, 4.0F}, InitialLeaf{5U, 722.0F, 293.0F, 0x3d4cccccU, 3.0F},
                InitialLeaf{6U, 719.0F, 253.0F, 0x3db851ebU, 2.0F}, InitialLeaf{7U, 728.0F, 267.0F, 0x3eeb851eU, 2.0F}, InitialLeaf{8U, 732.0F, 266.0F, 0x3e23d70aU, 6.0F},
                InitialLeaf{9U, 700.0F, 296.0F, 0x3f0a3d70U, 5.0F}, InitialLeaf{10U, 718.0F, 249.0F, 0x3f000000U, 6.0F}, InitialLeaf{11U, 730.0F, 301.0F, 0x3db851ebU, 4.0F},
                InitialLeaf{12U, 745.0F, 255.0F, 0x3e4cccccU, 2.0F}, InitialLeaf{13U, 704.0F, 240.0F, 0x3ec28f5cU, 0.0F}, InitialLeaf{14U, 743.0F, 244.0F, 0x3ef5c28fU, 7.0F},
                InitialLeaf{15U, 750.0F, 299.0F, 0x3ea8f5c2U, 2.0F}, InitialLeaf{16U, 754.0F, 274.0F, 0x3e2e147bU, 5.0F}, InitialLeaf{17U, 733.0F, 251.0F, 0x3e9eb852U, 2.0F},
                InitialLeaf{18U, 748.0F, 250.0F, 0x3e800000U, 2.0F}, InitialLeaf{19U, 757.0F, 290.0F, 0x3e947ae1U, 1.0F}, InitialLeaf{20U, 744.0F, 240.0F, 0x3f1eb852U, 5.0F},
                InitialLeaf{21U, 725.0F, 261.0F, 0x3ed1eb85U, 0.0F}, InitialLeaf{22U, 750.0F, 282.0F, 0x3db851ebU, 4.0F}, InitialLeaf{23U, 719.0F, 303.0F, 0x3ec7ae14U, 6.0F},
                InitialLeaf{24U, 763.0F, 260.0F, 0x3e6147aeU, 6.0F}, InitialLeaf{25U, 745.0F, 268.0F, 0x3e0f5c29U, 6.0F}, InitialLeaf{26U, 703.0F, 287.0F, 0x3e6b851eU, 1.0F},
                InitialLeaf{27U, 733.0F, 303.0F, 0x3ed1eb85U, 7.0F}, InitialLeaf{28U, 733.0F, 253.0F, 0x3e2e147bU, 6.0F}, InitialLeaf{29U, 729.0F, 268.0F, 0x3e6147aeU, 2.0F},
                InitialLeaf{30U, 750.0F, 278.0F, 0x3e947ae1U, 0.0F}, InitialLeaf{31U, 704.0F, 255.0F, 0x3dccccccU, 7.0F}}}},
        SetupCase{10U, 176ULL, 2433418682U,
            {{InitialLeaf{0U, 755.0F, 242.0F, 0x3ea3d70aU, 0.0F}, InitialLeaf{1U, 709.0F, 293.0F, 0x3d75c28fU, 1.0F}, InitialLeaf{2U, 755.0F, 253.0F, 0x3e570a3dU, 3.0F},
                InitialLeaf{3U, 708.0F, 247.0F, 0x3eb33333U, 7.0F}, InitialLeaf{4U, 750.0F, 255.0F, 0x3f199999U, 5.0F}, InitialLeaf{5U, 748.0F, 245.0F, 0x3e199999U, 6.0F},
                InitialLeaf{6U, 720.0F, 240.0F, 0x3ec28f5cU, 0.0F}, InitialLeaf{7U, 729.0F, 259.0F, 0x3e8a3d70U, 3.0F}, InitialLeaf{8U, 712.0F, 256.0F, 0x3db851ebU, 5.0F},
                InitialLeaf{9U, 752.0F, 251.0F, 0x3ca3d70aU, 1.0F}, InitialLeaf{10U, 717.0F, 273.0F, 0x00000000U, 6.0F}, InitialLeaf{11U, 726.0F, 296.0F, 0x3e947ae1U, 4.0F},
                InitialLeaf{12U, 725.0F, 247.0F, 0x3d75c28fU, 1.0F}, InitialLeaf{13U, 759.0F, 266.0F, 0x3ee66666U, 6.0F}, InitialLeaf{14U, 735.0F, 255.0F, 0x3f1eb852U, 4.0F},
                InitialLeaf{15U, 704.0F, 277.0F, 0x3e570a3dU, 3.0F}, InitialLeaf{16U, 747.0F, 294.0F, 0x3eeb851eU, 3.0F}, InitialLeaf{17U, 741.0F, 241.0F, 0x3e8f5c29U, 4.0F},
                InitialLeaf{18U, 701.0F, 285.0F, 0x3f1eb852U, 4.0F}, InitialLeaf{19U, 733.0F, 251.0F, 0x3ef0a3d7U, 7.0F}, InitialLeaf{20U, 734.0F, 268.0F, 0x3f1eb852U, 4.0F},
                InitialLeaf{21U, 737.0F, 259.0F, 0x3e999999U, 5.0F}, InitialLeaf{22U, 731.0F, 267.0F, 0x3e999999U, 7.0F}, InitialLeaf{23U, 752.0F, 268.0F, 0x3e8a3d70U, 3.0F},
                InitialLeaf{24U, 708.0F, 298.0F, 0x3f051eb8U, 3.0F}, InitialLeaf{25U, 717.0F, 271.0F, 0x3f1eb852U, 1.0F}, InitialLeaf{26U, 749.0F, 299.0F, 0x3f11eb85U, 5.0F},
                InitialLeaf{27U, 704.0F, 249.0F, 0x3cf5c28fU, 6.0F}, InitialLeaf{28U, 753.0F, 259.0F, 0x3df5c28fU, 4.0F}, InitialLeaf{29U, 721.0F, 296.0F, 0x3dccccccU, 3.0F},
                InitialLeaf{30U, 746.0F, 285.0F, 0x3eeb851eU, 1.0F}, InitialLeaf{31U, 720.0F, 242.0F, 0x3e6147aeU, 3.0F}}}}}};

const std::array<DriftCase, 5> zero_delta_cases = {
    {DriftCase{0U, 32ULL, 2186156320U,
         {{LeafDrift{0U, 700.0F, 0.0F}, LeafDrift{1U, 701.0F, 0.0F}, LeafDrift{2U, 702.0F, 0.0F}, LeafDrift{3U, 703.0F, 0.0F}, LeafDrift{4U, 704.0F, 0.0F},
             LeafDrift{5U, 705.0F, 0.0F}, LeafDrift{6U, 706.0F, 0.0F}, LeafDrift{7U, 707.0F, 0.0F}, LeafDrift{8U, 708.0F, 0.0F}, LeafDrift{9U, 709.0F, 0.0F},
             LeafDrift{10U, 710.0F, 0.0F}, LeafDrift{11U, 711.0F, 0.0F}, LeafDrift{12U, 712.0F, 0.0F}, LeafDrift{13U, 713.0F, 0.0F}, LeafDrift{14U, 714.0F, 0.0F},
             LeafDrift{15U, 715.0F, 0.0F}, LeafDrift{16U, 716.0F, 0.0F}, LeafDrift{17U, 717.0F, 0.0F}, LeafDrift{18U, 718.0F, 0.0F}, LeafDrift{19U, 719.0F, 0.0F},
             LeafDrift{20U, 720.0F, 0.0F}, LeafDrift{21U, 721.0F, 0.0F}, LeafDrift{22U, 722.0F, 0.0F}, LeafDrift{23U, 723.0F, 0.0F}, LeafDrift{24U, 724.0F, 0.0F},
             LeafDrift{25U, 725.0F, 0.0F}, LeafDrift{26U, 726.0F, 0.0F}, LeafDrift{27U, 728.0F, 0.0F}, LeafDrift{28U, 728.0F, 0.0F}, LeafDrift{29U, 729.0F, 0.0F},
             LeafDrift{30U, 730.0F, 0.0F}, LeafDrift{31U, 731.0F, 0.0F}}}},
        DriftCase{1U, 32ULL, 1423890337U,
            {{LeafDrift{0U, 700.0F, 0.0F}, LeafDrift{1U, 701.0F, 0.0F}, LeafDrift{2U, 702.0F, 0.0F}, LeafDrift{3U, 703.0F, 0.0F}, LeafDrift{4U, 704.0F, 0.0F},
                LeafDrift{5U, 705.0F, 0.0F}, LeafDrift{6U, 706.0F, 0.0F}, LeafDrift{7U, 707.0F, 0.0F}, LeafDrift{8U, 708.0F, 0.0F}, LeafDrift{9U, 709.0F, 0.0F},
                LeafDrift{10U, 710.0F, 0.0F}, LeafDrift{11U, 711.0F, 0.0F}, LeafDrift{12U, 712.0F, 0.0F}, LeafDrift{13U, 713.0F, 0.0F}, LeafDrift{14U, 714.0F, 0.0F},
                LeafDrift{15U, 715.0F, 0.0F}, LeafDrift{16U, 716.0F, 0.0F}, LeafDrift{17U, 717.0F, 0.0F}, LeafDrift{18U, 718.0F, 0.0F}, LeafDrift{19U, 719.0F, 0.0F},
                LeafDrift{20U, 720.0F, 0.0F}, LeafDrift{21U, 721.0F, 0.0F}, LeafDrift{22U, 722.0F, 0.0F}, LeafDrift{23U, 723.0F, 0.0F}, LeafDrift{24U, 724.0F, 0.0F},
                LeafDrift{25U, 725.0F, 0.0F}, LeafDrift{26U, 726.0F, 0.0F}, LeafDrift{27U, 727.0F, 0.0F}, LeafDrift{28U, 728.0F, 0.0F}, LeafDrift{29U, 729.0F, 0.0F},
                LeafDrift{30U, 730.0F, 0.0F}, LeafDrift{31U, 731.0F, 0.0F}}}},
        DriftCase{37U, 33ULL, 1623264628U,
            {{LeafDrift{0U, 700.0F, 0.0F}, LeafDrift{1U, 701.0F, 0.0F}, LeafDrift{2U, 702.0F, 0.0F}, LeafDrift{3U, 703.0F, 0.0F}, LeafDrift{4U, 704.0F, 0.0F},
                LeafDrift{5U, 705.0F, 0.0F}, LeafDrift{6U, 706.0F, 0.0F}, LeafDrift{7U, 707.0F, 0.0F}, LeafDrift{8U, 708.0F, 0.0F}, LeafDrift{9U, 709.0F, 0.0F},
                LeafDrift{10U, 710.0F, 0.0F}, LeafDrift{11U, 711.0F, 0.0F}, LeafDrift{12U, 712.0F, 0.0F}, LeafDrift{13U, 713.0F, 0.0F}, LeafDrift{14U, 714.0F, 0.0F},
                LeafDrift{15U, 715.0F, 0.0F}, LeafDrift{16U, 716.0F, 0.0F}, LeafDrift{17U, 717.0F, 0.0F}, LeafDrift{18U, 718.0F, 0.0F}, LeafDrift{19U, 719.0F, 0.0F},
                LeafDrift{20U, 720.0F, 0.0F}, LeafDrift{21U, 721.0F, 0.0F}, LeafDrift{22U, 722.0F, 0.0F}, LeafDrift{23U, 723.0F, 0.0F}, LeafDrift{24U, 724.0F, 0.0F},
                LeafDrift{25U, 725.0F, 1.0F}, LeafDrift{26U, 725.0F, 0.0F}, LeafDrift{27U, 727.0F, 0.0F}, LeafDrift{28U, 728.0F, 0.0F}, LeafDrift{29U, 729.0F, 0.0F},
                LeafDrift{30U, 730.0F, 0.0F}, LeafDrift{31U, 730.0F, 0.0F}}}},
        DriftCase{12345U, 32ULL, 2285941721U,
            {{LeafDrift{0U, 700.0F, 0.0F}, LeafDrift{1U, 701.0F, 0.0F}, LeafDrift{2U, 702.0F, 0.0F}, LeafDrift{3U, 703.0F, 0.0F}, LeafDrift{4U, 704.0F, 0.0F},
                LeafDrift{5U, 705.0F, 0.0F}, LeafDrift{6U, 706.0F, 0.0F}, LeafDrift{7U, 707.0F, 0.0F}, LeafDrift{8U, 708.0F, 0.0F}, LeafDrift{9U, 709.0F, 0.0F},
                LeafDrift{10U, 710.0F, 0.0F}, LeafDrift{11U, 711.0F, 0.0F}, LeafDrift{12U, 712.0F, 0.0F}, LeafDrift{13U, 713.0F, 0.0F}, LeafDrift{14U, 714.0F, 0.0F},
                LeafDrift{15U, 715.0F, 0.0F}, LeafDrift{16U, 716.0F, 0.0F}, LeafDrift{17U, 717.0F, 0.0F}, LeafDrift{18U, 718.0F, 0.0F}, LeafDrift{19U, 719.0F, 0.0F},
                LeafDrift{20U, 720.0F, 0.0F}, LeafDrift{21U, 721.0F, 0.0F}, LeafDrift{22U, 722.0F, 0.0F}, LeafDrift{23U, 723.0F, 0.0F}, LeafDrift{24U, 724.0F, 0.0F},
                LeafDrift{25U, 725.0F, 0.0F}, LeafDrift{26U, 726.0F, 0.0F}, LeafDrift{27U, 727.0F, 0.0F}, LeafDrift{28U, 728.0F, 0.0F}, LeafDrift{29U, 729.0F, 0.0F},
                LeafDrift{30U, 730.0F, 0.0F}, LeafDrift{31U, 732.0F, 0.0F}}}},
        DriftCase{11U, 35ULL, 3152155980U,
            {{LeafDrift{0U, 700.0F, 0.0F}, LeafDrift{1U, 702.0F, 0.0F}, LeafDrift{2U, 702.0F, 0.0F}, LeafDrift{3U, 703.0F, 0.0F}, LeafDrift{4U, 704.0F, 0.0F},
                LeafDrift{5U, 705.0F, 0.0F}, LeafDrift{6U, 706.0F, 0.0F}, LeafDrift{7U, 707.0F, 0.0F}, LeafDrift{8U, 708.0F, 0.0F}, LeafDrift{9U, 709.0F, 0.0F},
                LeafDrift{10U, 710.0F, 1.0F}, LeafDrift{11U, 711.0F, 0.0F}, LeafDrift{12U, 712.0F, 0.0F}, LeafDrift{13U, 713.0F, 0.0F}, LeafDrift{14U, 714.0F, 0.0F},
                LeafDrift{15U, 715.0F, 0.0F}, LeafDrift{16U, 716.0F, 0.0F}, LeafDrift{17U, 717.0F, 0.0F}, LeafDrift{18U, 718.0F, 0.0F}, LeafDrift{19U, 719.0F, 0.0F},
                LeafDrift{20U, 720.0F, 0.0F}, LeafDrift{21U, 721.0F, 3.0F}, LeafDrift{22U, 722.0F, 0.0F}, LeafDrift{23U, 723.0F, 0.0F}, LeafDrift{24U, 724.0F, 0.0F},
                LeafDrift{25U, 725.0F, 0.0F}, LeafDrift{26U, 726.0F, 0.0F}, LeafDrift{27U, 727.0F, 0.0F}, LeafDrift{28U, 728.0F, 0.0F}, LeafDrift{29U, 729.0F, 0.0F},
                LeafDrift{30U, 730.0F, 0.0F}, LeafDrift{31U, 731.0F, 0.0F}}}}}};

const SpawnExpectation expected_second_update_spawn = SpawnExpectation{209ULL, 3641609232U, 18, -109.25F, 44.0F, 0.875F, 248ULL, 1500032409U};

game::RoundState& fixture(game::Round& round) {
    auto& state = const_cast<game::RoundState&>(round.state());
    state.frog.type = -1;
    state.hidden_chicken.type = -1;
    state.spawn_accumulator = -10000;
    return state;
}

void step(game::Round& round, float time, game::RoundInput input = {}) {
    game::test::advance(round, time, input);
    round.resolve_actions({});
}

void setup() {
    for (const auto& row : setup_cases) {
        game::Round round(row.seed, 0);
        const auto& state = round.state();
        require(state.random_draws == row.draws && state.random_state == row.state, "Cartridges, 32 leaves and 8 stones retain the 176-draw setup boundary");
        for (const auto& expected : row.leaves) {
            const auto& leaf = state.persistent_leaves[expected.slot];
            require(leaf.world_x == expected.x && leaf.world_y == expected.y && leaf.amplitude == expected.amplitude
                        && std::bit_cast<std::uint32_t>(leaf.phase) == expected.phase_bits && leaf.screen_x == -1024 && leaf.screen_y == -1024 && leaf.depth == 3
                        && leaf.frame == 1,
                "Each persistent slot uses its four original setup draws before stones");
        }
        require(state.particles.size() == 68 && state.aircraft_smoke.size() == 68 && std::ranges::none_of(state.particles, &game::Particle::active),
            "Persistent slots 0..31 leave all 68 transient slots 32..99 free");
    }
}

void zero_delta() {
    for (const auto& row : zero_delta_cases) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.random_state = row.seed;
        state.random_draws = 0;
        for (std::size_t index = 0; index < state.persistent_leaves.size(); ++index) {
            state.persistent_leaves[index] = {.world_x = static_cast<float>(700 + index), .world_y = 240};
        }
        game::test::advance(round, 0, {});
        require(state.random_draws == row.draws && state.random_state == row.state, "Even delta 0 consumes 32 ordered branch draws plus each residue 2 replacement");
        for (const auto& expected : row.leaves) {
            const auto slot = expected.slot;
            const auto& leaf = state.persistent_leaves[slot];
            require(leaf.world_x == expected.world_x && leaf.amplitude == expected.amplitude && leaf.world_y == 240 && leaf.phase == 0
                        && leaf.screen_x == static_cast<float>(700 + slot) - 895 && leaf.screen_y == 255,
                "Pose precedes the slot's random drift and amplitude replacement");
        }
        const auto before = game::test::snapshot(state);
        const auto leaves = state.persistent_leaves;
        round.resolve_actions({});
        require(state.random_state == before.state && state.random_draws == before.draws && state.persistent_leaves == leaves,
            "Resolving an empty input cannot update leaves a second time");
    }
}

void update_order() {
    game::Round round(1, 0);
    step(round, 1);
    require(
        round.events().empty() && round.state().random_draws == expected_second_update_spawn.first_draws && round.state().random_state == expected_second_update_spawn.first_state,
        "Strict spawn threshold leaves the first sample to frog then persistent particles");
    game::test::advance(round, 1.125F, {});
    const auto& state = round.state();
    require(state.chickens[34].type == -7 && state.airplane_slot == 34 && state.airplane.type == expected_second_update_spawn.type
                && state.airplane.world_x == expected_second_update_spawn.x && state.airplane.world_y == expected_second_update_spawn.y
                && state.airplane.wait == expected_second_update_spawn.wait && state.random_draws == expected_second_update_spawn.draws
                && state.random_state == expected_second_update_spawn.state,
        "Second sample spawns first, updates scene/frog next, then consumes leaf draws");
    round.resolve_actions({});

    game::Round grant(1, 0);
    auto& g = fixture(grant);
    for (auto& chicken : g.chickens) {
        chicken.type = -2;
    }
    for (int slot = 70; slot < 76; ++slot) {
        g.chickens[static_cast<std::size_t>(slot)].type = -1;
    }
    g.camera = 0;
    g.random_state = 1;
    g.random_draws = 0;
    g.balloons_hit = 25;
    auto oracle = game::test::after_leaves({1, 0});
    std::array<float, 6> positions{};
    for (auto& x : positions) {
        x = static_cast<float>(oracle.next() % 2272);
    }
    game::test::advance(grant, 0, {});
    require(g.balloon_hero && g.score == 100 && g.random_state == oracle.state && g.random_draws == oracle.draws,
        "Normal pre-input hero grant consumes its six position words after all leaf words");
    for (std::size_t index = 0; index < positions.size(); ++index) {
        require(g.big_chickens[index + 70].world_x == positions[index] && g.big_chickens[index + 70].screen_y == -1024,
            "Grant uses the post-leaf CRT stream and newborn targets wait for their next scene pose");
    }
    grant.resolve_actions({});
}

void camera_recoil_and_render() {
    game::Round round(1, 0);
    auto& state = fixture(round);
    state.camera = 0;
    state.persistent_leaves[0] = {.world_x = 700, .world_y = 240};
    step(round, 0, {.fire_down = true});
    require(state.recoil == 8, "Firing sets the recoil for the next composed frame");
    state.persistent_leaves[0] = {.world_x = 700, .world_y = 240};
    game::test::advance(round, .125F, {.held_right = true});
    const auto& leaf = state.persistent_leaves[0];
    require(state.camera == 25 && leaf.screen_x == 666 && leaf.screen_y == 261.4F && leaf.phase == .005625F,
        "Leaves see the moved camera, prior shot recoil and current sampled delta");
    const auto before_render = round.state();
    const auto draw = world::persistent_leaf_draw(leaf);
    require(draw.image == "gfx/particle" && draw.source.x == 0 && draw.source.y == 1 && draw.source.width == 8 && draw.source.height == 1
                && draw.destination == contracts::ScreenPoint{666, 261},
        "Persistent leaf uses fixed source row1 and truncates its stored screen pose");
    for (int index = 0; index < 20; ++index) {
        static_cast<void>(world::persistent_leaf_draw(leaf));
        static_cast<void>(round.target_draws(3));
    }
    require(round.state() == before_render, "Repeated render submission consumes no random values and cannot advance leaves");
    require(std::ranges::none_of(round.target_draws(3),
                [](const auto& value) {
                    return value.sprite.image == "gfx/particle";
                }),
        "Persistent particles retain their separate post-scene draw pass");
    round.resolve_actions({});
}

void transient_capacity() {
    for (const int occupied : {67, 68}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.camera = 0;
        state.stones[0].world_x = 100;
        state.stones[0].world_y = 340;
        for (int index = 0; index < occupied; ++index) {
            state.particles[static_cast<std::size_t>(index)] = {.active = true, .type = 3, .world_y = 0};
        }
        game::test::advance(round, 0, {.pointer = {144, 374}, .fire_down = true});
        const auto leaves = state.persistent_leaves;
        const auto random = game::test::snapshot(state);
        round.resolve_actions([](const auto& draw, auto) {
            return draw.destination.x == 100;
        });
        require(state.selected_slot == game::first_stone_scene_slot && state.hits == 1 && state.score == 2 && state.random_draws == random.draws + 2
                    && state.persistent_leaves == leaves,
            "A stone hit retains its two local velocity draws and cannot reuse a persistent slot");
        require(std::ranges::count(state.particles, true, &game::Particle::active) == 68 && (occupied == 68 || (state.particles[67].type == 2 && state.particles[67].points == 2)),
            "The 68th transient slot accepts a popup and the 69th request is suppressed");
    }
}

void focus_and_reset() {
    game::Round round(10, 0);
    step(round, .125F, {.fire_down = true, .held_right = true});
    auto frozen = round.state();
    frozen.previous_sample = 100;
    frozen.delta = 0;
    round.reset_sample(100);
    require(round.state() == frozen, "Pause/focus sample reset changes no leaf pose, drift or RNG");
    step(round, 100.125F);
    require(round.state().delta == .125F && round.state().remaining == frozen.remaining - .125F && round.state().updates == frozen.updates + 1,
        "Resuming updates only the sampled post-focus delta without catch-up");

    game::Round fresh(10, 0);
    require(fresh.state().persistent_leaves != round.state().persistent_leaves && fresh.state().random_draws == 176 && fresh.state().updates == 0,
        "A new round rebuilds all 32 leaves and resets the random/update counters");
}
} // namespace

int main() {
    try {
        setup();
        zero_delta();
        update_order();
        camera_recoil_and_render();
        transient_capacity();
        focus_and_reset();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Persistent leaf setup/order/RNG, camera/recoil, transient capacity, focus/reset passed\n";
}
