// AVIEL - input. Moves the paddle (kinematic), clamps it to the play field,
// and keeps stationary balls parked on it until launch.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Physics.h"
#include <SDL3/SDL.h>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

/// @brief Moves the paddle from mouse/keyboard input (kinematic), clamps it to the
///        play field, and parks stationary balls on top of the paddle until launch.
/// @param keys SDL keyboard state array
/// @param dt   Fixed timestep in seconds
/// @param r    SDL renderer (used to resolve window position for mouse coords)
/// @return void
void inputSystem(const bool* keys, float dt, SDL_Renderer* r) {
    float globalMouseX = 0.0f, globalMouseY = 0.0f;
    SDL_GetGlobalMouseState(&globalMouseX, &globalMouseY);
    (void)globalMouseY;

    int windowX = 0, windowY = 0;
    SDL_Window* window = SDL_GetRenderWindow(r);
    if (window) SDL_GetWindowPosition(window, &windowX, &windowY);
    const float mouseWorldX = (globalMouseX - (float)windowX) / Config::PPM;
    static float prevMouseWorldX = mouseWorldX;
    static bool hasPrevMouse = false;
    float mouseVx = 0.0f;
    if (hasPrevMouse && dt > 0.0f)
        mouseVx = (mouseWorldX - prevMouseWorldX) / dt;
    prevMouseWorldX = mouseWorldX;
    hasPrevMouse = true;

    static const Mask paddleMask = MaskBuilder()
        .set<PaddleTag>()
        .set<Position>()
        .set<Size>()
        .set<PhysicsBody>()
        .build();
    static int paddleQuery = World::createQuery(paddleMask);

    for (Entity e = World::first(paddleQuery); !World::eof(paddleQuery); e = World::next(paddleQuery)) {
        const auto& p = e.get<Position>();
        float half = e.get<Size>().w * 0.5f;
        float targetX = mouseWorldX;
        if (targetX - half < Config::WALL) targetX = Config::WALL + half;
        if (targetX + half > Config::WORLD_W - Config::WALL) targetX = Config::WORLD_W - Config::WALL - half;

        float vx = mouseVx;

        if (keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A]) vx -= Config::PADDLE_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) vx += Config::PADDLE_SPEED;

        const float maxSpeed = Config::PADDLE_SPEED * 1.5f;
        if (vx < -maxSpeed) vx = -maxSpeed;
        if (vx >  maxSpeed) vx =  maxSpeed;

        phys::setPosition(e.entity(), targetX, p.y);
        e.get<Position>() = {targetX, p.y};
        phys::setVelocity(e.entity(), vx, 0.0f);

        if (e.has<PaddleImpact>()) {
            auto& impact = e.get<PaddleImpact>();
            impact.time -= dt;
            if (impact.time <= 0.0f) e.del<PaddleImpact>();
        }
    }

    static const Mask ballMask = MaskBuilder()
        .set<BallTag>()
        .set<Position>()
        .set<PhysicsBody>()
        .build();
    static int ballQuery = World::createQuery(ballMask);

    Entity paddle = World::first(paddleQuery);
    if (World::eof(paddleQuery)) return;

    const auto& paddlePos = paddle.get<Position>();
    const float parkedY = Config::paddleY() - Config::PADDLE_H * 0.5f - Config::BALL_RADIUS - 0.04f;

    for (Entity ball = World::first(ballQuery); !World::eof(ballQuery); ball = World::next(ballQuery)) {
        float vx = 0.0f, vy = 0.0f;
        phys::getVelocity(ball.entity(), vx, vy);
        if (vx * vx + vy * vy >= 1.0f) continue;

        phys::setPosition(ball.entity(), paddlePos.x, parkedY);
        ball.get<Position>() = {paddlePos.x, parkedY};
    }
}
