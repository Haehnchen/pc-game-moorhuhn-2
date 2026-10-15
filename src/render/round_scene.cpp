#include "render/round_scene.hpp"

#include "ui/screens.hpp"

namespace moorhuhn2::render {

void RoundScene::append(std::vector<contracts::SpriteDraw>& draws, const game::Round& round, const world::Presentation& landscape, contracts::ScreenPoint pointer) {
    scene_.clear();
    for (int depth = 4; depth >= 0; --depth) {
        round.append_target_draws(depth, scene_);

        for (const auto& leaf : round.state().persistent_leaves) {
            if (leaf.depth == depth) {
                scene_.push_back({depth, world::persistent_leaf_draw(leaf)});
            }
        }

        for (std::size_t index = 0; index < round.state().particles.size(); ++index) {
            const auto& particle = round.state().particles[index];
            if (!particle.active || particle.depth != depth) {
                continue;
            }

            if (particle.type == 2) {
                text_.clear();
                ui::number(text_, particle.points, static_cast<int>(particle.screen_x), static_cast<int>(particle.screen_y));
                for (const auto& draw : text_) {
                    scene_.push_back({depth, draw});
                }
            } else if (particle.type == 4 || particle.type == 5) {
                if (const auto smoke = world::aircraft_smoke_draw(round.state().aircraft_smoke[index])) {
                    scene_.push_back({depth, *smoke});
                }
            } else {
                scene_.push_back({depth, {"gfx/particle2", {0, particle.frame * 8, 8, 8}, {static_cast<int>(particle.screen_x), static_cast<int>(particle.screen_y)}, true}});
            }
        }
    }

    // Notices follow depth1/2 particles but precede foreground 0/HUD.
    for (const auto& notice : round.state().notices) {
        if (const auto label = world::notice_text(notice)) {
            text_.clear();
            ui::text(text_, label->text, label->x, label->y);
            for (const auto& draw : text_) {
                scene_.push_back({1, draw});
            }
        }
    }

    landscape.append_commands(draws, layer_, round.state().camera, scene_, {}, round.state().recoil, round.state().boat, round.state().branches, round.state().trees,
        round.state().scarecrow, &round.state().hunting_stand, &round.state().rose);
    const auto ammo = round.ammo_draws();
    draws.insert(draws.end(), ammo.begin(), ammo.end());
    ui::hud(draws, round.state().minutes, round.state().seconds, round.state().score);
    ui::cursor(draws, pointer);
}

std::vector<contracts::SpriteDraw> round_scene(const game::Round& round, const world::Presentation& landscape) {
    RoundScene composer;
    std::vector<contracts::SpriteDraw> draws;
    composer.append(draws, round, landscape, round.pointer());
    return draws;
}

} // namespace moorhuhn2::render
