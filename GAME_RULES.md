# Survive the Semester — Game Rules

> **Note:** This document describes the current implemented mechanics.
> A full detailed rules document is still to be written.

---

## Objective

Clear all 21 courses across 5 academic years before running out of lives or failing your average.
Each course is represented by a brick on the field. Hit bricks with the ball, catch assignment drops
with the paddle, survive exams, and graduate.

---

## The Field

- A paddle at the bottom of the screen, controlled with the **mouse** or **A/D / arrow keys**.
- A ball that bounces off walls, the paddle, and bricks.
- 21 course bricks arranged in a 7×3 grid, grouped by academic year.
- Drops falling from cleared bricks that must be caught with the paddle.

---

## Academic Years

The game is divided into **5 academic years**. Each year has a **45-second timer**.

| Year | Courses |
|------|---------|
| Year 1 | Courses 1–4 |
| Year 2 | Courses 5–8 |
| Year 3 | Courses 9–12 |
| Year 4 | Courses 13–16 |
| Year 5 | Courses 17–21 (including the Final Project) |

- The year advances automatically when all its courses are cleared **or** the 45-second timer expires.
- If Year 5 ends without clearing everything, the game is lost.

---

## Course Bricks

Each brick represents a university course.

- **Locked courses** are grayed out and cannot be hit until their prerequisites are met.
- **Prerequisite chain:** some courses require earlier courses to be completed first
  (e.g., Calculus 2 requires Calculus 1; the Final Project requires all other courses).
- **Starting unlocked:** Intro to CS, C Programming, Math Reasoning, Discrete Math, Probability.

### Hitting a Brick

- Each brick has a **progress meter** (3 or 5 segments depending on the course).
- Every ball hit increments the meter by one segment and spawns an **Assignment drop**.
- When the meter is full, the brick enters a brief **clear delay** (~0.5s), then is destroyed.
- Destroying a brick emits a `BrickCleared` event and spawns a **Tax drop**.

---

## Drops

Three types of drops fall from bricks and must be caught with the paddle.

| Drop | Color | Effect |
|------|-------|--------|
| **Assignment** | Course color | Increments the brick's progress meter |
| **Tax** | Gold with bar detail | Must be caught; missing it reduces your average by **−5 points** |

*(A `Bonus` drop type exists in the `DropType` enum but is not spawned by the current build.)*

- Drops that fall below the screen without being caught are lost.
- Tax drops that are missed emit a `TaxMissed` penalty.

---

## Course Progress & Exams

- Each course has a progress bar from 0% to 100%.
- Progress is advanced by ball hits (`BrickCleared` events).
- When a course reaches **100%**, an **Exam** is triggered for that course.

### Exam Phase

- The game enters `Phase::EXAM` for the active course.
- A timer counts down (**15 seconds**).
- **Projectiles** are launched toward the paddle every 3 seconds — dodge them to survive.
- Grade starts at **100** and loses **12 points per projectile hit**, with a minimum of **55**.
- After the exam, the course is marked **Done**, the grade is added to the weighted average, and play resumes.

---

## Scoring & Average Grade

- You start with an average of **100.0**.
- Your average is updated after each exam based on the grade received, weighted by course credits.
- Penalties:
  - **−10 points** per life lost
  - **−5 points** per Tax drop missed

**Pass threshold:** average ≥ 60 to graduate.

---

## Lives

- You start with **3 lives** (shown as pips in the HUD).
- You lose a life when the ball falls below the paddle.
- When lives reach 0, the game is lost.

---

## Academic Hazards

*(Not implemented.)* Academic hazards were planned early on but were cut from the current
build — there is no hazard entity, system, or `HazardType` enum in the code. The exam phase
is the main in-course challenge instead.

---

## Graduation Stage

After all 21 courses are cleared, the game transitions to `Phase::GRADUATION`.

- The paddle becomes a **student character**.
- The player must navigate the student across rows of chairs to reach the graduation stage.
- **Left click** to jump toward the stage (the student vaults over the nearest chair in the row).
- Reach the stage to win.

---

## Win & Lose Conditions

### You Win if:
- All 21 courses are cleared **and** your final average is **≥ 60**
- **and** the student successfully reaches the graduation stage.

### You Lose if:
- Lives reach **0**
- All academic years expire without clearing everything
- All exams are completed but your average is **< 60**

---

## Pause

- Press **ESC** or **P** (or click the on-screen **Pause** button) during PLAYING, EXAM, or
  GRADUATION to pause; press it again (or click **Resume**) to continue.
- A "PAUSED" banner shows over the frozen game; the elapsed timer is **frozen** while paused.
- Quitting is done from the **Exit** button on the win/lose screen, not from the pause state.

---

## Controls

The paddle (and graduation student) is **mouse-only** — there is no keyboard movement, and
the ball is launched with a click, not the Space bar.

| Input | Action |
|-------|--------|
| Mouse move | Move the paddle / graduation student |
| Left click | Launch the ball (stage 1) · jump / retry-after-foul (graduation) · UI buttons |
| ESC / P | Toggle pause |

---

## HUD

| Element | Location | Description |
|---------|----------|-------------|
| Lives pips | Top-left | Remaining lives (max 3) |
| Year badge | Top-center | Current academic year |
| Year timer bar | Top-center | 5-segment countdown (45s per year) |
| Academic month | Top-center | Oct → Jul progression |
| Average grade | Status line | Running weighted average |
| Phase indicator | Status line | PLAYING / EXAM / GRADUATION / WON / LOST |
| Elapsed timer | Top-right | Total play time since launch (`Time: M:SS`) — pauses with ESC |
| Win/Lose overlay | Center | Final result screen with average and reset prompt |
