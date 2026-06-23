#pragma once
/// @file Game.h
/// @brief Main game class (SurviveGame), singleton/lookup accessors for GameState and
///        Course entities, and declarations for the cleanup and restart utilities.

#include "bagel.h"
#include "Components.h"


struct SDL_Renderer;

/// @brief Returns the entity ID of the singleton GameState entity.
bagel::ent_type gameStateEntity();

/// @brief Returns a reference to the singleton GameState component.
GameState&      gameState();

/// @brief Binds the global GameState entity ID; used by the test harness.
void bindGameState(bagel::ent_type id);

/// @brief Finds the Course entity for the given track ID, or {-1} if none exists.
bagel::ent_type courseEntity(int id);

/// @brief Counts the number of tax drops that were successfully caught this game.
inline int countCaughtTax(const GameState& gs) {
    int caught = 0;
    for (int8_t outcome : gs.taxOutcome)
        if (outcome == GameState::TAX_CAUGHT) ++caught;
    return caught;
}

/// @brief Destroys all event entities (EventTag) at end of frame.
void eventCleanupSystem();

/// @brief Destroys Box2D bodies and ECS entities tagged with DeadTag.
void deadCleanupSystem();

/// @brief Destroys every live entity in the world (used by Play Again restart).
void destroyAllEntities();

/// @brief Spawns a fresh GameState and a complete Stage-1 scene (walls, bricks, paddle, ball).
void setupFreshPlayScene();

/// @brief Full Play-Again restart: destroys all entities, resets physics, spawns Stage 1.
void playAgainRestart();

/// @brief Top-level game object. Owns the system scheduler (tick), all full-screen UI
///        overlays (menu, guide, pause, end screen), and SDL input routing.
class SurviveGame {
public:
    /// @brief Initializes sprites, physics, and spawns the opening scene.
    /// @param renderer SDL renderer created by main.cpp
    /// @return True on success, false if sprite loading failed
    bool init(SDL_Renderer* renderer);

    /// @brief Routes SDL key-down events to debug hotkeys or pause toggle.
    /// @param scancode SDL_Scancode cast to int
    void onKeyDown(int scancode);

    /// @brief Routes a mouse button-down event to the active UI layer or gameplay handler.
    /// @param button SDL mouse button index
    /// @param px     Pixel X coordinate in window space
    /// @param py     Pixel Y coordinate in window space
    void onMouseDown(int button, float px, float py);

    /// @brief Scrolls the How-to-Play guide when it is visible.
    /// @param dy Scroll delta (positive = upward)
    void onScroll(float dy);

    /// @brief Runs one fixed-timestep simulation tick: dispatches to the active phase's systems.
    /// @param dt Fixed timestep in seconds
    void tick(float dt);

    /// @brief Renders the scene, HUD, and any full-screen overlay for the current phase.
    void draw();

    /// @brief Tears down the game scene and frees all resources.
    void shutdown();

    /// @brief Returns true while the game is paused.
    bool isPaused()   const;

    /// @brief Returns true when the user has clicked Exit on the end screen.
    bool wantsQuit()  const;
private:
    SDL_Renderer* _renderer = nullptr;
    bool _wantsQuit = false;
    bool _showGuide = false;
    bool _showLevelSelect = false;
    float _guideScroll = 0.0f;
    void setupScene();
};
