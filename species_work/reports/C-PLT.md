# C-PLT report: platform layer

## Files written

| File | Size | Notes |
|---|---|---|
| `design_next/architecture/components/platform_layer.md` | 112,721 bytes | component template §6.4, PLT-001 to PLT-131 |
| `species_work/scripts/C-PLT_number.py` | script | turns the symbolic requirement tokens of the draft into PLT ids |
| `species_work/scripts/C-PLT_check.py` | script | ids, keys, cited keys and requirement ids of other documents, DEC, ADR and asset ids, headings, truncation words |

The draft is kept at the session scratchpad (`plt/draft_1.md` to `draft_4.md`); the target is generated from it.

Checks run: `C-PLT_check.py` clean; forbidden-pattern scan empty; soft-pattern scan one hit (`Linux`, line 986, §8.2 Design-phase inputs, see Cross-document requests 1); Mermaid 11.4.1 parse of all 5 blocks passes (headless Chromium, `mermaid_check.js`); every relative link target exists; no truncation words; template headings §1 to §8 present; file under 120 KB.

**PLT id stability.** C-DBG and C-BLD cite PLT ids of a 131-requirement version. The current file has that numbering again (two short requirements, PLT-073 and PLT-116, were restored after a size trim so that every later id is unchanged). No old-to-new map is needed. Map of the ids that others cite: PLT-016 user-data root, PLT-017 folders, PLT-023 `--write-build-identity`, PLT-025 `--user-data`, PLT-042 quit, PLT-044 exit codes and `EndWithMessage`, PLT-117 no crash filter, PLT-118 fatal handlers, PLT-119 crash registration, PLT-121 debugger, PLT-122 local dumps, PLT-124 crash-report contents, PLT-125 privacy, PLT-126 check levels, PLT-127 the fatal path `FailFast`.

## Counts

- Requirements: 131 (PLT-001 to PLT-131), all in the range 001–299, unique and increasing.
- Tuning keys: 19 (`plt.os.minimum_build`, `plt.window.dpi_base`, `plt.window.hang_threshold`, `plt.lifecycle.exit_code.normal`, `plt.lifecycle.exit_code.fatal`, `plt.sysinfo.memory_unit`, and 13 `plt.display.reference.*` keys for the reference display-creation values that R-META-423/424 cite). Every key is cited by a requirement. Values owned elsewhere are cited, not redefined: `dbg.replay.capture_frame_rate` (fixed-step default rate, DBG-041), `dbg.blackbox.check_message_max_chars` (DBG-047), `rnd.option.window_mode.default`, `rnd.option.render_size.default`, `rnd.init.fallback_window_size`, `rnd.init.min_render_width/height`, `ref.meta.options.screen.*`, `ref.meta.settings.language.default`, `ref.meta.lang.base`, `ref.sim.frame.delta_clamp`, `ref.sim.frame.loading_frame_period`.
- Diagrams: 5 Mermaid, each captioned (3-A user-data root resolution; 4-B window states; 4-C language choice at start-up; 6-A window and swap-chain fallback chain; 6-B crash, check-failure and hang handling).
- Enumerations and tables: 1-A services (19), 1-C reference platform facts (10), 3.1-A user-data folders (6), 3.2-A runtime files (20), 3.3-A start-up options (5), 3.4-A reference display preferences (9), 3.6-A system summary groups (8), 4.3-A window messages (14), 4.5-A presentation features (6), 6.1-A reference creation attempts (4), 6.3-A crash-report contents (16), 6.4-A failures (14), start-up order (17 steps), shutdown order, language order (4), pointer services (4), fatal path (3 steps); 34 acceptance tests.

## Decisions cited

