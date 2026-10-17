#!/usr/bin/env python3
"""X-AST pass 1: build the Species asset set from the recovery CSVs.

Usage:
  python3 -I X-AST_build.py --drafts <ORIG/_work/drafts> --manifest <NEXT/assets/asset_manifest.csv>
                            --origin <WORK/asset_origin_map.csv> --json <scratch/xast.json>

Reads only the CSVs named in the X-AST brief.  Every in-scope original file is assigned to
exactly one Species file; every out-of-scope original file is assigned an exclusion class.
This script names original paths on purpose: its outputs that hold original names
(the origin map and the JSON) stay outside the design set.
"""
import argparse
import collections
import csv
import json
import re
import sys

# ----------------------------------------------------------------------------------------
# Document-level consumers (pass 2 replaces them with requirement IDs)
# ----------------------------------------------------------------------------------------
DOCS = {
    'overview': 'reference/00_reference_overview.md',
    'modes': 'reference/01_skirmish_rules_and_modes/README.md',
    'm_spawn_control': 'reference/01_skirmish_rules_and_modes/spawn_control.md',
    'm_zone_control': 'reference/01_skirmish_rules_and_modes/zone_control.md',
    'm_relic_run': 'reference/01_skirmish_rules_and_modes/relic_run.md',
    'm_siege': 'reference/01_skirmish_rules_and_modes/siege.md',
    'm_launch_race': 'reference/01_skirmish_rules_and_modes/launch_race.md',
    'm_flag_chain': 'reference/01_skirmish_rules_and_modes/flag_chain.md',
    'worker': 'reference/02_units/worker.md',
    'leader': 'reference/02_units/leader_and_formations.md',
    'builder': 'reference/02_units/builder.md',
    'collector': 'reference/02_units/collector.md',
    'essence': 'reference/02_units/essence_orbs.md',
    'strike': 'reference/02_units/strike_team.md',
    'transport': 'reference/02_units/transport.md',
    'creatures': 'reference/02_units/creatures_and_hazards.md',
    'econ': 'reference/03_buildings_and_world_objects/economy_and_spawn_objects.md',
    'modeobj': 'reference/03_buildings_and_world_objects/mode_objects.md',
    'defences': 'reference/03_buildings_and_world_objects/defences_and_props.md',
    'crates': 'reference/04_supply_crates_powers_and_weapons/crates_and_powers.md',
    'weapons': 'reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md',
    'controls': 'reference/06_controls_camera_and_commands.md',
    'maps': 'reference/08_maps_and_level_design/rules_and_conventions.md',
    'frontend': 'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md',
    'options': 'reference/09_front_end_and_match_flow/options_localisation_and_hooks.md',
    'audio': 'reference/10_audio_design.md',
    'help': 'reference/11_tutorial_and_help.md',
    'r_visual': 'architecture/components/renderer/visual_language.md',
    'r_world': 'architecture/components/renderer/world_renderers.md',
    'r_entity': 'architecture/components/renderer/entity_and_overlay_renderers.md',
    'vfx': 'architecture/components/animation_and_vfx.md',
    'terrain': 'architecture/components/terrain.md',
    'ui_pages': 'architecture/components/ui_framework/pages_and_screen_recipes.md',
    'ui_text': 'architecture/components/ui_framework/layout_rendering_input_and_localisation.md',
    'input': 'architecture/components/input.md',
    'aen': 'architecture/components/audio_engine.md',
    'ast': 'architecture/components/asset_pipeline_and_resources.md',
}

PRIO = {'fp': 'skirmish-first-playable', 'launch': 'launch', 'post': 'post-launch'}

MODES = ['spawn_control', 'zone_control', 'relic_run', 'siege', 'launch_race', 'flag_chain']
MODE_NAME = {'spawn_control': 'Spawn Control', 'zone_control': 'Zone Control', 'relic_run': 'Relic Run',
             'siege': 'Siege', 'launch_race': 'Launch Race', 'flag_chain': 'Flag Chain'}
# reference mode words -> Species mode stem (data only)
ORIG_MODE = {'Domination': 'spawn_control', 'KingOfTheHill': 'zone_control', 'CaptureTheStatue': 'relic_run',
             'Assault': 'siege', 'RocketRiot': 'launch_race', 'Blitzkrieg': 'flag_chain'}

# ----------------------------------------------------------------------------------------
# Texture profiles: source format, runtime format, colour space, alpha mode, placeholder kind
# ----------------------------------------------------------------------------------------
PROFILES = {
    'key': ('PNG RGBA8, sRGB; cut-out alpha painted by the artist',
            'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain', 'sRGB',
            'cut-out (alpha test at the consumer threshold)', 'sprite'),
    'alpha': ('PNG RGBA8, sRGB; straight alpha',
              'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain', 'sRGB',
              'straight alpha blend', 'sprite'),
    'add': ('PNG 8-bit greyscale, linear intensity on black',
            'DDS (DX10 header) BC4_UNORM, full mip chain', 'linear',
            'additive (black adds nothing; colour from the draw tint)', 'gradient'),
    'mask': ('PNG 8-bit greyscale mask, white = opaque',
             'DDS (DX10 header) BC4_UNORM, full mip chain', 'linear',
             'alpha mask (white colour, mask as alpha, tinted at draw)', 'gradient'),
    'dark': ('PNG 8-bit greyscale, white = full darkening',
             'DDS (DX10 header) BC4_UNORM, full mip chain', 'linear',
             'darkening layer (destination x (1 - texel))', 'gradient'),
    'icon': ('PNG RGBA8, sRGB; white or grey art with straight alpha (tinted at draw)',
             'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain', 'sRGB',
             'straight alpha, tinted by vertex colour', 'glyph'),
    'team': ('PNG RGBA8, sRGB; team area painted pure red (255,0,0)',
             'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain; team area recoloured per team at load', 'sRGB',
             'straight alpha; pure-red texels take the team colour', 'glyph'),
    'ui': ('PNG RGB8, sRGB; opaque', 'DDS (DX10 header) BC7_UNORM_SRGB, no mips', 'sRGB',
           'opaque', 'panel'),
    'uia': ('PNG RGBA8, sRGB; straight alpha', 'DDS (DX10 header) BC7_UNORM_SRGB, no mips', 'sRGB',
            'straight alpha blend', 'panel'),
    'uitile': ('PNG RGB8, sRGB; tileable along its long axis',
               'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain, wrap repeat', 'sRGB',
               'opaque fill (blend chosen by the widget)', 'panel'),
    'ramp': ('PNG RGB8, sRGB; look-up table, no filtering artefacts at the edges',
             'uncompressed R8G8B8A8_UNORM_SRGB table in the pack, no mips; read on the CPU', 'sRGB',
             'none (look-up table)', 'ramp'),
    'tile': ('PNG RGB8, sRGB; seamless tile', 'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain, wrap repeat',
             'sRGB', 'opaque (modulated by the sea light map)', 'noise'),
    'thumb': ('PNG RGB8, sRGB; rendered by the map tool', 'DDS (DX10 header) BC7_UNORM_SRGB, no mips', 'sRGB',
              'opaque', 'thumb'),
}

PLACEHOLDER = {
    'sprite': 'generated flat-colour figure or shape filling the role silhouette, with an alpha outline',
    'gradient': 'generated white-on-black radial or linear gradient of the stated size',
    'glyph': 'generated white geometric glyph (circle, arrow or ring) with the asset id initials',
    'panel': 'generated flat panel of the stated size with the asset id printed on it',
    'ramp': 'generated two-axis colour gradient (slope x height)',
    'noise': 'generated seamless value-noise tile',
    'thumb': 'auto-rendered top-down capture of the placeholder map',
    'mesh': 'primitive-built mesh (box, cylinder or cone) at the role bounding size, with every socket present',
    'audio': 'synthesised per sample group: sine or noise burst of the stated duration, looping where marked',
    'voice': 'text-to-speech line of the stated duration',
    'silence': 'silence of the stated length, looping where marked',
    'font': 'open-licence fallback font with the same script coverage',
    'strings': 'pseudo-localised text generated from each string id',
    'map': 'generated flat test map with the objects the mode requires for its player count',
    'explainer': 'static card that shows the mode name and the explainer id',
    'help': 'generated diagram card with the asset id and page number',
}

BRIEF = {
    'sprites': 'art_and_audio_briefs.md#3-unit-sprites',
    'vfx': 'art_and_audio_briefs.md#4-vfx-textures',
    'ui': 'art_and_audio_briefs.md#5-ui-panels-title-and-logo',
    'hud': 'art_and_audio_briefs.md#6-hud-supply-crate-and-power-icons',
    'cursors': 'art_and_audio_briefs.md#7-cursors-and-markers',
    'glyphs': 'art_and_audio_briefs.md#8-input-glyphs',
    'terrain': 'art_and_audio_briefs.md#9-terrain-ramps-water-and-waves',
    'meshes': 'art_and_audio_briefs.md#10-meshes',
    'help': 'art_and_audio_briefs.md#11-help-images-and-explainer-animations',
    'sfx': 'art_and_audio_briefs.md#12-sound-effects-by-source-and-event',
    'ui_audio': 'art_and_audio_briefs.md#13-ui-sounds',
    'ambience': 'art_and_audio_briefs.md#14-ambience',
    'music': 'art_and_audio_briefs.md#15-music',
    'fonts': 'art_and_audio_briefs.md#16-fonts',
    'strings': 'art_and_audio_briefs.md#17-player-facing-text',
    'maps': 'art_and_audio_briefs.md#18-maps-and-thumbnails',
}

CATEGORY_TITLE = {
    'sprites': 'Unit sprites', 'vfx': 'VFX textures', 'ui': 'UI panels and title',
    'hud': 'HUD, supply-crate and power icons', 'cursors': 'Cursors and markers', 'glyphs': 'Input glyphs',
    'terrain': 'Terrain ramps, water and waves', 'meshes': 'Meshes', 'help': 'Help images and explainer animations',
    'sfx': 'Audio by source and event', 'ui_audio': 'UI sounds', 'ambience': 'Ambience', 'music': 'Music',
    'fonts': 'Fonts', 'strings': 'String table', 'maps': 'Maps and thumbnails',
}

