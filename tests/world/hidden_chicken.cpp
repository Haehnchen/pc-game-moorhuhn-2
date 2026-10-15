#include "world/hidden_chicken.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct ImpactExpectation {
    int points;
    std::optional<world::HiddenPopup> popup;
    std::vector<world::HiddenSound> sounds;
};

struct UpdateCase {
    std::string id;
    world::HiddenChicken before;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    int pointer_x;
    bool enabled;
    std::optional<std::uint32_t> random;
    world::HiddenChicken after;
};

struct LeafUpdateCase {
    std::string id;
    std::uint32_t camera_bits;
    int recoil;
    std::uint32_t screen_x_bits;
    std::uint32_t screen_y_bits;
};

struct PrerequisiteCase {
    bool before_enabled;
    int leaf_hits;
    int spider_hits;
    int branch_hits;
    ImpactExpectation effect;
    bool after_enabled;
};

struct ImpactCase {
    std::string id;
    world::HiddenChicken before;
    bool enabled;
    ImpactExpectation effect;
    world::HiddenChicken after;
    int particle_count;
    int balloon_count;
};

const std::array<UpdateCase, 74> updates_cases = {
    {UpdateCase{"initial-pose", world::HiddenChicken{34, 1394.0F, 105.0F, -1024.0F, -1024.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
         std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-strict-equal-4", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x40800000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 4.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-next-float-over4-pointer-inside", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x40800001U, 0x44200000U, 0,
            320, false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x40800001U), 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-left-equality-blocks", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 64, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-left-outside-starts", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 63, false,
            0U, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 1.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-right-equality-blocks", world::HiddenChicken{34, 1394.0F, 105.0F, 100.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 465,
            false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-right-outside-starts", world::HiddenChicken{34, 1394.0F, 105.0F, 100.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 466,
            false, 32767U, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-extended-comparison-before-store", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 4.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x33000000U, 0x44200000U,
            0, 0, false, 2U, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 3.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-False-0", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, false, 0U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 1.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-False-1", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, false, 1U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-False-2", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, false, 2U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 3.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-False-32767", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, false,
            32767U, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-True-0", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, true, 0U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-True-1", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, true, 1U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 3.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-True-2", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, true, 2U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-rng-True-32767", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 0, true, 32767U,
            world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 3.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode0-armed-near-starts", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 5.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, true,
            0U, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"opening-table-0", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"opening-table-1", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 1.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 1.0F, 1, 4.0F, 1, 16, 3.0F, 250}},
        UpdateCase{"opening-table-2", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 2.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 2.0F, 1, 4.0F, 2, 16, 3.0F, 250}},
        UpdateCase{"opening-table-3", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 3.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 3.0F, 1, 4.0F, 3, 16, 3.0F, 250}},
        UpdateCase{"opening-table-4", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 4.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 4.0F, 1, 4.0F, 4, 16, 3.0F, 250}},
        UpdateCase{"opening-table-5", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 5.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 5.0F, 1, 4.0F, 5, 16, 3.0F, 250}},
        UpdateCase{"opening-table-6", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 6.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 6.0F, 1, 4.0F, 6, 16, 3.0F, 250}},
        UpdateCase{"opening-table-7", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 7.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 7.0F, 1, 4.0F, 7, 16, 3.0F, 250}},
        UpdateCase{"opening-table-8", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 8.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 8.0F, 1, 4.0F, 8, 16, 3.0F, 250}},
        UpdateCase{"opening-table-9", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 9.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 9.0F, 1, 4.0F, 9, 16, 3.0F, 250}},
        UpdateCase{"opening-table-10", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 10.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 10.0F, 1, 4.0F, 10, 16, 3.0F, 250}},
        UpdateCase{"opening-table-11", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 11.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 11.0F, 1, 4.0F, 11, 16, 3.0F, 250}},
        UpdateCase{"opening-table-12", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 12.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 12.0F, 1, 4.0F, 12, 16, 3.0F, 250}},
        UpdateCase{"opening-table-13", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 13.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 13.0F, 1, 4.0F, 13, 16, 3.0F, 250}},
        UpdateCase{"opening-table-14", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 14.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 14.0F, 1, 4.0F, 14, 16, 3.0F, 250}},
        UpdateCase{"opening-table-15", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 15.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 15.0F, 1, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"opening-sentinel", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"opening-pad-sentinel20", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 20.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"opening-native-safe-large-skip", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x447a0000U, 0x44200000U, 0, 320,
            false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"opening-retained-under1-stores1", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x33000000U), 1, 4.0F, 0, 16, 3.0F, 250}, 0x3d638e38U,
            0x44200000U, 0, 320, false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 1.0F, 1, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"opening-retained-under16-stores16", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 15.0F, 1, 4.0F, 15, 16, 3.0F, 250}, 0x3d638e38U, 0x44200000U, 0,
            320, false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 1, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"mode2-near-retreat", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode2-left-equality-holds", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 64, false,
            std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"mode2-left-interior-retreat", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 65,
            false, std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode2-right-equality-holds", world::HiddenChicken{34, 1394.0F, 105.0F, 100.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 465,
            false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"mode2-right-interior-retreat", world::HiddenChicken{34, 1394.0F, 105.0F, 100.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 464,
            false, std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode2-unarmed-no-clock-aging", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 3.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x3f800000U, 0x44200000U, 0, 0,
            false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 3.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"mode2-uses-old-pose-across-camera", world::HiddenChicken{34, 1394.0F, 105.0F, 400.0F, 105.0F, 0.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x00000000U, 0,
            65, false, std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 1395.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode2-armed-equality-holds", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 2.0F, 15, 16, 3.0F, 250}, 0x40000000U, 0x44200000U, 0, 320,
            true, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 2.0F, 2, 2.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"mode2-armed-next-float-retreat", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 2.0F, 15, 16, 3.0F, 250}, 0x40000001U, 0x44200000U, 0, 320,
            true, std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 3, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"mode2-armed-extended-comparison", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 2.0F, 2, 2.0F, 15, 16, 3.0F, 250}, 0x33000000U, 0x44200000U, 0, 320,
            true, std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 3, 2.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-0", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-1", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 1.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 1.0F, 3, 4.0F, 1, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-2", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 2.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 2.0F, 3, 4.0F, 2, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-3", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 3.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 3.0F, 3, 4.0F, 3, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-4", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 4.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 4.0F, 3, 4.0F, 4, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-5", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 5.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 5.0F, 3, 4.0F, 5, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-6", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 6.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 6.0F, 3, 4.0F, 6, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-7", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 7.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 7.0F, 3, 4.0F, 7, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-8", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 8.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 8.0F, 3, 4.0F, 8, 16, 3.0F, 250}},
        UpdateCase{"retreat-table-9", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 9.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 9.0F, 3, 4.0F, 9, 16, 3.0F, 250}},
        UpdateCase{"retreat-retained-under10-stores10-no-rng", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 9.0F, 3, 4.0F, 9, 16, 3.0F, 250}, 0x3d638e38U, 0x44200000U,
            0, 320, false, std::nullopt, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 10.0F, 3, 4.0F, 9, 16, 3.0F, 250}},
        UpdateCase{"retreat-sentinel-rng-0", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 10.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            0U, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-sentinel-rng-1", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 10.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            1U, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 0, 5.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-sentinel-rng-15", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 10.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            15U, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 0, 19.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-sentinel-rng-16", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 10.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320, false,
            16U, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-sentinel-rng-32767", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 10.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320,
            false, 32767U, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 0, 19.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"retreat-native-safe-large-skip", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, 0.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x447a0000U, 0x44200000U, 0, 320,
            false, 15U, world::HiddenChicken{34, 1395.0F, 105.0F, 371.0F, 105.0F, 0.0F, 0, 19.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"hidden-camera2-endpoint-recoil8", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44a00000U, 8, 320,
            false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, -654.0F, 113.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"hidden-camera2-retained-product", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44106000U, 0, 320,
            false, std::nullopt, world::HiddenChicken{34, 1394.0F, 105.0F, 470.0F, 105.0F, 0.0F, 0, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"reaction74-retained-under1", world::HiddenChicken{74, 1395.0F, 105.0F, 370.0F, 105.0F, binary32(0x33000000U), 3, 4.0F, 0, 16, 3.0F, 250}, 0x3d888888U,
            0x44200000U, 0, 320, false, std::nullopt, world::HiddenChicken{74, 1395.0F, 105.0F, 371.0F, 105.0F, 1.0F, 3, 4.0F, 0, 16, 3.0F, 250}},
        UpdateCase{"reaction74-zero-delta-stored1", world::HiddenChicken{74, 1395.0F, 105.0F, 370.0F, 105.0F, 1.0F, 3, 4.0F, 0, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320,
            false, std::nullopt, world::HiddenChicken{74, 1395.0F, 105.0F, 371.0F, 105.0F, 1.0F, 3, 4.0F, 1, 16, 3.0F, 250}},
        UpdateCase{"reaction74-retained-under16-stores16", world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 15.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x3d888888U, 0x44200000U,
            0, 320, false, std::nullopt, world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"reaction74-before-removal", world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x417fffffU), 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U,
            0x44200000U, 0, 320, false, std::nullopt, world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x417fffffU), 2, 4.0F, 15, 16, 3.0F, 250}},
        UpdateCase{"reaction74-equal16-removes", world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 2, 4.0F, 15, 16, 3.0F, 250}, 0x00000000U, 0x44200000U, 0, 320,
            false, std::nullopt, world::HiddenChicken{-1, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 2, 4.0F, 16, 16, 3.0F, 250}},
        UpdateCase{"reaction74-backlog-removes", world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 4.0F, 0, 16, 3.0F, 250}, 0x40000000U, 0x44200000U, 0, 320,
            false, std::nullopt, world::HiddenChicken{-1, 1394.0F, 105.0F, 370.0F, 105.0F, 30.0F, 1, 4.0F, 30, 16, 3.0F, 250}},
        UpdateCase{"inactive-no-update", world::HiddenChicken{-1, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 2, 4.0F, 16, 16, 3.0F, 250}, 0x3f800000U, 0x44a00000U, 8, 320, false,
            std::nullopt, world::HiddenChicken{-1, 1394.0F, 105.0F, 370.0F, 105.0F, 16.0F, 2, 4.0F, 16, 16, 3.0F, 250}}}};

