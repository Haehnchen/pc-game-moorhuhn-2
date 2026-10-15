#include "../../game/persistent_leaves_support.hpp"
#include "../../game/round_clock.hpp"
#include "game/round.hpp"
#include "support/numeric.hpp"
#include "support/require.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace moorhuhn2;

namespace {
using moorhuhn2::test::require;

using moorhuhn2::test::binary32;

struct SpawnRecord {
    std::size_t slot;
    std::uint32_t world_x_bits;
    std::uint32_t screen_x_bits;
    std::uint32_t pan_bits;
};

struct DyingBalloon {
    std::size_t slot;
    std::uint32_t world_x_bits;
};

struct GrantExpectation {
    game::test::SourceRandom random;
    std::vector<SpawnRecord> records;
    std::vector<DyingBalloon> dying_balloons;
};

struct PredicateCase {
    std::string id;
    int hits;
    bool latched;
    int score;
    bool expected_latch;
    int expected_score;
    std::size_t grants;
};

struct AllocationCase {
    std::uint32_t seed;
    std::uint32_t camera_bits;
    std::size_t scene_capacity;
    std::vector<std::size_t> free_slots;
    std::vector<std::size_t> dying_slots;
    GrantExpectation expected;
};

struct NoticeExpectation {
    std::size_t slot;
    std::uint32_t age_bits;
    std::uint32_t y_bits;
    bool visible;
};

struct NoticeCase {
    std::vector<std::size_t> free_slots;
    float initial_age;
    std::uint32_t delta_bits;
    std::optional<NoticeExpectation> notice;
};

struct InputCounters {
    int score;
    int hits;
    int misses;
    int ammo_index;
};

struct HitSequence {
    std::uint32_t seed;
    contracts::ScreenPoint pointer;
    int initial_balloons_hit;
    InputCounters before;
    InputCounters after_hit;
    int hit_balloons;
    game::test::SourceRandom hit_random;
    GrantExpectation grant;
    InputCounters reload;
    game::test::SourceRandom reload_random;
};

struct StoneGrantCase {
    std::vector<std::size_t> free_slots;
    std::uint64_t expected_random_draws;
};

const std::array<PredicateCase, 9> predicate_cases = {{PredicateCase{"twenty_four", 24, false, 37, false, 37, 0U}, PredicateCase{"exact_twenty_five", 25, false, 37, true, 137, 1U},
    PredicateCase{"twenty_six_does_not_recover_skipped_count", 26, false, 37, false, 37, 0U}, PredicateCase{"negative_synthetic_counter", -1, false, 37, false, 37, 0U},
    PredicateCase{"already_latched", 25, true, 137, true, 137, 0U}, PredicateCase{"award_before_negative_clamp", 25, false, -50, true, 50, 1U},
    PredicateCase{"still_negative_after_award", 25, false, -101, true, 0, 1U}, PredicateCase{"largest_nonoverflowing_award", 25, false, 2147483547, true, 2147483647, 1U},
    PredicateCase{"signed_wrap_then_clamp", 25, false, 2147483548, true, 0, 1U}}};

const std::array<AllocationCase, 7> allocation_cases = {
    {AllocationCase{1U, 0x44200000U, 128U, {34U, 35U, 36U, 37U, 38U, 39U}, {},
         GrantExpectation{game::test::SourceRandom{1030492215U, 6ULL},
             {SpawnRecord{34U, 0x42240000U, 0xc475c000U, 0xbf800000U}, SpawnRecord{35U, 0x43918000U, 0xc4374000U, 0xbf800000U},
                 SpawnRecord{36U, 0x44dfc000U, 0x443f8000U, 0x3f706667U}, SpawnRecord{37U, 0x44bc8000U, 0x43f20000U, 0x3eff3333U},
                 SpawnRecord{38U, 0x44784000U, 0xc1f80000U, 0xbe9ccccdU}, SpawnRecord{39U, 0x4502c000U, 0x44858000U, 0x3f800000U}},
             {}}},
        AllocationCase{22U, 0x437a0000U, 128U, {34U, 70U, 127U}, {},
            GrantExpectation{game::test::SourceRandom{3525976323U, 3ULL},
                {SpawnRecord{34U, 0x42dc0000U, 0xc3910000U, 0xbf360000U}, SpawnRecord{70U, 0x44930000U, 0x44420000U, 0x3f746667U},
                    SpawnRecord{127U, 0x44128000U, 0x433a0000U, 0x3d066667U}},
                {}}},
        AllocationCase{1U, 0x44200000U, 128U, {}, {}, GrantExpectation{game::test::SourceRandom{1U, 0ULL}, {}, {}}},
        AllocationCase{4294967295U, 0x00000000U, 128U, {35U, 37U, 50U, 61U, 90U, 95U, 100U, 127U}, {},
            GrantExpectation{game::test::SourceRandom{515807877U, 6ULL},
                {SpawnRecord{35U, 0x420c0000U, 0x420c0000U, 0xbe500000U}, SpawnRecord{37U, 0x434b0000U, 0x434b0000U, 0x3d733333U},
                    SpawnRecord{50U, 0x4489c000U, 0x4489c000U, 0x3f800000U}, SpawnRecord{61U, 0x45005000U, 0x45005000U, 0x3f800000U},
                    SpawnRecord{90U, 0x44dd8000U, 0x44dd8000U, 0x3f800000U}, SpawnRecord{95U, 0x4483c000U, 0x4483c000U, 0x3f800000U}},
                {}}},
        AllocationCase{1U, 0x44200000U, 128U, {128U, 150U, 255U}, {}, GrantExpectation{game::test::SourceRandom{1U, 0ULL}, {}, {}}},
        AllocationCase{1U, 0x44200000U, 256U, {35U, 90U, 127U, 128U, 150U, 255U}, {},
            GrantExpectation{game::test::SourceRandom{1030492215U, 6ULL},
                {SpawnRecord{35U, 0x42240000U, 0xc475c000U, 0xbf800000U}, SpawnRecord{90U, 0x43918000U, 0xc4374000U, 0xbf800000U},
                    SpawnRecord{127U, 0x44dfc000U, 0x443f8000U, 0x3f706667U}, SpawnRecord{128U, 0x44bc8000U, 0x43f20000U, 0x3eff3333U},
                    SpawnRecord{150U, 0x44784000U, 0xc1f80000U, 0xbe9ccccdU}, SpawnRecord{255U, 0x4502c000U, 0x44858000U, 0x3f800000U}},
                {}}},
        AllocationCase{1U, 0x44200000U, 128U, {34U, 37U, 54U, 90U, 100U, 127U}, {35U, 70U},
            GrantExpectation{game::test::SourceRandom{1924036713U, 8ULL},
                {SpawnRecord{34U, 0x44dfc000U, 0x443f8000U, 0x3f706667U}, SpawnRecord{37U, 0x44bc8000U, 0x43f20000U, 0x3eff3333U},
                    SpawnRecord{54U, 0x44784000U, 0xc1f80000U, 0xbe9ccccdU}, SpawnRecord{90U, 0x4502c000U, 0x44858000U, 0x3f800000U},
                    SpawnRecord{100U, 0x42ec0000U, 0xc4628000U, 0xbf800000U}, SpawnRecord{127U, 0x4502e000U, 0x4485c000U, 0x3f800000U}},
                {DyingBalloon{35U, 0x44960000U}, DyingBalloon{70U, 0x44960000U}}}}}};

const std::array<NoticeCase, 4> notice_cases = {{NoticeCase{{2U, 5U, 7U}, 0.0F, 0x00000000U, NoticeExpectation{2U, 0x00000000U, 0x43340000U, true}},
    NoticeCase{{7U}, 0.0F, 0x3e000000U, NoticeExpectation{7U, 0x3e000000U, 0x432f0000U, true}}, NoticeCase{{}, 0.0F, 0x00000000U, std::nullopt},
    NoticeCase{{}, 3.0F, 0x3e000000U, std::nullopt}}};

const HitSequence expected_hit_sequence =
    HitSequence{22U, contracts::ScreenPoint{300, 198}, 24, InputCounters{37, 7, 3, 15}, InputCounters{38, 8, 3, 16}, 25, game::test::SourceRandom{3048797603U, 35ULL},
        GrantExpectation{game::test::SourceRandom{3893719112U, 74ULL},
            {SpawnRecord{35U, 0x450ce000U, 0x4499c000U, 0x3f800000U}, SpawnRecord{36U, 0x44d74000U, 0x442e8000U, 0x3f553333U},
                SpawnRecord{37U, 0x44bb8000U, 0x43ee0000U, 0x3ef8cccdU}, SpawnRecord{38U, 0x446d0000U, 0xc2980000U, 0xbec0cccdU},
                SpawnRecord{39U, 0x44b0a000U, 0x43c28000U, 0x3eb33333U}, SpawnRecord{40U, 0x44cea000U, 0x441d4000U, 0x3f39999aU}},
            {}},
        InputCounters{138, 8, 3, 8}, game::test::SourceRandom{2231975128U, 90ULL}};

const StoneGrantCase expected_earlier_stone_grant = StoneGrantCase{{34U, 37U, 54U, 90U, 100U, 127U}, 30ULL};

game::RoundState& fixture(game::Round& round) {
    auto& state = const_cast<game::RoundState&>(round.state());
    // These vectors isolate the grant. A dedicated case restores dying balloons
    // to verify their earlier scene-update draws in the shared CRT stream.
    state.frog.type = -1;
    state.hidden_chicken.type = -1;
    state.spider.type = -1;
    state.spawn_accumulator = -10000;
    state.random_state = 1;
    state.random_draws = 0;
    for (auto& target : state.chickens) {
        target.type = -2;
    }
    return state;
}

void tick(game::Round& round, float delta = 0, game::RoundInput input = {}) {
    game::test::advance(round, round.state().previous_sample + delta, input);
    round.resolve_actions({});
}

std::size_t event_count(const game::Round& round, game::RoundEventKind kind) {
    return static_cast<std::size_t>(std::ranges::count(round.events(), kind, &game::RoundEvent::kind));
}

void magic(const game::RoundEvent& event) {
    require(event.kind == game::RoundEventKind::balloon_hero && event.slot == -1 && event.depth == 0 && event.points == 100,
        "BALLOON HERO emits the source magic sample before its allocations");
}

void records(const game::Round& round, const GrantExpectation& expected) {
    const auto& state = round.state();
    require(state.random_state == expected.random.state && state.random_draws == expected.random.draws, "Grant preserves the literal ordered CRT state");
    std::size_t index = 1;
    magic(round.events().front());
    for (const auto& row : expected.records) {
        const auto slot = row.slot;
        const auto& target = state.big_chickens[slot];
        require(state.chickens[slot].type == -5 && target.type == 4 && target.world_x == binary32(row.world_x_bits) && target.world_y == 100
                    && target.screen_x == binary32(row.screen_x_bits) && target.screen_y == -1024 && target.animation_time == 0 && target.frame == 0 && target.frame_count == 15
                    && target.animation_rate == 5 && target.points == 25,
            "Successful explicit type4 allocation uses its literal source position and unupdated vertical pose");
        const auto& spawn = round.events()[index++];
        const auto& sound = round.events()[index++];
        require(spawn.kind == game::RoundEventKind::spawn && spawn.slot == static_cast<int>(slot) && spawn.depth == 1 && spawn.points == 25
                    && sound.kind == game::RoundEventKind::big_chicken_appear && sound.slot == static_cast<int>(slot) && sound.depth == 1 && sound.points == 0
                    && sound.pan == binary32(row.pan_bits),
            "Each first-free success emits spawn then its original appearance sound/pan");
    }
    require(round.events().size() == index, "Grant has no additional score sound or particle event");
}

void predicates(std::span<const PredicateCase> rows) {
    for (const auto& row : rows) {
        for (const bool special : {false, true}) {
            for (const int ammo_index : {8, 16}) {
                game::Round round(1, 0);
                auto& state = fixture(round);
                state.balloons_hit = row.hits;
                state.balloon_hero = row.latched;
                state.score = row.score;
                state.hits = 7;
                state.misses = 3;
                state.ammo_index = ammo_index;
                state.reload_start_index = 6;
                state.branch_ammo_granted = true;
                state.special_ammo = special;
                state.remaining = 73;
                for (auto& notice : state.notices) {
                    notice = {true, false, 0, 200, 3};
                }
                const auto cartridges = state.ammo;
                const auto particles = state.particles;
                const auto random = game::test::after_leaves(game::test::snapshot(state));
                tick(round);
                require(state.balloon_hero == row.expected_latch && state.score == row.expected_score && event_count(round, game::RoundEventKind::balloon_hero) == row.grants,
                    "Exact 25/latch and original award-before-clamp predicate: " + row.id);
                require(state.balloons_hit == row.hits && state.hits == 7 && state.misses == 3 && state.ammo_index == ammo_index && state.reload_start_index == 6
                            && state.ammo == cartridges && state.special_ammo == special && state.remaining == 73 && state.particles == particles
                            && state.random_draws == random.draws && state.random_state == random.state,
                    "Grant has no ammunition/time/accuracy/particle prerequisite or side effect, including with both pools full");
                if (row.grants == 1) {
                    magic(round.events().front());
                }
                require(round.events().size() == row.grants, "Full scene cannot emit spawn or appearance sounds");
            }
        }
    }
}

void allocations(std::span<const AllocationCase> rows) {
    for (const auto& row : rows) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.balloons_hit = 25;
        state.random_state = row.seed;
        state.camera = binary32(row.camera_bits);
        state.scene_capacity = row.scene_capacity;
        for (const auto& slot : row.free_slots) {
            state.chickens[slot].type = -1;
        }
        for (const auto& slot : row.dying_slots) {
            const auto index = slot;
            state.chickens[index].type = -3;
            state.balloons[index] = {.type = 61, .world_x = 1200, .world_y = 100, .frame = 5, .points = 1};
        }
        // The accepted fixture begins at the dying-balloon + grant boundary.
        game::RoundTestAccess::update_balloons(round);
        game::RoundTestAccess::grant_balloon_hero(round);
        records(round, row.expected);
        require(state.balloon_hero && state.score == 100 && state.notices[0].type == 100 && state.notices[0].age == 0 && state.notices[0].y == 180,
            "Scene shortage preserves the latch, score and independent notice request");
        require(std::ranges::count(state.big_chickens, 4, &world::BigChicken::type) == static_cast<std::ptrdiff_t>(row.expected.records.size()),
            "Six requests never allocate a seventh target or scan beyond the active capacity");
        for (const auto& corpse : row.expected.dying_balloons) {
            const auto index = corpse.slot;
            require(state.chickens[index].type == -3 && state.balloons[index].type == 61 && state.balloons[index].world_x == binary32(corpse.world_x_bits),
                "Retained dying balloons occupy their slots and consume their prior ordered scene draws");
        }
        const auto awarded_score = state.score;
        for (auto& target : state.chickens) {
            target.type = -1;
        }
        for (auto& target : state.big_chickens) {
            target.type = -1;
        }
        for (auto& balloon : state.balloons) {
            balloon.type = -1;
        }
        state.notices = {};
        const auto random = game::test::after_leaves(game::test::snapshot(state));
        tick(round);
        require(state.score == awarded_score && state.random_draws == random.draws && state.random_state == random.state && round.events().empty()
                    && state.notices == world::NoticePool{},
            "The latch consumes failed requests permanently; later free scene/notice slots cannot retry the grant");
    }
}