# ----------------------------------------------------------------------------------------
# Texture role table.
# orig (relative to the reference data root) | category | species path (relative to content/, no ext)
# | asset id | role | consumer keys | priority | profile | spec extra
# Rows that share an asset id form one group (numbered variants).
# ----------------------------------------------------------------------------------------
TEX_TABLE = r"""
sprites/darwinian.bmp|sprites|sprites/worker_front|sprite.worker.front|Worker card, front view: the crowd figure; also drawn large by the loading view, the Launch Race status panel and the front-end backdrop|worker,r_entity,frontend|fp|key|white or light grey body for team tint; alpha-test threshold 0.04; magnified with NEAREST on the front-end backdrop
sprites/darwinian_ouch1.bmp|sprites|sprites/worker_hurt_01|sprite.worker.hurt|Worker hurt pose; variants 01-03 are chosen by unit id while a Worker is airborne or dying, variant 04 is the fixed hurt pose|worker,r_entity|fp|key|same canvas and pivot as the front card; alpha-test threshold 0.04
sprites/darwinian_ouch2.bmp|sprites|sprites/worker_hurt_02|sprite.worker.hurt||worker,r_entity|fp|key|
sprites/darwinian_ouch3.bmp|sprites|sprites/worker_hurt_03|sprite.worker.hurt||worker,r_entity|fp|key|
sprites/darwinian_ouch4.bmp|sprites|sprites/worker_hurt_04|sprite.worker.hurt||worker,r_entity|fp|key|
sprites/darwinian_side.bmp|sprites|sprites/worker_side|sprite.worker.side|Worker card seen edge-on (side view), used when the card turns away from the camera|worker,r_entity|fp|key|alpha-test threshold 0.04
sprites/brainwaves.bmp|sprites|sprites/worker_state_overlay|sprite.worker.state_overlay|Overlay sprite drawn above a Worker in an altered control state|worker,r_entity|launch|key|drawn over the Worker card; role to be confirmed against reference/02_units/worker.md
sprites/futurewinian.bmp|sprites|sprites/visitor_front|sprite.visitor.front|Visitor faction figure card|worker,crates,r_entity|launch|alpha|alpha-test threshold 0.02
sprites/santahat.bmp|sprites|sprites/worker_seasonal_hat|sprite.worker.seasonal_hat|Seasonal-theme hat drawn 0.95 wu above every third Worker|worker,r_entity|post|key|small accessory card; drawn only while the seasonal theme is on
sprites/virii.bmp|sprites|sprites/crawler_segment|sprite.crawler.segment|Crawler body card; the renderer builds each Crawler's trail strip from it|creatures,r_entity|launch|add|hostile (Rogue faction) Crawlers draw it as a darkening layer
sprites/egg.bmp|sprites|sprites/crawler_egg|sprite.crawler_egg.body|Crawler egg card|creatures,r_entity|launch|alpha|
sprites/ghost.bmp|sprites|sprites/wisp|sprite.wisp.body|Wisp card|creatures,r_entity|launch|add|
sprites/nuke.bmp|sprites|sprites/barrage_missile|sprite.missile_barrage.missile|Missile Barrage missile card (new presentation of the mechanic)|crates,weapons,r_entity|launch|alpha|
sprites/sub.bmp|sprites|sprites/barrage_launcher|sprite.missile_barrage.launcher|Missile Barrage launcher card that surfaces before the volley (new presentation)|crates,weapons,r_entity|launch|alpha|
sprites/subshadow.bmp|sprites|sprites/barrage_launcher_shadow|sprite.missile_barrage.launcher_shadow|Shadow under the Missile Barrage launcher|crates,weapons,r_entity|launch|dark|
textures/bubble.bmp|vfx|textures/fx/fx_bubble_sphere|fx.bubble_sphere|Translucent bubble around a falling Supply crate, a shielded Worker and a Collector|crates,worker,collector,vfx|fp|add|
textures/clouds.bmp|vfx|textures/fx/fx_sky_cloud|fx.sky_cloud|Sky cloud layer (also the front-end background clouds)|r_world,frontend|fp|add|blobby layer MAG LINEAR / MIN NEAREST, flat layer NEAREST; wrap repeat; small tile magnified on purpose
textures/cloudyglow.bmp|vfx|textures/fx/fx_glow_cloudy|fx.glow_cloudy|Soft cloudy glow: front-end fill boxes, Supply crate glow, dust vortex, Spawn Node glow|crates,econ,vfx,frontend|fp|add|wrap repeat in front-end fill boxes
textures/fuel.bmp|vfx|textures/fx/fx_fuel_flow|fx.fuel_flow|Scrolling fuel-flow strip on fuel lines|modeobj,m_launch_race,vfx|launch|add|wrap repeat, scrolled along the line
textures/glow.bmp|vfx|textures/fx/fx_glow_round|fx.glow_round|Round soft glow: Worker glow, blob shadows, lights, Essence orbs carried by Leaders, Core Device, Solar Array|worker,essence,r_entity,vfx|fp|add|also drawn as a darkening blob shadow
textures/laser-long.bmp|vfx|textures/fx/fx_beam_long|fx.beam_long|Long beam strip: Launch Race status panel, Control Zone edge, Core Device pulse wave|modeobj,m_zone_control,m_siege,vfx|launch|add|
textures/laser.bmp|vfx|textures/fx/fx_beam_short|fx.beam_short|Beam strip: laser bolts, the sky grid lines and front-end decoration|weapons,r_world,vfx|fp|add|wrap repeat for the sky grid; drawn without mips by the sky and laser-bolt passes
textures/laserfence.bmp|vfx|textures/fx/fx_barrier_field|fx.barrier_field|Barrier Emitter field and the base layer of the Beam Dish signal|defences,vfx|fp|add|wrap repeat
textures/laserfence2.bmp|vfx|textures/fx/fx_barrier_field_wide|fx.barrier_field_wide|Wide Barrier Emitter field strip|defences,vfx|fp|add|wrap repeat
textures/lightning.bmp|vfx|textures/fx/fx_lightning_strip|fx.lightning_strip|Lightning-strike strip (map effect)|maps,vfx|launch|add|
textures/muzzleflash.bmp|vfx|textures/fx/fx_muzzle_flash|fx.muzzle_flash|Muzzle flash for turrets and units; also the Launch Race status panel|weapons,vfx|fp|add|
textures/particle.bmp|vfx|textures/fx/fx_particle_dot|fx.particle_dot|Particle dot: particle pool, cursor formation preview, off-screen arrows, route lines|vfx,controls|fp|add|MAG NEAREST (crisp pixels) in particle batches
textures/radarsignal.bmp|vfx|textures/fx/fx_dish_signal|fx.dish_signal|Beam Dish signal beam, combined with the barrier field texture|econ,vfx|launch|add|wrap repeat; two-texture beam
textures/shapewireframe.bmp|vfx|textures/fx/fx_shard_edges|fx.shard_edges|Edge-line pattern decaled on shatter shards|vfx|fp|key|decal over the shard colour where alpha is set
textures/starburst.bmp|vfx|textures/fx/fx_starburst|fx.starburst|Star flare on structure lights and manning ports|econ,defences,vfx|fp|add|
textures/subversion.bmp|vfx|textures/fx/fx_convert_beam|fx.convert_beam|Convert beam strip|crates,weapons,vfx|launch|add|
textures/tree.bmp|vfx|textures/fx/fx_tree_canopy|fx.tree_canopy|Tree canopy mask, tinted per tree|defences,r_world|fp|mask|
textures/triangleoutline.bmp|vfx|textures/fx/fx_terrain_grid|fx.terrain_grid|Triangle wire-grid overlay drawn once per terrain cell over the terrain and the sea|terrain,r_world,r_visual|fp|add|wrap repeat, one repeat per terrain cell
textures/darwinian.bmp|ui|textures/ui/ui_worker_badge|ui.worker_badge|Worker badge on map-select buttons|frontend,ui_pages|launch|uia|
textures/interface_blur.bmp|ui|textures/ui/ui_fill_blur|ui.fill_blur|Blurred fill for front-end boxes|frontend,ui_pages|fp|uitile|
textures/interface_grey.bmp|ui|textures/ui/ui_panel_neutral|ui.panel_neutral|Neutral panel strip: power-slot panel, Core Device countdown, fuel depot read-out|crates,ui_pages,modeobj|fp|uitile|
textures/interface_red.bmp|ui|textures/ui/ui_panel_accent|ui.panel_accent|Accent panel strip for windows and the power-slot panel (new palette, same token role)|ui_pages,crates|fp|uitile|
textures/ivlogo.bmp|ui|textures/ui/ui_studio_splash|ui.studio_splash|Studio mark on the loading view (placeholder until the studio mark exists)|frontend|launch|ui|
textures/menuhighlight.bmp|ui|textures/ui/ui_menu_highlight|ui.menu_highlight|Highlight beam behind the focused menu item|frontend,ui_pages|fp|add|wrap repeat
textures/multiwinia.bmp|ui|textures/ui/ui_logo_banner|ui.logo_banner|Large title logo on the main menu|frontend|launch|ui|
textures/mwtitle.bmp|ui|textures/ui/ui_title_wordmark|ui.title_wordmark|Title wordmark (new logo)|frontend,ui_pages|fp|uia|
icons/mwlogo.bmp|ui|textures/ui/ui_logo_emblem|ui.logo_emblem|Logo emblem on the main menu, drop-down menus and the options screen|frontend,ui_pages|launch|uia|wrap repeat where tiled
icons/locked.bmp|ui|textures/ui/ui_icon_locked|ui.icon_locked|Lock icon on password-protected match entries|frontend|launch|uia|
icons/tick.bmp|ui|textures/ui/ui_icon_check|ui.icon_check|Check-box tick|ui_pages,options|fp|uia|
icons/menu_basic.bmp|ui|textures/ui/ui_difficulty_basic|ui.difficulty_badge|Map difficulty badge (basic, intermediate, advanced) on mode and map buttons|frontend|launch|uia|
icons/menu_intermediate.bmp|ui|textures/ui/ui_difficulty_intermediate|ui.difficulty_badge||frontend|launch|uia|
icons/menu_advanced.bmp|ui|textures/ui/ui_difficulty_advanced|ui.difficulty_badge||frontend|launch|uia|
mwhelp/gear.bmp|ui|textures/ui/ui_backdrop_wheel|ui.backdrop_wheel|Large decorative wheel behind the front-end backdrop figure (new presentation)|frontend|launch|ui|
icons/icon_darwinian.bmp|hud|textures/icons/power/power_workers|icon.power.workers|Workers icon: selected-Workers cursor badge, power slot, Reinforcements outcome|crates,worker,controls|fp|icon|
icons/icon_officer.bmp|hud|textures/icons/power/power_leader|icon.power.leader|Leader icon: selected-unit cursor badge and the promotion power|crates,leader,controls|fp|icon|
icons/icon_squad.bmp|hud|textures/icons/power/power_strike_team|icon.power.strike_team|Strike team power|crates,strike|fp|icon|
icons/icon_grenade.bmp|hud|textures/icons/power/power_grenade|icon.power.grenade|Strike team grenade weapon|crates,strike,weapons|fp|icon|
icons/icon_rocket.bmp|hud|textures/icons/power/power_rocket|icon.power.rocket|Strike team rocket weapon|crates,strike,weapons|fp|icon|
icons/icon_laser.bmp|hud|textures/icons/power/power_laser|icon.power.laser|Laser upgrade power level|crates,weapons|post|icon|role to be confirmed against reference/04
icons/icon_airstrike.bmp|hud|textures/icons/power/power_airstrike|icon.power.airstrike|airstrike outcome and power|crates|fp|icon|
icons/icon_armour.bmp|hud|textures/icons/power/power_transport|icon.power.transport|Transport outcome and power|crates,transport|fp|icon|
icons/icon_engineer.bmp|hud|textures/icons/power/power_builder|icon.power.builder|Builder outcome and power; also drawn over the Builder|crates,builder|launch|icon|
icons/icon_harvester.bmp|hud|textures/icons/power/power_collector|icon.power.collector|Collector outcome and power|crates,collector|launch|icon|
icons/icon_nuke.bmp|hud|textures/icons/power/power_missile_barrage|icon.power.missile_barrage|Missile Barrage outcome and power|crates|launch|icon|
icons/icon_subversion.bmp|hud|textures/icons/power/power_convert|icon.power.convert|Convert outcome and power|crates|launch|icon|
icons/icon_personalshield.bmp|hud|textures/icons/power/power_shield|icon.power.shield|Shield outcome and power|crates|fp|icon|
icons/icon_hotfeet.bmp|hud|textures/icons/power/power_speed_boost|icon.power.speed_boost|Speed Boost outcome and power|crates|fp|icon|
icons/icon_gunturret.bmp|hud|textures/icons/power/power_gun_turret|icon.power.gun_turret|gun turret outcome and power|crates,defences|fp|icon|
icons/icon_infection.bmp|hud|textures/icons/power/power_swarm|icon.power.swarm|Swarm outcome|crates|launch|icon|
icons/icon_magicalforest.bmp|hud|textures/icons/power/power_essence_grove|icon.power.essence_grove|Essence Grove outcome|crates|launch|icon|
icons/icon_darkforest.bmp|hud|textures/icons/power/power_blight_grove|icon.power.blight_grove|Blight Grove outcome|crates|launch|icon|
icons/icon_antnest.bmp|hud|textures/icons/power/power_hive|icon.power.hive|Hive outcome|crates|launch|icon|
icons/icon_plague.bmp|hud|textures/icons/power/power_sickness|icon.power.sickness|Sickness outcome|crates|launch|icon|
icons/icon_egg.bmp|hud|textures/icons/power/power_brood|icon.power.brood|Brood outcome|crates|launch|icon|
icons/icon_meteorshower.bmp|hud|textures/icons/power/power_meteor_shower|icon.power.meteor_shower|meteor shower outcome|crates|launch|icon|
icons/icon_rage.bmp|hud|textures/icons/power/power_fury|icon.power.fury|Fury outcome|crates|fp|icon|
icons/icon_randomizer.bmp|hud|textures/icons/power/power_allegiance_scrambler|icon.power.allegiance_scrambler|Allegiance Scrambler outcome (shown on the crate)|crates|launch|icon|
icons/icon_triffid.bmp|hud|textures/icons/power/power_spitter_plants|icon.power.spitter_plants|Spitter Plants outcome (shown on the crate)|crates|launch|icon|
icons/icon_taskmanager.bmp|hud|textures/icons/power/power_slots_upgrade|icon.power.slots_upgrade|Power-slots power level|crates|post|icon|role to be confirmed against reference/04
icons/icon_notask.bmp|hud|textures/icons/hud/hud_slot_empty|icon.hud.slot_empty|Empty power slot|crates,ui_pages|fp|icon|
icons/icon_delete.bmp|hud|textures/icons/hud/hud_slot_discard|icon.hud.slot_discard|Discard-power button on a power slot|crates,ui_pages|fp|icon|
icons/icon_shadow.bmp|hud|textures/icons/hud/hud_icon_halo|icon.hud.icon_halo|Darkening halo behind HUD icons|crates,ui_pages,r_visual|fp|dark|
icons/arrows_load.bmp|hud|textures/icons/hud/hud_transport_boarding|icon.hud.transport_state|Transport state read-out on its power slot: boarding, travelling, unloading|transport,crates|fp|icon|
icons/arrows_moving.bmp|hud|textures/icons/hud/hud_transport_travelling|icon.hud.transport_state||transport,crates|fp|icon|
icons/arrows_unload.bmp|hud|textures/icons/hud/hud_transport_unloading|icon.hud.transport_state||transport,crates|fp|icon|
icons/rocketstatuspanel.bmp|hud|textures/icons/hud/hud_launch_panel|icon.hud.launch_panel|Launch Race status panel frame|m_launch_race,ui_pages|launch|uia|
icons/rocketcracked.bmp|hud|textures/icons/hud/hud_launch_panel_damaged|icon.hud.launch_panel_damaged|Launch Race status panel overlay for a damaged Launch Vessel|m_launch_race,ui_pages|launch|ui|
icons/mouse_game.bmp|cursors|textures/icons/cursors/cursor_world|cursor.world|Default world cursor|controls,ui_pages|fp|icon|blurred darkening shadow generated at run time
icons/mouse_highlight.bmp|cursors|textures/icons/cursors/cursor_highlight|cursor.highlight|Cursor over a selectable unit; also the power-slot screen-zone highlight|controls,crates|fp|icon|
icons/mouse_selection.bmp|cursors|textures/icons/cursors/cursor_select|cursor.select|Selection cursor; also the power-slot screen-zone highlight|controls,crates|fp|icon|
icons/mouse_movehere.bmp|cursors|textures/icons/cursors/cursor_move_target|cursor.move_target|Move-here cursor|controls|fp|icon|
icons/mouse_placement.bmp|cursors|textures/icons/cursors/cursor_place|cursor.place|Placement cursor for powers that place a structure|controls,crates|fp|icon|
icons/mouse_enter.bmp|cursors|textures/icons/cursors/cursor_enter|cursor.enter|Enter cursor over a Transport or a gate|controls|fp|icon|
icons/mouse_disabled.bmp|cursors|textures/icons/cursors/cursor_unavailable|cursor.unavailable|Unavailable cursor; also the kick-player button and invalid power targets|controls,frontend|fp|icon|
icons/mouse_turrettarget.bmp|cursors|textures/icons/cursors/cursor_turret_aim|cursor.turret_aim|Turret aiming cursor|controls,defences|fp|icon|
icons/selectionarrow.bmp|cursors|textures/icons/cursors/cursor_pointer|cursor.pointer|Pointer arrow for menus; also the scroll-bar arrow|controls,ui_pages|fp|icon|
icons/compass.bmp|cursors|textures/icons/markers/marker_edge_compass|marker.edge_compass|Off-screen compass marker and power-slot compass|controls,r_entity|fp|icon|
icons/compass_shadow.bmp|cursors|textures/icons/markers/marker_edge_compass_shadow|marker.edge_compass_shadow|Shadow for the off-screen compass marker|controls,r_entity|fp|mask|
icons/marker_colour.bmp|cursors|textures/icons/markers/marker_team_disc|marker.team_disc|Team-coloured disc under off-screen markers and power slots|controls,r_entity,crates|fp|mask|
icons/marker_shadow.bmp|cursors|textures/icons/markers/marker_team_disc_shadow|marker.team_disc_shadow|Shadow for the team disc|controls,r_entity|fp|mask|
icons/icon_crate.bmp|cursors|textures/icons/markers/marker_supply_crate|marker.supply_crate|Off-screen marker and recent-power badge for a Supply crate|crates,r_entity|fp|icon|
icons/icon_statue.bmp|cursors|textures/icons/markers/marker_relic|marker.relic|Off-screen marker for a Relic|m_relic_run,modeobj|launch|icon|
icons/target.bmp|cursors|textures/icons/markers/marker_crate_landing|marker.crate_landing|Ground marker where a Supply crate will land|crates|fp|icon|
icons/nukesymbol.bmp|cursors|textures/icons/markers/marker_barrage_target|marker.barrage_target|Ground symbol under a Missile Barrage target (new presentation)|crates,weapons|launch|icon|
icons/arrow.bmp|cursors|textures/icons/markers/marker_flag_pointer|marker.flag_pointer|Pointer drawn on a cloth flag|leader,r_entity|fp|alpha|
icons/thickarrow.bmp|cursors|textures/icons/markers/marker_jump_pad|marker.jump_pad|Direction arrow on a jump pad|defences|fp|alpha|wrap repeat, scrolled
icons/widearrow.bmp|cursors|textures/icons/markers/marker_order_trail|marker.order_trail|Order trail mark left by a Leader's order|leader,controls|fp|alpha|
icons/banner_none.bmp|cursors|textures/icons/markers/leader_banner_idle|marker.leader_banner|Leader banner per order state: idle, follow, formation, gather|leader|fp|team|cloth texture; team area pure red
icons/banner_follow.bmp|cursors|textures/icons/markers/leader_banner_follow|marker.leader_banner||leader|fp|team|
icons/banner_formation.bmp|cursors|textures/icons/markers/leader_banner_formation|marker.leader_banner||leader|fp|team|
icons/banner_absorb.bmp|cursors|textures/icons/markers/leader_banner_gather|marker.leader_banner||leader|fp|team|
icons/flag.bmp|cursors|textures/icons/markers/flag_post_neutral|marker.flag_post_state|Flag Chain flag per state: neutral, lowered, locked, raised|m_flag_chain,modeobj|launch|team|cloth texture; team area pure red
icons/flag_down.bmp|cursors|textures/icons/markers/flag_post_lowered|marker.flag_post_state||m_flag_chain,modeobj|launch|team|
icons/flag_locked.bmp|cursors|textures/icons/markers/flag_post_locked|marker.flag_post_state||m_flag_chain,modeobj|launch|team|
icons/flag_up.bmp|cursors|textures/icons/markers/flag_post_raised|marker.flag_post_state||m_flag_chain,modeobj|launch|team|
icons/button_a.bmp|glyphs|textures/icons/input/glyph_pad_face_south|glyph.pad.face_south|Gamepad face button, south position|input,controls,ui_pages|launch|icon|platform-neutral shape and label; no console trade dress
icons/button_b.bmp|glyphs|textures/icons/input/glyph_pad_face_east|glyph.pad.face_east|Gamepad face button, east position|input,controls,ui_pages|launch|icon|
icons/button_x.bmp|glyphs|textures/icons/input/glyph_pad_face_west|glyph.pad.face_west|Gamepad face button, west position|input,controls,ui_pages|launch|icon|
icons/button_y.bmp|glyphs|textures/icons/input/glyph_pad_face_north|glyph.pad.face_north|Gamepad face button, north position|input,controls,ui_pages|launch|icon|
icons/button_abxy_shadow.bmp|glyphs|textures/icons/input/glyph_pad_face_shadow|glyph.pad.face_shadow|Darkening shadow under face-button glyphs|input,ui_pages|launch|dark|
icons/button_lb.bmp|glyphs|textures/icons/input/glyph_pad_shoulder_left|glyph.pad.shoulder_left|Gamepad left shoulder button (also on the power-slot panel)|input,controls,ui_pages,crates|launch|icon|
icons/button_lb_shadow.bmp|glyphs|textures/icons/input/glyph_pad_shoulder_left_shadow|glyph.pad.shoulder_left_shadow|Darkening shadow under the left shoulder glyph|input,ui_pages|launch|dark|
icons/button_rb.bmp|glyphs|textures/icons/input/glyph_pad_shoulder_right|glyph.pad.shoulder_right|Gamepad right shoulder button|input,controls,ui_pages|launch|icon|
icons/button_rb_shadow.bmp|glyphs|textures/icons/input/glyph_pad_shoulder_right_shadow|glyph.pad.shoulder_right_shadow|Darkening shadow under the right shoulder glyph|input,ui_pages|launch|dark|
icons/button_lt.bmp|glyphs|textures/icons/input/glyph_pad_trigger_left|glyph.pad.trigger_left|Gamepad left trigger|input,ui_pages|launch|icon|
icons/button_rt.bmp|glyphs|textures/icons/input/glyph_pad_trigger_right|glyph.pad.trigger_right|Gamepad right trigger|input,ui_pages|launch|icon|
icons/button_trigger_shadow.bmp|glyphs|textures/icons/input/glyph_pad_trigger_shadow|glyph.pad.trigger_shadow|Darkening shadow under trigger glyphs|input,ui_pages|launch|dark|
icons/button_tab.bmp|glyphs|textures/icons/input/glyph_key_tab|glyph.key.tab|Keyboard Tab key on the power-slot panel|input,crates,ui_pages|fp|icon|
"""