const std::array<LeafUpdateCase, 4> leaf_updates_cases = {
    {LeafUpdateCase{"camera0-recoil0", 0x00000000U, 0, 0x4512e000U, 0x43910000U}, LeafUpdateCase{"camera1280-recoil8", 0x44a00000U, 8, 0x440bc000U, 0x43943333U},
        LeafUpdateCase{"camera640-recoil3", 0x44200000U, 3, 0x44b5e000U, 0x43923333U}, LeafUpdateCase{"retained-camera-product", 0x44106000U, 7, 0x44c0c000U, 0x4393cccdU}}};

const std::array<PrerequisiteCase, 14> prerequisites_cases = {
    {PrerequisiteCase{false, 10, 6, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{true, 10, 6, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{false, 11, 6, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{true, 11, 6, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{false, 9, 6, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, false},
        PrerequisiteCase{true, 9, 6, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{false, 10, 5, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, false},
        PrerequisiteCase{true, 10, 5, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{false, 10, 7, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, false},
        PrerequisiteCase{true, 10, 7, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{false, 10, 6, 1, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, false},
        PrerequisiteCase{true, 10, 6, 1, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true},
        PrerequisiteCase{false, 0, 0, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, false},
        PrerequisiteCase{true, 0, 0, 0, ImpactExpectation{50, world::HiddenPopup{2360.0F, 320.0F, 50}, {world::HiddenSound::score_50}}, true}}};

const std::array<ImpactCase, 8> impacts_cases = {
    {ImpactCase{"hit-mode0-enabledFalse", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 0, 3.0F, 0, 16, 3.0F, 250}, false,
         ImpactExpectation{0, std::nullopt, {}}, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 0, 3.0F, 0, 16, 3.0F, 250}, 0, 0},
        ImpactCase{"hit-mode0-enabledTrue", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 0, 3.0F, 0, 16, 3.0F, 250}, true,
            ImpactExpectation{250, world::HiddenPopup{1404.0F, 163.0F, 250},
                {world::HiddenSound::chicken_shot_close, world::HiddenSound::chicken_shot_mid, world::HiddenSound::chicken_shot_far, world::HiddenSound::magic_balloons}},
            world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 0, 3.0F, 0, 16, 3.0F, 250}, 32, 24},
        ImpactCase{"hit-mode1-enabledFalse", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 1, 3.0F, 0, 16, 3.0F, 250}, false,
            ImpactExpectation{0, std::nullopt, {}}, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 1, 3.0F, 0, 16, 3.0F, 250}, 0, 0},
        ImpactCase{"hit-mode1-enabledTrue", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 1, 3.0F, 0, 16, 3.0F, 250}, true,
            ImpactExpectation{250, world::HiddenPopup{1404.0F, 163.0F, 250},
                {world::HiddenSound::chicken_shot_close, world::HiddenSound::chicken_shot_mid, world::HiddenSound::chicken_shot_far, world::HiddenSound::magic_balloons}},
            world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 1, 3.0F, 0, 16, 3.0F, 250}, 32, 24},
        ImpactCase{"hit-mode2-enabledFalse", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 2, 3.0F, 15, 16, 3.0F, 250}, false,
            ImpactExpectation{0, std::nullopt, {}}, world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 2, 3.0F, 15, 16, 3.0F, 250}, 0, 0},
        ImpactCase{"hit-mode2-enabledTrue", world::HiddenChicken{34, 1394.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 2, 3.0F, 15, 16, 3.0F, 250}, true,
            ImpactExpectation{250, world::HiddenPopup{1404.0F, 163.0F, 250},
                {world::HiddenSound::chicken_shot_close, world::HiddenSound::chicken_shot_mid, world::HiddenSound::chicken_shot_far, world::HiddenSound::magic_balloons}},
            world::HiddenChicken{74, 1394.0F, 105.0F, 370.0F, 105.0F, 0.0F, 2, 3.0F, 0, 16, 3.0F, 250}, 32, 24},
        ImpactCase{"hit-mode3-enabledFalse", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 3, 3.0F, 0, 16, 3.0F, 250}, false,
            ImpactExpectation{0, std::nullopt, {}}, world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 3, 3.0F, 0, 16, 3.0F, 250}, 0, 0},
        ImpactCase{"hit-mode3-enabledTrue", world::HiddenChicken{34, 1395.0F, 105.0F, 370.0F, 105.0F, binary32(0x3f000000U), 3, 3.0F, 0, 16, 3.0F, 250}, true,
            ImpactExpectation{250, world::HiddenPopup{1405.0F, 163.0F, 250},
                {world::HiddenSound::chicken_shot_close, world::HiddenSound::chicken_shot_mid, world::HiddenSound::chicken_shot_far, world::HiddenSound::magic_balloons}},
            world::HiddenChicken{74, 1395.0F, 105.0F, 370.0F, 105.0F, 0.0F, 3, 3.0F, 0, 16, 3.0F, 250}, 32, 24}}};

