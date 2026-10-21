# Logging, checks and profiling

> Status: Engine baseline · Prefix: DBG · ADRs: [ADR-067](../../../ADR/ADR-067-logging.md) · [ADR-065](../../../ADR/ADR-065-crash-reporting.md) · [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md) · [ADR-057](../../../ADR/ADR-057-version-identity.md) · [ADR-074](../../../ADR/ADR-074-platform-clock-with-injectable-time.md) · Folder: [diagnostics, test and performance](README.md) · Related: [platform layer](../platform_layer.md) · [frame graph and passes](../renderer/frame_graph_and_passes.md) · [build tooling and distribution](../build_tooling_and_distribution/README.md)

This document specifies the logging system, the check levels, the black box and the fatal path, and the profiling layer with its marker catalogue and its profile view. The responsibilities of the component, the interface sketch (Listing 2.1-A), the facilities per build configuration (DBG-004) and the threading contract are in the [README](README.md).

## 1 Responsibilities and boundaries

This document holds items 1 to 3 of DBG-001 ([README](README.md) §1). The crash capture that the fatal path hands over to, and the files and memory registered with Windows Error Reporting, are the [platform layer](../platform_layer.md)'s (DBG-003).

## 2 Interfaces

The logging, check, black-box and profiling declarations are those of Listing 2.1-A ([README](README.md) §2.1). Their threading contract is DBG-027 and DBG-028.

## 3 Data model

### 3.3 Log records and categories

**DBG-043** (MUST) A log record holds: the platform-clock time (ADR-074) and the UTC calendar time used only for display; the thread name; the category; the level; and the message, formatted from a compile-time format string with arguments. Categories are the fixed set of Listing 2.1-A: the nine of ADR-067 plus `Diagnostics` (this component) and `Test` (the test harness and the soak runner). Levels are ordered trace < debug < info < warning < error < fatal.

**DBG-044** (MUST) A log file starts with a header that holds the full build identity (ADR-057), the OS build, the adapter description, the channel tag (BLD-021) and the date and time of the start of the run (ADR-067).

### 3.4 Profile marker catalogue

**DBG-045** (MUST) Every engine subsystem update, every frame-graph pass (RND-041) and every content-loading stage carries a marker (ADR-068). The catalogue below covers every Species engine subsystem, frame-graph pass and loading stage (ADR-068); a row's "per kind" marker is one marker per actor kind or structure kind, named by the kind's identifier. Markers nest by call order on their thread.

| Area | Species markers |
|---|---|
| frame and loop | `frame.match_loop` (the whole simulation advance of a frame, around the world step); `render.frame`; `render.clear`; `render.present` |
| camera and input | `camera.advance`; `input.advance`; `render.match_input` |
| sky | `render.sky`; `render.sky.blobby_layers`; `render.sky.flat_layers`; `render.sky.advance`; `render.clouds` |
| terrain and sea | `render.terrain.surface`; `render.terrain.wire_overlay`; `render.sea`; `render.sea.advance`; `render.sea.wave_build` (the coastline wave mesh, built on the GPU in Species, ADR-011) |
| simulation, world step | `sim.laser_bolts.advance`; `sim.effects.advance`; `sim.structures.advance` with one marker per structure kind; `sim.essence_orbs.advance`; `sim.teams.advance_group_members`; `sim.teams.advance_groups`; `sim.teams.advance_ungrouped` with one marker per actor kind; `sim.actor_group.advance` with one marker per actor kind |
| Workers | `sim.worker.task_search`; `sim.worker.search.threats` with `.projectiles`, `.trees`, `.hives`, `.bot_nodes`, `.enemies`; `sim.worker.search.ports`; `sim.worker.search.carryables`; `sim.worker.search.supply_crates`; `sim.worker.search.transports`; `sim.worker.search.flags`; `sim.worker.search.leaders`; `sim.worker.search.random_position`; `sim.worker.combat`; `sim.worker.throw_grenade`; `sim.worker.move_to_target`; `sim.worker.push.sea`; `sim.worker.push.structures` |
| creatures | `sim.crawler.record_trail`; `sim.crawler.move_to_target`; `sim.crawler.idle`; `sim.crawler.attacking`; `sim.crawler.to_essence_orb`; `sim.crawler.to_egg`; `sim.crawler.search.enemies`; `sim.crawler.search.essence_orbs`; `sim.crawler.search.eggs`; `sim.crawler.search.idle_direction`; `sim.centipede.search_essence_orbs`; `sim.spider.search_essence_orbs` |
| structures and objects | `sim.spawn_node.evaluate`; `sim.spawn_node.spawn_workers`; `sim.structure.push_actors`; `sim.essence_orb.push_from_structures`; `sim.spitter_plant.check_trigger`; `sim.bomber.update_position` |
| server loop | `server.tick` (one whole tick); `server.receive` (the connections and messages delivered at its start); `server.step` (the world's step); `server.publish` (the connections' parts of the tick); ADR-095 |
| network | `net.host.advance`; `net.host.update_clients`; `net.host.send`; `net.host.advertise`; the Species networking markers follow DEC-200 |
| effects | `effects.particles.advance`; `effects.particles.render`; `effects.shatter.advance`; `effects.shatter.render` |
| world rendering | `render.structures` and `render.structures.transparent`, each with one marker per structure kind; `render.structures.transparent_sort`; `render.structure_ports`; `render.essence_orbs`; `render.weapons`; `render.actor_groups` with one marker per actor kind; `render.ungrouped_actors` with one marker per actor kind; `render.crawlers`; `render.workers`; `render.cloth_flags`; `render.beam_dish.beam`; `render.transit_gate.destination`; `render.transit_gate.surface_advance`; `render.transit_gate.surface`; `render.routes` |
| overlays and UI | `render.world_cursor`; `render.world_markers`; `render.centre_message`; `render.mode_hud`; `render.hint_overlay`; `render.power_slots_panel`; `devtools.profile_view.render` |
| audio | `audio.update`, with one marker per update step of AEN-123, named by the step; `audio.stream`, the stream task; `audio.bank_load`, one per bank load (AEN-322) |

