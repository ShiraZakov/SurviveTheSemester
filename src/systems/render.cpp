
#include "systems/systems.h"
#include "Components.h"
#include "Game.h"
#include "Config.h"
#include "Sprites.h"
#include <SDL3/SDL.h>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static SDL_FRect spriteDest(Entity e, float worldX, float drawY, float worldW, float worldH,
                            const SpritePart& sp) {
    const float boxW = worldW * Config::PPM;
    const float boxH = worldH * Config::PPM;
    const float cx = worldX * Config::PPM;
    const float aspect = sp.part.h / sp.part.w;

    float drawW = boxW;
    float drawH = drawW * aspect;

    if (e.has<PaddleTag>() && !e.has<GradStudentTag>()) {
        const float bottom = (drawY + worldH * 0.5f) * Config::PPM;
        return {cx - drawW * 0.5f, bottom - drawH, drawW, drawH};
    }

    if (drawH > boxH) {
        drawH = boxH;
        drawW = drawH / aspect;
    }
    const float top = (drawY - worldH * 0.5f) * Config::PPM + (boxH - drawH) * 0.5f;
    return {cx - drawW * 0.5f, top, drawW, drawH};
}

static void fillCircle(SDL_Renderer* r, float cx, float cy, float rad) {
    for (int dy = -(int)rad; dy <= (int)rad; ++dy) {
        float dx = SDL_sqrtf(rad * rad - (float)dy * (float)dy);
        SDL_FRect line{cx - dx, cy + (float)dy, 2.0f * dx, 1.0f};
        SDL_RenderFillRect(r, &line);
    }
}

static void drawTaxDrop(SDL_Renderer* r, float cx, float cy, float half) {
    const float outer = half * 1.12f;
    SDL_FRect glow{cx - outer, cy - outer, outer * 2.0f, outer * 2.0f};
    SDL_SetRenderDrawColorFloat(r, 1.0f, 0.95f, 0.25f, 0.35f);
    SDL_RenderFillRect(r, &glow);

    SDL_FRect body{cx - half, cy - half, half * 2.0f, half * 2.0f};
    SDL_SetRenderDrawColorFloat(r, 1.0f, 0.78f, 0.05f, 1.0f);
    SDL_RenderFillRect(r, &body);
    SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 1.0f, 1.0f);
    SDL_RenderRect(r, &body);

    const float inset = half * 0.28f;
    SDL_FRect core{body.x + inset, body.y + inset, body.w - inset * 2.0f, body.h - inset * 2.0f};
    SDL_SetRenderDrawColorFloat(r, 1.0f, 0.92f, 0.35f, 1.0f);
    SDL_RenderFillRect(r, &core);

    const float barW = half * 0.55f;
    const float barH = half * 0.16f;
    SDL_FRect bar{cx - barW * 0.5f, cy - barH * 0.5f, barW, barH};
    SDL_SetRenderDrawColorFloat(r, 0.15f, 0.08f, 0.0f, 1.0f);
    SDL_RenderFillRect(r, &bar);
}

static void drawEntity(SDL_Renderer* r, Entity e) {
    if (e.has<GradChairTag>() && e.has<GradChairInfo>()) {
        if (e.get<GradChairInfo>().hidden) return;
    }

    const auto& p = e.get<Position>();
    const auto& s = e.get<Size>();
    const auto& d = e.get<Drawable>();
    float drawY = p.y;
    if (e.has<PaddleImpact>())
        drawY += 0.14f * (e.get<PaddleImpact>().time / 0.10f);

    if (e.has<DropTag>() && e.has<DropInfo>() && e.get<DropInfo>().type == DropType::Tax) {
        const float half = (s.w * 0.5f) * Config::PPM;
        drawTaxDrop(r, p.x * Config::PPM, drawY * Config::PPM, half);
        return;
    }

    if (e.has<SpritePart>()) {
        if (sprites::ready()) {
            const SpritePart sp = e.get<SpritePart>();
            const SDL_FRect dest = spriteDest(e, p.x, drawY, s.w, s.h, sp);
            sprites::drawPart(r, sp, dest);
            if (e.has<BrickProgress>()) {
                const auto& prog = e.get<BrickProgress>();
                if (!prog.unlocked) return;
                const float mw = s.w * Config::PPM * 0.55f;
                const float mh = prog.max == 5 ? 10.0f : 12.0f;
                const float mx = (p.x * Config::PPM) - mw * 0.5f;
                const float my = (drawY + s.h * 0.5f) * Config::PPM - mh - 4.0f;
                if (prog.max == 5)
                    sprites::drawMeter5(r, prog.filled, prog.max, mx, my, mw, mh);
                else
                    sprites::drawMeter3(r, prog.filled, prog.max, mx, my, mw, mh);
            }
            return;
        }
        // Fall through to Drawable fallback if textures are unavailable.
    }

    SDL_SetRenderDrawColorFloat(r, d.r, d.g, d.b, d.a);
    if (d.shape == Shape::Circle) {
        fillCircle(r, p.x * Config::PPM, drawY * Config::PPM, (s.w * 0.5f) * Config::PPM);
    } else {
        SDL_FRect rect{
            (p.x - s.w * 0.5f) * Config::PPM,
            (drawY - s.h * 0.5f) * Config::PPM,
            s.w * Config::PPM, s.h * Config::PPM};
        SDL_RenderFillRect(r, &rect);
    }
}

void renderSystem(SDL_Renderer* r) {
    GameState& gs = gameState();
    if (sprites::ready()
        && gs.gradInitialized
        && (gs.phase == Phase::GRADUATION || gs.phase == Phase::WON || gs.phase == Phase::LOST)) {
        sprites::drawGraduationStage(r);
    }

    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (e.has<DeadTag>()) continue;
        if (!e.has<Position>() || !e.has<Size>() || !e.has<Drawable>()) continue;
        if (e.has<GradStudentTag>()) continue;
        drawEntity(r, e);
    }

    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (e.has<DeadTag>()) continue;
        if (!e.has<Position>() || !e.has<Size>() || !e.has<Drawable>()) continue;
        if (!e.has<GradStudentTag>()) continue;
        drawEntity(r, e);
    }
}
