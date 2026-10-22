---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T10)
---

# The scripted steward and the growth scene at M2

## Context and Problem Statement

O80 has a scripted steward tune M2, and the owner's play confirm it (the M2 plan §5). M2 task T10 builds three things:
- the scripted steward, the economy half of an AI settlement (MVP-056);
- the growth scene, which runs one settlement under it for a run of play on a corpus of seeds;
- the CI regression of the scene's first hour, and the performance scenes of ADR-080's row: the Worker at work in a settlement, and the region at its bound with four settlements.

It then tunes table 3's first guesses towards M2's exit by the levers of §9, question 2, in their order. The exit is a growth step at least every 20 minutes in the first 5 hours, a plateau after 4 to 6 hours, and M3's first contact in hour 2 to 3; where not all three hold, the plateau window yields first (O84).

The plan fixes the steward's policy in outline:
- the default priorities;
- a dwelling queued within a stated margin of the population limit;
- a storehouse between outer resource sites once the territory holds them;
- a layout that keeps every port and path clear.

It leaves open:
- how the steward chooses a place without commands that fail;
- what it reads;
- where its state lives;
- how the scene judges a plateau;
- what the CI regression can afford.

## Decision Drivers

* The steward is what M3's AI settlements extend, so it takes the one command path (MVP-094) as an AI sender, reads only what a steward's client could see, and is saved with the world.
* The scene is the measurement the tuning answers to (the M2 plan §9, question 8), so it runs the game's own libraries, not a model.
* CI builds Debug only, and a Debug build runs the simulation about fifteen to twenty times slower than the Linux harness. That ratio is measured from the CI log of the turret's stable-hash test: 9 s in CI against 0.68 s locally.

## Considered Options

For the steward's places:
1. **Check before sending**: a placement check that changes nothing (`CheckPlacement`), run over a fixed layout, and a command only for a place that passes.
2. **Send and learn**: send a placement and read its outcome's reason.

For the steward's state:
3. **Region state**, like the AIs of ADR-107, from state version 10.
4. **A controller outside the world**, like a test driver.

## Decision Outcome

Chosen options: 1 and 3.
- Option 2 needs the outcomes of an AI's commands, which the queue gives to the server and not to the region.
- Option 4 is not saved and not replayed, and M3's AI settlements need both.

* **The steward** (`ScriptedSteward`, region state from version 10, which reads a region of version 9 as having none) is its settlement team's AI commander.
  - It looks every `mvp.steward.think_s` (2 s) at the end of the region's tick, after the settlements' own step, and sends `PlaceStructure` commands that execute at the next tick.
  - It reads its settlement's population, limit, queue and stores, its own territory and structures, the terrain, and the public home resource sites of its site (WLD-084).
  - It leaves the priorities at MVP-036's defaults and builds no turret.
* **Storehouses** (MVP-034). Its spots are the midpoints of the four pairs of neighbouring outer home resource sites, in the generator's order.
  - It tries a spot once its territory holds both sites, when no storehouse of its own stands within the search's reach and no storehouse is queued.
  - The places tried are the spot, then up to `mvp.steward.storehouse_search_rings` (5) rings of eight places, `mvp.steward.storehouse_search_step` (20 wu) apart.
* **Dwellings** (MVP-022). A dwelling is placed when the population is within `mvp.steward.dwelling_margin` (5) of its limit, no dwelling is queued, and the limit is below `mvp.settlement.population_max`.
  - The places tried are a layout of rings from `mvp.steward.ring_first` (140 wu), `mvp.steward.ring_step` (70 wu) apart, up to `mvp.steward.rings_max` (5).
  - The places on a ring stand `mvp.steward.slot_spacing` (70 wu) apart, from the side away from the valley's centre.
  - No place lies within `mvp.steward.lane_half_width` (40 wu) of the line from the Wellspring to a home resource site of its own, which keeps the gatherers' paths clear.
* **A place** is the first in order that `CheckPlacement` passes, the same checks as `PlaceStructure` in the same order without the payment.
  - A place that only the stores refuse, or a full queue, waits for a later look.
  - Storehouses come before dwellings in a look, each only when its cost is held, as in the economy model's steward.
* **The growth scene** (`Tools/SpeciesPerformance`, scene `growth`) runs the valleys of seeds 1 to 8 for `mvp.run.length_hours` on one thread.
  - Each world has one settlement at `mvp.sandbox.steward_site` and its scripted steward.
  - It reports:
    - the server's growth steps (ADR-136);
    - a sample every `mvp.stats.sample_minutes`, with the Workers, the limit, the stores, the strength, the tasks, the idle Workers by reason, the structures and the mean step;
    - the tick at which the territory first reaches the halfway line of one of its valley border pairs, where M3's first contact falls (WLD-064), looked for once a minute.
  - Its judgement is the economy model's:
    - the plateau is the first growth step followed by an hour with none while its Worker count is within 95 % of the run's largest;
    - the largest gap is taken between growth steps from the start up to the fifth hour or the plateau, whichever comes first.