# Terrain ramps, sea tiles, wave gradients: original stem -> index (01 is the map default)
RAMPS = ['default', 'dark', 'desert', 'earth', 'firetop', 'icecaps', 'jailbreak', 'launchpad', 'marble', 'mine',
         'shrine', 'silver', 'stronghold', 'trippy']
SEAS = ['default', 'icecaps', 'launchpad', 'yellow']
WAVES = ['default', 'containmentfield', 'dull', 'earth', 'generator', 'icecaps', 'launchpad', 'yellow']
SEASONAL = {'ramp': 'icecaps', 'sea': 'icecaps', 'wave': 'earth'}

# Help images
HINTS = [  # orig stem, species stem, role, consumers, mode
    ('starthere', 'hint_start_here', 'Hint: where the player starts', 'help', None),
    ('spawnpoint', 'hint_spawn_nodes', 'Hint: capturing Spawn Nodes', 'help,econ', None),
    ('reinforcements', 'hint_reinforcements', 'Hint: reinforcements', 'help,crates', None),
    ('crates', 'hint_supply_crates', 'Hint: Supply crates', 'help,crates', None),
    ('koth', 'hint_zone_control', 'Hint: holding Control Zones', 'help,m_zone_control', 'zone_control'),
    ('capturestatues', 'hint_relic_run_capture', 'Hint: bringing a Relic home', 'help,m_relic_run', 'relic_run'),
    ('liftstatues', 'hint_relic_run_lift', 'Hint: lifting a Relic', 'help,m_relic_run', 'relic_run'),
    ('pulsebomb', 'hint_siege_core_device', 'Hint: the Core Device', 'help,m_siege', 'siege'),
    ('pulsebomb_attack', 'hint_siege_attack', 'Hint: attacking in Siege', 'help,m_siege', 'siege'),
    ('pulsebomb_defend', 'hint_siege_defend', 'Hint: defending in Siege', 'help,m_siege', 'siege'),
    ('spawnpoint_assault', 'hint_siege_spawn_nodes', 'Hint: Spawn Nodes in Siege', 'help,m_siege', 'siege'),
    ('wallbuster', 'hint_siege_breach_charge', 'Hint: the Breach Charge', 'help,m_siege', 'siege'),
    ('rocketriot1', 'hint_launch_race_01', 'Hint: fuelling a Launch Vessel', 'help,m_launch_race', 'launch_race'),
    ('rocketriot2', 'hint_launch_race_02', 'Hint: launching', 'help,m_launch_race', 'launch_race'),
    ('blitzkrieg1', 'hint_flag_chain_01', 'Hint: the flag chain', 'help,m_flag_chain', 'flag_chain'),
    ('blitzkrieg2', 'hint_flag_chain_02', 'Hint: attacking a flag', 'help,m_flag_chain', 'flag_chain'),
]
MODE_CARD = {'mode_domination': 'spawn_control', 'mode_kingofthehill': 'zone_control',
             'mode_capturethestatue': 'relic_run', 'mode_assault': 'siege', 'mode_rocketriot': 'launch_race',
             'mode_blitzkrieg': 'flag_chain'}
HELP_T_MODE = {0: 'spawn_control', 1: 'zone_control', 2: 'relic_run', 3: 'siege', 4: 'launch_race', 5: 'flag_chain'}
EXPLAINER_ANIM = {'domination.anim': 'spawn_control', 'koth.anim': 'zone_control', 'cts.anim': 'relic_run',
                  'assault_defend.anim': 'siege', 'rocket.anim': 'launch_race', 'blitz.anim': 'flag_chain'}
ANIM_FILE_MODE = {'domination': 'spawn_control', 'koth': 'zone_control', 'cts': 'relic_run',
                  'assault_defend': 'siege', 'rocket': 'launch_race', 'blitz': 'flag_chain'}
EXPLAINER_IMG = {  # orig stem -> (species stem, role)
    'armour': ('explainer_transport', 'Transport'), 'cloudyglow': ('explainer_glow', 'soft glow'),
    'darwinian': ('explainer_worker', 'Worker figure'), 'darwinian_ouch4': ('explainer_worker_hurt', 'hurt Worker'),
    'flag': ('explainer_flag_neutral', 'neutral flag'), 'flag_down': ('explainer_flag_lowered', 'lowered flag'),
    'flag_locked': ('explainer_flag_locked', 'locked flag'), 'flag_up': ('explainer_flag_raised', 'raised flag'),
    'fuelpump': ('explainer_fuel_pump', 'fuel pump'), 'fuelpumper': ('explainer_fuel_pump_arm', 'fuel pump arm'),
    'island': ('explainer_island_shape', 'island outline'), 'laser': ('explainer_beam', 'laser beam'),
    'muzzleflash': ('explainer_muzzle_flash', 'muzzle flash'), 'plusone': ('explainer_score_plus_one', '+1 score pop-up'),
    'rocket': ('explainer_launch_vessel', 'Launch Vessel'), 'solarpanel': ('explainer_solar_array', 'Solar Array'),
    'spawnpoint': ('explainer_spawn_node', 'Spawn Node'), 'statue': ('explainer_relic', 'Relic'),
    'trunkport': ('explainer_transit_gate', 'Transit Gate'), 'turretbarrel': ('explainer_turret_barrel', 'turret barrel'),
    'turretbase': ('explainer_turret_base', 'turret base'), 'water': ('explainer_water', 'water band'),
    'wmd': ('explainer_core_device', 'Core Device'), 'zone-back': ('explainer_zone_back', 'Control Zone, back layer'),
    'zone-front': ('explainer_zone_front', 'Control Zone, front layer'),
}

