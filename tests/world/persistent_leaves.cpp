#include "world/persistent_leaves.hpp"

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
#include <vector>

namespace {

using namespace moorhuhn2;

using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct InitializerCase {
    std::array<std::uint32_t, 4> random;
    world::PersistentLeaf after;
};

struct UpdateCase {
    std::string id;
    world::PersistentLeaf before;
    world::PersistentLeaf after;
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    std::vector<std::uint32_t> random;
};

struct PoolStep {
    std::uint32_t delta_bits;
    std::uint32_t camera_bits;
    int recoil;
    std::vector<std::uint32_t> random;
    std::uint32_t random_state;
    std::array<world::PersistentLeaf, 32> leaves;
};

struct PoolCase {
    std::uint32_t seed;
    std::vector<std::uint32_t> startup_random;
    std::uint32_t startup_random_state;
    std::array<world::PersistentLeaf, 32> initial;
    std::vector<PoolStep> steps;
};

const std::array<InitializerCase, 4> initializers_cases = {{InitializerCase{{{0U, 0U, 0U, 0U}}, world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1}},
    InitializerCase{{{32767U, 32767U, 32767U, 32767U}}, world::PersistentLeaf{763.0F, 303.0F, -1024.0F, -1024.0F, binary32(0x3f2147aeU), 7.0F, 3, 1}},
    InitializerCase{{{64U, 65U, 66U, 8U}}, world::PersistentLeaf{700.0F, 241.0F, -1024.0F, -1024.0F, binary32(0x3ca3d70aU), 0.0F, 3, 1}},
    InitializerCase{{{31U, 48U, 47U, 5U}}, world::PersistentLeaf{731.0F, 288.0F, -1024.0F, -1024.0F, binary32(0x3ef0a3d7U), 5.0F, 3, 1}}}};

