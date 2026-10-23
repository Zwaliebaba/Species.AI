# Developer tools

> Status: Engine baseline · Prefix: DBG · ADRs: [ADR-056](../../../ADR/ADR-056-products-configurations-and-feature-flags.md) · [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md) · [ADR-008](../../../ADR/ADR-008-gpu-memory-uploads-and-device-removal.md) · [ADR-065](../../../ADR/ADR-065-crash-reporting.md) · Folder: [diagnostics, test and performance](README.md) · Related: [input](../input/README.md) · [UI framework](../ui_framework/README.md) · [entity and overlay renderers](../renderer/entity_and_overlay_renderers.md) · [camera and sequences](../camera_and_sequences.md)

This document specifies the developer tools of the engine: the development overlay, the network statistics window, the debug commands and the debug-command window, development drawing with the bot overlay, the development text, and the classification of the reference cheats. The debug entry points and development bindings, the counter-provider contract and the hidden development settings are in the [README](README.md) §2.2, §2.3 and §3.2.

## 1 Responsibilities and boundaries

This document holds items 4 to 7 of DBG-001 ([README](README.md) §1). Drawing the development drawing pass and the text overlays is the renderer's (DBG-003).

## 2 Interfaces

The development drawing queue and the debug-command declarations are those of Listing 2.1-A ([README](README.md) §2.1); the debug actions are DBG-020, the counter providers DBG-023 and the debug-command payload DBG-024. The tools run on the presentation thread (DBG-029), and debug commands enter the simulation only through the command path (DBG-030).

## 3 Data model

**Development text.** Development captions are developer-facing, in the base language only, and live in the development string table, which uses `strings.dev_en_us`: only the editions that are never released hold it, and no player edition contains it (ADR-056). New text, keyed as follows; arguments are ICU MessageFormat arguments (ADR-063):

| String id | Text |
|---|---|
| `ui.dev.overlay.frame_rate` | {fps} fps |
| `ui.dev.overlay.frame_rate_entities` | {fps} fps · {count} objects |
| `ui.dev.profile.title` | Profile |
| `ui.dev.profile.header` | calls × ms per call = ms |
| `ui.dev.profile.reset` | Clear history |
| `ui.dev.profile.statistic.minimum`, `.average`, `.maximum` | Shortest · Mean · Longest |
| `ui.dev.netstats.title` | Network statistics |
| `ui.dev.netstats.host_frame`, `.send`, `.receive` | Host frame {n} · Sent {rate} KiB/s · Received {rate} KiB/s |
| `ui.dev.netstats.col.id`, `.address`, `.frame`, `.name`, `.status` | Id · Address · Frame · Team · State |
| `ui.dev.netstats.lost` | No messages for {seconds} s |
| `ui.dev.netstats.catching_up` | Catching up {percent} % |
| `ui.dev.netstats.latency` | Lag {ms} ms |
| `ui.dev.netstats.diverged` | Diverged |
| `ui.dev.netstats.empty` | Free slot |
| `ui.dev.netstats.queue_hint` | Incoming frames: best is zero or one waiting, none missing |
| `ui.dev.netstats.queue_label` | Frame {n} |
| `ui.dev.netstats.host_known`, `.host_estimated`, `.latency_estimate` | Host frame known {n} · Host frame estimated {n} · Estimated lag {seconds} s |
| `ui.dev.netstats.client_frame` | Own frame {n} |
| `ui.dev.command.title` | Debug commands |
| `ui.dev.command.kill_all`, `.spawn_workers`, `.spawn_transport`, `.spawn_crawlers`, `.spawn_orbs`, `.arbitrary_placement`, `.spawn_at_ports`, `.spawn_crate`, `.crate_normal` | Defeat enemies · Add Workers ({team}) · Add Transport · Add Crawlers · Add Essence orbs · Free placement · Fill structure ports · Drop Supply crate · Random outcome |
| `ui.dev.command.profile_view`, `.network_stats`, `.reset_render` | Profile · Network · Rebuild GPU resources |
| `ui.dev.command.crash_main`, `.crash_worker`, `.crash_terminate`, `.crash_invalid_parameter`, `.crash_pure_call`, `.hang_main` | Fault: main thread · Fault: worker thread · Fault: terminate · Fault: invalid parameter · Fault: pure call · Hang: main thread |
| `ui.dev.command.dump_bindings` | Write bindings to the log |
| `ui.match.cheats_used` | Debug commands were used in this session |

## 4 Behaviour and algorithms

### 4.4 Development overlay

