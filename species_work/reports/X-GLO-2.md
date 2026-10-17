# X-GLO-2 report: glossary M–Z

## Files written

| File | Size | Content |
|---|---|---|
| `design_next/glossary/m_to_z.md` | 70,730 bytes | 238 entries, letters M to Z (no entry starts with X or Y) |
| `species_work/scripts/X-GLO-2_decs.py` | helper | Prints status and specified behaviour of named DEC entries |
| `species_work/scripts/X-GLO-2_terms.py` | helper | Lists the CONVENTIONS §8 terms of a letter range that a file uses, and marks those with no glossary entry |
| `species_work/scripts/X-GLO-2_sort.py` | helper | Sorts the rows of each glossary section by the README order rule (stable) |
| `species_work/scripts/X-GLO-2_sweep.py` | helper | Reports original ledger names (renamed or dropped rows) found in a file, for manual review |

The format is the one fixed by X-GLO-1 in `glossary/README.md`: one table per letter, columns `Term | Kind | Definition | Owning document`, the four kinds, the word-by-word order rule (hyphen read as a space, bracketed qualifiers ignored), and bold letter links in the "Jump to" line. `glossary/README.md` already links `m_to_z.md`; it was not edited.

## Counts

- Requirements: 0. Tuning keys: 0. Diagrams: 0 (glossary; no ID range, no tuning table).
- Entries: 238 = 96 player concepts, 109 engine terms, 15 data terms, 18 abbreviations.
- Per section: M 30, N 3, O 9, P 30, Q 1, R 29, S 72, T 36, U 13, V 4, W 10, Z 1.
- Enumerations carried in definitions: the 6 modes in order; the 15 map options in order; the 5 map effects in order; the 13 lobby options of the match options; the 8 power-slot messages (6 in message order plus the 2 full-slot messages); the 5 team kinds; the 6 team slots and their roles; the 8 team colours; the 5 turret types; the 4 Power states; the 3 sound-parameter types; the 4 simulated-object handle fields; the 6 spider-gait parameters; the score-mode formulas (2 in Spawn Control, 4 in Zone Control); the research levels fixed at match start; the blocks of a map definition (10); the passes of the render pass table (10); the 3 live match-camera modes; the starting-force Power kinds (4).
- Owning documents linked: 56 distinct targets, all package targets in `species_work/packages/INDEX.md`.

## Checks

- `scan_patterns.py forbidden_patterns.tsv m_to_z.md`: no output.
- `scan_patterns.py soft_patterns.tsv m_to_z.md`: no output (two "port of" hits reworded).
- `X-GLO-1_check.py` on both glossary files: 0 problems (order per section, no duplicates, kinds, every link target an INDEX target or existing file, every DEC and ADR id exists, no truncation words, sizes under 120 KB); 418 entries in the two files together.
- Every M–Z term that X-GLO-1 listed as used by `a_to_l.md` has an entry, and every CONVENTIONS §8 term of M–Z that `a_to_l.md` uses has an entry (checked with `X-GLO-2_terms.py`).
- Every handed-over row of X-GLO-1 cross-document request 1 has an entry, including "previous camera view", kept because DEC-803 specifies camera animations as a capability.
- `X-GLO-2_sweep.py`: the only hits are plain English words that the ledger itself keeps as Species terms (crate, camera view, laser bolt, snow, lightning, waypoint, editor); no original name, class, file, string key or UI text appears.

## DECs and ADRs cited

- Open (42): DEC-003, DEC-043, DEC-045, DEC-046, DEC-047, DEC-048, DEC-049, DEC-051, DEC-066, DEC-079, DEC-089, DEC-101, DEC-107, DEC-110, DEC-115, DEC-116, DEC-203, DEC-204, DEC-208, DEC-222, DEC-227, DEC-242, DEC-243, DEC-244, DEC-245, DEC-246, DEC-300, DEC-301, DEC-302, DEC-303, DEC-304, DEC-322, DEC-326, DEC-450, DEC-453, DEC-454, DEC-462, DEC-551, DEC-556, DEC-562, DEC-651, DEC-803.
- Resolved (38), stated as the fixed rule: DEC-002, DEC-007, DEC-008, DEC-009, DEC-010, DEC-011, DEC-013, DEC-017, DEC-024, DEC-028, DEC-030, DEC-031, DEC-032, DEC-033, DEC-036, DEC-038, DEC-039, DEC-041, DEC-064, DEC-065, DEC-073, DEC-074, DEC-078, DEC-082, DEC-083, DEC-086, DEC-095, DEC-099, DEC-100, DEC-103, DEC-118, DEC-456, DEC-464, DEC-560, DEC-563, DEC-568, DEC-800, DEC-804.
- Dropped (3): DEC-057, DEC-058, DEC-063.
- ADRs (20): ADR-0003, ADR-0004, ADR-0005, ADR-0010, ADR-0015, ADR-0019, ADR-0020, ADR-0021, ADR-0023, ADR-0025, ADR-0030, ADR-0035, ADR-0051, ADR-0058, ADR-0059, ADR-0060, ADR-0063, ADR-0071, ADR-0074, ADR-0076.
- `decs_to_cite` was empty.

