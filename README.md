# Survive the Semester

A C++ game project developed as part of an **ECS (Entity Component System) programming course**
at MTA. The game is a Breakout-style arcade game themed around surviving a university semester.
The player controls a paddle to bounce a ball at course bricks, catch assignment drops, survive
exams, and maintain a passing average — all before the academic years run out.

---

## Built With

| Technology | Role |
|------------|------|
| **C++20** | Primary language |
| **BAGEL** (`bagel.h`) | Custom header-only ECS framework |
| **SDL3** | Window, renderer, input |
| **SDL3_image** | Spritesheet loading |
| **Box2D 3.x** | 2D physics (collision, kinematic bodies) |
| **CMake** | Build system |

---

## Gameplay

See [GAME_RULES.md](GAME_RULES.md) for the full rules.

**Short version:** clear all 21 course bricks across 5 academic years. Each brick has a progress
meter filled by ball hits and caught assignment drops. When a course reaches 100%, an exam starts.
Survive the exam, earn a grade, and keep your average above 60 to graduate.
Once all courses are cleared, a Graduation stage begins — navigate a student character across
a chair-filled auditorium to reach the stage.

---

## Project Structure

```
SurviveTheSemester/
│
├── src/
│   ├── main.cpp                  # SDL3 init, window/renderer, fixed-timestep loop
│   ├── Game.h / Game.cpp         # System scheduler, input routing, menu/pause/end screens
│   ├── GameStateAccess.cpp       # The GameState singleton accessor (gameState())
│   ├── GameRestart.cpp           # destroyAllEntities + scene setup + "Play Again"
│   ├── Config.h                  # All tunables: sizes, speeds, timers, thresholds
│   ├── Enums.h                   # Phase, LoseReason, CourseState, DropType, Shape
│   ├── Components.h              # All ECS component and tag structs (incl. GameState)
│   ├── Events.h                  # Event component structs + producer helpers
│   ├── Input.h                   # Shared mouse → world-meters helper
│   ├── Physics.h / Physics.cpp   # Box2D wrapper, ball speed regulation, contact→events
│   ├── EntityFactory.h / .cpp    # spawnX() functions — single source of truth for entities
│   ├── Sprites.h / Sprites.cpp   # Spritesheet rects, prerequisite graph, draw helpers
│   │
│   └── systems/
│       ├── input.cpp             # Paddle movement, ball parking
│       ├── render.cpp            # Drawable iteration, sprite + primitive drawing
│       ├── ball_paddle.cpp       # Bounce physics, spin, ball loss detection
│       ├── brick_progress.cpp    # Brick meter, clear delay, prerequisite unlocking
│       ├── drops.cpp             # Drop removal, TaxMissed penalty
│       ├── progress.cpp          # Course-track progress → ExamStarted trigger
│       ├── exam.cpp              # Exam phase: timer, projectiles, ExamFinished
│       ├── hud.cpp               # Lives, year, elapsed timer, status line, pause overlay
│       ├── endgame.cpp           # Average calculation, win/lose resolution
│       ├── graduation.cpp        # Graduation stage: student navigation, chair vaulting
│       ├── cleanup.cpp           # deadCleanupSystem — end-of-frame entity reaping
│       └── year.cpp              # Academic year timer and advancement
│
├── bagel.h                       # BAGEL ECS framework (do not edit)
├── lib/                          # SDL3, SDL3_image, Box2D libraries (do not edit)
├── res/                          # Game assets (spritesheets, textures)
├── tests/
│   ├── test_brick_prereqs.cpp    # Prerequisite resolution
│   ├── test_graduation_year.cpp  # Graduation + year logic
│   ├── test_play_again_reset.cpp # "Play Again" full reset (no double-free)
│   └── test_year_announce.cpp    # Year-transition announce timer
│
├── CMakeLists.txt
├── GAME_RULES.md                 # Full game rules and mechanics
└── README.md                     # This file
```

---

## Architecture

The project uses a **Data-Oriented Design (DOD)** approach via the BAGEL ECS:

- **Entities** are plain IDs (`ent_type`) with no data or logic.
- **Components** are pure POD structs (e.g., `Position`, `Course`, `GameState`).
- **Systems** are stateless free functions that iterate over component queries.
- **Cross-system communication** uses **event entities** — short-lived entities carrying a single
  event component, created by producers and consumed by other systems within the same frame,
  then deleted by `eventCleanupSystem`.

No virtual functions, no STL containers, no shared/unique pointers. All physics is encapsulated
behind `Physics.h` — other systems never call Box2D directly.

