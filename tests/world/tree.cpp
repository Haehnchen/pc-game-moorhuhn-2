#include "world/tree.hpp"

#include "support/numeric.hpp"
#include "support/reject_unchanged.hpp"
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

struct DrawExpectation {
    std::string image;
    std::array<int, 4> source;
    std::array<int, 2> destination;
};

struct TreeUpdateCase {
    std::string id;
    world::Tree before;
    float delta;
    float camera;
    int recoil;
    world::Tree after;
    bool ground_sound;
};

struct FragmentUpdateCase {
    std::string id;
    world::TreeFragment before;
    float delta;
    float camera;
    int recoil;
    world::TreeFragment after;
    bool ground_sound;
    bool highfly_sound;
};

struct ImpactCase {
    std::string id;
    world::Tree before;
    std::optional<std::array<std::uint32_t, 2>> random;
    int branch_hits;
    // Start-fall rows have no damage call and therefore no predicate.
    std::optional<bool> destroyed;
    world::Tree after;
};

struct LeafFactoryCase {
    float camera;
    int pointer_x;
    std::array<std::uint32_t, 6> random;
    world::TreeFragment state;
};

struct BranchFactoryCase {
    float camera;
    int pointer_x;
    std::array<std::uint32_t, 5> random;
    world::TreeFragment state;
};

struct HighflyFactoryCase {
    float camera;
    int pointer_x;
    world::TreeFragment state;
};

struct LeafHitCase {
    world::TreeFragment before;
    world::TreeFragment after;
    world::TreeLeafImpact impact;
};

struct TreeDrawCase {
    world::Tree state;
    DrawExpectation draw;
};

struct FragmentDrawCase {
    world::TreeFragment state;
    DrawExpectation draw;
};

struct ResetCase {
    std::array<world::Tree, 4> native_initial;
    std::array<world::Tree, 4> raw_initial;
    std::array<world::Tree, 4> first_update;
};

const std::array<world::Tree, 4> initial_cases = {
    {world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
        world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}};

struct FallStep {
    world::Tree after;
    int recoil;
    bool ground_sound;
};

