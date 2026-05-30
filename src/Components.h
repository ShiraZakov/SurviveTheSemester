#pragma once
// Component & tag schemas

#include "bagel.h"
#include "Enums.h"
#include <box2d/id.h>   // b2BodyId only (lightweight, no full Box2D dependency)

// ---- Data components ----
// Hot per-frame components (Position, PhysicsBody, Drawable, Course) get
// PackedStorage at the bottom of this file; the rest use BAGEL's default
// SparseStorage.
struct Position   { float x, y; };                                 // center point in world meters
struct Size       { float w, h; };                                 // full width & height, used for drawing and physics
struct Drawable   { float r, g, b, a; Shape shape; };              // render color (0..1) + which shape to draw
struct PhysicsBody{ b2BodyId body; };                              // handle to this entity's Box2D body

struct Course     { int id; CourseState state; float progress; };  // a course: its number, current state, progress 0..1
struct BrickInfo  { int courseId; };                               // brick -> course it belongs to
struct DropInfo   { int courseId; DropType type; };                // falling pickup: source course + kind of drop
struct HazardInfo { int courseId; HazardType type; };              // hazard: source course + kind of hazard
struct ProjInfo   { int courseId; };                               // exam projectile -> course that fired it

// on the singleton entity only
struct GameState {
    int   lives;
    float average;
    Phase phase;
    int   activeExamCourse; // -1 when not in exam
    int   coursesDone;
    int   coursesTotal;
    float gradeSum;         // running sum of exam grades; average = gradeSum / coursesDone
};

// ---- Tags (empty; checked via has<>(), never get<>()) ----
struct PaddleTag    {};
struct BallTag      {};
struct BrickTag     {};
struct WallTag      {};
struct DropTag      {};
struct ProjectileTag{};
struct HazardTag    {};
struct GameStateTag {};
struct DeadTag      {};   // marks entity for deletion by deadCleanupSystem

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
    template<> struct Storage<HazardTag>     final : NoInstance { using type = TaggedStorage<HazardTag>; };
    template<> struct Storage<GameStateTag>  final : NoInstance { using type = TaggedStorage<GameStateTag>; };
    template<> struct Storage<DeadTag>       final : NoInstance { using type = TaggedStorage<DeadTag>; };

    template<> struct Storage<Position>      final : NoInstance { using type = PackedStorage<Position>; };
    template<> struct Storage<PhysicsBody>   final : NoInstance { using type = PackedStorage<PhysicsBody>; };
    template<> struct Storage<Drawable>      final : NoInstance { using type = PackedStorage<Drawable>; };
    template<> struct Storage<Course>        final : NoInstance { using type = PackedStorage<Course>; };
}
