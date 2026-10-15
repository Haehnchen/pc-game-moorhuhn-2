#include "render/image_bank.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

using namespace moorhuhn2;

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function> void rejects(Function function) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }

    throw std::runtime_error("Invalid image request was accepted");
}

void color_conversion() {
    const std::array pixels{assets::RgbaPixel{248, 3, 248, 0}, assets::RgbaPixel{247, 0, 255, 0}, assets::RgbaPixel{255, 4, 255, 0}, assets::RgbaPixel{255, 255, 255, 0},
        assets::RgbaPixel{255, 0, 0, 0}, assets::RgbaPixel{0, 255, 0, 0}};
    const std::array views{render::NamedImageView{"test", {3, 2, 3, assets::ImageKind::truecolor, pixels, {}, {}}}};
    const render::ImageBank images(views);
    check(images.rgb565_at("test", 0, 0) == 0xf81f, "Converted key alias");
    check(images.rgb565_at("test", 1, 0) == 0xf01f, "Red adjacent bucket");
    check(images.rgb565_at("test", 2, 0) == 0xf83f, "Green adjacent bucket");
    check(!images.opaque("test", 0, 0) && images.opaque("test", 0, 0, false), "Key opacity");
    check(images.image("test").has_color_key, "Quantized RGB565 aliases must keep the color-key path");
    check(!images.opaque("test", -1, 0), "Off-image opacity");
    check(render::expand_rgb565(0x8410) == assets::RgbaPixel{132, 130, 132, 255}, "Rounded normalized display expansion");
}

void indexed_pixels() {
    const std::array pixels{assets::RgbaPixel{}, assets::RgbaPixel{}};
    const std::array<std::uint8_t, 2> indices{0, 1};
    const std::array palette{assets::PaletteEntry{248, 3, 248, 255}, assets::PaletteEntry{0, 255, 0, 0}};
    const std::array views{render::NamedImageView{"indexed", {2, 1, 2, assets::ImageKind::indexed, pixels, indices, palette}}};
    const render::ImageBank images(views);
    check(!images.opaque("indexed", 0, 0) && images.rgb565_at("indexed", 1, 0) == 0x07e0, "Palette RGB determines key; reserved byte is not alpha");
    check(images.image("indexed").has_color_key, "Key detection uses indexed palette colors");
    check(images.image("indexed").rgba[1].a == 255, "Palette reserved byte ignored");
    rejects([&] {
        static_cast<void>(images.image("missing"));
    });
    const std::array<std::uint8_t, 2> invalid_indices{0, 2};
    const std::array invalid_views{render::NamedImageView{"invalid", {2, 1, 2, assets::ImageKind::indexed, pixels, invalid_indices, palette}}};
    rejects([&] {
        const render::ImageBank invalid(invalid_views);
    });
}

void opaque_image() {
    // Padding and alpha do not participate in the original RGB565 color key.
    const std::array pixels{assets::RgbaPixel{255, 0, 0, 0}, assets::RgbaPixel{255, 0, 255, 255}, assets::RgbaPixel{0, 255, 0, 0}, assets::RgbaPixel{255, 0, 255, 255}};
    const std::array views{render::NamedImageView{"padded", {1, 2, 2, assets::ImageKind::truecolor, pixels, {}, {}}}};
    const render::ImageBank images(views);
    check(!images.image("padded").has_color_key, "Only visible image pixels determine the keyed drawing path");
}

} // namespace

int main() {
    try {
        color_conversion();
        indexed_pixels();
        opaque_image();
        std::cout << "Image conversion, palette and drawing-key checks passed\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';

        return 1;
    }
}
