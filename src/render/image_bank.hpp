#pragma once

#include "assets/asset_store.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace moorhuhn2::render {

inline constexpr std::uint16_t drawing_key_rgb565 = 0xf81f;

[[nodiscard]] constexpr std::uint16_t to_rgb565(assets::RgbaPixel pixel) noexcept {
    return static_cast<std::uint16_t>(((pixel.r >> 3) << 11) | ((pixel.g >> 2) << 5) | (pixel.b >> 3));
}

[[nodiscard]] constexpr assets::RgbaPixel expand_rgb565(std::uint16_t pixel) noexcept {
    return {static_cast<std::uint8_t>((((pixel >> 11) & 31) * 255 + 15) / 31), static_cast<std::uint8_t>((((pixel >> 5) & 63) * 255 + 31) / 63),
        static_cast<std::uint8_t>(((pixel & 31) * 255 + 15) / 31), 255};
}

struct NamedImageView {
    std::string_view id;
    assets::ImageView image;
};

struct ConvertedImageView {
    int width{}, height{};
    std::span<const std::uint16_t> rgb565;
    std::span<const assets::RgbaPixel> rgba;
    bool has_color_key{};
};

// Owns the preconverted pixels. Source views may expire after construction.
class ImageBank final {
public:
    ImageBank() = default;
    explicit ImageBank(std::span<const NamedImageView> images);
    explicit ImageBank(const assets::AssetStore& assets);

    // Copies a validated source image during startup; published entries stay immutable.
    void add(std::string_view id, assets::ImageView image);

    [[nodiscard]] ConvertedImageView image(std::string_view id) const;
    [[nodiscard]] std::uint16_t rgb565_at(std::string_view id, int x, int y) const;
    [[nodiscard]] bool opaque(std::string_view id, int x, int y, bool color_key = true) const;

private:
    struct Image {
        int width{}, height{};
        std::vector<std::uint16_t> rgb565;
        std::vector<assets::RgbaPixel> rgba;
        bool has_color_key{};
    };

    std::map<std::string, Image, std::less<>> images_;
};

} // namespace moorhuhn2::render
