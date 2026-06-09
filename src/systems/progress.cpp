// MAY - course progress. Advances a course from CourseHit / DropCaught events;
// at 100% marks the course DONE and fires ExamStarted.
// TODO (MAY): tune progress weights, drop spawning cadence, progress visuals.
#include "systems/systems.h"
#include "Components.h"
#include "Events.h"
#include "Game.h"
#include <algorithm>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static void bump(int courseId, float amount) {
    auto c = courseEntity(courseId);
    if (c.id < 0) return;
    auto& cs = Entity(c).get<Course>();
    cs.progress = std::min(1.0f, cs.progress + amount);
    if (cs.progress >= 1.0f && cs.state == CourseState::ACTIVE) {
        cs.state = CourseState::DONE;
        ev::examStarted(courseId);
    }
}

/// @brief Advances course progress from BrickCleared events. When a course reaches
///        100% it transitions to DONE and emits ExamStarted.
/// @return void
void courseProgressSystem() {
    // 0.14 * 8 bricks per row > 1.0, so clearing a course's bricks completes it
    // (drives the end-to-end demo). MAY: retune once real drops/weights exist.
    {
        static const Mask mask = MaskBuilder().set<BrickCleared>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
            bump(e.get<BrickCleared>().courseId, 0.14f);
    }
}
