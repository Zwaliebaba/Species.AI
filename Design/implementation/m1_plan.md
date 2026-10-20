# M1 plan

> Status: Final; agreed with the owner on 2026-10-20 (O69, O70), and M1 starts with task T2 · Related: [MVP scope](mvp_scope.md) §3 (M1) · [M0 plan](m0_plan.md) (how the work flows) · [MVP assets](mvp_assets.md) · [owner decisions](../decisions/owner_decisions.md) (O43, O58, O69, O70) · [ADR-080](../ADR/ADR-080-per-tick-budgets-and-population-caps.md) (the budgets M1 measures) · [ADR index and technology baseline](../ADR/README.md)

## 1 Purpose

This plan turns milestone M1 of the [MVP scope](mvp_scope.md), the battle sandbox, into work an implementing session can start cold. It covers:
- what M1 owes, beyond the scope's own row (§2);
- the order of the work, and why the simulation comes first (§3);
- the reference test machine and the performance script (§4);
- the tasks of M1, each one or more changes on M1's one pull request (§5, O71);
- the sandbox the steward plays (§6);
- what is carried, open or interim at M1 (§7);
- what the owner does during M1 (§8).

How work flows between the cloud session and the owner's PC is unchanged from the [M0 plan](m0_plan.md) §2: CI is the compiler, the owner runs what touches presentation, and nothing is claimed that was not done.

M1 ends when the exit criteria of the MVP scope's §3 hold:
- a battle of 400 figures runs at 10 ticks per second and the client at 60 frames per second (MVP-085);
- the reference test machine is pinned and recorded (O58, O69), the per-figure costs are measured on it and ADR-080's budgets are confirmed or corrected (O43);
- the steward finds a battle fun to watch and to steer, recorded in the session log (§6.3 of the scope).

## 2 What M1 owes

Besides the scope's row, the ADRs put these items at M1, most of them confirmations of Proposed records. M1 builds what each one needs and records the evidence in `playtests/M1.md`:

| Record | What M1 shows | Task |
|---|---|---|
| [ADR-080](../ADR/ADR-080-per-tick-budgets-and-population-caps.md) | the 400-figure battle on the reference test machine, and in the test runner a synthetic battle of 2,000 figures and a region at its bound, each within 25 % of item 2's budgets, or the provisional values amended within O43's ranges, before any content is tuned | T10 |
| [ADR-081](../ADR/ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](../ADR/ADR-082-counter-based-simulation-random-draws.md), [ADR-097](../ADR/ADR-097-keyed-draws-and-state-hash.md) | identical per-tick hashes of the sandbox and of the synthetic battle at 1, 2 and 8 worker threads; a cosmetic effect leaves every hash unchanged; a draw added to one purpose leaves every other purpose's values unchanged; the SipHash-2-4 test vectors and the quality test at 10⁸ values | T9 |
| [ADR-083](../ADR/ADR-083-server-process-ownership-and-threading.md) | a forced stall of 2 s caught up within 2 s at most twice the rate, and one of 30 s recorded as a freeze (O68) | T9 |
| [ADR-084](../ADR/ADR-084-client-boundary-through-the-transport.md) | a recorded command stream gives the same per-tick hashes on the headless server with presentation stubs and with the integrated client attached | T12 |
| [ADR-095](../ADR/ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-096](../ADR/ADR-096-game-skeleton-integrated-server-and-tick-report.md) | the first client command with its outcome message; replication in place of the tick report | T7, T12 |
| [ADR-098](../ADR/ADR-098-tuning-in-data-and-the-schema-generator.md) | the cooked tuning record, which ends the server's interim parse of the authoring file (O67) | T11 |

*Table 1 — What the ADRs put at M1, and the task that produces each.*

Two of them pull a later rule forward in a first form. ADR-084's test needs a recorded command stream, so M1 builds the command journal of MVP-076, which the scope states as a SHOULD. ADR-083's freeze is a log record at M1; the world's freeze record arrives with M2's world record, and the freeze records of MVP-071 with M4.

The likeness review of PIL-027 also falls at the end of M1, for the Worker and Leader silhouettes ([MVP assets](mvp_assets.md) §4).

## 3 The order of M1

**The simulation comes first, headless and measured, and the look follows.** O43 says the costs are measured before any content is tuned. ADR-080's per-figure budget is itself uncertain by a factor of 6 (DBG-205 read as the cost of a whole tick or as one frame's share of it). If the measured cost lands at the expensive end, the population limit, the side cap and `mvp.settlement.population_max` move, and with them everything M2 tunes. Finding that after the renderer work is the expensive order.