const auto tree_updates_cases = [] {
    std::vector<TreeUpdateCase> cases;
    constexpr std::array cameras{0.0F, binary32(0x3dcccccdU), 320.0F, 1280.0F, binary32(0x447a1000U)};
    constexpr std::array offsets{0, 0, 512, 2048, 1600};
    constexpr std::array recoils{0, 7, 8, 3, 4};
    for (std::size_t tree = 0; tree < initial_cases.size(); ++tree) {
        for (std::size_t camera = 0; camera < cameras.size(); ++camera) {
            auto after = initial_cases[tree];
            after.screen_x = after.world_x - offsets[camera];
            after.screen_y = after.world_y + recoils[camera];
            cases.push_back({"live" + std::to_string(tree) + "-camera" + std::to_string(cameras[camera]), initial_cases[tree], binary32(0x3e000000U), cameras[camera],
                recoils[camera], after, false});
        }
    }
    {
        auto before = world::Tree{66, 147.0F, -13.0F, 0.0F, 0.0F, binary32(0xbf000000U), 0.0F, 0.0F, 0.0F, 1, 0, 0};
        const std::array trace{FallStep{world::Tree{66, binary32(0x430cbc00U), binary32(0xc1370000U), binary32(0x430cbc00U), binary32(0xc1370000U), binary32(0xbf0051ecU),
                                            binary32(0x3e000000U), 0.0F, 0.0F, 1, 0, 0},
                                   0, false},
            FallStep{world::Tree{66, binary32(0x43067400U), binary32(0xc1050000U), binary32(0x43067400U), binary32(0xc0ea0000U), binary32(0xbf00a3d8U), binary32(0x3e800000U), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x43002800U), binary32(0xc0680000U), binary32(0x43002800U), binary32(0xbfd00000U), binary32(0xbf00f5c4U), binary32(0x3ec00000U), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x42f3b000U), binary32(0x40280000U), binary32(0x42f3b000U), binary32(0x40b40000U), binary32(0xbf0147b0U), binary32(0x3f000000U), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x42e70800U), binary32(0x41270000U), binary32(0x42e70800U), binary32(0x41670000U), binary32(0xbf01999cU), binary32(0x3f200000U), 0.0F,
                         0.0F, 1, 0, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x42da5800U), binary32(0x419e8000U), binary32(0x42da5800U), binary32(0x41c68000U), binary32(0xbf01eb88U), binary32(0x3f400000U), 0.0F,
                         0.0F, 1, 0, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x42cda000U), binary32(0x41f60000U), binary32(0x42cda000U), binary32(0x42130000U), binary32(0xbf023d74U), binary32(0x3f600000U), 0.0F,
                         0.0F, 1, 0, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x42c0e000U), binary32(0x422d0000U), binary32(0x42c0e000U), binary32(0x42490000U), binary32(0xbf028f60U), 1.0F, 0.0F, 0.0F, 1, 0, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x42b41800U), binary32(0x42654000U), binary32(0x42b41800U), binary32(0x4282a000U), binary32(0xbf02e14cU), binary32(0x3f900000U), 0.0F,
                         0.0F, 1, 0, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x42a74800U), binary32(0x4291e000U), binary32(0x42a74800U), binary32(0x4291e000U), binary32(0xbf033338U), binary32(0x3fa00000U), 0.0F,
                         0.0F, 1, 0, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x429a6fffU), binary32(0x42b44000U), binary32(0x429a6fffU), binary32(0x42b64000U), binary32(0xbf038524U), binary32(0x3fb00000U), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x428d8ffeU), binary32(0x42d9c000U), binary32(0x428d8ffeU), binary32(0x42ddc000U), binary32(0xbf03d710U), binary32(0x3fc00000U), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x4280a7fdU), binary32(0x43013000U), binary32(0x4280a7fdU), binary32(0x43043000U), binary32(0xbf0428fcU), binary32(0x3fd00000U), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x42676ff9U), binary32(0x43171000U), binary32(0x42676ff9U), binary32(0x431b1000U), binary32(0xbf047ae8U), binary32(0x3fe00000U), 0.0F,
                         0.0F, 1, 0, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x424d7ff8U), binary32(0x432e8000U), binary32(0x424d7ff8U), binary32(0x43338000U), binary32(0xbf04ccd4U), binary32(0x3ff00000U), 0.0F,
                         0.0F, 1, 0, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x42337ff6U), binary32(0x43478000U), binary32(0x42337ff6U), binary32(0x434d8000U), binary32(0xbf051ec0U), 2.0F, 0.0F, 0.0F, 1, 0, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x42196ff4U), binary32(0x43621000U), binary32(0x42196ff4U), binary32(0x43691000U), binary32(0xbf0570acU), binary32(0x40080000U), 0.0F,
                         0.0F, 1, 0, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x41fe9fe5U), binary32(0x437e3000U), binary32(0x41fe9fe5U), binary32(0x43831800U), binary32(0xbf05c298U), binary32(0x40100000U), 0.0F,
                         0.0F, 1, 0, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x41ca3fe1U), binary32(0x438df000U), binary32(0x41ca3fe1U), binary32(0x438df000U), binary32(0xbf061484U), binary32(0x40180000U), 0.0F,
                         0.0F, 1, 0, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x4195bfddU), binary32(0x439d9000U), binary32(0x4195bfddU), binary32(0x439e1000U), binary32(0xbf066670U), binary32(0x40200000U), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x41423fb2U), binary32(0x43adf800U), binary32(0x41423fb2U), binary32(0x43aef800U), binary32(0xbf06b85cU), binary32(0x40280000U), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x40b17f54U), binary32(0x43bf2800U), binary32(0x40b17f54U), binary32(0x43c0a800U), binary32(0xbf070a48U), binary32(0x40300000U), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0xbf8802f5U), binary32(0x43d12000U), binary32(0xbf8802f5U), binary32(0x43d32000U), binary32(0xbf075c34U), binary32(0x40380000U), 0.0F,
                         0.0F, 1, 0, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0xc0f600cfU), binary32(0x43e3e000U), binary32(0xc0f600cfU), binary32(0x43e66000U), binary32(0xbf07ae20U), 3.0F, 0.0F, 0.0F, 1, 0, 0},
                5, false},
            FallStep{world::Tree{-1, binary32(0xc1654071U), binary32(0x43f76800U), binary32(0xc1654071U), binary32(0x43fa6800U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F,
                         0.0F, 1, 0, 0},
                6, true}};
        for (std::size_t step = 0; step < trace.size(); ++step) {
            const auto& expected = trace[step];
            cases.push_back({"fall1-[0, 0]-" + std::to_string(step), before, binary32(0x3e000000U), 0.0F, expected.recoil, expected.after, expected.ground_sound});
            before = expected.after;
        }
    }
    cases.push_back(TreeUpdateCase{"removed-no-second-ground#1",
        world::Tree{
            -1, binary32(0xc1654071U), binary32(0x43f76800U), binary32(0xc1654071U), binary32(0x43fa6800U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 0.0F, 1, 0, 0},
        1.0F, 0.0F, 0,
        world::Tree{
            -1, binary32(0xc1654071U), binary32(0x43f76800U), binary32(0xc1654071U), binary32(0x43fa6800U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 0.0F, 1, 0, 0},
        false});
    {
        auto before = world::Tree{66, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 0.0F, 1, 0, 0};
        const std::array trace{FallStep{world::Tree{66, binary32(0x4312fc00U), binary32(0x41450001U), binary32(0x4312fc00U), binary32(0x41450001U), binary32(0xbaa3d70aU),
                                            binary32(0x4001999aU), 0.0F, 0.0F, 1, 0, 0},
                                   0, false},
            FallStep{world::Tree{66, binary32(0x4312f400U), binary32(0x421cc001U), binary32(0x4312f400U), binary32(0x4220c001U), binary32(0xbb23d70aU), binary32(0x4009999aU), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x4312e800U), binary32(0x42874001U), binary32(0x4312e800U), binary32(0x428b4001U), binary32(0xbb75c28fU), binary32(0x4011999aU), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x4312d800U), binary32(0x42c34001U), binary32(0x4312d800U), binary32(0x42c94001U), binary32(0xbba3d70aU), binary32(0x4019999aU), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x4312c400U), binary32(0x43013001U), binary32(0x4312c400U), binary32(0x43053001U), binary32(0xbbccccccU), binary32(0x4021999aU), 0.0F,
                         0.0F, 1, 0, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x4312ac00U), binary32(0x43225001U), binary32(0x4312ac00U), binary32(0x43275001U), binary32(0xbbf5c28eU), binary32(0x4029999aU), 0.0F,
                         0.0F, 1, 0, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x43129000U), binary32(0x43450001U), binary32(0x43129000U), binary32(0x434b0001U), binary32(0xbc0f5c28U), binary32(0x4031999aU), 0.0F,
                         0.0F, 1, 0, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x43127000U), binary32(0x43694001U), binary32(0x43127000U), binary32(0x43704001U), binary32(0xbc23d709U), binary32(0x4039999aU), 0.0F,
                         0.0F, 1, 0, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x43124c00U), binary32(0x43878801U), binary32(0x43124c00U), binary32(0x438b8801U), binary32(0xbc3851eaU), binary32(0x4041999aU), 0.0F,
                         0.0F, 1, 0, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x43122400U), binary32(0x439b3801U), binary32(0x43122400U), binary32(0x439b3801U), binary32(0xbc4ccccbU), binary32(0x4049999aU), 0.0F,
                         0.0F, 1, 0, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x4311f800U), binary32(0x43afb001U), binary32(0x4311f800U), binary32(0x43b03001U), binary32(0xbc6147acU), binary32(0x4051999aU), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x4311c800U), binary32(0x43c4f001U), binary32(0x4311c800U), binary32(0x43c5f001U), binary32(0xbc75c28dU), binary32(0x4059999aU), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x43119400U), binary32(0x43daf801U), binary32(0x43119400U), binary32(0x43dc7801U), binary32(0xbc851eb7U), binary32(0x4061999aU), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{-1, binary32(0x43115c00U), binary32(0x43f1c801U), binary32(0x43115c00U), binary32(0x43f3c801U), binary32(0xbc8f5c28U), binary32(0x4069999aU), 0.0F,
                         0.0F, 1, 0, 0},
                4, true}};
        for (std::size_t step = 0; step < trace.size(); ++step) {
            const auto& expected = trace[step];
            cases.push_back({"fall1-[5, 19]-" + std::to_string(step), before, binary32(0x3e000000U), 0.0F, expected.recoil, expected.after, expected.ground_sound});
            before = expected.after;
        }
    }
    cases.push_back(TreeUpdateCase{"removed-no-second-ground#2",
        world::Tree{
            -1, binary32(0x43115c00U), binary32(0x43f1c801U), binary32(0x43115c00U), binary32(0x43f3c801U), binary32(0xbc8f5c28U), binary32(0x4069999aU), 0.0F, 0.0F, 1, 0, 0},
        1.0F, 0.0F, 0,
        world::Tree{
            -1, binary32(0x43115c00U), binary32(0x43f1c801U), binary32(0x43115c00U), binary32(0x43f3c801U), binary32(0xbc8f5c28U), binary32(0x4069999aU), 0.0F, 0.0F, 1, 0, 0},
        false});
    {
        auto before = world::Tree{66, 147.0F, -13.0F, 0.0F, 0.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 0.0F, 1, 0, 0};
        const std::array trace{FallStep{world::Tree{66, binary32(0x43158400U), binary32(0xc02c0001U), binary32(0x43158400U), binary32(0xc02c0001U), binary32(0x3e4e147bU),
                                            binary32(0x3f533333U), 0.0F, 0.0F, 1, 0, 0},
                                   0, false},
            FallStep{world::Tree{66, binary32(0x43180c00U), binary32(0x41130000U), binary32(0x43180c00U), binary32(0x41230000U), binary32(0x3e4f5c29U), binary32(0x3f733333U), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x431a9800U), binary32(0x41b50000U), binary32(0x431a9800U), binary32(0x41c50000U), binary32(0x3e50a3d7U), binary32(0x3f89999aU), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x431d2800U), binary32(0x42168000U), binary32(0x431d2800U), binary32(0x42228000U), binary32(0x3e51eb85U), binary32(0x3f99999aU), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x431fbc00U), binary32(0x4258c000U), binary32(0x431fbc00U), binary32(0x4268c000U), binary32(0x3e533333U), binary32(0x3fa9999aU), 0.0F,
                         0.0F, 1, 0, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x43225400U), binary32(0x4290a000U), binary32(0x43225400U), binary32(0x429aa000U), binary32(0x3e547ae1U), binary32(0x3fb9999aU), 0.0F,
                         0.0F, 1, 0, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x4324f000U), 92.0F, binary32(0x4324f000U), 98.0F, binary32(0x3e55c28fU), binary32(0x3fc9999aU), 0.0F, 0.0F, 1, 0, 0}, 6, false},
            FallStep{world::Tree{66, binary32(0x43279000U), binary32(0x42e28000U), binary32(0x43279000U), binary32(0x42f08000U), binary32(0x3e570a3dU), binary32(0x3fd9999aU), 0.0F,
                         0.0F, 1, 0, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x432a3400U), binary32(0x43081000U), binary32(0x432a3400U), binary32(0x43101000U), binary32(0x3e5851ebU), binary32(0x3fe9999aU), 0.0F,
                         0.0F, 1, 0, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x432cdc00U), binary32(0x43207000U), binary32(0x432cdc00U), binary32(0x43207000U), binary32(0x3e599999U), binary32(0x3ff9999aU), 0.0F,
                         0.0F, 1, 0, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x432f8800U), binary32(0x433a6000U), binary32(0x432f8800U), binary32(0x433b6000U), binary32(0x3e5ae147U), binary32(0x4004cccdU), 0.0F,
                         0.0F, 1, 0, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x43323800U), binary32(0x4355e000U), binary32(0x43323800U), binary32(0x4357e000U), binary32(0x3e5c28f5U), binary32(0x400ccccdU), 0.0F,
                         0.0F, 1, 0, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x4334ec00U), binary32(0x4372f000U), binary32(0x4334ec00U), binary32(0x4375f000U), binary32(0x3e5d70a3U), binary32(0x4014cccdU), 0.0F,
                         0.0F, 1, 0, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x4337a400U), binary32(0x4388c800U), binary32(0x4337a400U), binary32(0x438ac800U), binary32(0x3e5eb851U), binary32(0x401ccccdU), 0.0F,
                         0.0F, 1, 0, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x433a6000U), binary32(0x4398e000U), binary32(0x433a6000U), binary32(0x439b6000U), binary32(0x3e5fffffU), binary32(0x4024cccdU), 0.0F,
                         0.0F, 1, 0, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x433d2000U), binary32(0x43a9c000U), binary32(0x433d2000U), binary32(0x43acc000U), binary32(0x3e6147adU), binary32(0x402ccccdU), 0.0F,
                         0.0F, 1, 0, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x433fe400U), binary32(0x43bb6800U), binary32(0x433fe400U), binary32(0x43bee800U), binary32(0x3e628f5bU), binary32(0x4034cccdU), 0.0F,
                         0.0F, 1, 0, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x4342ac00U), binary32(0x43cdd800U), binary32(0x4342ac00U), binary32(0x43d1d800U), binary32(0x3e63d709U), binary32(0x403ccccdU), 0.0F,
                         0.0F, 1, 0, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x43457800U), binary32(0x43e11000U), binary32(0x43457800U), binary32(0x43e11000U), binary32(0x3e651eb7U), binary32(0x4044cccdU), 0.0F,
                         0.0F, 1, 0, 0},
                0, false},
            FallStep{world::Tree{-1, binary32(0x43484800U), binary32(0x43f51000U), binary32(0x43484800U), binary32(0x43f59000U), binary32(0x3e666665U), binary32(0x404ccccdU), 0.0F,
                         0.0F, 1, 0, 0},
                1, true}};
        for (std::size_t step = 0; step < trace.size(); ++step) {
            const auto& expected = trace[step];
            cases.push_back({"fall1-[32767, 32767]-" + std::to_string(step), before, binary32(0x3e000000U), 0.0F, expected.recoil, expected.after, expected.ground_sound});
            before = expected.after;
        }
    }
    cases.push_back(TreeUpdateCase{"removed-no-second-ground#3",
        world::Tree{
            -1, binary32(0x43484800U), binary32(0x43f51000U), binary32(0x43484800U), binary32(0x43f59000U), binary32(0x3e666665U), binary32(0x404ccccdU), 0.0F, 0.0F, 1, 0, 0},
        1.0F, 0.0F, 0,
        world::Tree{
            -1, binary32(0x43484800U), binary32(0x43f51000U), binary32(0x43484800U), binary32(0x43f59000U), binary32(0x3e666665U), binary32(0x404ccccdU), 0.0F, 0.0F, 1, 0, 0},
        false});
    {
        auto before = world::Tree{66, 2067.0F, -13.0F, 0.0F, 0.0F, binary32(0xbf000000U), 0.0F, 0.0F, 0.0F, 1, 2, 0};
        const std::array trace{FallStep{world::Tree{66, binary32(0x4500cbc0U), binary32(0xc1370000U), binary32(0x414bc000U), binary32(0xc1370000U), binary32(0xbf0051ecU),
                                            binary32(0x3e000000U), 0.0F, 0.0F, 1, 2, 0},
                                   0, false},
            FallStep{world::Tree{66, binary32(0x45006740U), binary32(0xc1050000U), binary32(0x40ce8000U), binary32(0xc0ea0000U), binary32(0xbf00a3d8U), binary32(0x3e800000U), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x45000280U), binary32(0xc0680000U), binary32(0x3e200000U), binary32(0xbfd00000U), binary32(0xbf00f5c4U), binary32(0x3ec00000U), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x44ff3b00U), binary32(0x40280000U), binary32(0xc0c50000U), binary32(0x40b40000U), binary32(0xbf0147b0U), binary32(0x3f000000U), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x44fe7080U), binary32(0x41270000U), binary32(0xc147c000U), binary32(0x41670000U), binary32(0xbf01999cU), binary32(0x3f200000U), 0.0F,
                         0.0F, 1, 2, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x44fda580U), binary32(0x419e8000U), binary32(0xc196a000U), binary32(0x41c68000U), binary32(0xbf01eb88U), binary32(0x3f400000U), 0.0F,
                         0.0F, 1, 2, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x44fcda00U), binary32(0x41f60000U), binary32(0xc1c98000U), binary32(0x42130000U), binary32(0xbf023d74U), binary32(0x3f600000U), 0.0F,
                         0.0F, 1, 2, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x44fc0e00U), binary32(0x422d0000U), binary32(0xc1fc8000U), binary32(0x42490000U), binary32(0xbf028f60U), 1.0F, 0.0F, 0.0F, 1, 2, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x44fb4180U), binary32(0x42654000U), binary32(0xc217d000U), binary32(0x4282a000U), binary32(0xbf02e14cU), binary32(0x3f900000U), 0.0F,
                         0.0F, 1, 2, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x44fa7480U), binary32(0x4291e000U), binary32(0xc2317000U), binary32(0x4291e000U), binary32(0xbf033338U), binary32(0x3fa00000U), 0.0F,
                         0.0F, 1, 2, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x44f9a700U), binary32(0x42b44000U), binary32(0xc24b2000U), binary32(0x42b64000U), binary32(0xbf038524U), binary32(0x3fb00000U), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x44f8d900U), binary32(0x42d9c000U), binary32(0xc264e000U), binary32(0x42ddc000U), binary32(0xbf03d710U), binary32(0x3fc00000U), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x44f80a80U), binary32(0x43013000U), binary32(0xc27eb000U), binary32(0x43043000U), binary32(0xbf0428fcU), binary32(0x3fd00000U), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x44f73b80U), binary32(0x43171000U), binary32(0xc28c4800U), binary32(0x431b1000U), binary32(0xbf047ae8U), binary32(0x3fe00000U), 0.0F,
                         0.0F, 1, 2, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x44f66c00U), binary32(0x432e8000U), binary32(0xc2994000U), binary32(0x43338000U), binary32(0xbf04ccd4U), binary32(0x3ff00000U), 0.0F,
                         0.0F, 1, 2, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x44f59c00U), binary32(0x43478000U), binary32(0xc2a64000U), binary32(0x434d8000U), binary32(0xbf051ec0U), 2.0F, 0.0F, 0.0F, 1, 2, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x44f4cb80U), binary32(0x43621000U), binary32(0xc2b34800U), binary32(0x43691000U), binary32(0xbf0570acU), binary32(0x40080000U), 0.0F,
                         0.0F, 1, 2, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x44f3fa80U), binary32(0x437e3000U), binary32(0xc2c05800U), binary32(0x43831800U), binary32(0xbf05c298U), binary32(0x40100000U), 0.0F,
                         0.0F, 1, 2, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x44f32900U), binary32(0x438df000U), binary32(0xc2cd7000U), binary32(0x438df000U), binary32(0xbf061484U), binary32(0x40180000U), 0.0F,
                         0.0F, 1, 2, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x44f25700U), binary32(0x439d9000U), binary32(0xc2da9000U), binary32(0x439e1000U), binary32(0xbf066670U), binary32(0x40200000U), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x44f18480U), binary32(0x43adf800U), binary32(0xc2e7b800U), binary32(0x43aef800U), binary32(0xbf06b85cU), binary32(0x40280000U), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x44f0b180U), binary32(0x43bf2800U), binary32(0xc2f4e800U), binary32(0x43c0a800U), binary32(0xbf070a48U), binary32(0x40300000U), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x44efde00U), binary32(0x43d12000U), binary32(0xc3011000U), binary32(0x43d32000U), binary32(0xbf075c34U), binary32(0x40380000U), 0.0F,
                         0.0F, 1, 2, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x44ef0a00U), binary32(0x43e3e000U), binary32(0xc307b000U), binary32(0x43e66000U), binary32(0xbf07ae20U), 3.0F, 0.0F, 0.0F, 1, 2, 0},
                5, false},
            FallStep{world::Tree{-1, binary32(0x44ee3580U), binary32(0x43f76800U), binary32(0xc30e5400U), binary32(0x43fa6800U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F,
                         0.0F, 1, 2, 0},
                6, true}};
        for (std::size_t step = 0; step < trace.size(); ++step) {
            const auto& expected = trace[step];
            cases.push_back({"fall3-[0, 0]-" + std::to_string(step), before, binary32(0x3e000000U), 1280.0F, expected.recoil, expected.after, expected.ground_sound});
            before = expected.after;
        }
    }
    cases.push_back(TreeUpdateCase{"removed-no-second-ground#4",
        world::Tree{
            -1, binary32(0x44ee3580U), binary32(0x43f76800U), binary32(0xc30e5400U), binary32(0x43fa6800U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 0.0F, 1, 2, 0},
        1.0F, 0.0F, 0,
        world::Tree{
            -1, binary32(0x44ee3580U), binary32(0x43f76800U), binary32(0xc30e5400U), binary32(0x43fa6800U), binary32(0xbf08000cU), binary32(0x40480000U), 0.0F, 0.0F, 1, 2, 0},
        false});
    {
        auto before = world::Tree{66, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 0.0F, 1, 2, 0};
        const std::array trace{FallStep{world::Tree{66, binary32(0x45012fc0U), binary32(0x41450001U), binary32(0x4197e000U), binary32(0x41450001U), binary32(0xbaa3d70aU),
                                            binary32(0x4001999aU), 0.0F, 0.0F, 1, 2, 0},
                                   0, false},
            FallStep{world::Tree{66, binary32(0x45012f40U), binary32(0x421cc001U), binary32(0x4197a000U), binary32(0x4220c001U), binary32(0xbb23d70aU), binary32(0x4009999aU), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x45012e80U), binary32(0x42874001U), binary32(0x41974000U), binary32(0x428b4001U), binary32(0xbb75c28fU), binary32(0x4011999aU), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x45012d80U), binary32(0x42c34001U), binary32(0x4196c000U), binary32(0x42c94001U), binary32(0xbba3d70aU), binary32(0x4019999aU), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x45012c40U), binary32(0x43013001U), binary32(0x41962000U), binary32(0x43053001U), binary32(0xbbccccccU), binary32(0x4021999aU), 0.0F,
                         0.0F, 1, 2, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x45012ac0U), binary32(0x43225001U), binary32(0x41956000U), binary32(0x43275001U), binary32(0xbbf5c28eU), binary32(0x4029999aU), 0.0F,
                         0.0F, 1, 2, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x45012900U), binary32(0x43450001U), binary32(0x41948000U), binary32(0x434b0001U), binary32(0xbc0f5c28U), binary32(0x4031999aU), 0.0F,
                         0.0F, 1, 2, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x45012700U), binary32(0x43694001U), binary32(0x41938000U), binary32(0x43704001U), binary32(0xbc23d709U), binary32(0x4039999aU), 0.0F,
                         0.0F, 1, 2, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x450124c0U), binary32(0x43878801U), binary32(0x41926000U), binary32(0x438b8801U), binary32(0xbc3851eaU), binary32(0x4041999aU), 0.0F,
                         0.0F, 1, 2, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x45012240U), binary32(0x439b3801U), binary32(0x41912000U), binary32(0x439b3801U), binary32(0xbc4ccccbU), binary32(0x4049999aU), 0.0F,
                         0.0F, 1, 2, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x45011f80U), binary32(0x43afb001U), binary32(0x418fc000U), binary32(0x43b03001U), binary32(0xbc6147acU), binary32(0x4051999aU), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x45011c80U), binary32(0x43c4f001U), binary32(0x418e4000U), binary32(0x43c5f001U), binary32(0xbc75c28dU), binary32(0x4059999aU), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x45011940U), binary32(0x43daf801U), binary32(0x418ca000U), binary32(0x43dc7801U), binary32(0xbc851eb7U), binary32(0x4061999aU), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{-1, binary32(0x450115c0U), binary32(0x43f1c801U), binary32(0x418ae000U), binary32(0x43f3c801U), binary32(0xbc8f5c28U), binary32(0x4069999aU), 0.0F,
                         0.0F, 1, 2, 0},
                4, true}};
        for (std::size_t step = 0; step < trace.size(); ++step) {
            const auto& expected = trace[step];
            cases.push_back({"fall3-[5, 19]-" + std::to_string(step), before, binary32(0x3e000000U), 1280.0F, expected.recoil, expected.after, expected.ground_sound});
            before = expected.after;
        }
    }
    cases.push_back(TreeUpdateCase{"removed-no-second-ground#5",
        world::Tree{
            -1, binary32(0x450115c0U), binary32(0x43f1c801U), binary32(0x418ae000U), binary32(0x43f3c801U), binary32(0xbc8f5c28U), binary32(0x4069999aU), 0.0F, 0.0F, 1, 2, 0},
        1.0F, 0.0F, 0,
        world::Tree{
            -1, binary32(0x450115c0U), binary32(0x43f1c801U), binary32(0x418ae000U), binary32(0x43f3c801U), binary32(0xbc8f5c28U), binary32(0x4069999aU), 0.0F, 0.0F, 1, 2, 0},
        false});
    {
        auto before = world::Tree{66, 2067.0F, -13.0F, 0.0F, 0.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 0.0F, 1, 2, 0};
        const std::array trace{FallStep{world::Tree{66, binary32(0x45015840U), binary32(0xc02c0001U), binary32(0x41ac2000U), binary32(0xc02c0001U), binary32(0x3e4e147bU),
                                            binary32(0x3f533333U), 0.0F, 0.0F, 1, 2, 0},
                                   0, false},
            FallStep{world::Tree{66, binary32(0x450180c0U), binary32(0x41130000U), binary32(0x41c06000U), binary32(0x41230000U), binary32(0x3e4f5c29U), binary32(0x3f733333U), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x4501a980U), binary32(0x41b50000U), binary32(0x41d4c000U), binary32(0x41c50000U), binary32(0x3e50a3d7U), binary32(0x3f89999aU), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x4501d280U), binary32(0x42168000U), binary32(0x41e94000U), binary32(0x42228000U), binary32(0x3e51eb85U), binary32(0x3f99999aU), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x4501fbc0U), binary32(0x4258c000U), binary32(0x41fde000U), binary32(0x4268c000U), binary32(0x3e533333U), binary32(0x3fa9999aU), 0.0F,
                         0.0F, 1, 2, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x45022540U), binary32(0x4290a000U), binary32(0x42095000U), binary32(0x429aa000U), binary32(0x3e547ae1U), binary32(0x3fb9999aU), 0.0F,
                         0.0F, 1, 2, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x45024f00U), 92.0F, binary32(0x4213c000U), 98.0F, binary32(0x3e55c28fU), binary32(0x3fc9999aU), 0.0F, 0.0F, 1, 2, 0}, 6, false},
            FallStep{world::Tree{66, binary32(0x45027900U), binary32(0x42e28000U), binary32(0x421e4000U), binary32(0x42f08000U), binary32(0x3e570a3dU), binary32(0x3fd9999aU), 0.0F,
                         0.0F, 1, 2, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x4502a340U), binary32(0x43081000U), binary32(0x4228d000U), binary32(0x43101000U), binary32(0x3e5851ebU), binary32(0x3fe9999aU), 0.0F,
                         0.0F, 1, 2, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x4502cdc0U), binary32(0x43207000U), binary32(0x42337000U), binary32(0x43207000U), binary32(0x3e599999U), binary32(0x3ff9999aU), 0.0F,
                         0.0F, 1, 2, 0},
                0, false},
            FallStep{world::Tree{66, binary32(0x4502f880U), binary32(0x433a6000U), binary32(0x423e2000U), binary32(0x433b6000U), binary32(0x3e5ae147U), binary32(0x4004cccdU), 0.0F,
                         0.0F, 1, 2, 0},
                1, false},
            FallStep{world::Tree{66, binary32(0x45032380U), binary32(0x4355e000U), binary32(0x4248e000U), binary32(0x4357e000U), binary32(0x3e5c28f5U), binary32(0x400ccccdU), 0.0F,
                         0.0F, 1, 2, 0},
                2, false},
            FallStep{world::Tree{66, binary32(0x45034ec0U), binary32(0x4372f000U), binary32(0x4253b000U), binary32(0x4375f000U), binary32(0x3e5d70a3U), binary32(0x4014cccdU), 0.0F,
                         0.0F, 1, 2, 0},
                3, false},
            FallStep{world::Tree{66, binary32(0x45037a40U), binary32(0x4388c800U), binary32(0x425e9000U), binary32(0x438ac800U), binary32(0x3e5eb851U), binary32(0x401ccccdU), 0.0F,
                         0.0F, 1, 2, 0},
                4, false},
            FallStep{world::Tree{66, binary32(0x4503a600U), binary32(0x4398e000U), binary32(0x42698000U), binary32(0x439b6000U), binary32(0x3e5fffffU), binary32(0x4024cccdU), 0.0F,
                         0.0F, 1, 2, 0},
                5, false},
            FallStep{world::Tree{66, binary32(0x4503d200U), binary32(0x43a9c000U), binary32(0x42748000U), binary32(0x43acc000U), binary32(0x3e6147adU), binary32(0x402ccccdU), 0.0F,
                         0.0F, 1, 2, 0},
                6, false},
            FallStep{world::Tree{66, binary32(0x4503fe40U), binary32(0x43bb6800U), binary32(0x427f9000U), binary32(0x43bee800U), binary32(0x3e628f5bU), binary32(0x4034cccdU), 0.0F,
                         0.0F, 1, 2, 0},
                7, false},
            FallStep{world::Tree{66, binary32(0x45042ac0U), binary32(0x43cdd800U), binary32(0x42855800U), binary32(0x43d1d800U), binary32(0x3e63d709U), binary32(0x403ccccdU), 0.0F,
                         0.0F, 1, 2, 0},
                8, false},
            FallStep{world::Tree{66, binary32(0x45045780U), binary32(0x43e11000U), binary32(0x428af000U), binary32(0x43e11000U), binary32(0x3e651eb7U), binary32(0x4044cccdU), 0.0F,
                         0.0F, 1, 2, 0},
                0, false},
            FallStep{world::Tree{-1, binary32(0x45048480U), binary32(0x43f51000U), binary32(0x42909000U), binary32(0x43f59000U), binary32(0x3e666665U), binary32(0x404ccccdU), 0.0F,
                         0.0F, 1, 2, 0},
                1, true}};
        for (std::size_t step = 0; step < trace.size(); ++step) {
            const auto& expected = trace[step];
            cases.push_back({"fall3-[32767, 32767]-" + std::to_string(step), before, binary32(0x3e000000U), 1280.0F, expected.recoil, expected.after, expected.ground_sound});
            before = expected.after;
        }
    }
    cases.push_back(TreeUpdateCase{"removed-no-second-ground#6",
        world::Tree{
            -1, binary32(0x45048480U), binary32(0x43f51000U), binary32(0x42909000U), binary32(0x43f59000U), binary32(0x3e666665U), binary32(0x404ccccdU), 0.0F, 0.0F, 1, 2, 0},
        1.0F, 0.0F, 0,
        world::Tree{
            -1, binary32(0x45048480U), binary32(0x43f51000U), binary32(0x42909000U), binary32(0x43f59000U), binary32(0x3e666665U), binary32(0x404ccccdU), 0.0F, 0.0F, 1, 2, 0},
        false});
    cases.push_back(TreeUpdateCase{"tree-stored-y-boundary#1", world::Tree{66, 147.0F, 480.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, 0.0F, 0.0F, 8,
        world::Tree{66, 147.0F, 480.0F, 147.0F, 488.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, false});
    cases.push_back(TreeUpdateCase{"tree-stored-y-boundary#2", world::Tree{66, 147.0F, binary32(0x43f00001U), 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, 0.0F, 0.0F, 8,
        world::Tree{-1, 147.0F, binary32(0x43f00001U), 147.0F, binary32(0x43f40001U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, true});
    cases.push_back(TreeUpdateCase{"tree-stored-y-boundary#3", world::Tree{66, 147.0F, binary32(0x43efffffU), 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, 0.0F, 0.0F, 8,
        world::Tree{66, 147.0F, binary32(0x43efffffU), 147.0F, binary32(0x43f3ffffU), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, false});
    return cases;
}();

const std::array<FragmentUpdateCase, 123> fragment_updates_cases = {{FragmentUpdateCase{"initial-leaf-frame-overwritten#1",
                                                                         world::TreeFragment{
                                                                             27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1},
                                                                         0.0F, binary32(0x3dcccccdU), 8,
                                                                         world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, binary32(0x436e28f6U), -201.0F, 69.0F, 74.0F,
                                                                             0.0F, 4.0F, 19, 0, 1},
                                                                         false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#1", world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1},
        binary32(0x3e000000U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 2, binary32(0x4376c8f6U), binary32(0xc347c000U), binary32(0xc3849b85U), binary32(0xc340c000U), 69.0F, 74.0F, binary32(0x40200000U), 4.0F, 19, 2, 1},
        false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#2", world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1},
        binary32(0x3f533333U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 2, binary32(0x43938ae1U), binary32(0xc313f333U), binary32(0xc358ea3eU), binary32(0xc30cf333U), 69.0F, 74.0F, binary32(0x41840000U), 4.0F, 19, 16,
            1},
        false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#3", world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1},
        binary32(0x3f733333U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 2, binary32(0x4397dae1U), binary32(0xc30ab333U), binary32(0xc3504a3eU), binary32(0xc303b333U), 69.0F, 74.0F, 19.0F, 4.0F, 19, 18, 1}, false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#4", world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1}, 2.0F,
        binary32(0x43a00ccdU), 7, world::TreeFragment{27, 2, binary32(0x43bc147bU), -61.0F, binary32(0xc307d70aU), -54.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 0, 1}, false, false},
    FragmentUpdateCase{"dead-leaf-0#1", world::TreeFragment{67, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 5.0F, 25, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 2, binary32(0x4376c8f6U), binary32(0xc347c000U), binary32(0x4376c8f6U), binary32(0xc340c000U), 69.0F, 74.0F, binary32(0x40480000U), 5.0F, 25, 3, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-1#1",
        world::TreeFragment{67, 2, binary32(0x4376c8f6U), binary32(0xc347c000U), binary32(0x4376c8f6U), binary32(0xc340c000U), 69.0F, 74.0F, binary32(0x40480000U), 5.0F, 25, 3, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 2, binary32(0x437f68f6U), binary32(0xc33e8000U), binary32(0x437f68f6U), binary32(0xc3378000U), 69.0F, 74.0F, binary32(0x40c80000U), 5.0F, 25, 6, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-2#1",
        world::TreeFragment{67, 2, binary32(0x437f68f6U), binary32(0xc33e8000U), binary32(0x437f68f6U), binary32(0xc3378000U), 69.0F, 74.0F, binary32(0x40c80000U), 5.0F, 25, 6, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 2, binary32(0x4384047bU), binary32(0xc3354000U), binary32(0x4384047bU), binary32(0xc32e4000U), 69.0F, 74.0F, binary32(0x41160000U), 5.0F, 25, 9, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-3#1",
        world::TreeFragment{67, 2, binary32(0x4384047bU), binary32(0xc3354000U), binary32(0x4384047bU), binary32(0xc32e4000U), 69.0F, 74.0F, binary32(0x41160000U), 5.0F, 25, 9, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 2, binary32(0x4388547bU), -172.0F, binary32(0x4388547bU), -165.0F, 69.0F, 74.0F, binary32(0x41480000U), 5.0F, 25, 12, 1}, false, false},
    FragmentUpdateCase{"dead-leaf-4#1",
        world::TreeFragment{67, 2, binary32(0x4388547bU), -172.0F, binary32(0x4388547bU), -165.0F, 69.0F, 74.0F, binary32(0x41480000U), 5.0F, 25, 12, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 2, binary32(0x438ca47bU), binary32(0xc322c000U), binary32(0x438ca47bU), binary32(0xc31bc000U), 69.0F, 74.0F, binary32(0x417a0000U), 5.0F, 25, 15, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-5#1",
        world::TreeFragment{
            67, 2, binary32(0x438ca47bU), binary32(0xc322c000U), binary32(0x438ca47bU), binary32(0xc31bc000U), 69.0F, 74.0F, binary32(0x417a0000U), 5.0F, 25, 15, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 2, binary32(0x4390f47bU), binary32(0xc3198000U), binary32(0x4390f47bU), binary32(0xc3128000U), 69.0F, 74.0F, binary32(0x41960000U), 5.0F, 25, 18, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-6#1",
        world::TreeFragment{
            67, 2, binary32(0x4390f47bU), binary32(0xc3198000U), binary32(0x4390f47bU), binary32(0xc3128000U), 69.0F, 74.0F, binary32(0x41960000U), 5.0F, 25, 18, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 2, binary32(0x4395447bU), binary32(0xc3104000U), binary32(0x4395447bU), binary32(0xc3094000U), 69.0F, 74.0F, binary32(0x41af0000U), 5.0F, 25, 21, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-7#1",
        world::TreeFragment{
            67, 2, binary32(0x4395447bU), binary32(0xc3104000U), binary32(0x4395447bU), binary32(0xc3094000U), 69.0F, 74.0F, binary32(0x41af0000U), 5.0F, 25, 21, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{-1, 2, binary32(0x4399947bU), -135.0F, binary32(0x4399947bU), -128.0F, 69.0F, 74.0F, 25.0F, 5.0F, 25, 25, 1}, false, false},
    FragmentUpdateCase{"dead-leaf-8#1", world::TreeFragment{-1, 2, binary32(0x4399947bU), -135.0F, binary32(0x4399947bU), -128.0F, 69.0F, 74.0F, 25.0F, 5.0F, 25, 25, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{-1, 2, binary32(0x4399947bU), -135.0F, binary32(0x4399947bU), -128.0F, 69.0F, 74.0F, 25.0F, 5.0F, 25, 25, 1}, false, false},
    FragmentUpdateCase{"initial-leaf-frame-overwritten#2", world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1},
        0.0F, binary32(0x3dcccccdU), 8,
        world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, binary32(0x436e23d7U), binary32(0xc318999aU), 54.0F, 64.0F, 0.0F, 4.0F, 19, 0, 1}, false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#5", world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1},
        binary32(0x3e000000U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 3, binary32(0x4374e3d7U), -151.0F, binary32(0xc34b1c29U), binary32(0xc3116666U), 54.0F, 64.0F, binary32(0x40200000U), 4.0F, 19, 2, 1}, false,
        false},
    FragmentUpdateCase{"leaf-animation-cyclic#6", world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1},
        binary32(0x3f533333U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{
            27, 3, binary32(0x438d5852U), binary32(0xc2d46666U), binary32(0xc3254f5cU), binary32(0xc2c93333U), 54.0F, 64.0F, binary32(0x41840000U), 4.0F, 19, 16, 1},
        false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#7", world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1},
        binary32(0x3f733333U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 3, binary32(0x4390b852U), binary32(0xc2c46666U), binary32(0xc31e8f5cU), binary32(0xc2b93333U), 54.0F, 64.0F, 19.0F, 4.0F, 19, 18, 1}, false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#8", world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1}, 2.0F,
        binary32(0x43a00ccdU), 7, world::TreeFragment{27, 3, binary32(0x43ad11ecU), -31.0F, binary32(0xc2cbb850U), binary32(0xc1cb3333U), 54.0F, 64.0F, 0.0F, 4.0F, 19, 0, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-0#2", world::TreeFragment{67, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 5.0F, 25, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x4374e3d7U), -151.0F, binary32(0x4374e3d7U), binary32(0xc3116666U), 54.0F, 64.0F, binary32(0x40480000U), 5.0F, 25, 3, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-1#2",
        world::TreeFragment{67, 3, binary32(0x4374e3d7U), -151.0F, binary32(0x4374e3d7U), binary32(0xc3116666U), 54.0F, 64.0F, binary32(0x40480000U), 5.0F, 25, 3, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x437ba3d7U), -143.0F, binary32(0x437ba3d7U), binary32(0xc3096666U), 54.0F, 64.0F, binary32(0x40c80000U), 5.0F, 25, 6, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-2#2",
        world::TreeFragment{67, 3, binary32(0x437ba3d7U), -143.0F, binary32(0x437ba3d7U), binary32(0xc3096666U), 54.0F, 64.0F, binary32(0x40c80000U), 5.0F, 25, 6, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x438131ecU), -135.0F, binary32(0x438131ecU), binary32(0xc3016666U), 54.0F, 64.0F, binary32(0x41160000U), 5.0F, 25, 9, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-3#2",
        world::TreeFragment{67, 3, binary32(0x438131ecU), -135.0F, binary32(0x438131ecU), binary32(0xc3016666U), 54.0F, 64.0F, binary32(0x41160000U), 5.0F, 25, 9, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x438491ecU), -127.0F, binary32(0x438491ecU), binary32(0xc2f2cccdU), 54.0F, 64.0F, binary32(0x41480000U), 5.0F, 25, 12, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-4#2",
        world::TreeFragment{67, 3, binary32(0x438491ecU), -127.0F, binary32(0x438491ecU), binary32(0xc2f2cccdU), 54.0F, 64.0F, binary32(0x41480000U), 5.0F, 25, 12, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x4387f1ecU), -119.0F, binary32(0x4387f1ecU), binary32(0xc2e2cccdU), 54.0F, 64.0F, binary32(0x417a0000U), 5.0F, 25, 15, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-5#2",
        world::TreeFragment{67, 3, binary32(0x4387f1ecU), -119.0F, binary32(0x4387f1ecU), binary32(0xc2e2cccdU), 54.0F, 64.0F, binary32(0x417a0000U), 5.0F, 25, 15, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x438b51ecU), -111.0F, binary32(0x438b51ecU), binary32(0xc2d2cccdU), 54.0F, 64.0F, binary32(0x41960000U), 5.0F, 25, 18, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-6#2",
        world::TreeFragment{67, 3, binary32(0x438b51ecU), -111.0F, binary32(0x438b51ecU), binary32(0xc2d2cccdU), 54.0F, 64.0F, binary32(0x41960000U), 5.0F, 25, 18, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x438eb1ecU), -103.0F, binary32(0x438eb1ecU), binary32(0xc2c2cccdU), 54.0F, 64.0F, binary32(0x41af0000U), 5.0F, 25, 21, 1}, false,
        false},
    FragmentUpdateCase{"dead-leaf-7#2",
        world::TreeFragment{67, 3, binary32(0x438eb1ecU), -103.0F, binary32(0x438eb1ecU), binary32(0xc2c2cccdU), 54.0F, 64.0F, binary32(0x41af0000U), 5.0F, 25, 21, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{-1, 3, binary32(0x439211ecU), -95.0F, binary32(0x439211ecU), binary32(0xc2b2cccdU), 54.0F, 64.0F, 25.0F, 5.0F, 25, 25, 1}, false, false},
    FragmentUpdateCase{"dead-leaf-8#2",
        world::TreeFragment{-1, 3, binary32(0x439211ecU), -95.0F, binary32(0x439211ecU), binary32(0xc2b2cccdU), 54.0F, 64.0F, 25.0F, 5.0F, 25, 25, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 7, world::TreeFragment{-1, 3, binary32(0x439211ecU), -95.0F, binary32(0x439211ecU), binary32(0xc2b2cccdU), 54.0F, 64.0F, 25.0F, 5.0F, 25, 25, 1},
        false, false},
    FragmentUpdateCase{"initial-leaf-frame-overwritten#3", world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1},
        0.0F, binary32(0x3dcccccdU), 8,
        world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, binary32(0x436e23d7U), binary32(0xc2f13333U), 22.0F, 37.0F, 0.0F, 4.0F, 19, 0, 1}, false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#9", world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1},
        binary32(0x3e000000U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 3, binary32(0x4370e3d7U), binary32(0xc2f4c000U), binary32(0xc34f1c29U), binary32(0xc2e98ccdU), 22.0F, 37.0F, binary32(0x40200000U), 4.0F, 19, 2, 1},
        false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#10", world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1},
        binary32(0x3f533333U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{
            27, 3, binary32(0x4380251fU), binary32(0xc2c0f333U), binary32(0xc33fb5c2U), binary32(0xc2b5c000U), 22.0F, 37.0F, binary32(0x41840000U), 4.0F, 19, 16, 1},
        false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#11", world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1},
        binary32(0x3f733333U), binary32(0x43a00ccdU), 7,
        world::TreeFragment{27, 3, binary32(0x4381851fU), binary32(0xc2b7b333U), binary32(0xc33cf5c2U), binary32(0xc2ac8000U), 22.0F, 37.0F, 19.0F, 4.0F, 19, 18, 1}, false, false},
    FragmentUpdateCase{"leaf-animation-cyclic#12", world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1}, 2.0F,
        binary32(0x43a00ccdU), 7, world::TreeFragment{27, 3, binary32(0x438d11ecU), -53.0F, binary32(0xc325dc28U), binary32(0xc23d999aU), 22.0F, 37.0F, 0.0F, 4.0F, 19, 0, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-0#3", world::TreeFragment{67, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 5.0F, 25, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x4370e3d7U), binary32(0xc2f4c000U), binary32(0x4370e3d7U), binary32(0xc2e98ccdU), 22.0F, 37.0F, binary32(0x40480000U), 5.0F, 25, 3, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-1#3",
        world::TreeFragment{67, 3, binary32(0x4370e3d7U), binary32(0xc2f4c000U), binary32(0x4370e3d7U), binary32(0xc2e98ccdU), 22.0F, 37.0F, binary32(0x40480000U), 5.0F, 25, 3, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x4373a3d7U), binary32(0xc2eb8000U), binary32(0x4373a3d7U), binary32(0xc2e04ccdU), 22.0F, 37.0F, binary32(0x40c80000U), 5.0F, 25, 6, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-2#3",
        world::TreeFragment{67, 3, binary32(0x4373a3d7U), binary32(0xc2eb8000U), binary32(0x4373a3d7U), binary32(0xc2e04ccdU), 22.0F, 37.0F, binary32(0x40c80000U), 5.0F, 25, 6, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{67, 3, binary32(0x437663d7U), binary32(0xc2e24000U), binary32(0x437663d7U), binary32(0xc2d70ccdU), 22.0F, 37.0F, binary32(0x41160000U), 5.0F, 25, 9, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-3#3",
        world::TreeFragment{67, 3, binary32(0x437663d7U), binary32(0xc2e24000U), binary32(0x437663d7U), binary32(0xc2d70ccdU), 22.0F, 37.0F, binary32(0x41160000U), 5.0F, 25, 9, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 3, binary32(0x437923d7U), binary32(0xc2d90000U), binary32(0x437923d7U), binary32(0xc2cdcccdU), 22.0F, 37.0F, binary32(0x41480000U), 5.0F, 25, 12, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-4#3",
        world::TreeFragment{
            67, 3, binary32(0x437923d7U), binary32(0xc2d90000U), binary32(0x437923d7U), binary32(0xc2cdcccdU), 22.0F, 37.0F, binary32(0x41480000U), 5.0F, 25, 12, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 3, binary32(0x437be3d7U), binary32(0xc2cfc000U), binary32(0x437be3d7U), binary32(0xc2c48ccdU), 22.0F, 37.0F, binary32(0x417a0000U), 5.0F, 25, 15, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-5#3",
        world::TreeFragment{
            67, 3, binary32(0x437be3d7U), binary32(0xc2cfc000U), binary32(0x437be3d7U), binary32(0xc2c48ccdU), 22.0F, 37.0F, binary32(0x417a0000U), 5.0F, 25, 15, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 3, binary32(0x437ea3d7U), binary32(0xc2c68000U), binary32(0x437ea3d7U), binary32(0xc2bb4ccdU), 22.0F, 37.0F, binary32(0x41960000U), 5.0F, 25, 18, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-6#3",
        world::TreeFragment{
            67, 3, binary32(0x437ea3d7U), binary32(0xc2c68000U), binary32(0x437ea3d7U), binary32(0xc2bb4ccdU), 22.0F, 37.0F, binary32(0x41960000U), 5.0F, 25, 18, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{
            67, 3, binary32(0x4380b1ecU), binary32(0xc2bd4000U), binary32(0x4380b1ecU), binary32(0xc2b20ccdU), 22.0F, 37.0F, binary32(0x41af0000U), 5.0F, 25, 21, 1},
        false, false},
    FragmentUpdateCase{"dead-leaf-7#3",
        world::TreeFragment{
            67, 3, binary32(0x4380b1ecU), binary32(0xc2bd4000U), binary32(0x4380b1ecU), binary32(0xc2b20ccdU), 22.0F, 37.0F, binary32(0x41af0000U), 5.0F, 25, 21, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 7,
        world::TreeFragment{-1, 3, binary32(0x438211ecU), -90.0F, binary32(0x438211ecU), binary32(0xc2a8cccdU), 22.0F, 37.0F, 25.0F, 5.0F, 25, 25, 1}, false, false},
    FragmentUpdateCase{"dead-leaf-8#3",
        world::TreeFragment{-1, 3, binary32(0x438211ecU), -90.0F, binary32(0x438211ecU), binary32(0xc2a8cccdU), 22.0F, 37.0F, 25.0F, 5.0F, 25, 25, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 7, world::TreeFragment{-1, 3, binary32(0x438211ecU), -90.0F, binary32(0x438211ecU), binary32(0xc2a8cccdU), 22.0F, 37.0F, 25.0F, 5.0F, 25, 25, 1},
        false, false},
    FragmentUpdateCase{"branch-position-per-update#1",
        world::TreeFragment{28, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1}, 0.0F, binary32(0x43a00ccdU), 7,
        world::TreeFragment{28, 2, binary32(0x436e28f6U), binary32(0xc34f199aU), binary32(0xc388eb85U), binary32(0xc348199aU), 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"branch-position-per-update#2",
        world::TreeFragment{28, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1}, binary32(0x3e000000U),
        binary32(0x43a00ccdU), 7,
        world::TreeFragment{
            28, 2, binary32(0x436e28a4U), binary32(0xc34ef99aU), binary32(0xc388ebaeU), binary32(0xc347f99aU), binary32(0xbaa3d70aU), binary32(0x4001999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"branch-position-per-update#3",
        world::TreeFragment{28, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1}, binary32(0x3dcccccdU),
        binary32(0x43a00ccdU), 7, world::TreeFragment{28, 2, binary32(0x436e28b4U), -207.0F, binary32(0xc388eba6U), -200.0F, binary32(0xba83126fU), 2.0F, 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#1",
        world::TreeFragment{28, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 2, binary32(0x436e28a4U), binary32(0xc34ef99aU), binary32(0x436e28a4U), binary32(0xc346f99aU), binary32(0xbaa3d70aU), binary32(0x4001999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#2",
        world::TreeFragment{
            28, 2, binary32(0x436e28a4U), binary32(0xc34ef99aU), binary32(0x436e28a4U), binary32(0xc346f99aU), binary32(0xbaa3d70aU), binary32(0x4001999aU), 0.0F, 3.0F, 1, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 2, binary32(0x436e2800U), binary32(0xc34cd334U), binary32(0x436e2800U), binary32(0xc344d334U), binary32(0xbb23d70aU), binary32(0x4009999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#3",
        world::TreeFragment{
            28, 2, binary32(0x436e2800U), binary32(0xc34cd334U), binary32(0x436e2800U), binary32(0xc344d334U), binary32(0xbb23d70aU), binary32(0x4009999aU), 0.0F, 3.0F, 1, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 2, binary32(0x436e270aU), binary32(0xc34a8cceU), binary32(0x436e270aU), binary32(0xc3428cceU), binary32(0xbb75c28fU), binary32(0x4011999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#4",
        world::TreeFragment{
            28, 2, binary32(0x436e270aU), binary32(0xc34a8cceU), binary32(0x436e270aU), binary32(0xc3428cceU), binary32(0xbb75c28fU), binary32(0x4011999aU), 0.0F, 3.0F, 1, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 2, binary32(0x436e25c2U), binary32(0xc3482668U), binary32(0x436e25c2U), binary32(0xc3402668U), binary32(0xbba3d70aU), binary32(0x4019999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#5",
        world::TreeFragment{
            28, 2, binary32(0x436e25c2U), binary32(0xc3482668U), binary32(0x436e25c2U), binary32(0xc3402668U), binary32(0xbba3d70aU), binary32(0x4019999aU), 0.0F, 3.0F, 1, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 2, binary32(0x436e2428U), binary32(0xc345a002U), binary32(0x436e2428U), binary32(0xc33da002U), binary32(0xbbccccccU), binary32(0x4021999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#6",
        world::TreeFragment{
            28, 2, binary32(0x436e2428U), binary32(0xc345a002U), binary32(0x436e2428U), binary32(0xc33da002U), binary32(0xbbccccccU), binary32(0x4021999aU), 0.0F, 3.0F, 1, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 2, binary32(0x436e223cU), binary32(0xc342f99cU), binary32(0x436e223cU), binary32(0xc33af99cU), binary32(0xbbf5c28eU), binary32(0x4029999aU), 0.0F, 3.0F, 1, 0, 1},
        false, false},
    FragmentUpdateCase{"branch-position-per-update#4",
        world::TreeFragment{28, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2}, 0.0F, binary32(0x43a00ccdU), 7,
        world::TreeFragment{28, 3, binary32(0x436e8a3dU), -159.0F, binary32(0xc35175c3U), -152.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2}, false, false},
    FragmentUpdateCase{"branch-position-per-update#5",
        world::TreeFragment{28, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2}, binary32(0x3e000000U),
        binary32(0x43a00ccdU), 7,
        world::TreeFragment{
            28, 3, binary32(0x436e8a66U), binary32(0xc31ee333U), binary32(0xc351759aU), binary32(0xc317e333U), binary32(0x3ecd1eb9U), binary32(0x3de66666U), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"branch-position-per-update#6",
        world::TreeFragment{28, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2}, binary32(0x3dcccccdU),
        binary32(0x43a00ccdU), 7,
        world::TreeFragment{
            28, 3, binary32(0x436e8a5eU), binary32(0xc31ee8f6U), binary32(0xc35175a2U), binary32(0xc317e8f6U), binary32(0x3ecd0e56U), binary32(0x3db851ebU), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#7",
        world::TreeFragment{28, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 3, binary32(0x436e8a66U), binary32(0xc31ee333U), binary32(0x436e8a66U), binary32(0xc316e333U), binary32(0x3ecd1eb9U), binary32(0x3de66666U), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#8",
        world::TreeFragment{
            28, 3, binary32(0x436e8a66U), binary32(0xc31ee333U), binary32(0x436e8a66U), binary32(0xc316e333U), binary32(0x3ecd1eb9U), binary32(0x3de66666U), 0.0F, 3.0F, 1, 0, 2},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 3, binary32(0x436ef11eU), binary32(0xc31ea999U), binary32(0x436ef11eU), binary32(0xc316a999U), binary32(0x3ecd70a5U), binary32(0x3e666666U), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#9",
        world::TreeFragment{
            28, 3, binary32(0x436ef11eU), binary32(0xc31ea999U), binary32(0x436ef11eU), binary32(0xc316a999U), binary32(0x3ecd70a5U), binary32(0x3e666666U), 0.0F, 3.0F, 1, 0, 2},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 3, binary32(0x436f57ffU), binary32(0xc31e5333U), binary32(0x436f57ffU), binary32(0xc3165333U), binary32(0x3ecdc291U), binary32(0x3eacccccU), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#10",
        world::TreeFragment{
            28, 3, binary32(0x436f57ffU), binary32(0xc31e5333U), binary32(0x436f57ffU), binary32(0xc3165333U), binary32(0x3ecdc291U), binary32(0x3eacccccU), 0.0F, 3.0F, 1, 0, 2},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 3, binary32(0x436fbf09U), binary32(0xc31de000U), binary32(0x436fbf09U), binary32(0xc315e000U), binary32(0x3ece147dU), binary32(0x3ee66666U), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#11",
        world::TreeFragment{
            28, 3, binary32(0x436fbf09U), binary32(0xc31de000U), binary32(0x436fbf09U), binary32(0xc315e000U), binary32(0x3ece147dU), binary32(0x3ee66666U), 0.0F, 3.0F, 1, 0, 2},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 3, binary32(0x4370263cU), binary32(0xc31d5000U), binary32(0x4370263cU), binary32(0xc3155000U), binary32(0x3ece6669U), binary32(0x3f100000U), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"released-branch-sequence#12",
        world::TreeFragment{
            28, 3, binary32(0x4370263cU), binary32(0xc31d5000U), binary32(0x4370263cU), binary32(0xc3155000U), binary32(0x3ece6669U), binary32(0x3f100000U), 0.0F, 3.0F, 1, 0, 2},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{
            28, 3, binary32(0x43708d98U), binary32(0xc31ca333U), binary32(0x43708d98U), binary32(0xc314a333U), binary32(0x3eceb855U), binary32(0x3f2ccccdU), 0.0F, 3.0F, 1, 0, 2},
        false, false},
    FragmentUpdateCase{"highfly-sequence#1", world::TreeFragment{30, 2, 19.0F, -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc3d63800U), 19.0F, binary32(0xc3d23800U), 0.0F, binary32(0x40840000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#2",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc3d63800U), 19.0F, binary32(0xc3d23800U), 0.0F, binary32(0x40840000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc3bba800U), 19.0F, binary32(0xc3b7a800U), 0.0F, binary32(0x40880000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#3",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc3bba800U), 19.0F, binary32(0xc3b7a800U), 0.0F, binary32(0x40880000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc3a05000U), 19.0F, binary32(0xc39c5000U), 0.0F, binary32(0x408c0000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#4",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc3a05000U), 19.0F, binary32(0xc39c5000U), 0.0F, binary32(0x408c0000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc3843000U), 19.0F, binary32(0xc3803000U), 0.0F, binary32(0x40900000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#5",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc3843000U), 19.0F, binary32(0xc3803000U), 0.0F, binary32(0x40900000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc34e9000U), 19.0F, binary32(0xc3469000U), 0.0F, binary32(0x40940000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#6",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc34e9000U), 19.0F, binary32(0xc3469000U), 0.0F, binary32(0x40940000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc3133000U), 19.0F, binary32(0xc30b3000U), 0.0F, binary32(0x40980000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#7",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc3133000U), 19.0F, binary32(0xc30b3000U), 0.0F, binary32(0x40980000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc2ac8000U), 19.0F, binary32(0xc29c8000U), 0.0F, binary32(0x409c0000U), 0.0F, 0.0F, 0, 0, 0}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#8",
        world::TreeFragment{30, 2, 19.0F, binary32(0xc2ac8000U), 19.0F, binary32(0xc29c8000U), 0.0F, binary32(0x409c0000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0xc1be0000U), 19.0F, binary32(0xc17c0000U), 0.0F, 5.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"highfly-sequence#9", world::TreeFragment{30, 2, 19.0F, binary32(0xc1be0000U), 19.0F, binary32(0xc17c0000U), 0.0F, 5.0F, 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, 19.0F, binary32(0x42214000U), 19.0F, binary32(0x42414000U), 0.0F, binary32(0x40a40000U), 0.0F, 0.0F, 0, 0, 1}, false, false},
    FragmentUpdateCase{"highfly-sequence#10",
        world::TreeFragment{30, 2, 19.0F, binary32(0x42214000U), 19.0F, binary32(0x42414000U), 0.0F, binary32(0x40a40000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0x42d3e000U), 19.0F, binary32(0x42e3e000U), 0.0F, binary32(0x40a80000U), 0.0F, 0.0F, 0, 0, 1}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#11",
        world::TreeFragment{30, 2, 19.0F, binary32(0x42d3e000U), 19.0F, binary32(0x42e3e000U), 0.0F, binary32(0x40a80000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0x432d2000U), 19.0F, binary32(0x43352000U), 0.0F, binary32(0x40ac0000U), 0.0F, 0.0F, 0, 0, 1}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#12",
        world::TreeFragment{30, 2, 19.0F, binary32(0x432d2000U), 19.0F, binary32(0x43352000U), 0.0F, binary32(0x40ac0000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0x4371e000U), 19.0F, binary32(0x4379e000U), 0.0F, binary32(0x40b00000U), 0.0F, 0.0F, 0, 0, 1}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#13",
        world::TreeFragment{30, 2, 19.0F, binary32(0x4371e000U), 19.0F, binary32(0x4379e000U), 0.0F, binary32(0x40b00000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0x439c1800U), 19.0F, binary32(0x43a01800U), 0.0F, binary32(0x40b40000U), 0.0F, 0.0F, 0, 0, 1}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#14",
        world::TreeFragment{30, 2, 19.0F, binary32(0x439c1800U), 19.0F, binary32(0x43a01800U), 0.0F, binary32(0x40b40000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0x43c00800U), 19.0F, binary32(0x43c40800U), 0.0F, binary32(0x40b80000U), 0.0F, 0.0F, 0, 0, 1}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#15",
        world::TreeFragment{30, 2, 19.0F, binary32(0x43c00800U), 19.0F, binary32(0x43c40800U), 0.0F, binary32(0x40b80000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 19.0F, binary32(0x43e4c000U), 19.0F, binary32(0x43e8c000U), 0.0F, binary32(0x40bc0000U), 0.0F, 0.0F, 0, 0, 1}, false,
        false},
    FragmentUpdateCase{"highfly-sequence#16",
        world::TreeFragment{30, 2, 19.0F, binary32(0x43e4c000U), 19.0F, binary32(0x43e8c000U), 0.0F, binary32(0x40bc0000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{-1, 2, 19.0F, binary32(0x44052000U), 19.0F, binary32(0x44072000U), 0.0F, 6.0F, 0.0F, 0.0F, 0, 0, 1}, false, false},
    FragmentUpdateCase{"highfly-remove-and-sound#1", world::TreeFragment{30, 2, 19.0F, -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, 2.0F, 0.0F, 0,
        world::TreeFragment{-1, 2, 19.0F, 720.0F, 19.0F, 720.0F, 0.0F, 6.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"highfly-sequence#17", world::TreeFragment{30, 2, binary32(0xc2efae14U), -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3d63800U), binary32(0xc2efae14U), binary32(0xc3d23800U), 0.0F, binary32(0x40840000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#18",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3d63800U), binary32(0xc2efae14U), binary32(0xc3d23800U), 0.0F, binary32(0x40840000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3bba800U), binary32(0xc2efae14U), binary32(0xc3b7a800U), 0.0F, binary32(0x40880000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#19",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3bba800U), binary32(0xc2efae14U), binary32(0xc3b7a800U), 0.0F, binary32(0x40880000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3a05000U), binary32(0xc2efae14U), binary32(0xc39c5000U), 0.0F, binary32(0x408c0000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#20",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3a05000U), binary32(0xc2efae14U), binary32(0xc39c5000U), 0.0F, binary32(0x408c0000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3843000U), binary32(0xc2efae14U), binary32(0xc3803000U), 0.0F, binary32(0x40900000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#21",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3843000U), binary32(0xc2efae14U), binary32(0xc3803000U), 0.0F, binary32(0x40900000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc34e9000U), binary32(0xc2efae14U), binary32(0xc3469000U), 0.0F, binary32(0x40940000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#22",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc34e9000U), binary32(0xc2efae14U), binary32(0xc3469000U), 0.0F, binary32(0x40940000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3133000U), binary32(0xc2efae14U), binary32(0xc30b3000U), 0.0F, binary32(0x40980000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#23",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc3133000U), binary32(0xc2efae14U), binary32(0xc30b3000U), 0.0F, binary32(0x40980000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc2ac8000U), binary32(0xc2efae14U), binary32(0xc29c8000U), 0.0F, binary32(0x409c0000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#24",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc2ac8000U), binary32(0xc2efae14U), binary32(0xc29c8000U), 0.0F, binary32(0x409c0000U), 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc1be0000U), binary32(0xc2efae14U), binary32(0xc17c0000U), 0.0F, 5.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"highfly-sequence#25",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0xc1be0000U), binary32(0xc2efae14U), binary32(0xc17c0000U), 0.0F, 5.0F, 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x42214000U), binary32(0xc2efae14U), binary32(0x42414000U), 0.0F, binary32(0x40a40000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#26",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x42214000U), binary32(0xc2efae14U), binary32(0x42414000U), 0.0F, binary32(0x40a40000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x42d3e000U), binary32(0xc2efae14U), binary32(0x42e3e000U), 0.0F, binary32(0x40a80000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#27",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x42d3e000U), binary32(0xc2efae14U), binary32(0x42e3e000U), 0.0F, binary32(0x40a80000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x432d2000U), binary32(0xc2efae14U), binary32(0x43352000U), 0.0F, binary32(0x40ac0000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#28",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x432d2000U), binary32(0xc2efae14U), binary32(0x43352000U), 0.0F, binary32(0x40ac0000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x4371e000U), binary32(0xc2efae14U), binary32(0x4379e000U), 0.0F, binary32(0x40b00000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#29",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x4371e000U), binary32(0xc2efae14U), binary32(0x4379e000U), 0.0F, binary32(0x40b00000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x439c1800U), binary32(0xc2efae14U), binary32(0x43a01800U), 0.0F, binary32(0x40b40000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#30",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x439c1800U), binary32(0xc2efae14U), binary32(0x43a01800U), 0.0F, binary32(0x40b40000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x43c00800U), binary32(0xc2efae14U), binary32(0x43c40800U), 0.0F, binary32(0x40b80000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#31",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x43c00800U), binary32(0xc2efae14U), binary32(0x43c40800U), 0.0F, binary32(0x40b80000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x43e4c000U), binary32(0xc2efae14U), binary32(0x43e8c000U), 0.0F, binary32(0x40bc0000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#32",
        world::TreeFragment{30, 2, binary32(0xc2efae14U), binary32(0x43e4c000U), binary32(0xc2efae14U), binary32(0x43e8c000U), 0.0F, binary32(0x40bc0000U), 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{-1, 2, binary32(0xc2efae14U), binary32(0x44052000U), binary32(0xc2efae14U), binary32(0x44072000U), 0.0F, 6.0F, 0.0F, 0.0F, 0, 0, 1}, false, false},
    FragmentUpdateCase{"highfly-remove-and-sound#2", world::TreeFragment{30, 2, binary32(0xc2efae14U), -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, 2.0F, 0.0F, 0,
        world::TreeFragment{-1, 2, binary32(0xc2efae14U), 720.0F, binary32(0xc2efae14U), 720.0F, 0.0F, 6.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"highfly-sequence#33", world::TreeFragment{30, 2, 2567.0F, -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc3d63800U), 2567.0F, binary32(0xc3d23800U), 0.0F, binary32(0x40840000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#34",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc3d63800U), 2567.0F, binary32(0xc3d23800U), 0.0F, binary32(0x40840000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc3bba800U), 2567.0F, binary32(0xc3b7a800U), 0.0F, binary32(0x40880000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#35",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc3bba800U), 2567.0F, binary32(0xc3b7a800U), 0.0F, binary32(0x40880000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc3a05000U), 2567.0F, binary32(0xc39c5000U), 0.0F, binary32(0x408c0000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#36",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc3a05000U), 2567.0F, binary32(0xc39c5000U), 0.0F, binary32(0x408c0000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc3843000U), 2567.0F, binary32(0xc3803000U), 0.0F, binary32(0x40900000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#37",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc3843000U), 2567.0F, binary32(0xc3803000U), 0.0F, binary32(0x40900000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc34e9000U), 2567.0F, binary32(0xc3469000U), 0.0F, binary32(0x40940000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#38",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc34e9000U), 2567.0F, binary32(0xc3469000U), 0.0F, binary32(0x40940000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc3133000U), 2567.0F, binary32(0xc30b3000U), 0.0F, binary32(0x40980000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#39",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc3133000U), 2567.0F, binary32(0xc30b3000U), 0.0F, binary32(0x40980000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc2ac8000U), 2567.0F, binary32(0xc29c8000U), 0.0F, binary32(0x409c0000U), 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-sequence#40",
        world::TreeFragment{30, 2, 2567.0F, binary32(0xc2ac8000U), 2567.0F, binary32(0xc29c8000U), 0.0F, binary32(0x409c0000U), 0.0F, 0.0F, 0, 0, 0}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0xc1be0000U), 2567.0F, binary32(0xc17c0000U), 0.0F, 5.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"highfly-sequence#41", world::TreeFragment{30, 2, 2567.0F, binary32(0xc1be0000U), 2567.0F, binary32(0xc17c0000U), 0.0F, 5.0F, 0.0F, 0.0F, 0, 0, 1},
        binary32(0x3e000000U), binary32(0x3dcccccdU), 8,
        world::TreeFragment{30, 2, 2567.0F, binary32(0x42214000U), 2567.0F, binary32(0x42414000U), 0.0F, binary32(0x40a40000U), 0.0F, 0.0F, 0, 0, 1}, false, false},
    FragmentUpdateCase{"highfly-sequence#42",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x42214000U), 2567.0F, binary32(0x42414000U), 0.0F, binary32(0x40a40000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0x42d3e000U), 2567.0F, binary32(0x42e3e000U), 0.0F, binary32(0x40a80000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#43",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x42d3e000U), 2567.0F, binary32(0x42e3e000U), 0.0F, binary32(0x40a80000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0x432d2000U), 2567.0F, binary32(0x43352000U), 0.0F, binary32(0x40ac0000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#44",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x432d2000U), 2567.0F, binary32(0x43352000U), 0.0F, binary32(0x40ac0000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0x4371e000U), 2567.0F, binary32(0x4379e000U), 0.0F, binary32(0x40b00000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#45",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x4371e000U), 2567.0F, binary32(0x4379e000U), 0.0F, binary32(0x40b00000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0x439c1800U), 2567.0F, binary32(0x43a01800U), 0.0F, binary32(0x40b40000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#46",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x439c1800U), 2567.0F, binary32(0x43a01800U), 0.0F, binary32(0x40b40000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0x43c00800U), 2567.0F, binary32(0x43c40800U), 0.0F, binary32(0x40b80000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#47",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x43c00800U), 2567.0F, binary32(0x43c40800U), 0.0F, binary32(0x40b80000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{30, 2, 2567.0F, binary32(0x43e4c000U), 2567.0F, binary32(0x43e8c000U), 0.0F, binary32(0x40bc0000U), 0.0F, 0.0F, 0, 0, 1},
        false, false},
    FragmentUpdateCase{"highfly-sequence#48",
        world::TreeFragment{30, 2, 2567.0F, binary32(0x43e4c000U), 2567.0F, binary32(0x43e8c000U), 0.0F, binary32(0x40bc0000U), 0.0F, 0.0F, 0, 0, 1}, binary32(0x3e000000U),
        binary32(0x3dcccccdU), 8, world::TreeFragment{-1, 2, 2567.0F, binary32(0x44052000U), 2567.0F, binary32(0x44072000U), 0.0F, 6.0F, 0.0F, 0.0F, 0, 0, 1}, false, false},
    FragmentUpdateCase{"highfly-remove-and-sound#3", world::TreeFragment{30, 2, 2567.0F, -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, 2.0F, 0.0F, 0,
        world::TreeFragment{-1, 2, 2567.0F, 720.0F, 2567.0F, 720.0F, 0.0F, 6.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"fragment-ground-boundary#1", world::TreeFragment{27, 2, binary32(0x436e28f6U), 480.0F, -1024.0F, -1024.0F, 69.0F, 0.0F, 0.0F, 4.0F, 19, 18, 1}, 0.0F, 0.0F,
        0, world::TreeFragment{27, 2, binary32(0x436e28f6U), 480.0F, binary32(0x436e28f6U), 480.0F, 69.0F, 0.0F, 0.0F, 4.0F, 19, 0, 1}, false, false},
    FragmentUpdateCase{"fragment-ground-boundary#2",
        world::TreeFragment{27, 2, binary32(0x436e28f6U), binary32(0x43f00001U), -1024.0F, -1024.0F, 69.0F, 0.0F, 0.0F, 4.0F, 19, 18, 1}, 0.0F, 0.0F, 0,
        world::TreeFragment{-1, 2, binary32(0x436e28f6U), binary32(0x43f00001U), binary32(0x436e28f6U), binary32(0x43f00001U), 69.0F, 0.0F, 0.0F, 4.0F, 19, 0, 1}, false, false},
    FragmentUpdateCase{"fragment-ground-boundary#3", world::TreeFragment{67, 2, binary32(0x436e28f6U), 480.0F, -1024.0F, -1024.0F, 69.0F, 0.0F, 0.0F, 5.0F, 25, 0, 1}, 0.0F, 0.0F,
        0, world::TreeFragment{67, 2, binary32(0x436e28f6U), 480.0F, binary32(0x436e28f6U), 480.0F, 69.0F, 0.0F, 0.0F, 5.0F, 25, 0, 1}, false, false},
    FragmentUpdateCase{"fragment-ground-boundary#4",
        world::TreeFragment{67, 2, binary32(0x436e28f6U), binary32(0x43f00001U), -1024.0F, -1024.0F, 69.0F, 0.0F, 0.0F, 5.0F, 25, 0, 1}, 0.0F, 0.0F, 0,
        world::TreeFragment{-1, 2, binary32(0x436e28f6U), binary32(0x43f00001U), binary32(0x436e28f6U), binary32(0x43f00001U), 69.0F, 0.0F, 0.0F, 5.0F, 25, 0, 1}, false, false},
    FragmentUpdateCase{"fragment-ground-boundary#5", world::TreeFragment{28, 2, binary32(0x436e28f6U), 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 1}, 0.0F, 0.0F, 0,
        world::TreeFragment{28, 2, binary32(0x436e28f6U), 480.0F, binary32(0x436e28f6U), 480.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 1}, false, false},
    FragmentUpdateCase{"fragment-ground-boundary#6", world::TreeFragment{28, 2, binary32(0x436e28f6U), binary32(0x43f00001U), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 1},
        0.0F, 0.0F, 0, world::TreeFragment{-1, 2, binary32(0x436e28f6U), binary32(0x43f00001U), binary32(0x436e28f6U), binary32(0x43f00001U), 0.0F, 0.0F, 0.0F, 3.0F, 1, 0, 1},
        true, false},
    FragmentUpdateCase{"fragment-ground-boundary#7", world::TreeFragment{30, 2, binary32(0x419947aeU), 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0}, 0.0F, 0.0F, 0,
        world::TreeFragment{30, 2, binary32(0x419947aeU), 480.0F, binary32(0x419947aeU), 480.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 1}, false, true},
    FragmentUpdateCase{"fragment-ground-boundary#8", world::TreeFragment{30, 2, binary32(0x419947aeU), binary32(0x43f00001U), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0},
        0.0F, 0.0F, 0, world::TreeFragment{-1, 2, binary32(0x419947aeU), binary32(0x43f00001U), binary32(0x419947aeU), binary32(0x43f00001U), 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 1},
        false, true},
    FragmentUpdateCase{"highfly-sound-boundary#1", world::TreeFragment{30, 2, binary32(0x419947aeU), -50.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0}, 0.0F, 0.0F, 0,
        world::TreeFragment{30, 2, binary32(0x419947aeU), -50.0F, binary32(0x419947aeU), -50.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0}, false, false},
    FragmentUpdateCase{"highfly-sound-boundary#2", world::TreeFragment{30, 2, binary32(0x419947aeU), binary32(0xc247ffffU), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0},
        0.0F, 0.0F, 0, world::TreeFragment{30, 2, binary32(0x419947aeU), binary32(0xc247ffffU), binary32(0x419947aeU), binary32(0xc247ffffU), 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 1},
        false, true},
    FragmentUpdateCase{"highfly-sound-boundary#3", world::TreeFragment{30, 2, binary32(0x419947aeU), binary32(0xc2480001U), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0},
        0.0F, 0.0F, 0, world::TreeFragment{30, 2, binary32(0x419947aeU), binary32(0xc2480001U), binary32(0x419947aeU), binary32(0xc2480001U), 0.0F, 0.0F, 0.0F, 0.0F, 0, 0, 0},
        false, false},
    FragmentUpdateCase{"highfly-retained-velocity", world::TreeFragment{30, 2, binary32(0x419947aeU), -100.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0},
        binary32(0x3eaaaaabU), binary32(0x3dcccccdU), 7,
        world::TreeFragment{30, 2, binary32(0x419947aeU), binary32(0x4231c71eU), binary32(0x419947aeU), binary32(0x424dc71eU), 0.0F, binary32(0x408aaaabU), 0.0F, 0.0F, 0, 0, 1},
        false, true}}};

const std::array<ImpactCase, 40> impacts_cases = {{ImpactCase{"frame0-count0", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}, std::nullopt, 0,
                                                       false, world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}},
    ImpactCase{"frame0-count3", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}, std::nullopt, 3, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}},
    ImpactCase{"frame0-count4", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 9}},
    ImpactCase{"frame0-count5", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}, std::nullopt, 5, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}},
    ImpactCase{"frame2-count0", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}, std::nullopt, 0, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}},
    ImpactCase{"frame2-count3", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}, std::nullopt, 3, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}},
    ImpactCase{"frame2-count4", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 9}},
    ImpactCase{"frame2-count5", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}, std::nullopt, 5, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}},
    ImpactCase{"frame1-count0", world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, std::nullopt, 0, false,
        world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}},
    ImpactCase{"frame1-count3", world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, std::nullopt, 3, false,
        world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}},
    ImpactCase{"frame1-count4", world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, std::nullopt, 4, false,
        world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, -1}},
    ImpactCase{"frame1-count5", world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, std::nullopt, 5, false,
        world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}},
    ImpactCase{"frame3-count0", world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, std::nullopt, 0, false,
        world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}},
    ImpactCase{"frame3-count3", world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, std::nullopt, 3, false,
        world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}},
    ImpactCase{"frame3-count4", world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, std::nullopt, 4, false,
        world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, -1}},
    ImpactCase{"frame3-count5", world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, std::nullopt, 5, false,
        world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}},
    ImpactCase{"crown1-qualifying2", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 9}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 8}},
    ImpactCase{"crown1-qualifying3", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 8}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 7}},
    ImpactCase{"crown1-qualifying4", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 7}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 6}},
    ImpactCase{"crown1-qualifying5", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 6}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}},
    ImpactCase{"crown1-qualifying6", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 4}},
    ImpactCase{"crown1-qualifying7", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 4}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 3}},
    ImpactCase{"crown1-qualifying8", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 3}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 2}},
    ImpactCase{"crown1-qualifying9", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 2}, std::nullopt, 4, false,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 1}},
    ImpactCase{"crown1-qualifying10", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 1}, std::nullopt, 4, true,
        world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}},
    ImpactCase{"crown1-startfall-[0, 0]", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, {{0U, 0U}}, 0, std::nullopt,
        world::Tree{66, 147.0F, -13.0F, 0.0F, 0.0F, binary32(0xbf000000U), 0.0F, 0.0F, 0.0F, 1, 0, 0}},
    ImpactCase{"crown1-startfall-[5, 19]", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, {{5U, 19U}}, 0, std::nullopt,
        world::Tree{66, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 0.0F, 1, 0, 0}},
    ImpactCase{"crown1-startfall-[32767, 32767]", world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, {{32767U, 32767U}}, 0, std::nullopt,
        world::Tree{66, 147.0F, -13.0F, 0.0F, 0.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 0.0F, 1, 0, 0}},
    ImpactCase{"crown3-qualifying2", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 9}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 8}},
    ImpactCase{"crown3-qualifying3", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 8}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 7}},
    ImpactCase{"crown3-qualifying4", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 7}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 6}},
    ImpactCase{"crown3-qualifying5", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 6}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 5}},
    ImpactCase{"crown3-qualifying6", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 5}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 4}},
    ImpactCase{"crown3-qualifying7", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 4}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 3}},
    ImpactCase{"crown3-qualifying8", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 3}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 2}},
    ImpactCase{"crown3-qualifying9", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 2}, std::nullopt, 4, false,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 1}},
    ImpactCase{"crown3-qualifying10", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 1}, std::nullopt, 4, true,
        world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 0}},
    ImpactCase{"crown3-startfall-[0, 0]", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 0}, {{0U, 0U}}, 0, std::nullopt,
        world::Tree{66, 2067.0F, -13.0F, 0.0F, 0.0F, binary32(0xbf000000U), 0.0F, 0.0F, 0.0F, 1, 2, 0}},
    ImpactCase{"crown3-startfall-[5, 19]", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 0}, {{5U, 19U}}, 0, std::nullopt,
        world::Tree{66, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 0.0F, 1, 2, 0}},
    ImpactCase{"crown3-startfall-[32767, 32767]", world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 0}, {{32767U, 32767U}}, 0, std::nullopt,
        world::Tree{66, 2067.0F, -13.0F, 0.0F, 0.0F, binary32(0x3e4ccccdU), binary32(0x3f333333U), 0.0F, 0.0F, 1, 2, 0}}}};

