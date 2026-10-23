---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T10)
---

# The cost measurement: marker timing, the performance tool and the performance script

## Context and Problem Statement

Task T10 of the [M1 plan](../implementation/m1_plan.md) builds the MVP's performance job: `Build\RunPerformance.py` (§4), the headless scenes and the travel scene, with the per-object costs read from the profile markers (ADR-068).
- The owner's first run pins the reference test machine (O58, O69, DBG-209).
- ADR-080 item 2's budgets are then confirmed or amended from the measured figures.
- `wld.travel.valley_path_factor` is set from the crowd's walk (O71, WLD-100).

These decisions are open:
- how a script reads per-object costs from the markers, which are PIX events with no time a program can read;
- which executable runs the scenes in the measured `Release|ARM64` build, and how each run measures the same work;
- which scene and which markers measure each row of ADR-080's budgets;
- how the record pins the machine and judges a figure;
- how the travel factor is measured and set.

## Decision Drivers

* Every budget is stated on one core of the reference test machine (ADR-080 item 1), in a Release build (DBG-208).
* One macro layer emits the markers, and no other code calls the PIX runtime (ADR-068, DBG-080).
* A development tool never ships, and no game program links one (`AGENTS.md` §2, ADR-090).
* CI's hosted runner cannot time anything (O69), but every step it can check, it checks (`AGENTS.md` §6).
* A result never changes run-time behaviour (DBG-208).

## Considered Options

* 1 — marker timing in-process behind `ProfileScope`, a tool project that runs the scenes on the game's libraries, and the script around it
* 2 — PIX timing captures of the markers as they are, exported by `pixtool` and read by the script
* 3 — a scene switch in `SpeciesServer.exe` with timers of its own

## Decision Outcome

Chosen option: "1".
- Option 2 ties the job to PIX's installation and its export format.
- Option 3 puts the scene code into a program that players receive, since the measured build is Release.

* **Marker timing** (`NeuronCore/MarkerTimings`). It is off unless a measurement turns it on; a marker then costs one relaxed load more.
  - While it is on, `ProfileScope` times the inside of its PIX event with the performance counter, so the event's own cost is not the work's.
  - Each thread records into a table of its own, in static storage: 64 threads and 64 names a thread, with relaxed atomics. A marker beyond either is not recorded. Recording takes no lock and never allocates, so the destructor that records cannot throw.
  - `TakeMarkerTimings` merges the tables by name text, between runs.
* **The region step's markers** (DBG-045 amendment).
  - `sim.region.commands`, and once in each slice: `sim.teams.advance_ungrouped`, `sim.laser_bolts.advance`, `sim.effects.advance`, `sim.bot_nodes.advance` and `sim.essence_orbs.advance`.
  - `sim.ai.step`, with `sim.ai.stray_recall`, `sim.ai.leader_policies` and `sim.ai.main_cycle` inside it.
  - `sim.routes.free_unheld`, beside the existing `sim.region.step`.
* **The performance tool** (`Tools/SpeciesPerformance`). It references GameLogic, GameShared, NeuronServer and NeuronCore, and is built in every configuration. So the script measures the `Release|ARM64` build of the libraries the server links.
  - A scene is one region on the valley of seed 1 under a fixed key. It is stepped on the calling thread, with no workers, as one reference core runs it. Each step is timed with marker timing on.
  - Its recorded command stream is what its AIs send, which the simulation makes again from the key and seed, plus the travel scene's two commands. So every run on one build does the same work.
  - It reads the cooked tuning record beside it, as the programs do (ADR-108), or the authored record that `--tuning-source` names. It writes JSON.
  - `--quick` runs every scene for a few ticks. CI uses it.

  | Scene | What it runs | Ticks |
  |---|---|---|
  | `sandbox-battle` | the sandbox of [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md), with the AI on both sides | 3,000 |
  | `synthetic-battle` | ADR-080 item 2's 2,000 figures: two halves `mvp.perf.synthetic_gap` (200 wu) apart across the valley's centre, an AI on each | 3,000 |
  | `region-bound` | ADR-080 item 10: a battle of 2 × 600 at site 0 and one at site 1, and two settlements of 600 at work at sites 2 and 3, standing in as wandering Workers of one team until M2 | 1,000 |
  | `at-work` | the two settlements at work alone | 1,000 |
  | `empty-region` | the valley with no figure | 1,000 |
  | `draws` | 10⁷ SipHash-2-4 hashes of a draw's five words. The tool hashes the words itself, because only the simulation makes draws (`CheckProjectFiles` rule 14) | — |
  | `travel` | WLD-100's eight edges inside the valley (the four valley neighbours, and each site with the centre) on the corpus of seeds 1 to 8. On each edge, 200 Workers take the steward's selection and crowd order along the land routes at the first tick, and walk for at most 6,000 ticks | — |

