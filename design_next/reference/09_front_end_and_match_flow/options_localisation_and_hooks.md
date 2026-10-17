# Options, localisation and hooks

> Status: Reference · Prefix: R-META · Related: [Folder index](README.md) · [Screen flow and match setup](screen_flow_and_match_setup.md) · [01 Skirmish rules and modes](../01_skirmish_rules_and_modes/README.md) · [01 Launch Race](../01_skirmish_rules_and_modes/launch_race.md) · [06 Controls, camera and commands](../06_controls_camera_and_commands/README.md) · [08 Maps and level design](../08_maps_and_level_design/README.md) · [10 Audio design](../10_audio_design.md) · [11 Tutorial and help](../11_tutorial_and_help.md) · [12 Simulation model](../12_simulation_model/README.md) · [UI framework](../../architecture/components/ui_framework/README.md) · [Renderer options](../../architecture/components/renderer/options_and_budgets.md)

## 1 Purpose and player experience

This document specifies what the player can set and what the client remembers between sessions, how the game speaks the player's language, and the hooks of the features that the design phase designs: achievements, statistics and leaderboards. It also specifies the one in-match status panel that belongs to no mode page, the Launch Vessel status panel of Launch Race.

**Settings.** The player opens the settings from the help-and-options branch, at the front end or from the in-match menu ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) §2.17). Four pages follow one pattern: the page edits copies of the stored values, an apply action stores and applies them, and the back action discards them. The screen options set the screen size, a window or full screen, and three full-screen display parameters; the graphics options set four detail levels; the sound options set the audio output, the mixing rate, the volume, the number of sounds and the stereo orientation; the other options switch the in-match tips, set the network ports and choose the text language. A handful of further settings are stored without any page: the player name, the preferred colour, the session-browser filters, renderer and input refinements, and development aids. The client keeps all of them in one settings store in the player's profile folder. No setting changes a match: everything that changes the simulation is a match option.

**Language.** Every word the player reads comes from a string table keyed by Species string ids. The reference set has nine languages. A language whose table lacks a string shows the base-language text for that string, and only complete languages are offered (DEC-099). Fonts are licensed families rasterised at run time into a glyph cache, with companion faces for Japanese and Korean.

**Hooks.** The match rules evaluate 12 achievement conditions and keep a small set of statistics, but nothing is awarded, stored or ranked: the platform layer behind them is missing, the achievements page never finishes loading, and the leaderboard pages have no data source. This document records exactly what those hooks compute, where, and what the pages would show (DEC-047). The design phase designs the features.

**Loading.** The loading view itself is [front_end_presentation.md](front_end_presentation.md); this document adds the parts of start-up that read the settings and the language (§2.10).

## 2 Rules

### 2.1 The settings store

**R-META-400** (MUST) The client keeps the player's settings in the settings store (`SettingsStore`): one set of named, typed values, held in memory and backed by one settings file per user in the `settings\` folder of the user-data folder `%LOCALAPPDATA%\<Studio>\Species\` (ADR-0071, DEC-250). The store and its file handling are specified by the [platform layer](../../architecture/components/platform_layer.md), the file grammar and the preferences schema by [data schemas](../../architecture/components/data_schemas.md). This document specifies which settings exist, on which page each one is shown, what the pages do and the defaults of §4.2 that no other document owns.

**R-META-401** (MUST) Settings are presentation and process state. No simulation rule reads a setting, and a setting that would change simulation results exists only as a match option in the match setup (DEC-010; [reference/12](../12_simulation_model/07_randomness_and_integrity.md) R-SIM-660). The stored difficulty setting of §2.6 is the one stored value that a simulation rule could read, and it is inert in every match (R-META-449).

**R-META-402** (MUST) The store is created at start-up, after the start-up configuration has been read and before the window, the renderer and the audio engine are created, so each of them reads its settings from the store. Its defaults come from the one defaults table of DEC-100, which is available before any pack file is mounted, so packaged and development builds start with the same defaults.

**R-META-403** (MUST) **First run.** When the settings file cannot be opened, the store takes every default from the defaults table (DEC-100): the defaults of §4.2 and the defaults that the owning documents state for their settings. It then applies the forced value of R-META-405, and the renderer's start-up writes the display settings and saves the store (R-META-424). Where the reference has two defaults for one setting, the table holds the one that §5 item 1 names.