const std::array<LeafFactoryCase, 3> leaf_factories_cases = {
    {LeafFactoryCase{
         0.1F, 123, {{0U, 319U, 149U, 18U, 59U, 39U}}, world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1}},
        LeafFactoryCase{
            0.1F, 123, {{1U, 255U, 99U, 18U, 49U, 34U}}, world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1}},
        LeafFactoryCase{0.1F, 123, {{32767U, 32767U, 32767U, 32767U, 32767U, 32767U}},
            world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1}}}};

const std::array<BranchFactoryCase, 2> branch_factories_cases = {
    {BranchFactoryCase{
         0.1F, 123, {{0U, 319U, 149U, 5U, 19U}}, world::TreeFragment{28, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1}},
        BranchFactoryCase{0.1F, 123, {{1U, 255U, 99U, 9U, 0U}},
            world::TreeFragment{28, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2}}}};

const std::array<HighflyFactoryCase, 3> highfly_factories_cases = {
    {HighflyFactoryCase{0.0F, 123, world::TreeFragment{30, 2, 19.0F, -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}},
        HighflyFactoryCase{0.1F, -16, world::TreeFragment{30, 2, binary32(0xc2efae14U), -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}},
        HighflyFactoryCase{1280.0F, 623, world::TreeFragment{30, 2, 2567.0F, -480.0F, -1024.0F, -1024.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}}}};

