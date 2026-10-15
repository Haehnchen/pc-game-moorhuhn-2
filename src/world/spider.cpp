#include "world/spider.hpp"

#include "world/numeric.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

using detail::stored;

bool valid_state(const Spider& spider) {
    if ((spider.type != -1 && spider.type != 20 && spider.type != 60) || !std::isfinite(spider.world_x) || !std::isfinite(spider.world_y) || !std::isfinite(spider.screen_x)
        || !std::isfinite(spider.screen_y) || !std::isfinite(spider.animation_time) || spider.animation_time < 0 || spider.animation_rate != 3 || spider.frame_count != 25
        || !std::isfinite(spider.countdown) || (spider.mode != 0 && spider.mode != 1) || spider.points <= 0) {
        return false;
    }

    if (spider.type == 20) {
        return spider.frame >= 0 && spider.frame <= (spider.mode == 0 ? 24 : 8) && (spider.mode != 0 || spider.countdown == 0);
    }

    if (spider.type == 60) {
        return (spider.countdown == 0 || spider.countdown == 1) && spider.frame >= 0 && spider.frame < (spider.countdown == 0 ? 25 : 8);
    }

    return true;
}

contracts::SpriteDraw draw(const Spider& spider, bool collision) {
    if (!valid_state(spider) || spider.type == -1 || (collision && spider.type != 20)) {
        throw std::invalid_argument("Cannot draw an inactive spider or collide with its reaction");
    }

    const bool first_sheet = collision || (spider.type == 20 && spider.mode == 0);

    return {first_sheet ? "gfx/spinne1" : "gfx/spinne2", {spider.frame * 50, 0, 50, 200}, {static_cast<int>(spider.screen_x), static_cast<int>(spider.screen_y)}, true};
}

} // namespace

void update_spider(Spider& spider, float delta, float camera, int recoil) {
    if (!valid_state(spider) || !detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid spider update state, time, camera or recoil");
    }

    if (spider.type == -1) {
        return;
    }

    auto next = spider;
    const long double elapsed = delta;
    const auto offset = detail::camera_offset(camera, 1.6F);
    next.screen_x = stored(static_cast<long double>(spider.world_x) - offset);
    next.screen_y = stored(static_cast<long double>(spider.world_y) + recoil);

    if (spider.type == 20 && spider.mode == 0) {
        const long double animation = spider.animation_time + static_cast<long double>(spider.animation_rate) * elapsed * 5.0F;

        if (animation >= std::numeric_limits<int>::max()) {
            throw std::invalid_argument("Spider animation exceeds the original signed frame range");
        }

        next.animation_time = stored(animation);
        next.frame = static_cast<int>(animation);

        if (next.frame >= next.frame_count) {
            next.frame = 0;
            next.animation_time = 0;
        }
    } else {
        bool animate = spider.type == 60;

        if (spider.type == 20) {
            const long double countdown = static_cast<long double>(spider.countdown) - elapsed;
            next.countdown = stored(countdown);
            animate = countdown <= 0;
        }

        if (animate) {
            const long double animation = spider.animation_time + elapsed * 50.0F;
            next.animation_time = stored(animation);

            if (animation >= spider.animation_rate) {
                // Retraction and reactions change only one cell per update.
                next.animation_time = stored(animation - spider.animation_rate);

                if (spider.type == 20) {
                    --next.frame;

                    if (next.frame < 0) {
                        next.frame = 0;
                        next.mode = 0;
                        next.countdown = 0;
                    }
                } else {
                    ++next.frame;

                    if (spider.countdown == 0 && next.frame >= next.frame_count) {
                        next.type = 20;
                        next.frame = 0;
                        next.animation_time = 0;
                    } else if (spider.countdown != 0 && next.frame >= 8) {
                        next.type = -1;
                    }
                }
            }
        }
    }

    if (!std::isfinite(next.animation_time) || !std::isfinite(next.countdown) || !std::isfinite(next.screen_x) || !std::isfinite(next.screen_y)) {
        throw std::invalid_argument("Spider update exceeds the original finite field range");
    }

    spider = next;
}

SpiderImpact hit_spider(Spider& spider, int& spider_hits, bool special_ammunition) {
    if (!valid_state(spider) || spider.type != 20 || spider_hits < 0 || (!special_ammunition && spider_hits == std::numeric_limits<int>::max())
        || ((special_ammunition || spider_hits == 2 || spider_hits == 25) && spider.points > std::numeric_limits<int>::max() / 2)) {
        throw std::invalid_argument("Spider impact requires a live spider and valid shared counter/points");
    }

    SpiderImpact result;

    if (special_ammunition || spider_hits == 2 || spider_hits == 25) {
        spider.points *= 2;
        result.points = spider.points;
    } else if (spider_hits == 0) {
        result.points = spider.points;
    }

    if (result.points != 0) {
        const int text_offset = spider.points >= 100 ? -30 : spider.points >= 10 ? -20 : -10;
        result.popup = SpiderPopup{stored(static_cast<long double>(spider.world_x) + 25 + text_offset), stored(static_cast<long double>(spider.world_y) + 100), spider.points};
    }

    result.swings_sound = !special_ammunition;

    if (!special_ammunition) {
        ++spider_hits;
    }

    spider.countdown = special_ammunition ? 1.0F : 0.0F;
    spider.type = 60;
    spider.frame = 0;
    spider.animation_time = 0;

    return result;
}

contracts::SpriteDraw spider_draw(const Spider& spider) {
    return draw(spider, false);
}

contracts::SpriteDraw spider_collision(const Spider& spider) {
    return draw(spider, true);
}

SpiderBonus apply_spider_bonus(int& spider_hits, bool& hero_awarded) {
    if (spider_hits < 0) {
        throw std::invalid_argument("Invalid spider bonus counter");
    }

    if (hero_awarded) {
        return {};
    }

    if (spider_hits == 3) {
        spider_hits = 4;

        return {SpiderBonusKind::first, 0, 16, std::nullopt};
    }

    if (spider_hits == 26) {
        hero_awarded = true;

        return {SpiderBonusKind::hero, 100, 20, Notice{true, false, 0, 180, notice_type::spider_hero}};
    }

    return {};
}

} // namespace moorhuhn2::world
