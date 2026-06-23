/// @file cleanup.cpp
/// @brief End-of-frame entity reaping.
///        Systems tag entities DeadTag instead of deleting mid-frame; deadCleanupSystem
///        destroys their Box2D bodies and ECS entities once, after all systems have run.
// Systems never delete entities mid-frame (that would mutate queries other systems are
// still walking); they tag them DeadTag instead. This runs last each frame to destroy
// those bodies and entities once, after every system has seen them.

#include "systems/systems.h"
#include "Components.h"
#include "Physics.h"

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

/// @brief Destroys the Box2D body and deletes every entity tagged DeadTag this frame.
/// @return void
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
