// YUVAL - exam phase. PLACEHOLDER: immediately resolves any started exam as a
// pass. Replace with the real exam: set Phase::EXAM, run a timer, spawn
// projectiles to dodge/shoot, then emit ExamFinished{passed, grade}.
#include "systems/systems.h"
#include "Components.h"
#include "Events.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

void examSystem(float dt) {
    (void)dt;
    static const Mask mask = MaskBuilder().set<ExamStarted>().build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        ev::examFinished(e.get<ExamStarted>().courseId, true, 85.0f);
}
