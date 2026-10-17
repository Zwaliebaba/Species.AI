#!/usr/bin/env python3
"""Assign every live source file (manifest vcproj_build != '' and != excluded-all) to exactly one analysis partition.
Usage: python3 -I partition.py <manifest.csv> <out_json> <out_md>"""
import csv, json, sys, fnmatch, collections
man, outj, outm = sys.argv[1:4]
C='code/'; W='code/worldobject/'; L='code/lib/'; I='code/interface/'; S4='contrib/systemiv/lib/'
def stems(prefix, names): return [prefix + n + '.*' for n in names.split()]
P = collections.OrderedDict()
P['P01_app_core'] = ('App core, main loop, location, team/unit', ['20_architecture','21_simulation_and_entity_model','06_player_control_and_input'],
    stems(C,'app main globals gametimer location location_input user_input team unit'))
P['P02_game_modes'] = ('Multiwinia game rules and mode objects', ['01_game_modes','03_buildings_and_world_objects','09_meta_systems','05_economy_and_population'],
    stems(C,'multiwinia achievement_tracker rocket_status_panel') + stems(W,'multiwiniazone statue flag rocket trunkport pulsebomb controlstation'))
P['P03_entity_core'] = ('Entity framework, Darwinian, officer, spirits, spatial & routing', ['02_units_and_entities','21_simulation_and_entity_model','05_economy_and_population'],
    stems(W,'entity entity_leg worldobject darwinian spirit officer') + stems(C,'routing_system obstruction_grid entity_grid entity_grid_cache'))
P['P04_entities_military'] = ('Military units, weapons, crate-delivered units', ['02_units_and_entities','04_crates_powerups_and_weapons'],
    stems(W,'armour insertion_squad lasertrooper tank harvester weapons shaman airstrike nuke engineer') + stems(C,'shaman_interface'))
P['P05_entities_creatures'] = ('Creatures, hazards, legacy entities', ['02_units_and_entities','04_crates_powerups_and_weapons'],
    stems(W,'armyant anthill centipede spider virii souldestroyer tripod triffid tentacle egg sporegenerator dustball lander dropship spaceship meteor lightning snow spam wallbuster'))
P['P06_buildings_core'] = ('Core buildings: base class, spawn economy, turrets, transport', ['03_buildings_and_world_objects','05_economy_and_population'],
    stems(W,'building spawnpoint generator powerstation controltower gunturret radardish portal teleport jumppad carryablebuilding constructionyard spiritreceiver spiritstore incubator'))
P['P07_buildings_misc'] = ('Misc and legacy buildings, scenery, fences', ['03_buildings_and_world_objects'],
    stems(W,'blueprintstore bridge cave chess clonelab factory feedingtube generichub goddish library mine randomiser researchitem restrictionzone safearea scripttrigger smokemarker staticshape switch tree upgrade_port wall laserfence eruptionmarker'))
P['P08_crates_ai'] = ('Crates/power-ups and CPU AI, objective markers', ['04_crates_powerups_and_weapons','07_ai'],
    stems(W,'crate ai aiobjective') + stems(C,'markersystem'))
P['P09_control_taskmanager'] = ('Task manager, unit control UI, gestures, game cursor', ['06_player_control_and_input'],
    stems(C,'taskmanager taskmanager_interface taskmanager_interface_gestures taskmanager_interface_icons taskmanager_interface_multiwinia gesture gesture_demo gesture_demo_win32 gamecursor'))
P['P10_input_camera'] = ('Input driver/filter graph, camera, control help', ['06_player_control_and_input'],
    ['code/lib/input/*'] + stems(C,'camera control_help'))
P['P11_terrain_world'] = ('Landscape, water, clouds, level file, global world', ['23_terrain_and_landscape','08_levels_and_level_design','24_rendering'],
    stems(C,'landscape landscape_renderer water water_reflection clouds level_file mapfile mapcrc global_world global_internet'))
