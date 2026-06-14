#pragma once
// Survive the Semester - game orchestration & shared accessors (CORE / SHIRA).

#include "bagel.h"
#include "Components.h"
#include <algorithm>

struct SDL_Renderer;

// Singleton / lookup accessors used by all verticals.
bagel::ent_type gameStateEntity();
GameState&      gameState();
void bindGameState(bagel::ent_type id);   // test harness
bagel::ent_type courseEntity(int id);   // returns {-1} if no course with that id

inline int countCaughtTax(const GameState& gs) {
    int caught = 0;
    for (int8_t outcome : gs.taxOutcome)
        if (outcome == GameState::TAX_CAUGHT) ++caught;
    return caught;
}

// Core cleanup systems (owned by SHIRA, run at end of each frame).
void eventCleanupSystem();   // deletes all event entities
void deadCleanupSystem();    // destroys bodies + entities tagged DeadTag

class SurviveGame {
public:
    bool init(SDL_Renderer* renderer);
    void onKeyDown(int scancode);                     // SDL_Scancode, passed as int
    void onMouseDown(int button, float px, float py); // SDL mouse button + pixel coords
    void tick(const bool* keys, float dt); // one fixed-timestep simulation step
    void draw();                           // render once per frame
    void shutdown();
    bool isPaused()   const;
    bool wantsQuit()  const;
private:
    SDL_Renderer* _renderer = nullptr;
    bool _wantsQuit = false;
    void setupScene();
};