So the tasks fall in two phases:
- **Phase A, T2 to T10**, builds the battle as a headless simulation. CI tests every task, and T2 to T9 need no run checklist from the owner. It ends with the owner's first run of the performance script (T10), which pins the reference test machine and settles ADR-080.
- **Phase B, T12 to T18**, makes the battle visible and steerable. Every task states a run checklist (M0 plan §2) and is not merged before the owner's answer.

T11, the content build, depends on nothing in phase A and runs beside it. Phase B's tasks follow the table's order.

## 4 The reference test machine and the performance script

The owner's PC is the reference test machine (O58): a Windows on Arm PC with a Snapdragon X processor and an Adreno X1-85 GPU. It measures the `Release|ARM64` development build, natively (O69, [ADR-094](../ADR/ADR-094-arm64-development-build.md)). That machine does not meet ADR-050's x64 minimum, so its figures bind the MVP's budgets only. The x64 build that ships is not measured until the minimum-specification tier, decided before the first external build, re-baselines every result (DBG-209 amendment).

The frame-rate scenes run on a display whose native mode is 1,920 × 1,080 at 60 Hz, so the borderless window (O65) is exactly MVP-085's target.

The performance job of DBG-208 is, for the MVP, a script that the owner runs on that machine: `Build\RunPerformance.py` (O69). CI's hosted runner is a virtual machine and cannot time anything, and a self-hosted runner was not chosen. The script:
- builds `Release|ARM64` through `Species.slnx`, as `AGENTS.md` §3 requires;
- runs the measurement scenes (T10, T18), each from a fixed seed with a recorded command stream, so that every run measures the same work;
- reads the machine's configuration: processor, memory size, graphics adapter, driver version, display mode and OS build;
- writes the configuration, the build's identity as the playtest README defines it until ADR-057's identity exists, and each figure against its budget into the performance section of `playtests/M1.md`.

Its first run pins the machine. Every later run compares the configuration with the pinned one and stops on a difference, because a change of the machine re-baselines every result (DBG-209). Once the machine is pinned, a result above its budget fails the run.

## 5 Tasks

The tasks land in this order on one pull request for the whole of M1, each as one or more changes that cite the requirements they implement (O71, as O67 did for M0). A task that adds a tuning value adds its key to the MVP scope's table 3 and to the tuning record in the same change (MVP-003, MVP-099).

