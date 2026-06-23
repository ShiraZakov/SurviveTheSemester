#pragma once
/// @file Enums.h
/// @brief Shared enumerations used across all systems and components.

/// @brief Overall game phase; drives which systems run each frame.
enum class Phase       { MENU, PLAYING, EXAM, GRADUATION, WON, LOST };

/// @brief Distinguishes the reason a Stage::LOST state was reached.
enum class LoseReason  { None, NoLives, YearsExhausted, AverageTooLow, Stage1AvgTooLow };

/// @brief Lifecycle state of a single course track (colored row of bricks).
enum class CourseState { LOCKED, ACTIVE, DONE };

/// @brief Category of a falling collectible drop.
enum class DropType    { Assignment, Bonus, Tax };

/// @brief Primitive render shape used by the Drawable component fallback renderer.
enum class Shape       { Rect, Circle };