* **The figures.** The script derives each figure from the tool's result and judges it against ADR-080.

  | ADR-080 budget | Measured by |
  |---|---|
  | figure in a battle, 2.1 µs | the teams', laser bolts', effects' and orbs' markers ÷ the figure-ticks, in both battles |
  | Worker at settlement work, 1.2 µs | the teams' marker ÷ the figure-ticks, in `at-work` |
  | figure of an expedition, 0.4 µs | the teams' marker ÷ the figure-ticks, in `travel` |
  | a keyed draw, 0.03 µs | `draws` |
  | AI settlement or deputy, 60 µs (20 µs until O74) | `sim.ai.step` ÷ (ticks × AIs), in both battles |
  | command of one side of a battle, 30 µs | `sim.region.commands` ÷ (ticks × sides), in both battles |
  | region fixed cost, 30 µs | the empty region's mean step |
  | region step, 12.5 ms (item 3) | the 95th percentile of `region-bound`'s steps |
  | integrated server, 1 reference core (item 7) | the sandbox battle's mean step ÷ the tick period |

  A settlement at abstract fidelity, a figure materialised and a seen-layer stamp are not built at M1, and are measured when they are.
* **The script** (`Build\RunPerformance.py`).
  - It restores the packages and builds `Release|ARM64` through `Species.slnx`, then runs the tool.
  - It reads the configuration with CIM and the registry: processor, memory size, graphics adapter, driver version, display mode and OS build. The build's identity is the revision it was built from, marked when the tree is modified (the playtest README, until ADR-057).
  - It writes the performance section of `playtests/M1.md`. The first run adds the reference test machine as a table, with the same configuration as JSON in a comment that later runs read. Each run adds a table of every figure against its budget: within budget, within 25 %, or above.
  - A later run on a different configuration stops with exit code 2 and records nothing (DBG-209). After the pin, a figure above its budget fails the run with exit code 1; the run that pins only reports (DBG-208).
  - A build other than `Release|ARM64` is reported and never recorded.
  - `--self-test` runs the tool with `--quick`, then proves the record's four rules on fixtures in a temporary folder. CI runs it on the `Debug|x64` tool (ADR-070 amendment).
* **The travel factor.** For each Worker that arrived, the factor is its walking ticks × the tick period × its speed ÷ the straight distance it covered. A walk shorter than half its edge is not counted.
  - The script reports the largest per-edge median over the corpus against the record.
  - O71 set the factor from this ratio, so it is 1.05: the largest per-edge median is 1.0437 (seed 2, site 3 – centre), rounded up.

### Consequences

* Good, because the per-object costs come from the same markers PIX shows, so a PIX capture and the record divide the step alike.
* Good, because each run measures the same work, and the record refuses a changed machine.
* Bad, because `ProfileScope` holds two fields more and reads one atomic more. This tool times work inside the markers, not the markers' own cost; DBG-206's marker test measures that.
* Bad, because the synthetic battle no longer starts at the sandbox's sites (ADR-109 amendment): halves that start 2,000 wu apart did not meet in 3,000 ticks, so they measured a walk.
* Bad, because the AI already exceeds its budget on the harness build (below). The owner's run gives the figure that binds.
* It forecloses: reading marker times from PIX captures in the performance job; scene code in a program that players receive.

