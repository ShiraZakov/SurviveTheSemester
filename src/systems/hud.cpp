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
    constexpr float scale = 1.5f;
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

void hudSystem(SDL_Renderer* r) {
    GameState& gs = gameState();

    sprites::drawMeter3(r, gs.lives, Config::START_LIVES, 16.0f, 10.0f, 200.0f, 42.0f);

    // Year display — label + year number on one horizontal row, timer below
    if (sprites::ready()) {
        constexpr float labelW = 200.0f;
        constexpr float labelH = 48.0f;
        constexpr float yearW  = 180.0f;
        constexpr float yearH  = 54.0f;
        constexpr float meterW = 260.0f;
        constexpr float meterH = 20.0f;
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

    // Status line
    SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 1.0f, 1.0f);
    char buf[128];
    std::snprintf(buf, sizeof buf, "%s  avg:%.0f  time:%.0fs  tax:%d/%d",
                  phaseName(gs.phase), gs.average, gs.totalTime,
                  countCaughtTax(gs), GameState::COURSE_GRADES);
    SDL_RenderDebugText(r, 16.0f, 58.0f, buf);

    if (!gs.started && (gs.phase == Phase::PLAYING || gs.phase == Phase::LOST || gs.phase == Phase::WON)) {
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.5f, 1.0f);
        SDL_RenderDebugText(r, Config::WINDOW_W * 0.5f - 90.0f, Config::WINDOW_H * 0.42f,
                            "Press SPACE to start");
    }

    if (gs.phase == Phase::WON || gs.phase == Phase::LOST) {
        char end[96];
        if (gs.phase == Phase::WON)
            std::snprintf(end, sizeof end, "YOU WON  avg:%.0f  time:%.0fs  -  press R",
                          gs.average, gs.totalTime);
        else
            std::snprintf(end, sizeof end, "GAME OVER  -  press R");
        SDL_RenderDebugText(r, Config::WINDOW_W * 0.5f - 120.0f, Config::WINDOW_H * 0.5f, end);
    }
}