void notice_pools(std::span<const NoticeCase> rows) {
    for (const auto& row : rows) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.balloons_hit = 25;
        for (auto& notice : state.notices) {
            notice = {true, false, row.initial_age, 200, 3};
        }
        for (const auto& slot : row.free_slots) {
            state.notices[slot] = {false, true, 2, 12, 102};
        }
        const auto random = game::test::after_leaves(game::test::snapshot(state));
        tick(round, binary32(row.delta_bits));
        require(state.score == 100 && state.balloon_hero && state.random_draws == random.draws && state.random_state == random.state && round.events().size() == 1,
            "Notice exhaustion never suppresses score or magic");
        magic(round.events().front());
        const int slot = row.notice ? static_cast<int>(row.notice->slot) : -1;
        if (slot >= 0) {
            const auto& notice = state.notices[static_cast<std::size_t>(slot)];
            require(notice.active && notice.type == 100 && notice.age == binary32(row.notice->age_bits) && notice.y == binary32(row.notice->y_bits)
                        && notice.visible == row.notice->visible,
                "First inactive notice is overwritten, then ages/moves during the same composition update");
            const auto text = world::notice_text(notice);
            require(text && text->text == "BALLOON HERO +100" && text->x == 160 && text->y == static_cast<int>(notice.y), "Grant uses the accepted notice100 text and anchor");
        } else {
            require(std::ranges::none_of(state.notices,
                        [](const auto& notice) {
                            return notice.type == 100;
                        }),
                "Entries expiring during composition were still occupied at the earlier grant");
            tick(round);
            require(state.score == 100 && round.events().empty(), "Notice failure cannot defer or repeat an already-latched grant");
        }
    }
}