void check_impact(const world::HiddenImpact& actual, const ImpactExpectation& value) {
    require(actual.points == value.points && actual.popup.has_value() == value.popup.has_value() && actual.sound_count == value.sounds.size(),
        "Original hidden target score, popup presence or sound count differs");
    if (actual.popup) {
        const auto& popup = value.popup;
        require(*actual.popup == *popup, "Original hidden target popup anchor differs");
    }
    for (std::size_t index = 0; index < actual.sound_count; ++index) {
        require(actual.sounds[index] == value.sounds[index], "Original hidden target sound order differs");
    }
}

void original_vectors() {
    for (const auto& value : updates_cases) {
        auto chicken = value.before;
        const auto delta = binary32(value.delta_bits);
        const int pointer_x = value.pointer_x;
        const bool enabled = value.enabled;
        const bool needs_random = world::hidden_chicken_needs_random(chicken, delta, pointer_x, enabled);
        require(needs_random == value.random.has_value(), "Original RNG request gate differs");
        const auto random = needs_random ? value.random : std::nullopt;
        world::update_hidden_chicken(chicken, delta, binary32(value.camera_bits), value.recoil, pointer_x, enabled, random);
        require(chicken == value.after, value.id + ": original hidden update differs");
    }
    for (const auto& value : leaf_updates_cases) {
        world::HiddenLeaf leaf;
        world::update_hidden_leaf(leaf, binary32(value.camera_bits), value.recoil);
        require(leaf.screen_x == binary32(value.screen_x_bits) && leaf.screen_y == binary32(value.screen_y_bits), value.id + ": original stationary Leaf pose differs");
    }
    for (const auto& value : prerequisites_cases) {
        world::HiddenLeaf leaf;
        bool enabled = value.before_enabled;
        const auto impact = world::hit_hidden_leaf(leaf, enabled, value.leaf_hits, value.spider_hits, value.branch_hits);
        check_impact(impact, value.effect);
        require(enabled == value.after_enabled && leaf.type == -1 && impact.particle_count == 0 && impact.balloon_count == 0,
            "Original prerequisite conjunction, sticky flag or Leaf removal differs");
    }
    for (const auto& value : impacts_cases) {
        auto chicken = value.before;
        const auto impact = world::hit_hidden_chicken(chicken, value.enabled);
        check_impact(impact, value.effect);
        require(chicken == value.after && impact.particle_count == value.particle_count && impact.balloon_count == value.balloon_count,
            value.id + ": original hidden response fields or effect requests differ");
    }
}