> **Amendment 2026-10-20 (M1 task T10; [ADR-110](../../../ADR/ADR-110-cost-measurement-marker-timing-and-performance-script.md)).** The MVP's region step adds markers that the catalogue lacks, so that the performance job divides the step by the objects it advances: `sim.region.commands` (the tick's commands), `sim.bot_nodes.advance` (the bot navigation nodes, in each slice), `sim.ai.step` with `sim.ai.stray_recall`, `sim.ai.leader_policies` and `sim.ai.main_cycle` (each AI of the region), and `sim.routes.free_unheld` (DEC-651). The world step's `sim.region.step` is ADR-085's region job. While marker timing is on, each marker's time is read in-process (DBG-080 amendment).

> **Amendment 2026-10-20 (M1 task T12; [ADR-111](../../../ADR/ADR-111-replication-state-datagrams-and-the-client-replica.md)).** Replication adds `net.replication.collect` (the battle's replicated state, once a tick), `net.replication.build` (each client's update) and, on the client, `client.replica.apply` (each state datagram applied to the replica).

> **Amendment 2026-10-20 (M1 task T13; [ADR-113](../../../ADR/ADR-113-the-frame-at-m1.md), [ADR-115](../../../ADR/ADR-115-the-world-frame-at-m1.md)).** The world frame adds `render.camera` (pass 1), `render.frame_constants` (pass 0), `render.resolve` and `render.final`. Species draws the terrain's grid in the surface pass (ADR-011), so pass 4 is `render.terrain.surface` and `render.terrain.wire_overlay` has no pass. At M1 the waves are evaluated on the CPU: `render.sea.advance` is their evaluation and `render.sea.wave_build` the pass that draws them. `render.sky.advance` is the sky's instances for the frame.

**DBG-046** (MUST NOT) The catalogue has no marker for campaign-only systems (DEC-052), for the pixelation, deform and reflection effects (DEC-059) or for the gesture and icon Power interfaces (DEC-055), which Species does not specify.

### 3.5 Black-box contents

**DBG-047** (MUST) The black box is a fixed-size block of `dbg.blackbox.block_bytes` (4096 bytes) registered with Windows Error Reporting by the platform layer (ADR-065). It holds:

