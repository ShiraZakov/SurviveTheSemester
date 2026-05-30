// MAY - HUD (graphical, no SDL_ttf). Lives pips, per-course progress bars, status.
// TODO (MAY): nicer layout, course labels by color legend, exam timer bar.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Game.h"
#include <SDL3/SDL.h>
#include <cstdio>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static const char* phaseName(Phase p) {
    switch (p) {
        case Phase::MENU:    return "MENU";
        case Phase::PLAYING: return "PLAYING";
        case Phase::EXAM:    return "EXAM";
        case Phase::WON:     return "WON";
        default:             return "LOST";
    }
}

void hudSystem(SDL_Renderer* r) {
    GameState& gs = gameState();

    // Lives as pips
    for (int i = 0; i < gs.lives; ++i) {
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.35f, 0.35f, 1.0f);
        SDL_FRect pip{8.0f + i * 18.0f, 8.0f, 12.0f, 12.0f};
        SDL_RenderFillRect(r, &pip);
    }

    // Per-course progress bars
    {
        static const Mask mask = MaskBuilder().set<Course>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            const auto& c = e.get<Course>();
            float bx = 8.0f, by = 46.0f + c.id * 16.0f, bw = 160.0f, bh = 10.0f;
            SDL_SetRenderDrawColorFloat(r, 0.25f, 0.25f, 0.30f, 1.0f);
            SDL_FRect bg{bx, by, bw, bh};
            SDL_RenderFillRect(r, &bg);
            float rr, gg, bb; Config::courseColor(c.id, rr, gg, bb);
            SDL_SetRenderDrawColorFloat(r, rr, gg, bb, 1.0f);
            SDL_FRect fg{bx, by, bw * c.progress, bh};
            SDL_RenderFillRect(r, &fg);
        }
    }

    // Status line
    SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 1.0f, 1.0f);
    char buf[96];
    std::snprintf(buf, sizeof buf, "%s  avg:%.0f  done:%d/%d",
                  phaseName(gs.phase), gs.average, gs.coursesDone, gs.coursesTotal);
    SDL_RenderDebugText(r, 8.0f, 28.0f, buf);

    if (gs.phase == Phase::WON || gs.phase == Phase::LOST) {
        SDL_RenderDebugText(r, Config::WINDOW_W * 0.5f - 60.0f, Config::WINDOW_H * 0.5f,
                            gs.phase == Phase::WON ? "YOU WON  -  press R" : "GAME OVER  -  press R");
    }
}