**DBG-095** (MUST) The **frame-rate line** of the overlay is drawn in pass 29 (RND-076) in the body text role ([layout](../ui_framework/layout_rendering_input_and_localisation.md) §4), right-aligned at (W − `dbg.overlay.frame_rate_right` (10), H − `dbg.overlay.frame_rate_bottom` (40)), and only while the interface is shown. While a match world exists it shows the frame rate and the number of live simulated objects (`ui.dev.overlay.frame_rate_entities`); elsewhere it shows the frame rate alone (`ui.dev.overlay.frame_rate`). The frame rate is the renderer's counter of frames completed in the last `ref.sim.frame.fps_window` (RND-011, R-SIM-576). Its visibility is the renderer's frame-rate overlay flag of RND-011, off by default and absent from Release builds.

> **Amendment 2026-10-19 (O62).** The frame-rate line, every page of the overlay, the development windows and the text of the development drawing queue are drawn in the development font role, with the development face EditorFont (`font.dev.editor`), not in the body role ([layout](../ui_framework/layout_rendering_input_and_localisation.md) UI-195 amendment of O62). The face exists in Debug builds only, like the overlay.

> **Amendment 2026-10-20 (M1 task T15a, [ADR-118](../../../ADR/ADR-118-development-overlay-at-m1.md)).** Until the input router of M1 task T15b binds `action.debug.overlay`, a Debug build shows the frame-rate line on every frame; T15b restores the default of off. The line's object count is the size of the client's replica of its interest set (ADR-086), since a client holds no other. The two strings are literals that cite their ids until the string table is data. EditorFont holds no U+00B7, so the separator of `ui.dev.overlay.frame_rate_entities` draws as the face's missing glyph until the face holds it.

> **Amendment 2026-10-21 (M1 task T15b, [ADR-120](../../../ADR/ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md)).** `action.debug.overlay` is bound (F5), and the line is off by default again.

> **Amendment 2026-10-22 (O92, [ADR-147](../../../ADR/ADR-147-the-directwrite-text-system-at-m2.md)).** EditorFont is retired. The development role draws from Cascadia Mono, or Consolas where that is not installed, through DirectWrite, at `dbg.overlay.text_size` × the DPI scale. The line's distances from the edges, `dbg.overlay.frame_rate_right` and `dbg.overlay.frame_rate_bottom`, are scaled by the DPI scale too.

> **Amendment 2026-10-23 (O106, [ADR-176](../../../ADR/ADR-176-the-frame-page-under-the-settlement-panel.md)).** In a world, where the settlement HUD holds the bottom-right corner, the line is the frame page's first row, and the page stands under the HUD's settlement panel on a dark backdrop. On the front end the line keeps its anchor at the bottom right.

**DBG-096** (MUST) The **frame page** shows, besides the frame-rate line: the frame-time graph of DBG-089; the percentiles `dbg.telemetry.percentiles` (50th, 95th and 99th) of the frame-time history; the CPU share of each root profile node, as its last-period time × 100 ÷ the length of the last period, in percent; and the live entity counts per actor kind and per structure kind, with the counts of Essence orbs, laser bolts, effects and particles.

> **Amendment 2026-10-21 (M1 task T15b, [ADR-120](../../../ADR/ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md)).** At M1 the root profile nodes are the frame loop's steps (input, session, presentation, render and the renderer's wait), from a frame profile rather than the profile tree. The frame-time graph is drawn in the page, under its rows. The counts are each team's living Workers and Leaders, the Essence orbs and the effects; M1 has no structures, laser bolts or particles. Two budget rows are shown (DBG-097). The frame's budget row reads the frame's work, its time less the renderer's waits, against RND-454's target, since a synchronised display holds every frame at the display period. The server's row reads its longest tick of the last second against half the tick period (ADR-080 item 6), from the statistics a Debug server sends.

**DBG-097** (MUST) A **budget row** shows a measured value against its budget (`CounterSink::Budget`); a value above its budget is drawn in the warning colour `dbg.overlay.warning_rgb` (255, 64, 64), and nothing else changes. The `audio.update` share of the frame page is drawn in the warning colour above `dbg.overlay.audio_share_warning` (15 %) of the frame.

**DBG-098** (MUST) The overlay shows one page at a time; `action.debug.overlay` shows and hides it, and the page list of DBG-023 is stepped with the menu actions while the overlay holds the focus. The overlay and every page of it exist in Debug builds only (DBG-004).

**DBG-099** (MUST) The **world layers** of the overlay are drawn with the development drawing primitives of §4.7, each switched on separately in Debug builds: the bot overlay (DBG-135); hit volumes of structures, trees and actors; route nodes and routes; the actor grid, the obstacle grid and the actor-grid cache, each as cell outlines with their counts; the spider foot homes, targets and states, the cloth-flag bounding spheres and the Crawler trail samples ([animation and VFX](../animation_and_vfx/explainers_sprites_and_procedural_animation.md) §6); Transport waypoint and attack-target arrows; airstrike approach and exit arrows; and Hive health and occupancy text.