1. a fixed marker text, so that the block can be found in a dump;
2. the full build identity (ADR-057);
3. the UTC date and time of the start of the run, and of the failure when one is recorded;
4. the most recent `dbg.blackbox.recent_markers` (16) frame and tick markers, each with its frame index or tick;
5. the current map id, or the world region id once the design phase defines regions;
6. the most recent `dbg.blackbox.recent_checks` (8) failed-check and warning messages;
7. for a failed check or a fatal exit, the black-box record: the product name, the build identity, the UTC date and time, the message truncated to `dbg.blackbox.check_message_max_chars` (512 characters; the full message goes to the log), the condition text, the source file and line, and the thread name.

It never holds chat text, account tokens, the names of other players or the Windows user name, and every path in it is written relative to the install directory or the user-data root, with the root shown as a placeholder (ADR-065, ADR-067; see DEC-970). It does not hold the channel tag, which reaches the log header only (BLD-021, BLD-120).

**DBG-048** (MUST) A failed check is recorded with the build identity, the time, the message and the stack. The black-box record (DBG-047 item 7) holds the first three, and the stack is taken from the dump, symbolised through the symbol store of every distributed build ([build tooling and distribution](../build_tooling_and_distribution/README.md), ADR-065); no Species code walks frame pointers.

## 4 Behaviour and algorithms

### 4.1 Logging

**DBG-060** (MUST) Logging follows ADR-067: `Log(category, level, format, arguments)` with a compile-time format string, so text from content, players or the network is always an argument and is logged verbatim, a percent sign or a brace included. No log call formats into a fixed-size buffer; a long message is logged whole.

**DBG-061** (MUST) A log call writes its record into the calling thread's lock-free buffer of `dbg.log.thread_buffer_kib` (256 KiB) and returns; one logging thread drains every buffer to the sinks. When a buffer is full, trace records are dropped first, then debug records; warnings, errors and fatal records are never dropped, and the caller waits only when its buffer holds nothing but such records. The cost on the calling thread stays within `dbg.budget.log_call_us` (DBG-206).

**DBG-062** (MUST) Dropped records are counted per category and level. The counts appear on the log page of the development overlay, and a warning record states the counts since the previous such warning whenever a drain finds new drops.

**DBG-063** (MUST) `LogHexDump` writes `dbg.log.hex_bytes_per_line` (16) bytes per line in hexadecimal; `LogValueDump` writes 32-bit integers in decimal, `dbg.log.hex_bytes_per_line` ÷ 4 values per line.

**DBG-064** (MUST) Sinks and levels per configuration are those of ADR-067: the rotating UTF-8 file (DBG-049) and the TraceLogging provider in both configurations, and the debugger output in Debug builds. Release builds record info and above; Debug builds record debug and above, with trace enabled per category at run time. If the log file cannot be opened, records go to the remaining sinks, and the failure is reported once, with the path, on those sinks.

**DBG-065** (MUST) The log records at least the content that the reference debug log carries, as categorised records: the start-up header (DBG-044); content mounting with entry counts and durations, and every missing resource ([asset pipeline and resources](../asset_pipeline_and_resources.md) AST-126); every settings entry that fails to parse or is out of range (DEC-800); network connection, divergence and disconnection events (DEC-206); audio readiness, bank loads and device changes (AEN-324); key-binding parse outcomes ([input](../input/README.md)); and a memory report every `ref.sim.frame.memory_log_interval` (60 s, [tick timing](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md)) giving the available physical memory, the process's committed memory and working set, and the audio engine's memory summary (AEN-324).

**DBG-066** (MUST NOT) No record holds chat text, account tokens, passwords, or an identifier of another player other than a session-local number (ADR-067). No diagnostic is formatted while its level is disabled: a disabled record costs the level test only.

### 4.2 Checks, the black box and the fatal path

**DBG-070** (MUST) Species has these check levels:

| Level | Configurations | On failure |
|---|---|---|
| development check `ENGINE_ASSERT(c)` | Debug | breaks into an attached debugger (PLT-121); otherwise logs an error record with the condition, file and line, notes it in the black box and continues, and the test harness turns it into a test failure (DBG-072) |
| release check `ENGINE_CHECK(c, message)` | Debug and Release | the fatal path of DBG-073 |
| plain release check (no message) | Debug and Release | as a release check; the message is the condition text, file and line |
| container bounds check `ENGINE_CHECK_INDEX(i, n)` | Debug and Release | as a release check: an out-of-range container access is a fatal, reported error in every build |
| abort, a release check that always fails, with a message | Debug and Release | as a release check |
| fatal exit `FatalExit(string id, arguments)` | Debug and Release | DBG-074 |