**R-META-404** (MUST) **Loading.** Each entry of the settings file has the type, range and default that the preferences schema declares; an entry that does not parse, or whose value is outside its type or range, is written to the log and takes its default, and every other entry keeps its stored value (DEC-800 item 5). When a key occurs more than once, the first occurrence is used and every later one is written to the log and ignored. Comment lines and blank lines are kept for the next save. The reference reader that DEC-800 replaces is described in §5 item 2.

**R-META-405** (MUST) After every load, the control-scheme setting (`ui.options.control_scheme`) is set to the match control scheme, so the alternative scheme never applies (DEC-056). The binding files of the scheme are [reference/06](../06_controls_camera_and_commands/actions_and_bindings.md).

**R-META-406** (MUST) The store saves the settings file on each of these events, and on no other:
1. the apply action of any options page (§2.3 to §2.6);
2. every change of the player name, from the name field or the chat rename command, and every creation of the front-end screen, which stores the player name it read ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) R-META-008);
3. the name apply action of the player options page;
4. the renderer's start-up, after it has set the display mode (R-META-424);
5. leaving the session-browser filter page, which stores the filters;
6. the release of the master-volume slider (R-META-433).

**R-META-407** (MUST) A save writes the file losslessly and atomically (DEC-800 items 1, 3 and 4): every comment and blank line of the loaded file in its place, each loaded key once with its current value, then every key that the loaded file did not hold. A save that fails leaves the previous file in place, is written to the log and shows `ui.dialog.settings_save_failed`.

**R-META-408** (MUST) These are not kept in the settings store:
1. the bindings of the control schemes, and the per-keyboard-layout binding overrides, which are content ([reference/06](../06_controls_camera_and_commands/actions_and_bindings.md), [input](../../architecture/components/input.md));
2. a user bindings file, written only by the key-rebinding page, which no player can reach (DEC-050);
3. any match, lobby or session state: nothing about a session outlives the process ([reference/12](../12_simulation_model/02_match_lifecycle_and_state_reset.md) R-SIM-031);
4. achievement progress, which is held for the session only (R-META-490);
5. identity tokens, which belong in the operating system's credential store (DEC-248, DEC-250);
6. user profiles and mods, which the library does not specify (DEC-061);
7. the command-history record of the replay hook, which is a development file (DEC-046; [reference/12](../12_simulation_model/07_randomness_and_integrity.md) R-SIM-740).

**R-META-409** (MUST) The settings file carries no schema version and is never migrated: it is the client-local store of the reference. Versioning, forward migration and synchronisation of settings are the design phase's (DEC-250).

### 2.2 Settings navigation and the shared page pattern

**R-META-410** (MUST) The settings screen, titled `ui.options.title`, lists from top to bottom: the screen options `ui.options.screen.title`, the graphics options `ui.options.graphics.title`, the sound options `ui.options.sound.title`, the other options `ui.options.other.title`, then Back (`ui.common.back`). It has no controls page: the key-rebinding page exists only as an unreachable hook (DEC-050; [reference/06](../06_controls_camera_and_commands/actions_and_bindings.md) R-CTRL-034).

**R-META-411** (MUST) The settings screen opens from the help-and-options window, at the front end and from the in-match menu ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) R-META-072). Each options page is a full-screen options screen (`OptionsScreen`), built by the full-screen page recipe of [pages_and_screen_recipes.md](../../architecture/components/ui_framework/pages_and_screen_recipes.md).

**R-META-412** (MUST) Every options page follows one pattern:
1. on opening, the page reads each of its settings into a copy held by the page; its controls edit only the copies;
2. the apply action `ui.options.apply` is inactive, and drawn greyed, while every copy that the page compares equals the stored value (each page states which copies it compares);
3. the apply action writes the copies to the store, saves it, applies the page's effects and re-reads the copies from the store;
4. Back closes the page and discards every copy.

The master-volume slider is the one control that acts at once and saves on its own (R-META-433).

**R-META-413** (MUST) The options controls are of four kinds:
1. a **check box**, toggled by activation, showing `ui.icon_check` while on;
2. a **cycle list**, whose activation advances to the next choice and wraps from the last to the first;
3. a **full list**, whose activation opens the list of choices over the page, and whose choice closes it;
4. a **slider** (R-META-433) and a **number field** (R-META-444), which edit a value directly.

