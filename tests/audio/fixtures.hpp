#pragma once

#include "assets/asset_store.hpp"
#include "assets/fixtures.hpp"

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace audio_test {

// Keep the original encoded bytes and metadata, but do not repeatedly prepare
// the entire soundtrack for a test that exercises only a few mapped sounds.
inline moorhuhn2::assets::AssetStore load_assets(const std::filesystem::path& root, const std::set<std::string_view>& selected) {
    std::ifstream manifest(root / "manifest.txt");
    std::string image_row;
    std::vector<std::string> audio_rows;
    for (std::string line; std::getline(manifest, line);) {
        std::istringstream fields(line);
        std::string kind, id;
        fields >> kind >> id;
        if (kind == "image" && id == "gfx/mainscreen") {
            image_row = line;
        } else if (kind == "audio" && selected.contains(id)) {
            audio_rows.push_back(line);
        }
    }
    if (image_row.empty() || audio_rows.size() != selected.size()) {
        throw std::runtime_error("Missing original assets for audio fixture");
    }

    asset_test::Fixtures fixture(std::filesystem::temp_directory_path() / "moorhuhn2-audio-tests");
    const auto copy = [&](const std::filesystem::path& path) {
        asset_test::write(fixture.path / path, moorhuhn2::assets::detail::read_file(root / path));
    };
    copy("images/gfx/mainscreen.png");
    for (const auto id : selected) {
        copy("audio/" + std::string(id) + ".mp3");
    }
    std::ostringstream text;
    text << "MH2ASSETS\nimages 1\n" << image_row << "\naudio " << audio_rows.size() << '\n';
    for (const auto& row : audio_rows) {
        text << row << '\n';
    }
    asset_test::write(fixture.path / "manifest.txt", asset_test::bytes(text.str()));
    return moorhuhn2::assets::AssetStore::load_directory(fixture.path);
}

} // namespace audio_test
