#include "ui/scores.hpp"

#include "support/require.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using moorhuhn2::test::require;

std::vector<std::uint8_t> read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);

    return {std::istreambuf_iterator<char>(file), {}};
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 3, "Expected original save fixture and scratch path");
        using namespace moorhuhn2;
        const auto original = read(argv[1]);
        ui::Scores scores;
        scores.decode(original);
        require(scores.encode() == original && original.size() == 88, "Exact supplied five-record 88-byte SAV round trip");
        require(!scores.insert(25, 0, 0), "Tying last place must not qualify");
        const auto row = scores.insert(100, 2, 1);
        require(row == 3 && scores.entries()[3].accuracy == 66, "Equal 100 follows existing 100; truncating accuracy");
        require(scores.type(*row, 'A') && !scores.type(*row, '9') && !scores.type(*row, ' '), "Name accepts ASCII letters only");
        for (int i = 0; i < 7; ++i) {
            require(scores.type(*row, 'b'), "Eight letters allowed");
        }

        require(!scores.type(*row, 'c') && scores.backspace(*row), "Ninth letter rejected, backspace works");
        const auto before = scores.encode();
        for (std::size_t size = 0; size < original.size(); ++size) {
            bool rejected = false;
            try {
                scores.decode(std::span(original).first(size));
            } catch (const std::runtime_error&) {
                rejected = true;
            }

            require(rejected && scores.encode() == before, "Truncated save cannot partially replace table");
        }

        auto invalid = original;
        invalid[8] = 255;
        const auto reject_decode = [&](const auto& bytes, const char* message) {
            bool rejected = false;
            try {
                scores.decode(bytes);
            } catch (const std::runtime_error&) {
                rejected = true;
            }

            require(rejected && scores.encode() == before, message);
        };
        reject_decode(invalid, "Unbounded original length leaves the destination unchanged");
        invalid = original;
        invalid.push_back(0);
        reject_decode(invalid, "Trailing save byte leaves the destination unchanged");
        invalid = original;
        // Fail in the final name after all earlier records have decoded.
        invalid.back() = static_cast<std::uint8_t>('0') ^ 0xf0;
        reject_decode(invalid, "Malformed final decoded name cannot partially replace the table");
        const auto replace_word = [&](std::size_t offset, std::uint32_t value) {
            for (unsigned shift = 0; shift < 32; shift += 8) {
                invalid[offset++] = static_cast<std::uint8_t>(value >> shift);
            }
        };
        invalid = original;
        replace_word(4, 101U ^ 0xff8888ffU);
        reject_decode(invalid, "Decoded accuracy above 100 leaves the destination unchanged");
        invalid = original;
        replace_word(0, 0x80000000U ^ 0xaa5533ffU);
        reject_decode(invalid, "Decoded score above signed range leaves the destination unchanged");
        invalid = original;
        replace_word(0, 0U ^ 0xaa5533ffU);
        reject_decode(invalid, "Ascending score pair leaves the destination unchanged");
        const auto zero = scores.insert(251, 0, 0);
        require(zero == 0 && scores.entries()[0].accuracy == 0, "Defined native zero-shot percentage");
        const std::filesystem::path scratch(argv[2]);
        std::filesystem::create_directories(scratch);
        const auto before_missing = scores.encode();
        scores.load(scratch / "missing.sav");
        require(scores.encode() == before_missing, "Missing save file keeps the current table");
        const auto oversized = scratch / "oversized.sav";
        {
            std::ofstream output(oversized, std::ios::binary | std::ios::trunc);
            const std::string payload(101, '\0');
            output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
            output.close();
            require(static_cast<bool>(output), "Oversized save fixture write");
        }

        const auto before_load = scores.encode();
        bool rejected = false;
        try {
            scores.load(oversized);
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        require(rejected && scores.encode() == before_load, "Oversized save file leaves the destination unchanged");
        const auto save = scratch / std::filesystem::path(u8"分数.sav");
        scores.save(save);
        require(scores.insert(999, 3, 1) == 0, "Replacement save includes the latest score");
        scores.save(save);
        ui::Scores restarted;
        restarted.load(save);
        require(restarted.encode() == scores.encode() && !std::filesystem::exists(std::filesystem::path(save).concat(".tmp")), "Atomic save/restart");
        const auto directory_target = scratch / "cannot-replace-directory";
        std::filesystem::create_directories(directory_target);
        rejected = false;
        try {
            scores.save(directory_target);
        } catch (const std::exception&) {
            rejected = true;
        }

        require(rejected && std::filesystem::is_directory(directory_target) && !std::filesystem::exists(directory_target.string() + ".tmp"),
            "Failed replacement retains destination and removes temporary");
        std::cout << "Highscore table, name entry and storage checks passed\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
