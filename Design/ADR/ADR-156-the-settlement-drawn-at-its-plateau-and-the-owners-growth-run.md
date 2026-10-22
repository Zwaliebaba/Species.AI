---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T19, owner decisions O75, O84, O88)
---

# The settlement drawn at its plateau and the owner's growth run

## Context and Problem Statement

M2's T19 makes the milestone's exit measurable. It has two parts that need code:
- **The rendered scene.** The performance script draws a settlement at its plateau and judges it against `mvp.client.fps_target`, with O75's 17.7 ms at the 99th percentile. M1's frame interval is taken up at T19 as well (O88, [ADR-148](ADR-148-the-frame-starts-at-the-latency-wait.md)).
- **The owner's growth run** of the M2 plan §5: the owner plays from 20 Workers to the plateau across sessions, and the server's statistics (MVP-064) are compared with the growth scene's.

What exists today:
- The frame bench ([ADR-126](ADR-126-frame-bench-and-rendered-scenes.md)) draws two battles, each a world that starts at once.
- A settlement reaches its plateau only after about 4.3 hours of play (the growth scene, [ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md)).
- The statistics are held in the world's state, so the owner cannot read them out of a saved world.

A world run by a scripted steward and then given to a connecting steward also has a gap. The steward takes the team's command, but the scripted steward stays in the region's state. Its snapshot then holds a commander that may not command, and the reader refuses that snapshot as damaged.

## Decision Drivers

* The rendered scene draws what the steward sees at the plateau: the settlement's structures, its Workers at work, the territory and the HUD.
* The bench runs unattended, on its own, with no input file.
* The owner's evidence is read from the server's own statistics, by the same judgement as the growth scene's.
* No snapshot the server writes is one its reader refuses.

## Considered Options

1. **The bench grows the world itself.** It grows the world headless before its window opens, saves a snapshot, and continues it with `restoreOnly` ([ADR-155](ADR-155-the-front-end-and-saved-worlds-at-m2.md)).
2. **The headless tool hands its snapshot to the bench.** The headless tool's growth run of seed 1 already ends with a snapshot.
3. **A synthetic settlement placed at once**, as the synthetic battle is.

For the owner's run:

4. **The headless tool reads a saved world's statistics.**
5. **The client shows or exports the statistics.**

## Decision Outcome

Chosen options: 1 and 4.
- **Option 2** ties the bench to a file another tool wrote, and the bench could no longer run a scene on its own.
- **Option 3** places structures and Workers that no growth made. It also needs a placement layout of its own, and its frame would not be the steward's.
- **Option 5** adds a player-facing path that M2 does not need. The statistics are evidence for the owner, not a HUD figure.

* **The settlement scene** (`Tools/SpeciesFrameBench`, `settlement-drawn`):
  - **Growing the world.** Before the window opens, the bench grows seed 1's valley on one thread. The steward's settlement stands at `mvp.sandbox.steward_site`, run by its scripted steward, as `--watch --world` starts it (ADR-140). Growth stops when the population reaches `mvp.settlement.population_max`, where the growth scene's plateau lies (O84), or when `mvp.run.length_hours` passes. A quick run grows it for one minute of play.
  - **Why before the window.** A window that is not pumped for the minute of growth would be marked as not responding.
  - **Continuing it.** The bench saves the world into a folder of its own under the temporary folder. The integrated server continues it with `restoreOnly`, as the front end continues a saved world.
  - **What it draws.** The scene's client connects as the steward, so the HUD draws the commander's figures. The camera is over the Wellspring, as the settlement view enters it.
  - **The measurement** is the battles': a warm-up of 10 s, then 1,200 ticks.
  - **The result.** The JSON gives the scene's `grown` record: the ticks it grew for, and its Workers and structures.
  - **The clean-stop marker.** The world opened has none, so its server records an unclean stop at its first tick. That freeze changes nothing that is drawn.
* **The performance script** (`Build/RunPerformance.py`):
  - **Judged scenes.** The sandbox's battle and the settlement drawn are both judged against the frame budget. A first rendered run pins the display only when both were drawn at 1,920 × 1,080.
  - **Reported.** How the settlement grew is reported.
  - **The self-test** runs the bench's three scenes. Its fixtures prove that a settlement drawn above the budget fails the run.
