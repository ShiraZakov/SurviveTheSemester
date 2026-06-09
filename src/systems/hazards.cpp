// YUVAL/MAY - academic hazards. One LoseLife hazard spawns per year advance;
// ball passing through it costs the player a life.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Events.h"
#include "Game.h"
#include <algorithm>
#include <cstdlib>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

// Returns a pseudo-random float in [lo, hi] using SDL_rand.
static inline float randRange(float lo, float hi) {
    return lo + (static_cast<float>(SDL_rand(10000)) / 10000.0f) * (hi - lo);
}

/// @brief Spawns one LoseLife hazard per year that passes, positioned randomly in
///        the middle third of the field. Consumes HazardTriggered events and applies
///        effects: LoseLife emits LifeLost; ReduceProgress subtracts 25% progress.
/// @return void
void hazardSystem() {
    GameState& gs = gameState();

    // Spawn one hazard each time a new year starts (year 1 does not spawn)
    if (gs.started && gs.currentYear > 1 && gs.currentYear > gs.lastHazardYear) {
        gs.lastHazardYear = gs.currentYear;
        const float xMin = Config::WALL + 1.0f;
        const float xMax = Config::WORLD_W - Config::WALL - 1.0f;
        const float yMin = Config::WORLD_H * 0.35f;
        const float yMax = Config::WORLD_H * 0.60f;
        spawnHazard(-1, randRange(xMin, xMax), randRange(yMin, yMax), HazardType::LoseLife);
    }

    // Consume HazardTriggered events
    {
        static const Mask mask = MaskBuilder().set<HazardTriggered>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            const auto& h = e.get<HazardTriggered>();
            if (h.type == HazardType::LoseLife) {
                ev::lifeLost(1);
            } else { // ReduceProgress
                auto c = courseEntity(h.courseId);
                if (c.id >= 0) {
                    auto& cs = Entity(c).get<Course>();
                    cs.progress = std::max(0.0f, cs.progress - 0.25f);
                }
            }
        }
    }
}