const std::array<LeafHitCase, 3> leaf_hits_cases = {{LeafHitCase{
                                                         world::TreeFragment{27, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 4.0F, 19, 18, 1},
                                                         world::TreeFragment{67, 2, binary32(0x436e28f6U), -209.0F, -1024.0F, -1024.0F, 69.0F, 74.0F, 0.0F, 5.0F, 25, 0, 1},
                                                         world::TreeLeafImpact{1, 258.1600036621094F, -179.0F}},
    LeafHitCase{world::TreeFragment{27, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 4.0F, 19, 18, 1},
        world::TreeFragment{67, 3, binary32(0x436e23d7U), -159.0F, -1024.0F, -1024.0F, 54.0F, 64.0F, 0.0F, 5.0F, 25, 0, 1}, world::TreeLeafImpact{1, 258.1400146484375F, -129.0F}},
    LeafHitCase{world::TreeFragment{27, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 4.0F, 19, 11, 1},
        world::TreeFragment{67, 3, binary32(0x436e23d7U), -127.0F, -1024.0F, -1024.0F, 22.0F, 37.0F, 0.0F, 5.0F, 25, 0, 1}, world::TreeLeafImpact{1, 258.1400146484375F, -97.0F}}}};

const std::array<TreeDrawCase, 6> tree_draws_cases = {
    {TreeDrawCase{world::Tree{26, 153.0F, 0.0F, 153.0F, 3.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, DrawExpectation{"gfx/tree1_02", {{0, 0, 97, 352}}, {{153, 3}}}},
        TreeDrawCase{world::Tree{26, 147.0F, -13.0F, 147.0F, -10.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10}, DrawExpectation{"gfx/tree1_01", {{0, 0, 110, 128}}, {{147, -10}}}},
        TreeDrawCase{world::Tree{26, 2073.0F, 0.0F, 2073.0F, 3.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, DrawExpectation{"gfx/tree1_02", {{0, 0, 97, 352}}, {{2073, 3}}}},
        TreeDrawCase{world::Tree{26, 2067.0F, -13.0F, 2067.0F, -10.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}, DrawExpectation{"gfx/tree1_01", {{0, 0, 110, 128}}, {{2067, -10}}}},
        TreeDrawCase{
            world::Tree{66, 147.0F, -13.0F, 147.0F, -10.0F, binary32(0xbf000000U), 0.0F, 0.0F, 0.0F, 1, 0, 0}, DrawExpectation{"gfx/tree1_01", {{0, 0, 110, 128}}, {{147, -10}}}},
        TreeDrawCase{world::Tree{66, 2067.0F, -13.0F, 2067.0F, -10.0F, binary32(0x3ecccccdU), binary32(0x3ff33333U), 0.0F, 0.0F, 1, 2, 0},
            DrawExpectation{"gfx/tree1_01", {{0, 0, 110, 128}}, {{2067, -10}}}}}};

const std::array<FragmentDrawCase, 49> fragment_draws_cases = {{FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 0, 1},
                                                                    DrawExpectation{"gfx/Leaf", {{0, 0, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 1, 1}, DrawExpectation{"gfx/Leaf", {{0, 60, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 2, 1}, DrawExpectation{"gfx/Leaf", {{0, 120, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 3, 1}, DrawExpectation{"gfx/Leaf", {{0, 180, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 4, 1}, DrawExpectation{"gfx/Leaf", {{0, 240, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 5, 1}, DrawExpectation{"gfx/Leaf", {{0, 300, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 6, 1}, DrawExpectation{"gfx/Leaf", {{0, 360, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 7, 1}, DrawExpectation{"gfx/Leaf", {{0, 420, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 8, 1}, DrawExpectation{"gfx/Leaf", {{0, 480, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 9, 1}, DrawExpectation{"gfx/Leaf", {{0, 540, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 10, 1}, DrawExpectation{"gfx/Leaf", {{0, 600, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 11, 1}, DrawExpectation{"gfx/Leaf", {{0, 660, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 12, 1}, DrawExpectation{"gfx/Leaf", {{0, 720, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 13, 1}, DrawExpectation{"gfx/Leaf", {{0, 780, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 14, 1}, DrawExpectation{"gfx/Leaf", {{0, 840, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 15, 1}, DrawExpectation{"gfx/Leaf", {{0, 900, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 16, 1}, DrawExpectation{"gfx/Leaf", {{0, 960, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 17, 1}, DrawExpectation{"gfx/Leaf", {{0, 1020, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{27, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 4.0F, 19, 18, 1}, DrawExpectation{"gfx/Leaf", {{0, 1080, 60, 60}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 0, 1}, DrawExpectation{"gfx/Leaf2", {{0, 0, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 1, 1}, DrawExpectation{"gfx/Leaf2", {{0, 80, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 2, 1}, DrawExpectation{"gfx/Leaf2", {{0, 160, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 3, 1}, DrawExpectation{"gfx/Leaf2", {{0, 240, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 4, 1}, DrawExpectation{"gfx/Leaf2", {{0, 320, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 5, 1}, DrawExpectation{"gfx/Leaf2", {{0, 400, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 6, 1}, DrawExpectation{"gfx/Leaf2", {{0, 480, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 7, 1}, DrawExpectation{"gfx/Leaf2", {{0, 560, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 8, 1}, DrawExpectation{"gfx/Leaf2", {{0, 640, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 9, 1}, DrawExpectation{"gfx/Leaf2", {{0, 720, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 10, 1}, DrawExpectation{"gfx/Leaf2", {{0, 800, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 11, 1}, DrawExpectation{"gfx/Leaf2", {{0, 880, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 12, 1}, DrawExpectation{"gfx/Leaf2", {{0, 960, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 13, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1040, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 14, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1120, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 15, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1200, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 16, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1280, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 17, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1360, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 18, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1440, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 19, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1520, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 20, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1600, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 21, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1680, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 22, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1760, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 23, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1840, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{67, 2, -81.0F, -60.0F, -81.0F, -60.0F, 10.0F, 35.0F, 0.0F, 5.0F, 25, 24, 1}, DrawExpectation{"gfx/Leaf2", {{0, 1920, 80, 80}}, {{-81, -60}}}},
    FragmentDrawCase{
        world::TreeFragment{28, 2, binary32(0x436e28f6U), binary32(0xc34f199aU), binary32(0x436e28f6U), binary32(0xc348199aU), 0.0F, binary32(0x3ff33333U), 0.0F, 3.0F, 1, 0, 1},
        DrawExpectation{"gfx/branch1", {{0, 0, 78, 60}}, {{238, -200}}}},
    FragmentDrawCase{world::TreeFragment{28, 3, binary32(0x436e8a3dU), -159.0F, binary32(0x436e8a3dU), -152.0F, binary32(0x3ecccccdU), 0.0F, 0.0F, 3.0F, 1, 0, 2},
        DrawExpectation{"gfx/branch2", {{0, 0, 118, 58}}, {{238, -152}}}},
    FragmentDrawCase{world::TreeFragment{30, 2, 19.0F, -480.0F, 19.0F, -473.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, DrawExpectation{"gfx/highfly", {{0, 0, 206, 210}}, {{19, -473}}}},
    FragmentDrawCase{world::TreeFragment{30, 2, binary32(0xc2efae14U), -480.0F, binary32(0xc2efae14U), -473.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0},
        DrawExpectation{"gfx/highfly", {{0, 0, 206, 210}}, {{-119, -473}}}},
    FragmentDrawCase{
        world::TreeFragment{30, 2, 2567.0F, -480.0F, 2567.0F, -473.0F, 0.0F, 4.0F, 0.0F, 0.0F, 0, 0, 0}, DrawExpectation{"gfx/highfly", {{0, 0, 206, 210}}, {{2567, -473}}}}}};

const std::array<ResetCase, 3> resets_cases = {
    {ResetCase{{{world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                   world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}},
         {{world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0},
             world::Tree{26, 147.0F, -13.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
             world::Tree{26, 2073.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0},
             world::Tree{26, 2067.0F, -13.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}},
         {{world::Tree{26, 153.0F, 0.0F, -1895.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, -1901.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
             world::Tree{26, 2073.0F, 0.0F, 25.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 19.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}}},
        ResetCase{
            {{world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}},
            {{world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, 100.0F, 500.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                world::Tree{26, 2073.0F, 0.0F, 80.0F, 470.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0},
                world::Tree{26, 2067.0F, -13.0F, -30.0F, 510.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}},
            {{world::Tree{26, 153.0F, 0.0F, -1895.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, -1901.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                world::Tree{26, 2073.0F, 0.0F, 25.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 19.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}}},
        ResetCase{
            {{world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                world::Tree{26, 2073.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}},
            {{world::Tree{26, 153.0F, 0.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, 147.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                world::Tree{26, 2073.0F, 0.0F, 25.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 19.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}},
            {{world::Tree{26, 153.0F, 0.0F, -1895.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 0}, world::Tree{26, 147.0F, -13.0F, -1901.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 10},
                world::Tree{26, 2073.0F, 0.0F, 25.0F, 8.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 0}, world::Tree{26, 2067.0F, -13.0F, 19.0F, -5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 10}}}}}};

template <class State> void compare(const State& actual, const State& expected, std::string_view context) {
    require(actual == expected, std::string(context) + ": original state differs");
    require(std::bit_cast<std::uint32_t>(actual.world_x) == std::bit_cast<std::uint32_t>(expected.world_x)
                && std::bit_cast<std::uint32_t>(actual.world_y) == std::bit_cast<std::uint32_t>(expected.world_y)
                && std::bit_cast<std::uint32_t>(actual.screen_x) == std::bit_cast<std::uint32_t>(expected.screen_x)
                && std::bit_cast<std::uint32_t>(actual.screen_y) == std::bit_cast<std::uint32_t>(expected.screen_y)
                && std::bit_cast<std::uint32_t>(actual.velocity_x) == std::bit_cast<std::uint32_t>(expected.velocity_x)
                && std::bit_cast<std::uint32_t>(actual.velocity_y) == std::bit_cast<std::uint32_t>(expected.velocity_y)
                && std::bit_cast<std::uint32_t>(actual.animation_time) == std::bit_cast<std::uint32_t>(expected.animation_time)
                && std::bit_cast<std::uint32_t>(actual.animation_rate) == std::bit_cast<std::uint32_t>(expected.animation_rate),
        std::string(context) + ": original binary32 words differ");
}

void compare_draw(const contracts::SpriteDraw& actual, const DrawExpectation& expected) {
    const auto& source = expected.source;
    const auto& destination = expected.destination;
    require(actual.image == expected.image && actual.source.x == source.at(0) && actual.source.y == source.at(1) && actual.source.width == source.at(2)
                && actual.source.height == source.at(3) && actual.destination.x == destination.at(0) && actual.destination.y == destination.at(1) && actual.color_key
                && !actual.mirror_x,
        "Original tree/effect draw differs");
}

void original_vectors() {
    const auto initial = world::initial_trees();
    for (std::size_t index = 0; index < initial.size(); ++index) {
        compare(initial[index], initial_cases[index], "Fresh tree storage");
    }
    for (const auto& expected : tree_updates_cases) {
        auto tree = expected.before;
        const auto result = world::update_tree(tree, expected.delta, expected.camera, expected.recoil);
        compare(tree, expected.after, expected.id);
        require(result.ground_sound == expected.ground_sound, expected.id + ": tree ground effect differs");
    }
    for (const auto& expected : fragment_updates_cases) {
        auto fragment = expected.before;
        const auto result = world::update_tree_fragment(fragment, expected.delta, expected.camera, expected.recoil);
        compare(fragment, expected.after, expected.id);
        require(result.ground_sound == expected.ground_sound && result.highfly_sound == expected.highfly_sound, expected.id + ": fragment ground/highfly effect differs");
    }
    for (const auto& expected : impacts_cases) {
        auto tree = expected.before;
        if (expected.random) {
            world::start_tree_fall(tree, (*expected.random)[0], (*expected.random)[1]);
        } else {
            const bool destroyed = world::damage_tree(tree, expected.branch_hits);
            require(destroyed == expected.destroyed, expected.id + ": original tree destruction predicate differs");
        }
        compare(tree, expected.after, expected.id);
    }
    for (const auto& expected : leaf_factories_cases) {
        compare(world::make_tree_leaf(expected.camera, expected.pointer_x, expected.random), expected.state, "Tree leaf initializer");
    }
    for (const auto& expected : branch_factories_cases) {
        compare(world::make_tree_branch(expected.camera, expected.pointer_x, expected.random), expected.state, "Tree branch initializer");
    }
    for (const auto& expected : highfly_factories_cases) {
        compare(world::make_tree_highfly(expected.camera, expected.pointer_x), expected.state, "Tree highfly initializer");
    }
    for (const auto& expected : leaf_hits_cases) {
        auto leaf = expected.before;
        const auto impact = world::hit_tree_leaf(leaf);
        compare(leaf, expected.after, "Leaf impact");
        require(
            impact.points == expected.impact.points && impact.popup_x == expected.impact.popup_x && impact.popup_y == expected.impact.popup_y, "Original leaf score/popup differs");
    }
    for (const auto& expected : tree_draws_cases) {
        compare_draw(world::tree_draw(expected.state), expected.draw);
    }
    for (const auto& expected : fragment_draws_cases) {
        compare_draw(world::tree_fragment_draw(expected.state), expected.draw);
    }
    for (const auto& expected : resets_cases) {
        auto trees = world::initial_trees();
        for (std::size_t index = 0; index < trees.size(); ++index) {
            compare(trees[index], expected.native_initial[index], "Reset fresh native storage");
            auto raw = expected.raw_initial[index];
            static_cast<void>(world::update_tree(trees[index], 0.016F, 1280, 8));
            static_cast<void>(world::update_tree(raw, 0.016F, 1280, 8));
            compare(trees[index], expected.first_update[index], "Reset normal pose");
            require(raw == trees[index], "Raw retained tree pose must normalize before presentation");
        }
    }
}

using moorhuhn2::test::reject_unchanged;

void invalid_inputs() {
    const auto crown = world::initial_trees()[1];
    const auto leaf = world::make_tree_leaf(0, 0, {0, 0, 0, 0, 0, 0});
    const auto update_tree = [](auto& tree) {
        static_cast<void>(world::update_tree(tree, 0.125F, 0, 0));
    };
    const auto update_fragment = [](auto& fragment) {
        static_cast<void>(world::update_tree_fragment(fragment, 0.125F, 0, 0));
    };
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        reject_unchanged(
            crown,
            [delta](auto& tree) {
                static_cast<void>(world::update_tree(tree, delta, 0, 0));
            },
            "Invalid tree time");
        reject_unchanged(
            leaf,
            [delta](auto& fragment) {
                static_cast<void>(world::update_tree_fragment(fragment, delta, 0, 0));
            },
            "Invalid fragment time");
    }
    for (const float camera : {-1.0F, 1280.25F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        reject_unchanged(
            crown,
            [camera](auto& tree) {
                static_cast<void>(world::update_tree(tree, 0.125F, camera, 0));
            },
            "Invalid tree camera");
        reject_unchanged(
            leaf,
            [camera](auto& fragment) {
                static_cast<void>(world::update_tree_fragment(fragment, 0.125F, camera, 0));
            },
            "Invalid fragment camera");
    }
    for (const int recoil : {-1, 9}) {
        reject_unchanged(
            crown,
            [recoil](auto& tree) {
                static_cast<void>(world::update_tree(tree, 0.125F, 0, recoil));
            },
            "Invalid tree recoil");
        reject_unchanged(
            leaf,
            [recoil](auto& fragment) {
                static_cast<void>(world::update_tree_fragment(fragment, 0.125F, 0, recoil));
            },
            "Invalid fragment recoil");
    }
    reject_unchanged(
        crown,
        [](auto& tree) {
            static_cast<void>(world::damage_tree(tree, -1));
        },
        "Invalid branch count");
    reject_unchanged(
        crown,
        [](auto& tree) {
            world::start_tree_fall(tree, 0, 0);
        },
        "Undestroyed crown fall");
    auto destroyed = crown;
    destroyed.points = 0;
    reject_unchanged(
        destroyed,
        [](auto& tree) {
            world::start_tree_fall(tree, 32768, 0);
        },
        "Invalid fall x draw");
    reject_unchanged(
        destroyed,
        [](auto& tree) {
            world::start_tree_fall(tree, 0, 32768);
        },
        "Invalid fall y draw");
    world::start_tree_fall(destroyed, 0, 0);
    reject_unchanged(
        destroyed,
        [](auto& tree) {
            static_cast<void>(world::damage_tree(tree, 4));
        },
        "Repeat falling damage");
    reject_unchanged(
        destroyed,
        [](auto& tree) {
            world::start_tree_fall(tree, 0, 0);
        },
        "Repeat fall start");
    reject_unchanged(
        destroyed,
        [](auto& tree) {
            static_cast<void>(world::update_tree(tree, std::numeric_limits<float>::max(), 0, 0));
        },
        "Overflowing crown motion");
    reject_unchanged(
        leaf,
        [](auto& fragment) {
            static_cast<void>(world::update_tree_fragment(fragment, std::numeric_limits<float>::max(), 0, 0));
        },
        "Overflowing leaf motion");
    auto invalid_tree = crown;
    invalid_tree.frame = 4;
    reject_unchanged(invalid_tree, update_tree, "Unsupported tree frame");
    invalid_tree = crown;
    invalid_tree.world_x = std::numeric_limits<float>::infinity();
    reject_unchanged(invalid_tree, update_tree, "Nonfinite tree position");
    auto invalid_leaf = leaf;
    invalid_leaf.depth = 4;
    reject_unchanged(invalid_leaf, update_fragment, "Unsupported leaf depth");
    invalid_leaf = leaf;
    invalid_leaf.frame = 19;
    reject_unchanged(invalid_leaf, update_fragment, "Unsupported live leaf frame");
    invalid_leaf = leaf;
    static_cast<void>(world::hit_tree_leaf(invalid_leaf));
    reject_unchanged(
        invalid_leaf,
        [](auto& fragment) {
            static_cast<void>(world::hit_tree_leaf(fragment));
        },
        "Repeat leaf impact");
    auto inactive_tree = crown;
    inactive_tree.type = -1;
    require(world::update_tree(inactive_tree, 1, 0, 0) == world::TreeUpdate{}, "Inactive tree must do nothing");
    reject_unchanged(
        inactive_tree,
        [](auto& tree) {
            static_cast<void>(world::tree_draw(tree));
        },
        "Removed tree draw");
    auto inactive_leaf = leaf;
    inactive_leaf.type = -1;
    require(world::update_tree_fragment(inactive_leaf, 1, 0, 0) == world::TreeFragmentUpdate{}, "Inactive fragment must do nothing");
    reject_unchanged(
        inactive_leaf,
        [](auto& fragment) {
            static_cast<void>(world::tree_fragment_draw(fragment));
        },
        "Removed fragment draw");
    for (const int type : {0, 22, 62, 67}) {
        invalid_tree = crown;
        invalid_tree.type = type;
        reject_unchanged(invalid_tree, update_tree, "Unsupported tree type");
    }
    for (const int type : {0, 26, 29, 66}) {
        invalid_leaf = leaf;
        invalid_leaf.type = type;
        reject_unchanged(invalid_leaf, update_fragment, "Unsupported fragment type");
    }
    for (const int pointer : {-17, 640}) {
        reject_unchanged(
            leaf,
            [pointer](auto&) {
                static_cast<void>(world::make_tree_leaf(0, pointer, {0, 0, 0, 0, 0, 0}));
            },
            "Invalid spawn pointer");
    }
    reject_unchanged(
        leaf,
        [](auto&) {
            static_cast<void>(world::make_tree_leaf(0, 0, {0, 0, 0, 0, 0, 32768}));
        },
        "Invalid leaf spawn draw");
    reject_unchanged(
        leaf,
        [](auto&) {
            static_cast<void>(world::make_tree_branch(0, 0, {0, 0, 0, 0, 32768}));
        },
        "Invalid released branch draw");
    invalid_tree = crown;
    invalid_tree.screen_x = 2147483648.0F;
    reject_unchanged(
        invalid_tree,
        [](auto& tree) {
            static_cast<void>(world::tree_draw(tree));
        },
        "Unrepresentable tree draw pose");
    invalid_leaf = leaf;
    invalid_leaf.screen_y = 2147483648.0F;
    reject_unchanged(
        invalid_leaf,
        [](auto& fragment) {
            static_cast<void>(world::tree_fragment_draw(fragment));
        },
        "Unrepresentable leaf draw pose");
}

} // namespace

int main() {
    try {
        original_vectors();
        invalid_inputs();
        std::cout << "Original four-tree damage/fall, released leaf/branch/highfly states, cells, binary32 storage and reset passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
