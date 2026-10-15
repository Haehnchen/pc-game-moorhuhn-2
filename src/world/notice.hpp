#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace moorhuhn2::world {

namespace notice_type {
inline constexpr int balloon_hero = 100;
inline constexpr int spider_hero = 101;
inline constexpr int zeppelin_killer = 102;
} // namespace notice_type

struct Notice {
    bool active{};
    // Expiry can clear active while the original still draws this update.
    bool visible{};
    float age{};
    float y{};
    int type{};

    bool operator==(const Notice&) const = default;
};

using NoticePool = std::array<Notice, 8>;

struct NoticeText {
    std::string_view text;
    int x{};
    int y{};

    bool operator==(const NoticeText&) const = default;
};

// First inactive slot wins. A full pool is unchanged; the caller owns the
// distinct score/sound consequences of failed multiple-hit and hero requests.
[[nodiscard]] bool allocate_notice(NoticePool& pool, int type, float y);
// Run during composition after depth1/2 particles and before foreground 0/HUD.
// New entries age immediately. Application time controls blinking without RNG.
void update_notice(Notice& notice, float delta, float accumulated_time);
// Unknown types draw no text. Visible entries may already be inactive.
[[nodiscard]] std::optional<NoticeText> notice_text(const Notice& notice);

} // namespace moorhuhn2::world
