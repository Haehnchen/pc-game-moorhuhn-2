#include "world/presentation.hpp"

#include "assets/asset_store.hpp"
#include "render/frame_catalog.hpp"
#include "render/framebuffer.hpp"
#include "world/persistent_leaves.hpp"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace moorhuhn2;

namespace {

struct Region {
    std::string_view label;
    std::array<int, 4> rect;
    std::string_view rgb565_le_fnv1a64;
};

struct Scene {
    float camera;
    std::string name;
    std::string timer;
    std::vector<Region> regions;
};

const std::array captured_scenes{
    Scene{0.0F, "left", " 1:24",
        {{"sky", {124, 64, 16, 16}, "957e7e2b713c8592"}, {"mountains", {60, 128, 16, 16}, "39832a9ea376c89c"}, {"trees", {60, 176, 16, 16}, "c034f496c80cf535"},
            {"field", {60, 288, 16, 16}, "44626cfe095d9f0c"}, {"grass", {60, 352, 16, 16}, "3efba1bf33970d80"}, {"foreground", {60, 384, 16, 16}, "82aa3f20937c2aa8"},
            {"timer", {0, 0, 110, 50}, "fc4942db3916b246"}, {"score", {605, 0, 35, 50}, "d8d84b0f029a65ca"}, {"cursor", {320, 240, 32, 32}, "a30d98941fbf5675"},
            {"tower", {284, 176, 32, 64}, "742ead39ffd4e14d"}}},
    Scene{640.0F, "middle", " 1:29",
        {{"sky", {60, 64, 16, 16}, "e6860a0560ea0d57"}, {"mountains", {60, 128, 16, 16}, "d00bd7f4f07cf0b3"}, {"trees", {60, 176, 16, 16}, "70612f481c6e3dfe"},
            {"field", {60, 288, 16, 16}, "7a56a6a9124350e3"}, {"grass", {60, 352, 16, 16}, "9fe962fde645f9c3"}, {"foreground", {60, 384, 16, 16}, "a0959a7654e38f70"},
            {"timer", {0, 0, 110, 50}, "9881cd2dd8f9a305"}, {"score", {605, 0, 35, 50}, "d611da8124052a3a"}, {"cursor", {320, 240, 32, 32}, "f77a742de7457565"}}},
    Scene{1280.0F, "right", " 1:17",
        {{"sky", {60, 64, 16, 16}, "fec49a36da012f7c"}, {"mountains", {60, 128, 16, 16}, "02eb4374db7a159b"}, {"trees", {60, 176, 16, 16}, "314066d98217c33c"},
            {"field", {60, 288, 16, 16}, "cc1e9b0322f3bc70"}, {"grass", {60, 352, 16, 16}, "41cec8173c1ab1d6"}, {"foreground", {60, 384, 16, 16}, "ea5cb890a84cc079"},
            {"timer", {0, 0, 110, 50}, "3269677d2f51d31b"}, {"score", {605, 0, 35, 50}, "4e77833e07945ffe"}, {"cursor", {320, 240, 32, 32}, "831616ca4ae79738"}}}};

std::string region_hash(const render::Framebuffer& frame, const std::array<int, 4>& rectangle) {
    const int left = rectangle[0], top = rectangle[1], width = rectangle[2], height = rectangle[3];
    std::uint64_t value = 14695981039346656037ULL;

    for (auto y = top; y < top + height; ++y) {
        for (auto x = left; x < left + width; ++x) {
            const auto word = frame.rgb565_at(x, y);

            for (const auto byte : {word & 255, word >> 8}) {
                value = (value ^ static_cast<std::uint8_t>(byte)) * 1099511628211ULL;
            }
        }
    }

    std::ostringstream result;
    result << std::hex << std::setw(16) << std::setfill('0') << value;

    return result.str();
}

std::size_t captured_regions(const render::FrameCatalog& frames, const render::ImageBank& images, const world::Presentation& presentation, std::span<const Scene> scenes) {
    render::Framebuffer framebuffer;
    std::vector<contracts::SpriteDraw> commands;
    std::vector<contracts::DepthDraw> layer;
    std::size_t checked_pixels = 0;

    for (const auto& scene : scenes) {
        framebuffer.clear();
        commands.clear();
        presentation.append_commands(commands, layer, scene.camera);
        const auto timer = scene.timer;

        for (std::size_t index = 0; index < timer.size(); ++index) {
            const auto character = timer[index];

            if (character == ' ') {
                continue;
            }

            const auto cell = character == ':' ? 96 : 98 + character - '0';
            commands.push_back({"gfx/font", frames.frame_source("gfx/font", cell), {-10 + static_cast<int>(index) * 20, 0}});
        }

        commands.push_back({"gfx/font", frames.frame_source("gfx/font", 98), {610, 0}});
        commands.push_back({"gfx/mousecursor", frames.frame_source("gfx/mousecursor", 0), {320, 240}});
        framebuffer.draw(images, commands);

        for (const auto& region : scene.regions) {
            if (region_hash(framebuffer, region.rect) != region.rgb565_le_fnv1a64) {
                throw std::runtime_error("Original pixel region differs: " + scene.name + "/" + std::string(region.label));
            }

            checked_pixels += static_cast<std::size_t>(region.rect[2]) * static_cast<std::size_t>(region.rect[3]);
        }
    }

    return checked_pixels;
}

void introduction_and_exit(const world::Presentation& presentation) {
    const std::array<float, 12> strips{3, 7, 11};
    const auto introduction = presentation.commands(640, {}, strips, 8);

    if (introduction.front().destination.y != 6 || introduction[1].destination.y != 10) {
        throw std::runtime_error("Original visible-column strip offset or recoil differs");
    }

    for (const auto& command : introduction) {
        if (!command.image.starts_with("gfx/M2BG")) {
            throw std::runtime_error("Original introduction draws backgrounds only");
        }
    }

    // Source stage 1 freezes actors after the last update; firing afterward
    // changes background recoil only. Test both the shot and recovery edge.
    for (const bool shot : {false, true}) {
        const auto frozen = presentation.commands(0, {}, {}, shot ? 8 : 0, {}, {}, {}, {}, nullptr, nullptr, shot ? 0 : 8);

        if (frozen.front().destination.y != (shot ? 3 : 0)) {
            throw std::runtime_error("Exit background must use post-action recoil");
        }

        for (const auto& command : frozen) {
            if ((command.image == "gfx/house" && command.destination.y != (shot ? 221 : 225)) || (command.image == "gfx/boat" && command.destination.y != (shot ? 264 : 270))) {
                throw std::runtime_error("Exit village/boat must retain the last simulated recoil pose");
            }
        }
    }
}

void painter_order(const render::FrameCatalog& frames, const world::Presentation& presentation) {
    const std::array dynamic{contracts::DepthDraw{4, {"gfx/Chickleft3", frames.frame_source("gfx/Chickleft3", 0), {0, 0}}},
        contracts::DepthDraw{2, {"gfx/Chickleft1", frames.frame_source("gfx/Chickleft1", 18), {10, 20}}}};
    const auto commands = presentation.commands(640, dynamic);
    std::size_t far = 0, near = 0, foreground = 0;

    for (std::size_t index = 0; index < commands.size(); ++index) {
        if (commands[index].image == "gfx/Chickleft3") {
            far = index;
        } else if (commands[index].image == "gfx/Chickleft1") {
            near = index;
        } else if (commands[index].image == "gfx/M2BG05" && foreground == 0) {
            foreground = index;
        }
    }

    if (!(far < near && near < foreground)) {
        throw std::runtime_error("Original depth painter order differs");
    }

    // Particles follow scene records, including the expanded bonus-mode pool.
    const std::array expanded{contracts::DepthDraw{2, {"gfx/Chickleft3", {}, {}}}, contracts::DepthDraw{2, {"gfx/Chickleft1", {}, {}}, 255}};
    bool saw_scene = false;

    for (const auto& command : presentation.commands(640, expanded)) {
        if (command.image == "gfx/Chickleft1") {
            saw_scene = true;
        } else if (command.image == "gfx/Chickleft3" && !saw_scene) {
            throw std::runtime_error("Particles must follow expanded scene slots");
        }
    }
}

void persistent_leaf_composition(const render::FrameCatalog& frames, const render::ImageBank& images, const world::Presentation& presentation) {
    render::Framebuffer framebuffer;
    const auto leaf = world::persistent_leaf_draw({.screen_x = 20, .screen_y = 30});
    const auto cell = frames.frame_source("gfx/particle", 1);

    if (cell.x != 0 || cell.y != 1 || cell.width != 8 || cell.height != 1) {
        throw std::runtime_error("Persistent leaf loader must select row1 of the eight-row sheet");
    }

    framebuffer.clear();
    framebuffer.draw(images, std::array{leaf});

    for (int x = 0; x < 8; ++x) {
        if (framebuffer.rgb565_at(20 + x, 30) != (x == 0 ? 0x8400 : 0)) {
            throw std::runtime_error("Persistent leaf must paint exactly its one source-colored pixel");
        }
    }

    const contracts::SpriteDraw black_candidate{"gfx/particle", {0, 0, 8, 1}, {20, 30}};

    if (framebuffer.matches_sprite_pixel(images, black_candidate, {20, 30}) || !framebuffer.matches_sprite_pixel(images, black_candidate, {21, 30})) {
        throw std::runtime_error("Leaf occlusion must affect the source pixel-equality hit test only at its opaque pixel");
    }

    const std::array leaf_layer{contracts::DepthDraw{3, leaf}, contracts::DepthDraw{3, {"gfx/stone", frames.frame_source("gfx/stone", 0), {20, 30}}, 255},
        contracts::DepthDraw{3, {"gfx/particle2", frames.frame_source("gfx/particle2", 1), {20, 30}}}};
    bool saw_target = false, saw_leaf = false, saw_transient = false;

    for (const auto& command : presentation.commands(0, leaf_layer)) {
        if (command.image == "gfx/stone") {
            saw_target = true;
        } else if (command.image == "gfx/particle") {
            if (!saw_target || saw_transient) {
                throw std::runtime_error("Persistent leaves must follow all scene slots and precede transient particles");
            }

            saw_leaf = true;
        } else if (command.image == "gfx/particle2") {
            if (!saw_leaf) {
                throw std::runtime_error("Transient particles must follow persistent leaves");
            }

            saw_transient = true;
        } else if (command.image == "gfx/M2BG01" && !saw_transient) {
            throw std::runtime_error("Near scenery must cover the depth3 particle pass");
        }
    }

    if (!saw_target || !saw_leaf || !saw_transient) {
        throw std::runtime_error("Missing persistent-leaf composition layer");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error("Expected runtime assets");
        }

        const auto assets = assets::AssetStore::load_directory(argv[1]);
        const render::ImageBank images(assets);
        const render::FrameCatalog frames(assets);
        const world::Presentation presentation(frames);
        const auto checked_pixels = captured_regions(frames, images, presentation, captured_scenes);
        introduction_and_exit(presentation);
        painter_order(frames, presentation);
        persistent_leaf_composition(frames, images, presentation);
        std::cout << checked_pixels << " original left/middle/right, font and cursor pixels matched\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';

        return 1;
    }
}
