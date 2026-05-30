// AVIEL - ball/paddle behavior. Consumes CourseHit and removes the hit brick.
// TODO (AVIEL): scoring, lastCourse tracking, ball-loss -> LifeLost when the
// bottom wall is opened, brick hit juice.
#include "systems/systems.h"
#include "Components.h"
#include "Events.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

void courseHitSystem() {
    static const Mask mask = MaskBuilder().set<CourseHit>().build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        auto brick = e.get<CourseHit>().brick;
        if (brick.id < 0) continue;               // synthetic/debug hit, no brick
        Entity b(brick);
        if (b.has<BrickTag>() && !b.has<DeadTag>())
            b.add(DeadTag{});
    }
}
