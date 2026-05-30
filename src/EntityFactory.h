#pragma once
// Entity factory
// Single source of truth for creating entities including their Box2D body.
// Everyone calls these; nobody else creates bodies.

#include "bagel.h"
#include "Enums.h"

bagel::Entity spawnPaddle(float x, float y);
bagel::Entity spawnBall(float x, float y);
bagel::Entity spawnWall(float x, float y, float w, float h);
bagel::Entity spawnBrick(int courseId, float x, float y);
bagel::Entity spawnDrop(int courseId, float x, float y, DropType type);
bagel::Entity spawnProjectile(int courseId, float x, float y, float vx, float vy);
bagel::Entity spawnHazard(int courseId, float x, float y, HazardType type);
bagel::Entity spawnCourse(int id);          // invisible per-course aggregate (Course state)
bagel::Entity spawnGameState();             // the singleton
