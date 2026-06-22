
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Game.h"
#include "Sprites.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

/// @brief Renders the current academic month label centered at the given position.
/// @param r SDL renderer
/// @param centerX Horizontal center in pixels
/// @param y Top edge of the text in pixels
/// @param currentMonth Index 0–9 (Oct=0 … Jul=9)
/// @return void
static void drawCurrentAcademicMonth(SDL_Renderer* r, float centerX, float y, int currentMonth) {
    char line[32];
    std::snprintf(line, sizeof line, "Month: %s", Config::academicMonthShort(currentMonth));
    constexpr float scale = 2.0f;
    const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
        * static_cast<float>(std::strlen(line));
    const float drawX = (centerX - textW * 0.5f) / scale;
    const float drawY = y / scale;

    SDL_SetRenderDrawColorFloat(r, 0.85f, 0.92f, 1.0f, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r, drawX, drawY, line);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

static void drawYearAnnouncement(SDL_Renderer* r, float yearAnnounceTimer, int currentYear) {
    if (yearAnnounceTimer <= 0.0f) return;

    const float W = static_cast<float>(Config::WINDOW_W);
    const float H = static_cast<float>(Config::WINDOW_H);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(r, 0.02f, 0.04f, 0.10f, 0.72f);
    SDL_FRect overlay{0.0f, 0.0f, W, H};
    SDL_RenderFillRect(r, &overlay);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    if (sprites::ready()) {
        constexpr float yearW = 340.0f;
        constexpr float yearH = 128.0f;
        sprites::draw(r, sprites::yearSpriteId(currentYear),
                      (W - yearW) * 0.5f, H * 0.36f, yearW, yearH);
    }

    char title[32];
    std::snprintf(title, sizeof title, "Year %d", currentYear);
    constexpr float titleScale = 3.4f;
    const float titleW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * titleScale
        * static_cast<float>(std::strlen(title));
    SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.35f, 1.0f);
    SDL_SetRenderScale(r, titleScale, titleScale);
    SDL_RenderDebugText(r,
        (W * 0.5f - titleW * 0.5f) / titleScale,
        (H * 0.56f) / titleScale,
        title);
    SDL_SetRenderScale(r, 1.0f, 1.0f);

    const char* subtitle = "New academic year";
    constexpr float subScale = 1.8f;
    const float subW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * subScale
        * static_cast<float>(std::strlen(subtitle));
    SDL_SetRenderDrawColorFloat(r, 0.88f, 0.94f, 1.0f, 1.0f);
    SDL_SetRenderScale(r, subScale, subScale);
    SDL_RenderDebugText(r,
        (W * 0.5f - subW * 0.5f) / subScale,
        (H * 0.66f) / subScale,
        subtitle);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

static SDL_FRect pauseButtonRect() {
    return {16.0f, 50.0f, 96.0f, 34.0f};
}

bool hudPauseButtonVisible(Phase phase) {
    return phase == Phase::PLAYING || phase == Phase::EXAM || phase == Phase::GRADUATION;
}

bool hudPauseButtonAvailable() {
    const GameState& gs = gameState();
    if (!hudPauseButtonVisible(gs.phase)) return false;
    if (gs.gradAwaitingSpace) return false;
    if (gs.phase == Phase::PLAYING && !gs.started) return false;
    if (gs.yearAnnounceTimer > 0.0f) return false;
    return true;
}

bool hudPauseButtonHit(float px, float py) {
    if (!hudPauseButtonAvailable()) return false;
    const SDL_FRect b = pauseButtonRect();
    return px >= b.x && px <= b.x + b.w && py >= b.y && py <= b.y + b.h;
}

static void drawPauseButton(SDL_Renderer* r, bool paused) {
    const SDL_FRect b = pauseButtonRect();
    const char* label = paused ? "Resume" : "Pause";

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(r, 0.10f, 0.16f, 0.30f, 0.92f);
    SDL_RenderFillRect(r, &b);
    SDL_SetRenderDrawColorFloat(r, 0.55f, 0.72f, 1.0f, 1.0f);
    SDL_RenderRect(r, &b);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    constexpr float scale = 1.35f;
    const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
        * static_cast<float>(std::strlen(label));
    SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 1.0f, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r,
        (b.x + (b.w - textW) * 0.5f) / scale,
        (b.y + 10.0f) / scale,
        label);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

static void drawPausedBanner(SDL_Renderer* r) {
    const float W = static_cast<float>(Config::WINDOW_W);
    const char* title = "PAUSED";
    constexpr float scale = 2.4f;
    const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
        * static_cast<float>(std::strlen(title));

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_FRect bar{(W - textW - 24.0f) * 0.5f, 8.0f, textW + 24.0f, 34.0f};
    SDL_SetRenderDrawColorFloat(r, 0.05f, 0.08f, 0.16f, 0.72f);
    SDL_RenderFillRect(r, &bar);
    SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.35f, 0.95f);
    SDL_RenderRect(r, &bar);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.35f, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r, (W * 0.5f - textW * 0.5f) / scale, 14.0f / scale, title);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

