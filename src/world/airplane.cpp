#include "world/airplane.hpp"

#include "contracts/random.hpp"
#include "world/numeric.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace moorhuhn2::world {
namespace {

static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);

using detail::stored;

void check(const Airplane& airplane) {
    if ((airplane.type != -1 && airplane.type != 18 && airplane.type != 19) || airplane.depth != 4 || !detail::strictly_in_int_range(airplane.world_x)
        || !detail::strictly_in_int_range(airplane.world_y) || !detail::strictly_in_int_range(airplane.screen_x) || !detail::strictly_in_int_range(airplane.screen_y)
        || !std::isfinite(airplane.wait) || !std::isfinite(airplane.animation_time) || airplane.animation_time < 0 || !std::isfinite(airplane.smoke_time) || airplane.smoke_time < 0
        || airplane.frame < 0 || airplane.frame >= 20 || airplane.frame_count != 20 || airplane.animation_rate != 5 || airplane.points != -25) {
        throw std::invalid_argument("Invalid airplane state");
    }
}

void check(const AirplaneState& state) {
    if (state.hits < 0) {
        throw std::invalid_argument("Invalid airplane hit count");
    }
}

void check_update(float delta, float camera, int recoil) {
    if (!detail::valid_update(delta, camera, recoil)) {
        throw std::invalid_argument("Invalid aircraft elapsed time, camera or recoil");
    }
}

void stop_loops(AirplaneState& state, AirplaneEvents& events) {
    if (state.engine_loop) {
        events.push_back({AirplaneEventKind::stop_engine});
        state.engine_loop = false;
    }

    if (state.failure_loop) {
        events.push_back({AirplaneEventKind::stop_failure});
        state.failure_loop = false;
    }
}

void check(const AircraftSmoke& smoke) {
    if ((smoke.type != -1 && smoke.type != 4 && smoke.type != 5) || !detail::strictly_in_int_range(smoke.world_x) || !detail::strictly_in_int_range(smoke.world_y)
        || !detail::strictly_in_int_range(smoke.screen_x) || !detail::strictly_in_int_range(smoke.screen_y) || !std::isfinite(smoke.velocity_x) || !std::isfinite(smoke.velocity_y)
        || !std::isfinite(smoke.age) || smoke.age < 0 || (smoke.frame != 4 && smoke.frame != 5) || (smoke.type >= 0 && smoke.frame != smoke.type)) {
        throw std::invalid_argument("Invalid aircraft smoke state");
    }
}

} // namespace

std::optional<Airplane> make_airplane(AirplaneState& state, const AircraftRandom& random, std::vector<AirplaneEvent>& events) {
    check(state);

    if (state.active) {
        return std::nullopt;
    }

    Airplane airplane;
    const bool right = contracts::draw_random(random) % 2 == 0;
    const int horizontal = static_cast<int>(contracts::draw_random(random) % 200);
    airplane.type = right ? 18 : 19;
    airplane.world_x = right ? static_cast<float>(-120 - horizontal) : static_cast<float>(2400 + horizontal);
    airplane.world_y = static_cast<float>(64 - contracts::draw_random(random) % 64);
    airplane.wait = static_cast<float>(contracts::draw_random(random) % 16);
    state = {true, 0, false, true, false};
    events.push_back({AirplaneEventKind::start_engine});

    return airplane;
}

