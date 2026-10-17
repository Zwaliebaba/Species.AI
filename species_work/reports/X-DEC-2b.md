# X-DEC-2b report: deferred decisions on process architecture, tick rate and build order

Phase 2, wave 2a. Brief §9.2 X-DEC (DEFER-DESIGN topics DD-THREADS, DD-TICK, DD-ROADMAP). ID range DEC-220 to DEC-239.

## Files written

| File | What |
|---|---|
| `design_next/decisions/deferred/process_tick_and_roadmap.md` | 10 Open DEC entries (DEC-220 to DEC-229), 55.8 KB; heading, scope paragraph, index table; §6.5 format plus the two extra fields "Reference behaviour" and "Constraints and lessons" |
| `species_work/dec_map_X-DEC-2b.csv` | 10 rows: `dec_id, title, status, affects_prefix, affects_files, orig_refs` (csv module, `lineterminator='\n'`) |
| `species_work/scripts/X-DEC-2b_decmap.py` | writes the CSV; checks that every mapped id has a heading in the DEC file |
| `species_work/scripts/X-DEC-2b_coverage.py` | checks that every package input range appears in `orig_refs` (result: 36 of 36 covered, 0 unknown refs) |
| `species_work/reports/X-DEC-2b.md` | this report |

## Entries

| DEC | Title | Topic | Constraints |
|---|---|---|---|
| DEC-220 | Process roles and the headless server build | DD-THREADS | 9 |
| DEC-221 | Composition root, service lifetimes and the application state machine | DD-THREADS | 11 |
| DEC-222 | Simulation and presentation boundary | DD-THREADS | 8 |
| DEC-223 | World, region and match state ownership and reset | DD-THREADS | 9 |
| DEC-224 | Threading and job model | DD-THREADS | 13 |
| DEC-225 | Frame loop, simulation pacing and catch-up | DD-THREADS (with DD-TICK pacing and 41 32-I2) | 9 |
| DEC-226 | Loading, streaming and asynchronous work | DD-THREADS | 9 |
| DEC-227 | Simulation tick rate | DD-TICK | 8 |
| DEC-228 | Per-tick CPU budget, population caps and stress populations | DD-TICK | 9 |
| DEC-229 | Build order, milestones and work breakdown | DD-ROADMAP | 9 (item 6 has 9 work packages, item 7 has 8 workstreams) |

DEC-230 to DEC-239 are unused. Every entry: Status Open, Decide by: design phase; the specified behaviour is the reference default; the integrity rules themselves are stated as fixed (kept in `reference/12`) and only the enforcing architecture is open.

Recommendations in brief: A (three build targets, server library without presentation deps); A (composition root per role, scoped lifetimes incl. world and region, explicit state machine; do it first); A (immutable per-tick snapshot + typed event queue, build-enforced, interpolation); A (one state owner per region and per skirmish match, no simulation statics); A (one simulation thread per region/match + deterministic job system for hot phases; a step at least reproducible on the same build); A (server tick loop on its own thread, sleeping to deadline; bounded visible catch-up); A for the world, B acceptable for the skirmish (streaming vs blocking view); D (20 Hz full simulation, lower rate for the abstract model, confirmed by measurement); A+B (client-derived and server-first budgets); A (reference order adapted: headless server core, headless skirmish slice, presentation on the integrated server with the in-process transport, then world systems).

## Counts

