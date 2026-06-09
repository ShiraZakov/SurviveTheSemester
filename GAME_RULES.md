# Survive the Semester — Game Rules

## Objective

Clear all 21 courses across 5 academic years before running out of lives or failing your average.
Each course is represented by a brick on the field. Hit bricks with the ball, catch assignment drops
with the paddle, survive exams, and avoid academic hazards to graduate.

---

## The Field

- A paddle at the bottom of the screen, controlled with the **mouse** or **A/D arrow keys**.
- A ball that bounces off walls, the paddle, and bricks.
- 21 course bricks arranged in a 7×3 grid, grouped by academic year.
- Drops falling from cleared bricks that must be caught with the paddle.
- Academic hazards scattered across the field.

---

## Academic Years

The game is divided into **5 academic years**. Each year has a **30-second timer**.

| Year | Courses |
|------|---------|
| Year 1 | Courses 1–4 |
| Year 2 | Courses 5–8 |
| Year 3 | Courses 9–12 |
| Year 4 | Courses 13–16 |
| Year 5 | Courses 17–21 (including the Final Project) |

- The year advances automatically when all its courses are cleared **or** the 30-second timer expires.
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
| **Bonus** | Gold/glowing | Bonus points toward course progress |
| **Tax** | Gold with bar detail | Must be caught; missing it reduces your average by **−5 points** |

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
- Your average is updated after each exam based on the grade received.
- Penalties:
  - **−10 points** per life lost
  - **−5 points** per Tax drop missed

**Pass threshold:** average ≥ 60 to graduate.

---

## Lives

- You start with **3 lives** (shown as pips in the HUD).
- You lose a life when:
  - The ball falls below the paddle
  - An **Academic Hazard** of type `LoseLife` is triggered
- When lives reach 0, the game is lost.

---

## Academic Hazards

Starting from **Year 2**, one hazard entity spawns at the beginning of each new year.
It appears at a random position in the middle third of the field and is triggered when the ball passes through it.

| Hazard Type | Effect |
|-------------|--------|
| `LoseLife` | Lose 1 life immediately |
| `ReduceProgress` | The target course loses 25% of its progress *(not yet active)* |

---

## Win & Lose Conditions

### You Win if:
- All 21 courses are cleared **and** your final average is **≥ 60**.

### You Lose if:
- Lives reach **0**
- All academic years expire without clearing everything
- All exams are completed but your average is **< 60**

---

## Controls

| Input | Action |
|-------|--------|
| Mouse move | Move paddle |
| A / ← | Move paddle left |
| D / → | Move paddle right |
| Space | Launch ball from paddle |
| Escape / Q | Quit game |

---

## HUD

| Element | Location | Description |
|---------|----------|-------------|
| Lives pips | Top-left | Remaining lives (max 3) |
| Year badge | Top-center | Current academic year |
| Year timer bar | Top-center | 5-segment countdown (30s per year) |
| Academic month | Top-center | Oct → Jul progression |
| Average grade | Status line | Running weighted average |
| Phase indicator | Status line | PLAYING / EXAM / WON / LOST |
| Win/Lose overlay | Center | Final result screen |
