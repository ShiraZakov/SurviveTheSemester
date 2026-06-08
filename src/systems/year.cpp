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

void yearSystem(float dt) {
    GameState& gs = gameState();
    if (gs.phase != Phase::PLAYING && gs.phase != Phase::EXAM) return;
    if (!gs.started || gs.yearsExhausted) return;

    gs.totalTime += dt;
    gs.yearTimer += dt;

    const bool yearComplete = yearCoursesCleared(gs.currentYear)
        || gs.yearTimer >= Config::YEAR_SECONDS;
    if (!yearComplete) return;

    if (gs.currentYear < Config::YEAR_COUNT) {
        gs.currentYear += 1;
        gs.yearTimer = 0.0f;
        return;
    }

    gs.yearsExhausted = true;
}