AirplaneEvents update_airplane(Airplane& airplane, AirplaneState& state, float delta, float camera, int recoil, const AircraftRandom& random) {
    check(airplane);
    check(state);
    check_update(delta, camera, recoil);
    AirplaneEvents events;

    if (airplane.type == -1) {
        return events;
    }

    auto next = airplane;
    auto next_state = state;
    const bool right = airplane.type == 18;
    const int camera_offset = detail::camera_offset(camera, 1.2F);
    const long double moved_x = airplane.world_x + (right ? 1 : -1) * static_cast<long double>(delta) * 150.0F;
    next.world_x = stored(moved_x);
    // The original retains moved_x in x87 after storing world_x; screen_x uses
    // that extended operand. screen_y precedes the crash fall and RNG drift.
    next.screen_x = stored(moved_x - camera_offset);
    next.screen_y = stored(static_cast<long double>(airplane.world_y) + static_cast<long double>(recoil) * 0.6F);
    const float pan = std::clamp(stored((static_cast<long double>(next.screen_x) + 45 - 320) * 0.0015625F), -1.0F, 1.0F);

    if (next_state.engine_loop) {
        events.push_back({AirplaneEventKind::pan_engine, pan});
    }

    if (next_state.failure_loop) {
        events.push_back({AirplaneEventKind::pan_failure, pan});
    }

    if ((right && next.world_x > 2400) || (!right && next.world_x < -120)) {
        next.type = -1;
        next_state.active = false;
        stop_loops(next_state, events);
    }

    // An exit does not bypass the remaining original update instructions.
    if (state.hits > 2) {
        next.world_y = stored(static_cast<long double>(next.world_y) + static_cast<long double>(delta) * 40.0F);
        const int drift = static_cast<int>(contracts::draw_random(random) % 2);
        next.world_x = stored(static_cast<long double>(next.world_x) + (right ? -drift : drift));

        if (next.world_y > 250) {
            next.type = -1;
            next_state.active = false;
            events.push_back({AirplaneEventKind::crash});
            stop_loops(next_state, events);
        }
    }

    const long double wait = static_cast<long double>(next.wait) - delta;
    next.wait = stored(wait);

    if (wait < 0) {
        const long double animation = static_cast<long double>(delta) * 50.0F + next.animation_time;
        next.animation_time = stored(animation);

        if (animation >= next.animation_rate) {
            next.animation_time = stored(animation - next.animation_rate);
            ++next.frame;

            if (next.frame >= next.frame_count) {
                next.frame = 0;
                next.wait = static_cast<float>(contracts::draw_random(random) % 16);
            }
        }
    }

    if (state.hits > 0) {
        const long double smoke = static_cast<long double>(state.hits) * delta * 10.0F + next.smoke_time;
        next.smoke_time = stored(smoke);

        if (smoke > 1) {
            next.smoke_time = stored(smoke - 1);
            events.push_back({AirplaneEventKind::smoke, 0, next.world_x, next.world_y, 0, next.type});
        }
    }

    check(next);
    airplane = next;
    state = next_state;

    return events;
}

std::vector<AirplaneEvent> hit_airplane(Airplane& airplane, AirplaneState& state) {
    check(airplane);
    check(state);

    if (airplane.type == -1 || !state.active || state.hits == std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Airplane impact requires a live airplane");
    }

    std::vector<AirplaneEvent> events{{AirplaneEventKind::shot}};
    ++state.hits;

    if (state.hits > 2 && !state.penalty_applied) {
        airplane.wait = 16;
        state.penalty_applied = true;
        events.push_back({AirplaneEventKind::penalty, 0, stored(static_cast<long double>(airplane.world_x) + 15), stored(static_cast<long double>(airplane.world_y) + 50), -25});

        if (state.engine_loop) {
            events.push_back({AirplaneEventKind::stop_engine});
            state.engine_loop = false;
        }

        events.push_back({AirplaneEventKind::start_failure});
        state.failure_loop = true;
    } else {
        airplane.wait = 0;
    }

    return events;
}

std::optional<contracts::SpriteDraw> airplane_draw(const Airplane& airplane) {
    check(airplane);

    if (airplane.type == -1) {
        return std::nullopt;
    }

    return contracts::SpriteDraw{airplane.type == 18 ? "gfx/rplane" : "gfx/lplane", {airplane.frame % 10 * 90, airplane.frame / 10 * 100, 90, 100},
        {static_cast<int>(airplane.screen_x), static_cast<int>(airplane.screen_y)}, true};
}

std::optional<contracts::SpriteDraw> airplane_collision(const Airplane& airplane) {
    return airplane_draw(airplane);
}

