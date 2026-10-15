#include "assets/asset_package.hpp"

#include "fixtures.hpp"
#include "support/require.hpp"

#include <array>
#include <functional>
#include <iostream>
#include <ranges>
#include <stdexcept>

namespace {
using namespace asset_test;
using moorhuhn2::test::require;
using moorhuhn2::test::require_throws;

void package_checks(const std::filesystem::path& root) {
    require(detail::crc32(bytes("123456789")) == 0xcbf43926U, "CRC32 check vector");

    for (const auto& name : std::array{"../x", "x/../y", "/x", "x//y", "x/", "a\\b", "a:b", "a..b", "NUL.txt", "com1.mp3", "x/./y", "x\n"}) {
        require(!detail::safe_path(name), "Accepted unsafe path");
    }

    require(detail::safe_path("images/gfx/M2BG01.png"), "Rejected source-derived safe path");
    const auto valid = package({{"aa", bytes("alpha")}, {"bb", bytes("beta")}});
    const auto path = root / "fixture.pak";
    write(path, valid);
    const detail::AssetPackage loaded(path);
    require(std::ranges::equal(loaded.read("aa"), bytes("alpha")), "Package payload differs");
    require_throws(
        [&] {
            static_cast<void>(loaded.read("missing"));
        },
        "Accepted invalid data: missing entry");
    const auto corrupt = [&](std::string cause, const std::function<void(Bytes&)>& change) {
        auto data = valid;
        change(data);
        write(path, data);
        require_throws(
            [&] {
                static_cast<void>(detail::AssetPackage(path));
            },
            "Accepted invalid data: " + cause);
    };
    corrupt("magic", [](Bytes& data) {
        data[0] = 0;
    });
    corrupt("zero count", [](Bytes& data) {
        number(data, 8, 0, 4);
    });
    corrupt("oversized count", [](Bytes& data) {
        number(data, 8, 257, 4);
    });
    corrupt("index beyond EOF", [](Bytes& data) {
        number(data, 12, 0xffffffffU, 4);
    });
    corrupt("truncated entry", [](Bytes& data) {
        number(data, 12, 10, 4);
    });
    corrupt("long name", [](Bytes& data) {
        number(data, 16, 241, 2);
    });
    corrupt("unsafe name", [](Bytes& data) {
        data[38] = '/';
    });
    corrupt("payload before index", [](Bytes& data) {
        number(data, 18, 1, 8);
    });
    corrupt("huge length", [](Bytes& data) {
        number(data, 26, ~std::uint64_t{}, 8);
    });
    corrupt("overlapping payload", [](Bytes& data) {
        number(data, 42, 64, 8);
    });
    corrupt("checksum", [](Bytes& data) {
        data.back() ^= 1;
    });
    corrupt("truncation", [](Bytes& data) {
        data.pop_back();
    });
    corrupt("extra data", [](Bytes& data) {
        data.push_back(0);
    });

    for (const auto& files : std::array{std::vector<std::pair<std::string, Bytes>>{{"aa", bytes("a")}, {"aa", bytes("b")}},
             std::vector<std::pair<std::string, Bytes>>{{"bb", bytes("a")}, {"aa", bytes("b")}}}) {
        write(path, package(files));
        require_throws(
            [&] {
                static_cast<void>(detail::AssetPackage(path));
            },
            "Accepted invalid data: duplicate/unordered names");
    }

    write(path, bytes("short"));
    require_throws(
        [&] {
            static_cast<void>(detail::AssetPackage(path));
        },
        "Accepted invalid data: short header");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: asset_package_tests FIXTURE_DIR");
        Fixtures fixtures(argv[1]);
        package_checks(fixtures.path);
        std::cout << "Asset package bounds, paths, CRC and payload checks passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
