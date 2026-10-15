#include "ui/loading.hpp"

#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

struct LoadMilestone {
    std::string_view asset{};
    int progress{};
};

const std::array<std::string_view, 70> image_order = {{"gfx/loading", "gfx/loadingbar", "gfx/mainscreen", "gfx/scorescreen", "gfx/M2BG01", "gfx/M2BG02", "gfx/M2BG03", "gfx/M2BG04",
    "gfx/M2BG05", "gfx/hochsitz3", "gfx/hochsitz1", "gfx/hochsitz2", "gfx/hochsitz4", "gfx/hochsitzcol", "gfx/boat", "gfx/boatcol", "gfx/flyaway1", "gfx/flyaway3", "gfx/flyaway2",
    "gfx/big_appear", "gfx/big_hit", "gfx/frog", "gfx/Chickleft1", "gfx/Chickleft2", "gfx/Chickleft3", "gfx/Chickright1", "gfx/Chickright2", "gfx/Chickright3", "gfx/DChickleft1",
    "gfx/DChickleft2", "gfx/DChickleft3", "gfx/DChickright1", "gfx/DChickright2", "gfx/DChickright3", "gfx/stone", "gfx/particle", "gfx/particle2", "gfx/pole", "gfx/scheuche",
    "gfx/lplane", "gfx/rplane", "gfx/spinne1", "gfx/spinne2", "gfx/ballon", "gfx/branch1", "gfx/branch2", "gfx/tree1_01", "gfx/tree1_02", "gfx/blimp", "gfx/Leaf", "gfx/Leaf2",
    "gfx/specialhuhn", "gfx/rose", "gfx/highfly", "gfx/church", "gfx/house", "gfx/hide1", "gfx/hide2", "gfx/hide3", "gfx/Witansmall", "gfx/jennie", "gfx/kopf", "gfx/snake",
    "gfx/Bossscreen", "gfx/Making", "gfx/WitanLogo", "gfx/Computer", "gfx/disclaimer", "gfx/mousecursor", "gfx/font"}};

const std::array<std::string_view, 38> source_audio_order = {{"sfx/Moorhuhn2_Title_Music", "sfx/Moorhuhn2_Highscore_Music", "sfx/sfx_background_birds_STEREO_LOOP",
    "sfx/sfx_shotgun_blast", "sfx/sfx_gun_reload", "sfx/sfx_empty_magazine", "sfx/sfx_big_chicken_pops_up", "sfx/chickenhit2", "sfx/sfx_chicken_shot_FAR",
    "sfx/sfx_chicken_shot_MID", "sfx/sfx_chicken_shot_CLOSE", "sfx/sfx_stone_shot", "sfx/sfx_frog", "sfx/sfx_frog_shot", "sfx/sfx_typename_click", "sfx/sfx_balloon_shot",
    "sfx/sfx_airplane_loop", "sfx/sfx_airplane_motor_failure_loop", "sfx/sfx_airplane_shot", "sfx/sfx_airplane_crash", "sfx/sfx_zeppelin_shot", "sfx/sfx_scarecrow_shot",
    "sfx/sfx_tree_shot", "sfx/sfx_treebranch_shot", "sfx/sfx_treebranch_hits_ground", "sfx/sfx_churchbell", "sfx/sfx_churchbell_2", "sfx/sfx_get_ready_after_typing_name",
    "sfx/sfx_game_over", "sfx/sfx_time_running_out", "sfx/sfx_spider_swings", "sfx/sfx_boat_sinking", "sfx/sfx_magic_balloons_appear", "sfx/sfx_50_point_score",
    "sfx/sfx_tree_breaks_down", "sfx/sfx_guardhouse_breaks_down", "sfx/sfx_guardhouse_ladder_shot", "sfx/sfx_house_shot"}};

const std::array<std::string_view, 1> extra_audio_order = {{"sfx/sfx_big_chicken_shot"}};