const std::array<UpdateCase, 15> updates_cases = {{UpdateCase{"zero-delta-still-draws", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
                                                       world::PersistentLeaf{700.0F, 240.0F, 700.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x00000000U, 0, {63U}},
    UpdateCase{"positive-drift-after-pose", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{701.0F, 240.0F, 700.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x00000000U, 0, {0U}},
    UpdateCase{"negative-drift-after-pose", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{699.0F, 240.0F, 700.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x00000000U, 0, {1U}},
    UpdateCase{"amplitude-change-after-pose", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{700.0F, 240.0F, 700.0F, 255.0F, 0.0F, 7.0F, 3, 1}, 0x00000000U, 0x00000000U, 0, {2U, 32767U}},
    UpdateCase{"next-phase-does-not-change-current-pose", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{700.0F, 240.0F, 700.0F, 255.0F, binary32(0x3ee66667U), 0.0F, 3, 1}, 0x41200000U, 0x00000000U, 0, {3U}},
    UpdateCase{"old-sway-and-conditional-residue", world::PersistentLeaf{763.0F, 303.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 4.0F, 3, 1},
        world::PersistentLeaf{763.0F, 303.0F, binary32(0xc2e2a931U), binary32(0x43994eabU), binary32(0x3e82e148U), 7.0F, 3, 1}, 0x3e000000U, 0x44200000U, 8, {66U, 32767U}},
    UpdateCase{"camera-product-retained640", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{700.0F, 240.0F, -195.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x44200000U, 0, {32767U}},
    UpdateCase{"camera-product-retained1280", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{700.0F, 240.0F, -1091.0F, binary32(0x4382b333U), 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x44a00000U, 8, {32767U}},
    UpdateCase{"camera-below-first-pixel", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{701.0F, 240.0F, 700.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x3f36db6dU, 0, {64U}},
    UpdateCase{"camera-above-first-pixel", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{699.0F, 240.0F, 699.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000000U, 0x3f36db6eU, 0, {65U}},
    UpdateCase{"fractional-negative-pose", world::PersistentLeaf{binary32(0xc0b00000U), binary32(0xc0b80000U), -1024.0F, -1024.0F, binary32(0x3e800000U), 4.0F, 3, 1},
        world::PersistentLeaf{binary32(0xc0b00000U), binary32(0xc0b80000U), binary32(0x4152b678U), binary32(0xc0c4552aU), binary32(0x3e80b852U), 4.0F, 3, 1}, 0x3d000000U,
        0x00000000U, 3, {62U}},
    UpdateCase{"no-wrap-no-removal", world::PersistentLeaf{2450.0F, 900.0F, -1024.0F, -1024.0F, binary32(0x3f8ccccdU), 4.0F, 3, 1},
        world::PersistentLeaf{2450.0F, 900.0F, binary32(0x4425a9d5U), binary32(0x4464ae15U), binary32(0x42386667U), 4.0F, 3, 1}, 0x447a0000U, 0x44a00000U, 0, {63U}},
    UpdateCase{"maximum-initial-phase-zero-delta", world::PersistentLeaf{763.0F, 303.0F, -1024.0F, -1024.0F, binary32(0x3f2147aeU), 7.0F, 3, 1},
        world::PersistentLeaf{763.0F, 303.0F, binary32(0x42280b84U), binary32(0x439a0f6aU), binary32(0x3f2147aeU), 0.0F, 3, 1}, 0x00000000U, 0x43fa0000U, 7, {130U, 8U}},
    UpdateCase{"phase-minimum-float-step", world::PersistentLeaf{700.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 3, 1},
        world::PersistentLeaf{700.0F, 240.0F, 700.0F, 255.0F, 0.0F, 0.0F, 3, 1}, 0x00000001U, 0x00000000U, 0, {3U}},
    UpdateCase{"phase-rate-retained-before-store", world::PersistentLeaf{763.0F, 303.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 4.0F, 3, 1},
        world::PersistentLeaf{763.0F, 303.0F, binary32(0xc2e2a931U), binary32(0x439681dfU), binary32(0x3e875f70U), 4.0F, 3, 1}, 0x3ea3d70aU, 0x44200000U, 1, {63U}}}};

const std::array<PoolCase, 2> pools_cases = {
    {PoolCase{0U,
         {38U, 7719U, 21238U, 2437U, 8855U, 11797U, 8365U, 32285U, 10450U, 30612U, 5853U, 28100U, 1142U, 281U, 20537U, 15921U, 8945U, 26285U, 2997U, 14680U, 20976U, 31891U, 21655U,
             25906U, 18457U, 1323U, 28881U, 2240U, 9725U, 32278U, 2446U, 590U, 840U, 18587U, 16907U, 21237U, 23611U, 12617U, 12456U, 867U, 29533U, 6878U, 28223U, 17887U, 31597U,
             20584U, 12212U, 31111U, 7578U, 17066U, 7629U, 29404U, 12279U, 13505U, 24388U, 11649U, 12329U, 7176U, 2331U, 19264U, 22114U, 14136U, 26928U, 1102U, 21652U, 8404U,
             24337U, 27856U, 5598U, 24772U, 14097U, 13213U, 4683U, 16703U, 15260U, 15942U, 2747U, 27375U, 28871U, 18004U, 16673U, 3152U, 11819U, 23504U, 239U, 4186U, 2804U, 28937U,
             3023U, 10335U, 20533U, 21393U, 16020U, 11574U, 25983U, 13961U, 624U, 7065U, 27569U, 12830U, 24167U, 12236U, 31249U, 23636U, 15619U, 25727U, 12700U, 24323U, 14922U,
             22774U, 6811U, 12062U, 3565U, 25191U, 119U, 18747U, 26050U, 19620U, 16334U, 5968U, 17524U, 3577U, 15046U, 25531U, 14458U, 29272U, 26530U, 32134U},
         4253435008U,
         {{world::PersistentLeaf{738.0F, 279.0F, -1024.0F, -1024.0F, binary32(0x3f0a3d70U), 5.0F, 3, 1},
             world::PersistentLeaf{723.0F, 261.0F, -1024.0F, -1024.0F, binary32(0x3ee66666U), 5.0F, 3, 1},
             world::PersistentLeaf{718.0F, 260.0F, -1024.0F, -1024.0F, binary32(0x3e947ae1U), 4.0F, 3, 1},
             world::PersistentLeaf{754.0F, 265.0F, -1024.0F, -1024.0F, binary32(0x3f11eb85U), 1.0F, 3, 1},
             world::PersistentLeaf{749.0F, 285.0F, -1024.0F, -1024.0F, binary32(0x3f07ae14U), 0.0F, 3, 1},
             world::PersistentLeaf{748.0F, 259.0F, -1024.0F, -1024.0F, binary32(0x3e6b851eU), 2.0F, 3, 1},
             world::PersistentLeaf{725.0F, 283.0F, -1024.0F, -1024.0F, binary32(0x3e2e147bU), 0.0F, 3, 1},
             world::PersistentLeaf{761.0F, 262.0F, -1024.0F, -1024.0F, binary32(0x3e0f5c29U), 6.0F, 3, 1},
             world::PersistentLeaf{708.0F, 267.0F, -1024.0F, -1024.0F, binary32(0x3de147aeU), 5.0F, 3, 1},
             world::PersistentLeaf{759.0F, 249.0F, -1024.0F, -1024.0F, binary32(0x3eccccccU), 3.0F, 3, 1},
             world::PersistentLeaf{729.0F, 270.0F, -1024.0F, -1024.0F, binary32(0x3f2147aeU), 7.0F, 3, 1},
             world::PersistentLeaf{745.0F, 280.0F, -1024.0F, -1024.0F, binary32(0x3f051eb8U), 7.0F, 3, 1},
             world::PersistentLeaf{726.0F, 282.0F, -1024.0F, -1024.0F, binary32(0x3e051eb8U), 4.0F, 3, 1},
             world::PersistentLeaf{755.0F, 241.0F, -1024.0F, -1024.0F, binary32(0x3d23d70aU), 1.0F, 3, 1},
             world::PersistentLeaf{741.0F, 248.0F, -1024.0F, -1024.0F, binary32(0x3e8a3d70U), 0.0F, 3, 1},
             world::PersistentLeaf{734.0F, 296.0F, -1024.0F, -1024.0F, binary32(0x3ef5c28fU), 6.0F, 3, 1},
             world::PersistentLeaf{720.0F, 260.0F, -1024.0F, -1024.0F, binary32(0x3e2e147bU), 0.0F, 3, 1},
             world::PersistentLeaf{730.0F, 244.0F, -1024.0F, -1024.0F, binary32(0x3e2e147bU), 5.0F, 3, 1},
             world::PersistentLeaf{711.0F, 303.0F, -1024.0F, -1024.0F, binary32(0x3e8f5c29U), 6.0F, 3, 1},
             world::PersistentLeaf{759.0F, 287.0F, -1024.0F, -1024.0F, binary32(0x3d8f5c29U), 4.0F, 3, 1},
             world::PersistentLeaf{733.0F, 256.0F, -1024.0F, -1024.0F, binary32(0x3edc28f5U), 0.0F, 3, 1},
             world::PersistentLeaf{747.0F, 266.0F, -1024.0F, -1024.0F, binary32(0x3f051eb8U), 1.0F, 3, 1},
             world::PersistentLeaf{715.0F, 271.0F, -1024.0F, -1024.0F, binary32(0x3f07ae14U), 1.0F, 3, 1},
             world::PersistentLeaf{720.0F, 294.0F, -1024.0F, -1024.0F, binary32(0x3f2147aeU), 1.0F, 3, 1},
             world::PersistentLeaf{748.0F, 265.0F, -1024.0F, -1024.0F, binary32(0x3efae147U), 6.0F, 3, 1},
             world::PersistentLeaf{739.0F, 252.0F, -1024.0F, -1024.0F, binary32(0x3e2e147bU), 4.0F, 3, 1},
             world::PersistentLeaf{703.0F, 303.0F, -1024.0F, -1024.0F, binary32(0x3e8f5c29U), 3.0F, 3, 1},
             world::PersistentLeaf{710.0F, 294.0F, -1024.0F, -1024.0F, binary32(0x3e8a3d70U), 6.0F, 3, 1},
             world::PersistentLeaf{745.0F, 279.0F, -1024.0F, -1024.0F, binary32(0x3f0ccccdU), 3.0F, 3, 1},
             world::PersistentLeaf{702.0F, 276.0F, -1024.0F, -1024.0F, binary32(0x3e0f5c29U), 0.0F, 3, 1},
             world::PersistentLeaf{752.0F, 297.0F, -1024.0F, -1024.0F, binary32(0x3d75c28fU), 3.0F, 3, 1},
             world::PersistentLeaf{758.0F, 264.0F, -1024.0F, -1024.0F, binary32(0x3eae147bU), 6.0F, 3, 1}}},
         {PoolStep{0x00000000U, 0x00000000U, 0,
              {32581U, 435U, 11667U, 29160U, 26814U, 9642U, 976U, 32527U, 3823U, 21094U, 1768U, 9244U, 13824U, 14021U, 30807U, 29296U, 26726U, 14263U, 23640U, 1062U, 9047U, 15080U,
                  10747U, 15328U, 10113U, 1183U, 16759U, 12743U, 16891U, 11494U, 7361U, 29133U},
              1909300640U,
              {{world::PersistentLeaf{738.0F, 279.0F, binary32(0x4436353bU), binary32(0x43922a61U), binary32(0x3f0a3d70U), 5.0F, 3, 1},
                  world::PersistentLeaf{723.0F, 261.0F, binary32(0x4437c701U), binary32(0x43887811U), binary32(0x3ee66666U), 5.0F, 3, 1},
                  world::PersistentLeaf{718.0F, 260.0F, binary32(0x442fc199U), binary32(0x437ac496U), binary32(0x3e947ae1U), 4.0F, 3, 1},
                  world::PersistentLeaf{754.0F, 265.0F, binary32(0x4440495eU), binary32(0x4386eac4U), binary32(0x3f11eb85U), 1.0F, 3, 1},
                  world::PersistentLeaf{749.0F, 285.0F, binary32(0x4437ffacU), binary32(0x43923cb9U), binary32(0x3f07ae14U), 0.0F, 3, 1},
                  world::PersistentLeaf{748.0F, 259.0F, binary32(0x443d70c2U), binary32(0x4387a3e6U), binary32(0x3e6b851eU), 2.0F, 3, 1},
                  world::PersistentLeaf{725.0F, 283.0F, binary32(0x4434161eU), binary32(0x43865ec5U), binary32(0x3e2e147bU), 0.0F, 3, 1},
                  world::PersistentLeaf{761.0F, 262.0F, binary32(0x44436d35U), binary32(0x4381c00aU), binary32(0x3e0f5c29U), 6.0F, 3, 1},
                  world::PersistentLeaf{708.0F, 267.0F, binary32(0x443118c4U), binary32(0x438cffa4U), binary32(0x3de147aeU), 5.0F, 3, 1},
                  world::PersistentLeaf{759.0F, 249.0F, binary32(0x443a267eU), binary32(0x436fff96U), binary32(0x3eccccccU), 3.0F, 3, 1},
                  world::PersistentLeaf{729.0F, 270.0F, binary32(0x4430c0b8U), binary32(0x4386c29eU), binary32(0x3f2147aeU), 7.0F, 3, 1},
                  world::PersistentLeaf{745.0F, 280.0F, binary32(0x4434c1dbU), binary32(0x438b9d7fU), binary32(0x3f051eb8U), 7.0F, 3, 1},
                  world::PersistentLeaf{727.0F, 282.0F, binary32(0x4439dd60U), binary32(0x438ff574U), binary32(0x3e051eb8U), 4.0F, 3, 1},
                  world::PersistentLeaf{755.0F, 241.0F, binary32(0x443fc126U), binary32(0x436718faU), binary32(0x3d23d70aU), 1.0F, 3, 1},
                  world::PersistentLeaf{741.0F, 248.0F, binary32(0x443a226dU), binary32(0x43696c57U), binary32(0x3e8a3d70U), 0.0F, 3, 1},
                  world::PersistentLeaf{734.0F, 296.0F, binary32(0x443b29e2U), binary32(0x438ea0c6U), binary32(0x3ef5c28fU), 6.0F, 3, 1},
                  world::PersistentLeaf{720.0F, 260.0F, binary32(0x4432d61eU), binary32(0x4375bd8bU), binary32(0x3e2e147bU), 0.0F, 3, 1},
                  world::PersistentLeaf{730.0F, 244.0F, binary32(0x4434f2d2U), binary32(0x4365bd8bU), binary32(0x3e2e147bU), 5.0F, 3, 1},
                  world::PersistentLeaf{711.0F, 303.0F, binary32(0x44300652U), binary32(0x43906aa4U), binary32(0x3e8f5c29U), 6.0F, 3, 1},
                  world::PersistentLeaf{759.0F, 287.0F, binary32(0x443a1f46U), binary32(0x438aa8a1U), binary32(0x3d8f5c29U), 4.0F, 3, 1},
                  world::PersistentLeaf{733.0F, 256.0F, binary32(0x4435777fU), binary32(0x43869908U), binary32(0x3edc28f5U), 0.0F, 3, 1},
                  world::PersistentLeaf{747.0F, 266.0F, binary32(0x4436c159U), binary32(0x43849d7fU), binary32(0x3f051eb8U), 1.0F, 3, 1},
                  world::PersistentLeaf{715.0F, 271.0F, binary32(0x442f482fU), binary32(0x438b3cb9U), binary32(0x3f07ae14U), 1.0F, 3, 1},
                  world::PersistentLeaf{720.0F, 294.0F, binary32(0x44300086U), binary32(0x4392c29eU), binary32(0x3f2147aeU), 1.0F, 3, 1},
                  world::PersistentLeaf{747.0F, 265.0F, binary32(0x443c0a3aU), binary32(0x437a4c17U), binary32(0x3efae147U), 6.0F, 3, 1},
                  world::PersistentLeaf{739.0F, 252.0F, binary32(0x443746aeU), binary32(0x436dbd8bU), binary32(0x3e2e147bU), 4.0F, 3, 1},
                  world::PersistentLeaf{703.0F, 303.0F, binary32(0x442e456bU), binary32(0x43906aa4U), binary32(0x3e8f5c29U), 3.0F, 3, 1},
                  world::PersistentLeaf{710.0F, 294.0F, binary32(0x4432bcffU), binary32(0x438bb62bU), binary32(0x3e8a3d70U), 6.0F, 3, 1},
                  world::PersistentLeaf{745.0F, 279.0F, binary32(0x443aaf49U), binary32(0x4392f705U), binary32(0x3f0ccccdU), 3.0F, 3, 1},
                  world::PersistentLeaf{702.0F, 276.0F, binary32(0x44333293U), binary32(0x4388c00aU), binary32(0x3e0f5c29U), 0.0F, 3, 1},
                  world::PersistentLeaf{751.0F, 297.0F, binary32(0x443aafccU), binary32(0x438d5396U), binary32(0x3d75c28fU), 3.0F, 3, 1},
                  world::PersistentLeaf{758.0F, 264.0F, binary32(0x444098e8U), binary32(0x438a0e46U), binary32(0x3eae147bU), 6.0F, 3, 1}}}},
             PoolStep{0x3e000000U, 0x44200000U, 8,
                 {8572U, 28651U, 28254U, 28470U, 3019U, 23974U, 8821U, 20822U, 1910U, 11452U, 30248U, 32156U, 19911U, 13475U, 6436U, 719U, 6388U, 21255U, 3447U, 18840U, 8854U,
                     16493U, 14755U, 11841U, 26204U, 6167U, 24816U, 26556U, 32001U, 5927U, 30214U, 24551U},
                 1608992448U,
                 {{world::PersistentLeaf{738.0F, 279.0F, binary32(0xc3262b14U), binary32(0x43955d94U), binary32(0x3f0bae14U), 5.0F, 3, 1},
                     world::PersistentLeaf{723.0F, 261.0F, binary32(0xc31fe3fdU), binary32(0x438bab44U), binary32(0x3ee947aeU), 5.0F, 3, 1},
                     world::PersistentLeaf{718.0F, 260.0F, binary32(0xc33ff99aU), binary32(0x4380957eU), binary32(0x3e975c29U), 4.0F, 3, 1},
                     world::PersistentLeaf{754.0F, 265.0F, binary32(0xc2fbb513U), binary32(0x438a1df7U), binary32(0x3f135c29U), 1.0F, 3, 1},
                     world::PersistentLeaf{749.0F, 285.0F, binary32(0xc31f0150U), binary32(0x43956fedU), binary32(0x3f091eb8U), 0.0F, 3, 1},
                     world::PersistentLeaf{748.0F, 259.0F, binary32(0xc3093cf9U), binary32(0x438ad719U), binary32(0x3e7147adU), 2.0F, 3, 1},
                     world::PersistentLeaf{725.0F, 283.0F, binary32(0xc32ea789U), binary32(0x438991f9U), binary32(0x3e33d70aU), 0.0F, 3, 1},
                     world::PersistentLeaf{761.0F, 262.0F, binary32(0xc2e29659U), binary32(0x4384f33dU), binary32(0x3e151eb8U), 6.0F, 3, 1},
                     world::PersistentLeaf{708.0F, 267.0F, binary32(0xc33a9cefU), binary32(0x439032d7U), binary32(0x3deccccdU), 5.0F, 3, 1},
                     world::PersistentLeaf{759.0F, 249.0F, binary32(0xc3166607U), binary32(0x437665fdU), binary32(0x3ecfae14U), 3.0F, 3, 1},
                     world::PersistentLeaf{729.0F, 270.0F, binary32(0xc33bfd1fU), binary32(0x4389f5d1U), binary32(0x3f22b852U), 7.0F, 3, 1},
                     world::PersistentLeaf{745.0F, 280.0F, binary32(0xc32bf895U), binary32(0x438ed0b2U), binary32(0x3f068f5cU), 7.0F, 3, 1},
                     world::PersistentLeaf{727.0F, 282.0F, binary32(0xc3168a81U), binary32(0x439328a7U), binary32(0x3e0ae147U), 4.0F, 3, 1},
                     world::PersistentLeaf{755.0F, 241.0F, binary32(0xc2fff6cdU), binary32(0x436d7f61U), binary32(0x3d3ae148U), 1.0F, 3, 1},
                     world::PersistentLeaf{741.0F, 248.0F, binary32(0xc316764bU), binary32(0x436fd2bdU), binary32(0x3e8d1eb8U), 0.0F, 3, 1},
                     world::PersistentLeaf{734.0F, 296.0F, binary32(0xc3125879U), binary32(0x4391d3f9U), binary32(0x3ef8a3d7U), 6.0F, 3, 1},
                     world::PersistentLeaf{720.0F, 260.0F, binary32(0xc333a789U), binary32(0x437c23f1U), binary32(0x3e33d70aU), 0.0F, 3, 1},
                     world::PersistentLeaf{730.0F, 244.0F, binary32(0xc32b34b7U), binary32(0x436c23f1U), binary32(0x3e33d70aU), 5.0F, 3, 1},
                     world::PersistentLeaf{711.0F, 303.0F, binary32(0xc33ee6b9U), binary32(0x43939dd7U), binary32(0x3e923d71U), 6.0F, 3, 1},
                     world::PersistentLeaf{759.0F, 287.0F, binary32(0xc31682e9U), binary32(0x438ddbd4U), binary32(0x3d9ae148U), 4.0F, 3, 1},
                     world::PersistentLeaf{733.0F, 256.0F, binary32(0xc3292202U), binary32(0x4389cc3bU), binary32(0x3edf0a3dU), 0.0F, 3, 1},
                     world::PersistentLeaf{747.0F, 266.0F, binary32(0xc323fa9bU), binary32(0x4387d0b2U), binary32(0x3f068f5cU), 1.0F, 3, 1},
                     world::PersistentLeaf{715.0F, 271.0F, binary32(0xc341df44U), binary32(0x438e6fedU), binary32(0x3f091eb8U), 1.0F, 3, 1},
                     world::PersistentLeaf{719.0F, 294.0F, binary32(0xc33efde8U), binary32(0x4395f5d1U), binary32(0x3f22b852U), 1.0F, 3, 1},
                     world::PersistentLeaf{747.0F, 265.0F, binary32(0xc30fd717U), binary32(0x4380593fU), binary32(0x3efdc28fU), 6.0F, 3, 1},
                     world::PersistentLeaf{739.0F, 252.0F, binary32(0xc321e547U), binary32(0x437423f1U), binary32(0x3e33d70aU), 4.0F, 3, 1},
                     world::PersistentLeaf{703.0F, 303.0F, binary32(0xc345ea55U), binary32(0x43939dd7U), binary32(0x3e923d71U), 3.0F, 3, 1},
                     world::PersistentLeaf{710.0F, 294.0F, binary32(0xc3340c03U), binary32(0x438ee95fU), binary32(0x3e8d1eb8U), 6.0F, 3, 1},
                     world::PersistentLeaf{744.0F, 279.0F, binary32(0xc31442deU), binary32(0x43962a38U), binary32(0x3f0e3d71U), 3.0F, 3, 1},
                     world::PersistentLeaf{702.0F, 276.0F, binary32(0xc33235b2U), binary32(0x438bf33dU), binary32(0x3e151eb8U), 0.0F, 3, 1},
                     world::PersistentLeaf{751.0F, 297.0F, binary32(0xc31540d1U), binary32(0x439086c9U), binary32(0x3d866666U), 3.0F, 3, 1},
                     world::PersistentLeaf{758.0F, 264.0F, binary32(0xc2f938c2U), binary32(0x438d4179U), binary32(0x3eb0f5c3U), 6.0F, 3, 1}}}},
             PoolStep{0x3c800000U, 0x44a00000U, 0,
                 {9083U, 22279U, 31674U, 24846U, 17014U, 1799U, 26665U, 15537U, 2560U, 26952U, 15875U, 20360U, 30086U, 5850U, 20522U, 1733U, 19455U, 20257U, 25468U, 19097U, 28775U,
                     22141U, 8940U, 29177U, 5743U, 10281U, 6616U, 9122U, 12147U, 32612U, 10391U, 18523U},
                 3361423328U,
                 {{world::PersistentLeaf{738.0F, 279.0F, binary32(0xc484021fU), binary32(0x4392e94dU), binary32(0x3f0bdc28U), 5.0F, 3, 1},
                     world::PersistentLeaf{723.0F, 261.0F, binary32(0xc4836f1eU), binary32(0x4386b92eU), binary32(0x3ee9a3d7U), 5.0F, 3, 1},
                     world::PersistentLeaf{718.0F, 260.0F, binary32(0xc4885d0eU), binary32(0x437efce4U), binary32(0x3e97b852U), 4.0F, 3, 1},
                     world::PersistentLeaf{754.0F, 265.0F, binary32(0xc47f4004U), binary32(0x43848b37U), binary32(0x3f138a3dU), 1.0F, 3, 1},
                     world::PersistentLeaf{749.0F, 285.0F, binary32(0xc4837efbU), binary32(0x43941abaU), binary32(0x3f094cccU), 0.0F, 3, 1},
                     world::PersistentLeaf{748.0F, 259.0F, binary32(0xc480aaa4U), binary32(0x4385f5c2U), binary32(0x3e71ffffU), 2.0F, 3, 1},
                     world::PersistentLeaf{725.0F, 283.0F, binary32(0xc4865dceU), binary32(0x43877973U), binary32(0x3e348f5cU), 0.0F, 3, 1},
                     world::PersistentLeaf{761.0F, 262.0F, binary32(0xc47cddf7U), binary32(0x437ef1c3U), binary32(0x3e15d70aU), 6.0F, 3, 1},
                     world::PersistentLeaf{709.0F, 267.0F, binary32(0xc4868994U), binary32(0x438c910aU), binary32(0x3dee3d71U), 5.0F, 3, 1},
                     world::PersistentLeaf{759.0F, 249.0F, binary32(0xc4832284U), binary32(0x43744328U), binary32(0x3ed00a3dU), 3.0F, 3, 1},
                     world::PersistentLeaf{729.0F, 270.0F, binary32(0xc487628aU), binary32(0x4389259aU), binary32(0x3f22e666U), 7.0F, 3, 1},
                     world::PersistentLeaf{745.0F, 280.0F, binary32(0xc4856650U), binary32(0x438e01e7U), binary32(0x3f06bd70U), 7.0F, 3, 1},
                     world::PersistentLeaf{727.0F, 282.0F, binary32(0xc482a21bU), binary32(0x438d9fa4U), binary32(0x3e0b9999U), 4.0F, 3, 1},
                     world::PersistentLeaf{755.0F, 241.0F, binary32(0xc4807e49U), binary32(0x436409edU), binary32(0x3d3dc290U), 1.0F, 3, 1},
                     world::PersistentLeaf{741.0F, 248.0F, binary32(0xc483685cU), binary32(0x43690d99U), binary32(0x3e8d7ae1U), 0.0F, 3, 1},
                     world::PersistentLeaf{734.0F, 296.0F, binary32(0xc482fba9U), binary32(0x438d3f34U), binary32(0x3ef90000U), 6.0F, 3, 1},
                     world::PersistentLeaf{720.0F, 260.0F, binary32(0xc486fdceU), binary32(0x4377f2e6U), binary32(0x3e348f5cU), 0.0F, 3, 1},
                     world::PersistentLeaf{730.0F, 244.0F, binary32(0xc4861d12U), binary32(0x4367f2e6U), binary32(0x3e348f5cU), 5.0F, 3, 1},
                     world::PersistentLeaf{711.0F, 303.0F, binary32(0xc4889a7fU), binary32(0x43918fdcU), binary32(0x3e92999aU), 6.0F, 3, 1},
                     world::PersistentLeaf{759.0F, 287.0F, binary32(0xc48334c4U), binary32(0x438cb8e9U), binary32(0x3d9c51ecU), 4.0F, 3, 1},
                     world::PersistentLeaf{733.0F, 256.0F, binary32(0xc48492c1U), binary32(0x43876340U), binary32(0x3edf6666U), 0.0F, 3, 1},
                     world::PersistentLeaf{747.0F, 266.0F, binary32(0xc4846d52U), binary32(0x438701e7U), binary32(0x3f06bd70U), 1.0F, 3, 1},
                     world::PersistentLeaf{715.0F, 271.0F, binary32(0xc487d43fU), binary32(0x438d1abaU), binary32(0x3f094cccU), 1.0F, 3, 1},
                     world::PersistentLeaf{719.0F, 294.0F, binary32(0xc487ea93U), binary32(0x4395259aU), binary32(0x3f22e666U), 1.0F, 3, 1},
                     world::PersistentLeaf{747.0F, 265.0F, binary32(0xc482d25fU), binary32(0x437a1cf5U), binary32(0x3efe1eb8U), 6.0F, 3, 1},
                     world::PersistentLeaf{739.0F, 252.0F, binary32(0xc484ea04U), binary32(0x436ff2e6U), binary32(0x3e348f5cU), 4.0F, 3, 1},
                     world::PersistentLeaf{703.0F, 303.0F, binary32(0xc4895fdbU), binary32(0x43918fdcU), binary32(0x3e92999aU), 3.0F, 3, 1},
                     world::PersistentLeaf{710.0F, 294.0F, binary32(0xc4875881U), binary32(0x438b86cdU), binary32(0x3e8d7ae1U), 6.0F, 3, 1},
                     world::PersistentLeaf{744.0F, 279.0F, binary32(0xc481f5a2U), binary32(0x439259e2U), binary32(0x3f0e6b85U), 3.0F, 3, 1},
                     world::PersistentLeaf{702.0F, 276.0F, binary32(0xc486786aU), binary32(0x438678e1U), binary32(0x3e15d70aU), 0.0F, 3, 1},
                     world::PersistentLeaf{751.0F, 297.0F, binary32(0xc4834df2U), binary32(0x438e639eU), binary32(0x3d87d70aU), 3.0F, 3, 1},
                     world::PersistentLeaf{758.0F, 264.0F, binary32(0xc47df833U), binary32(0x438857adU), binary32(0x3eb151ecU), 6.0F, 3, 1}}}}}},
        PoolCase{42U,
            {175U, 400U, 17869U, 30056U, 16083U, 12879U, 8016U, 7644U, 15809U, 1769U, 32409U, 29950U, 13471U, 7099U, 2336U, 23225U, 21188U, 13712U, 14351U, 19676U, 8907U, 26730U,
                29686U, 25607U, 9190U, 7040U, 6336U, 6091U, 3445U, 3715U, 30182U, 3597U, 28888U, 17445U, 7906U, 31147U, 11649U, 30172U, 8167U, 19555U, 27469U, 29923U, 27343U,
                16368U, 3860U, 7299U, 4181U, 31349U, 23935U, 14736U, 6532U, 21023U, 16409U, 2870U, 5487U, 29702U, 24745U, 16630U, 12067U, 29269U, 3776U, 6635U, 24538U, 32171U,
                9102U, 2386U, 4465U, 19905U, 22521U, 1097U, 17692U, 32076U, 16625U, 23767U, 4697U, 23845U, 26109U, 20013U, 4642U, 679U, 9754U, 7723U, 2663U, 5464U, 24262U, 30758U,
                28091U, 8140U, 2392U, 25833U, 846U, 26567U, 17042U, 1017U, 19227U, 4514U, 28793U, 27166U, 29282U, 4848U, 31332U, 18718U, 27737U, 14433U, 18260U, 17612U, 799U,
                17126U, 30339U, 6465U, 29937U, 24597U, 1600U, 30314U, 29088U, 464U, 23801U, 13507U, 3138U, 22817U, 26778U, 18273U, 29992U, 30825U, 18454U, 3894U, 4113U, 24762U},
            1622821034U,
            {{world::PersistentLeaf{747.0F, 256.0F, -1024.0F, -1024.0F, binary32(0x3e051eb8U), 0.0F, 3, 1},
                world::PersistentLeaf{719.0F, 255.0F, -1024.0F, -1024.0F, binary32(0x3e23d70aU), 4.0F, 3, 1},
                world::PersistentLeaf{701.0F, 281.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 6.0F, 3, 1},
                world::PersistentLeaf{731.0F, 299.0F, -1024.0F, -1024.0F, binary32(0x3ea3d70aU), 1.0F, 3, 1},
                world::PersistentLeaf{704.0F, 256.0F, -1024.0F, -1024.0F, binary32(0x3e199999U), 4.0F, 3, 1},
                world::PersistentLeaf{711.0F, 282.0F, -1024.0F, -1024.0F, binary32(0x3f0a3d70U), 7.0F, 3, 1},
                world::PersistentLeaf{738.0F, 240.0F, -1024.0F, -1024.0F, 0.0F, 3.0F, 3, 1},
                world::PersistentLeaf{753.0F, 243.0F, -1024.0F, -1024.0F, binary32(0x3ec28f5cU), 5.0F, 3, 1},
                world::PersistentLeaf{724.0F, 277.0F, -1024.0F, -1024.0F, binary32(0x3eae147bU), 3.0F, 3, 1},
                world::PersistentLeaf{701.0F, 268.0F, -1024.0F, -1024.0F, binary32(0x3ec7ae14U), 3.0F, 3, 1},
                world::PersistentLeaf{713.0F, 275.0F, -1024.0F, -1024.0F, binary32(0x3e199999U), 0.0F, 3, 1},
                world::PersistentLeaf{720.0F, 243.0F, -1024.0F, -1024.0F, binary32(0x3e570a3dU), 5.0F, 3, 1},
                world::PersistentLeaf{763.0F, 256.0F, -1024.0F, -1024.0F, binary32(0x3d23d70aU), 7.0F, 3, 1},
                world::PersistentLeaf{725.0F, 294.0F, -1024.0F, -1024.0F, binary32(0x3ef0a3d7U), 6.0F, 3, 1},
                world::PersistentLeaf{741.0F, 294.0F, -1024.0F, -1024.0F, binary32(0x3eb33333U), 5.0F, 3, 1},
                world::PersistentLeaf{700.0F, 283.0F, -1024.0F, -1024.0F, binary32(0x3e851eb8U), 3.0F, 3, 1},
                world::PersistentLeaf{714.0F, 258.0F, -1024.0F, -1024.0F, binary32(0x3efae147U), 1.0F, 3, 1},
                world::PersistentLeaf{757.0F, 249.0F, -1024.0F, -1024.0F, binary32(0x3e8f5c29U), 4.0F, 3, 1},
                world::PersistentLeaf{749.0F, 263.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 5.0F, 3, 1},
                world::PersistentLeaf{761.0F, 285.0F, -1024.0F, -1024.0F, binary32(0x3eae147bU), 7.0F, 3, 1},
                world::PersistentLeaf{726.0F, 283.0F, -1024.0F, -1024.0F, binary32(0x3ec7ae14U), 0.0F, 3, 1},
                world::PersistentLeaf{706.0F, 278.0F, -1024.0F, -1024.0F, binary32(0x3f170a3dU), 4.0F, 3, 1},
                world::PersistentLeaf{724.0F, 281.0F, -1024.0F, -1024.0F, binary32(0x3e0f5c29U), 7.0F, 3, 1},
                world::PersistentLeaf{718.0F, 297.0F, -1024.0F, -1024.0F, binary32(0x3e8a3d70U), 2.0F, 3, 1},
                world::PersistentLeaf{757.0F, 270.0F, -1024.0F, -1024.0F, binary32(0x3eae147bU), 0.0F, 3, 1},
                world::PersistentLeaf{736.0F, 270.0F, -1024.0F, -1024.0F, binary32(0x3e800000U), 1.0F, 3, 1},
                world::PersistentLeaf{720.0F, 252.0F, -1024.0F, -1024.0F, binary32(0x3e9eb852U), 6.0F, 3, 1},
                world::PersistentLeaf{703.0F, 241.0F, -1024.0F, -1024.0F, binary32(0x3efae147U), 5.0F, 3, 1},
                world::PersistentLeaf{700.0F, 282.0F, -1024.0F, -1024.0F, binary32(0x3ea3d70aU), 0.0F, 3, 1},
                world::PersistentLeaf{757.0F, 243.0F, -1024.0F, -1024.0F, binary32(0x3ca3d70aU), 1.0F, 3, 1},
                world::PersistentLeaf{726.0F, 273.0F, -1024.0F, -1024.0F, binary32(0x3eccccccU), 1.0F, 3, 1},
                world::PersistentLeaf{722.0F, 294.0F, -1024.0F, -1024.0F, binary32(0x3e2e147bU), 2.0F, 3, 1}}},
            {PoolStep{0x00000000U, 0x00000000U, 0,
                 {13122U, 29585U, 15005U, 32385U, 5354U, 25572U, 22277U, 22888U, 2847U, 9930U, 28424U, 31610U, 16846U, 6728U, 809U, 21384U, 23447U, 18771U, 23063U, 5836U, 23771U,
                     31379U, 25709U, 26061U, 19735U, 7271U, 2203U, 14466U, 625U, 12841U, 27428U, 21946U, 19345U, 9907U},
                 649303956U,
                 {{world::PersistentLeaf{747.0F, 256.0F, binary32(0x443e3223U), binary32(0x4382f574U), binary32(0x3e051eb8U), 1.0F, 3, 1},
                     world::PersistentLeaf{719.0F, 255.0F, binary32(0x4434f59eU), binary32(0x43707e90U), binary32(0x3e23d70aU), 4.0F, 3, 1},
                     world::PersistentLeaf{700.0F, 281.0F, binary32(0x443468a0U), binary32(0x438b1b78U), binary32(0x3e800000U), 6.0F, 3, 1},
                     world::PersistentLeaf{731.0F, 299.0F, binary32(0x4434c7baU), binary32(0x439c070dU), binary32(0x3ea3d70aU), 1.0F, 3, 1},
                     world::PersistentLeaf{704.0F, 256.0F, binary32(0x443381aaU), binary32(0x4375e1a5U), binary32(0x3e199999U), 4.0F, 3, 1},
                     world::PersistentLeaf{711.0F, 282.0F, binary32(0x442f3a8eU), binary32(0x4393aa61U), binary32(0x3f0a3d70U), 7.0F, 3, 1},
                     world::PersistentLeaf{738.0F, 240.0F, 738.0F, 255.0F, 0.0F, 3.0F, 3, 1},
                     world::PersistentLeaf{753.0F, 243.0F, binary32(0x443d55c7U), binary32(0x43645b83U), binary32(0x3ec28f5cU), 5.0F, 3, 1},
                     world::PersistentLeaf{724.0F, 277.0F, binary32(0x4437a7a2U), binary32(0x43908e46U), binary32(0x3eae147bU), 3.0F, 3, 1},
                     world::PersistentLeaf{701.0F, 268.0F, binary32(0x442db070U), binary32(0x437dee60U), binary32(0x3ec7ae14U), 3.0F, 3, 1},
                     world::PersistentLeaf{713.0F, 275.0F, binary32(0x443504aeU), binary32(0x438470d3U), binary32(0x3e199999U), 0.0F, 3, 1},
                     world::PersistentLeaf{720.0F, 243.0F, binary32(0x44317439U), binary32(0x437fe8e2U), binary32(0x3e570a3dU), 5.0F, 3, 1},
                     world::PersistentLeaf{763.0F, 256.0F, binary32(0x4442e195U), binary32(0x437618faU), binary32(0x3d23d70aU), 7.0F, 3, 1},
                     world::PersistentLeaf{725.0F, 294.0F, binary32(0x443a5dfbU), binary32(0x439152beU), binary32(0x3ef0a3d7U), 6.0F, 3, 1},
                     world::PersistentLeaf{741.0F, 294.0F, binary32(0x443dead7U), binary32(0x4395b0a0U), binary32(0x3eb33333U), 5.0F, 3, 1},
                     world::PersistentLeaf{700.0F, 283.0F, binary32(0x44324339U), binary32(0x438855a4U), binary32(0x3e851eb8U), 3.0F, 3, 1},
                     world::PersistentLeaf{714.0F, 258.0F, binary32(0x44334ad7U), binary32(0x43734c17U), binary32(0x3efae147U), 1.0F, 3, 1},
                     world::PersistentLeaf{757.0F, 249.0F, binary32(0x443bb062U), binary32(0x436ad548U), binary32(0x3e8f5c29U), 4.0F, 3, 1},
                     world::PersistentLeaf{749.0F, 263.0F, binary32(0x444029bdU), binary32(0x43821b78U), binary32(0x3e800000U), 5.0F, 3, 1},
                     world::PersistentLeaf{761.0F, 285.0F, binary32(0x44417eaaU), binary32(0x43948e46U), binary32(0x3eae147bU), 7.0F, 3, 1},
                     world::PersistentLeaf{726.0F, 283.0F, binary32(0x44343308U), binary32(0x43867730U), binary32(0x3ec7ae14U), 0.0F, 3, 1},
                     world::PersistentLeaf{706.0F, 278.0F, binary32(0x4433bf8dU), binary32(0x4385871bU), binary32(0x3f170a3dU), 4.0F, 3, 1},
                     world::PersistentLeaf{724.0F, 281.0F, binary32(0x443a6c50U), binary32(0x438b400aU), binary32(0x3e0f5c29U), 7.0F, 3, 1},
                     world::PersistentLeaf{718.0F, 297.0F, binary32(0x4434809eU), binary32(0x438d362bU), binary32(0x3e8a3d70U), 2.0F, 3, 1},
                     world::PersistentLeaf{757.0F, 270.0F, binary32(0x443f765cU), binary32(0x438d0e46U), binary32(0x3eae147bU), 0.0F, 3, 1},
                     world::PersistentLeaf{736.0F, 270.0F, binary32(0x443bee31U), binary32(0x43859b78U), binary32(0x3e800000U), 1.0F, 3, 1},
                     world::PersistentLeaf{720.0F, 252.0F, binary32(0x442f59b4U), binary32(0x43817b9bU), binary32(0x3e9eb852U), 1.0F, 3, 1},
                     world::PersistentLeaf{703.0F, 241.0F, binary32(0x4430bd8dU), binary32(0x43624c17U), binary32(0x3efae147U), 5.0F, 3, 1},
                     world::PersistentLeaf{700.0F, 282.0F, binary32(0x442d273eU), binary32(0x4393870dU), binary32(0x3ea3d70aU), 0.0F, 3, 1},
                     world::PersistentLeaf{757.0F, 243.0F, binary32(0x4440e4f4U), binary32(0x43792ee5U), binary32(0x3ca3d70aU), 1.0F, 3, 1},
                     world::PersistentLeaf{726.0F, 273.0F, binary32(0x44324ce2U), binary32(0x4383ffcbU), binary32(0x3eccccccU), 1.0F, 3, 1},
                     world::PersistentLeaf{722.0F, 294.0F, binary32(0x44332e66U), binary32(0x438bdec5U), binary32(0x3e2e147bU), 2.0F, 3, 1}}}},
                PoolStep{0x3e000000U, 0x44200000U, 8,
                    {1803U, 22972U, 27570U, 10274U, 10887U, 4138U, 7417U, 28890U, 11262U, 9515U, 3333U, 8650U, 27441U, 29128U, 1991U, 4593U, 29365U, 26519U, 17556U, 14112U, 19874U,
                        24471U, 2424U, 3074U, 25999U, 20572U, 6350U, 5979U, 31933U, 12140U, 7437U, 30861U, 6290U},
                    2559735463U,
                    {{world::PersistentLeaf{747.0F, 256.0F, binary32(0xc3054c37U), binary32(0x438628a7U), binary32(0x3e0ae147U), 1.0F, 3, 1},
                        world::PersistentLeaf{719.0F, 255.0F, binary32(0xc32b2988U), binary32(0x4376e4f6U), binary32(0x3e299999U), 4.0F, 3, 1},
                        world::PersistentLeaf{700.0F, 281.0F, binary32(0xc32e5d80U), binary32(0x438e4eabU), binary32(0x3e82e148U), 6.0F, 3, 1},
                        world::PersistentLeaf{731.0F, 299.0F, binary32(0xc32be118U), binary32(0x439f3a40U), binary32(0x3ea6b852U), 1.0F, 3, 1},
                        world::PersistentLeaf{704.0F, 256.0F, binary32(0xc330f95aU), binary32(0x437c480bU), binary32(0x3e1f5c28U), 4.0F, 3, 1},
                        world::PersistentLeaf{711.0F, 282.0F, binary32(0xc34215c9U), binary32(0x4396dd94U), binary32(0x3f0bae14U), 7.0F, 3, 1},
                        world::PersistentLeaf{738.0F, 240.0F, -157.0F, binary32(0x4382b333U), binary32(0x3bb851ecU), 3.0F, 3, 1},
                        world::PersistentLeaf{753.0F, 243.0F, binary32(0xc309a8e4U), binary32(0x436ac1eaU), binary32(0x3ec570a4U), 5.0F, 3, 1},
                        world::PersistentLeaf{724.0F, 277.0F, binary32(0xc3206178U), binary32(0x4393c179U), binary32(0x3eb0f5c3U), 3.0F, 3, 1},
                        world::PersistentLeaf{701.0F, 268.0F, binary32(0xc3483e41U), binary32(0x43822a63U), binary32(0x3eca8f5cU), 3.0F, 3, 1},
                        world::PersistentLeaf{713.0F, 275.0F, binary32(0xc32aed47U), binary32(0x4387a406U), binary32(0x3e1f5c28U), 0.0F, 3, 1},
                        world::PersistentLeaf{720.0F, 243.0F, binary32(0xc3392f1eU), binary32(0x438327a4U), binary32(0x3e5cccccU), 5.0F, 3, 1},
                        world::PersistentLeaf{763.0F, 256.0F, binary32(0xc2e6f35aU), binary32(0x437c7f61U), binary32(0x3d3ae148U), 7.0F, 3, 1},
                        world::PersistentLeaf{725.0F, 294.0F, binary32(0xc3158814U), binary32(0x439485f1U), binary32(0x3ef3851fU), 6.0F, 3, 1},
                        world::PersistentLeaf{741.0F, 294.0F, binary32(0xc30754a4U), binary32(0x4398e3d3U), binary32(0x3eb6147bU), 5.0F, 3, 1},
                        world::PersistentLeaf{700.0F, 283.0F, binary32(0xc335f31dU), binary32(0x438b88d7U), binary32(0x3e880000U), 3.0F, 3, 1},
                        world::PersistentLeaf{714.0F, 258.0F, binary32(0xc331d4a4U), binary32(0x4379b27eU), binary32(0x3efdc28fU), 1.0F, 3, 1},
                        world::PersistentLeaf{757.0F, 249.0F, binary32(0xc3103e77U), binary32(0x43713baeU), binary32(0x3e923d71U), 4.0F, 3, 1},
                        world::PersistentLeaf{749.0F, 263.0F, binary32(0xc2fcb219U), binary32(0x43854eabU), binary32(0x3e82e148U), 5.0F, 3, 1},
                        world::PersistentLeaf{761.0F, 285.0F, binary32(0xc2f20ab3U), binary32(0x4397c179U), binary32(0x3eb0f5c3U), 7.0F, 3, 1},
                        world::PersistentLeaf{726.0F, 283.0F, binary32(0xc32e33e1U), binary32(0x4389aa63U), binary32(0x3eca8f5cU), 0.0F, 3, 1},
                        world::PersistentLeaf{706.0F, 278.0F, binary32(0xc33001cdU), binary32(0x4388ba4eU), binary32(0x3f187ae1U), 4.0F, 3, 1},
                        world::PersistentLeaf{724.0F, 281.0F, binary32(0xc3154ec1U), binary32(0x438e733dU), binary32(0x3e151eb8U), 7.0F, 3, 1},
                        world::PersistentLeaf{718.0F, 297.0F, binary32(0xc32cfd88U), binary32(0x4390695fU), binary32(0x3e8d1eb8U), 7.0F, 3, 1},
                        world::PersistentLeaf{757.0F, 270.0F, binary32(0xc301268fU), binary32(0x43904179U), binary32(0x3eb0f5c3U), 0.0F, 3, 1},
                        world::PersistentLeaf{736.0F, 270.0F, binary32(0xc30f473dU), binary32(0x4388ceabU), binary32(0x3e82e148U), 1.0F, 3, 1},
                        world::PersistentLeaf{720.0F, 252.0F, binary32(0xc33d2b92U), binary32(0x4384aeceU), binary32(0x3ea1999aU), 1.0F, 3, 1},
                        world::PersistentLeaf{703.0F, 241.0F, binary32(0xc33c09cdU), binary32(0x4368b27eU), binary32(0x3efdc28fU), 5.0F, 3, 1},
                        world::PersistentLeaf{700.0F, 282.0F, binary32(0xc34a6307U), binary32(0x4396ba40U), binary32(0x3ea6b852U), 0.0F, 3, 1},
                        world::PersistentLeaf{757.0F, 243.0F, binary32(0xc2f6d85eU), binary32(0x437f954bU), binary32(0x3cd1eb85U), 1.0F, 3, 1},
                        world::PersistentLeaf{726.0F, 273.0F, binary32(0xc335cc78U), binary32(0x438732feU), binary32(0x3ecfae14U), 1.0F, 3, 1},
                        world::PersistentLeaf{722.0F, 294.0F, binary32(0xc3324668U), binary32(0x438f11f9U), binary32(0x3e33d70aU), 2.0F, 3, 1}}}},
                PoolStep{0x3c800000U, 0x44a00000U, 0,
                    {488U, 1765U, 7520U, 28745U, 24687U, 4491U, 1282U, 13289U, 29247U, 24913U, 13389U, 16243U, 9117U, 28793U, 16201U, 3680U, 7032U, 15947U, 3058U, 842U, 13385U,
                        29797U, 12859U, 22506U, 31805U, 30291U, 7506U, 18754U, 150U, 3935U, 21030U, 21531U, 26702U, 12970U},
                    2997544441U,
                    {{world::PersistentLeaf{747.0F, 256.0F, binary32(0xc48081c6U), binary32(0x43809fa4U), binary32(0x3e0b9999U), 1.0F, 3, 1},
                        world::PersistentLeaf{719.0F, 255.0F, binary32(0xc4862764U), binary32(0x43700811U), binary32(0x3e2a51ebU), 4.0F, 3, 1},
                        world::PersistentLeaf{700.0F, 281.0F, binary32(0xc4861536U), binary32(0x4388d844U), binary32(0x3e833d71U), 6.0F, 3, 1},
                        world::PersistentLeaf{731.0F, 299.0F, binary32(0xc484e203U), binary32(0x439cdc7eU), binary32(0x3ea7147bU), 1.0F, 3, 1},
                        world::PersistentLeaf{704.0F, 256.0F, binary32(0xc486b82fU), binary32(0x4372e526U), binary32(0x3e20147aU), 4.0F, 3, 1},
                        world::PersistentLeaf{711.0F, 282.0F, binary32(0xc4876befU), binary32(0x4394694dU), binary32(0x3f0bdc28U), 7.0F, 3, 1},
                        world::PersistentLeaf{738.0F, 240.0F, binary32(0xc482e98fU), binary32(0x437e3a4aU), binary32(0x3bcf5c2aU), 1.0F, 3, 1},
                        world::PersistentLeaf{753.0F, 243.0F, binary32(0xc4820225U), binary32(0x43641490U), binary32(0x3ec5cccdU), 5.0F, 3, 1},
                        world::PersistentLeaf{724.0F, 277.0F, binary32(0xc4838a5fU), binary32(0x438ed7adU), binary32(0x3eb151ecU), 3.0F, 3, 1},
                        world::PersistentLeaf{701.0F, 268.0F, binary32(0xc489a89dU), binary32(0x438026d6U), binary32(0x3ecaeb85U), 3.0F, 3, 1},
                        world::PersistentLeaf{713.0F, 275.0F, binary32(0xc485d676U), binary32(0x4382f293U), binary32(0x3e20147aU), 0.0F, 3, 1},
                        world::PersistentLeaf{720.0F, 243.0F, binary32(0xc48666a6U), binary32(0x4380d507U), binary32(0x3e5d851eU), 5.0F, 3, 1},
                        world::PersistentLeaf{763.0F, 256.0F, binary32(0xc47e3b48U), binary32(0x437309edU), binary32(0x3d3dc290U), 7.0F, 3, 1},
                        world::PersistentLeaf{725.0F, 294.0F, binary32(0xc4830252U), binary32(0x438f1819U), binary32(0x3ef3e148U), 6.0F, 3, 1},
                        world::PersistentLeaf{741.0F, 294.0F, binary32(0xc480c0a2U), binary32(0x4393557eU), binary32(0x3eb670a4U), 5.0F, 3, 1},
                        world::PersistentLeaf{700.0F, 283.0F, binary32(0xc487518bU), binary32(0x4386e0d0U), binary32(0x3e885c29U), 3.0F, 3, 1},
                        world::PersistentLeaf{714.0F, 258.0F, binary32(0xc486dec2U), binary32(0x43731cf5U), binary32(0x3efe1eb8U), 1.0F, 3, 1},
                        world::PersistentLeaf{757.0F, 249.0F, binary32(0xc482b367U), binary32(0x436d1fb8U), binary32(0x3e92999aU), 4.0F, 3, 1},
                        world::PersistentLeaf{749.0F, 263.0F, binary32(0xc4801127U), binary32(0x437fb087U), binary32(0x3e833d71U), 5.0F, 3, 1},
                        world::PersistentLeaf{761.0F, 285.0F, binary32(0xc47d0404U), binary32(0x4392d7adU), binary32(0x3eb151ecU), 7.0F, 3, 1},
                        world::PersistentLeaf{726.0F, 283.0F, binary32(0xc4864c82U), binary32(0x4387a6d6U), binary32(0x3ecaeb85U), 0.0F, 3, 1},
                        world::PersistentLeaf{706.0F, 278.0F, binary32(0xc486a225U), binary32(0x43842f5dU), binary32(0x3f18a8f5U), 4.0F, 3, 1},
                        world::PersistentLeaf{724.0F, 281.0F, binary32(0xc482f2beU), binary32(0x4388f8e1U), binary32(0x3e15d70aU), 7.0F, 3, 1},
                        world::PersistentLeaf{718.0F, 297.0F, binary32(0xc4865b32U), binary32(0x438d06cdU), binary32(0x3e8d7ae1U), 7.0F, 3, 1},
                        world::PersistentLeaf{757.0F, 270.0F, binary32(0xc47f7149U), binary32(0x438b57adU), binary32(0x3eb151ecU), 0.0F, 3, 1},
                        world::PersistentLeaf{736.0F, 270.0F, binary32(0xc48220ecU), binary32(0x43835844U), binary32(0x3e833d71U), 1.0F, 3, 1},
                        world::PersistentLeaf{720.0F, 252.0F, binary32(0xc48742cbU), binary32(0x4383684aU), binary32(0x3ea1f5c3U), 6.0F, 3, 1},
                        world::PersistentLeaf{703.0F, 241.0F, binary32(0xc4884e73U), binary32(0x43621cf5U), binary32(0x3efe1eb8U), 5.0F, 3, 1},
                        world::PersistentLeaf{700.0F, 282.0F, binary32(0xc488bbe3U), binary32(0x43945c7eU), binary32(0x3ea7147bU), 0.0F, 3, 1},
                        world::PersistentLeaf{757.0F, 243.0F, binary32(0xc47e8562U), binary32(0x43748943U), binary32(0x3cd7ae14U), 1.0F, 3, 1},
                        world::PersistentLeaf{726.0F, 273.0F, binary32(0xc48705cbU), binary32(0x43862194U), binary32(0x3ed00a3dU), 1.0F, 3, 1},
                        world::PersistentLeaf{722.0F, 294.0F, binary32(0xc486e3e9U), binary32(0x438cf973U), binary32(0x3e348f5cU), 2.0F, 3, 1}}}}}}}};

void draw(const world::PersistentLeaf& value) {
    const auto command = world::persistent_leaf_draw(value);
    require(command.image == "gfx/particle" && command.source.x == 0 && command.source.y == 1 && command.source.width == 8 && command.source.height == 1 && command.color_key
                && !command.mirror_x && command.destination.x == static_cast<int>(value.screen_x) && command.destination.y == static_cast<int>(value.screen_y),
        "Persistent leaf source cell, pose or color key differs");
}

struct CRT {
    std::uint32_t state{};
    std::vector<std::uint32_t> values;

    std::uint32_t operator()() {
        state = state * 214013U + 2531011U;
        const auto value = (state >> 16U) & 0x7fffU;
        values.push_back(value);
        return value;
    }
};

void vectors() {
    require(world::persistent_leaf_count == 32, "Original persistent particle count differs");
    for (const auto& value : initializers_cases) {
        const auto result = world::make_persistent_leaf(value.random);
        require(result == value.after, "Original persistent leaf initialization differs");
        draw(result);
    }
    for (const auto& value : updates_cases) {
        auto actual = value.before;
        const auto before = actual;
        const auto expected = value.after;
        const auto id = value.id;
        std::size_t consumed = 0;
        world::update_persistent_leaf(actual, binary32(value.delta_bits), binary32(value.camera_bits), value.recoil, [&] {
            require(actual == before, id + ": leaf changed before its random draws completed");
            require(consumed < value.random.size(), id + ": unexpected conditional random draw");
            return value.random[consumed++];
        });
        require(actual == expected, id + ": original persistent leaf update fields differ");
        require(consumed == value.random.size(), id + ": random consumption differs");
        draw(actual);
    }
    for (const auto& value : pools_cases) {
        CRT random{value.seed, {}};
        auto leaves = world::make_persistent_leaves([&] {
            return random();
        });
        require(random.values == value.startup_random && random.values.size() == 128 && random.state == value.startup_random_state,
            "Ordered persistent leaf startup CRT consumption differs");
        for (std::size_t index = 0; index < leaves.size(); ++index) {
            require(leaves[index] == value.initial[index], "Original persistent leaf startup slot differs");
        }
        for (const auto& step : value.steps) {
            random.values.clear();
            for (auto& actual : leaves) {
                world::update_persistent_leaf(actual, binary32(step.delta_bits), binary32(step.camera_bits), step.recoil, [&] {
                    return random();
                });
            }
            require(random.values == step.random && random.state == step.random_state, "Original ordered32 particle update CRT draws/state differ");
            for (std::size_t index = 0; index < leaves.size(); ++index) {
                require(leaves[index] == step.leaves[index], "Original ordered32 leaf update slot differs");
                draw(leaves[index]);
            }
        }
    }
}

template <class Function> void rejects(Function&& function) {
    bool rejected = false;
    try {
        function();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "Invalid persistent leaf request accepted");
}

void invalid_inputs() {
    rejects([] {
        static_cast<void>(world::make_persistent_leaf({0, 0, 32768, 0}));
    });
    rejects([] {
        static_cast<void>(world::make_persistent_leaves({}));
    });
    for (const auto delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        auto value = world::make_persistent_leaf({0, 0, 0, 0});
        const auto before = value;
        rejects([&] {
            world::update_persistent_leaf(value, delta, 0, 0, [] {
                throw std::runtime_error("Invalid input consumed RNG");
                return 0U;
            });
        });
        require(value == before, "Rejected persistent leaf time mutates state");
    }
    for (const auto camera : {-1.0F, 1281.0F, std::numeric_limits<float>::quiet_NaN()}) {
        auto value = world::PersistentLeaf{};
        rejects([&] {
            world::update_persistent_leaf(value, 0, camera, 0, [] {
                throw std::runtime_error("Invalid camera consumed RNG");
                return 0U;
            });
        });
    }
    for (const auto recoil : {-1, 9}) {
        auto value = world::PersistentLeaf{};
        rejects([&] {
            world::update_persistent_leaf(value, 0, 0, recoil, [] {
                return 0U;
            });
        });
    }
    auto value = world::PersistentLeaf{};
    rejects([&] {
        world::update_persistent_leaf(value, 0, 0, 0, {});
    });
    for (const auto first : {32768U, 2U}) {
        std::size_t consumed = 0;
        rejects([&] {
            world::update_persistent_leaf(value, 1, 0, 0, [&] {
                return consumed++ == 0 ? first : 32768U;
            });
        });
        require(value == world::PersistentLeaf{}, "Rejected persistent leaf randomness mutates state");
    }
    for (const auto amplitude : {-1.0F, 8.0F, 1.5F, std::numeric_limits<float>::quiet_NaN()}) {
        value.amplitude = amplitude;
        rejects([&] {
            static_cast<void>(world::persistent_leaf_draw(value));
        });
    }
}

} // namespace

int main() {
    try {
        vectors();
        invalid_inputs();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Persistent leaves: original setup, poses, ordered RNG and draw vectors passed\n";
}
