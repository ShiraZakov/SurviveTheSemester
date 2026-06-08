#pragma once
// Shared enums

enum class Phase       { MENU, PLAYING, EXAM, WON, LOST };
enum class CourseState { LOCKED, ACTIVE, DONE };
enum class DropType    { Assignment, Bonus, Tax };
enum class HazardType  { LoseLife, ReduceProgress };   // v1 effects only
enum class Shape       { Rect, Circle };
