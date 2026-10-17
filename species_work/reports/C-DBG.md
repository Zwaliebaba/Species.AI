# C-DBG report: diagnostics, test and performance

## Files written

| File | Size |
|---|---|
| `design_next/architecture/components/diagnostics_test_and_performance.md` | 121 722 bytes (118.9 KiB): one file, under the 120 KiB limit, **with only about 1.1 KiB of headroom** |
| `species_work/scripts/C-DBG_check.py` (self-check), `C-DBG_edit1.py` to `C-DBG_edit7.py` (edit passes) | scripts |

**No split.** The draft passed 120 KiB once (123.4 KB). I compacted it to fit rather than split it, because 35 documents already link to `diagnostics_test_and_performance.md`, and I lost no fact doing so. **Any further addition needs the folder split** (`diagnostics_test_and_performance/` with a `README.md`). These documents would then need their links updated: CONVENTIONS.md; the glossary (2 files); 7 decision files; reference/06 actions_and_bindings, 07 (2), 09 options, and 12 (01, 03, 04, 05, 07); and the components asset_pipeline, audio_engine, build_tooling, data_schemas (README, config_input_and_strings), input, platform_layer, renderer (README, entity_and_overlay, frame_graph, options_and_budgets), terrain and ui_framework (4).

## Counts

- Requirements: 114 (DBG-001 to DBG-241, with gaps by section, ascending and unique). Self-check: `python3 -I species_work/scripts/C-DBG_check.py <REPO>` prints OK.
- Tuning keys: 124 `dbg.*` keys, each defined once and cited by a requirement. Cited keys owned by others: `ref.sim.*` (reference/12), `ref.econ.orb.*`, `ref.unit.worker.retarget_ticks`, `ref.bot.link.range`, `rnd.budget.target_frame_rate`, `plt.lifecycle.exit_code.fatal`.
- Diagrams: 2 Mermaid (the soak host and soak client state machines), each with a caption.
- Enumerations: 23 tables and 8 numbered lists. Among them: the profile-marker catalogue, 16 hotspots (H1 to H16), 22 debug-window commands, 10 counter pages, 10 network-window layout and colour groups, 13 acceptance-test-mapped golden-hash targets, and 30 acceptance tests (T1 to T30).
- Scans: the forbidden and soft pattern scans print nothing.

## Open DECs cited (all `decs_to_cite` Open entries are cited by a requirement)

DEC-046, DEC-106, DEC-201, DEC-202, DEC-206, DEC-208, DEC-220, DEC-224, DEC-225, DEC-228, DEC-229, DEC-240, DEC-242, DEC-250, DEC-251, DEC-802. Also cited: DEC-045, DEC-094, DEC-115, DEC-200, DEC-207, DEC-222 and DEC-227 (Open); DEC-041, DEC-650, DEC-653 and DEC-804 (Resolved, stated as fixed rules); DEC-004, DEC-007, DEC-013, DEC-014, DEC-015 and DEC-800 (Resolved); and DEC-060, DEC-334, DEC-341, DEC-052, DEC-055 and DEC-059 (Dropped).

**Deferred inputs (DEC-227, DEC-228).** The tick budget guidance (`dbg.budget.tick_share_guidance`, 25 % of 16.7 ms), the design population (2 000) and the stress population (4 000; 4 teams × 150 units; 24 Spawn Nodes) are keyed and marked as design-phase inputs (DBG-204, DBG-205). DEC-106 is the envelope at the population cap (DBG-202, DBG-205).

**Golden-hash targets.** DBG-145 cites the integrity acceptance criteria A3, A4, A6 and A8 to A18 of `07_randomness_and_integrity.md`, and the R-SIM-708 determinism corpus.

## New DEC requests

1. **Client minimum-specification hardware and the reference test machine.** Several budgets (RND-454, AST-125, DBG-209) and BLD table 5-A (`dbg.machine.developer_class`) depend on a named machine, and no DEC fixes one. Default: a pinned CI configuration at the minimum of ADR-0050 and ADR-0002, whose hardware is recorded with each result.
2. **What the cheats-used marker suppresses.** Achievements, leaderboards and statistics in a session that has used debug commands. Default: the marker is shown in the lobby and the results and carried in the statistics (DBG-118), and nothing is suppressed.
3. **Debug-command authority before DEC-202 is decided.** DBG-118 makes the host's own participant the only authorised sender. Please confirm this, or fold it into DEC-202.