const std::array<LoadMilestone, 104> calls = {{{"gfx/mainscreen", 1}, {"gfx/scorescreen", 2}, {"gfx/M2BG01", 5}, {"gfx/M2BG02", 5}, {"gfx/M2BG03", 5}, {"gfx/M2BG04", 5},
    {"gfx/M2BG05", 5}, {"gfx/hochsitz3", 6}, {"gfx/hochsitz1", 6}, {"gfx/hochsitz2", 6}, {"gfx/hochsitz4", 6}, {"gfx/hochsitzcol", 6}, {"gfx/boat", 6}, {"gfx/boatcol", 6},
    {"gfx/flyaway1", 6}, {"gfx/flyaway3", 6}, {"gfx/flyaway2", 6}, {"gfx/big_appear", 6}, {"gfx/big_hit", 6}, {"gfx/frog", 6}, {"gfx/Chickleft1", 7}, {"gfx/Chickleft2", 7},
    {"gfx/Chickleft3", 7}, {"gfx/Chickright1", 7}, {"gfx/Chickright2", 7}, {"gfx/Chickright3", 7}, {"gfx/DChickleft1", 8}, {"gfx/DChickleft2", 8}, {"gfx/DChickleft3", 8},
    {"gfx/DChickright1", 8}, {"gfx/DChickright2", 8}, {"gfx/DChickright3", 8}, {"gfx/stone", 9}, {"gfx/particle", 9}, {"gfx/particle2", 9}, {"gfx/pole", 9}, {"gfx/scheuche", 9},
    {"gfx/lplane", 9}, {"gfx/rplane", 9}, {"gfx/spinne1", 9}, {"gfx/spinne2", 9}, {"gfx/ballon", 9}, {"gfx/branch1", 10}, {"gfx/branch2", 10}, {"gfx/tree1_01", 10},
    {"gfx/tree1_02", 10}, {"gfx/blimp", 10}, {"gfx/Leaf", 11}, {"gfx/Leaf2", 11}, {"gfx/specialhuhn", 11}, {"gfx/rose", 11}, {"gfx/highfly", 11}, {"gfx/church", 11},
    {"gfx/house", 11}, {"gfx/hide1", 12}, {"gfx/hide2", 12}, {"gfx/hide3", 12}, {"gfx/Witansmall", 12}, {"gfx/jennie", 12}, {"gfx/kopf", 12}, {"gfx/snake", 12},
    {"gfx/Bossscreen", 12}, {"gfx/Making", 12}, {"gfx/WitanLogo", 12}, {"gfx/Computer", 12}, {"gfx/disclaimer", 12}, {"sfx/Moorhuhn2_Title_Music", 13},
    {"sfx/Moorhuhn2_Highscore_Music", 14}, {"sfx/sfx_background_birds_STEREO_LOOP", 15}, {"sfx/sfx_shotgun_blast", 16}, {"sfx/sfx_gun_reload", 16}, {"sfx/sfx_empty_magazine", 16},
    {"sfx/sfx_big_chicken_pops_up", 16}, {"sfx/chickenhit2", 16}, {"sfx/sfx_chicken_shot_FAR", 16}, {"sfx/sfx_chicken_shot_MID", 16}, {"sfx/sfx_chicken_shot_CLOSE", 16},
    {"sfx/sfx_stone_shot", 16}, {"sfx/sfx_frog", 16}, {"sfx/sfx_frog_shot", 16}, {"sfx/sfx_typename_click", 16}, {"sfx/sfx_balloon_shot", 16}, {"sfx/sfx_airplane_loop", 16},
    {"sfx/sfx_airplane_motor_failure_loop", 17}, {"sfx/sfx_airplane_shot", 17}, {"sfx/sfx_airplane_crash", 17}, {"sfx/sfx_zeppelin_shot", 17}, {"sfx/sfx_scarecrow_shot", 17},
    {"sfx/sfx_tree_shot", 17}, {"sfx/sfx_treebranch_shot", 18}, {"sfx/sfx_treebranch_hits_ground", 18}, {"sfx/sfx_churchbell", 18}, {"sfx/sfx_churchbell_2", 19},
    {"sfx/sfx_get_ready_after_typing_name", 19}, {"sfx/sfx_game_over", 19}, {"sfx/sfx_time_running_out", 19}, {"sfx/sfx_spider_swings", 19}, {"sfx/sfx_boat_sinking", 19},
    {"sfx/sfx_magic_balloons_appear", 19}, {"sfx/sfx_50_point_score", 19}, {"sfx/sfx_tree_breaks_down", 19}, {"sfx/sfx_guardhouse_breaks_down", 19},
    {"sfx/sfx_guardhouse_ladder_shot", 19}, {"sfx/sfx_house_shot", 19}}};

