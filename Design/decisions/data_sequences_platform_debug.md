# Decision register: data formats, sequences, platform and diagnostics

> Status: Reference decisions · Range: DEC-800 to DEC-849 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the decisions on data schemas and content writers, camera sequences and tutorials, the platform layer, the build, and diagnostics, test and performance that no other register file holds. Most decisions of this scope exist already, and this file cites them instead of repeating them: build-time validation, the data representation of actors and match options, the map asset, the preference defaults, the camera corrections, tutorials, cheats and the soak test are in `decisions/verdicts_and_fixes.md`; the numeric parsing semantics, the settings file format, content and version identity, frame pacing and catch-up are design-phase topics under `decisions/deferred/`; the open questions on data formats, tutorials, the platform layer and diagnostics are in `decisions/open_questions.md`; engine-technology choices (schemas and authoring syntax, validators, version identity, presentation and frame cap) are ADRs under `ADR/`. The entries below add the remaining rules: lossless and atomic writers, duplicate keys and length limits in authoring files, maps that match setup does not list, camera animations without content, and the rules of the soak-test sequence. Each entry states the behaviour that the reference documents specify now. A **Resolved** entry states the corrected rule that the documents apply; an **Open** entry states the faithful reference behaviour as the default, with a recommendation.

## Decisions of this scope recorded elsewhere