Their look, focus and navigation are the [UI framework](../../architecture/components/ui_framework/core_and_widgets.md)'s.

### 2.3 Screen options

**R-META-414** (MUST) The screen options page, titled `ui.options.screen.title`, shows from top to bottom: the screen size `ui.options.screen.size` (full list), play in a window `ui.options.screen.windowed` (check box), the refresh rate `ui.options.screen.refresh` (cycle list), the colour bits `ui.options.screen.colour_bits` (cycle list), the depth bits `ui.options.screen.depth_bits` (cycle list), apply and Back.

**R-META-415** (MUST) The screen-size list holds every distinct width × height that the display reports for a mode of at least `ref.meta.options.screen.min_colour_bits` (15) bits per pixel and at least `ref.meta.options.screen.min_size` (640 × 480), in the order in which the display reports them, each captioned `ui.options.screen.size_value`. The size is stored as two settings, the width and the height. The page's copy is the entry of the stored size; the stored size is always in the list, because the renderer snaps it to an offered size at start-up (R-META-424). In the Species engine these entries are the render sizes that the renderer offers, with the default `rnd.option.render_size.default` ([renderer options](../../architecture/components/renderer/options_and_budgets.md) RND-442).

**R-META-416** (MUST) Choosing a size, the page's first selection included, rebuilds the refresh-rate choices from the rates that the display reports for that size, each captioned `ui.options.screen.refresh_value`, and selects the page's refresh copy among them. When the copy is not one of them, the control shows its label without a value and the copy keeps its value.

**R-META-417** (MUST) **Play in a window.** The page's copy defaults to `ref.meta.options.screen.windowed_default` (off) when the setting is missing. On, the game runs in a captioned window with minimise and maximise boxes, centred on the desktop; a window larger than the desktop cannot be created (R-META-423). Off, the game runs full screen at the chosen size. The Species window modes are windowed and borderless full screen, the display mode is never changed, and the default is borderless full screen (ADR-0075, `rnd.option.window_mode.default`).

**R-META-418** (MUST) **Refresh rate.** The cycle list offers the rates of the chosen size; the page's copy defaults to `ref.meta.options.screen.refresh_default` (60 Hz). It sets the display frequency of the full-screen mode. While play in a window is ticked, the control is inactive and drawn greyed with `ui.options.screen.fullscreen_only`. The Species engine has no refresh-rate setting (RND-443).

**R-META-419** (MUST) **Colour bits.** The cycle list offers `ref.meta.options.screen.colour_choices` (16, 24 and 32 bits), each captioned `ui.options.screen.bits_value`; the page's copy defaults to `ref.meta.options.screen.colour_default` (16). It sets the bits per pixel of the full-screen mode and the colour bits of the back buffer, and is inactive and greyed while play in a window is ticked, as R-META-418. The Species engine has no equivalent (DEC-100, RND-443).

**R-META-420** (MUST) **Depth bits.** The cycle list offers `ref.meta.options.screen.depth_choices` (16 and 24 bits), captioned `ui.options.screen.bits_value`; the page's copy defaults to `ref.meta.options.screen.depth_default` (24). It sets the precision of the depth buffer, in both window modes. The Species depth buffer has one fixed format, so the engine has no equivalent (RND-443).

**R-META-421** (MUST) The screen options compare the width, the height, the refresh rate, play in a window, the colour bits and the depth bits for R-META-412 item 2.

**R-META-422** (MUST) The screen options apply runs, in this order:
1. write the width and the height of the chosen size, the refresh rate, play in a window, the colour bits and the depth bits;
2. create the window and the renderer again with the new values; a change from one full-screen mode to another always performs a real display-mode change, even when only the refresh rate differs;
3. when the screen size changed, lay the user interface out for the new size and move every open window so that its centre keeps its proportional place: new centre = old centre × new size ÷ old size, on each axis;
4. save the store and re-read the page's copies;
5. close every window; at the front end, build the front-end screen again; in a match, open the match view again;
6. open the settings screen and the screen options page again, so that the player is back on the screen options at the new size.

The Species renderer applies a new size or window mode by reconfiguring itself, without creating the device again or reloading content ([renderer options](../../architecture/components/renderer/options_and_budgets.md) RND-447, ADR-0075).

