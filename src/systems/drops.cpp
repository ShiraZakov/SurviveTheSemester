// MAY - assignment drops. Removes drops that fell past the floor (uncaught).
// Tax drops that miss the paddle emit TaxMissed.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Events.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

/// @brief Removes drops that have fallen below the screen floor. Tax drops that
///        are missed emit a TaxMissed event before being tagged for deletion.
/// @param dt Fixed timestep in seconds (unused, reserved for future timed effects)
/// @return void
void dropSystem(float dt) {
    (void)dt;
    static const Mask mask = MaskBuilder()
        .set<DropTag>()
        .set<DropInfo>()
        .set<Position>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.has<DeadTag>()) continue;
        if (e.get<Position>().y <= Config::WORLD_H + 1.0f) continue;
        if (e.get<DropInfo>().type == DropType::Tax)
            ev::taxMissed(e.get<DropInfo>().courseIndex);
        e.add(DeadTag{});
    }
}
