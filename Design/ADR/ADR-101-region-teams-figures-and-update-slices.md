---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T2)
---

# Region teams, figures and the update slices: the region's first state

## Context and Problem Statement

M1's battle sandbox runs crowds of Workers and Leaders on the reference crowd rules (MVP-040). Its first task, T2 of the [M1 plan](../implementation/m1_plan.md), gives `RegionSimulation` (ADR-096) the reference's tick in its update slices (R-SIM-510 to R-SIM-516), the containers the figures live in (R-SIM-204 to R-SIM-225), the Worker's and the Leader's stats and health change (R-UNIT-011 to R-UNIT-033), the game's draw purposes (ADR-097) and a state hash and snapshot that cover the figures (MVP-093, MVP-095).

The reference describes a match world with six team slots, actor groups, structures and a tick clock with game speed. The region needs part of it, in a world that has no match, and in the layout of ADR-090. These decisions are open:
- what a region holds, and how its figures are stored and advanced;
- what a figure is in code;
- the tick clock;
- where the reference's values come from;
- the draw purposes;
- how the state layout grows and migrates.

## Decision Drivers

* The reference's observable order is the specification: slot order, last-freed-first reuse, the slice bounds of R-SIM-213 and the rule of R-SIM-215 decide which object advances when (DEC-455, R-SIM-211).
* A figure created during another figure's advance must not move it (R-SIM-128).
* Every tuning value lives in data (MVP-099, `AGENTS.md` §5).
* The state hash and the snapshot cover the same bytes (ADR-097, ADR-099), and a layout change is a reviewed change (DBG-147).

## Considered Options

* 1 — team slots of ungrouped figures in a generic sliced pool over a deque; a figure as a plain aggregate with its rules as free functions; the region's tick as its clock; the reference values in the tuning record
* 2 — one pool of every figure of the region, with the team as a field
* 3 — a figure class with its own virtual advance per kind, as the reference's actor types are

## Decision Outcome

Chosen option: "1".
- Option 2 changes the reference order: R-SIM-223 advances every figure of team 0 before any of team 1 in each slice, which a single pool by slot does not give.
- Option 3 brings the reference's type hierarchy (R-SIM-103) into a game that builds two kinds, and hides the order of a figure's steps in overrides; the MVP's rules read clearer as functions over one aggregate.

* **Teams** (`RegionSimulation::Team`). A region holds team slots in index order, made on first use, each with its ungrouped figures. The MVP builds no actor groups (R-SIM-102): Workers and Leaders are ungrouped actors (R-UNIT-022), and a formation is a Leader with following Workers, not a group. A region holds at most 256 teams.
* **The sliced pool** (GameLogic `SlicedPool<T>`). It implements:
  - slots that never move, a free list whose head is the last freed slot, and growth by the step `ref.sim.container.ungrouped_step`, the new slots handed out in ascending order (R-SIM-207, R-SIM-209);
  - the slice bounds from the capacity when each slice starts, the last advanced slot, and slot 0 recorded for a new or emptied pool (R-SIM-213 to R-SIM-215);
  - the advance of a slice in ascending slot order, slots that become used during it included, each advance able to ask for its slot's removal (R-SIM-206, R-SIM-221 step 5).
  - Its values live in a `std::deque`, which grows without moving the existing ones, so a figure that an advance creates leaves the advancing figure where it is (R-SIM-128).
* **The figure** (`Figure`) is a plain aggregate (`AGENTS.md` R8): stable id, kind, position, velocity, front, spawn point, roam range, speed, health, reload interval and timer, and the flags on ground, enabled, destroyed, dead and in formation. Later tasks add fields.
  - Its stable id from the region's counter (ADR-083 item 5) stands for the reference's serial (R-SIM-137); its team and slot are its container's.
  - Health is an integer: every change is an integer amount (R-UNIT-026), and the stat table's healths are whole.
  - The rules are free functions: `SetKind`, `SpawnFront`, `ApplyHealthChange` and `BaseAdvance`. `BaseAdvance` runs R-SIM-111 steps 1 to 3 and the corpse fade of R-UNIT-034. The water step, the world clamp and route following come with T4 and T5.
* **The tick** (R-SIM-510, R-SIM-218). A region step executes the tick's commands, then runs slices 0 to `ref.sim.update_slices` − 1. Each slice runs the teams in index order. The weapons, structures and Essence orbs of R-SIM-512 follow the teams in each slice when their pools exist (T6).
  - The tick clock is the region's tick. The world has no game speed, no pause and no match time (R-SIM-540 is skirmish's), so the clock update of R-SIM-541 has nothing to do.
* **Spawning** (R-SIM-226). `SpawnFigures` runs the spawn steps in order: id, kind, position, on-ground flag, velocity, front, spawn point and roam range.
  - Its draws are the region's next tick's, inside a step the tick being stepped.
  - The valid-spawn-position search (R-SIM-368) and the actor-grid registration come with T4, which owns the grids.
