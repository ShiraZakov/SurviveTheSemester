/// @file Physics.cpp
/// @brief The only file that touches Box2D directly.
//
// Two jobs:
//   1. A thin phys:: API (init/shutdown, get/set velocity, set position, destroy body)
//      so gameplay systems never call Box2D — they read Position and go through these.
//   2. Turning Box2D's per-step contact/sensor results into ECS event entities
//      (CourseHit, PaddleHit, DropCaught, ProjectileHit) — see contactEventSystem.
//
// Box2D owns motion: physicsStepSystem steps the world, then copies body transforms
// back into Position. The entity<->body link is the body's userData, which stores
// (entity id + 1) so that a userData of 0 means "no entity".

#include "Physics.h"
#include "Components.h"
#include "Events.h"
#include "Config.h"
#include "Game.h"
#include "systems/systems.h"

#include <box2d/box2d.h>
#include <cmath>
#include <cstdint>

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static b2WorldId g_world = b2_nullWorldId;

/// @brief Creates the Box2D world with zero gravity.
void phys::init() {
    b2WorldDef wd = b2DefaultWorldDef();
    wd.gravity = {0.0f, 0.0f};
    g_world = b2CreateWorld(&wd);
}

/// @brief Destroys the Box2D world and invalidates all body handles.
void phys::shutdown() {
    if (B2_IS_NON_NULL(g_world)) {
        b2DestroyWorld(g_world);
        g_world = b2_nullWorldId;
    }
}

/// @brief Returns the Box2D world handle; intended for EntityFactory use only.
b2WorldId phys::world() { return g_world; }

/// @brief Decodes the body's userData back to the ECS entity ID that owns it.
ent_type phys::entityOf(b2BodyId b) {
    int v = static_cast<int>(reinterpret_cast<intptr_t>(b2Body_GetUserData(b)));
    return { v - 1 };   // userData stores id+1; 0 -> {-1} (no entity)
}

/// @brief Sets the linear velocity of the Box2D body attached to entity @p e.
void phys::setVelocity(ent_type e, float vx, float vy) {
    Entity en{e};
    if (!en.has<PhysicsBody>()) return;
    b2BodyId body = en.get<PhysicsBody>().body;
    if (!b2Body_IsValid(body)) return;
    b2Body_SetLinearVelocity(body, {vx, vy});
}

/// @brief Reads the linear velocity of the Box2D body attached to entity @p e into (vx, vy).
void phys::getVelocity(ent_type e, float& vx, float& vy) {
    vx = vy = 0.0f;
    Entity en{e};
    if (!en.has<PhysicsBody>()) return;
    b2BodyId body = en.get<PhysicsBody>().body;
    if (!b2Body_IsValid(body)) return;
    b2Vec2 v = b2Body_GetLinearVelocity(body);
    vx = v.x; vy = v.y;
}

/// @brief Teleports the Box2D body attached to entity @p e to world position (x, y).
void phys::setPosition(ent_type e, float x, float y) {
    Entity en{e};
    if (!en.has<PhysicsBody>()) return;
    b2BodyId body = en.get<PhysicsBody>().body;
    if (!b2Body_IsValid(body)) return;
    b2Body_SetTransform(body, {x, y}, b2Rot_identity);
}

/// @brief Destroys the Box2D body attached to an entity, if one exists
/// @param e The entity whose body should be destroyed
/// @return void
void phys::destroyBody(ent_type e) {
    if (!B2_IS_NON_NULL(g_world)) return;
    Entity en{e};
    if (!en.has<PhysicsBody>()) return;
    b2BodyId body = en.get<PhysicsBody>().body;
    if (!b2Body_IsValid(body)) {
        en.del<PhysicsBody>();
        return;
    }
    b2DestroyBody(body);
    en.del<PhysicsBody>();
}

/// @brief Copies Box2D body transforms into Position components (Box2D is the
///        source of truth). Skips dead entities still lingering in the query.
/// @return void
static void syncPositionsFromBodies() {
    static const Mask mask = MaskBuilder()
        .set<PhysicsBody>()
        .set<Position>()
        .build();
    static int q = World::createQuery(mask);
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.has<DeadTag>()) continue;   // dead entities linger in the query until cleanup
        b2BodyId body = e.get<PhysicsBody>().body;
        if (!b2Body_IsValid(body)) continue;
        b2Vec2 p = b2Body_GetPosition(body);
        e.get<Position>() = {p.x, p.y};
    }
}