# Out-of-scope textures whose CSV reach is FULL
TEX_EXCLUDE_FULL = {
    'textures/beam.bmp': 'dropped feature (dropped crate outcome and its structure)',
    'textures/prices.bmp': 'store and branding art',
    'icons/icon_controller.bmp': 'dropped feature (dropped weapon)',
    'icons/icon_dropship.bmp': 'dropped feature (dropped unit)',
    'icons/icon_fireextinguisher.bmp': 'dropped feature (disabled crate outcome)',
    'icons/icon_shaman.bmp': 'dropped feature (dropped unit)',
    'icons/icon_tank.bmp': 'dropped feature (dropped mode and unit)',
    'thumbnails/default.bmp': 'unreachable fallback',
    'thumbnails/mp_cts_3p_3.bmp': 'cut or test map',
    'mwhelp/mode_tankbattle.bmp': 'dropped feature (dropped mode)',
}
for _g in ['airstrike', 'armour', 'controller', 'engineer', 'grenade', 'officer', 'rocket', 'squad']:
    TEX_EXCLUDE_FULL['icons/gesture_%s.bmp' % _g] = 'dropped feature (gesture input)'
TEX_EXCLUDE_FULL['icons/gestureguide.bmp'] = 'dropped feature (gesture input)'

REACH_CLASS = {
    'none': 'unused (never named)', 'DEAD': 'unused (named only in dead code or data)',
    'LEGACY': 'campaign, single-player or unreachable code path', 'D3D': 'Direct3D 9 only',
    'DEBUG': 'debug only', 'EDITOR': 'editor only',
}


def reach_class(reach):
    if reach.startswith('none'):
        if 'source' in reach or 'Paint Shop' in reach or '3ds Max' in reach:
            return 'authoring source'
        return REACH_CLASS['none']
    return REACH_CLASS.get(reach, 'unused (never named)')


# ----------------------------------------------------------------------------------------
# Meshes.  orig stem -> (group folder, species stem, asset id, role, consumers, priority, group key)
# ----------------------------------------------------------------------------------------
MESH_TABLE = r"""
darwinian|units|worker_figure|mesh.unit.worker_figure|Worker 3-D figure (team-recoloured mesh with a flag socket)|worker,r_entity|launch
officer|units|leader_figure|mesh.unit.leader_figure|Leader figure carrying its banner|leader,r_entity|fp
squad|units|trooper_figure|mesh.unit.trooper_figure|Trooper figure|strike,r_entity|fp
engineer|units|builder_body|mesh.unit.builder_body|Builder body|builder,r_entity|launch
armour|units|transport_hull|mesh.unit.transport_hull|Transport hull|transport,r_entity|fp
harvester|units|collector_body|mesh.unit.collector_body|Collector body|collector,r_entity|launch
spaceinvader|units|bomber_body|mesh.unit.bomber_body|Bomber flying the airstrike|crates,weapons,r_entity|fp
throwable|projectiles|thrown_weapon|mesh.projectile.thrown_weapon|Thrown weapon: grenade, the Bomber's bombs and rockets|weapons,r_entity|fp
turretshell|projectiles|turret_round|mesh.projectile.turret_round|Turret round|weapons,defences|fp
emptyshell|projectiles|shell_casing|mesh.projectile.shell_casing|Ejected shell casing|weapons,defences|fp
meteor|projectiles|meteor_rock|mesh.projectile.meteor_rock|Meteor of the meteor shower|crates,weapons|launch
armyant|creatures|ant_body_01|mesh.creature.ant_body|ant body, three variants chosen per ant|creatures|launch
armyant2|creatures|ant_body_02|mesh.creature.ant_body||creatures|launch
armyant3|creatures|ant_body_03|mesh.creature.ant_body||creatures|launch
centipede|creatures|centipede_segment|mesh.creature.centipede_segment|centipede body segment|creatures|launch
centipedehead|creatures|centipede_head_segment|mesh.creature.centipede_head_segment|centipede head segment|creatures|launch
spider|creatures|spider_body|mesh.creature.spider_body|spider body with eight leg sockets|creatures|launch
spider_leg_upper|creatures|spider_thigh|mesh.creature.spider_thigh|spider upper leg part (procedural leg)|creatures|launch
spider_leg_lower|creatures|spider_shin|mesh.creature.spider_shin|spider lower leg part (procedural leg)|creatures|launch
souldestroyerhead|creatures|reaper_serpent_head|mesh.creature.reaper_serpent_head|Reaper Serpent head|creatures|launch
souldestroyertail|creatures|reaper_serpent_tail|mesh.creature.reaper_serpent_tail|Reaper Serpent tail segment|creatures|launch
sporegenerator|creatures|brood_drifter_body|mesh.creature.brood_drifter_body|Brood Drifter body|creatures|launch
spaceship|creatures|abductor_hull|mesh.creature.abductor_hull|Abductor hull (also placed by maps as scenery)|creatures,crates,maps|launch
triffidegg|creatures|seed_pod|mesh.creature.seed_pod|Seed Pod (also the Spitter Plant's pod)|creatures,defences|launch
triffidhead|structures|spitter_plant_head|mesh.structure.spitter_plant_head|Spitter Plant head|defences,crates|launch
anthill|structures|hive_mound|mesh.structure.hive_mound|Hive mound (also the placement-cursor preview)|defences,crates|launch
spawnpointmultiwinia|structures|spawn_node|mesh.structure.spawn_node|Spawn Node (the match variant)|econ,m_spawn_control|fp
spawnpoint|structures|spawn_node_alt|mesh.structure.spawn_node_alt|Spawn Node alternative body (second constructor branch)|econ|post
masterspawnpoint|structures|source_node|mesh.structure.source_node|Source Node|econ|launch
spawnlink|structures|relay_mast|mesh.structure.relay_mast|Relay|econ|fp
trunkport_mp|structures|transit_gate|mesh.structure.transit_gate|Transit Gate (the match variant)|econ|fp
trunkport|structures|transit_gate_alt|mesh.structure.transit_gate_alt|Transit Gate alternative body (second constructor branch)|econ|post
controlstation|structures|command_console|mesh.structure.command_console|Command Console|modeobj,m_siege|launch
pulsebombcontrolstation|structures|command_console_linked|mesh.structure.command_console_linked|Command Console linked to a Core Device (with dish-direction socket)|modeobj,m_siege|launch
pulsebomb|structures|core_device|mesh.structure.core_device|Core Device|modeobj,m_siege|launch
wallbuster|structures|breach_charge|mesh.structure.breach_charge|Breach Charge|modeobj,m_siege|launch
rocket|structures|launch_vessel|mesh.structure.launch_vessel|Launch Vessel|modeobj,m_launch_race|launch
fuelgenerator|structures|fuel_pump_housing|mesh.structure.fuel_pump_housing|fuel pump housing|modeobj,m_launch_race|launch
fuelgeneratorpump|structures|fuel_pump_arm|mesh.structure.fuel_pump_arm|fuel pump moving arm|modeobj,m_launch_race|launch
fuelpipebase|structures|fuel_line_foot|mesh.structure.fuel_line_foot|fuel line foot|modeobj,m_launch_race|launch
fuelpipe|structures|fuel_line_span|mesh.structure.fuel_line_span|fuel line span between feet|modeobj,m_launch_race|launch
fuelstation|structures|fuel_depot|mesh.structure.fuel_depot|fuel depot|modeobj,m_launch_race|launch
solarpanel|structures|solar_array|mesh.structure.solar_array|Solar Array|econ|launch
pylon|structures|grid_pylon|mesh.structure.grid_pylon|grid pylon (also the grid terminal and the grid splitter)|econ|launch
radardish|structures|beam_dish|mesh.structure.beam_dish|Beam Dish with a rotating dish node|econ|launch
laserfence|structures|barrier_emitter|mesh.structure.barrier_emitter|Barrier Emitter post|defences|fp
battlecannonbase|structures|turret_mount|mesh.structure.turret_mount|turret mount: socket carrier only (manning ports and status lights), no geometry|defences|fp
battlecannonturret|structures|turret_head|mesh.structure.turret_head|turret head (also the placement-cursor preview)|defences,crates|fp
cannon_bullets|structures|turret_barrel_gun|mesh.structure.turret_barrel|turret barrel set per turret type: gun, rocket, flame, and the shared cannon barrel|defences|fp
cannon_rocket|structures|turret_barrel_rocket|mesh.structure.turret_barrel||defences|launch
cannon_flame|structures|turret_barrel_flame|mesh.structure.turret_barrel||defences|launch
battlecannonbarrel|structures|turret_barrel_cannon|mesh.structure.turret_barrel||defences|launch
wall|structures|wall_section|mesh.structure.wall_section|wall section|defences|fp
researchitem|structures|supply_crate|mesh.structure.supply_crate|Supply crate body|crates|fp
aitarget|structures|bot_nav_node|mesh.structure.bot_nav_node|bot navigation node marker|maps|post
controlpad|structures|structure_default|mesh.structure.structure_default|default structure body assigned before a structure sets its own mesh|econ|post
carrystatue|relics|relic_01|mesh.relic.variant|Relic, eight interchangeable variants|modeobj,m_relic_run|launch
carrycube|relics|relic_02|mesh.relic.variant||modeobj,m_relic_run|launch
carryglobe|relics|relic_03|mesh.relic.variant||modeobj,m_relic_run|launch
carrysquaddie|relics|relic_04|mesh.relic.variant||modeobj,m_relic_run|launch
carryteapot|relics|relic_05|mesh.relic.variant||modeobj,m_relic_run|launch
carrytelescope|relics|relic_06|mesh.relic.variant||modeobj,m_relic_run|launch
carryweights|relics|relic_07|mesh.relic.variant||modeobj,m_relic_run|launch
carrybillboard|relics|relic_08|mesh.relic.variant||modeobj,m_relic_run|launch
beachheadobstruction|scenery|scenery_prop_01|mesh.scenery.prop|map scenery mesh, seven pieces placed by maps|maps,defences|launch
carryspider|scenery|scenery_prop_02|mesh.scenery.prop||maps,defences|launch
rockhead|scenery|scenery_prop_03|mesh.scenery.prop||maps,defences|launch
statue|scenery|scenery_prop_04|mesh.scenery.prop||maps,defences|launch
temple2|scenery|scenery_prop_05|mesh.scenery.prop||maps,defences|launch
temple3|scenery|scenery_prop_06|mesh.scenery.prop||maps,defences|launch
temple4|scenery|scenery_prop_07|mesh.scenery.prop||maps,defences|launch
"""

MESH_DROP = {  # FULL in the CSV, but only consumed by features the reference library drops
    'blueprintconsole', 'blueprintrelay', 'blueprintstore', 'boxkite', 'bridgeend', 'bridgetower', 'cave',
    'chesspiece', 'clonelab', 'constructionyard', 'constructionyardrung', 'controltower', 'controltowerdish',
    'displayscreen', 'factory', 'feedingtube', 'fenceswitch', 'flying_egg', 'generator', 'goddish', 'incubator',
    'lander', 'library', 'mine', 'minecart', 'minepolygon1', 'mineprimitive1', 'wheel', 'missile', 'powerstation',
    'primaryupgradeport', 'receiverlink', 'refinery', 'spiritprocessor', 'spiritreceiver', 'spiritreceiverhead',
    'tank_body', 'tank_turret', 'tracklink', 'tripod', 'tripod_leg_part', 'upgrade_port',
}

SOCKET_FAMILY = {
    'Port': 'port', 'Light': 'light', 'BarrelEnd': 'muzzle', 'Status': 'status', 'Carry': 'carry', 'Leg': 'leg',
    'Engine': 'engine', 'SpiritLink': 'essence_link', 'Barrel': 'barrel_mount', 'Tail': 'tail',
    'Destination': 'gate_exit', 'End': 'segment_end', 'Fuel': 'fuel_point', 'Glow': 'glow', 'Grab': 'grab',
    'Shells': 'casing_eject', 'Eggs': 'egg_spawn', 'Entrance': 'entrance', 'Flag': 'flag', 'PowerLocation': 'grid_link',
    'Focus': 'beam_focus', 'TeleportEntrance': 'gate_entrance', 'Surface': 'gate_surface', 'Door': 'door',
    'Window': 'window', 'Booster': 'thruster', 'Fence': 'field_anchor', 'Kills': 'kill_counter', 'Eye': 'eye',
    'Brass': 'casing_eject', 'Laser': 'muzzle', 'LaunchPoint': 'launch_point', 'TriffidStem': 'stem',
    'Tip': 'arm_tip', 'EggLay': 'egg_lay', 'DishDirection': 'dish_direction',
}
NODE_NAME = {'Door': 'door', 'Cylinder01': 'column', 'Dish': 'dish', 'UpperMount': 'upper_mount'}