/// @brief Draws the HUD: lives pips, year badge, year timer bar, academic month,
///        status line, start prompt, and win/lose overlay. Graphical only — no SDL_ttf.
/// @param r SDL renderer
/// @return void
void hudSystem(SDL_Renderer* r) {
    GameState& gs = gameState();

    constexpr float livesW = 128.0f;
    constexpr float livesH = livesW * (71.0f / 252.0f);
    sprites::drawMeter3(r, gs.lives, Config::START_LIVES, 16.0f, 10.0f, livesW, livesH);
    if (hudPauseButtonAvailable())
        drawPauseButton(r, gs.paused);

    // Year display — label + year number on one horizontal row, timer below
    if (sprites::ready()) {
        constexpr float labelW = 155.0f;
        constexpr float labelH = 36.0f;
        constexpr float yearW  = 140.0f;
        constexpr float yearH  = 42.0f;
        constexpr float meterW = 160.0f;
        constexpr float meterH = meterW * (71.0f / 252.0f); // natural sprite aspect ratio
        constexpr float gap    = 2.0f;
        const float cx = static_cast<float>(Config::WINDOW_W) * 0.5f;
        const float rowW = yearW + gap + labelW;
        const float rowY = 6.0f;
        const float rowH = yearH > labelH ? yearH : labelH;
        const float rowX = cx - rowW * 0.5f;

        sprites::draw(r, sprites::yearSpriteId(gs.currentYear),
                      rowX, rowY + (rowH - yearH) * 0.5f, yearW, yearH);
        sprites::draw(r, sprites::Id::CURRENT_YEAR,
                      rowX + yearW + gap, rowY + (rowH - labelH) * 0.5f, labelW, labelH);

        const float remaining = Config::YEAR_SECONDS - gs.yearTimer;
        const int yearFilled = remaining <= 0.0f
            ? 0
            : static_cast<int>((remaining / Config::YEAR_SECONDS) * 5.0f + 0.5f);
        const float meterY = rowY + rowH + gap;
        sprites::drawMeter5(r, yearFilled, 5, cx - meterW * 0.5f, meterY, meterW, meterH);

        const int month = Config::academicMonthIndex(gs.yearTimer);
        drawCurrentAcademicMonth(r, cx, meterY + meterH + 10.0f, month);
    }

    // Average grade — large, right-aligned (SDL debug text only)
    {
        char avgBuf[32];
        std::snprintf(avgBuf, sizeof avgBuf, "avg: %.0f", gs.average);
        constexpr float scale = 2.6f;
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(avgBuf));
        const float drawX = (static_cast<float>(Config::WINDOW_W) - textW - 12.0f) / scale;
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.35f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r, drawX, 44.0f / scale, avgBuf);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    if (gs.slowBallCheat && (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM)) {
        constexpr float scale = 1.35f;
        const char* label = "SLOW BALL: ON";
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(label));
        const float drawX = (static_cast<float>(Config::WINDOW_W) - textW - 12.0f) / scale;
        SDL_SetRenderDrawColorFloat(r, 0.55f, 0.95f, 1.0f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r, drawX, 82.0f / scale, label);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    // Exam phase indicator
    if (gs.phase == Phase::EXAM) {
        const float remaining = Config::EXAM_DURATION - gs.examTimer;
        const int   secs      = remaining > 0.0f ? static_cast<int>(remaining) + 1 : 0;
        char examBuf[64];
        std::snprintf(examBuf, sizeof examBuf, "EXAM  time:%ds  hits:%d", secs, gs.examHits);
        constexpr float scale = 2.0f;
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(examBuf));
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.30f, 0.20f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r,
            (static_cast<float>(Config::WINDOW_W) * 0.5f - textW * 0.5f) / scale,
            static_cast<float>(Config::WINDOW_H) * 0.66f / scale,
            examBuf);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    if (!gs.started && gs.phase == Phase::PLAYING) {
        constexpr float scale = 2.5f;
        const char* msg = "Click to launch";
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(msg));
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.3f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r,
            (static_cast<float>(Config::WINDOW_W) * 0.5f - textW * 0.5f) / scale,
            static_cast<float>(Config::WINDOW_H) * 0.70f / scale,
            msg);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    if (gs.phase == Phase::GRADUATION) {
        char foulBuf[32];
        std::snprintf(foulBuf, sizeof foulBuf, "fouls: %d", gs.gradFouls);
        constexpr float scale = 1.6f;
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.45f, 0.35f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r, 8.0f / scale, 118.0f / scale, foulBuf);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    if (gs.phase == Phase::GRADUATION && gs.gradAwaitingSpace && gs.lives > 0) {
        constexpr float scale = 2.0f;
        const char* msg = "Foul!  Click to retry graduation";
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(msg));
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.45f, 0.35f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r,
            (static_cast<float>(Config::WINDOW_W) * 0.5f - textW * 0.5f) / scale,
            static_cast<float>(Config::WINDOW_H) * 0.88f / scale,
            msg);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    } else if (gs.phase == Phase::GRADUATION && gs.gradAnimStep == 0
        && gs.gradNextChair < Config::graduationPathRows()) {
        constexpr float scale = 2.0f;
        const char* msg = "Left click to jump toward the stage";
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(msg));
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.3f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r,
            (static_cast<float>(Config::WINDOW_W) * 0.5f - textW * 0.5f) / scale,
            static_cast<float>(Config::WINDOW_H) * 0.88f / scale,
            msg);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    // End-screen overlay (Game.cpp drawEndScreen) owns WON/LOST messaging.

    // Elapsed timer — top-right corner, visible once game has started
    if (gs.started || gs.phase == Phase::WON || gs.phase == Phase::LOST) {
        const int totalSecs = static_cast<int>(gs.totalTime);
        const int mins = totalSecs / 60;
        const int secs = totalSecs % 60;
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof timeBuf, "Time: %d:%02d", mins, secs);
        constexpr float scale = 1.4f;
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(timeBuf));
        const float drawX = (static_cast<float>(Config::WINDOW_W) - textW - 8.0f) / scale;
        SDL_SetRenderDrawColorFloat(r, 0.85f, 0.92f, 1.0f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r, drawX, 8.0f / scale, timeBuf);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    // Pause banner — game stays visible underneath
    if (gs.paused)
        drawPausedBanner(r);

    drawYearAnnouncement(r, gs.yearAnnounceTimer, gs.currentYear);
}
