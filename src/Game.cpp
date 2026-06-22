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
#include <algorithm>
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
    destroyAllEntities();
    phys::shutdown();
}

/// @brief True when at least one BrickCleared event exists this frame. Prerequisite
///        state only changes when a brick clears, so the scheduler uses this to skip
///        brickUnlockSystem (a full-world prereq recompute) on the vast majority of
///        frames where nothing was cleared.
static bool anyBrickClearedThisFrame() {
    static const Mask mask = MaskBuilder().set<BrickCleared>().build();
    static int q = World::createQuery(mask);
    World::first(q);   // forces lazy cleanup of stale entries before the eof check
    return !World::eof(q);
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
#ifdef DEBUG_GRADUATION_STAGE
    bindGameState(spawnGameState().entity());
    setupGraduationPreview();
    return;
#endif
    setupFreshPlayScene();
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

enum GuideKind : uint8_t { Section, Header, Bullet, Blank };

struct GuideEntry {
    const char* text;
    GuideKind kind;
};

static constexpr GuideEntry kGuideEntries[] = {
    {"Stage 1 - Course Breakout",                                Section},
    {"Controls",                                                 Header},
    {"Mouse  -  move the paddle",                                Bullet},
    {"Left click  -  launch the ball",                           Bullet},
    {"",                                                         Blank },
    {"Bricks & Drops",                                           Header},
    {"Hit course bricks to fill the meter",                      Bullet},
    {"Catch green assignments to advance the meter",             Bullet},
    {"When a brick clears, catch the falling gold tax",          Bullet},
    {"",                                                         Blank },
    {"Average  (starts at 100)",                                 Header},
    {"Tax caught   ->  average holds",                           Bullet},
    {"Tax missed   ->  -5",                                      Bullet},
    {"Ball dropped (foul)  ->  -10, timer pauses until serve",   Bullet},
    {"",                                                         Blank },
    {"Time",                                                     Header},
    {"5 academic years, up to ~30s each",                        Bullet},
    {"Months progress  Oct  ->  Jul",                            Bullet},
    {"Timer pauses while the ball is out of play",               Bullet},
    {"",                                                         Blank },
    {"Win",                                                      Header},
    {"Clear every brick with average >= 60",                      Bullet},
    {"  -> unlocks Stage 2 (Graduation)",                        Bullet},
    {"Or end with average >= 60 before time runs out",           Bullet},
    {"",                                                         Blank },
    {"Lose",                                                     Header},
    {"0 lives,  or",                                             Bullet},
    {"year 5 ends without finishing,  or",                       Bullet},
    {"average drops below 60",                                   Bullet},
    {"",                                                         Blank },
    {"",                                                         Blank },
    {"Stage 2 - Graduation Ceremony",                            Section},
    {"Goal",                                                     Header},
    {"Reach the stage by jumping between chair rows",            Bullet},
    {"",                                                         Blank },
    {"Controls",                                                 Header},
    {"Mouse        -  move horizontally",                        Bullet},
    {"Left click   -  jump to the next row forward",             Bullet},
    {"Space        -  retry after a foul (if lives remain)",     Bullet},
    {"",                                                         Blank },
    {"Obstacles (red blocks)",                                   Header},
    {"Two blocks slide between rows 0 <-> 1 and 1 <-> 2",        Bullet},
    {"A block pushing you carries you along the row",            Bullet},
    {"Pushed off the row edge while carried  ->  foul",          Bullet},
    {"A block directly above blocks your next jump",             Bullet},
    {"Constant speed; obstacles do not reset after a foul",      Bullet},
    {"",                                                         Blank },
    {"Fouls & Lives",                                            Header},
    {"Each foul costs one life (no average penalty)",            Bullet},
    {"Lives left   ->  press Space to retry from the same row",  Bullet},
    {"0 lives      ->  instant loss",                            Bullet},
    {"",                                                         Blank },
    {"Win / Lose",                                               Header},
    {"Win   -  touch the stage",                                 Bullet},
    {"Lose  -  out of lives, or year 5 timer ends",              Bullet},
    {"",                                                         Blank },
    {"Carry-over from Stage 1",                                  Header},
    {"Lives and average carry into Stage 2",                     Bullet},
    {"In Stage 2 only lives matter for fouls",                   Bullet},
};

static constexpr float kGuideViewportTop    = 140.0f;
static constexpr float kGuideViewportBottom = 750.0f;
static constexpr float kGuideViewportLeft   = 110.0f;
static constexpr float kGuideViewportRight  = static_cast<float>(Config::WINDOW_W) - 130.0f;

static float guideEntryHeight(GuideKind kind) {
    switch (kind) {
        case Section: return 64.0f;
        case Header:  return 46.0f;
        case Bullet:  return 34.0f;
        case Blank:   return 18.0f;
    }
    return 34.0f;
}

static float guideContentHeight() {
    float total = 0.0f;
    for (const auto& e : kGuideEntries) total += guideEntryHeight(e.kind);
    return total;
}

static float guideMaxScroll() {
    const float viewportH = kGuideViewportBottom - kGuideViewportTop;
    return std::max(0.0f, guideContentHeight() - viewportH);
}

static void drawGuideEntry(SDL_Renderer* r, const GuideEntry& e, float y) {
    if (e.kind == Blank) return;
    float scale, rr, gg, bb;
    switch (e.kind) {
        case Section: scale = 2.4f;  rr = 1.00f; gg = 0.78f; bb = 0.32f; break;
        case Header:  scale = 1.85f; rr = 0.55f; gg = 0.85f; bb = 1.00f; break;
        case Bullet:  scale = 1.55f; rr = 0.92f; gg = 0.96f; bb = 1.00f; break;
        default:                  scale = 1.55f; rr = 1.00f; gg = 1.00f; bb = 1.00f; break;
    }
    const float indent = (e.kind == Bullet) ? 36.0f : 0.0f;
    SDL_SetRenderDrawColorFloat(r, rr, gg, bb, 1.0f);
    SDL_SetRenderScale(r, scale, scale);
    SDL_RenderDebugText(r,
        (kGuideViewportLeft + indent) / scale,
        y / scale,
        e.text);
    SDL_SetRenderScale(r, 1.0f, 1.0f);

    if (e.kind == Section) {
        const float underlineY = y + 32.0f;
        SDL_SetRenderDrawColorFloat(r, 1.0f, 0.78f, 0.32f, 0.55f);
        SDL_FRect underline{kGuideViewportLeft, underlineY,
                            kGuideViewportRight - kGuideViewportLeft, 2.0f};
        SDL_RenderFillRect(r, &underline);
    }
}

static void drawGuideScrollbar(SDL_Renderer* r, float scroll, float maxScroll) {
    if (maxScroll <= 0.0f) return;
    const float viewportH = kGuideViewportBottom - kGuideViewportTop;
    const float contentH  = guideContentHeight();
    constexpr float trackW = 10.0f;
    const float trackX = kGuideViewportRight + 18.0f;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(r, 0.18f, 0.22f, 0.32f, 0.70f);
    SDL_FRect track{trackX, kGuideViewportTop, trackW, viewportH};
    SDL_RenderFillRect(r, &track);

    const float thumbH = std::max(48.0f, viewportH * (viewportH / contentH));
    const float thumbY = kGuideViewportTop + (viewportH - thumbH) * (scroll / maxScroll);
    SDL_SetRenderDrawColorFloat(r, 0.55f, 0.72f, 1.0f, 0.95f);
    SDL_FRect thumb{trackX, thumbY, trackW, thumbH};
    SDL_RenderFillRect(r, &thumb);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void drawGuide(SDL_Renderer* r, float scroll) {
    drawOverlayPanel(r);
    drawCenteredText(r, "How to Play", 55.0f, 3.0f, 1.0f, 0.95f, 0.35f);

    const float maxScroll = guideMaxScroll();
    const float clamped   = std::clamp(scroll, 0.0f, maxScroll);

    // No SDL clip rect here: SDL3 stores clip in logical coords, so per-line
    // SetRenderScale shifts the clip horizontally and chops the first chars.
    // Skip lines that don't fully fit; partial lines simply don't draw.
    float cursorY = kGuideViewportTop - clamped;
    for (const auto& e : kGuideEntries) {
        const float h = guideEntryHeight(e.kind);
        if (cursorY >= kGuideViewportTop && cursorY + h <= kGuideViewportBottom)
            drawGuideEntry(r, e, cursorY);
        cursorY += h;
    }

    drawGuideScrollbar(r, clamped, maxScroll);

    drawCenteredText(r, "scroll to read more", 770.0f, 1.15f,
                     0.72f, 0.78f, 0.92f);
    drawButton(r, centeredButton(800.0f, "Back"));
}

static void drawLevelSelect(SDL_Renderer* r) {
    drawOverlayPanel(r);
    drawCenteredText(r, "Choose Starting Level", 105.0f, 3.0f, 1.0f, 0.95f, 0.35f);
    drawCenteredText(r, "Pick where the game should begin.", 185.0f, 1.35f, 0.86f, 0.92f, 1.0f);
    drawButton(r, centeredButton(320.0f, "Stage 1 - Courses"));
    drawButton(r, centeredButton(400.0f, "Stage 2 - Graduation"));
    drawButton(r, centeredButton(510.0f, "Back"));
}

static void drawMenu(SDL_Renderer* r, bool showGuide, bool showLevelSelect, float guideScroll) {
    if (showGuide) {
        drawGuide(r, guideScroll);
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

static float endScreenPlayAgainY(LoseReason loseReason) {
    switch (loseReason) {
    case LoseReason::Stage1AvgTooLow: return 380.0f;
    default: return 340.0f;
    }
}

static float endScreenExitY(LoseReason loseReason) {
    switch (loseReason) {
    case LoseReason::Stage1AvgTooLow: return 460.0f;
    default: return 420.0f;
    }
}

static void drawEndScreen(SDL_Renderer* r, Phase phase, float average, LoseReason loseReason) {
    drawOverlayPanel(r);
    if (phase == Phase::WON) {
        char msg[96];
        std::snprintf(msg, sizeof msg, "You Won! Average: %.0f", average);
        drawCenteredText(r, msg, 170.0f, 3.0f, 0.35f, 1.0f, 0.40f);
    } else {
        drawCenteredText(r, "Game Over", 130.0f, 3.2f, 1.0f, 0.30f, 0.25f);
        switch (loseReason) {
        case LoseReason::Stage1AvgTooLow: {
            char reason[96];
            std::snprintf(reason, sizeof reason,
                "Average below 60 (yours: %.0f). Need 60 to graduate.", average);
            drawCenteredText(r, reason, 205.0f, 1.45f, 1.0f, 0.85f, 0.35f);
            drawCenteredText(r, "Stage 2 not unlocked.", 255.0f, 1.7f, 1.0f, 0.45f, 0.35f);
            break;
        }
        case LoseReason::AverageTooLow:
            drawCenteredText(r, "Average dropped below 60.", 205.0f, 1.6f, 1.0f, 0.85f, 0.35f);
            break;
        case LoseReason::YearsExhausted:
            drawCenteredText(r, "Year 5 ended before you finished.", 205.0f, 1.5f, 1.0f, 0.85f, 0.35f);
            break;
        case LoseReason::NoLives:
            drawCenteredText(r, "Out of lives.", 205.0f, 1.6f, 1.0f, 0.85f, 0.35f);
            break;
        default:
            break;
        }
    }
    drawButton(r, centeredButton(endScreenPlayAgainY(loseReason), "Play Again"));
    drawButton(r, centeredButton(endScreenExitY(loseReason), "Exit"));
}

void SurviveGame::onKeyDown(int sc) {
    GameState& gs = gameState();

    if (gs.paused) {
        if (sc == SDL_SCANCODE_ESCAPE || sc == SDL_SCANCODE_P)
            gs.paused = false;
        return;
    }

    switch (sc) {
        case SDL_SCANCODE_ESCAPE:
        case SDL_SCANCODE_P: {
            if (hudPauseButtonAvailable())
                gs.paused = true;
            break;
        }
        // Debug-hotkey synthesizer: lets each vertical be exercised solo.
        case SDL_SCANCODE_H: ev::courseHit(0, {-1});                 break;
        case SDL_SCANCODE_J: ev::dropCaught(0, 0, DropType::Assignment, {-1}); break;
        case SDL_SCANCODE_E: ev::examStarted(0);                     break;
        case SDL_SCANCODE_L: ev::lifeLost(1);                        break;
        case SDL_SCANCODE_S:
            if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM)
                ev::toggleSlowBallCheat();
            break;
        default: break;
    }
}

void SurviveGame::onScroll(float dy) {
    if (gameState().phase != Phase::MENU || !_showGuide) return;
    constexpr float kScrollStep = 40.0f;
    _guideScroll = std::clamp(_guideScroll - dy * kScrollStep, 0.0f, guideMaxScroll());
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
    if (gs.yearAnnounceTimer > 0.0f) return;

    if (hudPauseButtonHit(px, py)) {
        gs.paused = !gs.paused;
        return;
    }

    if (gs.phase == Phase::WON || gs.phase == Phase::LOST) {
        const LoseReason reason = gs.loseReason;
        if (inside(centeredButton(endScreenPlayAgainY(reason), "Play Again"), px, py)) {
            playAgainRestart();
            startStageOne();
            _showGuide = false;
            _showLevelSelect = false;
            _guideScroll = 0.0f;
        } else if (inside(centeredButton(endScreenExitY(reason), "Exit"), px, py)) {
            _wantsQuit = true;
        }
        return;
    }

    if (gs.paused) return;

    if (gs.phase == Phase::MENU) {
        if (_showGuide) {
            if (inside(centeredButton(800.0f, "Back"), px, py)) {
                _showGuide = false;
                _guideScroll = 0.0f;
            }
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
            _guideScroll = 0.0f;
        } else if (inside(centeredButton(490.0f, "Choose Level"), px, py)) {
            _showLevelSelect = true;
        }
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
    if (gs.paused && !hudPauseButtonAvailable())
        gs.paused = false;

    if (gs.yearAnnounceTimer > 0.0f) {
        yearSystem(dt);
        eventCleanupSystem();
        deadCleanupSystem();
        return;
    }

    if (gs.phase == Phase::PLAYING || gs.phase == Phase::EXAM) {
        inputSystem(dt, _renderer);
        physicsStepSystem(dt);
        contactEventSystem();
        brickMeterSystem();
        brickClearDelaySystem(dt);
        courseHitSystem();
        dropSystem(dt);
        if (anyBrickClearedThisFrame())
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
        drawMenu(_renderer, _showGuide, _showLevelSelect, _guideScroll);
    else if (gs.phase == Phase::WON || gs.phase == Phase::LOST)
        drawEndScreen(_renderer, gs.phase, gs.average, gs.loseReason);
    SDL_RenderPresent(_renderer);
}
