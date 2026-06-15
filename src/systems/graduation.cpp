// Graduation stage (stage 2): same ECS/render pattern as stage 1.
#include "systems/systems.h"
#include "Components.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Game.h"
#include "Physics.h"
#include "Sprites.h"
#include "Events.h"

#include <SDL3/SDL.h>

#include <cmath>

using bagel::Entity;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;
using bagel::ent_type;

namespace {

template<typename Tag>
static void markTagDead() {
    static const Mask mask = MaskBuilder().set<Tag>().build();
    static int q = World::createQuery(mask);
    if (World::eof(q)) return;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        e.add(DeadTag{});
}

static bool gradStudent(Entity& out) {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradStudentTag>() || !e.has<Position>()) continue;
        out = e;
        return true;
    }
    return false;
}

template<typename Fn>
static void forEachGradObstacle(Fn&& fn) {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradObstacleTag>() || !e.has<GradObstacleInfo>() || !e.has<Position>()) continue;
        fn(e);
    }
}

template<typename Fn>
static void forEachGradObstacleWithSize(Fn&& fn) {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradObstacleTag>() || !e.has<GradObstacleInfo>()
            || !e.has<Position>() || !e.has<Size>()) continue;
        fn(e);
    }
}

static bool verticalOverlap(float ay, float ah, float by, float bh) {
    return ay + ah * 0.5f > by - bh * 0.5f
        && ay - ah * 0.5f < by + bh * 0.5f;
}

static bool boxesOverlap(float ax, float ay, float aw, float ah,
                         float bx, float by, float bw, float bh) {
    return verticalOverlap(ay, ah, by, bh)
        && ax + aw * 0.5f > bx - bw * 0.5f
        && ax - aw * 0.5f < bx + bw * 0.5f;
}

static bool obstaclePushContact(float sx, float sy, float sw, float sh,
                                float ox, float oy, float ow, float oh, float dir) {
    if (!verticalOverlap(sy, sh, oy, oh)) return false;

    constexpr float pad = 0.08f;
    if (dir > 0.0f) {
        const float blockRight = ox + ow * 0.5f;
        const float studentLeft = sx - sw * 0.5f;
        const float gap = studentLeft - blockRight;
        return gap <= pad && gap >= -pad;
    }
    const float blockLeft = ox - ow * 0.5f;
    const float studentRight = sx + sw * 0.5f;
    const float gap = blockLeft - studentRight;
    return gap <= pad && gap >= -pad;
}

static void pushStudentOutsideObstacle(GameState& gs, float sy, float sw, float sh,
                                       float ox, float oy, float ow, float oh) {
    if (!verticalOverlap(sy, sh, oy, oh)) return;
    if (!boxesOverlap(gs.gradStudentX, sy, sw, sh, ox, oy, ow, oh)) return;

    const float leftPos = ox - (ow + sw) * 0.5f - 0.03f;
    const float rightPos = ox + (ow + sw) * 0.5f + 0.03f;
    const float distLeft = gs.gradStudentX > leftPos
        ? gs.gradStudentX - leftPos : leftPos - gs.gradStudentX;
    const float distRight = gs.gradStudentX > rightPos
        ? gs.gradStudentX - rightPos : rightPos - gs.gradStudentX;
    gs.gradStudentX = distLeft <= distRight ? leftPos : rightPos;
}

static bool horizontalOverlap(float ax, float aw, float bx, float bw) {
    return ax + aw * 0.5f > bx - bw * 0.5f
        && ax - aw * 0.5f < bx + bw * 0.5f;
}

static float clampStudentXAgainstObstacles(float x, float sy, float sw, float sh, int gradNextChair) {
    const float halfW = sw * 0.5f;
    x = Config::clampGradStudentX(x, halfW);
    if (gradNextChair <= 0) return x;

    const int rowGap = gradNextChair - 1;
    forEachGradObstacleWithSize([&](Entity e) {
        const auto& info = e.get<GradObstacleInfo>();
        if (info.rowGap != rowGap) return;

        const auto& op = e.get<Position>();
        const auto& os = e.get<Size>();
        if (!verticalOverlap(sy, sh, op.y, os.h)) return;
        if (!horizontalOverlap(x, sw, op.x, os.w)) return;

        const float leftPos = op.x - (os.w + sw) * 0.5f - 0.03f;
        const float rightPos = op.x + (os.w + sw) * 0.5f + 0.03f;
        const float distLeft = x > leftPos ? x - leftPos : leftPos - x;
        const float distRight = x > rightPos ? x - rightPos : rightPos - x;
        x = distLeft <= distRight ? leftPos : rightPos;
    });
    return Config::clampGradStudentX(x, halfW);
}