struct DrawExpectation {
    std::string_view image{};
    std::array<int, 4> source{};
    std::array<int, 2> destination{};
    bool color_key{};
    bool mirror_x{};
};

struct DrawCase {
    int progress{};
    std::vector<DrawExpectation> draws{};
};

const std::array<DrawCase, 3> draw_cases = {
    {{1, {{"gfx/loading", {0, 0, 267, 200}, {187, 100}, true, false}, {"gfx/loadingbar", {0, 0, 267, 10}, {187, 100}, true, false},
             {"gfx/loadingbar", {0, 10, 267, 10}, {187, 110}, true, false}, {"gfx/loadingbar", {0, 20, 267, 10}, {187, 120}, true, false},
             {"gfx/loadingbar", {0, 30, 267, 10}, {187, 130}, true, false}, {"gfx/loadingbar", {0, 40, 267, 10}, {187, 140}, true, false},
             {"gfx/loadingbar", {0, 50, 267, 10}, {187, 150}, true, false}, {"gfx/loadingbar", {0, 60, 267, 10}, {187, 160}, true, false},
             {"gfx/loadingbar", {0, 70, 267, 10}, {187, 170}, true, false}, {"gfx/loadingbar", {0, 80, 267, 10}, {187, 180}, true, false},
             {"gfx/loadingbar", {0, 90, 267, 10}, {187, 190}, true, false}, {"gfx/loadingbar", {0, 100, 267, 10}, {187, 200}, true, false},
             {"gfx/loadingbar", {0, 110, 267, 10}, {187, 210}, true, false}, {"gfx/loadingbar", {0, 120, 267, 10}, {187, 220}, true, false},
             {"gfx/loadingbar", {0, 130, 267, 10}, {187, 230}, true, false}, {"gfx/loadingbar", {0, 140, 267, 10}, {187, 240}, true, false},
             {"gfx/loadingbar", {0, 150, 267, 10}, {187, 250}, true, false}, {"gfx/loadingbar", {0, 160, 267, 10}, {187, 260}, true, false},
             {"gfx/loadingbar", {0, 170, 267, 10}, {187, 270}, true, false}, {"gfx/loadingbar", {0, 180, 267, 10}, {187, 280}, true, false}}},
        {2, {{"gfx/loading", {0, 0, 267, 200}, {187, 100}, true, false}, {"gfx/loadingbar", {0, 0, 267, 10}, {187, 100}, true, false},
                {"gfx/loadingbar", {0, 10, 267, 10}, {187, 110}, true, false}, {"gfx/loadingbar", {0, 20, 267, 10}, {187, 120}, true, false},
                {"gfx/loadingbar", {0, 30, 267, 10}, {187, 130}, true, false}, {"gfx/loadingbar", {0, 40, 267, 10}, {187, 140}, true, false},
                {"gfx/loadingbar", {0, 50, 267, 10}, {187, 150}, true, false}, {"gfx/loadingbar", {0, 60, 267, 10}, {187, 160}, true, false},
                {"gfx/loadingbar", {0, 70, 267, 10}, {187, 170}, true, false}, {"gfx/loadingbar", {0, 80, 267, 10}, {187, 180}, true, false},
                {"gfx/loadingbar", {0, 90, 267, 10}, {187, 190}, true, false}, {"gfx/loadingbar", {0, 100, 267, 10}, {187, 200}, true, false},
                {"gfx/loadingbar", {0, 110, 267, 10}, {187, 210}, true, false}, {"gfx/loadingbar", {0, 120, 267, 10}, {187, 220}, true, false},
                {"gfx/loadingbar", {0, 130, 267, 10}, {187, 230}, true, false}, {"gfx/loadingbar", {0, 140, 267, 10}, {187, 240}, true, false},
                {"gfx/loadingbar", {0, 150, 267, 10}, {187, 250}, true, false}, {"gfx/loadingbar", {0, 160, 267, 10}, {187, 260}, true, false},
                {"gfx/loadingbar", {0, 170, 267, 10}, {187, 270}, true, false}}},
        {19, {{"gfx/loading", {0, 0, 267, 200}, {187, 100}, true, false}, {"gfx/loadingbar", {0, 0, 267, 10}, {187, 100}, true, false}}}}};