## ADR conflicts

None blocking. Notes:
- ADR-0066 calls the repeatable scene a "benchmark scene". The brief forbids naming the superseded benchmark mechanism, so the document calls it the "repeatable performance scene" (DBG-183). Suggest the same wording in ADR-0066.
- ADR-0068 places the overlay in `Development` and `Profile` builds. DBG-004 also includes `Debug`, which compiles in everything that `Development` does (ADR-0056). This is not a contradiction.
- ADR-0065's confirmation requires crash debug commands. These are rows 17 to 22 of DBG-117 and match platform layer T22.

## Missing assets

None. The development text is a development-only string table (`ui.dev.*`); the overlay uses the UI font roles, and the development sphere is procedural (RND-330).

## Cross-document requests

1. **C-PLT (platform_layer.md).** The two documents swapped ownership mid-run. PLT now defers to DBG-070 to DBG-075, and DBG-073 again has the steps 1, 2 and 3 that PLT-118, PLT-126 and PLT-127 cite. I re-aligned to your current text: `FailFast` (PLT-127), `EndWithMessage` (PLT-044), PLT-117 to PLT-125, PLT-023 and PLT-025.
   - Your `plt.crash.check_message_max_chars` key is gone. The 512-character truncation is now `dbg.blackbox.check_message_max_chars` (DBG-047 item 7).
   - A development check without a debugger logs, notes the failure in the black box and continues (DBG-070). Please don't restate this.
2. **C-DAT (config_input_and_strings.md table 3.1).** Three fixes to the development settings, plus missing schemas:
   - (a) `debug.bot_overlay` has three states, so it must be an integer: 0 is off; 1 shows text and arrows; any other non-zero value shows arrows only (DBG-041, DBG-135).
   - (b) `debug.bot_overlay_team` is a lobby position, default 1, not a team slot.
   - (c) The reference command-history setting is one setting with three values. Please replace `debug.command_record` and `debug.replay` with `debug.command_history` (off, record, play back), and add `debug.capture_frame_rate` (25) and `debug.soak_session_name`.
   - (d) The schemas still needed: the soak configuration and soak report (DBG-162, DBG-166), and the telemetry session record (DBG-181).
   - (e) `DAT-056` was renumbered to `DAT-050` while I worked. I cite DAT-050.
3. **G06 (reference/06 actions_and_bindings M5).** M5 says `action.debug.network_stats` "toggles" the window. The reference only opens it; the window's close box closes it (DBG-020). Please align.
4. **C-INP (input.md).** Please add the development binding layer of DBG-020: F11 opens the command window; F (held) drops packets; L and K turn special lighting on and off (`key:l` also drives the chat log); R toggles soak rendering; F5 shows the overlay; F10 shows the profile view. This layer is compiled into development builds only. INP-009 then still holds for the default set.
5. **G09 (reference/09).** The cheats-used marker (`ui.match.cheats_used`) needs a lobby and results hook (DBG-118).
6. **C-SIM (reference/12/07).** Please cite `dbg.replay.capture_frame_rate` (25) as the "fixed capture rate" of R-SIM-740.
7. **Answered:**
   - C-RND-a 10: pass 24 is DBG-130/131 and the overlays are DBG-095 to DBG-099.
   - C-RND-b 9: RND-453, RND-359, RND-330 and RND-442 are cited (DBG-003, DBG-130, DBG-135, DBG-020).
   - C-AEN: AEN-322 and AEN-323 are on the audio page; T27 is in DBG-208.
   - C-UI-a 10: UI-001, UI-060, UI-061 and UI-166, with the UI counter pages.
   - C-AST 9: the AST-125 machine is DBG-209; AST-126 is on the assets page; table 5-A is in DBG-208.
   - X-DEC-2b 6: DBG-142, DBG-160, DBG-166 and DBG-204.
   - C-BLD (coordinator message): `ci-nightly-soak` and the `soak` edition are cited (DBG-160), and `ci-nightly-performance` with BLD table 5-A (DBG-208). The reference developer machine class is `dbg.machine.developer_class` (DBG-209). The black box does not carry the channel tag; the log header does (DBG-044, DBG-047).
