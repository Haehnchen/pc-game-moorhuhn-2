#include "world/scarecrow.hpp"

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

struct UpdateCase {
    std::string id;
    world::ScarecrowPiece before;
    float delta;
    float camera;
    int recoil;
    world::ScarecrowPiece after;
};

struct ImpactExpectation {
    bool release_hat;
    float popup_x;
    float popup_y;
    int popup_points;
};

struct ImpactCase {
    std::string id;
    world::ScarecrowPiece before;
    std::optional<world::ScarecrowPiece> adjacent;
    ImpactExpectation prepared;
    std::array<std::uint32_t, 2> random;
    world::ScarecrowPiece after;
};

struct RecordCase {
    int hits;
    bool ordered;
    int frame;
    int hits_after;
    bool ordered_after;
};

struct CompletionExpectation {
    int balloon_count;
    int chicken_count;
    bool magic_sound;
};

struct CompletionCase {
    int hits;
    bool completed;
    bool ordered;
    bool completed_after;
    CompletionExpectation result;
};

struct DrawExpectation {
    std::string image;
    std::array<int, 4> source;
    std::array<int, 2> destination;
};

struct DrawCase {
    world::ScarecrowPiece state;
    DrawExpectation result;
};

const std::array<world::ScarecrowPiece, 8> initial_cases = {{world::ScarecrowPiece{16, 1950.0F, 220.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0},
    world::ScarecrowPiece{17, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
    world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
    world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
    world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
    world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
    world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
    world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}}};