void deferred_input(const HitSequence& sequence) {
    const auto prime = [&](game::Round& round, int ammo_index) -> game::RoundState& {
        auto& state = fixture(round);
        state.random_state = sequence.seed;
        state.balloons_hit = sequence.initial_balloons_hit;
        state.score = sequence.before.score;
        state.hits = sequence.before.hits;
        state.misses = sequence.before.misses;
        state.ammo_index = ammo_index;
        state.chickens[34].type = -3;
        state.balloons[34] = {.type = 21, .world_x = 1200, .world_y = 200, .frame = 0, .points = 1};
        for (const auto& record : sequence.grant.records) {
            state.chickens[record.slot].type = -1;
        }
        return state;
    };
    const auto select_balloon = [](const auto& draw, auto) {
        return draw.image == "gfx/ballon";
    };
    const contracts::ScreenPoint pointer = sequence.pointer;
    game::Round round(1, 0);
    auto& state = prime(round, sequence.before.ammo_index);
    game::test::advance(round, 0, {.pointer = pointer, .fire_down = true});
    require(!state.balloon_hero && state.balloons_hit == 24 && round.events().empty(), "The pre-input update cannot anticipate the 25th hit");
    round.resolve_actions(select_balloon);
    const auto& hit = sequence.after_hit;
    require(!state.balloon_hero && state.selected_slot == 34 && state.balloons[34].type == 61 && state.balloons_hit == sequence.hit_balloons && state.score == hit.score
                && state.hits == hit.hits && state.misses == hit.misses && state.ammo_index == hit.ammo_index && state.random_state == sequence.hit_random.state
                && state.random_draws == sequence.hit_random.draws && round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::shot
                && round.events()[1].kind == game::RoundEventKind::balloon_hit,
        "The 25th impact grants only its ordinary points/sound/accuracy before the next update");
    game::test::advance(round, 0, {.pointer = pointer, .fire_down = true, .reload_down = true});
    records(round, sequence.grant);
    require(state.balloon_hero && state.score == 138 && state.ammo_index == 16 && state.hits == 8 && state.misses == 3,
        "The next advance grants 100 points before its empty-fire and reload actions");
    round.resolve_actions(select_balloon);
    const auto& reload = sequence.reload;
    require(state.score == reload.score && state.hits == reload.hits && state.misses == reload.misses && state.ammo_index == reload.ammo_index
                && state.random_state == sequence.reload_random.state && state.random_draws == sequence.reload_random.draws
                && round.events()[round.events().size() - 2].kind == game::RoundEventKind::empty && round.events().back().kind == game::RoundEventKind::reload,
        "Empty fire/reload follows the complete magic/spawn sequence without changing hero points or accuracy");
    tick(round);
    for (const auto& record : sequence.grant.records) {
        const auto& target = state.big_chickens[record.slot];
        require(target.type == 4 && target.screen_y == 100 && target.animation_time == 0, "New big chickens acquire their first visible pose on the following scene update");
    }
    require(event_count(round, game::RoundEventKind::balloon_hero) == 0 && state.score == 138, "Following updates cannot repeat magic or points");

    game::Round next_hit(1, 0);
    auto& next = prime(next_hit, 8);
    next.chickens[41].type = -3;
    next.balloons[41] = {.type = 21, .world_x = 1220, .world_y = 200, .frame = 1, .points = 2};
    game::test::advance(next_hit, 0, {.pointer = pointer, .fire_down = true});
    next_hit.resolve_actions(select_balloon);
    game::test::advance(next_hit, 0, {.pointer = {320, 198}, .fire_down = true});
    require(next.balloon_hero && next.balloons_hit == 25 && next.score == 138, "Exact 25 grant runs before the next input can increment the counter");
    next_hit.resolve_actions(select_balloon);
    require(next.selected_slot == 41 && next.balloons_hit == 26 && next.score == 140 && next.hits == 9 && next.misses == 3 && next.ammo_index == 10,
        "The 26th hit retains the completed hero reward and adds only its own ordinary award");
    tick(next_hit);
    require(next.score == 140 && event_count(next_hit, game::RoundEventKind::balloon_hero) == 0, "Count 26 and the latch prevent a repeated grant");
}

