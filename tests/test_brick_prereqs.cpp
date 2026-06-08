// Unit tests for brick prerequisite ECS logic and final-project locking.
#include "Components.h"
#include "Config.h"
#include "Sprites.h"
#include "systems/systems.h"

#include <cassert>
#include <cstdio>
#include <vector>

using bagel::Entity;
using bagel::ent_type;

static void resetWorld() {
    std::vector<ent_type> ids;
    for (Entity e = Entity::first(); !e.eof(); e.next())
        if (e.mask().ctz() >= 0) ids.push_back(e.entity());
    for (ent_type id : ids) Entity(id).destroy();
}

static Entity spawnTestBrick(int courseIndex, uint32_t prereqMask, bool unlocked = false) {
    Entity e = Entity::create();
    e.addAll(BrickInfo{0, courseIndex},
             BrickProgress{0, 3, unlocked, 0.0f},
             BrickPrereqMask{prereqMask},
             BrickTag{});
    return e;
}

static void test_course_prereq_mask_catalog() {
    resetWorld();
    assert(sprites::coursePrereqMask(0) == 0);
    assert(sprites::coursePrereqMask(1) == 0);
    assert(sprites::coursePrereqMask(2) == (1u << 6));
    assert(!sprites::courseStartsUnlocked(sprites::FINAL_COURSE_INDEX));
    assert(sprites::courseStartsUnlocked(0));

    const uint32_t finalMask = sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX);
    assert(finalMask == ((1u << sprites::FINAL_COURSE_INDEX) - 1u));
    assert((finalMask & (1u << sprites::FINAL_COURSE_INDEX)) == 0);
}

static void test_final_locked_while_any_other_lives() {
    resetWorld();
    Entity intro = spawnTestBrick(0, 0, true);
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        false);

    assert(!brickPrereqsMet(finalBrick));
    assert(brickShowsLocked(finalBrick));
    assert(isFinalProjectLocked());
    assert(!brickIsPlayable(finalBrick));

    intro.destroy();
    finalBrick.destroy();
}

static void test_final_unlocks_when_only_final_remains() {
    resetWorld();
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        false);

    assert(brickPrereqsMet(finalBrick));
    assert(!brickShowsLocked(finalBrick));
    assert(!isFinalProjectLocked());

    finalBrick.destroy();
}

static void test_final_unlocks_when_others_are_dead() {
    resetWorld();
    Entity intro = spawnTestBrick(0, 0, true);
    Entity calc = spawnTestBrick(2, sprites::coursePrereqMask(2), false);
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        false);

    assert(!brickPrereqsMet(finalBrick));
    intro.add(DeadTag{});
    assert(!brickPrereqsMet(finalBrick));
    calc.add(DeadTag{});
    assert(brickPrereqsMet(finalBrick));
    assert(!brickShowsLocked(finalBrick));
    assert(!isFinalProjectLocked());

    intro.destroy();
    calc.destroy();
    finalBrick.destroy();
}

static void test_single_prereq_course() {
    resetWorld();
    Entity math = spawnTestBrick(6, 0, true);
    Entity calc = spawnTestBrick(2, sprites::coursePrereqMask(2), false);

    assert(!brickPrereqsMet(calc));
    assert(!brickIsPlayable(calc));
    math.add(DeadTag{});
    assert(brickPrereqsMet(calc));

    math.destroy();
    calc.destroy();
}

static void test_final_not_playable_when_unlocked_flag_true_but_prereqs_missing() {
    resetWorld();
    Entity intro = spawnTestBrick(0, 0, true);
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        true);

    assert(!brickPrereqsMet(finalBrick));
    assert(!brickIsPlayable(finalBrick));

    intro.destroy();
    finalBrick.destroy();
}

static void test_final_mask_has_every_other_course_bit() {
    resetWorld();
    const uint32_t mask = sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX);
    for (int i = 0; i < sprites::FINAL_COURSE_INDEX; ++i)
        assert((mask & (1u << i)) != 0);
}

static void test_locked_sprite_flags() {
    resetWorld();
    assert(sprites::courseUsesLockedArt(sprites::FINAL_COURSE_INDEX));
    assert(sprites::courseShowsLockedSprite(sprites::FINAL_COURSE_INDEX, false));
    assert(!sprites::courseShowsLockedSprite(sprites::FINAL_COURSE_INDEX, true));
    assert(!sprites::courseUsesLockedArt(15)); // databases
    assert(!sprites::courseShowsLockedSprite(15, false));
}

static void test_databases_starts_unlocked() {
    resetWorld();
    assert(sprites::courseStartsUnlocked(15));
    assert(sprites::coursePrereqMask(15) == 0);
    Entity db = spawnTestBrick(15, 0, true);
    assert(brickPrereqsMet(db));
    assert(!brickShowsLocked(db));
    assert(brickIsPlayable(db));
    db.destroy();
}

