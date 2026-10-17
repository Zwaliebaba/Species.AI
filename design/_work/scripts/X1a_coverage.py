#!/usr/bin/env python3
"""X1a: write coverage/X1a_text_formats.csv. Usage: python3 -I X1a_coverage.py SRC_ROOT OUT_CSV"""
import csv, glob, os, sys
root, out = sys.argv[1], sys.argv[2]
S_LVL = '2. Level files'
rows = []
def add(path, depth, reason, sections):
    rows.append((path, depth, reason, sections))
def rel(p): return os.path.relpath(p, root).replace(os.sep, '/')
for p in sorted(glob.glob(os.path.join(root, 'data/levels/mp_*.txt'))):
    add(rel(p), 'full', '', S_LVL + ' (machine-parsed by X1a_level_survey.py)')
for p in sorted(glob.glob(os.path.join(root, 'data/levels/map_*.txt'))):
    add(rel(p), 'full', '', S_LVL + '; 3. Campaign world files')
for p in sorted(glob.glob(os.path.join(root, 'data/levels/mission_*.txt'))):
    add(rel(p), 'full', '', S_LVL + '; 2.15 Mission-only blocks; 3. Campaign world files')
add('data/levels/level_enum.txt', 'full', '', '2.19 level_enum.txt')
for p in sorted(glob.glob(os.path.join(root, 'data/levels/old/*.txt'))):
    add(rel(p), 'full', '', S_LVL + ' (old/ set, unreachable)')
for p in sorted(glob.glob(os.path.join(root, 'data/purity/*'))):
    n = os.path.basename(p)
    sec = {'game.txt': '3. Campaign world files', 'buyme.txt': '9. Cutscene scripts',
           'map_mine.txt': S_LVL, 'mission_mine_enable.txt': S_LVL}.get(n, '')
    add(rel(p), 'full', '', '10. data/purity; ' + sec)
for p in sorted(glob.glob(os.path.join(root, 'data/input/*.txt'))):
    n = os.path.basename(p)
    add(rel(p), 'full', '', '7.7 locales.txt' if n == 'locales.txt' else '7. Input binding files')
for p in sorted(glob.glob(os.path.join(root, 'data/input/keyboards/*'))):
    add(rel(p), 'full', '', '7.6 Keyboard layout files')
for p in sorted(glob.glob(os.path.join(root, 'data/language/*'))):
    add(rel(p), 'full', '', '8. Language files (machine-parsed by X1a_language_survey.py)')
for p in sorted(glob.glob(os.path.join(root, 'data/scripts/*'))):
    add(rel(p), 'full', '', '9. Cutscene scripts (checked by X1a_script_check.py)')
top = {'data/game.txt': '3. Campaign world files', 'data/game_demo.txt': '3. Campaign world files',
       'data/game_demo2.txt': '3. Campaign world files', 'data/game_unlockall.txt': '3. Campaign world files',
       'data/locations.txt': '3.4 locations.txt', 'data/multiwinia.txt': '4. multiwinia.txt',
       'data/multiwinia-testbed.txt': '4. multiwinia.txt', 'data/betatesters.txt': '5. Credit name lists',
       'data/multiwinia_betatesters.txt': '5. Credit name lists', 'data/default_preferences.txt': '6. Preferences',
       'testbedconfig.txt': '11. testbedconfig.txt', 'MultiwiniaSupport.ini': '12. MultiwiniaSupport.ini'}
