# C-DAT report: data schemas

Package `species_work/packages/C-DAT.json`, range DAT-001 to DAT-399. Target folder `design_next/architecture/components/data_schemas/` (component template §6.4, with §9 tuning table as in the sibling component folders). All 39 input ranges (KEEP, MERGE, KEEP-HOOKS) were read at their line numbers and rewritten.

## Files written

| File | Bytes | Requirements | Holds |
|---|---|---|---|
| `README.md` | 56,092 | DAT-001 to DAT-053 (53) | responsibilities, record-type registry (table 1-A), schema language and annotations, logical types, the limit form (DEC-120), presence and defaults (DEC-567), cooked header, versioning and migration (ADR-0061), strict parser, validation order and classes (DEC-801), canonical writer (DEC-800), loaders, identity, diagnostics, shared tests, open decisions, `dat.*` keys |
| `map_schema.md` | 48,962 | DAT-070 to DAT-110 (41) | map definition blocks, header, 15 map options, 5 map effects, terrain records, lights, start positions (DEC-560), camera views and animations, starting forces, routes, primary objectives, baked tree crowns, map index, mode catalogue and match-option schema, custom-map container and achievement hooks, map validation (DEC-561) |
| `map_structure_records.md` | 28,550 | DAT-140 to DAT-161 (22) | base fields and the fields of the 32 placeable structure kinds, typed links (DEC-457), spawn-network tree (DEC-456) |
| `config_input_and_strings.md` | 42,085 | DAT-190 to DAT-218 (29) | settings schema (one entry per row of the R-META §4.2 register plus RND, AEN, DBG and PLT settings), settings-file text form and reader/writer rules, binding-set and keyboard-layout schemas, string tables, variants and caption markup |
| `entity_audio_geometry_and_panel_schemas.md` | 36,951 | DAT-260 to DAT-290 (31) | actor stat table, Supply-crate outcome catalogue, sound sources/events/layers, modulatable values, effect bindings, effect-kind table, sample-group weights, cooked mesh/collision/sim records, colour-ramp row convention, screenshot output, explainer animations |
| `import_and_build_records.md` | 18,364 | DAT-350 to DAT-361 (12) | texture, mesh and audio import records, engine asset table, edition manifests (C-BLD request applied: fields of BLD table 3.2-A, flags of BLD table 3.2-B, groups `all`/`efigs`) |
| `test_and_telemetry_records.md` | 25,008 | DAT-370 to DAT-386 (17) | soak configuration (`test.soak_configuration`), soak report (`test.soak_report`: RFC 4180 match table with the DBG-166 columns plus a JSON pass summary), telemetry session record (`telemetry.session`: exactly the DBG-181 data set) — C-DBG request (d) |

Self-check `species_work/scripts/C-DAT_check.py` passes (ids unique, ascending, in range; every cited DAT, DEC, ADR, asset id, requirement id and tuning key exists; Open `decs_to_cite` cited by requirements; template headings; no truncation words; links resolve or are planned). Both pattern scans print nothing. A dry run of `check_design.py` (mermaid skipped; the 9 blocks parse with mermaid 11 separately) shows no gate failure in the folder except gate 7 below. Helper `species_work/scripts/C-DAT_renumber.py` renumbered ids contiguously per file.

## Counts

- Requirements: 205.
- Tuning keys owned: 11 (`dat.id.max_chars`, `dat.text.max_chars`, `dat.notes.max_chars`, `dat.schema.draft`, `dat.schema.initial_version`, `dat.parse.max_depth`, `dat.writer.indent_spaces`, `dat.perf.catalogue_ms`, `dat.perf.records_ms`, `dat.perf.full_build_s`, `dat.perf.incremental_s`). Every field default cites its owner's key; no duplicate keys defined.
- Diagrams: 9 Mermaid.
- Captioned tables: 46. Enumerations stated in full: about 50 closed value sets (map options, effects, structure kinds, turret types, barrier modes, actor kinds, sound positions/sharing/loop/audience/bus/links/update, 14 effect kinds with 63 parameter names, texture kinds, alpha modes, samplers, derivations, feature flags, diagnostic classes, logical types, caption markers and others, each listed member by member).

## Open DECs cited by requirements