P['P12_render_core'] = ('Renderer, effects, shapes, textures, text, resources', ['24_rendering','25_textures_and_assets','26_animation_and_vfx'],
    stems(C,'renderer particle_system explosion deform effect_processor 3d_sierpinski_gasket') +
    stems(L,'shape 3d_sprite sphere_renderer render_utils debug_render persisting_debug_render text_renderer bitmap texture texture_uv resource shader FixedPipeline targetcursor poster_maker'))
P['P13_gfx_backend_math'] = ('GL/D3D9 backend, window managers, video, math', ['24_rendering','31_platform_build_and_tools','22_determinism_and_networking'],
    stems(L,'opengl_directx opengl_directx_dlist opengl_directx_dlist_dev opengl_directx_inline opengl_directx_internals opengl_directx_matrix_stack opengl_directx_stubs ogl_extensions ogl_extensions_directx opengl_trace safegl window_manager window_manager_directx window_manager_win32 avi_generator avi_player math_utils matrix33 matrix34 vector2 vector3 plane invert_matrix rgb_colour'))
P['P14_platform_lib'] = ('Platform/lib utilities, SystemIV core libs, targets', ['31_platform_build_and_tools','32_debug_test_and_performance','20_architecture','29_data_formats_reference'],
    stems(L,'hi_res_time preferences language_table exception_handler GetWinVer MiniVersion system_info user_info work_queue runnable auto_vector 2d_array 2d_surface_map presized_array slice_darray universal_include') +
    ['code/unicode/*'] + stems(C,'keygen') +
    [S4+'filesys/*', S4+'math/*', S4+'profiler.*', S4+'string_utils.*', S4+'tosser/*', S4+'unicode/*', S4+'debug_utils.*', S4+'memory_leak.*', 'targets/*'])
P['P15_network'] = ('Networking, metaserver, netlib', ['22_determinism_and_networking','09_meta_systems'],
    ['code/network/*', S4+'metaserver/*', S4+'netlib/*', S4+'network_stream.*'])
P['P16_sound'] = ('Sound engine and sound tooling windows', ['10_audio_design','27_audio_engine'],
    ['code/sound/*'] + stems(I,'soundeditor_window soundparameter_window sound_profile_window soundstats_window prefs_sound_window'))
P['P17a_ui_framework_editor'] = ('Window framework, eclipse, editor and debug windows', ['28_ui_framework','08_levels_and_level_design','32_debug_test_and_performance'],
    ['contrib/eclipse/*'] + stems(I,'darwinia_window input_field scrollbar checkbox drop_down_menu message_dialog filedialog multiwinia_filedialog multiwinia_window editor_window buildings_window camera_anim_window camera_mount_window landscape_window lights_window globalworldeditor_window instant_unit_window tree_window invalidcrates_window cheat_window debugmenu network_window gesture_window grabber_window') + stems(C,'location_editor'))
P['P17b_menus_options'] = ('Menus, options, profiles, leaderboards, in-game menu', ['09_meta_systems','28_ui_framework'],
    stems(I,'mainmenus helpandoptions_windows leaderboards_window multiwiniaoptions_window prefs_graphics_window prefs_keybindings_window prefs_other_window prefs_screen_window profilewindow userprofile_window mods_window chatinput_window controllerunplugged_window demoend_window reallyquit_window save_on_quit_window') + stems(C,'game_menu'))
P['P18_ui_gamemenu'] = ('Front-end GameMenuWindow and all UI buttons', ['09_meta_systems','28_ui_framework'],
    ['code/UI/*'])
P['P19_help_tutorial_script'] = ('Help system, tutorials, script engine, Sepulveda', ['30_scripting_tutorial_and_cutscenes','09_meta_systems'],
    stems(C,'multiwiniahelp helpsystem tutorial tutorial_demo1 tutorial_demo2 script sepulveda sepulveda_strings'))
P['P20_sequences_loaders_bench'] = ('Start/end sequences, attract, loaders, benchmark, test harness', ['30_scripting_tutorial_and_cutscenes','09_meta_systems','32_debug_test_and_performance'],
    ['code/loaders/*'] + stems(C,'loading_screen attract benchmark testharness startsequence demoendsequence animatedpanel animatedpanel_renderer'))