**DBG-071** (MUST) Content errors never reach a check: the content build rejects them (DEC-015, ADR-059), and a load path reports and recovers. A check states a program invariant; a failed check is a defect.

**DBG-072** (MUST) The test builds' simulation checks of §4.8 are development checks that the test harness turns into test failures; they never run in Release builds.

**DBG-073** (MUST) The fatal path of a failed release check, a container bounds check or an abort runs in this order; the terminate, invalid-parameter and pure-call handlers enter it at step 1 with their reason ([platform layer](../platform_layer.md) PLT-118):

1. write the black-box record of the failure (DBG-047 item 7);
2. write the full message as a fatal log record and flush the log synchronously (ADR-067), so the registered log file holds it;
3. hand over to the platform layer's `FailFast` (PLT-127), which breaks into an attached debugger, or shows `ui.error.internal` with the run id when a window exists, and ends the process with a fail-fast exception, which Windows Error Reporting captures with the registered files and the black box (ADR-065).

The handlers of PLT-118 reach steps 1 and 2 through the platform's log sink (`PlatformLogSink::RecordFatal`, PLT-133), which this component implements.

**DBG-074** (MUST) `FatalExit(string id, arguments)` serves failures of the environment, which are not defects (PLT-044): it writes the string id and arguments to the black box and as a fatal log record, flushes the log, and hands over to the platform layer's `EndWithMessage`, which shows the message and ends the process with `plt.lifecycle.exit_code.fatal` (PLT-044); no dump is written.

**DBG-075** (MUST) The black box is updated as the program runs: the frame loop notes each frame index and the tick clock notes each tick (`NoteFrame`, `NoteTick`); the outermost markers of a frame and a tick are noted as markers; loading a map or entering a region notes its id; each warning and each failed development check is noted as a check message. Updates are atomic stores into the ring slots, never allocations.

### 4.3 Profiling

**DBG-080** (MUST) The profiling layer is ADR-068's: `ENGINE_PROFILE_SCOPE` and its GPU counterpart `ENGINE_PROFILE_GPU_SCOPE` emit PIX events through WinPixEventRuntime in Debug and Release builds alike, and no other code calls the PIX runtime; a GPU scope also writes a timestamp query pair, resolved into a readback buffer and read `dbg.profile.gpu_readback_latency` (2) or more frames later, so the CPU never waits for the GPU to measure it. System-wide problems are captured as ETW traces with the TraceLogging provider of DBG-064.

> **Amendment 2026-10-20 (M1 task T10; [ADR-110](../../../ADR/ADR-110-cost-measurement-marker-timing-and-performance-script.md)).** A CPU scope also records its time inside its event while marker timing is on (`NeuronCore/MarkerTimings`): the performance tool turns it on to read per-object costs in-process, and nothing else does. Off, a scope costs one relaxed atomic load more.

**DBG-081** (MUST) The same markers feed the profile tree. Each thread's tree is rooted at a root node. Entering a marker finds or creates the child of the current node with the marker's name and starts its timer; leaving it adds one call and the elapsed time to the node, updates the node's shortest and longest call, and makes the parent current again. Nesting follows call order only; a leave whose name differs from the current node's fails a development check (`ENGINE_ASSERT`). The trees exist in every Debug build, whether or not the profile view is open, and the profile view and the overlay read them; in Release builds the markers feed PIX only (DBG-004). Every node is timed whether or not the view shows it, so a collapsed subtree still shows its time.

**DBG-082** (MUST) Once every `dbg.profile.rollup_period_ms` (1000 ms) of the platform clock, checked once per frame, the profiling service rolls every node over: the current period's calls and time become the last period's, the last period's are added to the history (one more history period), and the current period restarts; the length of the period just ended is recorded as now − (end of period − `dbg.profile.rollup_period_ms`), and the next period ends `dbg.profile.rollup_period_ms` after now. The roll-over runs in every frame loop, the front-end loop included.

**DBG-083** (MUST) Each node keeps the fields of `ProfileNodeStats`: calls and time of the current and last periods, time, periods and calls since the last history reset, and the shortest and longest call since that reset. The shortest call starts at the largest representable value and the longest at 0.

**DBG-084** (MUST) `ResetHistory` clears the history, shortest and longest of every node of every tree. `MaxChildTime(node)` is the largest child history time divided by the history periods of the node's first child in name order. A node is looked up by its path of names from the root.

