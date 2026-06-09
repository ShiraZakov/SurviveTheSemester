#pragma once
// Shared tunables
// World units are METERS (Box2D native). Pixels only appear at the SDL boundary
// (render + input), via PPM. Position is the CENTER of an entity.

namespace Config {
    // Window / world
    constexpr int   WINDOW_W = 1200;
    constexpr int   WINDOW_H = 940;
    constexpr float PPM      = 72.0f;                 // pixels per meter
    constexpr float WORLD_W  = WINDOW_W / PPM;
    constexpr float WORLD_H  = WINDOW_H / PPM;

    // Simulation
    constexpr float FIXED_DT = 1.0f / 60.0f;
    constexpr int   SUBSTEPS = 4;

    // Geometry (sizes tuned to spritesheet aspect ratios)
    constexpr float WALL         = 0.35f;
    constexpr float BALL_RADIUS  = 0.24f;
    constexpr float BALL_SPEED   = 9.0f;
    constexpr float PADDLE_W     = 2.8f;              // MTA paddle art ~ 2.2:1
    constexpr float PADDLE_H     = 0.72f;
    constexpr float PADDLE_VISUAL_H = PADDLE_W * (351.0f / 563.0f); // full sprite height in world units
    constexpr float PADDLE_SPEED = 14.0f;
    constexpr float BRICK_W      = 1.65f;             // course brick art ~ 1:1.32
    constexpr float BRICK_H      = 1.85f;
    constexpr float BRICK_GAP    = 0.15f;
    constexpr int   BRICK_COLS   = 7;                 // 7×3 = 21 courses, one brick each
    constexpr int   COURSES      = 3;
    constexpr float DROP_SIZE    = 0.55f;

    inline float paddleY() {
        return WORLD_H - WALL - 0.45f - PADDLE_H * 0.5f;
    }

    inline float brickGridWidth() {
        return BRICK_COLS * BRICK_W + (BRICK_COLS - 1) * BRICK_GAP;
    }

    inline int gridCourseIndex(int row, int col) {
        return row * BRICK_COLS + col;
    }
    constexpr float DROP_FALL    = 3.5f;

    // Rules
    constexpr int   START_LIVES = 3;
    constexpr float PASS        = 60.0f;   // average needed to win
    constexpr float BRICK_CLEAR_DELAY = 0.5f;
    constexpr float START_AVERAGE  = 100.0f;
    constexpr float TAX_MISS_PENALTY = 5.0f; // average drops when course tax is not caught
    constexpr float FOUL_PENALTY = 10.0f;    // each life lost (foul) deducts this from average
    constexpr float TAX_DROP_SIZE = 0.72f;   // tax marker drawn larger than assignment drops
    constexpr int   CATALOG_COURSES = 21;

    // Academic credits (נ"ז) per catalog course — weights the GPA average.
    inline int courseCredits(int courseIndex) {
        static constexpr int credits[CATALOG_COURSES] = {
            4, 4, 5, 5, 4, 4, 3,   // intro .. mathematical thinking
            4, 4, 4, 4, 4, 4, 4,   // discrete .. algorithms
            4, 4, 4, 4, 4, 4, 8,   // databases .. final project
        };
        if (courseIndex < 0 || courseIndex >= CATALOG_COURSES) return 4;
        return credits[courseIndex];
    }
    constexpr int   YEAR_COUNT   = 5;
    constexpr float YEAR_SECONDS = 30.0f;    // advance year when year courses done OR this elapses
    constexpr int   ACADEMIC_MONTHS = 10;  // Oct..Jul
    constexpr float MONTH_SECONDS = YEAR_SECONDS / static_cast<float>(ACADEMIC_MONTHS);

    inline int academicMonthIndex(float yearTimer) {
        int month = static_cast<int>(yearTimer / MONTH_SECONDS);
        if (month < 0) return 0;
        if (month >= ACADEMIC_MONTHS) return ACADEMIC_MONTHS - 1;
        return month;
    }

    inline const char* academicMonthShort(int month) {
        static constexpr const char* names[ACADEMIC_MONTHS] = {
            "Oct", "Nov", "Dec", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        };
        if (month < 0 || month >= ACADEMIC_MONTHS) return names[0];
        return names[month];
    }

    // Catalog-index buckets per academic year (grid layout stays unchanged).
    inline int yearCourseStart(int year) {
        static constexpr int sizes[YEAR_COUNT] = {4, 4, 4, 4, 5};
        int start = 0;
        for (int y = 1; y < year; ++y) start += sizes[y - 1];
        return start;
    }

    inline int yearCourseCount(int year) {
        static constexpr int sizes[YEAR_COUNT] = {4, 4, 4, 4, 5};
        return sizes[year - 1];
    }

    inline bool courseInYear(int year, int courseIndex) {
        const int start = yearCourseStart(year);
        return courseIndex >= start && courseIndex < start + yearCourseCount(year);
    }

    // Exam phase
    constexpr float EXAM_DURATION      = 15.0f;  // seconds per exam
    constexpr float EXAM_PROJ_INTERVAL =  3.0f;  // seconds between projectile spawns
    constexpr float EXAM_PROJ_SPEED    =  7.0f;  // m/s
    constexpr float EXAM_BASE_GRADE    = 100.0f;
    constexpr float EXAM_HIT_PENALTY   = 12.0f;  // deducted per projectile hit; <=3 hits = pass
    constexpr float EXAM_MIN_GRADE     = 55.0f;  // floor so grade never reads 0

    // Course color (graphical identity). Writes rgb in 0..1.
    inline void courseColor(int id, float& r, float& g, float& b) {
        switch (((id % COURSES) + COURSES) % COURSES) {
            case 0:  r = 0.90f; g = 0.32f; b = 0.32f; break; // red
            case 1:  r = 0.32f; g = 0.80f; b = 0.42f; break; // green
            default: r = 0.34f; g = 0.55f; b = 0.95f; break; // blue
        }
    }
}
