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

void inputSystem(const bool* keys, float dt, SDL_Renderer* r);  // AVIEL
void courseHitSystem();                        // AVIEL  (consumes CourseHit)
void brickUnlockSystem();                      // AVIEL
void brickMeterSystem();                       // MAY
void brickClearDelaySystem(float dt);          // MAY
void renderSystem(SDL_Renderer* r);            // AVIEL

void dropSystem(float dt);                     // MAY
void courseProgressSystem();                   // MAY    (consumes CourseHit/DropCaught)
void hudSystem(SDL_Renderer* r);               // MAY

void examSystem(float dt);                     // YUVAL  (consumes ExamStarted -> ExamFinished)

void yearSystem(float dt);                     // SHIRA  (year timer + row completion)
void gameStateSystem();                        // SHIRA  (consumes LifeLost/ExamFinished)
void deadCleanupSystem();
void graduationOnLivesDepleted();
void enterGraduationStage();
void graduationSystem(float dt);
void graduationInputSystem(SDL_Renderer* r);
void graduationOnMouseDown(SDL_Renderer* r);
void graduationOnSpace();
void graduationOnYear5Expired();