- Open (all of `decs_to_cite`, each cited by at least one requirement): DEC-200, DEC-207 (PLT-100); DEC-220 (PLT-005); DEC-221 (PLT-038, PLT-042, PLT-043, PLT-086); DEC-224 (PLT-084, PLT-085, PLT-087, PLT-090); DEC-225 (PLT-055, PLT-056, PLT-067); DEC-240 (PLT-087); DEC-248, DEC-249 (PLT-020, PLT-099, PLT-100); DEC-250 (PLT-020, PLT-031); DEC-251 (PLT-019).
- Also cited: DEC-226, DEC-228, DEC-044, DEC-046, DEC-206, DEC-204 (Open); DEC-009, DEC-010, DEC-099 (Resolved).
- Resolved, stated as the fixed rule: DEC-041 (PLT-048), DEC-100 (PLT-030, PLT-080, PLT-115), DEC-800 (PLT-033, PLT-079, PLT-081, PLT-089).
- Dropped: DEC-333 (PLT-020, PLT-101), DEC-341 (§8.1), DEC-060, DEC-061 (table 3.2-A).

## New DEC requests

None required. Two behaviours are new engine-baseline rules without a register entry, stated in the document; the coordinator may want entries:

1. A system-summary query that fails is never fatal (PLT-093); the reference stops on a failed locale query.
2. The system summary and the crash-context record never hold the Windows user name, and paths are written relative to placeholders (PLT-035, PLT-125); the reference crash log records the user name and the program path.

## ADR conflicts and clarifications

No conflict. Clarifications for X-ADR:

1. **ADR-0074** says the platform layer selects the time source at start-up. DBG-241 needs the fixed-step source when the development setting `dev.command_history` is 1 or 2, which is known only after the settings load. PLT-049 replaces the source at start-up step 10 with continuity (PLT-051). The ADR text could mention it.
2. **ADR-0065** names BLD and DBG as its components. The crash path (handlers, registrations, thread naming) is PLT's, as BLD-110 and DBG-222 state; the ADR's "More Information" could add PLT.
3. **ADR-0050** says a "launcher" shows the below-minimum message. Species has no launcher (ADR-0071 lists `species.exe` only); the platform start-up check does it (PLT-040). It shows a notice and continues; the ADR could say so.
4. **ADR-0066** requires "the platform layer's HTTP client". No ADR chooses the HTTP stack. PLT-099 specifies behaviour (HTTPS only, OS certificate validation, system proxy) without naming a library. An ADR request: choose the client HTTP stack (the in-box WinHTTP is the obvious default).
5. **ADR-0071** lists `settings\`, `logs\` and `crashes\`. PLT-017 adds `maps\` (the player's map folder, R-MAP-391) and `diagnostics\` (development outputs, `Debug` and `Development` only). This extends the rule; it does not contradict it.

## Missing assets

1. **Application icon:** the executable's icon resource, also used as the window icon (PLT-057). No row in `asset_manifest.csv`. Proposal: `ui.app_icon`, a multi-size Windows icon (16, 24, 32, 48, 64 and 256 px), priority high, consumer `architecture/components/platform_layer.md`.

## Cross-document requests

1. **Coordinator, CONVENTIONS §9:** allow `Linux` in the "Design-phase inputs" of `architecture/components/*.md` for dedicated-server hosting. Hit: `platform_layer.md` §8.2 item 2, written as the coordinator asked (design-phase input only, never a decision).
2. **C-DAT** (`data_schemas/config_input_and_strings.md`):
   - add two settings to table 3.1-A: `window.monitor` (text, the monitor's device name, absent: the primary monitor) and `window.client_rect` (four integers, desktop coordinates, absent: centred render size) (PLT-028);
   - DAT-198 asks the platform layer to name the file: it is `settings\settings.txt` (PLT-031).
3. **C-INP** (`input.md`): INP-100 step 4 says the settings store loads after the window is created. Per R-META-402 and PLT-038 the store loads at step 10, before the window (step 13). Please reword it as "the router reads its settings from the store". The other C-INP requests are adopted: forwarding (PLT-008), focus and layout notices with BCP 47 tags (PLT-010, PLT-064, PLT-069), close request as `action.app.quit` (PLT-009), pointer capture, confinement, warping and a hidden system pointer (PLT-068), pumping during loading views (PLT-067).
4. **C-RND-b** (`renderer/options_and_budgets.md`): `species::render::WindowMode` duplicates `species::platform::WindowMode` (same enumerators). Please alias the renderer's to the platform's. C-RND-a's request is answered: the chain of RND-023 is PLT-112; the colour-depth, refresh-rate, depth-bit and overscan preferences have no Species meaning (PLT-029).
5. **C-BLD** (`build_tooling_and_distribution.md`):
   - BLD-103 cites PLT-042 for `plt.lifecycle.exit_code.normal`; the exit codes are PLT-044.
   - `--user-data` does not move the local-dump folder that BLD-113's registry file names (PLT-025).
   - The coordinator's six C-BLD requests are adopted: PLT-022 to PLT-024 (both switches), PLT-104 and PLT-105 (identity composed from `species_build_identity.h` and the mounted manifest, compared with `build_identity.json`, BLD-020), PLT-106 (channel tag, BLD-021), PLT-024 (`ui.repair.passed` and `ui.repair.failed` through the pre-UI message path with compiled-in text), PLT-017 and PLT-122 (`crashes\`), PLT-117 to PLT-125 (crash path, registrations, thread naming).
6. **G09b** (`options_localisation_and_hooks.md`):
   - the keys asked for exist: `plt.display.reference.reader_windowed`, `reader_colour_bits`, `reader_refresh`, `reader_depth_bits`, `reader_vsync_request`, `overscan`, `desktop_attempt_depth_bits`, `fallback_vsync_request`, `window_attempt_colour_bits`, `window_attempt_depth_bits`, `window_attempt_refresh`, `last_attempt_depth_bits` and `failure_message_position` (PLT-029, PLT-113). R-META-423/424 can cite them.
   - The settings-store rules are confirmed: first occurrence wins, comments and blank lines kept (PLT-079).
7. **C-AEN:** adopted. The audio clock is the platform clock (PLT-001, PLT-046). Device-arrival and default-change notices reach the engine on the main thread (PLT-011, PLT-103). The audio settings live in the settings store (PLT-030).
8. **C-AST:**
   - the AST-102 messages go through the pre-UI message path with compiled-in text (PLT-039, table 6.4-A);
   - the loose-content option is named `--loose-content <directory>` (PLT-026, table 3.3-A); AST-105 could cite the name.
9. **C-DBG:** aligned with the coordinator's notice:
   - `FailFast` is PLT-127 and `EndWithMessage` PLT-044;
   - development checks follow DBG-070 (PLT-126);
   - `dbg.blackbox.check_message_max_chars` is cited (table 6.3-A), and the former `plt.crash.check_message_max_chars` is not defined;
   - development outputs go to `diagnostics\` (PLT-017), which DBG's language report already uses.
10. **X-DEC-2b:** adopted. DEC-225 items 6, 8 and 9 are PLT-056, PLT-048 to PLT-051 and PLT-067. The per-user profile directory and its switch are PLT-016 and PLT-025: no switch writes to the working directory.
11. **X-AST pass 2:**
    - new string ids, each needing new base-language text and compiled-in text (table 6.4-A): `ui.error.os_unsupported`, `ui.error.user_data_unavailable`, `ui.error.user_data_inside_install`, `ui.error.language_missing`, `ui.error.internal`;
    - the application icon (Missing assets).
12. **X-GLO:** candidate entries, all linking to the platform layer: platform time, platform clock, fixed-step source, manual source, pre-UI message, run id, user-data root, system summary, crash-context record, fatal path. The existing "profile folder" entry matches PLT-016.

## Names needed that are not in the ledger (proposals)

These are new identifiers for the interface sketch. None repeats a reference identifier, and the ledger names `SettingsStore`, `JobQueue`, `TickClock` and `kProductName` are used as given.

- **Clock:** `PlatformClock`, `TimeSource`, `TimeSourceKind`, `ManualTimeSource`, `Nanoseconds`.
- **Window:**
  - `AppWindow`, `WindowMode` (platform), `Permanence`, `WindowPlacement`, `WindowMessage`, `WindowInputSink`, `WindowEvent`;
  - the events `Resized`, `DpiChanged`, `FocusChanged`, `Minimised`, `Restored`, `ModeChanged`, `MonitorChanged`, `DisplayConfigurationChanged`, `KeyboardLayoutChanged`, `EndOfSession`;
  - the display types `MonitorInfo`, `DesktopMode`, `ModeSize`;
  - the geometry types `PointPx`, `SizePx`, `RectPx`, `NativeWindowHandle`.
- **Files:** `PlatformPaths`, `UserFolder`, `MappedFile`, `FileError`, `PlatformError`, `SettingValue`.
- **Lifecycle:** `StartupOptions`, `MessageSeverity`, `MessageArgument`, `LifecycleHooks`, and the functions `EndWithMessage`, `FailFast`, `ExitAfterTeardown`, `ShowPlatformMessage`.
- **Threads and crash:** `PlatformThread`, `CrashContext`, `CrashFileKind`, `SystemSummary`.
- **Services:** `HttpsClient`, `HttpsRequest`, `HttpsResponse`, `HttpsError`, `AudioEndpointChange`, `ScreenshotImage`, `ScreenshotWriter`.
- **Files and settings:** the settings file name `settings.txt`, and the user-data folders `maps\` and `diagnostics\`.

## Dropped as legacy (one line per group)

- **Superseded window and display code:** the window-manager class hierarchy, pixel formats, display-mode changes, desktop restore, and the temporary small window created between two full-screen modes. Species never changes the display mode (ADR-0075, ADR-0012).
- **Re-creating the window and renderer and rebuilding every texture on a mode or size change:** replaced by in-place mode changes and swap-chain resize (PLT-061, RND-030).
- **The gamma-ramp function, the per-API vertical-sync flags and the unused multisample path:** brightness in the final pass and the present modes of ADR-0012.
- **The window procedure's per-message return codes,** including the cursor-message fall-through that reset the pointer offset: behaviour restated in table 4.3-A.
- **The commented-out Alt+Tab hot-key binding:** Alt+Tab is never captured (PLT-065).
- **The non-Windows clock, window, input, thread, file-lookup, logging and data-folder variants:** client is Windows only (ADR-0050). The server variants are a design-phase input.
- **The cycle-counter clock variant, the absolute-counter variant, the double-precision seconds representation and its uptime caveat:** integer nanoseconds (ADR-0074).
- **The main-thread-only exception filter and its crash-log format:** registers, 16 code bytes, a stack dump of at most 3072 words in 4 columns, the 24 exception descriptions, the module walk and the crash-reference code from the file time. WER dumps carry this content (table 6.3-A, ADR-0065).
- **The support tool and its pieces:** the external diagnostic program, the compressed bundle, the Send dialog and the plain-TCP upload with its magic header and crash-report id. Consent comes from Windows diagnostic-data settings (ADR-0065).
- **The frame-pointer stack walk and the linker-map symbolisation scripts:** dump plus symbol store (DBG-048, ADR-0065).
- **The debug-assert side effect of showing the system pointer in release builds:** development checks are compiled out (DBG-070).
- **The graphics-runtime version probe in the registry, the audio-device scoring over the legacy wave API (with its last-device-wins defect), the English locale-name and country queries, the OS-name table and the version checks for old Windows versions:** BCP 47 tags, the default audio device and the minimum-build check (PLT-040, PLT-075, PLT-094).
- **The working-directory data root, the substring matching of switches and the media-centre launch switch:** content opens from the install directory (PLT-015). Options are whole arguments (PLT-022). There is no media-centre launch.
- **Credential-key files, clipboard paste of keys and the trial gating:** DEC-333, DEC-248.
- **Campaign profiles and saves, the mod search path, benchmark files, the sound-error report, gesture data, test-bed and trace logs, the rich saved-game header:** not specified (table 3.2-A).
- **The UTF-16 settings file and the locale call that changes the C library's case mapping:** UTF-8 (ADR-0063), with ICU per language (PLT-078).
- **Lost-focus pause in single player:** focus never holds the world (R-SIM-675). Its consumers are listed in PLT-064.
