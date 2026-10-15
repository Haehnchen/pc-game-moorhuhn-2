#include "render/framebuffer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

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

    throw std::runtime_error("Invalid draw was accepted");
}

void renderer_pixels() {
    const std::array pixels{assets::RgbaPixel{248, 3, 248, 0}, assets::RgbaPixel{247, 0, 255, 0}, assets::RgbaPixel{255, 4, 255, 0}, assets::RgbaPixel{255, 255, 255, 0},
        assets::RgbaPixel{255, 0, 0, 0}, assets::RgbaPixel{0, 255, 0, 0}};
    const std::array views{render::NamedImageView{"test", {3, 2, 3, assets::ImageKind::truecolor, pixels, {}, {}}}};
    const render::ImageBank images(views);
    render::Framebuffer frame(3, 3);
    const std::array cropped{contracts::SpriteDraw{"test", {1, 0, 2, 2}, {-1, 1}}};
    frame.draw(images, cropped);
    check(frame.rgb565_at(0, 1) == 0xf83f && frame.rgb565_at(0, 2) == 0x07e0, "Crop and left clipping preserve source origin");
    check(frame.rgb565_at(1, 1) == 0, "Clipped sprite width");
    frame.clear(0x001f);
    const std::array ordered{contracts::SpriteDraw{"test", {0, 1, 3, 1}, {0, 0}}, contracts::SpriteDraw{"test", {0, 0, 3, 1}, {0, 0}}};
    frame.draw(images, ordered);
    check(frame.rgb565_at(0, 0) == 0xffff && frame.rgb565_at(1, 0) == 0xf01f, "Painter order and keyed pixel preservation");
    const auto before = std::vector(frame.pixels().begin(), frame.pixels().end());
    const std::array invalid{ordered[0], contracts::SpriteDraw{"test", {2, 0, 2, 1}, {0, 0}}};
    rejects([&] {
        frame.draw(images, invalid);
    });
    check(std::equal(before.begin(), before.end(), frame.pixels().begin()), "Invalid list did not change frame");
    const std::array outside{contracts::SpriteDraw{"test", {0, 0, 3, 2}, {std::numeric_limits<int>::max(), std::numeric_limits<int>::min()}}};
    frame.draw(images, outside);
    check(std::equal(before.begin(), before.end(), frame.pixels().begin()), "Extreme clipping");
    frame.clear();
    const std::array mirrored{contracts::SpriteDraw{"test", {0, 1, 3, 1}, {-1, 0}, true, true}};
    frame.draw(images, mirrored);
    check(frame.rgb565_at(0, 0) == 0xf800 && frame.rgb565_at(1, 0) == 0xffff, "Mirrored crop and clipping");
    frame.set_rgb565_at(2, 2, 1);
    check(frame.rgb565_at(2, 2) == 1 && frame.pixels()[8] == assets::RgbaPixel{0, 0, 8, 255}, "Raw score watermark word reaches both pixel representations");

    for (const auto point : {contracts::ScreenPoint{-1, 0}, {0, -1}, {3, 0}, {0, 3}}) {
        rejects([&] {
            frame.set_rgb565_at(point.x, point.y, 0xffff);
        });
    }

    check(frame.rgb565_at(2, 2) == 1, "Rejected raw writes preserve the frame");
}

void clipped_draw_modes() {
    constexpr std::array<std::uint16_t, 35> words{
        0xf81f, 1, 2, 3, 4, 5, 0xf81f, 7, 0xf81f, 9, 10, 11, 0xf81f, 13, 14, 15, 0xf81f, 17, 0xf81f, 19, 20, 21, 22, 23, 0xf81f, 25, 26, 27, 0xf81f, 29, 30, 31, 32, 33, 0xf81f};
    std::array<assets::RgbaPixel, words.size()> pixels;
    std::ranges::transform(words, pixels.begin(), render::expand_rgb565);
    const std::array views{render::NamedImageView{"grid", {7, 5, 7, assets::ImageKind::truecolor, pixels, {}, {}}}};
    const render::ImageBank images(views);
    render::Framebuffer frame(5, 4);
    constexpr std::uint16_t background = 0x1234;

    for (const bool mirror : {false, true}) {
        for (const bool color_key : {false, true}) {
            for (const auto crop : {contracts::ImageRect{0, 0, 7, 5}, {2, 1, 4, 3}}) {
                for (int top = -5; top <= frame.height(); ++top) {
                    for (int left = -7; left <= frame.width(); ++left) {
                        const contracts::SpriteDraw draw{"grid", crop, {left, top}, color_key, mirror};
                        frame.clear(background);
                        frame.draw(images, std::span(&draw, 1));

                        // Sample from each destination pixel independently of renderer traversal.
                        for (int y = 0; y < frame.height(); ++y) {
                            for (int x = 0; x < frame.width(); ++x) {
                                auto expected = background;
                                const auto local_x = x - left;
                                const auto local_y = y - top;
                                if (local_x >= 0 && local_x < crop.width && local_y >= 0 && local_y < crop.height) {
                                    const auto source_x = crop.x + (mirror ? crop.width - 1 - local_x : local_x);
                                    const auto word = words[static_cast<std::size_t>((crop.y + local_y) * 7 + source_x)];
                                    if (!color_key || word != 0xf81f) {
                                        expected = word;
                                    }
                                }
                                check(frame.rgb565_at(x, y) == expected, "Clipped draw differs from source pixel");
                                check(frame.pixels()[static_cast<std::size_t>(y * frame.width() + x)] == render::expand_rgb565(expected),
                                    "Clipped draw must update both RGB565 and RGBA consistently");
                            }
                        }
                    }
                }
            }
        }
    }
}

