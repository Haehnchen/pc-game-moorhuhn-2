#pragma once

#include "contracts/draw.hpp"
#include "contracts/time.hpp"
#include "game/specials/multi_hit.hpp"
#include "world/airplane.hpp"
#include "world/balloon.hpp"
#include "world/big_chicken.hpp"
#include "world/blimp.hpp"
#include "world/boat.hpp"
#include "world/branch.hpp"
#include "world/frog.hpp"
#include "world/hidden_chicken.hpp"
#include "world/hunting_stand.hpp"
#include "world/notice.hpp"
#include "world/persistent_leaves.hpp"
#include "world/rose.hpp"
#include "world/scarecrow.hpp"
#include "world/special_chicken.hpp"
#include "world/spider.hpp"
#include "world/stone.hpp"
#include "world/tree.hpp"
#include "world/village.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace moorhuhn2::game {
namespace detail {
struct SceneObject;
}

inline constexpr int boat_scene_slot = 3;
inline constexpr int spider_scene_slot = 5;
inline constexpr int hunting_stand_scene_slot = 4;
inline constexpr int hidden_chicken_scene_slot = 14;
inline constexpr int hidden_leaf_scene_slot = 33;
inline constexpr int rose_scene_slot = 32;
inline constexpr int first_scarecrow_scene_slot = 6;
inline constexpr int first_tree_scene_slot = 15;
inline constexpr int frog_scene_slot = 19;
inline constexpr int first_branch_scene_slot = 20;
inline constexpr int first_stone_scene_slot = 24;
inline constexpr int initial_scene_slots = 34;
// Chicken::type markers for scene slots owned by other records.
inline constexpr int inert_scene_marker = -2;
inline constexpr int balloon_scene_marker = -3;
inline constexpr int tree_fragment_scene_marker = -4;
inline constexpr int big_chicken_scene_marker = -5;
inline constexpr int special_chicken_scene_marker = -6;
inline constexpr int airplane_scene_marker = -7;
inline constexpr int blimp_scene_marker = -8;
inline constexpr std::size_t initial_scene_capacity = 128;
inline constexpr std::size_t transient_particle_capacity = 68;
inline constexpr int ammo_capacity = 16;
inline constexpr int initial_ammo_index = 8;

struct RoundInput {
    contracts::ScreenPoint pointer{320, 240};
    bool fire_down{};
    bool reload_down{};
    bool held_left{};
    bool held_right{};
    bool cancel{};
};

enum class RoundEventKind : std::uint8_t {
    shot,
    empty,
    reload,
    chicken_hit,
    countdown,
    game_over,
    spawn,
    boat_hit,
    stone_hit,
    special_ammo,
    balloon_hit,
    boat_sunk,
    rare_balloon,
    church_bell,
    church_bell_second,
    house_hit,
    frog_croak,
    frog_hit,
    frog_time_bonus,
    spider_hit,
    spider_balloons,
    spider_hero,
    multi_hit_bonus,
    branch_hit,
    branch_ground,
    tree_hit,
    tree_destroyed,
    tree_ground,
    leaf_hit,
    tree_highfly,
    frog_tree_bonus,
    scarecrow_hit,
    scarecrow_balloons,
    scarecrow_chickens,
    hunting_stand_chicken,
    hunting_stand_hit,
    hunting_stand_ladder,
    hunting_stand_broken,
    hidden_chicken_close,
    hidden_chicken_mid,
    hidden_chicken_far,
    hidden_chicken_balloons,
    hidden_leaf_hit,
    big_chicken_appear,
    special_chicken_hit,
    airplane_engine_start,
    airplane_engine_pan,
    airplane_engine_stop,
    airplane_failure_start,
    airplane_failure_pan,
    airplane_failure_stop,
    airplane_hit,
    airplane_crash,
    blimp_hit,
    blimp_killer,
    balloon_hero
};

struct RoundEvent {
    RoundEventKind kind{};
    int slot{-1};
    int depth{};
    int points{};
    float pan{};
    bool operator==(const RoundEvent&) const = default;
};

