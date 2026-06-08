#include "Sprites.h"
#include "Components.h"

#include <SDL3_image/SDL_image.h>

#include <array>
#include <cstdint>
#include <string>

namespace sprites {

namespace {

enum class Atlas { Courses, Student, Locked, COUNT };

struct Rect { float x, y, w, h; };

struct Entry { Atlas atlas; Rect rect; };

static SDL_Texture* g_textures[static_cast<int>(Atlas::COUNT)]{};
static std::array<Entry, static_cast<size_t>(Id::COUNT)> g_entries{};

static void set(Id id, Atlas atlas, float x1, float y1, float x2, float y2) {
    g_entries[static_cast<size_t>(id)] = {atlas, {x1, y1, x2 - x1, y2 - y1}};
}

static void defineSprites() {
    set(Id::COURSE_INTRO_CS, Atlas::Courses, 26, 71, 238, 350);
    set(Id::COURSE_C_PROGRAMMING, Atlas::Courses, 238, 66, 454, 350);
    set(Id::COURSE_CALCULUS_1, Atlas::Courses, 455, 67, 666, 350);
    set(Id::COURSE_CALCULUS_2, Atlas::Courses, 671, 69, 874, 350);
    set(Id::COURSE_LINEAR_ALGEBRA_1, Atlas::Courses, 876, 69, 1086, 350);
    set(Id::COURSE_LOGIC_AND_SETS, Atlas::Courses, 1085, 65, 1300, 350);
    set(Id::COURSE_MATHEMATICAL_REASONING, Atlas::Courses, 1307, 69, 1510, 350);
    set(Id::COURSE_DISCRETE_MATH, Atlas::Courses, 17, 373, 214, 627);
    set(Id::COURSE_PROBABILITY, Atlas::Courses, 214, 373, 407, 627);
    set(Id::COURSE_COMPUTER_STRUCTURE, Atlas::Courses, 405, 373, 592, 627);
    set(Id::COURSE_OOP_CPP, Atlas::Courses, 594, 373, 774, 627);
    set(Id::COURSE_DATA_STRUCTURES, Atlas::Courses, 776, 373, 959, 627);
    set(Id::COURSE_LINEAR_ALGEBRA_2, Atlas::Courses, 959, 373, 1146, 627);
    set(Id::COURSE_COMPUTER_ARCHITECTURE, Atlas::Courses, 1147, 373, 1328, 627);
    set(Id::COURSE_ALGORITHMS, Atlas::Courses, 1327, 373, 1515, 627);
    set(Id::COURSE_DATABASES, Atlas::Courses, 75, 653, 302, 933);
    set(Id::COURSE_OPERATING_SYSTEMS, Atlas::Courses, 975, 653, 1209, 933);
    set(Id::COURSE_COMPUTER_NETWORKS, Atlas::Courses, 750, 653, 975, 933);
    set(Id::COURSE_COMPUTABILITY, Atlas::Courses, 527, 653, 751, 933);
    set(Id::COURSE_COMPLEXITY, Atlas::Courses, 303, 653, 526, 933);
    set(Id::COURSE_FINAL_PROJECT, Atlas::Courses, 1207, 653, 1461, 933);

    set(Id::PLAYER_RUN_RIGHT, Atlas::Student, 606, 138, 946, 304);
    set(Id::PLAYER_RUN_LEFT, Atlas::Student, 579, 368, 946, 516);
    set(Id::PADDLE_MTA, Atlas::Student, 58, 120, 563, 351);
    set(Id::BALL_DEFAULT, Atlas::Student, 701, 570, 838, 677);
    set(Id::ASSIGNMENT_GREEN, Atlas::Student, 976, 126, 1150, 260);
    set(Id::ASSIGNMENT_BLUE, Atlas::Student, 356, 382, 509, 505);
    set(Id::METER3_EMPTY, Atlas::Student, 124, 748, 381, 827);
    set(Id::METER3_1_3, Atlas::Student, 460, 748, 723, 831);
    set(Id::METER3_2_3, Atlas::Student, 809, 744, 1065, 829);
    set(Id::METER3_3_3, Atlas::Student, 1149, 743, 1410, 827);
    set(Id::METER5_EMPTY, Atlas::Student, 15, 923, 267, 994);
    set(Id::METER5_1_5, Atlas::Student, 283, 919, 502, 996);
    set(Id::METER5_2_5, Atlas::Student, 517, 917, 739, 994);
    set(Id::METER5_3_5, Atlas::Student, 757, 921, 982, 991);
    set(Id::METER5_4_5, Atlas::Student, 1001, 917, 1241, 995);
    set(Id::METER5_5_5, Atlas::Student, 1254, 919, 1528, 993);

    set(Id::LOCKED_CALCULUS_1, Atlas::Locked, 203, 174, 423, 472);
    set(Id::LOCKED_CALCULUS_2, Atlas::Locked, 424, 174, 638, 472);
    set(Id::LOCKED_LINEAR_ALGEBRA_1, Atlas::Locked, 640, 174, 847, 472);
    set(Id::LOCKED_LOGIC_AND_SETS, Atlas::Locked, 847, 174, 1053, 472);
    set(Id::LOCKED_COMPUTER_STRUCTURE, Atlas::Locked, 130, 472, 331, 760);
    set(Id::LOCKED_OOP_CPP, Atlas::Locked, 333, 472, 541, 760);
    set(Id::LOCKED_DATA_STRUCTURES, Atlas::Locked, 543, 472, 747, 760);
    set(Id::LOCKED_LINEAR_ALGEBRA_2, Atlas::Locked, 746, 472, 957, 760);
    set(Id::LOCKED_COMPUTER_ARCHITECTURE, Atlas::Locked, 959, 472, 1168, 760);
    set(Id::LOCKED_ALGORITHMS, Atlas::Locked, 1167, 472, 1378, 760);
    set(Id::LOCKED_OPERATING_SYSTEMS, Atlas::Locked, 202, 758, 424, 1021);
    set(Id::LOCKED_COMPUTER_NETWORKS, Atlas::Locked, 421, 758, 635, 1021);
    set(Id::LOCKED_COMPUTABILITY, Atlas::Locked, 636, 758, 848, 1021);
    set(Id::LOCKED_COMPLEXITY, Atlas::Locked, 850, 758, 1061, 1021);
    set(Id::LOCKED_FINAL_PROJECT, Atlas::Locked, 1063, 758, 1283, 1021);
    set(Id::YEAR_FIRST, Atlas::Locked, 148, 101, 388, 165);
    set(Id::YEAR_SECOND, Atlas::Locked, 416, 97, 629, 163);
    set(Id::YEAR_THIRD, Atlas::Locked, 666, 102, 884, 159);
    set(Id::YEAR_FOURTH, Atlas::Locked, 917, 101, 1136, 161);
    set(Id::YEAR_FIFTH, Atlas::Locked, 1175, 95, 1392, 165);
    set(Id::CURRENT_YEAR, Atlas::Locked, 756, 17, 975, 82);
}

static constexpr Id kCourseBricks[] = {
    Id::COURSE_INTRO_CS,
    Id::COURSE_C_PROGRAMMING,
    Id::COURSE_CALCULUS_1,
    Id::COURSE_CALCULUS_2,
    Id::COURSE_LINEAR_ALGEBRA_1,
    Id::COURSE_LOGIC_AND_SETS,
    Id::COURSE_MATHEMATICAL_REASONING,
    Id::COURSE_DISCRETE_MATH,
    Id::COURSE_PROBABILITY,
    Id::COURSE_COMPUTER_STRUCTURE,
    Id::COURSE_OOP_CPP,
    Id::COURSE_DATA_STRUCTURES,
    Id::COURSE_LINEAR_ALGEBRA_2,
    Id::COURSE_COMPUTER_ARCHITECTURE,
    Id::COURSE_ALGORITHMS,
    Id::COURSE_DATABASES,
    Id::COURSE_OPERATING_SYSTEMS,
    Id::COURSE_COMPUTER_NETWORKS,
    Id::COURSE_COMPUTABILITY,
    Id::COURSE_COMPLEXITY,
    Id::COURSE_FINAL_PROJECT,
};

static constexpr Id kLockedBricks[] = {
    Id::COURSE_INTRO_CS,
    Id::COURSE_C_PROGRAMMING,
    Id::LOCKED_CALCULUS_1,
    Id::LOCKED_CALCULUS_2,
    Id::LOCKED_LINEAR_ALGEBRA_1,
    Id::LOCKED_LOGIC_AND_SETS,
    Id::COURSE_MATHEMATICAL_REASONING,
    Id::COURSE_DISCRETE_MATH,
    Id::COURSE_PROBABILITY,
    Id::LOCKED_COMPUTER_STRUCTURE,
    Id::LOCKED_OOP_CPP,
    Id::LOCKED_DATA_STRUCTURES,
    Id::LOCKED_LINEAR_ALGEBRA_2,
    Id::LOCKED_COMPUTER_ARCHITECTURE,
    Id::LOCKED_ALGORITHMS,
    Id::COURSE_DATABASES,
    Id::LOCKED_OPERATING_SYSTEMS,
    Id::LOCKED_COMPUTER_NETWORKS,
    Id::LOCKED_COMPUTABILITY,
    Id::LOCKED_COMPLEXITY,
    Id::LOCKED_FINAL_PROJECT,
};

static constexpr int kCourseCount = static_cast<int>(sizeof(kCourseBricks) / sizeof(kCourseBricks[0]));

// Prerequisite: arrow "A -> B" means B requires A cleared first.
static constexpr int kPrereq[] = {
    PREREQ_NONE,          // INTRO_CS
    PREREQ_NONE,          // C_PROGRAMMING
    6,                    // CALCULUS_1        <- MATHEMATICAL_REASONING
    2,                    // CALCULUS_2        <- CALCULUS_1
    6,                    // LINEAR_ALGEBRA_1  <- MATHEMATICAL_REASONING
    2,                    // LOGIC_AND_SETS    <- CALCULUS_1
    PREREQ_NONE,          // MATHEMATICAL_REASONING
    PREREQ_NONE,          // DISCRETE_MATH
    PREREQ_NONE,          // PROBABILITY
    2,                    // COMPUTER_STRUCTURE <- CALCULUS_1
    1,                    // OOP_CPP           <- C_PROGRAMMING
    1,                    // DATA_STRUCTURES   <- C_PROGRAMMING
    4,                    // LINEAR_ALGEBRA_2  <- LINEAR_ALGEBRA_1
    9,                    // COMPUTER_ARCHITECTURE <- COMPUTER_STRUCTURE
    11,                   // ALGORITHMS        <- DATA_STRUCTURES
    PREREQ_NONE,          // DATABASES
    13,                   // OPERATING_SYSTEMS <- COMPUTER_ARCHITECTURE
    9,                    // COMPUTER_NETWORKS <- COMPUTER_STRUCTURE
    11,                   // COMPUTABILITY     <- DATA_STRUCTURES
    14,                   // COMPLEXITY        <- ALGORITHMS
    PREREQ_ALL_COURSES,   // FINAL_PROJECT     <- all other courses
};

static constexpr int kMeterMax[] = {
    3, 3, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5,
};

static constexpr bool kUsesLocked[] = {
    false, false, true, true, true, true, false, false, false,
    true, true, true, true, true, true, false,
    true, true, true, true, true,
};

static_assert(kCourseCount == COURSE_COUNT, "course catalog size mismatch");
static_assert(sizeof(kPrereq) / sizeof(kPrereq[0]) == COURSE_COUNT, "kPrereq size mismatch");
static_assert(sizeof(kMeterMax) / sizeof(kMeterMax[0]) == COURSE_COUNT, "kMeterMax size mismatch");
static_assert(sizeof(kUsesLocked) / sizeof(kUsesLocked[0]) == COURSE_COUNT, "kUsesLocked size mismatch");

static Id pickCourseSprite(int courseIndex, bool locked) {
    const int idx = ((courseIndex % kCourseCount) + kCourseCount) % kCourseCount;
    return locked ? kLockedBricks[idx] : kCourseBricks[idx];
}

static Id meter3Id(int filled, int maxFilled) {
    if (filled <= 0) return Id::METER3_EMPTY;
    if (filled >= maxFilled) return Id::METER3_3_3;
    if (filled * 3 <= maxFilled) return Id::METER3_1_3;
    return Id::METER3_2_3;
}

static Id meter5Id(int filled, int maxFilled) {
    if (filled <= 0) return Id::METER5_EMPTY;
    const int step = (filled * 5 + maxFilled - 1) / maxFilled;
    switch (step) {
        case 1: return Id::METER5_1_5;
        case 2: return Id::METER5_2_5;
        case 3: return Id::METER5_3_5;
        case 4: return Id::METER5_4_5;
        default: return Id::METER5_5_5;
    }
}

static bool loadTexture(SDL_Renderer* renderer, const char* file, int atlasIndex) {
    const std::string rel = std::string("res/assets/") + file;
    g_textures[atlasIndex] = IMG_LoadTexture(renderer, rel.c_str());
    if (!g_textures[atlasIndex]) {
        if (const char* base = SDL_GetBasePath()) {
            const std::string abs = std::string(base) + "res/assets/" + file;
            g_textures[atlasIndex] = IMG_LoadTexture(renderer, abs.c_str());
        }
    }
    if (!g_textures[atlasIndex]) {
        SDL_Log("sprites: failed to load '%s': %s", rel.c_str(), SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(g_textures[atlasIndex], SDL_SCALEMODE_LINEAR);
    SDL_SetTextureBlendMode(g_textures[atlasIndex], SDL_BLENDMODE_BLEND);
    return true;
}

} // namespace

void initCatalog() {
    defineSprites();
}

bool init(SDL_Renderer* renderer) {
    initCatalog();
    const char* files[] = {
        "course_bricks_spritesheet.jpeg",
        "student_spritesheet.jpeg",
        "locked_courses_spritesheet.jpeg",
    };
    for (int i = 0; i < static_cast<int>(Atlas::COUNT); ++i) {
        if (!loadTexture(renderer, files[i], i)) return false;
    }
    return true;
}

bool ready() {
    for (auto* tex : g_textures)
        if (!tex) return false;
    return true;
}

SpritePart makePart(Id id) {
    const auto& entry = g_entries[static_cast<size_t>(id)];
    return SpritePart{
        {entry.rect.x, entry.rect.y, entry.rect.w, entry.rect.h},
        static_cast<int>(entry.atlas),
    };
}

int courseIndexFromSprite(int spriteIndex) {
    if (spriteIndex < 0 || spriteIndex >= kCourseCount) return 0;
    return spriteIndex;
}

bool courseStartsUnlocked(int courseIndex) {
    return coursePrereqMask(courseIndex) == 0;
}

bool courseShowsLockedSprite(int courseIndex, bool unlocked) {
    if (unlocked) return false;
    const int i = ((courseIndex % kCourseCount) + kCourseCount) % kCourseCount;
    return kUsesLocked[i] || i == FINAL_COURSE_INDEX;
}

bool courseUsesLockedArt(int courseIndex) {
    const int i = ((courseIndex % kCourseCount) + kCourseCount) % kCourseCount;
    return kUsesLocked[i];
}

int coursePrereq(int courseIndex) {
    const int i = ((courseIndex % kCourseCount) + kCourseCount) % kCourseCount;
    return kPrereq[i];
}

uint32_t coursePrereqMask(int courseIndex) {
    const int prereq = coursePrereq(courseIndex);
    if (prereq == PREREQ_NONE) return 0;
    if (prereq == PREREQ_ALL_COURSES) {
        uint32_t mask = 0;
        for (int i = 0; i < FINAL_COURSE_INDEX; ++i)
            mask |= (1u << i);
        return mask;
    }
    if (prereq < 0 || prereq >= FINAL_COURSE_INDEX) return 0;
    return 1u << prereq;
}

int courseMeterMax(int courseIndex) {
    const int i = ((courseIndex % kCourseCount) + kCourseCount) % kCourseCount;
    return kMeterMax[i];
}

Id courseSpriteId(int courseIndex, bool locked) {
    return pickCourseSprite(courseIndex, locked);
}

Id yearSpriteId(int year) {
    switch (year) {
        case 1: return Id::YEAR_FIRST;
        case 2: return Id::YEAR_SECOND;
        case 3: return Id::YEAR_THIRD;
        case 4: return Id::YEAR_FOURTH;
        case 5: return Id::YEAR_FIFTH;
        default: return Id::YEAR_FIRST;
    }
}

void shutdown() {
    for (auto& tex : g_textures) {
        if (tex) SDL_DestroyTexture(tex);
        tex = nullptr;
    }
}

void drawPart(SDL_Renderer* r, const SpritePart& sp, const SDL_FRect& dest) {
    if (sp.sheet < 0 || sp.sheet >= static_cast<int>(Atlas::COUNT)) return;
    SDL_Texture* tex = g_textures[sp.sheet];
    if (!tex) return;
    SDL_RenderTexture(r, tex, &sp.part, &dest);
}

void draw(SDL_Renderer* r, Id id, float dstX, float dstY, float dstW, float dstH) {
    const SDL_FRect dest{dstX, dstY, dstW, dstH};
    drawPart(r, makePart(id), dest);
}

void drawMeter3(SDL_Renderer* r, int filled, int maxFilled, float x, float y, float w, float h) {
    draw(r, meter3Id(filled, maxFilled), x, y, w, h);
}

void drawMeter5(SDL_Renderer* r, int filled, int maxFilled, float x, float y, float w, float h) {
    draw(r, meter5Id(filled, maxFilled), x, y, w, h);
}

} // namespace sprites