rows = list(csv.DictReader(open(man)))
live = [r for r in rows if r['type'].startswith('source/') and r['vcproj_build'] and r['vcproj_build'] != 'Debug;Release;DebugDirect3D;ReleaseDirect3D']
assign = collections.defaultdict(list); unassigned=[]; multi=[]
for r in live:
    hits = [k for k,(_,_,pats) in P.items() if any(fnmatch.fnmatchcase(r['path'], p) for p in pats)]
    if not hits: unassigned.append(r['path'])
    elif len(hits) > 1: multi.append((r['path'], hits))
    else: assign[hits[0]].append(r)
out = {}
for k,(title,docs,_) in P.items():
    fs = sorted(assign[k], key=lambda r: r['path'])
    out[k] = {'title': title, 'target_docs': docs, 'files': [r['path'] for r in fs], 'lines': sum(int(r['lines']) for r in fs)}
json.dump(out, open(outj,'w'), indent=1)
with open(outm,'w') as fh:
    fh.write('# Partition plan (Phase 1.3)\n\nGenerated by `_work/scripts/partition.py` from `manifest.csv`. Every live source file (in `Darwinia.vcproj`/`eclipse.vcproj`, not excluded in all configs) is assigned to exactly one partition; the script fails loudly otherwise.\n\n')
    fh.write('| Partition | Subsystem | Files | Lines | Primary target documents |\n|---|---|---|---|---|\n')
    for k,v in out.items(): fh.write(f"| {k} | {v['title']} | {len(v['files'])} | {v['lines']} | {', '.join(v['target_docs'])} |\n")
    fh.write(f"| **Total** | | {sum(len(v['files']) for v in out.values())} | {sum(v['lines'] for v in out.values())} | |\n\n")
    fh.write('## Cross-cutting agents (Phase 2, mandatory)\n\n| Agent | Scope | Target documents |\n|---|---|---|\n')
    fh.write('| X1a_text_formats | Level files (`data/levels/*`), `game*.txt`, `multiwinia*.txt`, `locations.txt`, `default_preferences.txt`, `data/input/*`, `data/language/*`, `data/scripts/*`, `data/mwhelp/*.txt`, `multiwinia-testbed.txt`, `betatesters*.txt`, `testbedconfig.txt`, `MultiwiniaSupport.ini` — grammar verified against parsers | 29, 08, 06, 09, 30 |\n')
    fh.write('| X1b_asset_formats | `.shp`, `.bmp`/texture loading, `.anim`, `gestures.txt`/`gesture_demos`, `effects.txt`, `stats.txt`, `sounds.txt`, `.dat`/RAR archives, terrain colour ramps, thumbnails, `data/purity`, `data/sepulveda` | 29, 25, 26, 10 |\n')
    fh.write('| X2a_asset_catalogue_visual | `data/textures`, `sprites`, `shapes`, `animations`, `terrain`, `icons`, `thumbnails`, `mwhelp` images, `shaders`, fonts — dims/format/alpha, load sites, consumers, referenced-or-unused | 25 |\n')
    fh.write('| X2b_asset_catalogue_audio | `data/sounds`, `data/mwsounds`, `sounds.txt`, `tools/sounds.txt`, `tools/menu_sounds.txt` — every sample, its sound events and code sites | 10, 25 |\n')
    fh.write('| X3_determinism_sync | All simulation code: RNG sources, float usage, update order, timestep, syncdiff, simulated vs cosmetic state | 22, 21 |\n')
    fh.write('| X4_level_catalogue | Every shipped map/mission (`data/levels`), thumbnails, `PERMITTED_MAPS`/map IDs, modes and player counts, per-level options | 08, 01 |\n')
    if unassigned: fh.write('\n## UNASSIGNED\n' + '\n'.join(unassigned) + '\n')
    if multi: fh.write('\n## MULTIPLY ASSIGNED\n' + '\n'.join(f'{p}: {h}' for p,h in multi) + '\n')
print('live files', len(live), 'unassigned', len(unassigned), 'multi', len(multi))
for p in unassigned: print('  UNASSIGNED', p)
for p,h in multi: print('  MULTI', p, h)
for k,v in out.items(): print(f"{k:28s} {len(v['files']):4d} {v['lines']:6d}")