struct Chicken {
    // Original pool type: -1 vacant, -2..-8 scene markers, 0/1 circular,
    // 2/3 horizontal, 40..43 dying.
    int type{-1};
    int depth{};
    float world_x{};
    float world_y{};
    float screen_x{-1024.0F};
    float screen_y{-1024.0F};
    float phase{};
    float animation_time{};
    int frame{};
    int points{};
    bool operator==(const Chicken&) const = default;
};

struct AmmoSprite {
    float x{};
    float y{415.0F};
    float velocity_x{};
    float velocity_y{};
    float animation_time{};
    int frame{21};
    int state{}; // 0 absent, 1 loaded, 2 falling spent cartridge.
    bool operator==(const AmmoSprite&) const = default;
};

struct Particle {
    bool active{};
    int type{2}; // 2 score text, 3 debris, 4 blimp smoke, 5 airplane smoke.
    int depth{2};
    float world_x{};
    float world_y{};
    float screen_x{-1024.0F};
    float screen_y{-1024.0F};
    float age{};
    float velocity_x{};
    float velocity_y{};
    int points{};
    int frame{};
    bool operator==(const Particle&) const = default;
};

struct RoundState {
    float camera{640.0F};
    float remaining{90.0F};
    float previous_sample{};
    float delta{};
    float accumulated_time{};

    float spawn_accumulator{};
    float spawn_threshold{50.0F};
    std::size_t scene_capacity{initial_scene_capacity};

    int minutes{1};
    int seconds{30};
    int countdown_second{};
    int score{};
    int ammo_index{initial_ammo_index};
    int reload_start_index{initial_ammo_index};
    int hits{};
    int misses{};
    int recoil{};

    int stones_hit{};
    int balloons_hit{};
    int frog_hits{};
    int spider_hits{};
    int branches_hit{};
    int leaves_hit{};
    int scarecrow_hits{};

    bool scarecrow_ordered{true};
    bool scarecrow_complete{};
    std::array<bool, 2> trees_destroyed{};
    bool spider_hero{};
    bool balloon_hero{};
    bool special_ammo{};
    bool stone_bonus_granted{};
    bool hidden_chicken_enabled{};
    bool house88_mode{};
    bool branch_ammo_granted{};
    bool special_chicken_spawned{};
    int special_chicken_slot{-1};

    std::uint32_t random_state{};
    std::uint64_t random_draws{};
    std::uint64_t updates{};
    bool exit_requested{};

    std::array<Chicken, contracts::scene_pool_capacity> chickens{};
    // Dynamic scene records; markers -5 through -8 reserve shared scene slots.
    std::array<world::BigChicken, contracts::scene_pool_capacity> big_chickens{};
    world::SpecialChicken special_chicken;
    world::Airplane airplane;
    world::AirplaneState airplane_state;
    int airplane_slot{-1};
    world::Blimp blimp;
    world::BlimpProgress blimp_progress;
    int blimp_slot{-1};

    std::array<AmmoSprite, ammo_capacity> ammo{};
    std::array<world::Stone, 8> stones{};
    std::array<world::Branch, 4> branches{world::initial_branches()};
    std::array<world::Tree, 4> trees{world::initial_trees()};
    std::array<world::ScarecrowPiece, 8> scarecrow{world::initial_scarecrow()};
    // Original scene slots shared with chickens/balloons; marker -4 reserves them.
    std::array<world::TreeFragment, contracts::scene_pool_capacity> tree_fragments{};

    world::Boat boat;
    world::HuntingStand hunting_stand;
    world::HiddenChicken hidden_chicken;
    world::HiddenLeaf hidden_leaf;
    world::Rose rose;
    world::Village village;
    world::Frog frog;
    world::Spider spider;

    world::NoticePool notices{};
    specials::MultiHitState multi_hit;
    int selected_slot{-1};

    // Shares scene slots with chickens; chicken type -3 reserves a balloon slot.
    std::array<world::Balloon, contracts::scene_pool_capacity> balloons{};

