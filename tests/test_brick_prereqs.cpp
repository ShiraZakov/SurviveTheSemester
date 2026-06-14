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
    sprites::initCatalog();
    Entity e = Entity::create();
    e.addAll(BrickInfo{0, courseIndex},
             BrickProgress{0, 3, unlocked, 0.0f},
             BrickPrereqMask{prereqMask},
             sprites::makePart(sprites::courseSpriteId(
                 courseIndex, sprites::courseShowsLockedSprite(courseIndex, unlocked))),
             BrickTag{});
    return e;
}

static bool spritePartEq(const SpritePart& a, const SpritePart& b) {
    return a.sheet == b.sheet
        && a.part.x == b.part.x && a.part.y == b.part.y
        && a.part.w == b.part.w && a.part.h == b.part.h;
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
    assert(brickShowsLocked(oop));
    assert(!brickIsPlayable(oop));
    oop.get<BrickProgress>().unlocked = true;
    assert(!brickShowsLocked(oop));
    assert(brickIsPlayable(oop));

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

static void test_locked_art_stays_locked_until_unlock_flag() {
    resetWorld();
    sprites::initCatalog();
    Entity algo = spawnTestBrick(14, 0, true);
    Entity complexity = spawnTestBrick(19, sprites::coursePrereqMask(19), false);

    assert(sprites::courseUsesLockedArt(19));
    assert(brickShowsLocked(complexity));
    assert(!brickIsPlayable(complexity));

    algo.add(DeadTag{});
    assert(brickPrereqsMet(complexity));
    assert(brickShowsLocked(complexity));
    assert(!brickIsPlayable(complexity));

    complexity.get<BrickProgress>().unlocked = true;
    assert(!brickShowsLocked(complexity));
    assert(brickIsPlayable(complexity));

    const SpritePart lockedSp = sprites::makePart(sprites::courseSpriteId(19, true));
    const SpritePart openSp = sprites::makePart(sprites::courseSpriteId(19, false));
    const SpritePart drawSp = brickSpritePart(complexity);
    assert(drawSp.sheet == openSp.sheet);
    assert(drawSp.part.x == openSp.part.x);
    assert(drawSp.part.x != lockedSp.part.x);

    algo.destroy();
    complexity.destroy();
}

static void test_grid_has_single_complexity_brick() {
    int complexityCount = 0;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col)
            if (Config::gridCourseIndex(row, col) == 19)
                ++complexityCount;
    assert(complexityCount == 1);
}

static void test_grid_maps_each_course_once() {
    bool seen[sprites::COURSE_COUNT]{};
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col) {
            const int ci = Config::gridCourseIndex(row, col);
            assert(ci >= 0 && ci < sprites::COURSE_COUNT);
            assert(!seen[ci]);
            seen[ci] = true;
        }
    for (int i = 0; i < sprites::COURSE_COUNT; ++i)
        assert(seen[i]);
}

static void test_unlocked_sprite_matches_catalog_index() {
    sprites::initCatalog();
    for (int ci = 0; ci < sprites::COURSE_COUNT; ++ci) {
        const SpritePart expected = sprites::makePart(sprites::courseSpriteId(ci, false));
        resetWorld();
        Entity brick = spawnTestBrick(ci, sprites::coursePrereqMask(ci), true);
        const SpritePart drawn = brickSpritePart(brick);
        assert(spritePartEq(drawn, expected));
        brick.destroy();
    }
}

static void test_locked_sprite_matches_catalog_index() {
    sprites::initCatalog();
    for (int ci = 0; ci < sprites::COURSE_COUNT; ++ci) {
        if (!sprites::courseUsesLockedArt(ci) && ci != sprites::FINAL_COURSE_INDEX) continue;
        resetWorld();
        if (ci == sprites::FINAL_COURSE_INDEX)
            spawnTestBrick(0, 0, true);
        Entity brick = spawnTestBrick(ci, sprites::coursePrereqMask(ci), false);
        const bool locked = brickDrawLocked(brick);
        const SpritePart expected = sprites::makePart(sprites::courseSpriteId(ci, locked));
        assert(spritePartEq(brickSpritePart(brick), expected));
        assert(spritePartEq(brick.get<SpritePart>(), expected));
        brick.destroy();
    }
}

