// AVIEL - input. Moves the paddle (kinematic) and clamps it to the play field.
// TODO (AVIEL): launch-on-Space handoff, paddle feel/acceleration.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "Physics.h"
#include <SDL3/SDL.h>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

void inputSystem(const bool* keys, float dt) {
    (void)dt;
    static const Mask mask = MaskBuilder()
        .set<PaddleTag>()
        .set<Position>()
        .set<Size>()
        .set<PhysicsBody>()
        .build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        float vx = 0.0f;
        if (keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A]) vx -= Config::PADDLE_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) vx += Config::PADDLE_SPEED;

        const auto& p = e.get<Position>();
        float half = e.get<Size>().w * 0.5f;
        if (vx < 0.0f && p.x - half <= Config::WALL)                  vx = 0.0f;
        if (vx > 0.0f && p.x + half >= Config::WORLD_W - Config::WALL) vx = 0.0f;

        phys::setVelocity(e.entity(), vx, 0.0f);
    }
}