**R-META-423** (MUST) When the window or the renderer cannot be created with the requested values, the message `ui.dialog.screen_mode_failed` is shown with the requested size, and creation is retried with, in this order: the desktop's own size with 24 depth bits and no vertical-sync request; 800 × 600 in a window with 16 colour bits, 16 depth bits and 60 Hz; the same with 24 depth bits. When the last attempt fails, start-up stops with a fatal error. The retry values and the message position are the [platform layer](../../architecture/components/platform_layer.md)'s (window creation). After a successful creation the store holds the values in effect, the size, play in a window, the colour bits, the depth bits, the refresh rate, the vertical-sync request and an overscan value of 0, and is saved.

**R-META-424** (MUST) At start-up the renderer reads the screen settings, with its own defaults for missing ones, stated by the [platform layer](../../architecture/components/platform_layer.md): full screen, 32 colour bits, 75 Hz, 24 depth bits and the vertical-sync request on. A stored size of 0 × 0, which is the first-run state, takes the desktop's size, refresh rate and colour bits and writes them to the store. The size is then snapped to the closest mode that the display offers. After the mode is set, the renderer writes the screen settings back and saves the store (R-META-406 item 4); the overscan value is always written as 0 and has no other effect. The Species render size follows the same first-run rule from `rnd.option.render_size.default` (0 × 0, the desktop size of the window's monitor) and the snapping of RND-022.

### 2.4 Graphics options

**R-META-425** (MUST) The graphics options page, titled `ui.options.graphics.title`, shows from top to bottom: sea detail `ui.options.graphics.sea`, sky detail `ui.options.graphics.sky`, structure detail `ui.options.graphics.structures` and unit detail `ui.options.graphics.units`, then apply and Back. Each is a cycle list of three choices: `ui.options.graphics.level_high` (detail level 1), `ui.options.graphics.level_medium` (2) and `ui.options.graphics.level_low` (3); a missing setting shows level 1. The four settings, their defaults (`rnd.option.sea_detail.default`, `rnd.option.sky_detail.default`, `rnd.option.structure_detail.default`, `rnd.option.entity_detail.default`, each 1) and their effects are the renderer's ([renderer options](../../architecture/components/renderer/options_and_budgets.md) RND-442 to RND-446).

**R-META-426** (MUST) The graphics options compare six copies for R-META-412 item 2: the four detail levels and two hidden settings that the page reads and writes back unchanged, the terrain detail and the pixel effect, neither of which has any effect (R-META-456). The apply action writes all six and saves the store.

**R-META-427** (MUST) When a match is loaded, the graphics apply also generates the terrain and the sea again from the map definition (DEC-246, DEC-250); outside a match it only stores the settings. The sea detail is read when the sea is built, so a change takes effect at the next build, which this apply performs in a match; the sky, structure and unit detail are read on every frame. In the Species renderer no setting rebuilds terrain or sea geometry: every sea-detail mesh is built at match start, and every change takes effect on the next frame (RND-447 to RND-449).

**R-META-428** (MUST) Two further graphics controls exist only as removed code and are never shown: a terrain detail of four levels and a pixel-effect check box. Their stored settings have no effect (R-META-456).

### 2.5 Sound options

**R-META-429** (MUST) The sound options page, titled `ui.options.sound.title`, shows from top to bottom: the audio output `ui.options.sound.output` (full list), the mixing rate `ui.options.sound.mix_rate` (cycle list), the master volume `ui.options.sound.master_volume` (slider), the bus volumes of R-META-434 (sliders), the number of sound voices `ui.options.sound.voices` (cycle list), swap left and right `ui.options.sound.swap_stereo` (check box), then apply and Back. The settings, their defaults and their engine effects are the [audio engine](../../architecture/components/audio_engine.md)'s; the mix that they change is [reference/10](../10_audio_design.md)'s.

**R-META-430** (MUST) **Audio output.** The list offers exactly one choice, the platform audio back end `ui.options.sound.output_platform` (ADR-0022), and the apply action stores it. A stored value that names no back end, which only an edit of the settings file can produce, switches all audio output off: the audio engine then runs on its silent device. The first-run value is the platform back end.

