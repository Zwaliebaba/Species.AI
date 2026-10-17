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