# ----------------------------------------------------------------------------------------
# Audio
# ----------------------------------------------------------------------------------------
SOURCES = {  # object type -> (kind, species source, consumer keys, priority)
    'Darwinian': ('unit', 'worker', 'worker,audio', 'fp'),
    'Officer': ('unit', 'leader', 'leader,audio', 'fp'),
    'Squadie': ('unit', 'trooper', 'strike,audio', 'fp'),
    'Engineer': ('unit', 'builder', 'builder,audio', 'launch'),
    'Armour': ('unit', 'transport', 'transport,audio', 'fp'),
    'Harvester': ('unit', 'collector', 'collector,audio', 'launch'),
    'SpaceInvader': ('unit', 'bomber', 'crates,weapons,audio', 'fp'),
    'Spirit': ('unit', 'essence_orb', 'essence,audio', 'fp'),
    'Virii': ('creature', 'crawler', 'creatures,audio', 'launch'),
    'Centipede': ('creature', 'centipede', 'creatures,audio', 'launch'),
    'Spider': ('creature', 'spider', 'creatures,audio', 'launch'),
    'ArmyAnt': ('creature', 'ant', 'creatures,audio', 'launch'),
    'SoulDestroyer': ('creature', 'reaper_serpent', 'creatures,audio', 'launch'),
    'SporeGenerator': ('creature', 'brood_drifter', 'creatures,audio', 'launch'),
    'SpaceShip': ('creature', 'abductor', 'creatures,crates,audio', 'launch'),
    'TriffidEgg': ('creature', 'seed_pod', 'creatures,audio', 'launch'),
    'Nuke': ('weapon', 'barrage_missile', 'crates,weapons,audio', 'launch'),
    'Meteor': ('weapon', 'meteor', 'crates,weapons,audio', 'launch'),
    'Laser': ('weapon', 'laser_bolt', 'weapons,audio', 'fp'),
    'Grenade': ('weapon', 'grenade', 'weapons,audio', 'fp'),
    'Rocket': ('weapon', 'rocket', 'weapons,audio', 'fp'),
    'AirStrikeBomb': ('weapon', 'airstrike_bomb', 'crates,weapons,audio', 'fp'),
    'RadarDish': ('building', 'beam_dish', 'econ,audio', 'launch'),
    'LaserFence': ('building', 'barrier_emitter', 'defences,audio', 'fp'),
    'GunTurret': ('building', 'turret', 'defences,audio', 'fp'),
    'Tree': ('building', 'tree', 'defences,audio', 'fp'),
    'Pylon': ('building', 'grid_pylon', 'econ,audio', 'launch'),
    'SolarPanel': ('building', 'solar_array', 'econ,audio', 'launch'),
    'AntHill': ('building', 'hive', 'defences,crates,audio', 'launch'),
    'Triffid': ('building', 'spitter_plant', 'defences,crates,audio', 'launch'),
    'SpawnPoint': ('building', 'spawn_node', 'econ,audio', 'fp'),
    'FuelGenerator': ('building', 'fuel_pump', 'modeobj,m_launch_race,audio', 'launch'),
    'FuelPipe': ('building', 'fuel_line', 'modeobj,m_launch_race,audio', 'launch'),
    'FuelStation': ('building', 'fuel_depot', 'modeobj,m_launch_race,audio', 'launch'),
    'EscapeRocket': ('building', 'launch_vessel', 'modeobj,m_launch_race,audio', 'launch'),
    'MultiwiniaZone': ('building', 'control_zone', 'modeobj,m_zone_control,m_relic_run,audio', 'launch'),
    'ControlStation': ('building', 'command_console', 'modeobj,m_siege,audio', 'launch'),
    'Statue': ('building', 'relic', 'modeobj,m_relic_run,audio', 'launch'),
    'PulseBomb': ('building', 'core_device', 'modeobj,m_siege,audio', 'launch'),
    'Crate': ('crate', 'supply_crate', 'crates,audio', 'fp'),
    'CratePowerup': ('crate', 'outcome', 'crates,audio', 'launch'),
    'MultiwiniaInterface': ('ui', 'match_ui', 'audio,ui_pages', 'fp'),
    'Interface': ('ui', 'power_slots', 'audio,crates', 'fp'),
    'Gesture': ('ui', 'power_input', 'audio,crates', 'fp'),
    'Ambience': ('ambience', 'match', 'audio', 'fp'),
}

EVENTS = {
    'Attack': 'attack', 'Die': 'die', 'LoseHealth': 'hurt', 'ChangeDirection': 'turn',
    'WeaponReturns': 'weapon_ready', 'FireLaser': 'fire_laser', 'EnterTeleport': 'enter_gate',
    'ExitTeleport': 'exit_gate', 'LayEgg': 'lay_egg', 'Pounce': 'pounce', 'FootFall': 'footstep',
    'GivenOrders': 'ordered', 'SeenThreatRunAway': 'flee_bark', 'VictoryJump': 'victory_jump', 'OnFire': 'burning',
    'Selected': 'selected', 'GivenOrdersFromPlayer': 'ordered_by_player', 'LiftBuilding': 'lift_relic',
    'SeenThreatAttack': 'attack_bark', 'SeenThreatAttackHorn': 'attack_horn', 'SetOrderGoto': 'order_waypoint',
    'SetOrderFormation': 'order_formation', 'SetOrders': 'ordered', 'EnemySighted': 'enemy_sighted',
    'Bounce': 'bounce', 'BurstOpen': 'burst_open', 'DropSpirit': 'drop_essence', 'Explode': 'explode',
    'Land': 'land', 'TractorBeam': 'tractor_beam', 'DropGrenade': 'drop_bomb', 'BeginRotation': 'rotate_start',
    'EndRotation': 'rotate_stop', 'ConnectionEstablished': 'link_up', 'ConnectionLost': 'link_lost',
    'Spark': 'spark', 'Damage': 'damaged', 'FireShell': 'fire_gun', 'FireFlame': 'fire_flame',
    'RegisterKill': 'kill_tick', 'CreateNormal': 'ambient_normal', 'CreateMagic': 'ambient_essence',
    'CreateEvil': 'ambient_blight', 'Burn': 'burning', 'TriggerSurge': 'surge', 'Operate': 'charging',
    'TurnOn': 'activate', 'TurnOff': 'deactivate', 'PumpUp': 'pump_up', 'PumpDown': 'pump_down',
    'PumpFuel': 'flow', 'LoadPassenger': 'load_passenger', 'Happy': 'fuelled', 'Unhappy': 'hungry',
    'Malfunction': 'malfunction', 'EngineBurn': 'engine_burn', 'Flight': 'flight', 'CaptureStatue': 'relic_scored',
    'ChangeTeam': 'owner_changed', 'BeginCapture': 'capture_begin', 'LostCapture': 'capture_lost',
    'Capturing_0pc': 'capture_progress_00', 'Capturing_20pc': 'capture_progress_20',
    'Capturing_40pc': 'capture_progress_40', 'Capturing_60pc': 'capture_progress_60',
    'Capturing_80pc': 'capture_progress_80', 'Captured': 'captured', 'Falling': 'falling', 'Drop': 'drop',
    'InjuredEntity': 'hit_bark', 'Flash': 'fuse_blink', 'PickedUp': 'collected',
    'GestureBegin': 'begin', 'GestureSuccess': 'confirm', 'EnterLocation': 'bed',
    'DeleteTask': 'discard', 'MouseOverIcon': 'icon_hover', 'CountdownTick': 'countdown_tick',
    'CountdownZeroTick': 'countdown_zero', 'SelectionCircle': 'selection_circle', 'LobbyAmbience': 'lobby_bed',
    'MenuRollOver': 'menu_hover', 'MenuSelect': 'menu_select', 'MenuCancel': 'menu_back',
    'UnitHighlight': 'unit_highlight', 'SelectUnit': 'unit_select', 'MouseError': 'error',
    'ToggleUnitState': 'toggle_state', 'ChatMessage': 'chat_message',
}
CRATE_OUTCOME = {
    'Squad': 'strike_team', 'Harvester': 'collector', 'Armour': 'transport', 'Darwinians': 'reinforcements',
    'AntNest': 'hive', 'Eggs': 'brood', 'Airstrike': 'airstrike', 'MagicalForest': 'essence_grove',
    'DarkForest': 'blight_grove', 'CrateMania': 'supply_storm', 'GunTurret': 'gun_turret',
    'Meteor': 'meteor_shower', 'HotFeet': 'speed_boost', 'Nuke': 'missile_barrage', 'Plague': 'sickness',
    'Randomiser': 'allegiance_scrambler', 'PersonalShield': 'shield', 'Subversion': 'convert',
    'SpawnMania': 'spawn_surge', 'Bitzkreig': 'frenzy', 'Rocketwinians': 'rocket_troops', 'Infection': 'swarm',
    'Rage': 'fury', 'Triffids': 'spitter_plants', 'SpaceShip': 'abductor',
}
FP_OUTCOMES = {'reinforcements', 'strike_team', 'transport', 'airstrike', 'gun_turret', 'shield', 'speed_boost', 'fury'}
# primary binding overrides for banks shared by several bindings: bank -> (object type, event)
PRIMARY = {
    ('Group', 'spideregglay'): ('Spider', 'LayEgg'), ('Group', 'spiderpounce'): ('Spider', 'Pounce'),
    ('Group', 'officercreate'): ('Officer', 'Create'), ('Group', 'souldestpain'): ('SoulDestroyer', 'LoseHealth'),
    ('Sample', 'ufo_engine2'): ('SpaceShip', 'Create'), ('Sample', 'ufo_engine1'): ('SpaceShip', 'Create'),
    ('Sample', 'countdownbeep3'): ('MultiwiniaInterface', 'CountdownTick'),
    ('Group', 'newsamplegroup3'): ('Grenade', 'Explode'),
}

# ----------------------------------------------------------------------------------------
# Strings: categories of player-facing text (counts only)
# ----------------------------------------------------------------------------------------
P17B_FILE_CAT = {
    'game_menu.cpp': ('in', 'ui.menu', 'front-end menus'),
    'chatinput_window.cpp': ('in', 'ui.chat', 'chat'),
    'controllerunplugged_window.cpp': ('in', 'ui.dialog', 'controller-disconnected dialog'),
    'helpandoptions_windows.cpp': ('in', 'ui.help', 'help and options screens'),
    'leaderboards_window.cpp': ('in', 'ui.leaderboard', 'leaderboard and statistics pages'),
    'mainmenus.cpp': ('in', 'ui.menu', 'main and pause menus'),
    'mainmenus.h': ('in', 'ui.menu', 'main and pause menus'),
    'prefs_graphics_window.cpp': ('in', 'ui.options', 'graphics options'),
    'prefs_screen_window.cpp': ('in', 'ui.options', 'display options'),
    'prefs_other_window.cpp': ('in', 'ui.options', 'general options'),
    'prefs_keybindings_window.cpp': ('in', 'ui.options', 'input options'),
    'demoend_window.cpp': ('out', None, 'trial-version and store prompts'),
    'mods_window.cpp': ('out', None, 'dropped feature (mods)'),
    'userprofile_window.cpp': ('out', None, 'dropped feature (user profiles)'),
    'multiwiniaoptions_window.cpp': ('out', None, 'editor only (map editor match options)'),
}
P17B_KEY_OUT = [  # key pattern -> exclusion class (applied before the file category)
    (r'^location_(?!unknown)', 'campaign, single-player or unreachable code path'),
    (r'darwinia', 'campaign, single-player or unreachable code path'),
    (r'_auth$|authkey', 'trial-version and store prompts'),
    (r'cratehelp|menu_crates$|crate_hint', 'debug only'),
    (r'bootloader', 'unused (named only in dead code or data)'),
    (r'portforwarding|serverport|clientport', 'deferred to the design phase (networking options)'),
    (r'^dialog_(profile|mods)$', 'dropped feature (user profiles, mods)'),
    (r'gestures$|prefs_icons$', 'dropped feature (gesture input)'),
]
P02_KEY_OUT = [(r'tank', 'dropped feature (dropped mode)')]
P17A_FILE_CAT = {
    'darwinia_window.cpp': ('in', 'ui.window', 'window chrome'),
    'input_field.cpp': ('in', 'ui.input', 'text and number fields'),
    'message_dialog.cpp': ('in', 'ui.dialog', 'message dialog'),
    'eclwindow.cpp': ('in', 'ui.window', 'window chrome'),
    'filedialog.cpp': ('in', 'ui.dialog', 'file dialog'),
    'cheat_window.cpp': ('out', None, 'debug only'), 'debugmenu.cpp': ('out', None, 'debug only'),
    'gesture_window.cpp': ('out', None, 'debug only'), 'grabber_window.cpp': ('out', None, 'debug only'),
}
LANGUAGES = [('en-GB', 'fp', 'English (United Kingdom), the source language'), ('fr-FR', 'launch', 'French'),
             ('de-DE', 'launch', 'German'), ('it-IT', 'launch', 'Italian'), ('es-ES', 'launch', 'Spanish'),
             ('pt-BR', 'launch', 'Portuguese (region to be confirmed)'), ('ru-RU', 'launch', 'Russian'),
             ('ja-JP', 'launch', 'Japanese'), ('ko-KR', 'launch', 'Korean')]


# ----------------------------------------------------------------------------------------
def read_csv(path):
    with open(path, newline='', encoding='utf-8') as f:
        return list(csv.DictReader(f))


def docs(keys):
    out = []
    for k in keys.split(','):
        k = k.strip()
        if not k:
            continue
        if k not in DOCS:
            raise SystemExit('unknown consumer key %r' % k)
        if DOCS[k] not in out:
            out.append(DOCS[k])
    return out