DEC-045, DEC-204, DEC-207, DEC-223, DEC-240, DEC-241, DEC-246, DEC-250, DEC-251, DEC-309, DEC-462, DEC-562, DEC-567, DEC-750, DEC-802, DEC-803 (all Open entries of `decs_to_cite`), and also DEC-047, DEC-089, DEC-243, DEC-245, DEC-247, DEC-317, DEC-330, DEC-452. Resolved and stated as fixed rules: DEC-012, DEC-015, DEC-019, DEC-073, DEC-090, DEC-100, DEC-120, DEC-456, DEC-457, DEC-560, DEC-561, DEC-563, DEC-800, DEC-801 (with one failing fixture per duplicate case, X-DEC-5c request 1), and others listed in each §8.

## New DEC requests (candidates; defaults already written)

1. **Relay-only Barrier Emitter ring** (DAT-150): a ring whose every emitter starts in `relay_only` forwards a request without end; written as a build error (DEC-014 spirit). Other chain loops are accepted. Confirm or record.
2. **Co-op group order** (DAT-088): derived group lists are in ascending position order; the reference keeps the authored list order, which alliance assignment reads as G[0], G[1] (R-MODE-020). Groups limited to at most 2 positions (DAT-087). Proposal: record as a minor Open decision.
3. **Settings file text form** (DAT-196): `key = value` lines, comments and blanks kept, until DEC-250 decides; no schema version (R-META-409). Proposal: fold into DEC-250.
4. **Hive ants-inside out of range** (DAT-159): build error instead of the run-time replacement by 0 of R-BLDG-633.
5. **Spitter Plant reload lower bound** (table 3.2-A): `⌊reload ÷ 10⌋ ≥ 1`, so the first-deadline draw of R-BLDG-645 has a value.
6. **Zero facing** (table 3.1-A of structures): accepted and normalised by the DEC-241 degenerate rule rather than rejected; confirm.

## ADR conflicts