static void test_unlock_system_keeps_same_course_sprite() {
    sprites::initCatalog();
    for (int ci = 0; ci < sprites::FINAL_COURSE_INDEX; ++ci) {
        if (!sprites::courseUsesLockedArt(ci)) continue;
        const int prereq = sprites::coursePrereq(ci);
        if (prereq < 0) continue;

        resetWorld();
        Entity req = spawnTestBrick(prereq, sprites::coursePrereqMask(prereq), true);
        Entity brick = spawnTestBrick(ci, sprites::coursePrereqMask(ci), false);
        req.add(DeadTag{});

        brickUnlockSystem();

        const SpritePart expected = sprites::makePart(sprites::courseSpriteId(ci, false));
        assert(brick.get<BrickProgress>().unlocked);
        assert(spritePartEq(brickSpritePart(brick), expected));
        assert(spritePartEq(brick.get<SpritePart>(), expected));

        req.destroy();
        brick.destroy();
    }
}

static void test_unlock_never_swaps_to_other_course_rect() {
    sprites::initCatalog();
    resetWorld();
    Entity c = spawnTestBrick(1, 0, true);
    Entity algo = spawnTestBrick(14, 0, true);
    Entity complexity = spawnTestBrick(19, sprites::coursePrereqMask(19), false);
    Entity oop = spawnTestBrick(10, sprites::coursePrereqMask(10), false);

    const SpritePart complexityLocked = brickSpritePart(complexity);
    const SpritePart oopLocked = brickSpritePart(oop);
    assert(!spritePartEq(complexityLocked, oopLocked));

    algo.add(DeadTag{});
    brickUnlockSystem();

    const SpritePart complexityOpen = brickSpritePart(complexity);
    const SpritePart oopStillLocked = brickSpritePart(oop);
    const SpritePart complexityExpected = sprites::makePart(sprites::courseSpriteId(19, false));
    const SpritePart oopExpectedLocked = sprites::makePart(sprites::courseSpriteId(10, true));

    assert(spritePartEq(complexityOpen, complexityExpected));
    assert(spritePartEq(oopStillLocked, oopExpectedLocked));
    assert(!spritePartEq(complexityOpen, oopStillLocked));

    c.destroy();
    algo.destroy();
    complexity.destroy();
    oop.destroy();
}

static void test_networks_stays_locked_art_until_unlock_flag() {
    sprites::initCatalog();
    resetWorld();
    Entity structure = spawnTestBrick(9, 0, true);
    Entity networks = spawnTestBrick(17, sprites::coursePrereqMask(17), false);

    const SpritePart lockedNet = sprites::makePart(sprites::courseSpriteId(17, true));
    const SpritePart openNet = sprites::makePart(sprites::courseSpriteId(17, false));
    assert(spritePartEq(brickSpritePart(networks), lockedNet));
    assert(!spritePartEq(openNet, lockedNet));

    structure.add(DeadTag{});
    assert(brickPrereqsMet(networks));
    assert(brickShowsLocked(networks));
    assert(spritePartEq(brickSpritePart(networks), lockedNet));

    brickUnlockSystem();
    assert(networks.get<BrickProgress>().unlocked);
    assert(spritePartEq(brickSpritePart(networks), openNet));

    structure.destroy();
    networks.destroy();
}

static void test_computability_open_sprite_not_networks() {
    sprites::initCatalog();
    const SpritePart net = sprites::makePart(sprites::courseSpriteId(17, false));
    const SpritePart comp = sprites::makePart(sprites::courseSpriteId(18, false));
    assert(!spritePartEq(net, comp));

    resetWorld();
    Entity ds = spawnTestBrick(11, 0, true);
    Entity compBrick = spawnTestBrick(18, sprites::coursePrereqMask(18), false);
    ds.add(DeadTag{});
    brickUnlockSystem();

    assert(compBrick.get<BrickProgress>().unlocked);
    assert(spritePartEq(brickSpritePart(compBrick), comp));
    assert(!spritePartEq(brickSpritePart(compBrick), net));

    ds.destroy();
    compBrick.destroy();
}

