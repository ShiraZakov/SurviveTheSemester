// Survive the Semester - entry point (CORE / SHIRA).
// Owns SDL setup and the fixed-timestep loop. Per-frame work lives in SurviveGame.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Config.h"
#include "Game.h"

int main() {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Survive the Semester",
                                     Config::WINDOW_W, Config::WINDOW_H, 0,
                                     &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SurviveGame game;
    if (!game.init(renderer)) {
        SDL_Log("Game init failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    Uint64 prev = SDL_GetTicks();
    double acc = 0.0;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                game.onKeyDown((int)ev.key.scancode);
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                game.onMouseDown((int)ev.button.button,
                                 (float)ev.button.x, (float)ev.button.y);
            } else if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
                game.onScroll((float)ev.wheel.y);
            }
        }
        if (game.wantsQuit()) { running = false; continue; }

        Uint64 now = SDL_GetTicks();
        double frame = (now - prev) / 1000.0;
        prev = now;
        if (frame > 0.25) frame = 0.25;   // avoid spiral of death
        acc += frame;

        if (!game.isPaused()) {
            while (acc >= Config::FIXED_DT) {
                game.tick(Config::FIXED_DT);
                acc -= Config::FIXED_DT;
            }
        } else {
            acc = 0.0;  // discard accumulated time while paused
        }

        game.draw();
        SDL_Delay(1);
    }

    game.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
