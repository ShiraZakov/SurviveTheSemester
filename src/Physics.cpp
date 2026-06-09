#include "Physics.h"
#include "Components.h"
#include "Events.h"
#include "Config.h"
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

void phys::init() {
    b2WorldDef wd = b2DefaultWorldDef();
    wd.gravity = {0.0f, 0.0f};
    g_world = b2CreateWorld(&wd);
}

void phys::shutdown() {
    if (B2_IS_NON_NULL(g_world)) {
        b2DestroyWorld(g_world);
        g_world = b2_nullWorldId;
    }
}

b2WorldId phys::world() { return g_world; }

ent_type phys::entityOf(b2BodyId b) {
    int v = static_cast<int>(reinterpret_cast<intptr_t>(b2Body_GetUserData(b)));
    return { v - 1 };   // userData stores id+1; 0 -> {-1} (no entity)
}

void phys::setVelocity(ent_type e, float vx, float vy) {
    Entity en{e};
    if (en.has<PhysicsBody>())
        b2Body_SetLinearVelocity(en.get<PhysicsBody>().body, {vx, vy});
}

void phys::getVelocity(ent_type e, float& vx, float& vy) {
    vx = vy = 0.0f;
    Entity en{e};
    if (en.has<PhysicsBody>()) {
        b2Vec2 v = b2Body_GetLinearVelocity(en.get<PhysicsBody>().body);
        vx = v.x; vy = v.y;
    }
}

void phys::setPosition(ent_type e, float x, float y) {
    Entity en{e};
    if (en.has<PhysicsBody>())
        b2Body_SetTransform(en.get<PhysicsBody>().body, {x, y}, b2Rot_identity);
}

/// @brief Destroys the Box2D body attached to an entity, if one exists
/// @param e The entity whose body should be destroyed
/// @return void
void phys::destroyBody(ent_type e) {
    Entity en{e};
    if (en.has<PhysicsBody>())
        b2DestroyBody(en.get<PhysicsBody>().body);
}

/// @brief Steps the Box2D world and syncs Position components from body transforms.
///        Also regulates ball speed to stay within a playable band.
/// @param dt Fixed timestep in seconds
/// @return void
void physicsStepSystem(float dt) {
    if (!b2World_IsValid(g_world)) return;
    b2World_Step(g_world, dt, Config::SUBSTEPS);

    // Box2D is the source of truth: copy body transforms into Position.
    {
        static const Mask mask = MaskBuilder()
            .set<PhysicsBody>()
            .set<Position>()
            .build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            b2Vec2 p = b2Body_GetPosition(e.get<PhysicsBody>().body);
            e.get<Position>() = {p.x, p.y};
        }
    }

    // Keep the ball in a playable speed band so paddle hits can add feel
    // without letting the ball stall or run away.
    {
        static const Mask mask = MaskBuilder()
            .set<BallTag>()
            .set<PhysicsBody>()
            .build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
            b2BodyId b = e.get<PhysicsBody>().body;
            b2Vec2 v = b2Body_GetLinearVelocity(b);
            float len = b2Length(v);
            if (len > 0.01f) {
                constexpr float MIN_SPEED_FACTOR = 0.90f;
                constexpr float MAX_SPEED_FACTOR = 1.30f;
                constexpr float MIN_VERTICAL_RATIO = 0.35f;
                const float minSpeed = Config::BALL_SPEED * MIN_SPEED_FACTOR;
                const float maxSpeed = Config::BALL_SPEED * MAX_SPEED_FACTOR;
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
}

static void onContactPair(ent_type a, ent_type b) {
    if (a.id < 0 || b.id < 0) return;
    Entity ea{a}, eb{b};

    // Projectile-paddle: exam hit
    {
        Entity proj{a}, tgt{b};
        if      (ea.has<ProjectileTag>()) { proj = ea; tgt = eb; }
        else if (eb.has<ProjectileTag>()) { proj = eb; tgt = ea; }
        if (proj.has<ProjectileTag>() && tgt.has<PaddleTag>() && proj.has<ProjInfo>()) {
            ev::projectileHit(proj.get<ProjInfo>().courseId);
            proj.add(DeadTag{});   // remove projectile on impact
            return;
        }
    }

    // Ball-driven contacts
    Entity ball{a}, other{b};
    if      (ea.has<BallTag>()) { ball = ea; other = eb; }
    else if (eb.has<BallTag>()) { ball = eb; other = ea; }
    else return;

    if (other.has<BrickTag>() && other.has<BrickInfo>()) {
        if (!brickIsPlayable(other)) return;
        ev::courseHit(other.get<BrickInfo>().courseId, other.entity());
    }
    else if (other.has<PaddleTag>())
        ev::paddleHit(ball.entity(), other.entity());
    // hazards are sensors — handled in onSensorPair
}

static void onSensorPair(ent_type sensor, ent_type visitor) {
    if (sensor.id < 0 || visitor.id < 0) return;
    Entity s{sensor}, v{visitor};

    // Drop caught by paddle
    if (s.has<DropTag>() && s.has<DropInfo>() && !s.has<DeadTag>() && v.has<PaddleTag>()) {
        const auto& drop = s.get<DropInfo>();
        ev::dropCaught(drop.courseId, drop.courseIndex, drop.type, drop.sourceBrick, drop.gradeValue);
        s.add(DeadTag{});
        return;
    }

    // Hazard triggered by ball passing through
    if (s.has<HazardTag>() && s.has<HazardInfo>() && v.has<BallTag>()) {
        ev::hazardTriggered(s.get<HazardInfo>().courseId, s.get<HazardInfo>().type);
        return;
    }
    // visitor/sensor roles can be swapped by Box2D — check both orientations
    if (v.has<HazardTag>() && v.has<HazardInfo>() && s.has<BallTag>()) {
        ev::hazardTriggered(v.get<HazardInfo>().courseId, v.get<HazardInfo>().type);
    }
}

/// @brief Reads Box2D contact and sensor events for the current step and emits
///        ECS event entities (CourseHit, PaddleHit, HazardTriggered, DropCaught).
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
