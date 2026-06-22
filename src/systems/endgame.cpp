// endgame.cpp — folds gameplay events into GameState and resolves win/lose.
// Reads LifeLost / DropCaught(Tax) / TaxMissed / ExamFinished events into lives, average,
// and coursesDone, then checks end conditions in priority order: out of lives > all bricks
// cleared (win, or stage-1 avg-too-low) > years exhausted > all exams done but avg < pass.
// During graduation only the out-of-lives check applies (delegated to graduationOnLivesDepleted).

#include "systems/systems.h"
#include "Components.h"
#include "Events.h"
#include "Game.h"
#include "Config.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static bool allBricksCleared() {
    static const Mask mask = MaskBuilder().set<BrickTag>().build();
    static int q = World::createQuery(mask);
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        if (!e.has<DeadTag>()) return false;
    return true;
}

/// @brief Folds LifeLost, DropCaught (Tax), TaxMissed, and ExamFinished events into
///        GameState (lives, average, coursesDone). Resolves win/lose conditions.
/// @return void
void gameStateSystem() {
    GameState& gs = gameState();

    {
        static const Mask mask = MaskBuilder().set<ToggleSlowBallCheat>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            (void)e;
            gs.slowBallCheat = !gs.slowBallCheat;
        }
    }

    {
        static const Mask mask = MaskBuilder().set<LifeLost>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            const int amount = e.get<LifeLost>().amount;
            gs.lives -= amount;
            if (gs.phase != Phase::GRADUATION) {
                gs.average = std::max(0.0f,
                    gs.average - Config::FOUL_PENALTY * static_cast<float>(amount));
                gs.started = false;
            }
        }
    }

    {
        static const Mask mask = MaskBuilder().set<DropCaught>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            const auto& c = e.get<DropCaught>();
            if (c.type != DropType::Tax) continue;
            if (c.courseIndex < 0 || c.courseIndex >= GameState::COURSE_GRADES) continue;
            if (gs.taxOutcome[static_cast<size_t>(c.courseIndex)] != GameState::TAX_PENDING) continue;
            gs.taxOutcome[static_cast<size_t>(c.courseIndex)] = GameState::TAX_CAUGHT;
        }
    }

    {
        static const Mask mask = MaskBuilder().set<TaxMissed>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            const int ci = e.get<TaxMissed>().courseIndex;
            if (ci < 0 || ci >= GameState::COURSE_GRADES) continue;
            if (gs.taxOutcome[static_cast<size_t>(ci)] != GameState::TAX_PENDING) continue;
            gs.taxOutcome[static_cast<size_t>(ci)] = GameState::TAX_MISSED;
            gs.average = std::max(0.0f, gs.average - Config::TAX_MISS_PENALTY);
        }
    }

    {
        static const Mask mask = MaskBuilder().set<ExamFinished>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
            gs.coursesDone += 1;
    }

    if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM) {
        if (gs.lives <= 0) {
            gs.phase = Phase::LOST;
            gs.loseReason = LoseReason::NoLives;
        } else if (allBricksCleared()) {
            if (gs.average < Config::PASS) {
                gs.phase = Phase::LOST;
                gs.loseReason = LoseReason::Stage1AvgTooLow;
            } else {
                gs.phase = Phase::GRADUATION;
                gs.started = true;
                gs.yearsExhausted = false;
                enterGraduationStage();
            }
        } else if (gs.yearsExhausted) {
            gs.phase = Phase::LOST;
            gs.loseReason = LoseReason::YearsExhausted;
        } else if (gs.coursesTotal > 0 && gs.coursesDone >= gs.coursesTotal
                 && gs.average < Config::PASS) {
            gs.phase = Phase::LOST;
            gs.loseReason = LoseReason::AverageTooLow;
        }

        if (gs.phase == Phase::LOST || gs.phase == Phase::WON) {
            gs.started = false;
            gs.paused = false;
        }
    } else if (gs.phase == Phase::GRADUATION && gs.lives <= 0) {
        graduationOnLivesDepleted();
    }
}
