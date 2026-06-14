#pragma once
// Box2D integration
// ALL Box2D usage lives behind this seam. Verticals read Position and consume
// event entities; they call phys::setVelocity / setPosition instead of touching
// bodies directly.

#include "bagel.h"
#include <box2d/id.h>

namespace phys {
    void           init();                 // create the b2World (zero gravity)
    void           shutdown();             // destroy the b2World
    b2WorldId      world();                // for the factory only
    bagel::ent_type entityOf(b2BodyId);    // body userData -> entity

    void setVelocity(bagel::ent_type e, float vx, float vy);
    void getVelocity(bagel::ent_type e, float& vx, float& vy);
    void setPosition(bagel::ent_type e, float x, float y);
    void destroyBody(bagel::ent_type e);  // destroy Box2D body if entity has one
}

// Systems (called by the scheduler in game.cpp).
void physicsStepSystem(float dt);   // step world + sync Position + hold ball speed
void contactEventSystem();          // contacts/sensors -> CourseHit/DropCaught/ProjectileHit
