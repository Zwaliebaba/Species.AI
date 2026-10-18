# Performance baseline

> Status: Engine baseline · Prefix: DBG · ADRs: [ADR-066](../../../ADR/ADR-066-telemetry-with-consent.md) · [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md) · [ADR-070](../../../ADR/ADR-070-continuous-integration.md) · Folder: [diagnostics, test and performance](README.md) · Related: [tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [renderer options and budgets](../renderer/options_and_budgets.md) · [build tooling and distribution](../build_tooling_and_distribution.md)

This document holds the **reference performance figures** of the engine: the frame and tick structure that they assume, the population per map, the hotspot catalogue, the stress populations, the Species budget baseline and the reference test machine. They are the starting point of the Species design phase.

## 1 Responsibilities and boundaries

This document holds item 12 of DBG-001 ([README](README.md) §1). Each component owns its own budgets; this document collects the baseline and the performance tests that report against it (DBG-208).

## 2 Interfaces

This document has no interface of its own. The performance tests run in the test harness (DBG-141, [test harness, soak test and telemetry](test_harness_soak_and_telemetry.md) §4.8).

## 3 Data model

The figures and budgets are the tables of §5.2 to §5.4; the reference test machine is §5.5.

## 4 Behaviour and algorithms

This document specifies no behaviour of its own: the performance tests that report against these budgets are run by the test harness (DBG-141, DBG-208).

## 5 Concurrency and performance budget

### 5.2 Frame and tick structure of the reference

**DBG-201** (MUST) The reference frame and tick structure, which the reference performance figures assume, is specified in [tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) and summarised here:

| Quantity | Reference value | Key |
|---|---|---|
| simulation tick | one per `ref.sim.tick_period` (0.1 s), so 1 ÷ `ref.sim.tick_period` per second | `ref.sim.tick_period` |
| team control state send period | one tick period | `ref.sim.frame.control_send_period` |
| update slices per tick | 10, so a render frame runs part of a tick | `ref.sim.update_slices` |
| slices per render frame | ⌊0.3 × pending slices⌋ clamped to 0–20, plus the rest of a tick in progress; 1 to 3 pending slices advance nothing | `ref.sim.pacing.gain`, `ref.sim.pacing.max_slices_per_frame` |
| catch-up | more than 20 frames behind: 20 slices per iteration until caught up or 0.3 s of real time is spent in the frame | `ref.sim.pacing.catch_up_threshold`, `ref.sim.pacing.catch_up_slices`, `ref.sim.pacing.catch_up_budget` |
| presentation frame delta | at most 0.25 s | `ref.sim.frame.delta_clamp` |
| deferred work | one deferred job per loop iteration | `ref.sim.frame.deferred_jobs_per_iteration` |
| memory report | every 60 s to the log | `ref.sim.frame.memory_log_interval` |
| frame-time range inspected | 50, 100 and 200 ms per frame | `dbg.profile.view.graph_marks_ms` |

The cost of one tick must fit, spread over its 10 slices, into the render frames of one tick period. When it does not, the participant falls behind, catch-up spends up to 0.3 s of each frame simulating, and the frame rate falls further: a spiral bounded only by that cap. Species bounds catch-up in ticks per frame and shows a catching-up state instead (DEC-225).

### 5.3 Reference performance figures

**DBG-202** (MUST) No measured frame time, tick cost, draw count, particle count or per-system cost exists for the reference (DEC-106). The reference figures are its limits and the structure of its work:

| Figure | Reference value |
|---|---|
| bodies per map | the population cap of each map ([map rules and conventions](../../../reference/08_maps_and_level_design/rules_and_conventions.md)) |
| bodies per team | the map cap ÷ the open slots (R-ECON-052 rule 3, DEC-079): 350 to 1,000 bodies on the reference maps; in Siege, a defender cap of 200 to 350 and an attacker share of 550 to 1,725 (rule 2). Transit orbs in flight, Transit Gate waves and the sources that R-ECON-059 does not check, Supply crate outcomes, Builder deliveries and conversions among them, can take a team over its cap; a Collector delivery cannot (DEC-080, R-ECON-060) ([population flow](../../../reference/05_population_flow.md)) |
| engine population limit | none: only the map and mode options limit the population; the live simulated-object count is kept for display |
| Essence orbs | one per death, none for the Rogue faction; each is collectable for `ref.econ.orb.floating_ticks` (301 to 500 ticks, 30.1 to 50 s) and then ascends for `ref.econ.orb.ascent_ticks` (901 ticks, 90.1 s), so thousands exist after a large battle |
| laser bolts | every Worker shot is a simulated object; with a 3 s reload, 2,000 Workers in combat create up to about 670 laser bolts per second |
| Worker task searches | every Worker searches every `ref.unit.worker.retarget_ticks` (11 to 20 ticks), staggered: about 1,000 to 2,000 full searches per second at 2,000 Workers ([Worker](../../../reference/02_units/worker.md)) |
| simulated objects at peak | about 2,000 to 6,000: units, Essence orbs, laser bolts, effects and structures |
| network traffic | commands only; independent of the population (DEC-201) |

**DBG-203** (MUST) The reference hotspots, which the Species budgets and the performance scenes cover; the last column names the markers of §3.4 and the outputs that measure each:

| # | Hotspot | Mechanism and figures | Measured by |
|---|---|---|---|
| H1 | population | Workers dominate the object count; no engine cap; caps per map as DBG-202 | the per-kind markers of `sim.teams.advance_ungrouped` and `sim.actor_group.advance`; the peak population of the soak report (DBG-166) |
| H2 | Worker task searches | each Worker re-runs its task search every 11 to 20 ticks: threats, ports, carryable structures, Supply crates, Transports, flags, Leaders and a random position, with per-tick movement and pushes away from the sea and structures | `sim.worker.task_search` and the other `sim.worker.` markers |
| H3 | spatial queries | actor-grid cells of 32 wu; obstacle-grid cells of 32 wu in a match and 64 wu outside; every search of H2 and H12 queries the grids, and re-used recent results, with their staleness and range shortcuts, change results (DEC-094) | `sim.worker.search.threats` with its steps, and `sim.crawler.search.enemies` |
| H4 | Essence orbs | every death adds one orb and collection is the only sink, so the orb count grows while fighting outpaces collection; the orbs advance in update slices | `sim.essence_orbs.advance`, `sim.essence_orb.push_from_structures` and `render.essence_orbs` |
| H5 | laser bolts and effects | every Worker shot is a laser bolt, so the bolt count grows with the Workers in combat; every effect advances on every tick | `sim.laser_bolts.advance` and `sim.effects.advance` |
| H6 | particles | particle creation has no limit and is refused only while the match is paused; all particles advance in slice 0 of a tick; tree fire particles ÷ the structure detail; the Species pool has a fixed capacity (ADR-010) | `effects.particles.advance` and `effects.particles.render` |
| H7 | shatter effects | up to about 501 source triangles scanned per fragment; fragments with a perimeter below 6 skipped ([animation and VFX](../animation_and_vfx/README.md)) | `effects.shatter.advance` and `effects.shatter.render` |
| H8 | draw submission | one draw submission per Worker, with a high-detail factor 1 − d² ÷ D², D = 99,999.9, 1,000 or 500 wu for entity detail 1, 2 or 3; structures drawn one by one; transparent structures sorted by depth every frame | `render.workers`, `render.structures` and `render.structures.transparent_sort` |
| H9 | terrain | one full mesh with no level of detail, drawn in two passes (surface and wire overlay) | `render.terrain.surface` and `render.terrain.wire_overlay` (RND-453) |
| H10 | sea | the coastline wave mesh rebuilt on the CPU every frame; sea detail 1 to 3 sets its resolution, 0 draws the flat layer only | `render.sea`, `render.sea.advance` and `render.sea.wave_build` |
| H11 | structures | one marker per structure kind for advance and draw; decorative counts by structure detail (a Spawn Node draws 20, 10 or 3 glow blobs) | the per-kind markers of `sim.structures.advance` and `render.structures` |
| H12 | Crawlers and creatures | a trail-sample history per Crawler; per-state searches | the `sim.crawler.` markers, `sim.centipede.search_essence_orbs` and `sim.spider.search_essence_orbs` |
| H13 | bot placement loops | several bot target-area loops draw positions until one is valid, with no iteration limit: deterministic, but a frame hitch, or a hang on every participant, when an area has few or no valid points; DEC-014 bounds them | the simulation update-time percentiles of the soak report (DBG-166) |
| H14 | audio | a 20 Hz logic update (`ref.sim.frame.audio_update_period`) with sort and priority passes, on the audio engine's own thread whatever the frame rate (AEN-006) | `audio.update` and its step markers (AEN-322) |
| H15 | log output | one log line per frame in some states; the asynchronous log of §4.1 (ADR-067) keeps writing it off the frame | the log page of the overlay (DBG-062) and the logging budgets of DBG-206 |
| H16 | network | no bandwidth limiting or adaptation (DEC-207) | `net.host.send` and `net.host.update_clients` |

### 5.4 Species budget baseline

**DBG-204** (MUST) The design population is `dbg.stress.design_population` (2,000 units per map, plus Essence orbs and projectiles); the stress population is `dbg.stress.population` (4,000 units), tested on an uncapped stress test map (DBG-148) with `dbg.stress.teams` (4) teams × `dbg.stress.start_units` (150) starting units and `dbg.stress.spawn_nodes` (24) Spawn Nodes. The population cap is a hard engine limit, not only a map option. These are the defaults of DEC-228 and design-phase inputs: the design phase confirms or replaces both populations.

**DBG-205** (MUST) The per-tick CPU budget is a design-phase input (DEC-227, DEC-228). The Species starting guidance (DEC-228) is a hard budget at the design population on the reference test machine (DBG-209) of `dbg.budget.tick_share_guidance` (25 %) of a frame at `rnd.budget.target_frame_rate` (16.7 ms), about 4.2 ms, split into per-system budgets for the hotspots H1 to H6; the design phase states whether the figure is the cost of a whole tick or a per-frame share of it (DEC-228 item 1), and a server budget per region on a named community-host specification (DEC-228 item 7). Measurement precedes commitment: the first simulation-core milestone measures the tick cost of a synthetic 2,000-unit match (DEC-106, DEC-229), the per-system budgets are then recorded in this section, and the scheduled soak job reports the tick-cost percentiles and the peak population (DBG-166).

**DBG-206** (MUST) The diagnostics budgets, on the reference test machine: a log call costs at most `dbg.budget.log_call_us` (1 µs) on the calling thread, and eight threads logging 10⁶ debug records raise the frame time by at most `dbg.budget.log_flood_frame_ms` (0.5 ms); all markers of a frame cost at most `dbg.budget.markers_frame_ms` (0.2 ms) in a Release build; the shown overlay page costs at most `dbg.budget.overlay_ms` (0.5 ms) of presentation-thread time.

**DBG-207** (MUST) The Species budget baseline per area, each owned where stated:

| Area | Baseline | Owner |
|---|---|---|
| simulation tick | a fixed tick counted in ticks, 20 to 30 per second recommended (DEC-227), with the budget of DBG-205 | design phase |
| population | DBG-204 | design phase (DEC-228) |
| searches and grids | keep the staggered 11-to-20-tick task search; spatial queries exact, with no cache shortcut that changes a result, data-oriented (structure of arrays, vectorised over cell lists), with a query budget per tick | [space, routing and physics](../../../reference/12_simulation_model/05_space_routing_and_physics.md), DEC-094 |
| particles and visual effects | fixed-capacity pools with a hard cap and graceful level of detail, wholly outside the simulation | [animation and VFX](../animation_and_vfx/README.md), ADR-010 |
| rendering | instanced drawing, a fixed number of calls per instance type and pass, a per-frame budget; 60 frames per second at the design population on the reference test machine (DBG-209) | RND-454, RND-455, RND-456 |
| catch-up | bounded in ticks per frame and shown to the player as a catching-up state | DEC-225 |
| logging | asynchronous and buffered, per-category levels, no file opened per line | ADR-067, §4.1 |

**DBG-208** (MUST) Performance tests run in Release builds on the reference test machine in the performance job, a scheduled CI job that ADR-070 adds by amendment and that is guarded like the committed jobs, together with the build budgets of [build tooling](../build_tooling_and_distribution.md) table 5-A (BLD-117, ADR-070), and report against these budgets: the design-population scene (a 2,000-unit bot-only Spawn Control match, rendered and headless) against DBG-205 and RND-454; the stress scene against DBG-204 and RND-456; the logging and marker tests of DBG-206; and the components' own budgets: the asset budgets of table 5-A (AST-125), the audio update (AEN-303, test T27), the UI budgets (UI-240), the input budgets (INP-200), the camera budgets (CAM-120) and the data budgets (DAT-050). Once the reference test machine is pinned (DBG-209), a result above its budget fails the performance job; until then the job reports it and does not fail. A result never changes run-time behaviour.

### 5.5 Reference test machine

**DBG-209** (MUST) The reference test machine is one pinned and recorded configuration: a processor, memory size, graphics adapter, driver version, display mode and OS build that meet the minima of ADR-050 and ADR-002. Every performance budget of the component specifications refers to it, the data budgets `dat.perf.*` of the data schemas among them; a budget stated on the minimum CPU, on minimum-specification hardware or on the minimum-specification machine is stated on this machine. A change of the machine re-baselines every recorded result. Its named hardware is a design-phase input (DEC-228 items 6 and 7, DEC-971). Until the design phase names it, this interim rule applies (DEC-971): the first machine that meets the minima of ADR-050 and ADR-002 and runs the performance job is pinned and recorded as the reference test machine; until that machine is pinned, every performance test runs, reports its result with the build identity and the hardware of the agent that ran it, and does not fail. The reference developer machine, on which the build budgets of build tooling table 5-A are measured, is `dbg.machine.developer_class`: the reference test machine's processor and memory with a local solid-state disk.

## 6 Failure modes, diagnostics and telemetry

Once the reference test machine is pinned, a result above its budget fails the performance job; until then it is reported and does not fail (DBG-208, DBG-209). A result never changes run-time behaviour.

## 7 Acceptance tests

| # | Kind | Test | Requirements |
|---|---|---|---|
| T10 | performance | All markers of the design-population frame cost at most the marker budget in a Release build. | DBG-206 |
| T29 | performance | The design-population scene and the stress scene report frame and tick percentiles and peak population on the reference test machine; the component budget tests of DBG-208 run in the performance job, each result recorded with the build identity and the hardware of the agent that ran it; before the machine is pinned, a result above its budget is reported and the job passes, and after it the job fails. | DBG-204, DBG-205, DBG-208, DBG-209 |

## 8 Open decisions

DEC-106, DEC-227, DEC-228, DEC-229 and DEC-971 (Open) apply to this document; their defaults are listed in the [README](README.md) §8.1, with the design-phase inputs of the whole component.

## 9 Tuning table

A value whose notes say "Species" is a Species engine-baseline value; every other value is the reference tool's value, kept as the starting value (DEC-004).

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `dbg.budget.log_call_us` | 1 | µs per call | Species; calling-thread cost, reference test machine | DBG-061, DBG-206 | — |
| `dbg.budget.log_flood_frame_ms` | 0.5 | ms | Species; frame-time increase while 8 threads log 10⁶ debug records | DBG-206 | — |
| `dbg.budget.markers_frame_ms` | 0.2 | ms per frame | Species; every marker of a frame, Release build | DBG-206 | — |
| `dbg.budget.overlay_ms` | 0.5 | ms per frame | Species; presentation-thread cost of the shown overlay page | DBG-206 | — |
| `dbg.budget.tick_share_guidance` | 0.25 | share of a frame | Species; design-phase input: 25 % of a 16.7 ms frame, about 4.2 ms per tick | DBG-205 | DEC-228 |
| `dbg.stress.design_population` | 2000 | units per map | reference peak, plus Essence orbs and projectiles | DBG-204, DBG-205 | DEC-228, DEC-106 |
| `dbg.stress.population` | 4000 | units | twice the design population; design-phase input | DBG-204 | DEC-228 |
| `dbg.stress.teams` | 4 | teams | stress map | DBG-204 | DEC-228 |
| `dbg.stress.start_units` | 150 | units per team | stress map starting forces | DBG-204 | DEC-228 |
| `dbg.stress.spawn_nodes` | 24 | Spawn Nodes | stress map | DBG-204 | DEC-228 |
| `dbg.machine.developer_class` | the reference test machine with a local solid-state disk | machine class | Species; exact hardware a design-phase input; BLD table 5-A | DBG-209 | DEC-228, DEC-971 |
