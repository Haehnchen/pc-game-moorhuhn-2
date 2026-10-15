#pragma once

#include "contracts/draw.hpp"
#include "render/frame_catalog.hpp"
#include "world/boat.hpp"
#include "world/branch.hpp"
#include "world/hunting_stand.hpp"
#include "world/rose.hpp"
#include "world/scarecrow.hpp"
#include "world/tree.hpp"

#include <optional>
#include <span>
#include <vector>

namespace moorhuhn2::world {

// Stateless presentation: inputs already contain the chosen animation frame.
class Presentation final {
public:
    explicit Presentation(const render::FrameCatalog& frames) : frames_(frames) {}

    // Twelve offsets mean the original introduction: background columns only.
    [[nodiscard]] std::vector<contracts::SpriteDraw> commands(float camera, std::span<const contracts::DepthDraw> dynamic = {}, std::span<const float> strip_offsets = {},
        int recoil = 0, const Boat& boat = {}, std::span<const Branch> branches = {}, std::span<const Tree> trees = {}, std::span<const ScarecrowPiece> scarecrow = {},
        const HuntingStand* hunting_stand = nullptr, const Rose* rose = nullptr, std::optional<int> object_recoil = std::nullopt) const;
    // Appends to result; layer is reusable scratch storage and must not alias dynamic.
    void append_commands(std::vector<contracts::SpriteDraw>& result, std::vector<contracts::DepthDraw>& layer, float camera, std::span<const contracts::DepthDraw> dynamic = {},
        std::span<const float> strip_offsets = {}, int recoil = 0, const Boat& boat = {}, std::span<const Branch> branches = {}, std::span<const Tree> trees = {},
        std::span<const ScarecrowPiece> scarecrow = {}, const HuntingStand* hunting_stand = nullptr, const Rose* rose = nullptr,
        std::optional<int> object_recoil = std::nullopt) const;
    [[nodiscard]] static int camera_offset(float camera, int depth);

private:
    const render::FrameCatalog& frames_;
};

} // namespace moorhuhn2::world
