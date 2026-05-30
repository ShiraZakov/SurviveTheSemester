#pragma once
// Survive the Semester - game orchestration & shared accessors (CORE / SHIRA).

#include "bagel.h"
#include "Components.h"

struct SDL_Renderer;

// Singleton / lookup accessors used by all verticals.
bagel::ent_type gameStateEntity();
GameState&      gameState();
bagel::ent_type courseEntity(int id);   // returns {-1} if no course with that id

// Core cleanup systems (owned by SHIRA, run at end of each frame).
void eventCleanupSystem();   // deletes all event entities
void deadCleanupSystem();    // destroys bodies + entities tagged DeadTag

class SurviveGame {
public:
    bool init(SDL_Renderer* renderer);
    void onKeyDown(int scancode);          // SDL_Scancode, passed as int
    void tick(const bool* keys, float dt); // one fixed-timestep simulation step
    void draw();                           // render once per frame
    void shutdown();
private:
    SDL_Renderer* _renderer = nullptr;
    void setupScene();
    void debugOverlay();
};
