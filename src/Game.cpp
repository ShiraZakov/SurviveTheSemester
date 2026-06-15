#include "Game.h"
#include "Components.h"
#include "Events.h"
#include "Config.h"
#include "EntityFactory.h"
#include "Physics.h"
#include "Sprites.h"
#include "systems/systems.h"

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <cstdio>
#include <cstring>

using bagel::Entity;
using bagel::ent_type;
using bagel::Mask;
using bagel::MaskBuilder;
using bagel::World;

bagel::ent_type courseEntity(int id) {
    static const Mask mask = MaskBuilder().set<Course>().build();
    static int q = World::createQuery(mask);

    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        if (e.get<Course>().id == id) return e.entity();
    }
    return {-1};
}

/// @brief Destroys all event entities (EventTag) at end of frame. One-frame lifetime.
/// @return void
void eventCleanupSystem() {
    static const Mask mask = MaskBuilder().set<EventTag>().build();
    static int q = World::createQuery(mask);

    bagel::Bag<ent_type, 64> dead;
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q))
        dead.push(e.entity());
    for (int i = 0; i < dead.size(); ++i) Entity(dead[i]).destroy();
}

static void clearAll() {
    bagel::Bag<ent_type, 256> all;
    for (Entity e = Entity::first(); !e.eof(); e.next())
        if (e.mask().ctz() >= 0) all.push(e.entity());
    for (int i = 0; i < all.size(); ++i) {
        phys::destroyBody(all[i]);
        Entity(all[i]).destroy();
    }
}

#ifdef DEBUG_GRADUATION_STAGE
static void setupGraduationPreview() {
    const float W = Config::WORLD_W, H = Config::WORLD_H, t = Config::WALL;
    spawnWall(W * 0.5f, t * 0.5f,     W, t);
    spawnWall(W * 0.5f, H - t * 0.5f, W, t);
    spawnWall(t * 0.5f, H * 0.5f,     t, H);
    spawnWall(W - t * 0.5f, H * 0.5f, t, H);
    spawnPaddle(W * 0.5f, Config::paddleY());

    GameState& gs = gameState();
    gs.phase = Phase::GRADUATION;
    gs.started = true;
    enterGraduationStage();
}
#endif

void SurviveGame::setupScene() {
    bindGameState(spawnGameState().entity());

#ifdef DEBUG_GRADUATION_STAGE
    setupGraduationPreview();
    return;
#endif

    const float W = Config::WORLD_W, H = Config::WORLD_H, t = Config::WALL;
    spawnWall(W * 0.5f, t * 0.5f,     W, t);   // top
    spawnWall(W * 0.5f, H - t * 0.5f, W, t);   // bottom (closed for the core smoke test)
    spawnWall(t * 0.5f, H * 0.5f,     t, H);   // left
    spawnWall(W - t * 0.5f, H * 0.5f, t, H);   // right

    for (int c = 0; c < Config::COURSES; ++c) spawnCourse(c);

    const float bw = Config::BRICK_W, bh = Config::BRICK_H, gap = Config::BRICK_GAP;
    const float gridW = Config::brickGridWidth();
    const float startX = (W - gridW) * 0.5f + bw * 0.5f;
    const float startY = 3.2f;
    for (int row = 0; row < Config::COURSES; ++row)
        for (int col = 0; col < Config::BRICK_COLS; ++col) {
            float x = startX + col * (bw + gap);
            float y = startY + row * (bh + gap * 0.5f);
            const int courseIndex = Config::gridCourseIndex(row, col);
            spawnBrick(row, courseIndex, x, y);
        }

    const float paddleY = Config::paddleY();
    spawnPaddle(W * 0.5f, paddleY);
    spawnBall(W * 0.5f, paddleY + Config::PADDLE_H * 0.5f - Config::PADDLE_VISUAL_H - Config::BALL_RADIUS + 0.3f);
}

bool SurviveGame::init(SDL_Renderer* renderer) {
    _renderer = renderer;
    if (!sprites::init(renderer)) return false;
    phys::init();
    setupScene();
    return true;
}

void SurviveGame::shutdown() {
    clearAll();
    phys::shutdown();
    sprites::shutdown();
}

static void launchBallAndStart() {
    GameState& gs = gameState();
    static const Mask mask = MaskBuilder().set<BallTag>().build();
    static int q = World::createQuery(mask);
    for (Entity e = World::first(q); !World::eof(q); e = World::next(q)) {
        float vx, vy; phys::getVelocity(e.entity(), vx, vy);
        if (vx * vx + vy * vy < 1.0f) {
            phys::setVelocity(e.entity(), Config::BALL_SPEED * 0.55f, -Config::BALL_SPEED * 0.83f);
            gs.started = true;
        }
    }
}

static void startStageOne() {
    GameState& gs = gameState();
    gs.phase = Phase::PLAYING;
    gs.started = false;
}

static void startStageTwo() {
    GameState& gs = gameState();
    gs.phase = Phase::GRADUATION;
    gs.started = true;
    enterGraduationStage();
}

struct UiButton {
    SDL_FRect rect;
    const char* label;
};