**DBG-100** (MUST) The development overlay and the developer tools of this document also host the developer tools of other components, each in Debug builds only (DBG-004): the developer fly mode of the camera (CAM-080 to CAM-084), driven by the development bindings of DBG-020; the camera overlay (CAM-125) on the camera page; the page host's current page, stack, waiting state, pending request and last refused request (UI-482) on the ui page; the input counters (INP-241) on the input page; and the binding dump of the input router (INP-242) as a local command of the debug-command window (DBG-117 row 23).

### 4.5 Network statistics window

**DBG-105** (MUST) The network statistics window is a tool window (UI-166) of Debug builds (DEC-041). It shows the reference metric set of DBG-106 to DBG-110, in correct units, until the design phase defines the Species networking metrics (see DEC-208), whose per-client and per-server metrics then replace the rows; its captions use `ui.dev.netstats.*` string ids (§4.11).

> **Amendment 2026-10-18 (ADR-088; ADR-084).** The Species metrics of DEC-208 are those of ADR-088 item 7 (option B): the host block shows the per-server metrics, read through the administration interface, each participant row one connection's metrics, and the participant block the client's own connection. No client steps the simulation (ADR-084), so the command frames, the catching-up state, the frame-queue strip and the divergence column of DBG-107 to DBG-109 have no counterpart; the layout of DBG-106 and the rates of DBG-110 stand.

**DBG-106** (MUST) The window is `dbg.netstats.width` × `dbg.netstats.height` (600 × 200 px). On a host its height is `dbg.netstats.host_height_base` + (`dbg.netstats.max_client_rows` × `dbg.netstats.row_pitch` + `dbg.netstats.table_padding`) = 170 + (6 × 20 + 15) = 305 px, and an inset panel (UI-061) at (`dbg.netstats.inset_x`, `dbg.netstats.inset_y`) = (10, 110), as wide as the window less 2 × `dbg.netstats.margin` (10 px) and `dbg.netstats.inset_height` (135 px) high, holds the participant table. The window is placed at (`dbg.netstats.margin`, H − its height) = (10, H − h). Text uses the body role, and text lines are `dbg.netstats.line_pitch` (15 px) apart, the first one line pitch below the window top.

**DBG-107** (MUST) **Host block** (only on the host), text size `dbg.netstats.host_text_size` (12) at the window's x + `dbg.netstats.margin`: the number of the last command frame issued; the send rate; the receive rate. A horizontal rule follows, then a header row (text size `dbg.netstats.header_text_size`, 14) with the columns id, address, last frame, name and status at the offsets `dbg.netstats.columns` (0, 60, 200, 260 and 410 px) from x + `dbg.netstats.column_origin` (20 px), and the divergence column at offset `dbg.netstats.divergence_column` (500 px).

> **Amendment 2026-10-18 (ADR-088).** The last command frame and the divergence column give way to the per-server metrics of the DBG-105 amendment.

**DBG-108** (MUST) Up to `dbg.netstats.max_client_rows` (6) participant rows follow, `dbg.netstats.row_pitch` (20 px) apart; slots from the seventh on are not shown (DEC-208 item 7). For each connected participant: its id, in `dbg.netstats.id_rgba` (200, 200, 255, 200); its address and port; its last acknowledged frame; the name of the team kind (`TeamKind`) of the lobby team whose participant id matches, in that team's colour; and its status:

1. when more than `dbg.netstats.lost_after_ms` (2000 ms) have passed since its last message: the lost text with the whole seconds since that message, in red;
2. otherwise, while it has not caught up: the catching-up text with 100 × its last acknowledged frame ÷ the host's last frame, in percent, in `dbg.netstats.catching_up_rgb` (200, 200, 30);
3. otherwise: its latency, estimated as (host's last frame − its last acknowledged frame) × the tick period in ms, in the colour red = min(255, `dbg.netstats.lag_red_step` (20) × the frame lag), green = 255 − red, blue 0.

The divergence column shows the diverged text in red while the participant has a recorded divergence frame. An empty slot shows the empty text at alpha `dbg.netstats.empty_alpha` (50).

> **Amendment 2026-10-18 (ADR-088).** A row shows its connection's metrics of ADR-088 item 7 (round trip, jitter, loss, rates, the age of the newest state, the reliable backlog and the rejected commands); the acknowledged frame, the catching-up percentage, the frame lag and the divergence text have no counterpart (DBG-105 amendment).

**DBG-109** (MUST) **Participant block** (whenever the process is connected to a host): when at least one frame has been processed, a caption (text size `dbg.netstats.queue_caption_size`, 13) at the window bottom − `dbg.netstats.queue_caption_rise` (50 px) states that the ideal queue holds 0 or 1 frames with no gaps; below it a label with the last processed frame number at the bottom − `dbg.netstats.queue_label_rise` (25 px), and a strip starting at the bottom − `dbg.netstats.strip_rise` (30 px) of `dbg.netstats.strip_boxes` (10) boxes for the frames last + 1 to last + 10, each (window width − `dbg.netstats.strip_side_space` (120 px)) ÷ 10 = 48 px wide less a `dbg.netstats.strip_gap` (20 %) gap and `dbg.netstats.strip_box_height` (20 px) high, with the background `dbg.netstats.strip_background_rgb` (20, 20, 50), filled `dbg.netstats.strip_fill_rgb` (0, 255, 0) when that frame is in the participant's incoming queue, and outlined `dbg.netstats.strip_outline_rgba` (255, 255, 255, 100). A participant that is not the host also shows the last host frame it knows, the host frame it estimates, and its estimated latency in seconds with one decimal, centred, at text size `dbg.netstats.latency_text_size` (15). A right column at x + `dbg.netstats.right_column_x` (250 px) from the top shows the last valid frame received from the host and the participant's send and receive rates.

> **Amendment 2026-10-18 (ADR-084; ADR-088).** A client holds no frame queue: the block shows its connection's metrics, the interpolation buffer among them (ADR-088 item 7), in place of the frame strip and the host-frame estimates (DBG-105 amendment).

**DBG-110** (MUST) Every rate is in bytes per second averaged over windows of `dbg.netstats.rate_window_ms` (5000 ms): when more than one window has passed since the window start, the rate becomes the bytes counted ÷ the time since the window start and the count restarts. The window shows rates in KiB/s with one decimal and states whether protocol headers are counted (DEC-208 item 3). Each rate is updated at the end of every window, whether or not traffic was counted in it, and the round trip is measured directly once the transport provides it (DEC-208 item 2).

### 4.6 Debug commands and the debug-command window

**DBG-115** (MUST) The debug-command window exists in Debug builds only. `action.debug.command_window` opens it while no other UI window is open. It is a tool window of `dbg.commands.window_size` (200 × 200 px) at `dbg.commands.window_pos` (250, 50); buttons that do not fit grow it (UI-025). Buttons start at y = `dbg.commands.first_row_y` (25) with a row pitch of `dbg.commands.row_pitch` (20 px). With m = `dbg.commands.margin` (10 px) and g = `dbg.commands.pair_gap` (5 px): a full-width button is at x = m and the window width − 2m = 180 px wide; a pair of buttons is each (window width − 2m − g) ÷ 2 = 87 px wide, the second at x = m + 87 + g = 102; the Supply crate row has its button at x = m, the window width ÷ 2 − 2m = 80 px wide, and the outcome drop-down at x = the window width ÷ 2 = 100, the window width ÷ 2 − m = 90 px wide. Every button is in the window's navigation order, so a gamepad reaches it (UI-032). Captions are `ui.dev.command.*` string ids.

**DBG-116** (MUST) Every spawn command targets the point P where the pick ray through the screen centre (W ÷ 2, H ÷ 2) meets the terrain ([camera](../camera_and_sequences.md) CAM-090, [terrain](../terrain/README.md)); without a hit, no command is sent.

**DBG-117** (MUST) The window offers these commands, in this order; "sequenced" commands change simulation state and follow DBG-118:

| # | Command | Effect when it executes | Kind |
|---|---|---|---|
| 1 | kill all enemies | for every team that is not a friend of the sender's team: every actor of every actor group and every ungrouped actor takes a health change of −`dbg.commands.kill_health_factor` (2) × its health stat; then every Hive and every Spitter Plant of any team takes `dbg.commands.kill_structure_damage` (999) damage | sequenced |
| 2–7 | spawn Workers, one button each for team slots 0, 1, 2 and 3, the Rogue faction and the Visitor faction | `dbg.commands.spawn_workers` (20) Workers of that team at P, at zero velocity, scattered within `dbg.commands.spawn_spread` (30 wu) by the spawn-position search ([space, routing and physics](../../../reference/12_simulation_model/05_space_routing_and_physics.md)) | sequenced |
| 8 | spawn a Transport | one Transport of team slot `dbg.commands.vehicle_team_slot` (2) at P, scatter 0 | sequenced |
| 9 | spawn Crawlers | `dbg.commands.spawn_crawlers` (20) Crawlers of the Rogue faction at P, scatter 0, with a roam range of `dbg.commands.crawler_range` (1000 wu) | sequenced |
| 10 | spawn Essence orbs | `dbg.commands.spawn_orbs` (10) Essence orbs of team slot 2, each at P + (`SimRng::NextSigned(dbg.commands.orb_jitter)`, 0, `SimRng::NextSigned(dbg.commands.orb_jitter)`) with `dbg.commands.orb_jitter` = 20 wu, that is ±10 wu, x drawn before z: 20 simulation draws | sequenced |
| 11 | allow arbitrary placement | turns off the sender team's verify-targeting flag, so the target-area test of [selection and Power slots](../../../reference/06_controls_camera_and_commands/selection_and_power_slots.md) R-CTRL-192 passes at its step 2 for that team's commands on every participant | sequenced |
| 12 | spawn at structure ports | for every structure and every one of its ports: `dbg.commands.port_workers` (3) Workers of team slot 0 at the port position, scattered within `dbg.commands.spawn_spread` (30 wu); only while a match world exists | sequenced |
| 13 | spawn a Supply crate, with an outcome drop-down | creates a Supply crate at (P.x, `dbg.commands.crate_height` (300 wu), P.z) at render detail 1, with a new structure id and its world marker; the drop-down lists every outcome of the catalogue in catalogue order ([crates and powers](../../../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md)) after the default entry, normal choice; a chosen outcome is forced, so the crate's capture roll is skipped (R-SIM-624); offered only while a match world exists | sequenced |
| 14 | profile view | opens the profile view (§4.3) | local |
| 15 | network statistics | opens the network statistics window (§4.5) | local |
| 16 | reset render resources | releases and rebuilds every GPU resource through the device-removal path (ADR-008); trees rebuild their geometry from the same per-tree seed, and their hit volumes are baked data (R-SIM-651), so nothing in the simulation changes | local |
| 17–22 | crash tests | a fault on the main thread, a fault on a worker thread, a call of the terminate handler, an invalid parameter, a pure call, and a hang of the main thread, for the crash-capture tests of ADR-065 and the platform layer | local |
| 23 | dump bindings | writes every action with its bindings, its glyph override and the binding count to the log, through the input router's `DumpBindings` (INP-242) | local |

> **Amendment 2026-10-19 (ADR-082).** The commands that draw (row 10 and any other) make their `SimRng` from a draw context keyed as ADR-082 item 3 states, with the console command as its purpose, the spawned object as its subject and the tick, never from a match-wide sequential stream, so a console command never shifts another draw.

**DBG-118** (MUST) A sequenced debug command is a player command (DBG-024) that the authority accepts only in a session created with cheats enabled, and only from a participant authorised to send it; until DEC-202 is decided, the host's own participant is the only authorised sender. The authority validates the team slot, that P lies on the map and that a forced outcome is in the catalogue; a rejected command is dropped, logged and counted. An accepted command executes on its tick on every participant, like any command (R-SIM-670). The first accepted debug command sets the session's cheats-used marker, which the lobby and the match results show (`ui.match.cheats_used`) and which the match statistics carry ([reference/09](../../../reference/09_front_end_and_match_flow/README.md) hooks). The marker suppresses nothing: the achievement, leaderboard and statistics hooks run as in any other session (see DEC-972).

> **Amendment 2026-10-18 (ADR-087; ADR-084).** The authority is the server: an accepted command executes once, on the state owner that it addresses, in the first tick after it arrives (ADR-087 items 1 and 4), and reaches clients by replication; no participant applies it, and row 11 of DBG-117 holds on the server only. The world host's own client is the authorised sender until DEC-202 is decided, and the world record carries the cheats-used marker (ADR-087 item 8); the lobby and the match results that show it are the skirmish's.

**DBG-119** (MUST) Local commands (rows 14 to 23) and the special-lighting actions of DBG-020 never travel as commands and never touch simulation state.

**DBG-120** (MUST NOT) The window offers only the commands of DBG-117. It opens no sound-editor or sound-statistics window: their content is on the audio page of the overlay (AEN-323).

**DBG-121** (MUST) In the reference every cheat changes the local copy of the simulation directly and sends nothing; a cheat that changes simulation state is therefore a divergence in any session with two or more participants, and a session with one human participant and bot teams, which has nobody to compare with, never reports one. Species removes that behaviour: DBG-221 lists the classification.

### 4.7 Development drawing

**DBG-130** (MUST) The development drawing primitives are drawn in pass 24 (RND-076), in Debug builds only, unlit. Lines are drawn as a line list, one pixel wide; a line of a primitive whose width is above 1 px is drawn as a screen-facing quad of that width in pixels:

| Primitive | Drawing |
|---|---|
| 2-D square | a square in its own orthographic projection, depth test off |
| cube | the twelve edges, each `dbg.draw.cube_line_width` (2 px) wide |
| sphere rings | three orthogonal circles of `dbg.draw.ring_segments` (20) segments |
| sphere | the development sphere of RND-330 (80 triangles), as lines or filled |
| vertical cylinder | `dbg.draw.cylinder_edges` (16) side edges |
| arrow | a line whose width in pixels is the given width ÷ the distance to the camera × the arrow length, plus two head lines of `dbg.draw.arrow_head_fraction` (20 %) of its length |
| point marker | an arrow from the point + `dbg.draw.point_marker_offset` (20, 20, 20) wu to the point, and its label as 3-D text of size `dbg.draw.point_marker_text_size` (3) |
| matrix | written to the log, at most `dbg.draw.matrix_log_limit` (10) times per process |

**DBG-131** (MUST) The persistent drawing queue holds 2-D squares, point markers, spheres and arrows, starting with room for `dbg.draw.queue_initial` (100) items and growing as needed. Each item has a label of at most `dbg.draw.label_max` (63) characters; look-up and removal match a case-insensitive label prefix. Each item has a life in frames: 0 keeps it until it is removed; otherwise each drawing decrements it, and an item whose life is 1 is removed after it is drawn. The queue is drawn every frame and emptied at every match start and end; no engine path fills it except a developer's own code, so it is normally empty. Every item stores its label, so a prefix search matches only the items in the queue.

**DBG-132** (MUST NOT) Divergence markers (an arrow down to each object that differs between two states, with its name and a description as 3-D text) are not specified here; how a divergence is shown is DEC-206. The red divergence banner and the lost-connection banner are player-facing presentation hooks of the networking model (RND-076, DEC-200).

**DBG-135** (MUST) **Bot overlay.** While `debug.bot_overlay` is 1, every bot navigation node (`BotNavNode`, [navigation graph](../../../reference/07_bots/01_navigation_graph.md) R-BOT-025) draws the priority of one team as text with two decimals, outlined, in that team's colour, `dbg.bot_overlay.text_height` (60 wu) + `dbg.bot_overlay.text_step` (15 wu) × the team slot above the node; the team is the one at lobby position `debug.bot_overlay_team`. While `debug.bot_overlay` is not 0, every node also draws an arrow along each of its links, `dbg.bot_overlay.arrow_length_factor` (0.5) × the link length long and `dbg.bot_overlay.arrow_width` (3) wide, in grey when the node has no owning team and in its owning team's colour otherwise. The overlay is drawn in pass 16 (RND-359); priorities exist for human teams too (R-BOT-044).

**DBG-136** (MUST) The map editor's development drawing is a hook of [map editor hooks](../../../reference/08_maps_and_level_design/map_editor_hooks.md) (DEC-045), not of this component: bot objectives as spheres (red, blue for a defence objective, flashing white while active); bot objective markers with their sphere, scan circle and an arrow to their bound structure; the bot controller's sphere with its team counts; the link-range sphere of `ref.bot.link.range` (600 wu); structure facing arrows; the requirement and link arrows of grid structures and Beam Dishes; and the spheres of Control Zones, exclusion zones, jump pads, volcano vents and Population Limiters.

## 5 Concurrency and performance budget

The shown overlay page has the presentation-thread budget of DBG-206 ([performance baseline](performance_baseline.md) §5.4).

## 6 Failure modes, diagnostics and telemetry

**DBG-221** (MUST) The classification of every reference cheat (DEC-202 item 8) and its Species form:

| Reference cheat | Changes simulation state | With two or more participants in the reference | Species |
|---|---|---|---|
| kill all enemies | yes: the health of every enemy actor, Hives and Spitter Plants | divergence | sequenced command (DBG-117 row 1) |
| spawn Workers for one of six team slots | yes: 20 actors, object ids, population counts | divergence | sequenced (rows 2 to 7) |
| spawn a Transport | yes | divergence | sequenced (row 8) |
| spawn Crawlers | yes | divergence | sequenced (row 9) |
| spawn Essence orbs | yes, and 20 simulation draws on one participant only | divergence, also visible in a generator-position check | sequenced (row 10) |
| allow arbitrary placement | the local copy of the team's verify-targeting flag; the target command, executed everywhere, repeats the test | divergence at the first placement that is otherwise invalid | sequenced session rule (row 11) |
| grant all research | no change in a match | none | not specified (DBG-120) |
| reset render resources | GPU resources only; trees rebuild from the same seed | none | local (row 16) |
| spawn at structure ports | yes | divergence | sequenced (row 12) |
| spawn a Supply crate | yes: a new structure; the structure-id counter advances | divergence | sequenced (row 13) |
| profile view, sound editor, sound statistics, network statistics | no; audio is outside the simulation | none | local, or the audio overlay page (rows 14, 15) |

The failures of the developer tools are rows of the failure table of DBG-220 ([README](README.md) §6).

## 7 Acceptance tests

| # | Kind | Test | Requirements |
|---|---|---|---|
| T14 | unit | The network statistics window on a host is 305 px high; a participant 3 frames behind shows a latency of 300 ms in red 60, green 195; one silent for 2.1 s shows the lost text; rates over a 5 s window print in KiB/s with one decimal. | DBG-106 to DBG-110 |
| T15 | integration | In a two-participant session with cheats enabled, every sequenced command of DBG-117 gives identical per-tick hashes on both participants; in a session without cheats each is rejected and counted; the first accepted one sets the cheats-used marker. | DBG-117, DBG-118 |
| T16 | unit | The Essence orb command consumes exactly 20 simulation draws, x before z. | DBG-117 |
| T17 | unit | A forced-outcome Supply crate skips the capture roll; the normal choice draws it. | DBG-117 |
| T18 | unit | The drawing queue removes a life-1 item after one drawing, keeps a life-0 item, and a label-prefix search never matches a removed item. | DBG-131 |
| T19 | golden-image | With the bot overlay at 1 and team position 1, every navigation node shows that team's priority with two decimals at 60 + 15 × slot wu and half-length link arrows; at 2 only the arrows. | DBG-135 |
| T32 | integration | In a Debug build the camera page shows the camera overlay fields of CAM-125, the ui page shows the page host's fields of UI-482, and the dump-bindings command writes the binding dump of INP-242 to the log; a Release build holds none of them. | DBG-100, DBG-117 |

## 8 Open decisions

DEC-041 (Resolved), DEC-202 (Resolved, ADR-087), DEC-208 (Resolved, ADR-081) and DEC-972 (Open) apply to this document; their defaults are listed in the [README](README.md) §8.1, with the decisions and design-phase inputs of the whole component.

## 9 Tuning table

A value whose notes say "Species" is a Species engine-baseline value; every other value is the reference tool's value, kept as the starting value (DEC-004).

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `dbg.overlay.frame_rate_right` | 10 | px | from the right edge | DBG-095 | — |
| `dbg.overlay.frame_rate_bottom` | 40 | px | from the bottom edge | DBG-095 | — |
| `dbg.overlay.text_size` | 14 | px at a DPI scale of 1 | Species; the overlay's text in the development role (O92) | DBG-095 | — |
| `dbg.overlay.warning_rgb` | (255, 64, 64) | RGB | Species; over-budget rows | DBG-097 | — |
| `dbg.overlay.audio_share_warning` | 15 | % of the frame | | DBG-097 | — |
| `dbg.netstats.width` | 600 | px | | DBG-106 | DEC-208 |
| `dbg.netstats.height` | 200 | px | participant without host block | DBG-106 | DEC-208 |
| `dbg.netstats.host_height_base` | 170 | px | host height = 170 + (6 × 20 + 15) = 305 | DBG-106 | DEC-208 |
| `dbg.netstats.max_client_rows` | 6 | rows | reference; DEC-208 item 7 | DBG-106, DBG-108 | DEC-208 |
| `dbg.netstats.row_pitch` | 20 | px | | DBG-106, DBG-108 | — |
| `dbg.netstats.table_padding` | 15 | px | | DBG-106 | — |
| `dbg.netstats.inset_x` | 10 | px | | DBG-106 | — |
| `dbg.netstats.inset_y` | 110 | px | | DBG-106 | — |
| `dbg.netstats.inset_height` | 135 | px | | DBG-106 | — |
| `dbg.netstats.margin` | 10 | px | window position and text indent | DBG-106, DBG-107 | — |
| `dbg.netstats.line_pitch` | 15 | px | | DBG-106 | — |
| `dbg.netstats.host_text_size` | 12 | px | | DBG-107 | — |
| `dbg.netstats.header_text_size` | 14 | px | | DBG-107 | — |
| `dbg.netstats.column_origin` | 20 | px | from the window's x | DBG-107 | — |
| `dbg.netstats.columns` | 0, 60, 200, 260, 410 | px | id, address, last frame, name, status | DBG-107 | — |
| `dbg.netstats.divergence_column` | 500 | px | | DBG-107 | DEC-206 |
| `dbg.netstats.id_rgba` | (200, 200, 255, 200) | RGBA | | DBG-108 | — |
| `dbg.netstats.lost_after_ms` | 2000 | ms | | DBG-108 | DEC-208 |
| `dbg.netstats.catching_up_rgb` | (200, 200, 30) | RGB | | DBG-108 | — |
| `dbg.netstats.lag_red_step` | 20 | red per frame of lag | capped at 255 | DBG-108 | — |
| `dbg.netstats.empty_alpha` | 50 | alpha | | DBG-108 | — |
| `dbg.netstats.queue_caption_size` | 13 | px | | DBG-109 | — |
| `dbg.netstats.queue_caption_rise` | 50 | px | above the window bottom | DBG-109 | — |
| `dbg.netstats.queue_label_rise` | 25 | px | | DBG-109 | — |
| `dbg.netstats.strip_rise` | 30 | px | | DBG-109 | — |
| `dbg.netstats.strip_boxes` | 10 | boxes | | DBG-109 | — |
| `dbg.netstats.strip_side_space` | 120 | px | box width = (window width − 120) ÷ 10 | DBG-109 | — |
| `dbg.netstats.strip_gap` | 20 | % of a box | | DBG-109 | — |
| `dbg.netstats.strip_box_height` | 20 | px | | DBG-109 | — |
| `dbg.netstats.strip_background_rgb` | (20, 20, 50) | RGB | | DBG-109 | — |
| `dbg.netstats.strip_fill_rgb` | (0, 255, 0) | RGB | | DBG-109 | — |
| `dbg.netstats.strip_outline_rgba` | (255, 255, 255, 100) | RGBA | | DBG-109 | — |
| `dbg.netstats.right_column_x` | 250 | px | | DBG-109 | — |
| `dbg.netstats.latency_text_size` | 15 | px | | DBG-109 | — |
| `dbg.netstats.rate_window_ms` | 5000 | ms | reference averaging window | DBG-110 | DEC-208 |
| `dbg.commands.window_size` | 200 × 200 | px | | DBG-115 | — |
| `dbg.commands.window_pos` | (250, 50) | px | | DBG-115 | — |
| `dbg.commands.first_row_y` | 25 | px | | DBG-115 | — |
| `dbg.commands.row_pitch` | 20 | px | | DBG-115 | — |
| `dbg.commands.margin` | 10 | px | | DBG-115 | — |
| `dbg.commands.pair_gap` | 5 | px | | DBG-115 | — |
| `dbg.commands.kill_health_factor` | 2 | × health stat | health change −2 × the stat | DBG-117 | DEC-202 |
| `dbg.commands.kill_structure_damage` | 999 | damage | every Hive and Spitter Plant | DBG-117 | DEC-202 |
| `dbg.commands.spawn_workers` | 20 | Workers | per button | DBG-117 | DEC-202 |
| `dbg.commands.spawn_spread` | 30 | wu | | DBG-117 | — |
| `dbg.commands.vehicle_team_slot` | 2 | team slot | the Transport and Essence orb commands | DBG-117 | — |
| `dbg.commands.spawn_crawlers` | 20 | Crawlers | | DBG-117 | DEC-202 |
| `dbg.commands.crawler_range` | 1000 | wu | roam range | DBG-117 | — |
| `dbg.commands.spawn_orbs` | 10 | Essence orbs | 2 simulation draws each | DBG-117 | DEC-202 |
| `dbg.commands.orb_jitter` | 20 | wu | `NextSigned` range: ±10 wu | DBG-117 | DEC-013 |
| `dbg.commands.port_workers` | 3 | Workers per port | | DBG-117 | DEC-202 |
| `dbg.commands.crate_height` | 300 | wu | creation height of the Supply crate | DBG-117 | — |
| `dbg.draw.cube_line_width` | 2 | px | | DBG-130 | — |
| `dbg.draw.ring_segments` | 20 | segments | | DBG-130 | — |
| `dbg.draw.cylinder_edges` | 16 | edges | | DBG-130 | — |
| `dbg.draw.arrow_head_fraction` | 0.2 | of the arrow length | | DBG-130 | — |
| `dbg.draw.point_marker_offset` | (20, 20, 20) | wu | | DBG-130 | — |
| `dbg.draw.point_marker_text_size` | 3 | wu | | DBG-130 | — |
| `dbg.draw.matrix_log_limit` | 10 | log writes per process | | DBG-130 | — |
| `dbg.draw.queue_initial` | 100 | items | grows as needed | DBG-131 | — |
| `dbg.draw.label_max` | 63 | characters | | DBG-131 | — |
| `dbg.bot_overlay.text_height` | 60 | wu | above the node | DBG-135 | — |
| `dbg.bot_overlay.text_step` | 15 | wu per team slot | | DBG-135 | — |
| `dbg.bot_overlay.arrow_length_factor` | 0.5 | of the link length | | DBG-135 | — |
| `dbg.bot_overlay.arrow_width` | 3 | arrow width | | DBG-135 | — |