**R-META-431** (MUST) **Mixing rate.** The cycle list offers `ref.meta.options.sound.mix_rate_choices` (11 025, 22 050 and 44 100 Hz), each captioned `ui.options.sound.rate_value`; the page's copy and the first-run value are `ref.meta.options.sound.mix_rate_default` (22 050 Hz), while the audio engine's own reader uses the default that the audio engine states when the setting is missing. It sets the output mixing rate, and the apply action restarts the audio engine (R-META-437). The Species audio engine mixes at its fixed runtime rate (ADR-0025), so the setting has no Species equivalent (DEC-100).

**R-META-432** (MUST) **Sound voices.** The cycle list offers `ref.meta.options.sound.voice_choices` (8, 16, 32 and 64 voices), each captioned `ui.options.sound.voices_value`; the page's copy defaults to `ref.meta.options.sound.voices_page_default` (16) when the setting is missing, while the first-run value and the audio engine's default are the audio engine's (32). The count is the number of voices the engine mixes: one fewer effect voices than the count, plus music. The apply action restarts the audio engine. The Species mix shares a fixed budget of 64 real voices over its buses (DEC-570).

**R-META-433** (MUST) **Master volume.** The slider sets a value from 0 to `ref.meta.options.sound.volume_max` (255). Its track spans `ref.meta.options.sound.slider_track` (from 50 % to 95 % of the control's width), and the value is the pointer's fraction along the track × 255, truncated. Pressing or dragging on the track applies the value at once to every playing sound and to music; releasing writes it and saves the store (R-META-406 item 6). The slider works independently of the apply and Back actions, and Back does not undo it. The volume law, the attenuation per step, is [reference/10](../10_audio_design.md)'s.

**R-META-434** (MUST) Below the master volume the page has one volume slider for each bus of the mix, in this order: interface `ui.options.sound.volume_ui`, announcer and stings `ui.options.sound.volume_announcer`, unit voices `ui.options.sound.volume_voice`, world `ui.options.sound.volume_world`, ambience `ui.options.sound.volume_ambience` and music `ui.options.sound.volume_music` (DEC-570). Each acts like the master volume (R-META-433) on its bus; the bus default levels are those of DEC-570 and [reference/10](../10_audio_design.md). The reference page has the master volume only (DEC-044).

**R-META-435** (MUST) **Swap left and right.** The check box is off by default. On, the left and right channels of every positioned sound are exchanged; the setting is read on every audio update, so it takes effect when the apply action stores it.

**R-META-436** (MUST) The sound options compare, for R-META-412 item 2, the copies of the mixing rate, the voices, swap left and right and the audio output with the stored settings, reading each missing setting as the page's value: `ref.meta.options.sound.mix_rate_default`, `ref.meta.options.sound.voices_page_default`, off, and, for a missing or empty audio output, a match with either choice. So with the voices setting missing, the apply action shows as active (§5 item 1).

**R-META-437** (MUST) The sound options apply writes the settings, restarts the audio engine, saves the store and re-reads the copies. A restart creates the engine's voices and devices again: playing sounds survive but lose their voices, so a looping sound starts again from the beginning of its sample, a one-shot sound expires a few audio updates later, and music keeps its read position; the exact rule is the [audio engine](../../architecture/components/audio_engine.md)'s.

**R-META-438** (MUST) Three further sound controls exist only as removed code and are never shown: a sample-memory level, a hardware-positioning check box and a real-time effects check box. Their settings are R-META-454.

### 2.6 Other options

**R-META-440** (MUST) The other options page, titled `ui.options.other.title`, shows from top to bottom: in-match tips `ui.options.other.tips` (check box), use forwarded port `ui.options.other.port_forwarding` (check box), the hosting port `ui.options.other.host_port` (number field), the joining port `ui.options.other.join_port` (number field), the text language `ui.options.other.language` (full list), then apply and Back.

**R-META-441** (MUST) **In-match tips.** The setting defaults to `ref.meta.settings.tips.default` (on). On, the contextual hints of [reference/11](../11_tutorial_and_help.md) appear in matches (DEC-037); off, none appears. It has no other effect.

**R-META-442** (MUST) **Use forwarded port.** The setting defaults to `ref.meta.settings.port_forwarding.default` (off). On, the client reports its own configured listening port to the session instead of the port at which others see it, and a hosted session is advertised with its own listening port. How sessions are reached and discovered is the design phase's (DEC-207, DEC-245); the reference's traversal through a directory service is not specified.

**R-META-443** (MUST) **Hosting port.** The setting defaults to `ref.meta.settings.host_port.default` (4000). It is the port on which a hosted session listens, and the port of a manual session entry that names none (R-META-452).

**R-META-444** (MUST) The two port controls are number fields that show the stored whole number as text and accept `ref.meta.settings.port_min` (0) to `ref.meta.settings.port_max` (65 535). The forwarded-port check box and both port fields are inactive while this participant hosts a session.

**R-META-445** (MUST) **Joining port.** The setting defaults to `ref.meta.settings.join_port.default` (4001). It is the port on which the client listens. When that port cannot be opened and is not `ref.meta.settings.port_any` (0), the client opens any free port instead.

**R-META-446** (MUST) **Text language.** The full list has one entry per offered language (R-META-463), each captioned with the language's name (R-META-467). The page's copy is the entry whose tag equals the stored language, compared without regard to case. Applying another language switches to it (R-META-466).

**R-META-447** (MUST) The other options compare the tips, the text language, the forwarded port, the hosting port and the joining port for R-META-412 item 2; they do not compare the hidden settings of R-META-449.

**R-META-448** (MUST) The other options apply runs, in this order:
1. write the tips setting and the control-help setting;
2. at the front end only, never in a match, write the difficulty setting (R-META-449 item 2);
3. when a match is loaded, generate the terrain and the sea again from the map definition, as the graphics apply does (R-META-427);
4. when the forwarded port, the hosting port or the joining port changed: write all three and re-open the client's listening port at once, also when this participant does not host;
5. when the language changed: store it, switch to it (R-META-466) and mark the windows for rebuilding;
6. write the automatic-camera setting;
7. write the large-menus setting, and mark the windows for rebuilding when it changed;
8. when marked, rebuild the windows (R-META-450);
9. save the store.

**R-META-449** (MUST) The other options page reads these settings and writes them back without showing a control:
1. **Control help**, default `ref.meta.settings.control_help.default` (on). It has no visible effect: the contextual control-help panel is a hook that matches never show (DEC-050).
2. **Difficulty**, stored as a level + 1 with the default `ref.meta.settings.difficulty.default` (1). At the front end the page reads the stored value − 1, at least 0; in a match it reads the current difficulty level. The apply action writes it only at the front end, storing level + 1 and setting the process difficulty level. The setting is inert: every map load sets the difficulty level to 0 and no map can carry one, so every match runs at level 0 (DEC-010). Species has no process-global difficulty.
3. **Automatic camera**, the tracking switch of [reference/06](../06_controls_camera_and_commands/camera.md) R-CTRL-480, with the choices and the default of `ref.ctrl.camera.automatic_setting`.
4. **Large menus**, default `ref.meta.settings.large_menus.default` (off). On, the small tool-style windows scale with the screen height by the large-menu factor of the [UI framework](../../architecture/components/ui_framework/layout_rendering_input_and_localisation.md), also at start-up; the full-screen options screens are laid out in screen fractions and do not change.
5. **Seasonal theme**, read but never written; it has no effect on the seasonal theme ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) R-META-114, DEC-101, DEC-118).

