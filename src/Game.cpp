#include "Game.h"
#include "Components.h"
#include "Events.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Physics.h"
#include "Sprites.h"
#include "systems/systems.h"

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <cstdio>
#include <cstring>

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

bagel::ent_type courseEntity(int id) {
    static const Mask mask = MaskBuilder().set<Course>().build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.get<Course>().id == id) return e.entity();
    }
    return {-1};
}

/// @brief Destroys all event entities (EventTag) at end of frame. One-frame lifetime.
/// @return void
void eventCleanupSystem() {
    static const Mask mask = MaskBuilder().set<EventTag>().build();
    static int q = World::createQuery(mask);

    bagel::Bag<ent_type, 64> dead;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        dead.push(e.entity());
    for (int i = 0; i < dead.size(); ++i) Entity(dead[i]).destroy();
}

static void clearAll() {
    bagel::Bag<ent_type, 256> all;
    for (Entity e = Entity::first(); !e.eof(); e.next())
        if (e.mask().ctz() >= 0) all.push(e.entity());
    for (int i = 0; i < all.size(); ++i) {
        phys::destroyBody(all[i]);
        Entity(all[i]).destroy();
    }
}

static void setupGraduationPreview() {
    const float W = Config::WORLD_W, H = Config::WORLD_H, t = Config::WALL;
    spawnWall(W * 0.5f, t * 0.5f,     W, t);
    spawnWall(W * 0.5f, H - t * 0.5f, W, t);
    spawnWall(t * 0.5f, H * 0.5f,     t, H);
    spawnWall(W - t * 0.5f, H * 0.5f, t, H);
    spawnPaddle(W * 0.5f, Config::paddleY());

    GameState& gs = gameState();
    gs.phase = Phase::GRADUATION;
    gs.started = true;
    enterGraduationStage();
}

void SurviveGame::setupScene() {
    bindGameState(spawnGameState().entity());

#ifdef DEBUG_GRADUATION_STAGE
    setupGraduationPreview();
    return;
#endif

    const float W = Config::WORLD_W, H = Config::WORLD_H, t = Config::WALL;
    spawnWall(W * 0.5f, t * 0.5f,     W, t);   // top
    spawnWall(W * 0.5f, H - t * 0.5f, W, t);   // bottom (closed for the core smoke test)
    spawnWall(t * 0.5f, H * 0.5f,     t, H);   // left
    spawnWall(W - t * 0.5f, H * 0.5f, t, H);   // right

    for (int c = 0; c < Config::COURSES; ++c) spawnCourse(c);

    const float bw = Config::BRICK_W, bh = Config::BRICK_H, gap = Config::BRICK_GAP;
    const float gridW = Config::brickGridWidth();
    const float startX = (W - gridW) * 0.5f + bw * 0.5f;
    const float startY = 3.2f;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col) {
            float x = startX + col * (bw + gap);
            float y = startY + row * (bh + gap * 0.5f);
            const int courseIndex = Config::gridCourseIndex(row, col);
            spawnBrick(row, courseIndex, x, y);
        }

    const float paddleY = Config::paddleY();
    spawnPaddle(W * 0.5f, paddleY);
    spawnBall(W * 0.5f, paddleY + Config::PADDLE_H * 0.5f - Config::PADDLE_VISUAL_H - Config::BALL_RADIUS + 0.3f);
}

bool SurviveGame::init(SDL_Renderer* renderer) {
    _renderer = renderer;
    if (!sprites::init(renderer)) return false;
    phys::init();
    setupScene();
    return true;
}

void SurviveGame::shutdown() {
    clearAll();
    phys::shutdown();
    sprites::shutdown();
}

static void launchBallAndStart() {
    GameState& gs = gameState();
    static const Mask mask = MaskBuilder().set<BallTag>().build();
    static int q = World::createQuery(mask);
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        float vx, vy; phys::getVelocity(e.entity(), vx, vy);
        if (vx * vx + vy * vy < 1.0f) {
            phys::setVelocity(e.entity(), Config::BALL_SPEED * 0.55f, -Config::BALL_SPEED * 0.83f);
            gs.started = true;
        }
    }
}

void SurviveGame::onKeyDown(int sc) {
    switch (sc) {
        // Debug-hotkey synthesizer: lets each vertical be exercised solo.
        case SDL_SCANCODE_H: ev::courseHit(0, {-1});                 break;
        case SDL_SCANCODE_J: ev::dropCaught(0, 0, DropType::Assignment, {-1}); break;
        case SDL_SCANCODE_E: ev::examStarted(0);                     break;
        case SDL_SCANCODE_K: ev::hazardTriggered(0, HazardType::LoseLife); break;
        case SDL_SCANCODE_L: ev::lifeLost(1);                        break;
        case SDL_SCANCODE_SPACE: {
            GameState& gs = gameState();
            if (gs.phase == Phase::GRADUATION && gs.gradAwaitingSpace) {
                graduationOnSpace();
                break;
            }
            if (gs.phase == Phase::LOST || gs.phase == Phase::WON) {
                clearAll();
                setupScene();
            }
            launchBallAndStart();
            break;
        }
        case SDL_SCANCODE_R:
            clearAll();
            setupScene();
            break;
        default: break;
    }
}

void SurviveGame::onMouseDown(int button) {
    if (button != SDL_BUTTON_LEFT) return;
    if (gameState().phase == Phase::GRADUATION)
        graduationOnMouseDown(_renderer);
}

void SurviveGame::tick(const bool* keys, float dt) {
    GameState& gs = gameState();
    if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM) {
        inputSystem(keys, dt, _renderer);
        physicsStepSystem(dt);
        contactEventSystem();
        brickMeterSystem();
        brickClearDelaySystem(dt);
        courseHitSystem();
        dropSystem(dt);
        brickUnlockSystem();
        courseProgressSystem();
        examSystem(dt);
        hazardSystem();
        yearSystem(dt);
        gameStateSystem();
        eventCleanupSystem();
        deadCleanupSystem();
    } else if (gs.phase == Phase::GRADUATION) {
        if (!gs.gradAwaitingSpace)
            graduationInputSystem(_renderer);
        graduationSystem(dt);
        gameStateSystem();
        yearSystem(dt);
        eventCleanupSystem();
        deadCleanupSystem();
    } else {
        eventCleanupSystem();   // drain stray (e.g. debug) events while paused
    }
}


void SurviveGame::draw() {
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
    SDL_RenderClear(_renderer);
    renderSystem(_renderer);
    hudSystem(_renderer);
    SDL_RenderPresent(_renderer);
}