using moorhuhn2::contracts::SpriteDraw;
namespace ui = moorhuhn2::ui;

using moorhuhn2::test::require;

void check_order(std::span<const std::string_view> actual, std::span<const std::string_view> expected) {
    require(actual.size() == expected.size(), "Original resource order length differs");
    std::set<std::string_view> unique;
    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(actual[index] == expected[index], "Original canonical resource order differs");
        require(unique.insert(actual[index]).second, "A loading resource is repeated");
    }
}

void check_milestones() {
    const auto actual = ui::loading_milestones();
    const auto& expected = calls;
    require(actual.size() == 104 && actual.size() == expected.size(), "All 104 original redraw calls must remain");
    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(
            actual[index].asset == expected[index].asset && actual[index].progress == expected[index].progress, "Resource completion/progress argument differs from source bytes");
        // Every source milestone remains drawable, including repeated arguments.
        require(!ui::loading_draws(actual[index].progress).empty(), "A source progress call cannot be drawn");
    }

    check_order(ui::loading_image_order(), image_order);
    check_order(ui::loading_audio_order(), source_audio_order);
    for (const auto& extra : extra_audio_order) {
        const auto asset = extra;
        require(std::ranges::find(actual, asset, &ui::LoadingMilestone::asset) == actual.end(), "Manifest-only audio must not invent an original redraw milestone");
        require(std::ranges::find(ui::loading_audio_order(), asset) == ui::loading_audio_order().end(), "The original audio order cannot claim a manifest-only load");
    }
}

bool same_draw(const SpriteDraw& actual, const DrawExpectation& expected) {
    const auto& source = expected.source;
    const auto& destination = expected.destination;

    return actual.image == expected.image
           && std::tie(actual.source.x, actual.source.y, actual.source.width, actual.source.height) == std::tuple{source[0], source[1], source[2], source[3]}
           && std::tie(actual.destination.x, actual.destination.y) == std::tuple{destination[0], destination[1]} && actual.color_key == expected.color_key
           && actual.mirror_x == expected.mirror_x;
}

void check_draws() {
    for (const auto& expected : draw_cases) {
        const auto progress = expected.progress;
        const auto actual = ui::loading_draws(progress);
        const auto& draws = expected.draws;
        require(actual.size() == draws.size(), "Source loading overlay count differs");
        for (std::size_t index = 0; index < actual.size(); ++index) {
            require(same_draw(actual[index], draws[index]), "Source loading cells, placement, painter order or keying differ");
        }
    }

    for (const auto invalid : {std::numeric_limits<int>::min(), -1, 0, 20, 21, std::numeric_limits<int>::max()}) {
        bool rejected = false;
        try {
            static_cast<void>(ui::loading_draws(invalid));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Unsupported loading arguments must be rejected before composing sprites");
    }
}
} // namespace

int main() {
    try {
        check_milestones();
        check_draws();
        std::cout << "Loading source outcomes passed: 104 resource redraws, 70/38 resource order and exact 1/2/19 draw commands\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
