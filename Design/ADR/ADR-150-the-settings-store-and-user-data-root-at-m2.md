---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T15)
---

# The settings store and the user-data root at M2

## Context and Problem Statement

M2's T15 asks for the settings store, with the display options and DEC-964's reduced settings read from it in place of named constants ([M2 plan](../implementation/m2_plan.md) table 2; [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md)). Its run checklist asks that a changed option survive a restart. Until now:
- The display options were constants: the window mode, the frame-rate cap, low latency, the present mode, the scene's samples (`PREFERRED_SCENE_SAMPLES`), the sea and entity detail, the Worker distance scale factor and the brightness.
- DEC-964's reduced-flash and reduced-motion scales were not applied (ADR-123).
- No user-data root existed: ADR-137 moved the saves folder to T18, and the logs sit beside the executable (ADR-092).

The platform layer specifies the store (PLT-030 to PLT-034, PLT-079 to PLT-082, PLT-089) and the root (PLT-016 to PLT-018, PLT-025), and the data schemas specify the file (DAT-193 to DAT-201) and its table (table 3.1-A). Five points are left to decide:
1. which settings the M2 client stores;
2. the ranges no requirement states;
3. the root while no publisher exists (PLT-016's `<Studio>`, O49);
4. where a save is written;
5. how the engine and the game share the store.

## Decision Drivers

* PLT-030: one store, created after the start-up options and the root and before the window, with its defaults compiled in (DEC-100), so a first run and a development tree start alike.
* DAT-200 and PLT-116: nothing in the file stops the reader, and a settings failure never stops the client.
* DAT-201: a save keeps the player's comments, unknown keys and order.
* R9: the engine reads no game setting by name.

## Considered Options

1. **The store in NeuronCore with the defaults table in the game**: the engine owns the file format, the store and the root; GameClient owns the table and maps settings onto the engine's configuration structures.
2. **The store and its table in GameClient**: the engine's renderer reads the store through game code.
3. **No store yet**: the options become presentation-record keys, which a rebuild of the content changes.

## Decision Outcome

Chosen option: 1.
- Option 2 puts a platform service in the game layer. The headless server and the tools would need GameClient to read a setting, and the renderer would know the game's keys.
- Option 3 fails the run checklist: a change made in the game would not survive a restart.

* **The root** (`NeuronCore/UserData`):
  - **Where.** `--user-data <directory>` when given, in every build (PLT-025). Otherwise the Local AppData known folder (`SHGetKnownFolderPath`, `FOLDERID_LocalAppData`) then `Species`, with no publisher's folder until O49 names one (DEC-1004).
  - **Proved.** The root is created and proved writable by writing and removing a probe file through `ReplaceFileAtomically`.
  - **Inside the install directory.** An option naming the executable's folder or one inside it ends the start-up with `ui.error.user_data_inside_install` (PLT-025). Paths are compared after normalization, without regard to case.
  - **Unavailable.** A root that cannot be resolved, created or written shows the notice `ui.error.user_data_unavailable` with the folder, and the settings live in memory (PLT-018).
  - **Folders.** `UserDataFolder` creates a folder of the root on first use (PLT-017): `settings\` now, the saved worlds' folder at T18.
* **The file** (`NeuronCore/SettingsFile`) is DAT-196 to DAT-201:
  - **Lines.** One line per entry `key = value`, comment or blank, in UTF-8 with line feeds.
  - **Reading leniently.** The reader accepts a byte-order mark, a carriage return before the line feed and blanks around `=`, so a file saved by Notepad reads whole. The writer spells table 3.2-A's form exactly.
  - **Malformed lines.** A line that is not UTF-8, has no `=`, has no valid key or is longer than 4,096 bytes is an "other" line: logged, kept and written back.
  - **Values.** Each is typed by its spec, never by its shape: `true` and `false`, a decimal integer with an optional minus, a decimal in its shortest round-trip form, a text literally, an enumerator by name, a binding as its canonical JSON text. One out of range or of another form takes its default and is logged.
  - **Keys.** The first occurrence of a key wins. A key the schema does not hold is kept with its text and written back unchanged.
  - **Saving.** Every comment, blank and other line stays in place; each loaded key is written once, at its first line, with its current value; then each key the file did not hold whose value differs from its default, in byte-wise order. The write goes through `ReplaceFileAtomically` (PLT-089).
* **The store** (`NeuronCore/SettingsStore`), PLT-030 to PLT-034:
  - **Specs.** It takes the specs and the file's path, or none for memory only. A spec whose key ends with `.` types a family, as `binding.` types the user layer's entries (INP-070).
  - **Reading.** `Get` returns an optional value; `GetBool`, `GetInteger`, `GetDecimal` and `GetText` read with the schema's type, and another type is a development check (PLT-033).
  - **Changing.** `Set` refuses a key the schema does not hold or a value outside its type, range or enumerators (DAT-197). It notifies the subscribers whose prefix matches, in subscription order, and nobody for an equal value (PLT-034). `Clear` makes a setting absent.
  - **Saving.** `Save` writes on the main thread at M2. The saves are rare (the first run's write-back now, the options page's apply at T18), and PLT-014's file writer thread arrives when the audio engine's volume saves make them frequent. A failed save keeps the previous file and is logged; `ui.dialog.settings_save_failed` waits for the UI's dialogs.
* **The client's table** (`GameClient/ClientSettings`, DAT-193) holds these settings, with their defaults compiled in. ADR-116 keeps the `rnd.option.*.default` keys as named constants, so they stay constants here.

  | Key | Type and range | Default |
  |---|---|---|
  | `render.window_mode` | `windowed`, `borderless_full_screen` | `borderless_full_screen` |
  | `render.render_width`, `render.render_height` | integer, 1 to 16,384 px | absent: the desktop size, written back at the first start |
  | `render.present_mode` | `vertical_sync`, `uncapped` | `vertical_sync` |
  | `render.frame_rate_cap` | integer, 0 to 1,000 frames a second; 0 is none | 0 |
  | `render.low_latency` | boolean | false |
  | `render.anti_aliasing` | `off`, `2x`, `4x` | `4x` |
  | `render.brightness` | decimal, 0.5 to 2 | 1 |
  | `render.sea_detail` | integer, 0 to 3 | 1 |
  | `render.entity_detail` | integer, 1 to 3 | 1 |
  | `render.worker_scale_factor` | decimal, 0 to 10 | 2 |
  | `interface.ui_scale`, `interface.text_scale` | decimal, 0.5 to 3 | absent: `ui.scale.ui_default` and `ui.scale.text_default` |
  | `interface.reduced_flash`, `interface.reduced_motion` | boolean | false (R-META-511) |
  | `audio.master_volume` | integer, 0 to 255 | 255, stored for T18's slider |
  | `binding.<action id>` | the canonical JSON of a binding node | absent (INP-070); T15's second step reads them |

  The ranges no requirement states are this ADR's:
  - the frame-rate cap's 1,000;
  - the render size's 16,384 px, D3D12's largest texture side;
  - the brightness's 0.5 to 2;
  - the Worker distance scale factor's 10;
  - the UI and text scales' 0.5 to 3, where UI-168 leaves them to the design phase.

  The enumerators' spellings are this ADR's too, since table 3.1-A names the values but not their text.
* **The readers** at the start:
  - **The window.** The stored mode and render size feed PLT-112's display attempts.
  - **The renderer.** `ApplyRenderSettings` fills the present mode, the cap, low latency and the preferred scene samples of `RenderConfig`. The samples are read when the device objects are created, so a change of anti-aliasing applies at the next start.
  - **The presentation.** `DisplayOptions` carries the rest:
    - the sea detail, when a world's surfaces are built;
    - the entity detail and the Worker distance scale factor, in the crowd view, the Leaders' figures and banners, and the Leaders' picking spheres;
    - the brightness, each frame;
    - the UI and text scales, in the UI's metrics and the badge's count;
    - the reduced settings, in the effects.
  - **DEC-964.** `interface.reduced_flash` multiplies the full-screen flash's alpha by `vfx.screen.reduced_flash_scale` before its onsets are counted (VFX-235). `interface.reduced_motion` multiplies the shake's offset, not the shake itself, by `vfx.screen.reduced_motion_scale` (VFX-232). Both keys are added to the presentation record at 0.25.
  - **The write-back** (PLT-080, R-META-424). The start writes the window mode and render size it started with back to the store, and saves on a first run or when they changed, for example after a fallback.
* **What waits:**
  - subscriptions that apply a change while running, with T18's options page;
  - the window's monitor and windowed rectangle (PLT-028);
  - the logs under the root: the logger starts before the command line is parsed, so they stay in ADR-092's folder beside the executable until PLT-038's start-up order is built;
  - the HDR settings, with HDR output (ADR-014).

### Consequences

* Good, because a setting changed in the file or, from T18, on the options page survives a restart, and a hand-edited file keeps its comments.
* Good, because the engine's renderer still takes plain configuration and knows no game key.
* Good, because the file and the store are portable C++ with their own tests; only the root's known folder and its install check are Windows code.
* Bad, because a save blocks the main thread for a file write, acceptable for one save at the start and one per options apply.
* Bad, because the ranges above are guesses the design has not made; a narrower range later turns a stored value into its default (DAT-198).
* It forecloses: settings read by the simulation (PLT-032), and a store with no file behind it at all.

### Confirmation

* NeuronCoreTests:
  - `SettingsStoreTests`:
    - an invalid entry, a repeated key, an unknown key and an other line;
    - a save that keeps every line in its place and adds the new keys in order;
    - the subscribers' order and silence for an equal value;
    - refusals;
    - memory only;
    - the values' text.
  - `StartupOptionsTests`: `--user-data` in every build.
* GameClientTests:
  - `ClientSettingsTests`: every default fits its spec, RND-442's defaults reach `RenderConfig` and `DisplayOptions`, and the write-back.
  - `EffectsLookTests`: DEC-964's two scales.
  - `CrowdLookTests`: RND-446's entity detail.
* The owner's T15 run:
  - a first start writes `settings\settings.txt` under `%LOCALAPPDATA%\Species\`;
  - a value edited there, such as `render.window_mode = windowed` or `interface.reduced_motion = true`, holds after a restart;
  - `--user-data` moves the file.

## Pros and Cons of the Options

### 1 — the store in NeuronCore with the defaults table in the game

* Good, because the platform service sits with the platform layer, and the game decides its settings.
* Bad, because each engine reader needs a mapping in the game, as `ApplyRenderSettings` is.

### 2 — the store and its table in GameClient

* Good, because one project holds everything about settings.
* Bad, because the engine cannot read a setting without the game.

### 3 — no store yet

* Good, because nothing new is built.
* Bad, because T15's checklist cannot pass.

## More Information

* Amends [ADR-116](ADR-116-presentation-tuning-record.md) and [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md); amends PLT-016 of the [platform layer](../architecture/components/platform_layer.md), noting there the M2 forms of PLT-025 and PLT-081 that this ADR states, and table 3.1-A of the [data schemas](../architecture/components/data_schemas/config_input_and_strings.md); opens DEC-1004.
