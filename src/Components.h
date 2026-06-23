#pragma once
/// @file Components.h
/// @brief All ECS component and tag structs, plus the GameState singleton.
///
/// Three kinds of thing live here:
///   - Data components: POD structs attached to entities (Position, Course, …).
///   - GameState: one large struct on a single "singleton" entity holding all
///                cross-system status (lives, phase, timers, stage-2 state).
///   - Tags: empty structs used only for has<>() filtering (BallTag, …).
///
/// Two meanings of "course" — easy to confuse:
///   - courseId    (0..COURSES-1 = 0..2): a colored row/track of bricks.
///   - courseIndex (0..20): which of the 21 catalog courses a brick represents.
///
/// Storage tiers (Tagged / Packed / Sparse) are customized at the bottom of this file.

#include "bagel.h"
#include "Enums.h"
#include <array>
#include <SDL3/SDL.h>
#include <box2d/id.h>   // b2BodyId only (lightweight, no full Box2D dependency)

// ---- Data components ----
// Hot per-frame components (Position, PhysicsBody, Drawable, Course) get
// PackedStorage at the bottom of this file; the rest use BAGEL's default
// SparseStorage.

/// @brief Center position of an entity in world meters (Box2D coordinate space).
struct Position        { float x, y; };
/// @brief Full width and height of an entity; used for drawing and physics shape creation.
struct Size            { float w, h; };
/// @brief Render color (0..1 RGBA) and primitive shape used by the fallback renderer.
struct Drawable        { float r, g, b, a; Shape shape; };
/// @brief A crop rectangle into a spritesheet texture plus the atlas index.
struct SpritePart      { SDL_FRect part; int sheet; };
/// @brief Handle to this entity's Box2D rigid body.
struct PhysicsBody     { b2BodyId body; };

/// @brief Per-course aggregate: track ID, lifecycle state, and 0..1 completion progress.
struct Course          { int id; CourseState state; float progress; };
/// @brief Identifies which colored track (courseId 0..2) and catalog course (courseIndex 0..20) a brick belongs to.
struct BrickInfo       { int courseId; int courseIndex; };
/// @brief Brick meter state: how many hits filled, maximum needed, whether prereqs are met, and clear-delay countdown.
struct BrickProgress   { int filled; int max; bool unlocked; float clearDelay; };
/// @brief Bitmask of catalog-course indices that must be cleared before this brick unlocks.
struct BrickPrereqMask { uint32_t mustClear; };
/// @brief Data carried by a falling drop: its course, catalog index, type, the brick it came from, and a grade value.
struct DropInfo        { int courseId; int courseIndex; DropType type;
                        bagel::ent_type sourceBrick; float gradeValue; };
/// @brief Identifies which course fired an exam projectile, used for hit attribution.
struct ProjInfo        { int courseId; };
/// @brief Remaining seconds of the visual bounce animation after the ball hits the paddle.
struct PaddleImpact    { float time; };

/// @brief Singleton component (one entity only) holding all cross-system runtime state:
///        lives, average grade, game phase, academic year/timer, exam state, graduation
///        progress, input cache, and cheat flags.
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
/// @brief Marks the paddle entity.
struct PaddleTag    {};
/// @brief Marks the ball entity.
struct BallTag      {};
/// @brief Marks a course brick entity.
struct BrickTag     {};
/// @brief Marks a boundary wall entity.
struct WallTag      {};
/// @brief Marks a falling drop (assignment, bonus, or tax) entity.
struct DropTag      {};
/// @brief Marks an exam projectile entity.
struct ProjectileTag{};
/// @brief Marks a graduation ceremony chair entity.
struct GradChairTag {};
/// @brief Marks the graduation student entity (repurposed paddle in stage 2).
struct GradStudentTag {};
/// @brief Marks a sliding graduation obstacle entity.
struct GradObstacleTag {};
/// @brief Marks the singleton GameState entity.
struct GameStateTag {};
/// @brief Marks any entity for deletion by deadCleanupSystem at end of frame.
struct DeadTag      {};

/// @brief Per-chair data: grid index and whether the chair is hidden during a vault animation.
struct GradChairInfo { int index; bool hidden; };
/// @brief Per-obstacle data: which row-gap it occupies, current travel direction, and foul-contact flag.
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
