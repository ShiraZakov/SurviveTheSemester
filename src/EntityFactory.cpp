#include "EntityFactory.h"
#include "Components.h"
#include "Config.h"
#include "Physics.h"

#include <box2d/box2d.h>
#include <cstdint>

using bagel::Entity;
using bagel::ent_type;

static b2BodyId makeBody(ent_type e, b2BodyType type, float x, float y) {
    b2BodyDef bd = b2DefaultBodyDef();
    bd.type = type;
    bd.position = {x, y};
    bd.userData = reinterpret_cast<void*>(static_cast<intptr_t>(e.id + 1));  // 0 == "no entity"
    return b2CreateBody(phys::world(), &bd);
}

static void addBox(b2BodyId body, float w, float h, bool sensor, bool contactEv, bool sensorEv) {
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = 1.0f;
    sd.material.friction = 0.0f;
    sd.material.restitution = 1.0f;
    sd.isSensor = sensor;
    sd.enableContactEvents = contactEv;
    sd.enableSensorEvents = sensorEv;
    b2Polygon box = b2MakeBox(w * 0.5f, h * 0.5f);
    b2CreatePolygonShape(body, &sd, &box);
}

static void addCircle(b2BodyId body, float radius, bool contactEv, bool sensorEv) {
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = 1.0f;
    sd.material.friction = 0.0f;
    sd.material.restitution = 1.0f;
    sd.enableContactEvents = contactEv;
    sd.enableSensorEvents = sensorEv;
    b2Circle c = {{0.0f, 0.0f}, radius};
    b2CreateCircleShape(body, &sd, &c);
}

Entity spawnPaddle(float x, float y) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_kinematicBody, x, y);
    addBox(body, Config::PADDLE_W, Config::PADDLE_H, false, true, true);  // sensorEv: catches drops
    e.addAll(Position{x, y}, Size{Config::PADDLE_W, Config::PADDLE_H},
             Drawable{0.85f, 0.85f, 0.90f, 1.0f, Shape::Rect}, PhysicsBody{body}, PaddleTag{});
    return e;
}

Entity spawnBall(float x, float y) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_dynamicBody, x, y);
    addCircle(body, Config::BALL_RADIUS, true, false);
    float d = Config::BALL_RADIUS * 2.0f;
    e.addAll(Position{x, y}, Size{d, d},
             Drawable{1.0f, 0.95f, 0.40f, 1.0f, Shape::Circle}, PhysicsBody{body}, BallTag{});
    return e;
}

Entity spawnWall(float x, float y, float w, float h) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_staticBody, x, y);
    addBox(body, w, h, false, false, false);
    e.addAll(Position{x, y}, Size{w, h},
             Drawable{0.40f, 0.40f, 0.50f, 1.0f, Shape::Rect}, PhysicsBody{body}, WallTag{});
    return e;
}

Entity spawnBrick(int courseId, float x, float y) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_staticBody, x, y);
    addBox(body, Config::BRICK_W, Config::BRICK_H, false, true, false);  // contactEv: ball-brick reported
    float r, g, b; Config::courseColor(courseId, r, g, b);
    e.addAll(Position{x, y}, Size{Config::BRICK_W, Config::BRICK_H},
             Drawable{r, g, b, 1.0f, Shape::Rect}, PhysicsBody{body}, BrickInfo{courseId}, BrickTag{});
    return e;
}

Entity spawnDrop(int courseId, float x, float y, DropType type) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_kinematicBody, x, y);
    addBox(body, Config::DROP_SIZE, Config::DROP_SIZE, true, false, true);  // sensor
    b2Body_SetLinearVelocity(body, {0.0f, Config::DROP_FALL});
    float r, g, b; Config::courseColor(courseId, r, g, b);
    e.addAll(Position{x, y}, Size{Config::DROP_SIZE, Config::DROP_SIZE},
             Drawable{r, g, b, 0.90f, Shape::Rect}, PhysicsBody{body}, DropInfo{courseId, type}, DropTag{});
    return e;
}

Entity spawnProjectile(int courseId, float x, float y, float vx, float vy) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_kinematicBody, x, y);
    addCircle(body, 0.12f, true, false);
    b2Body_SetLinearVelocity(body, {vx, vy});
    e.addAll(Position{x, y}, Size{0.24f, 0.24f},
             Drawable{1.0f, 0.50f, 0.20f, 1.0f, Shape::Circle}, PhysicsBody{body}, ProjInfo{courseId}, ProjectileTag{});
    return e;
}

Entity spawnHazard(int courseId, float x, float y, HazardType type) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_staticBody, x, y);
    addBox(body, 0.5f, 0.5f, false, true, false);
    e.addAll(Position{x, y}, Size{0.5f, 0.5f},
             Drawable{0.70f, 0.20f, 0.70f, 1.0f, Shape::Rect}, PhysicsBody{body}, HazardInfo{courseId, type}, HazardTag{});
    return e;
}

Entity spawnCourse(int id) {
    Entity e = Entity::create();
    e.add(Course{id, CourseState::ACTIVE, 0.0f});
    return e;
}

Entity spawnGameState() {
    Entity e = Entity::create();
    e.addAll(GameState{Config::START_LIVES, 0.0f, Phase::PLAYING, -1, 0, Config::COURSES, 0.0f},
             GameStateTag{});
    return e;
}
