---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T18), owner (O75, O79)
---

# The frame bench and the rendered performance scenes

## Context and Problem Statement

M1 task T18 ends the milestone (M1 plan table 2 row T18; MVP-002). Its code is the performance job's rendered scenes:
- **The sandbox's battle drawn** at 1,920 × 1,080, judged against `mvp.client.fps_target` (MVP-085).
- **A scene of `mvp.client.figures_max` figures**, reported.

O75 settled how those scenes are judged and pinned:
- The battle of 400 passes when the 99th percentile of its frame interval is at most 17.7 ms, 16.7 ms and 1 ms of tolerance. Its missed frames and the scene of `mvp.client.figures_max` figures are reported.
- The rendered scenes pin their own display, an external 1,920 × 1,080, 60 Hz monitor, at their first run, and T10's pin stays valid.
- Only the hardware stops a run. The driver and the OS build are recorded with each run.

O79 settled the rest: a tool program, a synthetic battle of 750 a side, a camera fixed on the fight, and 1,200 ticks after a warm-up of 10 s.

Six things do not hold as the code stands:
1. **No program to draw a scene.** [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md) forecloses scene code in the program that players receive, and its performance tool has no window or renderer.
2. **No world of 1,500.** The world server starts the sandbox alone, and the synthetic battle takes its size from `mvp.perf.synthetic_figures` (2,000).
3. **No aimed camera.** The settlement view enters the world over the steward's site, and only the router moves it (ADR-119).
4. **The pin compares every item.** `Build\RunPerformance.py` stops on any difference from T10's pin, the driver, OS build and display included.
5. **No measure of a frame.** The design names the 99th percentile of the frame interval, but not where an interval starts and ends, what a missed frame is, or how the figures on screen are counted.
6. **The rules reach only the two programs.** R7 gives an application manifest to a program project alone, and the check of BLD-042's exported Agility SDK constants reads `Species` alone.

## Decision Drivers

* The frame measured as players get it: the same libraries, runtime, window and settings as `Species.exe` (MVP-085).
* No scene code in the program that players receive (ADR-110).
* Every run of one build draws the same work: a fixed seed, fixed AIs and a fixed camera (DBG-208).
* CI proves that the scenes run, though it cannot time them.

## Considered Options

1. **A tool program** (O79). `Tools/SpeciesFrameBench` composes the client and the integrated server as `Species.exe` does, and runs the scripted scenes in the borderless window.
2. **The performance tool extended.** `SpeciesPerformance` gains the client libraries and a window, and runs the rendered scenes beside the headless ones.
3. **A switch in `Species.exe`.** A start-up option makes the game run the scenes.

## Decision Outcome

Chosen option: 1, because it measures the game's own frame with no scene code in the game. Option 2 makes the headless tool need a GPU and a desktop for scenes that need neither. Option 3 reopens ADR-110's foreclosure.

* **The frame bench** (`Tools/SpeciesFrameBench`):
  - **What it is.** A console-subsystem application of the solution's `Tools` folder. It references the libraries `Species` references and never ships.
  - **How it composes.** As `Species.exe` does: the borderless window over the primary display, the frame renderer with the game's defaults (vertical sync, 4x multisampling), the mounted loose content with both records, the integrated server with its two workers, the client session, the world presentation and the settlement view.
  - **What it leaves out.** It takes the first display attempt only, with no fallback chain, and has no development overlay and no verbs. The input router reads only the window's close request.
  - **How it is built.** `Release|ARM64` is measured. A Debug build's `--warp` draws on the WARP adapter for CI.
  - **Its program files.** It restores the Agility SDK and `WinPixEventRuntime`, exports the Agility SDK constants from its own `AgilitySdkExports.cpp`, and holds the game's manifest (per-monitor DPI awareness v2, UTF-8). So its window has the game's pixels and its device the game's runtime.