AircraftSmoke make_airplane_smoke(int source_type, float world_x, float world_y, const AircraftRandom& random) {
    if ((source_type != -1 && source_type != 18 && source_type != 19) || !detail::strictly_in_int_range(world_x) || !detail::strictly_in_int_range(world_y)) {
        throw std::invalid_argument("Invalid airplane smoke source");
    }

    AircraftSmoke smoke;
    smoke.type = 5;
    smoke.world_x = stored(static_cast<long double>(world_x) + (source_type == 19 ? 15 : 71));
    smoke.world_y = stored(static_cast<long double>(world_y) + 50);
    smoke.velocity_x = stored(static_cast<long double>(contracts::draw_random(random) % 32) * (source_type == 19 ? 0.02F : -0.02F));
    smoke.velocity_y = stored(static_cast<long double>(static_cast<int>(contracts::draw_random(random) % 20) - 10) * 0.01F);
    check(smoke);

    return smoke;
}

AircraftSmoke make_blimp_smoke(float world_x, float world_y, const AircraftRandom& random) {
    if (!detail::strictly_in_int_range(world_x) || !detail::strictly_in_int_range(world_y)) {
        throw std::invalid_argument("Invalid blimp smoke source");
    }

    AircraftSmoke smoke;
    smoke.type = 4;
    smoke.frame = 4;
    smoke.world_x = world_x;
    smoke.world_y = world_y;
    smoke.velocity_x = stored(static_cast<long double>(static_cast<int>(contracts::draw_random(random) % 64) - 32) * 0.01F);
    smoke.velocity_y = stored(static_cast<long double>(static_cast<int>(contracts::draw_random(random) % 20) - 5) * 0.01F);
    check(smoke);

    return smoke;
}

void update_aircraft_smoke(AircraftSmoke& smoke, float delta, float camera, int recoil) {
    check(smoke);
    check_update(delta, camera, recoil);

    if (smoke.type == -1) {
        return;
    }

    auto next = smoke;
    const int camera_offset = detail::camera_offset(camera, 1.2F);
    const float x_acceleration = smoke.type == 5 ? 0.05F : 0.01F;
    next.velocity_x = stored(static_cast<long double>(smoke.velocity_x) + (smoke.velocity_x > 0 ? 1 : -1) * static_cast<long double>(delta) * x_acceleration);
    next.velocity_y = smoke.type == 5 ? stored(static_cast<long double>(smoke.velocity_y) + (smoke.velocity_y > 0 ? 1 : -1) * static_cast<long double>(delta) * 0.01F)
                                      : stored(static_cast<long double>(smoke.velocity_y) - static_cast<long double>(delta) * 0.2F);
    next.world_x = stored(static_cast<long double>(delta) * next.velocity_x * 100.0F + smoke.world_x);
    next.world_y = stored(static_cast<long double>(delta) * next.velocity_y * 100.0F + smoke.world_y);
    next.screen_x = stored(static_cast<long double>(next.world_x) - camera_offset);
    next.screen_y = stored(static_cast<long double>(recoil) * 0.6F + next.world_y);

    if (smoke.type == 5) {
        // Type5 uses FADD/FST/FCOMP with the EXE's 53-bit precision setting.
        const volatile double age = static_cast<double>(delta) + smoke.age;
        next.age = static_cast<float>(age);

        if (age > 2) {
            next.type = -1;
        }
    } else if (next.world_y < -8) {
        next.type = -1;
    }

    check(next);
    smoke = next;
}

std::optional<contracts::SpriteDraw> aircraft_smoke_draw(const AircraftSmoke& smoke) {
    check(smoke);

    if (smoke.type == -1) {
        return std::nullopt;
    }

    return contracts::SpriteDraw{"gfx/particle2", {0, smoke.frame * 8, 8, 8}, {static_cast<int>(smoke.screen_x), static_cast<int>(smoke.screen_y)}, true};
}

} // namespace moorhuhn2::world
