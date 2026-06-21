// SHIRA - academic year progression. Years advance only by the per-year timer
// (YEAR_SECONDS). Brick/course progress does not affect the year clock.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Game.h"

/// @brief Advances the academic year timer. Moves to the next year when the
///        per-year timer expires. Sets yearsExhausted if year 5 ends without
///        completing everything (stage 1). During graduation, year 5 expiry
///        ends the game (LOST).
/// @param dt Fixed timestep in seconds
/// @return void
void yearSystem(float dt) {
    GameState& gs = gameState();
    if (gs.phase != Phase::PLAYING && gs.phase != Phase::EXAM && gs.phase != Phase::GRADUATION) return;

    if (gs.yearAnnounceTimer > 0.0f) {
        gs.yearAnnounceTimer -= dt;
        if (gs.yearAnnounceTimer < 0.0f) gs.yearAnnounceTimer = 0.0f;
        return;
    }

    const bool inGraduation = gs.phase == Phase::GRADUATION;
    if (!inGraduation && (!gs.started || gs.yearsExhausted)) return;
    if (inGraduation && gs.gradAwaitingSpace) return;

    gs.totalTime += dt;
    gs.yearTimer += dt;

    if (gs.yearTimer < Config::YEAR_SECONDS) return;

    if (gs.currentYear < Config::YEAR_COUNT) {
        gs.currentYear += 1;
        gs.yearTimer = 0.0f;
        gs.yearAnnounceTimer = Config::YEAR_ANNOUNCE_SECONDS;
        return;
    }

    if (gs.phase == Phase::GRADUATION) {
        graduationOnYear5Expired();
        return;
    }

    gs.yearsExhausted = true;
}
