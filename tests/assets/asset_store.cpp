#include "assets/asset_store.hpp"

#include "fixtures.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {
using namespace asset_test;
using moorhuhn2::test::require;
using moorhuhn2::test::require_throws;
namespace assets = moorhuhn2::assets;
namespace detail = assets::detail;

assets::AssetError asset_failure(const std::function<void()>& operation, const std::string& cause) {
    try {
        operation();
    } catch (const assets::AssetError& error) {
        return error;
    }

    throw std::runtime_error("Accepted invalid data: " + cause);
}

void equivalent(assets::AssetStore& loose, const assets::AssetStore& packed) {
    require(std::ranges::equal(loose.image_ids(), packed.image_ids()), "Image ID equivalence");
    require(std::ranges::equal(loose.audio_ids(), packed.audio_ids()), "Audio ID equivalence");
    require(loose.image_ids().size() == 70 && loose.audio_ids().size() == 39, "MH2 full inventory");
    std::size_t indexed{};
    std::set<std::pair<std::uint32_t, std::uint32_t>> formats;

    for (const auto& id : loose.image_ids()) {
        const auto left = loose.image(id), right = packed.image(id);
        require(left.width == right.width && left.height == right.height && left.row_stride == left.width && left.kind == right.kind, "Image dimensions/kind equivalence: " + id);
        require(std::ranges::equal(left.rgba, right.rgba) && std::ranges::equal(left.indices, right.indices) && std::ranges::equal(left.palette, right.palette),
            "Image payload equivalence: " + id);
        require(loose.sprite_grid(id) == packed.sprite_grid(id), "Sprite grids survive package round trip: " + id);

        indexed += left.kind == assets::ImageKind::indexed ? 1U : 0U;
    }

    require(indexed == 59, "59 indexed BMP sources");
    require(loose.sprite_grid("gfx/mainscreen").has_value() && loose.sprite_grid("gfx/frog").has_value() && loose.sprite_grid("gfx/spinne1").has_value()
                && loose.sprite_grid("gfx/spinne2").has_value(),
        "Adopted scene image groups have sprite grids");

    for (const auto& id : loose.audio_ids()) {
        const auto left = loose.audio(id), right = packed.audio(id);
        require(left.sample_rate == right.sample_rate && left.channels == right.channels && left.decoded_frames == right.decoded_frames
                    && std::ranges::equal(left.encoded, right.encoded),
            "Audio equivalence: " + id);
        formats.emplace(left.sample_rate, left.channels);
    }

    require(formats.contains({22050, 2}) && formats.contains({44100, 1}) && formats.contains({44100, 2}), "Native audio formats must be preserved");
    const auto title = loose.image("gfx/mainscreen");
    require(title.width == 640 && title.height == 480 && title.indices.empty() && title.palette.empty(), "640x480 truecolor title");
    auto moved = std::move(loose);
    require(moved.image("gfx/mainscreen").rgba.data() == title.rgba.data(), "Move must preserve borrowed views");
    require(title.rgba[0].a == 255, "Borrowed view remains readable after move");
    require_throws(
        [&] {
            static_cast<void>(loose.image_ids());
        },
        "Accepted invalid data: moved-from store");
    require_throws(
        [&] {
            static_cast<void>(moved.image("unknown"));
        },
        "Accepted invalid data: unknown image");
    require_throws(
        [&] {
            static_cast<void>(moved.audio("gfx/mainscreen"));
        },
        "Accepted invalid data: image as audio");
    require_throws(
        [&] {
            static_cast<void>(moved.sprite_grid("unknown"));
        },
        "Accepted invalid data: unknown sprite grid");
    loose = std::move(moved);
}

struct ManifestRows {
    std::vector<std::string> images, audio;

    [[nodiscard]] std::string text() const {
        std::ostringstream output;
        output << "MH2ASSETS\nimages " << images.size() << '\n';

        for (const auto& row : images) {
            output << row << '\n';
        }

        output << "audio " << audio.size() << '\n';

        for (const auto& row : audio) {
            output << row << '\n';
        }

        return output.str();
    }
};