- Requirements: 0 (decision register; no R- or engine IDs; "must" kept lower case by design, stated in the file's scope).
- Tuning keys defined: 0. Cited: `ref.sim.tick_period`; identifiers from the ledger: `kTickPeriod`, `kUpdateSlicesPerTick`, `MatchWorld`.
- Diagrams: 0. Enumerations: 0 (the reference lifetime classes and state flow are given in prose).
- Numbered constraints and lessons: 94.

Key figures captured: 10 Hz / 0.1 s tick, 10 slices per tick, rules update in slice 8, about 30 % of the slice backlog per frame, dead band 1–3 slices; frame advance clamp 0.25 s; catch-up 20 slices per iteration and at most 0.3 s per frame; loading loop 60 Hz cap, keep-alive at most once per 1.0 s, 1.0 s logo fade; teardown: 3 leave messages, 100 ms block; deferred deletion on the 6th poll; affinity masks assume ≤ 8 logical CPUs; ten kinds of game thread plus one OS audio callback thread; ≈ 40 globals and ≈ 30 root-owned services; display delay 100/50/33 ms at 10/20/30 Hz; budget ≤ 25 % of a 16.7 ms frame (≈ 4.2 ms); peak 2,000 units, stress 2× with 4 × 150 start units and 24 Spawn Nodes; 1–2 s search stagger, 1 s grid cache; 64-bit generational handle (32 + 32); 60 fps with 2,000 units on minimum spec; 2,000-unit replay on two Intel and two AMD machines; 4 clients, 10 % loss, 150 ms RTT, 30 minutes, rejoin at minute 20; 31 crate outcomes; mode second of 11 ticks (from the prompt §6.1, as a rescaling hazard).

## Gate results

- `forbidden_patterns.tsv`: 0 hits.
- `soft_patterns.tsv`: 1 hit, justified: `Linux` at DEC-220 item 6 ("The vision brief recommends a Linux dedicated-server build for community hosts"). This is the server-hosting context that §7.2 allows. **Request to the coordinator:** add to `CONVENTIONS.md` §9 Allowed terms: `Linux` | `decisions/deferred/process_tick_and_roadmap.md` (DEC-220) | dedicated-server hosting, vision §6 item 10.
- `lockstep` is not used; the entries say "every peer runs the same simulation" instead.
- State names are written as plain words (boot, front end, lobby …) so no identifier outside the ledger is introduced.

## DEC requests

None outside this file. Deduplication notes for wave 2b (X-DEC-3, X-DEC-5a, X-DEC-5c): the open questions on the listener during shutdown, the thread entry calling convention, OS audio threads touching game memory, unlocked texture look-ups outside loading views, the static teleport table and the catch-up spiral are captured here as lessons (DEC-224 items 6–8, 10, 12; DEC-223 item 2; DEC-225 item 3). Reference them rather than opening new entries. The never-instantiated file-system object is dead content and is dropped (no lesson).

## ADR requests

None (threading, tick rate and build order are superseded-verdict topics; no ADR). **For X-ADR-a/b:** `adr/README.md` §"Deferred to the design phase" can point its threading, tick-rate and process-architecture rows at DEC-220 to DEC-228.

## Cross-document requests

1. **C-SIM-c** (`reference/12_simulation_model/01_tick_timing_and_main_loop.md`): DEC-222, DEC-225 and DEC-227 cite `ref.sim.tick_period`, `kTickPeriod` and `kUpdateSlicesPerTick`, the 30 % backlog share, the 1–3-slice dead band, the 20-slice/0.3 s catch-up and the 0.25 s advance clamp as reference behaviour in that file. Please keep those keys and figures there. Also please confirm that the pause rules are specified as reference match behaviour: pause allowed only with no remote-player team, outside the grace period and before game over; paused when the clock is paused or a client is out of sync.
2. **C-SIM-a** (`02_match_lifecycle_and_state_reset.md`, `03_entity_model_and_identity.md`): DEC-223 lists the surviving per-match state (Control Zone count, sixth-slot Spawn Node count, Transport debounce, teleport table, tick-clock fields) and the seeding points (simulation generator at session join, presentation generator re-seeded with 1 per match, id seed per load). Please make sure the reset points and the resolved fixes appear there; DEC-223 points to that file.
3. **C-RND-b** (`renderer/options_and_budgets.md`): DEC-228 item 5 cites instanced unit drawing (one draw per unit type per pass) and a per-frame draw-call budget. Please also cover the inverted-colour ("negative") render option with its white clear colour, if it is not already in your inputs.
4. **C-AEN** (`audio_engine.md`): DEC-222 item 5, DEC-224 item 12 and DEC-226 item 6 need a stated contract: what happens to cues issued off-thread, during a loading view or before banks are loaded (the reference drops them), and that the callback thread touches only queued data.
5. **C-AST** (`asset_pipeline_and_resources.md`) and the pack-file ADR: DEC-226 item 5 assumes concurrency-safe content readers.
6. **C-DBG** (`diagnostics_test_and_performance.md`): DEC-220 item 3 and DEC-228 items 2 and 8 cite the nightly soak (every mode × map, bot teams; divergences, crashes, peak population, tick-cost percentiles), the headless runner and the 2× stress population. Please state them as engine-baseline items there, if your inputs carry them.
7. **C-PLT** (`platform_layer.md`): DEC-225 items 6, 8 and 9 cite the present-interval pacing, the monotonic clock with an injectable fake clock, and the message pump as platform-layer duties.
8. **X-DEC-2a** (`networking.md`): related entries are DEC-222 item 8 (snapshot and replication formats), DEC-225 item 4 (keep-alive and server ticking during loads), DEC-227 item 6 (command and snapshot cadence) and DEC-229 item 8 (networking exit checks: 10 % loss, 150 ms RTT, 30 min, rejoin at minute 20).
9. **X-DEC-2c** (`numerics_lobby_terrain_services.md`): related entries are DEC-224 item 4 (the same numeric environment on every simulation thread), DEC-223 item 6 (persistence of state owners), DEC-225 item 7 (lobby as a server state) and DEC-226 item 8 (terrain generated or baked).
10. **Facts in this package's ranges that belong to other topics.** I did not put them in DEC entries. Please confirm that the owning package specifies them from its own inputs; otherwise they need a home:
    - **G09b / C-UI-a:** the language switch: CJK languages alias the game and editor fonts to the title font, with horizontal scale 1.0 (0.6 otherwise) and spacing 0.91 (editor) and 1.03 (title); the large-menus option; the seasonal presentation mode active on 24–26 December local time.
    - **G09b (KH-ACHIEVE):** awards are suppressed when any of the match's player teams is a bot, except for three conditions.
    - **G09a:** the maximum player count is looked up from the selected map.
    - **C-INP:** the input-driver chain order at start-up; the controller-unplugged dialog.
    - **C-UI-b / G06:** the Power slots panel variant is re-created when the input device class changes, and only while the panel is hidden.
    - **C-BLD:** the version-file switch.
    - **C-PLT:** the per-user profile directory under local application data, and the switch that disables it.

## Names needed but not in the ledger

None introduced. Generic role words are used for roles that have no ledger name: state owner, sequencer, loader thread, online-service threads, composition root.

## Dropped (content of the DEFER-DESIGN ranges not carried into a DEC), with reasons

- **DL-CAMPAIGN:** the campaign and prologue game modes; profile load/save and level reset; the global-world holder; campaign-only pause on focus loss; the developer start-map preference. Unreachable in a match.
- **DL-DEAD / DL-DEBUG:** testbed members and methods, attract mode, benchmark, video grabber, boot-loader sequence, OS thumbnail and rich-save hooks, mod system, sound-editor processor, declared-but-undefined globals, the never-instantiated file-system object. Not compiled or unreachable.
- **DL-D3D9 / DL-LEGACY-TECH:** graphics-API extension pointers, device globals, the alternate-device mip-map worker, display-list jobs, the per-thread floating-point precision mechanism, the container library, the archive format and the decompressor. Superseded technology. The behaviour-level lessons are kept as constraints, without technology names.
- **DL-DEMO / DL-SERVICES:** entitlement and trial-limit checks, the buy-now address, online-service hosts, ports, retry periods and registration timings. Dropped; DD-SERVICES is X-DEC-2c's.
- **DD-NET detail:** message kinds, sequence ids, sync byte, client and server ports. Dropped; X-DEC-2a owns the networking lessons.
- **DP-INDEX:** the scope table of the architecture document (a reading guide); a C++ declaration-order note (an implementation detail; its lesson is DEC-221 item 2).

## Publishing check (coordinator rule)

Nothing was published or uploaded. I did not use the Artifact tool or any Gamma, Google Drive, Docs or GitHub tool. Every file was written to local disk with Write, Edit or Bash.