## New DEC requests

None.

## ADR conflicts

None in `m_to_z.md`. One tension for C-AEN to settle: ADR-0025 holds short sounds as resident 16-bit PCM banks that load and release with the map or front end, while the reference sample cache decodes on first play and evicts least-recently-used samples above 60 MiB. The "sample cache" entry states both (the ADR rule first, the reference behaviour as "the reference"); the `a_to_l.md` "LRU" entry states only the reference rule. Once `audio_engine.md` exists, both entries may need a one-line update.

## Missing assets

None; the glossary cites no asset ids.

## Cross-document requests

1. **To the coordinator (for `a_to_l.md`; A–L terms my source range yields that X-GLO-1 lacks).** I did not edit `a_to_l.md`.
   - *extrapolation time*: the Species name of the render-time extrapolation of positions since the last tick (from the M–Z source row on prediction time); used by `reference/12_simulation_model/01_tick_timing_and_main_loop.md`. Proposal: engine term, "The time by which every renderer extrapolates object positions past the last tick: 0 while the world is held, otherwise the real time since the tick was taken, capped at 1.0 s, × game speed; it is extrapolation, not interpolation."
   - *in-match loop*: the per-frame loop of a match (from the M–Z source row on main loops); my "main loop" entry uses it.
   - *camera animation*: a named list of camera-view nodes (move or cut, with a duration), kept as a capability by DEC-803; my "map definition" and "previous camera view" entries use it.
   - *Essence-orb rain* and *dust wind*: two of the five map effects (my "map effect" entry; DEC-562 uses "Essence-orb rain").
   - *demotion*: folded into my "promotion to Leader" entry (DEC-086); an own entry is optional.
   - *glTF*: abbreviation used in my "mesh" entry (ADR-0019); an own entry is optional.
2. **To the coordinator (`a_to_l.md` "dt" wording).** `a_to_l.md` defines dt as "the tick period, 0.1 game seconds × game speed". `reference/12_simulation_model/01_tick_timing_and_main_loop.md` and DEC-227 make the tick period 0.1 s (`ref.sim.tick_period`) and advance match time by tick period × game speed. My "tick period" and "tick" entries follow reference/12; proposal for dt: "the game time that one tick advances: the tick period × game speed".
3. **To G05, G08a and C-DAT (starting units and starting forces).** The glossary now separates the two: *starting forces* are the map's starting-unit records (X-GLO-1's name proposal); *starting units* are everything present at match start, starting forces plus port seeding (the term G05 uses). Documents should use them in these meanings.
4. **To G08a and C-DAT (map option and lobby option names).** The glossary gives Species names to the 15 map options (no Catch-up bonus, instant Spawn Node capture, Transit Gate reinforcement count, Transit Gate Transport cap, no gun turrets, attacker reinforcement count, defender reinforcement count, defender reinforcement power, attacker reinforcement power, same-team Beam Dish links, no Beam Dish aiming, airstrike threshold, Visitor-faction position, attacker reinforcement power option, defender reinforcement power option) and to the lobby options (score mode, time limit, reinforcement interval, reinforcement count, Transport cap, turret frequency, turret cap, starting powers). The owning documents should use the same names, or tell the coordinator theirs so the glossary can follow.
5. **To the coordinator (paths, same as X-GLO-1 request 5).** `m_to_z.md` links `reference/06_controls_camera_and_commands.md` 20 times and `architecture/components/data_schemas.md` 11 times; if G06 or C-DAT turns its target into a folder, these links need updating.
6. **To the coordinator ("teleport").** CONVENTIONS §8.12 names the term "teleport" (`TeleportStructure`), and `a_to_l.md` writes "teleport structures". The entry is "teleport"; either wording finds it.
7. **To the coordinator ("options screen" owner).** X-GLO-1 pointed it at the UI framework folder; I linked `architecture/components/ui_framework/core_and_widgets.md`, because the ORIG range for the full-screen page style lies in a C-UI-a input range (front-end widget family). C-UI-a may place it elsewhere.

## Names needed that are not in the ledger