8. **X-GLO.** Terms to add: black box, development overlay, counter provider, debug command, cheats-used marker, soak test, soak entry, profile view, development drawing queue, reference test machine, design population, stress population.
9. **Coordinator.**
   - Marker names use `frame.`, `sim.`, `render.`, `effects.`, `audio.`, `net.`, `camera.`, `input.` and `devtools.`. They avoid the tuning-key prefixes (`vfx.`, `dbg.` and the others) on purpose.
   - Development string ids are `ui.dev.*`, plus `ui.match.cheats_used`. Setting ids use C-DAT's `debug.` prefix.

## Names not in the ledger (proposals)

- **Types and macros:** `LogLevel`, `LogCategory`, `LogStats`, `BlackBox`, `ProfileTree`, `ProfileNodeStats`, `ProfileNodeId`, `CounterProvider`, `CounterSink`, `DevDrawQueue`, `DevDrawItem`, `DevDrawItemId`, `DebugCommand`, `DebugCommandKind`, `SoakRunner`, `SoakConfig`, `SoakEntry`, `SoakProgression`, `SoakReport`, `TelemetrySessionRecord`, `FatalExit`, `CheckFailed`, `Vec2`; the macros `SPECIES_ASSERT`, `SPECIES_CHECK`, `SPECIES_CHECK_INDEX` and `SPECIES_PROFILE_GPU_SCOPE`. `SPECIES_PROFILE_SCOPE` comes from ADR-0068.
- **Actions:** `action.debug.command_window`, `.drop_packet`, `.special_lighting_on`, `.special_lighting_off`, `.soak_rendering`, `.overlay`, `.profile_view`.
- **Setting ids:** `debug.command_history`, `debug.capture_frame_rate`, `debug.soak_session_name`.

## Dropped as legacy (one line per group)

- **Campaign cheats:** enabling campaign buildings and opening locations; granting all research (DEC-115: research levels are fixed at match start); the narrator portrait; the sound-editor and sound-statistics windows, replaced by the audio overlay page. All covered by DEC-041 and DEC-052.
- **Benchmark recorder, its upload and video capture:** the per-second recorder, video capture, input and camera event logs, and gesture recordings (DEC-060). The repeatable scene is the ADR-0066 performance scene.
- **The superseded profiler's mechanics:** the GPU-flush toggle and its render bracketing, the tree that exists only while its window is open, expansion-gated timing, and its leaks.
- **Unreachable or inert entry points:** the tools menu with no opener, the commented-out hot keys, the editor binding on the network-statistics key, debug quit, the campaign camera fix-up, and the campaign speed-up key.
- **The test-bed's UI driving:** menu buttons with colliding names, the 23 application-state fields, the English mode-name copies, the local-time log, the wrong map-identity index, and the linear-progression bounds errors (DEC-804). Its configuration file and its build-time changes are outside my ranges.
- **Lockstep integrity tooling** (DD-NET to DEC-206 and DEC-208): snapshot hashing and the one-byte generator-position value, divergence arrows, divergence dump files, late-join re-simulation (D3), and the link-simulator details.
- **Debug-log mechanics** (ADR-0067): opening and closing the file on every call under a mutex, the fixed 10 240-byte buffer, per-OS log paths, the null-name failure message, the line count of log call sites, and file-transfer verbose logging.
- **Assertion mechanics** (ADR-0065, DEC-341): the message box with a rethrow that has no active exception, the frame-pointer stack walk, the linker-map symbolisation script, and the debug-assert cursor call.
- **Hotspots:** H9 (main-thread compilation of static geometry and the full GPU-resource rebuild path), H18 (campaign per-frame world events and the gesture bitmap re-upload) and H19 (the profiler's GPU flush). These are legacy technology or campaign work. The terminal hardware note keeps CPU, memory, disk and GPU class, but not the OS names.
- **Profile sections without a Species system:** campaign world, internet, narrator, campaign help, gesture and icon interfaces, worship, the essence store, tri-walker, deform and reflection (DEC-059), pixel effect (DEC-059), and the device-buffer sections of the audio output.
- **Development drawing:** the spawn-point object of campaign maps, fence-switch and track-junction links, Vehicle Duel arrows (dropped mode), the chaos-game point cloud, and the terrain storage-mode setting (RND-443).
- **Outputs:** the support-tool bundle and its system-report files, the version file (moved to PLT-023), and the leak report (outside my ranges).
- **Cheat-window minor quirks:** an uninitialised team id, a button name collision, and an unused flag.
