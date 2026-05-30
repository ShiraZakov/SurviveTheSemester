#pragma once
// Shared tunables
// World units are METERS (Box2D native). Pixels only appear at the SDL boundary
// (render + input), via PPM. Position is the CENTER of an entity.

namespace Config {
    // Window / world
    constexpr int   WINDOW_W = 1200;
    constexpr int   WINDOW_H = 900;
    constexpr float PPM      = 75.0f;                 // pixels per meter
    constexpr float WORLD_W  = WINDOW_W / PPM;        // 16 m
    constexpr float WORLD_H  = WINDOW_H / PPM;        // 12 m

    // Simulation
    constexpr float FIXED_DT = 1.0f / 60.0f;
    constexpr int   SUBSTEPS = 4;

    // Geometry
    constexpr float WALL         = 0.4f;
    constexpr float BALL_RADIUS  = 0.18f;
    constexpr float BALL_SPEED   = 9.0f;
    constexpr float PADDLE_W     = 2.2f;
    constexpr float PADDLE_H     = 0.4f;
    constexpr float PADDLE_Y     = 11.0f;
    constexpr float PADDLE_SPEED = 14.0f;
    constexpr float BRICK_W      = 1.3f;
    constexpr float BRICK_H      = 0.5f;
    constexpr float BRICK_GAP    = 0.15f;
    constexpr int   BRICK_COLS   = 8;
    constexpr int   COURSES      = 3;
    constexpr float DROP_SIZE    = 0.4f;
    constexpr float DROP_FALL    = 3.5f;

    // Rules
    constexpr int   START_LIVES = 3;
    constexpr float PASS        = 60.0f;   // average needed to win

    // Course color (graphical identity). Writes rgb in 0..1.
    inline void courseColor(int id, float& r, float& g, float& b) {
        switch (((id % COURSES) + COURSES) % COURSES) {
            case 0:  r = 0.90f; g = 0.32f; b = 0.32f; break; // red
            case 1:  r = 0.32f; g = 0.80f; b = 0.42f; break; // green
            default: r = 0.34f; g = 0.55f; b = 0.95f; break; // blue
        }
    }
}
