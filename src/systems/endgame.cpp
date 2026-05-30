// SHIRA - endgame integration. Folds LifeLost / ExamFinished into GameState and
// resolves win/lose. (Overlays are drawn by the HUD.)
#include "systems/systems.h"
#include "Components.h"
#include "Events.h"
#include "Game.h"
#include "Config.h"

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

void gameStateSystem() {
    GameState& gs = gameState();

    {
        static const Mask mask = MaskBuilder().set<LifeLost>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
            gs.lives -= e.get<LifeLost>().amount;
    }

    {
        static const Mask mask = MaskBuilder().set<ExamFinished>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            const auto& f = e.get<ExamFinished>();
            gs.gradeSum   += f.grade;
            gs.coursesDone += 1;
            gs.average = gs.coursesDone > 0 ? gs.gradeSum / gs.coursesDone : 0.0f;
        }
    }

    if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM) {
        if (gs.lives <= 0)
            gs.phase = Phase::LOST;
        else if (gs.coursesTotal > 0 && gs.coursesDone >= gs.coursesTotal)
            gs.phase = (gs.average >= Config::PASS) ? Phase::WON : Phase::LOST;
    }
}
