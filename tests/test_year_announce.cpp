// Year transition overlay: must appear only when the 45s year timer expires,
// last exactly 5s, and freeze the year timer during the overlay.
#include "Components.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Game.h"
#include "Physics.h"
#include "Sprites.h"
#include "systems/systems.h"

#include <cassert>
#include <cmath>
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

static GameState& bindGradYearState(int year, float yearTimer) {
    resetWorld();
    Entity e = Entity::create();
    GameState gs{
        Config::START_LIVES, Config::START_AVERAGE, Phase::GRADUATION, -1, 0, Config::COURSES,
        {}, year, yearTimer, 0.0f, 0.0f, false, true,
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

static GameState& bindPlayingYearState(int year, float yearTimer, bool spawnYearBrick) {
    resetWorld();
    sprites::initCatalog();
    phys::init();
    Entity e = Entity::create();
    GameState gs{
        Config::START_LIVES, Config::START_AVERAGE, Phase::PLAYING, -1, 0, Config::COURSES,
        {}, year, yearTimer, 0.0f, 0.0f, false, true,
        0.0f, 0.0f, 0, 0, 0.0f, false, 0,
        false, 0, 0, 0, 0.0f,
        Config::WORLD_W * 0.5f, Config::WORLD_W * 0.5f,
        Config::graduationStudentStartY(), -1,
        0, false, false};
    gs.taxOutcome.fill(GameState::TAX_PENDING);
    e.addAll(gs, GameStateTag{});
    bindGameState(e.entity());

    if (spawnYearBrick) {
        const int courseIndex = Config::yearCourseStart(year);
        spawnBrick(0, courseIndex, Config::WORLD_W * 0.5f, 4.0f);
    }
    return gameState();
}

static void test_timer_expiry_triggers_announce_for_each_year() {
    for (int year = 1; year <= 4; ++year) {
        GameState& gs = bindGradYearState(year, Config::YEAR_SECONDS - 1.0f);
        gs.gradAwaitingSpace = false;

        yearSystem(1.0f);

        assert(gs.currentYear == year + 1);
        assert(gs.yearTimer == 0.0f);
        assert(std::fabs(gs.yearAnnounceTimer - Config::YEAR_ANNOUNCE_SECONDS) < 0.001f);
    }
}

static void test_no_announce_before_timer_reaches_45_seconds() {
    for (int year = 1; year <= 4; ++year) {
        GameState& gs = bindGradYearState(year, 0.0f);
        gs.gradAwaitingSpace = false;

        yearSystem(Config::YEAR_SECONDS - 0.01f);
        assert(gs.currentYear == year);
        assert(gs.yearAnnounceTimer == 0.0f);
        assert(gs.yearTimer + 0.01f >= Config::YEAR_SECONDS - 0.02f);
    }
}

static void test_announce_lasts_exactly_five_seconds_each_year() {
    for (int year = 1; year <= 4; ++year) {
        GameState& gs = bindGradYearState(year, Config::YEAR_SECONDS);
        gs.gradAwaitingSpace = false;

        yearSystem(0.0f);
        assert(gs.currentYear == year + 1);
        assert(std::fabs(gs.yearAnnounceTimer - Config::YEAR_ANNOUNCE_SECONDS) < 0.001f);

        yearSystem(2.0f);
        assert(std::fabs(gs.yearAnnounceTimer - (Config::YEAR_ANNOUNCE_SECONDS - 2.0f)) < 0.001f);
        assert(gs.yearTimer == 0.0f);

        yearSystem(3.0f);
        assert(gs.yearAnnounceTimer == 0.0f);
        assert(gs.yearTimer == 0.0f);
    }
}

static void test_year_timer_frozen_during_announce() {
    GameState& gs = bindGradYearState(2, Config::YEAR_SECONDS);
    gs.gradAwaitingSpace = false;
    yearSystem(0.0f);
    assert(gs.yearAnnounceTimer > 0.0f);

    const float before = gs.yearAnnounceTimer;
    yearSystem(1.5f);
    assert(gs.yearTimer == 0.0f);
    assert(std::fabs(gs.yearAnnounceTimer - (before - 1.5f)) < 0.001f);

    yearSystem(gs.yearAnnounceTimer);
    assert(gs.yearAnnounceTimer == 0.0f);

    yearSystem(1.0f);
    assert(std::fabs(gs.yearTimer - 1.0f) < 0.001f);
}

static void test_full_45_plus_5_cycle_years_one_through_four() {
    GameState& gs = bindGradYearState(1, 0.0f);
    gs.gradAwaitingSpace = false;

    for (int year = 1; year <= 4; ++year) {
        assert(gs.currentYear == year);
        assert(gs.yearAnnounceTimer == 0.0f);

        yearSystem(Config::YEAR_SECONDS - 0.001f);
        assert(gs.currentYear == year);
        assert(gs.yearAnnounceTimer == 0.0f);

        yearSystem(0.001f);
        assert(gs.currentYear == year + 1);
        assert(std::fabs(gs.yearAnnounceTimer - Config::YEAR_ANNOUNCE_SECONDS) < 0.001f);
        assert(gs.yearTimer == 0.0f);
        assert(Config::academicMonthIndex(Config::YEAR_SECONDS - 0.001f)
            == Config::ACADEMIC_MONTHS - 1); // Jul at end of year

        yearSystem(Config::YEAR_ANNOUNCE_SECONDS);
        assert(gs.yearAnnounceTimer == 0.0f);
        assert(gs.yearTimer == 0.0f);
    }
    assert(gs.currentYear == 5);
}

static void test_playing_timer_expiry_shows_announce_with_uncleared_courses() {
    GameState& gs = bindPlayingYearState(2, Config::YEAR_SECONDS - 0.5f, true);
    yearSystem(0.5f);

    assert(gs.currentYear == 3);
    assert(std::fabs(gs.yearAnnounceTimer - Config::YEAR_ANNOUNCE_SECONDS) < 0.001f);
    phys::shutdown();
}

static void test_playing_cleared_courses_do_not_advance_year() {
    GameState& gs = bindPlayingYearState(1, 12.0f, false); // no bricks => would have been "cleared"
    yearSystem(0.0f);

    assert(gs.currentYear == 1);
    assert(std::fabs(gs.yearTimer - 12.0f) < 0.001f);
    assert(gs.yearAnnounceTimer == 0.0f);
    phys::shutdown();
}

static void test_playing_cleared_year_bricks_do_not_skip_to_next_year() {
    GameState& gs = bindPlayingYearState(2, 5.0f, false);
    const int courseIndex = Config::yearCourseStart(2);
    spawnBrick(0, courseIndex, Config::WORLD_W * 0.5f, 4.0f);
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<BrickTag>()) continue;
        e.add(DeadTag{});
    }

    yearSystem(0.0f);

    assert(gs.currentYear == 2);
    assert(std::fabs(gs.yearTimer - 5.0f) < 0.001f);
    assert(gs.yearAnnounceTimer == 0.0f);
    phys::shutdown();
}

