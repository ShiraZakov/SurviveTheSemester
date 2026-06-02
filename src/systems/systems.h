#pragma once
// Survive the Semester - vertical-owned system signatures (the contract).
// The scheduler in game.cpp calls these in a fixed order. Each teammate fills
// in the body of their system(s); signatures must not change without telling
// the team.

struct SDL_Renderer;

void inputSystem(const bool* keys, float dt, SDL_Renderer* r);  // AVIEL
void courseHitSystem();                        // AVIEL  (consumes CourseHit)
void renderSystem(SDL_Renderer* r);            // AVIEL

void dropSystem(float dt);                     // MAY
void courseProgressSystem();                   // MAY    (consumes CourseHit/DropCaught)
void hudSystem(SDL_Renderer* r);               // MAY

void examSystem(float dt);                     // YUVAL  (consumes ExamStarted -> ExamFinished)
void hazardSystem();                           // YUVAL  (consumes HazardTriggered)

void gameStateSystem();                        // SHIRA  (consumes LifeLost/ExamFinished)
