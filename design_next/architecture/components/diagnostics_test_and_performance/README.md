# Diagnostics, test and performance

> Status: Engine baseline · Prefix: DBG · ADRs: [ADR-067](../../../ADR/ADR-067-logging.md) · [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md) · [ADR-069](../../../ADR/ADR-069-test-framework.md) · [ADR-070](../../../ADR/ADR-070-continuous-integration.md) · [ADR-066](../../../ADR/ADR-066-telemetry-with-consent.md) · [ADR-065](../../../ADR/ADR-065-crash-reporting.md) · [ADR-056](../../../ADR/ADR-056-editions-build-flavours-and-feature-flags.md) · [ADR-057](../../../ADR/ADR-057-version-identity.md) · [ADR-074](../../../ADR/ADR-074-platform-clock-with-injectable-time.md) · [ADR-059](../../../ADR/ADR-059-content-build-and-validators.md) · [ADR-060](../../../ADR/ADR-060-content-identity-by-hash.md) · [ADR-001](../../../ADR/ADR-001-direct3d-12-renderer-on-the-agility-sdk.md) · [ADR-008](../../../ADR/ADR-008-gpu-memory-uploads-and-device-removal.md) · Related: [tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) · [platform layer](../platform_layer.md) · [build tooling and distribution](../build_tooling_and_distribution.md) · [renderer options and budgets](../renderer/options_and_budgets.md)

This component is the Species engine's developer and quality infrastructure. It specifies the logging system, the assertion levels and the black-box record, the profiling layer and its in-game views, the development overlay, the development windows and debug commands that the engine keeps, the development drawing queue, the test harness with its headless simulation tests, the headless soak test and the opt-in telemetry data set. It also holds the **reference performance figures**: the population per map, the frame and tick structure, the hotspot catalogue, the stress populations and the budgets that the Species design phase starts from.

**Notation and units.**

- **Costs** (frame time, tick cost, call cost) are real time on the platform clock (ADR-074), in milliseconds (ms), microseconds (µs) or nanoseconds (ns), measured on the reference test machine (§5.5).
- **Simulation durations** are in ticks at the reference tick rate, one tick per `ref.sim.tick_period` (0.1 s) of [reference/12](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md); a duration in seconds next to a tick count is a derived display value. Delays of developer tools that run while no match world exists are counted in milliseconds of the platform clock, never accumulated from frame-time differences (DEC-804).
- **Distances** are in world units (`wu`). Positions and sizes of developer windows and overlays are in pixels of the 2-D overlay space, origin at the top-left corner and y growing downwards (RND-215), before the UI scale of UI-166; W × H is the render size.
- **Colours** are RGBA8 tuples (red, green, blue, alpha), or RGB triples when the alpha is 255.
- **Development builds** are the `Debug` and `Development` flavours of ADR-056. **Test builds** are the development builds and every CI test configuration that runs the test harness (ADR-070).

## Folder index

| File | Holds | Requirement IDs |
|---|---|---|
| [README.md](README.md) (this file) | responsibilities and boundaries, the facilities per build configuration, the interface sketch, the debug entry points and development bindings, the counter providers and data contracts, the threading contract, ownership and lifetimes, the hidden development settings, the diagnostic outputs, the tuning-table convention, diagnostics and concurrency, the failure modes, the component-wide tests and the open decisions | DBG-001 to DBG-006, DBG-020 to DBG-031, DBG-040 to DBG-042, DBG-049, DBG-200, DBG-220, DBG-222, DBG-223 |
| [logging_checks_and_profiling.md](logging_checks_and_profiling.md) | §3.3 to §3.5 and §4.1 to §4.3: log records and categories, the profile marker catalogue, the black-box contents, logging, the check levels, the black box and the fatal path, profiling and the profile view | DBG-043 to DBG-048, DBG-060 to DBG-066, DBG-070 to DBG-075, DBG-080 to DBG-090 |
| [developer_tools.md](developer_tools.md) | §4.4 to §4.7: the development overlay, the network statistics window, the debug commands and the debug-command window, development drawing and the bot overlay; the development text; the classification of the reference cheats | DBG-095 to DBG-099, DBG-105 to DBG-110, DBG-115 to DBG-121, DBG-130 to DBG-136, DBG-221 |
| [test_harness_soak_and_telemetry.md](test_harness_soak_and_telemetry.md) | §4.8 to §4.10 and §8.2: the test harness, the soak test, telemetry and the replay hooks | DBG-140 to DBG-150, DBG-160 to DBG-167, DBG-180 to DBG-183, DBG-240, DBG-241 |
| [performance_baseline.md](performance_baseline.md) | §5.2 to §5.5: the frame and tick structure of the reference, the reference performance figures, the Species budget baseline and the reference test machine | DBG-201 to DBG-209 |

