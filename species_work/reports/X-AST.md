# X-AST report (pass 1)

Date: 2026-10-17. Package: `species_work/packages/X-AST.json` (budget-exempt, script-driven). No publishing, sharing or upload tool was used; nothing was published. No git write command was run.

## 1 Files written

| File | Size | Notes |
|---|---:|---|
| `design_next/assets/GameAssets.md` | 149,048 B | owner's document, **over 120 KB, kept as one document as the owner asked** |
| `design_next/assets/asset_manifest.csv` | 763,880 B | 1,311 rows (one per file), 401 asset ids; **over 120 KB** (see §7, request 1) |
| `design_next/assets/README.md` | 7,226 B | counts, constraints per category, placeholder rule |
| `design_next/assets/art_and_audio_briefs.md` | 20,073 B | style guide and one brief per category |
| `species_work/asset_origin_map.csv` | 230,614 B | private; 2,157 rows: every in-scope original mapped to its Species file, one row per excluded original, string-category rows |
| `species_work/scripts/X-AST_build.py` | | builds the set from the CSVs; writes the manifest, the origin map and a scratch JSON |
| `species_work/scripts/X-AST_docs.py` | | writes GameAssets.md and README.md from the JSON (never writes original names) |
| `species_work/scripts/X-AST_check.py` | | coverage, stem-collision, root-folder and manifest checks |

`design_next/assets/.gitkeep` removed. Run order (all `python3 -I`, paths as arguments): `X-AST_build.py --drafts design/_work/drafts --manifest … --origin … --json <scratch>/xast.json`, then `X-AST_docs.py --json … --out design_next/assets/`, then `X-AST_check.py`.

Inputs read: the CSVs of the brief (X2a textures, shapes, refs; X2b samples, sound events; X4 levels, thumbnails, mode map lists; the three string CSVs, counts and categories only), `rename_map.tsv`, `CONVENTIONS.md`, `standards_baseline.md`, `owner_answers.md`, `drop_topics.md`, and the prose the brief allows (25 §1, §2.4, §2.6–§2.8, §4.3–§4.4, §7.1, §7.3; 10 §14.3; 24 §7.1). X2a_refs was used only to read call-site function names for role decisions. 25 §3–§6 catalogue prose was not read.

## 2 Totals per category

| Category | Asset ids | Files | first-playable | launch | post-launch | Reference files replaced |
|---|---:|---:|---:|---:|---:|---:|
| Unit sprites | 12 | 15 | 6 | 8 | 1 | 15 |
| VFX textures | 18 | 18 | 13 | 5 | 0 | 18 |
| UI panels and title | 13 | 15 | 6 | 9 | 0 | 15 |
| HUD, supply-crate and power icons | 32 | 34 | 17 | 15 | 2 | 34 |
| Cursors and markers | 22 | 28 | 22 | 6 | 0 | 28 |
| Input glyphs | 13 | 13 | 1 | 12 | 0 | 13 |
| Terrain ramps, water and waves | 3 | 26 | 3 | 23 | 0 | 26 |
| Meshes | 53 | 71 | 16 | 51 | 4 | 71 |
| Help images and explainer animations | 22 | 75 | 1 | 74 | 0 | 76 |
| Audio by source and event | 167 | 863 | 470 | 393 | 0 | 863 |
| UI sounds | 12 | 14 | 14 | 0 | 0 | 14 |
| Ambience | 4 | 21 | 11 | 10 | 0 | 21 |
| Music (new) | 4 | 6 | 0 | 6 | 0 | 0 |
| Fonts | 5 | 5 | 1 | 4 | 0 | 138 |
| String tables | 9 | 9 | 1 | 8 | 0 | 0 |
| Maps and thumbnails | 12 | 98 | 4 | 94 | 0 | 98 |
| **Total** | **401** | **1,311** | **586** | **718** | **7** | **1,430** |

