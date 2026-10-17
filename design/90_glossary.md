# 90 — Glossary

> Part C (reference) · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: the final documents 00–10, 20–32, 40 and 41 (00's player-facing glossary §8 incorporated); the class list `_work/class_assignment.csv` (159 classes); the "IP items" tables of every document and `_work/routed/_meta_ip.md`; `_work/build_matrix.md`. Term harvest and table rendering: `_work/scripts/S_90_terms.py`; curated term list `_work/script_inputs/S_90_terms.tsv`.

## Contents

- [1 Conventions](#1-conventions)
  - [1.1 Scope](#11-scope)
  - [1.2 How to read an entry](#12-how-to-read-an-entry)
  - [1.3 Paths, citations, units and links](#13-paths-citations-units-and-links)
  - [1.4 Evidence tags and build tags](#14-evidence-tags-and-build-tags)
  - [1.5 The IP column and placeholder names](#15-the-ip-column-and-placeholder-names)
  - [1.6 How the term list was built and checked](#16-how-the-term-list-was-built-and-checked)
- [2 Glossary A–Z](#2-glossary-az)
- [3 Canonical placeholder names](#3-canonical-placeholder-names)
  - [3.1 Selection rules](#31-selection-rules)
  - [3.2 Factions, people and units](#32-factions-people-and-units)
  - [3.3 Creatures](#33-creatures)
  - [3.4 Buildings and mode objects](#34-buildings-and-mode-objects)
  - [3.5 Game modes](#35-game-modes)
  - [3.6 Crate outcomes, powers and comeback systems](#36-crate-outcomes-powers-and-comeback-systems)
  - [3.7 Engine, UI and code identifiers](#37-engine-ui-and-code-identifiers)
  - [3.8 Brand, characters, text, art, audio, maps and services](#38-brand-characters-text-art-audio-maps-and-services)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

## 1 Conventions

### 1.1 Scope

This is the single glossary of the design-recovery set. It defines every domain term, engine term, abbreviation, class or subsystem nickname, data-format name, build tag and evidence tag that a reader meets in documents 00–41, in alphabetical order (§2), and it is the **naming authority** for the neutral placeholders that replace Introversion IP (§3). Documents 41 (risks), 91 (cross-reference index) and the README refer to §3 as canonical.

- Every one of the 159 classes of `code/worldobject/` listed in `_work/class_assignment.csv` has an entry (some Darwinia-legacy families share one row, for example `MineBuilding / Mine / MineCart / …`). Engine classes outside `code/worldobject/` that the documents use as vocabulary (`App`, `Location`, `Team`, `Server`, `Directory`, `LList` and the others) have entries too.
- The player-facing glossary of [00 §8](00_overview.md#8-player-facing-glossary) is incorporated: every concept there appears here with the same meaning and the same placeholder.
- Definitions are short (one to three sentences). The linked "Primary document" section holds the full specification, constants and evidence.

### 1.2 How to read an entry

| Column | Content |
|---|---|
| Term | The word as the documents use it. Code identifiers are in backticks or in their exact code spelling, including the code's own misspellings (`GameTypeBlitzkreig`, `CrateRewardBitzkreig`, `m_naplamBombers`). Rows that cover a family list the members separated by " / ". |
| Kind | One of the kinds below; a term that is both a player concept and a class carries both, separated by ";". |
| Definition | What the term means in this code base, in our own words. Numbers are the shipped values. |
| Primary document | Link to the section that specifies the term; when several links are given the first is primary. |
| IP? | `yes → <placeholder>` for Introversion or third-party IP (placeholder from §3); `no` for generic or technical terms; `no (generic; …)` where a generic word has an optional placeholder in 00. |

| Kind | Used for |
|---|---|
| player concept | something a player sees or decides about, or a rule the new game must reproduce |
| engine | a subsystem, mechanism, convention or identifier family of the code |
| engine (class) | a C++ class or struct |
| engine (build macro) | a preprocessor macro, Visual Studio configuration or product target |
| data format | a file format, block of a file format or naming convention |
| build tag | one of the seven `[B:*]` tags |
| evidence tag | one of `[C]`, `[I]`, `[U]` |
| abbreviation | an acronym or short form |
| IP name | an Introversion or third-party name that has no other entry (brand, character, codename) |

### 1.3 Paths, citations, units and links

- Every path is relative to the `multiwinia/` source root at commit `f909630` and starts with `code/`, `data/`, `contrib/`, `tools/`, `targets/` or `docs/`. A citation `path:Symbol` names the file and the function, class or data key that holds the fact.
- `<P>` is the user profile directory (`…\Introversion\Multiwinia\` on Windows, [31 §5.2](31_platform_build_and_tools.md#52-profile-user-data-directory-resolution-c-codeappcppappgetprofiledirectory-codemaincppinitialise-codemaincppappmain)); `./` is the working directory.
- Units: **wu** (world units, about one metre); seconds are **game seconds** unless called wall-clock seconds; a **tick** is 0.1 s of game time at normal speed; a **mode second** is 11 ticks (1.1 game seconds), see [05 §8.1](05_economy_and_population.md#81-tick-model).
- Section links use the documents' own headings; the rendering script resolves every link against the current heading list and refuses to emit a broken one.

### 1.4 Evidence tags and build tags

| Tag | Meaning (full entries in §2) |
|---|---|
| [C] | confirmed: read directly in code or `data/` |
| [I] | inferred: derived by stated reasoning |
| [U] | unknown: cannot be settled from the tree; numbered NN-U<n> |
| [B:FULL] | live in the baseline `Release\|Win32` + `TARGET_PC_FULLGAME` build |
| [B:D3D] | only under `USE_DIRECT3D` (`ReleaseDirect3D`) |
| [B:DEBUG] | only under `TARGET_DEBUG` |
| [B:EDITOR] | only under `LOCATION_EDITOR` (no shipped target) |
| [B:DEMO] | demo gate: runtime `IS_DEMO` or the `TARGET_MULTIWINIA_DEMOONLY` target |
| [B:LEGACY] | compiled into FULL but unreachable in Multiwinia (Darwinia campaign, blocked objects) |
| [B:DEAD] | not compiled in any PC configuration |

Definitions in §2 restate facts that are evidenced in the primary document; they carry no tag unless the fact itself is an inference (marked [I]) or build-restricted (marked with its `[B:*]` tag). An untagged definition of a feature means [B:FULL].

### 1.5 The IP column and placeholder names

The licence posture is INTERNAL_STUDY: Introversion names, text, art, audio and brands must not appear in the new game. Every term that is IP has `yes → <placeholder>` in §2, and the placeholder is the canonical one of §3. Earlier documents sometimes proposed different placeholders for the same thing; §3 records the chosen name, the rejected alternatives with the documents that proposed them, and why. Where [00 §8](00_overview.md#8-player-facing-glossary) already chose a name, that choice is kept.

### 1.6 How the term list was built and checked

`_work/scripts/S_90_terms.py harvest` (run with `python3 -I`) scanned the 26 final documents present at the last run (00–10, 20–32, 40, 41) and produced: every heading with its anchor (2,310), every bold string (3,319 distinct), every upper-case token of 2–7 characters (1,062 occurring at least twice), the 3,000 most frequent backticked identifiers, mention counts of the 159 classes, per-document counts of the ten tags, and the 353 rows of all "IP items" tables. The terms were then curated by hand into `_work/script_inputs/S_90_terms.tsv`, with definitions written from the primary documents and spot-checked against the source (for example `GRACE_TIME` 3.0, `CRATE_SCAN_RANGE` 35.0, `INCLUDE_THEBEAST` inside `#ifdef TARGET_DEBUG`, task capacity 6 or 2, the 45 s retribution timer, the 30.0 s activation timer, multiplayer spirit timers ÷3 and ÷2, `NUM_TEAMS` 6, `NUM_SLICES_PER_FRAME` 10, `SERVER_ADVANCE_PERIOD` = `0.1 × m_gameSpeed`). `S_90_terms.py render` sorts the terms, escapes table pipes, resolves every "NN:section" link against the real headings (aborting on a miss) and checks that every class of the class list appears in a term. Tag occurrences counted by the harvest (41 was still growing): [C] 6,400, [I] 989, [U] 310, [B:FULL] 1,659, [B:LEGACY] 921, [B:DEAD] 693, [B:DEBUG] 381, [B:EDITOR] 202, [B:D3D] 195, [B:DEMO] 4. The rendered glossary holds 617 terms: 197 classes, 21 player concepts that are also classes, 104 player concepts, 8 player concepts that are also engine mechanisms, 77 engine terms, 34 build macros and configurations, 46 data formats, 102 abbreviations, 18 IP names, 7 build tags and 3 evidence tags.

## 2 Glossary A–Z

<!-- GLOSSARY:BEGIN -->
Jump to: [A](#a) · [B](#b) · [C](#c) · [D](#d) · [E](#e) · [F](#f) · [G](#g) · [H](#h) · [I](#i) · [J](#j) · [K](#k) · [L](#l) · [M](#m) · [N](#n) · [O](#o) · [P](#p) · [Q](#q) · [R](#r) · [S](#s) · [T](#t) · [U](#u) · [V](#v) · [W](#w) · [X](#x) · [Z](#z)

### A

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Achievement | player concept | One of 12 named goals evaluated by `AchievementTracker`; in the PC baseline unlocking is stubbed, so nothing is ever awarded. | [09 §7](09_meta_systems.md#7-achievements-statistics-and-leaderboards) | yes → per §3 achievement row |
| Achievement names | IP name | Dominator, Hoarder, Hail To The King, Unclad Skies, Aggravated Assault, Master Player, Master Of Multiwinia, Explorer, Carnage, Blitz Master, Wrong Game, Genocide (`code/app.h`). | [09 §7.1](09_meta_systems.md#71-achievement-catalogue-and-exact-unlock-conditions), [01 IP items](01_game_modes.md#ip-items) | yes → see §3 |
| Activation delay | player concept | The 30.0 s (300 ticks) after any ownership change before a spawn point produces; skipped for a team's only spawn point and on maps with `instantspawnpointcapture`. | [05 §2.3](05_economy_and_population.md#23-activation-delay) | no |
| ADSR | abbreviation | Attack, Decay, Sustain, Release: the four-stage volume envelope a sound instance applies over its playback. | [27 §6.3](27_audio_engine.md#63-envelope-adsr--exact) | no |
| AI | abbreviation | Artificial intelligence; in this set usually the CPU player, which is the entity class `AI` (one per CPU team) driving that team's officers, tasks and crowds. See `AI` entity. | [07 §1](07_ai.md#1-overview) | no |
| `AI` entity | engine (class) | Entity type 18: the invisible, invulnerable "brain" entity of every CPU team, simulated on every peer in lock-step. | [02 §24](02_units_and_entities.md#24-ai), [07 §3.1](07_ai.md#31-ai-entity-typeai-c-codeworldobjectaihai) | no |
| AI priority (AITarget) | engine | The per-team value of an AI graph node, recomputed every 2 s by a mode-specific formula (standard, CTS, Assault and Blitzkrieg diffusion, Rocket Riot); CPU teams send officers and crowds toward high-priority nodes. | [07 §5](07_ai.md#5-priority-model--every-formula) | no |
| AI type (difficulty) | player concept | Lobby option for CPU players (`Multiwinia::m_aiType` ∈ `AITypeEasy`, `AITypeStandard`, `AITypeHard`; shown as Easy, Normal, Hard); one setting for every CPU team. | [07 §13](07_ai.md#13-difficulty-levels--every-parameter) | no |
| AIObjective | engine (class) | Building type 71: node of the Assault (and Rocket Riot) objective chain that directs attacking and defending CPU teams. | [03 §10.2](03_buildings_and_world_objects.md#102-aiobjective), [07 §11](07_ai.md#11-ai-objectives--assault-objective-chain-and-rocket-riot-markers) | no |
| AIObjectiveMarker | engine (class) | Building type 72: point of interest of an AI objective whose ownership decides completion. | [03 §10.3](03_buildings_and_world_objects.md#103-aiobjectivemarker) | no |
| Airstrike | player concept | Outcome 4 `CrateRewardAirstrike` (basic): 3–7 smoke markers each summon 2 bombers that drop one bomb (explosion radius 25, damage 100). | [04 §5.5](04_crates_powerups_and_weapons.md#55-airstrike) | no |
| AirStrikeMarker | engine (class) | Smoke marker thrown by an airstrike; once at rest (after 2 s) it summons an `AirstrikeUnit` of bombers. | [04 §6.5](04_crates_powerups_and_weapons.md#65-airstrikemarker) | no |
| AirstrikeUnit | engine (class) | `Unit` container of the 2 bombers summoned by one airstrike smoke marker; flies in, bombs the marker and leaves. | [02 §11](02_units_and_entities.md#11-airstrike) | no |
| AISpawnPoint | engine (class) | [B:LEGACY] Darwinia scripted enemy spawner. | [03 §10.4](03_buildings_and_world_objects.md#104-aispawnpoint) | no |
| AITarget | engine (class) | Building type 42: node of the AI's navigation and priority graph, linked to neighbours within 600 wu, with per-team counts and priorities. | [03 §10.1](03_buildings_and_world_objects.md#101-aitarget), [07 §4](07_ai.md#4-the-aitarget-node-graph--the-ais-map) | no |
| ALIVE_DATA | abbreviation | Network data-field code `dg` carrying a packed `TeamControls` record, the continuous per-team input sent every frame. | [22 §5.2](22_determinism_and_networking.md#52-team-controls-payload-alive_data--codeteamcppteamcontrolspack), [29 §11.7](29_data_formats_reference.md#117-packed-team-controls-alive_data-payload) | no |
| Alliance | player concept | A group of teams that `Location::IsFriend` treats as friendly (co-op pairs, Assault attacker or defender sides); not a separate object in the code. | [21 §9.4](21_simulation_and_entity_model.md#94-friendship--locationisfrienda-b) | no |
| .anim | data format | Text keyframe script of an animated help panel (`data/animations/*.anim`, six files), read by `AnimatedPanel::Read`. | [29 §8.6](29_data_formats_reference.md#86-anim--animated-help-panel), [26 §3](26_animation_and_vfx.md#3-anim-format-and-animatedpanel-playback) | yes → new choreographies |
| Animated panel | player concept | Front-end mode explainer animation played from a `.anim` script (`AnimatedPanel`), one per shipped mode. | [09 §9.3](09_meta_systems.md#93-animated-game-mode-panels-animatedpanel-animatedpanelrenderer-bfull), [26 §3](26_animation_and_vfx.md#3-anim-format-and-animatedpanel-playback) | yes → new explainer animations |
| AntHill (ant hill) | engine (class) | Monster nest created by the Ants Nest outcome that sends army ants out and converts collected spirits into ants. | [03 §9.1](03_buildings_and_world_objects.md#91-anthill) | yes → Hive |
| Ants Nest | player concept | Outcome 14 `CrateRewardAntNest` (good and bad pools): 2 monster-team ant hills releasing 51–75 army ants each; at most 4 nests. | [04 §5.15](04_crates_powerups_and_weapons.md#515-ants-nest) | yes → Hive |
| API | abbreviation | Application programming interface. | [20 §3.4](20_architecture.md#34-third-party-libraries-and-os-apis) | no |
| App (g_app) | engine (class) | The process-wide application object (`code/app.h`), reached through the global `g_app`; owns every subsystem pointer (renderer, sound, network, location, menus) and the game-mode state. | [20 §4](20_architecture.md#4-the-app-class) | no |
| AppDebugOut / AppReleaseAssert | engine | Debug logging into `debug.txt` and the always-on assertion that writes the black box and exits. | [32 §10.1](32_debug_test_and_performance.md#101-debug-output-appdebugout-and-debugtxt-c-contribsystemivlibdebug_utilscpp-codemaincppinitialise), [32 §10.2](32_debug_test_and_performance.md#102-asserts-and-the-black-box-c-contribsystemivlibdebug_utilsh-contribsystemivlibdebug_utilscpp-summary-also-in-31-65) | no |
| AppRandom / frand / sfrand | engine | The unsynced LCG family (MSVC `rand()` clone, seeded 1 per level): for cosmetics only, though trees misuse it. | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code), [22 §14.7](22_determinism_and_networking.md#147-the-shared-non-synced-lcg-is-a-simulation-input-for-trees--and-it-is-not-thread-safe) | no |
| ARB / WGL / GLU | abbreviation | OpenGL extension-family prefix (Architecture Review Board), the Windows OpenGL binding layer, and the OpenGL utility library; all three are used by the native renderer. | [24 §1.3](24_rendering.md#13-native-opengl-path-bfull) | no |
| Armour | player concept; engine (class) | Entity type 15: hovering troop transport carrying up to 100 Darwinians over land and sea; no weapon in Multiwinia; deploys its load at a target. | [02 §8](02_units_and_entities.md#8-armour), [06 §6.4](06_player_control_and_input.md#64-armour) | yes → Transport |
| Armour (crate outcome) | player concept | Outcome 3 `CrateRewardArmour` (basic; invalid in Rocket Riot and where `trunkportarmour` is set): a placed troop transport. | [04 §5.4](04_crates_powerups_and_weapons.md#54-armour) | yes → Transport |
| ArmyAnt (army ant) | engine (class) | Entity type 14: fast monster-team ant from an ant hill that collects spirits, carries Darwinians back to be killed and bites other enemies. | [02 §13](02_units_and_entities.md#13-armyant) | no (generic; "Ant") |
| Array2D / SurfaceMap2D | engine (class) | Template 2-D arrays over the world (`code/lib/2d_array.*`, `2d_surface_map.*`) used for height maps and grids. | [23 §9.1](23_terrain_and_landscape.md#91-sizes), [21 §6.1](21_simulation_and_entity_model.md#61-overview) | no |
| ASCII | abbreviation | 7-bit character encoding; most data text files are ASCII or 8-bit, widened byte-for-byte to Unicode on load. | [29 §3.1](29_data_formats_reference.md#31-textreader-line-reader-and-tokeniser) | no |
| ASLT / DOM / RR / BLITZ / TANK | abbreviation | Short mode tokens used in tables: Assault, Domination, Rocket Riot, Blitzkrieg, Tank Battle (with KOTH and CTS). | [08 §3.1](08_levels_and_level_design.md#31-master-table), [01 §1.1](01_game_modes.md#11-the-seven-game-types) | yes → Siege, Spawn Control, Launch Race, Flag Chain, Armour Duel |
| Assault | player concept | Game type `GameTypeAssault`: attackers must capture every control station linked to the defenders' pulse bomb before its countdown ends; if the countdown reaches zero the bomb detonates and the defenders win. Single round only in the shipped code. | [01 §4.4](01_game_modes.md#44-assault-gametypeassault-bfull) | yes → Siege |
| Attackers / defenders | player concept | The two sides of an Assault map: the level's `AttackingTeams` positions attack, the rest defend the pulse bomb. | [01 §2.5.6](01_game_modes.md#256-assault-sides-c-codelocationcpplocationsetupassault-getfreeattackerposition-getfreedefenderposition-isattacking) | no |
| Attract mode | engine | [B:DEAD] self-playing demo mode (`ATTRACTMODE_ENABLED`). | [09 §9.7](09_meta_systems.md#97-attract-mode-attractmode-bdead) | no |
| Authentication (keys) | engine | `contrib/systemiv/lib/metaserver/authentication.*`: local key format checks, hashed metaserver checks, demo key and blacklist handling. | [22 §12.5](22_determinism_and_networking.md#125-authentication-keys-c-contribsystemivlibmetaserverauthentication-contribsystemivlibmathhashcpphashdata), [09 §6.3](09_meta_systems.md#63-authentication-keys) | yes → platform entitlements |
| Authentication key | player concept | The product key typed on `PageAuthKeyEntry`, checked locally and by the metaserver; decides full or demo status. | [09 §6.3](09_meta_systems.md#63-authentication-keys), [22 §12.5](22_determinism_and_networking.md#125-authentication-keys-c-contribsystemivlibmetaserverauthentication-contribsystemivlibmathhashcpphashdata) | yes → platform entitlements |
| Authentication key file | data format | `<P>authkey` and `authkey.dev`, the stored product key. | [29 §11.3](29_data_formats_reference.md#113-authentication-key-files) | no |
| AUTHENTICATION_LEVEL | engine (build macro) | Set to 1 in FULLGAME: the server removes any remote client whose authentication-key check fails (`Server::AuthenticateClients`). | [22 §12.5](22_determinism_and_networking.md#125-authentication-keys-c-contribsystemivlibmetaserverauthentication-contribsystemivlibmathhashcpphashdata) | no |
| AVI | abbreviation | Audio Video Interleave video file; written only by the [B:DEBUG] grabber (`AVIGenerator`). | [31 §3.12](31_platform_build_and_tools.md#312-video-capture--avigenerator-bdebug-c-codelibavi_generatorcpp-codelibavi_generatorh) | no |
| AVIGenerator | engine (class) | [B:DEBUG] AVI video grabber; no code adds frames, so it is non-functional. | [31 §3.12](31_platform_build_and_tools.md#312-video-capture--avigenerator-bdebug-c-codelibavi_generatorcpp-codelibavi_generatorh), [32 §5.4](32_debug_test_and_performance.md#54-grabber-window-avi-capture-bdebug-non-functional-c-codeinterfacegrabber_windowcpp-codelibavi_generatorcpp-codelibhi_res_timecpp) | no |

### B

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| [B:D3D] | build tag | Only in the secondary `ReleaseDirect3D` configuration (`USE_DIRECT3D`), where a Direct3D 9 layer emulates the OpenGL API; [I] probably does not compile at this commit. | [24 §1.5](24_rendering.md#15-direct3d-9-path-opengl-api-emulation-layer-bd3d), [31 §7.9](31_platform_build_and_tools.md#79-evidence-that-releasedirect3d-does-not-compile-at-this-commit) | no |
| [B:DEAD] | build tag | Not compiled in any PC configuration (macro never defined, file excluded, or platform-only); documented only in appendices. | [31 §2.5](31_platform_build_and_tools.md#25-linux-and-darwinia-mac-targets-bdead), [32 §2](32_debug_test_and_performance.md#2-build-flag-table--every-debug-and-test-feature) | no |
| [B:DEBUG] | build tag | Only under `TARGET_DEBUG` (cheat menu, sound and gesture editors, `TRACK_SYNC_RAND`, Tank Battle, The Beast crate). | [32 §2](32_debug_test_and_performance.md#2-build-flag-table--every-debug-and-test-feature), [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line) | no |
| [B:DEMO] | build tag | Restricted by the demo gate: either the runtime check `IS_DEMO` (no valid full-version key) inside the full build, or the separate `TARGET_MULTIWINIA_DEMOONLY` product target; each use states which. | [09 §6](09_meta_systems.md#6-demofull-game-gating-and-the-authentication-key-ui) | no |
| [B:EDITOR] | build tag | Only under `LOCATION_EDITOR`, which every shipped target leaves commented out: the in-game map editor and its windows. | [08 §8](08_levels_and_level_design.md#8-the-map-editor-beditor) | no |
| [B:FULL] | build tag | Present and reachable in the baseline build: `Release\|Win32` (OpenGL) with `TARGET_PC_FULLGAME`, shipped as `multiwinia.exe`. | [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line), [20 §1](20_architecture.md#1-scope-and-reading-guide) | no |
| [B:LEGACY] | build tag | Compiled into the full build but unreachable in Multiwinia: Darwinia campaign code, objects blocked by `BLOCK_OLD_DARWINIA_OBJECTS`, single-player-only branches. | [03 §12](03_buildings_and_world_objects.md#12-darwinia-legacy-buildings-blocked), [21 §2.8](21_simulation_and_entity_model.md#28-every-class-base-header-build-tag-behaviour-specification) | no |
| BandwidthCounter / NetworkQuality | engine (class) | Per-link byte-rate statistics, and a [B:DEAD] link simulator (latency, loss, bandwidth) for testing. | [22 §11.1](22_determinism_and_networking.md#111-bandwidthcounter-codenetworkbandwidth-c), [22 §11.3](22_determinism_and_networking.md#113-networkquality-codenetworknetwork_quality--a-link-simulator-not-a-measurement-c) | no |
| Bang (explosion model) | engine | `Location::Bang(pos, range, damage, team)`: the shared explosion that damages entities in range, pushes them, spawns a `Shockwave` and debris; used by grenades, bombs, warheads and meteors. | [04 §6.1](04_crates_powerups_and_weapons.md#61-shared-explosion-model--locationbang), [02 §2.7](02_units_and_entities.md#27-explosions--locationbang-and-shockwave) | no |
| Basic / advanced crate | player concept | `Crate::IsBasicCrate` = Squad, Engineer, Armour, Airstrike, Darwinians, Gun Turret, Magical Forest (7 types); every other outcome is advanced (24 in FULL). | [04 §4.5](04_crates_powerups_and_weapons.md#45-basic-vs-advanced-crates) | no |
| Basic crates only | player concept | Lobby check box (`multiwinia_param_basiccrates` → `Multiwinia::m_basicCratesOnly`, network option `GAMEOPTION_BASICCRATES`): only the 7 basic outcomes can be drawn and bad outcomes are disabled. | [04 §4.5](04_crates_powerups_and_weapons.md#45-basic-vs-advanced-crates) | no |
| Benchmark mode | engine | Three pieces, none playable in FULL: the [B:DEAD] `BenchMark` per-second telemetry recorder with FTP upload (`BENCHMARK_AND_FTP`), the stress-test maps `mp_benchmark_1` and `mp_assault_stress`, and the hardware-compatibility SKU. There is no benchmark score. | [32 §8](32_debug_test_and_performance.md#8-benchmark-mode) | no |
| BGR24 / BGRA32 / P4 / P8 / RLE8 | abbreviation | Bitmap pixel layouts found in the shipped `.bmp` files: 24-bit and 32-bit direct colour, 16- and 256-entry palettised, and run-length-encoded 8-bit (one file, which the decoder cannot read). All decode to RGBA8. | [25 §2.2](25_textures_and_assets.md#22-bitmap-decoding-bitmaprgbaloadbmp--supported-bmp-variants), [29 §8.3](29_data_formats_reference.md#83-bmp-bitmaps-and-textures) | no |
| BinaryReader | engine (class) | Reader for binary resources (archives, bitmaps, Ogg files). | [29 §3.4](29_data_formats_reference.md#34-binaryreader-conventions) | no |
| BitmapRGBA | engine (class) | In-memory RGBA8 image (`code/lib/bitmap.cpp`): BMP decoding, colour keying, pixel operations and texture upload. | [25 §2.2](25_textures_and_assets.md#22-bitmap-decoding-bitmaprgbaloadbmp--supported-bmp-variants), [25 §2.5](25_textures_and_assets.md#25-texture-upload-bitmaprgbaconverttotexture) | no |
| Black box | engine | Release-assert report `blackbox.txt` with recent debug output. | [31 §6.5](31_platform_build_and_tools.md#65-asserts-and-the-black-box-c-contribsystemivlibdebug_utilsh-contribsystemivlibdebug_utilscpp), [29 §11.8](29_data_formats_reference.md#118-blackboxtxt--release-assert-report) | no |
| blackbox.txt / ERRORLOG.TXT / CRASH.DMP / debug.txt | data format | Diagnostic files: release-assert report, SEH crash log and minidump, debug log. | [29 §11.8](29_data_formats_reference.md#118-blackboxtxt--release-assert-report), [29 §11.9](29_data_formats_reference.md#119-errorlogtxt-and-crashdmp--seh-crash-report), [29 §11.10](29_data_formats_reference.md#1110-debugtxt--debug-log) | no |
| Blitzkrieg | player concept | Game type `GameTypeBlitzkreig` (sic): capture flag zones linked in a graph; a team is eliminated when an enemy captures its home flag while holding an unbroken chain of owned flags back to its own home flag. At time-up, flags held plus 10 per elimination decide. | [01 §4.6](01_game_modes.md#46-blitzkrieg-gametypeblitzkreig-bfull) | yes → Flag Chain |
| BLOCK_OLD_DARWINIA_OBJECTS | engine (build macro) | On in FULLGAME: `Building::BuildingIsBlocked` and `Entity::EntityIsBlocked` refuse level-file placement of Darwinia-only types (39 building types, the blocked entity types). Code can still create them at run time. | [03 §3.12](03_buildings_and_world_objects.md#312-buildingisblocked), [02 §1.1](02_units_and_entities.md#11-every-entitytype-value) | no |
| Blocked type | engine | A building or entity type that `BuildingIsBlocked` / `EntityIsBlocked` refuse to place from level files under `BLOCK_OLD_DARWINIA_OBJECTS`. | [03 §3.12](03_buildings_and_world_objects.md#312-buildingisblocked), [02 §1.1](02_units_and_entities.md#11-every-entitytype-value) | no |
| BlueprintBuilding / BlueprintStore / BlueprintConsole / BlueprintRelay | engine (class) | [B:LEGACY] Darwinia "Pattern Buffer" infection puzzle buildings. | [03 §12.11](03_buildings_and_world_objects.md#1211-blueprintbuilding) | yes → drop |
| .bmp (BMP) | data format; abbreviation | Windows bitmap, the only image format the game loads (`TEXTURE_EXTENSION` = "bmp"): every texture, sprite, icon, thumbnail, help image and colour ramp (506 files), decoded to RGBA8 with optional colour keying. | [29 §8.3](29_data_formats_reference.md#83-bmp-bitmaps-and-textures), [25 §3](25_textures_and_assets.md#3-per-file-catalogue--bitmaps-animation-scripts-and-shaders) | yes → new art |
| BOM | abbreviation | Byte-order mark; no shipped text file has one, and a UTF-8 BOM would be read as part of the first token. | [29 §3.1](29_data_formats_reference.md#31-textreader-line-reader-and-tokeniser) | no |
| Boot-loader | engine | [B:DEAD] retro mini-program loading screens (`code/loaders/`, `USE_LOADERS`). | [09 A.1 Boot-loader framework and mini-programs [B:DEAD] (USE_LOADERS commented out)](09_meta_systems.md#a1-boot-loader-framework-and-mini-programs-bdead-use_loaders-commented-out) | yes → drop |
| BoxKite | engine (class) | [B:LEGACY] spirit-worship lantern released by worshipping Darwinians (single-player only). | [04 §6.22](04_crates_powerups_and_weapons.md#622-boxkite) | no |
| Bridge | engine (class) | Teleport subclass built by engineers at research level 5, which is unreachable. | [03 §12.15](03_buildings_and_world_objects.md#1215-bridge) | no |
| BTree | engine (class) | SystemIV binary tree keyed by string ids (sorted, fast look-up); used for example by the phrase table `LangTable`. | [20 §3.1](20_architecture.md#31-module-inventory) | no |
| Building | engine (class) | Abstract base of all static world objects: `Building::Type` (75 values, `TypeInvalid` 0 to `TypeSmokeMarker` 74), ports, team, shape, damage, level-file line format and factory. | [03 §3.1](03_buildings_and_world_objects.md#31-building), [03 §2.1](03_buildings_and_world_objects.md#21-building-type-enum-all-75-values) | no |
| BuildingPort | engine (class) | Struct for one port: shape marker, current occupant, world transform and per-team reservation counters (0–50). | [03 §3.3](03_buildings_and_world_objects.md#33-buildingport) | no |
| Burst / pause cycle | player concept | The spawn-point request rhythm: while owned, activated and under the cap a point requests one virtual spirit every 0.2 s until it has requested `int(20 × handicap multiplier)` (20 without handicap), then pauses 10–15 s; the timer freezes while the cap is reached. | [05 §3.1](05_economy_and_population.md#31-request-cycle-multiwinia) | no |

### C

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| [C] | evidence tag | Confirmed: the statement was read directly in code or `data/` at commit `f909630` and is cited as `path:Symbol`. | [00 Open questions and unknowns](00_overview.md#open-questions-and-unknowns), [29 §1.4](29_data_formats_reference.md#14-failure-vocabulary-and-verification-method) | no |
| Camera | player concept; engine (class) | `g_app->m_camera` (`code/camera.cpp`): 15 camera modes, of which free movement (the match camera), entity tracking and building aim are live in Multiwinia. | [06 §9](06_player_control_and_input.md#9-camera), [24 §5.1](24_rendering.md#51-projection-and-view-c-coderenderercpprenderersetupprojmatrixfor3d-renderframe-codecameracppcamerasetupprojectionmatrix-camerasetupmodelviewmatrix) | no |
| Camera mount / camera animation | engine | Named camera positions in a level file (`player0`–`player3`, `start`) used at match start, and keyframed camera paths ([B:LEGACY] playback). | [30 §4](30_scripting_tutorial_and_cutscenes.md#4-camera-animation-and-camera-mounts), [08 §6.5](08_levels_and_level_design.md#65-camera-mounts-and-animations) | yes → `spawn_view_<n>`, `default_view` |
| Capture (building) | player concept | Ownership change of a building by occupation or proximity. Each capturable type has its own rule and period (spawn point: a single team with at least 3 of 5 port occupants, re-evaluated every 2–3 s; gun turret 1 s; solar panel and control station every tick; zones and carryables 1 s). | [03 §3.7](03_buildings_and_world_objects.md#37-capture-model-summary-every-capturable-type) | no |
| Capture the Statue | player concept | Game type `GameTypeCaptureTheStatue`: groups of Darwinians lift statues and carry them into their own drop zone; each delivery scores 1 point; highest score at time-up wins. | [01 §4.3](01_game_modes.md#43-capture-the-statue-gametypecapturethestatue-bfull) | yes → Relic Run |
| CarryableBuilding | engine (class) | Base of objects lifted and carried by groups of Darwinians (Statue, WallBuster, ChessPiece): lifters, carry radius, speed scale. | [03 §8.3](03_buildings_and_world_objects.md#83-carryablebuilding) | no |
| Cave | engine (class) | [B:LEGACY] Virii spawner, effectively indestructible. | [03 §12.16](03_buildings_and_world_objects.md#1216-cave) | no |
| Centipede | engine (class) | Entity type 9: chain of segments led by a head that hunts enemies, eats spirits and grows; killing a middle segment splits it. | [02 §18](02_units_and_entities.md#18-centipede) | no (generic word) |
| Cheat window | engine | [B:DEBUG] window of 23 cheats (`CHEATMENU_ENABLED`), several of which desynchronise network games. | [32 §4](32_debug_test_and_performance.md#4-cheat-window--all-23-cheats) | no |
| CHEATMENU_ENABLED | engine (build macro) | [B:DEBUG] switch for the cheat window and its 23 cheats. | [32 §4](32_debug_test_and_performance.md#4-cheat-window--all-23-cheats) | no |
| ChessBase / ChessPiece | engine (class) | Prototype "chess" mode base and carryable pieces; no pieces ever spawn because its game options do not exist. | [03 §12.17](03_buildings_and_world_objects.md#1217-chessbase) | no |
| Christmas mod | engine | `Location::ChristmasModEnabled`: server-switched seasonal mode (snow, Santa hats, light colours). | [26 §4.3](26_animation_and_vfx.md#43-seasonal-content--christmas-mode-bfull), [09 §5.16](09_meta_systems.md#516-seasonal-christmas-mode-lobby-negotiated-snow-bfull) | no |
| Christmas mode | player concept | Seasonal theme negotiated in the lobby: snow weather, Santa hats on Darwinians and icy terrain ramps. | [09 §5.16](09_meta_systems.md#516-seasonal-christmas-mode-lobby-negotiated-snow-bfull), [26 §4.3](26_animation_and_vfx.md#43-seasonal-content--christmas-mode-bfull) | yes → optional seasonal theme with new art |
| CJK | abbreviation | Chinese, Japanese and Korean scripts; served by bitmap font atlases under `data/textures/unicode/`. | [25 §2.15](25_textures_and_assets.md#215-fonts-and-atlases), [28 §10.5](28_ui_framework.md#105-font-selection-per-language-c-codeappcppappsetlanguage) | no |
| ClientToServer | engine (class) | `g_app->m_clientToServer`: every peer's client side; sends requests and `ALIVE` input, receives letters, executes them in sequence order and produces the sync byte. | [22 §6.4](22_determinism_and_networking.md#64-client-connection-states-c-codenetworkclienttoserverh) | no |
| CloneLab | engine (class) | Capturable incubator variant found only in four unshipped prototype maps. | [03 §11.3](03_buildings_and_world_objects.md#113-clonelab) | no |
| Clouds | engine (class) | Sky and cloud-layer renderer (sky, blobby and flat layers); it can also cross-fade Dr Sepulveda's face into the sky, reachable only through a [B:DEBUG] cheat. | [24 §4.4](24_rendering.md#44-clouds-and-sky-clouds-bfull-c-codecloudscpp) | no |
| Co-op | player concept | Two alliances of two players sharing a colour family, on maps whose options allow it; allied teams are friends for targeting and scoring. | [01 §2.5.5](01_game_modes.md#255-co-op-alliances-c-codelocationcpplocationsetupcoop-setcoopteam-setsoloteam-isfriend) | no |
| Colour ramp | data format | A `data/terrain/landscape_*.bmp` image mapping terrain slope and height to a vertex colour (flat-shaded look); wave and water tables are similar ramps. | [23 §11](23_terrain_and_landscape.md#11-colour-ramp-application-dataterrainlandscape_bmp), [29 §8.5](29_data_formats_reference.md#85-terrain-water-and-wave-colour-ramps) | yes → `ramp_<theme>` |
| Commented-out outcomes | player concept | `CrateRewardFormationIncrease`, `CrateRewardExtinguisher` and `CrateRewardTank`: enum members commented out, never compiled ([B:DEAD]). | [04 §2.1](04_crates_powerups_and_weapons.md#21-identity-build-gating-and-pool-membership) | no |
| ConstructionYard | engine (class) | [B:LEGACY] Darwinia armour factory (level name `Yard`). | [03 §12.1](03_buildings_and_world_objects.md#121-constructionyard) | no |
| Control (ControlType) | engine | One of 164 named input actions (`ControlUnitPrimaryFireTarget`, `ControlToggleReady`) bound in `input_preferences.txt`. | [06 §12.2](06_player_control_and_input.md#122-control-table-all-164) | no |
| Control help / MultiwiniaHelp | engine (class) | The old gamepad control-help system (never shown in Multiwinia) and its live replacement, the hint overlays. | [06 §10](06_player_control_and_input.md#10-control-help-and-multiwinia-hint-overlays) | no |
| ControllerGrenade | engine (class) | [B:LEGACY] squad grenade that hands nearby friendly Darwinians to squad control. | [04 §6.17](04_crates_powerups_and_weapons.md#617-controllergrenade) | no |
| ControlStation (control station) | player concept; engine (class) | 5-port console (3 ports when linked to a pulse bomb) whose owner controls a linked building; in Assault capturing every linked station destroys the bomb. | [03 §5.4](03_buildings_and_world_objects.md#54-controlstation) | yes → Uplink Console |
| ControlTower | engine (class) | Remote reprogramming console worked by engineers; only Darwinia and dev maps place it. | [03 §11.1](03_buildings_and_world_objects.md#111-controltower) | no |
| Countdown | player concept | The 3-second "GAME STARTING IN" display at the end of the grace period, decremented 0.1 per tick with a tick sound per number and `CountdownZeroTick` at zero. | [01 §2.4](01_game_modes.md#24-grace-period-readiness-and-countdown), [10 §7.2](10_audio_design.md#72-countdown-timer-and-match-end-audio-verified-schedule) | no |
| CPU (player) | abbreviation | Computer-controlled team that fills an empty lobby slot (team type `TeamTypeCPU`), steered by an `AI` entity; also used for the processor in performance sections. | [07 §2.3](07_ai.md#23-instantiation-and-lifetime-c), [09 §5.12](09_meta_systems.md#512-ready-start-kick-request-ai-spectator) | no |
| Crash reporter (MultiwiniaSupport.exe) | engine | The SEH crash handler writes `ERRORLOG.TXT` and `CRASH.DMP`; the support tool (XCrashReport-based) gathers the files listed in `MultiwiniaSupport.ini`. | [31 §6.4](31_platform_build_and_tools.md#64-support-tool--multiwiniasupportexe-c-contribxcrashreportxcrashreportcppcappinitinstance-contribxcrashreportdlgcpp-contribxcrashreportsendemailcppsendemail-contribxcrashreportsysteminfogateringcpp-multiwiniasupportini) | yes → own crash reporter |
| Crate | player concept; engine (class) | `Crate` (Building): a falling bubble-wrapped box dropped every `CrateDropRate` seconds; the team holding the strict Darwinian majority within 35 wu (`CRATE_SCAN_RANGE`) opens it; a crate left neutral on the ground for 60 s expires with no effect. | [04 §1](04_crates_powerups_and_weapons.md#1-crate-lifecycle) | yes → Supply crate |
| Crate drop rate / drop mode | player concept | Lobby options `CrateDropRate` (seconds between drops, 0 = none, default 40) and `CrateDropMode` (Random or Weighted placement). | [04 §1.2](04_crates_powerups_and_weapons.md#12-options-that-control-crates), [01 §3.2](01_game_modes.md#32-every-option-definition-39--38-shipped--1-debug) | no |
| Crate Mania | player concept | Outcome 19 `CrateRewardCrateMania` (immediate, only while fewer than 4 crates exist): 5–9 extra crates drop at once. | [04 §5.20](04_crates_powerups_and_weapons.md#520-crate-mania) | yes → Supply Storm |
| Crate reward enum | engine | `Crate::CrateReward*`, 31 outcomes in FULL (indices 0–30) plus The Beast (31) in DEBUG; `NumCrateRewards` sizes the per-level invalid-crate table. | [04 §2.1](04_crates_powerups_and_weapons.md#21-identity-build-gating-and-pool-membership) | no |
| CRC | abbreviation | Cyclic redundancy check; a map's identity is the CRC-32 (reflected `0xEDB88320`, initial value 0, no final XOR) of its level-file bytes plus one `0xFF`, listed in `code/mapcrc.h` by the `tools/MapCRC` tool. | [31 §10.2](31_platform_build_and_tools.md#102-crc-definition-c-contribsystemivlibmathfilecrccppfilecrc-readfiledata-contribunrarcrccppcrc), [29 §4.6](29_data_formats_reference.md#46-map-identity-crc-and-codemapcrch) | no |
| Credit lists (betatesters) | data format | `data/betatesters.txt` and `data/multiwinia_betatesters.txt`: name lists shown in credits; contain real people's names. | [29 §5.5](29_data_formats_reference.md#55-credit-name-lists-betatesterstxt-multiwinia_betatesterstxt) | yes → do not ship |
| CRLF / LF | abbreviation | Line-ending conventions (carriage return plus line feed, or line feed only); the text reader accepts both. | [29 §3.1](29_data_formats_reference.md#31-textreader-line-reader-and-tokeniser) | no |
| Crowd order | player concept | A move order given to the selected Darwinians (command `ORDERDGS`): they walk to the point as a loose group, optionally by direct route. | [06 §5.3](06_player_control_and_input.md#53-player-commands-and-their-effect) | no |
| CRT | abbreviation | The C run-time library (MSVC static multithreaded); its `rand()` and maths functions matter for determinism. | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model), [23 §17.2](23_terrain_and_landscape.md#172-crt-functions-and-how-risky-they-are) | no |
| CSV | abbreviation | Comma-separated values; used for generated tables under `_work/drafts/`. | [03 §2.1](03_buildings_and_world_objects.md#21-building-type-enum-all-75-values) | no |
| CTS | abbreviation | Capture the Statue, game type `GameTypeCaptureTheStatue`. | [01 §4.3](01_game_modes.md#43-capture-the-statue-gametypecapturethestatue-bfull) | yes → Relic Run |
| Cut-scene script | data format | `data/scripts/*.txt` (42 files) of script commands ([B:LEGACY] or [B:DEAD]). | [29 §6.2](29_data_formats_reference.md#62-cut-scene-scripts-blegacy), [30 §2.6](30_scripting_tutorial_and_cutscenes.md#26-script-file-register--one-row-per-datascripts-file) | yes → new scripts |

### D

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| D3D / D3D9 / D3DX | abbreviation | Direct3D, Microsoft's 3-D API; version 9 is the [B:D3D] back-end, D3DX its helper library. | [24 §1.5](24_rendering.md#15-direct3d-9-path-opengl-api-emulation-layer-bd3d) | no |
| D<nnn> / IP<nn> (register ids) | abbreviation | Entry identifiers of 41's registers: docs-vs-code discrepancy rows (D…) and IP replacement categories (IP01–IP50). | [41](41_risks_and_open_questions.md) | no |
| Dark Forest | player concept | Outcome 18 `CrateRewardDarkForest` (good and bad pools): 2–4 evil soul-eating trees near the target. | [04 §5.19](04_crates_powerups_and_weapons.md#519-dark-forest) | yes → Blight Grove |
| DArray | engine (class) | SystemIV dynamic array with per-slot used flags: an entry's index never changes and freed slots are reused. | [21 §5.3](21_simulation_and_entity_model.md#53-container-semantics-iteration-order-and-slot-re-use) | no |
| Darwin Research Associates | IP name | In-fiction corporation logo (`icons/darwin_research_associates.bmp`) of the [B:LEGACY] logo splash. | [30 §5.2](30_scripting_tutorial_and_cutscenes.md#52-logo-splash-showdarwinlogo-blegacy), [24 §4.25](24_rendering.md#425-loading-screen-frame-bfull-c-codeloading_screencpploadingscreenrenderframe) | yes → omit (product logo placeholder if needed) |
| Darwinia | IP name | Introversion's earlier single-player game whose engine and campaign code Multiwinia inherits (`DARWINIA_VERSION`, `darwinia_gl.exe`, `NET_DARWINIA_*`, `DarwiniaWindow`). | [00 §2.5](00_overview.md#25-what-is-not-in-the-shipped-game), [20 §1](20_architecture.md#1-scope-and-reading-guide) | yes → "legacy campaign" in prose, `<GAME_TITLE>` in identifiers |
| Darwinian | player concept; engine (class) | Entity type 12, the population unit of every mode: never built or ordered individually, it searches its own tasks (capture ports, open crates, carry statues, follow officers), fights with lasers and grenades; health 50, speed 20 wu/s. | [02 §4](02_units_and_entities.md#4-darwinian) | yes → Worker |
| Darwinians ("Multiwinians") | player concept | Outcome 6 `CrateRewardDarwinians` (basic, immediate): 50–99 Darwinians of the capturing team appear at the crate. | [04 §5.7](04_crates_powerups_and_weapons.md#57-darwinians-multiwinians) | yes → Reinforcements |
| DarwiniaWindow / DarwiniaButton | engine (class) | Darwinia-style window and button layer on top of Eclipse used by options, debug and editor windows. | [28 §3](28_ui_framework.md#3-darwinia-layer-darwiniawindow-and-darwiniabutton) | yes → `UiWindow`, `UiButton` |
| .dat (archive) | data format | A RAR archive packed from `data/` by the data build (`init.dat`, `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat`, `sounds.dat`, `sounds2.dat`), each decompressed whole into memory by `Resource::ParseArchive`; later archives override earlier ones, and a loose file under `data/` always beats every archive. | [29 §10.1](29_data_formats_reference.md#101-dat-archives-rar), [31 §8.7](31_platform_build_and_tools.md#87-the-archives--names-contents-load-order-override-rules) | yes → own packaging names |
| Data build pipeline | engine | `tools/build_multiwinia_data.bat` and Perl list tools that pack `data/` into the `.dat` archives per SKU using exclusion and inclusion lists. | [31 §8](31_platform_build_and_tools.md#8-data-build-pipeline-and-the-dat-archives) | no |
| Debug / DebugDirect3D | engine (build macro) | The two developer configurations (`darwinia_d_gl.exe`, `darwinia_d.exe`); unrelated to the `TARGET_DEBUG` product target, which is a separate source switch. | [31 §7.3](31_platform_build_and_tools.md#73-the-four-project-configurations--complete-compiler-and-linker-settings-c-targetsvs2005darwiniavcproj-configurations-block) | no |
| Debug menu / DebugKeyBindings | engine | 18 static debug entry points and the unreachable "Tools Menu". | [32 §3](32_debug_test_and_performance.md#3-debug-entry-points-and-the-debug-menu) | no |
| DEBUG_RENDER_ENABLED | engine (build macro) | Defined globally, so the debug drawing primitives and overlays are compiled into the shipped executable. | [32 §11.1](32_debug_test_and_performance.md#111-primitives-debug_render_enabled-defined-globally--compiled-into-full-c-codelibdebug_rendercpp-codelibpersisting_debug_rendercpp-codelibsphere_renderercpp-also-24-423) | no |
| Deform effect | engine | [B:D3D] screen-space distortion effect (`code/deform.cpp`) using HLSL shaders. | [24 §4.13](24_rendering.md#413-deform-effect-screen-space-distortion-bd3d-c-codedeformh-codedeformcpp) | no |
| Demo (demo version) | player concept | The runtime-gated limited product (no valid full key): restricted maps and modes, enforced by `IS_DEMO` and the metaserver's demo limits. | [09 §6.1](09_meta_systems.md#61-definitions-and-the-demo-limits), [22 §11.5](22_determinism_and_networking.md#115-demo-limitations-codenetworkdemo_limitations-c) | no |
| Demo limitations | engine | `code/network/demo_limitations.*`: limits on what a demo client may host or join, filled from the metaserver data item `Demo2Limits` or built-in defaults. | [22 §11.5](22_determinism_and_networking.md#115-demo-limitations-codenetworkdemo_limitations-c) | no |
| Desync (out of sync) | player concept | Divergence of a client's simulation from its peers, detected by the per-frame sync byte; the affected client is reported out of sync and the game pauses for it, with no recovery. | [22 §9](22_determinism_and_networking.md#9-sync-checking-and-desync-handling) | no |
| Desync dump | data format | Files `syncerror-…` and `syncdiff-…` written on a sync error for forensics. | [29 §11.4](29_data_formats_reference.md#114-desync-dump-files), [32 §13.2](32_debug_test_and_performance.md#132-forensics-produced-on-desync-c-codenetworkclienttoservercppclienttoserverprocessserverletters-clienttoserversendiframe-codenetworkiframecppiframecalculatediff-codenetworksyncdiffh) | no |
| Direct control | player concept | Steering a single unit or building yourself (squad, armour, harvester, engineer, gun turret aim, radar dish aim) through the continuous `TeamControls` packet. | [06 §6](06_player_control_and_input.md#6-direct-control-of-units-and-buildings) | no |
| Directory | engine (class) | SystemIV registry-like container (`contrib/systemiv/lib/tosser/directory.h`): a named node with typed named fields and child nodes, serialised to bytes; every network and metaserver message is one. | [22 §4.2](22_determinism_and_networking.md#42-message-container--directory-tosser), [29 §3.5](29_data_formats_reference.md#35-directory-serialisation-network-wire-container) | no |
| Directory wire format | data format | Serialisation of a `Directory` (named node, typed fields, children) used for every network letter, metaserver message, `.mwm` and server history. | [29 §3.5](29_data_formats_reference.md#35-directory-serialisation-network-wire-container), [22 §4.2](22_determinism_and_networking.md#42-message-container--directory-tosser) | no |
| Display list | engine | OpenGL compiled command list used for shapes, trees and the global internet (`USE_DISPLAY_LISTS`); emulated in the D3D build. | [25 §2.11](25_textures_and_assets.md#211-shapes-cache-and-display-lists), [24 §4.5](24_rendering.md#45-shape-rendering-shp-meshes-c-codelibshapeh-codelibshapecpp) | no |
| DisplayScreen | engine (class) | [B:LEGACY] cosmetic hologram screen. | [03 §12.2](03_buildings_and_world_objects.md#122-displayscreen) | no |
| Distributor and partner names | IP name | Ambrosia Software (`AMBR` key prefix), Buka (`origin` tag), eGames and Cinemaware, Steam app id 1520, Xbox Live Arcade, Logitech: third-party names in keys, splash art and packaging. | [31 §14.1](31_platform_build_and_tools.md#141-repository-root-10-files), [22 §12.5](22_determinism_and_networking.md#125-authentication-keys-c-contribsystemivlibmetaserverauthentication-contribsystemivlibmathhashcpphashdata) | yes → omit |
| DLL | abbreviation | Windows dynamic-link library (for example the delay-loaded `xinput1_3.dll`). | [06 §11.6.6](06_player_control_and_input.md#1166-xinput--driver-5-present-only-if-xinput1_3dll-loads) | no |
| Domination | player concept | Game type `GameTypeSkirmish` (enum 0): capture and hold spawn points; a team owning none for 30 consecutive mode seconds (after the first 30 game seconds) is eliminated; last team standing or best score at time-up wins. | [01 §4.1](01_game_modes.md#41-domination-gametypeskirmish-bfull) | yes → Spawn Control |
| Dr Sepulveda | IP name | Darwinia's mentor character (portraits, dialogue, help); dead code in Multiwinia apart from leftover phrase ids. | [30 §6](30_scripting_tutorial_and_cutscenes.md#6-dr-sepulveda-dialogue-system-bdead) | yes → Advisor (drop) |
| Draw | player concept | A tied result at time-up with sudden death off; declared through `DeclareWinner` with no winning team. | [01 §2.11](01_game_modes.md#211-ending-declarewinnert-and-the-end-of-game-sequence) | no |
| Drop zone / statue spawn zone | player concept | CTS `MultiwiniaZone`s: team-owned zones that score delivered statues, and neutral zones where new statues appear. | [03 §8.1](03_buildings_and_world_objects.md#81-multiwiniazone), [01 §4.3](01_game_modes.md#43-capture-the-statue-gametypecapturethestatue-bfull) | yes → Relic Base |
| DropShip | engine (class) | Entity type 26: small transport that drops 20 Darwinians above a target; reachable only from an unshipped level option ([B:LEGACY]). | [02 §28](02_units_and_entities.md#28-dropship) | no |
| DSP | abbreviation | Digital signal processing; the audio filter chain configured by `effects.txt`, compiled but off in the shipped build (`USE_SOUND_DSPEFFECTS` commented out). | [27 §12](27_audio_engine.md#12-dsp-filter-chain), [10 §10](10_audio_design.md#10--audio-design) | no |
| DSP filter (DspBlueprint) | engine (class) | One of 14 audio filter types defined by `effects.txt`; the chain is compiled but switched off in FULL. | [27 §12](27_audio_engine.md#12-dsp-filter-chain), [10 §10](10_audio_design.md#10--audio-design) | no |
| dt | abbreviation | The time step of one simulation update: `SERVER_ADVANCE_PERIOD`, a macro for `0.1 × m_gameSpeed` game seconds (`GameTimer::GetServerAdvancePeriod`), applied once per tick to every slice-advanced object. | [20 §10.3](20_architecture.md#103-client-tick-and-slices), [21 §8.1](21_simulation_and_entity_model.md#81-integration-model-and-time-base) | no |
| DustBall | engine (class) | Dust-storm particle and vortex of the level effect `dustwind`. | [04 §6.20](04_crates_powerups_and_weapons.md#620-dustball) | no |
| DX12 | abbreviation | DirectX 12, the target graphics API of the new game. | [24 §9](24_rendering.md#9-direct3d-12-mapping-notes-per-component), [40 §3](40_carry_forward_assessment.md#3-obsolete-technology-mapping) | no |
| DynamicBase / DynamicHub / DynamicNode | engine (class) | [B:LEGACY] generic scored objective network (hub with linked nodes). | [03 §12.20](03_buildings_and_world_objects.md#1220-dynamicbase) | no |

### E

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| EBNF | abbreviation | Extended Backus-Naur form, the grammar notation used in 29. | [29 §1.2](29_data_formats_reference.md#12-grammar-notation) | no |
| Eclipse | engine | Introversion's in-house window and button framework (`contrib/eclipse`, `EclWindow`, `EclButton`, `eclipse.cpp`): registration, z-order, hit tests, mouse capture, focus, tooltips. Live in FULL. | [28 §2](28_ui_framework.md#2-eclipse-core-contribeclipse) | yes → UI core |
| EclWindow / EclButton | engine (class) | Eclipse's window and button base classes. | [28 §2.1](28_ui_framework.md#21-data-model) | no |
| Effect (`WorldObject::Effect*`) | engine | Type ids of non-entity, non-building world objects kept in `Location::m_effects` (lasers excluded): grenades, markers, shockwaves, order markers, weather, zombies. | [21 §2.6](21_simulation_and_entity_model.md#26-other-worldobject-derived-classes-effects-weapons-spirits-order-markers-and-unit-subclasses--32-classes), [21 §5.2](21_simulation_and_entity_model.md#52-every-simulation-container) | no |
| EffectProcessor | engine (class) | [B:DEBUG] POKEY envelope sequencer used by the sound editor; unrelated to visual effects. | [26 §9.5](26_animation_and_vfx.md#95-effectprocessor-is-unrelated-c-codeeffect_processorh-codeeffect_processorcpp-codeappcppappinitialise) | yes → drop |
| effects.txt | data format | `data/effects.txt`: the 14 DSP filter blueprints (audio, not visuals); bound by row position. | [29 §7.5](29_data_formats_reference.md#75-effectstxt--dsp-filter-blueprints-audio), [26 §9](26_animation_and_vfx.md#9-effectstxt--what-it-actually-configures) | no |
| Egg | engine (class) | Entity type 5: small Virii egg that hatches 4 Virii 15 s after a spirit is placed in it. | [02 §15](02_units_and_entities.md#15-egg) | no (generic) |
| Eggs | player concept | Outcome 16 `CrateRewardEggs`: 2–4 triffid eggs owned by the task owner that hatch monsters. | [04 §5.17](04_crates_powerups_and_weapons.md#517-eggs) | yes → Brood |
| Elimination | player concept | Two notions: the hard rules flag set by `Multiwinia::EliminateTeam` (Domination, Blitzkrieg, Tank Battle), and the soft `IsEliminated` query (KOTH, CTS: no spawn points and fewer than 50 Darwinians) that drives retribution. `Team::Eliminate` additionally kills the team's units. | [01 §2.7](01_game_modes.md#27-elimination) | no |
| EMA | abbreviation | Exponential moving average, as in fuel and power throughput smoothing `t = 0.8·t + 0.2·n`. | [03 §6.6](03_buildings_and_world_objects.md#66-generator) | no |
| Engineer | player concept; engine (class) | Entity type 2: hovering drone that collects spirits one at a time and returns each to a friendly spawn point as a Darwinian; also reprograms Darwinia buildings ([B:LEGACY]). | [02 §10](02_units_and_entities.md#10-engineer) | yes → Builder |
| Engineer (crate outcome) | player concept | Outcome 2 `CrateRewardEngineer` (basic, needs spawn points): a placed hovering spirit collector. | [04 §5.3](04_crates_powerups_and_weapons.md#53-engineer) | yes → Builder |
| Entity | engine (class) | Abstract base of all units: `Entity::Type` (28 values incl. sentinel), stats from `stats.txt`, health, damage, death into a spirit, water, teleports and team-coloured shapes. | [02 §3.4](02_units_and_entities.md#34-entity--base-class-of-all-units), [02 §1.1](02_units_and_entities.md#11-every-entitytype-value) | no |
| EntityBlueprint | engine (class) | Static table loaded from `data/stats.txt`: row k gives Health, Speed and Rate of entity type k (positional, name column ignored). | [02 §3.5](02_units_and_entities.md#35-entityblueprint--the-statstxt-table), [29 §7.1](29_data_formats_reference.md#71-statstxt--per-entity-type-base-stats) | no |
| EntityGrid / EntityGridCache | engine (class) | Per-team uniform grid of entity ids (32 wu cells) kept up to date as entities move; answers neighbour and enemy-count queries. The cache memoises repeated queries within a tick. | [21 §6.2](21_simulation_and_entity_model.md#62-entitygrid), [21 §6.3](21_simulation_and_entity_model.md#63-entitygridcache) | no |
| EntityLeg / EntityFoot | engine (class) | Procedural two-bone leg and foot helpers used by the Spider ([B:FULL]) and the Tripod ([B:LEGACY]). | [02 §20](02_units_and_entities.md#20-entityleg), [26 §5.1](26_animation_and_vfx.md#51-entityleg--legged-locomotion-with-two-bone-ik-c-codeworldobjectentity_legh-codeworldobjectentity_legcpp) | no |
| EOF | abbreviation | End of file. | [29 §3.1](29_data_formats_reference.md#31-textreader-line-reader-and-tokeniser) | no |
| EruptionMarker | engine (class) | Atmospheric volcano vent erupting particles at synced-random intervals; no damage. | [03 §7.7](03_buildings_and_world_objects.md#77-eruptionmarker) | no |
| EscapeRocket (escape rocket) | player concept; engine (class) | Each team's Rocket Riot rocket: refuels, loads 100 passengers, counts down and launches; the first above y = 600 wins. | [03 §8.12](03_buildings_and_world_objects.md#812-escaperocket) | yes → Launch Vessel |
| EULA | abbreviation | End-user licence agreement (root `EULA.txt`); Introversion text, not to be reused. | [31 §14.1](31_platform_build_and_tools.md#141-repository-root-10-files) | yes → own licence |
| Event (sound event) | engine | A named moment (`Create`, `Die`, `Attack`) that code triggers on an object type; `data/sounds.txt` binds it to sample groups. | [10 §2.2](10_audio_design.md#22-the-four-trigger-entry-points) | no |
| Evilinians | IP name | Player-facing name of the monster team (`multiwinia_team_virus`). | [01 §2.5](01_game_modes.md#25-teams-slots-start-positions-colours-co-op-and-assault-sides) | yes → Rogue faction |
| ExplosionManager | engine (class) | Shape-shattering explosions: a mesh is broken into tumbling triangles that fly and fade (`code/explosion.cpp`). | [26 §8](26_animation_and_vfx.md#8-explosions--shape-shattering) | no |

### F

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Factory (building factory) | engine | `Building::CreateBuilding(type or name)`: the switch that instantiates the concrete class, forces `m_isGlobal` for six types and skips blocked types when creating by name. | [03 §3.11](03_buildings_and_world_objects.md#311-factory-name-table-and-type-ids) | no |
| FastDArray | engine (class) | `DArray` with a free list, so insertion is O(1); used for entities, effects and spirits. | [21 §5.3](21_simulation_and_entity_model.md#53-container-semantics-iteration-order-and-slot-re-use) | no |
| FeedingTube | engine (class) | [B:LEGACY] purely visual link between two focus markers. | [03 §12.19](03_buildings_and_world_objects.md#1219-feedingtube) | no |
| FenceSwitch | engine (class) | [B:LEGACY] 4-port switch toggling laser fences; placed on no map. | [03 §12.37](03_buildings_and_world_objects.md#1237-fenceswitch) | no |
| FIFO / LIFO | abbreviation | First-in-first-out and last-in-first-out ordering, used when describing queues and lists. | [21 §5.3](21_simulation_and_entity_model.md#53-container-semantics-iteration-order-and-slot-re-use) | no |
| FileSystem (SystemIV) | engine (class) | SystemIV archive-aware file system: compiled but unused. | [31 §8.10](31_platform_build_and_tools.md#810-systemiv-filesystem-compiled-unused-c-contribsystemivlibfilesysfile_systemcpp) | no |
| Fireball | engine (class) | Flame-turret projectile that grows and sets Darwinians of any team on fire. | [04 §6.10](04_crates_powerups_and_weapons.md#610-fireball) | no |
| Fixed-function pipeline | engine | OpenGL 1.1–1.3 rendering without programmable shaders (immediate mode, display lists, client vertex arrays, texture environment modes), the style the whole renderer is written in; emulated with HLSL in the D3D build. | [24 §1.3](24_rendering.md#13-native-opengl-path-bfull), [40 §3](40_carry_forward_assessment.md#3-obsolete-technology-mapping) | no |
| Flag | engine (class) | Cloth-flag render primitive (6 × 6 vertex grid) used by Blitzkrieg zones, armour and officers; presentation only. | [03 §8.2](03_buildings_and_world_objects.md#82-flag), [26 §5.8](26_animation_and_vfx.md#58-cloth-flags--flag-bfull-c-codeworldobjectflagh-codeworldobjectflagcppflaginitialise-flagrender-flagrayhit-owners-codeworldobjectofficercppofficerrenderflag-codeworldobjectarmourcpp-codeworldobjectmultiwiniazonecpp) | no |
| Flag (Blitzkrieg) | player concept | A Blitzkrieg `MultiwiniaZone` captured by Darwinian majority; zones are linked into a graph, and a home flag is each team's base. | [03 §8.1](03_buildings_and_world_objects.md#81-multiwiniazone), [01 §4.6](01_game_modes.md#46-blitzkrieg-gametypeblitzkreig-bfull) | yes → Flag post / Home post |
| FlameGrenade | engine (class) | Napalm bomb dropped by napalm bombers: ignites every Darwinian within 62.5 wu and damages trees and triffids; no `Bang`. | [04 §6.4](04_crates_powerups_and_weapons.md#64-flamegrenade) | no |
| FLOAT_NUMERICS | engine (build macro) | Global switch that makes SystemIV's `Fixed` numeric type a wrapper around `double`; the fixed-point alternatives `USE_FIXED` and `FIXED64_NUMERICS` are never defined. | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model) | no |
| Font atlas | data format | Bitmap font pages under `data/textures/unicode/` (fonts `speccy`, `editor`, `square`, including CJK pages) packed into `language.dat`. | [25 §2.15](25_textures_and_assets.md#215-fonts-and-atlases), [25 §3.2](25_textures_and_assets.md#32-datatexturesunicode-font-atlases) | yes → licensed open fonts |
| Formation | player concept | A block of up to 90 Darwinians marching in rows of 6 to 15 in front of an officer, which fight as one; set with the officer's formation order. | [06 §5.4](06_player_control_and_input.md#54-formation-geometry-and-membership), [02 §5.5](02_units_and_entities.md#55-officer--orders-and-formation-api-player-control) | no |
| FOV | abbreviation | Field of view of the camera projection. | [24 §5.1](24_rendering.md#51-projection-and-view-c-coderenderercpprenderersetupprojmatrixfor3d-renderframe-codecameracppcamerasetupprojectionmatrix-camerasetupmodelviewmatrix), [06 §9.4](06_player_control_and_input.md#94-common-components) | no |
| FP / FPU / x87 | abbreviation | Floating point, the floating-point unit, and the x87 instruction set the shipped Win32 build uses (53-bit precision control, no SSE2). | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model) | no |
| FPS | abbreviation | Frames per second (render rate), distinct from the 10 Hz simulation tick. | [20 §10.5](20_architecture.md#105-render-rate), [32 §3.5](32_debug_test_and_performance.md#35-fps-and-entity-count-overlay-c-coderenderercpprendererrenderer-rendererrenderframe-rendererfpsmeteradvance) | no |
| FRAND / SFRAND | engine | Marker macros that draw synced random vector components in a fixed x, y, z order inside `Vector3` constructors, avoiding unspecified argument evaluation order. | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code) | no |
| Front end | player concept | All screens outside a match: title, mode and map selection, lobby, server browser, Help and Options, credits; driven by `GameMenu` and `GameMenuWindow` pages. | [09 §2](09_meta_systems.md#2-front-end-architecture-and-full-screen-flow) | no |
| FSAA / MSAA | abbreviation | Full-scene and multisample anti-aliasing; the window can request multisampling, and a source comment ties the D3D deform path to FSAA. | [31 §3.11](31_platform_build_and_tools.md#311-vsync-multisampling-and-gamma--summary), [24 §8.1](24_rendering.md#81-every-render-related-preference-c) | no |
| FTP | abbreviation | File Transfer Protocol; the dead benchmark upload, and the name of the in-game `FTPManager` blob transfer over the letter channel. | [22 §11.4](22_determinism_and_networking.md#114-ftp-manager-codenetworkftp_manager--reliable-blob-transfer-over-the-letter-channel-c), [32 §8](32_debug_test_and_performance.md#8-benchmark-mode) | no |
| FTPManager | engine (class) | Reliable blob transfer over the letter channel (`FILESEND` letters); in the baseline it only moves IFrame sync snapshots between client and server after a desync. | [22 §11.4](22_determinism_and_networking.md#114-ftp-manager-codenetworkftp_manager--reliable-blob-transfer-over-the-letter-channel-c) | no |
| Fuel (Rocket Riot) | player concept | Pressure passed down the chain solar panel → power line → fuel generator → pipes → fuel station → escape rocket; the rocket needs 95 % to load passengers. | [03 §8.8](03_buildings_and_world_objects.md#88-fuelbuilding), [01 §4.5](01_game_modes.md#45-rocket-riot-gametyperocketriot-bfull) | no |
| FuelBuilding | engine (class) | Base of the Rocket Riot fuel chain (`m_fuelLink`, current fuel level). | [03 §8.8](03_buildings_and_world_objects.md#88-fuelbuilding) | no |
| FuelGenerator (fuel generator) | engine (class) | Converts power surges into fuel pressure (surges / 20). | [03 §8.9](03_buildings_and_world_objects.md#89-fuelgenerator) | no |
| FuelPipe | engine (class) | Relay segment of the fuel chain. | [03 §8.10](03_buildings_and_world_objects.md#810-fuelpipe) | no |
| FuelStation (fuel station) | engine (class) | Boarding gate of a rocket and the last fuel hop; sends idle own Darwinians aboard while the rocket loads. | [03 §8.11](03_buildings_and_world_objects.md#811-fuelstation) | no (generic; "fuel depot") |
| Futurewinian team | player concept | Team slot 5, the faction dropped by the Space Ship outcome; its Darwinians use the `futurewinian.bmp` sprite. | [01 §2.5](01_game_modes.md#25-teams-slots-start-positions-colours-co-op-and-assault-sides), [02 §23](02_units_and_entities.md#23-spaceship) | yes → Visitor faction |
| Futurewinians | IP name | Player-facing name of team 5 (`multiwinia_team_future`) and its sprite. | [02 §23](02_units_and_entities.md#23-spaceship) | yes → Visitor faction |

### G

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Game mode (App) | engine | `App::m_gameMode` ∈ {GameModeNone, GameModePrologue, GameModeCampaign, GameModeMultiwinia}; only Multiwinia is reachable in FULL. | [20 §4.4](20_architecture.md#44-enumerations-declared-in-app) | no |
| Game options | player concept | The lobby parameters of a mode defined in `data/multiwinia.txt` (39 definitions: time limit, handicap, crates, retribution, score mode and others), replicated to all peers. | [01 §3](01_game_modes.md#3-the-option-system), [09 §4](09_meta_systems.md#4-game-setup-options) | no |
| Game speed | player concept; engine | `GameTimer::m_gameSpeed`, the multiplier on game time per tick (1 = normal); changed deterministically by pause, the Slow Down and Speed Up crates, meteor slow motion and the end of a match. | [20 §10.7](20_architecture.md#107-gametimer-api-and-semantics), [22 §2.6](22_determinism_and_networking.md#26-pause-and-game-speed) | no |
| Game type | player concept; engine | One of the seven `Multiwinia::GameType*` values (Skirmish = Domination, KingOfTheHill, CaptureTheStatue, Assault, RocketRiot, Blitzkreig, TankBattle); selects the per-mode `Advance*` rule function and the option blueprint in `data/multiwinia.txt`. | [01 §1.1](01_game_modes.md#11-the-seven-game-types) | yes → mode names in §3 |
| game.txt / locations.txt | data format | [B:LEGACY] Darwinia campaign world definition and world-map location positions. | [29 §4.4](29_data_formats_reference.md#44-gametxt-family--campaign-world-definition-blegacy), [29 §4.5](29_data_formats_reference.md#45-locationstxt--world-map-positions-blegacy) | no |
| GameCursor | engine (class) | The in-world cursor: highlights, selection circle, placement previews and context icons. | [06 §8](06_player_control_and_input.md#8-game-cursor-highlights-and-world-markers), [24 §4.19](24_rendering.md#419-mouse-cursor-highlights-and-placement-previews-pass-20-c-codegamecursorcpp) | no |
| GameMenu / GameMenuWindow | engine (class) | The front-end controller (map catalogue, menu lifetime) and the full-screen paged screen whose pages are numbered (for example `PageMain` 0, `PageGameSetup` 6, `PageWaitForGameToStart` 37). | [09 §2.1](09_meta_systems.md#21-lifecycle-gamemenu-controller-and-gamemenuwindow-the-screen), [28 §6](28_ui_framework.md#6-gamemenuwindow-page-framework) | no |
| GAMEOPTION_* | engine | Ids of lobby options outside the per-mode blueprint that travel as network game options (for example `GAMEOPTION_BASICCRATES`). | [22 §5.3](22_determinism_and_networking.md#53-lobby-game-option-ids-gameoption_-codenetworknetwork_definesh), [09 §4.3](09_meta_systems.md#43-lobby-level-options-outside-the-blueprint) | no |
| GameOptionsWindow | engine (class) | Full-screen options-style page used by Help and Options, settings and in-game menus. | [28 §5.1](28_ui_framework.md#51-gameoptionswindow--the-full-screen-page-c-codeinterfacemainmenuscppgameoptionswindow) | no |
| GameTimer (g_gameTimer) | engine (class) | Network-safe game clock (`code/gametimer.h`): game time, game speed with requested blends, `SERVER_ADVANCE_PERIOD`, `GetNetworkTime`. | [20 §10.7](20_architecture.md#107-gametimer-api-and-semantics) | no |
| GDF | abbreviation | Windows Vista Game Definition File compiled into the executable for Game Explorer ([B:DEAD] Vista integration). | [31 §7.5](31_platform_build_and_tools.md#75-resource-scripts-compiled-into-the-exe-c) | yes → own ids |
| Generator | engine (class) | [B:LEGACY] Darwinia power source enabled by a control tower. | [03 §6.6](03_buildings_and_world_objects.md#66-generator) | no |
| Gesture recognition | engine | Mouse-gesture classifier trained from `gestures.txt` that ran tasks in Darwinia; [B:LEGACY] in Multiwinia. | [06 §7](06_player_control_and_input.md#7-gesture-recognition) | yes → drop |
| GESTURE_EDITOR / AVI_GENERATOR | engine (build macro) | [B:DEBUG] switches for the gesture training window and the AVI video grabber. | [32 §5.4](32_debug_test_and_performance.md#54-grabber-window-avi-capture-bdebug-non-functional-c-codeinterfacegrabber_windowcpp-codelibavi_generatorcpp-codelibhi_res_timecpp), [32 §5.5](32_debug_test_and_performance.md#55-gesture-editor-bdead-c-codeinterfacegesture_windowcpp-codegesture_demo_win32cpp) | no |
| gestures.txt / gesture demos | data format | Gesture-classifier training samples, parsed at start-up but unused in play; recorded demo strokes are [B:DEAD]. | [29 §7.2](29_data_formats_reference.md#72-gesturestxt--gesture-classifier-training-set), [29 §7.3](29_data_formats_reference.md#73-gesture_demostxt--recorded-gesture-strokes-bdead) | yes → drop |
| GetNetworkTime / GetHighResTime | engine | Game time (`GameTimer::GetGameTime`, deterministic) versus wall-clock time (`GetHighResTime`, QPC-based, not deterministic). | [20 §10.1](20_architecture.md#101-clocks), [31 §4.1](31_platform_build_and_tools.md#41-hi_res_time-api-and-baseline-implementation-c-codelibhi_res_timeh-codelibhi_res_timecpp) | no |
| GL | abbreviation | OpenGL, the baseline renderer API (fixed-function pipeline, display lists). | [24 §1.3](24_rendering.md#13-native-opengl-path-bfull) | no |
| GL job (main-thread hand-off) | engine | A `Job` a worker thread queues on the loading screen when it needs the GPU; the main thread runs it and the worker waits. | [20 §11.4](20_architecture.md#114-hand-off-to-the-main-thread) | no |
| Global building | engine | A building with `m_isGlobal` set (forced for RadarDish, ControlTower, TrunkPort, Incubator, DynamicHub, FenceSwitch), mirrored as a `GlobalBuilding` record (team, online flag, link) in `g_app->m_globalWorld`: the Darwinia campaign's cross-level persistence, mostly inert in Multiwinia. | [03 §3.13](03_buildings_and_world_objects.md#313-global-buildings) | no |
| GlobalResearch | engine | The research table (`GlobalResearch::Type*`, one level per type); fixed in Multiwinia, its type ids double as task types. | [02 §2.3](02_units_and_entities.md#23-research-levels-are-fixed-in-multiwinia), [06 §3.3](06_player_control_and_input.md#33-task-types--complete-enumeration-29) | no |
| GlobalWorld | engine (class) | Darwinia's world-map object; in Multiwinia only a holder of the local team id and research levels ([B:LEGACY] otherwise). | [20 §8.4](20_architecture.md#84-location-globalworld-and-multiwinia) | no |
| GodDish | engine (class) | [B:LEGACY] script-driven temple dish that spawns Spam. | [03 §12.23](03_buildings_and_world_objects.md#1223-goddish) | no |
| Good / bad outcome | player concept | Every opened crate resolves once: with probability 0.9 a good outcome for the opening team (`Crate::CaptureCrate`, uniform over the valid good types), otherwise a bad outcome (`Crate::RunUncapturedCrate`) created for the monster team (Space Ship: the Futurewinian team). Basic-crates-only games always take the good branch. | [04 §3](04_crates_powerups_and_weapons.md#3-probability-tables) | no |
| GOTO (order) | abbreviation | The officer order (state `OrderGoto`) that turns an officer into a signpost redirecting idle Darwinians that pass within 100 wu toward the order point; sent as network command `ORDERGOTO` (`coa`). | [06 §5.2](06_player_control_and_input.md#52-order-model), [02 §5.5](02_units_and_entities.md#55-officer--orders-and-formation-api-player-control) | yes → Waypoint beacon |
| GPU | abbreviation | Graphics processing unit. | [26 §13](26_animation_and_vfx.md#13-re-implementation-notes-for-a-modern-gpu-vfx-system-directx-12) | no |
| Gq/s | IP name | Fictional output unit ("gigaquads per second") in Darwinia generator objective text. | [03 §6.6](03_buildings_and_world_objects.md#66-generator) | yes → units/s |
| Grace period | player concept | The frozen pre-match phase (game speed 0) during which every human team must press Ready; once all are ready a 3-second countdown (`GRACE_TIME`) runs and un-readying restarts it. CPU teams are never waited for. | [01 §2.4](01_game_modes.md#24-grace-period-readiness-and-countdown) | no |
| GRACE_TIME | engine (build macro) | `#define` of 3.0 s: length of the all-ready countdown that ends the grace period (20.0 s under `TESTBED_ENABLED`). | [01 §2.4](01_game_modes.md#24-grace-period-readiness-and-countdown) | no |
| Grenade | engine (class) | Darwinian and squad grenade: 3.0 s fuse, power 25 → `Bang(pos, 12.5, 50, team)`; mortar shells explode on impact. | [04 §6.3](04_crates_powerups_and_weapons.md#63-grenade) | no |
| Guide grid | data format | Optional coarse grid of 8-bit heights in a tile definition that pins the fractal generator to a designed shape. | [23 §6](23_terrain_and_landscape.md#6-guide-grid-encoding) | no |
| Gun Turret / Rocket Turret / Flame Turret (crate outcomes) | player concept | Outcomes 10, 24, 25: a placed `GunTurret` of type Standard (0), Rocket (1) or Flame (3), built neutral and captured by manning; Rocket Turret is invalid in Assault. | [04 §5.11](04_crates_powerups_and_weapons.md#511-gun-turret-rocket-turret-flame-turret) | no (generic) |
| GunTurret (gun turret) | player concept; engine (class) | 4-port manned weapon emplacement (types Standard, Rocket, Mortar, Flame, Subversion via the `state` field) owned by whoever mans it; fires automatically and can be aimed by its owner. | [03 §5.1](03_buildings_and_world_objects.md#51-gunturret) | no (generic) |
| GunTurretTarget | engine (class) | Aim-point object of a gun turret that Darwinians flee in single-player only. | [04 §6.21](04_crates_powerups_and_weapons.md#621-gunturrettarget) | no |

### H

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Handicap | player concept | Lobby option (0–90 %, default 90) multiplying the spawn output of a team that owns fewer than two thirds of the other teams' average spawn points, by 1 + H/100 in every reachable case. | [01 §2.8](01_game_modes.md#28-handicap), [05 §3.2](05_economy_and_population.md#32-handicap) | yes → Catch-up bonus |
| Harvester | player concept; engine (class) | Entity type 20: flying collector that sucks up floating spirits of any team and converts each into a Darwinian of its own team at a friendly spawn point, outside the cap. | [02 §9](02_units_and_entities.md#9-harvester) | yes → Collector |
| Harvester (crate outcome) | player concept | Outcome 1 `CrateRewardHarvester` (advanced, needs spawn points): a placed flying spirit collector. | [04 §5.2](04_crates_powerups_and_weapons.md#52-harvester) | yes → Collector |
| Harvesting | player concept | Collection of floating spirits of any team by a Harvester, which turns each into a Darwinian of its own team at a friendly spawn point, bypassing the population cap. | [05 §5.3](05_economy_and_population.md#53-who-can-collect-a-spirit), [02 §9](02_units_and_entities.md#9-harvester) | no |
| HashTable | engine (class) | SystemIV string-keyed hash table (used for example by the FTP manager's transfer lists). | [20 §3.1](20_architecture.md#31-module-inventory) | no |
| Help overlays | player concept | Contextual in-match hints (`MultiwiniaHelp`) that replace Darwinia's control-help system: short prompts about selection, promotion, crates and modes. | [09 §8.2](09_meta_systems.md#82-in-game-contextual-help-multiwiniahelp-outside-tutorials-bfull), [06 §10.4](06_player_control_and_input.md#104-live-multiwinia-hint-overlays-multiwiniahelp--the-replacement) | no |
| Hill (zone) | player concept | A KOTH `MultiwiniaZone` ring that scores for the team with the most Darwinians inside it. | [03 §8.1](03_buildings_and_world_objects.md#81-multiwiniazone) | yes → Control Zone |
| HistoricWayPoint | engine (class) | Breadcrumb position recorded by an insertion squad so followers walk the leader's path. | [02 §7.7](02_units_and_entities.md#77-historicwaypoint--squad-breadcrumb) | no |
| HLSL | abbreviation | High Level Shading Language; the 14 files in `data/shaders/` used only by the [B:D3D] path. | [24 §6](24_rendering.md#6-shaders-datashaders--all-bd3d), [29 §9.1](29_data_formats_reference.md#91-hlsl-shaders-datashaders-bd3d) | no |
| HLSL shader files | data format | `data/shaders/*` (14 files: fixed-function emulation, deform, water mesh waves, `globals.h`), compiled at run time by the [B:D3D] path. | [29 §9.1](29_data_formats_reference.md#91-hlsl-shaders-datashaders-bd3d), [24 §6](24_rendering.md#6-shaders-datashaders--all-bd3d) | yes → re-author in HLSL SM6 from 24 |
| Host / client | player concept | The host runs the `Server` and a `ClientToServer` in one process; every other player is a client of that server. All simulation runs on every client. | [22 §1.2](22_determinism_and_networking.md#12-peer-roles) | no |
| Hot Feet | player concept | Outcome 9 `CrateRewardHotFeet`: for up to 30 s own Darwinians within 70 wu run at double speed leaving a fire trail. | [04 §5.10](04_crates_powerups_and_weapons.md#510-hot-feet) | yes → Speed Boost |
| HP | abbreviation | Health points, the unit of `m_stats[Health]` and of damage. | [02 §2.6](02_units_and_entities.md#26-damage-model) | no |
| HUD | abbreviation | Heads-up display: the in-match overlay with scores, timers, task slots and messages. | [01 §2.12](01_game_modes.md#212-the-match-hud) | no |
| HW3D | abbreviation | Hardware 3-D sound mixing (DirectSound hardware buffers); an option exists but `USE_SOUND_HW3D` is off. | [27 §11.2](27_audio_engine.md#112-directsound-soundlibrary3ddirectsound--the-live-back-end-bfull) | no |

### I

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| [I] | evidence tag | Inferred: derived from confirmed facts by stated reasoning (arithmetic, absence of callers, reachability); never presented as read directly. | [00 Open questions and unknowns](00_overview.md#open-questions-and-unknowns) | no |
| IEEE 754 / binary32 | abbreviation | The floating-point standard; binary32 is single precision, to which every `iv_*` maths result is rounded. | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model) | no |
| IFrame | engine (class) | `code/network/iframe.cpp`: produces the per-frame sync byte (in FULL the next synced-RNG draw, `255 × syncfrand(1)`); in debug and beta builds a SHA-1 world snapshot that can be diffed. | [22 §7](22_determinism_and_networking.md#7-iframes) | no |
| IK | abbreviation | Inverse kinematics; the two-bone leg solver of `EntityLeg`. | [26 §5.1](26_animation_and_vfx.md#51-entityleg--legged-locomotion-with-two-bone-ik-c-codeworldobjectentity_legh-codeworldobjectentity_legcpp) | no |
| IME | abbreviation | Input method editor for typing non-Latin text. | [28 §10.6](28_ui_framework.md#106-text-entry-of-non-latin-text-c) | no |
| INCLUDE_CRATES_BASIC / INCLUDE_CRATES_ADVANCED | engine (build macro) | Compile the 7 basic crate outcomes and the 24 advanced ones respectively; both are on in FULLGAME. | [04 §1.1](04_crates_powerups_and_weapons.md#11-build-gating) | no |
| INCLUDE_THEBEAST | engine (build macro) | Defined inside `code/worldobject/crate.h` only under `TARGET_DEBUG`; compiles The Beast crate outcome, which makes it [B:DEBUG]. | [04 §5.30](04_crates_powerups_and_weapons.md#530-the-beast-bdebug) | no |
| INCLUDE_TUTORIAL | engine (build macro) | Compiles the two live Multiwinia tutorials and the Tutorials page; on in FULLGAME. | [30 §3.1](30_scripting_tutorial_and_cutscenes.md#31-overview-and-setup) | no |
| INCLUDEGAMEMODE_* | engine (build macro) | One macro per mode (`_DOMINATION`, `_KOTH`, `_CTS`, `_ROCKET`, `_ASSAULT`, `_BLITZ`, `_TANKBATTLE`, `_PROLOGUE`, `_CAMPAIGN`) that compiles that mode's code; FULLGAME defines the first six. | [01 §1.2](01_game_modes.md#12-build-gating-and-runtime-permission), [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line) | no |
| Incubator | engine (class) | Converts stored spirits into Darwinians of the owner's team; only Darwinia missions and sandbox maps place it. | [03 §11.2](03_buildings_and_world_objects.md#112-incubator), [05 §5.5](05_economy_and_population.md#55-incubator-bfull-code-content-only-on-mp_sandbox) | yes → Hatchery |
| Infection | player concept | Outcome 11 `CrateRewardSpam` (bad pool only): 100 homing `SpamInfection` streaks of the monster team that kill Darwinians and convert spirits. | [04 §5.12](04_crates_powerups_and_weapons.md#512-infection-craterewardspam) | yes → Swarm |
| INI | abbreviation | Key-value configuration file (`MultiwiniaSupport.ini`). | [29 §11.14](29_data_formats_reference.md#1114-multiwiniasupportini--crash-report-file-list) | no |
| Input binding file | data format | `data/input/input_preferences*.txt` and `<P>user_input_preferences.txt`: `CONTROL = SPEC` binding lines (driver expressions) and `CONTROL ~ ICONPATH` icon lines over the 164 control names. | [29 §5.2](29_data_formats_reference.md#52-input-binding-files), [06 §11.11](06_player_control_and_input.md#1111-binding-file-grammar-formal) | no |
| InputDriver | engine (class) | A node type of the binding graph (conjoin, chord, invert, idle, W32 key/mouse, XInput, prefs, value, alias, pipe). | [06 §11.6](06_player_control_and_input.md#116-driver-catalogue) | no |
| InputManager (g_inputManager) | engine (class) | Query-driven input binding graph: controls resolved through drivers (keyboard, mouse, XInput, preferences, aliases, conjunctions, chords). | [06 §11](06_player_control_and_input.md#11-input-architecture) | no |
| InsertionSquad (squad) | engine (class) | `Unit` container of 3 `Squadie`s created by the Squad task: walks single file behind a point man, fires at the cursor when selected, 60 s life. | [02 §7](02_units_and_entities.md#7-insertion-squad), [06 §6.3](06_player_control_and_input.md#63-squad-insertionsquad) | yes → Strike team |
| Instant units | data format | Level-file `InstantUnits` rows `type team x z count inAUnit state spread [waypoint [route [runAsTask]]]` placing starting entities for the team at that start position; 16 blocked entity types are refused. | [08 §6.6](08_levels_and_level_design.md#66-instant-units-starting-forces) | no |
| INTERNAL_STUDY (licence posture) | engine | The licence posture of this set: source and data may be studied and quoted in short marked excerpts (≤ 15 lines), but nothing that is Introversion IP may ship in the new game. | [40 §6](40_carry_forward_assessment.md#6-ip-replacement-summary), [41](41_risks_and_open_questions.md) | no |
| Introversion Software | IP name | The developer and publisher: company folder, store, metaserver, crash-report and FTP hosts, version resource, installers. | [31 §14.1](31_platform_build_and_tools.md#141-repository-root-10-files), [22 §12.1](22_determinism_and_networking.md#121-components-and-endpoints-c-codeappcppappinitmetaserver-contribsystemivlibmetaservermetaserver_definesh) | yes → `<STUDIO>` |
| Invalid crates | data format | Level-file `InvalidCrates` list of outcome names excluded from the draws on that map. | [04 §4.1](04_crates_powerups_and_weapons.md#41-invalidcrates--syntax-and-semantics), [08 §5.1](08_levels_and_level_design.md#51-every-key) | no |
| IP (network) | abbreviation | Internet Protocol address; the server stamps `FROMIP`/`FROMPORT` on every received letter. | [22 §5](22_determinism_and_networking.md#5-message-catalogue) | no |
| IP (rights) | abbreviation | Intellectual property; every Introversion name, text, art and audio asset is listed for replacement (see §3 and "IP items"). | [40 §6](40_carry_forward_assessment.md#6-ip-replacement-summary), [41](41_risks_and_open_questions.md) | no |
| IS_DEMO | engine (build macro) | Macro for `!g_app->IsFullVersion()`: the runtime demo gate based on the authentication key, applied inside the full build. | [09 §6.2](09_meta_systems.md#62-every-gate-the-ui-applies) | no |
| iv_* wrappers / iv_trunc_low_nibble | engine | Maths wrappers (`iv_sin`, `iv_sqrt` and others) that round every CRT result to IEEE binary32 to keep peers bit-identical. | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model) | no |

### J

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| JumpPad | engine (class) | Invisible launcher that throws Darwinians standing within 40 wu into a ballistic jump every 0.3 s, for every team. | [03 §5.5](03_buildings_and_world_objects.md#55-jumppad) | no (generic) |

### K

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Keep / Adapt / Replace / Drop (verdicts) | engine | The four carry-forward verdicts of 40: reproduce the design as specified; keep the concept but change listed parts; keep the role with a different design or technology; do not build ("Drop (redesign)" when the feature must be designed from scratch). Code is never carried forward. | [40 §1.1](40_carry_forward_assessment.md#11-how-to-read-the-verdicts) | no |
| Keyboard layout / locales.txt | data format | `data/input/keyboards/*.txt` key-name overrides per layout, and `locales.txt` mapping a Windows LANGID to a layout. | [29 §5.3](29_data_formats_reference.md#53-keyboard-layout-override-files), [29 §5.4](29_data_formats_reference.md#54-localestxt--windows-langid--keyboard-layout) | no |
| King of the Hill | player concept | Game type `GameTypeKingOfTheHill`: teams score for each hill zone in which they have the strictly largest Darwinian head count (recounted every game second); highest score at time-up wins; no elimination. | [01 §4.2](01_game_modes.md#42-king-of-the-hill-gametypekingofthehill-bfull) | yes → Zone Control |
| KOTH | abbreviation | King of the Hill, game type `GameTypeKingOfTheHill`. | [01 §4.2](01_game_modes.md#42-king-of-the-hill-gametypekingofthehill-bfull) | yes → Zone Control |

### L

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| LAN / WAN | abbreviation | Local-area network (broadcast discovery on UDP 5009) and wide-area network (Internet play through the metaserver). | [22 §12](22_determinism_and_networking.md#12-lobby-and-server-discovery-protocol), [09 §5.2](09_meta_systems.md#52-server-discovery-components-and-endpoints) | no |
| LAN discovery | engine | Broadcast of hosted games on the local network (UDP 5009) for the server browser. | [22 §12](22_determinism_and_networking.md#12-lobby-and-server-discovery-protocol), [09 §5.2](09_meta_systems.md#52-server-discovery-components-and-endpoints) | no |
| LAN_PLAY_ENABLED / WAN_PLAY_ENABLED / MULTIPLAYER_DISABLED | engine (build macro) | Product-target switches for LAN browsing, Internet (metaserver) play, or no multiplayer at all; FULLGAME enables LAN and WAN. | [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line), [09 §1.1](09_meta_systems.md#11-what-is-live-in-the-shipped-product) | no |
| Lander | engine (class) | Entity type 7: Darwinia beach-landing craft unloading LaserTroopers ([B:LEGACY]). | [02 §31](02_units_and_entities.md#31-lander) | no |
| LandMine | engine (class) | Proximity mine written for the tank; no live caller. | [04 §6.14](04_crates_powerups_and_weapons.md#614-landmine) | no |
| Landscape | engine (class) | The terrain height field (`code/landscape.cpp`): one `SurfaceMap2D<double>` generated once per level load from the level's landscape blocks; sea level is y = 0. | [23 §1](23_terrain_and_landscape.md#1-scope-summary-and-build-tags), [23 §2](23_terrain_and_landscape.md#2-pipeline-overview) | no |
| LandscapeDef / LandscapeFlattenArea | engine (class) | The parsed `Landscape_StartDefinition` block (cell size, world size X and Z, outside height, maximum height; the colour-ramp file names go to `LevelFile`) and the axis-aligned squares of the five flatten operation types. | [23 §4.2](23_terrain_and_landscape.md#42-landscape_startdefinition-block--landscapedef-and-levelfile-strings), [23 §8](23_terrain_and_landscape.md#8-smoothing-and-flatten-areas-five-types) | no |
| LandscapeRenderer | engine (class) | Builds and draws the terrain mesh with per-vertex ramp colours and the wireframe overlay (VBO for the landscape). | [24 §4.1](24_rendering.md#41-landscape-renderer-landscaperenderer-bfull-c-codelandscape_rendererh-codelandscape_renderercpp) | no |
| LandscapeTile | engine (class) | One square fractal height map (midpoint displacement with a private LCG, optional guide grid, peak rescale) merged into the world by per-sample maximum. | [23 §5](23_terrain_and_landscape.md#5-tile-generation--exact-algorithm-all-three-generation-methods), [23 §7](23_terrain_and_landscape.md#7-height-map-composition-across-tiles-merge-and-bilinear-resampling) | no |
| LangTable | engine (class) | Phrase table (`code/lib/language_table.cpp`): phrase id → localised text, with input-mode variants and caption markup. | [28 §10.1](28_ui_framework.md#101-phrase-lookup-and-fallbacks-c-codeliblanguage_tableh-codeliblanguage_tablecpplangtablelookupphrase), [29 §6.1](29_data_formats_reference.md#61-language-phrase-tables-and-caption-markup) | no |
| Language file | data format | `data/language/<language>.txt`: `phrase_id text` lines (UTF-16 or 8-bit) with caption markup for input-mode variants; nine languages. | [29 §6.1](29_data_formats_reference.md#61-language-phrase-tables-and-caption-markup), [09 §10](09_meta_systems.md#10-localisation-coverage) | yes → own string table |
| Laser | engine (class) | Pooled laser bolt fired by Darwinians, squads and subversion weapons (0.6 s life, ricochets once off terrain); mind-control lasers convert. | [04 §6.7](04_crates_powerups_and_weapons.md#67-laser) | no |
| LaserFence (laser fence) | player concept; engine (class) | Post that, when raised, projects an energy curtain to the next post; kills or ignites enemies touching it and blocks routes; switched by power. | [03 §7.1](03_buildings_and_world_objects.md#71-laserfence) | yes → Barrier Emitter |
| LaserTrooper | engine (class) | Entity type 1 (`TypeLaserTroop`): Darwinia laser infantry landed by a Lander; [B:LEGACY]. | [02 §26](02_units_and_entities.md#26-lasertrooper) | no |
| Late join | player concept | Joining a session already in progress: the client replays the server's entire letter history from sequence 0 to reconstruct the match. | [22 §8.8](22_determinism_and_networking.md#88-reconnect-late-join-spectators-kick-password), [30 §7.2](30_scripting_tutorial_and_cutscenes.md#72-late-join-replay-from-sequence-0-bfull) | no |
| LCG | abbreviation | Linear congruential generator; the unsynced `AppRandom` family (`holdrand × 214013 + 2531011`) and the per-tile landscape generator use one. | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code), [23 §5.1](23_terrain_and_landscape.md#51-the-private-per-tile-rng) | no |
| Letter | engine | A network message between client and server (a `Directory`); server letters are sequenced, client letters carry requests and `ALIVE` input. | [22 §5](22_determinism_and_networking.md#5-message-catalogue) | no |
| Level effects | data format | Map flags in `MultiwiniaOptions` `Effects`: `snow`, `dustwind`, `soulrain`, `lightning`, `SpecialLighting`. | [08 §5.3](08_levels_and_level_design.md#53-effects-enum-levelfileleveleffect) | no |
| Level file | data format | `data/levels/*.txt` text map definition in `<Name>_StartDefinition` blocks: `Landscape`, `LandscapeTiles`, `LandFlattenAreas`, `Lights`, `Buildings`, `Routes`, `CameraMounts`, `CameraAnimations`, `InstantUnits`, `MultiwiniaOptions`, `RunningPrograms`, `PrimaryObjectives`, `Difficulty`, `LeaderboardStats`. `mp_` files are Multiwinia maps; `map_` and `mission_` files are Darwinia. | [29 §4.1](29_data_formats_reference.md#41-level-files--map-and-mission-map_-mp_-mission_), [08 §2](08_levels_and_level_design.md#2-map-inventory-discovery-and-identity) | yes → new maps |
| level_enum.txt | data format | `data/levels/level_enum.txt`: map ids 1–59 used by the achievement tracker (50–59 are absent PDLC maps); excluded from retail data. | [29 §4.2](29_data_formats_reference.md#42-level_enumtxt--achievement-level-ids), [08 §7.3](08_levels_and_level_design.md#73-datalevelslevel_enumtxt-achievements-and-pdlc-remnants) | yes → new ids |
| LevelOptions | data format | The 15 level option words inside `MultiwiniaOptions` (`nohandicap`, `instantspawnpointcapture`, `trunkportreinforcements`, `trunkportarmour`, `nogunturrets` and the others). | [08 §5.2](08_levels_and_level_design.md#52-leveloptions-enum-levelfileleveloption-numleveloptions--15) | no |
| Library / ResearchItem | engine (class) | [B:LEGACY] Darwinia research collection buildings. | [03 §12.25](03_buildings_and_world_objects.md#1225-library) | no |
| Light | engine (class) | Plain record of a level directional light (colour and direction) bound to an OpenGL light slot; not simulated. | [02 §3.3](02_units_and_entities.md#33-light--level-light-record) | no |
| Lightning | engine (class) | Storm strike of the level effect `lightning` (not a crate outcome). | [04 §6.18](04_crates_powerups_and_weapons.md#618-lightning), [26 §7.3](26_animation_and_vfx.md#73-lightning-bfull-c-codeworldobjectlightningcpplightningadvance-lightningrender-codelocationcpplocationadvancelightning) | no |
| LList | engine (class) | SystemIV doubly linked list with a cached cursor (so even reads mutate state and need a lock across threads). | [20 §11.3](20_architecture.md#113-shared-data-and-synchronisation), [21 §5.3](21_simulation_and_entity_model.md#53-container-semantics-iteration-order-and-slot-re-use) | no |
| LMB / RMB | abbreviation | Left and right mouse button. | [06 §12.2](06_player_control_and_input.md#122-control-table-all-164) | no |
| LoadingScreen | engine (class) | The loading frame with spinner and progress drawn while a match loads on a worker thread. | [09 §9.2](09_meta_systems.md#92-loading-screen-loadingscreen-bfull), [24 §4.25](24_rendering.md#425-loading-screen-frame-bfull-c-codeloading_screencpploadingscreenrenderframe) | no |
| Lobby | player concept | The game-setup page (`PageGameSetup`) where the host picks map and options, players pick colour and side, CPU players fill slots and everyone readies up before start. | [09 §3.6](09_meta_systems.md#36-pagegamesetup-6--the-lobby-single-player-setup-host-lobby-and-client-lobby-share-one-page), [09 §5.6](09_meta_systems.md#56-lobby-and-option-synchronisation) | no |
| LobbyTeam | engine (class) | Persistent per-slot roster record in `Multiwinia::m_teams[6]` (id, client, type, colour, ready flag) that survives lobby, match and results. | [21 §9.2](21_simulation_and_entity_model.md#92-lobbyteam-and-team), [01 §2.5.1](01_game_modes.md#251-lobbyteam--per-slot-roster-record-c-codeteamhlobbyteam) | no |
| Location | engine (class) | The match world (`code/location.h`, `g_app->m_location`): teams, buildings, spirits, lasers, effects, landscape, water, grids, routing; advances the simulation in slices and resolves `WorldObjectId`s. | [21 §5](21_simulation_and_entity_model.md#5-location-containers-creation-removal-and-update-ordering) | no |
| Location editor | engine (class) | [B:EDITOR] in-game map editor (`LocationEditor`, `MainEditWindow` and editor windows). | [08 §8](08_levels_and_level_design.md#8-the-map-editor-beditor) | no |
| LOCATION_EDITOR | engine (build macro) | Compiles the in-game map editor (`LocationEditor` and the editor windows); commented out in every target, hence [B:EDITOR]. | [08 §8.1](08_levels_and_level_design.md#81-build-status-and-reachability) | no |
| Lockstep | engine | The network model: all peers run the full simulation from identical ordered command streams and a shared RNG seed; only inputs travel, never state. | [22 §1.1](22_determinism_and_networking.md#11-at-a-glance), [22 §1.3](22_determinism_and_networking.md#13-why-the-model-works-and-its-preconditions) | no |
| LOD | abbreviation | Level of detail: frustum- and distance-based render simplification in about 73 world-object files; camera state reaches the simulation only through squad direct control and the blocked `ScriptTrigger`. | [24 §5.3](24_rendering.md#53-frustum-tests-cpu-culling-helpers-c-codecameracpp), [22 §14.11](22_determinism_and_networking.md#1411-local-only-state-leaking-into-simulation--every-known-desync-path) | no |
| LRU | abbreviation | Least recently used, the eviction rule of the decoded-sample cache (above 60 MiB). | [27 §9](27_audio_engine.md#9-sample-cache) | no |
| LT / RT / LB / RB | abbreviation | Gamepad left and right triggers and shoulder buttons (XInput). | [06 §12.2](06_player_control_and_input.md#122-control-table-all-164) | no |

### M

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| m_others | engine | The team's loose entity list (`Team::m_others`, unit id −1): Darwinians, officers, armour and other bodies; it is what the population cap counts. | [21 §9.2](21_simulation_and_entity_model.md#92-lobbyteam-and-team), [05 §4.1](05_economy_and_population.md#41-what-is-counted) | no |
| MAG / MIN / LML / L | abbreviation | Texture filter shorthand in asset tables: magnification and minification filters, `LINEAR_MIPMAP_LINEAR` and `LINEAR`. | [25 §3.0](25_textures_and_assets.md#30-column-legend) | no |
| Magical Forest | player concept | Outcome 12 `CrateRewardForest` (basic, immediate): 1–2 spirit-dropping trees near the crate. | [04 §5.13](04_crates_powerups_and_weapons.md#513-magical-forest-craterewardforest) | yes → Spirit Grove |
| Main loops | engine | The busy loops on the main thread: the outer `RunTheGame`, `MainMenuLoop` for front end and lobby, `LocationGameLoop` for the in-match frame (input, network, simulation slices, render) and the loading-screen loop. | [20 §9](20_architecture.md#9-main-loops) | no |
| Map | player concept | A level file `data/levels/mp_*.txt` named `mp_<mode>_<n>P_<k>` (57 Multiwinia maps, 49 in retail) defining terrain, buildings, starting units and mode options. | [08 §3](08_levels_and_level_design.md#3-master-catalogue-of-the-57-multiwinia-maps) | yes → new maps named `map_<mode>_<players>p_<nn>` |
| Map CRC (mapcrc.h) | data format | Generated header of `MAPID_*` ids and file CRCs identifying official maps. | [29 §4.6](29_data_formats_reference.md#46-map-identity-crc-and-codemapcrch), [31 §10](31_platform_build_and_tools.md#10-map-crc-tool-and-runtime-map-identity) | yes → new map ids |
| Map CRC tool | engine | `tools/MapCRC`: computes the identity CRC of every map file into `code/mapcrc.h`. | [31 §10](31_platform_build_and_tools.md#10-map-crc-tool-and-runtime-map-identity), [08 §2.3](08_levels_and_level_design.md#23-map-identity-mapcrc) | no |
| Map names | IP name | Display names and descriptions of the 49 retail maps (for example "Beneath the Baleful Eye") and the `mp_*` file names. | [08 §3.1](08_levels_and_level_design.md#31-master-table) | yes → `map_<mode>_<players>p_<nn>` and new names |
| Map options | player concept | Per-level `MultiwiniaOptions` block: supported game types, player count, population cap, co-op flag, invalid crates, trunk-port settings and level options such as `instantspawnpointcapture`. | [08 §5](08_levels_and_level_design.md#5-per-level-options-multiwiniaoptions) | no |
| MarkerSystem | engine (class) | World markers drawn at objective and event positions, with off-screen arrows. | [24 §4.18](24_rendering.md#418-markers-pass-16-c-codemarkersystemcppmarkersystemrender), [06 §8.12](06_player_control_and_input.md#812-world-markers-markersystem-and-their-input-role) | no |
| MasterSpawnPoint (master spawn point) | engine (class) | Virtual source of all spawn spirits, one per level, usually invisible; level name `SpawnPointMaster`. | [03 §4.4](03_buildings_and_world_objects.md#44-masterspawnpoint) | yes → Source Node |
| Match | player concept | One played map from the end of loading to the results screen; the lifetime of a `Location` and of the per-match state. | [20 §8.3](20_architecture.md#83-one-match-end-to-end), [01 §2.2](01_game_modes.md#22-match-lifecycle) | no |
| Matchmaker | engine | The metaserver component that helps peers behind NAT reach each other (hole-punching requests) before a join. | [22 §12.4](22_determinism_and_networking.md#124-nat-traversal-c-contribsystemivlibmetaservermatchmakercpp-serveradvertise-clienttoserveradvance-codeuigamemenuwindowcppgamemenuwindowjoinserver) | yes → own relay or platform service |
| .max | data format | 3ds Max scene sources of the shapes (`data/shapes/source/`), converted by ShapeExporter; not read by the game. | [29 §8.2](29_data_formats_reference.md#82-max-sources-and-the-shape-tool-chain-authoring), [25 §4.7](25_textures_and_assets.md#47-max-authoring-sources) | yes → new sources |
| Mega Rage | player concept | Outcome 21 `CrateRewardBitzkreig` (sic, immediate, global): for 15 s every Darwinian of every team behaves as raging. | [04 §5.22](04_crates_powerups_and_weapons.md#522-mega-rage-craterewardbitzkreig) | yes → Frenzy |
| Metaserver | engine | Introversion's external Internet server directory (`contrib/systemiv/lib/metaserver`, UDP 5113): server registration and listing, key authentication, MOTD, latest version and demo limits. Its server side is not in the tree. | [22 §12](22_determinism_and_networking.md#12-lobby-and-server-discovery-protocol), [09 §5.2](09_meta_systems.md#52-server-discovery-components-and-endpoints) | yes → own directory service |
| Meteor | engine (class) | Entity type 23: one rock of a meteor shower, falling straight onto a point near the target and exploding. | [02 §22](02_units_and_entities.md#22-meteor) | no (generic) |
| Meteor (Meteor Shower) | player concept | Outcome 17 `CrateRewardMeteor`: 5–9 meteors into a 420 × 420 square (radius 50, damage 100) with 0.4× slow motion. | [04 §5.18](04_crates_powerups_and_weapons.md#518-meteor-shower) | no (generic) |
| MineBuilding / Mine / MineCart / Refinery / TrackLink / TrackJunction / TrackStart / TrackEnd | engine (class) | [B:LEGACY] Darwinia mine-cart conveyor system. | [03 §12.27](03_buildings_and_world_objects.md#1227-minebuilding) | no |
| Missile | engine (class) | Guided missile written for the tank and armour; no live caller. | [04 §6.13](04_crates_powerups_and_weapons.md#613-missile) | no |
| Mode second | player concept | The unit of the per-mode rule clocks (time limit, reinforcement and elimination counters): one decrement every 11 ticks, so a displayed "second" lasts 1.1 game seconds. | [05 §8.1](05_economy_and_population.md#81-tick-model), [01 §2.3](01_game_modes.md#23-per-tick-order) | no |
| Monster team | player concept | Team slot 4, a CPU faction ("Evilinians") that owns every bad crate outcome, ant hills, triffids and infected units, and can even win KOTH on score. | [01 §2.5](01_game_modes.md#25-teams-slots-start-positions-colours-co-op-and-assault-sides), [04 §2.1](04_crates_powerups_and_weapons.md#21-identity-build-gating-and-pool-membership) | yes → Rogue faction |
| MOTD | abbreviation | Message of the day, a text item fetched from the metaserver and shown in the main-menu information box. | [09 §5.15](09_meta_systems.md#515-metaserver-driven-front-end-content) | no |
| MP / SP | abbreviation | Multiplayer and single-player. In code `App::Multiplayer()` is true for every Multiwinia match (including vs-CPU); SP branches are mostly Darwinia legacy. | [20 §4.5](20_architecture.md#45-methods), [21 §10.4](21_simulation_and_entity_model.md#104-local-only-state-leaking-into-the-simulation) | no |
| MSVC / VS2005 | abbreviation | Microsoft Visual C++ and Visual Studio 2005, the shipped toolchain (`targets/vs2005/`). | [31 §7.1](31_platform_build_and_tools.md#71-solution-and-projects-c-targetsvs2005darwiniasln) | no |
| MT19937 | abbreviation | Mersenne Twister generator (Matsumoto–Nishimura); the algorithm behind the synced `syncrand` stream. | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code) | yes → `std::mt19937` or own generator (attribution if reference code is copied) |
| Multiwinia | IP name | The product name (`APP_NAME`, window title, profile folder, metaserver game name, many identifiers such as `MultiwiniaZone`); subtitle "Survival of the flattest". | [00 §2.1](00_overview.md#21-product-and-build), [09 §3.1](09_meta_systems.md#31-pagemain-0--title-screen) | yes → `<GAME_TITLE>` |
| Multiwinia (rules object) | engine (class) | `g_app->m_multiwinia` (`code/multiwinia.h`): game type, options, lobby teams, grace period, per-mode `Advance*` rules, trunk ports, retribution, scores, HUD and `DeclareWinner`. | [01 §2.1](01_game_modes.md#21-the-rules-object), [01 §2.14](01_game_modes.md#214-multiwinia-data-members-and-methods) | yes → MatchRules |
| multiwinia.txt | data format | `data/multiwinia.txt`: per-game-type blueprints of lobby parameters (name, min, max, default, step, control type). | [29 §4.3](29_data_formats_reference.md#43-multiwiniatxt--game-types-and-lobby-parameters), [01 §3.1](01_game_modes.md#31-data-file-and-loader) | no |
| Multiwinian(s) | IP name | Manual and string name for Darwinians in Multiwinia (for example the Darwinians crate is shown as "Multiwinians"). | [02 §4](02_units_and_entities.md#4-darwinian), [04 §5.7](04_crates_powerups_and_weapons.md#57-darwinians-multiwinians) | yes → Worker(s) |
| MultiwiniaOfficerOrders | engine (class) | Multiplayer order visualiser effect that follows the route to an officer's order point, leaving `OfficerOrderTrail` arrows. | [02 §5.8](02_units_and_entities.md#58-officerorders-multiwiniaofficerorders-officerordertrail--order-effects) | yes → `OrderMarker` |
| MultiwiniaOptions (block) | data format | The level-file block of mode settings: `GameTypes`, `PopulationCap`, `DefenderPopulationCap`, `NumPlayers`, `PlayTime`, `Difficulty`, `Effects`, `LevelOptions`, `InvalidCrates`, `AttackingTeams`, co-op keys, `OfficialLevel`. | [08 §5.1](08_levels_and_level_design.md#51-every-key) | yes → `MatchOptions` |
| MultiwiniaSupport.ini | data format | File list read by the crash-support tool. | [29 §11.14](29_data_formats_reference.md#1114-multiwiniasupportini--crash-report-file-list) | yes → own crash reporter |
| MultiwiniaTutorial | engine | The live tutorial director: two scripted tutorials with gameplay overrides and hard-coded map ids. | [30 §3](30_scripting_tutorial_and_cutscenes.md#3-live-multiwinia-tutorials-bfull) | no |
| MultiwiniaZone (mode zone) | engine (class) | Circular mode zone: KOTH hill, CTS drop zone or statue spawn zone, or Blitzkrieg flag, depending on the game type. | [03 §8.1](03_buildings_and_world_objects.md#81-multiwiniazone) | yes → Control Zone |
| Music channel | engine | Dedicated streaming voice for music; no music ever plays in the shipped Multiwinia. | [27 §8](27_audio_engine.md#8-music-channel), [10 §6](10_audio_design.md#6-music-system) | no |
| MuzzleFlash | engine (class) | Short-lived cosmetic flash at a firing point. | [04 §6.12](04_crates_powerups_and_weapons.md#612-muzzleflash) | no |
| MW | abbreviation | Multiwinia, used as a prefix in tables and asset names (`mwhelp`, `mwsounds`, `mwtitle`). | [25 §1.1](25_textures_and_assets.md#11-scope-asset-classes-and-how-to-read-this-document) | yes → `<GAME_TITLE>` |
| .mwm | data format | [B:EDITOR] custom-map container ("Multiwinia map", `code/mapfile.cpp:MapFile`) under `<P>maps/`: a length-prefixed zlib stream of a `Directory` holding the level text and embedded textures. | [29 §11.5](29_data_formats_reference.md#115-mwm-custom-map-container-beditor), [08 §8.8](08_levels_and_level_design.md#88-mwm-custom-map-container-mapfile-and-save-path) | no |

### N

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Napalm Strike | player concept | Outcome 28 `CrateRewardNapalmStrike`: an airstrike (task option 1) whose bombs ignite Darwinians within 62.5 wu and burn trees. | [04 §5.27](04_crates_powerups_and_weapons.md#527-napalm-strike) | yes → Firebomb Run |
| NAT | abbreviation | Network address translation; the matchmaker helps peers behind NAT routers connect (hole punching). | [22 §12.4](22_determinism_and_networking.md#124-nat-traversal-c-contribsystemivlibmetaservermatchmakercpp-serveradvertise-clienttoserveradvance-codeuigamemenuwindowcppgamemenuwindowjoinserver) | no |
| NET_DARWINIA_* | IP name | Prefix of every network command and field identifier (`code/network/network_defines.h`). | [22 §5.1](22_determinism_and_networking.md#51-data-field-codes-complete-list-from-codenetworknetwork_definesh) | yes → `NET_GAME_*` |
| NetLib | engine (class) | SystemIV's "very thin portable UDP network library" (`contrib/systemiv/lib/netlib`): sockets, listener threads, `NetMutex`, `NetEvent`, `NetStartThread`. | [22 §3](22_determinism_and_networking.md#3-transport--netlib-over-udp) | no |
| NetMutex / NetEvent / NetThread | engine (class) | NetLib's portable mutex, event and thread wrappers (Win32 critical sections, events and `CreateThread`). | [20 §11.1](20_architecture.md#111-primitives) | no |
| Network statistics window | engine | The F9 "Network Stats" window, shipped in FULL (`NETWORK_STATS_ENABLED`). | [32 §5.1](32_debug_test_and_performance.md#51-network-stats-window--ships-in-full-c-codeinterfacenetwork_windowcppnetworkwindowcreate-networkwindowrender), [22 §11.2](22_determinism_and_networking.md#112-network-statistics-window-bfull-network_stats_enabled-codeinterfacenetwork_windowcpp-toggled-by-controltogglenetworkstats-in-codeuser_inputcpp-default-f9-per-32) | no |
| NetworkValue | engine (class) | Binding between a lobby UI control and a replicated game option (`code/network/networkvalue.*`); floats travel as `%.2f` text. | [22 §4.4](22_determinism_and_networking.md#44-networkvalue-family--ui--network-option-binding-codenetworknetworkvalue), [28 §4.12](28_ui_framework.md#412-value-binding-model-codenetworknetworkvalue) | no |
| Neutral (team 255) | player concept; engine | The "no team" value of `WorldObjectId::m_teamId`; neutral buildings can be captured, neutral spawn points produce nothing. | [21 §9.1](21_simulation_and_entity_model.md#91-team-slots-and-ids) | no |
| NN-U<n> | abbreviation | Identifier of an unknown or design-critical inference in document NN (for example 05-U5), listed in that document's "Open questions and unknowns". | [41](41_risks_and_open_questions.md), [00 Open questions and unknowns](00_overview.md#open-questions-and-unknowns) | no |
| NPOT / POT | abbreviation | Non-power-of-two and power-of-two texture dimensions. | [25 §2.8](25_textures_and_assets.md#28-power-of-two-handling) | no |
| NSIS / Inno Setup | abbreviation | Windows installer generators; the Multiwinia installer scripts use Inno Setup (`tools/InstallerScript.iss`). | [31 §9.2](31_platform_build_and_tools.md#92-inno-setup-scripts-multiwinia-c-toolsinstallerscriptiss) | no |
| Nuke | player concept; engine (class) | Outcome 5 `CrateRewardNuke` (advanced): 10 submarine-launched warheads at the target (radius 50, damage 100 each), the first exactly on the point and nine within ±150; needs sea on the map. Each warhead is entity type 21 `Nuke`. | [04 §5.6](04_crates_powerups_and_weapons.md#56-nuke), [02 §12](02_units_and_entities.md#12-nuke) | no (generic; placeholder Missile Barrage) |
| NUM_SLICES_PER_FRAME | engine (build macro) | `#define` 10 in `code/globals.h`: the number of slices one simulation tick is split into. | [20 §10.3](20_architecture.md#103-client-tick-and-slices), [21 §5.4](21_simulation_and_entity_model.md#54-slice-partitioning) | no |
| NUM_TEAMS | engine (build macro) | `#define` 6 in `code/globals.h`: four player slots plus the monster and Futurewinian teams. | [21 §9.1](21_simulation_and_entity_model.md#91-team-slots-and-ids) | no |

### O

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| ObstructionGrid | engine (class) | Spatial index of buildings: each world cell lists the unique ids of the buildings overlapping it; used for obstacle pushes and placement tests. | [21 §6.4](21_simulation_and_entity_model.md#64-obstructiongrid) | no |
| Officer | player concept; engine (class) | Entity type 13, a promoted Darwinian that never fights: a GOTO signpost redirecting idle Darwinians within 100 wu, or the leader of a formation. | [02 §5](02_units_and_entities.md#5-officer), [06 §5](06_player_control_and_input.md#5-officer-orders-promotion-and-formations) | yes → Leader |
| OfficerFormation | engine (class) | Formation slot record `{entity unique id, timer}` in an officer's formation array: created with 5.0 s, the member adds dt each tick it registers and the officer subtracts dt, so a member that stops registering drops out when its banked time runs out; the slot index is the member's formation position. | [02 §5.6](02_units_and_entities.md#56-officerformation--formation-slot) | no |
| OfficerOrders | engine (class) | [B:LEGACY] single-player order visualiser effect that travels to the order point. | [02 §5.8](02_units_and_entities.md#58-officerorders-multiwiniaofficerorders-officerordertrail--order-effects) | no |
| OfficerOrderTrail | engine (class) | Arrow decal effect left on the ground by `MultiwiniaOfficerOrders`. | [02 §5.8](02_units_and_entities.md#58-officerorders-multiwiniaofficerorders-officerordertrail--order-effects) | no |
| .ogg | data format | Ogg Vorbis audio samples in `data/sounds/` and `data/mwsounds/` (1332 files, 898 live). | [29 §7.6](29_data_formats_reference.md#76-ogg-vorbis-samples), [25 §5](25_textures_and_assets.md#5-audio-sample-catalogue) | yes → new audio |
| One-minute warning | player concept | HUD message `dialog_oneminute` shown once when the remaining time first drops to 60 mode seconds or less. | [10 §7.2](10_audio_design.md#72-countdown-timer-and-match-end-audio-verified-schedule), [01 §2.12](01_game_modes.md#212-the-match-hud) | no |
| OpenGL emulation layer (opengl_directx) | engine | `code/lib/opengl_directx*.cpp` and `FixedPipeline`: Direct3D 9 implementations of the GL entry points, matrix stacks, display lists and the fixed-function pipeline as HLSL shaders ([B:D3D]). | [24 §1.5](24_rendering.md#15-direct3d-9-path-opengl-api-emulation-layer-bd3d) | no |
| origin / version files | data format | One-line text files in the install directory naming the distribution origin (for example `buka`) and version, reported to the metaserver. | [29 §11.11](29_data_formats_reference.md#1111-origin-and-version-files) | no |
| OS | abbreviation | Operating system. | [31 §2](31_platform_build_and_tools.md#2-platform-abstraction) | no |
| Other product targets | engine (build macro) | `TARGET_PC_PREVIEW`, `TARGET_PC_HARDWARECOMPAT`, `TARGET_ASSAULT_STRESSTEST`, `TARGET_BETATEST_GROUP_ALL`, `TARGET_BETATEST_GROUP_A/B/C`: commented-out targets that restrict modes, crates and `PERMITTED_MAPS` for previews, hardware tests, stress tests and beta groups. | [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line) | no |
| Outside height | player concept; engine | The negative sea-floor height of everything outside the tiles. | [23 §9.2](23_terrain_and_landscape.md#92-outside-height) | no |

### P

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| P01–P20, X1a–X4, S<nn> | abbreviation | Identifiers of the analysis agents whose drafts were merged (partition agents, cross-cutting agents, synthesis agents); they appear in "Sources merged" headers and "Resolved:" notes. | [20 §1](20_architecture.md#1-scope-and-reading-guide) | no |
| Pack list | data format | Build-time inclusion, exclusion and sound lists in `tools/` that decide which `data/` files enter each archive per SKU. | [29 §10.2](29_data_formats_reference.md#102-build-time-pack-lists-exclusion-inclusion-and-sound-lists), [31 §8.4](31_platform_build_and_tools.md#84-exclusion--inclusion-list-grammar-c-toolslistfilespl-toolscopyfilespl) | no |
| Page (front-end page) | engine | One numbered state of `GameMenuWindow` (for example `PageMain` 0, `PageGameSetup` 6) with its own set-up, widgets and teardown. | [28 §6.2](28_ui_framework.md#62-page-slots), [09 §3](09_meta_systems.md#3-page-by-page-and-window-by-window-specification) | no |
| ParticleSystem / Particle | engine (class) | Pooled particles (`SliceDArray<Particle>`, 1500 initial) of 19 hard-coded types, advanced per tick and drawn as billboards; cosmetic. | [26 §6](26_animation_and_vfx.md#6-particle-system) | no |
| Passengers | player concept | The 100 Darwinians that must board an escape rocket through its fuel station before launch. | [03 §8.12](03_buildings_and_world_objects.md#812-escaperocket) | no |
| Pause | player concept | Allowed only with no remote human players; ramps game speed to 0 over about 2 s (rate 0.2 per frame) and back. | [22 §2.6](22_determinism_and_networking.md#26-pause-and-game-speed) | no |
| PCM | abbreviation | Pulse-code modulation: uncompressed sample data delivered to the sound back-end. | [27 §6.4](27_audio_engine.md#64-loop-logic-and-end-of-sample-handling-inside-the-pcm-callback) | no |
| PDLC | abbreviation | Premium downloadable content; map ids 50–59 in `level_enum.txt` reserved for two absent map packs. | [08 §7.3](08_levels_and_level_design.md#73-datalevelslevel_enumtxt-achievements-and-pdlc-remnants) | yes → new map names |
| Pecan / Hummingbird | IP name | Internal codenames used as constant values in the input binding files (`value Pecan`, `value Hummingbird`). | [06 §11.6.8](06_player_control_and_input.md#1168-value-value-pecanhummingbird) | yes → `AlwaysTrue`, `FeatureFlagX` |
| PERMITTED_MAPS | engine (build macro) | Optional whitelist of map ids for restricted targets (preview, hardware-compat, stress, beta groups); undefined in FULLGAME, so every map is allowed. | [08 §2.4](08_levels_and_level_design.md#24-permitted-maps-per-build-target-and-runtime-gate), [31 §8.6](31_platform_build_and_tools.md#86-inclusion-lists-and-permitted_maps-c-the-six-inclusion-lists-in-tools-table-codelibuniversal_includeh) | no |
| Personal Shield | player concept | Outcome 7 `CrateRewardBooster`: for up to 30 s own Darwinians within 70 wu ignore all resistible damage. | [04 §5.8](04_crates_powerups_and_weapons.md#58-personal-shield-craterewardbooster) | yes → Shield |
| Phrase id | data format | Key of a localised string in `data/language/*.txt` (for example `multiwinia_gametype_*`, `cratename_*`, `crate_message_*`). | [29 §6.1](29_data_formats_reference.md#61-language-phrase-tables-and-caption-markup) | yes → own string table |
| Pixel effect | engine | [B:LEGACY] screen-space pixelation ghosting effect of the renderer. | [24 §4.14](24_rendering.md#414-pixel-effect-screen-space-pixelation-ghosting-blegacy-c-coderenderercpp) | no |
| Plague | player concept | Outcome 15 `CrateRewardPlague` (bad pool only): infects every Darwinian of every team within 70 wu. | [04 §5.16](04_crates_powerups_and_weapons.md#516-plague) | yes → Sickness |
| PNG | abbreviation | Portable Network Graphics; written for screenshots and thumbnails via libpng, never read as game data. | [29 §8.4](29_data_formats_reference.md#84-png-screenshot-output-runtime) | no |
| POKEY | abbreviation | Atari sound chip whose envelope sequencer `EffectProcessor` emulates in the [B:DEBUG] sound editor; unrelated to `effects.txt` visuals. | [26 §9.5](26_animation_and_vfx.md#95-effectprocessor-is-unrelated-c-codeeffect_processorh-codeeffect_processorcpp-codeappcppappinitialise), [27 §13](27_audio_engine.md#13-profiles-statistics-debug-and-editor-tooling) | yes → drop |
| Population cap | player concept | Per-team limit on cap-counted bodies (`Team::m_others`) that freezes spawn-point and trunk-port production: the level's `PopulationCap` divided by the level's `NumPlayers` (Assault has separate attacker and defender caps). | [05 §4](05_economy_and_population.md#4-the-population-cap) | no |
| Port | player concept; engine | A slot on a building, created from a `MarkerPort` shape marker, that one Darwinian occupies to operate or capture it; ports also carry per-team reservation counters. | [03 §3.6](03_buildings_and_world_objects.md#36-ports-occupancy-and-reservation) | no |
| Portal | engine (class) | [B:DEBUG] The Beast's portal that expands and spawns tentacles. | [03 §9.3](03_buildings_and_world_objects.md#93-portal) | yes → Rift (Rift Horror) |
| Power-up slot (task slot) | player concept | One of the 6 task-manager slots (2 while eliminated); a full manager rejects new grants with "Powerup Limit Reached!". | [06 §3.5](06_player_control_and_input.md#35-slots-and-capacity) | yes → Power slot |
| PowerBuilding | engine (class) | Base of the power network (`m_powerLink`, a list of travelling power surges); Pylon, PylonEnd, PylonStart, PowerSplitter, SolarPanel, Generator derive from it. | [03 §6.1](03_buildings_and_world_objects.md#61-powerbuilding) | no |
| PowerSplitter | engine (class) | Editor-visible router that forwards a surge to the first linked pylon of the surge's team (Rocket Riot). | [03 §6.5](03_buildings_and_world_objects.md#65-powersplitter) | no |
| Powerstation | engine (class) | [B:LEGACY] 4-port building that enables a linked laser fence when fully manned. | [03 §6.8](03_buildings_and_world_objects.md#68-powerstation) | no |
| Prediction time | engine | `g_predictionTime`: the render-time extrapolation of positions since the last tick (no interpolation); render-only. | [21 §10.2](21_simulation_and_entity_model.md#102-prediction-time-extrapolation-not-interpolation), [24 §2.4](24_rendering.md#24-render-time-prediction-convention-c) | no |
| Preferences file | data format | `<P>preferences.txt` (saved as UTF-16LE) and `data/default_preferences.txt`: one `KEY = VALUE` item per line (alphanumeric, case-sensitive keys), read by `PrefsManager`. | [29 §5.1](29_data_formats_reference.md#51-preferences-preferencestxt-default_preferencestxt), [09 §11](09_meta_systems.md#11-preferences-and-every-user-option) | no |
| PrefsManager (g_prefsManager) | engine (class) | Preference store backed by `preferences.txt` with built-in first-run defaults. | [09 §11.1](09_meta_systems.md#111-preference-storage-and-lifecycle), [29 §5.1](29_data_formats_reference.md#51-preferences-preferencestxt-default_preferencestxt) | no |
| Profile directory (`<P>`) | engine | The per-user data folder (`%LOCALAPPDATA%\Introversion\Multiwinia\`, or the working directory with `--NoProfile`) holding preferences, keys, logs and dumps. | [31 §5.2](31_platform_build_and_tools.md#52-profile-user-data-directory-resolution-c-codeappcppappgetprofiledirectory-codemaincppinitialise-codemaincppappmain) | yes → `<STUDIO>\<GAME_TITLE>\` |
| Profiler | engine (class) | SystemIV section profiler (`contrib/systemiv/lib/profiler`) displayed in the profiler window. | [32 §9](32_debug_test_and_performance.md#9-profiler) | no |
| Promotion / demotion | player concept | Turning a selected Darwinian into an officer (command `PROMOTE`) and back; officers are created only this way. | [06 §5.1](06_player_control_and_input.md#51-promotion-and-demotion) | yes → Leader |
| Pseudo-unit codes | engine | Negative `WorldObjectId::m_unitId` values selecting a non-team container: `UNIT_BUILDINGS` −100, `UNIT_SPIRITS` −101, `UNIT_LASERS` −102, `UNIT_EFFECTS` −103. | [21 §3.2](21_simulation_and_entity_model.md#32-pseudo-unit-codes-and-the-container-each-id-resolves-into) | no |
| PSO | abbreviation | Pipeline state object (Direct3D 12); the render-pass table in 24 is written so it maps onto a PSO catalogue. | [24 §9](24_rendering.md#9-direct3d-12-mapping-notes-per-component) | no |
| .psp / .grd | data format | Authoring leftovers (a Paint Shop Pro banner template and a Photoshop gradient); never read. | [29 §8.11](29_data_formats_reference.md#811-psp--paint-shop-pro-image-authoring), [29 §8.12](29_data_formats_reference.md#812-grd--photoshop-gradient-authoring) | no |
| PulseBomb (pulse bomb) | player concept; engine (class) | The Assault "WMD": a countdown bomb of the defending team, destroyed when attackers own all linked control stations, otherwise detonating with a pulse wave. | [03 §8.7](03_buildings_and_world_objects.md#87-pulsebomb) | yes → Core Device |
| PulseWave | engine (class) | The Assault pulse-bomb detonation wave (initial radius 200). | [04 §6.15](04_crates_powerups_and_weapons.md#615-pulsewave) | no |
| Purity control | engine | [B:DEAD] anti-tamper data set and kill switch (`data/purity/`, `PURITY_CONTROL`). | [29 §6.4](29_data_formats_reference.md#64-datapurity--purity-control-data-set-directory-convention-bdead) | no |
| Pylon | engine (class) | Plain power-line relay segment. | [03 §6.3](03_buildings_and_world_objects.md#63-pylon) | no (generic) |
| PylonEnd | engine (class) | Invisible line terminator that delivers a surge immediately to a fuel generator, laser fence or (legacy) construction yard. | [03 §6.4](03_buildings_and_world_objects.md#64-pylonend) | no |
| PylonStart | engine (class) | [B:LEGACY] surge emitter fed by the campaign generator. | [03 §6.7](03_buildings_and_world_objects.md#67-pylonstart) | no |

### Q

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| QPC | abbreviation | `QueryPerformanceCounter`, the Win32 high-resolution clock behind `GetHighResTime`. | [31 §4.1](31_platform_build_and_tools.md#41-hi_res_time-api-and-baseline-implementation-c-codelibhi_res_timeh-codelibhi_res_timecpp) | no |
| Quick match | player concept | Automatic match-finding page (`PageQuickMatchGame`), compiled but unreachable. | [09 §5.9](09_meta_systems.md#59-quick-match-pagequickmatchgame-5--unreachable) | no |

### R

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| RadarDish (radar dish) | player concept; engine (class) | One-way long-range transporter: units entering its door travel along its beam to the receiver dish it is aimed at; in multiplayer it has no capture rule of its own and takes the team of a linked control station. | [03 §5.3](03_buildings_and_world_objects.md#53-radardish) | yes → Relay Dish |
| Rage | player concept | Outcome 23 `CrateRewardRage`: for up to 30 s own Darwinians within 70 wu fire every tick, take 75 % damage and move 1.5× faster. | [04 §5.24](04_crates_powerups_and_weapons.md#524-rage) | no (generic; placeholder Fury) |
| Randomiser | engine (class) | Effect object (`EffectRandomiser`, not a building) of the Randomiser outcome that reassigns Darwinians to random teams for 15 s. | [03 §9.4](03_buildings_and_world_objects.md#94-randomiser) | yes → Allegiance Scrambler |
| Randomiser (crate outcome) | player concept | Outcome 13 `CrateRewardRandomiser` (bad pool only): for 15 s converts Darwinians within 100 wu to random teams, through a `Randomiser` effect object. | [04 §5.14](04_crates_powerups_and_weapons.md#514-randomiser), [03 §9.4](03_buildings_and_world_objects.md#94-randomiser) | yes → Allegiance Scrambler |
| Rank announcement | player concept | Once per mode second (not Assault, not Blitzkrieg) the local player gets a position message (phrases `dialog_position_<n>` or `dialog_tied_<n>`) whenever their rank among the non-special teams changes. | [01 §2.6](01_game_modes.md#26-scores-and-rank-announcements) | no |
| RAR | abbreviation | The archive format of the `.dat` data packs, read by the linked `contrib/unrar` library. | [29 §10.1](29_data_formats_reference.md#101-dat-archives-rar) | no |
| Ready (ready-up) | player concept | Per-team flag `Team::m_ready` toggled with Space or pad A during the grace period (network command `TOGGLE_READY`); in the lobby it toggles `LobbyTeam::m_ready`. | [01 §2.4](01_game_modes.md#24-grace-period-readiness-and-countdown), [09 §5.12](09_meta_systems.md#512-ready-start-kick-request-ai-spectator) | no |
| ReceiverBuilding / ReceiverLink / ReceiverSpiritSpawner / SpiritReceiver / SpiritProcessor | engine (class) | [B:LEGACY] Darwinia spirit-receiver network that pulls floating unprocessed spirits into a processor. | [03 §12.3](03_buildings_and_world_objects.md#123-receiverbuilding) | no |
| RecordDemo (command replay) | engine | Hidden record and replay of the server command history ([B:FULL], hidden). | [30 §7.1](30_scripting_tutorial_and_cutscenes.md#71-recorddemo--command-history-record-and-replay-bfull-hidden) | no |
| redshirt2 | data format | Light substitution cipher (16-entry offset table over printable characters) for text written to the user profile by Darwinia campaign saves; a file starting with the magic `redshirt2` is decoded automatically by `TextReader::CleanLine`. No shipped file uses it ([B:LEGACY] use). | [29 §3.3](29_data_formats_reference.md#33-redshirt2-obfuscated-text) | yes → signed or hashed assets |
| Reinforcements | player concept | Timed waves from owned trunk ports: every 60 mode seconds each owned port emits Darwinians and, per mode, armour, turret or air-strike tasks ("Reinforcements in *S"). | [05 §6.1](05_economy_and_population.md#61-trunk-port-reinforcement-waves--multiwiniaadvancetrunkports) | no |
| ReleaseDirect3D | engine (build macro) | The secondary Visual Studio configuration that defines `USE_DIRECT3D`, links Direct3D 9 and emits `darwinia.exe`; only Darwinia-era scripts package it. See [B:D3D]. | [31 §7.3](31_platform_build_and_tools.md#73-the-four-project-configurations--complete-compiler-and-linker-settings-c-targetsvs2005darwiniavcproj-configurations-block), [24 §1.2](24_rendering.md#12-is-the-d3d-configuration-buildable-at-this-commit-i) | no |
| Release\|Win32 | engine (build macro) | The baseline Visual Studio 2005 configuration: OpenGL renderer, `NDEBUG`, output `darwinia_gl.exe`, renamed to `multiwinia.exe` by every Multiwinia packaging script. | [31 §7.3](31_platform_build_and_tools.md#73-the-four-project-configurations--complete-compiler-and-linker-settings-c-targetsvs2005darwiniavcproj-configurations-block) | no |
| Render pass table | engine | The ordered list of passes in one match frame (sky, landscape, water, buildings, entities, lasers, particles, explosions, markers, UI) with full state; maps to a PSO catalogue. | [24 §3.2](24_rendering.md#32-ordered-pass-table--one-multiwinia-match-frame-bfull) | no |
| Renderer | engine (class) | `g_app->m_renderer` (`code/renderer.cpp`): window and context creation, baseline GL state, frame composition, fades, pause banner, screenshots. | [24 §2.1](24_rendering.md#21-renderer-singleton-c-coderendererhrenderer-coderenderercpprendererrenderer), [24 §3](24_rendering.md#3-frame-composition) | no |
| Resource | engine (class) | `g_app->m_resource` (`code/lib/resource.cpp`): archive parsing, file look-up order (archives, then loose `data/`), texture, shape and display-list caches. | [25 §2.9](25_textures_and_assets.md#29-resource-manager-look-up-order-and-archives), [31 §8.8](31_platform_build_and_tools.md#88-runtime-archive-semantics-and-lookup-precedence-c-codelibresourcecppresourceparsearchive-gettextreader-getbinaryreader-getuncompressedfile-listresources-contribunrarunrarcppuncompressedarchiveuncompressedarchive-memmappedfilememmappedfile-contribunrarunrarh-contribsystemivlibtosserhash_tablecpphashtablegetindex) | no |
| RestrictionZone | engine (class) | Invisible circle that prevents crates from landing inside it and can block power-up targeting. | [03 §7.5](03_buildings_and_world_objects.md#75-restrictionzone), [04 §4.6](04_crates_powerups_and_weapons.md#46-restriction-zones-drop-and-targeting-exclusion) | no |
| Results screen | player concept | The end-of-match sequence started by `Multiwinia::DeclareWinner`: scores are broadcast (and ignored by receivers), game speed is ramped to 0 and locked, and the winner and score overlay is shown. | [01 §2.11](01_game_modes.md#211-ending-declarewinnert-and-the-end-of-game-sequence), [09 §3.23](09_meta_systems.md#323-in-match-screens-ready-up-countdown-and-results-owned-by-01) | no |
| Retribution | player concept | Comeback mechanism (option `RetributionMode`): every 45 s a team that `IsEliminated` receives random free powers, limited to 2 task slots while eliminated. | [01 §2.9](01_game_modes.md#29-retribution-comeback-powerups-bfull), [04 §4.4](04_crates_powerups_and_weapons.md#44-retribution-eliminated-team-grants) | yes → Comeback Bonus |
| RGB / RGBA / RGBA8 | abbreviation | Colour channel orders; RGBA8 (8 bits per channel) is the in-memory format of every decoded bitmap. | [25 §2.3](25_textures_and_assets.md#23-pixel-operations-used-by-the-resource-system-and-by-gameplay-code-to-build-run-time-textures) | no |
| RNG | abbreviation | Random number generator; synced (`syncrand` family, identical on all peers) versus unsynced (`AppRandom`, `frand`, CRT `rand`) generators are the key determinism distinction. | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code) | no |
| Rocket | engine (class) | Rocket projectile fired by rocket turrets and by Rocketwinians. | [04 §6.6](04_crates_powerups_and_weapons.md#66-rocket) | no |
| Rocket Riot | player concept | Game type `GameTypeRocketRiot`: capture solar panels to power the fuel chain into your escape rocket, load 100 Darwinians at 95 % fuel, launch; the first rocket above world height y = 600 wins. No time limit, no draw. | [01 §4.5](01_game_modes.md#45-rocket-riot-gametyperocketriot-bfull) | yes → Launch Race |
| Rocketwinians | player concept | Outcome 22 `CrateRewardRocketDarwinians`: the capturer's Darwinians throw rockets instead of grenades for the rest of the match. | [04 §5.23](04_crates_powerups_and_weapons.md#523-rocketwinians) | yes → Rocket Troops |
| Route | engine | A generated path (list of waypoints) held in a slot-shared array; a unit follows it point by point. | [21 §7.5](21_simulation_and_entity_model.md#75-following-a-route), [21 §7.6](21_simulation_and_entity_model.md#76-route-lifetime-ids-and-the-slot-sharing-hazard) | no |
| RoutingSystem / LandRoutingSystem | engine (class) | Path-finding: a waypoint graph over walkable land built at load (`LandRoutingSystem::Initialise`) and route generation between points. | [21 §7](21_simulation_and_entity_model.md#7-pathfinding-and-routing) | no |
| RTS | abbreviation | Real-time strategy, the genre. | [00 §1](00_overview.md#1-pitch) | no |
| RTT | abbreviation | Round-trip time of a network message. | [22 §2.5](22_determinism_and_networking.md#25-estimated-latency-statistics-only) | no |
| Running programs | data format | Level-file block of Darwinia tasks that start with the level ([B:LEGACY]). | [08 §6.7](08_levels_and_level_design.md#67-objectives-running-programs-difficulty-and-leaderboard-blocks) | no |

### S

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| SafeArea | engine (class) | [B:LEGACY] objective circle counting friendly entities. | [03 §12.35](03_buildings_and_world_objects.md#1235-safearea) | no |
| Sample cache (CachedSampleManager) | engine (class) | Lazy decode-to-RAM cache of mono 16-bit PCM samples with LRU eviction above 60 MiB. | [27 §9](27_audio_engine.md#9-sample-cache) | no |
| Sample group | data format | A named set of Ogg samples in `sounds.txt` from which one is picked per play. | [10 §3.5](10_audio_design.md#35-sample-groups) | no |
| Score | player concept | Per-slot integer `Multiwinia::m_score` whose meaning depends on the mode (zone seconds, statues delivered, flags held); the HUD shows it with the delta since the last mode second. | [01 §2.6](01_game_modes.md#26-scores-and-rank-announcements) | no |
| Script | engine (class) | Cut-scene script interpreter (43 opcodes) for `data/scripts/*.txt`; scripts cannot run in Multiwinia. | [30 §2](30_scripting_tutorial_and_cutscenes.md#2-script-language-script) | no |
| ScriptTrigger | engine (class) | [B:LEGACY] invisible trigger that runs a cut-scene script. | [03 §12.36](03_buildings_and_world_objects.md#1236-scripttrigger) | no |
| SDK | abbreviation | Software development kit. | [20 §3.4](20_architecture.md#34-third-party-libraries-and-os-apis) | no |
| SDL | abbreviation | Simple DirectMedia Layer; used only by the Mac/Linux targets, [B:DEAD] on PC. | [31 §2.2](31_platform_build_and_tools.md#22-platform-map--every-platform-specific-file) | no |
| SEH | abbreviation | Windows structured exception handling; the crash handler writes `ERRORLOG.TXT` and `CRASH.DMP`. | [31 §6.2](31_platform_build_and_tools.md#62-seh-crash-report--recordexceptioninfo-c-codemaincppappmain-codelibexception_handlercpprecordexceptioninfo) | no |
| Selection circle | player concept | The circle grown by holding the left button (up to 100 wu) that selects Darwinians for a crowd order (`Location::SelectDarwinians`). | [06 §2](06_player_control_and_input.md#2-selection-model) | no |
| Sepulveda | engine (class) | [B:DEAD] Darwinia mentor dialogue system (portrait, text queue, timing). | [30 §6](30_scripting_tutorial_and_cutscenes.md#6-dr-sepulveda-dialogue-system-bdead) | yes → Advisor (drop) |
| SEQID | abbreviation | Sequence id of a server letter (`NET_DARWINIA_SEQID`); clients execute letters strictly in sequence order, `−1` marks unsequenced letters. | [22 §6](22_determinism_and_networking.md#6-server-sequencing-and-client-ordering) | no |
| Server | engine (class) | `g_app->m_server` (`code/network/server.cpp`), present only on the host: accepts clients, assigns team slots, sequences and broadcasts commands at 10 Hz, keeps the full history, compares sync bytes and advertises the game. | [22 §1.2](22_determinism_and_networking.md#12-peer-roles), [22 §6.1](22_determinism_and_networking.md#61-serveradvance--one-tick-every-01-s-c-codenetworkservercppserveradvance) | no |
| Server browser | player concept | The LAN/WAN list of joinable games (`PageJoinGame`) filled from LAN broadcasts and metaserver listings. | [09 §5.8](09_meta_systems.md#58-lanwan-server-browser-pagejoingame-4) | no |
| SERVER_ADVANCE_PERIOD | engine (build macro) | Macro for `g_gameTimer.GetServerAdvancePeriod()` = `0.1 × m_gameSpeed`: the dt of one tick. | [20 §10.7](20_architecture.md#107-gametimer-api-and-semantics) | no |
| serverHistory.dat | data format | Optional recording of the server's letter history (`Directory` records), preference-gated. | [29 §11.6](29_data_formats_reference.md#116-serverhistorydat--server-message-recording) | no |
| ServerToClientLetter | engine (class) | One server-to-client message (`code/network/servertoclientletter.h`): receiver id, disconnect flag and a `Directory` payload; UPDATE letters carry one tick's commands. | [22 §5.5](22_determinism_and_networking.md#55-server--client-letters-servertoclientletter-every-command-the-server-emits) | no |
| Session | engine | One hosted or joined network game from connection through lobby, matches and leaving; the lifetime class between process and match. | [22 §8](22_determinism_and_networking.md#8-session-lifecycle), [20 §8.1](20_architecture.md#81-lifetime-classes) | no |
| SFX | abbreviation | Sound effects. | [10 §7](10_audio_design.md#7-front-end-ui-hud-voice-and-ambience) | no |
| SHA-1 | abbreviation | Hash function (from the bundled unrar sources) used for the server seed and the debug snapshot hash of an IFrame. | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code), [22 §7.1](22_determinism_and_networking.md#71-two-build-variants-of-iframegenerate) | yes → any public-domain hash |
| Shaman | engine (class) | Entity type 19: a cut game-mode summoner unit; deletes itself on its first tick ([B:LEGACY]). | [02 §27](02_units_and_entities.md#27-shaman) | yes → Summoner |
| Shape (.shp mesh) | engine (class) | A hierarchical flat-shaded mesh (`code/lib/shape.cpp`): tree of `ShapeFragment`s with transforms, triangles, colours and named `ShapeMarker`s. | [24 §4.5](24_rendering.md#45-shape-rendering-shp-meshes-c-codelibshapeh-codelibshapecpp), [29 §8.1](29_data_formats_reference.md#81-shp--hierarchical-flat-shaded-mesh-shape) | no |
| Shape marker | engine | Named point inside a `.shp` mesh (for example `MarkerPort01`, `MarkerDoor`, `MarkerSpiritLink`) that code looks up to place ports, doors, beams and effects. | [25 §4.4](25_textures_and_assets.md#44-marker-vocabulary), [29 §8.1](29_data_formats_reference.md#81-shp--hierarchical-flat-shaded-mesh-shape) | no |
| Shape shattering | engine | The explosion effect that breaks a destroyed mesh into tumbling triangles (`ExplosionManager`). | [26 §8](26_animation_and_vfx.md#8-explosions--shape-shattering) | no |
| ShapeExporter | engine | 3ds Max plug-in (`tools/ShapeExporter`) that writes `.shp` files from `.max` scenes. | [31 §11.1](31_platform_build_and_tools.md#111-shapeexporter-3ds-max--shp-c-toolsshapeexportershapeexportercpp-toolsshapeexportershapeexporterh-toolsshapeexporterdllentrycpp) | no |
| ShapeFragment / ShapeMarker | engine (class) | A node of a shape's hierarchy (geometry plus transform) and a named point attached to a fragment. | [29 §8.1](29_data_formats_reference.md#81-shp--hierarchical-flat-shaded-mesh-shape), [25 §4.3](25_textures_and_assets.md#43-hierarchies-and-markers-every-shp) | no |
| Shockwave | engine (class) | Expanding damage ring created by every `Bang`. | [04 §6.11](04_crates_powerups_and_weapons.md#611-shockwave) | no |
| .shp (shape) | data format | Text mesh format of `data/shapes/` (139 files): a fragment hierarchy with transforms, vertices, coloured triangles and named markers, parsed by `Shape::Load`. | [29 §8.1](29_data_formats_reference.md#81-shp--hierarchical-flat-shaded-mesh-shape), [25 §4](25_textures_and_assets.md#4-shape-catalogue) | yes → new meshes (format may be kept) |
| Simulated vs cosmetic state | engine | The determinism boundary: state that must evolve identically on all peers (positions, health, synced RNG) versus render-only state (particles, prediction, sound). | [21 §10.1](21_simulation_and_entity_model.md#101-classification-of-every-major-subsystem), [22 §14.10](22_determinism_and_networking.md#1410-simulated-vs-cosmetic-state) | no |
| SKU | abbreviation | Stock-keeping unit: one retail edition of the product, chosen at data-build time (language set, exclusion list, distributor `origin` tag). | [31 §8.2](31_platform_build_and_tools.md#82-invocation-and-sku-arguments-c-toolsbuild_multiwinia_databat-toolsbuild_language_datbat), [09 §10.3](09_meta_systems.md#103-language-packaging-per-sku) | no |
| Slice | engine | One tenth of a simulation tick: every slice-partitioned container advances about a tenth of its slots per slice (`NUM_SLICES_PER_FRAME` = 10), so each object advances exactly once per tick. | [21 §5.4](21_simulation_and_entity_model.md#54-slice-partitioning), [20 §10.3](20_architecture.md#103-client-tick-and-slices) | no |
| SliceDArray | engine (class) | `FastDArray` that also hands out per-slice index ranges (`GetNextSliceBounds`) so a loop over all members can be spread across the 10 slices of a tick (`code/lib/slice_darray.h`). | [21 §5.4](21_simulation_and_entity_model.md#54-slice-partitioning) | no |
| Slow Down | player concept | Outcome 29 `CrateRewardSlowDownTime` (immediate, global): game speed eases to 0.2–0.4× for 15 s of ticks. | [04 §5.28](04_crates_powerups_and_weapons.md#528-slow-down) | yes → Time Dilation |
| SmokeMarker | engine (class) | Emitter of a continuous rising smoke column (atmosphere only). | [03 §7.6](03_buildings_and_world_objects.md#76-smokemarker) | no |
| Snow | engine (class) | Ambient flake of the level effect `snow` or Christmas mode. | [04 §6.19](04_crates_powerups_and_weapons.md#619-snow) | no |
| SolarPanel (solar panel) | player concept; engine (class) | 5-port capturable power source that emits surges into the power network (Rocket Riot fuel, laser fences). | [03 §6.2](03_buildings_and_world_objects.md#62-solarpanel) | yes → Energy Collector |
| Soul | player concept | Synonym of spirit used in strings, sound names and design notes; see Spirit. | [02 §6](02_units_and_entities.md#6-spirit), [05 §5](05_economy_and_population.md#5-spirit-soul-lifecycle) | yes → Essence orb |
| SoulDestroyer (Soul Destroyer) | engine (class) | Entity type 16: fast flying serpent that dives through enemies and destroys their spirits; the strongest monster (at most 2 alive in multiplayer). | [02 §21](02_units_and_entities.md#21-souldestroyer) | yes → Reaper Serpent |
| SOUND_EDITOR | engine (build macro) | [B:DEBUG] switch for the sound editor, sound statistics, profile windows and the POKEY `EffectProcessor`. | [27 §13.2](27_audio_engine.md#132-reachability-of-the-tools-bdebug) | no |
| SoundInstance | engine (class) | One playing or queued sound: blueprint parameters, position type, envelope, loop and priority. | [27 §6](27_audio_engine.md#6-sound-instances-and-parameters) | no |
| SoundLibrary3d / SoundLibrary2d | engine (class) | Device back-end interfaces: DirectSound 3-D (live), a software mixer over waveOut, and null devices. | [27 §11](27_audio_engine.md#11-device-back-ends-2d3d-library-abstraction) | no |
| SoundParameter | engine (class) | The universal modulatable sound value: type 0 fixed, 1 ranged random (unsynced, smoothed), 2 linked (clamped linear map of an input such as distance or speed); every instance and DSP parameter is one. | [27 §6.7](27_audio_engine.md#67-soundparameter-evaluation-engine-semantics), [10 §3.3](10_audio_design.md#33-soundparameter--the-universal-modulatable-value) | no |
| sounds.txt | data format | `data/sounds.txt`: sound-source blueprints (object type → events → instance parameter blocks and sample groups). | [29 §7.4](29_data_formats_reference.md#74-soundstxt--sound-event-blueprints), [10 §3](10_audio_design.md#3-the-soundstxt-content-model-every-instance-parameter) | yes → new event set |
| SoundSourceBlueprint / SoundEventBlueprint | engine (class) | The `data/sounds.txt` model: per object type, a list of named events, each with sound-instance parameter blocks. | [27 §1.4](27_audio_engine.md#14-class-catalogue), [10 §2](10_audio_design.md#2-the-binding-model-object-types--event-names--blueprint--samples) | no |
| SoundStreamDecoder | engine (class) | Ogg Vorbis and WAV decoder feeding the sample cache and the music channel. | [27 §10](27_audio_engine.md#10-streaming-decoder-ogg-vorbis--wav) | no |
| SoundSystem | engine (class) | `g_app->m_soundSystem` (`code/sound/soundsystem.cpp`): loads sound blueprints, triggers events into sound instances and updates voices at 20 Hz. | [27 §1](27_audio_engine.md#1-architecture-layering-and-build-reality), [27 §3](27_audio_engine.md#3-update-rate-the-20-hz-tick) | no |
| Space Invader (name) | IP name | Bomber class name echoing a third-party arcade trademark. | [02 §11](02_units_and_entities.md#11-airstrike) | yes → Bomber |
| Space Ship | player concept | Outcome 26 `CrateRewardSpaceShip` (bad pool, once per game, not Assault): a UFO abducts Darwinians for 60–120 s and drops as many (at most 100) Futurewinians. | [04 §5.25](04_crates_powerups_and_weapons.md#525-space-ship), [02 §23](02_units_and_entities.md#23-spaceship) | yes → Abductor |
| SpaceInvader | engine (class) | Entity type 10: one bomber of an airstrike (or napalm strike); leaves no spirit. | [02 §11.3](02_units_and_entities.md#113-spaceinvader--stats) | yes → Bomber |
| SpaceShip | engine (class) | Entity type 25: the Space Ship outcome's UFO that abducts Darwinians and drops Futurewinians. | [02 §23](02_units_and_entities.md#23-spaceship) | yes → Abductor |
| Spam | engine (class) | [B:LEGACY] Darwinia "Spam" building emitting SpamInfection; the live Infection outcome reuses only `SpamInfection`. | [03 §12.24](03_buildings_and_world_objects.md#1224-spam) | yes → drop (legacy building; the Infection effect is Swarm) |
| SpamInfection | engine (class) | Homing infection streak of the Infection outcome that attacks entities and converts spirits. | [04 §6.16](04_crates_powerups_and_weapons.md#616-spaminfection) | yes → Swarm |
| Spawn Mania | player concept | Outcome 20 `CrateRewardSpawnMania` (immediate, global): for 5 s every owned spawn point requests a spirit every tick. | [04 §5.21](04_crates_powerups_and_weapons.md#521-spawn-mania), [05 §3.6](05_economy_and_population.md#36-spawn-mania) | yes → Spawn Surge |
| Spawn network | player concept | The level's tree of master spawn point, spawn links and spawn points along which virtual spirits travel. | [03 §4](03_buildings_and_world_objects.md#4-spawn-network), [05 §2](05_economy_and_population.md#2-territory-capture-spawn-points) | yes → Source Node, Relays, Spawn Nodes |
| SpawnBuilding | engine (class) | Abstract base of the spawn network (SpawnLink, MasterSpawnPoint, SpawnPoint): outgoing links and the virtual spirits travelling on them. | [03 §4.2](03_buildings_and_world_objects.md#42-spawnbuilding) | no |
| SpawnBuildingLink | engine (class) | One outgoing spawn-network link: neighbour id, the spawn points reachable through it, and the spirits in transit. | [03 §4.2](03_buildings_and_world_objects.md#42-spawnbuilding) | no |
| SpawnBuildingSpirit | engine (class) | A virtual spirit in transit on a spawn link (target spawn point and progress 0–1); drawn only, never interceptable. | [03 §4.2](03_buildings_and_world_objects.md#42-spawnbuilding) | no |
| SpawnLink (spawn link) | engine (class) | Pure relay node of the spawn tree; indestructible, no ports. | [03 §4.3](03_buildings_and_world_objects.md#43-spawnlink) | yes → Relay |
| SpawnPoint (spawn point) | engine (class) | The territory node of the economy: a 5-port capturable building that, when owned and activated, requests virtual spirits and emits Darwinians for its owner. | [03 §4.5](03_buildings_and_world_objects.md#45-spawnpoint), [05 §2](05_economy_and_population.md#2-territory-capture-spawn-points) | yes → Spawn Node |
| SpawnPopulationLock (population lock) | engine (class) | Invisible regional population limiter (radius 500, max 100 by default), used only when a map has no `PopulationCap`; inert on shipped maps. | [03 §4.6](03_buildings_and_world_objects.md#46-spawnpopulationlock), [05 §4.8](05_economy_and_population.md#48-spawnpopulationlock-fallback-limiter-bfull-inert-on-shipped-maps) | yes → Population Limiter |
| Speccy / editor / square (fonts) | IP name | The three bitmap fonts of the game, including CJK and Hangul pages. | [25 §2.15](25_textures_and_assets.md#215-fonts-and-atlases), [28 §8.2](28_ui_framework.md#82-fonts-and-text-blend-modes-c-codelibtext_renderercpp-codeappcppappsetlanguage) | yes → licensed open fonts |
| Spectator | player concept | A connected participant without a team (team type `TeamTypeSpectator`); the dedicated spectator UI is [B:DEAD]. | [22 §8.8](22_determinism_and_networking.md#88-reconnect-late-join-spectators-kick-password) | no |
| Speed Up | player concept | Outcome 30 `CrateRewardSpeedUpTime` (immediate, global): game speed eases to 1.3–2.0× for 15 s of ticks. | [04 §5.29](04_crates_powerups_and_weapons.md#529-speed-up) | yes → Time Rush |
| Spider | engine (class) | Entity type 11: large eight-legged walker that pounces on enemies from up to 150 wu and lays Virii eggs. | [02 §19](02_units_and_entities.md#19-spider) | no (generic word) |
| SpiderParameters | engine (class) | One of the spider's three gait parameter sets (leg lift, slope, swing duration, lift delay, look-ahead, ideal speed). | [02 §19.6](02_units_and_entities.md#196-spiderparameters--gait-parameter-set) | no |
| Spirit | player concept; engine (class) | The soul a dying entity leaves (not an `Entity`; stored by value in `Location::m_spirits`): rises 6 s, floats 30–50 s in multiplayer (collectable), then ascends for 90 s and vanishes. | [02 §6](02_units_and_entities.md#6-spirit), [05 §5](05_economy_and_population.md#5-spirit-soul-lifecycle) | yes → Essence orb |
| SpiritStore | engine (class) | Helper container of stored spirits (used by Darwinia buildings). | [03 §12.9](03_buildings_and_world_objects.md#129-spiritstore) | no |
| SporeGenerator (Spore Generator) | engine (class) | Entity type 6: hovering four-tailed creature that drifts near its spawn point and lays Virii eggs near spirits. | [02 §17](02_units_and_entities.md#17-sporegenerator) | yes → Brood Drifter |
| Squad (crate outcome) | player concept | Outcome 0 `CrateRewardSquad` (basic): a placed task spawning a 3-man insertion squad that lives 60 s. | [04 §5.1](04_crates_powerups_and_weapons.md#51-squad) | no (generic; placeholder Strike team) |
| Squadie | engine (class) | Entity type 4 (`TypeInsertionSquadie`, level name "Squadie"): one soldier of an insertion squad; leaves no spirit. | [02 §7.1](02_units_and_entities.md#71-insertionsquad-and-squadie--role-and-acquisition) | yes → Trooper |
| SSE2 | abbreviation | x86 SIMD instruction set; not enabled in the shipped build (`/arch:SSE2` absent). | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model) | no |
| START_GAME | engine | The lobby letter that starts a match on every peer: loading begins on a worker thread, the game timer is reset to speed 0 and buffered frames are caught up afterwards; the server relays only the first one of its lifetime. | [22 §8.5](22_determinism_and_networking.md#85-game-start) | no |
| Starting powerups | player concept | Option `StartingPowerups` (Domination, KOTH): 0–3 random powers granted to every team at match start. | [04 §4.3](04_crates_powerups_and_weapons.md#43-startingpowerups) | no |
| Starting units | player concept | Forces present at match start: the level's `InstantUnits` plus 5 Darwinians seeded on every port of every team-owned spawn point and solar panel. | [05 §6.7](05_economy_and_population.md#67-starting-units) | no |
| StartSequence / DemoEndSequence | engine (class) | First-time intro sequence ([B:LEGACY], never constructed) and demo end sequence ([B:DEAD]). | [30 §5](30_scripting_tutorial_and_cutscenes.md#5-start-sequence-logo-splash-and-demo-end-sequence) | no |
| StaticShape | engine (class) | Places any `.shp` mesh as a solid, non-interactive obstacle. | [03 §7.4](03_buildings_and_world_objects.md#74-staticshape) | no |
| stats.txt | data format | `data/stats.txt`: one row per entity type (name, health, speed, rate), bound by row position. | [29 §7.1](29_data_formats_reference.md#71-statstxt--per-entity-type-base-stats), [02 §3.5](02_units_and_entities.md#35-entityblueprint--the-statstxt-table) | no |
| Statue | player concept; engine (class) | `Statue` (CarryableBuilding), created at run time in CTS only: a relic lifted by enough Darwinians and carried; delivered within 30 wu of an owned drop zone it scores 1 point. | [03 §8.4](03_buildings_and_world_objects.md#84-statue) | yes → Relic |
| STRETCH_RECT | engine (build macro) | Defined in `code/deform.cpp`; selects the StretchRect capture path of the [B:D3D] deform effect, which makes its pre-render `Start()` a no-op (source comment: it "should make FSAA possible"). | [24 §4.13](24_rendering.md#413-deform-effect-screen-space-distortion-bd3d-c-codedeformh-codedeformcpp) | no |
| Subversion | player concept | Outcome 8 `CrateRewardSubversion`: for up to 30 s own Darwinians within 70 wu fire lasers that convert enemy Darwinians. | [04 §5.9](04_crates_powerups_and_weapons.md#59-subversion) | yes → Convert |
| Sudden death | player concept | Option `SuddenDeath`: on a tied score at time-up every team not on the top score is wiped and the next single leader wins; KOTH instead adds one-minute extensions. | [01 §2.10](01_game_modes.md#210-sudden-death-shared-shape) | no |
| Sync byte | engine | The one byte per frame each client sends so the server can detect divergence; in FULL it only reveals divergence of the synced RNG stream position. | [22 §9.1](22_determinism_and_networking.md#91-what-the-per-frame-sync-byte-covers) | no |
| Synced RNG (syncrand, syncfrand, syncsfrand) | engine | The deterministic MT19937 stream shared by all peers (`contrib/systemiv/lib/math/random_number.*`), seeded per session by the server; `syncfrand(r)` ∈ [0, r), `syncsfrand(r)` ∈ [−r/2, r/2). | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code), [22 §14.3](22_determinism_and_networking.md#143-seeding-and-lifetime-of-the-synced-stream) | no |
| SystemIV | engine | Introversion's shared utility library (`contrib/systemiv`): in Multiwinia only containers (tosser), NetLib, metaserver, maths and RNG, file readers, Unicode strings, debug utilities and the profiler are compiled. | [20 §3.1](20_architecture.md#31-module-inventory), [31 §8.10](31_platform_build_and_tools.md#810-systemiv-filesystem-compiled-unused-c-contribsystemivlibfilesysfile_systemcpp) | yes → own or standard libraries |

### T

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Tank | engine (class) | Entity type 24: player-driven rocket tank, the only unit of Tank Battle; unobtainable in FULL. | [02 §25](02_units_and_entities.md#25-tank) | no (generic) |
| Tank Battle | player concept | Game type `GameTypeTankBattle`, compiled only with `INCLUDEGAMEMODE_TANKBATTLE` ([B:DEBUG]): each team has `NumTanks` tank lives; last team with tanks wins. | [01 §4.7](01_game_modes.md#47-tank-battle-gametypetankbattle-bdebug) | yes → Armour Duel |
| TARGET_DEBUG | engine (build macro) | Developer product target: adds Tank Battle, Prologue/Campaign menu entries, the cheat menu, sound and gesture editors, AVI capture, `TRACK_SYNC_RAND` and The Beast crate. Source of [B:DEBUG]. | [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line), [32 §2.1](32_debug_test_and_performance.md#21-product-level-debug-switches) | no |
| TARGET_FOR_NEW_GAME | engine | The target platform of the new game: Windows, C++23, DirectX 12, multiplayer. | [40 §3](40_carry_forward_assessment.md#3-obsolete-technology-mapping) | no |
| TARGET_MULTIWINIA_DEMOONLY | engine (build macro) | Commented-out product target for a stand-alone demo executable (single-player only, KOTH and CTS, basic crates). Distinct from the runtime `IS_DEMO` gate. | [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line), [09 §6.1](09_meta_systems.md#61-definitions-and-the-demo-limits) | no |
| TARGET_PC_FULLGAME | engine (build macro) | The single active product target in `code/lib/universal_include.h`; it enables LAN/WAN play, all six shipped modes, basic and advanced crates, the tutorial and `BLOCK_OLD_DARWINIA_OBJECTS`. | [31 §7.6](31_platform_build_and_tools.md#76-product-targets-and-the-feature-macros-each-sets-c-codelibuniversal_includeh-verified-line-by-line) | no |
| Task (power, power-up) | player concept; engine | A granted ability (unit, strike, turret, buff) held by the `TaskManager` until the player places it on the map; called "program" in the Darwinia fiction. | [06 §3](06_player_control_and_input.md#3-task-manager) | yes → Power |
| TaskManager / Task | engine (class) | Per-team container of granted powers (`code/taskmanager.h`): 6 slots in multiplayer; each `Task` moves through Idle, Started (waiting to be placed), Running and Stopping. | [06 §3](06_player_control_and_input.md#3-task-manager) | yes → Power slots / Power |
| TaskManagerInterfaceMultiwinia | engine (class) | The live task-bar interface: power slots at the screen edge, the running-task bar and the "power-up received" pop-up. | [06 §4.3](06_player_control_and_input.md#43-taskmanagerinterfacemultiwinia--the-live-interface-bfull) | no |
| TCP / UDP | abbreviation | Transport protocols; the game uses only UDP (game ports 4000/4001, metaserver 5113, LAN 5009). | [22 §3](22_determinism_and_networking.md#3-transport--netlib-over-udp) | no |
| Team | engine (class) | Per-match team container (`code/team.h`, `Location::m_teams[6]`): units, `m_others`, task manager, selection, colour and team type, bound by reference to its `LobbyTeam`. | [21 §9.2](21_simulation_and_entity_model.md#92-lobbyteam-and-team) | no |
| Team colour | player concept | One of eight colours (green, red, yellow, blue, orange, purple, cyan, pink) chosen in the lobby and applied to units, buildings and the HUD; co-op uses colour families. | [01 §2.5.3](01_game_modes.md#253-colour-tables-rgb-0255-c-codemultiwiniacpp-teamcolours-groupcolours1-groupcolours2-defaultcolour) | no |
| Team slot | player concept; engine | One of `NUM_TEAMS` = 6 team indices: 0–3 players, 4 the monster team, 5 the Futurewinian team; 255 = none. | [21 §9.1](21_simulation_and_entity_model.md#91-team-slots-and-ids) | no |
| Team type | engine | `Team::m_teamType`: Unused (−1), LocalPlayer (0), RemotePlayer (1), CPU (2), Spectator (3). | [21 §9.1](21_simulation_and_entity_model.md#91-team-slots-and-ids) | no |
| TeamControls | engine (class) | The continuous per-team input packet (cursor world position, button states, unit-control flags) packed into `ALIVE_DATA` every frame and executed by `Location::UpdateTeam`. | [21 §9.7](21_simulation_and_entity_model.md#97-teamcontrols--the-continuous-per-team-input-packet), [06 §6.1](06_player_control_and_input.md#61-teamcontrols--the-continuous-input-packet) | no |
| Teleport | engine (class) | Abstract base of RadarDish and the legacy Bridge: entrance marker, send period and entity transport. | [03 §5.2](03_buildings_and_world_objects.md#52-teleport) | no |
| Tentacle | engine (class) | Entity type 22: portal tentacle that slams and swipes; spawned only by The Beast's portal ([B:DEBUG]). | [02 §30](02_units_and_entities.md#30-tentacle) | no (generic) |
| Test harness | engine | [B:DEAD] campaign test harness (`TEST_HARNESS_ENABLED`). | [32 §7.1](32_debug_test_and_performance.md#71-campaign-test-harness-testharness-bdead-c-codetestharnesscpp-codemaincpprunthegame-codeglobal_worldcppglobaleventactionexecute) | no |
| Testbed | engine | [B:DEAD] automated server/client test sequence driven by `testbedconfig.txt`. | [32 §6](32_debug_test_and_performance.md#6-testbed-mode) | no |
| TESTBED_ENABLED | engine (build macro) | Never defined: automated two-machine test bed with `testbedconfig.txt`; also stretches `GRACE_TIME` to 20 s. | [32 §6](32_debug_test_and_performance.md#6-testbed-mode) | no |
| testbedconfig.txt | data format | [B:DEAD] automated test-bed sequence. | [29 §11.15](29_data_formats_reference.md#1115-testbedconfigtxt--automated-test-bed-sequence-bdead), [32 §6.3](32_debug_test_and_performance.md#63-testbedconfigtxt-c-codeuigamemenuwindowcppgamemenuwindowreadtestbedconfig) | no |
| TextReader | engine (class) | Line reader and tokeniser shared by every text format (comments, separators, `redshirt2` detection); implemented by text-file and Unicode-file readers. | [29 §3.1](29_data_formats_reference.md#31-textreader-line-reader-and-tokeniser) | no |
| TextRenderer | engine (class) | Bitmap-font text drawing (`code/lib/text_renderer.cpp`) with per-language font pages. | [24 §4.22](24_rendering.md#422-text-rendering-and-font-atlases-incl-unicodecjk-c-codelibtext_renderercpp-codeappcppappsetlanguage), [28 §8.2](28_ui_framework.md#82-fonts-and-text-blend-modes-c-codelibtext_renderercpp-codeappcppappsetlanguage) | no |
| The Beast | player concept | Outcome 31 `CrateRewardTheBeast` ([B:DEBUG], `INCLUDE_THEBEAST`): a monster-team `Portal` that summons tentacles. | [04 §5.30](04_crates_powerups_and_weapons.md#530-the-beast-bdebug), [03 §9.3](03_buildings_and_world_objects.md#93-portal) | yes → Rift Horror |
| ThrowableWeapon | engine (class) | Abstract base of thrown objects (`throwable.shp`): ballistic flight with bounce energy. | [04 §6.2](04_crates_powerups_and_weapons.md#62-throwableweapon) | no |
| Thumbnails / mwhelp / icons naming | data format | Naming conventions of `data/thumbnails/` (one per map), `data/mwhelp/` (help images) and `data/icons/` (task, cursor, marker and HUD icons). | [29 §8.7](29_data_formats_reference.md#87-dataicons-naming-convention), [29 §8.8](29_data_formats_reference.md#88-datamwhelp-naming-convention), [29 §8.9](29_data_formats_reference.md#89-datathumbnails-naming-convention) | yes → new art |
| Tick | engine | One simulation step = one server UPDATE letter, produced every 0.1 s of real time (10 Hz) and advancing game time by `0.1 × game speed`. | [20 §10.2](20_architecture.md#102-server-tick--10-hz-of-real-time), [22 §2.1](22_determinism_and_networking.md#21-server-tick) | no |
| Time limit | player concept | Lobby option `TimeLimit` (mode minutes, 0 = unlimited) after which scores decide, or sudden death starts on a tie. | [01 §3.3](01_game_modes.md#33-option-semantics-by-name) | no |
| TM | abbreviation | Task manager, the slot list of granted powers. | [06 §3](06_player_control_and_input.md#3-task-manager) | yes → Power slots |
| Tosser | engine | SystemIV's container sub-library (`contrib/systemiv/lib/tosser`): `LList`, `DArray`, `FastDArray`, `BTree`, `HashTable`, `Directory`. | [20 §3.1](20_architecture.md#31-module-inventory) | no |
| TRACK_SYNC_RAND | engine (build macro) | [B:DEBUG] switch that logs every synced random draw with file and line for desync forensics; its tracked `syncsfrand` is not bit-identical to the untracked one. | [32 §13.3](32_debug_test_and_performance.md#133-sync-rng-call-tracking-track_sync_rand-bdebug-c-contribsystemivlibmathrandom_numberh-contribsystemivlibmathrandom_numbercpp-codelibvector3h-codelibvector3cpp), [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code) | no |
| Tree | player concept; engine (class) | Procedurally drawn tree that blocks movement and projectiles, burns, regrows, spreads; magic trees drop spirits, evil trees kill Darwinians. | [03 §7.3](03_buildings_and_world_objects.md#73-tree) | no (generic) |
| Trees (magic and evil) | player concept | Scenery `Tree` buildings: flammable, regrowing, blocking; Magical Forest trees drop spirits, Dark Forest ("evil") trees kill Darwinians and corrupt neighbours. | [03 §7.3](03_buildings_and_world_objects.md#73-tree) | no |
| Triffid | engine (class) | Monster plant that launches triffid eggs (Triffids outcome). | [03 §9.2](03_buildings_and_world_objects.md#92-triffid) | yes → Spitter Plant |
| Triffid (name) | IP name | Third-party literary name used for the egg-launching plant and its crate outcome. | [03 §9.2](03_buildings_and_world_objects.md#92-triffid), [04 §5.26](04_crates_powerups_and_weapons.md#526-triffids) | yes → Spitter Plant |
| TriffidEgg | engine (class) | Entity type 17: a timed monster egg launched by a triffid (or the Eggs task) that bursts into a creature group of its spawn type. | [02 §14](02_units_and_entities.md#14-triffidegg) | yes → Seed Pod |
| Triffids | player concept | Outcome 27 `CrateRewardTriffids` (bad pool only): 1–3 monster-team egg-launching plants near the crate. | [04 §5.26](04_crates_powerups_and_weapons.md#526-triffids), [03 §9.2](03_buildings_and_world_objects.md#92-triffid) | yes → Spitter Plants |
| Tripod | engine (class) | Entity type 8: three-legged walker firing twin lasers; nothing spawns it ([B:LEGACY]). | [02 §29](02_units_and_entities.md#29-tripod) | yes → Tri-walker |
| TripodNavData | engine (class) | Hex-direction navigator (six unit directions 60° apart) used by the Tripod. | [02 §29.2](02_units_and_entities.md#292-tripodnavdata--hex-direction-navigator) | no |
| TrueSkill | IP name | Microsoft's rating-system trademark named in leaderboard remnants. | [09 §7.5](09_meta_systems.md#75-leaderboards-blegacy--no-data-source) | yes → Skill rating |
| TrunkPort (trunk port) | player concept; engine (class) | Fixed gate that emits reinforcement waves for its owner (and, in Darwinia, linked to other locations). | [03 §8.6](03_buildings_and_world_objects.md#86-trunkport), [05 §6.1](05_economy_and_population.md#61-trunk-port-reinforcement-waves--multiwiniaadvancetrunkports) | yes → Warp Gate |
| TTL | abbreviation | Time to live (expiry of a cached server listing or packet). | [22 §12.6](22_determinism_and_networking.md#126-client-side-server-list-c-contribsystemivlibmetaservermetaservercpp) | no |
| TurretShell / TurretEmptyShell | engine (class) | Standard gun-turret shell (0.6 s, 500 wu/s) and its ejected casing. | [04 §6.8](04_crates_powerups_and_weapons.md#68-turretshell), [04 §6.9](04_crates_powerups_and_weapons.md#69-turretemptyshell) | no |
| Tutorial | player concept | The two live Multiwinia tutorials ("The Basics" on `mp_koth_2P_1`, "Advanced Concepts" on `mp_koth_3P_1`), driven by `MultiwiniaTutorial` with scripted CPU behaviour. | [30 §3](30_scripting_tutorial_and_cutscenes.md#3-live-multiwinia-tutorials-bfull) | yes → new tutorial maps and copy |

### U

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| [U] | evidence tag | Unknown: the source tree cannot settle the point; every such item is numbered NN-U<n> in its document's "Open questions and unknowns" with what would resolve it. | [00 Open questions and unknowns](00_overview.md#open-questions-and-unknowns) | no |
| UI / UX | abbreviation | User interface; user experience (tables use UX for a player-facing quirk). | [28 §1](28_ui_framework.md#1-architecture-layering-and-build-reality) | no |
| UnicodeString | engine (class) | SystemIV wide string; single-byte text is widened byte-for-byte (ISO-8859-1). | [28 §10.4](28_ui_framework.md#104-unicode-strings-c-contribsystemivlibunicodeunicode_string-p14-29l6), [29 §3.1](29_data_formats_reference.md#31-textreader-line-reader-and-tokeniser) | no |
| Unit (group container) | engine (class) | `code/unit.h:Unit`: a team-owned group of same-type entities moving together (insertion squads, bomber flights, Virii groups, centipede segments, instant units). | [21 §9.6](21_simulation_and_entity_model.md#96-unit--groups-of-entities-in-formation), [21 §4.5](21_simulation_and_entity_model.md#45-unit-group-container) | no |
| UnprocessedSpirit | engine (class) | [B:LEGACY] floating spirit cloud object owned by a SpiritProcessor. | [03 §12.8](03_buildings_and_world_objects.md#128-unprocessedspirit) | no |
| Unrar (UncompressedArchive, MemMappedFile) | engine | The linked `contrib/unrar` library that opens `.dat` archives into memory-mapped files; not thread-safe, which serialises sound loading. | [31 §8.8](31_platform_build_and_tools.md#88-runtime-archive-semantics-and-lookup-precedence-c-codelibresourcecppresourceparsearchive-gettextreader-getbinaryreader-getuncompressedfile-listresources-contribunrarunrarcppuncompressedarchiveuncompressedarchive-memmappedfilememmappedfile-contribunrarunrarh-contribsystemivlibtosserhash_tablecpphashtablegetindex), [20 §3.1](20_architecture.md#31-module-inventory) | no |
| UPDATE (letter) | engine | The per-tick server letter carrying every command of that tick in order; exactly one per tick even if empty, numbered by sequence id. | [22 §6.1](22_determinism_and_networking.md#61-serveradvance--one-tick-every-01-s-c-codenetworkservercppserveradvance) | no |
| UpgradePort / PrimaryUpgradePort | engine (class) | [B:LEGACY] Darwinia upgrade buildings with no live placement. | [03 §12.38](03_buildings_and_world_objects.md#1238-upgradeport) | no |
| URL | abbreviation | Web address; store, metaserver and crash-report URLs are Introversion endpoints. | [22 §12.1](22_determinism_and_networking.md#121-components-and-endpoints-c-codeappcppappinitmetaserver-contribsystemivlibmetaservermetaserver_definesh) | yes → own endpoints |
| USE_CRASHREPORTING / DUMP_DEBUG_LOG | engine (build macro) | Global crash-report hook (structured-exception report and support tool) and the switch that writes `debug.txt`; DUMP_DEBUG_LOG is on in FULLGAME. | [31 §6](31_platform_build_and_tools.md#6-crash-handling-and-system-information), [32 §10.1](32_debug_test_and_performance.md#101-debug-output-appdebugout-and-debugtxt-c-contribsystemivlibdebug_utilscpp-codemaincppinitialise) | no |
| USE_DIRECT3D | engine (build macro) | Set by `Direct3D.vsprops`; replaces native OpenGL with the `code/lib/opengl_directx*` emulation layer over Direct3D 9. | [24 §1.1](24_rendering.md#11-how-the-backend-is-chosen-c) | no |
| USE_DISPLAY_LISTS | engine (build macro) | Enables OpenGL display lists for shapes, trees and the global internet; emulated under Direct3D by `opengl_directx_dlist*`. | [24 §4.5](24_rendering.md#45-shape-rendering-shp-meshes-c-codelibshapeh-codelibshapecpp), [25 §2.11](25_textures_and_assets.md#211-shapes-cache-and-display-lists) | no |
| USE_LOADERS | engine (build macro) | Commented out: gates the boot-loader mini-programs and the first-time start sequence, so they are [B:DEAD]. | [09 A.1 Boot-loader framework and mini-programs [B:DEAD] (USE_LOADERS commented out)](09_meta_systems.md#a1-boot-loader-framework-and-mini-programs-bdead-use_loaders-commented-out) | no |
| USE_SEPULVEDA_HELP_TUTORIAL | engine (build macro) | Never defined: gates the Darwinia mentor, help system and tutorial code (49 files), all [B:DEAD]. | [30 §6.1](30_scripting_tutorial_and_cutscenes.md#61-build-gating) | no |
| UserInput / LocationInput | engine (class) | Per-frame input pumps: UI and Eclipse mouse handling, and the in-match translation of controls into network commands. | [06 §1.2](06_player_control_and_input.md#12-pipeline-and-authority-model), [20 §4.2](20_architecture.md#42-subsystem-pointer-members--owner-and-lifetime) | no |
| UTF-8 / UTF-16 | abbreviation | Unicode encodings. The non-English language tables are UTF-16 (big-endian for French, German, Italian, Korean, Portuguese, Spanish; little-endian for Japanese, Russian); preferences are saved as UTF-16LE. | [31 §12.3](31_platform_build_and_tools.md#123-text-encodings-shipped-c-file-inspection-of-datalanguage-contribsystemivlibunicodeunicode_text_stream_readercpp) | no |
| UV | abbreviation | Texture coordinates. | [24 §4.5](24_rendering.md#45-shape-rendering-shp-meshes-c-codelibshapeh-codelibshapecpp) | no |

### V

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| VBO | abbreviation | OpenGL vertex buffer object. | [24 §4.1](24_rendering.md#41-landscape-renderer-landscaperenderer-bfull-c-codelandscape_rendererh-codelandscape_renderercpp) | no |
| Vector3 / Matrix34 | engine (class) | Double-precision maths types of `code/lib`; a second SystemIV `Vector3` coexists. | [22 §14.8](22_determinism_and_networking.md#148-floating-point-model), [31 §7.7](31_platform_build_and_tools.md#77-other-global-compile-time-configuration-c-codelibuniversal_includeh) | no |
| VFX | abbreviation | Visual effects: particles, beams, explosions, weather. | [26 §1](26_animation_and_vfx.md#1-scope-clocks-and-conventions) | no |
| Virii | engine (class) | Entity type 3: fast red worm creatures with glowing trails that bite enemies and ferry spirits into eggs. | [02 §16](02_units_and_entities.md#16-virii) | yes → Crawler |
| ViriiHistory | engine (class) | One sample of a Virii trail (position, side vector, glow step, distance). | [02 §16.7](02_units_and_entities.md#167-viriihistory--trail-sample) | no |
| ViriiUnit | engine (class) | `Unit` container that batch-renders a Virii group and pre-checks for enemies. | [02 §16.6](02_units_and_entities.md#166-viriiunit--virii-group-container) | no |
| Virtual spirit | player concept; engine | A `SpawnBuildingSpirit`: a token travelling along spawn links from the master spawn point to a requesting spawn point (1 s per link) that becomes one Darwinian on arrival; it has no position and cannot be intercepted. | [03 §4.2](03_buildings_and_world_objects.md#42-spawnbuilding), [05 §3.4](05_economy_and_population.md#34-spirit-transit-through-the-spawn-network) | no |
| VK | abbreviation | Windows virtual-key code. | [06 §11.6.5](06_player_control_and_input.md#1165-w32-keymouse--driver-4) | no |
| Voice (channel) | engine | One playback channel of the sound back-end; with DirectSound the effect channels number `min(SoundChannels preference, 64) − 1` (fewer with hardware 3-D), and sound instances compete for them by priority. | [27 §5](27_audio_engine.md#5-voice-channel-management), [10 §9](10_audio_design.md#9-priorities-and-voice-limits) | no |
| VS / PS | abbreviation | Vertex shader and pixel shader (HLSL, [B:D3D]); also Visual Studio. | [24 §6](24_rendering.md#6-shaders-datashaders--all-bd3d) | no |

### W

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Wall | player concept; engine (class) | Straight wall segment that blocks movement and fire; destroyed only by a single heavy hit (the Assault wall buster). | [03 §7.2](03_buildings_and_world_objects.md#72-wall) | no (generic) |
| WallBuster (wall buster) | player concept; engine (class) | Assault carryable explosive charge (10–30 lifters) that breaches walls. | [03 §8.5](03_buildings_and_world_objects.md#85-wallbuster) | yes → Breach Charge |
| Water | engine (class) | Sea renderer: flat water tiles, shore mask, depth map and dynamic coastline waves. | [24 §4.2](24_rendering.md#42-water-renderer-water-bfull-c-codewaterh-codewatercpp), [23 §13](23_terrain_and_landscape.md#13-water-level-shore-mask-depth-map-and-coastline-waves) | no |
| WAV | abbreviation | Uncompressed audio container, supported by the stream decoder alongside Ogg Vorbis. | [27 §10](27_audio_engine.md#10-streaming-decoder-ogg-vorbis--wav) | no |
| WindowManager | engine (class) | Window and display-mode abstraction: `WindowManagerWin32` (OpenGL, live) and `WindowManagerDirectX` ([B:D3D]). | [31 §3](31_platform_build_and_tools.md#3-window-management) | no |
| WMD | abbreviation | "Weapon of mass destruction": the player-facing name of the Assault pulse bomb. | [03 §8.7](03_buildings_and_world_objects.md#87-pulsebomb), [01 §4.4](01_game_modes.md#44-assault-gametypeassault-bfull) | yes → Core Device |
| WorkQueue | engine (class) | Job queue served by its own worker thread; `App` uses one to load sounds in the background. | [20 §11.2](20_architecture.md#112-thread-inventory) | no |
| WorldObject | engine (class) | Root of every simulated object (`code/worldobject/worldobject.h`): id, type, position, velocity, `Advance` and render hooks. | [02 §3.2](02_units_and_entities.md#32-worldobject--base-of-all-simulated-objects), [21 §4.1](21_simulation_and_entity_model.md#41-worldobject) | no |
| WorldObjectId | engine (class) | Weak handle `{teamId, unitId, index, uniqueId}` addressing any simulated object; `unitId` may be a pseudo-unit code (buildings, effects, lasers, spirits) and the unique id guards against slot reuse. | [02 §3.1](02_units_and_entities.md#31-worldobjectid--object-handle), [21 §3](21_simulation_and_entity_model.md#3-object-identity-and-handles) | no |
| wu / u | abbreviation | World units, the length unit of the simulation (about one metre); speeds are wu per second. | [21 §8.1](21_simulation_and_entity_model.md#81-integration-model-and-time-base) | no |

### X

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| XBLA / Xbox 360 | abbreviation | Xbox Live Arcade; remnants of the console version (session pages, glyphs, `_xbox` phrase suffixes) are compiled out or unused on PC. | [31 §2.2](31_platform_build_and_tools.md#22-platform-map--every-platform-specific-file), [09 A.9 Other remnants](09_meta_systems.md#a9-other-remnants) | yes → platform-neutral glyphs |
| XInput | abbreviation | Microsoft gamepad API, delay-loaded from `xinput1_3.dll`; input driver 5. | [06 §11.6.6](06_player_control_and_input.md#1166-xinput--driver-5-present-only-if-xinput1_3dll-loads) | no |
| XOR | abbreviation | Exclusive or (bitwise). | [22 §14.2](22_determinism_and_networking.md#142-random-number-generators--every-generator-in-the-live-code) | no |

### Z

| Term | Kind | Definition | Primary document | IP? |
|---|---|---|---|---|
| Zombie | engine (class) | Visual ghost effect left by Darwinians killed by a Soul Destroyer or an evil tree. | [02 §21.6](02_units_and_entities.md#216-zombie--ghost-effect) | yes → Wisp |
<!-- GLOSSARY:END -->

## 3 Canonical placeholder names

### 3.1 Selection rules

1. **00 first.** Where [00 §8](00_overview.md#8-player-facing-glossary) names a placeholder, it is kept unless it is clearly worse; none was found clearly worse.
2. **Owning document second.** Otherwise the proposal of the document that specifies the object (02 for units and creatures, 03 for buildings, 04 for crates, 05 for the economy, 28 for UI, 22 for networking) wins.
3. **No collisions.** One placeholder names one thing. Two collisions had to be broken: "Swarm" (00 and 04 use it for the Infection outcome; 20, 21, 24 and 26 used it for Virii) stays with Infection, so Virii become **Crawler** (02's proposal); "Collector" (00 uses it for the Harvester; 01 used it for the Hoarder achievement) stays with the Harvester, so the achievement becomes **Stockpiler**. "Trooper" is reserved for the squad member, so it is rejected for the Darwinian (07) and for the officer (10).
4. **Generic words may stay.** Ordinary English words that are not distinctive (formation, port, tree, wall, spider, centipede, airstrike, nuke, meteor, gun turret, team colour names) may be kept; the IP column says `no (generic …)`.
5. **Identifiers.** Placeholders for code identifiers drop the IP word and keep the role (`MultiwiniaZone` → Control Zone / `ControlZone`).

In every table: "Rejected alternatives" lists other proposals with the document that made them in brackets.

### 3.2 Factions, people and units

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| Darwinian (unit, class, sprite) | name / art | **Worker** | Citizen (02, 21); Sprite, worker unit (03); Trooper (07); unit (01, 08, 30); Drone (20); Units (32); `unit_basic` sprite set (24, 25) | 00 choice; "Trooper" is the squad member |
| Multiwinian(s) | name | **Worker(s)** | the game / units (01); Citizen (02) | same unit as Darwinian |
| "Evil Darwinians" (monster-team Darwinians) | text | **hostile workers** | Hostile reinforcements! (04) | 05 wording |
| Officer | name | **Leader** | Commander (10, 24) | 00 choice |
| GOTO officer / signpost | concept | **Waypoint beacon** | — | 00 choice |
| Insertion squad (whole squad) | name | **Strike team** | Strike Team (02), squad (24) | 00 choice |
| Squadie (squad member) | name | **Trooper** | Strike team (00 IP row, which names the squad) | 02 and 10 proposal |
| Armour | name | **Transport** | — (06, 24 agree) | 00 choice |
| Harvester | name | **Collector** | — (05, 06, 24 agree) | 00 choice |
| Engineer | name | **Builder** | — (05, 06 agree) | 00 choice |
| Spirit / soul | concept / art | **Essence orb** | soul orb (24) | 00 choice |
| Monster team, "Evilinians" | name | **Rogue faction** | Corrupted (04, 07); `team_name_hostile` (21) | 00 choice |
| Futurewinian team, "Futurewinians" | name | **Visitor faction** | Future faction (01); Visitors (02, 04, 07); Vanguard faction (03); alt-faction (05); alien faction (08); Neutral Hostile (20); Elite faction, `team_name_elite` (21); Abductee (26) | 00 choice |
| Team colour names (green, red, yellow, blue, orange, purple, cyan, pink) | text | generic, may be kept | — | 01, 21, 24 agree |
| CPU player, "(CPU)" | text | **Bot** | — | 00 and 07 |

### 3.3 Creatures

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| Virii | name / art | **Crawler** | Swarm (20, 21, 24, 26) | collision rule: Swarm is the Infection outcome |
| Soul Destroyer | name | **Reaper Serpent** | Wraith (03); devourer (24); Reaper (26) | owning document 02 |
| Spore Generator | name | **Brood Drifter** | Spawner (26) | owning document 02 |
| Triffid (plant) | name (third-party literary) | **Spitter Plant** | Pod Launcher (02); Pod Plant (03); spore pod (24); Launcher-plant (26) | 00 and 04 chose Spitter Plants for the outcome |
| TriffidEgg | name | **Seed Pod** | — | 02 proposal |
| Army ant | name | generic ("Ant") | — | 02 |
| Centipede | name | generic, may be kept | segmented crawler (24); Segment-crawler (26) | collision with Crawler avoided by keeping the generic word |
| Spider | name | generic, may be kept | leaper (24); Strider (26) | |
| Egg (Virii egg) | name | generic, may be kept | — | |
| SpaceInvader (bomber) | name (third-party trademark echo) | **Bomber** | — | 02 |
| Space Ship (UFO) | name / presentation | **Abductor** | new presentation (02) | 00 and 04 |
| Zombie (ghost effect) | name | **Wisp** | Ghost (26) | owning document 02 |
| The Beast / Portal / Tentacle | name | **Rift Horror** (portal: Rift) | The Maw (02); Summoned Horror (04) | 00 and 03; [B:DEBUG] |
| Shaman | name | **Summoner** (cut; drop) | Mystic (06) | 03 and 24 |
| Tripod | name | **Tri-walker** (legacy; drop) | — | 26 |

### 3.4 Buildings and mode objects

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| Spawn point (`SpawnPoint`) | name | **Spawn Node** | Gateway (05) | 00 and 03 |
| Master spawn point (`SpawnPointMaster`) | name | **Source Node** | — (05 agrees) | 00 |
| Spawn link | name | **Relay** (plural Relays) | — (05 agrees) | 00 |
| Spawn population lock | name | **Population Limiter** | — | 05 |
| Trunk port | name | **Warp Gate** | — (01, 03, 05 agree) | 00 |
| Pulse bomb, "WMD" | name | **Core Device** | — (01, 03 agree) | 00 |
| Control station, "WMD Control Panel" | name | **Uplink Console** | — (03 agrees) | 00 |
| Escape rocket | name | **Launch Vessel** | — (03 agrees) | 00 |
| Fuel station | name | generic ("fuel depot") | — | 00 |
| Solar panel | name | **Energy Collector** | — (03 agrees) | 00 |
| Radar dish | name | **Relay Dish** | — (03 agrees) | 00 |
| Laser fence | name | **Barrier Emitter** | — (03 agrees) | 00 |
| Wall buster | name | **Breach Charge** | — (03 agrees) | 00 |
| Statue (CTS) | name | **Relic** | — (03 agrees) | 00 |
| CTS drop zone | name | **Relic Base** | — | 00 |
| KOTH hill (`MultiwiniaZone`) | name | **Control Zone** | Zone (08); `ControlZone` (21, same name as identifier); `CaptureZone` (10) | 00 |
| Blitzkrieg flag / home flag | name | **Flag post / Home post** | — | 00 |
| Ant hill (Ants Nest) | name | **Hive** | Swarm nest (06) | 00 |
| Incubator | name | **Hatchery** | — | 05 |
| Gun turret, wall, tree, jump pad | name | generic, may be kept | Turret (06) | 00 |
| Darwinia buildings (Pattern Buffer and Blueprint buildings, Control Tower, Clone Lab, GodDish, Spam, ResearchItem) | name | drop (legacy) | — | 03 |

### 3.5 Game modes

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| Domination (code `Skirmish`) | name | **Spawn Control** | Control Points (09) | 00 and 01 |
| King of the Hill (KOTH) | name | **Zone Control** | Hill (09) | 00 and 01 |
| Capture the Statue (CTS) | name | **Relic Run** | Relic Carry (09) | 00 and 01 |
| Assault | name | **Siege** | — (09 agrees) | 00 and 01 |
| Rocket Riot | name | **Launch Race** | — (09 agrees) | 00 and 01 |
| Blitzkrieg (code `Blitzkreig`) | name | **Flag Chain** | Flag Assault (09) | 00 and 01 |
| Tank Battle | name | **Armour Duel** | — | 00 and 01; [B:DEBUG] |
| "Sudden Death", "Ready", "Grace period" | text | generic, may be kept | — | 01 |

### 3.6 Crate outcomes, powers and comeback systems

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| Crate ("Crate Drop!") | name / text | **Supply crate** | supply drop (06, message text) | 00 |
| Task, power-up, "program" | name / fiction | **Power** | — | 00 |
| Task Manager, task slots | name | **Power slots** | — (06 agrees) | 00 |
| Retribution | name | **Comeback Bonus** | comeback bonus (06, lower case) | 00, 01, 05 |
| Handicap | name | **Catch-up bonus** | — | 00 |
| Squad (outcome) | name | generic ("Strike team") | — | 00 |
| Armour / Harvester / Engineer (outcomes) | name | Transport / Collector / Builder | — | 00 |
| Airstrike | name | generic, may be kept | Air strike (06) | 00 |
| Napalm Strike | name | **Firebomb Run** | Incendiary strike (06) | 00 and 04 |
| Napalm messages ("I love the smell of Napalm in the morning...", film quote) | text | "Firebombs inbound!" | — | 04 |
| Nuke (Nuke Barrage) | name | generic ("Missile Barrage") | — | 00 |
| Meteor (Meteor Shower) | name | generic, may be kept | Meteor storm (06) | 00 |
| Gun Turret / Rocket Turret / Flame Turret | name | generic, may be kept | Turret, Missile turret, Flame turret (06) | 00 |
| Personal Shield | name | **Shield** | — (04, 06 agree) | 00 |
| Subversion | name | **Convert** | — (04, 06 agree) | 00 |
| Hot Feet | name | **Speed Boost** | Haste (04); Speed boost (06) | 00, 01, 03 |
| Rage | name | generic ("Fury") | Frenzy (06) | 00; Frenzy is Mega Rage |
| Darwinians ("Multiwinians") | name | **Reinforcements** | — (04 agrees) | 00 |
| Spawn Mania | name | **Spawn Surge** | Rapid Spawn (04) | 00, 01, 03, 05 |
| Rocketwinians | name | **Rocket Troops** | — (04 agrees) | 00 |
| Crate Mania | name | **Supply Storm** | — (04 agrees) | 00 |
| Mega Rage (code `Bitzkreig`) | name | **Frenzy** | — (01, 04 agree) | 00 |
| Slow Down | name | **Time Dilation** | — (04 agrees) | 00 |
| Speed Up | name | **Time Rush** | — (04 agrees) | 00 |
| Magical Forest | name | **Spirit Grove** | Grove (04, 06) | 00 and 03 |
| Dark Forest | name | **Blight Grove** | — (03, 04, 06 agree) | 00 |
| Ants Nest | name | **Hive** | Swarm nest (06) | 00 |
| Eggs | name | **Brood** | — (03, 06 agree) | 00 |
| Infection (code `Spam`, `SpamInfection`) | name | **Swarm** | Contagion (06) | 00 and 04 |
| Randomiser | name | **Allegiance Scrambler** | Scrambler (04) | 00 and 03 |
| Plague | name | **Sickness** | Blight (06) | 00 and 04 |
| Space Ship | name | **Abductor** | — (04 agrees) | 00 |
| Triffids | name (third-party literary) | **Spitter Plants** | Spore Pods (03); Pod Launcher / Seed Pod (02) | 00 and 04 |
| The Beast | name | **Rift Horror** | The Maw (02); Summoned Horror (04) | 00 and 03; [B:DEBUG] |
| Task-manager fiction strings ("RUN PROGRAM", "SYNTAX ERROR", "PROGRAM TERMINATED", "ACQUIRED", "UPGRADED", "OBJECTIVES COMPLETED", "Powerup Limit Reached!", "Retribution : Two powerups max") | text | "Ready", "Not available", "Cancelled", "Unlocked", "Improved", "Objectives complete", "All power slots full", "Comeback powers: slots full" | — | 06 |

### 3.7 Engine, UI and code identifiers

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| `Multiwinia` (rules class) | identifier | **MatchRules** | Match (07) | 07 |
| `MultiwiniaOptions` (level block) | identifier | **MatchOptions** | — | 08 |
| `MultiwiniaZone` | identifier | **ControlZone** | Zone (08); CaptureZone (10) | consistent with §3.4 |
| `MultiwiniaOfficerOrders` | identifier | **OrderMarker** | — | 21 |
| `NET_DARWINIA_*` network identifiers | identifier | **`NET_GAME_*`** | — | 22 |
| Eclipse (UI framework), `EclipseLMouseDown` and similar controls | identifier / brand | **UI core**; `UiPointerDown`, `UiPointerHeld`, `UiAltPointerDown` | `ui-core` (28, same name) | 06 and 28 |
| `DarwiniaWindow`, `DarwiniaButton`, `MultiwiniaWindow`, `MultiwiniaFileDialog`, `RenderBigDarwinian` | identifier | `UiWindow`, `UiButton`, `MapEditorHubPage`, `MapFilePicker`, `RenderBackdrop` | — | 28 |
| `MultiwiniaControlMethod`, `input_preferences_darwinia*` | identifier | `ControlScheme`, `bindings_classic.txt` | — | 06 |
| Codenames `Pecan`, `Hummingbird` | identifier | `AlwaysTrue`, `FeatureFlagX` | — | 06 |
| Camera mounts `player0`–`player3`, `start`, `CamPosBefore` | identifier | `spawn_view_<n>`, `default_view`, `previous_view` | `__start_pose` (08) | 06, 08, 30 |
| `SepulvedaHelp` control, `Sepulveda` sound source | identifier | `AdvisorHelp`, `Narrator` | — | 06, 10, 27 |
| `MultiwiniaInterface` sound source | identifier | `MatchUI` | — | 10, 27 |
| Special profiles `AccessAllAreas`, `AttractMode`, `game_unlockall.txt` | identifier | drop | `__unlockall__`, `__attract__` (09) | 20 and 30 |
| "Futurewar Shape Format" (exporter description) | text | "Shape format v1" | — | 31 |
| "Gq/s" output unit | text | "units/s" | — | 03 |
| "DWs" in the FPS overlay | text | "Units" | — | 32 |
| `MT19937` reference code, SHA-1 from unrar | third-party code | `std::mt19937` or own generator (with attribution if reference code is copied); any public-domain hash (xxHash, BLAKE3) | — | 22 |
| `.mwm` extension | identifier | own extension | — | 29 |

### 3.8 Brand, characters, text, art, audio, maps and services

| Original term | Kind | Canonical placeholder | Rejected alternatives (doc) | Note |
|---|---|---|---|---|
| "Multiwinia" (title, `APP_NAME`, window class, profile folder) | brand | **`<GAME_TITLE>`** | `<GameName>` (20); the game (01); project codename (02); `<Game>` (03); `<GAME TITLE>` (09); `<NewTitle>` (22); `GAME_NAME`, `game.exe` (31) | 00 and 24 |
| Subtitle "Survival of the flattest" | text | own tagline | — | 00, 09, 28 |
| Title, logo, box and price art (`textures/mwtitle.bmp`, `icons/mwlogo.bmp`, `textures/ivlogo.bmp`, `textures/multiwinia.bmp`, `prices*.bmp`) | art | `game_title_wordmark`, `studio_logo_placeholder`; no store art | — | 25 and 40 |
| "Darwinia" (prequel, `DARWINIA_*` macros, `darwinia_gl.exe`) | brand | "legacy campaign" in prose; `<GAME_TITLE>` in identifiers | `<GameName>` (20) | 03 |
| "Introversion Software", company folder `Introversion\Multiwinia\`, registry key, Mac bundle id, version resource | brand | **`<STUDIO>`** (folder `<STUDIO>\<GAME_TITLE>\`) | `<Studio>` (20); own publisher name (00); own studio name (31) | upper case to match `<GAME_TITLE>` |
| Product and partner identifiers (`METASERVER_GAMETYPE_MULTIWINIA` = 4, Steam app id 1520, crash-report id `0B9432AF`, Game Explorer GUID) | identifier | own ids | — | 20, 22, 31 |
| Licensing material (fixed demo key, blacklisted keys, `DEMO` / `AMBR` key prefixes, `hsh` hash prefix, key format, `redshirt2` cipher) | key strings | platform entitlements; never copy | — | 09, 22, 23 |
| Store, metaserver, crash-report and FTP hosts | service | own endpoints from configuration (for example `directory.example.net`) | — | 22, 31, 32 |
| Dr Sepulveda (mentor) and portraits | character / art | **Advisor**; feature dropped | Narrator (10, 27); `advisor_face_*` (25) | 00 drops it |
| "Darwin Research Associates" logo | art | omit (product logo placeholder if needed) | `in_fiction_corp_logo` (25) | 24 and 30 |
| Achievement names Dominator, Hoarder, Hail To The King, Unclad Skies, Aggravated Assault, Master Player, Master Of Multiwinia, Explorer, Carnage, Blitz Master, Wrong Game, Genocide | name | Total Control, **Stockpiler**, Crowned, Clean Launch, Iron Defence, Giant Slayer, Completionist, Tourist, Massacre, Lightning War, Friendly Fire, Annihilator | Collector for Hoarder (01) | 01's list; collision rule for Hoarder |
| TrueSkill (Microsoft trademark) | brand | **Skill rating** | — | 09 |
| Map file names `mp_<mode>_<n>P_<k>`, `MAPID_*` | identifier | **`map_<mode>_<players>p_<nn>`** | `map_<mode>_<n>`, `region_<nn>` (23); `map_<mode>_<players>p_<n>` (40, same scheme) | 08 |
| Map display names and descriptions, layouts | text / level design | new names and new maps | — | 00, 08 |
| Map thumbnails | art | engine-generated thumbnails `map_<mode>_<players>p_<nn>_thumb` | — | 08 and 25 |
| Terrain colour ramps `landscape_*` and their theme words (icecaps, launchpad, containment, stronghold, jailbreak) | art | **`ramp_<theme>`** with neutral theme names | `terrain_ramp_<theme>` (25, 40) | 08, 23, 24 |
| Wave tables `waves_*`, water tiles `water_*` | art | `wavegrad_<theme>`, `seabed_<theme>` | `waves_<theme>`, `water_<theme>` (25) | 23 |
| Fonts `speccy`, `editor`, `square` (incl. CJK pages) | art | licensed open fonts (an OFL UI font plus Noto CJK) in a glyph cache | — | 20, 24, 25, 28, 31 agree |
| Darwinian sprite family, shapes, icons, help art, panel animations | art | new art with role names (`unit_*`, `fx_*`, `ui_*`, `icon_<ability>`, `help_<mode>_<page>`, `mode_<mode>`) | — | 25 |
| All 1,332 audio samples, sound event and sample-group names | audio | new audio named by function (`sfx_<event>`, `vo_<role>_<line>`, `amb_<bed>`); neutral event ids may stay | — | 10, 25, 27 |
| Licensed music (`tresk_*`, `trash80_*`, `dma_sc*`) and retro loader tunes | audio | commissioned `music_*`; never reuse | — | 00, 10, 27, 31 |
| Christmas mode (Santa hats, ice ramps) | art / theme | optional seasonal theme with new art | — | 02, 03, 24 |
| Gesture symbols and training data | design / data | drop | new symbol set (06) | 26 |
| POKEY ("Pokey Playground") | third-party chip name | drop | Synth sandbox (27, tool name) | 27 |
| Distributors and partners (Ambrosia `AMBR`, Buka `origin`, eGames/Cinemaware, Steam app id 1520, Xbox Live Arcade, Logitech) | brand | omit; platform entitlements instead of key prefixes | — | 09, 22, 31 |
| Beta-tester credit lists | personal data | never ship | — | 01, 31 |
| Developer names in comments | personal data | do not carry over | — | 04, 07 |
| All UI and narration text (`data/language/*.txt`) | text | own string table (phrase ids may be kept as neutral keys, texts not) | — | 28 |
| Literal English captions hard-coded in code ("PAUSED", "Master Volume", debug and editor captions) | text | new string-table entries | — | 27, 28, 32 |
| Credits (`multiwinia_credits_*`, `bootloader_credits_*`) | text | own credits (with consent) | — | 09 |
| Cut-scene scripts, narration and tutorial text | text | author new scripts and copy | — | 30 |
| Darwinia campaign location names (garden, containment, generator, mine, yard, escort, pattern_buffer, biosphere, receiver, temple, launchpad) | name | drop; generic sector names if ever needed | — | 08, 09, 30 |
| EULAs, legal texts and the rating notice `esrb_warning` | text | own licence; rating text from the new game's own rating | — | 09, 31 |
| Controller glyphs and Xbox naming (`icons/button_*.bmp`, `ControlXBoxMenuCycle*`, `_xbox` phrase suffixes, XBLA pages) | third-party brand | platform-neutral glyphs `input_glyph_<button>`; `PadMenuCycleUp`, `PadMenuCycleDown` | — | 06, 09, 25 |
| Shader sources `data/shaders/*` (14 files) | code / art | re-author in HLSL SM6 from [24](24_rendering.md) | — | 24, 25 |
| Menu palettes and art direction (red glass panel, steel-blue highlight, green and red captions on blue-glow boxes) | art direction | new palette keeping the token structure | — | 28 |
| Voice, bark and announcer sets (`darwin_*`, `DSelect*`, `Attack*`, `CrateReward_*`) and signature ambiences (`MultiLobbyAmb*`, `High*`) | audio | `vo_unit_select_01`, `vo_reward_<function>`, `vo_match_win`; `amb_lobby_pad_01`, `amb_altitude_01` | — | 10, 25 |
| Distinctive design expressions (submarine-launched multi-warhead nuke with ground symbol, UFO abduction event, giant-unit menu backdrop) | design expression | keep the mechanic, new presentation | — | 02, 09 |
| Introversion tools and binaries (`animator.exe` / `animator.dat`, `MultiwiniaSupport.exe`, `autopatch.exe`) | software | own tools (panel editor, crash reporter, updater) | — | 20, 26, 31 |

## Open questions and unknowns

None. The glossary records no fact of its own. The placeholder choices of §3 are design decisions taken under the rules of §3.1, not unknowns; the two collision resolutions (Virii → Crawler, Hoarder → Stockpiler) are the only places where the canonical name departs from every name the owning document proposed for that item.

## Docs-vs-code discrepancies

None found. The terminology of `docs/` (for example "Multiwinians" in `docs/multiwinia_manual.pdf`, "spirit" in `docs/spirit resource system.txt`) matches the code's meaning of each term; the behavioural discrepancies those documents contain are recorded by their owning documents ([00](00_overview.md#docs-vs-code-discrepancies) lists the headline ones).

## IP items

Summary by category; the per-term canonical placeholders and rejected alternatives are in §3, and every owning document keeps its full register.

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| Product and studio names ("Multiwinia", "Darwinia", "Introversion Software", subtitle) | brand / text | `code/lib/universal_include.h` (`APP_NAME`), `code/app.cpp:App::GetProfileDirectory`, `data/language/english.txt` | `<GAME_TITLE>`, `<STUDIO>`, own tagline (§3.8) |
| Unit and faction names (Darwinian, Multiwinian, Officer, Squadie, Armour, Harvester, Engineer, Evilinians, Futurewinians) | name | `code/worldobject/entity.cpp:Entity::GetTypeName`, `data/language/english.txt` | Worker, Leader, Strike team / Trooper, Transport, Collector, Builder, Rogue faction, Visitor faction (§3.2) |
| Spirit / soul concept and orb visual | concept / art | `code/worldobject/spirit.cpp:Spirit::Render` | Essence orb (§3.2) |
| Creature names (Virii, Soul Destroyer, Spore Generator, Triffid, SpaceInvader, Zombie, The Beast, Shaman, Tripod) | name | `code/worldobject/entity.cpp:Entity::GetTypeName`, `code/worldobject/building.cpp:Building::GetTypeName` | §3.3 |
| Building and mode-object names | name | `data/language/english.txt` (`buildingname_*`), `code/worldobject/building.cpp:Building::GetTypeName` | §3.4 |
| Game-mode names | name | `data/multiwinia.txt`, `data/language/english.txt` (`multiwinia_gametype_*`) | §3.5 |
| Crate outcome names and messages, task-manager fiction, retribution, handicap | name / text | `code/worldobject/crate.cpp:Crate::GetName`, `data/language/english.txt` (`cratename_*`, `crate_message_*`) | §3.6 |
| Code identifiers carrying IP words (`Multiwinia*`, `Darwinia*`, `NET_DARWINIA_*`, Eclipse, codenames) | identifier | `code/network/network_defines.h`, `contrib/eclipse/eclipse.cpp`, `code/interface/darwinia_window.h` | §3.7 |
| Achievement names (12) | name | `code/app.h` | §3.8 |
| Map names, layouts, thumbnails, terrain ramps | name / level design / art | `data/levels/level_enum.txt`, `data/levels/`, `data/thumbnails/`, `data/terrain/` | §3.8 |
| Fonts, sprites, shapes, icons, help art, panel animations, shaders | art | `data/textures/`, `data/sprites/`, `data/shapes/`, `data/icons/`, `data/mwhelp/`, `data/animations/`, `data/shaders/` | §3.8 |
| Audio samples, licensed music, event names | audio | `data/sounds.txt`, `tools/multiwinia_exclusions.txt` | §3.8 |
| Characters and in-fiction brands (Dr Sepulveda, Darwin Research Associates) | character / art | `data/sepulveda/sepulveda1.bmp`, `code/sepulveda.cpp`, `data/icons/darwin_research_associates.bmp` | Advisor (drop), omit (§3.8) |
| Third-party names and code (Triffid, Space Invaders echo, TrueSkill, POKEY, MT19937, SHA-1, distributors) | brand / code | `code/worldobject/triffid.cpp`, `code/worldobject/airstrike.h`, `contrib/systemiv/lib/math/random_number.cpp`, `contrib/systemiv/lib/hash.h` | §3.3, §3.7, §3.8 |
| Services and identifiers (metaserver and store hosts, crash-report id, Game Explorer GUID) | service | `code/app.cpp:App::InitMetaServer`, `code/app.cpp:App::GetBuyNowURL`, `code/main.cpp:AppMain` | own endpoints and ids (§3.8) |
| Personal data (beta-tester lists, developer names) | personal data | `data/betatesters.txt`, `data/multiwinia_betatesters.txt` | never ship (§3.8) |

## Source map

The few files that define the core vocabulary of this glossary.

| File | Role |
|---|---|
| `code/lib/universal_include.h` | product targets (`TARGET_*`), feature macros (`INCLUDEGAMEMODE_*`, `INCLUDE_CRATES_*`, `BLOCK_OLD_DARWINIA_OBJECTS`, `LOCATION_EDITOR`), `GRACE_TIME`, `APP_NAME`, the `iv_*` maths wrappers |
| `targets/vs2005/Darwinia.vcproj` | the four Visual Studio configurations (`Release\|Win32`, `ReleaseDirect3D` and the debug pair) |
| `code/globals.h` | `NUM_TEAMS` (6) and `NUM_SLICES_PER_FRAME` (10) |
| `code/gametimer.h` | `SERVER_ADVANCE_PERIOD`, `GetNetworkTime`, `g_predictionTime` |
| `code/app.h` | `App` game-mode enum and the achievement enum |
| `code/multiwinia.h` | `Multiwinia::GameType*` (seven game types) and `AIType*` (Easy, Standard, Hard) |
| `code/team.h` | team types, `LobbyTeam`, `TeamControls` |
| `code/worldobject/worldobject.h` | `WorldObjectId`, `WorldObject::Effect*` types, pseudo-unit codes `UNIT_*` |
| `code/worldobject/entity.h` | `Entity::Type*` enum (entity types 0–26 plus sentinel) |
| `code/worldobject/entity.cpp` | `Entity::GetTypeName`, `Entity::EntityIsBlocked`, `EntityBlueprint` |
| `code/worldobject/building.h` | `Building::Type*` enum (75 values) |
| `code/worldobject/building.cpp` | `Building::GetTypeName` (level-file names), `Building::BuildingIsBlocked`, `Building::CreateBuilding` |
| `code/worldobject/crate.h` | `Crate::CrateReward*` enum, `CRATE_SCAN_RANGE`, `INCLUDE_THEBEAST` gate |
| `code/taskmanager.h` | `Task` states and the task-manager interface |
| `code/global_world.h` | `GlobalResearch` types (task types) |
| `code/level_file.h` | level-file blocks and the `LevelOption*` enum |
| `code/network/network_defines.h` | `NET_DARWINIA_*` commands and field codes, ports, `GAMEOPTION_*` ids |
| `contrib/systemiv/lib/math/random_number.h` | `syncrand`, `syncfrand`, `syncsfrand`, `TRACK_SYNC_RAND` forms |
| `data/multiwinia.txt` | game-type blueprint names and lobby parameter names |
| `data/stats.txt` | entity stat rows (Health, Speed, Rate) |
| `data/language/english.txt` | every player-facing name (`buildingname_*`, `cratename_*`, `multiwinia_gametype_*`, `multiwinia_team_*`) |

## Appendix — dead/legacy code

The glossary itself contains no code. Build-restricted vocabulary is tagged in each definition; by tag:

- **[B:LEGACY]** — Darwinia campaign and blocked objects: `LaserTrooper`, `Lander`, `Tripod`, `TripodNavData`, `DropShip`, `Shaman`, `BoxKite`, `OfficerOrders`, `ControllerGrenade`, `AISpawnPoint`, `Generator`, `PylonStart`, `Powerstation`, `ConstructionYard`, `DisplayScreen`, the receiver, blueprint, dynamic, mine and track families, `Cave`, `FeedingTube`, `GodDish`, `Spam`, `Library / ResearchItem`, `SafeArea`, `ScriptTrigger`, `FenceSwitch`, `UpgradePort / PrimaryUpgradePort`, `UnprocessedSpirit`, gesture recognition, `GlobalWorld` beyond the team id, `game.txt` / `locations.txt`, the `redshirt2` save cipher, running programs, the pixel effect, camera-animation playback and the start sequence.
- **[B:DEAD]** — `USE_SEPULVEDA_HELP_TUTORIAL` (Sepulveda), `USE_LOADERS` (boot-loaders), attract mode, testbed (`TESTBED_ENABLED`, `testbedconfig.txt`), test harness, purity control, the `BenchMark` recorder, the network-quality simulator, the demo end sequence, SDL and non-Windows code, the commented-out crate outcomes.
- **[B:DEBUG]** — `TARGET_DEBUG` features: Tank Battle, The Beast with `Portal` and `Tentacle`, the cheat window, sound and gesture editors with the POKEY `EffectProcessor`, `AVIGenerator`, `TRACK_SYNC_RAND`.
- **[B:EDITOR]** — the location editor, `.mwm` custom maps.
- **[B:D3D]** — the Direct3D 9 OpenGL-emulation layer, HLSL shaders, the deform effect, `WindowManagerDirectX`.
