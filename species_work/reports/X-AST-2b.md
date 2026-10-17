# X-AST-2b report: asset pass 2, part b (Phase 4)

Status: done. Date 2026-10-17. I wrote only my own files: `design_next/assets/*`, `species_work/asset_origin_map.csv`, this report and the helper scripts `species_work/scripts/X-AST-2b_*.py`. I ran no git write command and used no publishing or upload tool, so nothing was published.

## 1 Files written

| File | Size (bytes) | Change |
|---|---:|---|
| `design_next/assets/asset_manifest.csv` | 760,998 | 1,411 rows and 452 asset ids (was 1,412 and 453). Every `consumers` cell now holds requirement ids; a document path stands in only where a document cites the asset outside its requirements (§2). Format strings of the 36 streamed rows fixed; two terrain rows and two icon rows changed (§4). |
| `design_next/assets/GameAssets.md` | 201,444 | Status line, reading note, §2.1, §2.4, §2.5, §3, §6, §7, §10, §11.1, §13, §15, §18.1 and §20 aligned with the manifest. It stays exempt from the 120 KB gate (`CheckDesign.py` `SIZE_EXEMPT`, the owner's single document). |
| `design_next/assets/README.md` | 8,477 | Status, the consumer rule, counts, the excluded-file count and the streamed-audio constraints. |
| `design_next/assets/art_and_audio_briefs.md` | 23,949 | Status, the UI-panel brief (logo banner removed) and the terrain brief (A-36). |
| `species_work/asset_origin_map.csv` | 238,026 | 2,227 rows. The logo banner's reference file is now a "not created: drawn by no rule" row. |

Helper scripts, each run with `python3 -I` and paths as arguments; all three are idempotent:
- `X-AST-2b_apply.py <manifest> <origin_map>`: the manifest row edits of §4 and the origin-map row of the removed asset;
- `X-AST-2b_consumers.py <design_root> <manifest> <detail_csv> [--apply]`: the requirement-level consumers (§2). Without `--apply` it only writes the per-citation detail CSV (asset, file, line, kind, consumer, rule) and prints the gaps, so the coordinator can re-run it after later document edits;
- `X-AST-2b_docs.py <design_root> <origin_map>`: the GameAssets, README and briefs edits; it regenerates the summary tables, the §20 totals and the §18.1 string census from the manifest, the origin map and the documents. Every fixed-text replacement states how often its old text must occur, and the script writes nothing when one does not match.

Run order: `X-AST-2b_apply.py`, then `X-AST-2b_consumers.py … --apply`, then `X-AST-2b_docs.py`.

## 2 Task 1: consumers as requirement ids

**Citations read.** Every `.md` file under `reference/` and `architecture/components/` (the scope of gate 7):
- backticked asset ids in prose, skipping tuning keys, as gate 7 counts them: 2,614 citations;
- wildcards with at least two literal segments (`icon.power.*`, `glyph.pad.*`, `font.ui.*`, `sprite.worker.*`, `cursor.*.shadow`) and placeholder patterns (`icon.power.<kind>`, `sfx.crate.outcome.<outcome>`, `thumb.<mode>`), expanded to the ids they name: 192 citations. Stem-wide wildcards (`sfx.*`, `icon.*`, `fx.*` and the like) are statements about a category and are not counted, as in pass 2a;
- bare ids inside fenced blocks (diagrams and sketches): 1 citation (`fx.glow_round` in the explainer file).

That is 2,807 citations (asset, file, line). Gate 7 reports 2,622, because it counts repeated ids on one line and no patterns.

**Rule for each citation**, in this order:

| Rule | Consumer | Citations |
|---|---|---:|
| a | the requirement whose block holds the citation (from its `**ID**` to the next blank line, definition or heading, as `CheckDesign.py` reads requirement blocks) | 833 |
| b1 | outside a requirement: the nearest requirement of the smallest enclosing section, that is the last one defined before the citation, or else the first one after it | 721 |
| b1 + c | as b1, plus the requirement ids that the line ties to the citation (below) | 233 |
| c | the line-tied ids alone, where the smallest section holds no requirement (acceptance tables, dependency items with a requirement in brackets) | 52 |
| b2 | the nearest requirement of an enclosing section, walking up to the level-2 section | 0 |
| d | none of these: the document path | 968 |

"Line-tied ids" are every defined requirement id of a table row. In other lines they are the ids of the same clause (between semicolons or sentence ends) that come before the citation, and the ids in a bracket right after it. A range of at most ten ids of one prefix (`R-ECON-018 to R-ECON-022`) is expanded.

This clause rule replaces a first, wider rule that took every id on the line. That rule tied, for example, every interface sound in the reference/10 dependency list to R-AUD-313 ("… and the voice and sting groups of R-AUD-313"). The detail CSV lets the coordinator audit any attribution.

**Document paths.** A rule-d path is kept only when no requirement of the same document consumes the asset. 968 citations fall to rule d, nearly all of them §7 "Dependencies" lists. Only **111 (asset, document) pairs** keep a path; every other rule-d document also cites the asset inside a requirement. The 111 pairs are in 21 documents:

| Document | Assets |
|---|---:|
| reference/04 `crates_and_powers.md` | 26 |
| reference/09 `screen_flow_and_match_setup.md` | 22 |
| reference/04 `README.md` | 19 |
| reference/08 `README.md` | 12 |
| reference/01 `README.md` | 10 |
| reference/01 `spawn_control.md` | 3 |
| reference/03 `mode_objects.md` | 3 |
| reference/02 `strike_team.md` | 2 |
| reference/03 `defences_and_props.md` | 2 |
| one each: reference/02 `worker.md`, `transport.md`, `builder.md`, `collector.md`; reference/06 `camera.md`; reference/01 `zone_control.md`, `relic_run.md`, `siege.md`, `launch_race.md`, `flag_chain.md`; `input/README.md`; `diagnostics_test_and_performance/developer_tools.md` | 11 |

Only one asset is consumed by a document path alone: `strings.dev_en_gb`. It is named in the §3 data model of `developer_tools.md`, which holds no requirement. This is exactly the consumer that FX-11 asked for (§5).

**Result.**
- 2,020 (asset, consumer) pairs, against 1,783 document pairs before.
- 1,909 of them are requirement ids: 464 distinct ids under 21 prefixes.
- Every id has 1 to 25 consumers, with a median of 4.
- The order in each cell is: reference requirement ids, then component ids, each by prefix and number, then document paths.
- All rows of one asset id share one cell, as before.
- The 4 stale paths of the split documents are gone: `audio_engine.md` (938 rows), `terrain.md`, `input.md` and `diagnostics_test_and_performance.md`.

**What the column does not list.** The asset pipeline consumes every row, and the audio engine loads every sample group: AEN-074 names "every other resident group of the event catalogue" without listing them. Neither component appears on a row unless one of its requirements names the asset. The README states this.

## 3 Tasks 2 to 5

| Task | Outcome |
|---|---|
| 2: ids cited but missing | **None.** No reference or component document cites an asset id that the manifest lacks (gate 7, and the script with patterns). No document anywhere in `design_next/` (decisions, ADRs, glossary, CONVENTIONS) cites one either. No stale `ui.` or `map.` asset id remains. No citation needed reporting as a defect. |
| 3: rows nothing consumes | **One id, one row removed:** `uitex.logo_banner` (`content/textures/ui/ui_logo_banner.png`, a 394x150 title logo). No document or decision cites it. X-AST-2a had already noted that no rule draws it. It moves to GameAssets §20 as the class "drawn by no rule" (textures 1), and its reference file becomes an excluded row of the origin map. No other row lacks a consumer. The alternative bodies `mesh.structure.spawn_node_alt` and `mesh.structure.transit_gate_alt` and their collision assets stay, because R-BLDG-124 and R-BLDG-140 cite them. |
| 4: consistency | **GameAssets:** status line; reading note; §2.1 folder row ("logo emblem"); §2.4 and §2.5 (§4 below); §3 summary regenerated; §6 row removed; §7 icon rows; §10; §11.1; §13 and §15 formats; §18.1 regenerated; §20 totals 115 → 116 textures, 1,068 → 1,069. **README:** counts and the excluded-file count 787 → 788. **Briefs §5:** 20 files, 13 ids, "the logo emblem". **Checks:** the GameAssets table ids equal the manifest ids (the maps of §19 are listed by mode). The pass-1 checker `X-AST_check.py` exits 0: 1,459 in-scope originals, each mapped to exactly one Species file; 788 excluded; no stem collision and no near match. |
| 5: gates and scans | See §7. |

## 4 Row reconciliation

| | Before (2a) | After (2b) |
|---|---:|---:|
| Asset ids | 453 | 452 |
| Manifest rows | 1,412 | 1,411 |
| first-playable / launch / post-launch rows | 652 / 734 / 26 | 652 / 732 / 27 |
| In-scope reference files ("replaced") | 1,460 | 1,459 |
| Excluded reference files / text entries | 787 / 281 | 788 / 281 |
| Origin-map rows naming a Species file | 1,428 (1,412 + 16 text-category rows) | 1,427 (1,411 + 16) |

**The priority moves** (launch −2; post-launch +2 −1):
- `terrain.ramp` file 14 and `terrain.shore_wave_gradient` file 08 go from launch to post-launch (A-36, §5).
- `uitex.logo_banner` is removed; it was post-launch.

**Other manifest row edits:**
- **Streamed formats (residual 32).** 36 rows read `target_format` "Opus stream in the pack (decoded on a worker thread)". They now read "MS-ADPCM stream in the pack, 48 kHz, decoded by XAudio2" (ADR-025, AEN-072, AST-080):
  - the 4 ambience groups (21 rows);
  - `sfx.creature.abductor.presence_loop` (9 rows);
  - the 4 music groups (6 rows).

  These are the streamed groups of AEN-073. The script asserts that no cell names a third-party codec.
- **Research-level icons.** `icon.power.laser` and `icon.power.slots_upgrade` lose "role to be confirmed against reference/04". R-CRATE-014 now settles their role: research-level icons that no rule of a match draws (DEC-115). They stay post-launch.

## 5 The requests from the wave B and residual reports

| Source | Outcome |
|---|---|
| p4_residuals row 15 (FX-05 §11 item 2) | Done by the requirement-level consumers. **`mesh.structure.bot_nav_node`:** R-BOT-025 (in `reference/07_bots/01_navigation_graph.md`) and R-MAP-111. **`marker.edge_compass_empty`:** R-CTRL-386 (in `reference/06_controls_camera_and_commands/cursor_and_markers.md`) and RND-357. |
| p4_residuals row 32 (FX-09 §13 item 2) | **Manifest:** the 36 Opus rows are fixed (§4). **GameAssets:** the three Opus sentences are rewritten as MS-ADPCM decoded by XAudio2, with no codec in the executable (ADR-025, AST-080): §2.4 (the "long audio and music" profile), §13 and §15. FX-09 named two of them; the §13 intro was a third. **README:** the ambience and music constraints say the same. **Consumer paths `audio_engine.md`:** replaced by AEN-073, AEN-074 and AEN-253 where the audio engine names a group (§2). The ADR, CONVENTIONS, renderer and data-schema parts of row 32 belong to the coordinator. |
| FX-11 §7 (X-AST) | **`strings.dev_en_gb`:** its consumer is `architecture/components/diagnostics_test_and_performance/developer_tools.md` (§2). **GameAssets §18.1:** `ui.dev.*` is now 23, with main owner `diagnostics_test_and_performance/developer_tools`; `ui.dev.command.dump_bindings` has its own named row. **DAT-278:** a consumer of all 23 `mesh.collision.*` ids, because it lists them (rule a). |
| FX-04 §10 item 7 | **The 20 `crate.<outcome>.tooltip` ids** are in the §18.1 census (`crate.*` 37 → 57), with a named row that lists all 20, main owner reference/04 README (R-CRATE-011, R-CTRL-236). **`mesh.structure.turret_mount`** needs no collision asset. The mount is a socket carrier with no geometry: 0 nodes, accepted by AST-045 as a socket-only mesh. The turret's sphere, mesh and ray tests read the turret mesh and the barrel mesh (table 2.12-A), whose collision assets are `mesh.collision.turret_head` and `mesh.collision.turret_barrel` (table 2.12-B, R-BLDG-405). GameAssets §11.1 now says why the mount has none. The "first-playable outcome pool" question (RB-273) is not an asset change and stays with its owner. |
| RES-1 §5 item 2 (A-36) | **Which files:** the R-MAP-086 counts follow the manifest's file order, so the ramp that no match map uses is `terrain.ramp` file 14, and the gradient is `terrain.shore_wave_gradient` file 08. Each is used once over the 57 reference map files, and 0 times on the 49 match maps. Species recreates only the 49 match-map slots. **Rows:** both are kept, because R-MAP-086 states that the manifest recreates all 14 ramps and all 8 gradients, and because the map editor's terrain panel lists every file (R-MAP-325). Their roles say that no match map names them and that the editor lists them. They move to post-launch. GameAssets §2.5 and §10 and briefs §9 say the same. If the coordinator prefers removing them, R-MAP-086's "14 files" and "8" must change first. |

## 6 For the coordinator

1. **Consumers drift with the documents.** RES-2 was editing component documents while this pass ran. After the last document edit of Phase 4, re-run `X-AST-2b_consumers.py design_next design_next/assets/asset_manifest.csv <scratch>/detail.csv --apply` and then `X-AST-2b_docs.py design_next species_work/asset_origin_map.csv`. Both are idempotent. My last run was against the tree at the time of §7.
2. **String ids still cited by no document:**
   - `ui.dialog.sound_settings_failed` (the audio engine should cite it);
   - `match.kill_milestone` (a proposal for the match-rules owner; R-AUD-146 describes the event but names no string id).

   §18.1 lists both as "not yet cited".
3. **GameAssets §18.1 main owner.** The census now prefers any other citing document over `reference/00_reference_overview.md` as a family's main owner. The overview summarises the other documents, and without this rule it became the main owner of `match.game_over.*`, `match.grace_period.*`, `mode.complexity.*` and `ui.confirm_leave.*`.
4. **111 document-path consumers** (§2). They come from §7 Dependencies lists that name an asset no requirement of the same document names. Examples: reference/04 `crates_and_powers.md` lists 26 such assets, and reference/09 `screen_flow_and_match_setup.md` lists the map and thumbnail ids. Each owner may either cite the asset in the requirement that uses it or drop it from the list. Neither is needed for gate 7.
5. **Priorities the owner may revisit:**
   - post-launch for ramp 14 and wave gradient 08 (A-36);
   - the removal of the title logo banner.

   The proposals of the X-AST-2a report §6 item 4 also still stand.

## 7 Checks

- **`python3 Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid`:** gates 1, 2, 3, 5, 6, 7, 8, 9, 10 and 11 pass; gate 4 is skipped. Gate 7 counts 452 asset ids, 1,411 manifest rows and 2,622 asset citations; every row names a consumer.
- **`scan_patterns.py forbidden_patterns.tsv design_next/assets/`:** no output.
- **`scan_patterns.py soft_patterns.tsv design_next/assets/`:** no output.
- **Origin map:** `species_work/asset_origin_map.csv` is the private mapping of reference files. It is the only file that names them, as in passes 1 and 2a, so the forbidden-pattern scan reports those names there by design.
- **`X-AST_check.py`:** exits 0 (§3, task 4).
- **Sizes:** no file of mine is over 120 KB except the exempt GameAssets.md (201 KB) and the CSVs, which gate 10 exempts. Asset files hold no Mermaid.
