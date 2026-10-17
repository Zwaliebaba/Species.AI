# 00 — Overview

> Part A · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: no draft section was routed to 00; this document summarises the **final** functional documents [01](01_game_modes.md), [02](02_units_and_entities.md), [03](03_buildings_and_world_objects.md), [04](04_crates_powerups_and_weapons.md), [05](05_economy_and_population.md), [06](06_player_control_and_input.md), [07](07_ai.md), [08](08_levels_and_level_design.md), [09](09_meta_systems.md), [10](10_audio_design.md), with one-paragraph context from [20](20_architecture.md), [22](22_determinism_and_networking.md) and [24](24_rendering.md); headline findings from `_work/phase2_log.md`; map counts from `_work/drafts/X4_level_catalogue.md` / `X4_levels.csv` (cross-checked against [08 §3.3](08_levels_and_level_design.md#33-summary)); IP placeholders from the IP tables of 01–10 and `_work/closing/closing_ip.md`. Mechanics are **not** re-derived here: every number and rule is copied from its owning document, whose section is linked, together with the evidence tag and citation used there. A sample of the load-bearing constants was re-read in the source tree for this synthesis (listed in §10.1).

## Contents

1. [Pitch](#1-pitch)
2. [The game as actually shipped](#2-the-game-as-actually-shipped)
3. [Core loop](#3-core-loop)
4. [What makes it distinctive](#4-what-makes-it-distinctive)
5. [The player's verbs](#5-the-players-verbs)
6. [Match flow from the player's perspective](#6-match-flow-from-the-players-perspective)
7. [Design-critical quirks the new team must decide on](#7-design-critical-quirks-the-new-team-must-decide-on)
8. [Player-facing glossary](#8-player-facing-glossary)
9. [Reading guide to the functional set](#9-reading-guide-to-the-functional-set)
10. [Verification notes for this overview](#10-verification-notes-for-this-overview)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

Naming convention in this document: original in-game names (Darwinian, spawn point, crate, …) are used so that the reader can follow the citations; every one of them that is Introversion IP has a neutral placeholder in the glossary (§8) and in the IP table. The new game must not carry the original names, text, art, audio or map designs.

---

## 1 Pitch

*Multiwinia* is a 2–4-player real-time strategy game in which nobody builds units: each side's army is a crowd of small, self-willed soldiers that pours continuously out of the **spawn points** the side holds, so the whole game is a fight over territory that converts into a flow of bodies [C] `code/worldobject/spawnpoint.cpp:SpawnPoint::Advance` ([05 §1.1](05_economy_and_population.md#11-concept)). Players never order individual soldiers; they draw a circle around part of the crowd and send it somewhere, or promote one soldier into an **officer** who becomes a signpost or the head of a marching formation [C] `code/location_input.cpp:LocationInput::AdvanceTeamControl` ([06 At a glance](06_player_control_and_input.md#at-a-glance)). Every 20–60 seconds a **crate** parachutes onto the map; the side that crowds it first opens one of 31 outcomes — air strikes, nukes, armoured transports, turrets, crowd buffs, forests, monsters, time warps — of which one in ten is a hostile surprise for everybody [C] `code/location.cpp:Location::AdvanceCrates`, `code/worldobject/crate.cpp:Crate::AdvanceOnGround` ([04 §1](04_crates_powerups_and_weapons.md#1-crate-lifecycle)). Six modes put different objects at the centre of the same crowd-and-territory engine: spawn points (Domination), hill zones (King of the Hill), carried statues (Capture the Statue), a timed bomb and its control stations (Assault), a rocket that must be fuelled and boarded (Rocket Riot) and chains of flags (Blitzkrieg) [C] `code/multiwinia.cpp:Multiwinia::Advance` ([01 §1.4](01_game_modes.md#14-the-modes-at-a-glance)). Matches are short (default time limits 5–15 minutes), readable from a distance in a black-background, flat-shaded, neon-glow wireframe look, and playable against CPU opponents that see everything but cheat at nothing else [C] `data/multiwinia.txt` ([01 §5](01_game_modes.md#5-cross-mode-comparison)), [24 §7.1](24_rendering.md#71-how-the-look-is-produced-c-unless-marked), [07 §14](07_ai.md#14-cheats-and-advantages).

---

## 2 The game as actually shipped

### 2.1 Product and build

| Aspect | Shipped reality | Evidence | Owning doc |
|---|---|---|---|
| Product target | `TARGET_PC_FULLGAME` (`DARWINIA_GAMETYPE` "multiwinia"), the only active target; LAN and WAN play compiled in; basic and advanced crates and the tutorial compiled in | [C] `code/lib/universal_include.h` (target block) | `_work/build_matrix.md` §3 |
| Executable | `Release|Win32` OpenGL build `darwinia_gl.exe`, copied to `multiwinia.exe` by every Multiwinia packaging script; the Direct3D 9 configuration probably does not compile at this commit [I] | [C] `targets/vs2005/Darwinia.vcproj`, `tools/build_multiwinia_retail.bat` | [24 §1](24_rendering.md#1-backend-architecture), [31](31_platform_build_and_tools.md) |
| Every match is "multiplayer" | versus-CPU, LAN/WAN and both tutorials run through the same local or remote server and the same lockstep simulation; `App::Multiplayer()` is true in every Multiwinia game, `App::IsSinglePlayer()` only for the Darwinia campaign | [C] `code/app.cpp:App::Multiplayer`, `App::IsSinglePlayer`, `code/network/clienttoserver.cpp:ClientToServer::ProcessServerUpdates` | [06 §1.1](06_player_control_and_input.md#11-conventions-and-global-gates), [22 §1.1](22_determinism_and_networking.md#11-at-a-glance) |
| Demo/full gate | a runtime gate driven by the authentication key (`IS_DEMO`): demo keys restrict modes, maps and hosting through metaserver-supplied limits | [C] `code/app.cpp:App::IsGameModePermitted` | [09 §6](09_meta_systems.md#6-demofull-game-gating-and-the-authentication-key-ui) |
| Data | `data/` packed into `.dat` archives; loose files override archives; the retail data set excludes Darwinia levels, Tank Battle arenas, music and several dev maps | [C] `tools/multiwinia_exclusions.txt` | [31](31_platform_build_and_tools.md), [08 §2](08_levels_and_level_design.md#2-map-inventory-discovery-and-identity) |

### 2.2 The six modes

The anonymous enum in `code/multiwinia.h:Multiwinia` defines seven game types; six are compiled into [B:FULL] and Tank Battle only into `TARGET_DEBUG` [C] `code/multiwinia.h`, `code/lib/universal_include.h`, `code/app.cpp:App::IsGameModePermitted` ([01 §1.1](01_game_modes.md#11-the-seven-game-types)). The expected list "Domination, King of the Hill, Capture the Statue, Rocket Riot, Assault, Blitzkrieg" is therefore confirmed for the shipped build [C].

| # | Mode (original name → placeholder) | Objective | Ends by | Time limit option (default) | Players | Retail maps | Complexity label | Spec |
|---|---|---|---|---|---|---|---|---|
| 0 | Domination (`GameTypeSkirmish`) → *Spawn Control* | own spawn points; a team owning none for 30 mode seconds is eliminated | last team standing, or score at time-up | 0–60 min (10), 0 = unlimited | free-for-all 2–4; co-op 2v2 on two maps | 9 | Simple | [01 §4.1](01_game_modes.md#41-domination-gametypeskirmish-bfull) |
| 1 | King of the Hill → *Zone Control* | hold hill zones | score at time-up (never by elimination) | 1–60 min (5) | free-for-all 2–4; co-op 2v2 on three maps | 9 | Simple | [01 §4.2](01_game_modes.md#42-king-of-the-hill-gametypekingofthehill-bfull) |
| 2 | Capture the Statue → *Relic Run* | carry statues into your own zone (1 point each) | score at time-up | 1–60 min (10) | free-for-all 2–4; co-op on one map | 9 | Medium | [01 §4.3](01_game_modes.md#43-capture-the-statue-gametypecapturethestatue-bfull) |
| 3 | Assault → *Siege* | attackers capture every control station linked to the defenders' bomb before it detonates | bomb destroyed (attackers win) or detonation (defenders win) | none — the bomb's level timer (900 s / 1200 s) | 1v1, 1v2 and 2v2 (ForceCoop) | 8 | Medium | [01 §4.4](01_game_modes.md#44-assault-gametypeassault-bfull) |
| 4 | Rocket Riot → *Launch Race* | refuel your rocket via solar power, load 100 Darwinians, launch | first rocket above world height 600 | none | free-for-all 2–4 | 7 | Hard | [01 §4.5](01_game_modes.md#45-rocket-riot-gametyperocketriot-bfull) |
| 5 | Blitzkrieg → *Flag Chain* | capture a connected chain of flags into an enemy's home flag to eliminate it | last team standing, or flags + eliminations at time-up | 0–60 min (15), 0 = unlimited | free-for-all 2–4 | 7 | Hard | [01 §4.6](01_game_modes.md#46-blitzkrieg-gametypeblitzkreig-bfull) |

[C] `code/multiwinia.cpp:Multiwinia::AdvanceSkirmish`, `AdvanceKingOfTheHill`, `AdvanceCaptureStatue`, `AdvanceAssault`, `AdvanceRocketRiot`, `AdvanceBlitkrieg`; option ranges and defaults `data/multiwinia.txt`; labels `code/multiwinia.cpp:MultiwiniaGameBlueprint::GetDifficulty` ([01 §1.1, §1.4, §5](01_game_modes.md#5-cross-mode-comparison)). A "mode second" is 11 ticks = 1.1 game seconds (§7, Q1), so "30 mode seconds" and the on-screen clock run 10 % slower than game time [C] ([01 Conventions](01_game_modes.md#conventions-used-in-this-document)).

### 2.3 Maps and player counts

| Quantity | Value | Evidence | Owning doc |
|---|---|---|---|
| Multiwinia map files in the tree | 57 `data/levels/mp_*.txt` (53 mode maps + 4 with `GameTypes none`) | [C] directory listing; `code/game_menu.cpp:GameMenu::AddMap` | [08 §3](08_levels_and_level_design.md#3-master-catalogue-of-the-57-multiwinia-maps) |
| Maps listed in the retail menu | **49**: Domination 9, King of the Hill 9, Capture the Statue 9, Assault 8, Rocket Riot 7, Blitzkrieg 7 — equal to `data/levels/level_enum.txt` ids 1–49 | [C] `code/game_menu.cpp:GameMenu::CreateMapList`, `tools/multiwinia_exclusions.txt`, `data/levels/level_enum.txt` | [08 §3.3](08_levels_and_level_design.md#33-summary), [01 §1.3](01_game_modes.md#13-maps-per-mode-retail-menu) |
| Player counts of the 49 | 2-player 22, 3-player 15, 4-player 12 (`mp_assault_3p_1` declares `NumPlayers 2`) | [C] `NumPlayers` lines of the level files (re-counted from `X4_levels.csv` for this overview) | [08 §3.3](08_levels_and_level_design.md#33-summary) |
| Map size | square worlds 2000–7000 world units on a side | [C] `Landscape` blocks (same re-count) | [08 §3.3](08_levels_and_level_design.md#33-summary), [23](23_terrain_and_landscape.md) |
| Population caps | level `PopulationCap` 1400–2000 on 47 retail maps; two co-op maps (`mp_domination_4P_2`, `mp_koth_4P_3`) have none | [C] `code/level_file.cpp:LevelFile::ParseMultiwiniaOptions`; level files | [05 §4.6](05_economy_and_population.md#46-per-map-table-every-mp_-level-file-in-datalevels) |
| Maps with spawn points | 32 of the 49 | [C] building census | [05 §2](05_economy_and_population.md#2-territory-capture-spawn-points), [03 §1.5](03_buildings_and_world_objects.md#15-shipped-map-census) |
| Co-op-capable maps | Domination 2, King of the Hill 3, Capture the Statue 1, Assault 4 (ForceCoop), Rocket Riot 0, Blitzkrieg 0 | [C] map tables | [01 §5](01_game_modes.md#5-cross-mode-comparison) |
| Not shipped | `mp_assault_stress`, `mp_benchmark_1`, `mp_sandbox`, `mp_sandbox_all`, `mp_tankbattle_2P/3P/4P`, `mp_cts_3p_3`; 12 Darwinia `map_*` + 17 `mission_*`; 30 superseded maps in `data/levels/old/` | [C] `tools/multiwinia_exclusions.txt` | [08 §3.4](08_levels_and_level_design.md#34-hidden-test-stress-benchmark-sandbox-tankbattle-and-cut-maps) |

### 2.4 Who is in every match

- Six team slots (`NUM_TEAMS = 6`): slots 0–3 are players (only 0 … `NumPlayers−1` are used on a given map), slot 4 is the **monster team** ("Evilinians", owner of hostile crate outcomes and creatures) and slot 5 the **Futurewinian team** (dropped by the Space Ship outcome). Both special teams are created as CPU teams in **every** Multiwinia game and each owns one AI entity [C] `code/globals.h`, `code/location.cpp:Location::InitMonsterTeam`, `Location::InitFuturewinianTeam` ([01 Conventions](01_game_modes.md#conventions-used-in-this-document)).
- At game start the host fills every empty player slot with a CPU team; one AI difficulty (Easy / Normal / Hard) applies to all CPU teams [C] `code/UI/GameMenuWindow.cpp` (lobby update, `PlayGameButton`), `code/multiwinia.h` ([09 §5.1](09_meta_systems.md#51-start-sequences-driven-from-the-front-end), [07 §13](07_ai.md#13-difficulty-levels--every-parameter)).
- A human who leaves mid-match is replaced by a CPU player immediately; no mode treats a leaver as eliminated [C] `code/multiwinia.cpp:Multiwinia::RemoveTeams` ([01 §2.13](01_game_modes.md#213-players-leaving)).
- Eight selectable team colours; in Assault the colour *is* the side (green = defenders, red = attackers) [C] `code/multiwinia.h` (`MULTIWINIA_NUM_TEAMCOLOURS`, `ASSAULT_DEFENDER_COLOURID`) ([01 §2.5](01_game_modes.md#25-teams-slots-start-positions-colours-co-op-and-assault-sides)).

### 2.5 What is NOT in the shipped game

Everything below exists in the source tree, often fully coded, but a player of the [B:FULL] build never meets it.

| Feature | Build tag / state | What exists | Evidence | Owning doc |
|---|---|---|---|---|
| Darwinia single-player campaign (global world, campaign maps, scripts, Dr Sepulveda) | [B:LEGACY] — compiled in, unreachable; campaign levels excluded from every Multiwinia data build; the Darwinia main page is debug-only | the whole campaign code path guarded by `App::IsSinglePlayer()`; 12 `map_*` + 17 `mission_*` levels; 42 scripts that cannot run in Multiwinia | [C] `code/app.cpp:App::IsSinglePlayer`, `tools/multiwinia_exclusions.txt` | [09 §2.6](09_meta_systems.md#26-compiled-but-unreachable-and-non-baseline-flow), [08 §7](08_levels_and_level_design.md#7-darwinia-map_mission_-files-datalevelsold-and-level_enumtxt), [30](30_scripting_tutorial_and_cutscenes.md) |
| Tank Battle mode | [B:DEBUG] — `INCLUDEGAMEMODE_TANKBATTLE` commented out in FULLGAME; arenas not in retail data; Tank unit also unreachable as a crate reward | blueprint, three arenas, dispatch case, `Tank` entity and AI | [C] `code/lib/universal_include.h`, `code/multiwinia.cpp:Multiwinia::AdvanceTankBattle` | [01 §4.7](01_game_modes.md#47-tank-battle-gametypetankbattle-bdebug), [02 §1.1](02_units_and_entities.md#11-every-entitytype-value) |
| Map editor and custom `.mwm` maps | [B:EDITOR] — `LOCATION_EDITOR` commented out in every target; even the menu entry is compiled out | editor core and its editor windows (`code/location_editor.*` plus the editor window files) | [C] `code/lib/universal_include.h` | [08 §8](08_levels_and_level_design.md#8-the-map-editor-beditor) |
| Achievements (12) | stubbed — conditions are evaluated in the rules code, but `App::EarnedAchievement` always returns false and the tracker never persists; the Achievements page is unreachable; `level_enum.txt` (needed for two of them) is excluded from retail data | catalogue, unlock conditions, page widgets | [C] `code/app.cpp:App::GiveAchievement`, `App::EarnedAchievement`, `code/achievement_tracker.cpp` | [09 §7.2](09_meta_systems.md#72-platform-reality-in-the-pc-baseline--achievements-are-stubbed), [01 §2.11](01_game_modes.md#211-ending-declarewinnert-and-the-end-of-game-sequence) |
| Music | never heard — no reachable trigger, all 11 tracks excluded from the shipped archives, and a triggered track would hit a fatal release assert | a complete music engine and `OTHER Music` event blocks in `data/sounds.txt` | [C] `code/sound/sample_cache.cpp:CachedSample::CachedSample`, `tools/multiwinia_exclusions.txt` | [10 §6.4](10_audio_design.md#64-why-music-is-never-heard-in-multiwinia--and-the-assert) |
| Leaderboards | [B:LEGACY] stub — no data source or backend | UI windows only (`LeaderboardLoader` is a stub; the metaserver message set carries no ranking fields) | [C] `code/interface/leaderboards_window.cpp:LeaderboardLoader::DoLeaderboardSearch`, `code/UI/GameMenuWindow.cpp:GameMenuWindow::SetupLeaderboardPage` | [09 §7.5](09_meta_systems.md#75-leaderboards-blegacy--no-data-source) |
| Quick match | unreachable — its button is commented out | page code (entry button commented out: "It doesn't work") | [C] `code/UI/GameMenuWindow.cpp` | [09 §5.9](09_meta_systems.md#59-quick-match-pagequickmatchgame-5--unreachable) |
| Key rebinding | unreachable — the key-binding window's menu button is commented out; only hand-edited files change controls | `PrefsKeybindingsWindow` | [C] `code/interface/mainmenus.cpp:OptionsMenuWindow::Create` | [06 At a glance](06_player_control_and_input.md#at-a-glance), [09 §11.9](09_meta_systems.md#119-control-options-prefskeybindingswindow-blegacy--unreachable) |
| Darwinia control scheme, gesture recogniser, icon task bar, gamepad control-help panel | [B:LEGACY] — `PrefsManager::Load` forces `MultiwiniaControlMethod = 1` after every load | full gesture recogniser (8 symbols) and two alternative task-manager interfaces | [C] `code/lib/preferences.cpp:PrefsManager::Load`, `code/taskmanager_interface.cpp:TaskManagerInterface::CreateTaskManager` | [06 §4.1](06_player_control_and_input.md#41-variant-selection--which-interface-is-live), [06 §7.1](06_player_control_and_input.md#71-liveness) |
| Multi-round Assault (sides rotate, best defence time wins) | unreachable — `m_assaultSingleRound` is never set false | full round-rotation and time-record code | [C] `code/multiwinia.cpp:Multiwinia::AdvanceAssault`, `Multiwinia::ResetLocation` | [01 §6](01_game_modes.md#6-known-bugs-that-shape-the-shipped-rules) (B5) |
| The Beast crate outcome; Shaman, DropShip, Tripod, LaserTrooper units; Tank and FormationIncrease crates | [B:DEBUG] (Beast: `INCLUDE_THEBEAST` only under `TARGET_DEBUG`) / [B:LEGACY] / commented out | classes and behaviour | [C] `code/worldobject/crate.h`, `code/worldobject/entity.cpp:Entity::NewEntity` | [04 §5.30](04_crates_powerups_and_weapons.md#530-the-beast-bdebug), [02 §1.1](02_units_and_entities.md#11-every-entitytype-value) |
| Spectator UI | [B:DEAD] — compiled only under `TESTBED_ENABLED` | spectator team type and buttons | [C] `code/UI/GameMenuWindow.cpp` | [01 §2.5.2](01_game_modes.md#252-slots-and-start-positions) |
| In-match objective tabs | cut — computed every frame, never drawn | `MultiwiniaHelp` objective tabs | [C] `code/multiwiniahelp.cpp` | [09 §8.3](09_meta_systems.md#83-objective-tabs--computed-every-frame-but-never-drawn-cut-feature) |
| Intro start sequence, boot-loader mini-programs, attract mode, benchmark, demo-end screen, user profiles, mods | [B:LEGACY] / [B:DEAD] (`USE_LOADERS` off) | code present | [C] `code/lib/universal_include.h` | [09 §9.1](09_meta_systems.md#91-build-reality-of-every-sequence-feature), [09 Appendix](09_meta_systems.md#appendix--deadlegacy-code) |
| Audio DSP effects | data-authored (22 uses), not applied in the shipped configuration | DSP chain | [C] `data/sounds.txt` | [10 §1.3](10_audio_design.md#13-headline-numbers) |
| A post-match results screen in the front end | does not exist — results are only the in-match "GAME OVER" overlay; leaving rebuilds the main page | — | [C] `code/game_menu.cpp:GameMenu::CreateMenu` | [01 §2.2](01_game_modes.md#22-match-lifecycle) |

---
