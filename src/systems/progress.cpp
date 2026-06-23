/// @file progress.cpp
/// @brief Course-track progress accumulation and exam triggering.
///        Consuming BrickCleared events advances a track's progress and, at 100%, emits ExamStarted.
// Each cleared brick (BrickCleared) bumps its colored track's Course.progress; when a
// track reaches 100% it flips ACTIVE -> DONE and emits ExamStarted. Note this works on
// courseId (the 0..2 track), not the 0..20 catalog index — see Components.h.

#include "systems/systems.h"
#include "Components.h"
#include "Events.h"
#include "Game.h"
#include <algorithm>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

/// @brief Adds progress to a course and fires ExamStarted when it reaches 100%.
/// @param courseId ID of the course to advance
/// @param amount Progress fraction to add (0.0–1.0)
/// @return void
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
    // (drives the end-to-end demo). TODO: retune once real drops/weights exist.
    {
        static const Mask mask = MaskBuilder().set<BrickCleared>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
            bump(e.get<BrickCleared>().courseId, 0.14f);
    }
}