static void test_grid_has_single_networks_brick() {
    int networksCount = 0;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col)
            if (Config::gridCourseIndex(row, col) == 17)
                ++networksCount;
    assert(networksCount == 1);
    assert(Config::gridCourseIndex(2, 3) == 17);
}

static void test_all_open_course_sprites_unique() {
    sprites::initCatalog();
    for (int a = 0; a < sprites::COURSE_COUNT; ++a) {
        const SpritePart sa = sprites::makePart(sprites::courseSpriteId(a, false));
        for (int b = a + 1; b < sprites::COURSE_COUNT; ++b) {
            const SpritePart sb = sprites::makePart(sprites::courseSpriteId(b, false));
            assert(!spritePartEq(sa, sb));
        }
    }
}

static void test_locked_art_open_differs_from_locked_sprite() {
    sprites::initCatalog();
    for (int ci = 0; ci < sprites::COURSE_COUNT; ++ci) {
        if (!sprites::courseUsesLockedArt(ci) && ci != sprites::FINAL_COURSE_INDEX) continue;
        const SpritePart open = sprites::makePart(sprites::courseSpriteId(ci, false));
        const SpritePart locked = sprites::makePart(sprites::courseSpriteId(ci, true));
        assert(!spritePartEq(open, locked));
    }
}

static void test_all_locked_course_sprites_unique() {
    sprites::initCatalog();
    for (int a = 0; a < sprites::COURSE_COUNT; ++a) {
        if (!sprites::courseUsesLockedArt(a) && a != sprites::FINAL_COURSE_INDEX) continue;
        const SpritePart sa = sprites::makePart(sprites::courseSpriteId(a, true));
        for (int b = a + 1; b < sprites::COURSE_COUNT; ++b) {
            if (!sprites::courseUsesLockedArt(b) && b != sprites::FINAL_COURSE_INDEX) continue;
            const SpritePart sb = sprites::makePart(sprites::courseSpriteId(b, true));
            assert(!spritePartEq(sa, sb));
        }
    }
}

static void test_bottom_row_open_sprites_distinct() {
    sprites::initCatalog();
    for (int i = 15; i <= sprites::FINAL_COURSE_INDEX; ++i) {
        const SpritePart si = sprites::makePart(sprites::courseSpriteId(i, false));
        for (int j = i + 1; j <= sprites::FINAL_COURSE_INDEX; ++j) {
            const SpritePart sj = sprites::makePart(sprites::courseSpriteId(j, false));
            assert(!spritePartEq(si, sj));
        }
    }
    const SpritePart net = sprites::makePart(sprites::courseSpriteId(17, false));
    const SpritePart comp = sprites::makePart(sprites::courseSpriteId(18, false));
    assert(net.part.x < comp.part.x);
}

static void test_each_locked_art_unlock_keeps_course_sprite() {
    sprites::initCatalog();
    for (int ci = 0; ci < sprites::FINAL_COURSE_INDEX; ++ci) {
        if (!sprites::courseUsesLockedArt(ci)) continue;

        const uint32_t mask = sprites::coursePrereqMask(ci);
        if (mask == 0) continue;

        resetWorld();
        for (int req = 0; req < sprites::FINAL_COURSE_INDEX; ++req) {
            if ((mask & (1u << req)) == 0) continue;
            spawnTestBrick(req, sprites::coursePrereqMask(req), true);
        }
        Entity brick = spawnTestBrick(ci, mask, false);
        const SpritePart locked = sprites::makePart(sprites::courseSpriteId(ci, true));
        assert(spritePartEq(brickSpritePart(brick), locked));

        for (Entity e = Entity::first(); !e.eof(); e.next()) {
            if (e.mask().ctz() < 0) continue;
            if (!e.has<BrickInfo>()) continue;
            if (e.get<BrickInfo>().courseIndex == ci) continue;
            e.add(DeadTag{});
        }

        brickUnlockSystem();
        const SpritePart open = sprites::makePart(sprites::courseSpriteId(ci, false));
        assert(brick.get<BrickProgress>().unlocked);
        assert(spritePartEq(brickSpritePart(brick), open));
        assert(!spritePartEq(brickSpritePart(brick), locked));

        brick.destroy();
        resetWorld();
    }
}

