#pragma once
#include "bagel.h"
#include "Enums.h"
#include <array>
#include <SDL3/SDL.h>
#include <box2d/id.h>   // b2BodyId only (lightweight, no full Box2D dependency)

// ---- Data components ----
// Hot per-frame components (Position, PhysicsBody, Drawable, Course) get
// PackedStorage at the bottom of this file; the rest use BAGEL's default
// SparseStorage.
struct Position        { float x, y; };                                            // center point in world meters
struct Size            { float w, h; };                                            // full width & height, used for drawing and physics
struct Drawable        { float r, g, b, a; Shape shape; };                         // render color (0..1) + which shape to draw
struct SpritePart      { SDL_FRect part; int sheet; };                             // spritesheet crop + texture index (breakout style)
struct PhysicsBody     { b2BodyId body; };                                         // handle to this entity's Box2D body

struct Course          { int id; CourseState state; float progress; };             // a course: its number, current state, progress 0..1
struct BrickInfo       { int courseId; int courseIndex; };                         // row track + catalog index (0..20)
struct BrickProgress   { int filled; int max; bool unlocked; float clearDelay; };  // meter + prereq; clearDelay > 0 = full meter pause before destroy
struct BrickPrereqMask { uint32_t mustClear; };                                    // bit i set => course index i must be cleared first
struct DropInfo        { int courseId; int courseIndex; DropType type;
                        bagel::ent_type sourceBrick; float gradeValue; };
struct ProjInfo        { int courseId; };                                          // exam projectile -> course that fired it
struct PaddleImpact    { float time; };                                            // remaining visual bounce time after ball contact

// on the singleton entity only
struct GameState {
    int   lives;
    float average;
    Phase phase;
    int   activeExamCourse; // -1 when not in exam
    int   coursesDone;
    int   coursesTotal;
    static constexpr int COURSE_GRADES = 21;
    static constexpr int8_t TAX_PENDING = -1;
    static constexpr int8_t TAX_CAUGHT  = 0;
    static constexpr int8_t TAX_MISSED  = 1;
    std::array<int8_t, COURSE_GRADES> taxOutcome; // per-course tax result
    int   currentYear;      // 1..YEAR_COUNT
    float yearTimer;        // seconds elapsed in current year
    float yearAnnounceTimer; // >0: freeze gameplay and show year transition overlay
    float totalTime;        // total elapsed seconds (score: lower is better)
    bool  yearsExhausted;   // true when year 5 ended without clearing everything
    bool  started;          // false until first click launch; timers wait for this
    // Exam phase state (valid only while phase == EXAM)
    float examTimer;        // elapsed seconds in the current exam
    float examSpawnTimer;   // seconds since the last projectile was spawned
    int   examHits;         // projectile hits taken this exam
    int   examProjSlot;     // cycles 0→1→2 to distribute projectile spawn positions
    // Input state
    float prevMouseWorldX;  // previous frame mouse X for paddle velocity calculation
    bool  hasPrevMouse;     // false until the first mouse sample is taken
    bool  slowBallCheat;    // toggled by S; physics clamps balls to a slower speed band
    // Graduation stage (stage 2)
    bool  gradInitialized;
    int   gradNextChair;
    int   gradChairTotal;
    int   gradAnimStep;     // 0=idle, 1=vault, 2=landed
    float gradAnimTimer;
    float gradStudentX;     // horizontal position driven by mouse
    float gradJumpStartX;   // vault animation origin X
    float gradJumpStartY;   // vault animation origin Y
    int   gradActiveChair;  // chair index being vaulted (-1 when idle)
    int   gradFouls;
    bool  gradBeingDragged;
    bool  gradAwaitingSpace;
    bool  paused = false;   // true while gameplay is frozen (pause button / ESC / P)
    LoseReason loseReason = LoseReason::None;
};

// ---- Tags (empty; checked via has<>(), never get<>()) ----
struct PaddleTag    {};
struct BallTag      {};
struct BrickTag     {};
struct WallTag      {};
struct DropTag      {};
struct ProjectileTag{};
struct GradChairTag {};
struct GradStudentTag {};
struct GradObstacleTag {};
struct GameStateTag {};
struct DeadTag      {};   // marks entity for deletion by deadCleanupSystem

struct GradChairInfo { int index; bool hidden; };
struct GradObstacleInfo { int rowGap; float dir; bool contactFouled; };

// ---- Storage customization ----
// Tags: zero-storage (presence-only, checked via has<>).
// Hot components: PackedStorage so iteration is contiguous (every-frame queries
// in physicsStepSystem / renderSystem / hudSystem / courseProgressSystem).
// Everything else falls back to BAGEL's default SparseStorage.
namespace bagel {
    template<> struct Storage<PaddleTag>     final : NoInstance { using type = TaggedStorage<PaddleTag>; };
    template<> struct Storage<BallTag>       final : NoInstance { using type = TaggedStorage<BallTag>; };
    template<> struct Storage<BrickTag>      final : NoInstance { using type = TaggedStorage<BrickTag>; };
    template<> struct Storage<WallTag>       final : NoInstance { using type = TaggedStorage<WallTag>; };
    template<> struct Storage<DropTag>       final : NoInstance { using type = TaggedStorage<DropTag>; };
    template<> struct Storage<ProjectileTag> final : NoInstance { using type = TaggedStorage<ProjectileTag>; };
    template<> struct Storage<GradChairTag>  final : NoInstance { using type = TaggedStorage<GradChairTag>; };
    template<> struct Storage<GradStudentTag> final : NoInstance { using type = TaggedStorage<GradStudentTag>; };
    template<> struct Storage<GradObstacleTag> final : NoInstance { using type = TaggedStorage<GradObstacleTag>; };
    template<> struct Storage<GameStateTag>  final : NoInstance { using type = TaggedStorage<GameStateTag>; };
    template<> struct Storage<DeadTag>       final : NoInstance { using type = TaggedStorage<DeadTag>; };

    template<> struct Storage<Position>      final : NoInstance { using type = PackedStorage<Position>; };
    template<> struct Storage<PhysicsBody>   final : NoInstance { using type = PackedStorage<PhysicsBody>; };
    template<> struct Storage<Drawable>      final : NoInstance { using type = PackedStorage<Drawable>; };
    template<> struct Storage<SpritePart>      final : NoInstance { using type = PackedStorage<SpritePart>; };
    template<> struct Storage<BrickProgress>   final : NoInstance { using type = PackedStorage<BrickProgress>; };
    template<> struct Storage<Course>          final : NoInstance { using type = PackedStorage<Course>; };
}
