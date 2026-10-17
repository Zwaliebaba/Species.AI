# Diagnostics, test and performance

> Status: Engine baseline · Prefix: DBG · ADRs: [ADR-0067](../../adr/0067-logging.md) · [ADR-0068](../../adr/0068-profiling-and-gpu-timing.md) · [ADR-0069](../../adr/0069-test-framework.md) · [ADR-0070](../../adr/0070-continuous-integration.md) · [ADR-0066](../../adr/0066-telemetry-with-consent.md) · [ADR-0065](../../adr/0065-crash-reporting.md) · [ADR-0056](../../adr/0056-editions-build-flavours-and-feature-flags.md) · [ADR-0057](../../adr/0057-version-identity.md) · [ADR-0074](../../adr/0074-platform-clock-with-injectable-time.md) · [ADR-0059](../../adr/0059-content-build-and-validators.md) · [ADR-0060](../../adr/0060-content-identity-by-hash.md) · [ADR-0001](../../adr/0001-direct3d-12-renderer-on-the-agility-sdk.md) · [ADR-0008](../../adr/0008-gpu-memory-uploads-and-device-removal.md) · Related: [tick timing and main loop](../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [randomness and integrity](../../reference/12_simulation_model/07_randomness_and_integrity.md) · [entity model](../../reference/12_simulation_model/03_entity_model_and_identity.md) · [containers and update order](../../reference/12_simulation_model/04_containers_and_update_order.md) · [space, routing and physics](../../reference/12_simulation_model/05_space_routing_and_physics.md) · [population flow](../../reference/05_population_flow.md) · [bots](../../reference/07_bots/README.md) · [map rules and conventions](../../reference/08_maps_and_level_design/rules_and_conventions.md) · [options and hooks](../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) · [platform layer](platform_layer.md) · [build tooling and distribution](build_tooling_and_distribution.md) · [renderer options and budgets](renderer/options_and_budgets.md) · [frame graph and passes](renderer/frame_graph_and_passes.md) · [UI framework](ui_framework/README.md) · [audio engine](audio_engine.md) · [asset pipeline and resources](asset_pipeline_and_resources.md) · [input](input.md) · [camera and sequences](camera_and_sequences.md)

This component is the Species engine's developer and quality infrastructure. It specifies the logging system, the assertion levels and the black-box record, the profiling layer and its in-game views, the development overlay, the development windows and debug commands that the engine keeps, the development drawing queue, the test harness with its headless simulation tests, the headless soak test and the opt-in telemetry data set. It also holds the **reference performance figures**: the population per map, the frame and tick structure, the hotspot catalogue, the stress populations and the budgets that the Species design phase starts from.

**Notation and units.**

- **Costs** (frame time, tick cost, call cost) are real time on the platform clock (ADR-0074), in milliseconds (ms), microseconds (µs) or nanoseconds (ns), measured on the reference test machine (§5.5).
- **Simulation durations** are in ticks at the reference tick rate, one tick per `ref.sim.tick_period` (0.1 s) of [reference/12](../../reference/12_simulation_model/01_tick_timing_and_main_loop.md); a duration in seconds next to a tick count is a derived display value. Delays of developer tools that run while no match world exists are counted in milliseconds of the platform clock, never accumulated from frame-time differences (DEC-804).
- **Distances** are in world units (`wu`). Positions and sizes of developer windows and overlays are in pixels of the 2-D overlay space, origin at the top-left corner and y growing downwards (RND-215), before the UI scale of UI-166; W × H is the render size.
- **Colours** are RGBA8 tuples (red, green, blue, alpha), or RGB triples when the alpha is 255.
- **Development builds** are the `Debug` and `Development` flavours of ADR-0056. **Test builds** are the development builds and every CI test configuration that runs the test harness (ADR-0070).

## 1 Responsibilities and boundaries

**DBG-001** (MUST) The diagnostics component owns:

1. the logging system: its API, categories, levels, pipeline, sinks and drop policy (§4.1);
2. the assertion levels, the black-box record and the fatal-check path up to its hand-over to crash handling (§4.2);
3. the profiling layer: the marker macros, the collection of GPU timestamps, the profile tree and its view (§4.3), and the marker catalogue (§3.4);
4. the development overlay, its pages and the counter-provider contract through which other components publish counters into it (§2.3, §4.4);
5. the development windows: the network statistics window (§4.5) and the debug-command window (§4.6);
6. the debug commands and their classification by effect on simulation state (§4.6, §6.2);
7. the development drawing queue, its primitives and the development overlay layers drawn with them, among them the bot overlay (§4.7);
8. the hidden development settings that switch these facilities (§3.2);
9. the test harness: unit, property, golden-hash, golden-image, fuzz and performance tests, the headless simulation tests and the per-tick test-build checks (§4.8);
10. the headless soak test (§4.9);
11. the assembly of the opt-in telemetry data set (§4.10);
12. the reference performance figures, the Species budget baseline, the stress populations and the reference test machine (§5).

**DBG-002** (MUST NOT) No diagnostic facility changes a simulation result. Overlays, windows, the profile tree and logging read presentation state, counters and the immutable simulation snapshot only (DEC-007, DEC-222). A debug command that changes simulation state does so only as a sequenced command (DBG-104). Instrumentation never changes arithmetic, the number of simulation draws or a drawn value ([randomness and integrity](../../reference/12_simulation_model/07_randomness_and_integrity.md) R-SIM-613). A check that inspects simulation state after a tick never writes it. Test T7 confirms that a match played with every diagnostic facility on gives the same per-tick simulation hashes as the same match played with every facility off.

**DBG-003** (MUST) The neighbouring components own these parts, and this component cites them and never restates their rules:

| Part | Owner |
|---|---|
| crash capture: no top-level exception filter, the terminate, invalid-parameter and pure-call handlers, fail-fast termination, the files and memory blocks registered with Windows Error Reporting, thread naming and dump contents | [platform layer](platform_layer.md), ADR-0065 |
| the symbol store, the CI pipeline definitions, binary checks, the edition manifests and the dedicated-server build | [build tooling and distribution](build_tooling_and_distribution.md), ADR-0070, ADR-0056 |
| drawing of the development drawing pass (pass 24) and the text overlays (pass 29); the development sphere | [frame graph and passes](renderer/frame_graph_and_passes.md) RND-076; [entity and overlay renderers](renderer/entity_and_overlay_renderers.md) RND-330 |
| the bot overlay's pass (pass 16) | [entity and overlay renderers](renderer/entity_and_overlay_renderers.md) RND-359 |
| tool-window chrome, close box, inset panel and layout | [UI framework](ui_framework/README.md) UI-001, [core and widgets](ui_framework/core_and_widgets.md) UI-060, UI-061, [layout](ui_framework/layout_rendering_input_and_localisation.md) UI-166 |
| each component's own markers, counters and development pages | renderer RND-041, RND-386, RND-454; audio engine AEN-322, AEN-323, AEN-325; asset pipeline AST-126; input INP-241; UI core and layout (their §6); animation and VFX (its §6); terrain TER-254; camera (its §6) and the developer fly mode CAM-080 |
| the renderer's budgets and reference caps | [renderer options and budgets](renderer/options_and_budgets.md) RND-453 to RND-457 |
| the language test | [options and hooks](../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-473 |
| the command-history recording hook | [randomness and integrity](../../reference/12_simulation_model/07_randomness_and_integrity.md) R-SIM-740 to R-SIM-742 |
| tick timing, update slices and catch-up | [tick timing and main loop](../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) |
| the networking metric set and the link simulator | DEC-208 (design phase) |
| run-time divergence detection | DEC-206 (design phase) |

This component composes those parts into one developer view and one test harness.

**DBG-004** (MUST) Each facility exists in these build flavours (ADR-0056); a facility marked "no" is absent from the binary, not merely disabled:

| Facility | `Debug` | `Development` | `Profile` | `Shipping` |
|---|---|---|---|---|
| log file and ETW sinks, info level and above | yes | yes | yes | yes |
| debug-level records, per-category trace records, debugger output sink | yes | yes | no | no |
| debug assertions (`SPECIES_ASSERT`) | yes | yes | no | no |
| release checks and container bounds checks | yes | yes | yes | yes |
| black-box record | yes | yes | yes | yes |
| PIX markers and GPU timestamps | yes | yes | yes | no |
| development overlay: performance pages (frame-time graph, pass and subsystem times, entity counts) and the profile view | yes | yes | yes | no |
| development overlay: every other page; development windows; debug commands; development drawing; hidden development settings | yes | yes | no | no |
| test hooks and the per-tick test-build checks | yes | yes | no | no |
| opt-in telemetry | yes | yes | yes | yes |

**DBG-005** (MUST) The reference player build contains these diagnostics, reached as stated; this is the reference that the Species facilities replace:

| Facility | Present in the reference player build | How it is reached | Species counterpart |
|---|---|---|---|
| network statistics window | yes | a key opens it in menus, the lobby and a match (DBG-020) | development builds (DBG-120) |
| debug log | yes | always written to the profile folder, truncated at start-up | the logging system, every flavour (§4.1) |
| release assertions and the assertion record | yes | any failed release assertion or container bounds check | release checks and the black box (§4.2) |
| crash log and minidump | yes | any unhandled exception on the main thread | crash capture on every thread ([platform layer](platform_layer.md), ADR-0065) |
| world-state dump on divergence | yes | the first divergence notice | design phase (DEC-206) |
| development drawing | compiled in; only the bot overlay is drawn | a hidden setting (§3.2) | development builds (§4.7) |
| profile tree | compiled in; not reachable | — | the profile view (§4.3) |
| frame-rate and entity-count overlay | compiled in; forced off | — | the development overlay (§4.4) |
| hidden development settings | yes | the settings file | development builds (§3.2) |
| debug-command window | compiled in; empty | — | development builds (§4.6) |

**DBG-006** (MUST) The Species debug and test baseline is:

1. **Debug commands.** Every developer cheat that changes simulation state is a sequenced debug command, which the authority accepts only in a session created with cheats enabled and every participant applies on the same tick; such a session carries a session-wide cheats-used marker (§4.6, DEC-041, DEC-202).
2. **Replay as the regression tool.** When the design phase gives Species reproducible replays (DEC-046), every match records its seed and command stream into a local ring buffer, the headless runner replays it at maximum speed, the end-of-match state hash must match, and a recorded match serves as the repeatable performance scene of ADR-0066 (§8.2).
3. **Headless soak test.** Bot teams play complete matches on every mode and map in CI, with no user interface driven (§4.9, DEC-804, ADR-0070).
4. **Profiling.** Scoped CPU markers that PIX and ETW traces show, GPU timestamps per pass, and an in-game overlay with a frame-time graph and per-system budgets; every marker is safe to use from any thread (§4.3, ADR-0068).
5. **Development console and overlays,** reachable in development builds: frame time, entity counts per type, network statistics with correct units, per-client lag in ticks and gaps in the incoming frame queue, bot overlays, and actor-grid and obstacle-grid visualisers (§4.4 to §4.7).
6. **Logging and crash reporting.** An asynchronous, buffered, categorised log; crash capture on every thread and through the terminate path; dumps with every thread; a symbol store for every build; no stack walk that depends on frame pointers; no credentials in a binary or a report (§4.1, §4.2, ADR-0065, ADR-0067).
7. **Telemetry with consent** (§4.10, ADR-0066).
8. **Validators in the content build** instead of run-time-only checks: the language completeness test and the content checks run in the content build (ADR-0059, R-META-473).

Run-time state hashing for divergence detection is DEC-206; the per-tick state hash of the test harness (DBG-160) is a test tool.

## 2 Interfaces

### 2.1 Interface sketch

```cpp
namespace species::diag {

// ---- Logging (ADR-0067) ----------------------------------------------------
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error, Fatal };     // severity order
enum class LogCategory : std::uint8_t {                                                // fixed set, extended only in code
  Platform, Render, Audio, Input, Ui, Content, Simulation, Network, Tools, Diagnostics, Test };

template <class... Args>
void Log(LogCategory category, LogLevel level, std::format_string<Args...> fmt, Args&&... args);
void LogHexDump(LogCategory category, LogLevel level, std::span<const std::byte> bytes);        // DBG-063
void LogValueDump(LogCategory category, LogLevel level, std::span<const std::int32_t> values);  // DBG-063
void SetCategoryLevel(LogCategory category, LogLevel minimum);   // development builds, at run time
struct LogStats { std::uint64_t written; std::uint64_t dropped_trace; std::uint64_t dropped_debug; };
LogStats GetLogStats();
void FlushLogSynchronously();                                    // fatal path only (DBG-073)

// ---- Checks and the black box (§4.2, ADR-0065) -----------------------------
#define SPECIES_ASSERT(condition)                   /* debug assertion: Debug and Development only   */
#define SPECIES_CHECK(condition, fmt, ...)          /* release check: every flavour, fatal           */
#define SPECIES_CHECK_INDEX(index, size)            /* container bounds check: every flavour, fatal  */
[[noreturn]] void CheckFailed(std::source_location where, std::string_view condition, std::string_view message);
[[noreturn]] void FatalExit(std::string_view message);          // orderly fatal exit that is not a crash

class BlackBox {                                                 // one per process, owned by the composition root
public:
  void NoteFrame(std::uint64_t frame_index);
  void NoteTick(std::uint64_t tick);
  void NoteMarker(std::string_view marker);                     // most recent frame and tick markers
  void NoteLocation(std::string_view map_or_region_id);
  void NoteCheck(std::string_view message);                     // most recent check messages
  std::span<const std::byte> MemoryBlock() const;               // registered by the platform layer
};

// ---- Profiling (ADR-0068) --------------------------------------------------
#define SPECIES_PROFILE_SCOPE(name)                 /* CPU marker; expands to nothing in Shipping    */
#define SPECIES_PROFILE_GPU_SCOPE(command_list, name) /* GPU marker plus a timestamp pair            */

struct ProfileNodeStats {
  std::uint32_t calls_current, calls_last;      // this roll-up period, previous roll-up period
  double ms_current, ms_last;                   // accumulated time
  double ms_history;                            // since the last history reset
  std::uint32_t history_periods, history_calls;
  double shortest_ms, longest_ms;               // per call, since the last history reset
};
class ProfileTree {                                              // one per thread that carries markers
public:
  ProfileNodeId Root() const;
  std::span<const ProfileNodeId> ChildrenByName(ProfileNodeId node) const;
  const ProfileNodeStats& Stats(ProfileNodeId node) const;
  std::optional<ProfileNodeId> Lookup(std::string_view path) const;
  void ResetHistory();                                           // DBG-084
  double MaxChildTime(ProfileNodeId node) const;                 // DBG-084
};
std::span<const float> RecentFrameTimesMs();                     // newest first, DBG-085

// ---- Development overlay counters (§2.3) -----------------------------------
class CounterSink {
public:
  virtual void Count(std::string_view name, std::int64_t value) = 0;
  virtual void Gauge(std::string_view name, double value, std::string_view unit) = 0;
  virtual void Budget(std::string_view name, double used, double budget, std::string_view unit) = 0;
};
class CounterProvider {
public:
  virtual std::string_view Page() const = 0;                     // overlay page id, for example "audio"
  virtual void Publish(CounterSink& sink) const = 0;             // presentation thread, start of frame
  virtual ~CounterProvider() = default;
};
void RegisterCounterProvider(CounterProvider& provider);
void UnregisterCounterProvider(CounterProvider& provider);

// ---- Development drawing (§4.7) --------------------------------------------
namespace draw {
void Square2d(Vec2 centre, float size, ColourRgba colour);
void Cube(Vec3 centre, float size, ColourRgba colour);
void SphereRings(Vec3 centre, float radius, ColourRgba colour);
void Sphere(Vec3 centre, float radius, ColourRgba colour, bool filled);
void VerticalCylinder(Vec3 base, float radius, float height, ColourRgba colour);
void Arrow(Vec3 from, Vec3 to, float width, ColourRgba colour);
void PointMarker(Vec3 point, std::string_view label, ColourRgba colour);
void Text3d(Vec3 point, float size, std::string_view text, ColourRgba colour, bool outlined);
void LogMatrix(const Mat3x4& matrix, std::string_view label);
}
class DevDrawQueue {                                             // the persistent queue, DBG-131
public:
  void Add(DevDrawItem item, std::string_view label, std::uint32_t life_frames);   // 0 = until removed
  void RemoveByLabelPrefix(std::string_view prefix);             // case-insensitive
  std::optional<DevDrawItemId> FindByLabelPrefix(std::string_view prefix) const;
  void Render();                                                 // pass 24, once per frame
};

// ---- Debug commands (§4.6) -------------------------------------------------
enum class DebugCommandKind : std::uint8_t {                     // order is the window's button order
  KillAllEnemies, SpawnWorkers, SpawnTransport, SpawnCrawlers, SpawnEssenceOrbs,
  AllowArbitraryPlacement, SpawnAtStructurePorts, SpawnSupplyCrate };
struct DebugCommand {
  DebugCommandKind kind;
  std::uint8_t team_slot;                                        // target team slot where the kind names one
  Vec3 point;                                                    // the screen-centre ground point (DBG-116)
  std::int16_t forced_outcome;                                   // SpawnSupplyCrate: CrateOutcome, or −1
};

// ---- Soak test (§4.9) ------------------------------------------------------
enum class SoakProgression : std::uint8_t { Linear, Random, Sequence };
struct SoakEntry { MapId map; MatchMode mode; std::uint8_t bot_teams; std::uint64_t seed; };   // seed 0 = the authority's own seed
struct SoakConfig {
  SoakProgression progression; std::vector<SoakEntry> entries; std::uint64_t progression_seed;
  std::string session_name; std::uint64_t terminate_at_tick;      // 0 = off
  std::uint8_t clients; bool rendering;
};
class SoakRunner {
public:
  explicit SoakRunner(SoakConfig config);                        // owns every piece of soak state (DEC-804)
  SoakReport Run();                                              // one pass over the entries
};

// ---- Telemetry (ADR-0066) --------------------------------------------------
struct TelemetrySessionRecord;                                   // the fixed data set of DBG-181
void SubmitTelemetry(const TelemetrySessionRecord& record);      // queued; sent only with consent

}  // namespace species::diag
```

Caption: Listing 2.1-A, the diagnostics interface (declarations only). `ProfileNodeId`, `DevDrawItem`, `DevDrawItemId` and `SoakReport` are opaque handles and records of this component; `Vec2` is the 2-D counterpart of `Vec3`.

### 2.2 Debug entry points and development bindings

**DBG-020** (MUST) Development builds add a development binding set on top of the default binding set of the [input](input.md) component (INP-009), which binds no developer key other than `action.debug.network_stats`. The debug actions and their default bindings are:

| Action | Default binding | Context and conditions | Effect | Flavours |
|---|---|---|---|---|
| `action.debug.network_stats` | `key:f9:down` | the front end, the lobby and a match: the action is polled by the front-end loop and the match loop | opens the network statistics window when it is not open; it never closes it, the window's close box does (UI-060); in `Profile` and `Shipping` builds it does nothing | every flavour binds it; development builds act on it |
| `action.debug.command_window` | `key:f11:down` | only while no UI window is open | opens the debug-command window (§4.6) | development |
| `action.debug.drop_packet` | `key:f:held` | a participant connected to a host | discards every incoming network message while held, as a manual packet-loss test (DEC-208) | development |
| `action.debug.special_lighting_on`, `action.debug.special_lighting_off` | `key:l:held`, `key:k:held` | a match | sets the renderer's special-lighting setting on, or off (RND-442); `key:l` also drives `action.view.chat_log`, and both act | development |
| `action.debug.soak_rendering` | `key:r:down` | a soak participant (§4.9) | toggles the participant's rendering and sound together | development |
| `action.debug.overlay` | `key:f5:down` | any | shows or hides the development overlay (§4.4) | `Debug`, `Development`, `Profile` |
| `action.debug.profile_view` | `key:f10:down` | any | shows or hides the profile view (§4.3) | `Debug`, `Development`, `Profile` |
| `action.debug.camera_fast`, `action.debug.camera_slow`, `action.debug.camera_rotate` | the [camera](camera_and_sequences.md) component's (CAM-084) | the developer fly mode | CAM-081, CAM-082 | development |

**DBG-021** (MUST) The screenshot action (`action.view.screenshot`) is a player action of [reference/06](../../reference/06_controls_camera_and_commands/actions_and_bindings.md) row A10 and the renderer, not a debug action.

**DBG-022** (MUST NOT) No binding exists for: a second editor toggle on the network statistics key; a gamepad binding of the debug-command window; a debug quit action; a key that resets the campaign world camera; or a key that multiplies campaign production rates. The reference holds these only as inert or campaign-only bindings.

### 2.3 Events and data contracts

**DBG-023** (MUST) **Counter providers.** A component publishes its development counters through one `CounterProvider` per overlay page. `Publish` runs on the presentation thread at the start of each frame while the page is shown, reads only the component's own counters, and allocates no memory. The pages and their providers are:

| Page | Provider | Content |
|---|---|---|
| frame | this component | the frame-time graph, frame rate, entity counts per type (DBG-095, DBG-097) |
| render | renderer | per-pass GPU times against the pass-group budgets (RND-041, RND-454), compacted instance counts per instance type (RND-386), the input-mode box flag (RND-076) |
| simulation | this component, from the profile tree | per-subsystem CPU times of the marker catalogue (§3.4) against their budgets (§5.4) |
| audio | audio engine | the counters of AEN-323 and the per-entry-kind and per-event costs of AEN-322 |
| assets | asset pipeline | resident bytes per load group and content type, cache entries per load state, the override list, the placeholder count (AST-126) |
| input | input | events per frame (latest and maximum), dropped events, reconciliations, the age of the oldest event (INP-241) |
| ui | UI framework | panels, widgets, clicks and cancelled presses per frame; draws, vertices, batches and glyphs rasterised per frame; glyph-cache evictions (UI core and layout §6) |
| vfx | animation and VFX | live and dropped particles per type and class, live explosions and shards, the shake scalar, flash onsets and effect events per tick (animation and VFX §6) |
| log | this component | records written and dropped per category and level (DBG-062) |
| network | the networking model | the metric set of DEC-208 |

**DBG-024** (MUST) **Debug commands** travel as player commands of the command catalogue ([teams and player commands](../../reference/12_simulation_model/06_teams_and_player_commands.md)) with the payload of `DebugCommand`: the sender's team, the command kind, the target team slot, the target point and the forced outcome. Who may send one and how it is validated is DBG-118 and DEC-202.

**DBG-025** (MUST) **Soak artefacts.** Each soak pass produces one soak report: one row per match (DBG-172) plus the pass totals, the crash dumps of the pass ([platform layer](platform_layer.md)), the assertion failures, the simulation update-time percentiles and the peak population per match. The report is a CI artefact (ADR-0070).

**DBG-026** (MUST) **Telemetry record.** One `TelemetrySessionRecord` per session holds exactly the data set of DBG-181; a schema check rejects any field outside it (ADR-0066).

### 2.4 Threading contract

**DBG-027** (MUST) Every logging, marker and black-box call is safe from any thread and never blocks on I/O: logging writes into a per-thread buffer (DBG-061), a CPU marker pushes and pops a per-thread marker stack, and the black box is written with atomic stores into a fixed-size block.

**DBG-028** (MUST) Each thread that carries markers has its own profile tree; the profile view merges the trees of the threads it shows, by thread name, at the start of a frame. No marker call takes a lock that another marker call holds.

**DBG-029** (MUST) The development overlay, the development windows and the development drawing queue run on the presentation thread. The drawing queue is filled and drawn on that thread only; a simulation-side check that wants a visual queues a presentation event, never a draw call (DEC-007).

**DBG-030** (MUST) Debug commands enter the simulation only through the command path, at a tick boundary, like every other command (R-SIM-670); no diagnostic code runs inside a tick except the markers and the per-tick test-build checks, which run after the tick's last step and read only.

**DBG-031** (MUST) The soak runner, the headless simulation tests and the telemetry sender each run on threads they own and stop and join before their owner is destroyed (DEC-224).

## 3 Data model

### 3.1 Types, ownership and lifetimes

**DBG-040** (MUST) Every diagnostic object has one owner and an explicit lifetime; none is reached through a process-wide global, and none is static storage of a UI module (DEC-804):

| Object | Owner | Lifetime |
|---|---|---|
| logger, its per-thread buffers and its sinks | the composition root of each program (client, server, tools) | process; the logging thread starts first and stops last |
| black box | the composition root; its memory block is registered by the platform layer | process |
| profile trees, frame-time history | the profiling service | process; history reset on request (DBG-084) |
| counter providers | the component that registers them | the component; unregistered before it is destroyed |
| development overlay, profile view, development windows | the client's UI core (UI-005) | while shown |
| development drawing queue | the renderer's frame context | per match world; emptied at every match start and end |
| soak runner and its state | the test run that creates it | one soak pass |
| telemetry queue | the telemetry service | process; persisted between runs only as DEC-250 decides |

### 3.2 Hidden development settings

**DBG-041** (MUST) Development builds read these settings from the settings store (`SettingsStore`). Players never see them (R-META-457), and `Profile` and `Shipping` builds do not contain them (DBG-004):

| Setting | Default | Values and effect | Specified in |
|---|---|---|---|
| `dev.bot_overlay` | `dbg.setting.bot_overlay.default` (0) | 0: off; 1: the bot priority text for one team and the link arrows; any other non-zero value: the link arrows only | DBG-135 |
| `dev.bot_overlay_team` | `dbg.setting.bot_overlay_team.default` (1) | the lobby position (not the team slot) whose priorities the overlay shows | DBG-135 |
| `dev.command_history` | `dbg.setting.command_history.default` (0) | 0: off; 1: record; 2: play back (R-SIM-740) | DBG-241 |
| `dev.capture_frame_rate` | `dbg.replay.capture_frame_rate` (25) | presentation frames per second of the fixed step that command-history modes 1 and 2 use | DBG-241 |
| language test | off | [reference/09](../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-473 | reference/09 |
| special lighting | off | toggled by the debug actions of DBG-020 | RND-442 |
| poster resolution | 1 | screenshots as N × N tiles | RND-110, RND-442 |
| `dev.soak_session_name` | empty | the session name a soak host advertises and soak clients look for; a soak configuration's name replaces it | DBG-162 |

**DBG-042** (MUST) The reference's terrain storage-mode setting has no Species meaning (RND-443), and its campaign test-harness switch is not specified (DEC-052). The reference start-up switches with a diagnostic role belong to their owners: writing the version identity to a file and exiting ([build tooling and distribution](build_tooling_and_distribution.md), ADR-0057), and keeping every log in the working directory instead of the profile folder ([platform layer](platform_layer.md)).

### 3.3 Log records and categories

**DBG-043** (MUST) A log record holds: the platform-clock time (ADR-0074) and the UTC calendar time used only for display; the thread name; the category; the level; and the message, formatted from a compile-time format string with arguments. Categories are the fixed set of Listing 2.1-A: the nine of ADR-0067 plus `Diagnostics` (this component) and `Test` (the test harness and the soak runner). Levels are ordered trace < debug < info < warning < error < fatal.

**DBG-044** (MUST) A log file starts with a header that holds the full build identity (ADR-0057), the OS build, the adapter description and the date and time of the start of the run (ADR-0067). This replaces the reference header line of product name, version and build date.

### 3.4 Profile marker catalogue

**DBG-045** (MUST) Every engine subsystem update, every frame-graph pass (RND-041) and every content-loading stage carries a marker (ADR-0068). The catalogue below is the Species marker set for the systems that the reference instruments; a row's "per kind" marker is one marker per actor kind or structure kind, named by the kind's identifier. Markers nest by call order on their thread.

| Area | Species markers (the reference section each replaces is implied by the name) |
|---|---|
| frame and loop | `frame.match_loop` (the whole simulation advance of a frame, around the world step); `render.frame`; `render.clear`; `render.present` |
| camera and input | `camera.advance`; `input.advance`; `render.match_input` |
| sky | `render.sky`; `render.sky.blobby_layers`; `render.sky.flat_layers`; `render.sky.advance`; `render.clouds` |
| terrain and sea | `render.terrain.surface`; `render.terrain.wire_overlay`; `render.sea`; `render.sea.advance`; `render.sea.wave_build` (the coastline wave mesh, built on the GPU in Species, ADR-0011) |
| simulation, world step | `sim.laser_bolts.advance`; `sim.effects.advance`; `sim.structures.advance` with one marker per structure kind; `sim.essence_orbs.advance`; `sim.teams.advance_group_members`; `sim.teams.advance_groups`; `sim.teams.advance_ungrouped` with one marker per actor kind; `sim.actor_group.advance` with one marker per actor kind |
| Workers | `sim.worker.task_search`; `sim.worker.search.threats` with `.projectiles`, `.trees`, `.hives`, `.bot_nodes`, `.enemies`; `sim.worker.search.ports`; `sim.worker.search.carryables`; `sim.worker.search.supply_crates`; `sim.worker.search.transports`; `sim.worker.search.flags`; `sim.worker.search.leaders`; `sim.worker.search.random_position`; `sim.worker.combat`; `sim.worker.throw_grenade`; `sim.worker.move_to_target`; `sim.worker.push.sea`; `sim.worker.push.structures` |
| creatures | `sim.crawler.record_trail`; `sim.crawler.move_to_target`; `sim.crawler.idle`; `sim.crawler.attacking`; `sim.crawler.to_essence_orb`; `sim.crawler.to_egg`; `sim.crawler.search.enemies`; `sim.crawler.search.essence_orbs`; `sim.crawler.search.eggs`; `sim.crawler.search.idle_direction`; `sim.centipede.search_essence_orbs`; `sim.spider.search_essence_orbs` |
| structures and objects | `sim.spawn_node.evaluate`; `sim.spawn_node.spawn_workers`; `sim.structure.push_actors`; `sim.essence_orb.push_from_structures`; `sim.spitter_plant.check_trigger`; `sim.bomber.update_position` |
| network | `net.host.advance`; `net.host.update_clients`; `net.host.send`; `net.host.advertise`; the Species networking markers follow DEC-200 |
| effects | `vfx.particles.advance`; `vfx.particles.render`; `vfx.shatter.advance`; `vfx.shatter.render` |
| world rendering | `render.structures` and `render.structures.transparent`, each with one marker per structure kind; `render.structures.transparent_sort`; `render.structure_ports`; `render.essence_orbs`; `render.weapons`; `render.actor_groups` with one marker per actor kind; `render.ungrouped_actors` with one marker per actor kind; `render.crawlers`; `render.workers`; `render.cloth_flags`; `render.beam_dish.beam`; `render.transit_gate.destination`; `render.transit_gate.surface_advance`; `render.transit_gate.surface`; `render.routes` |
| overlays and UI | `render.world_cursor`; `render.world_markers`; `render.centre_message`; `render.mode_hud`; `render.hint_overlay`; `render.power_slots_panel`; `dbg.profile_view.render` |
| audio | `audio.update` and one marker per update step of AEN-123, `audio.decode`, `audio.bank_load` (AEN-322); the reference's instance-start sections map onto `audio.instance.start`, `audio.instance.open_stream`, `audio.instance.set_parameters`, `audio.instance.set_frequency_volume_position` and `audio.instance.setup_effects`, and its effect sections onto `audio.effect.resonant_low_pass`, `audio.effect.gargle` and `audio.effect.echo` |

**DBG-046** (MUST NOT) The catalogue has no marker for campaign-only systems (DEC-052), for the pixelation, deform and reflection effects (DEC-059), for the gesture and icon Power interfaces (DEC-055), or for the reference's device-buffer locking in audio output, which the Species audio engine does not have (ADR-0022). The reference instruments 186 call sites in its compiled code; the rows above keep every one whose system Species specifies.

### 3.5 Black-box contents

**DBG-047** (MUST) The black box is a fixed-size block of `dbg.blackbox.block_bytes` (4096 bytes) registered with Windows Error Reporting by the platform layer (ADR-0065). It holds:

1. a fixed marker text, so that the block can be found in a dump;
2. the full build identity (ADR-0057);
3. the UTC date and time of the start of the run, and of the failure when one is recorded;
4. the most recent `dbg.blackbox.recent_markers` (16) frame and tick markers, each with its frame index or tick;
5. the current map id, or the world region id once the design phase defines regions;
6. the most recent `dbg.blackbox.recent_checks` (8) failed-check and warning messages;
7. for a fatal check: its message, its condition text, its source file and line, and the thread name.

It never holds chat text, account tokens or the names of other players (ADR-0065, ADR-0067).

**DBG-048** (MUST) The reference's assertion record is a text report with a banner, the product name, version and build date, the date as hour and minute and as day, month and year, the error message, and a stack trace of return addresses that a frame-pointer walk collects and an offline script resolves through a linker map; the walk stops at a misaligned, non-increasing or unreadable frame and is short or wrong in optimised builds. Species keeps the content (identity, time, message, stack) and takes the stack from the dump, symbolised through the symbol store of every build ([build tooling and distribution](build_tooling_and_distribution.md), ADR-0065); no Species code walks frame pointers.

### 3.6 Diagnostic outputs

**DBG-049** (MUST) Every diagnostic output of a Species client, with its writer:

| Output | Writer | Content | Flavours |
|---|---|---|---|
| log files in the logs folder (ADR-0071), one per run, the last `dbg.log.files_kept` (10) kept | logging system (§4.1) | the log | every flavour |
| ETW events of the Species TraceLogging provider | logging system | the same records | every flavour |
| black-box block | this component and the platform layer | DBG-047 | every flavour |
| crash report and dump | platform layer, through WER (ADR-0065) | dump of every thread, registered files and the black box | every flavour |
| language completeness report | the client's language test (R-META-473) | three lists per language | development |
| command-history recording | the replay hook (§8.2) | the command frames of a match | development |
| soak report | the soak runner (DBG-172) | one row per match and the pass totals | CI |
| telemetry queue | the telemetry service (§4.10) | unsent session records | every flavour, with consent |
| PIX captures and ETW traces | PIX and Windows Performance Analyzer | captures taken by a developer | `Debug`, `Development`, `Profile` |
| test reports and performance results | the test harness | CI artefacts (ADR-0070) | test builds |

Key-binding parse errors go to the log ([input](input.md)). The world-state dump on divergence is DEC-206. The version file is the build tooling's (ADR-0057).

## 4 Behaviour and algorithms

### 4.1 Logging

**DBG-060** (MUST) Logging follows ADR-0067: `Log(category, level, format, arguments)` with a compile-time format string, so text from content, players or the network is always an argument and is logged verbatim, a percent sign or a brace included. No log call formats into a fixed-size buffer; a long message is logged whole.

**DBG-061** (MUST) A log call writes its record into the calling thread's lock-free buffer of `dbg.log.thread_buffer_kib` (256 KiB) and returns; one logging thread drains every buffer to the sinks. When a buffer is full, trace records are dropped first, then debug records; warnings, errors and fatal records are never dropped, and the caller waits only when its buffer holds nothing but such records. The cost on the calling thread stays within `dbg.budget.log_call_us` (DBG-206).

**DBG-062** (MUST) Dropped records are counted per category and level. The counts appear on the log page of the development overlay, and a warning record states the counts since the previous such warning whenever a drain finds new drops.

**DBG-063** (MUST) `LogHexDump` writes `dbg.log.hex_bytes_per_line` (16) bytes per line in hexadecimal; `LogValueDump` writes 32-bit integers in decimal, `dbg.log.hex_bytes_per_line` ÷ 4 values per line.

**DBG-064** (MUST) Sinks and levels per flavour are those of ADR-0067: the rotating UTF-8 file (DBG-049), the TraceLogging provider, and the debugger output in `Debug` and `Development` builds. `Shipping` and `Profile` builds record info and above; development builds record debug and above, with trace enabled per category at run time. If the log file cannot be opened, records go to the remaining sinks, and the failure is reported once, with the path, on those sinks.

**DBG-065** (MUST) The log records at least the content that the reference debug log carries, as categorised records: the start-up header (DBG-044); content mounting with entry counts and durations, and every missing resource ([asset pipeline and resources](asset_pipeline_and_resources.md) AST-126); every settings entry that fails to parse or is out of range (DEC-800); network connection, divergence and disconnection events (DEC-206); audio readiness, bank loads and device changes (AEN-324); key-binding parse outcomes ([input](input.md)); and a memory report every `ref.sim.frame.memory_log_interval` (60 s, [tick timing](../../reference/12_simulation_model/01_tick_timing_and_main_loop.md)) giving the available physical memory, the process commit and working set, and the audio engine's memory summary (AEN-324).

**DBG-066** (MUST NOT) No record holds chat text, account tokens, passwords, or an identifier of another player other than a session-local number (ADR-0067). No diagnostic is formatted while its level is disabled: a disabled record costs the level test only. In the reference a state line is formatted before and after every body and structure update even where the log discards it (DEC-206).

### 4.2 Checks, the black box and the fatal path

**DBG-070** (MUST) Species has these check levels; the reference level each replaces is named in the second column:

| Species level | Reference counterpart | Flavours | On failure |
|---|---|---|---|
| debug assertion `SPECIES_ASSERT(c)` | debug assertion, no effect outside debug builds | `Debug`, `Development` | log at error level with the condition, file and line, note in the black box, break into an attached debugger; execution continues when no debugger is attached |
| release check `SPECIES_CHECK(c, message)` | release assertion with a formatted message | every flavour | the fatal path of DBG-073 |
| plain release check (no message) | assertion that reports the condition text, file and line | every flavour | the fatal path, message = condition text |
| container bounds check `SPECIES_CHECK_INDEX(i, n)` | the assertion with arguments that every container bounds check uses | every flavour | the fatal path; an out-of-range container access is a fatal, reported error in every build |
| abort | release assertion that is always false, with a message | every flavour | the fatal path |
| fatal exit `FatalExit(message)` | logged exit with a message dialog and the assertion record | every flavour | DBG-074 |

**DBG-071** (MUST) Content errors never reach a check: the content build rejects them (DEC-015, ADR-0059), and a load path reports and recovers. A check states a program invariant; a failed check is a defect.

**DBG-072** (MUST) The test builds' simulation checks of §4.8 are debug assertions that the test harness turns into test failures; they never run in `Profile` or `Shipping` builds.

**DBG-073** (MUST) The fatal path of a failed check runs in this order:

1. write the message, the condition text, the source location and the thread name into the black box (DBG-047);
2. write a fatal log record with the same content and flush the log synchronously (ADR-0067), so the registered log file holds it;
3. hand over to the platform layer's fatal path, which shows the fatal-error message through the platform message path when a window exists and ends the process with a fail-fast exception, so that Windows Error Reporting captures a dump with the registered files and the black box ([platform layer](platform_layer.md), ADR-0065).

In the reference a failed release assertion formats its message into 512 bytes, shows a fatal-error message box, writes the assertion record and terminates through the runtime's terminate path; whether its crash handler also runs is a property of its runtime library (DEC-341). Species has one defined path.

**DBG-074** (MUST) `FatalExit` is an orderly fatal exit that is not a defect, for a condition such as a missing required capability: it writes the message to the black box and the log, flushes the log, hands the message to the platform message path and ends the process with a non-zero exit code, without a dump.

**DBG-075** (MUST) The black box is updated as the program runs: the frame loop notes each frame index and the tick clock notes each tick (`NoteFrame`, `NoteTick`); the outermost markers of a frame and a tick are noted as markers; loading a map or entering a region notes its id; each warning and each failed debug assertion is noted as a check message. Updates are atomic stores into the ring slots, never allocations.

### 4.3 Profiling

**DBG-080** (MUST) The profiling layer is ADR-0068's: `SPECIES_PROFILE_SCOPE` and its GPU counterpart emit PIX events through WinPixEventRuntime in `Debug`, `Development` and `Profile` builds and expand to nothing in `Shipping` builds; a GPU scope also writes a timestamp query pair, resolved into a readback buffer and read `dbg.profile.gpu_readback_latency` (2) or more frames later, so the CPU never waits for the GPU to measure it. System-wide problems are captured as ETW traces with the TraceLogging provider of DBG-064.

**DBG-081** (MUST) The same markers feed the profile tree. Each thread's tree is rooted at a root node. Entering a marker finds or creates the child of the current node with the marker's name and starts its timer; leaving it adds one call and the elapsed time to the node, updates the node's shortest and longest call, and makes the parent current again. Nesting follows call order only; a leave whose name differs from the current node's is a debug assertion. Every node is timed whether or not the view shows it.

**DBG-082** (MUST) Once every `dbg.profile.rollup_period_ms` (1000 ms) of the platform clock, checked once per frame, the profiling service rolls every node over: the current period's calls and time become the last period's, the last period's are added to the history (one more history period), and the current period restarts; the length of the period just ended is recorded as now − (end of period − `dbg.profile.rollup_period_ms`), and the next period ends `dbg.profile.rollup_period_ms` after now. The roll-over runs in every frame loop, the front-end loop included.

**DBG-083** (MUST) Each node keeps the fields of `ProfileNodeStats`: calls and time of the current and last periods, time, periods and calls since the last history reset, and the shortest and longest call since that reset. The shortest call starts at the largest representable value and the longest at 0.

**DBG-084** (MUST) `ResetHistory` clears the history, shortest and longest of every node of every tree. `MaxChildTime(node)` is the largest child history time divided by the history periods of the node's first child in name order. A node is looked up by its path of names from the root.

**DBG-085** (MUST) After every frame the frame time in milliseconds is pushed to the front of the frame-time history, which keeps the newest `dbg.profile.frame_history` (200) frames.

**DBG-086** (MUST) The **profile view** is a tool window of `dbg.profile.view.width` × `dbg.profile.view.height` (570 × 600 px) at (W − `dbg.profile.view.offset_x`, `dbg.profile.view.y`) = (W − 590, 30). It has two buttons on the row at y = `dbg.profile.view.button_y` (18): reset history at x = `dbg.profile.view.reset_x` (190), which calls `ResetHistory`, and the statistic choice at x = `dbg.profile.view.statistic_x` (330), whose caption cycles minimum → average → maximum → minimum. A column header (`ui.dev.profile.header`) is right-aligned at x + `dbg.profile.view.statistic_x`.

**DBG-087** (MUST) The view lists the children of each expanded node in name order, depth first from the root, which is always expanded, and skips a node whose history average (history time ÷ history periods) is 0. Each row shows: an indent of one step per depth; a marker that reads "+" for a collapsed node with children, "−" for an expanded one and a blank for a leaf; the name padded to `dbg.profile.view.name_width` (24) characters minus the indent; the last period's calls; the time per call in ms with one decimal; the last period's total time in ms with no decimals; the chosen statistic (shortest call, history average per period or longest call) in ms with one decimal; and, for a node of a frame-graph pass, its GPU time from the timestamps. The reference computes the chosen statistic but never shows it, and prints its two decimal columns as unreadable text; the Species view shows all of them.

**DBG-088** (MUST) Each row is `dbg.profile.view.row_height` (12 px) high. Its brightness is `dbg.profile.view.brightness_min` + `dbg.profile.view.brightness_range` × time ÷ largest time among the rows, clamped to [105, 255]. A bar starts at x + `dbg.profile.view.bar_x` (340), is √(time in ms) × `dbg.profile.view.bar_scale` (10) px long and `dbg.profile.view.bar_height` (11 px) high, in `dbg.profile.view.bar_rgb` (150, 150, 250) with the row's brightness as alpha, drawn as an outline when the node is expanded and filled when it is collapsed. A primary press inside a row's band toggles that node's expansion. The window grows by one row height for every row past its height.

**DBG-089** (MUST) The bottom `dbg.profile.view.graph_height` (100 px) of the view hold the frame-time graph: guide lines at the frame times of `dbg.profile.view.graph_marks_ms` (200, 100 and 50 ms, that is 5, 10 and 20 frames per second), each labelled with its time and rate; one red vertical line per frame of the frame-time history, newest at the right, of height 0.5 × (frame time ÷ 100 ms) × 100 px, so a frame of `dbg.profile.view.graph_full_ms` (200 ms) fills the graph.

**DBG-090** (MUST) Subsystem profilers belong to their subsystems: the audio engine brackets its update, update steps, decode task and bank loads, and counts the cost of triggers per entry kind and of decoding per event (AEN-322); the profile view and the audio page show them.