* **The performance scenes** (ADR-080's row):
  - `settlement-at-work` measures the first seed's settlement for 1,000 ticks at the end of its run;
  - `region-bound-settled` founds a settlement at each of the four sites, each with a scripted steward, runs them for the run's length, and measures 1,000 ticks.
  - The snapshot's cost at a settlement's plateau (ADR-137) is the first seed's world at the end of its run, encoded and saved through the snapshot store, with its size.
  - `Build/RunPerformance.py` judges `settlement-at-work` against the Worker's budget and `region-bound-settled` against the region's, and reports each seed's growth against M2's exit and the snapshot's cost. The owner's run records them with `--record playtests/M2.md`.
* **The CI regression** (`GrowthSceneTests`) runs the scene's first hour on the tests' valley and records, at 2 worker threads, at a cost of about 11 s on the Linux harness.
  - Its first five minutes hash alike at 0 worker threads (MVP-093).
  - The hour must hold:
    - no gap of more than 20 minutes between growth steps, counted from its start and up to its end;
    - a dwelling by 50 minutes;
    - 50 to 75 Workers at its end.
  - Those bounds sit about the scene of the tests' valley on the tuned values: a first step at 15 minutes, a first dwelling at 40, and 60 Workers at the hour.

* **The tuning** (the M2 plan §9, question 2), measured by the growth scene on seeds 1 to 8 on the Linux harness's optimised build of the game's libraries. Its results are the same simulation that CI and the owner's machine run, though a compiler that contracts floating point differently may move them by a tick here and there.
  - On the first guesses:
    - the largest gap was 17.3 to 17.8 minutes, before the last dwellings;
    - the halfway line fell at 2.20 to 2.25 hours;
    - the plateau at `mvp.settlement.population_max` (300) came at 3.81 to 3.85 hours, before the window.
  - Lever 1, `mvp.territory.radius_per_worker`, stays at 4. At 3.5 the plateau came at 4.15 hours and the halfway line at 2.77; at 3 the halfway line came at 3.17. It delays contact about as much as the plateau.
  - Lever 2: `mvp.cost.dwelling_ru` at 350 and 400 made the largest gap 20.1 to 23.3 minutes, so it stays at 300. `mvp.cost.dwelling_escalation` goes from 1.15 to 1.10, which shortens the last dwellings' waits.
  - Lever 3: `mvp.essence.orb_cost_ru` goes from 20 to 27, which delays every step alike.
  - Of the sets that met all three targets on two seeds and then on all eight (orb cost 26 at 1.15, 27 at 1.10 and 28 at 1.10), the set taken has the widest margins on the step and the contact, which O84 holds before the plateau:
    - the largest gap is 14.9 to 15.4 minutes, the wait for the first step;
    - the plateau comes at 4.27 to 4.32 hours;
    - the halfway line falls at 2.65 to 2.67 hours.
  - The upkeep and `mvp.resource.ports_per_unit` are not moved. The owner accepted a plateau at the cap (O84), and the stores stay full there.
  - The regression's bounds were recorded on these values.

### Consequences

* Good, because the steward is the AI settlement's economy as M3 will run it: one command path, saved, replayed and hashed with the world.
* Good, because the tuning answers to the game's own simulation over a corpus, not to the model.
* Bad, because the steward's policy is fixed: it never sets priorities or builds turrets, and a settlement played differently grows differently. The owner's own play confirms the tuning (O80).
* Bad, because the CI regression bounds one valley and the tests' records, not the committed meshes; the committed records are measured by the scene.

### Confirmation

GameLogicTests (`ScriptedStewardTests`):
- the dwelling layout on its rings, clear of every lane, and the four storehouse spots, none held at the founding radius;
- the steward placing nothing at 20 Workers of 40, one dwelling at its layout's first place within the margin and no second while it is queued, and one storehouse about a held spot once 120 Workers widen the territory;
- the steward saved and restored, and its settlement hashing alike at 0, 1, 2 and 8 worker threads.

GameLogicTests (`GrowthSceneTests`): the first hour's regression.

The growth scene on seeds 1 to 8, on the tuned values: every seed meets the gap and the plateau window, and reaches the halfway line in hour 2 to 3.

## Pros and Cons of the Options

### 1 — check before sending

* Good, because no command fails for a reason the steward could have seen.
* Bad, because the check runs once for each place tried in a look.

### 2 — send and learn

* Good, because it needs no second form of the checks.
* Bad, because the region does not see its AI's outcomes.

### 3 — region state

* Good, because it is saved and replayed with the world.
* Bad, because a change to its fields is a change of the state version.

### 4 — a controller outside the world

* Good, because the world's state does not change.
* Bad, because a continued world loses its steward.

## More Information

* **Amendment 2026-10-22 (M2 task T19, [ADR-156](ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md)).** A steward who connects to a world that is not watched takes each settlement a steward runs from its scripted steward, which stops, so the world's snapshot still restores. `SpeciesPerformance.exe --saved-world` reads a saved world's growth from its statistics and judges it as the growth scene is, and every run of the tool reads its own growth snapshot back that way.
* **Amendment 2026-10-22 (M3 task T2, [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md)).** Each AI settlement of an M3 world is run by a scripted steward from its founding, as its AI commander (O98).
* **Amendment 2026-10-22 (M3 task T7, [ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md)).** The scripted steward of an AI settlement also runs its decisions, its claims, its attacks and its battle cores, after its own look, sharing its sequence of commands with them.