Section numbers are those of the whole component: a section cited as §4.8 is in the file that the index names for it.

## 1 Responsibilities and boundaries

**DBG-001** (MUST) The diagnostics component owns:

1. the logging system: its API, categories, levels, pipeline, sinks and drop policy (§4.1);
2. the check levels and the flavours that compile them in, the black box, and the fatal path up to the platform layer's last step (§4.2);
3. the profiling layer: the marker macros, the collection of GPU timestamps, the profile tree and its view (§4.3), and the marker catalogue (§3.4);
4. the development overlay, its pages and the counter-provider contract through which other components publish counters into it (§2.3, §4.4);
5. the development windows: the network statistics window (§4.5) and the debug-command window (§4.6);
6. the debug commands and their classification by effect on simulation state (§4.6, DBG-221);
7. the development drawing queue, its primitives and the development overlay layers drawn with them, among them the bot overlay (§4.7);
8. the hidden development settings that switch these facilities (§3.2);
9. the test harness: unit, property, golden-hash, golden-image, fuzz and performance tests, the headless simulation tests and the per-tick test-build checks (§4.8);
10. the headless soak test (§4.9);
11. the assembly of the opt-in telemetry data set (§4.10);
12. the reference performance figures, the Species budget baseline, the stress populations and the reference test machine (§5).

**DBG-002** (MUST NOT) No diagnostic facility changes a simulation result. Overlays, windows, the profile tree and logging read presentation state, counters and the immutable simulation snapshot only (DEC-007, DEC-222). A debug command that changes simulation state does so only as a sequenced command (DBG-118). Instrumentation never changes arithmetic, the number of simulation draws or a drawn value ([randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) R-SIM-613). A check that inspects simulation state after a tick never writes it. Test T7 confirms that a match played with every diagnostic facility on gives the same per-tick simulation hashes as the same match played with every facility off.

**DBG-003** (MUST) The neighbouring components own these parts, and this component cites them and never restates their rules:

| Part | Owner |
|---|---|
| crash capture: no top-level exception filter, the terminate, invalid-parameter and pure-call handlers, fail-fast termination, the files and memory blocks registered with Windows Error Reporting, thread naming and dump contents | [platform layer](../platform_layer.md), ADR-065 |
| the symbol store, the CI pipeline definitions, binary checks, the edition manifests and the dedicated-server build | [build tooling and distribution](../build_tooling_and_distribution.md), ADR-070, ADR-056 |
| drawing of the development drawing pass (pass 24) and the text overlays (pass 29); the development sphere | [frame graph and passes](../renderer/frame_graph_and_passes.md) RND-076; [entity and overlay renderers](../renderer/entity_and_overlay_renderers.md) RND-330 |
| the bot overlay's pass (pass 16) | [entity and overlay renderers](../renderer/entity_and_overlay_renderers.md) RND-359 |
| tool-window chrome, close box, inset panel and layout | [UI framework](../ui_framework/README.md) UI-001, [core and widgets](../ui_framework/core_and_widgets.md) UI-060, UI-061, [layout](../ui_framework/layout_rendering_input_and_localisation.md) UI-166 |
| each component's own markers, counters and development pages | renderer RND-041, RND-386, RND-454; audio engine AEN-322, AEN-323, AEN-325; asset pipeline AST-126; input INP-241; UI core and layout (their §6); animation and VFX (its §6); terrain TER-254; camera (its §6) and the developer fly mode CAM-080 |
| the renderer's budgets and reference caps | [renderer options and budgets](../renderer/options_and_budgets.md) RND-453 to RND-457 |
| the language test | [options and hooks](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-473 |
| the command-history recording hook | [randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) R-SIM-740 to R-SIM-742 |
| tick timing, update slices and catch-up | [tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) |
| the networking metric set and the link simulator | DEC-208 (design phase) |
| run-time divergence detection | DEC-206 (design phase) |

This component composes those parts into one developer view and one test harness.

**DBG-004** (MUST) Each facility exists in these build flavours (ADR-056); a facility marked "no" is absent from the binary, not merely disabled:

| Facility | `Debug` | `Development` | `Profile` | `Shipping` |
|---|---|---|---|---|
| log file and ETW sinks, info level and above | yes | yes | yes | yes |
| debug-level records, per-category trace records, debugger output sink | yes | yes | no | no |
| development checks (`SPECIES_ASSERT`) | yes | yes | no | no |
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
| network statistics window | yes | a key opens it in menus, the lobby and a match (DBG-020) | development builds (DBG-105) |
| debug log | yes | always written to the profile folder, truncated at start-up | the logging system, every flavour (§4.1) |
| release assertions and the assertion record | yes | any failed release assertion or container bounds check | release checks and the black box (§4.2) |
| crash log and minidump | yes | any unhandled exception on the main thread | crash capture on every thread ([platform layer](../platform_layer.md), ADR-065) |
| world-state dump on divergence | yes | the first divergence notice | design phase (DEC-206) |
| development drawing | compiled in; only the bot overlay is drawn | a hidden setting (§3.2) | development builds (§4.7) |
| profile tree | compiled in; not reachable | — | the profile view (§4.3) |
| frame-rate and entity-count overlay | compiled in; forced off | — | the development overlay (§4.4) |
| hidden development settings | yes | the settings file | development builds (§3.2) |
| debug-command window | compiled in; empty | — | development builds (§4.6) |

**DBG-006** (MUST) The Species debug and test baseline is:

1. **Debug commands.** Every developer cheat that changes simulation state is a sequenced debug command, which the authority accepts only in a session created with cheats enabled and every participant applies on the same tick; such a session carries a session-wide cheats-used marker (§4.6, DEC-041, DEC-202).
2. **Replay as the regression tool.** When the design phase gives Species reproducible replays (DEC-046), every match records its seed and command stream into a local ring buffer, the headless runner replays it at maximum speed, the end-of-match state hash must match, and a recorded match serves as the repeatable performance scene of ADR-066 (§8.2).
3. **Headless soak test.** Bot teams play complete matches on every mode and map in CI, with no user interface driven (§4.9, DEC-804, ADR-070).
4. **Profiling.** Scoped CPU markers that PIX and ETW traces show, GPU timestamps per pass, and an in-game overlay with a frame-time graph and per-system budgets; every marker is safe to use from any thread (§4.3, ADR-068).
5. **Development console and overlays,** reachable in development builds: frame time, entity counts per type, network statistics with correct units, per-client lag in ticks and gaps in the incoming frame queue, bot overlays, and actor-grid and obstacle-grid visualisers (§4.4 to §4.7).
6. **Logging and crash reporting.** An asynchronous, buffered, categorised log; crash capture on every thread and through the terminate path; dumps with every thread; a symbol store for every build; no stack walk that depends on frame pointers; no credentials in a binary or a report (§4.1, §4.2, ADR-065, ADR-067).
7. **Telemetry with consent** (§4.10, ADR-066).
8. **Validators in the content build** instead of run-time-only checks: the language completeness test and the content checks run in the content build (ADR-059, R-META-473).

Run-time state hashing for divergence detection is DEC-206; the per-tick state hash of the test harness (DBG-144) is a test tool.

## 2 Interfaces

### 2.1 Interface sketch

```cpp
namespace species::diag {

// Logging (ADR-067). LogLevel is in severity order; LogCategory is fixed and extended only in code.
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error, Fatal };
enum class LogCategory : std::uint8_t { Platform, Render, Audio, Input, Ui, Content, Simulation, Network, Tools, Diagnostics, Test };
template <class... Args> void Log(LogCategory, LogLevel, std::format_string<Args...>, Args&&...);
void LogHexDump(LogCategory, LogLevel, std::span<const std::byte>);          // DBG-063
void LogValueDump(LogCategory, LogLevel, std::span<const std::int32_t>);     // DBG-063
void SetCategoryLevel(LogCategory, LogLevel minimum);                         // development builds
struct LogStats { std::uint64_t written, dropped_trace, dropped_debug; };
LogStats GetLogStats();
void FlushLogSynchronously();                                                 // fatal path only (DBG-073)

// Checks and the black box (§4.2, ADR-065)
#define SPECIES_ASSERT(condition)            /* development check: Debug and Development */
#define SPECIES_CHECK(condition, fmt, ...)   /* release check: every flavour, fatal */
#define SPECIES_CHECK_INDEX(index, size)     /* container bounds check: every flavour, fatal */
[[noreturn]] void CheckFailed(std::source_location, std::string_view condition, std::string_view message);
[[noreturn]] void FatalExit(std::string_view string_id, std::span<const MessageArgument> arguments);  // DBG-074
class BlackBox {                                       // one per process, owned by the composition root
public:
  void NoteFrame(std::uint64_t frame_index);  void NoteTick(std::uint64_t tick);
  void NoteMarker(std::string_view marker);   void NoteLocation(std::string_view map_or_region_id);
  void NoteCheck(std::string_view message);
  std::span<const std::byte> MemoryBlock() const;      // registered by the platform layer
};

// Profiling (ADR-068)
#define SPECIES_PROFILE_SCOPE(name)                   /* CPU marker; nothing in Shipping */
#define SPECIES_PROFILE_GPU_SCOPE(command_list, name) /* GPU marker and a timestamp pair */
struct ProfileNodeStats {
  std::uint32_t calls_current, calls_last, history_periods, history_calls;
  double ms_current, ms_last, ms_history, shortest_ms, longest_ms;
};
class ProfileTree {                                    // one per thread that carries markers
public:
  ProfileNodeId Root() const;
  std::span<const ProfileNodeId> ChildrenByName(ProfileNodeId) const;
  const ProfileNodeStats& Stats(ProfileNodeId) const;
  std::optional<ProfileNodeId> Lookup(std::string_view path) const;
  void ResetHistory();  double MaxChildTime(ProfileNodeId) const;            // DBG-084
};
std::span<const float> RecentFrameTimesMs();                                  // newest first, DBG-085

// Development overlay counters (§2.3)
class CounterSink {
public:
  virtual void Count(std::string_view name, std::int64_t value) = 0;
  virtual void Gauge(std::string_view name, double value, std::string_view unit) = 0;
  virtual void Budget(std::string_view name, double used, double budget, std::string_view unit) = 0;
};
class CounterProvider {
public:
  virtual std::string_view Page() const = 0;                 // overlay page id, for example "audio"
  virtual void Publish(CounterSink&) const = 0;              // presentation thread, start of frame
  virtual ~CounterProvider() = default;
};
void RegisterCounterProvider(CounterProvider&);  void UnregisterCounterProvider(CounterProvider&);

// Development drawing (§4.7): one function per primitive of DBG-130, and the persistent queue
class DevDrawQueue {
public:
  void Add(DevDrawItem, std::string_view label, std::uint32_t life_frames);  // 0 = until removed
  void RemoveByLabelPrefix(std::string_view prefix);                         // case-insensitive
  std::optional<DevDrawItemId> FindByLabelPrefix(std::string_view prefix) const;
  void Render();                                                             // pass 24, once per frame
};

// Debug commands (§4.6); the enumerator order is the window's button order
enum class DebugCommandKind : std::uint8_t { KillAllEnemies, SpawnWorkers, SpawnTransport, SpawnCrawlers,
  SpawnEssenceOrbs, AllowArbitraryPlacement, SpawnAtStructurePorts, SpawnSupplyCrate };
struct DebugCommand { DebugCommandKind kind; std::uint8_t team_slot; Vec3 point; std::int16_t forced_outcome; };  // −1 = normal choice

// Soak test (§4.9)
enum class SoakProgression : std::uint8_t { Linear, Random, Sequence };
struct SoakEntry { MapId map; MatchMode mode; std::uint8_t bot_teams; std::uint64_t seed; };   // seed 0 = authority's own
struct SoakConfig { SoakProgression progression; std::vector<SoakEntry> entries; std::uint64_t progression_seed;
                    std::string session_name; std::uint64_t terminate_at_tick; std::uint8_t clients; bool rendering; };
class SoakRunner { public: explicit SoakRunner(SoakConfig); SoakReport Run(); };  // owns all soak state

// Telemetry (ADR-066)
struct TelemetrySessionRecord;                                // the data set of DBG-181
void SubmitTelemetry(const TelemetrySessionRecord&);         // queued; sent only with consent

}  // namespace species::diag
```

Caption: Listing 2.1-A, the diagnostics interface (declarations only). `ProfileNodeId`, `DevDrawItem`, `DevDrawItemId` and `SoakReport` are opaque handles and records of this component.

### 2.2 Debug entry points and development bindings

**DBG-020** (MUST) Development builds add a development binding set on top of the default binding set of the [input](../input.md) component (INP-009), which binds no developer key other than `action.debug.network_stats`. The debug actions and their default bindings are:

| Action | Default binding | Context and conditions | Effect | Flavours |
|---|---|---|---|---|
| `action.debug.network_stats` | `key:f9:down` | the front end, the lobby and a match: the action is polled by the front-end loop and the match loop | opens the network statistics window when it is not open; it never closes it, the window's close box does (UI-060); in `Profile` and `Shipping` builds it does nothing | every flavour binds it; development builds act on it |
| `action.debug.command_window` | `key:f11:down` | only while no UI window is open | opens the debug-command window (§4.6) | development |
| `action.debug.drop_packet` | `key:f:held` | a participant connected to a host | discards every incoming network message while held, as a manual packet-loss test (DEC-208) | development |
| `action.debug.special_lighting_on`, `action.debug.special_lighting_off` | `key:l:held`, `key:k:held` | a match | sets the renderer's special-lighting setting on, or off (RND-442); `key:l` also drives `action.view.chat_log`, and both act | development |
| `action.debug.soak_rendering` | `key:r:down` | a soak participant (§4.9) | toggles the participant's rendering and sound together | development |
| `action.debug.overlay` | `key:f5:down` | any | shows or hides the development overlay (§4.4) | `Debug`, `Development`, `Profile` |
| `action.debug.profile_view` | `key:f10:down` | any | shows or hides the profile view (§4.3) | `Debug`, `Development`, `Profile` |
| `action.debug.camera_fast`, `action.debug.camera_slow`, `action.debug.camera_rotate` | the [input](../input.md) component's development bindings (CAM-084) | the developer fly mode | CAM-081, CAM-082 | development |

**DBG-021** (MUST) The screenshot action (`action.view.screenshot`) is a player action of [reference/06](../../../reference/06_controls_camera_and_commands/actions_and_bindings.md) row A10 and the renderer, not a debug action.

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

**DBG-024** (MUST) **Debug commands** travel as player commands of the command catalogue ([teams and player commands](../../../reference/12_simulation_model/06_teams_and_player_commands.md)) with the payload of `DebugCommand`: the sender's team, the command kind, the target team slot, the target point and the forced outcome. Who may send one and how it is validated is DBG-118 and DEC-202.

**DBG-025** (MUST) **Soak artefacts.** Each soak pass produces one soak report: one row per match (DBG-166) plus the pass totals, the crash dumps of the pass ([platform layer](../platform_layer.md)), the assertion failures, the simulation update-time percentiles and the peak population per match. The report is a CI artefact (ADR-070).

**DBG-026** (MUST) **Telemetry record.** One `TelemetrySessionRecord` per session holds exactly the data set of DBG-181; a schema check rejects any field outside it (ADR-066).

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
| `debug.bot_overlay` | `dbg.setting.bot_overlay.default` (0) | 0: off; 1: the bot priority text for one team and the link arrows; any other non-zero value: the link arrows only | DBG-135 |
| `debug.bot_overlay_team` | `dbg.setting.bot_overlay_team.default` (1) | the lobby position (not the team slot) whose priorities the overlay shows | DBG-135 |
| `debug.command_history` | `dbg.setting.command_history.default` (0) | 0: off; 1: record; 2: play back (R-SIM-740) | DBG-241 |
| `debug.capture_frame_rate` | `dbg.replay.capture_frame_rate` (25) | presentation frames per second of the fixed step that command-history modes 1 and 2 use | DBG-241 |
| language test | off | [reference/09](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-473 | reference/09 |
| special lighting | off | toggled by the debug actions of DBG-020 | RND-442 |
| poster resolution | 1 | screenshots as N × N tiles | RND-110, RND-442 |
| `debug.soak_session_name` | empty | the session name a soak host advertises and soak clients look for; a soak configuration's name replaces it | DBG-162 |

**DBG-042** (MUST) The reference's terrain storage-mode setting has no Species meaning (RND-443), and its campaign test-harness switch is not specified (DEC-052). The reference start-up switches with a diagnostic role belong to the platform layer: writing the build identity to a file and exiting (PLT-023, ADR-057), and moving every log and user file out of the profile folder (PLT-025).

### 3.6 Diagnostic outputs

**DBG-049** (MUST) Every diagnostic output of a Species client, with its writer:

| Output | Writer | Content | Flavours |
|---|---|---|---|
| log files in the logs folder (ADR-071), one per run, the last `dbg.log.files_kept` (10) kept | logging system (§4.1) | the log | every flavour |
| ETW events of the Species TraceLogging provider | logging system | the same records | every flavour |
| black-box block | this component and the platform layer | DBG-047 | every flavour |
| crash report and dump; on test machines also a local dump in the `crashes` folder (PLT-122) | platform layer, through WER (ADR-065) | dump of every thread, registered files, the black box and the crash-context record (PLT-124) | every flavour |
| language completeness report, in the `diagnostics` folder (PLT) | the client's language test (R-META-473) | three lists per language | development |
| command-history recording, in the `diagnostics` folder | the replay hook (§8.2) | the command frames of a match | development |
| soak report | the soak runner (DBG-166) | one row per match and the pass totals | CI |
| telemetry queue | the telemetry service (§4.10) | unsent session records | every flavour, with consent |
| PIX captures and ETW traces | PIX and Windows Performance Analyzer | captures taken by a developer | `Debug`, `Development`, `Profile` |
| test reports and performance results | the test harness | CI artefacts (ADR-070) | test builds |

Key-binding parse errors go to the log ([input](../input.md)). The world-state dump on divergence is DEC-206. The version file is the platform layer's (PLT-023).

## 4 Behaviour and algorithms

The behaviour of the component is specified in the topic files of this folder: §4.1 to §4.3 in [logging, checks and profiling](logging_checks_and_profiling.md), §4.4 to §4.7 in [developer tools](developer_tools.md) and §4.8 to §4.10 in [test harness, soak test and telemetry](test_harness_soak_and_telemetry.md).

### 4.11 Tuning table

A value whose notes say "Species" is a Species engine-baseline value; every other value is the reference tool's value, kept as the starting value (DEC-004).

Each topic file defines the keys its requirements own in its own tuning table: `dbg.log.*`, `dbg.blackbox.*` and `dbg.profile.*` in [logging, checks and profiling](logging_checks_and_profiling.md); `dbg.overlay.*`, `dbg.netstats.*`, `dbg.commands.*`, `dbg.draw.*` and `dbg.bot_overlay.*` in [developer tools](developer_tools.md); `dbg.trace.*`, `dbg.soak.*` and `dbg.telemetry.*` in [test harness, soak test and telemetry](test_harness_soak_and_telemetry.md); `dbg.budget.*`, `dbg.stress.*` and `dbg.machine.*` in [performance baseline](performance_baseline.md). The keys of the hidden development settings (§3.2) are these:

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `dbg.setting.bot_overlay.default` | 0 | setting value | off | DBG-041 | — |
| `dbg.setting.bot_overlay_team.default` | 1 | lobby position | | DBG-041 | — |
| `dbg.setting.command_history.default` | 0 | setting value | off | DBG-041 | DEC-046 |
| `dbg.replay.capture_frame_rate` | 25 | frames per second | fixed presentation step of the command-history modes | DBG-041, DBG-241 | DEC-046 |

Keys cited here and defined elsewhere: `ref.sim.tick_period`, `ref.sim.frame.fps_window` and `ref.sim.frame.memory_log_interval` in [tick timing](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md); `ref.sim.handle.max_slots` and `ref.sim.handle.max_group_slots` in [entity model](../../../reference/12_simulation_model/03_entity_model_and_identity.md); `ref.bot.link.range` in [navigation graph](../../../reference/07_bots/01_navigation_graph.md); `rnd.budget.target_frame_rate` in [renderer options and budgets](../renderer/options_and_budgets.md).

## 5 Concurrency and performance budget

### 5.1 Diagnostics and concurrency

**DBG-200** (MUST) Diagnostics stay outside the simulation budget (DEC-228 item 9): logging is asynchronous and buffered (DEC-224 item 12), markers cost a timer read and a per-thread stack operation, and the per-tick checks exist in test builds only. Diagnostic threads take no fixed processor affinity (DEC-224 item 5); the reference pins its main thread, which runs the simulation, rendering and every network send, to the odd-numbered processors 1, 3, 5 and 7, and loads levels on one worker thread.

Sections 5.2 to 5.5 are in [performance baseline](performance_baseline.md).

## 6 Failure modes, diagnostics and telemetry

**DBG-220** (MUST) Failures of this component and their handling; none of them changes a simulation result:

| Failure | Behaviour | Requirement |
|---|---|---|
| a thread's log buffer is full | trace, then debug records dropped and counted; warnings and above kept | DBG-061, DBG-062 |
| the log file cannot be opened | the other sinks continue; reported once | DBG-064 |
| a failed release check or bounds check | black box, flushed fatal record, platform fatal path, dump | DBG-073 |
| a failed development check | debugger break, otherwise logged and noted; the test fails | DBG-070, DBG-072 |
| a marker left with a different name | debug assertion; the tree pops the current node | DBG-081 |
| GPU timestamps not ready | the previous values are shown; the CPU never waits | DBG-080 |
| a debug command in a session without cheats, from an unauthorised sender or with an invalid argument | dropped, logged, counted | DBG-118 |
| a debug command with no terrain hit at the screen centre | not sent | DBG-116 |
| a soak map whose content hash does not match the manifest | the entry fails and is reported; the pass continues | DBG-161 |
| a soak participant crashes | dump collected, row records the crash, next entry in a new process | DBG-167 |
| a per-tick test-build check fails | the test fails with tick, handle and field | DBG-143 |
| telemetry cannot be sent | retried with back-off; the oldest record dropped beyond the queue bound | DBG-182 |

The classification of the reference cheats, DBG-221, is in [developer tools](developer_tools.md) §6.

**DBG-222** (MUST) Crash capture is the platform layer's ([platform layer](../platform_layer.md) PLT-117 to PLT-125, ADR-065): this component supplies the black box (DBG-047) and the registered log file (DBG-049), and the CI-provided local-dump setting on test machines collects soak and test crashes (DBG-167). The dedicated-server crash path is a design-phase decision (ADR-065).

**DBG-223** (MUST) This component sends no telemetry other than the data set of DBG-181, and it sends that only with consent; component counters reach telemetry only through that data set (AEN-325, AST-126).

## 7 Acceptance tests

The tests of each topic are in its file: T1 to T6, T8, T9, T12 and T13 in [logging, checks and profiling](logging_checks_and_profiling.md); T14 to T19 in [developer tools](developer_tools.md); T20 to T28 and T30 in [test harness, soak test and telemetry](test_harness_soak_and_telemetry.md); T10 and T29 in [performance baseline](performance_baseline.md). The component-wide tests are these:

| # | Kind | Test | Requirements |
|---|---|---|---|
| T7 | golden-hash | A recorded match replayed with every diagnostic facility on and with every facility off gives identical per-tick simulation hashes. | DBG-002 |
| T11 | binary check | A `Shipping` binary imports no PIX runtime and contains no marker names, no development text and no debug-command code. | DBG-004, DBG-080 |

## 8 Open decisions

### 8.1 Decisions and architecture records

| Decision | Status | Default in this document |
|---|---|---|
| DEC-041 | Resolved | the diagnostics concepts are rebuilt as specified here |
| DEC-046 | Open | replay: the command-history hook only (§8.2) |
| DEC-060 | Dropped | no benchmark recorder, no upload of it and no in-game video capture; consented telemetry (§4.10) and external capture tools replace them |
| DEC-106 | Open | the reference caps; measured on the first prototype (DBG-205) |
| DEC-201 | Open | no network budget stated (DBG-202) |
| DEC-202 | Open | debug commands accepted only in cheat-enabled sessions from the host's participant (DBG-118) |
| DEC-206 | Open | no run-time divergence check; the test-harness hash only (DBG-144) |
| DEC-208 | Open | the reference metric set in the network statistics window (DBG-105) |
| DEC-220 | Open | the headless server product runs the tests and the soak (DBG-142, DBG-160) |
| DEC-224 | Open | diagnostics on their own threads, no fixed affinity (DBG-031, DBG-200) |
| DEC-225 | Open | the reference pacing figures (DBG-201) |
| DEC-227, DEC-228 | Open | tick budget guidance and stress populations as design-phase inputs (DBG-204, DBG-205) |
| DEC-229 | Open | tick cost measured at the first simulation-core milestone (DBG-205) |
| DEC-240, DEC-242 | Open | golden hashes agree across the CI agents of one build; the generator is opaque to the harness (DBG-146, DBG-147) |
| DEC-250, DEC-251 | Open | round-trip tests once persistence is defined (DBG-149) |
| DEC-334 | Dropped | the reference's sandbox, benchmark and stress maps are not reproduced (DBG-148) |
| DEC-341 | Dropped | the reference runtime library's assertion and debug-build details are not specified (DBG-073) |
| DEC-650, DEC-653 | Resolved | the per-tick grid and finiteness checks (DBG-143) |
| DEC-802 | Open | soak and stress maps are test maps (DBG-148) |
| DEC-804 | Resolved | the soak rules (DBG-161) |

Section 8.2 is in [test harness, soak test and telemetry](test_harness_soak_and_telemetry.md).

### 8.3 Design-phase inputs

1. **Tick budget and stress population** (DEC-227, DEC-228): the per-tick CPU budget, per system and per region, its client and server forms, and the design and stress populations of DBG-204.
2. **Performance envelope** (DEC-106): the measured cost at the population cap on the prototype, and whether the reference caps stand.
3. **Reference test machine and server specification** (DBG-209): the client minimum-specification hardware and the named community-host specification.
4. **Replays** (DEC-046): a formal replay file with build identity, content hashes, seed, team layout, command stream and periodic snapshots; the replay corpus as nightly regression tests (ADR-070); its storage (DEC-250).
5. **Divergence detection** (DEC-206) and **determinism scope** (DEC-240): whether the state hash of DBG-144 also runs in released builds, and which builds must agree bit for bit.
6. **Network diagnostics** (DEC-208, DEC-201): the Species metric set, a view for many participants, the link simulator, and whether community operators receive the metrics as live telemetry.
7. **Debug-command authority** (DEC-202): who may send a debug command in a world or skirmish session.
8. **Process roles and threading** (DEC-220, DEC-224, DEC-225): the headless runner's process form, the job model whose task groups receive markers (ADR-068), and catch-up pacing.
9. **Field captures**: whether PIX markers stay in `Shipping` builds (ADR-068, ADR-056); the dedicated-server crash path (ADR-065).
10. **Build order** (DEC-229): QA and performance work from the simulation core onward, with the nightly soak and the replay corpus as its first milestone.
11. **Test maps** (DEC-802): the visibility field that keeps soak and stress maps out of player editions.
12. **World persistence** (DEC-251): round-trip and migration tests of the world database.
