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