class Builder:
    def __init__(self):
        self.groups = collections.OrderedDict()   # asset_id -> group dict
        self.files = {}                           # species file -> asset_id
        self.origin = []                          # rows for the origin map (in scope)
        self.excluded = []                        # (orig path, class, kind)
        self.orig_to_file = {}                    # orig path -> species file

    def group(self, aid, **kw):
        g = self.groups.get(aid)
        if g is None:
            g = dict(asset_id=aid, files=[], originals=[], bound=[], notes=[])
            g.update(kw)
            self.groups[aid] = g
        else:
            for k, v in kw.items():
                if v and not g.get(k):
                    g[k] = v
        return g

    def add_file(self, aid, species_file, originals, statuses, spec_file=''):
        if species_file in self.files:
            raise SystemExit('duplicate species file %s' % species_file)
        self.files[species_file] = aid
        g = self.groups[aid]
        g['files'].append(dict(path=species_file, spec=spec_file, originals=list(originals)))
        for o in originals:
            if o in self.orig_to_file:
                raise SystemExit('original %s mapped twice' % o)
            self.orig_to_file[o] = species_file
        self.origin.append(dict(species_filename=species_file, asset_id=aid,
                                original_paths=';'.join(originals), original_count=len(originals),
                                original_status=';'.join(sorted(set(statuses)))))

    def exclude(self, orig, cls, kind):
        self.excluded.append((orig, cls, kind))


def size_of(row):
    try:
        return int(row['width']), int(row['height'])
    except (ValueError, KeyError):
        return None


def ui_size(w, h):
    return '%dx%d px display size; author at 2x (%dx%d)' % (w, h, 2 * w, 2 * h)


def build_textures(b, rows):
    table = []
    for line in TEX_TABLE.strip().splitlines():
        p = line.split('|')
        if len(p) != 9:
            raise SystemExit('bad TEX_TABLE line: %s' % line)
        table.append(p)
    by_orig = {r['path'][len('data/'):]: r for r in rows}
    seen = set()
    # role table
    for orig, cat, spath, aid, role, cons, prio, prof, extra in table:
        r = by_orig.get(orig)
        if r is None:
            raise SystemExit('missing texture row %s' % orig)
        if r['reach'] != 'FULL':
            raise SystemExit('role table names a non-FULL texture %s' % orig)
        src, tgt, cs, am, ph = PROFILES[prof]
        g = b.group(aid, category=cat, subcategory=spath.split('/')[-2],
                    role=role, consumers=docs(cons + ',ast'), priority=PRIO[prio], source_format=src,
                    target_format=tgt, colour_space=cs, alpha_mode=am, placeholder=PLACEHOLDER[ph],
                    brief_ref=BRIEF[cat], kind='texture')
        wh = size_of(r)
        twox = cat in ('ui', 'hud', 'glyphs') or spath.startswith('textures/icons/cursors/')
        sz = ui_size(*wh) if twox else '%dx%d px' % wh
        spec = sz + ('; ' + extra if extra else '')
        if 'spec' not in g or not g.get('spec'):
            g['spec'] = spec
        b.add_file(aid, 'content/%s.png' % spath, [r['path']], [r['reach']], spec)
        seen.add(r['path'])
    return seen


def build_terrain(b, rows, seen):
    by = {r['path']: r for r in rows}
    sets = [('ramp', 'landscape_%s', RAMPS, 'terrain/terrain_ramp_%02d', 'terrain.ramp',
             'Terrain colour ramp: colour by slope (x) and height (y), read on the CPU when the terrain mesh is built',
             'ramp', 'terrain,r_world,maps'),
            ('sea', 'water_%s', SEAS, 'terrain/sea_tile_%02d', 'terrain.sea_tile',
             'Sea surface tile (repeated, multiplied by the sea light map)', 'tile', 'terrain,r_world,maps'),
            ('wave', 'waves_%s', WAVES, 'terrain/shore_wave_gradient_%02d', 'terrain.shore_wave_gradient',
             'Shore-wave colour gradient: one row read on the CPU by the sea renderer', 'ramp', 'terrain,r_world,maps')]
    for key, pat, names, spat, aid, role, prof, cons in sets:
        src, tgt, cs, am, ph = PROFILES[prof]
        g = b.group(aid, category='terrain', subcategory='terrain', role=role, consumers=docs(cons + ',ast'),
                    priority=PRIO['launch'], source_format=src, target_format=tgt, colour_space=cs, alpha_mode=am,
                    placeholder=PLACEHOLDER[ph], brief_ref=BRIEF['terrain'], kind='texture')
        for i, n in enumerate(names, 1):
            orig = 'data/terrain/' + pat % n + '.bmp'
            r = by[orig]
            assert r['reach'] == 'FULL', orig
            wh = size_of(r)
            notes = []
            if i == 1:
                notes.append('map default')
            if n == SEASONAL[key]:
                notes.append('forced by the seasonal theme')
            spec = '%dx%d px' % wh
            if key == 'ramp':
                spec += '; x = slope, y = height'
            if key == 'wave':
                spec += '; the wave colour runs along x'
            if key == 'sea':
                spec += '; wrap repeat'
            spec += ('; ' + ', '.join(notes) if notes else '')
            if not g.get('spec'):
                g['spec'] = spec
            prio_file = 'skirmish-first-playable' if i == 1 else 'launch'
            b.add_file(aid, 'content/textures/%s.png' % (spat % i), [orig], [r['reach']], spec)
            g['files'][-1]['priority'] = prio_file
            seen.add(orig)


def build_help(b, rows, seen):
    by = {r['path']: r for r in rows}
    # hint images
    src, tgt, cs, am, ph = PROFILES['ui']
    for stem, sstem, role, cons, mode in HINTS:
        orig = 'data/mwhelp/%s.bmp' % stem
        r = by[orig]
        aid = 'help.hint.' + sstem[len('hint_'):]
        wh = size_of(r)
        g = b.group(aid, category='help', subcategory='hint', role=role + ' (hint overlay picture)',
                    consumers=docs(cons + ',ast'), priority=PRIO['launch'], source_format=src, target_format=tgt,
                    colour_space=cs, alpha_mode=am, placeholder=PLACEHOLDER['help'], brief_ref=BRIEF['help'],
                    kind='texture', spec=ui_size(*wh))
        b.add_file(aid, 'content/textures/help/%s.png' % sstem, [orig], [r['reach']], ui_size(*wh))
        seen.add(orig)
    # camera-controls hint (two reference files, one Species file without baked text)
    o1, o2 = 'data/mwhelp/control_camera.bmp', 'data/mwhelp/control_camera_french.bmp'
    src, tgt, cs, am, ph = PROFILES['uia']
    wh = size_of(by[o1])
    b.group('help.hint.camera_controls', category='help', subcategory='hint',
            role='Hint: camera controls, shown during the grace period; text labels come from the string table',
            consumers=docs('help,controls,ast'), priority=PRIO['launch'], source_format=src, target_format=tgt,
            colour_space=cs, alpha_mode=am, placeholder=PLACEHOLDER['help'], brief_ref=BRIEF['help'],
            kind='texture', spec=ui_size(*wh) + '; no baked text (one picture for every language)')
    b.add_file('help.hint.camera_controls', 'content/textures/help/hint_camera_controls.png', [o1, o2],
               [by[o1]['reach'], by[o2]['reach']], ui_size(*wh))
    seen.update([o1, o2])
    # mode cards
    src, tgt, cs, am, ph = PROFILES['ui']
    for stem, mode in MODE_CARD.items():
        orig = 'data/mwhelp/%s.bmp' % stem
        r = by[orig]
        wh = size_of(r)
        aid = 'help.mode_card'
        b.group(aid, category='help', subcategory='mode_card',
                role='Mode card on the mode-select buttons, one per mode', consumers=docs('frontend,modes,ast'),
                priority=PRIO['launch'], source_format=src, target_format=tgt, colour_space=cs, alpha_mode=am,
                placeholder=PLACEHOLDER['help'], brief_ref=BRIEF['help'], kind='texture', spec=ui_size(*wh))
        b.add_file(aid, 'content/textures/help/mode_card_%s.png' % mode, [orig], [r['reach']],
                   ui_size(*wh) + '; ' + MODE_NAME[mode])
        b.groups[aid]['files'][-1]['priority'] = 'skirmish-first-playable' if mode == 'spawn_control' else 'launch'
        seen.add(orig)
    # general help pages
    for p in range(1, 6):
        orig = 'data/mwhelp/general%d.bmp' % p
        r = by[orig]
        wh = size_of(r)
        aid = 'help.general_page'
        b.group(aid, category='help', subcategory='help_page', role='General help page picture, pages 1-5',
                consumers=docs('help,ast'), priority=PRIO['launch'], source_format=src, target_format=tgt,
                colour_space=cs, alpha_mode=am, placeholder=PLACEHOLDER['help'], brief_ref=BRIEF['help'],
                kind='texture', spec=ui_size(150, 150))
        b.add_file(aid, 'content/textures/help/help_general_page_%02d.png' % p, [orig], [r['reach']],
                   ui_size(*wh))
        seen.add(orig)
    # per-mode help pages
    for r in rows:
        m = re.match(r'data/mwhelp/help_(\d)_(\d)\.bmp$', r['path'])
        if not m:
            continue
        t, p = int(m.group(1)), int(m.group(2))
        mode = HELP_T_MODE[t]
        aid = 'help.mode_page'
        b.group(aid, category='help', subcategory='help_page',
                role='Per-mode help page picture (pages per mode: Spawn Control 1, Zone Control 2, Relic Run 3, '
                     'Siege 3, Launch Race 4, Flag Chain 3)', consumers=docs('help,modes,ast'),
                priority=PRIO['launch'], source_format=src, target_format=tgt, colour_space=cs, alpha_mode=am,
                placeholder=PLACEHOLDER['help'], brief_ref=BRIEF['help'], kind='texture', spec=ui_size(150, 150))
        wh = size_of(r)
        b.add_file(aid, 'content/textures/help/help_%s_page_%02d.png' % (mode, p), [r['path']], [r['reach']],
                   ui_size(*wh))
        seen.add(r['path'])
    # explainer animations (scripts) and their pictures
    for r in rows:
        if r['dir'] != 'animations':
            continue
        name = r['path'].split('/')[-1]
        if name.endswith('.anim'):
            mode = EXPLAINER_ANIM[name]
            m = re.search(r'\((\d+) x (\d+), t 0\.00\.\.([\d.]+) s\)', r['format'])
            w, h, dur = int(m.group(1)), int(m.group(2)), float(m.group(3))
            aid = 'anim.explainer'
            b.group(aid, category='help', subcategory='explainer',
                    role='Explainer animation: a timed key-frame script that animates pictures on a canvas, one per mode',
                    consumers=docs('help,modes,vfx,ui_pages,ast'), priority=PRIO['launch'],
                    source_format='JSON key-frame script in the Species explainer format (pictures referenced by asset id)',
                    target_format='cooked explainer script in the pack', colour_space='n/a', alpha_mode='n/a',
                    placeholder=PLACEHOLDER['explainer'], brief_ref=BRIEF['help'], kind='script',
                    spec='canvas and loop length per file')
            b.add_file(aid, 'content/ui/explainers/explainer_%s.json' % mode, [r['path']], [r['reach']],
                       'canvas %dx%d px, loop %.2f s; %s' % (w, h, dur, MODE_NAME[mode]))
            seen.add(r['path'])
        else:
            stem = name[:-4]
            sstem, role = EXPLAINER_IMG[stem]
            uses = sorted(set(ANIM_FILE_MODE[a] for a in re.findall(r'keyframe \((\w+)\.anim\)', r['consumer'])))
            prof = 'uia' if r['alpha_in_file'].startswith('A8') else ('key' if float(r['pink_fraction'] or 0) > 0 else 'ui')
            src, tgt, cs, am, ph = PROFILES[prof]
            if prof == 'key':
                tgt = 'DDS (DX10 header) BC7_UNORM_SRGB, no mips'
            aid = 'help.explainer_picture'
            b.group(aid, category='help', subcategory='explainer_picture',
                    role='Picture animated by the explainer scripts', consumers=docs('help,vfx,ast'),
                    priority=PRIO['launch'], source_format='PNG RGBA8, sRGB; straight alpha',
                    target_format='DDS (DX10 header) BC7_UNORM_SRGB, no mips', colour_space='sRGB',
                    alpha_mode='straight alpha blend', placeholder=PLACEHOLDER['panel'], brief_ref=BRIEF['help'],
                    kind='texture', spec='size per file')
            wh = size_of(r)
            used = ', '.join(MODE_NAME[u] for u in uses) if uses else 'every explainer'
            b.add_file(aid, 'content/textures/help/explainer/%s.png' % sstem, [r['path']], [r['reach']],
                       '%s; %dx%d px; used by %s' % (role, wh[0], wh[1], used))
            seen.add(r['path'])


