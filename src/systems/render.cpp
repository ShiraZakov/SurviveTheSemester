// AVIEL - base renderer. Draws every Drawable from Position + Size (meters -> pixels).
// TODO (AVIEL): sprites, polish, layering.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include <SDL3/SDL.h>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

static void fillCircle(SDL_Renderer* r, float cx, float cy, float rad) {
    for (int dy = -(int)rad; dy <= (int)rad; ++dy) {
        float dx = SDL_sqrtf(rad * rad - (float)dy * (float)dy);
        SDL_FRect line{cx - dx, cy + (float)dy, 2.0f * dx, 1.0f};
        SDL_RenderFillRect(r, &line);
    }
}

void renderSystem(SDL_Renderer* r) {
    static const Mask mask = MaskBuilder()
        .set<Position>()
        .set<Size>()
        .set<Drawable>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        const auto& p = e.get<Position>();
        const auto& s = e.get<Size>();
        const auto& d = e.get<Drawable>();
        SDL_SetRenderDrawColorFloat(r, d.r, d.g, d.b, d.a);
        if (d.shape == Shape::Circle) {
            fillCircle(r, p.x * Config::PPM, p.y * Config::PPM, (s.w * 0.5f) * Config::PPM);
        } else {
            SDL_FRect rect{
                (p.x - s.w * 0.5f) * Config::PPM,
                (p.y - s.h * 0.5f) * Config::PPM,
                s.w * Config::PPM, s.h * Config::PPM};
            SDL_RenderFillRect(r, &rect);
        }
    }
}
