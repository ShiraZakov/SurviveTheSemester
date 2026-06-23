#pragma once
/// @file Config.h
/// @brief Shared game constants and layout helpers.
///        World units are meters (Box2D native); pixels appear only at the SDL boundary via PPM.

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
    constexpr float SLOW_BALL_CHEAT_SPEED_FACTOR = 0.55f;
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

    /// @brief Returns the Y center of the paddle in world meters (just above the floor wall).
    inline float paddleY() {
        return WORLD_H - WALL - 0.45f - PADDLE_H * 0.5f;
    }

    /// @brief Returns the total pixel-width of the 7-column brick grid including gaps.
    inline float brickGridWidth() {
        return BRICK_COLS * BRICK_W + (BRICK_COLS - 1) * BRICK_GAP;
    }

    /// @brief Converts a grid (row, col) position to a flat catalog course index.
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

    /// @brief Returns the academic credit weight (נ"ז) for the given catalog course index.
    ///        Used to weight the GPA average.
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
    constexpr float YEAR_SECONDS = 45.0f;    // advance year when this elapses (independent of bricks)
    constexpr float YEAR_ANNOUNCE_SECONDS = 5.0f; // gameplay freeze while showing new year
    constexpr int   ACADEMIC_MONTHS = 10;  // Oct..Jul
    constexpr float MONTH_SECONDS = YEAR_SECONDS / static_cast<float>(ACADEMIC_MONTHS);

    /// @brief Returns the 0-based academic month index (0=Oct … 9=Jul) from elapsed year time.
    inline int academicMonthIndex(float yearTimer) {
        int month = static_cast<int>(yearTimer / MONTH_SECONDS);
        if (month < 0) return 0;
        if (month >= ACADEMIC_MONTHS) return ACADEMIC_MONTHS - 1;
        return month;
    }

    /// @brief Returns the abbreviated name of the given academic month index (0=Oct … 9=Jul).
    inline const char* academicMonthShort(int month) {
        static constexpr const char* names[ACADEMIC_MONTHS] = {
            "Oct", "Nov", "Dec", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        };
        if (month < 0 || month >= ACADEMIC_MONTHS) return names[0];
        return names[month];
    }

    /// @brief Returns the first catalog-course index belonging to the given 1-based academic year.
    inline int yearCourseStart(int year) {
        static constexpr int sizes[YEAR_COUNT] = {4, 4, 4, 4, 5};
        int start = 0;
        for (int y = 1; y < year; ++y) start += sizes[y - 1];
        return start;
    }

    /// @brief Returns the number of catalog courses belonging to the given 1-based academic year.
    inline int yearCourseCount(int year) {
        static constexpr int sizes[YEAR_COUNT] = {4, 4, 4, 4, 5};
        return sizes[year - 1];
    }

    /// @brief Returns true if the given catalog course belongs to the given academic year.
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

    // Graduation stage (stage 2) — rects from graduation_stage_spritesheet.png
    constexpr int   GRAD_CHAIR_ROWS       = 3;
    constexpr int   GRAD_CHAIR_COLS       = 5;
    constexpr float GRAD_BG_ASPECT        = 380.0f / 1536.0f;
    constexpr float GRAD_CHAIR_W          = 0.95f;
    constexpr float GRAD_CHAIR_H          = GRAD_CHAIR_W * (105.0f / 117.0f);
    constexpr float GRAD_CHAIR_COL_GAP    = 0.35f;
    constexpr float GRAD_CHAIR_ROW_GAP    = 0.55f;
    constexpr float GRAD_STUDENT_W        = 0.75f;
    constexpr float GRAD_STUDENT_H        = GRAD_STUDENT_W * (116.0f / 98.0f);
    constexpr float GRAD_VAULT_W          = 1.37f;
    constexpr float GRAD_VAULT_H          = GRAD_VAULT_W * (272.0f / 177.0f);
    constexpr float GRAD_STUDENT_GAP      = 0.45f;
    constexpr float GRAD_VAULT_DURATION   = 0.32f;
    constexpr float GRAD_LAND_DURATION    = 0.14f;
    constexpr float GRAD_CHAIR_STAGE_GAP  = 0.90f;
    constexpr float GRAD_OBSTACLE_W       = 1.65f;
    constexpr float GRAD_OBSTACLE_H       = 0.48f;
    constexpr float GRAD_OBSTACLE_SPEED   = 3.2f;

    /// @brief Returns the Y world-coordinate of the bottom edge of the graduation stage backdrop.
    inline float graduationStageBottomY() {
        return (static_cast<float>(WINDOW_W) * GRAD_BG_ASPECT) / PPM;
    }

    /// @brief Returns the Y world-coordinate of the student's starting position (bottom of screen).
    inline float graduationStudentStartY() {
        return WORLD_H - WALL - GRAD_STUDENT_H * 0.5f - 0.45f;
    }

    /// @brief Returns the Y world-coordinate of the top of the chair grid zone.
    inline float graduationChairZoneTopY() {
        return graduationStageBottomY() + GRAD_CHAIR_STAGE_GAP + GRAD_CHAIR_H * 0.5f;
    }

    /// @brief Returns the Y world-coordinate of the bottom of the chair grid zone.
    inline float graduationChairZoneBottomY() {
        return graduationStudentStartY()
            - GRAD_STUDENT_H - GRAD_STUDENT_GAP
            - GRAD_CHAIR_H * 0.5f - 0.20f;
    }

    /// @brief Returns the vertical distance between adjacent chair rows in world meters.
    inline float graduationRowStep() {
        if (GRAD_CHAIR_ROWS <= 1) return 0.0f;
        return (graduationChairZoneBottomY() - graduationChairZoneTopY())
            / static_cast<float>(GRAD_CHAIR_ROWS - 1);
    }

    /// @brief Returns the total number of chairs in the graduation grid (rows × cols).
    inline int graduationChairTotal() {
        return GRAD_CHAIR_ROWS * GRAD_CHAIR_COLS;
    }

    /// @brief Returns the number of chair rows the student must cross to reach the stage.
    inline int graduationPathRows() {
        return GRAD_CHAIR_ROWS;
    }

    /// @brief Returns the grid index of the center chair in the given row (the default landing target).
    inline int graduationPathChairIndex(int row) {
        return row * GRAD_CHAIR_COLS + GRAD_CHAIR_COLS / 2;
    }

    /// @brief Computes the world-space center position (x, y) of the chair at the given grid index.
    inline void graduationChairPos(int index, float& x, float& y) {
        const int row = index / GRAD_CHAIR_COLS;
        const int col = index % GRAD_CHAIR_COLS;
        const float gridW = GRAD_CHAIR_COLS * GRAD_CHAIR_W
            + (GRAD_CHAIR_COLS - 1) * GRAD_CHAIR_COL_GAP;
        const float startX = (WORLD_W - gridW) * 0.5f + GRAD_CHAIR_W * 0.5f;
        x = startX + col * (GRAD_CHAIR_W + GRAD_CHAIR_COL_GAP);
        y = graduationChairZoneBottomY() - row * graduationRowStep();
    }

    /// @brief Returns the X world-coordinates of the left and right edges of the chair grid.
    inline void graduationChairRowSpan(float& leftEdge, float& rightEdge) {
        float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        graduationChairPos(0, x0, y0);
        graduationChairPos(GRAD_CHAIR_COLS - 1, x1, y1);
        leftEdge = x0 - GRAD_CHAIR_W * 0.5f;
        rightEdge = x1 + GRAD_CHAIR_W * 0.5f;
    }

    /// @brief Clamps the student's X position so it stays within the horizontal chair-row bounds.
    /// @param halfW Half of the student sprite width
    inline float clampGradStudentX(float x, float halfW) {
        float left = 0.0f, right = 0.0f;
        graduationChairRowSpan(left, right);
        const float minX = left + halfW;
        const float maxX = right - halfW;
        if (x < minX) return minX;
        if (x > maxX) return maxX;
        return x;
    }

    /// @brief Returns the Y world-coordinate of the center of the gap between the given adjacent chair rows.
    inline float graduationObstacleY(int rowGap) {
        float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        graduationChairPos(rowGap * GRAD_CHAIR_COLS, x0, y0);
        graduationChairPos((rowGap + 1) * GRAD_CHAIR_COLS, x1, y1);
        return (y0 + y1) * 0.5f;
    }

    /// @brief Returns the number of row-gap slots that can contain an obstacle.
    inline int graduationObstacleRowGapCount() {
        return GRAD_CHAIR_ROWS > 1 ? GRAD_CHAIR_ROWS - 1 : 1;
    }

    /// @brief Returns the grid index of the chair in the given row closest to worldX.
    inline int graduationNearestChairInRow(int row, float worldX) {
        int best = graduationPathChairIndex(row);
        float bestDist = 1e9f;
        for (int col = 0; col < GRAD_CHAIR_COLS; ++col) {
            const int index = row * GRAD_CHAIR_COLS + col;
            float x = 0.0f, y = 0.0f;
            graduationChairPos(index, x, y);
            const float dist = x > worldX ? x - worldX : worldX - x;
            if (dist < bestDist) {
                bestDist = dist;
                best = index;
            }
        }
        return best;
    }

    /// @brief Writes the display color (r, g, b in 0..1) for the given course track ID.
    inline void courseColor(int id, float& r, float& g, float& b) {
        switch (((id % COURSES) + COURSES) % COURSES) {
            case 0:  r = 0.90f; g = 0.32f; b = 0.32f; break; // red
            case 1:  r = 0.32f; g = 0.80f; b = 0.42f; break; // green
            default: r = 0.34f; g = 0.55f; b = 0.95f; break; // blue
        }
    }
}
