// exam.cpp — the exam phase.
// An ExamStarted event puts the game into Phase::EXAM: a timer runs while projectiles
// are fired at the paddle from three columns; the player dodges. When the timer ends the
// grade is computed from how many hits were taken (100 - 12*hits, floored at 55) and an
// ExamFinished event is emitted, returning play to Phase::PLAYING.

#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Events.h"
#include "EntityFactory.h"
#include "Game.h"
#include <cmath>

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

/// @brief Returns the X world-coordinate for one of three spawn columns (left/center/right).
/// @param slot Slot index; wraps with modulo 3
/// @return X position in world units
static inline float spawnSlotX(int slot) {
    constexpr float slots[3] = {0.22f, 0.50f, 0.78f};
    return Config::WORLD_W * slots[slot % 3];
}

/// @brief Returns the current X center of the paddle entity, or the screen midpoint if none exists.
/// @return X position in world units
static inline float paddleCenterX() {
    static const Mask mask = MaskBuilder().set<PaddleTag>().set<Position>().build();
    static int q = World::createQuery(mask);
    Entity p = World::first(q);
    if (World::eof(q)) return Config::WORLD_W * 0.5f;
    return p.get<Position>().x;
}

/// @brief Spawns one exam projectile from the given column slot, aimed at the paddle.
/// @param courseId Course whose exam is active (stored in ProjInfo for hit attribution)
/// @param slot Spawn column index (0=left, 1=center, 2=right)
/// @return void
static void spawnExamProjectile(int courseId, int slot) {
    const float sx = spawnSlotX(slot);
    const float sy = Config::WALL + 0.4f;
    const float tx = paddleCenterX();
    const float ty = Config::paddleY();
    const float dx = tx - sx;
    const float dy = ty - sy;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01f) return;
    const float speed = Config::EXAM_PROJ_SPEED;
    spawnProjectile(courseId, sx, sy, dx / len * speed, dy / len * speed);
}

/// @brief Tags all live projectile entities as Dead so they are removed at end of frame.
/// @return void
static void clearExamProjectiles() {
    static const Mask mask = MaskBuilder().set<ProjectileTag>().build();
    static int q = World::createQuery(mask);
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        if (!e.has<DeadTag>()) e.add(DeadTag{});
}

/// @brief Transitions GameState into Phase::EXAM and resets all exam counters/timers.
/// @param courseId Course that triggered the exam
/// @return void
static void startExam(int courseId) {
    GameState& gs = gameState();
    gs.phase             = Phase::EXAM;
    gs.activeExamCourse  = courseId;
    gs.examTimer         = 0.0f;
    gs.examSpawnTimer    = 0.0f;   // spawn first projectile immediately
    gs.examHits          = 0;
    gs.examProjSlot      = 0;
}

/// @brief Calculates the exam grade, emits ExamFinished, clears projectiles,
///        and returns GameState to Phase::PLAYING.
/// @return void
static void finishExam() {
    GameState& gs = gameState();
    const float grade = std::max(
        Config::EXAM_MIN_GRADE,
        Config::EXAM_BASE_GRADE - static_cast<float>(gs.examHits) * Config::EXAM_HIT_PENALTY);
    const bool passed = grade >= Config::PASS;
    ev::examFinished(gs.activeExamCourse, passed, grade);

    clearExamProjectiles();
    gs.phase            = Phase::PLAYING;
    gs.activeExamCourse = -1;
}

/// @brief Manages the exam phase: sets Phase::EXAM on ExamStarted, runs a timer,
///        spawns projectiles to dodge, and emits ExamFinished with a grade on completion.
///        Grade = 100 − (hits × 12), floored at 55. Passing requires ≤ 3 hits.
/// @param dt Fixed timestep in seconds
/// @return void
void examSystem(float dt) {
    GameState& gs = gameState();

    // Consume ExamStarted — only when not already in an exam
    if (gs.phase != Phase::EXAM) {
        static const Mask startMask = MaskBuilder().set<ExamStarted>().build();
        static int startQ = World::createQuery(startMask);
        Entity first = World::first(startQ);
        if (!World::eof(startQ))
            startExam(first.get<ExamStarted>().courseId);
        return;
    }

    // Running exam — advance timers
    gs.examTimer      += dt;
    gs.examSpawnTimer += dt;

    // Spawn a projectile when interval elapses
    if (gs.examSpawnTimer >= Config::EXAM_PROJ_INTERVAL) {
        gs.examSpawnTimer -= Config::EXAM_PROJ_INTERVAL;
        spawnExamProjectile(gs.activeExamCourse, gs.examProjSlot);
        ++gs.examProjSlot;
    }

    // Count projectile hits this frame
    {
        static const Mask hitMask = MaskBuilder().set<ProjectileHit>().build();
        static int hitQ = World::createQuery(hitMask);
        for (Entity e = World::first(hitQ); !World::eof(hitQ); e = World::next(hitQ))
            ++gs.examHits;
    }

    // Exam over when timer expires
    if (gs.examTimer >= Config::EXAM_DURATION)
        finishExam();
}
