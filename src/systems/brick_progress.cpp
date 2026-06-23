/// @file brick_progress.cpp
/// @brief Per-brick meter advancement, prerequisite unlocking, and brick clearing.
///        Implements brickMeterSystem, brickUnlockSystem, brickClearDelaySystem, and
///        the brickIsPlayable / brickPrereqsMet helpers used by other systems.
//
// Each brick has a small segment meter (BrickProgress). Ball hits and caught Assignment
// drops advance it; when full, the brick waits a brief clearDelay then is destroyed,
// which emits BrickCleared and spawns a Tax drop. Bricks can be locked behind
// prerequisite courses (BrickPrereqMask) — brickUnlockSystem unlocks them once the
// required catalog courses are gone, and also swaps to the locked/unlocked sprite.
// "Playable" (brickIsPlayable) = unlocked, not mid-clear, and prerequisites met.

#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Events.h"
#include "Physics.h"
#include "Sprites.h"

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

bool brickPrereqsMet(Entity brick) {
    if (!brick.has<BrickPrereqMask>()) return true;
    const uint32_t required = brick.get<BrickPrereqMask>().mustClear;
    if (required == 0) return true;

    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<BrickTag>() || !e.has<BrickInfo>() || e.has<DeadTag>()) continue;
        const int ci = e.get<BrickInfo>().courseIndex;
        if (ci < 0 || ci > sprites::FINAL_COURSE_INDEX) continue;
        if ((required & (1u << ci)) != 0) return false;
    }
    return true;
}

/// @brief Returns whether a brick should draw its locked sprite given a pre-computed prereq result.
///        Callers in brickUnlockSystem pass the prereqsMet they already computed to avoid
///        re-running the expensive full-world scan.
static bool brickLockedForSprite(int ci, const BrickProgress& prog, bool prereqsMet) {
    if (ci == sprites::FINAL_COURSE_INDEX)
        return !prereqsMet;
    return sprites::courseShowsLockedSprite(ci, prog.unlocked);
}

bool brickDrawLocked(Entity brick) {
    if (!brick.has<BrickInfo>() || !brick.has<BrickProgress>()) return false;
    const int ci = brick.get<BrickInfo>().courseIndex;
    const auto& prog = brick.get<BrickProgress>();

    // Only the final-project brick needs the (expensive) prereq scan; skip it otherwise.
    const bool prereqsMet = (ci == sprites::FINAL_COURSE_INDEX) ? brickPrereqsMet(brick) : true;
    return brickLockedForSprite(ci, prog, prereqsMet);
}

bool brickShowsLocked(Entity brick) {
    return brickDrawLocked(brick);
}

SpritePart brickSpritePart(Entity brick) {
    const int ci = brick.get<BrickInfo>().courseIndex;
    return sprites::makePart(sprites::courseSpriteId(ci, brickDrawLocked(brick)));
}

bool isFinalProjectLocked() {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<BrickTag>() || !e.has<BrickInfo>() || e.has<DeadTag>()) continue;
        if (e.get<BrickInfo>().courseIndex == sprites::FINAL_COURSE_INDEX) continue;
        return true;
    }
    return false;
}

bool brickIsPlayable(Entity brick) {
    if (!brick.has<BrickProgress>()) return false;
    const auto& prog = brick.get<BrickProgress>();
    if (!prog.unlocked || prog.clearDelay > 0.0f) return false;
    return brickPrereqsMet(brick);
}

/// @brief Updates the brick's SpritePart to the correct locked/unlocked art for its course.
static void applyBrickSprite(Entity brick, int courseIndex, bool locked) {
    brick.get<SpritePart>() = sprites::makePart(sprites::courseSpriteId(courseIndex, locked));
}

/// @brief Emits BrickCleared, spawns the Tax drop, removes the Box2D body, and tags the brick Dead.
static void clearBrick(Entity brick) {
    const auto& info = brick.get<BrickInfo>();
    const auto& pos  = brick.get<Position>();
    ev::brickCleared(info.courseId, info.courseIndex);
    spawnDrop(info.courseId, info.courseIndex, pos.x, pos.y, DropType::Tax, brick.entity());
    phys::destroyBody(brick.entity());
    brick.add(DeadTag{});
}

