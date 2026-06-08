#pragma once
// Spritesheet rects from Book1.xlsx — breakout-assignment3 style (part on entity + SDL_RenderTexture).

#include "Components.h"
#include "Enums.h"
#include <cstdint>

namespace sprites {

constexpr int PREREQ_NONE = -1;
constexpr int PREREQ_ALL_COURSES = -2;   // final project: all other courses cleared

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

    COUNT
};

void initCatalog(); // sprite rects only (no textures) — for tests
bool init(SDL_Renderer* renderer);
void shutdown();
bool ready();

SpritePart makePart(Id id);

int courseIndexFromSprite(int spriteIndex);
bool courseUsesLockedArt(int courseIndex);
int coursePrereq(int courseIndex);
uint32_t coursePrereqMask(int courseIndex);
int courseMeterMax(int courseIndex);
Id courseSpriteId(int courseIndex, bool locked);
Id yearSpriteId(int year);

constexpr int COURSE_COUNT = 21;
constexpr int FINAL_COURSE_INDEX = COURSE_COUNT - 1;

bool courseStartsUnlocked(int courseIndex);
bool courseShowsLockedSprite(int courseIndex, bool unlocked);

void drawPart(SDL_Renderer* r, const SpritePart& sp, const SDL_FRect& dest);
void draw(SDL_Renderer* r, Id id, float dstX, float dstY, float dstW, float dstH);
void drawMeter3(SDL_Renderer* r, int filled, int maxFilled, float x, float y, float w, float h);
void drawMeter5(SDL_Renderer* r, int filled, int maxFilled, float x, float y, float w, float h);

} // namespace sprites
