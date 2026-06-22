#pragma once
// Event entities = the cross-system "hooks"
//
// An event is a short-lived entity carrying ONE event component + EventTag.
// Producers call the ev::* helpers below. Consumers iterate via a Mask query
// (MaskBuilder().set<EventT>().build() + World::createQuery).
// The core deletes ALL event entities at end-of-frame (eventCleanupSystem), so
// every event lives exactly one frame and may have multiple consumers.

#include "bagel.h"
#include "Components.h"

// ---- Event components ----
struct CourseHit     { int courseId; bagel::ent_type brick; };      // onCourseHit
struct PaddleHit     { bagel::ent_type ball; bagel::ent_type paddle; };
struct DropCaught    { int courseId; int courseIndex; DropType type; bagel::ent_type brick; float grade; };
struct BrickCleared  { int courseId; int courseIndex; };
struct ExamStarted   { int courseId; };                             // startExam
struct ExamFinished  { int courseId; bool passed; float grade; };   // finishExam
struct LifeLost      { int amount; };
struct TaxMissed     { int courseIndex; };
struct ProjectileHit { int courseId; };   // exam projectile reached the paddle
struct ToggleSlowBallCheat {};

struct EventTag {};   // marks every event entity for end-of-frame cleanup

namespace bagel {
    template<> struct Storage<EventTag> final : NoInstance { using type = TaggedStorage<EventTag>; };
}

// ---- Producer hooks (the only sanctioned way to raise an event) ----
namespace ev {
    inline void courseHit(int courseId, bagel::ent_type brick) {
        bagel::Entity::create().addAll(CourseHit{courseId, brick}, EventTag{});
    }
    inline void paddleHit(bagel::ent_type ball, bagel::ent_type paddle) {
        bagel::Entity::create().addAll(PaddleHit{ball, paddle}, EventTag{});
    }
    inline void dropCaught(int courseId, int courseIndex, DropType type,
                           bagel::ent_type brick, float grade = 0.0f) {
        bagel::Entity::create().addAll(DropCaught{courseId, courseIndex, type, brick, grade}, EventTag{});
    }
    inline void brickCleared(int courseId, int courseIndex) {
        bagel::Entity::create().addAll(BrickCleared{courseId, courseIndex}, EventTag{});
    }
    inline void examStarted(int courseId) {
        bagel::Entity::create().addAll(ExamStarted{courseId}, EventTag{});
    }
    inline void examFinished(int courseId, bool passed, float grade) {
        bagel::Entity::create().addAll(ExamFinished{courseId, passed, grade}, EventTag{});
    }
    inline void lifeLost(int amount) {
        bagel::Entity::create().addAll(LifeLost{amount}, EventTag{});
    }
    inline void taxMissed(int courseIndex) {
        bagel::Entity::create().addAll(TaxMissed{courseIndex}, EventTag{});
    }
    inline void projectileHit(int courseId) {
        bagel::Entity::create().addAll(ProjectileHit{courseId}, EventTag{});
    }
    inline void toggleSlowBallCheat() {
        bagel::Entity::create().addAll(ToggleSlowBallCheat{}, EventTag{});
    }
}
