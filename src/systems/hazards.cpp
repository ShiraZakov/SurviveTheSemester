// YUVAL - academic hazards (v1: lose life / reduce progress).
// TODO (YUVAL): spawn hazards, telegraph/animate, deferred advanced effects.
#include "systems/systems.h"
#include "Components.h"
#include "Events.h"
#include "Game.h"
#include <algorithm>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

/// @brief Consumes HazardTriggered events and applies effects: LoseLife emits
///        LifeLost; ReduceProgress subtracts 25% from the target course's progress.
/// @return void
void hazardSystem() {
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
