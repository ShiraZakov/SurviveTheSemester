// MAY - assignment drops. Removes drops that fell past the floor (uncaught).
// Catching is handled by the physics sensor -> DropCaught event.
// TODO (MAY): spawn drops from active-course bricks on a timer; floor-miss penalty.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

void dropSystem(float dt) {
    (void)dt;
    static const Mask mask = MaskBuilder()
        .set<DropTag>()
        .set<Position>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (!e.has<DeadTag>() && e.get<Position>().y > Config::WORLD_H + 1.0f)
            e.add(DeadTag{});
    }
}