* **Values in data.** The reference's values that T2 reads are keys of the tuning record under `ref.*`: the slice count, the pool step, the default front, the health maximum, the Worker's and the Leader's stats, the formation explosive factor and the corpse fade. Each key keeps the reference table's name and value (O13), and the schema generator's header reads them (ADR-098).
  - `ref.sim.spawn.default_front` is a vector, which the schema subset of ADR-098 has no type for. It is an object of three decimals, `x`, `y` and `z`.
  - A reference key is defined once, in its reference table (CONVENTIONS §4), so it is not repeated in the MVP scope's table 3.
* **Draw purposes** (`DrawPurpose`, GameLogic). The game's enumeration of ADR-097, numbered from 1 and never renumbered: `ReloadTimer` (R-UNIT-015) and `SpawnFront` (R-UNIT-059). Each reference draw keeps its order within its own context; the order between contexts no longer matters, because each purpose is keyed apart (ADR-082).
* **State.** A region writes, after its queued commands, its team count and then each team's pool:
  - the pool's capacity, last advanced slot and free list from the bottom to the head;
  - each slot's use, and each used slot's figure, every field in declaration order, binary64 as its 64 bits (ADR-081 item 7).
  - A read refuses a free list that names a used slot, a slot listed twice or beyond the capacity, and a non-finite value (R-SIM-107).
  - `WORLD_STATE_VERSION` becomes 2. The migration step from version 1 reads a region of M0's layout as one with no teams (ADR-099 amendment).
  - The golden run of DBG-147 changes once: its empty regions now write a team count of 0. An independent model of the layout computes the same values.

### Consequences

* Good, because the reference's order holds by construction, and the pool's tests state it as the reference's acceptance criteria do.
* Good, because the rules read every value from data, so retuning a stat changes no code.
* Bad, because every figure field is in the hash, so every task that adds one changes the state version or, for a world with figures, the hashes. The golden run stays on empty regions, so a figure field does not change it.
* Bad, because a deque's elements are not contiguous, so a pass over every figure is slower than over a vector. ADR-080's measurement at T10 shows whether that matters.
* It forecloses: an object that moves in its pool; a figure kind with its own advance hierarchy; a reference value written in code.

### Confirmation

* `GameLogicTests` checks:
  - the pool's acceptance criteria AC-1, AC-2, AC-4, AC-5 and AC-6 of the containers, the same-tick rule of R-SIM-516, removal at an advance, and a round trip of its state with a free list that names a used slot refused;
  - the Worker's and the Leader's stats, the keyed reload draw, the spawn fronts, the health change with its clamp, kill and formation factor, and the corpse fade on the ground and in the air;
  - one advance per figure per tick, and, in development builds, the order of slices, teams and slots;
  - the same hashes with figures at 0, 1, 2 and 8 workers, a figure's state read back to the same bytes, and a changed figure changing its region's hash;
  - the golden run's new values, and a version-1 world restored as one with no teams.
* ADR-080's measurement at T10 confirms the pool's cost.

## Pros and Cons of the Options

### 1 — team slots over a generic sliced pool, a figure aggregate, free rules

* Good, because one pool type serves the figures now and the orbs, bolts and effects of T6.
* Bad, because the rules take the tuning record as an argument wherever they read it.

### 2 — one pool for the region

* Good, because there is one container.
* Bad, because the reference's team order needs a sort or a second index.

### 3 — a figure class hierarchy

* Good, because it reads like the reference's diagrams.
* Bad, because the two kinds share almost every step, and the order of a figure's steps would hide in overrides.

## More Information

* Related: [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-098](ADR-098-tuning-in-data-and-the-schema-generator.md), [ADR-099](ADR-099-world-snapshots.md).
* The rules: R-SIM-204 to R-SIM-226 and R-SIM-510 to R-SIM-516 ([simulation model](../reference/12_simulation_model/README.md)), R-UNIT-011 to R-UNIT-034, R-UNIT-059 and R-UNIT-116 ([units](../reference/02_units/README.md)).
* **Amendment 2026-10-20 (M1 task T2 and T3, [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md)).** The pool's values live in blocks of 64 slots owned by a vector of `unique_ptr`, not a `std::deque`: MSVC's debug deque allocates in its move constructor, so the pool could not move without the risk of a throw, which clang-tidy's `bugprone-exception-escape` refuses. A slot still never moves when the pool grows, and a block's 64 values are contiguous. The region's team bound is `RegionSimulation::TEAM_SLOTS` (256).
* **Amendment 2026-10-20 (M1 task T4, [ADR-103](ADR-103-region-grids-draw-contexts-and-figure-movement.md)).** The base advance takes the region's movement context and lives with the kinds' advances (`FigureAdvance`): it runs the swimming step of a figure that is not a Worker and the world clamp. `DrawScope` is replaced by `TickDraws`, one context per purpose and figure for the whole tick. A spawn takes a spread and an optional roam range, runs the spawn-position search and registers the figure in the actor grid (R-SIM-226 steps 3, 9 and 12).