const auto updates_cases = [] {
    std::vector<UpdateCase> cases;
    // Captured offsets and recoil Y words, including x87 truncation at 640/1280.
    constexpr std::array cameras{0.0F, 0.25F, 640.0F, 1280.0F};
    constexpr std::array offsets{0, 0, 895, 1791};
    const std::array recoil_y{binary32(0x43626666U), binary32(0x435f6666U), binary32(0x435f6666U), binary32(0x43736666U), binary32(0x43736666U), binary32(0x43606666U),
        binary32(0x434e6666U), binary32(0x433e6666U)};
    for (std::size_t piece = 0; piece < initial_cases.size(); ++piece) {
        for (std::size_t camera = 0; camera < cameras.size(); ++camera) {
            for (const int recoil : {0, 8}) {
                auto after = initial_cases[piece];
                after.screen_x = after.world_x - offsets[camera];
                after.screen_y = recoil ? recoil_y[piece] : after.world_y;
                cases.push_back({"initial-slot" + std::to_string(piece + 6) + "-camera" + std::to_string(cameras[camera]) + "-recoil" + std::to_string(recoil),
                    initial_cases[piece], 0, cameras[camera], recoil, after});
            }
        }
    }
    const std::array boundaries{
        UpdateCase{"fall-frame0-vx0-vy0-dt0", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1944.0F, 217.0F, 1049.0F, binary32(0x435f6666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2feb8U), binary32(0x4358f9a4U), binary32(0x44831eb8U), binary32(0x435f600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f27e14U), binary32(0x4358199aU), binary32(0x44829e14U), binary32(0x435e8000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f05000U), binary32(0x43748000U), binary32(0x44807000U), binary32(0x437ae666U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx0-vy0-dt2", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1840.0F, 807.0F, 945.0F, binary32(0x444b599aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx19-vy19-dt0", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1944.0F, 217.0F, 1049.0F, binary32(0x435f6666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx19-vy19-dt0.001",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f30127U), binary32(0x4358e152U), binary32(0x44832127U), binary32(0x435f47b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f3751fU), binary32(0x434e999aU), binary32(0x4483951fU), 213.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f57000U), 197.0F, binary32(0x44859000U), binary32(0x434b6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx19-vy19-dt2", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 2040.0F, 617.0F, 1145.0F, binary32(0x441bd99aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx10-vy0-dt0", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1944.0F, 217.0F, 1049.0F, binary32(0x435f6666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1944.0F, binary32(0x4358f9a4U), 1049.0F, binary32(0x435f600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2fe14U), binary32(0x4358199aU), binary32(0x44831e14U), binary32(0x435e8000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2d000U), binary32(0x43748000U), binary32(0x4482f000U), binary32(0x437ae666U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx10-vy0-dt2", world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1920.0F, 807.0F, 1025.0F, binary32(0x444b599aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx32767-vy32767-dt0",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1944.0F, 217.0F, 1049.0F, binary32(0x435f6666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2ff9eU), binary32(0x4358f0aeU), binary32(0x44831f9eU), binary32(0x435f5714U), binary32(0xbdf5e204U), binary32(0xbf1930beU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx32767-vy32767-dt0.1",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}, 0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2d7aeU), binary32(0x4354999aU), binary32(0x4482f7aeU), 219.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx32767-vy32767-dt0.5",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f21000U), 227.0F, binary32(0x44823000U), binary32(0x43696666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx32767-vy32767-dt2",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1896.0F, 737.0F, 1001.0F, binary32(0x4439d99aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx12345-vy5678-dt0",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1944.0F, 217.0F, 1049.0F, binary32(0x435f6666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx12345-vy5678-dt0.001",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2ff5cU), binary32(0x4358e29aU), binary32(0x44831f5cU), binary32(0x435f4900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx12345-vy5678-dt0.1",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}, 0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2be14U), binary32(0x434f199aU), binary32(0x4482de14U), binary32(0x43558000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx12345-vy5678-dt0.5",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f19000U), binary32(0x43478000U), binary32(0x4481b000U), binary32(0x434de666U), binary32(0xbe6b851eU), binary32(0xbeb33332U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame0-vx12345-vy5678-dt2",
            world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1880.0F, binary32(0x441cc001U), 985.0F, binary32(0x441e599bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"fall-frame1-vx0-vy0-dt0", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1970.0F, 217.0F, 1075.0F, binary32(0x435f6666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f63eb8U), binary32(0x4358f9a4U), binary32(0x44865eb8U), binary32(0x435f600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5be14U), binary32(0x4358199aU), binary32(0x4485de14U), binary32(0x435e8000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f39000U), binary32(0x43748000U), binary32(0x4483b000U), binary32(0x437ae666U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx0-vy0-dt2", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 1866.0F, 807.0F, 971.0F, binary32(0x444b599aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx19-vy19-dt0", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1970.0F, 217.0F, 1075.0F, binary32(0x435f6666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx19-vy19-dt0.001", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f64127U), binary32(0x4358e152U), binary32(0x44866127U), binary32(0x435f47b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f6b51fU), binary32(0x434e999aU), binary32(0x4486d51fU), 213.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f8b000U), 197.0F, binary32(0x4488d000U), binary32(0x434b6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx19-vy19-dt2", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 2066.0F, 617.0F, 1171.0F, binary32(0x441bd99aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx10-vy0-dt0", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1970.0F, 217.0F, 1075.0F, binary32(0x435f6666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1970.0F, binary32(0x4358f9a4U), 1075.0F, binary32(0x435f600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 0.10000000149011612F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f63e14U), binary32(0x4358199aU), binary32(0x44865e14U), binary32(0x435e8000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f61000U), binary32(0x43748000U), binary32(0x44863000U), binary32(0x437ae666U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx10-vy0-dt2", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1946.0F, 807.0F, 1051.0F, binary32(0x444b599aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx32767-vy32767-dt0", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1970.0F, 217.0F, 1075.0F, binary32(0x435f6666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f63f9eU), binary32(0x4358f0aeU), binary32(0x44865f9eU), binary32(0x435f5714U), binary32(0xbdf5e204U), binary32(0xbf1930beU),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx32767-vy32767-dt0.1", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f617aeU), binary32(0x4354999aU), binary32(0x448637aeU), 219.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx32767-vy32767-dt0.5", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f55000U), 227.0F, binary32(0x44857000U), binary32(0x43696666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx32767-vy32767-dt2", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1922.0F, 737.0F, 1027.0F, binary32(0x4439d99aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx12345-vy5678-dt0", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1970.0F, 217.0F, 1075.0F, binary32(0x435f6666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx12345-vy5678-dt0.001", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f63f5cU), binary32(0x4358e29aU), binary32(0x44865f5cU), binary32(0x435f4900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx12345-vy5678-dt0.1", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5fe14U), binary32(0x434f199aU), binary32(0x44861e14U), binary32(0x43558000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx12345-vy5678-dt0.5", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4d000U), binary32(0x43478000U), binary32(0x4484f000U), binary32(0x434de666U), binary32(0xbe6b851eU), binary32(0xbeb33332U),
                0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame1-vx12345-vy5678-dt2", world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10},
            2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1906.0F, binary32(0x441cc001U), 1011.0F, binary32(0x441e599bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 1, 10}},
        UpdateCase{"fall-frame2-vx0-vy0-dt0", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1949.0F, 237.0F, 1054.0F, binary32(0x43736666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f39eb8U), binary32(0x436cf9a4U), binary32(0x4483beb8U), binary32(0x4373600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f31e14U), binary32(0x436c199aU), binary32(0x44833e14U), binary32(0x43728000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f0f000U), binary32(0x43844000U), binary32(0x44811000U), binary32(0x43877333U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx0-vy0-dt2", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 1845.0F, 827.0F, 950.0F, binary32(0x4450599aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx19-vy19-dt0", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1949.0F, 237.0F, 1054.0F, binary32(0x43736666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx19-vy19-dt0.001", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f3a127U), binary32(0x436ce152U), binary32(0x4483c127U), binary32(0x437347b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f4151fU), binary32(0x4362999aU), binary32(0x4484351fU), 233.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f61000U), 217.0F, binary32(0x44863000U), binary32(0x435f6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx19-vy19-dt2", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 2045.0F, 637.0F, 1150.0F, binary32(0x4420d99aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx10-vy0-dt0", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1949.0F, 237.0F, 1054.0F, binary32(0x43736666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1949.0F, binary32(0x436cf9a4U), 1054.0F, binary32(0x4373600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 0.10000000149011612F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f39e14U), binary32(0x436c199aU), binary32(0x4483be14U), binary32(0x43728000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f37000U), binary32(0x43844000U), binary32(0x44839000U), binary32(0x43877333U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx10-vy0-dt2", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1925.0F, 827.0F, 1030.0F, binary32(0x4450599aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx32767-vy32767-dt0", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1949.0F, 237.0F, 1054.0F, binary32(0x43736666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f39f9eU), binary32(0x436cf0aeU), binary32(0x4483bf9eU), binary32(0x43735714U), binary32(0xbdf5e204U), binary32(0xbf1930beU),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx32767-vy32767-dt0.1", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f377aeU), binary32(0x4368999aU), binary32(0x448397aeU), 239.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx32767-vy32767-dt0.5", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f2b000U), 247.0F, binary32(0x4482d000U), binary32(0x437d6666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx32767-vy32767-dt2", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1901.0F, 757.0F, 1006.0F, binary32(0x443ed99aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx12345-vy5678-dt0", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1949.0F, 237.0F, 1054.0F, binary32(0x43736666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx12345-vy5678-dt0.001", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f39f5cU), binary32(0x436ce29aU), binary32(0x4483bf5cU), binary32(0x43734900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx12345-vy5678-dt0.1", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f35e14U), binary32(0x4363199aU), binary32(0x44837e14U), binary32(0x43698000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx12345-vy5678-dt0.5", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f23000U), binary32(0x435b8000U), binary32(0x44825000U), binary32(0x4361e666U), binary32(0xbe6b851eU), binary32(0xbeb33332U),
                0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame2-vx12345-vy5678-dt2", world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15},
            2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1885.0F, binary32(0x4421c001U), 990.0F, binary32(0x4423599bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 2, 15}},
        UpdateCase{"fall-frame3-vx0-vy0-dt0", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1966.0F, 237.0F, 1071.0F, binary32(0x43736666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5beb8U), binary32(0x436cf9a4U), binary32(0x4485deb8U), binary32(0x4373600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f53e14U), binary32(0x436c199aU), binary32(0x44855e14U), binary32(0x43728000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f31000U), binary32(0x43844000U), binary32(0x44833000U), binary32(0x43877333U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx0-vy0-dt2", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 1862.0F, 827.0F, 967.0F, binary32(0x4450599aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx19-vy19-dt0", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1966.0F, 237.0F, 1071.0F, binary32(0x43736666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx19-vy19-dt0.001", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5c127U), binary32(0x436ce152U), binary32(0x4485e127U), binary32(0x437347b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f6351fU), binary32(0x4362999aU), binary32(0x4486551fU), 233.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f83000U), 217.0F, binary32(0x44885000U), binary32(0x435f6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx19-vy19-dt2", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 2062.0F, 637.0F, 1167.0F, binary32(0x4420d99aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx10-vy0-dt0", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1966.0F, 237.0F, 1071.0F, binary32(0x43736666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1966.0F, binary32(0x436cf9a4U), 1071.0F, binary32(0x4373600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 0.10000000149011612F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5be14U), binary32(0x436c199aU), binary32(0x4485de14U), binary32(0x43728000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f59000U), binary32(0x43844000U), binary32(0x4485b000U), binary32(0x43877333U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx10-vy0-dt2", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1942.0F, 827.0F, 1047.0F, binary32(0x4450599aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx32767-vy32767-dt0", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1966.0F, 237.0F, 1071.0F, binary32(0x43736666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5bf9eU), binary32(0x436cf0aeU), binary32(0x4485df9eU), binary32(0x43735714U), binary32(0xbdf5e204U), binary32(0xbf1930beU),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx32767-vy32767-dt0.1", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f597aeU), binary32(0x4368999aU), binary32(0x4485b7aeU), 239.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx32767-vy32767-dt0.5", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f4d000U), 247.0F, binary32(0x4484f000U), binary32(0x437d6666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx32767-vy32767-dt2", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1918.0F, 757.0F, 1023.0F, binary32(0x443ed99aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx12345-vy5678-dt0", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1966.0F, 237.0F, 1071.0F, binary32(0x43736666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx12345-vy5678-dt0.001", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f5bf5cU), binary32(0x436ce29aU), binary32(0x4485df5cU), binary32(0x43734900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx12345-vy5678-dt0.1", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f57e14U), binary32(0x4363199aU), binary32(0x44859e14U), binary32(0x43698000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx12345-vy5678-dt0.5", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f45000U), binary32(0x435b8000U), binary32(0x44847000U), binary32(0x4361e666U), binary32(0xbe6b851eU), binary32(0xbeb33332U),
                0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame3-vx12345-vy5678-dt2", world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20},
            2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1902.0F, binary32(0x4421c001U), 1007.0F, binary32(0x4423599bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 3, 20}},
        UpdateCase{"fall-frame4-vx0-vy0-dt0", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1957.0F, 218.0F, 1062.0F, binary32(0x43606666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f49eb8U), binary32(0x4359f9a4U), binary32(0x4484beb8U), binary32(0x4360600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f41e14U), binary32(0x4359199aU), binary32(0x44843e14U), binary32(0x435f8000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f1f000U), binary32(0x43758000U), binary32(0x44821000U), binary32(0x437be666U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx0-vy0-dt2", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 1853.0F, 808.0F, 958.0F, binary32(0x444b999aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx19-vy19-dt0", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1957.0F, 218.0F, 1062.0F, binary32(0x43606666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx19-vy19-dt0.001", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4a127U), binary32(0x4359e152U), binary32(0x4484c127U), binary32(0x436047b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f5151fU), binary32(0x434f999aU), binary32(0x4485351fU), 214.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f71000U), 198.0F, binary32(0x44873000U), binary32(0x434c6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx19-vy19-dt2", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 2053.0F, 618.0F, 1158.0F, binary32(0x441c199aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx10-vy0-dt0", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1957.0F, 218.0F, 1062.0F, binary32(0x43606666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1957.0F, binary32(0x4359f9a4U), 1062.0F, binary32(0x4360600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 0.10000000149011612F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f49e14U), binary32(0x4359199aU), binary32(0x4484be14U), binary32(0x435f8000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f47000U), binary32(0x43758000U), binary32(0x44849000U), binary32(0x437be666U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx10-vy0-dt2", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1933.0F, 808.0F, 1038.0F, binary32(0x444b999aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx32767-vy32767-dt0", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1957.0F, 218.0F, 1062.0F, binary32(0x43606666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f49f9eU), binary32(0x4359f0aeU), binary32(0x4484bf9eU), binary32(0x43605714U), binary32(0xbdf5e204U), binary32(0xbf1930beU),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx32767-vy32767-dt0.1", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f477aeU), binary32(0x4355999aU), binary32(0x448497aeU), 220.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx32767-vy32767-dt0.5", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f3b000U), 228.0F, binary32(0x4483d000U), binary32(0x436a6666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx32767-vy32767-dt2", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1909.0F, 738.0F, 1014.0F, binary32(0x443a199aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx12345-vy5678-dt0", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1957.0F, 218.0F, 1062.0F, binary32(0x43606666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx12345-vy5678-dt0.001", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f49f5cU), binary32(0x4359e29aU), binary32(0x4484bf5cU), binary32(0x43604900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx12345-vy5678-dt0.1", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f45e14U), binary32(0x4350199aU), binary32(0x44847e14U), binary32(0x43568000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx12345-vy5678-dt0.5", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f33000U), binary32(0x43488000U), binary32(0x44835000U), binary32(0x434ee666U), binary32(0xbe6b851eU), binary32(0xbeb33332U),
                0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame4-vx12345-vy5678-dt2", world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25},
            2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1893.0F, binary32(0x441d0001U), 998.0F, binary32(0x441e999bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 4, 25}},
        UpdateCase{"fall-frame5-vx0-vy0-dt0", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 200.0F, 1063.0F, binary32(0x434e6666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4beb8U), binary32(0x4347f9a4U), binary32(0x4484deb8U), binary32(0x434e600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f43e14U), binary32(0x4347199aU), binary32(0x44845e14U), binary32(0x434d8000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f21000U), binary32(0x43638000U), binary32(0x44823000U), binary32(0x4369e666U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx0-vy0-dt2", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 1854.0F, 790.0F, 959.0F, binary32(0x4447199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx19-vy19-dt0", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 200.0F, 1063.0F, binary32(0x434e6666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx19-vy19-dt0.001", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4c127U), binary32(0x4347e152U), binary32(0x4484e127U), binary32(0x434e47b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f5351fU), binary32(0x433d999aU), binary32(0x4485551fU), 196.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f73000U), 180.0F, binary32(0x44875000U), binary32(0x433a6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx19-vy19-dt2", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 2054.0F, 600.0F, 1159.0F, binary32(0x4417999aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx10-vy0-dt0", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1958.0F, 200.0F, 1063.0F, binary32(0x434e6666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1958.0F, binary32(0x4347f9a4U), 1063.0F, binary32(0x434e600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 0.10000000149011612F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4be14U), binary32(0x4347199aU), binary32(0x4484de14U), binary32(0x434d8000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f49000U), binary32(0x43638000U), binary32(0x4484b000U), binary32(0x4369e666U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx10-vy0-dt2", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1934.0F, 790.0F, 1039.0F, binary32(0x4447199aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx32767-vy32767-dt0", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 200.0F, 1063.0F, binary32(0x434e6666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4bf9eU), binary32(0x4347f0aeU), binary32(0x4484df9eU), binary32(0x434e5714U), binary32(0xbdf5e204U), binary32(0xbf1930beU),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx32767-vy32767-dt0.1", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f497aeU), binary32(0x4343999aU), binary32(0x4484b7aeU), 202.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx32767-vy32767-dt0.5", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f3d000U), 210.0F, binary32(0x4483f000U), binary32(0x43586666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx32767-vy32767-dt2", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1910.0F, 720.0F, 1015.0F, binary32(0x4435999aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx12345-vy5678-dt0", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 200.0F, 1063.0F, binary32(0x434e6666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx12345-vy5678-dt0.001", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4bf5cU), binary32(0x4347e29aU), binary32(0x4484df5cU), binary32(0x434e4900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx12345-vy5678-dt0.1", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f47e14U), binary32(0x433e199aU), binary32(0x44849e14U), binary32(0x43448000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx12345-vy5678-dt0.5", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f35000U), binary32(0x43368000U), binary32(0x44837000U), binary32(0x433ce666U), binary32(0xbe6b851eU), binary32(0xbeb33332U),
                0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame5-vx12345-vy5678-dt2", world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30},
            2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1894.0F, binary32(0x44188001U), 999.0F, binary32(0x441a199bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 5, 30}},
        UpdateCase{"fall-frame6-vx0-vy0-dt0", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 184.0F, 1063.0F, binary32(0x433e6666U), binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx0-vy0-dt0.001", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4beb8U), binary32(0x4337f9a4U), binary32(0x4484deb8U), binary32(0x433e600aU), binary32(0xbeccd4a9U), binary32(0xbe7e5c92U),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx0-vy0-dt0.1", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f43e14U), binary32(0x4337199aU), binary32(0x44845e14U), binary32(0x433d8000U), binary32(0xbecfdf3bU), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx0-vy0-dt0.5", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f21000U), binary32(0x43538000U), binary32(0x44823000U), binary32(0x4359e666U), binary32(0xbedc28f5U), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx0-vy0-dt2", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx19-vy19-dt0", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50}, 0.0F,
            640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 184.0F, 1063.0F, binary32(0x433e6666U), binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx19-vy19-dt0.001", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4c127U), binary32(0x4337e152U), binary32(0x4484e127U), binary32(0x433e47b8U), binary32(0x3eb859c8U), binary32(0xbf99652cU),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx19-vy19-dt0.1", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f5351fU), binary32(0x432d999aU), binary32(0x4485551fU), 180.0F, binary32(0x3ebb645aU), binary32(0xbf851eb9U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx19-vy19-dt0.5", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50}, 0.5F,
            640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f73000U), 164.0F, binary32(0x44875000U), binary32(0x432a6666U), binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx19-vy19-dt2", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50}, 2.0F,
            640.0F, 8, world::ScarecrowPiece{500, 2054.0F, 584.0F, 1159.0F, binary32(0x4413999aU), binary32(0x3ef5c28fU), 2.0F, 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx10-vy0-dt0", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 0.0F, 640.0F, 8,
            world::ScarecrowPiece{57, 1958.0F, 184.0F, 1063.0F, binary32(0x433e6666U), 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx10-vy0-dt0.001", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, 1958.0F, binary32(0x4337f9a4U), 1063.0F, binary32(0x433e600aU), binary32(0xb87ba883U), binary32(0xbe7e5c92U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx10-vy0-dt0.1", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 0.10000000149011612F,
            640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4be14U), binary32(0x4337199aU), binary32(0x4484de14U), binary32(0x433d8000U), binary32(0xbbc49ba6U), binary32(0xbdb851ebU),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx10-vy0-dt0.5", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f49000U), binary32(0x43538000U), binary32(0x4484b000U), binary32(0x4359e666U), binary32(0xbcf5c28fU), binary32(0x3f0ccccdU),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx10-vy0-dt2", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}, 2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1934.0F, 774.0F, 1039.0F, binary32(0x4443199aU), binary32(0xbdf5c28fU), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx32767-vy32767-dt0", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 184.0F, 1063.0F, binary32(0x433e6666U), binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx32767-vy32767-dt0.001",
            world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50}, 0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4bf9eU), binary32(0x4337f0aeU), binary32(0x4484df9eU), binary32(0x433e5714U), binary32(0xbdf5e204U), binary32(0xbf1930beU),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx32767-vy32767-dt0.1", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f497aeU), binary32(0x4333999aU), binary32(0x4484b7aeU), 186.0F, binary32(0xbe010625U), binary32(0xbee147afU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx32767-vy32767-dt0.5", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{
                57, binary32(0x44f3d000U), 194.0F, binary32(0x4483f000U), binary32(0x43486666U), binary32(0xbe199999U), binary32(0x3e4cccccU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx32767-vy32767-dt2", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50},
            2.0F, 640.0F, 8, world::ScarecrowPiece{500, 1910.0F, 704.0F, 1015.0F, binary32(0x4431999aU), binary32(0xbe75c28fU), binary32(0x40266666U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx12345-vy5678-dt0", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50},
            0.0F, 640.0F, 8, world::ScarecrowPiece{57, 1958.0F, 184.0F, 1063.0F, binary32(0x433e6666U), binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx12345-vy5678-dt0.001", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50},
            0.0010000000474974513F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f4bf5cU), binary32(0x4337e29aU), binary32(0x4484df5cU), binary32(0x433e4900U), binary32(0xbe4cdc87U), binary32(0xbf92fec5U),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx12345-vy5678-dt0.1", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50},
            0.10000000149011612F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f47e14U), binary32(0x432e199aU), binary32(0x44849e14U), binary32(0x43348000U), binary32(0xbe52f1a9U), binary32(0xbf7d70a3U),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx12345-vy5678-dt0.5", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50},
            0.5F, 640.0F, 8,
            world::ScarecrowPiece{57, binary32(0x44f35000U), binary32(0x43268000U), binary32(0x44837000U), binary32(0x432ce666U), binary32(0xbe6b851eU), binary32(0xbeb33332U),
                0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"fall-frame6-vx12345-vy5678-dt2", world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50},
            2.0F, 640.0F, 8,
            world::ScarecrowPiece{500, 1894.0F, binary32(0x44148001U), 999.0F, binary32(0x4416199bU), binary32(0xbea3d70aU), binary32(0x40033334U), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"ground-y480-vy0-dt0", world::ScarecrowPiece{57, 1944.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}, 0.0F, 0.0F, 0,
            world::ScarecrowPiece{57, 1944.0F, 480.0F, 1944.0F, 480.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"ground-y480-vy0-dt5.960464477539063e-08", world::ScarecrowPiece{57, 1944.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
            5.960464477539063e-08F, 0.0F, 0, world::ScarecrowPiece{57, 1944.0F, 480.0F, 1944.0F, 480.0F, binary32(0xb175c28fU), binary32(0x33cccccdU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"ground-y480-vy0.1-dt0.1", world::ScarecrowPiece{57, 1944.0F, 480.0F, -1024.0F, -1024.0F, 0.0F, binary32(0x3dcccccdU), 0.0F, 0.0F, 1, 0, 5},
            0.10000000149011612F, 0.0F, 0,
            world::ScarecrowPiece{500, binary32(0x44f2fe14U), binary32(0x43f14ccdU), binary32(0x44f2fe14U), binary32(0x43f14ccdU), binary32(0xbbc49ba6U), binary32(0x3e851eb9U),
                0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"ground-y479.9999694824219-vy0-dt0.0001", world::ScarecrowPiece{57, 1944.0F, binary32(0x43efffffU), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
            9.999999747378752e-05F, 0.0F, 0,
            world::ScarecrowPiece{57, 1944.0F, binary32(0x43efffffU), 1944.0F, binary32(0x43efffffU), binary32(0xb6c9539bU), binary32(0x3927c5acU), 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"ground-y480.0000305175781-vy0-dt0", world::ScarecrowPiece{57, 1944.0F, binary32(0x43f00001U), -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}, 0.0F, 0.0F,
            0, world::ScarecrowPiece{500, 1944.0F, binary32(0x43f00001U), 1944.0F, binary32(0x43f00001U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"ground-y479-vy0-dt0.1", world::ScarecrowPiece{57, 1944.0F, 479.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5}, 0.10000000149011612F, 0.0F, 0,
            world::ScarecrowPiece{500, binary32(0x44f2fe14U), binary32(0x43f04ccdU), binary32(0x44f2fe14U), binary32(0x43f04ccdU), binary32(0xbbc49ba6U), binary32(0x3e23d70bU),
                0.0F, 0.0F, 1, 0, 5}},
        UpdateCase{"inert-camera0-dt0",
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}, 0.0F, 0.0F, 0,
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"inert-camera0-dt100",
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}, 100.0F, 0.0F, 0,
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"inert-camera1280-dt0",
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}, 0.0F, 1280.0F, 0,
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}},
        UpdateCase{"inert-camera1280-dt100",
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}, 100.0F, 1280.0F, 0,
            world::ScarecrowPiece{500, 1854.0F, 774.0F, 959.0F, binary32(0x4443199aU), binary32(0xbf051eb8U), binary32(0x403ccccdU), 0.0F, 0.0F, 1, 6, 50}}};
    cases.insert(cases.end(), boundaries.begin(), boundaries.end());
    return cases;
}();

const std::array<ImpactCase, 39> impacts_cases = {{ImpactCase{"frame0-draws0-0", world::ScarecrowPiece{17, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
                                                       world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
                                                       ImpactExpectation{false, 1950.0F, 233.0F, 5}, {{0U, 0U}},
                                                       world::ScarecrowPiece{
                                                           57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}},
    ImpactCase{"frame0-draws19-19", world::ScarecrowPiece{17, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10}, ImpactExpectation{false, 1950.0F, 233.0F, 5}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 0, 5}},
    ImpactCase{"frame0-draws10-0", world::ScarecrowPiece{17, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10}, ImpactExpectation{false, 1950.0F, 233.0F, 5}, {{10U, 0U}},
        world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 0, 5}},
    ImpactCase{"frame0-draws32767-32767", world::ScarecrowPiece{17, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10}, ImpactExpectation{false, 1950.0F, 233.0F, 5}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 0, 5}},
    ImpactCase{"frame0-draws12345-5678", world::ScarecrowPiece{17, 1944.0F, 217.0F, -1024.0F, -1024.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10}, ImpactExpectation{false, 1950.0F, 233.0F, 5}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1944.0F, 217.0F, -1024.0F, -1024.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 0, 5}},
    ImpactCase{"frame1-draws0-0", world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15}, ImpactExpectation{false, 1966.0F, 233.0F, 10}, {{0U, 0U}},
        world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}},
    ImpactCase{"frame1-draws19-19", world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15}, ImpactExpectation{false, 1966.0F, 233.0F, 10}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 1, 10}},
    ImpactCase{"frame1-draws10-0", world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15}, ImpactExpectation{false, 1966.0F, 233.0F, 10}, {{10U, 0U}},
        world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 1, 10}},
    ImpactCase{"frame1-draws32767-32767", world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15}, ImpactExpectation{false, 1966.0F, 233.0F, 10}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 1, 10}},
    ImpactCase{"frame1-draws12345-5678", world::ScarecrowPiece{17, 1970.0F, 217.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15}, ImpactExpectation{false, 1966.0F, 233.0F, 10}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1970.0F, 217.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 1, 10}},
    ImpactCase{"frame2-draws0-0", world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20}, ImpactExpectation{false, 1945.0F, 253.0F, 15}, {{0U, 0U}},
        world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}},
    ImpactCase{"frame2-draws19-19", world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20}, ImpactExpectation{false, 1945.0F, 253.0F, 15}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 2, 15}},
    ImpactCase{"frame2-draws10-0", world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20}, ImpactExpectation{false, 1945.0F, 253.0F, 15}, {{10U, 0U}},
        world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 2, 15}},
    ImpactCase{"frame2-draws32767-32767", world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20}, ImpactExpectation{false, 1945.0F, 253.0F, 15}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 2, 15}},
    ImpactCase{"frame2-draws12345-5678", world::ScarecrowPiece{17, 1949.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20}, ImpactExpectation{false, 1945.0F, 253.0F, 15}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1949.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 2, 15}},
    ImpactCase{"frame3-draws0-0", world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25}, ImpactExpectation{false, 1962.0F, 253.0F, 20}, {{0U, 0U}},
        world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}},
    ImpactCase{"frame3-draws19-19", world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25}, ImpactExpectation{false, 1962.0F, 253.0F, 20}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 3, 20}},
    ImpactCase{"frame3-draws10-0", world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25}, ImpactExpectation{false, 1962.0F, 253.0F, 20}, {{10U, 0U}},
        world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 3, 20}},
    ImpactCase{"frame3-draws32767-32767", world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25}, ImpactExpectation{false, 1962.0F, 253.0F, 20}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 3, 20}},
    ImpactCase{"frame3-draws12345-5678", world::ScarecrowPiece{17, 1966.0F, 237.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25}, ImpactExpectation{false, 1962.0F, 253.0F, 20}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1966.0F, 237.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 3, 20}},
    ImpactCase{"frame4-draws0-0", world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, ImpactExpectation{false, 1953.0F, 234.0F, 25}, {{0U, 0U}},
        world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}},
    ImpactCase{"frame4-draws19-19", world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, ImpactExpectation{false, 1953.0F, 234.0F, 25}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 4, 25}},
    ImpactCase{"frame4-draws10-0", world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, ImpactExpectation{false, 1953.0F, 234.0F, 25}, {{10U, 0U}},
        world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 4, 25}},
    ImpactCase{"frame4-draws32767-32767", world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, ImpactExpectation{false, 1953.0F, 234.0F, 25}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 4, 25}},
    ImpactCase{"frame4-draws12345-5678", world::ScarecrowPiece{17, 1957.0F, 218.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, ImpactExpectation{false, 1953.0F, 234.0F, 25}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1957.0F, 218.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 4, 25}},
    ImpactCase{"frame5-draws0-0", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{true, 1954.0F, 216.0F, 30}, {{0U, 0U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-draws19-19", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{true, 1954.0F, 216.0F, 30}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-draws10-0", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{true, 1954.0F, 216.0F, 30}, {{10U, 0U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-draws32767-32767", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{true, 1954.0F, 216.0F, 30}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-draws12345-5678", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{true, 1954.0F, 216.0F, 30}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame6-draws0-0", world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, std::nullopt,
        ImpactExpectation{false, 1954.0F, 200.0F, 50}, {{0U, 0U}},
        world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeccccccU), binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}},
    ImpactCase{"frame6-draws19-19", world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, std::nullopt,
        ImpactExpectation{false, 1954.0F, 200.0F, 50}, {{19U, 19U}},
        world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0x3eb851ebU), binary32(0xbf99999aU), 0.0F, 0.0F, 1, 6, 50}},
    ImpactCase{"frame6-draws10-0", world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, std::nullopt,
        ImpactExpectation{false, 1954.0F, 200.0F, 50}, {{10U, 0U}}, world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, binary32(0xbe800000U), 0.0F, 0.0F, 1, 6, 50}},
    ImpactCase{"frame6-draws32767-32767", world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, std::nullopt,
        ImpactExpectation{false, 1954.0F, 200.0F, 50}, {{32767U, 32767U}},
        world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbdf5c28fU), binary32(0xbf19999aU), 0.0F, 0.0F, 1, 6, 50}},
    ImpactCase{"frame6-draws12345-5678", world::ScarecrowPiece{17, 1958.0F, 184.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50}, std::nullopt,
        ImpactExpectation{false, 1954.0F, 200.0F, 50}, {{12345U, 5678U}},
        world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbe4cccccU), binary32(0xbf933333U), 0.0F, 0.0F, 1, 6, 50}},
    ImpactCase{"frame5-adjacent-none", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, std::nullopt,
        ImpactExpectation{false, 1954.0F, 216.0F, 30}, {{11U, 12U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3d23d70aU), binary32(0xbf59999aU), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-adjacent-falling", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{57, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeb851ebU), binary32(0xbeb33333U), 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{false, 1954.0F, 216.0F, 30},
        {{11U, 12U}}, world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3d23d70aU), binary32(0xbf59999aU), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-adjacent-inert", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{500, 1958.0F, 184.0F, 0.0F, 0.0F, binary32(0xbeb851ebU), binary32(0xbeb33333U), 0.0F, 0.0F, 1, 6, 50}, ImpactExpectation{false, 1954.0F, 216.0F, 30},
        {{11U, 12U}}, world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3d23d70aU), binary32(0xbf59999aU), 0.0F, 0.0F, 1, 5, 30}},
    ImpactCase{"frame5-adjacent-wrongframe", world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        world::ScarecrowPiece{17, 1958.0F, 200.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30}, ImpactExpectation{false, 1954.0F, 216.0F, 30}, {{11U, 12U}},
        world::ScarecrowPiece{57, 1958.0F, 200.0F, 0.0F, 0.0F, binary32(0x3d23d70aU), binary32(0xbf59999aU), 0.0F, 0.0F, 1, 5, 30}}}};