Generic, non-IP terms used as entry names but absent from the ledger and from CONVENTIONS §8. Proposal: accept them as generic terms, or list them in CONVENTIONS §8 as generic terms (as for X-GLO-1's list):
main loop, match, match setup, mode second, music channel, network statistics window, no team, one-minute warning, operator, outside height, page, passengers, pause, player command, power-slot message, presentation generator, rank announcement, ready (ready-up), reinforcement count, reinforcement interval, reinforcement wave, replay, results screen, route, score, score mode, server browser, settings file, simulation generator, simulation state, soak test, start position, starting powers, starting units, string id, team kind, team slot, tick, time limit, time-up, Transport cap, turret cap, turret frequency, tutorial, ungrouped actors.
- "starting forces": X-GLO-1's proposal (identifier `StartingForces`, string-id prefix `map.starting_forces`), used here.
- "Essence-orb rain": proposal for the fifth map effect (identifier `MapEffect::EssenceOrbRain`); DEC-562 already uses the words.
- The 15 map-option names of cross-document request 4: proposal to add them to CONVENTIONS §8 as `MapOption::<PascalCase name>`.
- "presentation generator" and "simulation generator" are the names of `reference/12_simulation_model/07_randomness_and_integrity.md`; CONVENTIONS §8.12 lists only their draw forms.

## Dropped as out of scope (one line per group)

- Provenance vocabulary: the analysis-agent identifiers, the open-question identifier pattern and the unknown-evidence tag.
- Brand and IP rows: the product name and its abbreviation prefix, map display names, the premium map packs, the font names, the internal codenames, the rating-system trademark (renamed Skill rating), the third-party arcade and literary names (renamed Bomber and Spitter Plant), the console-edition remnants, and the distributor origin and version files.
- Superseded technology: the graphics-API emulation layer, display lists and the vertex-buffer abbreviation, the texture-filter shorthand, non-power-of-two texture handling, the alternative graphics build configuration and its switches, the deform capture switch, the window-manager back ends, the sound device back ends, the reference's compressed sample format (Species uses WAV sources, resident PCM and streamed Opus, ADR-0025), the reference gamepad API (Species uses GameInput, ADR-0028), virtual-key codes (input-stack detail, DEC-338), the own structured-exception crash handler (Species uses Windows Error Reporting, ADR-0065), the SIMD note, the old compiler and IDE versions, the installer generators, the archive format and its library, the shared utility library and its container sub-library, the text-reader and wide-string classes, the hash function taken from the archive library, the generator-algorithm name (the algorithm is open, DEC-242), and the exclusive-or abbreviation (no Species use).
- Networking, sessions and online services (design-phase topics): the matchmaker and address translation, the directory service and its message of the day, the network library and its thread wrappers, the lobby value-binding class, transport protocols and ports, round-trip time, time to live, sequence ids, the server class and its letters, the per-tick update letter (folded into "player command"), the start-game letter, the sync byte, the session lifetime class, the studio endpoint URLs and the unreachable quick-match page.
- Campaign, blocked and dead content: the mine-cart, receiver and upgrade-port families, the stored and unprocessed orb objects, the safe area, scripts and script triggers, the power station and the campaign surge emitter, the single-player order visualiser, the vehicle and its mode (DEC-058), the walker and its navigator, the summoner, the debug-only rift outcome with its portal and tentacles (DEC-057), the intro and trial-end sequences, the mentor dialogue system, running programs, the pixel effect, the missile with no live caller, the campaign test harness, the test-bed configuration (replaced by the soak test, DEC-804), purity control, and the profiler reachable only from the cheat window (DEC-041).
- Authoring and packaging leftovers: image-editor templates and gradients, 3-D scene sources and the mesh exporter plug-in, the text mesh format (replaced by "mesh", ADR-0019), the thumbnail, help-image and icon naming conventions (new art), the build-time pack lists, the permitted-maps whitelist, the product-target, configuration and debug macros (sound editor, sync tracking, test bed, crash-report and debug-log switches), retail editions, and the custom-map container (map editor hooks, DEC-045).
- Folded into another entry rather than dropped: the team's loose entity list ("ungrouped actors"), the map identity tool ("map id"), the option blueprint file ("match options"), the preferences file ("settings file"), the sound back-end voice channel ("sound voice"), the slice ("update slices per tick" and "sliced pool"), the neutral team value ("no team"), the pseudo-unit container codes ("simulated-object handle"), the server history recording ("replay"), the master-spawn-point counts per map ("Source Node", without the counts), the multiplayer and single-player abbreviation (every match uses the same structure), the new-game platform target (the ADRs cover it), and the simulated-versus-cosmetic boundary ("simulation state").