static float landedStudentY(int chairIndex) {
    float cx = 0.0f, cy = 0.0f;
    Config::graduationChairPos(chairIndex, cx, cy);
    return cy - Config::GRAD_CHAIR_H * 0.5f - Config::GRAD_STUDENT_GAP
        - Config::GRAD_STUDENT_H * 0.5f;
}

static float idleStudentY(int gradNextChair, float studentX) {
    if (gradNextChair <= 0)
        return Config::graduationStudentStartY();

    const int standingRow = gradNextChair - 1;
    const int chair = Config::graduationNearestChairInRow(standingRow, studentX);
    return landedStudentY(chair);
}

static void resolveStudentAgainstObstacles(float sy, float sw, float sh, int gradNextChair) {
    GameState& gs = gameState();
    gs.gradStudentX = clampStudentXAgainstObstacles(
        gs.gradStudentX, sy, sw, sh, gradNextChair);
}

static float vaultArcHeight(int gradNextChair) {
    return gradNextChair <= 0 ? 1.15f : 0.85f;
}

static float animEase(float t) {
    return t * t * (3.0f - 2.0f * t);
}

static void resetObstacleContactFlags() {
    forEachGradObstacle([&](Entity e) {
        e.get<GradObstacleInfo>().contactFouled = false;
    });
}

static void spawnGraduationObstacles() {
    float rowLeft = 0.0f, rowRight = 0.0f;
    Config::graduationChairRowSpan(rowLeft, rowRight);
    const float obHalf = Config::GRAD_OBSTACLE_W * 0.5f;
    spawnGradObstacle(rowLeft + obHalf + 0.25f, 0, 1.0f);
    spawnGradObstacle(rowRight - obHalf - 0.25f, 1, -1.0f);
}

static int countGradChairEntities() {
    int count = 0;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>()) continue;
        ++count;
    }
    return count;
}

static int countGradObstacleEntities() {
    int count = 0;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradObstacleTag>()) continue;
        ++count;
    }
    return count;
}

static void markGradChairIndices(bool (&haveChair)[Config::GRAD_CHAIR_ROWS * Config::GRAD_CHAIR_COLS]) {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>() || !e.has<GradChairInfo>()) continue;
        const int index = e.get<GradChairInfo>().index;
        if (index >= 0 && index < Config::graduationChairTotal())
            haveChair[index] = true;
    }
}

static bool allGradChairSlotsFilled() {
    bool haveChair[Config::GRAD_CHAIR_ROWS * Config::GRAD_CHAIR_COLS]{};
    markGradChairIndices(haveChair);
    for (int i = 0; i < Config::graduationChairTotal(); ++i)
        if (!haveChair[i]) return false;
    return true;
}

static void spawnMissingGradChairs() {
    const int need = Config::graduationChairTotal();
    if (allGradChairSlotsFilled() && countGradChairEntities() == need)
        return;

    bagel::Bag<ent_type, 32> toDestroy;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>()) continue;
        toDestroy.push(e.entity());
    }
    for (int i = 0; i < toDestroy.size(); ++i)
        Entity(toDestroy[i]).destroy();

    for (int i = 0; i < need; ++i) {
        float x = 0.0f, y = 0.0f;
        Config::graduationChairPos(i, x, y);
        spawnGradChair(i, x, y);
    }
}

static void resetGraduationProgress();

static void unhideAllGradChairs() {
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>() || !e.has<GradChairInfo>()) continue;
        e.get<GradChairInfo>().hidden = false;
    }
}

static void clearGraduationAnim(GameState& gs) {
    gs.gradAnimStep = 0;
    gs.gradAnimTimer = 0.0f;
    gs.gradActiveChair = -1;
    gs.gradBeingDragged = false;
}

static void endGraduationLost(GameState& gs) {
    unhideAllGradChairs();
    clearGraduationAnim(gs);
    gs.gradAwaitingSpace = false;
    gs.phase = Phase::LOST;
    gs.started = false;
}

static void onGraduationFoul() {
    GameState& gs = gameState();
    if (gs.gradAwaitingSpace) return;

    ++gs.gradFouls;
    gs.lives -= 1;
    gs.gradBeingDragged = false;

    if (gs.lives <= 0) {
        graduationOnLivesDepleted();
        return;
    }

    gs.gradAwaitingSpace = true;
    gs.started = false;
    resetGraduationProgress();
}

static void resetGraduationProgress() {
    GameState& gs = gameState();

    unhideAllGradChairs();

    resetObstacleContactFlags();

    clearGraduationAnim(gs);
    gs.gradNextChair = 0;
    gs.gradStudentX = Config::clampGradStudentX(
        Config::WORLD_W * 0.5f, Config::GRAD_STUDENT_W * 0.5f);
}