for k, v in top.items(): add(k, 'full', '', v)
src = [
 ('contrib/systemiv/lib/filesys/text_stream_readers.h','full','','1. Shared line reader'),
 ('contrib/systemiv/lib/filesys/text_stream_readers.cpp','full','','1. Shared line reader'),
 ('contrib/systemiv/lib/unicode/unicode_text_stream_reader.h','full','','1. Shared line reader'),
 ('contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp','full','','1. Shared line reader'),
 ('contrib/systemiv/lib/unicode/unicode_string.cpp','skimmed','constructors, assignment, Resize, Length, ReplaceStringFlag, CopyUnicodeToCharArray only','1; 8. Language files'),
 ('contrib/systemiv/lib/filesys/file_system.h','full','','1.1'),
 ('contrib/systemiv/lib/filesys/file_system.cpp','full','','1.1'),
 ('contrib/systemiv/lib/filesys/text_file_writer.cpp','full','','13.3 Campaign save files'),
 ('code/unicode/unicode_text_file_writer.cpp','full','','6.3 Writer; 1.3'),
 ('code/unicode/unicode_text_file_writer.h','full','','6.3'),
 ('code/lib/resource.cpp','skimmed','ParseArchive, GetTextReader, GetBinaryReader, GetTextFileWriter, ListResources, FileExists only','1.1; 2.1'),
 ('code/lib/resource.h','skimmed','container types only','Cross-partition notes'),
 ('code/lib/language_table.h','full','','8. Language files'),
 ('code/lib/language_table.cpp','full','','8. Language files; 13.10'),
 ('code/lib/preferences.h','full','','6. Preferences'),
 ('code/lib/preferences.cpp','full','','6. Preferences'),
 ('code/level_file.h','full','','2. Level files'),
 ('code/level_file.cpp','full','','2. Level files'),
 ('code/landscape.cpp','skimmed','GuideGridSetPower/ToString/FromString only','2.6 LandscapeTiles'),
 ('code/worldobject/building.cpp','skimmed','Read, Write, BuildingIsBlocked, CreateBuilding, GetTypeName, ctor defaults','2.9 Buildings'),
 ('code/worldobject/building.h','skimmed','type enum','2.9'),
 ('code/worldobject/entity.cpp','skimmed','GetTypeName, GetTypeId, EntityIsBlocked, LogState','2.12; 13.5'),
 ('code/worldobject/entity.h','skimmed','type enum','2.12'),
 ('code/worldobject/worldobject.cpp','skimmed','LogState','13.5'),
 ('code/worldobject/triffid.h','skimmed','spawn enum','2.9.3'),
 ('code/worldobject/gunturret.h','skimmed','turret type enum','2.9.3'),
 ('code/worldobject/laserfence.h','skimmed','mode enum','2.9.3'),
 ('code/worldobject/crate.cpp','skimmed','GetName, LogState','2.4.2'),
 ('code/global_world.h','skimmed','class declarations and enums','3.'),
 ('code/global_world.cpp','skimmed','event/condition/action/research read+write, LoadGame, SaveGame, Parse*, LoadLocations, SaveLocations','3.; 2.14'),
 ('code/multiwinia.cpp','skimmed','LoadBlueprints, blueprint ctors, GetGameOption','4.'),
 ('code/multiwinia.h','skimmed','blueprint classes, game type enum','4.'),
 ('code/game_menu.cpp','skimmed','AddMap, CreateMapList','2.4.3'),
 ('code/game_menu.h','skimmed','MAX_GAME_TYPES','2.4.3'),
 ('code/achievement_tracker.cpp','full','','2.19'),
 ('code/achievement_tracker.h','full','','2.19'),
 ('code/script.cpp','skimmed','RunScript, Advance, AdvanceScript, GetOpCode, opcode table, RunCommand_PurityControl, RunCommand_EnterLocation','9.; 10.'),
 ('code/script.h','full','','9.'),
 ('code/lib/input/input.cpp','skimmed','parseInputPrefs, parseInputSpecString/Tokens, getControlID','7.'),
 ('code/lib/input/file_paths.cpp','full','','7.1; 7.7'),
 ('code/lib/input/inputspec.cpp','full','','7.2'),
 ('code/lib/input/inputspec.h','full','','7.2'),
 ('code/lib/input/inputdriver.cpp','full','','7.3'),
 ('code/lib/input/inputdriver.h','skimmed','enums','7.3'),
 ('code/lib/input/inputdriver_simple.cpp','full','','7.3'),
 ('code/lib/input/inputdriver_conjoin.cpp','skimmed','parseInputSpecification','7.3'),
 ('code/lib/input/inputdriver_chord.cpp','skimmed','parseInputSpecification','7.3'),
 ('code/lib/input/inputdriver_invert.cpp','skimmed','parseInputSpecification','7.3'),
 ('code/lib/input/inputdriver_idle.cpp','full','','7.3'),
 ('code/lib/input/inputdriver_win32.cpp','skimmed','acceptDriver, getControlID, getKeyId, control types','7.3'),
 ('code/lib/input/inputdriver_xinput.cpp','skimmed','controls table, acceptDriver, conditions, extra tokens','7.3'),
 ('code/lib/input/inputdriver_prefs.cpp','skimmed','parseInputSpecification','7.3'),
 ('code/lib/input/inputdriver_value.cpp','skimmed','parseInputSpecification','7.3'),
 ('code/lib/input/inputdriver_values.inc','full','','7.3'),
 ('code/lib/input/inputdriver_alias.cpp','skimmed','parseInputSpecification','7.3'),
 ('code/lib/input/keynames.cpp','full','','7.3'),
 ('code/lib/input/keydefs.h','skimmed','KEY_TILDE, KEY_ANY','7.3'),
 ('code/lib/input/control_bindings.cpp','skimmed','getControlID, action table','7.2'),
 ('code/lib/input/control_types.inc','skimmed','counted entries only (enumeration owned by P10)','7.2'),
 ('code/interface/prefs_keybindings_window.cpp','skimmed','ApplyKeybindingsButton','7.5'),
 ('code/main.cpp','skimmed','ReadInputPreferences, InitialiseInputManager, SetPreferenceOverrides, camera mount names, version switch, debug log, history','7.; 13.'),
 ('code/app.cpp','skimmed','App ctor, Initialise, language functions, LoadProfile/SaveProfile, GetPreferencesPath, InitMetaServer, ShutdownCurrentGame, testbed log, Vista thumbnail','3.1; 6.1; 8.1; 13.'),
 ('code/UI/GameMenuWindow.cpp','skimmed','ReadTestBedConfig, BuildGameEntries, ReadMultiwiniaBetaTesters, ReadCredits, param widgets, auth key save','5.; 11.; 13.4'),
 ('code/UI/AuthInputField.h','skimmed','key save path','13.4'),
 ('code/loaders/credits_loader.cpp','skimmed','GenerateBetaTesterList','5.'),
 ('code/loaders/loader.cpp','skimmed','CreateLoader gating','5.'),
 ('code/mapfile.cpp','full','','13.6'),
 ('code/mapfile.h','full','','13.6'),
 ('code/location.cpp','skimmed','LoadLevel, DumpWorldToSyncLog, effects/coop consumers','2.; 13.5'),
 ('code/network/clienttoserver.cpp','skimmed','GetSyncFileName, sync error handling, requested map/mission','13.5; 2.1'),
 ('code/network/iframe.cpp','skimmed','DumpSyncDiffs','13.5'),
 ('code/network/syncdiff.cpp','skimmed','Print','13.5'),
 ('code/network/server.cpp','skimmed','SaveHistory, LoadHistory','13.7'),
 ('code/lib/system_info.cpp','skimmed','language detection','8.1'),
 ('contrib/systemiv/lib/debug_utils.cpp','skimmed','AppDebugOutRedirect, AppDebugOut, AppGenerateBlackBox, HashDouble','13.5; 13.8'),
 ('contrib/systemiv/lib/math/math_stubs.cpp','full','','Conventions (F32)'),
 ('contrib/systemiv/lib/math/random_number.cpp','skimmed','DumpSyncRandLog','13.5'),
 ('contrib/systemiv/lib/metaserver/authentication.cpp','skimmed','key save/load/classification','13.4'),
 ('contrib/systemiv/lib/metaserver/authentication.h','skimmed','AUTHENTICATION_KEYLEN','13.4'),
 ('contrib/systemiv/lib/tosser/hash_table.cpp','skimmed','HashFunc, PutData, GetInsertPos','2.19; 8.'),
 ('contrib/systemiv/lib/tosser/btree.cpp','skimmed','compare function','8.'),
 ('contrib/systemiv/lib/network_stream_inlines.cpp','skimmed','Read/WriteNetworkValue','13.6'),
 ('contrib/XCrashReport/Dlg.cpp','skimmed','GetFileDetails, registry section','12.'),
 ('contrib/XCrashReport/IniDefines.h','full','','12.'),
 ('docs/input_preferences_syntax.txt','full','','Docs-vs-code discrepancies'),
 ('docs/top level map.txt','full','','Docs-vs-code discrepancies'),
 ('docs/changes.txt','skimmed','language and preferences entries only','Docs-vs-code discrepancies'),
 ('tools/multiwinia_exclusions.txt','full','','2.1; 2.19; 8.1; 9.1'),
 ('tools/full_exclusions.txt','full','','10.'),
 ('tools/build_multiwinia_data.bat','skimmed','exclusion selection only','2.1'),
 ('targets/vs2005/Darwinia.vcproj','skimmed','compile membership of parser sources','1.1'),
]
for r in src: add(*r)
# worldobject Read implementations
for p in ['ai','aiobjective','anthill','blueprintstore','bridge','chess','clonelab','controlstation','controltower',
          'factory','feedingtube','generator','generichub','gunturret','incubator','jumppad','laserfence','mine',
          'multiwiniazone','powerstation','pulsebomb','radardish','researchitem','restrictionzone','rocket',
          'safearea','scripttrigger','spawnpoint','spiritreceiver','staticshape','switch','tree','triffid',
          'trunkport','wall']:
    add(f'code/worldobject/{p}.cpp', 'skimmed', 'Read(TextReader*) and constructor defaults only', '2.9.3 Per-type extra columns')
with open(out, 'w', newline='') as f:
    w = csv.writer(f); w.writerow(['path', 'depth', 'reason', 'sections']); w.writerows(rows)
print(len(rows))
