// Unit tests for graduation year timer behaviour.
#include "Components.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Game.h"
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
    bindGameState(ent_type{-1});
}

static GameState& bindFreshGameState() {
    resetWorld();
    Entity e = Entity::create();
    GameState gs{
        Config::START_LIVES, Config::START_AVERAGE, Phase::GRADUATION, -1, 0, Config::COURSES,
        {}, 5, 0.0f, 0.0f, false, true,
        0.0f, 0.0f, 0, 0, 0.0f, false, 0,
        true, 0, Config::graduationChairTotal(), 0, 0.0f,
        Config::WORLD_W * 0.5f, Config::WORLD_W * 0.5f,
        Config::graduationStudentStartY(), -1,
        0, false, false};
    gs.taxOutcome.fill(GameState::TAX_PENDING);
    e.addAll(gs, GameStateTag{});
    bindGameState(e.entity());
    return gameState();
}

static void test_graduation_timer_runs_during_play() {
    GameState& gs = bindFreshGameState();
    gs.gradAwaitingSpace = false;
    gs.yearTimer = 0.0f;

    yearSystem(1.0f);
    assert(gs.yearTimer == 1.0f);
    assert(gs.phase == Phase::GRADUATION);
}

static void test_graduation_timer_pauses_during_foul_wait() {
    GameState& gs = bindFreshGameState();
    gs.gradAwaitingSpace = true;
    gs.yearTimer = 44.0f;
    gs.currentYear = 5;

    yearSystem(2.0f);
    assert(gs.yearTimer == 44.0f);
    assert(gs.phase == Phase::GRADUATION);
}

static void test_graduation_year5_expiry_causes_loss() {
    GameState& gs = bindFreshGameState();
    gs.gradAwaitingSpace = false;
    gs.currentYear = 5;
    gs.yearTimer = Config::YEAR_SECONDS - 0.5f;

    yearSystem(1.0f);
    assert(gs.phase == Phase::LOST);
    assert(gs.yearsExhausted);
}

static void test_graduation_year_advances_before_year5() {
    GameState& gs = bindFreshGameState();
    gs.gradAwaitingSpace = false;
    gs.currentYear = 3;
    gs.yearTimer = Config::YEAR_SECONDS;

    yearSystem(0.0f);
    assert(gs.currentYear == 4);
    assert(gs.yearTimer == 0.0f);
    assert(gs.phase == Phase::GRADUATION);
}

static bool anyGradChairHidden() {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>() || !e.has<GradChairInfo>()) continue;
        if (e.get<GradChairInfo>().hidden) return true;
    }
    return false;
}

static void spawnAllGradChairs() {
    for (int i = 0; i < Config::graduationChairTotal(); ++i) {
        float x = 0.0f, y = 0.0f;
        Config::graduationChairPos(i, x, y);
        spawnGradChair(i, x, y);
    }
}

static void test_lost_mid_vault_restores_all_chairs() {
    GameState& gs = bindFreshGameState();
    gs.gradAnimStep = 1;
    gs.gradActiveChair = Config::graduationPathChairIndex(0);
    gs.currentYear = 5;
    gs.yearTimer = Config::YEAR_SECONDS - 0.1f;
    gs.gradAwaitingSpace = false;

    spawnAllGradChairs();
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairInfo>()) continue;
        if (e.get<GradChairInfo>().index == gs.gradActiveChair)
            e.get<GradChairInfo>().hidden = true;
    }
    assert(anyGradChairHidden());

    yearSystem(1.0f);

    assert(gs.phase == Phase::LOST);
    assert(gs.gradAnimStep == 0);
    assert(gs.gradActiveChair == -1);
    assert(!anyGradChairHidden());
}

static void test_year5_expired_restores_hidden_chair() {
    GameState& gs = bindFreshGameState();
    gs.gradAnimStep = 1;
    gs.gradActiveChair = Config::graduationPathChairIndex(1);

    spawnAllGradChairs();
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairInfo>()) continue;
        if (e.get<GradChairInfo>().index == gs.gradActiveChair)
            e.get<GradChairInfo>().hidden = true;
    }
    assert(anyGradChairHidden());

    graduationOnYear5Expired();

    assert(gs.phase == Phase::LOST);
    assert(gs.yearsExhausted);
    assert(gs.gradAnimStep == 0);
    assert(gs.gradActiveChair == -1);
    assert(!anyGradChairHidden());
}

static void test_enter_graduation_spawns_stage_and_all_chairs() {
    resetWorld();
    sprites::initCatalog();
    Entity gsEnt = spawnGameState();
    bindGameState(gsEnt.entity());
    GameState& gs = gameState();
    gs.phase = Phase::GRADUATION;

    enterGraduationStage();

    int stageCount = 0;
    int chairCount = 0;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (e.has<GradStageTag>()) ++stageCount;
        if (e.has<GradChairTag>()) ++chairCount;
    }

    assert(gs.gradInitialized);
    assert(stageCount == 1);
    assert(chairCount == Config::graduationChairTotal());
    resetWorld();
}

int main() {
    test_graduation_timer_runs_during_play();
    test_graduation_timer_pauses_during_foul_wait();
    test_graduation_year5_expiry_causes_loss();
    test_graduation_year_advances_before_year5();
    test_lost_mid_vault_restores_all_chairs();
    test_year5_expired_restores_hidden_chair();
    test_enter_graduation_spawns_stage_and_all_chairs();
    std::printf("All %d graduation year tests passed.\n", 7);
    return 0;
}