static void applyObstacleDrag(float dt) {
    GameState& gs = gameState();
    gs.gradBeingDragged = false;
    if (gs.gradAnimStep != 0 || gs.gradAwaitingSpace) return;
    if (gs.gradNextChair <= 0) return;

    const int rowGap = gs.gradNextChair - 1;
    const float sy = idleStudentY(gs.gradNextChair, gs.gradStudentX);
    const float sw = Config::GRAD_STUDENT_W;
    const float sh = Config::GRAD_STUDENT_H;
    const float halfW = sw * 0.5f;

    float rowLeft = 0.0f, rowRight = 0.0f;
    Config::graduationChairRowSpan(rowLeft, rowRight);
    const float minX = rowLeft + halfW;
    const float maxX = rowRight - halfW;
    constexpr float edgeEps = 0.04f;

    bool fouled = false;
    forEachGradObstacleWithSize([&](Entity e) {
        if (fouled) return;
        auto& info = e.get<GradObstacleInfo>();
        if (info.rowGap != rowGap) return;

        const auto& op = e.get<Position>();
        const auto& os = e.get<Size>();

        if (!verticalOverlap(sy, sh, op.y, os.h)) {
            info.contactFouled = false;
            return;
        }

        pushStudentOutsideObstacle(gs, sy, sw, sh, op.x, op.y, os.w, os.h);

        if (!obstaclePushContact(gs.gradStudentX, sy, sw, sh,
                                 op.x, op.y, os.w, os.h, info.dir)) {
            info.contactFouled = false;
            return;
        }

        gs.gradBeingDragged = true;

        if (info.dir > 0.0f)
            gs.gradStudentX = std::max(gs.gradStudentX, op.x + (os.w + sw) * 0.5f + 0.03f);
        else
            gs.gradStudentX = std::min(gs.gradStudentX, op.x - (os.w + sw) * 0.5f - 0.03f);

        gs.gradStudentX += info.dir * Config::GRAD_OBSTACLE_SPEED * dt;
        gs.gradStudentX = Config::clampGradStudentX(gs.gradStudentX, halfW);

        if (gs.gradStudentX <= minX + edgeEps || gs.gradStudentX >= maxX - edgeEps) {
            if (!info.contactFouled) {
                info.contactFouled = true;
                onGraduationFoul();
                fouled = true;
            }
        }
    });
    if (fouled) return;

    resolveStudentAgainstObstacles(sy, sw, sh, gs.gradNextChair);
}

static void updateObstacles(float dt) {
    float left = 0.0f, right = 0.0f;
    Config::graduationChairRowSpan(left, right);
    const float halfW = Config::GRAD_OBSTACLE_W * 0.5f;
    const float minX = left + halfW;
    const float maxX = right - halfW;

    forEachGradObstacle([&](Entity e) {
        auto& info = e.get<GradObstacleInfo>();
        auto& pos = e.get<Position>();

        pos.x += info.dir * Config::GRAD_OBSTACLE_SPEED * dt;
        if (pos.x <= minX) {
            pos.x = minX;
            info.dir = 1.0f;
        } else if (pos.x >= maxX) {
            pos.x = maxX;
            info.dir = -1.0f;
        }
        pos.y = Config::graduationObstacleY(info.rowGap);
    });
}

static void syncChairVisibility() {
    GameState& gs = gameState();
    const int hideChair = (gs.gradAnimStep == 1) ? gs.gradActiveChair : -1;
    for (Entity e = Entity::first(); !e.eof(); e.next()) {
        if (e.mask().ctz() < 0) continue;
        if (!e.has<GradChairTag>() || !e.has<GradChairInfo>()) continue;
        auto& info = e.get<GradChairInfo>();
        info.hidden = (hideChair >= 0 && info.index == hideChair);
    }
}

static bool studentTouchesStage() {
    Entity student{ent_type{-1}};
    if (!gradStudent(student)) return false;

    const auto& p = student.get<Position>();
    const auto& s = student.get<Size>();
    const float top = p.y - s.h * 0.5f;
    if (top > Config::graduationStageBottomY()) return false;

    const float halfW = s.w * 0.5f;
    return p.x + halfW > 0.0f && p.x - halfW < Config::WORLD_W;
}

static void finishGraduation() {
    GameState& gs = gameState();
    gs.phase = Phase::WON;
    gs.started = false;
}