std::string image_row(const std::filesystem::path& root, const assets::AssetStore& store, std::string_view id) {
    const auto image = store.image(id);
    const auto indexed = image.kind == assets::ImageKind::indexed;
    const auto grid = store.sprite_grid(id).value_or(assets::SpriteGrid{});
    std::ostringstream output;
    output << "image " << id << ' ' << (indexed ? "indexed" : "truecolor") << ' ' << image.width << ' ' << image.height << ' '
           << std::filesystem::file_size(root / ("images/" + std::string(id) + ".png")) << ' ' << image.indices.size() << ' ' << image.palette.size() * 4 << ' ' << grid.columns
           << ' ' << grid.rows << ' ' << grid.cell_width << ' ' << grid.cell_height;

    return output.str();
}

std::string audio_row(const assets::AssetStore& store, std::string_view id) {
    const auto audio = store.audio(id);
    std::ostringstream output;
    output << "audio " << id << ' ' << audio.encoded.size() << ' ' << audio.sample_rate << ' ' << audio.channels << ' ' << audio.decoded_frames;

    return output.str();
}

void replace_field(std::string& row, std::size_t index, std::string_view value) {
    std::istringstream input(row);
    std::vector<std::string> fields;

    for (std::string field; input >> field;) {
        fields.push_back(std::move(field));
    }

    fields.at(index) = value;
    row.clear();

    for (const auto& field : fields) {
        if (!row.empty()) {
            row += ' ';
        }

        row += field;
    }
}