* **The owner's growth run:**
  - **The tool's reading.** `SpeciesPerformance.exe --saved-world <world folder>` runs no scene. It reads the newest snapshot of the folder that restores, and the statistics of the settlement a steward runs (`ReadSavedWorld`). It then judges them with the growth scene's `JudgeGrowth`: the largest gap between growth steps up to the plateau or hour 5, and the plateau.
  - **Its JSON** holds the growth steps and the samples.
  - **The tool's own check.** Every run of the tool reads the growth scene's own snapshot back as a saved world, and fails unless it gives the run's growth steps.
  - **The script.** `Build\RunPerformance.py --saved-world <world folder>` writes the judgement into `playtests/M2.md`, in a section of its own, "The owner's growth run". It neither reads nor pins the machine.
  - **A plateau still forming.** A world read less than an hour after a step that brought it within 95 % of its largest Worker count has an open gap that may be the plateau's quiet hour. The script reports that gap as open rather than as a missed step.
* **The steward takes the settlement** (`ConnectSteward`, `RegionSimulation::RemoveScriptedStewards`):
  - In a world that is not watched, a connecting steward commands each settlement a steward runs, and that settlement's scripted steward stops. A watched world keeps its scripted steward.
  - The rule is journaled with the connection, so a replay applies it too.

### Consequences

* Good, because the rendered scene is the steward's own frame at the plateau: the HUD, the structures, 300 Workers at work and the territory, from one function that starts every world.
* Good, because the owner's growth run is judged by the same code as the growth scene, from the server's own record.
* Good, because a world taken from a scripted steward saves a snapshot its reader restores.
* Bad, because the bench spends the growth's time before it draws, about 155,000 ticks of one settlement. As a measure, a Linux build of the game's libraries at `-O2` with the development checks on grew seed 1 for 180,000 ticks, five hours of play, in 80 s.
* Bad, because a saved world's halfway line is not recorded, so the owner's run cannot judge M3's contact hour.
* It forecloses: a scripted steward that keeps running a settlement its steward has taken; a rendered settlement scene that depends on another tool's file.

### Confirmation

* GameLogicTests: `ScriptedStewardTests.Mvp094AStewardWhoConnectsTakesTheSettlementFromItsScriptedSteward`. The steward commands the settlement, its scripted steward stops, and the world's snapshot restores; a watched world keeps its scripted steward.
* The tool's read-back check, on every run. CI's quick run reads the growth scene's snapshot back as a saved world.
* CI: `RunPerformance.py --self-test` with the bench on WARP draws the three scenes. Its fixtures prove:
  - a settlement drawn above the budget fails the run;
  - how the settlement grew is reported;
  - the owner's growth run is recorded and judged in its own section, and later runs stay in that section;
  - a gap open while the plateau may be forming is not judged yet.
* The owner, on `Release|ARM64`:
  - `python Build\RunPerformance.py --record playtests/M2.md` judges both rendered scenes;
  - `python Build\RunPerformance.py --saved-world "%LOCALAPPDATA%\Species\saves\world-0001"` records the growth run.

## Pros and Cons of the Options

### 1 — the bench grows the world

* Good, because the bench runs every scene alone.
* Bad, because the growth's time is spent on every run.

### 2 — the headless tool's snapshot

* Good, because the growth is run once.
* Bad, because the bench depends on another tool's file and order.

### 3 — a synthetic settlement

* Good, because nothing grows.
* Bad, because no growth made what it draws.

### 4 — the tool reads a saved world

* Good, because one judgement serves the scene and the owner.
* Bad, because the owner runs a tool beside the game.

### 5 — the client shows the statistics

* Good, because the owner reads them in the game.
* Bad, because it is player-facing work that M2 does not need.

## More Information

* Amends [ADR-126](ADR-126-frame-bench-and-rendered-scenes.md) (a third rendered scene, judged) and [ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md) (the scripted steward stops when a steward takes its settlement; the saved-world reading).
* Owner decisions O75, O84, O88; MVP-064, MVP-075, MVP-085, MVP-094; PIL-055; DBG-208.
* The code:
  - `Tools/SpeciesFrameBench` (`GrowSettlement`, `settlement-drawn`);
  - `Tools/SpeciesPerformance` (`ReadSavedWorld`, `--saved-world`);
  - `Build/RunPerformance.py`;
  - `GameLogic/CommandJournal` (`ConnectSteward`) and `RegionSimulation` (`RemoveScriptedStewards`).