static void test_all_four_playing_transitions_show_announce() {
    for (int year = 1; year <= 4; ++year) {
        GameState& gs = bindPlayingYearState(year, Config::YEAR_SECONDS - 0.5f, true);
        yearSystem(0.5f);

        assert(gs.currentYear == year + 1);
        assert(std::fabs(gs.yearAnnounceTimer - Config::YEAR_ANNOUNCE_SECONDS) < 0.001f);
        phys::shutdown();
    }
}

static void test_year_five_expiry_has_no_announce() {
    GameState& gs = bindGradYearState(5, Config::YEAR_SECONDS - 0.25f);
    gs.gradAwaitingSpace = false;

    yearSystem(0.25f);

    assert(gs.phase == Phase::LOST);
    assert(gs.yearAnnounceTimer == 0.0f);
}

int main() {
    test_timer_expiry_triggers_announce_for_each_year();
    test_no_announce_before_timer_reaches_45_seconds();
    test_announce_lasts_exactly_five_seconds_each_year();
    test_year_timer_frozen_during_announce();
    test_full_45_plus_5_cycle_years_one_through_four();
    test_playing_timer_expiry_shows_announce_with_uncleared_courses();
    test_playing_cleared_courses_do_not_advance_year();
    test_playing_cleared_year_bricks_do_not_skip_to_next_year();
    test_all_four_playing_transitions_show_announce();
    test_year_five_expiry_has_no_announce();
    std::printf("All %d year-announce timing tests passed.\n", 10);
    return 0;
}