static bool syncStudentVisual() {
    GameState& gs = gameState();
    Entity student{ent_type{-1}};
    if (!gradStudent(student)) return false;

    const int pathRows = Config::graduationPathRows();

    float sx = gs.gradStudentX;
    float sy = 0.0f;
    sprites::Id sprite = sprites::Id::GRAD_STUDENT_IDLE;
    float w = Config::GRAD_STUDENT_W;
    float h = Config::GRAD_STUDENT_H;

    if (gs.gradAnimStep == 1 && gs.gradActiveChair >= 0) {
        float cx = 0.0f, cy = 0.0f;
        Config::graduationChairPos(gs.gradActiveChair, cx, cy);
        float t = gs.gradAnimTimer / Config::GRAD_VAULT_DURATION;
        if (t > 1.0f) t = 1.0f;
        t = animEase(t);

        sx = gs.gradJumpStartX + (cx - gs.gradJumpStartX) * t;
        const float baseY = gs.gradJumpStartY + (cy - gs.gradJumpStartY) * t;
        constexpr float kPi = 3.14159265f;
        sy = baseY - std::sin(t * kPi) * vaultArcHeight(gs.gradNextChair);
        sprite = sprites::Id::GRAD_STUDENT_VAULT;
        w = Config::GRAD_VAULT_W;
        h = Config::GRAD_VAULT_H;
    } else if (gs.gradAnimStep == 2 && gs.gradActiveChair >= 0) {
        float cx = 0.0f, cy = 0.0f;
        Config::graduationChairPos(gs.gradActiveChair, cx, cy);
        float t = gs.gradAnimTimer / Config::GRAD_LAND_DURATION;
        if (t > 1.0f) t = 1.0f;
        t = animEase(t);

        sx = cx;
        const float landY = landedStudentY(gs.gradActiveChair);
        sy = cy + (landY - cy) * t;
        sprite = sprites::Id::GRAD_STUDENT_VAULT;
        w = Config::GRAD_VAULT_W;
        h = Config::GRAD_VAULT_H;
    } else if (gs.gradNextChair >= pathRows) {
        const int lastChair = Config::graduationNearestChairInRow(
            pathRows - 1, gs.gradStudentX);
        float cx = 0.0f, cy = 0.0f;
        Config::graduationChairPos(lastChair, cx, cy);
        sx = cx;
        sy = landedStudentY(lastChair);
    } else {
        sy = idleStudentY(gs.gradNextChair, gs.gradStudentX);
    }

    student.get<Position>() = {sx, sy};
    student.get<Size>() = {w, h};
    student.get<SpritePart>() = sprites::makePart(sprite);
    if (student.has<PhysicsBody>())
        phys::setPosition(student.entity(), sx, sy);

    return studentTouchesStage();
}

static void commitGraduationFrame() {
    syncChairVisibility();
    if (syncStudentVisual())
        finishGraduation();
}

static void onGraduationYear5Expired() {
    GameState& gs = gameState();
    if (gs.phase != Phase::GRADUATION) return;
    gs.yearsExhausted = true;
    endGraduationLost(gs);
}

} // namespace

void graduationOnYear5Expired() {
    onGraduationYear5Expired();
}

void graduationOnLivesDepleted() {
    GameState& gs = gameState();
    if (gs.phase != Phase::GRADUATION) return;
    endGraduationLost(gs);
}

void enterGraduationStage() {
    GameState& gs = gameState();
    deadCleanupSystem();

    if (!gs.gradInitialized) {
        markTagDead<BallTag>();
        markTagDead<BrickTag>();
        markTagDead<DropTag>();
        markTagDead<ProjectileTag>();
        deadCleanupSystem();

        for (Entity e = Entity::first(); !e.eof(); e.next()) {
            if (e.mask().ctz() < 0) continue;
            if (!e.has<PaddleTag>() || !e.has<Position>() || !e.has<SpritePart>()) continue;
            e.add(GradStudentTag{});
            e.get<SpritePart>() = sprites::makePart(sprites::Id::GRAD_STUDENT_IDLE);
            e.get<Size>() = {Config::GRAD_STUDENT_W, Config::GRAD_STUDENT_H};
            e.get<Position>() = {Config::WORLD_W * 0.5f, Config::graduationStudentStartY()};
            if (e.has<PhysicsBody>()) {
                phys::setPosition(e.entity(), Config::WORLD_W * 0.5f, Config::graduationStudentStartY());
                phys::setVelocity(e.entity(), 0.0f, 0.0f);
            }
            break;
        }

        gs.gradFouls = 0;
        gs.gradBeingDragged = false;
        gs.gradAwaitingSpace = false;
        gs.yearsExhausted = false;
        resetGraduationProgress();
    }

    gs.gradChairTotal = Config::graduationChairTotal();
    spawnMissingGradChairs();
    unhideAllGradChairs();

    if (countGradObstacleEntities() < Config::graduationObstacleRowGapCount())
        spawnGraduationObstacles();

    gs.gradInitialized = true;
    gs.started = true;
    commitGraduationFrame();
}

