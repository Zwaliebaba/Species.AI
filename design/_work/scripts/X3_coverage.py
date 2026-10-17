#!/usr/bin/env python3
"""X3: build the coverage fragment OUT/_work/coverage/X3_determinism_sync.csv.

Manual rows (files read by hand) + one row per file that contains a hit in the generated
RNG CSVs (scanned by X3_rng_sites.py) not already listed by hand.

Usage: X3_SRC=<SRC_ROOT> python3 -I X3_coverage.py <SYNC_CSV> <UNSYNC_CSV> <OUT_CSV> [<LOCAL_STATE_CSV>]
"""
import csv
import sys

sync_csv, unsync_csv, out_csv = sys.argv[1:4]
local_csv = sys.argv[4] if len(sys.argv) > 4 else None

S1 = '1. Determinism model at a glance'
S2 = '2. Random number sources'
S3 = '3. Floating-point model'
S4 = '4. Timestep, frame structure and update ordering'
S5 = '5. Simulated vs cosmetic state'
S6 = '6. Sync checking, desync reaction and forensics'
S7 = '7. Session-level preconditions and cross-match state'
S8 = '8. Determinism risk register'

manual = [
    ('contrib/systemiv/lib/math/random_number.h', 'full', '', [S2]),
    ('contrib/systemiv/lib/math/random_number.cpp', 'full', '', [S2, S6]),
    ('contrib/systemiv/lib/math/math_stubs.cpp', 'full', '', [S3]),
    ('contrib/systemiv/lib/math/math_stubs.h', 'full', '', [S3]),
    ('contrib/systemiv/lib/math/fixed.h', 'full', '', [S3]),
    ('contrib/systemiv/lib/math/fixed_64.h', 'skimmed', 'first 60 lines: type layout only; not compiled', [S3]),
    ('contrib/systemiv/lib/hash.h', 'full', '', [S2, S6]),
    ('contrib/systemiv/lib/debug_utils.cpp', 'skimmed', 'HashDouble only', [S6]),
    ('contrib/systemiv/lib/tosser/fast_darray.cpp', 'skimmed', 'Grow/PutData/RemoveData/free list', [S4]),
    ('contrib/systemiv/lib/tosser/directory.h', 'skimmed', 'USE_FIXED hook only', [S3]),
    ('contrib/systemiv/lib/metaserver/metaserver.cpp', 'skimmed', 'ServerRegisterWANThread sleep jitter', [S2]),
    ('code/lib/universal_include.h', 'skimmed', 'targets, TRACK_SYNC_RAND, iv_* macros, includes', [S2, S3]),
    ('code/lib/math_utils.cpp', 'skimmed', 'orphan MT19937 and header; geometry helpers not needed', [S2]),
    ('code/lib/math_utils.h', 'full', '', [S3]),
    ('code/lib/vector3.h', 'full', '', [S2, S3]),
    ('code/lib/vector3.cpp', 'full', '', [S2, S3]),
    ('code/lib/matrix34.cpp', 'skimmed', 'grep atan2/DecomposeToYDR', [S3]),
    ('code/lib/matrix33.cpp', 'skimmed', 'grep atan2/DecomposeToYDR', [S3]),
    ('code/lib/slice_darray.h', 'full', '', [S4]),
    ('code/lib/slice_darray.cpp', 'full', '', [S4]),
    ('code/lib/work_queue.cpp', 'full', '', [S3, S4]),
    ('code/lib/resource.cpp', 'skimmed', 'CreateDisplayListAsync only', [S2]),
    ('code/lib/opengl_directx.cpp', 'skimmed', 'CreateDevice flags (grep)', [S3]),
    ('code/main.cpp', 'skimmed', 'StandardiseCPUPrecision, Initialise, UpdateAdvanceTime, LocationGameLoop, DoEnterLocation/EnterLocation', [S3, S4, S5, S6]),
    ('code/main.h', 'skimmed', 'g_gameTime/g_advanceTime declarations', [S5]),
    ('code/app.cpp', 'skimmed', 'GamePaused, Multiplayer, UpdateDifficultyFromPreferences, ShutdownCurrentGame, affinity', [S2, S6, S7]),
    ('code/gametimer.cpp', 'full', '', [S4, S5]),
    ('code/gametimer.h', 'full', '', [S4]),
    ('code/globals.h', 'full', '', [S4]),
    ('code/location.cpp', 'skimmed', 'Location ctor/Init/Empty/InitBuildings, Advance*, GetBuilding, Bang, UpdateTeam, SelectDarwinians, IsFriend, DumpWorldToSyncLog, debug watch', [S2, S4, S5, S6, S7]),
    ('code/team.cpp', 'skimmed', 'Team::Advance, SelectUnit, RenderDarwinians, TeamControls pack/unpack/advance, RenderSyncErrors', [S3, S4, S5]),
    ('code/unit.cpp', 'skimmed', 'AdvanceEntities, GetFormationOffset', [S4, S7]),
    ('code/entity_grid.cpp', 'skimmed', 'GetNeighbours order', [S4]),
    ('code/entity_grid_cache.cpp', 'full', '', [S4]),
    ('code/entity_grid_cache.h', 'full', '', [S3, S4]),
    ('code/particle_system.cpp', 'skimmed', 'ParticleSystem::Advance, Particle::Advance RNG', [S2, S5]),
    ('code/clouds.cpp', 'skimmed', 'Clouds::Advance', [S4, S5]),
    ('code/loading_screen.cpp', 'skimmed', 'Render loop, jobs, threads', [S2, S4]),
    ('code/gamecursor.cpp', 'skimmed', 'RenderCrosshair', [S5]),
    ('code/camera.cpp', 'skimmed', 'SwitchEntityTracking, UpdateEntityTrackingMode, AdvanceFreeMovementMode, shake', [S5]),
    ('code/location_input.cpp', 'skimmed', 'NEXTWEAPON send, armour local SetOrders branch', [S5, S7]),
    ('code/taskmanager.cpp', 'skimmed', 'myTeamId guards, wall clock, float locals', [S3, S5]),
    ('code/taskmanager_interface_multiwinia.cpp', 'skimmed', 'm_mostRecentTaskId reset, DeleteTask zone', [S5]),
    ('code/multiwinia.cpp', 'skimmed', 'ResetLocation, AdvanceAssault end, AdvanceGracePeriod statics, SaveIFrame, DeclareWinner speed', [S4, S6, S7]),
    ('code/multiwiniahelp.cpp', 'skimmed', 'tutorial crate creation, rand()', [S2, S4, S5]),
    ('code/level_file.cpp', 'skimmed', 'ParseInstantUnits, ParseBuildings difficulty, ParseDifficulty', [S2, S7]),
    ('code/landscape.cpp', 'skimmed', 'private LCG (grep)', [S2]),
    ('code/renderer.cpp', 'skimmed', 'sync error banner', [S6]),
    ('code/explosion.cpp', 'skimmed', 'wall clock and RNG use', [S5]),
    ('code/mapcrc.h', 'skimmed', 'head only', [S7]),
    ('code/network/clienttoserver.cpp', 'skimmed', 'AdvanceAndCatchUp, SendSyncronisation, SetSyncState, GetNumSlicesToAdvance, GenerateSyncValue, KeepAlive, DecodeCmd, ProcessAllServerLetters, ProcessServerLetters, ProcessServerUpdates, AddIFrame/SendIFrame', [S1, S2, S4, S6]),
    ('code/network/clienttoserver.h', 'skimmed', 'sync members (grep)', [S6]),
    ('code/network/iframe.cpp', 'full', '', [S6]),
    ('code/network/iframe.h', 'full', '', [S6]),
    ('code/network/server.cpp', 'skimmed', 'MakeRandom, GenSyncRandSeed, Initialise, SendLetter, AdvanceIfNecessary, HandleSyncMessage, NotifyNetSync*, Save/LoadHistory', [S1, S2, S6]),
    ('code/network/servertoclient.h', 'skimmed', 'sync members (grep)', [S6]),
    ('code/network/networkvalue.cpp', 'skimmed', 'NetworkFloat/Double parse truncation', [S3]),
    ('code/network/network_quality.cpp', 'skimmed', 'AppRandom packet loss', [S2]),
    ('code/interface/prefs_other_window.cpp', 'skimmed', 'ApplyOtherButton difficulty, ctor', [S7]),
    ('code/interface/mainmenus.cpp', 'skimmed', 'OtherOptionsButton (grep)', [S7]),
    ('code/interface/network_window.cpp', 'skimmed', 'out-of-sync label (grep)', [S6]),
    ('code/sound/sound_instance.cpp', 'skimmed', 'OpenStream AppRandom', [S2]),
    ('code/worldobject/worldobject.h', 'skimmed', 'WorldObjectId and effect enum', [S4]),
    ('code/worldobject/worldobject.cpp', 'skimmed', 'unique id counter, LogState', [S4, S6]),
    ('code/worldobject/entity.cpp', 'skimmed', 'LogState, EntityIsBlocked, CreateEntity, GetStat, m_renderDamaged', [S2, S6, S7]),
    ('code/worldobject/building.cpp', 'skimmed', 'BuildingIsBlocked, CreateBuilding, Advance, Does*Hit, LogState', [S5, S6]),
    ('code/worldobject/crate.cpp', 'skimmed', 'SetDetail, Render, hit tests, RunExtinguisher, speed crates, LogState', [S2, S4, S5]),
    ('code/worldobject/crate.h', 'skimmed', 'INCLUDE_THEBEAST guard, reward enum', [S2]),
    ('code/worldobject/tree.cpp', 'skimmed', 'Advance, SetDetail, Generate*, RenderAlphas, RenderBranch, CreateAnotherTree, LogState', [S2, S3, S5]),
    ('code/worldobject/darwinian.cpp', 'skimmed', 'AdvanceOnFire/Sacraficing/Infection/OpeningCrate/WatchingSpectacle/Combat, SearchFor*, GetCrateOffset, LogState, Render', [S2, S3, S5]),
    ('code/worldobject/officer.cpp', 'skimmed', 'CalculateBoundingSphere, SetNextMode statics, LogState, Advance', [S5, S7]),
    ('code/worldobject/armour.cpp', 'skimmed', 'SetDirectOrders, SetOrders, IsUnloading', [S7]),
    ('code/worldobject/insertion_squad.cpp', 'skimmed', 'DirectControl', [S5]),
    ('code/worldobject/spider.cpp', 'skimmed', 'Advance, UpdateLegs, CalcWhichFootToMove, AdvanceAttack/Pouncing', [S3, S5]),
    ('code/worldobject/entity_leg.cpp', 'skimmed', 'LiftFoot, Advance (wall clock)', [S5]),
    ('code/worldobject/souldestroyer.cpp', 'skimmed', 'ctor spirit positions, Attack', [S2]),
    ('code/worldobject/triffid.cpp', 'skimmed', 'Advance fire particles, TriffidEgg::Advance volatile', [S2, S3]),
    ('code/worldobject/meteor.cpp', 'skimmed', 'Advance particles and slow-mo', [S2, S4]),
    ('code/worldobject/weapons.cpp', 'skimmed', 'ThrowableWeapon Advance/Render, PulseWave, Rocket::Explode', [S2, S5]),
    ('code/worldobject/radardish.cpp', 'skimmed', 'RenderAlphas save/restore, receiver fields', [S5]),
    ('code/worldobject/laserfence.cpp', 'skimmed', 'RenderAlphas link reset, Read, Advance link use', [S5]),
    ('code/worldobject/gunturret.cpp', 'skimmed', 'Render editor write, m_turretFront use', [S5]),
    ('code/worldobject/rocket.cpp', 'skimmed', 'EscapeRocket::SetupSpectacle/IsSpectacle', [S5]),
    ('code/worldobject/rocket.h', 'skimmed', 'float member', [S3]),
    ('code/worldobject/randomiser.cpp', 'skimmed', 'Render rand()', [S2]),
    ('code/worldobject/spiritstore.cpp', 'full', '', [S2]),
    ('code/worldobject/trunkport.cpp', 'skimmed', 'Advance/RenderAlphas wall clock', [S5]),
    ('code/worldobject/sporegenerator.cpp', 'skimmed', 'difficulty use', [S7]),
    ('code/worldobject/anthill.cpp', 'skimmed', 'difficulty use', [S7]),
    ('code/worldobject/virii.cpp', 'skimmed', 'RecordHistoryPosition pref', [S7]),
    ('code/worldobject/generator.cpp', 'skimmed', 'SolarPanel Initialise/Render orientation', [S5]),
    ('code/worldobject/aiobjective.cpp', 'skimmed', 'ctor draw (CSV)', [S2]),
    ('code/worldobject/ai.h', 'skimmed', 'RRSolarPanelInfo float', [S3]),
    ('targets/vs2005/Darwinia.vcproj', 'skimmed', 'Release|Win32 and ReleaseDirect3D compiler tool blocks; FloatingPointModel/OpenMP/math file grep', [S3]),
    ('targets/vs2005/Common.vsprops', 'full', '', [S3]),
    ('targets/vs2005/Direct3D.vsprops', 'full', '', [S3]),
    ('targets/vs2005/OpenGL.vsprops', 'full', '', [S3]),
    ('data/input/input_preferences.txt', 'skimmed', 'CameraFreeMovement binding (grep)', [S5]),
    ('data/default_preferences.txt', 'skimmed', 'DarwinianScaleFactor (grep)', [S7]),
    ('docs/Network.txt', 'skimmed', 'resync passage', ['Docs-vs-code discrepancies']),
    ('docs/multiwinia_todo.txt', 'skimmed', 'determinism todo', ['Docs-vs-code discrepancies']),
    ('docs/multiwiniareleasenotes-beta.txt', 'skimmed', 'sync fixes, tracking note', [S6, 'Docs-vs-code discrepancies']),
    ('docs/multiwiniachanges-beta.txt', 'skimmed', 'sync fixes (grep)', [S6]),
    ('docs/multiwinia-changes.txt', 'skimmed', 'sync fixes (grep)', [S6]),
]