**DBG-085** (MUST) After every frame the frame time in milliseconds is pushed to the front of the frame-time history, which keeps the newest `dbg.profile.frame_history` (200) frames.

**DBG-086** (MUST) The **profile view** is a tool window of `dbg.profile.view.width` × `dbg.profile.view.height` (570 × 600 px) at (W − `dbg.profile.view.offset_x`, `dbg.profile.view.y`) = (W − 590, 30). It has two buttons on the row at y = `dbg.profile.view.button_y` (18): reset history at x = `dbg.profile.view.reset_x` (190), which calls `ResetHistory`, and the statistic choice at x = `dbg.profile.view.statistic_x` (330), whose caption cycles minimum → average → maximum → minimum. A column header (`ui.dev.profile.header`) is right-aligned at x + `dbg.profile.view.statistic_x`.

**DBG-087** (MUST) The view lists the children of each expanded node in name order, depth first from the root, which is always expanded, and skips a node whose history average (history time ÷ history periods) is 0. Each row shows: an indent of one step per depth; a marker that reads "+" for a collapsed node with children, "−" for an expanded one and a blank for a leaf; the name padded to `dbg.profile.view.name_width` (24) characters minus the indent; the last period's calls; the time per call in ms with one decimal; the last period's total time in ms with no decimals; the chosen statistic (shortest call, history average per period or longest call) in ms with one decimal; and, for a node of a frame-graph pass, its GPU time from the timestamps.

**DBG-088** (MUST) Each row is `dbg.profile.view.row_height` (12 px) high. Its brightness is `dbg.profile.view.brightness_min` + `dbg.profile.view.brightness_range` × time ÷ largest time among the rows, clamped to [`dbg.profile.view.brightness_min`, `dbg.profile.view.brightness_min` + `dbg.profile.view.brightness_range`] = [105, 255]. A bar starts at x + `dbg.profile.view.bar_x` (340), is √(time in ms) × `dbg.profile.view.bar_scale` (10) px long and `dbg.profile.view.bar_height` (11 px) high, in `dbg.profile.view.bar_rgb` (150, 150, 250) with the row's brightness as alpha, drawn as an outline when the node is expanded and filled when it is collapsed. A primary press inside a row's band toggles that node's expansion. The window grows by one row height for every row past its height.

**DBG-089** (MUST) The bottom `dbg.profile.view.graph_height` (100 px) of the view hold the frame-time graph: guide lines at the frame times of `dbg.profile.view.graph_marks_ms` (200, 100 and 50 ms, that is 5, 10 and 20 frames per second), each labelled with its time and rate; one red vertical line per frame of the frame-time history, newest at the right, of height `dbg.profile.view.graph_height` × frame time ÷ `dbg.profile.view.graph_full_ms` (200 ms), that is 0.5 px per ms, so a 200 ms frame fills the graph.

**DBG-090** (MUST) Subsystem profilers belong to their subsystems: the audio engine brackets its update, update steps, stream task and bank loads, and counts the cost of triggers per entry kind and of stream reads per event (AEN-322); the profile view and the audio page show them.

## 5 Concurrency and performance budget

Diagnostics stay outside the simulation budget (DBG-200). The costs of a log call, of a logging flood and of the markers of a frame are budgets of DBG-206 ([performance baseline](performance_baseline.md) §5.4).

## 6 Failure modes, diagnostics and telemetry

The failures of logging, checks and profiling are rows of the failure table of DBG-220 ([README](README.md) §6).

## 7 Acceptance tests

