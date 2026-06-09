// MAY - HUD (graphical, no SDL_ttf). Lives, year display, status.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Game.h"
#include "Sprites.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

static void drawCurrentAcademicMonth(SDL_Renderer* r, float centerX, float y, int currentMonth) {
    char line[32];
    std::snprintf(line, sizeof line, "Month: %s", Config::academicMonthShort(currentMonth));
    constexpr float scale = 1.2f;
    const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
        * static_cast<float>(std::strlen(line));
    const float drawX = (centerX - textW * 0.5f) / scale;
    const float drawY = y / scale;

    SDL_SetRenderDrawColorFloat(r, 0.85f, 0.92f, 1.0f, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r, drawX, drawY, line);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

static const char* phaseName(Phase p) {
    switch (p) {
        case Phase::MENU:    return "MENU";
        case Phase::PLAYING: return "PLAYING";
        case Phase::EXAM:    return "EXAM";
        case Phase::WON:     return "WON";
        default:             return "LOST";
    }
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
        drawCurrentAcademicMonth(r, cx, meterY + meterH + 6.0f, month);
    }

    // Status line — phase + average grade, scaled up for readability
    {
        char buf[64];
        std::snprintf(buf, sizeof buf, "%s   avg: %.0f", phaseName(gs.phase), gs.average);
        constexpr float scale = 1.4f;
        SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 1.0f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r, 8.0f / scale, 52.0f / scale, buf);
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
        const char* msg = "Press SPACE to start";
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

    if (gs.phase == Phase::WON || gs.phase == Phase::LOST) {
        char end[96];
        if (gs.phase == Phase::WON)
            std::snprintf(end, sizeof end, "YOU WON!  avg: %.0f  -  press R", gs.average);
        else
            std::snprintf(end, sizeof end, "GAME OVER  -  press R");
        constexpr float scale = 2.5f;
        const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
            * static_cast<float>(std::strlen(end));
        SDL_SetRenderDrawColorFloat(r,
            gs.phase == Phase::WON ? 0.3f : 1.0f,
            gs.phase == Phase::WON ? 1.0f : 0.3f,
            0.2f, 1.0f);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r,
            (static_cast<float>(Config::WINDOW_W) * 0.5f - textW * 0.5f) / scale,
            static_cast<float>(Config::WINDOW_H) * 0.62f / scale,
            end);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }
}