    // Original slots 0..31 remain occupied by persistent leaves throughout a round.
    std::array<world::PersistentLeaf, world::persistent_leaf_count> persistent_leaves{};
    // Transient effects use the same original first-free slots 32..99.
    std::array<Particle, transient_particle_capacity> particles{};
    // Smoke shares these same particle slots; active/type reserve the entry.
    std::array<world::AircraftSmoke, transient_particle_capacity> aircraft_smoke{};
    bool operator==(const RoundState&) const = default;
};

// The app owns introduction, exit and boss/focus UI phases. All time samples are
// explicit binary32 application seconds; no SDL clock, input or RNG is accessed.
class Round {
public:
    using PixelMatch = std::function<bool(const contracts::SpriteDraw&, contracts::ScreenPoint)>;

    // The countdown cache belongs to the application; zero is its cold value.
    Round(float seed_time, float initial_sample, int countdown_second = 0);
    void reset_sample(float application_time);
    // Preserve MH2's update -> compose frame -> fire -> reload order.
    void advance(contracts::FrameTime time, const RoundInput& input);
    void resolve_actions(const PixelMatch& pixel_match);

    [[nodiscard]] const RoundState& state() const noexcept;

    [[nodiscard]] contracts::ScreenPoint pointer() const noexcept {
        return input_.pointer;
    }

    [[nodiscard]] std::span<const RoundEvent> events() const noexcept;

    [[nodiscard]] std::vector<contracts::DepthDraw> target_draws(int depth) const;
    void append_target_draws(int depth, std::vector<contracts::DepthDraw>& draws) const;
    [[nodiscard]] std::vector<contracts::SpriteDraw> ammo_draws() const;

private:
    friend struct RoundTestAccess;
    [[nodiscard]] std::uint32_t random();
    [[nodiscard]] std::optional<std::size_t> free_scene_slot() const;
    void spawn(std::optional<int> requested_type = {});
    void update_chicken(Chicken& chicken);
    void update_ammo();
    void load_ammo(int index, int state);
    void update_scene_object(std::size_t index);
    [[nodiscard]] std::optional<contracts::SpriteDraw> collision_sprite(const detail::SceneObject& object, int recoil) const;
    [[nodiscard]] int select_target(const PixelMatch& pixel_match, int recoil) const;
    void hit_target(int selected);
    void apply_village_hit(const detail::SceneObject& object);
    void apply_boat_hit();
    void apply_hunting_stand_hit(int selected);
    void apply_rose_hit();
    void apply_spider_hit();
    void apply_frog_hit();
    void apply_branch_hit(const detail::SceneObject& object, int selected);
    void apply_stone_hit(const detail::SceneObject& object);
    void apply_tree_leaf_hit(const detail::SceneObject& object, int selected);
    void apply_balloon_hit(const detail::SceneObject& object, int selected);
    void apply_big_chicken_hit(const detail::SceneObject& object, int selected);
    void apply_special_chicken_hit(int selected);
    void apply_blimp_hit(int selected);
    void apply_chicken_hit(const detail::SceneObject& object, int selected);
    void update_camera();
    void update_clock_and_spawn();
    void update_grants();
    void update_particles();
    void score_popup(float x, float y, int depth, int points);
    void centered_debris(float x, float y, int depth, int count);
    void apply_hidden_impact(const world::HiddenImpact& impact, int slot, int depth);
    void apply_airplane_events(std::span<const world::AirplaneEvent> events);
    void spawn_aircraft_smoke(bool blimp, int source_type, float x, float y);
    void wood_debris(int count);
    void spawn_tree_fragment(int type);
    void hit_tree(std::size_t index);
    void update_balloon(std::size_t index);
    void spawn_balloon();
    void grant_stone_bonus();
    void grant_spider_bonus();
    void grant_scarecrow_bonus();
    void grant_balloon_bonus();
    void hit_scarecrow(std::size_t index);
    void update_multi_hit();
    [[nodiscard]] std::optional<int> selected_type() const;
    RoundState state_;
    RoundInput input_;
    std::vector<RoundEvent> events_;
    bool awaiting_actions_{};
};

[[nodiscard]] contracts::SpriteDraw chicken_draw(const Chicken& chicken);

} // namespace moorhuhn2::game
