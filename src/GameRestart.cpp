/// @file GameRestart.cpp
/// @brief Scene reset and Play-Again restart logic.
///        Provides destroyAllEntities (wipe the world), setupFreshPlayScene (spawn Stage 1),
///        and playAgainRestart (full teardown + respawn).
#include "Game.h"
#include "EntityFactory.h"
#include "Physics.h"
#include "Config.h"

using bagel::Entity;
using bagel::ent_type;

/// @brief Iterates every live entity in the world and destroys it.
void destroyAllEntities() {
    bagel::Bag<ent_type, 256> all;
    for (Entity e = Entity::first(); !e.eof(); e.next())
        if (e.mask().ctz() >= 0) all.push(e.entity());
    for (int i = 0; i < all.size(); ++i)
        Entity(all[i]).destroy();
}

/// @brief Spawns the four boundary walls, three Course aggregates, the 7×3 brick grid,
///        the paddle, and the ball for a fresh Stage-1 play session.
static void spawnStageOneScene() {
    const float W = Config::WORLD_W, H = Config::WORLD_H, t = Config::WALL;
    spawnWall(W * 0.5f, t * 0.5f,     W, t);
    spawnWall(W * 0.5f, H - t * 0.5f, W, t);
    spawnWall(t * 0.5f, H * 0.5f,     t, H);
    spawnWall(W - t * 0.5f, H * 0.5f, t, H);

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
    spawnBall(W * 0.5f, paddleY + Config::PADDLE_H * 0.5f - Config::PADDLE_VISUAL_H
        - Config::BALL_RADIUS + 0.3f);
}

/// @brief Binds a new GameState singleton and spawns a complete Stage-1 scene.
void setupFreshPlayScene() {
    bindGameState(spawnGameState().entity());
    spawnStageOneScene();
}

/// @brief Full Play-Again restart: destroys every entity, reinitializes the Box2D world,
///        and spawns a fresh Stage-1 scene.
void playAgainRestart() {
    destroyAllEntities();
    phys::shutdown();
    phys::init();
    setupFreshPlayScene();
}