Audio: 183 live reference sample banks became 183 Species sample groups (167 sfx, 12 UI, 4 ambience); 182 live (source, event) bindings play them; 98 further events fire with no bound sound (silent; no file). Maps: 49 slots (Spawn Control 5/2/2, Zone Control 2/4/3, Relic Run 4/2/3, Siege 4/2/2, Launch Race 3/3/1, Flag Chain 4/2/1 for 2/3/4 players), matching the menu-list distribution exactly (the build script asserts it), plus 49 thumbnails.

## 3 Coverage check (in-scope rows → exactly one Species file)

`X-AST_check.py` exits 0:

| Source CSV | Rows | In scope (each → exactly one Species file) | Excluded |
|---|---:|---:|---:|
| X2a_textures.csv | 527 | 412 | 115 |
| X2a_shapes.csv | 272 | 71 | 201 |
| X2b_samples.csv | 1,332 | 898 (all `live` samples) | 434 |
| X4_levels.csv | 116 | 49 (the retail map list) | 67 |
| **Files** | **2,247** | **1,430** | **817** |
| String CSVs (entries, not files) | 502 | 221 → `content/strings/en_gb.json` | 281 |

Every original appears exactly once in the origin map; every in-scope row's Species file exists in the manifest; no Species file stem equals an original file stem, and none equals one after removing separators and digits (three sound stems were renamed for this: the ant and centipede attack groups use `_bite`, the spider pounce group `_leap`, while their asset ids keep the event name). X4_thumbnails was cross-checked against the planned maps (each planned map's thumbnail is mapped). Many-to-one collapses: 138 font pages → 5 font files; 2 camera-hint pictures (one with baked French text) → 1 text-free picture. Two samples sit in two reference groups; each is one file, owned by the smaller group (`sfx.creature.spider.hurt`), and the other group (`sfx.creature.spider.die`) lists them as shared.

## 4 Scans

- `scan_patterns.py forbidden_patterns.tsv design_next/assets/`: **0 hits** (after fixing two: a format label "(CPU)" and an exclusion-class name naming the previous graphics API).
- `scan_patterns.py soft_patterns.tsv design_next/assets/`: **0 hits** (reworded "shipped" once and "original" twice). Nothing to justify.
- Extra check: no line of `ip_tokens.txt` occurs as a whole word in any of the four outputs.

## 5 First-playable assumption

One skirmish mode (vision O8): **Spawn Control, 2 players**, map slots `map_spawn_control_2p_01` and `_02`, bots on, Supply crates on with a core outcome pool: Reinforcements, Strike team (grenade and rocket weapons), Transport, airstrike, gun turret, Shield, Speed Boost, Fury. Units: Worker (all cards), Leader and banners, Trooper, Transport, Bomber, Essence orbs. Structures: Spawn Node, Relay, Transit Gate, gun turret (mount, head, gun barrel), Barrier Emitter, wall, tree, jump pad, Supply crate. UI: wordmark, panels, menu highlight, check box, all cursors, markers, power-slot HUD for the core pool, keyboard Tab glyph, Spawn Control mode card, primary font, English strings. World: sky clouds, ramp 01, sea tile 01, wave gradient 01. Audio: all groups of those sources and weapons, the match UI sounds, the match ambience bed. Post-launch: seasonal hat, alternative Spawn Node and Transit Gate bodies, the bot navigation node mesh, the default structure mesh, and the two power icons whose role is unconfirmed.

## 6 Scope decisions beyond the CSV reach column

FULL rows excluded because their consumer is a dropped feature or out of scope (counts in GameAssets §20):

- textures: the dropped crate outcome's beam texture; the buy-now price strip (store art); 9 gesture-input icons; 5 research icons for dropped features (control weapon, drop ship, disabled extinguisher outcome, the dropped unit, the dropped mode/unit); the dropped mode's mode picture; the cut 3-player Relic Run map's thumbnail; the fallback thumbnail (25 §1.3 marks it unreachable, though the CSV says FULL).
- meshes: 42 FULL meshes whose only consumers are ledger `drop` identifiers (campaign buildings, the mine and track system, the dropped units, vehicle and tri-walker, the land mine and missile, display screen, drop ship).
- strings: key-level exclusions for campaign location names, trial and store prompts, debug crate help, networking port options, profiles, mods and gesture control schemes.

