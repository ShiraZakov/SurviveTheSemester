
#include "systems/systems.h"
#include "Components.h"
#include "Physics.h"

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

void deadCleanupSystem() {
    static const Mask mask = MaskBuilder().set<DeadTag>().build();
    static int q = World::createQuery(mask);
    if (World::eof(q)) return;

    // Stack buffer (no per-frame heap alloc): a query holds at most InitialEntities,
    // so 128 can never overflow.
    bagel::StaticBag<ent_type, 128> dead;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        dead.push(e.entity());
    for (int i = 0; i < dead.size(); ++i) {
        // A persistent query can list the same id twice (BAGEL pushes on add
        // without de-duping, and a recycled id can re-add over a stale entry).
        // Skip ids already destroyed this drain so we never free an id twice
        // (a double-free corrupts the recycle stack and collapses two later
        // spawns onto one id).
        if (Entity(dead[i]).mask().ctz() < 0) continue;
        phys::destroyBody(dead[i]);
        Entity(dead[i]).destroy();
    }
}
