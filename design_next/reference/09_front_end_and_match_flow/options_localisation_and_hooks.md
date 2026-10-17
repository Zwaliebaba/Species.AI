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