void ordering(const StoneGrantCase& stone_case) {
    game::Round stone(1, 0);
    auto& s = fixture(stone);
    s.stones_hit = 8;
    s.balloons_hit = 25;
    for (const auto& slot : stone_case.free_slots) {
        s.chickens[slot].type = -1;
    }
    auto random = game::test::after_leaves(game::test::snapshot(s));
    random.discard(stone_case.expected_random_draws);
    tick(stone);
    require(s.special_ammo && s.balloon_hero && s.score == 100 && s.random_draws == random.draws && s.random_state == random.state
                && std::ranges::count(s.balloons, 21, &world::Balloon::type) == 6
                && std::ranges::none_of(s.big_chickens,
                    [](const auto& target) {
                        return target.type >= 0;
                    })
                && stone.events().size() == 2 && stone.events()[0].kind == game::RoundEventKind::special_ammo && stone.events()[1].kind == game::RoundEventKind::balloon_hero,
        "Earlier eight-stone allocations consume scarce scene capacity and RNG before the hero's six attempts");

    game::Round spider(1, 0);
    auto& p = fixture(spider);
    p.spider_hits = 26;
    p.balloons_hit = 25;
    for (std::size_t index = 0; index < 7; ++index) {
        p.notices[index] = {true, false, 0, 200, 3};
    }
    tick(spider);
    require(p.spider_hero && p.balloon_hero && p.score == 200 && p.notices[7].type == 101 && spider.events().size() == 3
                && spider.events()[0].kind == game::RoundEventKind::spider_balloons && spider.events()[1].kind == game::RoundEventKind::spider_hero
                && spider.events()[2].kind == game::RoundEventKind::balloon_hero,
        "Earlier spider hero takes the last notice slot; balloon hero still awards 100 points and magic");

    for (const bool one_notice_slot : {false, true}) {
        game::Round later(1, 0);
        auto& l = fixture(later);
        l.frog_hits = 1;
        l.stones_hit = 4;
        l.remaining = 59;
        l.balloons_hit = 25;
        l.branches_hit = 4;
        l.blimp_progress.hits = 2;
        l.multi_hit.timers[0] = .125F;
        l.multi_hit.timers[1] = 1;
        l.multi_hit.timers[2] = 1;
        l.multi_hit.write_index = 3;
        if (one_notice_slot) {
            for (std::size_t index = 0; index < 7; ++index) {
                l.notices[index] = {true, false, 0, 200, 3};
            }
        }
        tick(later, .125F);
        require(l.remaining == 63.875F && l.frog_hits == 2 && l.balloon_hero && l.branch_ammo_granted && l.reload_start_index == 6 && l.blimp_progress.alternate
                    && l.multi_hit.write_index == 0 && later.events()[0].kind == game::RoundEventKind::frog_time_bonus
                    && later.events()[1].kind == game::RoundEventKind::balloon_hero && later.events()[2].kind == game::RoundEventKind::blimp_killer,
            "Hero follows frog-time and precedes branch-ammo, blimp and multiple-hit processing");
        if (one_notice_slot) {
            require(l.score == 100 && l.notices[7].type == 100 && later.events().size() == 3, "Hero takes the last notice slot before blimp and suppressible multiple-hit bonuses");
        } else {
            require(l.score == 105 && l.notices[0].type == 100 && l.notices[0].y == 175 && l.notices[1].type == 102 && l.notices[2].type == 3 && l.notices[2].y == 155
                        && later.events().size() == 4 && later.events()[3].kind == game::RoundEventKind::multi_hit_bonus,
                "Available notices and awards preserve hero, blimp, triple priority in the same update");
        }
    }
}

