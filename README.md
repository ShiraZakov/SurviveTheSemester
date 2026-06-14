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

## Team

| Member | Responsibility |
|--------|---------------|
| Shira | Engine core, physics integration, game loop, win/lose logic |
| Aviel | Ball, paddle, brick collisions, render system |
| May | Courses, drops, progress tracking, HUD |
| Yuval | Exam phase, academic hazards |

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
│   ├── Game.h / Game.cpp         # System scheduler, scene setup, debug hotkeys
│   ├── Config.h                  # All tunables: sizes, speeds, timers, thresholds
│   ├── Enums.h                   # Phase, CourseState, DropType, HazardType, Shape
│   ├── Components.h              # All ECS component and tag structs (incl. GameState)
│   ├── Events.h                  # Event component structs + producer helpers
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
│       ├── progress.cpp          # Course progress → ExamStarted trigger
│       ├── exam.cpp              # Exam phase: timer, projectiles, ExamFinished
│       ├── hazards.cpp           # Hazard spawning and effect application (Yuval — TODO)
│       ├── hud.cpp               # Lives, year, elapsed timer, status line, pause overlay
│       ├── endgame.cpp           # Average calculation, win/lose resolution
│       ├── graduation.cpp        # Graduation stage: student navigation, chair vaulting
│       └── year.cpp              # Academic year timer and advancement
│
├── bagel.h                       # BAGEL ECS framework (do not edit)
├── lib/                          # SDL3, SDL3_image, Box2D libraries (do not edit)
├── res/                          # Game assets (spritesheet, textures)
├── tests/
│   ├── test_brick_prereqs.cpp    # Unit tests for prerequisite resolution
│   └── test_graduation_year.cpp  # Unit tests for graduation + year logic
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

### System Execution Order (per frame)

```
inputSystem
physicsStepSystem(dt)       ← Box2D step + Position sync
contactEventSystem          ← contacts → CourseHit / HazardTriggered / DropCaught events
brickMeterSystem            ← hit responses, drop spawning
brickClearDelaySystem       ← timed brick destruction
courseHitSystem             ← paddle bounce, ball loss
dropSystem(dt)              ← remove fallen drops
brickUnlockSystem           ← prerequisite checking
courseProgressSystem        ← course state, ExamStarted trigger
examSystem(dt)              ← exam timer, projectiles, ExamFinished
hazardSystem                ← spawn and apply hazard effects
yearSystem                  ← year timer, totalTime accumulation, year advancement
gameStateSystem             ← life/tax/exam → win/lose logic
eventCleanupSystem          ← delete all event entities
deadCleanupSystem           ← destroy physics bodies + delete Dead entities
renderSystem                ← draw all Drawable entities
hudSystem                   ← lives, year, elapsed timer, status line, pause overlay
```

> **Note:** During pause (`gs.paused == true`) the main loop skips `tick()` entirely —
> all simulation systems are frozen including timers.

---

## Implemented Features

| Feature | Status | Owner |
|---------|--------|-------|
| Paddle + ball physics | ✅ Done | Aviel |
| 21-brick grid with prerequisites | ✅ Done | Aviel / May |
| Assignment & Tax drops | ✅ Done | May |
| Course progress → Exam trigger | ✅ Done | May |
| Exam phase (timer + projectiles) | ✅ Done | Yuval |
| Academic hazards | ⚠️ Partially done — spawn active, effects pending | Yuval |
| 5 academic years + year timer | ✅ Done | Shira |
| Win / Lose resolution | ✅ Done | Shira |
| Graduation stage (chair vaulting) | ✅ Done | Shira |
| HUD (lives, year, month) | ✅ Done | May |
| Elapsed time display | ✅ Done | May |
| ESC pause menu (Y/N quit) | ✅ Done | May |

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

| Input | Action |
|-------|--------|
| Mouse move | Move paddle |
| A / ← | Move paddle left |
| D / → | Move paddle right |
| Space | Launch ball / confirm action |
| ESC | Pause game (shows quit menu) |
| Y | Confirm quit (while paused) |
| N | Resume game (while paused) |
| R | Reset scene |

---

## Debug Hotkeys

Available during gameplay (useful for testing individual systems in isolation):

| Key | Action |
|-----|--------|
| H | Synthesize CourseHit event |
| J | Synthesize DropCaught event |
| E | Synthesize ExamStarted{course 0} |
| L | Synthesize LifeLost event |
| Space | Launch ball |
| R | Reset scene |