**R-META-450** (MUST) Rebuilding the windows after the other options apply closes every window; at the front end it builds the front-end screen again, and in a match it opens the match view again. The settings screen is not opened again.

### 2.7 Settings kept outside the options pages

**R-META-451** (MUST) The front end stores these settings:

| Setting | Default | Written by | Read by |
|---|---|---|---|
| player name | `ui.player.default_name` | every change of the name, from the name field or the chat rename command; every creation of the front-end screen; the name apply action ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) R-META-008, R-META-063) | the front-end screen when it is created; the default session name |
| preferred colour | `ref.meta.settings.preferred_colour.default` (none) | the colour picker of the player options page, except in Siege and in co-op | the match setup page when it assigns the local slot's colour |
| session name | the player name, or `ui.settings.default_session_name` when there is none | only an edit of the settings file | the advertised record of a hosted session (DEC-245) |

**R-META-452** (MUST) The session browser stores these settings, whose use is the browser's (DEC-245):
1. three filters with a control on the filter page, show incompatible sessions, show full sessions and show sessions with a password, and one filter with no control, show custom maps; each defaults to `ref.meta.settings.browser_filter.default` (on). The mode filter is not stored, so it resets every time;
2. up to `ref.meta.settings.manual_sessions` (30) manual session entries, numbered 1 upwards, each a host, a port and a name, which only an edit of the settings file can create. An entry without a port uses the hosting port (R-META-443).