## 7 Open questions and requests

**Cross-document requests (coordinator):**
1. `asset_manifest.csv` is 764 KB: one row per file with 15 columns cannot fit 120 KB. Exempt it from the 120 KB gate (it is data), or approve a split into `assets/manifest/<category>.csv`; gate 7 must then read the split. Asset ids repeat across a group's file rows by design.
2. `GameAssets.md` (149 KB) needs the owner's one-document exemption from the 120 KB gate, and the `assets/` documents need an exemption from the §6.3/§6.4 template-heading check.
3. CONVENTIONS §8: add the asset-id roots of GameAssets §2.3 as the stems documents may cite.

**For rewrite packages:**
4. G10 (audio): the Species source and event ids used in sound asset ids (GameAssets §13–§15, e.g. `presence_loop`, `flee_bark`, `attack_bark`, `hit_bark`, `capture_progress_00`…`_80`, `relic_scored`, `owner_changed`) should match the event model; tell X-AST pass 2 of any rename. Proposed source name not in the ledger: `airstrike_bomb` (the bomb dropped by the airstrike flight).
5. G04: confirm the roles of `icon.power.laser` (the laser power level) and `icon.power.slots_upgrade` (the power-slots level); confirm the first-playable outcome pool.
6. G02a/G02b: confirm the role of `sprite.worker.state_overlay` (an overlay above a Worker in an altered control state) and where the 3-D `mesh.unit.worker_figure` shows; confirm how the Visitor faction card is used.
7. G03a/G03b: Source Node has no placement in the live maps per the CSV consumer column; confirm whether it is created at run time (priority `launch` kept). Confirm which turret types use `turret_barrel_cannon` (two selection sites), and which constructor branch of the Spawn Node and the Transit Gate a match uses (the other variant is `post-launch`). Confirm whether `mesh.structure.bot_nav_node` is ever visible in a match.
8. C-UI: map the three reference text faces to text roles (`font.ui.primary`, `font.ui.stylised`, `font.ui.compact` are named by order of the reference faces, not by verified role).
9. C-DAT and C-AST: the map source format (assumed JSON text, Species map schema), the explainer script format (assumed JSON key frames), the string-table format (UTF-8 JSON with ICU MessageFormat) and the runtime mesh container name are assumptions pending the data-schema and asset-pipeline ADRs.
10. C-AST: adopt the texture profiles of GameAssets §2.4 (BC7 sRGB for colour, BC4 for additive, mask and darkening art, uncompressed tables for ramps), the 2x authoring rule for UI art, the 20 s threshold for Opus streaming, and the placeholder rule.

**DEC requests (design phase):**
11. Voice: whether Worker, Leader and Trooper barks and the outcome stings use words (one recording per language) or are non-lexical.
12. Language set (the reference nine) and the Portuguese region (`pt-BR` assumed).
13. Music slot list and triggers (6 files proposed).

## 8 Assets whose role could not be fully determined

`sprite.worker.state_overlay`, `icon.power.laser`, `icon.power.slots_upgrade`, `mesh.unit.worker_figure` (usage), `mesh.structure.spawn_node_alt`, `mesh.structure.transit_gate_alt`, `mesh.structure.turret_barrel_cannon` (which types), `mesh.structure.bot_nav_node` (visibility), `mesh.structure.structure_default`, and the font role assignment. Each has a stated default role in the manifest; none was dropped.

## 9 Pass 2

Replace document-level consumers with requirement IDs; add asset ids the documents cite that the manifest lacks (or report the citing file); remove rows nothing consumes; collect the Species string ids from `design_next/` into the string-table rows.