### Two meanings of "course" (read this before the code)

The word "course" is used two ways, which trips up new readers:

- **`courseId` (0–2)** — a colored **row/track** of bricks. There are only `Config::COURSES = 3`
  of these. Each has one `Course` component whose progress drives **one exam**, so `coursesTotal`
  is 3 (there are 3 exams, not 21).
- **`courseIndex` (0–20)** — which of the **21 real catalog courses** a brick represents. It
  selects the brick's sprite, prerequisites, meter size, and tax slot.

The 7×3 brick grid is therefore **3 tracks × 7 catalog courses = 21 bricks**. A brick stores both
values in `BrickInfo`. Keep this in mind: `progress.cpp` works on `courseId`, while prerequisites
and sprites in `brick_progress.cpp` / `Sprites.cpp` work on `courseIndex`.

### System Execution Order (per frame)

`SurviveGame::tick(dt)` runs the simulation. The Phase selects which systems run; the
stage-1 play/exam order is below. Order is part of the contract: event **producers** run
before **consumers**, and the two cleanup systems run last so events live exactly one frame.

```
inputSystem(dt)             ← paddle follows mouse; park ball before launch
physicsStepSystem(dt)       ← Box2D step + Position sync + ball-speed regulation
contactEventSystem          ← contacts/sensors → CourseHit / PaddleHit / DropCaught / ProjectileHit
brickMeterSystem            ← consume CourseHit + DropCaught: advance meters, spawn drops
brickClearDelaySystem(dt)   ← timed brick destruction → BrickCleared + Tax drop
ballPaddleSystem            ← paddle bounce (PaddleHit) + ball-loss → LifeLost
dropSystem(dt)              ← remove fallen drops; missed Tax → TaxMissed
brickUnlockSystem           ← prerequisite recompute (only when a brick cleared this frame)
courseProgressSystem        ← BrickCleared → track progress; 100% → ExamStarted
examSystem(dt)              ← exam timer, projectiles, ExamFinished
yearSystem(dt)              ← year timer, totalTime accumulation, year advancement
gameStateSystem             ← fold life/tax/exam events into GameState → win/lose logic
eventCleanupSystem          ← delete all event entities
deadCleanupSystem           ← destroy physics bodies + delete Dead entities
```

`renderSystem` and `hudSystem` run in `SurviveGame::draw()`, once per frame — **not** in
`tick()`. Other Phases take different branches of `tick()`: a year-transition overlay
freezes gameplay (only `yearSystem` runs); the **graduation** stage runs
`graduationInputSystem` + `graduationSystem` instead of the stage-1 systems above.

> **Note:** While paused (`gs.paused == true`) the main loop skips `tick()` entirely —
> all simulation systems are frozen, including timers.

---

## Implemented Features

| Feature | Status |
|---------|--------|
| Paddle + ball physics | ✅ Done |
| 21-brick grid with prerequisites | ✅ Done |
| Assignment & Tax drops | ✅ Done |
| Course progress → Exam trigger | ✅ Done |
| Exam phase (timer + projectiles) | ✅ Done |
| 5 academic years + year timer | ✅ Done |
| Win / Lose resolution | ✅ Done |
| Graduation stage (chair vaulting) | ✅ Done |
| HUD (lives, year, month) | ✅ Done |
| Elapsed time display | ✅ Done |
| ESC / P pause toggle | ✅ Done |

---

## Build

**Requirements:** CMake 3.15+, C++20 compiler, SDL3, SDL3_image, Box2D 3.x (all included in `lib/`).

```bash
cmake -S . -B build
cmake --build build
```

The binary and `res/` are placed together automatically by the post-build step in `CMakeLists.txt`.

---

## Controls

The paddle (and the graduation student) is **mouse-only** — there is no keyboard movement.

| Input | Action |
|-------|--------|
| Mouse move | Move the paddle / graduation student |
| Left click | Launch the ball (stage 1) · jump or retry-after-foul (graduation) · menu/end-screen buttons |
| ESC / P | Toggle pause (during play, exam, or graduation) |

Quitting is done from the on-screen **Exit** button on the win/lose screen.

---

## Debug Hotkeys

Available during gameplay to exercise individual systems in isolation (each synthesizes an
event entity, as if its real producer had fired):

| Key | Action |
|-----|--------|
| H | Synthesize `CourseHit` event |
| J | Synthesize `DropCaught` (Assignment) event |
| E | Synthesize `ExamStarted{course 0}` |
| L | Synthesize `LifeLost` event |
| S | Toggle the slow-ball cheat (play/exam only) |
