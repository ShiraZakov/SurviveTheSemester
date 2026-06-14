// SHIRA - academic year progression. Grid brick order is fixed; years track catalog
// course groups (not rows). Advance when that year's courses are all cleared OR the
// per-year timer expires.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Game.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static bool yearCoursesCleared(int year) {
    static const Mask mask = MaskBuilder()
        .set<BrickTag>()
        .set<BrickInfo>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.has<DeadTag>()) continue;
        if (Config::courseInYear(year, e.get<BrickInfo>().courseIndex)) return false;
    }
    return true;
}

/// @brief Advances the academic year timer. Moves to the next year when all its
///        courses are cleared or the per-year timer expires. Sets yearsExhausted
///        if year 5 ends without completing everything (stage 1). During graduation,
///        only the timer applies; year 5 expiry ends the game (LOST).
/// @param dt Fixed timestep in seconds
/// @return void
void yearSystem(float dt) {
    GameState& gs = gameState();
    if (gs.phase != Phase::PLAYING && gs.phase != Phase::EXAM && gs.phase != Phase::GRADUATION) return;

    const bool inGraduation = gs.phase == Phase::GRADUATION;
    if (!inGraduation && (!gs.started || gs.yearsExhausted)) return;
    if (inGraduation && gs.gradAwaitingSpace) return;

    gs.totalTime += dt;
    gs.yearTimer += dt;

    const bool timerExpired = gs.yearTimer >= Config::YEAR_SECONDS;
    const bool yearComplete = gs.phase == Phase::GRADUATION
        ? timerExpired
        : (yearCoursesCleared(gs.currentYear) || timerExpired);
    if (!yearComplete) return;

    if (gs.currentYear < Config::YEAR_COUNT) {
        gs.currentYear += 1;
        gs.yearTimer = 0.0f;
        return;
    }

    if (gs.phase == Phase::GRADUATION) {
        graduationOnYear5Expired();
        return;
    }

    gs.yearsExhausted = true;
}
