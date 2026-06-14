// Shared end-of-frame entity cleanup.
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

    bagel::Bag<ent_type, 128> dead;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        dead.push(e.entity());
    for (int i = 0; i < dead.size(); ++i) {
        phys::destroyBody(dead[i]);
        Entity(dead[i]).destroy();
    }
}
