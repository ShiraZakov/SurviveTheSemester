#pragma once
/// @file Physics.h
/// @brief Box2D integration seam. All Box2D usage is behind this API; no other file
///        touches Box2D directly. Verticals read the Position component and call
///        phys::setVelocity / setPosition instead of manipulating bodies themselves.

#include "bagel.h"
#include <box2d/id.h>

namespace phys {
    /// @brief Creates the Box2D world with zero gravity.
    void           init();

    /// @brief Destroys the Box2D world and invalidates all body handles.
    void           shutdown();

    /// @brief Returns the world handle; intended for EntityFactory use only.
    b2WorldId      world();

    /// @brief Resolves a body's userData back to the ECS entity that owns it.
    bagel::ent_type entityOf(b2BodyId);

    /// @brief Sets the linear velocity of the body attached to entity @p e.
    void setVelocity(bagel::ent_type e, float vx, float vy);

    /// @brief Reads the linear velocity of the body attached to entity @p e into (vx, vy).
    void getVelocity(bagel::ent_type e, float& vx, float& vy);

    /// @brief Teleports the body attached to entity @p e to world position (x, y).
    void setPosition(bagel::ent_type e, float x, float y);

    /// @brief Destroys the Box2D body of entity @p e if one exists; removes PhysicsBody component.
    void destroyBody(bagel::ent_type e);
}

// Systems (called by the scheduler in Game.cpp — see Physics.cpp for full docs).
void physicsStepSystem(float dt);
void contactEventSystem();