def build_thumbs_and_maps(b, tex_rows, levels, seen):
    by = {r['path'].lower(): r for r in tex_rows}
    retail = [r for r in levels if r['ships_retail'] == 'Y']
    # slot order: by mode, then player count, then file name (stable)
    slots = collections.defaultdict(list)
    for r in retail:
        mode = ORIG_MODE[r['gametypes_tokens'].split()[0]]
        slots[(mode, int(r['numplayers']))].append(r)
    plan = []
    for mode in MODES:
        for p in (2, 3, 4):
            lst = sorted(slots.get((mode, p), []), key=lambda r: r['file'].lower())
            for i, r in enumerate(lst, 1):
                plan.append((mode, p, i, r))
    first = {('spawn_control', 2, 1), ('spawn_control', 2, 2)}
    for mode, p, i, r in plan:
        stem = 'map_%s_%dp_%02d' % (mode, p, i)
        prio = 'skirmish-first-playable' if (mode, p, i) in first else 'launch'
        notes = []
        if r['tutorial']:
            notes.append('also hosts a tutorial')
        aid = 'map.' + mode
        b.group(aid, category='maps', subcategory='map', role='%s map slot (new layout)' % MODE_NAME[mode],
                consumers=docs('maps,m_%s,frontend,ast' % mode), priority=PRIO['launch'],
                source_format='map definition authored in the map tool (text, Species map schema)',
                target_format='cooked map definition in the pack (terrain baked or regenerated per the terrain decision)',
                colour_space='n/a', alpha_mode='n/a', placeholder=PLACEHOLDER['map'], brief_ref=BRIEF['maps'],
                kind='map', spec='players per file')
        b.add_file(aid, 'content/maps/%s.json' % stem, [r['file']],
                   ['FULL'], '%d players%s' % (p, '; ' + ', '.join(notes) if notes else ''))
        b.groups[aid]['files'][-1]['priority'] = prio
        # thumbnail
        trow = by.get(r['thumbnail'].lower())
        if trow is None:
            raise SystemExit('no thumbnail for %s (%s)' % (r['file'], r['thumbnail']))
        src, tgt, cs, am, ph = PROFILES['thumb']
        taid = 'thumb.' + mode
        b.group(taid, category='maps', subcategory='thumbnail', role='%s map thumbnail, rendered by the map tool' % MODE_NAME[mode],
                consumers=docs('frontend,maps,ast'), priority=PRIO['launch'], source_format=src, target_format=tgt,
                colour_space=cs, alpha_mode=am, placeholder=PLACEHOLDER['thumb'], brief_ref=BRIEF['maps'],
                kind='texture', spec='800x600 px (4:3; display size 400x300)')
        b.add_file(taid, 'content/thumbnails/%s_thumb.png' % stem, [trow['path']], [trow['reach']],
                   '800x600 px (4:3; display size 400x300); top-down capture of %s' % stem)
        b.groups[taid]['files'][-1]['priority'] = prio
        seen.add(trow['path'])
    return plan


def build_fonts(b, rows, seen):
    fonts = collections.OrderedDict([
        ('square_latin', ('font.ui.primary', 'content/fonts/ui_primary.otf',
                          'Primary UI face: the face that every language uses, Latin and Cyrillic scripts',
                          'U+0020-00FF, U+0410-044F, U+20A0-20AF', 'fp')),
        ('square_ja', ('font.ui.primary_ja', 'content/fonts/ui_primary_ja.otf',
                       'Japanese companion of the primary face (kana and CJK ideographs)',
                       'U+3000-30FF, U+4E00-9EFF', 'launch')),
        ('square_ko', ('font.ui.primary_ko', 'content/fonts/ui_primary_ko.otf',
                       'Korean companion of the primary face (Hangul syllables)', 'U+AC00-D7FF', 'launch')),
        ('speccy', ('font.ui.stylised', 'content/fonts/ui_stylised.otf',
                    'Stylised display face (the second reference face)', 'U+0020-00FF, U+0410-044F, U+20A0-20AF',
                    'launch')),
        ('editor', ('font.ui.compact', 'content/fonts/ui_compact.otf',
                    'Compact face for dense panels (the third reference face)', 'U+0020-00FF, U+0410-044F, U+20A0-20AF',
                    'launch')),
    ])
    members = collections.defaultdict(list)
    for r in rows:
        if not r['consumer'].startswith('TextRenderer'):
            continue
        m = re.match(r'data/textures/unicode/(\w+)\.([0-9a-f]+)\.([0-9a-f]+)\.bmp$', r['path'])
        font, lo = m.group(1), int(m.group(2), 16)
        if font == 'square':
            key = 'square_ja' if 0x3000 <= lo <= 0x9fff else ('square_ko' if 0xac00 <= lo <= 0xd7ff else 'square_latin')
        else:
            key = font
        members[key].append(r)
    for key, (aid, path, role, cover, prio) in fonts.items():
        rs = members[key]
        b.group(aid, category='fonts', subcategory='font', role=role, consumers=docs('ui_text,options,ast'),
                priority=PRIO[prio], source_format='OpenType font file (.otf or .ttf) licensed for embedding',
                target_format='font file stored in the pack; glyphs rasterised at run time into R8_UNORM glyph-cache pages',
                colour_space='n/a (coverage only)', alpha_mode='coverage (premultiplied in the UI shader)',
                placeholder=PLACEHOLDER['font'], brief_ref=BRIEF['fonts'], kind='font',
                spec='coverage %s' % cover)
        b.add_file(aid, path, [r['path'] for r in rs], [r['reach'] for r in rs],
                   'coverage %s; replaces %d bitmap atlas pages' % (cover, len(rs)))
        for r in rs:
            seen.add(r['path'])


def build_meshes(b, rows):
    table = {}
    for line in MESH_TABLE.strip().splitlines():
        p = line.split('|')
        if len(p) != 7:
            raise SystemExit('bad MESH_TABLE line %s' % line)
        table[p[0]] = p
    in_scope = set()
    for r in rows:
        if r['kind'] != 'shp':
            continue
        stem = r['path'].split('/')[-1][:-4]
        if stem not in table:
            continue
        if r['reach'] != 'FULL':
            raise SystemExit('mesh table names non-FULL %s' % stem)
        _, grp, sstem, aid, role, cons, prio = table[stem]
        sockets = []
        for mk in [m for m in r['marker_names'].split('; ') if m]:
            name, parent = mk.split('@')
            m = re.match(r'Marker([A-Za-z]+?)(\d*)$', name)
            fam, num = m.group(1), m.group(2)
            if fam not in SOCKET_FAMILY:
                # families whose name ends in letters followed by digits, e.g. Light1
                raise SystemExit('unknown socket family %s in %s' % (fam, stem))
            s = SOCKET_FAMILY[fam] + ('_%02d' % int(num) if num else '')
            if parent != 'sceneroot':
                s += ' (on node %s)' % NODE_NAME.get(parent, 'node_' + parent.lower())
            sockets.append(s)
        tris = int(r['triangles'] or 0)
        nodes = int(r['fragments'] or 0)
        spec = '%s triangles budget; %d node%s, depth %s; sockets: %s' % (
            ('<= %d' % tris) if tris else 'no', nodes, '' if nodes == 1 else 's', r['max_depth'],
            ', '.join(sockets) if sockets else 'none')
        g = b.group(aid, category='meshes', subcategory=grp, role=role, consumers=docs(cons + ',r_entity,ast'),
                    priority=PRIO[prio],
                    source_format='glTF 2.0 binary (.glb): no NORMAL attribute (flat faces), COLOR_0 per-face colour, metres, +Y up',
                    target_format='cooked Species mesh in the pack (face normals generated, colours attenuated at load)',
                    colour_space='sRGB vertex colours', alpha_mode='opaque (team parts painted mostly red)',
                    placeholder=PLACEHOLDER['mesh'], brief_ref=BRIEF['meshes'], kind='mesh', spec=spec)
        b.add_file(aid, 'content/meshes/%s/%s.glb' % (grp, sstem), [r['path']], [r['reach']], spec)
        b.groups[aid]['files'][-1]['priority'] = PRIO[prio]
        in_scope.add(r['path'])
    missing = set(table) - set(r['path'].split('/')[-1][:-4] for r in rows)
    if missing:
        raise SystemExit('mesh table names unknown shapes %s' % missing)
    for r in rows:
        if r['path'] in in_scope:
            continue
        stem = r['path'].split('/')[-1][:-4]
        if r['kind'] == 'max':
            b.exclude(r['path'], 'authoring source', 'mesh')
        elif r['reach'] == 'FULL':
            if stem not in MESH_DROP:
                raise SystemExit('FULL mesh with no decision: %s' % stem)
            b.exclude(r['path'], 'dropped feature (campaign or dropped unit, structure or mode)', 'mesh')
        else:
            b.exclude(r['path'], reach_class(r['reach']), 'mesh')


def parse_blocks(bp):
    out = []
    for blk in bp.split('||'):
        m = re.search(r'\[(Group|Sample) ([^\]]+)\]\s*(\S+)', blk)
        if m:
            out.append(((m.group(1), m.group(2).lower()), m.group(3)))
    return out


def build_audio(b, samples, events):
    live_pairs = [e for e in events if e['in_data'] == 'yes' and e['fired_in_FULL'] == 'yes']
    bank_pairs = collections.OrderedDict()   # bank -> list of (otype, event, block index, mode)
    for e in live_pairs:
        seen_b = []
        for bank, mode in parse_blocks(e['blueprint']):
            if bank in seen_b:
                continue
            seen_b.append(bank)
            bank_pairs.setdefault(bank, []).append((e['object_type'], e['event'], len(seen_b) - 1, mode))
    members = collections.defaultdict(list)
    for s in samples:
        for g in [g for g in s['via_groups'].split(';') if g]:
            members[('Group', g.lower())].append(s)
        members[('Sample', s['name'].lower())].append(s)
    assigned = {}
    bank_ids = collections.OrderedDict()
    primaries = collections.OrderedDict()
    owned = collections.defaultdict(list)    # (otype, event) -> banks it owns, in block order
    for bank, pairs in bank_pairs.items():
        prim = PRIMARY.get(bank)
        pp = [p for p in pairs if (p[0], p[1]) == prim] if prim else []
        primary = pp[0] if pp else pairs[0]
        primaries[bank] = primary
        owned[(primary[0], primary[1])].append((primary[2], bank, primary[3]))
    suffix_of = {}
    for key, lst in owned.items():
        lst.sort()
        used = collections.Counter()
        for idx, bank, mode in lst:
            ev = key[1]
            base = ('presence_loop' if 'Loop' in mode else 'spawn') if ev == 'Create' else EVENTS.get(ev, '')
            n = used[base]
            used[base] += 1
            suffix_of[bank] = (base, '' if n == 0 else '_' + 'abcdefgh'[n])
    for bank, pairs in bank_pairs.items():
        primary = primaries[bank]
        otype, ev, idx, mode = primary
        kind, src, cons, prio = SOURCES[otype]
        if otype == 'CratePowerup':
            outcome = CRATE_OUTCOME[ev]
            aid = 'sfx.crate.outcome.%s' % outcome
            folder, stem = 'content/audio/sfx/crate_outcome', 'outcome_' + outcome
            cat = 'sfx'
            prio = 'fp' if outcome in FP_OUTCOMES else 'launch'
        else:
            evs, suffix = suffix_of[bank]
            if not evs:
                raise SystemExit('no Species event for %s.%s' % (otype, ev))
            if kind == 'ui' and ev == 'LobbyAmbience' or kind == 'ambience':
                cat = 'ambience'
                stem = ('lobby_bed' if ev == 'LobbyAmbience' else 'match_bed') + suffix
                aid = 'amb.%s' % stem
                stem = 'amb_' + stem
                folder = 'content/audio/ambience'
                prio = 'launch' if ev == 'LobbyAmbience' else 'fp'
            elif kind == 'ui':
                cat = 'ui_audio'
                pre = {'match_ui': '', 'power_slots': 'power_slot_', 'power_input': 'power_input_'}[src]
                stem = 'ui_' + pre + evs + suffix
                aid = 'sfx.ui.%s.%s' % (src, evs + suffix)
                folder = 'content/audio/ui'
            else:
                cat = 'sfx'
                stem = src + '_' + evs + suffix
                aid = 'sfx.%s.%s.%s' % (kind, src, evs + suffix)
                folder = 'content/audio/sfx/%s' % src
        if aid in bank_ids.values():
            raise SystemExit('duplicate audio asset id %s' % aid)
        bank_ids[bank] = aid
        mem = [s for s in members[bank] if s['event_liveness'] == 'live']
        own = [s for s in mem if s['name'] not in assigned]
        shared = [s for s in mem if s['name'] in assigned]
        durs = [float(s['duration_s']) for s in mem]
        loop = 'Loop' in mode
        spatial = '2-D (not positional)' if mode.startswith('2D') else (
            'positional, follows the source' if mode.startswith('Att') else 'positional, fixed where it started')
        voice = otype == 'CratePowerup' or ev in ('SeenThreatRunAway', 'SeenThreatAttack', 'InjuredEntity',
                                                  'Selected', 'GivenOrdersFromPlayer')
        ph = PLACEHOLDER['silence'] if cat == 'ambience' else (PLACEHOLDER['voice'] if voice else PLACEHOLDER['audio'])
        chans = 'mono' if not mode.startswith('2D') else 'mono or stereo'
        spec = '%s, 48 kHz; %s; %s; reference duration %s s' % (
            chans, spatial, 'seamless loop' if loop else 'one-shot',
            ('%.2f' % durs[0]) if len(durs) == 1 or min(durs) == max(durs) else '%.2f-%.2f' % (min(durs), max(durs)))
        long_audio = max(durs) > 20.0
        g = b.group(aid, category=cat, subcategory=folder.split('/')[-1], role='', consumers=docs(cons + ',aen,ast'),
                    priority=PRIO[prio], source_format='WAV, 48 kHz, 24-bit PCM, %s' % chans,
                    target_format=('Opus stream in the pack (decoded on a worker thread)' if long_audio
                                   else '16-bit PCM, 48 kHz, resident in the pack'),
                    colour_space='n/a', alpha_mode='n/a', placeholder=ph, brief_ref=BRIEF[cat], kind='audio',
                    spec=spec, sample_count=len(mem), loop=loop, spatial=spatial, voice=voice,
                    primary=(otype, ev))
        g['bindings'] = pairs
        g['shared_files'] = [assigned[s['name']] for s in shared]
        n = len(own)
        for i, s in enumerate(sorted(own, key=lambda s: s['name']), 1):
            fn = '%s/%s.wav' % (folder, stem) if n == 1 else '%s/%s_%02d.wav' % (folder, stem, i)
            b.add_file(aid, fn, [s['path']], ['live'], '%.2f s reference length' % float(s['duration_s']))
            assigned[s['name']] = fn
    # exclusions
    for s in samples:
        if s['name'] in assigned:
            continue
        if s['event_liveness'] == 'live':
            raise SystemExit('live sample not assigned: %s' % s['name'])
        if s['archive'].startswith('NOT-SHIPPED'):
            b.exclude(s['path'], 'music and loader tracks never triggered and never packed', 'audio')
        elif s['event_liveness'] == 'dead-data':
            b.exclude(s['path'], 'bound only to sound events that never fire', 'audio')
        else:
            b.exclude(s['path'], 'unused (bound to no sound event)', 'audio')
    return bank_ids, live_pairs