| Topic | Decision |
|---|---|
| Content is validated at build time; a missing token, an unterminated block, a stray blank line or a missing asset is a build error; no load path is fatal | DEC-015 |
| Records bind by field name and stable id, never by row or block order; named fields with explicit types, units, defaults and ranges and no sentinel values; one platform-independent number parser in the content build; UTF-8 authoring files compiled to versioned binary records; a map asset with a separately readable header; schema-defined settings defaults | ADR-061 |
| Authoring syntax and schema language | ADR-062 |
| Validators for schema errors, unknown fields, keys that differ only in case, missing references and per-mode map rules, each naming file, line and field | ADR-059 |
| Numeric semantics of simulation data, including how simulation numbers are parsed | DEC-241 |
| Actor kinds are typed schema entries, not row positions; actor stats are floating-point values | DEC-019, DEC-090 |
| Typed match options with explicit per-mode defaults | DEC-073 |
| Map editor and the versioned, schema-validated map asset | DEC-045 |
| One defaults table for preferences, available before any pack file is mounted | DEC-100 |
| Settings file format, schema version and migration | DEC-250 |
| Camera moves land exactly on their target view; all camera motion uses one presentation clock | DEC-024, ADR-036 |
| Tutorials as step graphs keyed by object tags, with world-changing effects as sequenced simulation events | DEC-103, DEC-051 |
| Second tutorial start condition | DEC-110 |
| Cheats as sequenced debug commands in cheat-flagged sessions; a headless soak test | DEC-041, DEC-202 |
| Present modes, frame-rate cap and frame pacing | ADR-012 |
| One generated build identity | ADR-057, with compatibility checks in DEC-204 |
| Catch-up bounded per frame and shown as a catching-up state | DEC-225 |
| Performance envelope at the population cap and the per-tick budget | DEC-106, DEC-228 |
| Engine gap rules of data, platform and diagnostics: content-build validation of terrain, map and record data, co-op group order, the system summary and crash records, the reference test machine and the cheats-used marker | DEC-968 to DEC-972 |

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-800](#dec-800-content-and-settings-writers-are-lossless-typed-and-atomic) | Content and settings writers are lossless, typed and atomic | Resolved | DAT, R-MAP, PLT, BLD |
| [DEC-801](#dec-801-duplicate-keys-repeated-fields-and-over-long-values-are-build-errors) | Duplicate keys, repeated fields and over-long values are build errors | Resolved | DAT, BLD, R-MAP, AEN, UI |
| [DEC-802](#dec-802-maps-that-match-setup-does-not-list) | Maps that match setup does not list | Open | R-MAP, R-META, DAT, BLD, DBG |
| [DEC-803](#dec-803-camera-animations-without-content) | Camera animations without content | Open | CAM, R-MAP, R-TUT, DAT |
| [DEC-804](#dec-804-soak-test-sequence-and-timing-rules) | Soak-test sequence and timing rules | Resolved | DBG, BLD |

## 1 Data schemas and content writers

### DEC-800 Content and settings writers are lossless, typed and atomic
- Status: Resolved
- Affects: architecture/components/data_schemas/README.md and architecture/components/data_schemas/config_input_and_strings.md (DAT), reference/08_maps_and_level_design/map_editor_hooks.md (R-MAP), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META-404, R-META-407), architecture/components/platform_layer.md (PLT), architecture/components/build_tooling_and_distribution.md (BLD); DEC-045, DEC-100, DEC-250
- Specified behaviour: These rules apply to every Species program that writes content or settings: a map editor (DEC-045), a schema migration step of the content build (ADR-061) and the settings store (`SettingsStore`).
  1. **Lossless round trip.** Loading a file and saving it again reproduces every value exactly. A number is written in the shortest decimal form that parses back to the same value, never at a fixed number of decimals. A save writes every record that the loaded data holds, including camera views and camera animations (DEC-803), instant units and every per-map override. A field is omitted only when its value equals the schema default, so that a load reads back the same value; a zero is never dropped where the default is not zero.
  2. **Canonical output.** Each field is written once, in schema order, with a separator between every two values, so that saving unchanged data gives the same canonical content hash (ADR-060).
  3. **Atomic replacement.** A writer writes a complete new file beside the old one, flushes it and then replaces the old file in one step. A failed write leaves the previous file intact and is reported to the player or author and in the log; it is never skipped silently.
  4. **Values are data.** A stored text value is written literally. No character in it, a percent sign included, is ever interpreted as a formatting directive.
  5. **Typed settings.** Each settings entry has the type, range and default that the settings schema declares (DEC-100). A value's type comes from the schema, never from the shape of its text, and an entry is read back with the type it was written with. On load, an entry that does not parse, or whose value lies outside its type or range, is reported in the log and takes its default, and every other entry keeps its stored value. The settings file carries the schema version and migration of DEC-250.
- Options: A — the rules above; B — the reference writers. The map writer keeps structure positions and facings at two decimals, writes terrain-tile target heights with no decimals, writes two terrain-tile fields with no separator between them, so that they read back as two values only because the second field is 6 characters wide and its value leaves leading spaces, drops zero values and writes no camera animations. The settings writer rounds decimal values to two places; it re-emits the blank and comment lines of the loaded file, which serves as its template. It glues an entry that has no space before its `=` to the next line, so the next key is lost on the following load, and it creates an empty entry when two spaces precede the `=`. It passes stored values through a formatting function, writes the file in place, and returns silently when it cannot open the file. The settings reader types each value by the shape of its text: a leading digit or minus sign with no point gives an integer, with one point a decimal, and anything else a string. A lookup is type-strict, so a decimal setting stored as a whole number reads back as the caller's default. A settings file whose first key holds a character outside the code-point range from `A` to `z` (65 to 122), a digit for example, is discarded and rewritten with defaults.
- Recommendation: A. A writer that quantises values moves structures the first time a map is saved and changes the map's content hash (DEC-204); a settings writer that loses keys or interprets values corrupts player data. Both are integrity defects. Lossless, canonical writing also makes a schema migration of the content repository safe. The settings file format itself (versioning, migration and synchronisation) remains open in DEC-250.

### DEC-801 Duplicate keys, repeated fields and over-long values are build errors
- Status: Resolved
- Affects: architecture/components/data_schemas/README.md (DAT), architecture/components/build_tooling_and_distribution.md (BLD), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/01_skirmish_rules_and_modes/README.md (R-MODE-084), architecture/components/audio_engine/ (AEN), architecture/components/ui_framework/layout_rendering_input_and_localisation.md and architecture/components/ui_framework/pages_and_screen_recipes.md (UI-372)
- Specified behaviour: The content build (ADR-059) applies these rules to every authoring file, in addition to schema validation:
  1. A key that appears twice in one object is a build error that names both locations. The authoring parser detects it itself; no parser mode that keeps the first or the last occurrence is used.
  2. Two records of one record type with the same stable id are a build error, within one file and across the files of a content set. This covers two maps with the same map id, two string-table entries with the same key in one language, and two sample groups with the same name.
  3. A field holds one value. A list is an explicit array and is never assembled from repeated keys.
  4. A value that deliberately replaces another is an explicit, typed override record, such as a per-map override of a match option (DEC-065), never a repeated key.
  5. Every text field and every id has a maximum length in its schema, which the validator enforces with file, line and field. No runtime structure holds content text in a fixed-size buffer, and the build sets no limit on line length or file size other than these declared maximums.
- Options: A — the rules above; B — the reference rules. Within a block, a repeated single-value keyword keeps its last occurrence and a list keyword accumulates. A duplicate key keeps the first occurrence in the settings file and the last in a string table, and is fatal in the map index. Duplicate sample groups are both kept and the first is used. A name longer than 255 characters, a mode with more than 64 options and a resource path longer than 250 characters corrupt memory, and a line of 65 536 characters or more ends the program.
- Recommendation: A. A repeated key is almost always an editing mistake whose effect depends on the parser, and common JSON parsers keep the last occurrence without a warning, so the rule must be explicit and each case needs a failing fixture in the content build's tests. Declared maximum lengths turn these overruns into build errors with a location.

## 2 Maps and match setup

### DEC-802 Maps that match setup does not list
- Status: Open
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), architecture/components/data_schemas/map_schema.md (DAT), architecture/components/build_tooling_and_distribution.md (BLD), architecture/components/diagnostics_test_and_performance/ (DBG)
- Specified behaviour: A map's header lists the modes the map supports, and match setup offers a map only under the modes it lists. A map whose mode list is empty is still a valid, loadable map: it appears in no match-setup list and is reached only through test and tool paths. The reference uses such maps for stress, benchmark and sandbox play; those maps are not reproduced (DEC-063), and Species test maps are authored new (the stress map of DEC-228 is one). The other content that the reference loader tolerates without a message is a build error in Species: an unknown mode name, which the reference ignores; an unknown structure kind, or a structure or actor kind that no match creates (DEC-052, DEC-062), which the reference skips; surplus values, unknown fields and stray lines, which the reference ignores (ADR-059); and a repeated option, of which the reference keeps the last (DEC-801). A map header names the map's display name by a string id that must exist in the string tables (ADR-061, ADR-059), so no map shows a placeholder name.
- Options: A — the reference: listing follows from the mode list alone, and an empty mode list hides a map; B — an explicit visibility field in the map header with the values listed and test: a listed map names at least one mode and passes the per-mode map rules for every mode it names (ADR-059), a test map may name none, and the edition manifests exclude test maps from player editions (ADR-056); C — no hidden maps: every map names at least one mode, and test maps live only in a test content set that is never packed for players.
- Recommendation: B. An explicit field states the intent, so a listed map that loses its last mode by mistake fails the build instead of disappearing from match setup, while the test, stress and benchmark maps that the soak test (DEC-804) and the per-tick budget (DEC-228) need stay loadable in development builds.
- Decide by: design phase

## 3 Camera sequences and tutorials

### DEC-803 Camera animations without content
- Status: Open
- Affects: architecture/components/camera_and_sequences.md (CAM-053 to CAM-058, CAM-065 to CAM-071, CAM-080 to CAM-084), reference/06_controls_camera_and_commands/camera.md (R-CTRL-457), reference/08_maps_and_level_design/map_editor_hooks.md (R-MAP-309, R-MAP-371), reference/11_tutorial_and_help.md (R-TUT), architecture/components/data_schemas/map_schema.md (DAT); DEC-024, DEC-051
- Specified behaviour: A map can hold camera animations. Each is a named list of nodes, and each node names a camera view, a transition (move or cut) and a duration. The camera can play one animation at a time. Playback first saves the current camera mode and pose. For a move node the camera moves to the node's camera view over the node's duration; for a cut node it holds its current pose for the node's duration and then cuts to the node's view. When the last node has ended, the camera restores the saved mode; a stop request restores it at once. An animation with no nodes is not played, and a sequence that waits for the camera waits until the animation and any move still under way have ended. Moves land exactly on their view and run on the presentation clock (DEC-024). Every camera view that an animation names is a validated reference (ADR-059). No match path plays a camera animation, so the capability exists in the camera component and in the map format with no content that uses it. Three further camera capabilities have no match path and are kept the same way: the structure focus, an orbit around a structure, and the approach move are timeline-only operations, which only a presentation timeline (DEC-051) reaches, so the live player-camera modes of R-CTRL-457 are unchanged (CAM-053 to CAM-058); and the developer fly mode exists only in Debug builds and in the map editor, whose session runs the camera in it, as a developer tool and not a player-camera mode (CAM-080 to CAM-084, R-MAP-309).
- Options: A — the reference: keep the playback capability, the structure focus and the approach move in the camera component, the camera-animation record in the map format and the developer fly mode as a developer tool, with no player content that uses them; B — remove camera animations, the structure focus and the approach until the presentation timeline of DEC-051 is adopted, and specify them then as part of that timeline, keeping the developer fly mode; C — give camera animations a use in matches, such as a camera sweep at match start before the cut to the player's view.
- Recommendation: A while DEC-051 is open. The playback state machine, the focus orbit and the approach are small, the preferred option of DEC-051 needs exactly these camera moves, and the map editor needs the fly mode; with no content authored, their cost stays within the camera component and one map record type. If the design phase drops tutorials and presentation timelines, choose B. C changes the match-start camera sequence of DEC-024 and is a design-phase choice.
- Decide by: design phase

## 4 Diagnostics and test

### DEC-804 Soak-test sequence and timing rules
- Status: Resolved
- Affects: architecture/components/diagnostics_test_and_performance/ (DBG), architecture/components/build_tooling_and_distribution.md (BLD); DEC-041
- Specified behaviour: The headless soak test of DEC-041 runs a declared sequence of matches, each entry naming a map, a mode, the teams and a seed. It follows these rules:
  1. Each entry names its map by stable map id, and the map is checked against its content hash in the content manifest (ADR-060) before the match runs. No entry selects a map through an index into a separate identity table.
  2. A pass runs every entry exactly once, from the first entry to the last, in the declared order or in an order derived from a logged seed. The sequence never skips an entry and never reads past its end.
  3. Where the soak test drives the user interface, it addresses each element by an id that is unique within its page, and a duplicate id fails the test run.
  4. Delays and timeouts are counted in ticks, or in integer units of the platform clock (ADR-074); they are never accumulated from frame-time differences in single precision.
  5. Soak-test state belongs to the test run, which creates and resets it. No sequence state lives in a global variable or in static storage of a user-interface module.
  6. Each match logs its seed, map id, content hash and build identity (ADR-057), so that a failing match can be replayed.
- Options: A — the rules above; B — the reference test bed, which selects maps through a wrong index into the map identity table, skips the first entry and reads past the last one in its linear order, drives buttons by names that are not unique, measures delays as single-precision differences of a high-resolution clock, and keeps its sequence in global and static state.
- Recommendation: A. A soak test is useful only if every entry runs the intended map and every failure can be reproduced, and each reference defect breaks one of these. The design itself (scripted match sequences and a per-match log keyed by the seed) is kept.