static void test_networks_computability_open_sprites_match_sheet() {
    sprites::initCatalog();
    const SpritePart net = sprites::makePart(sprites::courseSpriteId(17, false));
    const SpritePart comp = sprites::makePart(sprites::courseSpriteId(18, false));
    assert(net.part.x != comp.part.x);
    assert(net.part.x < comp.part.x);

    resetWorld();
    Entity structure = spawnTestBrick(9, sprites::coursePrereqMask(9), true);
    Entity networks = spawnTestBrick(17, sprites::coursePrereqMask(17), false);
    structure.add(DeadTag{});
    brickUnlockSystem();

    assert(networks.get<BrickProgress>().unlocked);
    assert(spritePartEq(brickSpritePart(networks), net));
    assert(!spritePartEq(brickSpritePart(networks), comp));

    structure.destroy();
    networks.destroy();
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

static void test_os_open_sprite_not_complexity() {
    sprites::initCatalog();
    const SpritePart os = sprites::makePart(sprites::courseSpriteId(16, false));
    const SpritePart complexity = sprites::makePart(sprites::courseSpriteId(19, false));
    assert(!spritePartEq(os, complexity));
    assert(os.part.x < complexity.part.x);

    resetWorld();
    Entity arch = spawnTestBrick(13, 0, true);
    Entity osBrick = spawnTestBrick(16, sprites::coursePrereqMask(16), false);
    arch.add(DeadTag{});
    brickUnlockSystem();

    assert(osBrick.get<BrickProgress>().unlocked);
    assert(spritePartEq(brickSpritePart(osBrick), os));
    assert(!spritePartEq(brickSpritePart(osBrick), complexity));

    arch.destroy();
    osBrick.destroy();
}

static void test_complexity_open_sprite_not_os() {
    sprites::initCatalog();
    const SpritePart os = sprites::makePart(sprites::courseSpriteId(16, false));
    const SpritePart complexity = sprites::makePart(sprites::courseSpriteId(19, false));
    assert(!spritePartEq(os, complexity));

    resetWorld();
    Entity algo = spawnTestBrick(14, 0, true);
    Entity complexityBrick = spawnTestBrick(19, sprites::coursePrereqMask(19), false);
    algo.add(DeadTag{});
    brickUnlockSystem();

    assert(complexityBrick.get<BrickProgress>().unlocked);
    assert(spritePartEq(brickSpritePart(complexityBrick), complexity));
    assert(!spritePartEq(brickSpritePart(complexityBrick), os));

    algo.destroy();
    complexityBrick.destroy();
}

static void test_os_locked_open_sprites_differ_and_match_sheet() {
    sprites::initCatalog();
    const SpritePart locked = sprites::makePart(sprites::courseSpriteId(16, true));
    const SpritePart open = sprites::makePart(sprites::courseSpriteId(16, false));
    assert(!spritePartEq(locked, open));
    assert(locked.part.x != open.part.x);
}

static void test_grid_has_single_os_brick() {
    int osCount = 0;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col)
            if (Config::gridCourseIndex(row, col) == 16)
                ++osCount;
    assert(osCount == 1);
    assert(Config::gridCourseIndex(2, 2) == 16);
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
    test_grid_maps_each_course_once();
    test_all_open_course_sprites_unique();
    test_all_locked_course_sprites_unique();
    test_locked_art_open_differs_from_locked_sprite();
    test_bottom_row_open_sprites_distinct();
    test_each_locked_art_unlock_keeps_course_sprite();
    test_unlocked_sprite_matches_catalog_index();
    test_locked_sprite_matches_catalog_index();
    test_unlock_system_keeps_same_course_sprite();
    test_unlock_never_swaps_to_other_course_rect();
    test_networks_stays_locked_art_until_unlock_flag();
    test_computability_open_sprite_not_networks();
    test_grid_has_single_networks_brick();
    test_networks_computability_open_sprites_match_sheet();
    test_os_open_sprite_not_complexity();
    test_complexity_open_sprite_not_os();
    test_os_locked_open_sprites_differ_and_match_sheet();
    test_grid_has_single_os_brick();
    test_locked_art_stays_locked_until_unlock_flag();
    test_grid_has_single_complexity_brick();
    test_grid_has_single_final_brick();
    std::printf("All %d brick prereq tests passed.\n", 37);
    return 0;
}
