#include "world/presentation.hpp"

#include "world/numeric.hpp"
#include "world/village.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

constexpr std::array camera_factors{1.8F, 1.6F, 1.6F, 1.4F, 1.2F};

} // namespace

int Presentation::camera_offset(float camera, int depth) {
    if (!std::isfinite(camera) || camera < 0 || camera > 1280 || depth < 0 || depth > 4) {
        throw std::invalid_argument("Invalid MH2 camera or depth");
    }

    // Original x87 multiplication reaches _ftol without a binary32 product store.
    return detail::camera_offset(camera, camera_factors[depth]);
}

std::vector<contracts::SpriteDraw> Presentation::commands(float camera, std::span<const contracts::DepthDraw> dynamic, std::span<const float> strip_offsets, int recoil,
    const Boat& boat, std::span<const Branch> branches, std::span<const Tree> trees, std::span<const ScarecrowPiece> scarecrow, const HuntingStand* hunting_stand, const Rose* rose,
    std::optional<int> object_recoil) const {
    std::vector<contracts::SpriteDraw> result;
    std::vector<contracts::DepthDraw> layer;
    append_commands(result, layer, camera, dynamic, strip_offsets, recoil, boat, branches, trees, scarecrow, hunting_stand, rose, object_recoil);
    return result;
}

void Presentation::append_commands(std::vector<contracts::SpriteDraw>& result, std::vector<contracts::DepthDraw>& layer, float camera,
    std::span<const contracts::DepthDraw> dynamic, std::span<const float> strip_offsets, int recoil, const Boat& boat, std::span<const Branch> branches,
    std::span<const Tree> trees, std::span<const ScarecrowPiece> scarecrow, const HuntingStand* hunting_stand, const Rose* rose, std::optional<int> object_recoil) const {
    static_cast<void>(camera_offset(camera, 0));
    // Exit redraws backgrounds with the post-shot recoil, while scene actors
    // retain their last simulated poses. Boat/village poses are computed here.
    const int scene_recoil = object_recoil.value_or(recoil);

    if ((!strip_offsets.empty() && strip_offsets.size() != 12) || (!branches.empty() && branches.size() != 4) || (!trees.empty() && trees.size() != 4) || recoil < 0 || recoil > 8
        || (!scarecrow.empty() && scarecrow.size() != 8) || scene_recoil < 0 || scene_recoil > 8) {
        throw std::invalid_argument("Invalid MH2 strip offsets, scene record counts or recoil");
    }

    for (const auto offset : strip_offsets) {
        if (!std::isfinite(offset) || std::abs(offset) > 1000) {
            throw std::invalid_argument("Invalid MH2 strip displacement");
        }
    }

    for (const auto& draw : dynamic) {
        if (draw.depth < 0 || draw.depth > 4 || draw.scene_slot < 0 || draw.scene_slot > contracts::scene_pool_capacity) {
            throw std::invalid_argument("Invalid MH2 sprite depth");
        }
    }

    // Static anchor callers get the original live poses. Gameplay supplies all
    // four records, including removed ones, so absent branches stay absent.
    std::array<Branch, 4> initial_branch_states;

    if (branches.empty()) {
        initial_branch_states = initial_branches();

        for (auto& branch : initial_branch_states) {
            static_cast<void>(update_branch(branch, 0, camera, scene_recoil));
        }

        branches = initial_branch_states;
    }

    std::array<Tree, 4> initial_tree_states;

    if (trees.empty()) {
        initial_tree_states = initial_trees();

        for (auto& tree : initial_tree_states) {
            static_cast<void>(update_tree(tree, 0, camera, scene_recoil));
        }

        trees = initial_tree_states;
    }

    std::array<ScarecrowPiece, 8> initial_scarecrow_states;

    if (scarecrow.empty()) {
        initial_scarecrow_states = initial_scarecrow();

        for (auto& piece : initial_scarecrow_states) {
            update_scarecrow_piece(piece, 0, camera, scene_recoil);
        }

        scarecrow = initial_scarecrow_states;
    }

    HuntingStand initial_stand;

    if (!hunting_stand) {
        initial_stand = initial_hunting_stand();
        update_hunting_stand(initial_stand, 0, camera, scene_recoil);
        hunting_stand = &initial_stand;
    }

    Rose initial_rose;

    if (!rose) {
        update_rose(initial_rose, 0, camera, scene_recoil);
        rose = &initial_rose;
    }

    result.reserve(result.size() + 80 + dynamic.size());
    const auto background = [&](std::string_view image, int offset, float recoil_factor) {
        const auto first = offset / 64;
        const auto displacement = static_cast<int>(static_cast<double>(recoil) * recoil_factor);

        for (auto tile = first; tile * 64 - offset < contracts::logical_width; ++tile) {
            const auto strip = strip_offsets.empty() ? 0 : strip_offsets[tile - first];
            result.push_back({image, frames_.frame_source(image, tile), {tile * 64 - offset, static_cast<int>(displacement + strip)}, true});
        }
    };
    if (strip_offsets.empty()) {
        // The largest static layer contains the boat, two stand sprites and scarecrow.
        layer.reserve(dynamic.size() + scarecrow.size() + 3);
    }
    const auto depth = [&](int value) {
        if (!strip_offsets.empty()) {
            return;
        }

        layer.clear();

        if (value == 4) {
            for (const auto target : {VillageTarget::first_church, VillageTarget::second_church, VillageTarget::house}) {
                layer.push_back({value, village_draw(target, camera, scene_recoil), static_cast<int>(target)});
            }
        }

        if (value == 0) {
            if (const auto draw = rose_draw(*rose)) {
                layer.push_back({value, *draw, 32});
            }
        }

        for (const auto& draw : dynamic) {
            if (draw.depth == value) {
                layer.push_back(draw);
            }
        }

        if (value == 3) {
            layer.push_back({value, boat_draw(boat, camera, scene_recoil), 3});
            // Both copies belong to slot4; the right cover has no hit record.
            for (const auto& sprite : hunting_stand_draw(*hunting_stand)) {
                layer.push_back({value, sprite, 4});
            }

            for (std::size_t index = 0; index < scarecrow.size(); ++index) {
                if (scarecrow[index].type != 500) {
                    layer.push_back({value, scarecrow_draw(scarecrow[index]), 6 + static_cast<int>(index)});
                }
            }
        }

        if (value == 2) {
            for (std::size_t index = 0; index < trees.size(); ++index) {
                if (trees[index].type >= 0) {
                    layer.push_back({value, tree_draw(trees[index]), 15 + static_cast<int>(index)});
                }
            }

            for (std::size_t index = 0; index < branches.size(); ++index) {
                if (branches[index].type >= 0) {
                    layer.push_back({value, branch_draw(branches[index]), 20 + static_cast<int>(index)});
                }
            }
        }

        // A removed frog's slot can be reused before the static branches/stones.
        std::ranges::stable_sort(layer, {}, &contracts::DepthDraw::scene_slot);

        for (const auto& draw : layer) {
            result.push_back(draw.sprite);
        }
    };
    background("gfx/M2BG04", static_cast<int>(camera), 0.4F);
    background("gfx/M2BG03", camera_offset(camera, 4), 0.6F);
    depth(4);
    background("gfx/M2BG02", camera_offset(camera, 3), 0.8F);
    depth(3);
    background("gfx/M2BG01", camera_offset(camera, 2), 1.0F);
    depth(2);
    depth(1);
    background("gfx/M2BG05", camera_offset(camera, 0), 1.2F);
    depth(0);
}

} // namespace moorhuhn2::world
