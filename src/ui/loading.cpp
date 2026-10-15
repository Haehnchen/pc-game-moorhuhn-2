#include "ui/loading.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace moorhuhn2::ui {
namespace {
// 00408240 calls 0040f070 after these exact resource completions.
constexpr std::array<LoadingMilestone, 104> milestones{{
    {"gfx/mainscreen", 1},
    {"gfx/scorescreen", 2},
    {"gfx/M2BG01", 5},
    {"gfx/M2BG02", 5},
    {"gfx/M2BG03", 5},
    {"gfx/M2BG04", 5},
    {"gfx/M2BG05", 5},
    {"gfx/hochsitz3", 6},
    {"gfx/hochsitz1", 6},
    {"gfx/hochsitz2", 6},
    {"gfx/hochsitz4", 6},
    {"gfx/hochsitzcol", 6},
    {"gfx/boat", 6},
    {"gfx/boatcol", 6},
    {"gfx/flyaway1", 6},
    {"gfx/flyaway3", 6},
    {"gfx/flyaway2", 6},
    {"gfx/big_appear", 6},
    {"gfx/big_hit", 6},
    {"gfx/frog", 6},
    {"gfx/Chickleft1", 7},
    {"gfx/Chickleft2", 7},
    {"gfx/Chickleft3", 7},
    {"gfx/Chickright1", 7},
    {"gfx/Chickright2", 7},
    {"gfx/Chickright3", 7},
    {"gfx/DChickleft1", 8},
    {"gfx/DChickleft2", 8},
    {"gfx/DChickleft3", 8},
    {"gfx/DChickright1", 8},
    {"gfx/DChickright2", 8},
    {"gfx/DChickright3", 8},
    {"gfx/stone", 9},
    {"gfx/particle", 9},
    {"gfx/particle2", 9},
    {"gfx/pole", 9},
    {"gfx/scheuche", 9},
    {"gfx/lplane", 9},
    {"gfx/rplane", 9},
    {"gfx/spinne1", 9},
    {"gfx/spinne2", 9},
    {"gfx/ballon", 9},
    {"gfx/branch1", 10},
    {"gfx/branch2", 10},
    {"gfx/tree1_01", 10},
    {"gfx/tree1_02", 10},
    {"gfx/blimp", 10},
    {"gfx/Leaf", 11},
    {"gfx/Leaf2", 11},
    {"gfx/specialhuhn", 11},
    {"gfx/rose", 11},
    {"gfx/highfly", 11},
    {"gfx/church", 11},
    {"gfx/house", 11},
    {"gfx/hide1", 12},
    {"gfx/hide2", 12},
    {"gfx/hide3", 12},
    {"gfx/Witansmall", 12},
    {"gfx/jennie", 12},
    {"gfx/kopf", 12},
    {"gfx/snake", 12},
    {"gfx/Bossscreen", 12},
    {"gfx/Making", 12},
    {"gfx/WitanLogo", 12},
    {"gfx/Computer", 12},
    {"gfx/disclaimer", 12},
    {"sfx/Moorhuhn2_Title_Music", 13},
    {"sfx/Moorhuhn2_Highscore_Music", 14},
    {"sfx/sfx_background_birds_STEREO_LOOP", 15},
    {"sfx/sfx_shotgun_blast", 16},
    {"sfx/sfx_gun_reload", 16},
    {"sfx/sfx_empty_magazine", 16},
    {"sfx/sfx_big_chicken_pops_up", 16},
    {"sfx/chickenhit2", 16},
    {"sfx/sfx_chicken_shot_FAR", 16},
    {"sfx/sfx_chicken_shot_MID", 16},
    {"sfx/sfx_chicken_shot_CLOSE", 16},
    {"sfx/sfx_stone_shot", 16},
    {"sfx/sfx_frog", 16},
    {"sfx/sfx_frog_shot", 16},
    {"sfx/sfx_typename_click", 16},
    {"sfx/sfx_balloon_shot", 16},
    {"sfx/sfx_airplane_loop", 16},
    {"sfx/sfx_airplane_motor_failure_loop", 17},
    {"sfx/sfx_airplane_shot", 17},
    {"sfx/sfx_airplane_crash", 17},
    {"sfx/sfx_zeppelin_shot", 17},
    {"sfx/sfx_scarecrow_shot", 17},
    {"sfx/sfx_tree_shot", 17},
    {"sfx/sfx_treebranch_shot", 18},
    {"sfx/sfx_treebranch_hits_ground", 18},
    {"sfx/sfx_churchbell", 18},
    {"sfx/sfx_churchbell_2", 19},
    {"sfx/sfx_get_ready_after_typing_name", 19},
    {"sfx/sfx_game_over", 19},
    {"sfx/sfx_time_running_out", 19},
    {"sfx/sfx_spider_swings", 19},
    {"sfx/sfx_boat_sinking", 19},
    {"sfx/sfx_magic_balloons_appear", 19},
    {"sfx/sfx_50_point_score", 19},
    {"sfx/sfx_tree_breaks_down", 19},
    {"sfx/sfx_guardhouse_breaks_down", 19},
    {"sfx/sfx_guardhouse_ladder_shot", 19},
    {"sfx/sfx_house_shot", 19},
}};

constexpr auto image_order = [] {
    std::array<std::string_view, 70> result{};
    result[0] = "gfx/loading";
    result[1] = "gfx/loadingbar";
    std::size_t index = 2;
    for (const auto& milestone : milestones) {
        if (milestone.asset.starts_with("gfx/")) {
            result[index++] = milestone.asset;
        }
    }

    result[index++] = "gfx/mousecursor";
    result[index] = "gfx/font";

    return result;
}();

constexpr auto audio_order = [] {
    std::array<std::string_view, 38> result{};
    std::size_t index = 0;
    for (const auto& milestone : milestones) {
        if (milestone.asset.starts_with("sfx/")) {
            result[index++] = milestone.asset;
        }
    }

    return result;
}();
} // namespace

std::span<const LoadingMilestone> loading_milestones() noexcept {
    return milestones;
}

std::span<const std::string_view> loading_image_order() noexcept {
    return image_order;
}

std::span<const std::string_view> loading_audio_order() noexcept {
    return audio_order;
}

std::vector<contracts::SpriteDraw> loading_draws(int progress) {
    if (progress < 1 || progress > 19) {
        throw std::invalid_argument("Loading progress must be in 1..19");
    }
    std::vector<contracts::SpriteDraw> result;
    result.reserve(static_cast<std::size_t>(21 - progress));
    result.push_back({"gfx/loading", {0, 0, 267, 200}, {187, 100}});
    for (int frame = 0; frame < 20 - progress; ++frame) {
        result.push_back({"gfx/loadingbar", {0, frame * 10, 267, 10}, {187, 100 + frame * 10}});
    }

    return result;
}

} // namespace moorhuhn2::ui
