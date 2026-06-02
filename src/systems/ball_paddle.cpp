// AVIEL - ball/paddle behavior. Consumes CourseHit, removes hit bricks, and
// handles ball loss without depending on another vertical being complete.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Events.h"
#include "Physics.h"
#include <cmath>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static void resetBall(Entity ball) {
    static const Mask paddleMask = MaskBuilder()
        .set<PaddleTag>()
        .set<Position>()
        .build();
    static int paddleQuery = World::createQuery(paddleMask);

    Entity paddle = World::first(paddleQuery);
    float x = Config::WORLD_W * 0.5f;
    if (!World::eof(paddleQuery)) x = paddle.get<Position>().x;

    const float y = Config::PADDLE_Y - Config::PADDLE_H * 0.5f - Config::BALL_RADIUS - 0.05f;
    phys::setVelocity(ball.entity(), 0.0f, 0.0f);
    phys::setPosition(ball.entity(), x, y);
    ball.get<Position>() = {x, y};
}

static void ballLossSystem() {
    static const Mask mask = MaskBuilder()
        .set<BallTag>()
        .set<Position>()
        .set<PhysicsBody>()
        .build();
    static int q = World::createQuery(mask);

    const float lossY = Config::WORLD_H - Config::WALL - Config::BALL_RADIUS * 1.5f;
    for (Entity ball = World::first(q); !World::eof(q); ball = World::next(q)) {
        if (ball.get<Position>().y >= lossY) {
            ev::lifeLost(1);
            resetBall(ball);
        }
    }
}

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void paddleHitSystem() {
    static const Mask mask = MaskBuilder().set<PaddleHit>().build();
    static int q = World::createQuery(mask);

    constexpr float MAX_BOUNCE_X = 0.82f;
    constexpr float PADDLE_SPIN = 0.025f;
    constexpr float SAME_DIRECTION_BOOST = 0.045f;
    constexpr float OPPOSING_DIRECTION_DRAG = 0.015f;
    constexpr float CENTER_SLOWDOWN = 0.04f;
    constexpr float IMPACT_TIME = 0.10f;

    for (Entity ev = World::first(q); !World::eof(q); ev = World::next(q)) {
        const auto hitEvent = ev.get<PaddleHit>();
        Entity ball{hitEvent.ball};
        Entity paddle{hitEvent.paddle};
        if (!ball.has<BallTag>() || !ball.has<Position>()) continue;
        if (!paddle.has<PaddleTag>() || !paddle.has<Position>() || !paddle.has<Size>()) continue;

        float paddleVx = 0.0f, paddleVy = 0.0f;
        phys::getVelocity(paddle.entity(), paddleVx, paddleVy);
        (void)paddleVy;
        float ballVx = 0.0f, ballVy = 0.0f;
        phys::getVelocity(ball.entity(), ballVx, ballVy);

        const auto& ballPos = ball.get<Position>();
        const auto& paddlePos = paddle.get<Position>();
        const float halfPaddle = paddle.get<Size>().w * 0.5f;
        if (halfPaddle <= 0.0f) continue;

        const float hitOffset = clampf((ballPos.x - paddlePos.x) / halfPaddle, -1.0f, 1.0f);
        const float dirX = clampf(hitOffset * MAX_BOUNCE_X + paddleVx * PADDLE_SPIN,
                                  -MAX_BOUNCE_X, MAX_BOUNCE_X);
        const float dirY = -1.0f;
        const float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len <= 0.0f) continue;

        float speed = std::sqrt(ballVx * ballVx + ballVy * ballVy);
        if (speed < Config::BALL_SPEED * 0.5f) speed = Config::BALL_SPEED;

        const float horizontalMomentum = ballVx * paddleVx;
        if (horizontalMomentum > 0.0f) {
            speed += std::fabs(paddleVx) * SAME_DIRECTION_BOOST;
        } else if (horizontalMomentum < 0.0f) {
            speed -= std::fabs(paddleVx) * OPPOSING_DIRECTION_DRAG;
        }

        const float centerAmount = 1.0f - clampf(std::fabs(hitOffset) * 2.0f, 0.0f, 1.0f);
        speed *= 1.0f - CENTER_SLOWDOWN * centerAmount;

        phys::setVelocity(ball.entity(), dirX / len * speed, dirY / len * speed);
        if (paddle.has<PaddleImpact>())
            paddle.get<PaddleImpact>().time = IMPACT_TIME;
        else
            paddle.add(PaddleImpact{IMPACT_TIME});
    }
}

void courseHitSystem() {
    static const Mask mask = MaskBuilder().set<CourseHit>().build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        auto brick = e.get<CourseHit>().brick;
        if (brick.id < 0) continue;               // synthetic/debug hit, no brick
        Entity b(brick);
        if (b.has<BrickTag>() && !b.has<DeadTag>())
            b.add(DeadTag{});
    }

    paddleHitSystem();
    ballLossSystem();
}