void image_switches_and_validation() {
    const std::array keyed{render::expand_rgb565(0xf81f), render::expand_rgb565(0xf800)};
    const std::array opaque{render::expand_rgb565(0x001f), render::expand_rgb565(0x07e0)};
    const std::array views{render::NamedImageView{"keyed", {2, 1, 2, assets::ImageKind::truecolor, keyed, {}, {}}},
        render::NamedImageView{"opaque", {2, 1, 2, assets::ImageKind::truecolor, opaque, {}, {}}}};
    const render::ImageBank images(views);
    const std::array commands{
        contracts::SpriteDraw{"opaque", {0, 0, 2, 1}, {0, 0}}, contracts::SpriteDraw{"keyed", {0, 0, 2, 1}, {0, 0}}, contracts::SpriteDraw{"opaque", {0, 0, 2, 1}, {2, 0}}};
    render::Framebuffer frame(4, 1);
    frame.draw(images, commands);
    check(frame.rgb565_at(0, 0) == 0x001f && frame.rgb565_at(1, 0) == 0xf800 && frame.rgb565_at(2, 0) == 0x001f && frame.rgb565_at(3, 0) == 0x07e0,
        "Image switches must refresh both pixels and color-key metadata");

    const auto before = std::vector(frame.pixels().begin(), frame.pixels().end());
    const std::array invalid{commands[0], contracts::SpriteDraw{"missing", {0, 0, 2, 1}, {0, 0}}};
    rejects([&] {
        frame.draw(images, invalid);
    });
    check(std::ranges::equal(frame.pixels(), before), "Unknown image late in a batch must preserve the complete frame");

    const std::array replacement{render::NamedImageView{"opaque", {2, 1, 2, assets::ImageKind::truecolor, keyed, {}, {}}}};
    const render::ImageBank other_images(replacement);
    frame.draw(other_images, std::span(commands).first(1));
    check(frame.rgb565_at(0, 0) == 0x001f && frame.rgb565_at(1, 0) == 0xf800, "Image lookup caching must not outlive a draw call");
}

void original_hit_equality() {
    const std::array pixels{assets::RgbaPixel{252, 0, 252, 255}, assets::RgbaPixel{0, 0, 0, 255}, assets::RgbaPixel{0, 255, 0, 255}};
    const std::array views{render::NamedImageView{"hit", {3, 1, 3, assets::ImageKind::truecolor, pixels, {}, {}}}};
    const render::ImageBank images(views);
    render::Framebuffer frame(3, 1);
    const contracts::SpriteDraw candidate{"hit", {0, 0, 3, 1}, {0, 0}};
    check(frame.matches_sprite_pixel(images, candidate, {0, 0}), "Original transparent scratch zero equals black scene zero");
    check(frame.matches_sprite_pixel(images, candidate, {1, 0}), "Original black candidate equals black scene");
    check(!frame.matches_sprite_pixel(images, candidate, {2, 0}), "Different composed scene rejects candidate");
    frame.draw(images, std::span(&candidate, 1));
    check(frame.matches_sprite_pixel(images, candidate, {2, 0}), "Composed candidate pixel equality");
    frame.clear(0xf800);
    check(
        !frame.matches_sprite_pixel(images, candidate, {0, 0}) && !frame.matches_sprite_pixel(images, candidate, {1, 0}) && !frame.matches_sprite_pixel(images, candidate, {2, 0}),
        "Occluding pixel rejects all unequal scratch words");
    check(!frame.matches_sprite_pixel(images, candidate, {-1, 0}) && !frame.matches_sprite_pixel(images, candidate, {3, 0}), "Hit bounds");
    const contracts::SpriteDraw cleared_scratch{"", {0, 0, 3, 1}, {0, 0}};
    check(!frame.matches_sprite_pixel(images, cleared_scratch, {1, 0}), "Default source collision must reject nonblack final words");
    frame.clear();
    check(frame.matches_sprite_pixel(images, cleared_scratch, {1, 0}), "Default source collision retains cleared scratch and can match black");
    check(!frame.matches_sprite_pixel(images, cleared_scratch, {-1, 0}) && !frame.matches_sprite_pixel(images, cleared_scratch, {3, 0}), "Cleared-scratch geometry stays bounded");
    rejects([&] {
        frame.draw(images, std::span(&cleared_scratch, 1));
    });
    const contracts::SpriteDraw mirrored{"hit", {1, 0, 2, 1}, {0, 0}, true, true};
    frame.clear(0x07e0);
    check(frame.matches_sprite_pixel(images, mirrored, {0, 0}) && !frame.matches_sprite_pixel(images, mirrored, {1, 0}), "Hit crop and mirroring");
}

} // namespace

int main() {
    try {
        renderer_pixels();
        clipped_draw_modes();
        image_switches_and_validation();
        original_hit_equality();
        std::cout << "Framebuffer crop, clipping, mirror, painter order and hit equality passed\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';

        return 1;
    }
}