static void test_clear_delay_blocks_playable() {
    resetWorld();
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        true);
    finalBrick.get<BrickProgress>().clearDelay = 0.5f;
    assert(!brickIsPlayable(finalBrick));
    finalBrick.destroy();
}

static void test_one_remaining_non_final_still_blocks() {
    resetWorld();
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        false);

    for (int i = 0; i < sprites::FINAL_COURSE_INDEX - 1; ++i) {
        Entity b = spawnTestBrick(i, 0, true);
        b.add(DeadTag{});
    }

    spawnTestBrick(sprites::FINAL_COURSE_INDEX - 1, 0, true); // complexity still alive

    assert(!brickPrereqsMet(finalBrick));
    assert(isFinalProjectLocked());
    resetWorld();
}

static void test_oop_requires_c_programming() {
    resetWorld();
    Entity c = spawnTestBrick(1, 0, true);
    Entity oop = spawnTestBrick(10, sprites::coursePrereqMask(10), false);

    assert(!brickPrereqsMet(oop));
    c.add(DeadTag{});
    assert(brickPrereqsMet(oop));
    assert(!brickShowsLocked(oop));

    c.destroy();
    oop.destroy();
}

static void test_brick_without_prereq_mask_always_met() {
    resetWorld();
    Entity e = Entity::create();
    e.addAll(BrickInfo{0, 2}, BrickProgress{0, 3, false, 0.0f}, BrickTag{});
    assert(brickPrereqsMet(e));
    e.destroy();
}

static void test_all_twenty_prereqs_required() {
    resetWorld();
    Entity finalBrick = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        false);

    for (int i = 0; i < sprites::FINAL_COURSE_INDEX; ++i)
        spawnTestBrick(i, sprites::coursePrereqMask(i), sprites::coursePrereqMask(i) == 0);

    assert(!brickPrereqsMet(finalBrick));
    assert(isFinalProjectLocked());

    for (int i = 0; i < sprites::FINAL_COURSE_INDEX; ++i) {
        for (Entity e = Entity::first(); !e.eof(); e.next()) {
            if (e.mask().ctz() < 0) continue;
            if (!e.has<BrickInfo>()) continue;
            if (e.get<BrickInfo>().courseIndex != i) continue;
            e.add(DeadTag{});
            break;
        }
    }

    assert(brickPrereqsMet(finalBrick));
    assert(!isFinalProjectLocked());
    resetWorld();
}

static void test_sprite_catalog_rects_differ() {
    sprites::initCatalog();
    const SpritePart db = sprites::makePart(sprites::courseSpriteId(15, false));
    const SpritePart fin = sprites::makePart(sprites::courseSpriteId(20, false));
    const SpritePart lockedFin = sprites::makePart(sprites::courseSpriteId(20, true));
    assert(db.part.x < 400.0f);
    assert(fin.part.x > 1000.0f);
    assert(lockedFin.sheet != db.sheet || lockedFin.part.x != fin.part.x);
}

static void test_brick_sprite_part_matches_course_index() {
    resetWorld();
    sprites::initCatalog();
    Entity db = spawnTestBrick(15, 0, true);
    Entity fin = spawnTestBrick(
        sprites::FINAL_COURSE_INDEX,
        sprites::coursePrereqMask(sprites::FINAL_COURSE_INDEX),
        false);

    const SpritePart dbSp = brickSpritePart(db);
    const SpritePart finSp = brickSpritePart(fin);
    assert(dbSp.part.x < 400.0f);
    assert(finSp.part.x > 1000.0f);
    assert(dbSp.part.x != finSp.part.x);

    db.destroy();
    fin.destroy();
}

static void test_grid_has_single_final_brick() {
    resetWorld();
    assert(Config::gridCourseIndex(2, 6) == sprites::FINAL_COURSE_INDEX);
    assert(Config::gridCourseIndex(2, 1) == 15);
    int finalCount = 0;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col)
            if (Config::gridCourseIndex(row, col) == sprites::FINAL_COURSE_INDEX)
                ++finalCount;
    assert(finalCount == 1);
}

int main() {
    test_course_prereq_mask_catalog();
    test_final_mask_has_every_other_course_bit();
    test_locked_sprite_flags();
    test_databases_starts_unlocked();
    test_final_locked_while_any_other_lives();
    test_final_unlocks_when_only_final_remains();
    test_final_unlocks_when_others_are_dead();
    test_single_prereq_course();
    test_oop_requires_c_programming();
    test_final_not_playable_when_unlocked_flag_true_but_prereqs_missing();
    test_clear_delay_blocks_playable();
    test_one_remaining_non_final_still_blocks();
    test_brick_without_prereq_mask_always_met();
    test_all_twenty_prereqs_required();
    test_sprite_catalog_rects_differ();
    test_brick_sprite_part_matches_course_index();
    test_grid_has_single_final_brick();
    std::printf("All %d brick prereq tests passed.\n", 17);
    return 0;
}
