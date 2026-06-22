#include "EntityFactory.h"
#include "Components.h"
#include "Config.h"
#include "Physics.h"
#include "Sprites.h"

#include <box2d/box2d.h>

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

static void addCircle(b2BodyId body, float radius, bool sensor, bool contactEv, bool sensorEv) {
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = 1.0f;
    sd.material.friction = 0.0f;
    sd.material.restitution = 1.0f;
    sd.isSensor = sensor;
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
             Drawable{0.85f, 0.85f, 0.90f, 1.0f, Shape::Rect},
             sprites::makePart(sprites::Id::PADDLE_MTA), PhysicsBody{body}, PaddleTag{});
    return e;
}

Entity spawnBall(float x, float y) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_dynamicBody, x, y);
    addCircle(body, Config::BALL_RADIUS, false, true, false);
    float d = Config::BALL_RADIUS * 2.0f;
    e.addAll(Position{x, y}, Size{d, d},
             Drawable{1.0f, 0.95f, 0.40f, 1.0f, Shape::Circle},
             sprites::makePart(sprites::Id::BALL_DEFAULT), PhysicsBody{body}, BallTag{});
    return e;
}

Entity spawnWall(float x, float y, float w, float h) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_staticBody, x, y);
    addBox(body, w, h, false, false, false);
    e.addAll(Position{x, y}, Size{w, h}, PhysicsBody{body}, WallTag{});
    return e;
}

Entity spawnBrick(int courseId, int spriteIndex, float x, float y) {
    const int ci = sprites::courseIndexFromSprite(spriteIndex);
    const int meterMax = sprites::courseMeterMax(ci);
    const bool unlocked = sprites::courseStartsUnlocked(ci);
    const bool showLocked = sprites::courseShowsLockedSprite(ci, unlocked);

    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_staticBody, x, y);
    addBox(body, Config::BRICK_W, Config::BRICK_H, false, true, false);  // contactEv: ball-brick reported
    float r, g, b; Config::courseColor(courseId, r, g, b);
    e.addAll(Position{x, y}, Size{Config::BRICK_W, Config::BRICK_H},
             Drawable{r, g, b, 1.0f, Shape::Rect},
             sprites::makePart(sprites::courseSpriteId(ci, showLocked)),
             BrickProgress{0, meterMax, unlocked, 0.0f},
             BrickPrereqMask{sprites::coursePrereqMask(ci)},
             PhysicsBody{body}, BrickInfo{courseId, ci}, BrickTag{});
    return e;
}

Entity spawnDrop(int courseId, int courseIndex, float x, float y, DropType type,
                 ent_type sourceBrick, float gradeValue) {
    Entity e = Entity::create();
    const float size = type == DropType::Tax ? Config::TAX_DROP_SIZE : Config::DROP_SIZE;
    b2BodyId body = makeBody(e.entity(), b2_kinematicBody, x, y);
    addBox(body, size, size, true, false, true);  // sensor
    b2Body_SetLinearVelocity(body, {0.0f, Config::DROP_FALL});

    if (type == DropType::Tax) {
        e.addAll(Position{x, y}, Size{size, size},
                 Drawable{1.0f, 0.82f, 0.05f, 1.0f, Shape::Rect},
                 PhysicsBody{body},
                 DropInfo{courseId, courseIndex, type, sourceBrick, gradeValue},
                 DropTag{});
        return e;
    }

    float r, g, b; Config::courseColor(courseId, r, g, b);
    e.addAll(Position{x, y}, Size{size, size},
             Drawable{r, g, b, 0.90f, Shape::Rect},
             sprites::makePart(sprites::Id::ASSIGNMENT_GREEN),
             PhysicsBody{body},
             DropInfo{courseId, courseIndex, type, sourceBrick, gradeValue},
             DropTag{});
    return e;
}

Entity spawnProjectile(int courseId, float x, float y, float vx, float vy) {
    Entity e = Entity::create();
    b2BodyId body = makeBody(e.entity(), b2_kinematicBody, x, y);
    addCircle(body, 0.12f, true, false, true);
    b2Body_SetLinearVelocity(body, {vx, vy});
    e.addAll(Position{x, y}, Size{0.24f, 0.24f},
             Drawable{1.0f, 0.50f, 0.20f, 1.0f, Shape::Circle}, PhysicsBody{body}, ProjInfo{courseId}, ProjectileTag{});
    return e;
}

Entity spawnGradChair(int index, float x, float y) {
    Entity e = Entity::create();
    e.addAll(Position{x, y}, Size{Config::GRAD_CHAIR_W, Config::GRAD_CHAIR_H},
             Drawable{1.0f, 1.0f, 1.0f, 1.0f, Shape::Rect},
             sprites::makePart(sprites::Id::GRAD_CHAIR),
             GradChairInfo{index, false}, GradChairTag{});
    return e;
}

Entity spawnGradObstacle(float x, int rowGap, float dir) {
    Entity e = Entity::create();
    e.addAll(Position{x, Config::graduationObstacleY(rowGap)},
             Size{Config::GRAD_OBSTACLE_W, Config::GRAD_OBSTACLE_H},
             Drawable{0.88f, 0.28f, 0.22f, 1.0f, Shape::Rect},
             GradObstacleInfo{rowGap, dir, false},
             GradObstacleTag{});
    return e;
}

Entity spawnCourse(int id) {
    Entity e = Entity::create();
    e.add(Course{id, CourseState::ACTIVE, 0.0f});
    return e;
}

Entity spawnGameState() {
    Entity e = Entity::create();
    GameState gs{
        Config::START_LIVES, Config::START_AVERAGE, Phase::MENU, -1, 0, Config::COURSES,
        {}, 1, 0.0f, 0.0f, 0.0f, false, false,
        0.0f, 0.0f, 0, 0, 0.0f, false, false,
        false, 0, 0, 0, 0.0f, Config::WORLD_W * 0.5f, Config::WORLD_W * 0.5f,
        Config::graduationStudentStartY(), -1,
        0, false, false};
    gs.taxOutcome.fill(GameState::TAX_PENDING);
    e.addAll(gs, GameStateTag{});
    return e;
}