void drawing_and_reset() {
    world::HiddenChicken chicken;
    world::HiddenLeaf leaf;
    require(!world::hidden_chicken_draw(chicken), "Initial mode0 must remain invisible");
    const auto empty = world::hidden_chicken_collision(chicken);
    require(empty && empty->image.empty() && empty->source.width == 61 && empty->source.height == 117 && !empty->color_key,
        "Invisible mode0 must retain a cleared black collision rectangle");
    world::update_hidden_chicken(chicken, 0, 640, 8, 320, false);
    world::update_hidden_leaf(leaf, 1280, 8);
    for (int mode = 1; mode <= 3; ++mode) {
        chicken.mode = mode;
        const int count = mode == 3 ? 10 : 16;
        for (int frame = mode == 2 ? 15 : 0; frame < count; ++frame) {
            chicken.frame = frame;
            const auto draw = world::hidden_chicken_draw(chicken);
            const auto collision = world::hidden_chicken_collision(chicken);
            require(draw && collision && draw->image == (mode == 3 ? "gfx/hide2" : "gfx/hide1") && collision->image == draw->image && draw->source.x == 0
                        && draw->source.y == frame * (mode == 3 ? 121 : 117) && draw->source.width == (mode == 3 ? 78 : 61) && draw->source.height == (mode == 3 ? 121 : 117)
                        && collision->source.y == draw->source.y && collision->source.width == 61 && collision->source.height == 117 && draw->destination.x == 370
                        && draw->destination.y == 113 && draw->color_key && collision->color_key && !draw->mirror_x && !collision->mirror_x,
                "Original hidden drawing, collision crop or pose differs");
        }
    }
    chicken.type = 74;
    for (int frame = 0; frame < 16; ++frame) {
        chicken.frame = frame;
        const auto draw = world::hidden_chicken_draw(chicken);
        require(draw && draw->image == "gfx/hide3" && draw->source.y == frame * 114 && draw->source.width == 80 && draw->source.height == 114
                    && !world::hidden_chicken_collision(chicken),
            "Reaction74 must draw hide3 and remain excluded from collision");
    }
    chicken.type = -1;
    require(!world::hidden_chicken_draw(chicken) && !world::hidden_chicken_collision(chicken), "Removed hidden slot must produce no draw or collision command");
    const auto trigger = world::hidden_leaf_draw(leaf);
    const auto trigger_collision = world::hidden_leaf_collision(leaf);
    require(trigger && trigger_collision && trigger->image == "gfx/Leaf" && trigger_collision->image == "gfx/Leaf" && trigger->source.width == 60 && trigger->source.height == 60
                && trigger->destination.x == 559 && trigger->destination.y == 296,
        "Original slot33 Leaf0 linkage or projection differs");
    bool enabled = false;
    static_cast<void>(world::hit_hidden_leaf(leaf, enabled, 10, 6, 0));
    require(!world::hidden_leaf_draw(leaf) && !world::hidden_leaf_collision(leaf), "Type35 removal must suppress repeat selection");
    const world::HiddenChicken fresh_chicken;
    const world::HiddenLeaf fresh_leaf;
    require(fresh_chicken.type == 34 && fresh_chicken.mode == 0 && fresh_chicken.wait == 4 && fresh_chicken.frame == 0 && fresh_chicken.animation_time == 0
                && fresh_chicken.screen_x == -1024 && fresh_leaf.type == 35 && fresh_leaf.frame == 0 && fresh_leaf.screen_x == -1024,
        "New round must restore both records");
}