def describe_bindings(b, bank_ids):
    """Fill role text and the list of Species bindings of every audio group."""
    for bank, aid in bank_ids.items():
        g = b.groups[aid]
        binds = []
        for otype, ev, idx, mode in g['bindings']:
            kind, src, cons, prio = SOURCES[otype]
            if otype == 'CratePowerup':
                binds.append('crate.outcome.%s' % CRATE_OUTCOME[ev])
                continue
            evs = EVENTS.get(ev) or ('presence_loop' if 'Loop' in mode else 'spawn')
            if ev == 'Create':
                evs = 'presence_loop' if 'Loop' in mode else 'spawn'
            binds.append('%s.%s%s' % (src, evs, '' if idx == 0 else ' (layer %d)' % (idx + 1)))
        g['bound'] = binds
        otype, ev = g['primary']
        kind, src, cons, prio = SOURCES[otype]
        human = {'unit': 'unit', 'creature': 'creature', 'weapon': 'weapon', 'building': 'structure',
                 'crate': 'Supply crate', 'ui': 'interface', 'ambience': 'ambience'}[kind]
        if otype == 'CratePowerup':
            g['role'] = 'Announcement sting when a Supply crate yields %s' % CRATE_OUTCOME[ev].replace('_', ' ')
        else:
            g['role'] = 'Sample group for %s %s' % (src.replace('_', ' '), binds[0].split('.', 1)[1])
        if g['shared_files']:
            g['notes'].append('also plays %d file(s) owned by another group' % len(g['shared_files']))


def build_strings(b, p17b, p02, p17a):
    cats = collections.OrderedDict()   # (prefix, label) -> count
    out = collections.Counter()

    def add_in(prefix, label):
        cats[(prefix, label)] = cats.get((prefix, label), 0) + 1

    for r in p02:
        cls = None
        for pat, c in P02_KEY_OUT:
            if re.search(pat, r['key']):
                cls = c
        if cls:
            out[cls] += 1
        elif r['key'].startswith('multiwinia_team_'):
            add_in('match.team', 'team and faction names')
        else:
            add_in('mode', 'mode messages, objectives and HUD lines')
    for r in p17b:
        f = r['file'].split('/')[-1]
        cls = None
        for pat, c in P17B_KEY_OUT:
            if re.search(pat, r['id']):
                cls = c
                break
        state, prefix, label = P17B_FILE_CAT[f]
        if cls is None and state == 'out':
            cls = label
        if cls:
            out[cls] += 1
        else:
            add_in(prefix, label)
    for r in p17a:
        f = [x.split('/')[-1] for x in r['files'].split(';') if x][0]
        st = P17A_FILE_CAT.get(f)
        if st and st[0] == 'in':
            add_in(st[1], st[2])
        elif st:
            out[st[2]] += 1
        else:
            out['editor only (map editor windows)'] += 1
    total_in = sum(cats.values())
    for code, prio, lang in LANGUAGES:
        aid = 'strings.%s' % code.lower().replace('-', '_')
        b.group(aid, category='strings', subcategory='string_table',
                role='String table, %s' % lang, consumers=docs('options,ui_text,frontend,modes,help,ast'),
                priority=PRIO[prio], source_format='UTF-8 JSON, string id -> ICU MessageFormat text',
                target_format='cooked string table in the pack', colour_space='n/a', alpha_mode='n/a',
                placeholder=PLACEHOLDER['strings'], brief_ref=BRIEF['strings'], kind='strings',
                spec='every Species string id; about %d entries in the reference categories (ids collected in pass 2)' % total_in)
        b.add_file(aid, 'content/strings/%s.json' % code.lower().replace('-', '_'), [], [], 'BCP 47 tag %s' % code)
    return cats, out


def build_music(b):
    slots = [
        ('music.front_end.theme', 'content/audio/music/front_end_theme.wav', 'Front-end theme, loops under the menus and the lobby', 'launch', 'stereo, 48 kHz; seamless loop; 2-4 min'),
        ('music.match.bed', 'content/audio/music/match_bed_01.wav', 'In-match music bed, low intensity, sits under the crowd and the glow', 'launch', 'stereo, 48 kHz; seamless loop; 3-5 min; mixed below the effects bus'),
        ('music.match.bed', 'content/audio/music/match_bed_02.wav', '', 'launch', 'stereo, 48 kHz; seamless loop; 3-5 min'),
        ('music.match.bed', 'content/audio/music/match_bed_03.wav', '', 'launch', 'stereo, 48 kHz; seamless loop; 3-5 min'),
        ('music.match.sting_victory', 'content/audio/music/sting_victory.wav', 'Victory sting at the end of a match', 'launch', 'stereo, 48 kHz; one-shot; 5-10 s'),
        ('music.match.sting_defeat', 'content/audio/music/sting_defeat.wav', 'Defeat sting at the end of a match', 'launch', 'stereo, 48 kHz; one-shot; 5-10 s'),
    ]
    for aid, path, role, prio, spec in slots:
        b.group(aid, category='music', subcategory='music', role=role or 'In-match music bed', consumers=docs('audio,aen,ast'),
                priority=PRIO[prio], source_format='WAV, 48 kHz, 24-bit PCM, stereo',
                target_format='Opus stream in the pack (decoded on a worker thread)', colour_space='n/a',
                alpha_mode='n/a', placeholder=PLACEHOLDER['silence'], brief_ref=BRIEF['music'], kind='audio',
                spec=spec)
        b.add_file(aid, path, [], [], spec)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--drafts', required=True)
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--origin', required=True)
    ap.add_argument('--json', required=True)
    a = ap.parse_args()
    d = a.drafts.rstrip('/') + '/'
    tex = read_csv(d + 'X2a_textures.csv')
    shp = read_csv(d + 'X2a_shapes.csv')
    smp = read_csv(d + 'X2b_samples.csv')
    sev = read_csv(d + 'X2b_sound_events.csv')
    lev = read_csv(d + 'X4_levels.csv')
    thm = read_csv(d + 'X4_thumbnails.csv')
    mpl = read_csv(d + 'X4_mode_maplists.csv')
    p17b = read_csv(d + 'P17b_strings.csv')
    p02 = read_csv(d + 'P02_game_modes_strings.csv')
    p17a = read_csv(d + 'P17a_ui_framework_editor_ui_strings.csv')

    b = Builder()
    seen = build_textures(b, tex)
    build_terrain(b, tex, seen)
    build_help(b, tex, seen)
    plan = build_thumbs_and_maps(b, tex, lev, seen)
    build_fonts(b, tex, seen)
    # remaining textures: exclusions
    for r in tex:
        if r['path'] in seen:
            continue
        rel = r['path'][len('data/'):]
        if r['reach'] == 'FULL':
            if rel not in TEX_EXCLUDE_FULL:
                raise SystemExit('FULL texture with no decision: %s' % rel)
            b.exclude(r['path'], TEX_EXCLUDE_FULL[rel], 'texture')
        else:
            b.exclude(r['path'], reach_class(r['reach']), 'texture')
    build_meshes(b, shp)
    bank_ids, live_pairs = build_audio(b, smp, sev)
    describe_bindings(b, bank_ids)
    build_music(b)
    str_cats, str_out = build_strings(b, p17b, p02, p17a)
    # level files outside the plan
    planned = set(r['file'] for _, _, _, r in plan)
    for r in lev:
        if r['file'] in planned:
            continue
        if r['kind'] in ('map', 'mission'):
            cls = 'campaign, single-player or unreachable code path'
        elif r['dir'] == 'levels/old':
            cls = 'unused (never listed: superseded map files)'
        else:
            cls = 'cut or test map'
        b.exclude(r['file'], cls, 'map')

    # cross-check thumbnails CSV
    thumb_in = set(f['originals'][0].lower() for g in b.groups.values() if g['category'] == 'maps'
                   and g['subcategory'] == 'thumbnail' for f in g['files'])
    for t in thm:
        p = t['file'].lower()
        if t['level_file'] in planned and p not in thumb_in:
            raise SystemExit('retail thumbnail not mapped: %s' % t['file'])

    # mode/player distribution from the menu lists (retail)
    dist = collections.Counter()
    for r in mpl:
        if r['scope'] == 'retail' and r['gametype'] in ORIG_MODE:
            dist[(ORIG_MODE[r['gametype']], int(r['numplayers']))] += 1
    plan_dist = collections.Counter((m, p) for m, p, i, r in plan)
    if dist != plan_dist:
        raise SystemExit('map plan does not follow the menu-list distribution: %s vs %s' % (dist, plan_dist))

    # write the manifest
    cols = ['asset_id', 'category', 'subcategory', 'consumers', 'role', 'target_format', 'spec', 'colour_space',
            'alpha_mode', 'variants', 'priority', 'placeholder', 'brief_ref', 'species_filename', 'source_format']
    with open(a.manifest, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(cols)
        for g in b.groups.values():
            n = len(g['files'])
            for i, fobj in enumerate(g['files'], 1):
                var = 'single file' if n == 1 else 'file %d of %d' % (i, n)
                if g.get('shared_files'):
                    var += '; group also plays %d file(s) of another group' % len(g['shared_files'])
                spec = fobj.get('spec') or g.get('spec', '')
                if g['kind'] == 'audio' and fobj.get('spec'):
                    spec = g['spec'] + '; ' + fobj['spec']
                w.writerow([g['asset_id'], CATEGORY_TITLE[g['category']], g['subcategory'],
                            ';'.join(g['consumers']), g['role'] or g.get('role_hint', ''), g['target_format'],
                            spec, g['colour_space'], g['alpha_mode'], var, fobj.get('priority', g['priority']),
                            g['placeholder'], g['brief_ref'], fobj['path'], g['source_format']])
    # origin map
    with open(a.origin, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(['species_filename', 'asset_id', 'original_paths', 'original_count', 'original_status'])
        for o in b.origin:
            w.writerow([o['species_filename'], o['asset_id'], o['original_paths'], o['original_count'],
                        o['original_status']])
        for orig, cls, kind in b.excluded:
            w.writerow(['(not created: %s)' % cls, '', orig, 1, kind])
        for (prefix, label), n in str_cats.items():
            w.writerow(['content/strings/en_gb.json', 'strings.en_gb', 'reference text entries: %s' % label, n,
                        'player-facing'])
        for cls, n in str_out.items():
            w.writerow(['(not created: %s)' % cls, '', 'reference text entries', n, 'string'])
    # JSON for the document generator
    data = dict(groups=list(b.groups.values()), excluded=b.excluded,
                str_cats=[[p, l, n] for (p, l), n in str_cats.items()], str_out=list(str_out.items()),
                silent_events=sum(1 for e in sev if e['in_data'] == 'no' and e['fired_in_FULL'] == 'yes'),
                live_pairs=len(live_pairs), languages=LANGUAGES)
    with open(a.json, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=1)
    # summary
    cat_files = collections.Counter()
    for g in b.groups.values():
        for fo in g['files']:
            cat_files[g['category']] += 1
    print('groups', len(b.groups), 'files', len(b.files), 'excluded', len(b.excluded))
    for k, v in cat_files.items():
        print('  %-10s %d' % (k, v))
    print('exclusions by class:')
    for (cls, kind), n in sorted(collections.Counter((c, k) for _, c, k in b.excluded).items()):
        print('  %-8s %4d  %s' % (kind, n, cls))


if __name__ == '__main__':
    main()
