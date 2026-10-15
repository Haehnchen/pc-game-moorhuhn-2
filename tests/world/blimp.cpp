#include "world/blimp.hpp"

#include "support/numeric.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace moorhuhn2;
using moorhuhn2::world::Blimp;
using moorhuhn2::world::BlimpProgress;

namespace {

void check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

using moorhuhn2::test::binary32;

struct SpawnCase {
    std::string id;
    std::vector<std::uint32_t> random_values;
    bool alternate;
    float camera;
    world::Blimp after;
};

struct UpdateCase {
    std::string id;
    world::Blimp before;
    float delta_seconds;
    float camera;
    int recoil;
    world::Blimp after;
    bool smoke;
    bool active_after;
};

struct ImpactCase {
    std::string id;
    world::Blimp before;
    int hits_before;
    std::array<int, 2> pointer;
    world::Blimp after;
    int points;
    world::BlimpPopup popup;
    int hits_after;
};

struct ProgressCase {
    bool alternate_before;
    int hits;
    bool request_notice;
    bool alternate_after;
};

const std::array<SpawnCase, 4> initializers_cases = {
    {SpawnCase{"right-normal", {0U, 31U, 32767U, 15U}, false, 640.0F, world::Blimp{24, 4, 587.0F, 53.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 5.0F, -35, 15.0F, 0.0F}},
        SpawnCase{"left-normal", {32767U, 59U, 0U}, false, 640.0F, world::Blimp{25, 4, 1438.0F, 1.0F, -1024.0F, -1024.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}},
        SpawnCase{"right-alternate-camera-trunc", {2U, 0U, 60U, 32767U}, true, 0.8333333134651184F,
            world::Blimp{24, 4, -149.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 5.0F, -35, 15.0F, 0.0F}},
        SpawnCase{"left-alternate-limit-camera", {1U, 0U, 7U}, true, 1280.0F, world::Blimp{25, 4, 2206.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 3, 1, 5.0F, -35, 7.0F, 0.0F}}}};

const std::array<UpdateCase, 15> updates_cases = {
    {UpdateCase{"live-right-retained-screen-sum", world::Blimp{24, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, 3.814697265625e-06F, 640.0F, 8,
         world::Blimp{24, 4, binary32(0x44400001U), 60.0F, binary32(0x38700000U), binary32(0x4281999aU), 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, false, true},
        UpdateCase{"live-left-retained-screen-sum", world::Blimp{25, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, 3.814697265625e-06F, 640.0F, 8,
            world::Blimp{25, 4, binary32(0x443fffffU), 60.0F, binary32(0xb8700000U), binary32(0x4281999aU), 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, false, true},
        UpdateCase{"live-right-at-limit", world::Blimp{24, 4, 2400.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, 0.0F, 1280.0F, 0,
            world::Blimp{24, 4, 2400.0F, 60.0F, 864.0F, 60.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, false, true},
        UpdateCase{"live-right-beyond-limit", world::Blimp{24, 4, 2399.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, 0.125F, 1280.0F, 0,
            world::Blimp{-1, 4, binary32(0x45160e00U), 60.0F, binary32(0x44583800U), 60.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, false, false},
        UpdateCase{"live-left-at-limit", world::Blimp{25, 4, -120.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, 0.0F, 0.0F, 0,
            world::Blimp{25, 4, -120.0F, 60.0F, -120.0F, 60.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, false, true},
        UpdateCase{"live-left-beyond-limit", world::Blimp{25, 4, -119.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, 0.125F, 0.0F, 0,
            world::Blimp{-1, 4, binary32(0xc2f1c000U), 60.0F, binary32(0xc2f1c000U), 60.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, false, false},
        UpdateCase{"fall-right-exact-timer-limit", world::Blimp{64, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.375F, 0, 1, 5.0F, -35, 20.0F, 0.0F}, 0.125F, 640.0F, 4,
            world::Blimp{64, 4, binary32(0x44407800U), binary32(0x42750000U), binary32(0x3ff00000U), binary32(0x427e999aU), 1.0F, 0, 1, 5.0F, -35, 20.0F, 0.0F}, false, true},
        UpdateCase{"fall-left-exact-timer-limit", world::Blimp{65, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.375F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, 0.125F, 640.0F, 4,
            world::Blimp{65, 4, binary32(0x443f8800U), binary32(0x42750000U), binary32(0xbff00000U), binary32(0x427e999aU), 1.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, false, true},
        UpdateCase{"fall-right-stored-subtraction", world::Blimp{64, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, 0.20000000298023224F, 640.0F, 0,
            world::Blimp{64, 4, 771.0F, 62.0F, 3.0F, 62.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, true, true},
        UpdateCase{"fall-left-retained-subtraction", world::Blimp{65, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 1, 1, 5.0F, -35, 0.0F, 0.0F}, 0.20000000298023224F, 640.0F, 0,
            world::Blimp{65, 4, 765.0F, 62.0F, -3.0F, 62.0F, binary32(0x32800000U), 1, 1, 5.0F, -35, 0.0F, 0.0F}, true, true},
        UpdateCase{"fall-coarse-single-smoke", world::Blimp{64, 4, 100.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 5.0F, -35, 0.0F, 0.0F}, 0.75F, 0.0F, 2,
            world::Blimp{64, 4, binary32(0x42de8000U), binary32(0x42870000U), binary32(0x42de8000U), binary32(0x42896666U), binary32(0x40300000U), 2, 1, 5.0F, -35, 0.0F, 0.0F},
            true, true},
        UpdateCase{"fall-at-height-limit", world::Blimp{65, 4, 0.0F, 250.0F, -1024.0F, -1024.0F, 0.0F, 3, 1, 5.0F, -35, 0.0F, 0.0F}, 0.0F, 0.0F, 0,
            world::Blimp{65, 4, 0.0F, 250.0F, 0.0F, 250.0F, 0.0F, 3, 1, 5.0F, -35, 0.0F, 0.0F}, false, true},
        UpdateCase{"fall-height-expiry-still-emits", world::Blimp{65, 4, 0.0F, 250.0F, -1024.0F, -1024.0F, 0.0F, 3, 1, 5.0F, -35, 12.0F, 25.0F}, 0.25F, 0.0F, 0,
            world::Blimp{-1, 4, binary32(0xc0700000U), binary32(0x437c8000U), binary32(0xc0700000U), binary32(0x437c8000U), binary32(0x3e800000U), 3, 1, 5.0F, -35, 12.0F, 25.0F},
            true, false},
        UpdateCase{"fall-world-expiry-still-emits", world::Blimp{64, 4, 2400.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 2, 1, 5.0F, -35, 32.0F, 16.0F}, 0.25F, 0.0F, 0,
            world::Blimp{-1, 4, binary32(0x45163c00U), binary32(0x427a0000U), binary32(0x45163c00U), binary32(0x427a0000U), binary32(0x3e800000U), 2, 1, 5.0F, -35, 32.0F, 16.0F},
            true, false},
        UpdateCase{"fall-reloads-stored-screen-x", world::Blimp{64, 4, 768.0F, 60.0F, -1024.0F, -1024.0F, 0.0F, 0, 1, 5.0F, -35, 0.0F, 0.0F}, 3.814697265625e-06F, 640.0F, 0,
            world::Blimp{64, 4, binary32(0x44400001U), binary32(0x4270000aU), binary32(0x38800000U), binary32(0x4270000aU), binary32(0x37a00000U), 0, 1, 5.0F, -35, 0.0F, 0.0F},
            false, true}}};

const std::array<ImpactCase, 2> impacts_cases = {
    {ImpactCase{"normal-right", world::Blimp{24, 4, 880.0F, 30.0F, 112.75F, 34.79999923706055F, 0.0F, 0, 1, 5.0F, -35, 15.0F, 0.0F}, 0, {{115, 20}},
         world::Blimp{64, 4, 880.0F, 30.0F, binary32(0x42e18000U), binary32(0x420b3333U), 0.0F, 0, 1, 5.0F, -35, binary32(0x41920000U), binary32(0x3f9999a0U)}, -35,
         world::BlimpPopup{910.0F, 60.0F, -35}, 1},
        ImpactCase{"alternate-left-second", world::Blimp{25, 4, 1088.0F, 42.0F, 320.25F, 45.599998474121094F, 0.0F, 3, 1, 5.0F, -35, 0.0F, 0.0F}, 1, {{335, 52}},
            world::Blimp{65, 4, 1088.0F, 42.0F, binary32(0x43a02000U), binary32(0x42366666U), 0.0F, 3, 1, 5.0F, -35, binary32(0x41f60000U), binary32(0x41b33334U)}, -35,
            world::BlimpPopup{1118.0F, 72.0F, -35}, 2}}};

const std::array<ProgressCase, 6> progress_cases = {{ProgressCase{false, 0, false, false}, ProgressCase{false, 1, false, false}, ProgressCase{false, 2, true, true},
    ProgressCase{true, 2, false, true}, ProgressCase{false, 3, false, false}, ProgressCase{true, 3, false, true}}};

void check_state(const Blimp& actual, const Blimp& expected, std::string_view context) {
    check(actual == expected, std::string(context) + ": original state differs");
    check(std::bit_cast<std::uint32_t>(actual.world_x) == std::bit_cast<std::uint32_t>(expected.world_x)
              && std::bit_cast<std::uint32_t>(actual.world_y) == std::bit_cast<std::uint32_t>(expected.world_y)
              && std::bit_cast<std::uint32_t>(actual.screen_x) == std::bit_cast<std::uint32_t>(expected.screen_x)
              && std::bit_cast<std::uint32_t>(actual.screen_y) == std::bit_cast<std::uint32_t>(expected.screen_y)
              && std::bit_cast<std::uint32_t>(actual.animation_time) == std::bit_cast<std::uint32_t>(expected.animation_time)
              && std::bit_cast<std::uint32_t>(actual.animation_rate) == std::bit_cast<std::uint32_t>(expected.animation_rate)
              && std::bit_cast<std::uint32_t>(actual.hit_offset_x) == std::bit_cast<std::uint32_t>(expected.hit_offset_x)
              && std::bit_cast<std::uint32_t>(actual.hit_offset_y) == std::bit_cast<std::uint32_t>(expected.hit_offset_y),
        std::string(context) + ": original binary32 words differ");
}

template <typename Function> void rejects(Function function, std::string_view message) {
    bool rejected{};
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
            const auto id = vector.id;
            const auto draws = vector.random_values;
            std::size_t cursor{};
            BlimpProgress progress{false, vector.alternate, 7};
            const auto actual = moorhuhn2::world::initial_blimp(vector.camera, progress, [&] {
                check(cursor < draws.size(), "Blimp initializer consumed excess random values");
                return draws[cursor++];
            });
            check(actual.has_value() && cursor == draws.size(), "Blimp initializer random order/count differs");
            check_state(*actual, vector.after, id);
            check(progress.active && progress.alternate == vector.alternate && progress.hits == 7, "Blimp spawn gate changed unrelated progress");
            const auto before_gate = progress;
            const auto rejected = moorhuhn2::world::initial_blimp(vector.camera, progress, [&]() -> std::uint32_t {
                throw std::runtime_error("Occupied blimp gate consumed RNG");
            });
            check(!rejected && progress == before_gate, "Occupied blimp gate did not reject without mutation");
        }
        for (const auto& vector : updates_cases) {
            auto actual = vector.before;
            BlimpProgress progress{true, true, 7};
            const bool smoke = moorhuhn2::world::update_blimp(actual, vector.delta_seconds, vector.camera, vector.recoil, progress);
            check_state(actual, vector.after, vector.id);
            check(smoke == vector.smoke && progress.active == vector.active_after && progress.alternate && progress.hits == 7, "Original blimp smoke or gate transition differs");
            check(moorhuhn2::world::blimp_draw(actual).has_value() == (actual.type != -1), "Removed blimp remained drawable");
            check(moorhuhn2::world::blimp_collision(actual).has_value() == (actual.type == 24 || actual.type == 25), "Falling/removed blimp remained selectable");
        }
        for (const auto& vector : impacts_cases) {
            auto actual = vector.before;
            BlimpProgress progress{true, actual.frame >= 2, vector.hits_before};
            const auto pointer = vector.pointer;
            const auto impact = moorhuhn2::world::hit_blimp(actual, {pointer[0], pointer[1]}, progress);
            check_state(actual, vector.after, vector.id);
            check(impact.points == vector.points && impact.popup.points == vector.popup.points && impact.popup.world_x == vector.popup.world_x
                      && impact.popup.world_y == vector.popup.world_y,
                "Original blimp penalty popup differs");
            check(progress.active && progress.hits == vector.hits_after && progress.alternate == (actual.frame >= 2), "Blimp impact progress differs");
            check(!moorhuhn2::world::blimp_collision(actual), "Hit blimp remained selectable");
            const auto before_repeated = actual;
            const auto progress_before = progress;
            rejects(
                [&] {
                    static_cast<void>(moorhuhn2::world::hit_blimp(actual, {pointer[0], pointer[1]}, progress));
                },
                "Falling blimp accepted a repeated hit");
            check(actual == before_repeated && progress == progress_before, "Rejected repeated hit mutated blimp progress");
        }
        for (const auto& vector : progress_cases) {
            BlimpProgress progress{true, vector.alternate_before, vector.hits};
            const auto requested = moorhuhn2::world::update_blimp_progress(progress);
            check(requested == vector.request_notice && progress.alternate == vector.alternate_after && progress.active && progress.hits == vector.hits,
                "Original exact-two-hit alternate notice gate differs");
            check(!moorhuhn2::world::update_blimp_progress(progress), "Blimp alternate notice retriggered");
        }

        for (const int type : {24, 25, 64, 65}) {
            for (int frame = 0; frame < 4; ++frame) {
                Blimp blimp;
                blimp.type = type;
                blimp.frame = frame;
                blimp.screen_x = -0.75F;
                blimp.screen_y = -1.75F;
                const auto draw = moorhuhn2::world::blimp_draw(blimp);
                check(draw && draw->image == "gfx/blimp" && draw->source.x == 0 && draw->source.y == frame * 60 && draw->source.width == 120 && draw->source.height == 60
                          && draw->destination.x == 0 && draw->destination.y == -1 && draw->color_key && !draw->mirror_x,
                    "Original blimp stored cell or truncated draw position differs");
            }
        }

        Blimp inactive;
        BlimpProgress reset;
        check(!reset.active && !reset.alternate && reset.hits == 0 && !moorhuhn2::world::blimp_draw(inactive) && !moorhuhn2::world::blimp_collision(inactive),
            "New-round blimp state differs");
        rejects(
            [&] {
                static_cast<void>(moorhuhn2::world::update_blimp(inactive, 0, 0, 0, reset));
            },
            "Inactive blimp accepted an update");
        rejects(
            [&] {
                static_cast<void>(moorhuhn2::world::initial_blimp(0, reset, [] {
                    return 32768U;
                }));
            },
            "Blimp accepted an invalid CRT draw");
        check(reset == BlimpProgress{}, "Rejected invalid allocation changed the spawn gate");
        Blimp live;
        live.type = 24;
        const auto before_invalid = live;
        rejects(
            [&] {
                static_cast<void>(moorhuhn2::world::update_blimp(live, -1, 0, 0, reset));
            },
            "Blimp accepted negative elapsed time");
        rejects(
            [&] {
                static_cast<void>(moorhuhn2::world::update_blimp(live, std::numeric_limits<float>::quiet_NaN(), 0, 0, reset));
            },
            "Blimp accepted nonfinite elapsed time");
        check(live == before_invalid, "Rejected blimp update changed model state");
        reset.hits = std::numeric_limits<int>::max();
        static_cast<void>(moorhuhn2::world::hit_blimp(live, {0, 0}, reset));
        check(reset.hits == std::numeric_limits<int>::min(), "Original blimp hit counter integer wrap differs");
        std::cout << "Original blimp allocation gates/RNG, fixed variants, stored motion, strict removal, smoke cadence, penalties and exact-two-hit notice passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