template <typename Action> void rejects(Action action, std::string_view message) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, message);
}

void invalid_inputs_are_atomic() {
    world::HiddenChicken chicken;
    const auto original = chicken;
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, -1, 640, 0, 320, false);
        },
        "Negative elapsed time must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, 0, 1281, 0, 320, false);
        },
        "Out-of-range camera must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, 0, 640, 9, 320, false);
        },
        "Out-of-range recoil must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, 0, 640, 0, 640, false);
        },
        "Out-of-range pointer must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, 0, 640, 0, 320, false, 1);
        },
        "Unconsumed random draws must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, 5, 640, 0, 320, true);
        },
        "Missing emergence draw must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, 5, 640, 0, 320, true, 32768);
        },
        "Non-CRT random draw must be rejected");
    require(chicken == original, "Rejected update must not partly modify the object");
    static_cast<void>(world::hit_hidden_chicken(chicken, true));
    const auto reaction = chicken;
    rejects(
        [&] {
            static_cast<void>(world::hit_hidden_chicken(chicken, true));
        },
        "Repeat reaction hit must be rejected");
    rejects(
        [&] {
            world::update_hidden_chicken(chicken, std::numeric_limits<float>::max(), 640, 0, 320, true);
        },
        "Death frame overflow must be rejected");
    require(chicken == reaction, "Rejected reaction operation must preserve its record");
    world::HiddenLeaf leaf;
    bool enabled = false;
    rejects(
        [&] {
            static_cast<void>(world::hit_hidden_leaf(leaf, enabled, -1, 6, 0));
        },
        "Negative prerequisite counters must be rejected");
    require(leaf == world::HiddenLeaf{} && !enabled, "Invalid prerequisites must not remove or arm the trigger");
    static_cast<void>(world::hit_hidden_leaf(leaf, enabled, 10, 6, 0));
    rejects(
        [&] {
            static_cast<void>(world::hit_hidden_leaf(leaf, enabled, 10, 6, 0));
        },
        "Removed trigger repeats must be rejected");
}

} // namespace

int main() {
    try {
        original_vectors();
        drawing_and_reset();
        invalid_inputs_are_atomic();
        std::cout << "Hidden chicken: " << updates_cases.size() << " independent updates, " << prerequisites_cases.size() << " prerequisite cases, " << impacts_cases.size()
                  << " impacts, all drawing cells, reset and invalid inputs passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