void observer_checks(const std::filesystem::path& assets_root, const std::filesystem::path& fixture, assets::AssetStore& retained) {
    const std::array<std::string_view, 4> canonical_images{"gfx/loading", "gfx/loadingbar", "gfx/particle", "gfx/particle2"};
    const std::array<std::string_view, 2> canonical_audio{"sfx/sfx_tree_shot", "sfx/sfx_typename_click"};
    ManifestRows manifest;
    std::vector<std::pair<std::string, Bytes>> image_files, audio_files;
    const auto copy_file = [&](std::string path, auto& files) {
        auto payload = detail::read_file(assets_root / path);
        write(fixture / path, payload);
        files.emplace_back(std::move(path), std::move(payload));
    };

    for (const auto id : canonical_images) {
        manifest.images.push_back(image_row(assets_root, retained, id));
        copy_file("images/" + std::string(id) + ".png", image_files);

        if (retained.image(id).kind == assets::ImageKind::indexed) {
            copy_file("indexed/" + std::string(id) + ".indices", image_files);
            copy_file("indexed/" + std::string(id) + ".palette", image_files);
        }
    }

    for (const auto id : canonical_audio) {
        manifest.audio.push_back(audio_row(retained, id));
        copy_file("audio/" + std::string(id) + ".mp3", audio_files);
    }

    const auto write_package = [&](const std::filesystem::path& path, auto files, const Bytes& manifest_bytes) {
        files.emplace_back("manifest.txt", manifest_bytes);
        std::ranges::sort(files, [](const auto& left, const auto& right) {
            return left.first < right.first;
        });
        write(path, package(files));
    };
    const auto save = [&](const ManifestRows& candidate) {
        const auto manifest_bytes = bytes(candidate.text());
        write(fixture / "manifest.txt", manifest_bytes);
        write_package(fixture / "images.pak", image_files, manifest_bytes);
        write_package(fixture / "audio.pak", audio_files, manifest_bytes);
    };
    const std::array<std::string_view, 2> priority{"gfx/loadingbar", "gfx/loading"};
    const std::vector<std::string> image_sequence{"gfx/loadingbar", "gfx/loading", "gfx/particle", "gfx/particle2"};
    const std::vector<std::string> complete_sequence{"gfx/loadingbar", "gfx/loading", "gfx/particle", "gfx/particle2", "sfx/sfx_tree_shot", "sfx/sfx_typename_click"};
    const auto* retained_pixels = retained.image("gfx/mainscreen").rgba.data();

    for (const bool packaged : {false, true}) {
        const auto load = [&](const assets::LoadObserver& observer) {
            return packaged ? assets::AssetStore::load_packages(fixture, observer) : assets::AssetStore::load_directory(fixture, observer);
        };
        const auto fail = [&](const assets::LoadObserver& observer, std::string_view id, std::string_view cause) {
            const auto error = asset_failure(
                [&] {
                    retained = load(observer);
                },
                std::string(cause));
            require(error.id() == id && error.cause().find(cause) != std::string::npos, "Observer failure must retain asset context: " + std::string(error.what()));
            require(retained.image("gfx/mainscreen").rgba.data() == retained_pixels, "Observer/corruption failure must leave destination unchanged");

            return error;
        };

        struct ImageCopy {
            std::string id;
            std::uint32_t width{}, height{};
            assets::ImageKind kind{};
            std::vector<assets::RgbaPixel> rgba;
            Bytes indices;
            std::vector<assets::PaletteEntry> palette;
        };

        save(manifest);
        std::vector<std::string> observed;
        std::vector<ImageCopy> copied;
        const assets::LoadObserver observer{priority,
            [&](std::string_view id, assets::ImageView image) {
                require(image.row_stride == image.width && image.rgba.size() == std::size_t(image.width) * image.height, "Callback receives complete image storage");

                if (image.kind == assets::ImageKind::indexed) {
                    require(image.indices.size() == image.rgba.size() && !image.palette.empty(), "Callback receives complete indexed storage");
                }

                observed.emplace_back(id);
                copied.push_back({std::string(id), image.width, image.height, image.kind, {image.rgba.begin(), image.rgba.end()}, {image.indices.begin(), image.indices.end()},
                    {image.palette.begin(), image.palette.end()}});
            },
            [&](std::string_view id) {
                observed.emplace_back(id);
            }};
        {
            const auto loaded = load(observer);
            require(std::ranges::equal(loaded.image_ids(), canonical_images) && std::ranges::equal(loaded.audio_ids(), canonical_audio),
                "Priority decoding must preserve canonical public IDs");
            require(observed == complete_sequence, "Priority images precede remaining canonical images, then canonical audio");

            for (const auto& copy : copied) {
                const auto image = loaded.image(copy.id);
                require(copy.width == image.width && copy.height == image.height && copy.kind == image.kind && std::ranges::equal(copy.rgba, image.rgba)
                            && std::ranges::equal(copy.indices, image.indices) && std::ranges::equal(copy.palette, image.palette),
                    "Callback image copies match committed immutable storage");
            }
        }

        for (const auto& copy : copied) {
            const auto original = retained.image(copy.id);
            require(std::ranges::equal(copy.rgba, original.rgba) && std::ranges::equal(copy.indices, original.indices) && std::ranges::equal(copy.palette, original.palette),
                "Copied callback payload survives loader/store destruction");
        }

        const auto record_image = [&](std::string_view id, assets::ImageView) {
            observed.emplace_back(id);
        };
        const auto record_audio = [&](std::string_view id) {
            observed.emplace_back(id);
        };
        const assets::LoadObserver recording{priority, record_image, record_audio};

        for (const auto& invalid_order : {std::vector<std::string_view>{"gfx/missing"}, std::vector<std::string_view>{"gfx/loadingbar", "gfx/loadingbar"}}) {
            observed.clear();
            const assets::LoadObserver invalid{invalid_order, record_image, record_audio};
            const auto error = fail(invalid, invalid_order.back(), invalid_order.size() == 1 ? "missing from manifest" : "duplicate image priority ID");
            require(error.path() == "image_order" && observed.empty(), "Invalid priority prefix fails before callbacks");
        }

        const auto malformed_ids = [&](bool audio, bool duplicate) {
            auto invalid = manifest;
            auto& records = audio ? invalid.audio : invalid.images;

            if (duplicate) {
                records.push_back(records[0]);
            } else {
                std::swap(records[0], records[1]);
            }

            save(invalid);
            observed.clear();
            const auto id = audio ? canonical_audio[0] : canonical_images[0];
            fail(recording, id, "duplicate, unordered");
            require(observed.empty(), "Canonical manifest IDs must be validated before priority decoding/callbacks");
        };
        malformed_ids(false, false);
        malformed_ids(false, true);
        malformed_ids(true, false);
        malformed_ids(true, true);
        save(manifest);
        observed.clear();
        std::vector<assets::RgbaPixel> cancellation_copy;
        const assets::LoadObserver cancelled{priority,
            [&](std::string_view id, assets::ImageView image) {
                observed.emplace_back(id);

                if (id == "gfx/loadingbar") {
                    cancellation_copy.assign(image.rgba.begin(), image.rgba.end());
                } else {
                    throw std::runtime_error("close requested");
                }
            },
            record_audio};
        require(fail(cancelled, "gfx/loading", "image_loaded observer failed: close requested").path() == "manifest.txt", "Image observer exception path context");
        require(observed == std::vector<std::string>{"gfx/loadingbar", "gfx/loading"}, "Image cancellation stops later preparation");
        require(std::ranges::equal(cancellation_copy, retained.image("gfx/loadingbar").rgba), "Copied callback pixels survive cancelled transaction");
        observed.clear();
        const assets::LoadObserver nonstandard{priority,
            [&](std::string_view id, assets::ImageView) {
                observed.emplace_back(id);
                throw 42;
            },
            record_audio};
        fail(nonstandard, "gfx/loadingbar", "image_loaded observer failed with non-standard exception");
        require(observed == std::vector<std::string>{"gfx/loadingbar"}, "Non-standard observer exception aborts preparation");
        observed.clear();
        const assets::LoadObserver audio_cancelled{priority, record_image, [&](std::string_view id) {
                                                       observed.emplace_back(id);
                                                       throw std::runtime_error("audio close requested");
                                                   }};
        fail(audio_cancelled, canonical_audio[0], "audio_loaded observer failed: audio close requested");
        auto through_first_audio = image_sequence;
        through_first_audio.emplace_back(canonical_audio[0]);
        require(observed == through_first_audio, "Audio cancellation stops later audio preparation");
        auto invalid_image = manifest;
        replace_field(invalid_image.images[2], 5, "1");
        save(invalid_image);
        observed.clear();
        fail(recording, canonical_images[2], "runtime byte count mismatch");
        require(observed == std::vector<std::string>{"gfx/loadingbar", "gfx/loading"}, "Rejected image does not reach its callback");
        auto invalid_audio = manifest;
        replace_field(invalid_audio.audio[1], 2, "1");
        save(invalid_audio);
        observed.clear();
        fail(recording, canonical_audio[1], "runtime byte count mismatch");
        require(observed == through_first_audio, "No audio callback before encoded payload and metadata validation complete");

        if (packaged) {
            save(manifest);
            auto extra_audio = audio_files;
            extra_audio.emplace_back("unused", bytes("unreferenced"));
            write_package(fixture / "audio.pak", std::move(extra_audio), bytes(manifest.text()));
            observed.clear();
            fail(recording, canonical_audio[1], "Audio package contains unreferenced data");
            require(observed == complete_sequence, "Late package validation can abort after callbacks without publishing a store");
        }
    }
}