**The measurement in this task.** Neither table below was measured on the reference test machine, so neither binds a budget. Both come from the local harness: g++ 13 at -O1, on a 4-core 2.1 GHz Xeon under Linux, with the authored tuning record.
- **The travel corpus.** All 200 Workers arrived on all 64 edges. Per-edge medians run from 1.0032 to 1.0437, and the largest 90th percentile is 1.0485. The land routes that the terrain's check 8 measures are at most 1.0017 × the distance over seeds 1 to 64, so no seed re-rolls at 1.05. A walk is deterministic on one build and does not depend on the machine's speed. The owner's run measures it again on `Release|ARM64`.
- **The costs.**

  | Figure | Measured | Budget |
  |---|---|---|
  | figure in a battle | 0.27 µs (sandbox), 0.51 µs (synthetic) | 2.1 µs |
  | AI | 52 µs (sandbox), 118 µs (synthetic) | 20 µs |
  | command of one side | 0.67 µs, 6.2 µs | 30 µs |
  | Worker at work | 0.43 µs | 1.2 µs |
  | expedition figure | 0.22 µs | 0.4 µs |
  | region fixed cost | 9.8 µs | 30 µs |
  | region step at the bound, p95 | 2.4 ms | 12.5 ms |
  | keyed draw | 36 ns | 30 ns |
  | integrated server | 0.0024 cores | 1 core |

  - In the synthetic battle, the AI's time per tick (236 µs for its two AIs) is the stray recall's 137 µs and the Leader policies' 95 µs. Each of the stray recall's picks searches the nodes linearly (ADR-107).
  - The sandbox battle barely fights: 398 of its 400 figures remain after 3,000 ticks, ADR-107's stall. Its battle figure measures formation and walking more than combat.
  - The synthetic battle fights: 2,000 figures fall to 1,065.

### Confirmation

* `NeuronCoreTests` checks that a marker is timed only while timing is on, that nested scopes count, and that the times merge by name in name order.
* CI's step "Check the performance script" runs every scene for a few ticks, and the record's four rules on fixtures: the pin, a changed machine that stops the run, a pinned run within its budgets, and one above a budget that fails.
* The owner's first run on the reference test machine pins it and records every figure. ADR-080 is then confirmed or amended from them, by its levers in their order, and a figure outside O43's ranges goes to the owner (M1 plan T10).

## Pros and Cons of the Options

### 1 — marker timing, a tool and a script

* Good, because the record reads the same markers that PIX shows, in the build that is measured.
* Bad, because every marker reads one atomic more.

### 2 — PIX timing captures

* Good, because the markers stay as they are.
* Bad, because the job then depends on PIX's installation and export format, and a capture adds its own overhead to every tick.

### 3 — scenes in the server

* Good, because the server that players run is itself measured.
* Bad, because measurement code ships in Release.

## More Information

* Related: [ADR-068](ADR-068-profiling-and-gpu-timing.md), [ADR-070](ADR-070-continuous-integration.md), [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-094](ADR-094-arm64-development-build.md), [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md).
* The rules: DBG-045, DBG-080, DBG-206, DBG-208 and DBG-209 ([diagnostics](../architecture/components/diagnostics_test_and_performance/README.md)), WLD-100 ([world and sites](../gdd/01_world_and_regions/world_and_sites.md)), the [M1 plan](../implementation/m1_plan.md) §4 and T10.
* The code is in NeuronCore (`MarkerTimings`, `Profile`), GameLogic (`RegionSimulation`, `AiController`, `Sandbox`), `Tools/SpeciesPerformance` and `Build/RunPerformance.py`.
* **Amendment 2026-10-22 (owner decision O85, [ADR-126](ADR-126-frame-bench-and-rendered-scenes.md) amendment).** The reference test machine has no 1,920 × 1,080 monitor, so its own 2,880 × 1,920 panel is pinned for the rendered scenes, and the display rule of the amendment below is replaced.
  * **The display.** The first rendered run pins a 60 Hz display on which `sandbox-drawn` was drawn at 1,920 × 1,080: borderless on a display of that size, or in a window of that size on a larger one. A first rendered run that drew otherwise, or on a display not at 60 Hz, still stops and records nothing.
  * **The self-test.** CI's self-test now proves eighteen record rules: a first rendered run on a display not at 60 Hz pins nothing, and one in a window of 1,920 × 1,080 on a larger 60 Hz display pins it.
