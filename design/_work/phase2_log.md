# Phase 2 completion log (coordinator notes)

## P09_control_taskmanager — done
- 29 task types, 4 states, 6 slots MP (2 when eliminated); MW interface is the only live variant (Gestures/Icons unreachable); Rubine-style gesture recogniser (13 features, 8 symbols, Python port validated on 8 demos); 9 cursor types. [U]=5.
- Critical: MW task bar not clickable (keys/bumpers/world-click only); task id sent as 1 byte (256th+ task misaddressed); UI/render writes sim state (crosshair `=` vs `==` overwrites building type; local-only m_mostRecentTaskId reset; dormant local terminate paths).

## P16_sound — done
- 27.1–27.15 engine, 10.1–10.11 design, 09.1 sound options; ~33 engine + ~40 editor classes; 68 sources × 296 event blocks; 130 sample groups; 14 DSP types; 8 options. [U]=5.
- Critical: music likely never audible in MW [I] (triggers compiled out under USE_LOADERS or in Darwinia scripts — confirm with P19/P20); object-attached sounds hard-cut on object removal; new voice replaces quietest without priority compare; DSound vs software back-end volume curves differ (5 dB vs 4.34 dB per unit).

## P07_buildings_misc — done
- 38 classes (33 concrete, 4 abstract/helper, 1 effect); 8 live in MW (level census script P07_building_census.py); CloneLab compiled/loadable but in no shipped map; 29 blocked. [U]=4.
- Critical: Tree collision spheres generated inside display-list (render) build using unsynced RNG re-seeded per pass → store as data in new game; SmokeMarker overrides use float vs double base → never override → likely invisible 13-unit obstacle [I]; wall destruction threshold damage ≤ −300 (only WallBuster known).
- Solar power via pylons is the only live MW laser-fence switch; neutral (255) fences never powered; fence-toggle net message has no sender; ChessBase reads nonexistent game options.

## P01_app_core — done
- App (every member, ctor/startup/shutdown order, ~35 globals, layer + dependency diagrams); main loops, GameTimer, tick/slice model, threads; Location containers/level-load/per-slice order/weather/crates/render order/public API; Team/LobbyTeam/Unit/TeamControls; 35 input actions; 15 network commands; ~83 constants. [U]=8.
- Critical: local preview routes share routing-system free slots with sim orders (route-id desync risk); FenceSwitch::Switch() runs from local input without a network message (desync if any MW map has a fence switch — X4); sync RNG seeded only at client join, never per match; GamePaused() depends on client's own out-of-sync list.
- Docs-vs-code: server ticks at 10 Hz in code vs docs/Network.txt "25 FPS".

## P04_entities_military — done
- 15 weapon classes; Airstrike/Napalm; Nuke; InsertionSquad; Armour; Tank; Harvester; LaserTrooper; Shaman(+interface); Engineer. 30 classes/structs, 12 diagrams, ~95 constants, [U]=6. Sims: scripts/P04_airstrike_sim.py, P04_nuke_sim.py.
- Reachability: Shaman cut (Advance returns true on first tick → self-deletes); Tank unreachable in FULL (crate reward and task spawn commented out; mines/missiles never called); LaserTrooper + ControllerGrenade legacy-only; InsertionSquad only ever grenades in MW.
- Uncertain: squad weapon choice [I]; team colouring of squad/harvester/engineer/bomber shapes at render time; airstrike/nuke timing on real terrain (nuke launch delay intended to sync 10 warheads but nearer still land first).

## P08_crates_ai — done
- 32 reward enum values (31 shipped + 1 debug-only The Beast) + 3 commented; 90/10 good/bad pools; per-level probability table drafts/P08_crate_pools.csv; InvalidCrates syntax; StartingPowerups; Retribution never called. AI: AITarget node graph, 4 priority formulas, 19 task handlers, per-mode strategies, difficulty table, no cheats beyond perfect information; 8 marker types; ~80 constants; [U]=6.
- Correction applied: INCLUDE_THEBEAST is under #ifdef TARGET_DEBUG → off in FULL (build_matrix fixed).
- Bugs: unbounded loops; AI target-id bugs (RunNuke needs sea; RunSpam/RunPlague wrong ids). Unconfirmed: monster/Futurewinian team AI entities (Location::Init ordering).

## P17b_menus_options — done
- 32 window/controller classes; 14 visible option controls + 6 hidden round-tripped prefs; 341 pref accesses (drafts/P17b_prefs_accesses.csv); 166 string ids; 11 keys in default_preferences.txt, 76 in code. [U]=3.
- Live: only Screen, Graphics, Sound, Other options. Unreachable/dead: profiles, leaderboards (stub, no backend), mods, key rebinding, demo-end. Pixel effect never runs in MW; vsync/AA ignored in GL build. Options absent: multisampling, gamma, invert mouse, auto-zoom, v-sync.

## P12_render_core — done
- 24.1–24.9: renderer init/GL baseline, lighting, matrices/fades, ordered 35-row pass table + Mermaid pipeline, pixel effect, ~20 graphics prefs, shapes/team colouring/hit-test, billboards/debug, text + 138 Unicode/CJK font atlases, prediction convention, deform, 14 shaders. 25: BMP decode, upload, resource manager. 26: 19 particle types, shattering, EffectProcessor (= sound-chip envelope tool, debug-only). P12_shapes.csv (140 .shp). [U]=7, 14 contradictions.
- Critical: D3D build probably doesn't compile (deform.cpp calls Xbox 360 Resolve(); never restores RT); App::Multiplayer() true in every MW game → pixel effect, deform, water-reflection pre-render never run (D3D water samples never-rendered reflection texture); flat-shade provoking vertex last (GL) vs first (D3D); texture cache ignores flags → first requester fixes mip/mask.

