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
#include <vector>
#include <cstdio>
#include <cstring>

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static ent_type g_gs{-1};

bagel::ent_type gameStateEntity() { return g_gs; }
GameState& gameState() { return Entity(g_gs).get<GameState>(); }

bagel::ent_type courseEntity(int id) {
    static const Mask mask = MaskBuilder().set<Course>().build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.get<Course>().id == id) return e.entity();
    }
    return {-1};
}

void eventCleanupSystem() {
    static const Mask mask = MaskBuilder().set<EventTag>().build();
    static int q = World::createQuery(mask);

    std::vector<ent_type> dead;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        dead.push_back(e.entity());
    for (ent_type id : dead) Entity(id).destroy();
}

void deadCleanupSystem() {
    static const Mask mask = MaskBuilder().set<DeadTag>().build();
    static int q = World::createQuery(mask);

    std::vector<ent_type> dead;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        dead.push_back(e.entity());
    for (ent_type id : dead) {
        Entity e(id);
        if (e.has<PhysicsBody>()) b2DestroyBody(e.get<PhysicsBody>().body);
        e.destroy();
    }
}

static void clearAll() {
    std::vector<ent_type> all;
    for (Entity e = Entity::first(); !e.eof(); e.next())
        if (e.mask().ctz() >= 0) all.push_back(e.entity());
    for (ent_type id : all) {
        Entity e(id);
        if (e.has<PhysicsBody>()) b2DestroyBody(e.get<PhysicsBody>().body);
        e.destroy();
    }
}

void SurviveGame::setupScene() {
    g_gs = spawnGameState().entity();

    const float W = Config::WORLD_W, H = Config::WORLD_H, t = Config::WALL;
    spawnWall(W * 0.5f, t * 0.5f,     W, t);   // top
    spawnWall(W * 0.5f, H - t * 0.5f, W, t);   // bottom (closed for the core smoke test)
    spawnWall(t * 0.5f, H * 0.5f,     t, H);   // left
    spawnWall(W - t * 0.5f, H * 0.5f, t, H);   // right

    for (int c = 0; c < Config::COURSES; ++c) spawnCourse(c);

    const float bw = Config::BRICK_W, bh = Config::BRICK_H, gap = Config::BRICK_GAP;
    const float gridW = Config::brickGridWidth();
    const float startX = (W - gridW) * 0.5f + bw * 0.5f;
    const float startY = 2.4f;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col) {
            float x = startX + col * (bw + gap);
            float y = startY + row * (bh + gap * 0.5f);
            const int courseIndex = Config::gridCourseIndex(row, col);
            spawnBrick(row, courseIndex, x, y);
        }

    const float paddleY = Config::paddleY();
    spawnPaddle(W * 0.5f, paddleY);
    spawnBall(W * 0.5f, paddleY - Config::PADDLE_H * 0.5f - Config::BALL_RADIUS - 0.04f);
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
            if (gs.phase == Phase::LOST || gs.phase == Phase::WON) {
                clearAll();
                setupScene();
            }
            launchBallAndStart();
            break;
        }
        case SDL_SCANCODE_R: clearAll(); setupScene(); break;
        default: break;
    }
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
    } else {
        eventCleanupSystem();   // drain stray (e.g. debug) events while paused
    }
}

void SurviveGame::debugOverlay() {
    int ents = 0;
    for (Entity e = Entity::first(); !e.eof(); e.next())
        if (e.mask().ctz() >= 0) ++ents;
    int evs = 0;
    {
        static const Mask mask = MaskBuilder().set<EventTag>().build();
        static int q = World::createQuery(mask);
        for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) ++evs;
    }

    SDL_SetRenderDrawColorFloat(_renderer, 0.6f, 0.9f, 0.6f, 1.0f);
    char buf[128];
    std::snprintf(buf, sizeof buf, "entities:%d  events:%d", ents, evs);
    SDL_RenderDebugText(_renderer, 8.0f, Config::WINDOW_H - 28.0f, buf);
    SDL_RenderDebugText(_renderer, 8.0f, Config::WINDOW_H - 16.0f,
                        "A/D move  Space launch  H/J/E/K/L debug events  R restart  Esc quit");

    char row2[64] = "row2:";
    int finals = 0;
    {
        static const Mask mask = MaskBuilder()
            .set<BrickTag>()
            .set<BrickInfo>()
            .set<Position>()
            .build();
        static int q = World::createQuery(mask);
        for (Entity b = World::first(q); !World::eof(q); b = World::next(q)) {
            if (b.has<DeadTag>()) continue;
            const int ci = b.get<BrickInfo>().courseIndex;
            if (ci == sprites::FINAL_COURSE_INDEX) ++finals;
            if (b.get<Position>().y < 4.5f) continue;
            char tmp[8];
            std::snprintf(tmp, sizeof tmp, " %d", ci);
            if (std::strlen(row2) + std::strlen(tmp) + 1 < sizeof row2)
                std::strcat(row2, tmp);
        }
    }
    char fin[32];
    std::snprintf(fin, sizeof fin, "  finals:%d", finals);
    std::strncat(row2, fin, sizeof row2 - std::strlen(row2) - 1);
    SDL_RenderDebugText(_renderer, 8.0f, Config::WINDOW_H - 44.0f, row2);
}

void SurviveGame::draw() {
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
    SDL_RenderClear(_renderer);
    renderSystem(_renderer);
    hudSystem(_renderer);
    debugOverlay();
    SDL_RenderPresent(_renderer);
}