/// @brief Keeps each ball's speed within a playable band (and a minimum vertical
///        component) so paddle hits add feel without the ball stalling or running
///        away. Honors the slow-ball cheat.
/// @return void
static void regulateBallSpeed() {
    static const Mask mask = MaskBuilder()
        .set<BallTag>()
        .set<PhysicsBody>()
        .build();
    static int q = World::createQuery(mask);
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.has<DeadTag>()) continue;   // dead entities linger in the query until cleanup
        b2BodyId b = e.get<PhysicsBody>().body;
        if (!b2Body_IsValid(b)) continue;
        b2Vec2 v = b2Body_GetLinearVelocity(b);
        float len = b2Length(v);
        if (len > 0.01f) {
            constexpr float MIN_SPEED_FACTOR = 0.90f;
            constexpr float MAX_SPEED_FACTOR = 1.55f;
            constexpr float MIN_VERTICAL_RATIO = 0.35f;
            const float cheatScale = gameState().slowBallCheat
                ? Config::SLOW_BALL_CHEAT_SPEED_FACTOR
                : 1.0f;
            const float minSpeed = Config::BALL_SPEED * MIN_SPEED_FACTOR * cheatScale;
            const float maxSpeed = Config::BALL_SPEED * MAX_SPEED_FACTOR * cheatScale;
            float target = len;
            if (len < minSpeed) target = minSpeed;
            else if (len > maxSpeed) target = maxSpeed;

            float nx = v.x / len;
            float ny = v.y / len;
            float absNy = ny < 0.0f ? -ny : ny;
            if (absNy < MIN_VERTICAL_RATIO) {
                float signY = ny < 0.0f ? -1.0f : 1.0f;
                float signX = nx < 0.0f ? -1.0f : 1.0f;
                ny = signY * MIN_VERTICAL_RATIO;
                nx = signX * std::sqrt(1.0f - MIN_VERTICAL_RATIO * MIN_VERTICAL_RATIO);
            }

            b2Body_SetLinearVelocity(b, {nx * target, ny * target});
        }
    }
}

/// @brief Steps the Box2D world, then reconciles the ECS with it: syncs Position
///        from body transforms and regulates ball speed.
/// @param dt Fixed timestep in seconds
/// @return void
void physicsStepSystem(float dt) {
    if (!b2World_IsValid(g_world)) return;
    b2World_Step(g_world, dt, Config::SUBSTEPS);
    syncPositionsFromBodies();
    regulateBallSpeed();
}

/// @brief Handles a solid ball-brick or ball-paddle contact begin event and emits the correct ECS event.
static void onContactPair(ent_type a, ent_type b) {
    if (a.id < 0 || b.id < 0) return;
    Entity ea{a}, eb{b};

    // Ball-driven contacts
    Entity ball{a}, other{b};
    if      (ea.has<BallTag>()) { ball = ea; other = eb; }
    else if (eb.has<BallTag>()) { ball = eb; other = ea; }
    else return;

    if (other.has<BrickTag>() && other.has<BrickInfo>()) {
        if (other.has<DeadTag>()) return;
        if (!brickIsPlayable(other)) return;
        ev::courseHit(other.get<BrickInfo>().courseId, other.entity());
    }
    else if (other.has<PaddleTag>())
        ev::paddleHit(ball.entity(), other.entity());
}

/// @brief Handles a sensor overlap begin event (projectile-paddle or drop-paddle) and emits the correct ECS event.
static void onSensorPair(ent_type sensor, ent_type visitor) {
    if (sensor.id < 0 || visitor.id < 0) return;
    Entity s{sensor}, v{visitor};

    // Projectile sensor hits paddle or ball
    if (s.has<ProjectileTag>() && s.has<ProjInfo>() && !s.has<DeadTag>() &&
        (v.has<PaddleTag>() || v.has<BallTag>())) {
        ev::projectileHit(s.get<ProjInfo>().courseId);
        s.add(DeadTag{});
        return;
    }
    if (v.has<ProjectileTag>() && v.has<ProjInfo>() && !v.has<DeadTag>() &&
        (s.has<PaddleTag>() || s.has<BallTag>())) {
        ev::projectileHit(v.get<ProjInfo>().courseId);
        v.add(DeadTag{});
        return;
    }

    // Drop caught by paddle
    if (s.has<DropTag>() && s.has<DropInfo>() && !s.has<DeadTag>() && v.has<PaddleTag>()) {
        const auto& drop = s.get<DropInfo>();
        ev::dropCaught(drop.courseId, drop.courseIndex, drop.type, drop.sourceBrick, drop.gradeValue);
        s.add(DeadTag{});
        return;
    }

}

/// @brief Reads Box2D contact and sensor events for the current step and emits
///        ECS event entities (CourseHit, PaddleHit, DropCaught, ProjectileHit).
/// @return void
void contactEventSystem() {
    if (!b2World_IsValid(g_world)) return;
    b2ContactEvents ce = b2World_GetContactEvents(g_world);
    for (int i = 0; i < ce.beginCount; ++i) {
        ent_type a = phys::entityOf(b2Shape_GetBody(ce.beginEvents[i].shapeIdA));
        ent_type b = phys::entityOf(b2Shape_GetBody(ce.beginEvents[i].shapeIdB));
        onContactPair(a, b);
    }

    b2SensorEvents se = b2World_GetSensorEvents(g_world);
    for (int i = 0; i < se.beginCount; ++i) {
        ent_type sensor  = phys::entityOf(b2Shape_GetBody(se.beginEvents[i].sensorShapeId));
        ent_type visitor = phys::entityOf(b2Shape_GetBody(se.beginEvents[i].visitorShapeId));
        onSensorPair(sensor, visitor);
    }
}
