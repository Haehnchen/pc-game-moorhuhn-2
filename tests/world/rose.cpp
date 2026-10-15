#include "world/rose.hpp"

#include "support/numeric.hpp"
#include "support/require.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct UpdateCase {
    std::string id;
    world::Rose before;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    world::Rose after;
};

struct ImpactCase {
    std::string id;
    world::Rose before;
    std::uint32_t remaining_bits;
    world::RosePopup popup;
    int points;
    std::uint32_t remaining_after_bits;
    world::Rose after;
};

const world::Rose expected_initial = world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10};

const std::array<UpdateCase, 25> updates_cases = {{UpdateCase{"live-camera-0", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x00000000U,
                                                       0, world::Rose{29, 497.0F, 396.0F, 497.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-1", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x3f800000U, 0,
        world::Rose{29, 497.0F, 396.0F, 496.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-100", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x42c80000U, 0,
        world::Rose{29, 497.0F, 396.0F, 318.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-250", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x437a0000U, 0,
        world::Rose{29, 497.0F, 396.0F, 48.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-320", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x43a00000U, 0,
        world::Rose{29, 497.0F, 396.0F, -78.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-640", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x44200000U, 0,
        world::Rose{29, 497.0F, 396.0F, -654.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-1280", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x44a00000U, 0,
        world::Rose{29, 497.0F, 396.0F, -1806.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-276.1111145019531", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x438a0e39U, 0,
        world::Rose{29, 497.0F, 396.0F, 1.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-camera-624.1666259765625", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x441c0aaaU, 0,
        world::Rose{29, 497.0F, 396.0F, -626.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-recoil-1", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3e000000U, 0x437a0000U, 1,
        world::Rose{29, 497.0F, 396.0F, 48.0F, binary32(0x43c6999aU), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-recoil-3", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3e000000U, 0x437a0000U, 3,
        world::Rose{29, 497.0F, 396.0F, 48.0F, binary32(0x43c7cccdU), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-recoil-7", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3e000000U, 0x437a0000U, 7,
        world::Rose{29, 497.0F, 396.0F, 48.0F, binary32(0x43ca3333U), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-recoil-8", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3e000000U, 0x437a0000U, 8,
        world::Rose{29, 497.0F, 396.0F, 48.0F, binary32(0x43cacccdU), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"live-retains-unused-clock", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 12.0F, 0, 1, 3.0F, 10}, 0x41700000U, 0x43a00000U, 8,
        world::Rose{29, 497.0F, 396.0F, -78.0F, binary32(0x43cacccdU), 12.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-zero", world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000000U, 0x437a0000U, 0,
        world::Rose{69, 497.0F, 396.0F, 48.0F, 396.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-quarter", world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3e800000U, 0x437a0000U, 8,
        world::Rose{69, 497.0F, binary32(0x43cf6000U), 48.0F, binary32(0x43d42ccdU), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-one", world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x437a0000U, 0,
        world::Rose{69, 497.0F, 471.0F, 48.0F, 471.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-exact480", world::Rose{69, 497.0F, 405.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x437a0000U, 0,
        world::Rose{69, 497.0F, 480.0F, 48.0F, 480.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-stored480-retained-below", world::Rose{69, 497.0F, binary32(0x43ef6666U), -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000000U, 0x00000000U, 1,
        world::Rose{69, 497.0F, binary32(0x43ef6666U), 497.0F, 480.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-stored480-retained-above", world::Rose{69, 497.0F, binary32(0x43ebcccdU), -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000000U, 0x00000000U, 7,
        world::Rose{-1, 497.0F, binary32(0x43ebcccdU), 497.0F, 480.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-float-neighbor-below", world::Rose{69, 497.0F, binary32(0x43efffffU), -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000000U, 0x43a00000U, 0,
        world::Rose{69, 497.0F, binary32(0x43efffffU), -78.0F, binary32(0x43efffffU), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-float-neighbor-above", world::Rose{69, 497.0F, binary32(0x43f00001U), -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000000U, 0x43a00000U, 0,
        world::Rose{-1, 497.0F, binary32(0x43f00001U), -78.0F, binary32(0x43f00001U), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-small-delta-stores-world480", world::Rose{69, 497.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x33000000U, 0x00000000U, 0,
        world::Rose{69, 497.0F, 480.0F, 497.0F, 480.0F, 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"reaction-oversized-finite", world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x447a0000U, 0x44200000U, 8,
        world::Rose{-1, 497.0F, 75396.0F, -654.0F, binary32(0x479346cdU), 0.0F, 0, 1, 3.0F, 10}},
    UpdateCase{"removed-holds-all-fields", world::Rose{-1, 497.0F, 481.0F, 497.0F, 481.0F, 0.0F, 0, 1, 3.0F, 10}, 0x3f800000U, 0x44a00000U, 8,
        world::Rose{-1, 497.0F, 481.0F, 497.0F, 481.0F, 0.0F, 0, 1, 3.0F, 10}}}};

const std::array<ImpactCase, 16> impacts_cases = {
    {ImpactCase{"early-below", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42a9ffffU, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42a9ffffU,
         world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"early-start", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42aa0000U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42b40000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"early-after", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42aa0001U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42b40001U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"early-end-below", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42b3ffffU, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42bdffffU,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"early-end", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42b40000U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42be0000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"early-above", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42b40001U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42b40001U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"late-below-zero", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x80000001U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x80000001U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"late-zero", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000000U, world::RosePopup{486.0F, 409.0F, 50}, 50, 0x00000000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 50}},
        ImpactCase{"late-negative-zero", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x80000000U, world::RosePopup{486.0F, 409.0F, 50}, 50,
            0x80000000U, world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 50}},
        ImpactCase{"late-positive-neighbor", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x00000001U, world::RosePopup{486.0F, 409.0F, 50}, 50,
            0x00000001U, world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 50}},
        ImpactCase{"late-end-below", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x409fffffU, world::RosePopup{486.0F, 409.0F, 50}, 50, 0x409fffffU,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 50}},
        ImpactCase{"late-end", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x40a00000U, world::RosePopup{486.0F, 409.0F, 50}, 50, 0x40a00000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 50}},
        ImpactCase{"late-above", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x40a00001U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x40a00001U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"regular", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42700000U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42700000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"negative", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0xbf800000U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0xbf800000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}},
        ImpactCase{"above-round", world::Rose{29, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}, 0x42c80000U, world::RosePopup{486.0F, 409.0F, 10}, 10, 0x42c80000U,
            world::Rose{69, 497.0F, 396.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 3.0F, 10}}}};

void draws(const world::Rose& rose) {
    const auto draw = world::rose_draw(rose);
    const auto collision = world::rose_collision(rose);
    require(draw.has_value() == (rose.type != -1), "Original draw eligibility differs");
    require(collision.has_value() == (rose.type == 29), "Original collision eligibility differs");
    for (const auto& command : {draw, collision}) {
        if (!command) {
            continue;
        }
        require(command->image == "gfx/rose" && command->source.x == 0 && command->source.y == 0 && command->source.width == 18 && command->source.height == 26,
            "Original rose cell differs");
        require(command->destination.x == static_cast<int>(rose.screen_x) && command->destination.y == static_cast<int>(rose.screen_y) && command->color_key && !command->mirror_x,
            "Original rose pose/key differs");
    }
}

void original_vectors() {
    require(world::Rose{} == expected_initial, "Original initial rose fields differ");
    for (const auto& value : updates_cases) {
        auto rose = value.before;
        draws(rose);
        world::update_rose(rose, binary32(value.delta_bits), binary32(value.camera_bits), value.recoil);
        require(rose == value.after, value.id + ": original rose update differs");
        draws(rose);
    }
    for (const auto& value : impacts_cases) {
        auto rose = value.before;
        const auto impact = world::hit_rose(rose, binary32(value.remaining_bits));
        const auto& popup = value.popup;
        require(impact.points == value.points && impact.popup == popup, value.id + ": original rose award/popup differs");
        require(std::bit_cast<std::uint32_t>(impact.remaining_after) == std::bit_cast<std::uint32_t>(binary32(value.remaining_after_bits)),
            value.id + ": original rose timer store differs");
        require(rose == value.after, value.id + ": original rose impact state differs");
        require(!world::rose_collision(rose), "Reaction remains selectable");
    }
}

template <class Action> void rejected_without_mutation(world::Rose rose, Action action) {
    const auto before = rose;
    bool rejected = false;
    try {
        action(rose);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && rose == before, "Invalid rose operation was accepted or partially changed state");
}

void invalid_inputs() {
    const auto update = [](world::Rose& rose) {
        world::update_rose(rose, 0.1F, 250, 0);
    };
    auto rose = world::Rose{};
    for (int type : {28, 35, 70}) {
        rose.type = type;
        rejected_without_mutation(rose, update);
    }
    rose = {};
    rose.points = 50;
    rejected_without_mutation(rose, update);
    rejected_without_mutation(rose, [](auto& value) {
        static_cast<void>(world::hit_rose(value, 5));
    });
    rose = {};
    rose.frame = 1;
    rejected_without_mutation(rose, update);
    rose = {};
    rose.animation_time = -1;
    rejected_without_mutation(rose, update);
    rose = {};
    rose.world_y = 397;
    rejected_without_mutation(rose, update);
    for (float delta : {-0.1F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        rejected_without_mutation({}, [delta](auto& value) {
            world::update_rose(value, delta, 0, 0);
        });
    }
    for (float camera : {-1.0F, 1281.0F, std::numeric_limits<float>::infinity()}) {
        rejected_without_mutation({}, [camera](auto& value) {
            world::update_rose(value, 0, camera, 0);
        });
    }
    for (int recoil : {-1, 9}) {
        rejected_without_mutation({}, [recoil](auto& value) {
            world::update_rose(value, 0, 0, recoil);
        });
    }
    rose = {};
    rose.type = 69;
    rejected_without_mutation(rose, [](auto& value) {
        world::update_rose(value, std::numeric_limits<float>::max(), 0, 0);
    });
    for (int type : {69, -1}) {
        rose.type = type;
        rejected_without_mutation(rose, [](auto& value) {
            static_cast<void>(world::hit_rose(value, 60));
        });
    }
    for (float remaining : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        rejected_without_mutation({}, [remaining](auto& value) {
            static_cast<void>(world::hit_rose(value, remaining));
        });
    }
}

} // namespace

int main() {
    try {
        original_vectors();
        invalid_inputs();
        std::cout << "PASS: " << updates_cases.size() << " original updates," << impacts_cases.size() << " impacts, drawing/collision, reset and invalid-input atomicity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