void graduationInputSystem(SDL_Renderer* r) {
    GameState& gs = gameState();
    if (gs.phase != Phase::GRADUATION || !gs.gradInitialized) return;
    if (gs.gradAwaitingSpace || gs.gradAnimStep != 0 || gs.gradBeingDragged) return;

    float globalMouseX = 0.0f, globalMouseY = 0.0f;
    SDL_GetGlobalMouseState(&globalMouseX, &globalMouseY);
    (void)globalMouseY;

    int windowX = 0, windowY = 0;
    SDL_Window* window = SDL_GetRenderWindow(r);
    if (window) SDL_GetWindowPosition(window, &windowX, &windowY);

    const float mouseWorldX = (globalMouseX - static_cast<float>(windowX)) / Config::PPM;
    const float sy = idleStudentY(gs.gradNextChair, gs.gradStudentX);
    gs.gradStudentX = Config::clampGradStudentX(
        mouseWorldX, Config::GRAD_STUDENT_W * 0.5f);
    resolveStudentAgainstObstacles(
        sy, Config::GRAD_STUDENT_W, Config::GRAD_STUDENT_H, gs.gradNextChair);
}

void graduationOnMouseDown(SDL_Renderer* r) {
    GameState& gs = gameState();
    if (gs.phase != Phase::GRADUATION || !gs.gradInitialized) return;
    if (gs.gradAwaitingSpace || gs.gradAnimStep != 0) return;
    if (gs.gradNextChair >= Config::graduationPathRows()) return;

    float globalMouseX = 0.0f, globalMouseY = 0.0f;
    SDL_GetGlobalMouseState(&globalMouseX, &globalMouseY);
    (void)globalMouseY;

    int windowX = 0, windowY = 0;
    SDL_Window* window = SDL_GetRenderWindow(r);
    if (window) SDL_GetWindowPosition(window, &windowX, &windowY);

    const float mouseWorldX = (globalMouseX - static_cast<float>(windowX)) / Config::PPM;
    const float halfW = Config::GRAD_STUDENT_W * 0.5f;
    const float sx = Config::clampGradStudentX(mouseWorldX, halfW);

    const int targetChair = Config::graduationNearestChairInRow(
        gs.gradNextChair, sx);

    gs.gradJumpStartX = sx;
    gs.gradJumpStartY = idleStudentY(gs.gradNextChair, sx);
    gs.gradStudentX = sx;
    gs.gradBeingDragged = false;

    gs.gradActiveChair = targetChair;
    gs.gradAnimStep = 1;
    gs.gradAnimTimer = 0.0f;
    commitGraduationFrame();
}

void graduationOnSpace() {
    GameState& gs = gameState();
    if (gs.phase != Phase::GRADUATION || !gs.gradAwaitingSpace || gs.lives <= 0) return;

    gs.gradAwaitingSpace = false;
    gs.started = true;
    resetGraduationProgress();
    commitGraduationFrame();
}

void graduationSystem(float dt) {
    GameState& gs = gameState();
    if (gs.phase != Phase::GRADUATION) return;

    if (!gs.gradInitialized || !allGradChairSlotsFilled())
        enterGraduationStage();

    if (gs.gradAwaitingSpace) {
        commitGraduationFrame();
        return;
    }

    updateObstacles(dt);
    applyObstacleDrag(dt);
    commitGraduationFrame();
    if (gs.phase != Phase::GRADUATION) return;

    if (gs.gradAnimStep == 0) return;

    gs.gradAnimTimer += dt;
    if (gs.gradAnimStep == 1) {
        if (gs.gradAnimTimer < Config::GRAD_VAULT_DURATION) return;
        gs.gradAnimStep = 2;
        gs.gradAnimTimer = 0.0f;
        commitGraduationFrame();
        return;
    }

    if (gs.gradAnimTimer < Config::GRAD_LAND_DURATION) return;

    {
        float cx = 0.0f, cy = 0.0f;
        Config::graduationChairPos(gs.gradActiveChair, cx, cy);
        gs.gradStudentX = Config::clampGradStudentX(cx, Config::GRAD_STUDENT_W * 0.5f);
    }

    ++gs.gradNextChair;
    gs.gradAnimStep = 0;
    gs.gradAnimTimer = 0.0f;
    gs.gradActiveChair = -1;

    commitGraduationFrame();
}