| Task | Builds | Done when | Ids |
|---|---|---|---|
| **T1 Plan** | this plan; owner decisions O69 and O70; MVP-017 and the routes of MVP-040; the amendments of DBG-208, DBG-209, ADR-017 and ADR-080; the M0 plan's §4 and §6 and the MVP assets' milestones | the design checks pass | O58, O69, O70, MVP-002 |
| **T2 Update slices and figures** | `RegionSimulation`'s tick in the reference order: the tick's commands, the clock, then slices 0 to 9 with the world step's serial order and the slice pools (DEC-455's stated behaviour); the actor pool with stable ids; Workers and Leaders with their stats and integer damage at DEC-115's fixed research levels; the game's `DrawPurpose`; the state hash over every figure | unit tests: each object advances exactly once per tick, the slices run in order, and the golden hash changes by a reviewed change only | MVP-092, MVP-093, R-SIM-500 to R-SIM-516, R-UNIT-011 to R-UNIT-033, DBG-147 |
| **T3 Terrain library** | the generator of WLD-160 to WLD-162 for one region in NeuronCore (O60), with one template, one biome and one lighting preset, no set pieces and no bridges; the `wld.*` keys it reads, added to the tuning record; WLD-163's checks 1 and 3 to 6, with a failure re-rolling the seed (MVP-011); WLD-162's method 3 and flatten types 5 and 6, which the terrain component lacks (an amendment of TER-035); the region's own grid origin (CAM-095 amendment); generation outside every tick (TER-242); the terrain hash compared at connection, in the form that the task's ADR chooses between TER-252's and WLD-166's | a seed gives the same heights and hash on two runs and through the server's and the client's calls; every check has a fixture that passes and one that fails | MVP-005, MVP-011, WLD-160 to WLD-163, TER-009 |
| **T4 Grids and movement** | the actor grid and its enemy cache (DEC-094's stated behaviour); the obstacle grid; integration, the airborne, water and push steps; the movement of Workers and Leaders with the slope's speed factor | unit tests of query results and their stable order, of the push loop's bound (R-SIM-382), and of a crowd of 200 walking to a point on generated terrain | MVP-040, R-SIM-300 to R-SIM-329, R-SIM-360 to R-SIM-389, R-UNIT-040 to R-UNIT-042, R-UNIT-105 to R-UNIT-115 |
| **T5 Navigation and routes** | the navigation nodes of MVP-017 with `mvp.nav.node_spacing` in the tuning record; the path tests; the land route graph, its next-hop tables and route building (DEC-263's and DEC-651's stated behaviour); route following; WLD-163's check 8 | routes between every pair of site centres on a corpus of seeds; the same nodes and routes on two runs; check 8 has a fixture that passes and one that fails | MVP-017, MVP-040, R-SIM-330 to R-SIM-359, R-UNIT-110 |
| **T6 Combat, death and orbs** | the threat search; combat; the laser bolt; the grenade with its explosion and shockwave; burning; death and corpses; Essence orbs outside every territory; the effect events the client will draw (ADR-036, ADR-084 item 5) | unit tests of each rule's acceptance vectors; two crowds of 200 on the threat search alone fight to an end headless with a stable hash | MVP-040, R-UNIT-098 to R-UNIT-104, R-UNIT-079, R-UNIT-134, R-UNIT-034 to R-UNIT-039, R-UNIT-301 to R-UNIT-329, R-CRATE-405, R-CRATE-406, R-CRATE-410, R-CRATE-411, R-CRATE-415 to R-CRATE-418, R-CRATE-440 to R-CRATE-445, R-CRATE-470 to R-CRATE-473 |
| **T7 Leaders, formations and commands** | promotion as a command, since the MVP has no Powers; joining a Leader; the formation block and its slots; Leader orders, markers and trail marks; the battle verbs of §6 as commands through ADR-087's checks, each with its outcome message (ADR-095); the AI's own actions as command types that only an AI sender may send (DEC-122) | unit tests of every verb through the command queue, with every refusal counted and every outcome carrying its tick | MVP-040, MVP-094, MVP-101, R-UNIT-096, R-UNIT-140, R-UNIT-152 to R-UNIT-189 |
| **T8 Simple AI** | the reference bot's formation-Leader core and its dispatch with random target selection and the stray recall, with the node reads they need (O70), reading the whole battle until M3, at the reference cadences, and acting only through the command path at the next tick | the AI against itself, 200 a side, runs headless to a battle's end; the same hashes at 1, 2 and 8 worker threads | MVP-049, MVP-058 (interim), MVP-094, R-BOT-049 to R-BOT-052, R-BOT-076, R-BOT-078 to R-BOT-088, R-BOT-091, R-BOT-092, R-UNIT-188 |
| **T9 Sandbox, journal and determinism** | the sandbox world of §6 with its `mvp.sandbox.*` keys and the seed switch (an amendment of the platform layer's switch table); the command journal in its first form, every command since the world started, replayed on the same build; the determinism and draw tests of table 1; ADR-083's stall tests, on an injected clock so that CI's runner times nothing | CI runs every test of table 1's T9 rows and they pass | MVP-005, MVP-076, MVP-093, ADR-081, ADR-082, ADR-083 |
| **T10 Performance script and the cost measurement** | `Build\RunPerformance.py` (§4); the headless scenes: the sandbox's 400-figure battle with the AI on both sides, the synthetic battle of 2,000 figures, and a region at its bound with wandering Workers standing in for settlements at work until M2; the travel scene, in which 200 Workers walk each valley-neighbour edge and each site–centre edge of the valley on a corpus of seeds; the per-object costs read from the profile markers (ADR-068) | the owner's first run pins the machine; `wld.travel.valley_path_factor` is set from the travel scene's walking time × the Worker's speed ÷ the straight distance (O71, WLD-100); every cost is within 25 % of ADR-080 item 2, or ADR-080 is amended with the measured figures and the population limit, the side cap and `mvp.settlement.population_max` are set within O43's ranges, by ADR-080's levers in their order; a figure outside those ranges goes to the owner | O43, O58, O69, O71, ADR-080, DBG-208, DBG-209, WLD-100 |
| **T11 Content build** | `Tools/SpeciesContentBuild`, a tool project with no third-party code: the record cooker, whose output the server reads in place of the interim parse; the check that packs each texture as authored (O61, O64); the font packer for both faces (O56, O63); the CMO importer and mesh cooker for `LeaderFigure.cmo` and `ThrownWeapon.cmo` (O64, O69); the Debug loose-content mount of the cooked output; a CI step that runs it | cooked output is identical on two runs; the server starts from the cooked record; a texture off its row, a damaged mesh and a damaged glyph table are each refused with a message that names the file and the fault | MVP-086, MVP-099, AST-105, DAT-010, DAT-050, ADR-017, ADR-019, ADR-061, ADR-062, ADR-098 |
| **T12 Replication** | per-client deltas against an acknowledged baseline and the snapshot join, for the whole battle until the seen layer exists (M3); the client's replica of the two latest ticks, drawn one tick behind with interpolation, held up to 250 ms and never extrapolated; presentation events; the tick report retired | ADR-084's M1 test passes; the owner runs it: the title shows each side's figure count from the replica | MVP-090, MVP-097 (interim), ADR-084, ADR-086 |
| **T13 World frame and terrain** | the frame infrastructure: the pass table, the pipeline-state catalogue with its blend states, the root signature, barriers, the upload ring, scene targets, multisampling and reversed depth; the world frame's passes that the look needs (RND-067 amendment); camera-relative transforms; the terrain's facets from `terrain.ramp`, the wire grid (`fx.terrain_grid`), the sea ring and shore waves (`terrain.sea_tile`, `terrain.shore_wave_gradient`), the sky (`fx.sky_cloud`), fog (O59) and the lights; a fixed development view | the owner runs it: the valley of a seed is drawn, the same for the same seed, with the debug layer silent | MVP-080, MVP-081, MVP-100, ADR-003 to ADR-008, ADR-011, ADR-013, ADR-015, ADR-016, RND-416 to RND-428, WLD-246 |
| **T14 Input, camera and cursor** | the input router, which replaces O65's stand-in; the player camera and picking, bounded by the region while the sandbox has no territory; the game cursors and their shadows (`cursor.world`, `cursor.highlight`, `cursor.unavailable`) | the owner runs it: flies the valley with the reference keys and wheel, and the cursor follows the pointer and changes over a Leader | MVP-100, MVP-101, ADR-028, ADR-029, R-CTRL-450 to R-CTRL-497, CAM-090 to CAM-093 |
| **T15 Development overlay** | EditorFont text at whole-number scales with point sampling (O62, O63); the frame-rate line, the frame page, the budget rows and their toggle through the input router; the tick cost, each side's figures and the seed; the sandbox's restart | the owner runs it: the overlay toggles, and its figures agree with the log | MVP-005, MVP-100, DBG-095 to DBG-099 |
| **T16 Crowds drawn** | the Worker cards, instanced (`sprite.worker.front`, `sprite.worker.side`, `sprite.worker.hurt`), with their glow and blob shadows (`fx.glow_round`) and procedural animation; the Leader figure and banners (`mesh.unit.leader_figure`, `marker.leader_banner`, `marker.leader_banner_tint`) in their side's colour; laser bolts, muzzle flashes, grenades, deaths, corpse fades and particles (`fx.beam_short`, `fx.muzzle_flash`, `mesh.projectile.thrown_weapon`, `fx.particle_dot`); orbs (`fx.glow_round`, `fx.glow_cloudy`); `fx.starburst` for Leader orders | the owner runs it: a battle of 400 between two AI sides, seen walking, fighting, falling, fading and rising as orbs | MVP-080, MVP-082, ADR-009, ADR-010, ADR-020, ADR-035, ADR-036 |
| **T17 Battle verbs** | the client's side of T7's verbs with the reference bindings; the selection circle; the order markers, trail marks, route arrows and flag pointer (`marker.order_trail`, `marker.route_arrow`, `marker.flag_pointer`); the edge compass and team discs (`marker.edge_compass`, `marker.edge_compass_empty`, `marker.edge_compass_shadow`, `marker.team_disc`, `marker.team_disc_shadow`); the cursor badges (`icon.power.workers`, `icon.power.leader`) | the owner runs it: promotes Leaders, forms them up and steers a crowd against the AI | MVP-101, R-CTRL-042 to R-CTRL-044, R-CTRL-073, R-CTRL-155 to R-CTRL-161 |
| **T18 Exit** | the performance script's rendered scenes: the sandbox's battle drawn at 1,920 × 1,080 against `mvp.client.fps_target`, and a scene of `mvp.client.figures_max` figures, reported; the owner's sessions with the sandbox; the likeness review of the Worker and Leader silhouettes; the M1 record | the exit criteria of the MVP scope's §3 hold, each with its evidence in `playtests/M1.md` | MVP-002, MVP-085, PIL-027 |

*Table 2 — The tasks of M1, in order.*

Phase B starts after T10, so that a budget the measurement moves is settled before the look is built around crowds of that size.

## 6 The sandbox

The sandbox is what `Species.exe` starts at M1, until the front end exists (MVP-103):

- **The world** is the MVP's valley (MVP-010, MVP-011), generated from a seed that a development switch sets or that the game draws, shown in the development overlay (MVP-005). It has no settlement, territory, store or structure; the four plateaus are empty but for the two sides.
- **The sides.** The steward's side and the AI's side each start as `mvp.sandbox.side_workers` (200) Workers, with no Leader, around the centres of two diagonally opposite settlement sites (O70), each in its Cladan's colour. The steward promotes Leaders in play: a formation holds at most `ref.sim.team.formation_size_upgrade` (90), so a side of 200 needs at least three. The AI promotes by its own rule (T8).
- **The steward's verbs** are the reference's, with no widget (O70): promote a Worker, select and deselect a Leader, toggle its formation, order it (with a straight route on Ctrl), send it alone (Shift), face it with the pointer, demote it, and the selection circle with the crowd order. The camera is the reference player camera (R-CTRL-450 to R-CTRL-497).
- **Orbs rise.** No territory exists, so every orb follows the reference's rules for an orb outside every territory and ascends.
- **A battle ends** when one side has no figure left. The battle rules of MVP-045, the loss cap and renown are M3's. A development command restarts the sandbox, with the same seed or a new one.

The number of sides, their size and their sites are tuning keys under `mvp.sandbox.*`, which T9 adds to table 3 of the scope and to the tuning record.

## 7 Carried, open and interim at M1

- **Terrain.** T3's ADR chooses the terrain hash's form (TER-252 or WLD-166). WLD-162 uses a method and two flatten types that TER-035 lacks, so T3 amends the terrain component. WLD-163's check 7 needs the yields of the home resource sites and waits for M2; check 8 runs from T5. MVP-011's re-roll means the seed shown is the one that passed every check.
- **Open DECs.** M1 builds the stated behaviour of DEC-094, DEC-115, DEC-122, DEC-263, DEC-455, DEC-557 and DEC-559, and DEC-651's resolution: a region frees the routes nothing holds (O71). M1 is the first place where DEC-094's options can be compared on a measured cost.
- **Numerics.** Simulation state is binary64 (ADR-081); MVP-093 defers the own deterministic maths functions, so M1 reproduces a run within one build only, and an ARM64 build and an x64 build give different hashes. Every reference draw gets a purpose and a subject (ADR-082), and an acceptance criterion that counts draws in sequence is restated per purpose.
- **The AI reads the whole battle** until the seen layer arrives at M3 (O70), as the reference bot does (R-BOT-017). This departs from MVP-016 and PIL-084 at M1 only; MVP-058 ends it.
- **Formations and expeditions.** A formation holds 90 while an expedition holds up to `mvp.expedition.size_max` (200) (MVP-041); M3 settles how an expedition of several Leaders forms.
- **The x64 build is not measured** on the reference test machine (O69); the minimum-specification tier measures it before the first external build.
- **DirectXTex.** The content build restores no DirectXTex while every texture is authored in its runtime format (O69, ADR-017 amendment). The pinned package ships x64 static libraries only, which the tool's ARM64 build cannot link; the first texture that needs cooking settles that in an ADR.
- **CI builds `Debug|x64` only** (`AGENTS.md` §6). The `Release|ARM64` builds of T10 and T18 are the owner's, as is a Release build of any change that could break only under optimisation.
- **Content (O73).** T11 lands ahead of T9 and T10. Until packs exist, every configuration mounts the loose-content folder that the solution build cooks beside the executables, so the `Release|ARM64` server of T10 and T18 reads the cooked tuning record. M1 has no import records: textures are checked against their rows and meshes cooked at one world unit per source unit ([ADR-108](../ADR/ADR-108-content-build-loose-content-and-cooked-formats.md)).

## 8 What the owner does during M1

- **Runs the checklists** of T12 to T18 on Debug builds, as M0's were, and the performance script at T10 and T18 on `Release|ARM64` with the 1,920 × 1,080, 60 Hz display (O69).
- **Plays the sandbox** at T18 and writes the session log's three lines in `playtests/M1.md` after each session, in the format of the playtest README.
- **Reviews the likeness** of the Worker and Leader silhouettes (PIL-027), with the result recorded in the manifest.
- **Prepares M2:** the en-GB string table's first strings, which GameFont first draws at M2 (O70), and the hand-made assets of M2 ([MVP assets](mvp_assets.md) §4): the carried-load overlay and the M2 icons.
