#include "world/notice.hpp"

#include "world/numeric.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {

static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);

bool allocate_notice(NoticePool& pool, int type, float y) {
    if (!std::isfinite(y)) {
        throw std::invalid_argument("Notice allocation requires a finite position");
    }

    const auto vacant = std::ranges::find_if(pool, [](const auto& notice) {
        return !notice.active;
    });

    if (vacant == pool.end()) {
        return false;
    }

    *vacant = {true, false, 0, y, type};

    return true;
}

void update_notice(Notice& notice, float delta, float accumulated_time) {
    if (!std::isfinite(notice.age) || notice.age < 0 || !std::isfinite(notice.y) || !std::isfinite(delta) || delta < 0 || !std::isfinite(accumulated_time) || accumulated_time < 0
        || static_cast<long double>(accumulated_time) * 60 >= std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Invalid notice state, delta or accumulated sample");
    }

    if (!notice.active) {
        notice.visible = false;
        return;
    }

    // FADD uses the EXE's 53-bit precision; FST leaves this sum for FCOMP.
    const volatile double age = static_cast<double>(notice.age) + delta;
    const float y = static_cast<float>(static_cast<long double>(notice.y) - static_cast<long double>(delta) * 40.0F);

    if (!std::isfinite(static_cast<float>(age)) || !std::isfinite(y)) {
        throw std::invalid_argument("Notice update exceeds the original finite field range");
    }

    notice.age = static_cast<float>(age);
    notice.active = age <= 3;
    notice.y = y;
    notice.visible = (static_cast<int>(static_cast<long double>(accumulated_time) * 60) & 31) < 16;
}

std::optional<NoticeText> notice_text(const Notice& notice) {
    if (!notice.visible) {
        return std::nullopt;
    }

    std::string_view text;
    int x{};

    switch (notice.type) {
        case 3:
            text = "TRIPLE HIT +5";
            x = 200;
            break;
        case 4:
            text = "UNBELIEVABLE +10";
            x = 170;
            break;
        case 5:
            text = "MOORHUHN MASTER +15";
            x = 140;
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            text = "I'M NOT WORTHY +20";
            x = 150;
            break;
        case notice_type::balloon_hero:
            text = "BALLOON HERO +100";
            x = 160;
            break;
        case notice_type::spider_hero:
            text = "SPIDER HERO +150";
            x = 170;
            break;
        case notice_type::zeppelin_killer:
            text = "ZEPPELIN KILLER";
            x = 170;
            break;
        default:
            return std::nullopt;
    }

    if (!detail::in_int_range(notice.y)) {
        throw std::invalid_argument("Notice text position exceeds the original signed integer range");
    }

    return NoticeText{text, x, static_cast<int>(notice.y)};
}

} // namespace moorhuhn2::world
