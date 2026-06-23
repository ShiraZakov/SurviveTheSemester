#pragma once
/// @file systems.h
/// @brief Declarations for all vertical-owned system functions.
///        The scheduler in Game.cpp calls these in a fixed order each frame.
///        Signatures must not change without team agreement.

#include "Components.h"

struct SDL_Renderer;

namespace bagel { class Entity; }

/// @brief Returns true if all prerequisite courses for the given brick have been cleared.
bool brickPrereqsMet(bagel::Entity brick);

/// @brief Returns true if the brick should display its locked sprite (alias of brickDrawLocked).
bool brickShowsLocked(bagel::Entity brick);

/// @brief Returns true if the brick should draw with its locked-state art.
bool brickDrawLocked(bagel::Entity brick);

/// @brief Returns the SpritePart appropriate for the brick's current lock state.
SpritePart brickSpritePart(bagel::Entity brick);

/// @brief Returns true if the brick can be hit: unlocked, not mid-clear, and prerequisites met.
bool brickIsPlayable(bagel::Entity brick);

/// @brief Returns true if the Final Project brick is still locked (any other brick remains alive).
bool isFinalProjectLocked();

// Systems with full docs in their respective .cpp files:
void inputSystem(float dt, SDL_Renderer* r);
void ballPaddleSystem();
void brickUnlockSystem();
void brickMeterSystem();
void brickClearDelaySystem(float dt);
void renderSystem(SDL_Renderer* r);
void dropSystem(float dt);
void courseProgressSystem();
void hudSystem(SDL_Renderer* r);

/// @brief Returns true if the pause button should be visible for the given game phase.
bool hudPauseButtonVisible(Phase phase);

/// @brief Returns true if the pause button is currently interactive (not during year-announce, etc.).
bool hudPauseButtonAvailable();

/// @brief Returns true if the pixel point (px, py) lies inside the pause button rectangle.
bool hudPauseButtonHit(float px, float py);

void examSystem(float dt);
void yearSystem(float dt);
void gameStateSystem();
void deadCleanupSystem();

/// @brief Called when lives reach zero during the graduation stage; ends the game as LOST.
void graduationOnLivesDepleted();

/// @brief Transitions from Stage 1 to Stage 2: clears stage-1 entities, spawns chairs and obstacles.
void enterGraduationStage();

/// @brief Per-frame driver for Stage 2: moves obstacles, applies drag, advances vault animation.
/// @param dt Fixed timestep in seconds
void graduationSystem(float dt);

/// @brief Updates the student's horizontal position from the mouse each frame (when idle).
/// @param r SDL renderer (used to convert global mouse X to world units)
void graduationInputSystem(SDL_Renderer* r);

/// @brief Starts a vault jump toward the nearest chair in the next row on left-click.
void graduationOnMouseDown(SDL_Renderer* r);

/// @brief Retries graduation from the current row after a foul (Space key).
void graduationOnSpace();

/// @brief Called when year 5 expires during the graduation stage; ends the game as LOST.
void graduationOnYear5Expired();
