#pragma once

#include "contracts/draw.hpp"
#include "render/image_bank.hpp"

#include <span>
#include <vector>

namespace moorhuhn2::render {

class Framebuffer final {
public:
    explicit Framebuffer(int width = contracts::logical_width, int height = contracts::logical_height);

    void clear(std::uint16_t color = 0);
    // The whole list is validated before any destination pixel changes.
    void draw(const ImageBank& images, std::span<const contracts::SpriteDraw> commands);

    [[nodiscard]] int width() const noexcept {
        return width_;
    }

    [[nodiscard]] int height() const noexcept {
        return height_;
    }

    [[nodiscard]] std::span<const assets::RgbaPixel> pixels() const noexcept {
        return rgba_;
    }

    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;
    [[nodiscard]] std::uint16_t rgb565_at(int x, int y) const;
    void set_rgb565_at(int x, int y, std::uint16_t color);
    // Original hit helper draws the candidate onto black scratch pixels, then
    // compares the complete word with the already-composed scene, including zero.
    // An empty candidate image means its type draws nothing onto cleared scratch.
    [[nodiscard]] bool matches_sprite_pixel(const ImageBank& images, const contracts::SpriteDraw& sprite, contracts::ScreenPoint point) const;

private:
    int width_, height_;
    std::vector<std::uint16_t> rgb565_;
    std::vector<assets::RgbaPixel> rgba_;
};

} // namespace moorhuhn2::render