const auto records_cases = [] {
    std::array<RecordCase, 126> cases{};
    std::size_t index = 0;
    for (int hits = 0; hits <= 8; ++hits) {
        for (const bool ordered : {false, true}) {
            for (int frame = 0; frame <= 6; ++frame) {
                cases[index++] = {hits, ordered, frame, hits + 1, ordered && hits == frame};
            }
        }
    }
    return cases;
}();

const auto completions_cases = [] {
    std::array<CompletionCase, 40> cases{};
    std::size_t index = 0;
    for (const int hits : {0, 1, 2, 3, 4, 5, 6, 7, 8, std::numeric_limits<int>::max()}) {
        for (const bool completed : {false, true}) {
            for (const bool ordered : {false, true}) {
                CompletionExpectation result{};
                if (hits == 7 && !completed) {
                    result = ordered ? CompletionExpectation{24, 0, true} : CompletionExpectation{0, 10, false};
                }
                cases[index++] = {hits, completed, ordered, completed || hits == 7, result};
            }
        }
    }
    return cases;
}();

const std::array<DrawCase, 45> draws_cases = {{DrawCase{world::ScarecrowPiece{16, 1950.0F, 220.0F, 1950.0F, binary32(0x43626666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0},
                                                   DrawExpectation{"gfx/pole", {{0, 0, 48, 48}}, {{1950, 226}}}},
    DrawCase{
        world::ScarecrowPiece{16, 1950.0F, 220.0F, 1055.0F, binary32(0x43626666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, DrawExpectation{"gfx/pole", {{0, 0, 48, 48}}, {{1055, 226}}}},
    DrawCase{
        world::ScarecrowPiece{16, 1950.0F, 220.0F, 159.0F, binary32(0x43626666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 0}, DrawExpectation{"gfx/pole", {{0, 0, 48, 48}}, {{159, 226}}}},
    DrawCase{world::ScarecrowPiece{17, 1944.0F, 217.0F, 1944.0F, binary32(0x435f6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        DrawExpectation{"gfx/scheuche", {{0, 0, 32, 32}}, {{1944, 223}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f57000U), 197.0F, binary32(0x44f57000U), 197.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 0, 5},
        DrawExpectation{"gfx/scheuche", {{0, 0, 32, 32}}, {{1963, 197}}}},
    DrawCase{world::ScarecrowPiece{17, 1944.0F, 217.0F, 1049.0F, binary32(0x435f6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        DrawExpectation{"gfx/scheuche", {{0, 0, 32, 32}}, {{1049, 223}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f57000U), 197.0F, binary32(0x44859000U), 197.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 0, 5},
        DrawExpectation{"gfx/scheuche", {{0, 0, 32, 32}}, {{1068, 197}}}},
    DrawCase{world::ScarecrowPiece{17, 1944.0F, 217.0F, 153.0F, binary32(0x435f6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 0, 5},
        DrawExpectation{"gfx/scheuche", {{0, 0, 32, 32}}, {{153, 223}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f57000U), 197.0F, binary32(0x432c8000U), 197.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 0, 5},
        DrawExpectation{"gfx/scheuche", {{0, 0, 32, 32}}, {{172, 197}}}},
    DrawCase{world::ScarecrowPiece{17, 1970.0F, 217.0F, 1970.0F, binary32(0x435f6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        DrawExpectation{"gfx/scheuche", {{0, 32, 32, 32}}, {{1970, 223}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f8b000U), 197.0F, binary32(0x44f8b000U), 197.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 1, 10},
        DrawExpectation{"gfx/scheuche", {{0, 32, 32, 32}}, {{1989, 197}}}},
    DrawCase{world::ScarecrowPiece{17, 1970.0F, 217.0F, 1075.0F, binary32(0x435f6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        DrawExpectation{"gfx/scheuche", {{0, 32, 32, 32}}, {{1075, 223}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f8b000U), 197.0F, binary32(0x4488d000U), 197.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 1, 10},
        DrawExpectation{"gfx/scheuche", {{0, 32, 32, 32}}, {{1094, 197}}}},
    DrawCase{world::ScarecrowPiece{17, 1970.0F, 217.0F, 179.0F, binary32(0x435f6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 1, 10},
        DrawExpectation{"gfx/scheuche", {{0, 32, 32, 32}}, {{179, 223}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f8b000U), 197.0F, binary32(0x43468000U), 197.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 1, 10},
        DrawExpectation{"gfx/scheuche", {{0, 32, 32, 32}}, {{198, 197}}}},
    DrawCase{world::ScarecrowPiece{17, 1949.0F, 237.0F, 1949.0F, binary32(0x43736666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        DrawExpectation{"gfx/scheuche", {{0, 64, 32, 32}}, {{1949, 243}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f61000U), 217.0F, binary32(0x44f61000U), 217.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 2, 15},
        DrawExpectation{"gfx/scheuche", {{0, 64, 32, 32}}, {{1968, 217}}}},
    DrawCase{world::ScarecrowPiece{17, 1949.0F, 237.0F, 1054.0F, binary32(0x43736666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        DrawExpectation{"gfx/scheuche", {{0, 64, 32, 32}}, {{1054, 243}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f61000U), 217.0F, binary32(0x44863000U), 217.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 2, 15},
        DrawExpectation{"gfx/scheuche", {{0, 64, 32, 32}}, {{1073, 217}}}},
    DrawCase{world::ScarecrowPiece{17, 1949.0F, 237.0F, 158.0F, binary32(0x43736666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 2, 15},
        DrawExpectation{"gfx/scheuche", {{0, 64, 32, 32}}, {{158, 243}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f61000U), 217.0F, binary32(0x43318000U), 217.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 2, 15},
        DrawExpectation{"gfx/scheuche", {{0, 64, 32, 32}}, {{177, 217}}}},
    DrawCase{world::ScarecrowPiece{17, 1966.0F, 237.0F, 1966.0F, binary32(0x43736666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        DrawExpectation{"gfx/scheuche", {{0, 96, 32, 32}}, {{1966, 243}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f83000U), 217.0F, binary32(0x44f83000U), 217.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 3, 20},
        DrawExpectation{"gfx/scheuche", {{0, 96, 32, 32}}, {{1985, 217}}}},
    DrawCase{world::ScarecrowPiece{17, 1966.0F, 237.0F, 1071.0F, binary32(0x43736666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        DrawExpectation{"gfx/scheuche", {{0, 96, 32, 32}}, {{1071, 243}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f83000U), 217.0F, binary32(0x44885000U), 217.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 3, 20},
        DrawExpectation{"gfx/scheuche", {{0, 96, 32, 32}}, {{1090, 217}}}},
    DrawCase{world::ScarecrowPiece{17, 1966.0F, 237.0F, 175.0F, binary32(0x43736666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 3, 20},
        DrawExpectation{"gfx/scheuche", {{0, 96, 32, 32}}, {{175, 243}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f83000U), 217.0F, binary32(0x43428000U), 217.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 3, 20},
        DrawExpectation{"gfx/scheuche", {{0, 96, 32, 32}}, {{194, 217}}}},
    DrawCase{world::ScarecrowPiece{17, 1957.0F, 218.0F, 1957.0F, binary32(0x43606666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        DrawExpectation{"gfx/scheuche", {{0, 128, 32, 32}}, {{1957, 224}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f71000U), 198.0F, binary32(0x44f71000U), 198.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 4, 25},
        DrawExpectation{"gfx/scheuche", {{0, 128, 32, 32}}, {{1976, 198}}}},
    DrawCase{world::ScarecrowPiece{17, 1957.0F, 218.0F, 1062.0F, binary32(0x43606666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        DrawExpectation{"gfx/scheuche", {{0, 128, 32, 32}}, {{1062, 224}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f71000U), 198.0F, binary32(0x44873000U), 198.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 4, 25},
        DrawExpectation{"gfx/scheuche", {{0, 128, 32, 32}}, {{1081, 198}}}},
    DrawCase{world::ScarecrowPiece{17, 1957.0F, 218.0F, 166.0F, binary32(0x43606666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 4, 25},
        DrawExpectation{"gfx/scheuche", {{0, 128, 32, 32}}, {{166, 224}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f71000U), 198.0F, binary32(0x43398000U), 198.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 4, 25},
        DrawExpectation{"gfx/scheuche", {{0, 128, 32, 32}}, {{185, 198}}}},
    DrawCase{world::ScarecrowPiece{17, 1958.0F, 200.0F, 1958.0F, binary32(0x434e6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        DrawExpectation{"gfx/scheuche", {{0, 160, 32, 32}}, {{1958, 206}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f73000U), 180.0F, binary32(0x44f73000U), 180.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 5, 30},
        DrawExpectation{"gfx/scheuche", {{0, 160, 32, 32}}, {{1977, 180}}}},
    DrawCase{world::ScarecrowPiece{17, 1958.0F, 200.0F, 1063.0F, binary32(0x434e6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        DrawExpectation{"gfx/scheuche", {{0, 160, 32, 32}}, {{1063, 206}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f73000U), 180.0F, binary32(0x44875000U), 180.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 5, 30},
        DrawExpectation{"gfx/scheuche", {{0, 160, 32, 32}}, {{1082, 180}}}},
    DrawCase{world::ScarecrowPiece{17, 1958.0F, 200.0F, 167.0F, binary32(0x434e6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 5, 30},
        DrawExpectation{"gfx/scheuche", {{0, 160, 32, 32}}, {{167, 206}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f73000U), 180.0F, binary32(0x433a8000U), 180.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 5, 30},
        DrawExpectation{"gfx/scheuche", {{0, 160, 32, 32}}, {{186, 180}}}},
    DrawCase{world::ScarecrowPiece{17, 1958.0F, 184.0F, 1958.0F, binary32(0x433e6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50},
        DrawExpectation{"gfx/scheuche", {{0, 192, 32, 32}}, {{1958, 190}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f73000U), 164.0F, binary32(0x44f73000U), 164.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 6, 50},
        DrawExpectation{"gfx/scheuche", {{0, 192, 32, 32}}, {{1977, 164}}}},
    DrawCase{world::ScarecrowPiece{17, 1958.0F, 184.0F, 1063.0F, binary32(0x433e6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50},
        DrawExpectation{"gfx/scheuche", {{0, 192, 32, 32}}, {{1063, 190}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f73000U), 164.0F, binary32(0x44875000U), 164.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 6, 50},
        DrawExpectation{"gfx/scheuche", {{0, 192, 32, 32}}, {{1082, 164}}}},
    DrawCase{world::ScarecrowPiece{17, 1958.0F, 184.0F, 167.0F, binary32(0x433e6666U), 0.0F, 0.0F, 0.0F, 0.0F, 1, 6, 50},
        DrawExpectation{"gfx/scheuche", {{0, 192, 32, 32}}, {{167, 190}}}},
    DrawCase{world::ScarecrowPiece{57, binary32(0x44f73000U), 164.0F, binary32(0x433a8000U), 164.0F, binary32(0x3ec7ae14U), binary32(0xbeccccceU), 0.0F, 0.0F, 1, 6, 50},
        DrawExpectation{"gfx/scheuche", {{0, 192, 32, 32}}, {{186, 164}}}}}};

void compare(const world::ScarecrowPiece& actual, const world::ScarecrowPiece& expected, std::string_view context) {
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

void original_vectors() {
    const auto initial = world::initial_scarecrow();
    for (std::size_t index = 0; index < initial.size(); ++index) {
        compare(initial[index], initial_cases.at(index), "Fresh scarecrow storage");
    }
    for (const auto& expected : updates_cases) {
        auto piece = expected.before;
        world::update_scarecrow_piece(piece, expected.delta, expected.camera, expected.recoil);
        compare(piece, expected.after, expected.id);
    }
    for (const auto& expected : impacts_cases) {
        auto piece = expected.before;
        auto adjacent = initial[7];
        const bool has_adjacent = expected.adjacent.has_value();
        if (has_adjacent) {
            adjacent = *expected.adjacent;
        }
        const auto impact = world::prepare_scarecrow_hit(piece, has_adjacent ? &adjacent : nullptr);
        const auto& prepared = expected.prepared;
        require(
            impact.release_hat == prepared.release_hat && impact.popup_x == prepared.popup_x && impact.popup_y == prepared.popup_y && impact.popup_points == prepared.popup_points,
            expected.id + ": source popup/adjacent gate differs");
        world::start_scarecrow_fall(piece, expected.random.at(0), expected.random.at(1));
        compare(piece, expected.after, expected.id);
    }
    for (const auto& expected : records_cases) {
        int hits = expected.hits;
        bool ordered = expected.ordered;
        world::record_scarecrow_frame(hits, ordered, expected.frame);
        require(hits == expected.hits_after && ordered == expected.ordered_after, "Source frame-order latch differs");
    }
    for (const auto& expected : completions_cases) {
        bool completed = expected.completed;
        const auto result = world::apply_scarecrow_completion(expected.hits, completed, expected.ordered);
        require(completed == expected.completed_after && result.balloon_count == expected.result.balloon_count && result.chicken_count == expected.result.chicken_count
                    && result.magic_sound == expected.result.magic_sound,
            "Source completion latch/effects differ");
    }
    for (const auto& expected : draws_cases) {
        const auto actual = world::scarecrow_draw(expected.state);
        const auto& result = expected.result;
        const auto& source = result.source;
        const auto& destination = result.destination;
        require(actual.image == result.image && actual.source.x == source.at(0) && actual.source.y == source.at(1) && actual.source.width == source.at(2)
                    && actual.source.height == source.at(3) && actual.destination.x == destination.at(0) && actual.destination.y == destination.at(1) && actual.color_key
                    && !actual.mirror_x,
            "Source scarecrow/pole art cell differs");
    }
}

using moorhuhn2::test::reject_unchanged;

void invalid_inputs() {
    const auto initial = world::initial_scarecrow();
    for (const float delta : {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        reject_unchanged(
            initial[1],
            [delta](auto& piece) {
                world::update_scarecrow_piece(piece, delta, 0, 0);
            },
            "Invalid scarecrow time");
    }
    for (const float camera : {-1.0F, 1281.0F, std::numeric_limits<float>::infinity()}) {
        reject_unchanged(
            initial[1],
            [camera](auto& piece) {
                world::update_scarecrow_piece(piece, 0, camera, 0);
            },
            "Invalid scarecrow camera");
    }
    for (const int recoil : {-1, 9}) {
        reject_unchanged(
            initial[1],
            [recoil](auto& piece) {
                world::update_scarecrow_piece(piece, 0, 0, recoil);
            },
            "Invalid scarecrow recoil");
    }
    for (const auto random : {std::array<std::uint32_t, 2>{32768, 0}, std::array<std::uint32_t, 2>{0, 32768}}) {
        reject_unchanged(
            initial[1],
            [random](auto& piece) {
                world::start_scarecrow_fall(piece, random[0], random[1]);
            },
            "Invalid scarecrow CRT value");
    }
    reject_unchanged(
        initial[0],
        [](auto& piece) {
            world::start_scarecrow_fall(piece, 0, 0);
        },
        "Permanent pole cannot fall");
    auto falling = initial[1];
    world::start_scarecrow_fall(falling, 0, 0);
    reject_unchanged(
        falling,
        [](auto& piece) {
            world::start_scarecrow_fall(piece, 0, 0);
        },
        "Falling body cannot be hit twice");
    auto invalid = initial[1];
    invalid.points = 6;
    reject_unchanged(
        invalid,
        [](auto& piece) {
            world::update_scarecrow_piece(piece, 0, 0, 0);
        },
        "Unsupported body award");
    auto removed = falling;
    removed.type = 500;
    reject_unchanged(
        removed,
        [](auto& piece) {
            static_cast<void>(world::scarecrow_draw(piece));
        },
        "Inert body cannot draw");
    invalid = initial[1];
    invalid.screen_x = 2147483648.0F;
    reject_unchanged(
        invalid,
        [](auto& piece) {
            static_cast<void>(world::scarecrow_draw(piece));
        },
        "Unrepresentable draw position");
    falling.world_y = std::numeric_limits<float>::max();
    falling.velocity_y = std::numeric_limits<float>::max();
    reject_unchanged(
        falling,
        [](auto& piece) {
            world::update_scarecrow_piece(piece, 100, 0, 0);
        },
        "Overflowing fall storage");
    reject_unchanged(
        -1,
        [](int& hits) {
            bool ordered = true;
            world::record_scarecrow_frame(hits, ordered, 0);
        },
        "Negative hit count");
    reject_unchanged(
        std::numeric_limits<int>::max(),
        [](int& hits) {
            bool ordered = true;
            world::record_scarecrow_frame(hits, ordered, 0);
        },
        "Overflowing hit count");
    reject_unchanged(
        0,
        [](int& hits) {
            bool ordered = true;
            world::record_scarecrow_frame(hits, ordered, 7);
        },
        "Invalid body frame");
}

} // namespace

int main() {
    try {
        original_vectors();
        invalid_inputs();
        std::cout << "Scarecrow/pole source states, hit/order/completion, retained500, cells, binary32 storage and reset passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
