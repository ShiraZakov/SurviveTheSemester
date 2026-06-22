#pragma once
// Survive the Semester - vertical-owned system signatures (the contract).
// The scheduler in game.cpp calls these in a fixed order. Each teammate fills
// in the body of their system(s); signatures must not change without telling
// the team.

#include "Components.h"

struct SDL_Renderer;

namespace bagel { class Entity; }
bool brickPrereqsMet(bagel::Entity brick);
bool brickShowsLocked(bagel::Entity brick);
bool brickDrawLocked(bagel::Entity brick);
SpritePart brickSpritePart(bagel::Entity brick);
bool brickIsPlayable(bagel::Entity brick);
bool isFinalProjectLocked();

void inputSystem(float dt, SDL_Renderer* r);
void ballPaddleSystem();
void brickUnlockSystem();
void brickMeterSystem();
void brickClearDelaySystem(float dt);
void renderSystem(SDL_Renderer* r);

void dropSystem(float dt);
void courseProgressSystem();
void hudSystem(SDL_Renderer* r);
bool hudPauseButtonVisible(Phase phase);
bool hudPauseButtonAvailable();
bool hudPauseButtonHit(float px, float py);

void examSystem(float dt);


void yearSystem(float dt);
void gameStateSystem();
void deadCleanupSystem();
void graduationOnLivesDepleted();
void enterGraduationStage();
void graduationSystem(float dt);
void graduationInputSystem(SDL_Renderer* r);
void graduationOnMouseDown(SDL_Renderer* r);
void graduationOnSpace();
void graduationOnYear5Expired();