* **The scenes**, each a fresh world from seed 1 with both sides under the AI (O77's watched sandbox):

  | Scene | World | Figures | Judged |
  |---|---|---|---|
  | `sandbox-drawn` | the battle sandbox (ADR-109) | 2 × `mvp.sandbox.side_workers` (400) | the 99th-percentile frame interval against 1000 ÷ `mvp.client.fps_target` + 1 ms (O75) |
  | `figures-max-drawn` | the synthetic battle: two halves `mvp.perf.synthetic_gap` apart across the valley's centre | `mvp.client.figures_max` (1,500) | reported (O75) |

* **The world start** (ADR-109 amendment):
  - **The field.** `WorldStart` gains `syntheticFigures`, and a world that names a size is populated as a synthetic battle of that many figures in place of the sandbox's sides. The journal's format is 3.
  - **Passed through.** `WorldServer::Desc` and `IntegratedServer::Desc` carry it. A synthetic battle is a watched sandbox, so no steward commands a side.
  - **The headless tool** keeps its 2,000 through the same function, whose size defaults to `mvp.perf.synthetic_figures`.
* **The camera** (O79):
  - **Aiming it.** `SettlementView::EnterAt` enters the world again with CAM-060's entry pose, `ref.sim.match_entry.fallback_camera_height` along `fallback_camera_direction`, about a given ground point. The bench aims it once the world is entered: the sandbox at the midpoint of its sides' two sites, where they meet, and the synthetic battle at the valley's centre.
  - **Holding it.** The bench updates the view without focus, so the camera never moves. The game cursor is drawn wherever the pointer rests, as in the game.
* **The measurement** (O79):
  - **The warm-up.** It runs 10 s from the first world frame drawn after the camera is aimed.
  - **The measured span.** 1,200 ticks by the session's server tick, about 2 minutes. A quick run takes 1 s and 20 ticks.
  - **A frame interval** is the time from one frame boundary to the next, after the frame is presented.
  - **A frame's work** is its interval less the renderer's waits, as on the frame page (ADR-120).
  - **Not measured.** A frame not presented, minimised or occluded, is counted and not measured.
  - **The figures.** The 50th, 95th and 99th percentiles of both, by the frame page's rule (lower index), with their longest and mean.
  - **Missed frames.** An interval of *n* display periods, rounded, misses *n* − 1 of them, with the period 1000 ÷ `mvp.client.fps_target` ms.
  - **The figures alive and on screen.** The replica's living figures, at least and at most, and those whose replicated positions project inside the view, on average and at most.
  - **The output** is JSON, as the headless tool's is.
* **The script** (ADR-110 amendment, DBG-209 amendment):
  - **What it runs.** `Build\RunPerformance.py` runs the bench after the headless tool, unless `--headless-only` is given.
  - **The rows.** Each rendered scene adds rows to the run's table: the 99th-percentile frame interval, the median and longest interval, the work's 99th percentile, the missed frames, and the figures alive and on screen.
  - **What is judged.** Only `sandbox-drawn`'s 99th percentile is judged, and only when it was drawn at 1,920 × 1,080. Otherwise its row says it was not judged.
  - **The pins (O75).** A later run stops when its processor, memory or graphics adapter differs from the pinned ones, so T10's pin stays valid. Its rendered scenes stop when the display differs from the one their first run pinned, which the record keeps as a pin of its own. The driver, the OS build and the display are written with each run and stop nothing else.
  - **The pinning run** only reports, for the machine and for the display alike (DBG-208).
* **CI.** The script's self-test runs the bench's quick run on WARP, which must draw both scenes. It then proves eleven record rules on fixtures: the machine pin, another processor, another driver and OS build, a run within and one above budget, the display pin, another display for a rendered and a headless run, a rendered run within and above its frame budget, and one drawn at another size.
* **The rules** (R7 and BLD-042 amendments):
  - **Manifests.** A tool project that is an application is a program for R7, so it may hold its own manifest.
  - **Exports.** Every application that restores the Agility SDK exports its constants from its own `AgilitySdkExports.cpp`, which `Build\CheckProjectFiles.py` holds to the package's version.
  - **The header filter.** `.clang-tidy`'s header filter names the new project.

### Consequences

* Good, because the frame is measured through the game's own libraries, runtime, window and defaults, with no scene code in the game.
* Good, because every run draws the same work, and the record judges it against O75's limit on the display it pinned.
* Good, because CI shows on every build that both scenes start, draw and finish.
* Bad, because the bench repeats about 150 lines of the client's composition (the window, the records and the frame loop), which a change to `Species.exe` must follow; both compose the same libraries.
* Bad, because the fixed camera at the entry height frames the fight but not every figure; the figures on screen are reported beside the frame figures.
* Bad, because the rendered scenes take about 5 minutes of the owner's run, in real time.
* Neutral, because the first rendered run pins the display on the reference test machine, apart from T10's pin.
* It forecloses: rendered scenes in `Species.exe`; a frame figure taken from the frame page rather than from the bench.

### Confirmation

* GameLogicTests: the journal reads back a world start with its synthetic battle's figures, and a world start of `mvp.client.figures_max` gives that many figures with an AI on each side (`SandboxTests`).
* CI: the self-test of `Build\RunPerformance.py`, with the bench's quick run on WARP and its eleven fixtures.
* The owner's T18 run on `Release|ARM64` with the pinned display, recorded in `playtests/M1.md`.

## Pros and Cons of the Options

### 1 — a tool program

* Good, because it is the game's frame with the scene code kept out of the game.
* Bad, because it repeats part of the client's composition.

### 2 — the performance tool extended

* Good, because one tool runs every scene.
* Bad, because the headless tool then needs a GPU, a window and the client libraries to run any scene.

### 3 — a switch in `Species.exe`

* Good, because no composition is repeated.
* Bad, because the scene code ships in the program players receive, which ADR-110 forecloses.

## More Information

* Related: [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md), [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md).
* Owner decisions O58, O69, O75, O77 and O79; MVP-002, MVP-085; DBG-208, DBG-209; BLD-042; CAM-060.
* The code: `Tools/SpeciesFrameBench`; `Build/RunPerformance.py`, `Build/CheckProjectFiles.py`; `GameLogic/CommandJournal`, `Sandbox`, `WorldServer` and `IntegratedServer`; `GameClient/SettlementView`.
* **Amendment 2026-10-22 (M1's exit, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md) amendment).** The bench's result also gives the primary display's mode: its width, height and refresh rate. The script pins that mode for the rendered scenes and checks it at their first run. The confirmation's self-test now has sixteen fixtures, not eleven.
