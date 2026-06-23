#pragma once
/// @file EntityFactory.h
/// @brief Single source of truth for spawning entities with their Box2D bodies and ECS components.
///        All entity creation must go through these functions; no other code may create Box2D bodies.

#include "bagel.h"
#include "Enums.h"

/// @brief Spawns the kinematic paddle at the given world position.
bagel::Entity spawnPaddle(float x, float y);

/// @brief Spawns a dynamic ball at the given world position.
bagel::Entity spawnBall(float x, float y);

/// @brief Spawns a static boundary wall rectangle at the given world position.
bagel::Entity spawnWall(float x, float y, float w, float h);

/// @brief Spawns a static course brick with the correct sprite, meter, and prereq mask.
/// @param courseId  Colored track index (0..COURSES-1)
/// @param spriteIndex Catalog course index (0..20) that selects the brick's sprite
bagel::Entity spawnBrick(int courseId, int spriteIndex, float x, float y);

/// @brief Spawns a kinematic sensor drop (assignment, bonus, or tax) falling from the given position.
/// @param sourceBrick Entity ID of the brick that produced this drop ({-1} if none)
/// @param gradeValue  Grade contribution carried by this drop (0 for non-graded types)
bagel::Entity spawnDrop(int courseId, int courseIndex, float x, float y, DropType type,
                        bagel::ent_type sourceBrick = {-1}, float gradeValue = 0.0f);

/// @brief Spawns a kinematic exam projectile aimed with velocity (vx, vy).
bagel::Entity spawnProjectile(int courseId, float x, float y, float vx, float vy);

/// @brief Spawns a graduation ceremony chair entity at grid index and world position.
bagel::Entity spawnGradChair(int index, float x, float y);

/// @brief Spawns a graduation obstacle that slides in the gap between chair rows.
/// @param rowGap Row-gap index (0 = between rows 0 and 1)
/// @param dir    Initial travel direction (+1 right, −1 left)
bagel::Entity spawnGradObstacle(float x, int rowGap, float dir);

/// @brief Spawns an invisible Course aggregate entity carrying track progress for the given ID.
bagel::Entity spawnCourse(int id);

/// @brief Spawns the singleton GameState entity with default initial values.
bagel::Entity spawnGameState();
