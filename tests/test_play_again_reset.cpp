// Play Again: verify graduation tags and GameState reset for a new run.
#include "Components.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Game.h"
#include "Physics.h"
#include "Sprites.h"
#include "systems/systems.h"

#include <cassert>
#include <cstdio>

using bagel::Entity;
using bagel::ent_type;

template<typename Tag>
static int countTag() {
    int count = 0;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (e.has<Tag>()) ++count;
    }
    return count;
}

static int countCourseEntities() {
    int count = 0;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (e.has<Course>()) ++count;
    }
    return count;
}

static bool paddleHasGradStudentTag() {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (e.has<PaddleTag>() && e.has<GradStudentTag>()) return true;
    }
    return false;
}

static void playAgainReset() {
    playAgainRestart();
}

static void test_graduation_tags_cleared_on_play_again() {
    sprites::initCatalog();
    phys::init();

    setupFreshPlayScene();
    GameState& gs = gameState();
    gs.phase = Phase::GRADUATION;
    enterGraduationStage();

    assert(gs.gradInitialized);
    assert(countTag<GradChairTag>() == Config::graduationChairTotal());
    assert(countTag<GradObstacleTag>() == Config::graduationObstacleRowGapCount());
    assert(countTag<GradStudentTag>() == 1);
    assert(paddleHasGradStudentTag());

    playAgainReset();

    assert(countTag<GradChairTag>() == 0);
    assert(countTag<GradObstacleTag>() == 0);
    assert(countTag<GradStudentTag>() == 0);
    assert(!paddleHasGradStudentTag());
    assert(!gameState().gradInitialized);

    phys::shutdown();
}

static void test_lost_state_gamestate_reset_on_play_again() {
    sprites::initCatalog();
    phys::init();

    setupFreshPlayScene();
    GameState& gs = gameState();
    gs.phase = Phase::GRADUATION;
    enterGraduationStage();

    gs.phase = Phase::LOST;
    gs.paused = true;
    gs.gradAwaitingSpace = true;
    gs.gradFouls = 2;
    gs.lives = 0;
    gs.currentYear = 4;
    gs.yearTimer = 12.0f;
    gs.yearAnnounceTimer = 3.0f;
    gs.gradNextChair = 2;

    playAgainReset();

    const GameState& fresh = gameState();
    assert(fresh.phase == Phase::MENU);
    assert(!fresh.paused);
    assert(!fresh.gradInitialized);
    assert(!fresh.gradAwaitingSpace);
    assert(fresh.gradFouls == 0);
    assert(fresh.lives == Config::START_LIVES);
    assert(fresh.currentYear == 1);
    assert(fresh.yearTimer == 0.0f);
    assert(fresh.yearAnnounceTimer == 0.0f);
    assert(fresh.gradNextChair == 0);
    assert(countTag<GradChairTag>() == 0);
    assert(countTag<GradObstacleTag>() == 0);
    assert(countTag<GradStudentTag>() == 0);

    phys::shutdown();
}

static void test_stage_one_entities_restored_on_play_again() {
    sprites::initCatalog();
    phys::init();

    setupFreshPlayScene();
    gameState().phase = Phase::GRADUATION;
    enterGraduationStage();
    playAgainReset();

    assert(countTag<BrickTag>() == Config::COURSES * Config::BRICK_COLS);
    assert(countTag<BallTag>() == 1);
    assert(countTag<PaddleTag>() == 1);
    assert(countTag<WallTag>() == 4);
    assert(countCourseEntities() == Config::COURSES);
    assert(countTag<GameStateTag>() == 1);
    assert(countTag<GradChairTag>() == 0);
    assert(countTag<GradObstacleTag>() == 0);

    phys::shutdown();
}

static void test_dead_tag_entities_removed_on_play_again() {
    sprites::initCatalog();
    phys::init();

    setupFreshPlayScene();
    gameState().phase = Phase::GRADUATION;
    enterGraduationStage();

    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>()) continue;
        e.add(DeadTag{});
        break;
    }
    assert(countTag<DeadTag>() >= 1);

    playAgainReset();
    assert(countTag<DeadTag>() == 0);
    assert(countTag<GradChairTag>() == 0);

    phys::shutdown();
}

int main() {
    test_graduation_tags_cleared_on_play_again();
    test_lost_state_gamestate_reset_on_play_again();
    test_stage_one_entities_restored_on_play_again();
    test_dead_tag_entities_removed_on_play_again();
    std::printf("All %d play-again reset tests passed.\n", 4);
    return 0;
}