rows = {}
import glob, os
SRC_ROOT = os.environ.get('X3_SRC', '')
for mp in sorted(glob.glob(os.path.join(SRC_ROOT, 'data', 'levels', 'mp_*.txt'))):
    rel = os.path.relpath(mp, SRC_ROOT).replace(os.sep, '/')
    manual.append((rel, 'skimmed', 'scripted: LaserFence link integrity (scripts/X3_fence_links.py), CreatedAsDifficulty, AIObjectiveMarker presence', [S5, S7]))
for path, depth, reason, secs in manual:
    rows[path] = [path, depth, reason, ';'.join(secs)]

for csv_path, label in ((sync_csv, 'synced'), (unsync_csv, 'unsynced')):
    with open(csv_path, newline='') as fh:
        for r in csv.DictReader(fh):
            p = r['file']
            if p in rows:
                continue
            rows[p] = [p, 'skimmed', 'scanned by scripts/X3_rng_sites.py for RNG call sites; flagged lines read in context', S2]

if local_csv:
    with open(local_csv, newline='') as fh:
        for r in csv.DictReader(fh):
            p = r['file']
            if p not in rows:
                rows[p] = [p, 'skimmed', 'scanned by scripts/X3_local_state_sites.py for local-state references; flagged lines read in context', S5]

with open(out_csv, 'w', newline='') as fh:
    w = csv.writer(fh)
    w.writerow(['path', 'depth', 'reason', 'sections'])
    for k in sorted(rows):
        w.writerow(rows[k])
print('rows', len(rows))