void store_failures(const std::filesystem::path& assets_root, const std::filesystem::path& fixture, assets::AssetStore& retained) {
    // A source-derived title is sufficient; the loader has no hidden roster exception.
    ManifestRows manifest{{image_row(assets_root, retained, "gfx/mainscreen")}, {}};
    const std::string png_path = "images/gfx/mainscreen.png";
    write(fixture / png_path, detail::read_file(assets_root / png_path));
    const auto save = [&](const ManifestRows& data) {
        write(fixture / "manifest.txt", bytes(data.text()));
    };
    save(manifest);
    require(assets::AssetStore::load_directory(fixture).image_ids().size() == 1, "Independent single-image fixture");
    const auto fail = [&](std::string_view name, std::string invalid) {
        write(fixture / "manifest.txt", bytes(invalid));
        const auto* existing = retained.image("gfx/mainscreen").rgba.data();
        require_throws(
            [&] {
                retained = assets::AssetStore::load_directory(fixture);
            },
            "Accepted invalid data: " + std::string(name));
        require(retained.image("gfx/mainscreen").rgba.data() == existing, "Failed load must leave destination unchanged");
    };
    fail("header", "MH2ASSETS 2\nimages 0\naudio 0\n");
    fail("missing image row", "MH2ASSETS\nimages 1\naudio 0\n");
    fail("extra row", manifest.text() + "unknown\n");
    auto duplicate = manifest;
    duplicate.images.push_back(duplicate.images[0]);
    fail("duplicate ID", duplicate.text());

    for (const auto& [field, value] :
        std::vector<std::pair<std::size_t, std::string>>{{1, "../escape"}, {2, "unknown"}, {3, "1"}, {4, "-1"}, {5, "1"}, {6, "1"}, {8, "0"}, {9, "2"}, {10, "639"}}) {
        auto invalid = manifest;
        replace_field(invalid.images[0], field, value);
        fail("invalid image field " + std::to_string(field), invalid.text());
    }

    auto extra_field = manifest;
    extra_field.images[0] += " extra";
    fail("extra image field", extra_field.text());
    save(manifest);
    const auto manifest_bytes = bytes(manifest.text());
    const auto png = detail::read_file(assets_root / png_path);
    write(fixture / "images.pak", package({{png_path, png}, {"manifest.txt", manifest_bytes}}));
    write(fixture / "audio.pak", package({{"manifest.txt", manifest_bytes}}));
    require(assets::AssetStore::load_packages(fixture).image_ids().size() == 1, "Independent paired package fixture");
    write(fixture / "audio.pak", package({{"manifest.txt", bytes("MH2ASSETS\nimages 0\naudio 0\n")}}));
    require_throws(
        [&] {
            static_cast<void>(assets::AssetStore::load_packages(fixture));
        },
        "Accepted invalid data: mismatched package manifests");
    write(fixture / "audio.pak", package({{"extra", bytes("x")}, {"manifest.txt", manifest_bytes}}));
    require_throws(
        [&] {
            static_cast<void>(assets::AssetStore::load_packages(fixture));
        },
        "Accepted invalid data: unreferenced package entry");
    std::filesystem::remove(fixture / png_path);
    const auto outside = fixture.parent_path() / "outside.png";
    write(outside, png);
    std::error_code symlink_error;
    std::filesystem::create_symlink(outside, fixture / png_path, symlink_error);
    // Some standard libraries, including MinGW, do not implement symlink creation.
    if (symlink_error == std::errc::function_not_supported || symlink_error == std::errc::operation_not_supported) {
        std::cout << "Skipped symlink escape check: " << symlink_error.message() << '\n';

        return;
    }

    if (symlink_error) {
        throw std::filesystem::filesystem_error("Cannot create symlink fixture", outside, fixture / png_path, symlink_error);
    }

    const auto error = asset_failure(
        [&] {
            static_cast<void>(assets::AssetStore::load_directory(fixture));
        },
        "symlink escaping loose asset root");
    require(error.cause() == "runtime file escapes asset root", "Symlink must be rejected for escaping the asset root");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 4, "Usage: asset_tests LOOSE_DIR PACKAGE_DIR FIXTURE_DIR");
        Fixtures fixtures(argv[3]);
        auto loose = assets::AssetStore::load_directory(argv[1]);
        auto packed = assets::AssetStore::load_packages(argv[2]);
        equivalent(loose, packed);
        observer_checks(argv[1], fixtures.path / "observers", loose);
        store_failures(argv[1], fixtures.path / "loose", loose);
        std::cout << "MH2 assets: complete loose/package equivalence, preserved native data, ordered observers, transactional loads and corruption checks passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