void exit_and_reset() {
    for (const bool cancel : {false, true}) {
        game::Round round(1, 0);
        auto& state = fixture(round);
        state.balloons_hit = 25;
        state.remaining = cancel ? 90 : .125F;
        state.countdown_second = cancel ? 0 : 1;
        tick(round, .125F, {.cancel = cancel});
        require(state.exit_requested && state.balloon_hero && state.score == 100 && round.events().size() == 2 && round.events()[0].kind == game::RoundEventKind::game_over
                    && round.events()[1].kind == game::RoundEventKind::balloon_hero,
            "Timer/cancel stage change still completes the current hunting dispatch and hero grant");
    }
    game::Round fresh(1, 0);
    require(!fresh.state().balloon_hero && fresh.state().balloons_hit == 0 && fresh.state().score == 0 && fresh.state().hits == 0 && fresh.state().misses == 0
                && fresh.state().notices == world::NoticePool{}
                && std::ranges::all_of(fresh.state().big_chickens,
                    [](const auto& target) {
                        return target.type == -1;
                    }),
        "New round clears the hero counter/latch, granted targets and shared notices");
    auto& reset = fixture(fresh);
    reset.balloons_hit = 25;
    tick(fresh);
    require(reset.balloon_hero && reset.score == 100 && event_count(fresh, game::RoundEventKind::balloon_hero) == 1, "A fresh round can independently earn hero again");
}
} // namespace

int main() {
    try {
        predicates(predicate_cases);
        allocations(allocation_cases);
        notice_pools(notice_cases);
        deferred_input(expected_hit_sequence);
        ordering(expected_earlier_stone_grant);
        exit_and_reset();
        std::cout << "Balloon hero exact 25/latch, six requests, pools/RNG/events, deferred input, bonus priority and reset passed\n";
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