**R-META-453** (MUST) The renderer reads these settings, which no page shows; their defaults, values and effects are the renderer's ([renderer options](../../architecture/components/renderer/options_and_budgets.md) RND-442): the tree detail `rnd.option.tree_detail.default`; special lighting `rnd.option.special_lighting.default`, which Launch Race and a map's special-lighting effect switch on regardless; the screenshot tile factor `rnd.option.poster_resolution.default`; the inverted ink-on-paper style `rnd.option.inverted_style.default` (DEC-040); and the Worker distance scale factor `rnd.option.worker_scale_factor.default`.

**R-META-454** (MUST) The audio engine reads these settings, which no page shows; their defaults and effects are the [audio engine](../../architecture/components/audio_engine.md)'s:
1. the sample-memory level: level 1 chooses among every sample of a sample group, level 2 among the first half and level 3 among the first quarter; it is honoured whenever the settings file holds it;
2. the effects switch, off by default, so no audio effect runs unless the settings file is edited (DEC-567);
3. the output buffer length of the software output path, which has no Species equivalent (DEC-100).

**R-META-455** (MUST) The input system reads these settings, which no page shows; their use is [input](../../architecture/components/input.md)'s and [reference/06](../06_controls_camera_and_commands/actions_and_bindings.md)'s:
1. the keyboard layout, by default the operating system's keyboard layout looked up in the layout table; it selects the per-layout binding override;
2. the mouse-button count, default `ref.meta.settings.mouse_buttons.default` (3), which the client reads when it sends the team controls;
3. the selection lock: when it is set and off, a new object cannot be selected while something is selected; when it is missing, which is the default, selection is not restricted;
4. any setting named by a binding: a binding can test whether a named setting is above 0; no default binding does.

**R-META-456** (MUST) These stored settings have no effect, and Species specifies none of them: the terrain detail (RND-443); the pixel effect (RND-443); the overscan value (R-META-424); the vertical-sync request, which the display driver overrides ([reference/12](../12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-575); a fog switch; the seasonal setting (R-META-449 item 5); control help (R-META-449 item 1); the difficulty (R-META-449 item 2); a control-method choice whose two choices bind the chat log to the same key; a network address, a server port and a network-bypass flag that nothing reads (DEC-250); the value of the start-up mini-programs (DEC-054); a text speed of a feature that the library does not specify; and the profile, profile-unlock and mod settings (DEC-061).

**R-META-457** (MUST) These development settings are hidden from players: the language test (R-META-473); the command-history record and replay, which runs the match at a fixed step (DEC-046; [reference/12](../12_simulation_model/07_randomness_and_integrity.md) R-SIM-740); and the bot-information overlay with its team choice ([diagnostics](../../architecture/components/diagnostics_test_and_performance.md)).

### 2.8 Options the reference does not have

**R-META-458** (MUST) Until DEC-044 is decided, the option set is the reference set of §2.3 to §2.7, plus the bus volumes of DEC-570 (R-META-434). The reference has none of these options:
1. anti-aliasing: no setting is read;
2. gamma or brightness;
3. mouse inversion or an inverted vertical axis: inversion exists only in the binding syntax, and no default binding uses it;
4. automatic zoom: the nearest setting is the automatic camera (R-META-449 item 3);
5. a vertical-sync control: the stored request has no effect (R-META-456);
6. key rebinding: the rebinding page is unreachable (DEC-050);
7. separate music, effects and voice volumes (DEC-570 adds the bus volumes);
8. subtitles, a text size or colour-blind options: the only text-scale switch is large menus, which the full-screen pages ignore (R-META-449 item 4);
9. mouse sensitivity or camera speed: the camera speeds are binding and tuning values of [reference/06](../06_controls_camera_and_commands/camera.md);
10. network rate or bandwidth settings: only the ports and the forwarded port exist;
11. profile selection or save slots.

**R-META-459** (MUST) The bot difficulty is not a setting: it is a per-match option of the match setup page ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) §2.19).

