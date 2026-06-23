#pragma once
/// @file Events.h
/// @brief ECS event components and the ev:: producer helpers.
///
/// Events are short-lived entities (EventTag + one event component) created by producers
/// and consumed by any system in the same frame. eventCleanupSystem destroys them at
/// end-of-frame, so every event lives exactly one tick and may have multiple consumers.

#include "bagel.h"
#include "Components.h"

// ---- Event components ----
/// @brief Emitted by contactEventSystem when the ball hits a playable brick.
struct CourseHit     { int courseId; bagel::ent_type brick; };
/// @brief Emitted by contactEventSystem when the ball collides with the paddle.
struct PaddleHit     { bagel::ent_type ball; bagel::ent_type paddle; };
/// @brief Emitted by the physics sensor when a drop entity overlaps the paddle.
struct DropCaught    { int courseId; int courseIndex; DropType type; bagel::ent_type brick; float grade; };
/// @brief Emitted by brickClearDelaySystem when a brick's meter is full and its delay expires.
struct BrickCleared  { int courseId; int courseIndex; };
/// @brief Emitted by courseProgressSystem when a course track reaches 100% progress.
struct ExamStarted   { int courseId; };
/// @brief Emitted by examSystem when the exam timer expires, carrying the computed grade.
struct ExamFinished  { int courseId; bool passed; float grade; };
/// @brief Emitted by ballPaddleSystem (ball falls) or graduationSystem (foul).
struct LifeLost      { int amount; };
/// @brief Emitted by dropSystem when a Tax drop exits the bottom of the screen uncaught.
struct TaxMissed     { int courseIndex; };
/// @brief Emitted by the physics sensor when an exam projectile reaches the paddle area.
struct ProjectileHit { int courseId; };
/// @brief Emitted to toggle the slow-ball cheat speed band (debug hotkey S).
struct ToggleSlowBallCheat {};

/// @brief Presence tag on every event entity; eventCleanupSystem destroys all entities with this tag.
struct EventTag {};

namespace bagel {
    template<> struct Storage<EventTag> final : NoInstance { using type = TaggedStorage<EventTag>; };
}

// ---- Producer hooks (the only sanctioned way to raise an event) ----
namespace ev {
    /// @brief Raises a CourseHit event for the given course track and brick entity.
    /// @param courseId Colored track index (0..COURSES-1)
    /// @param brick Entity ID of the hit brick
    inline void courseHit(int courseId, bagel::ent_type brick) {
        bagel::Entity::create().addAll(CourseHit{courseId, brick}, EventTag{});
    }
    /// @brief Raises a PaddleHit event identifying the ball and paddle that collided.
    inline void paddleHit(bagel::ent_type ball, bagel::ent_type paddle) {
        bagel::Entity::create().addAll(PaddleHit{ball, paddle}, EventTag{});
    }
    /// @brief Raises a DropCaught event when the paddle catches a drop.
    /// @param grade Grade value carried by the drop (0 for non-graded drops)
    inline void dropCaught(int courseId, int courseIndex, DropType type,
                           bagel::ent_type brick, float grade = 0.0f) {
        bagel::Entity::create().addAll(DropCaught{courseId, courseIndex, type, brick, grade}, EventTag{});
    }
    /// @brief Raises a BrickCleared event after a brick's meter fills and its delay elapses.
    inline void brickCleared(int courseId, int courseIndex) {
        bagel::Entity::create().addAll(BrickCleared{courseId, courseIndex}, EventTag{});
    }
    /// @brief Raises an ExamStarted event, transitioning the given course into exam phase.
    inline void examStarted(int courseId) {
        bagel::Entity::create().addAll(ExamStarted{courseId}, EventTag{});
    }
    /// @brief Raises an ExamFinished event with the computed exam grade.
    /// @param passed True if the grade is at or above the passing threshold
    inline void examFinished(int courseId, bool passed, float grade) {
        bagel::Entity::create().addAll(ExamFinished{courseId, passed, grade}, EventTag{});
    }
    /// @brief Raises a LifeLost event deducting the given number of lives.
    inline void lifeLost(int amount) {
        bagel::Entity::create().addAll(LifeLost{amount}, EventTag{});
    }
    /// @brief Raises a TaxMissed event when a Tax drop exits the bottom uncaught.
    inline void taxMissed(int courseIndex) {
        bagel::Entity::create().addAll(TaxMissed{courseIndex}, EventTag{});
    }
    /// @brief Raises a ProjectileHit event when an exam projectile reaches the paddle.
    inline void projectileHit(int courseId) {
        bagel::Entity::create().addAll(ProjectileHit{courseId}, EventTag{});
    }
    /// @brief Raises a ToggleSlowBallCheat event (debug hotkey S).
    inline void toggleSlowBallCheat() {
        bagel::Entity::create().addAll(ToggleSlowBallCheat{}, EventTag{});
    }
}