1. **ADR-0061** says simulation values are "stored at full precision" until the numeric model is chosen; DEC-241's default and TER-030 round simulation decimals once to binary32 at cook time. DAT-010 follows DEC-241/TER (binary32-rounded value held as binary64; fields marked binary64 keep full precision). Proposal: reword ADR-0061 to "stored in the representation of the current numeric default (DEC-241)".
2. **ADR-0062** (Proposed) leaves the draft and the libraries to DAT. DAT pins `dat.schema.draft` (2020-12) and chooses a Species-owned strict parser and generated validator (needed for DEC-801's duplicate detection and per-value locations), with `std::from_chars`/`std::to_chars` as the number baseline; no third-party JSON library is pinned. Not a conflict; X-ADR may want to record it in ADR-0062.
3. **AEN-060 vs AST table 3.10-A**: AEN says `DspFilterDef` is built into the engine; AST lists DSP filter definitions as data records in packs. DAT-273 resolves it: a schema-defined table compiled into the engine and read by the validator, with no pack entry.

## Missing assets

None. The authoring folders `content/records/<family>/` (`modes`, `input`, `actors`, `crates`, `audio`, `vfx`, `timelines`, `ui`, `editions`) are not in GameAssets §2.1 (see X-AST below). No Species filename outside the manifest is written; the settings file name is left to the platform layer.

## Cross-document requests

- **Coordinator:** about 90 links point to `architecture/components/data_schemas.md` (TER, R-MAP, R-BLDG, AST, RND, glossary `m_to_z.md` ×11). Repoint: maps → `data_schemas/map_schema.md`; structures → `map_structure_records.md`; settings, bindings, strings → `config_input_and_strings.md`; stats, sound, meshes, explainers → `entity_audio_geometry_and_panel_schemas.md`; import records, editions → `import_and_build_records.md`; otherwise `README.md`. The schema ids were renamed to avoid asset stems: `maps.definition`, `maps.index`, `text.string_table`, `geometry.mesh`, `geometry.collision`, `geometry.sim_record`, `panels.explainer`, `effects.definition`, `interface.page_layout`, `interface.palette`, `interface.text_style`, `interface.font_roles`.
- **X-AST:** add the `content/records/<family>/` folders to GameAssets §2.1; sample-group weights live in `sound.groups` (DAT-275); explainer animations are found by their `mode` field, not member order (DAT-284).
- **G08a:** R-MAP-081 and `ref.map.terrain.guide_grid_levels` say a guide-grid cell takes one of 16 levels A to P; a cell is one value 0 to 255 (TER-036, DAT-083); 16 is the per-letter range of the reference authoring form. Table 2.6-A's single display-name field is split into the derived string id `name_id` and the custom-map text `display_name` (same for the description). Camera-animation steps may name any view of the map (R-MAP-127 "defined before" is a parse-order artefact). Co-op groups hold at most 2 positions.
- **G03a/G03b:** Barrier Emitter `link` "required field, none" is an absent field (DAT-146); grid-terminal link targets are fuel pump and Barrier Emitter, Command Console targets any kind; Launch Vessel reference spawn fields beyond R-BLDG-340 dropped; R-BLDG-633 could note that validated content never meets the out-of-range replacement.
- **G07:** marker pickup values named `normal`, `pickup_only`, `never_pickup`; absent `next_objective` is a chain end (table 3.2-A of structures).
- **C-INP:** INP-163 "last entry wins" never meets content, because two entries for one action in one layer are a build error (DAT-204, DEC-801); user-layer key form `binding.<action id>`; `setting` nodes validated against `settings.defaults` keys (DAT-195); authoring files carry top-level `schema` and `version` (DAT-016).
- **C-RND:** `rnd.option.render_size.default` uses 0 × 0 as "desktop size"; under DAT-008 the absence form expresses this. Setting keys are `render.<option>` from the `rnd.option.*` names.
- **C-PLT:** key the renderer-reader defaults of refresh rate, colour bits and the vertical-sync request (cited by text in table 3.1-A). See also the stale citations below.
- **G09b:** setting keys chosen in table 3.1-A; "no effect" register rows are inert entries; the caption-markup syntax is DAT-211 (`[[binding:…]]`, `[[kbm]]`/`[[pad]]`/`[[end]]`, `[[string:…]]`, `[[open]]`).
- **C-AEN / G10:** per-event `cooldown` (R-AUD-220) is event-level; AEN-062 engine fields are layer-level; effect parameters are authored by name and cooked in kind order (DAT-270 to DAT-272), which reconciles R-AUD-050's positional matching with ADR-0061; ranges are enforced; an event id that code never raises is a build error (DAT-267), which needs the generated list of raisable events. The cooked stream format AEN-072 stays AEN/AST-owned.
- **C-AST:** import-record enumerators are written in lower snake case (`colour`, `cpu_table`, `linear_clamp`); the mesh import record gains `role` (`render`, `collision`); AST-098's simulation-relevant record types are table 1-A ("statistics" = `actor.stats`, "Supply-crate tables" = `crate.outcomes`, "match options" = `mode.options`; no separate "rules" record exists); update AST table 3.10-A for `DspFilterDef` (ADR conflict 3).
- **G04:** outcome-catalogue fields and `delivery` enumerators (`power`, `at_crate`, `global`, `opening_team`, `none`) in DAT-264.
- **G02a:** the `actor_rate` field carries `ref.unit.trooper.actor_rate` (R-UNIT-014).
- **C-CAM, C-VFX, C-UI:** their record types are registered (`camera.timeline`, `effects.definition`, `interface.*`); fields stay theirs. Camera view names are lower case (DAT-090).
- **C-BLD:** request applied (table 3.5-A, 3.5-B); `languages` and `maps` are required fields.
- **X-GLO:** terms: record type, schema id, logical field type, simulation decimal, limit form, absence-meaningful field, derived field, canonical writer, caption marker.
- **C-DBG:** the `dat.perf.*` budgets assume the reference test machine; fixtures per diagnostic class. Further requests in the section below.

## Names chosen (not in the ledger)

- Schema ids of table 1-A (listed above plus `mode.options`, `settings.defaults`, `input.binding_set`, `input.keyboard_layouts`, `actor.stats`, `crate.outcomes`, `sound.sources`, `sound.groups`, `sound.effect_kinds`, `import.texture`, `import.mesh`, `import.audio`, `assets.engine_table`; `edition.manifest` already used by C-BLD).
- JSON field keys: the snake_case of the owners' field names in every table (for example `population_cap`, `gate_transport_cap`, `terrain_tiles`, `flatten_areas`, `start_positions`, `siege_side`, `coop_group`, `tree_crowns`, `valid_receivers`, `flag_links`, `mortar`, `pod_kinds`, `min_distance`, `loop_delay`, `copy_decay`, `cooldown`, `actor_rate`, `colour_role`); annotation names `x-species-*`; the `notes`, `schema` and `version` fields.
- Enumerator strings: modes, map effects (`dust_vortices`, `special_light_rig`), tile methods `average`, `random_pair`, `copy_one`, flatten types, Seed Pod kinds (`crawlers` … `workers`), barrier modes, pickup values, effect kinds and parameter names (table 3.5-A), outcome `delivery` values, control kinds `switch`, `list`, `spinner`.
- Setting keys (`render.*`, `screen.*`, `audio.*`, `interface.*`, `network.*`, `browser.*`, `player.*`, `input.*`, `camera.automatic`, `debug.*`, `binding.<action id>`).
- Code types: `FieldType`, `SimPrecision`, `Presence`, `FieldSpec`, `RecordSchema`, `SchemaRegistry`, `Diagnostic`, `AuthoringParser`, `RecordValidator`, `RecordCooker`, `CanonicalWriter`, `SchemaMigration`, `RecordView`, `RecordLoader`, `MapHeaderView`, `MapDefinitionView`, `MapLoader`, `StructureRecord`, `StructureLink`, `SettingSpec`, `SettingsDefaults`, `SettingsFile`, `StringTableBuilder`, `StringTableView`, `EffectBinding`, `DspParamDesc`, `ImportRecordReader`.

## Dropped as legacy (one line per group)

- Text grammars, tokeniser behaviour, block keywords, positional columns, nested optional tokens and every crash, hang or silent-skip they cause: replaced by named, typed JSON fields (DEC-015, DEC-801).
- Mission files, campaign blocks (running programs, difficulty, leaderboard statistics), campaign-only and blocked structure and actor kinds and their fields (three campaign structure records, one target-location field and one set of launch spawn fields): campaign content (DEC-052, DEC-466).
- Test, stress, benchmark and sandbox map files and their census; level-enum integer ids: DEC-334, DEC-563, DEC-063.
- Difficulty scaling of map records and stats: inert (DEC-010).
- Two-parser menu scanner, case-sensitivity differences, checksum of file bytes: one loader, content hash (DEC-045, DEC-204).
- Writer formats with fixed decimals, dropped zeros, glued lines, format-string values; UTF-16 file encodings; first-line validation; default-preferences overlay file: DEC-800, DEC-100, ADR-0063.
- Platform-specific binding and preference variants, the unreachable alternative scheme, binding lines for absent devices: DEC-056, DEC-553.
- Sound editor writer, sample-or-group switch, editor position kind, priority field, DSP data-type codes and the editor's default-seeding defect: tools and dead fields.
- Mesh text format (strips, depth fields, end markers, discarded normals, row-vector matrix notes, unenabled cylinder bounds): glTF sources and the cooked mesh (ADR-0019 to ADR-0021).
- Screenshot naming, libpng parameters and saved-game thumbnails: ADR-0076; no saves.
- Explainer authoring tool and its archive: tools and IP.
- Custom-map container byte layout and compression details: format is a design-phase input (DEC-207).

## Coordinator requests applied after the first pass

No DAT id was renumbered after the C-BLD and C-DBG messages. Checked: DBG's citation of DAT-050 and BLD's citations of README DAT-001 to DAT-053 (DAT-019, -022, -024 to -028, -030 to -033, -040, -046, -048, -051, -053) match the current text; PLT's do not (below).

- **C-BLD:** `edition.manifest` fields and flags (DAT-358, DAT-359; tables 3.5-A, 3.5-B). Schema ids renamed so none reads as an asset id (listed under Cross-document requests).
- **C-DBG (a)-(c):** table 3.1-A of config now holds `debug.bot_overlay` (integer, three states), `debug.bot_overlay_team` (lobby position), `debug.command_history` (0 off, 1 record, 2 play back; replaces `debug.command_record` and `debug.replay`), `debug.capture_frame_rate` (default `dbg.replay.capture_frame_rate`), `debug.soak_session_name`; I added `telemetry.consent` (DBG-180) and `telemetry.installation_id` (DBG-182) because DBG says both are stored in the settings.
- **C-DBG (d):** new file `test_and_telemetry_records.md` (DAT-370 to DAT-386), registered in README folder index and table 1-A; DAT-003 now names these as diagnostic output.
- **C-PLT 1:** `window.monitor` (text; absent = primary monitor) and `window.client_rect` (four integers) added, citing PLT-028 and PLT-059; DAT-197 now says how a list of integers is written (`a,b,c,d`), T42 covers a three-integer value.
- **C-PLT 2:** DAT-196 now cites PLT-031 for the file. I did not write the file name: the brief forbids Species file names that are not in the asset manifest.
- **C-PLT 3:** `debug.command_history` kept; PLT-049 says `dev.command_history` (mismatch for the coordinator's Phase 4).

## Requests from the follow-up work

- **C-DBG:** (1) DBG-162 and Listing 2.1-A use a seed of 0 for "keep the authority's seed" and a termination tick of 0 for "off"; the schema writes both as absent fields (DAT-374), so a forced seed of 0 cannot be authored until `SoakEntry::seed` becomes `std::optional` (then a minor version widens the range). (2) DBG-166 says "the tick of the first divergence, or −1"; the match table writes an empty cell (DAT-377); the first match's previous entry index is empty too. (3) DBG-181 has no session time, duration, sample count or operating-system version; the record therefore holds none (DAT-379); add them to DBG-181 if needed. (4) For network traffic and round-trip latency, DBG-181 does not say peak, mean or percentiles; the schema reduces them to the `dbg.telemetry.percentiles` like the times; confirm. (5) Entity counts are typed as totals `peak` and `mean`, not per type. (6) Whether the telemetry queue survives a restart is open; DAT-385 covers both. (7) `clients` defaults to the smallest number reaching `dbg.soak.min_participants`; `bot_teams` of a test map is required.
- **C-PLT (stale citations, Phase 4):** PLT cites the config ids as they stood before the one renumbering of this folder. Map: PLT-031 "DAT-198" (text form) → DAT-196; PLT-079 "DAT-200 to DAT-202" (reader) → DAT-198 to DAT-200; PLT-081 "DAT-203" (writer) → DAT-201. The DAT ids are left as they are, because C-BLD and C-DBG cite the current numbers.
- **C-PLT:** `window.client_rect` order chosen as left, top, right, bottom (Windows client-rectangle order); confirm. The telemetry `processor`, `memory`, `adapter` and `display` fields are taken from the groups of PLT table 3.6-A.
- **Coordinator, gate 7:** `check_design.py` reads three string ids in config table 3.1-A and DAT-201 (`ui.player.default_name`, `ui.settings.default_session_name`, `ui.dialog.settings_save_failed`) as asset ids, because `ui` is both an asset stem and a string-id prefix (CONVENTIONS §8 uses `ui.lobby.ready` as a string id). The same applies to PLT, AST and BLD. This needs a CONVENTIONS or gate decision; I did not rename the owners' string ids.

## Old-to-new id map of the one renumbering (before C-BLD and C-DBG cited ids)

README: DAT-001 to DAT-006 unchanged; old 010–021 → 007–018; old 025–059 → 019–053. map_schema: 070–073 unchanged; old 075–095 → 074–094; old 099–114 → 095–110. map_structure_records: 140–150 unchanged; old 155–165 → 151–161. config: 190–192 unchanged; old 195–203 → 193–201; old 205–208 → 202–205; old 210–218 → 206–214; old 220–221 → 215–216; old 226–227 → 217–218. entity, import: unchanged. The new file starts at DAT-370.

## Names chosen in the follow-up work

Schema ids `test.soak_configuration`, `test.soak_report`, `telemetry.session`; folder `content/records/test/`; setting keys `window.monitor`, `window.client_rect` (PLT's), `telemetry.consent`, `telemetry.installation_id`; configuration keys `id`, `progression` (`linear`, `random`, `sequence`), `progression_seed`, `sequence` (`map`, `mode`, `seed`), `test_maps` (`map`, `mode`, `bot_teams`), `session_name`, `termination_tick`, `clients`, `rendering`; match-table columns `utc_time`, `divergence_count`, `mode`, `map_id`, `entry_index`, `seed`, `previous_entry_index`, `match_ticks`, `first_divergence_tick`, `content_hash`, `build_identity`, `crashes`, `failed_checks`, `sim_update_us_p<n>`, `peak_population`; summary keys `configuration`, `edition`, `started_utc`, `ended_utc`, `entries`, `matches`, `divergences`, `match_table`, `failed_entries` (`reason` `content_hash_mismatch`), `crashes` (`role` `host`/`client`, `client`, `dump`), `failed_check_list`; telemetry keys `installation_id`, `build_identity`, `frame_time_us`, `sim_update_time_us`, `entity_count` (`peak`, `mean`), `network` (`bytes_in_per_s`, `bytes_out_per_s`, `round_trip_ms`), `renderer` (`preset`, `settings`), `adapter` (`description`, `driver_version`), `processor` (`logical_processors`, `physical_cores`, `architecture`, `brand`), `memory` (`physical_total`), `display` (`width`, `height`, `refresh_rate_hz`), percentile fields `p<n>`; code types `SoakProgressionKind`, `SoakEntryFailure`, `SoakParticipantRole`, `SoakSequenceItem`, `SoakTestMap`, `SoakConfigurationRecord`, `SoakMatchRow`, `SoakPassSummary`, `TelemetrySession`, `SoakMatchTableWriter`.
