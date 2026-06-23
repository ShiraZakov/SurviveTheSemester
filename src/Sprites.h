#pragma once
/// @file Sprites.h
/// @brief Spritesheet catalog, texture management, and draw helpers for all in-game art.
///        Sprite rects are defined in sprites::Id enum values and looked up at draw time.

#include "Components.h"
#include "Enums.h"
#include <cstdint>

namespace sprites {

constexpr int PREREQ_NONE = -1;
/// @brief Sentinel for the final-project prereq: requires ALL other courses to be cleared first.
constexpr int PREREQ_ALL_COURSES = -2;

/// @brief Enumeration of every sprite identifier used in the game.
enum class Id {
    COURSE_INTRO_CS,
    COURSE_C_PROGRAMMING,
    COURSE_CALCULUS_1,
    COURSE_CALCULUS_2,
    COURSE_LINEAR_ALGEBRA_1,
    COURSE_LOGIC_AND_SETS,
    COURSE_MATHEMATICAL_REASONING,
    COURSE_DISCRETE_MATH,
    COURSE_PROBABILITY,
    COURSE_COMPUTER_STRUCTURE,
    COURSE_OOP_CPP,
    COURSE_DATA_STRUCTURES,
    COURSE_LINEAR_ALGEBRA_2,
    COURSE_COMPUTER_ARCHITECTURE,
    COURSE_ALGORITHMS,
    COURSE_DATABASES,
    COURSE_OPERATING_SYSTEMS,
    COURSE_COMPUTER_NETWORKS,
    COURSE_COMPUTABILITY,
    COURSE_COMPLEXITY,
    COURSE_FINAL_PROJECT,

    PLAYER_RUN_RIGHT,
    PLAYER_RUN_LEFT,
    PADDLE_MTA,
    BALL_DEFAULT,
    ASSIGNMENT_GREEN,
    ASSIGNMENT_BLUE,
    METER3_EMPTY,
    METER3_1_3,
    METER3_2_3,
    METER3_3_3,
    METER5_EMPTY,
    METER5_1_5,
    METER5_2_5,
    METER5_3_5,
    METER5_4_5,
    METER5_5_5,

    LOCKED_CALCULUS_1,
    LOCKED_CALCULUS_2,
    LOCKED_LINEAR_ALGEBRA_1,
    LOCKED_LOGIC_AND_SETS,
    LOCKED_COMPUTER_STRUCTURE,
    LOCKED_OOP_CPP,
    LOCKED_DATA_STRUCTURES,
    LOCKED_LINEAR_ALGEBRA_2,
    LOCKED_COMPUTER_ARCHITECTURE,
    LOCKED_ALGORITHMS,
    LOCKED_OPERATING_SYSTEMS,
    LOCKED_COMPUTER_NETWORKS,
    LOCKED_COMPUTABILITY,
    LOCKED_COMPLEXITY,
    LOCKED_FINAL_PROJECT,
    YEAR_FIRST,
    YEAR_SECOND,
    YEAR_THIRD,
    YEAR_FOURTH,
    YEAR_FIFTH,
    CURRENT_YEAR,

    GRAD_STAGE_BACKGROUND,
    GRAD_CHAIR,
    GRAD_STUDENT_IDLE,
    GRAD_STUDENT_VAULT,

    COUNT
};

/// @brief Initializes sprite rect catalog without loading textures (for unit tests).
void initCatalog();

/// @brief Loads all spritesheet textures and initializes the sprite catalog.
/// @param renderer SDL renderer used for IMG_LoadTexture calls
/// @return True on success; false if any texture file failed to load
bool init(SDL_Renderer* renderer);

/// @brief Destroys all loaded textures and clears the atlas table.
void shutdown();

/// @brief Returns true when all atlas textures have been successfully loaded.
bool ready();

/// @brief Constructs a SpritePart (crop rect + atlas index) for the given sprite ID.
SpritePart makePart(Id id);

/// @brief Maps a spritesheet index (0..20) to the corresponding catalog course index.
int courseIndexFromSprite(int spriteIndex);

/// @brief Returns true if the given catalog course has a dedicated locked-state sprite.
bool courseUsesLockedArt(int courseIndex);

/// @brief Returns the single prerequisite catalog course index for the given course,
///        or PREREQ_NONE / PREREQ_ALL_COURSES for courses with special prerequisites.
int coursePrereq(int courseIndex);

/// @brief Returns a bitmask where bit i is set if catalog course i must be cleared first.
uint32_t coursePrereqMask(int courseIndex);

/// @brief Returns the number of hits required to fill the brick meter for this course.
int courseMeterMax(int courseIndex);

/// @brief Returns the sprite Id to use for a course brick in its current lock state.
/// @param locked True to return the locked-art variant
Id courseSpriteId(int courseIndex, bool locked);

/// @brief Returns the year-label sprite Id (YEAR_FIRST…YEAR_FIFTH) for the given 1-based year.
Id yearSpriteId(int year);

constexpr int COURSE_COUNT = 21;
/// @brief Catalog index of the final-project brick (requires all others cleared first).
constexpr int FINAL_COURSE_INDEX = COURSE_COUNT - 1;

/// @brief Returns true if the course starts the game already unlocked (no prerequisites).
bool courseStartsUnlocked(int courseIndex);

/// @brief Returns true if the brick should display its locked sprite given the current unlock state.
bool courseShowsLockedSprite(int courseIndex, bool unlocked);

/// @brief Renders a pre-cropped SpritePart to the given destination rectangle.
void drawPart(SDL_Renderer* r, const SpritePart& sp, const SDL_FRect& dest);

/// @brief Renders a named sprite Id scaled to the given destination rectangle.
void draw(SDL_Renderer* r, Id id, float dstX, float dstY, float dstW, float dstH);

/// @brief Draws the graduation stage backdrop spanning the full window width.
void drawGraduationStage(SDL_Renderer* r);

/// @brief Draws a 3-segment brick meter at the given position using the correct fill sprite.
void drawMeter3(SDL_Renderer* r, int filled, int maxFilled, float x, float y, float w, float h);

/// @brief Draws a 5-segment brick meter at the given position using the correct fill sprite.
void drawMeter5(SDL_Renderer* r, int filled, int maxFilled, float x, float y, float w, float h);

} // namespace sprites