## P19_help_tutorial_script — done
- Script language: grammar, 43 opcodes, one-line-per-frame execution, 6 start paths; 42 scripts (39 [B:LEGACY], 3 [B:DEAD]); scripts cannot run in MW (all triggers blocked/compiled out; scripts/* excluded from PC data build). 31 tutorial tabs, 49 help enum values; Demo1 38 chapters, Demo2 50. [U]=7.
- Key: live MW tutorials are a help-tab director inside MultiwiniaHelp running on mp_koth_2P_1 and mp_koth_3P_1; tutorial*.cpp, Sepulveda, HelpSystem are [B:DEAD]; data/mwhelp has no .txt files.
- Uncertain: how radar dish 4 becomes player-owned at Tutorial 2 start; whether CPU team 2 reliably takes spawn 83 first; whether mapcrc.h CRCs match current KOTH maps (else tutorial buttons disabled).

## P05_entities_creatures — done
- 28 classes (21 live in FULL, 1 debug-only, 6 legacy); 14 stats rows; 12 diagrams; ~60 #defines; 15 sound bindings. [U]=5.
- Eggs-crate creatures owned by task-placing player (not monster team) except SoulDestroyer → monster team in MP; meteor-shower damage not team-filtered; many timings per-frame not per-second (egg/spider gravity, empty-anthill counter, 1% lightning roll) and don't scale with slow/speed crates; SpaceShip speed 500 overflows byte stat (≈244 on MSVC) [I], movement applies tick length twice, wander points cluster in one quadrant.
- DropShip, Tripod unreachable; Beast/Tentacle debug-only; level-load block list doesn't stop crates spawning blocked types.

## P13_gfx_backend_math — done
- ~96 GL/GLU entry points + 14 ARB/WGL pointers tabulated; 28 D3D-vs-GL defects D1–D28; 3 FixedFunc shader pairs + uniform layout; 4 window managers; 9 math modules + 11 iv_* wrappers; 8 determinism risks; ~45 constants. [U]=10.
- ReleaseDirect3D probably does NOT compile at this commit (WindowManagerDirectX::CreateWin char* vs pure-virtual wchar_t* → abstract; D3DRS_LINEWIDTH undefined/Xbox-only) — corroborates P12 (deform.cpp Resolve()).
- GL/D3D builds unlikely to stay in lockstep: D3D config /fp:fast + no D3DCREATE_FPU_PRESERVE (24-bit x87) vs baseline /fp:precise 53-bit. D3D gluPerspective doubles _43 (near/far) — intent unknown.

## P15_network — done
- Relay lockstep: server sequences a 10 Hz command stream; host is also a client; no dedicated server. 40 C→S command types (+4 defined never sent); 11 S→C letter types (+2 UPDATE variants); 10 disconnect reasons; ~60 data-field codes; 14 game options; 13 metaserver/matchmaker messages; 9 threads; 4 sequence diagrams. [U]=5.
- Per-frame sync byte in FULL comes only from the synced RNG stream (not world state); detected desync freezes the game permanently.
- Uncertain: metaserver/matchmaker server side absent from tree; messages >1500 bytes dropped on receive and resent forever (stall) [I]; late join/rejoin replays full history from frame 0 [I]; server never checks sender owns the team its commands target. Key-check algorithm stubbed out in this source release.

## P18_ui_gamemenu — done
- 38 page slots: 18 live, 10 compiled-but-unreachable (Achievements, QuickMatch, Help…), 10 gap slots; live + legacy Mermaid flowcharts; per-page button tables; 74 widget classes (47 live); 6 game types × 34 parameter rows + TankBattle; 10 error triggers; 15 contradictions. [U]=8.
- Uncertain: server-side rejection of non-host option changes (UI only hides controls — P15 says server never checks team ownership); map PlayTime overwritten by default TimeLimit on host lobby entry; authkey vs authkey.dev at startup; "show passworded games" filter checks key "dy" which collides with GAMETIME → filters nothing.

## P02_game_modes — done
- 7 game types (TankBattle DEBUG-only); 39 option definitions + 5 other lobby options; full Multiwinia class (lifecycle SM, per-tick order, grace/ready, colours, ending, HUD); 11 world-object classes; rocket (7 states) + Blitzkrieg flag SMs; trunk-port reinforcement waves; 12 achievements; strings table drafts/P02_game_modes_strings.csv. [U]=8.
- Critical: every "1-second" rule timer fires every 11 ticks (1.1 s) due to float rounding; s_numMultiwiniaZones never reset between games and feeds sim (cross-session desync); on 3 Blitzkrieg maps trunk-port reinforcements run twice per tick; achievements stubbed out in PC source; multi-round Assault unreachable (no UI sets option).
- Uncertain: Darwinian port occupancy deciding control-station capture (P06); damage per shot vs escape rocket; whether shipped Steam/XBL build had a real achievement layer.

## P17a_ui_framework_editor — done
- 28 §1–10 (focus/mouse/keyboard rules, every widget, palette, controller bindings, localisation, new-screen walkthrough, class diagram); 08 E1–E5 (editor core: 9 modes, 5 tools; 16 editor windows, ~60 building-edit control rows); 32 D1–D7 (23 cheats, 17 debug entry points, network window fields); ~55 constants; UI string table (221 rows). [U]=5.
- Editor unreachable in FULL (Map Editor button exists only in editor builds; other route via Darwinia loop exits immediately); custom .mwm maps editor-build-only; F9 network stats window ships in FULL.
- Uncertain: "Create New Map" .txt readable as .mwm?; LevelFile::m_gameTypes single type vs bitmask; reinforcement-mode drop-down reuses values 0/1.

## P03_entity_core — done
- WorldObjectId/WorldObject; Entity (27-value type enum + blocked status, NewEntity, stats loading, team colour, collision); entity grid/cache/obstruction/routing; class diagrams for all 159 worldobject classes (drafts/P03_worldobject_classes.csv); Darwinian (25 states), Officer (3 states, 4 order types), Spirit (10 states), EntityLeg; 23 effect types; 5 damage types; 17 Darwinian sound events; ~80 constants; 13 contradictions/bugs. [U]=7.
- Critical: Darwinians watching a launching rocket take facing from wall clock (g_gameTime from GetHighResTime) → probable desync; Harvesters appear to collect any team's souls (no team check) → Darwinians at own spawns; grid shortcuts ignore range (cache ignores requested range; presence checks use cell capacity) → exact queries play differently.

## Phase 3 started (early): S07 (07_ai), S27 (27_audio_engine), S28 (28_ui_framework), S02 (02_units_and_entities), S04 (04_crates_powerups_and_weapons). Class→doc assignment: _work/class_assignment.csv.

## P06_buildings_core — done
- Building base (members, virtuals, ports/reservation counters, capture summary, damage, level-line format), 75-value type enum (+15 sub-enum) with class/blocked/forced-global/map counts, factory, 39-entry blocked list; spawn network; GunTurret 5 types (+crate turret); Teleport/RadarDish; JumpPad; power network; CarryableBuilding; ControlTower; Incubator; Portal ("The Beast"); legacy. Spawn economy: exact capture algorithm, 30 s activation delay, every cap rule, burst/pause spawn formula with handicap, table of 32 official levels with spawn points. 38 classes/structs, 9 diagrams. [U]=11.
- Critical: per-team pop cap = PopulationCap / NumPlayers (level file), not actual playing teams; co-op maps mp_domination_4P_2 and mp_koth_4P_3 have no spawn cap; which Team::m_others count toward cap; GetNetworkTime vs GetGameTime same synced clock? each building once per 0.1 s tick?

## P10_input_camera — done
- 10 input drivers (9 registered; pipe driver never registered), stub filters, 10 conditions, 27 XInput controls, formal binding grammar, file layering/locales; all 164 control types with default KB/M + XInput bindings (scripts/P10_bindings.py re-implements parser; drafts/P10_input_camera_bindings.csv, P10_input_camera_preflines.csv); camera 15 modes + 3 debug, mount/animation playback SM, mount census; projection/frustum; control help (29 conditions, 11 icons, 4 panels; never displays in MW); ~95 constants; 12 docs-vs-code. [U]=6.
- Critical: InsertionSquad::DirectControl reads local m_camLookAround while applying networked controls (confirmed desync path); unknown condition words silently = "down" (4 live bindings depend on it); analogue trigger value overflows a byte; XInput dead-zone constants from SDK headers; some rebinds save unreadable strings; squad camera zoom-out never moves at normal fps (integer distance).
## Phase 3 launched: S03, S05, S06

## P11_terrain_world — done
- 23.1–23.16: pipeline; every LandscapeData/LandscapeTiles/flatten field; exact tile generation (private per-tile LCG — not synced/app RNG); guide-grid encoding; tile merge; flatten; sizing; sim normals; colour-ramp formula; water shore mask/waves; lightmap docs-vs-code. 24.1–24.7: landscape strip mesh 2 passes (no LOD), flat/dynamic water, D3D water reflection+shaders, clouds/sky (also menu backdrop), legacy global internet/sphere world, lights/fog, GL-state pass table. 08.1–08.8: file kinds, load sequence, block dispatch, all MultiwiniaOptions keys + 15 level options, other blocks, saving, .mwm format, map CRC, GlobalWorld in MW. 30 classes; 14 block types; 17 option keys; 5 effects; 5 flatten types; 3 generation methods; 29 research types; 8 render passes. [U]=8.
- Validation: Python terrain re-implementation (scripts/P11_*.py) puts no Trees (793) / SpawnPoints (321) below sea level; CRC script reproduces all 87 MAPID_* values in mapcrc.h.
- Critical: bit-exact terrain depends on MSVC 2005 pow/exp/log rounding; negative double → unsigned short conversion in bilinear sampler; peers start by map file name only — no map CRC exchanged → modified map desyncs silently; code/water.cpp:119 hard compile error under USE_DIRECT3D (third D3D breakage).
## Phase 3 launched: S23, S24

## P14_platform_lib — done
- 31: solution + 4 VS configs with full flag tables; compile-time config; Mac/Linux targets; platform map; timing; user-data paths; 9 .dat archives, load order, override rules; data build pipeline + inclusion/exclusion list grammar; packaging scripts; 12 installer scripts; MapCRC; ShapeExporter; localisation build; FindUnusedResources logic; crash handling; system info; one row per root/targets/tools/docs file. 20: threading primitives, 8-thread map, main-thread GL job hand-off. 29: TextReader rules, "redshirt2" text encryption, preferences, language files + caption markup, Directory wire format, binary readers, FileCRC, hashes, key formats, keygen. 22: both RNG algorithms, seeding, float-truncation helpers, FP build differences. 32: profiler, debug output/asserts/black box, leak tracking. 21: 12 container types slot-reuse/iteration-order. drafts/P14_platform_lib_pref_keys.csv (~75 keys). [U]=8.
- Critical: GL vs D3D determinism (/fp:fast + no FPU_PRESERVE) and lobby checks only version string "dev"; default_preferences.txt read before main.dat loaded → packaged install likely only hard-coded defaults; authkey.dev read vs authkey written.
- Correction applied: targets/macosx/Multiwinia is a maintained Multiwinia Mac project.
## Phase 3 launched: S20, S31

## P20_sequences_loaders_bench — done
- 09: compiled/reachable table; loading screen; animated game-mode panels; loader framework/registry; 10 loader mini-programs; credits structure (keys only); attract mode; ~95 constants. 30: start sequence (captions, timings, fades, grid, camera) + demo end sequence. 32: benchmark recorder + FTP upload (credentials omitted), test harness, build flags. ~31 classes, 9 music events, 6 .anim files. [U]=4.
- Only loading screen + animated panels player-visible in FULL; loading on background worker thread, main thread runs GL jobs + network tick, no progress reporting; USE_LOADERS off → 7 loaders + RunBootloader dead; credits loader [B:LEGACY]; StartSequence compiled never constructed; attract, benchmark, test harness [B:DEAD].
## Phase 3 launched: S09, S30 (S32 pending a slot)

## X4_level_catalogue — done
- 57 mp_* (49 in retail menu: Domination 9, KOTH 9, CTS 9, Assault 8, Rocket Riot 7, Blitzkrieg 7; 8 excluded from retail data); 12 map_*, 17 mission_*, 30 old/. All map ids match mapcrc.h (86/86); building counts match P06 exactly. Tables: drafts/X4_levels.csv (116×125), X4_level_buildings.csv, X4_thumbnails.csv, X4_mode_maplists.csv. [U]=7.
- Critical: level_enum.txt excluded from every data build yet sole source of achievement level ids → "Master of Multiwinia" never awardable unless patched; benchmark/stress maps declare GameTypes none → menu discards them; OrderedInsert off-by-one → tie order depends on archive index order.
## Phase 3 launched: S01

## S27 (27_audio_engine.md) — done, ~154 KB
- Voice stealing confirmed (lowest-priority channel replaced, no compare; priority = perceived volume × 0.75 per louder copy of same event); volume curves 5 dB (DSound) vs 4.343 dB (software) confirmed. 5 contradictions resolved: master-volume slider (code/UI/soundsliderbutton.h) IS live (P17b/P18 said dead); WAV decoder unreachable; record-to-file hook no caller; smoothed params start values; sound tools reachable only via cheat window. New: nearest-emitter crowd sounds discarded unless emitter within ~316 units of camera; software mixer likely crashes if selected; no audio device → queued sounds never freed. 27-U1..U14.
## Phase 3 launched: S32

## X2b_asset_catalogue_audio — done
- 10.A–K: method + P16 check (0 mismatches; corrections: officercreate defined twice in sounds.txt; music releases 3.3–20.7 s not 0); name resolution/packaging; full event table (256 data pairs); 213 events fired by code with no data; stop sites; music; UI; voice; stats; unused samples. 25.A pipeline, 25.B one row per file (1332). CSVs: X2b_samples.csv, X2b_sound_events.csv (469), X2b_callsites.csv (285), X2b_summary.txt. [U]=4.
- Music NEVER heard in MW (triggers compiled out or Darwinia-only; all 11 tracks excluded from shipped archives; triggering would release-assert). 182/256 bound events live; 898 files can play; 192 unused (still shipped). Bugs: crate decay sound uses "%pc" format → silent; stationary loops (Assault pulse-bomb) never stopped. Match-end voice samples unbound.
## Phase 3 launched: S10

## X1b_asset_formats — done
- 19 format sections, 23 anchors (15 requested + text-common, redshirt, pack-list, icons, mwhelp, thumbnail, sepulveda, purity); EBNF/byte layouts verified against parsers + survey scripts (scripts/X1b_*.py). [U]=5.
- Archive lookups case-insensitive (contradicts P14 "callers must pass lower-case"); loose files always override archives; no encryption of archives/data — only user saves use "redshirt2"; effects.txt and stats.txt bind by ROW POSITION not name; effects.txt "Default" column never applied; blank line inside sounds.txt EVENT/EFFECT/SAMPLEGROUP crashes loader; 11 Darwinia music/loader samples absent from retail archives → fatal if triggered; textures/shaman_background.bmp is RLE8 (undecodable); 4/8-bit row padding latent bug; two shapes overrun memory in accurate hit tests (more positions than vertices); in-tree exporter outdated vs engine headers.
## Phase 3 launched: S26

## S07 (07_ai.md) — done, ~153 KB
- 16 contradictions resolved against code. Behaviour-changing: **Retribution IS live** (Multiwinia::Advance calls AdvanceRetribution when RetributionMode option set — verified by coordinator; P08 was wrong) → RunForest reachable; WallBuster AITarget doesn't exist (creation commented out); monster + Futurewinian teams always get AI entity; AI entity spawns at (20,0,20) clamped; Assault armour "mobile bomb" never runs; spawn-point Goto officers created in every mode + tutorials; timer periods 11/101/200 ticks not 1/10/20 s.
- New: officer promotion stalls when CPU team has selection (orphaned Officer tasks fill 6 slots); initial node graph built before walls/fences known; IsWalkable measures slope vs start point not per step. 12 open items.
- Corrections relayed to S04 and S01.

## S05 (05_economy_and_population.md) — done, ~131 KB
- 8 contradictions resolved: Retribution runs (absent option reads −1 → counts as on); destroyed ant hills release no spirits (monster-team spirits never created); spore generators spirit only when player-owned; handicap divides by count incl. monster+Futurewinian teams (only helps single-spawn team); spawn link 11 ticks first hop, 10 thereafter; doubled Blitzkrieg waves every 330 ticks; Engineer spirit collection + egg fertilisation LIVE in FULL; Harvester any-team souls confirmed.
- New: Harvester deliveries ignore pop cap and 30 s activation delay; AntHill ants not counted against monster cap. 05-U1..U12. Script scripts/S_05_economy_tables.py.
- Relayed to S01, S04, S02.

## X2a_asset_catalogue_visual — done
- 799 in-scope files catalogued (each under data/... path in markdown, script-checked). 666 non-source: 544 referenced in FULL, 71 unused, 23 legacy-only, 17 D3D-only, 10 debug-only, 1 editor-only; 133 .max authoring sources never read. Shapes: 139 files, 441 fragments, 150,820 triangles, 425 markers. Outputs: drafts/X2a_textures.csv, X2a_shapes.csv, X2a_refs.csv, X2a_missing.csv; scripts/X2a_catalogue.py, X2a_assemble.py (hand-written inputs copied to _work/script_inputs/X2a_part{A,C}.md). [U]=5.
- Critical: contents of patch.dat / branding.dat unknown — live MarkerSystem::RenderCompass can request missing icons/compass_empty.bmp → fatal assert unless an archive supplies it; which font atlases each retail SKU shipped (currency atlases in no language list); 52 FULL-referenced assets used only by buildings absent from MP maps.
## Phase 3 launched: S25

## S04 (04_crates_powerups_and_weapons.md) — done, ~196 KB
- All 22 class headings present with parameter tables; 31 shipped rewards + Beast (debug) + 3 commented; per-level probability table regenerated (scripts/S_04_crate_tables.py, 0 mismatches vs P08_crate_pools.csv); scripts/S_04_grenade_sim.py.
- 10 contradictions resolved: Retribution live (option-gated, effectively on unless 0); Squad crate = 3 squaddies not 5; Beast debug-only; shockwave ring starts r=39, ~10-wide band → 2–3 hits; rocket ~95 wu range; 5 enum comments contradict code (shield coverage, Spawn Mania, Mega Rage, Rage, tree/egg counts); −0.1/tick timers finish one tick late (1.0 s → 11 ticks).
- 04-U1: any map without sea would hang Nuke (no per-map heightmap scan done) — candidate for Phase 5 check using scripts/P11_* terrain port.

## S23 (23_terrain_and_landscape.md) — done, ~145 KB
- 5 contradictions resolved: undefined negative→u16 case never occurs in shipped maps (defined truncation case 29,051× must be copied); "landscape lightmap" is actually the water shore mask; Round truncates; far-edge wrap == clamp; no maps without sea (also independently confirmed by coordinator C_nuke_sea_check.py: min 26.2 % sea). 39.7M pow calls, only 17 near rounding boundary (all pow(x,2) exact). cellSize parsed at float precision shifts a tile by one cell in mp_koth_2P_1. scripts/S_23_terrain_oracle.py (SHA-256 digests). 23-U1..U5.

## X3_determinism_sync — done
- 10 RNG generators, 7 usage rules; FP model; timestep/slice schedule/container order/id allocation; sim-vs-cosmetic classification; render-writes-sim scan; 12 local-state leaks; sync tooling; statics surviving between matches; 20-row risk register. Tables: X3_syncrand_sites.csv (655 synced calls, none in UI/render/sound), X3_unsynced_rng_sites.csv (328 with verdicts), X3_local_state_refs.csv (1,941). [U]=7.
- Shipped sync byte = next synced-RNG output (only catches RNG draw-count divergence); InsertionSquad::DirectControl reads local free-look flag (F2) → live desync; Armour::SetDirectOrders function-static timer persists across matches; hand-edited Difficulty pref reaches sim via Options→Other→Apply; Tree hit spheres from shared non-thread-safe global LCG.
## Phase 2 complete except X1a. Phase 3 launched: S21, S22

## S24 (24_rendering.md) — done, ~258 KB
- 12 contradictions resolved: D3D build has 4 compile blockers (2 certain, 2 SDK-dependent); clear/fog colour always black; Darwinians are camera-turning cards; fake decal shadows exist (blob + flattened sprite); sprites/viriifull.bmp built at run time; 3 levels enable special lighting (mp_koth_2P_3, mp_koth_2P_3b, mp_koth_3P_1). New: team 5 never shows building ownership lights. No encoded perf budgets (24-U6); High/Medium/Low derived from 4 detail settings. 24-U1..U15.

## X1a_text_formats — done (Phase 2 COMPLETE)
- 21 fmt-* anchors: shared tokeniser; level files (14 block types, 18 MultiwiniaOptions keys, 15 LevelOptions, all 75 building type names, 27 entity names); level_enum; game.txt family; locations.txt; multiwinia.txt; betatester lists; preferences; input bindings/keyboard layouts/locales; language files (markup, _kbd/_xin keys, placeholders); scripts (43 opcodes); purity; testbedconfig; MultiwiniaSupport.ini; runtime-written files (auth key, sync dumps, .mwm, saves, logs, serverHistory.dat). drafts/X1a_level_field_usage.csv, X1a_encoding_survey.csv; scripts/X1a_*.py (7). [U]=5.
- level_enum.txt excluded from retail main.dat but "Master of Multiwinia" + "Explorer" depend on it (ids 1–49 match 49 retail maps); default_preferences.txt possibly never applied in packaged install; ja/ko/pt language files ~10% coverage, no English fallback. Archive lookups case-insensitive (confirms X1b vs P14).
## Phase 3 launched: S08, S29

## S03 (03_buildings_and_world_objects.md) — done, 326 KB, 25 diagrams, checks pass
- 34 of 75 building types live in MW. 11 contradictions resolved: GetNetworkTime == game timer; 8 dev maps not 9; Portal debug-only; port occupancy/lifter counting; walls only destroyable by WallBuster breach + defender scuttle; NO MW map has a FenceSwitch → local-input switch path unreachable (closes P01 desync concern); rocket damage/decay; Bridges unreachable (needs Engineer research 5, max 4). 33 defects/hazards listed. 17 open items.

## S06 (06_player_control_and_input.md) — done, ~348 KB, checks pass
- 11 contradictions resolved: Darwinia control scheme NOT selectable (PrefsManager::Load forces MultiwiniaControlMethod = 1); key rebinding unreachable (opening button commented out); "C" on selected officer kills it (not demote); armour right-click / Shift+right-click semantics pinned. icons/compass_empty.bmp missing (also X2a). All 164 control types in §12 with "used in match" flag + reverse index. 06-U1..U12.
## S30 (30_scripting_tutorial_and_cutscenes.md) — done, ~189 KB, checks pass
- 7 contradictions: scripts 37 LEGACY + 5 DEAD (not 39+3); radar dish 4 becomes player's when ≥3 units occupy ControlStation 51; tutorial map CRCs match mapcrc.h; phrase variants by input mode; music event names case-insensitive. New: Tutorial 2 checks ownership vs hard-coded team 0; scripted camera moves overshoot mount by 4–8 % uncorrected; CameraAnimations blocks not written on map save; script triggers T3–T5 debug-only. 30-U1..U8.
## Coordinator: manifest gained included_by_live column (18 non-project files compiled via #include); build_matrix §6 corrected; coverage checker treats them as live.

## S02 (02_units_and_entities.md) — done, ~330 KB, checks pass (resumed after rate limit)
- 26 entity types full template; 47 class headings. ~17 contradictions resolved: ant capture of Darwinians live; Spider leg update runs → EntityLeg live via Spider; dormant Eggs never decay (120 s limit dead); Harvester + Engineer fully team-coloured, Squadie + SpaceInvader fixed colours; Armour has no turret; airstrike formation offsets regenerated per match; AI entity at (20,·,20); Harvester any-team souls + bypasses pop cap; Virii slope sign corrected. 02-U1..U18.

## S20 (20_architecture.md) — done, ~206 KB, checks pass
- 12 contradictions: 10 game-created threads + OS waveOut callback (not 8/9); server 10 Hz vs docs 25 FPS; boot archives parsed on loading worker thread (P16 wrong); ExitLevelButton sets both flags → finished match returns straight to main menu; OpenMP only in ReleaseDirect3D; g_log/g_fogLogging/g_fogEnabled not compiled in shipped exe; testbed counts 23 members/27 methods; GetNetworkTime == GetGameTime; every object advances exactly once per tick; frame throttling only from driver vsync; m_location assigned before Location::Init.
- New: game speed can run away to −∞/NaN if a speed-crate hold is active at next match start; team-5 spawn-point counter slot never reset between matches. 20-U1..U14.

## S31 (31_platform_build_and_tools.md) — done, ~251 KB, checks pass
- 15 contradictions: archive lookups case-insensitive; no-arg data build packs English-only language.dat; authkey.dev read, typed key → authkey, pasted/demo → authkey.dev; MultiwiniaSupport.exe from contrib/XCrashReport uploads over plain TCP :4537 only on Send; ERRORLOG.TXT overwritten per crash; D3D 4 breakages; installers shipping ambx.dat; fullscreen switch order; Mac .cpp count. Scripts: S_31_archive_contents.py (reproduces data build per SKU), S_31_inventory.py, S_31_source_map.py, S_31_citation_fixes.py. 31-U1..U15.
- Flagged: 3 included_by_live marks were basename collisions → coordinator fixed manifest.py to resolve includes by path (now 15).

## S01 (01_game_modes.md) — done, ~180 KB, checks pass
- 6 shipped modes + TankBattle (debug); 39 option definitions + lobby + level-file options; Retribution live, option-gated, absent → −1 → on. 8 contradictions: game-over exit key testbed-only (players leave via in-game menu); PlayTime overwrite is a timing race (usually survives); 38 shipped option rows not 34; Blitzkrieg "base under attack" throttle ~210 s not 20 s; no-team (−1) colour = black not grey; option changes not checked against host. New: on 3 double-wave Blitzkrieg maps lobby options override level reinforcement values. 12 open items; 25 known bugs B1–B25. scripts/S_01_mode_tables.py.

## S09 (09_meta_systems.md) — done, ~390 KB, checks pass
- 7 contradictions: server never checks sender of option/start/ready commands (lobby restrictions UI-only; modified client can change any setting or start game); post-match only Esc → Exit Game returns to menu ("press Ready" path test-build-only); PageDarwinia debug-only (P20 campaign fade bug not player-visible); PlayTime vs default TimeLimit race usually map wins; P17b duplicate help key wrong (two others duplicated); saved player name is truncated one; tutorial map CRCs match → tutorial buttons active. 09-U1..U17. scripts/S_09_lang_coverage.py, S_09_source_map.py.

## S10 (10_audio_design.md) — done, ~319 KB, checks pass
- 10 contradictions (R1–R10): music RELEASE 3.29–20.71 s on 14/17 blocks (27 said 0 → coordinator corrected 27 after verifying data); 22 effect uses in 22 blocks (27 said 21 → corrected); no zero beep when timed game ends (code in branch that only runs while time remains); master volume slider live (filename-case include). scripts/S_10_audio_tables.py re-parses sounds.txt (0 mismatches vs P16/X2b): 469 rows, 74 bound-but-silent pairs, 22 effect uses, 77-file call-site map. 10-U1..U16.

## S26 (26_animation_and_vfx.md) — done, ~161 KB, checks pass
- No skeletal animation (legs, trails, cloth flags all procedural). All 6 .anim files object by object (scripts/S_26_anim_tracks.py); 19 particle types + 98 live emitter sites (scripts/S_26_particle_emitters.py). 8 contradictions: Spider uses EntityLeg; Darwinian cards blend to billboards with distance; spider ≤1 regular foot lift/tick + emergency lifts; shattering ≤502 tris/fragment; debris spin ±2 rad/s; gesture trail fades continuously; effects.txt configures only audio DSP filters (inactive unless hidden pref) — EffectProcessor unrelated; particles can't affect sim except via shared synced RNG draws at spawn. 26-U1..U9.

## S32 (32_debug_test_and_performance.md) — done, ~148 KB, checks pass
- 8 contradictions: 18 debug entry points (not 17); debug menu + profiler unreachable in FULL; syncerror-*.txt world dump written in FULL too (only preceding RNG-call log debug-only); benchmark bandwidth bytes/s averaged 5 s; cheat crate reward −1 → normal random reward; testbed "10× faster" server not implemented. docs/cpu cycles.txt is a gameplay design note (no perf data) → hotspots built from code (186 profiled call sites). No measured frame times anywhere (32-U2). FTP credentials deliberately omitted. 32-U1..U8 + 32-I1..I3. scripts/S_32_profile_sections.py.

## S25 (25_textures_and_assets.md) — done, ~694 KB, checks pass, (e) = 0
- All 2131 required asset files have a row (scripts/S_25_catalogue.py from X2a/X2b CSVs + script_inputs/S_25_template.md; self-checking). 11 contradictions: icons/compass_empty.bmp can NEVER be requested (Vector2::Normalise maps zero vector to (0,1)) → no reachable missing-file crash; viriifull built from 18 sprite copies (not 32); cloud + laser textures first loaded mip-mapped by start-up preload; archive lookups case-insensitive but texture cache case-sensitive; 506 BMPs (not 667); 441 fragment blocks vs 440 names. New hazard: carryweights.shp (live CTS mesh) overruns hit-test buffer on accurate hit test. 25-U1..U10.
## Coordinator: redacted FTP credential string (from code/benchmark.cpp) in drafts/P15_network.md and routed/_meta_ip.md; final docs only used placeholders.

## S21 (21_simulation_and_entity_model.md) — done, ~209 KB, checks pass, (d) = 0
- 9 class-diagram blocks containing all 159 classes (scripts/S_21_classdiagram.py verify); 159-row class table; 162-file source map. 8 contradictions: UnprocessedSpirit build tag; effect-id allocation; DArray vs FastDArray slot reuse; S07 walls-in-node-graph claim; m_specials membership; s_offsets reset; FenceSwitch leak; 25 fps vs 10 Hz. New: obstruction grid outside cell never cleared (building list grows all match) → EnterTeleports could dereference deleted building at far map edge in Release; spirit handles lack generation check; routing node tables never rebuilt after load. 21-U1..U11.

## S22 (22_determinism_and_networking.md) — done, ~290 KB, checks pass
- 13 contradictions resolved. **SUPERSEDES earlier log entries:** (1) Darwinian rocket-watch wall-clock facing is NOT a desync — EscapeRocket::IsSpectacle() returns false when g_app->Multiplayer() (always true in MW) [C, coordinator-verified rocket.cpp:EscapeRocket::IsSpectacle]; (2) FenceSwitch NOT a desync (blocked type, no shipped level has one); (3) Unit::s_offsets IS reset at every level load; (4) team-5 spawn-point counter NOT reset between matches (latent only); (5) sync RNG NOT stale across matches — each match runs in a new server session with a new seed, lobby draws nothing (supersedes P01 "seeded only at client join"). Also: desync freezes all peers at the same sequence id (error message is sequenced); RecordDemo replay unreliable (seed not stored; history loads after replaying session's own START_GAME). 22-U1..U21. Metaserver server side absent → only client half of protocol documented.

## S08 (08_levels_and_level_design.md) — done, ~299 KB, checks pass; all 174 data/levels + thumbnails files cited
- 10 contradictions: 86/86 + 87/87 CRC (86 levels + level_enum.txt); LevelFile::m_gameTypes single index not bitmask; no victory jumps on shipped maps (GenerateAutomaticObjectives always adds NeverTrue); light colours up to R 5.23; FenceSwitch desync can't arise; two Rocket Riot maps do have spawn points; plain-text .mwm unreadable; editor options control names; reinforcement-mode value collisions; archive names lower-cased when indexed ([C]). New: level-file routes ignored in MW; special lighting forced on every Rocket Riot match. scripts/S_08_tables.py fills 19 regions. 08-U1..U13.
## S00 (00_overview.md) — done, ~101 KB, checks pass, 352 links/0 bad anchors (41/90 pending)
- Retribution live; CPU AI entity DOES count against team pop cap (in Team::m_others; s_entityPopulation cap commented out) → coordinator verified (location.cpp spawns with unit −1) and corrected 07 §2/§14; 22 corrections applied (Q8 live-desync list = 22 §14.11). Glossary placeholder names diverge across 01–10 IP tables (e.g. "Gateway" vs "Spawn Node") → final naming in 90.

## S29 (29_data_formats_reference.md) — done, ~327 KB, 56 formats, checks pass
- 12 formats added to registry (binary-common, directory, map-crc, png-output, server-history, team-controls, blackbox, crash-report, debug-log, origin-version, language-errors, user-info). 8 contradictions: archive lookups case-insensitive (P14 wrong); UTF-16 reader doesn't recognise "redshirt2" marker (compares wrong chars); only 8-bit disk reader keeps unterminated last line; comment-glued-to-token + Key=1 preferences-writer bug; 8-bit language line with key but no text crashes; 63+ char camera-mount name termination → [I]; authkey writers reconciled. 29-U1..U15.

## PHASE 4 (content set 00–32): (a) 0 (b) 0 (c) 0 (d) 0 (e) 0; bad citations 0; oversize excerpts 0; Mermaid 152/152 OK; 99 generated (6189 rows, every row cited or justified); 91 generated (71 concepts, 159 classes, 56 formats).
## PHASE 5 started: seeded claim sample (scripts/sample_claims.py, seed 20261017, 12/doc → 288 claims across 24 docs, 40 excluded while in progress); 8 independent reviewers R1–R8 (review/REVIEW_BRIEF.md).

## S40 (40_carry_forward_assessment.md) — done, ~190 KB, checks pass
- 30 subsystem rows: 5 Keep / 11 Adapt / 13 Replace / 1 Drop; 72 feature verdicts; 38 legacy techs mapped (T1–T38); nothing must be bit-exact, only behaviour (tick counts, explosion falloff). MP recommendation: deterministic lockstep with server-sequenced command stream (relay or host) + optional headless referee; full state sync rejected (bandwidth), rollback rejected (CPU); 17 preconditions P1–P17 (fixed integer tick ~20 Hz, integer timers, x64 SSE2 /fp:strict sim lib + own transcendental math, no FMA/AVX in sim, PCG/xoshiro SimRng per match, sim isolation, no mutable statics, generational ids + stable order, 64-bit state hash per tick, command authority, content hashes, version gating, MTU/fragmentation, snapshot+command-tail join/reconnect, bounded algorithms, replays, headless sim, referee). Keep-list 25, fix-list 58, build order P0–P8. 40-U1..U17 (U15 ARM64, U16 cross-platform → fixed point).
- Coordinator stress-test: preconditions address float determinism (P3) and late-join (P13); residual risk = ARM64/cross-play scope (40-U15/U16) and authoring a vetted deterministic transcendental library.

## Review R2 (02, 04) — done: 62 claims (52 C / 8 partly / 2 wrong / 0 unverifiable); gaps 0 High, 6 Med, 12 Low; 5 cross-doc inconsistencies
- Shockwave model wrong in 02 §2.7 (band 20/~5 hits/ring 35 vs code ring 39, ~10-wide band, 2–3 hits; 04 §6.11 right; 26:888 repeats wrong values); IsInvincible has no live caller (02:860 wrong) — subversion lasers, SpamInfection, Randomiser bypass shields; creature contact attacks unresistable → 04 shield list incomplete; rocket moves 8 or 9 times depending on slice order (≈95/≈113 wu); front-end pref can change m_difficultyLevel on one peer (02:234); 03:898 overstates crate-turret self-destruct.