static UiButton centeredButton(float y, const char* label) {
    constexpr float w = 320.0f;
    constexpr float h = 58.0f;
    return {{(Config::WINDOW_W - w) * 0.5f, y, w, h}, label};
}

static bool inside(const UiButton& b, float x, float y) {
    return x >= b.rect.x && x <= b.rect.x + b.rect.w
        && y >= b.rect.y && y <= b.rect.y + b.rect.h;
}

static void drawCenteredText(SDL_Renderer* r, const char* text, float y, float scale,
                             float red, float green, float blue) {
    const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
        * static_cast<float>(std::strlen(text));
    SDL_SetRenderDrawColorFloat(r, red, green, blue, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r,
        (static_cast<float>(Config::WINDOW_W) * 0.5f - textW * 0.5f) / scale,
        y / scale,
        text);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

static void drawButton(SDL_Renderer* r, const UiButton& b) {
    SDL_SetRenderDrawColorFloat(r, 0.10f, 0.18f, 0.35f, 0.94f);
    SDL_RenderFillRect(r, &b.rect);
    SDL_SetRenderDrawColorFloat(r, 0.55f, 0.72f, 1.0f, 1.0f);
    SDL_RenderRect(r, &b.rect);

    const float scale = 1.65f;
    const float textW = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale
        * static_cast<float>(std::strlen(b.label));
    SDL_SetRenderDrawColorFloat(r, 1.0f, 1.0f, 1.0f, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r,
        (b.rect.x + b.rect.w * 0.5f - textW * 0.5f) / scale,
        (b.rect.y + 18.0f) / scale,
        b.label);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

static void drawOverlayPanel(SDL_Renderer* r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(r, 0.02f, 0.03f, 0.06f, 0.86f);
    SDL_FRect bg{0.0f, 0.0f, static_cast<float>(Config::WINDOW_W), static_cast<float>(Config::WINDOW_H)};
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void drawGuide(SDL_Renderer* r) {
    drawOverlayPanel(r);
    drawCenteredText(r, "How to Play", 70.0f, 3.0f, 1.0f, 0.95f, 0.35f);

    const char* lines[] = {
        "Stage 1 - Course Breakout:",
        "Move the paddle with the mouse and click to launch the ball.",
        "Break course bricks, catch assignments, and avoid losing lives.",
        "Clear courses before academic years run out.",
        "",
        "Exam / Graduation Stage:",
        "Use the mouse to move through the graduation challenge.",
        "Click to jump/advance when prompted by the stage.",
        "Avoid obstacles and fouls. Losing all lives ends the game.",
        "",
        "Win by finishing the final stage. Lose when lives reach zero."
    };
    SDL_SetRenderDrawColorFloat(r, 0.92f, 0.96f, 1.0f, 1.0f);
    for (int i = 0; i < 11; ++i)
        SDL_RenderDebugText(r, 190.0f, 150.0f + static_cast<float>(i) * 32.0f, lines[i]);

    drawButton(r, centeredButton(720.0f, "Back"));
}

static void drawLevelSelect(SDL_Renderer* r) {
    drawOverlayPanel(r);
    drawCenteredText(r, "Choose Starting Level", 105.0f, 3.0f, 1.0f, 0.95f, 0.35f);
    drawCenteredText(r, "Pick where the game should begin.", 185.0f, 1.35f, 0.86f, 0.92f, 1.0f);
    drawButton(r, centeredButton(320.0f, "Stage 1 - Courses"));
    drawButton(r, centeredButton(400.0f, "Stage 2 - Graduation"));
    drawButton(r, centeredButton(510.0f, "Back"));
}

static void drawMenu(SDL_Renderer* r, bool showGuide, bool showLevelSelect) {
    if (showGuide) {
        drawGuide(r);
        return;
    }
    if (showLevelSelect) {
        drawLevelSelect(r);
        return;
    }

    drawOverlayPanel(r);
    drawCenteredText(r, "Survive the Semester", 125.0f, 3.0f, 1.0f, 0.95f, 0.35f);
    drawCenteredText(r, "Break courses. Survive exams. Reach graduation.", 205.0f, 1.35f, 0.86f, 0.92f, 1.0f);
    drawButton(r, centeredButton(330.0f, "Start Game"));
    drawButton(r, centeredButton(410.0f, "Game Guide"));
    drawButton(r, centeredButton(490.0f, "Choose Level"));
}

static void drawEndScreen(SDL_Renderer* r, Phase phase, float average) {
    drawOverlayPanel(r);
    if (phase == Phase::WON) {
        char msg[96];
        std::snprintf(msg, sizeof msg, "You Won! Average: %.0f", average);
        drawCenteredText(r, msg, 170.0f, 3.0f, 0.35f, 1.0f, 0.40f);
    } else {
        drawCenteredText(r, "Game Over", 170.0f, 3.2f, 1.0f, 0.30f, 0.25f);
    }
    drawButton(r, centeredButton(340.0f, "Play Again"));
    drawButton(r, centeredButton(420.0f, "Exit"));
}

void SurviveGame::onKeyDown(int sc) {
    GameState& gs = gameState();

    // Handle pause menu keys first
    if (gs.paused) {
        if (sc == SDL_SCANCODE_Y) { _wantsQuit = true; }
        if (sc == SDL_SCANCODE_N) { gs.paused = false; }
        return;
    }

    switch (sc) {
        case SDL_SCANCODE_ESCAPE: {
            if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM
                    || gs.phase == Phase::GRADUATION) {
                gs.paused = true;
            }
            break;
        }
        // Debug-hotkey synthesizer: lets each vertical be exercised solo.
        case SDL_SCANCODE_H: ev::courseHit(0, {-1});                 break;
        case SDL_SCANCODE_J: ev::dropCaught(0, 0, DropType::Assignment, {-1}); break;
        case SDL_SCANCODE_E: ev::examStarted(0);                     break;
        case SDL_SCANCODE_L: ev::lifeLost(1);                        break;
        default: break;
    }
}

bool SurviveGame::isPaused() const {
    return gameState().paused;
}

bool SurviveGame::wantsQuit() const {
    return _wantsQuit;
}

void SurviveGame::onMouseDown(int button, float px, float py) {
    if (button != SDL_BUTTON_LEFT) return;
    GameState& gs = gameState();

    if (gs.phase == Phase::MENU) {
        if (_showGuide) {
            if (inside(centeredButton(720.0f, "Back"), px, py)) _showGuide = false;
            return;
        }
        if (_showLevelSelect) {
            if (inside(centeredButton(320.0f, "Stage 1 - Courses"), px, py)) {
                startStageOne();
                _showLevelSelect = false;
            } else if (inside(centeredButton(400.0f, "Stage 2 - Graduation"), px, py)) {
                startStageTwo();
                _showLevelSelect = false;
            } else if (inside(centeredButton(510.0f, "Back"), px, py)) {
                _showLevelSelect = false;
            }
            return;
        }
        if (inside(centeredButton(330.0f, "Start Game"), px, py)) {
            startStageOne();
        } else if (inside(centeredButton(410.0f, "Game Guide"), px, py)) {
            _showGuide = true;
        } else if (inside(centeredButton(490.0f, "Choose Level"), px, py)) {
            _showLevelSelect = true;
        }
        return;
    }

    if (gs.phase == Phase::WON || gs.phase == Phase::LOST) {
        if (inside(centeredButton(340.0f, "Play Again"), px, py)) {
            clearAll();
            setupScene();
            _showGuide = false;
            _showLevelSelect = false;
        } else if (inside(centeredButton(420.0f, "Exit"), px, py)) {
            _wantsQuit = true;
        }
        return;
    }

    if (gs.paused) {
        // YES button region (centered ~35% from left)
        constexpr float btnY  = Config::WINDOW_H * 0.60f;
        constexpr float yesCX = Config::WINDOW_W * 0.5f - 120.0f;
        constexpr float noCX  = Config::WINDOW_W * 0.5f + 120.0f;
        constexpr float hw = 80.0f, hh = 22.0f;
        if (px >= yesCX - hw && px <= yesCX + hw && py >= btnY - hh && py <= btnY + hh)
            _wantsQuit = true;
        else if (px >= noCX - hw && px <= noCX + hw && py >= btnY - hh && py <= btnY + hh)
            gs.paused = false;
        return;
    }

    if (gs.phase == Phase::PLAYING && !gs.started) {
        launchBallAndStart();
        return;
    }

    if (gs.phase == Phase::GRADUATION && gs.gradAwaitingSpace)
        graduationOnSpace();
    else if (gs.phase == Phase::GRADUATION)
        graduationOnMouseDown(_renderer);
}

void SurviveGame::tick(float dt) {
    GameState& gs = gameState();
    if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM) {
        inputSystem(dt, _renderer);
        physicsStepSystem(dt);
        contactEventSystem();
        brickMeterSystem();
        brickClearDelaySystem(dt);
        courseHitSystem();
        dropSystem(dt);
        brickUnlockSystem();
        courseProgressSystem();
        examSystem(dt);
        yearSystem(dt);
        gameStateSystem();
        eventCleanupSystem();
        deadCleanupSystem();
    } else if (gs.phase == Phase::GRADUATION) {
        if (!gs.gradAwaitingSpace)
            graduationInputSystem(_renderer);
        graduationSystem(dt);
        gameStateSystem();
        yearSystem(dt);
        eventCleanupSystem();
        deadCleanupSystem();
    } else {
        eventCleanupSystem();   // drain stray (e.g. debug) events while paused
    }
}


void SurviveGame::draw() {
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
    SDL_RenderClear(_renderer);
    renderSystem(_renderer);
    hudSystem(_renderer);
    GameState& gs = gameState();
    if (gs.phase == Phase::MENU)
        drawMenu(_renderer, _showGuide, _showLevelSelect);
    else if (gs.phase == Phase::WON || gs.phase == Phase::LOST)
        drawEndScreen(_renderer, gs.phase, gs.average);
    SDL_RenderPresent(_renderer);
}