/// @brief Increments the brick's fill counter if the brick is playable and not yet full;
///        sets clearDelay and removes the physics body when the meter completes.
static void tryAdvanceBrick(Entity brick) {
    if (!brick.has<BrickProgress>() || !brick.has<BrickInfo>() || !brick.has<Position>()) return;
    if (!brickIsPlayable(brick)) return;
    auto& prog = brick.get<BrickProgress>();
    if (prog.filled >= prog.max) return;

    ++prog.filled;
    if (prog.filled < prog.max) return;

    prog.clearDelay = Config::BRICK_CLEAR_DELAY;
    phys::destroyBody(brick.entity());
}

/// @brief Counts down clearDelay timers on full bricks; destroys brick + spawns Tax
///        drop when the timer reaches zero.
/// @param dt Fixed timestep in seconds
/// @return void
void brickClearDelaySystem(float dt) {
    static const Mask mask = MaskBuilder()
        .set<BrickTag>()
        .set<BrickInfo>()
        .set<BrickProgress>()
        .set<Position>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.has<DeadTag>()) continue;
        auto& prog = e.get<BrickProgress>();
        if (prog.clearDelay <= 0.0f) continue;

        prog.clearDelay -= dt;
        if (prog.clearDelay > 0.0f) continue;

        prog.clearDelay = 0.0f;
        clearBrick(e);
    }
}

/// @brief Checks prerequisite courses for every locked brick and unlocks them
///        (updating sprite) when all required courses are cleared.
/// @return void
void brickUnlockSystem() {
    static const Mask mask = MaskBuilder()
        .set<BrickTag>()
        .set<BrickInfo>()
        .set<BrickProgress>()
        .set<BrickPrereqMask>()
        .set<SpritePart>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.has<DeadTag>()) continue;

        auto& prog = e.get<BrickProgress>();
        const int ci = e.get<BrickInfo>().courseIndex;
        const bool prereqsMet = brickPrereqsMet(e);

        if (!prereqsMet)
            prog.unlocked = false;
        else if (!prog.unlocked)
            prog.unlocked = true;

        // Reuse prereqsMet (already computed above) instead of re-scanning via brickDrawLocked.
        applyBrickSprite(e, ci, brickLockedForSprite(ci, prog, prereqsMet));
    }
}

/// @brief Consumes CourseHit events to advance brick meters and spawn Assignment
///        drops. Consumes DropCaught (Assignment) events to further advance meters.
/// @return void
void brickMeterSystem() {
    {
        static const Mask mask = MaskBuilder().set<CourseHit>().build();
        static int q = World::createQuery(mask);
        for (Entity ev = World::first(q); !World::eof(q); ev = World::next(q)) {
            const auto hit = ev.get<CourseHit>();
            if (hit.brick.id < 0) continue;
            Entity brick{hit.brick};
            if (!brick.has<BrickTag>() || brick.has<DeadTag>()) continue;

            if (!brickIsPlayable(brick)) continue;
            if (brick.get<BrickProgress>().filled >= brick.get<BrickProgress>().max) continue;

            const auto& pos = brick.get<Position>();
            const auto& info = brick.get<BrickInfo>();
            spawnDrop(info.courseId, info.courseIndex, pos.x, pos.y, DropType::Assignment, brick.entity());
            tryAdvanceBrick(brick);
        }
    }
    {
        static const Mask mask = MaskBuilder().set<DropCaught>().build();
        static int q = World::createQuery(mask);
        for (Entity ev = World::first(q); !World::eof(q); ev = World::next(q)) {
            const auto caught = ev.get<DropCaught>();
            if (caught.type != DropType::Assignment || caught.brick.id < 0) continue;
            Entity brick{caught.brick};
            if (!brick.has<BrickTag>() || brick.has<DeadTag>()) continue;
            if (!brickIsPlayable(brick)) continue;
            tryAdvanceBrick(brick);
        }
    }
}
