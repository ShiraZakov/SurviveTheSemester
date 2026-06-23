# Survive the Semester — TODO & Next Steps

**Last updated:** 09.06.2026

---

## A. Critical Bugs (block correct win/lose logic)

### 1. Average not updated from exam grades — `endgame.cpp` (Shira)
`gameStateSystem` increments `coursesDone += 1` on `ExamFinished` but **never updates `gs.average`** based on the grade received.
Currently the average only decreases from `TaxMissed` and `LifeLost` — exam performance has no effect.
`Config.h` already defines `courseCredits()` with credit weights per course — currently unused.
**Required:** implement weighted GPA formula in `gameStateSystem` using course credits.

### 2. Win condition does not check average — `endgame.cpp` (Shira)
`allBricksCleared()` sets `Phase::WON` directly without verifying `average >= 60`.
**Required:** every WON path must check `gs.average >= Config::PASS` before resolving to WON.

---

## B. Rules Defined in GAME_RULES.md but Not Implemented

| # | Rule | Relevant File | Owner |
|---|------|--------------|-------|
| 1 | **Bonus Drop** — listed in rules, `DropType` enum has no Bonus entry at all | `EntityFactory.cpp`, `brick_progress.cpp` | Team |
| 2 | **ReduceProgress Hazard** — logic exists in `hazards.cpp` but no hazard of this type is ever spawned | `hazards.cpp` | Yuval |
| 3 | **Escape / Q to quit** — documented in controls, not handled in `Game.cpp` / `main.cpp` | `main.cpp` | Shira |

---

## C. Rules That Need Team Decision Before Further Development

> These are open questions. Until resolved, they cannot be implemented correctly.

### 1. Course Progress — how much per event?
- How many ball hits are required to complete a course? (currently `0.14` per `BrickCleared`)
- Does catching an Assignment drop advance progress? By how much?
- Does the Bonus drop exist in practice, and what is its value?

### 2. Tax Drop — when is it created?
- Is it spawned when **the brick is destroyed**, or when **the course reaches 100%**?
- Currently spawned in `clearBrick()` — i.e. when the brick is destroyed.

### 3. Number of active courses — 3 or 21?
- GAME_RULES.md defines 21 courses; `Config::COURSES = 3`.
- Is the final game intended to have 21 active courses, or is 3 the final scope?
- How does year progression relate to the number of courses completed?

### 4. Hazard — full behavior
- Does `ReduceProgress` stay in the rules or get removed?
- Does a hazard disappear after the ball passes through it? (currently it does not)
- Can more than one hazard be active per year?

### 5. Lose conditions — priority and ordering
- If the average drops below 60 **mid-game** — does the game end immediately?
- If lives reach 0 **during an exam** — what happens to the exam in progress?

---

## D. Visual Placeholders — Awaiting Sprites

| Element | Current State | Owner |
|---------|--------------|-------|
| **Tax Drop** | Yellow rectangle — unclear if visible during gameplay | Team |
| **Hazard** | Purple rectangle | Yuval |

---

## E. Open Development Tasks

| Task | Owner | Priority |
|------|-------|----------|
| full-loop-test stages 4–6 (exam grade → average → win/lose) | May | High |
| hazard-visual — assign a sprite to the purple rectangle | Yuval | Medium |
| sprite-layering — draw order in `render.cpp` | Aviel | Medium |
| render-exam-field — visual change to field during EXAM phase | Aviel | Low |
| win-lose-overlay — show final average + courses completed count | Shira | High |
| Full integration test | Shira | High |

---

## F. General Open Questions for the Team

- **Course progress meter in HUD** — what exactly does it represent? Is it calculated correctly relative to the active courses?
- **A/D keys alongside mouse** — do they work correctly in parallel? Needs testing.
- **Bottom wall behavior** — ball exits below paddle and triggers life loss. Does the logic hold in all edge cases?