| # | Kind | Test | Requirements |
|---|---|---|---|
| T1 | unit | A string holding format specifiers and braces, logged as an argument, appears verbatim in the file and ETW sinks. | DBG-060 |
| T2 | performance | Eight threads log 10⁶ debug records; the per-call cost and the frame-time increase stay within their budgets; warnings logged during the flood all arrive. | DBG-061, DBG-206 |
| T3 | unit | A full buffer drops trace before debug records, never warnings; the drop counts appear in the next drain warning. | DBG-061, DBG-062 |
| T4 | unit | A 20-byte hex dump gives two lines of 16 and 4 bytes. | DBG-063 |
| T5 | crash | A failed release check on the main thread, on a worker thread and through the terminate path each produce a dump with the registered log ending in the fatal record and a black box holding the message, condition, file and line; the six crash-test commands each produce a report (platform layer T22). | DBG-047, DBG-073, DBG-117 |
| T6 | unit | A captured dump and log of a session with a seeded chat message, account token and Windows user name contain none of the three strings, and the black box shows paths only relative to their placeholders. | DBG-047, DBG-066 |
| T8 | unit | With the manual clock, nested markers of known durations give the expected calls, current, last and history times, shortest and longest; after `ResetHistory` the history is empty. | DBG-081 to DBG-084 |
| T9 | unit | With the manual clock, a roll-over 1.2 s after the previous one records a period of 1.2 s and schedules the next end 1 s later; it also runs in the front-end loop. | DBG-082 |
| T12 | unit | GPU timestamps are read with no fence wait on the current frame, in a Debug and in a Release build. | DBG-080 |
| T13 | golden-image | The profile view with a fixed tree shows rows, bars of √ms × 10 px, brightness 105 to 255 and a 100 px graph where a 200 ms frame fills the height. | DBG-086 to DBG-089 |

## 8 Open decisions

The decisions and design-phase inputs of the whole component are in the [README](README.md) §8.

## 9 Tuning table

A value whose notes say "Species" is a Species engine-baseline value; every other value is the reference tool's value, kept as the starting value (DEC-004).

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `dbg.log.files_kept` | 10 | log files | Species; one per run, oldest deleted | DBG-049 | — |
| `dbg.log.thread_buffer_kib` | 256 | KiB | Species; per thread that logs | DBG-061 | — |
| `dbg.log.hex_bytes_per_line` | 16 | bytes | | DBG-063 | — |
| `dbg.blackbox.block_bytes` | 4096 | bytes | Species; registered with WER | DBG-047 | — |
| `dbg.blackbox.recent_markers` | 16 | markers | Species; frame and tick markers | DBG-047, DBG-075 | — |
| `dbg.blackbox.check_message_max_chars` | 512 | characters | the reference's message length; the log keeps the full text | DBG-047 | — |
| `dbg.blackbox.recent_checks` | 8 | messages | Species; warnings and failed checks | DBG-047, DBG-075 | — |
| `dbg.profile.gpu_readback_latency` | 2 | frames | Species; minimum delay before reading timestamps | DBG-080 | — |
| `dbg.profile.rollup_period_ms` | 1000 | ms | reference: one second | DBG-082 | — |
| `dbg.profile.frame_history` | 200 | frames | | DBG-085, DBG-089 | — |
| `dbg.profile.view.width` | 570 | px | | DBG-086 | — |
| `dbg.profile.view.height` | 600 | px | reference; grows with rows | DBG-086, DBG-088 | — |
| `dbg.profile.view.offset_x` | 590 | px | x = W − 590 | DBG-086 | — |
| `dbg.profile.view.y` | 30 | px | | DBG-086 | — |
| `dbg.profile.view.button_y` | 18 | px | | DBG-086 | — |
| `dbg.profile.view.reset_x` | 190 | px | | DBG-086 | — |
| `dbg.profile.view.statistic_x` | 330 | px | also the header's right edge | DBG-086 | — |
| `dbg.profile.view.name_width` | 24 | characters | minus the indent | DBG-087 | — |
| `dbg.profile.view.row_height` | 12 | px | | DBG-088 | — |
| `dbg.profile.view.brightness_min` | 105 | alpha | | DBG-088 | — |
| `dbg.profile.view.brightness_range` | 150 | alpha | maximum 255 | DBG-088 | — |
| `dbg.profile.view.bar_x` | 340 | px | | DBG-088 | — |
| `dbg.profile.view.bar_scale` | 10 | px per √ms | | DBG-088 | — |
| `dbg.profile.view.bar_height` | 11 | px | | DBG-088 | — |
| `dbg.profile.view.bar_rgb` | (150, 150, 250) | RGB | | DBG-088 | — |
| `dbg.profile.view.graph_height` | 100 | px | | DBG-089 | — |
| `dbg.profile.view.graph_marks_ms` | 200, 100, 50 | ms | 5, 10 and 20 frames per second | DBG-089 | DEC-225 |
| `dbg.profile.view.graph_full_ms` | 200 | ms | frame time at full graph height | DBG-089 | — |