* **Amendment 2026-10-22 (M1's exit).** Three ADRs' confirmations put a measurement at M1 that T10 did not take, and the tool and the script now take them.
  * **Generation and the log flood.** The tool also times the generation of the valleys of seeds 1 to 100, and their land route graphs on their own ([ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md), [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md)). It also times a log call while eight threads flood the logger ([ADR-091](ADR-091-log-pipeline-buffers-and-registration.md)).
  * **The rows.** The script judges the log call against `dbg.budget.log_call_us` (DBG-206) together with ADR-080's budgets. It reports, without judging:
    * a valley's generation, against the per-region share of WLD-165's budgets;
    * the graph;
    * the route sweep;
    * the server's tick rate while the battle is drawn: the ticks measured, divided by the sum of the frames' intervals (MVP-085).
  * **The display.** The rendered scenes now pin the display mode that the bench reports for the primary display, in place of the machine's reading, which may give the mode of another display. A first rendered run on any display other than 1,920 × 1,080 at 60 Hz stops and records nothing, so a run on the laptop's own panel cannot pin it.
  * **The self-test.** CI's self-test now proves sixteen record rules.
* **Amendment 2026-10-21 (M1 task T18, owner decisions O75 and O79, [ADR-126](ADR-126-frame-bench-and-rendered-scenes.md)).** The script runs the frame bench's rendered scenes after the tool, unless `--headless-only` is given, and adds their rows to each run: the sandbox's battle drawn is judged by the 99th percentile of its frame interval against 1000 ÷ `mvp.client.fps_target` + 1 ms at 1,920 × 1,080, and its missed frames and the scene of `mvp.client.figures_max` figures are reported. Only the hardware stops a run: a processor, memory size or graphics adapter that differs from the pinned one stops it, and so T10's pin stays valid; the rendered scenes pin their display at their first run and stop when it differs; the driver, the OS build and the display are written with each run. CI's self-test runs the bench's quick run on WARP and proves eleven record rules on fixtures.
* **Amendment 2026-10-22 (owner decision O100).** The reference test machine is the owner's x64 laptop, so the script builds, runs and records `Release|x64` in place of `Release|ARM64`, and a build of another configuration or platform is still reported and never recorded. The machine's graphics adapter is every hardware adapter, in name order and joined by ` + `, with their drivers likewise, because the laptop's panel is driven by one adapter while the game draws on another; the rendered run's line names the adapter the bench drew on. The change of the machine re-baselines every result: the ARM64 pin and its runs stay in `playtests/M1.md` as history, and the next run pins the laptop. The self-test now has twenty fixtures, the new one recording the adapter drawn on.
* **Amendment 2026-10-23 (M3 task T9, [ADR-164](ADR-164-the-command-paths-confirmation-the-valley-run-and-the-valley-at-war-at-m3.md)).** The tool measures the valley at war, four settlements at `mvp.settlement.population_max` with two raids in progress, as `valley-battles`. The script judges its 95th-percentile step against the region step budget of ADR-080 item 3, and its AI settlements' decisions (`sim.stewards.step`) against an AI's 60 µs. The valley run of seeds 1 to 8, `--scenes valley`, runs only when named, its seeds on threads of their own. `Build\RunPerformance.py --valley` runs it, judges Playtest 1's measurable lines over the corpus, and records them in `playtests/M3.md`. The self-test runs the quick valley run and proves its judgement and record on fixtures.
