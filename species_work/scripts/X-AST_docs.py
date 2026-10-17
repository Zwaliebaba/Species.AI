#!/usr/bin/env python3
"""X-AST pass 1: write GameAssets.md and README.md from the builder's JSON.

Usage:
  python3 -I X-AST_docs.py --json <scratch/xast.json> --out <NEXT/assets/>

The JSON holds original paths; this script never writes them.  It writes only Species
filenames, asset ids, specs and counts.
"""
import argparse
import collections
import json
import os
import re

PRIO_ORDER = ['skirmish-first-playable', 'launch', 'post-launch']
CAT_ORDER = ['sprites', 'vfx', 'ui', 'hud', 'cursors', 'glyphs', 'terrain', 'meshes', 'help', 'sfx', 'ui_audio',
             'ambience', 'music', 'fonts', 'strings', 'maps']
CAT_TITLE = {
    'sprites': 'Unit sprites', 'vfx': 'VFX textures', 'ui': 'UI panels and title',
    'hud': 'HUD, supply-crate and power icons', 'cursors': 'Cursors and markers', 'glyphs': 'Input glyphs',
    'terrain': 'Terrain ramps, water and waves', 'meshes': 'Meshes', 'help': 'Help images and explainer animations',
    'sfx': 'Audio by source and event', 'ui_audio': 'UI sounds', 'ambience': 'Ambience', 'music': 'Music',
    'fonts': 'Fonts', 'strings': 'String table', 'maps': 'Maps and thumbnails',
}
SECTION = {c: i + 4 for i, c in enumerate(CAT_ORDER)}

SHORT_FMT = {
    'PNG RGBA8, sRGB; cut-out alpha painted by the artist': 'PNG RGBA8 (cut-out)',
    'PNG RGBA8, sRGB; straight alpha': 'PNG RGBA8',
    'PNG 8-bit greyscale, linear intensity on black': 'PNG grey (additive)',
    'PNG 8-bit greyscale mask, white = opaque': 'PNG grey (mask)',
    'PNG 8-bit greyscale, white = full darkening': 'PNG grey (darkening)',
    'PNG RGBA8, sRGB; white or grey art with straight alpha (tinted at draw)': 'PNG RGBA8 (white art)',
    'PNG RGBA8, sRGB; team area painted pure red (255,0,0)': 'PNG RGBA8 (team red)',
    'PNG RGB8, sRGB; opaque': 'PNG RGB8',
    'PNG RGB8, sRGB; tileable along its long axis': 'PNG RGB8 (tileable)',
    'PNG RGB8, sRGB; look-up table, no filtering artefacts at the edges': 'PNG RGB8 (table)',
    'PNG RGB8, sRGB; seamless tile': 'PNG RGB8 (seamless)',
    'PNG RGB8, sRGB; rendered by the map tool': 'PNG RGB8 (map tool)',
}
SHORT_TGT = {
    'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain': 'BC7 sRGB, mips',
    'DDS (DX10 header) BC4_UNORM, full mip chain': 'BC4, mips',
    'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain; team area recoloured per team at load': 'BC7 sRGB, mips, per-team',
    'DDS (DX10 header) BC7_UNORM_SRGB, no mips': 'BC7 sRGB, no mips',
    'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain, wrap repeat': 'BC7 sRGB, mips, repeat',
    'uncompressed R8G8B8A8_UNORM_SRGB table in the pack, no mips; read on the CPU': 'RGBA8 sRGB table, read on the CPU',
}
PLACEHOLDER_DEF = [
    ('generated figure', 'flat-colour figure or shape that fills the role silhouette, with an alpha outline'),
    ('generated gradient', 'white-on-black radial or linear gradient of the stated size'),
    ('generated glyph', 'white geometric glyph (circle, arrow or ring) with the initials of the asset id'),
    ('generated panel', 'flat panel of the stated size with the asset id printed on it'),
    ('generated ramp', 'two-axis colour gradient (slope by height)'),
    ('generated noise tile', 'seamless value-noise tile'),
    ('diagram card', 'card with the asset id and the page number'),
    ('static mode card', 'still card that shows the mode name and the explainer id'),
    ('auto-rendered capture', 'top-down capture rendered by the map tool from the map that exists (a placeholder map until the layout is final)'),
    ('primitive mesh with sockets', 'boxes, cylinders and cones at the bounding size of the role, with every socket of the row present'),
    ('synthesised tone', 'sine or noise burst of the stated duration, looping where the row says loop'),
    ('text-to-speech line', 'synthetic voice line of the stated duration'),
    ('silence', 'silent file of the stated length, looping where the row says loop'),
    ('fallback font', 'an open-licence font with the same script coverage'),
    ('pseudo-localised text', 'text generated from each string id, padded to test layout'),
    ('generated test map', 'flat map with the objects the mode needs for its player count'),
]
SRC_NAME = {
    'worker': 'Worker', 'leader': 'Leader', 'trooper': 'Trooper', 'builder': 'Builder', 'transport': 'Transport',
    'collector': 'Collector', 'bomber': 'Bomber', 'essence_orb': 'Essence orb', 'crawler': 'Crawler',
    'centipede': 'centipede', 'spider': 'spider', 'ant': 'ant', 'reaper_serpent': 'Reaper Serpent',
    'brood_drifter': 'Brood Drifter', 'abductor': 'Abductor', 'seed_pod': 'Seed Pod',
    'barrage_missile': 'Missile Barrage missile', 'meteor': 'meteor', 'laser_bolt': 'laser bolt',
    'grenade': 'grenade', 'rocket': 'rocket', 'airstrike_bomb': 'airstrike bomb', 'beam_dish': 'Beam Dish',
    'barrier_emitter': 'Barrier Emitter', 'turret': 'turret', 'tree': 'tree', 'grid_pylon': 'grid pylon',
    'solar_array': 'Solar Array', 'hive': 'Hive', 'spitter_plant': 'Spitter Plant', 'spawn_node': 'Spawn Node',
    'fuel_pump': 'fuel pump', 'fuel_line': 'fuel line', 'fuel_depot': 'fuel depot', 'launch_vessel': 'Launch Vessel',
    'control_zone': 'Control Zone', 'command_console': 'Command Console', 'relic': 'Relic',
    'core_device': 'Core Device', 'supply_crate': 'Supply crate', 'match_ui': 'match interface',
    'power_slots': 'power-slot panel', 'power_input': 'power input', 'match': 'match',
}
EVENT_DESC = {
    'attack': 'attacks', 'die': 'dies', 'hurt': 'takes damage', 'turn': 'changes direction',
    'weapon_ready': 'weapon is ready again', 'fire_laser': 'fires its laser', 'enter_gate': 'enters a gate',
    'exit_gate': 'leaves a gate', 'lay_egg': 'lays an egg', 'pounce': 'pounces', 'footstep': 'footstep',
    'ordered': 'receives an order', 'flee_bark': 'flees from a threat (voice)', 'victory_jump': 'jumps in victory',
    'burning': 'burns', 'selected': 'is selected (voice)', 'ordered_by_player': 'is ordered by the player (voice)',
    'lift_relic': 'lifts a Relic', 'attack_bark': 'charges a threat (voice)', 'attack_horn': 'sounds the attack',
    'order_waypoint': 'is given a waypoint order', 'order_formation': 'is given a formation order',
    'enemy_sighted': 'sights an enemy', 'bounce': 'bounces', 'burst_open': 'bursts open',
    'drop_essence': 'drops Essence orbs', 'explode': 'explodes', 'land': 'lands', 'tractor_beam': 'tractor beam loop',
    'drop_bomb': 'drops a bomb', 'rotate_start': 'starts turning', 'rotate_stop': 'stops turning',
    'link_up': 'link established', 'link_lost': 'link lost', 'spark': 'sparks', 'damaged': 'is damaged',
    'fire_gun': 'fires', 'fire_flame': 'fires flame', 'kill_tick': 'registers a kill',
    'ambient_normal': 'ambient loop', 'ambient_essence': 'Essence Grove ambient loop',
    'ambient_blight': 'Blight Grove ambient loop', 'surge': 'energy surge passes', 'charging': 'charging loop',
    'activate': 'activates', 'deactivate': 'deactivates', 'pump_up': 'pump stroke up',
    'pump_down': 'pump stroke down', 'flow': 'fuel flow loop', 'load_passenger': 'loads a passenger',
    'fuelled': 'fuelled loop', 'hungry': 'needs fuel loop', 'malfunction': 'malfunction loop',
    'engine_burn': 'engine burn', 'flight': 'launch flight loop', 'relic_scored': 'a Relic is scored',
    'owner_changed': 'changes owner', 'capture_begin': 'capture starts', 'capture_lost': 'capture lost',
    'capture_progress_00': 'capture under way, 0-20 %', 'capture_progress_20': 'capture under way, 20-40 %',
    'capture_progress_40': 'capture under way, 40-60 %', 'capture_progress_60': 'capture under way, 60-80 %',
    'capture_progress_80': 'capture under way, 80-100 %', 'captured': 'is captured', 'falling': 'falls from the sky',
    'drop': 'is dropped', 'hit_bark': 'a unit is hit by a laser bolt (voice of the hit unit)',
    'fuse_blink': 'fuse blink', 'collected': 'is collected', 'begin': 'power input starts',
    'confirm': 'power input succeeds', 'bed': 'match ambience bed', 'discard': 'a power is discarded',
    'icon_hover': 'pointer over an icon', 'countdown_tick': 'countdown tick', 'countdown_zero': 'countdown reaches zero',
    'selection_circle': 'selection circle drawn', 'lobby_bed': 'lobby ambience bed', 'menu_hover': 'pointer over a menu item',
    'menu_select': 'menu item chosen', 'menu_back': 'menu back', 'unit_highlight': 'unit highlighted',
    'unit_select': 'unit selected', 'error': 'invalid action', 'toggle_state': 'unit state toggled',
    'chat_message': 'chat message arrives', 'presence_loop': 'loop while it exists', 'spawn': 'appears',
}


def esc(s):
    return str(s).replace('|', '\\|')


def pattern(paths):
    """Compress a list of files into a numbered pattern when they are <prefix>_01 .. <prefix>_NN."""
    if len(paths) == 1:
        return '`%s`' % paths[0], 1
    m = [re.match(r'^(.*)_(\d\d)((?:_[a-z]+)?\.\w+)$', p) for p in paths]
    if all(m) and len(set((x.group(1), x.group(3)) for x in m)) == 1:
        nums = sorted(int(x.group(2)) for x in m)
        if nums == list(range(1, len(nums) + 1)):
            return '`%s_{01..%02d}%s` (%d files)' % (m[0].group(1), len(nums), m[0].group(3), len(nums)), len(nums)
    return '<br>'.join('`%s`' % p for p in paths), len(paths)


def fmt_pair(g):
    s = SHORT_FMT.get(g['source_format'], g['source_format'])
    t = SHORT_TGT.get(g['target_format'], g['target_format'])
    return '%s → %s' % (s, t)


def prio_of(g):
    ps = collections.Counter(f.get('priority', g['priority']) for f in g['files'])
    if len(ps) == 1:
        return next(iter(ps))
    return '<br>'.join('%s: %d' % (p, ps[p]) for p in PRIO_ORDER if ps[p])


def variants_of(g):
    n = len(g['files'])
    if g.get('kind') == 'audio':
        sh = len(g.get('shared_files', []))
        txt = '%d variant%s' % (n, '' if n == 1 else 's') if n else 'no own file'
        if sh:
            txt += ' + %d shared' % sh
        return txt
    return 'single' if n == 1 else '%d' % n


def texture_rows(groups):
    out = ['| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |',
           '|---|---|---|---|---|---|---|---|']
    for g in groups:
        specs = set(f['spec'] for f in g['files'])
        pat, n = pattern([f['path'] for f in g['files']])
        compressed = len(g['files']) == 1 or pat.count('`') == 2
        if not compressed or (len(specs) > 1 and g['category'] == 'help'):
            for i, f in enumerate(g['files'], 1):
                out.append('| `%s` | `%s` | %s | %s | %s | %d of %d | %s | %s |' % (
                    f['path'], g['asset_id'], esc(g['role']) if i == 1 else '″', fmt_pair(g), esc(f['spec']), i,
                    len(g['files']), f.get('priority', g['priority']), g['placeholder']))
            continue
        spec = g['files'][0]['spec'] if len(specs) == 1 else g['spec']
        out.append('| %s | `%s` | %s | %s | %s | %s | %s | %s |' % (
            pat, g['asset_id'], esc(g['role']), fmt_pair(g), esc(spec), variants_of(g), prio_of(g), g['placeholder']))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--json', required=True)
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    d = json.load(open(a.json, encoding='utf-8'))
    groups = d['groups']
    by_cat = collections.OrderedDict((c, []) for c in CAT_ORDER)
    for g in groups:
        by_cat[g['category']].append(g)
    files_by = {c: sum(len(g['files']) for g in gs) for c, gs in by_cat.items()}
    prio_by = {c: collections.Counter(f.get('priority', g['priority']) for g in gs for f in g['files'])
               for c, gs in by_cat.items()}
    orig_by = {c: sum(len(f['originals']) for g in gs for f in g['files']) for c, gs in by_cat.items()}
    total_files = sum(files_by.values())
    total_ids = len(groups)
    L = []
    w = L.append

    # ---------------------------------------------------------------- header and §1
    w('# Species game assets')
    w('')
    w('> Status: Asset list, pass 1 (consumers named by document; pass 2 replaces them with requirement IDs) · '
      'Related: [assets README](README.md) · [asset_manifest.csv](asset_manifest.csv) · '
      '[art_and_audio_briefs.md](art_and_audio_briefs.md) · [CONVENTIONS](../CONVENTIONS.md)')
    w('')
    w('## Contents')
    w('')
    toc = ['1 Purpose and the newly-authored rule', '2 Filename convention and formats', '3 Summary']
    toc += ['%d %s' % (SECTION[c], CAT_TITLE[c]) for c in CAT_ORDER] + ['20 Not to be created']
    for t in toc:
        anchor = re.sub(r'[^a-z0-9 -]', '', t.lower()).replace(' ', '-')
        w('- [%s](#%s)' % (t, anchor))
    w('')
    w('## 1 Purpose and the newly-authored rule')
    w('')
    w('This document lists every asset Species must create so that the mechanics, look and feel of the reference '
      'library run with Species content. It covers what a skirmish match of the reference game reaches: units, '
      'creatures and hazards, structures and mode objects, Supply crates and powers, the HUD and the front end, '
      'help and explainer animations, sound effects, ambience, fonts, player-facing text and maps. It adds music, '
      'which the reference does not have. The persistent world adds further needs (world-scale terrain, '
      'settlements, a strategic zoom level); the design phase adds those to the manifest.')
    w('')
    w('**Every asset is newly authored.** Each file is made from scratch to the same technical role as its '
      'reference counterpart: size class, alpha mode, channel layout, loop behaviour, socket names and variant '
      'count. Nothing is traced, re-coloured, resampled or edited from the reference game\'s art, audio or text, '
      'and no reference file serves as a starting point, a paint-over base or a placeholder. Character silhouettes, '
      'textures, UI layout, logo and wordmark are new; the style stays the same (vision O9, '
      '[art_and_audio_briefs.md](art_and_audio_briefs.md)).')
    w('')
    w('How to read the tables: one row per asset id. A row names its file, or an explicit numbered pattern with its '
      'count, for example `content/audio/sfx/worker/worker_attack_{01..24}.wav` (24 files). '
      '[asset_manifest.csv](asset_manifest.csv) expands every row to one line per file, with the full format '
      'strings and the consuming documents.')
    w('')

    # ---------------------------------------------------------------- §2 convention
    w('## 2 Filename convention and formats')
    w('')
    w('### 2.1 Content root and folders')
    w('')
    w('All assets live under the content root `content/`. The content build rejects any file outside it.')
    w('')
    w('| Folder | Holds |')
    w('|---|---|')
    for f, h in [
        ('`content/sprites/`', 'unit, creature and effect cards (camera-facing billboards)'),
        ('`content/textures/fx/`', 'VFX textures: glows, beams, flares, sky clouds, wire grid, canopy mask'),
        ('`content/textures/ui/`', 'UI panels, title wordmark, logos, front-end decoration, small UI icons'),
        ('`content/textures/icons/power/`', 'power and Supply-crate outcome icons'),
        ('`content/textures/icons/hud/`', 'power-slot panel and status read-outs'),
        ('`content/textures/icons/cursors/`', 'world and menu cursors'),
        ('`content/textures/icons/markers/`', 'off-screen markers, ground markers, banners and flags'),
        ('`content/textures/icons/input/`', 'platform-neutral input glyphs'),
        ('`content/textures/terrain/`', 'terrain colour ramps, sea tiles, shore-wave gradients'),
        ('`content/textures/help/`', 'hint pictures, help pages, mode cards'),
        ('`content/textures/help/explainer/`', 'pictures animated by the explainer scripts'),
        ('`content/ui/explainers/`', 'explainer animation scripts, one per mode'),
        ('`content/meshes/<group>/`', 'flat-shaded meshes; groups `units`, `creatures`, `structures`, `projectiles`, `relics`, `scenery`'),
        ('`content/audio/sfx/<source>/`', 'sound effects, one folder per sound source; `crate_outcome/` holds the outcome stings'),
        ('`content/audio/ui/`', 'interface sounds'),
        ('`content/audio/ambience/`', 'ambience beds'),
        ('`content/audio/music/`', 'music (new)'),
        ('`content/fonts/`', 'licensed font files'),
        ('`content/strings/`', 'string tables, one per language'),
        ('`content/maps/`', 'map definitions, one per map slot'),
        ('`content/thumbnails/`', 'map thumbnails, one per map'),
    ]:
        w('| %s | %s |' % (f, h))
    w('')
    w('### 2.2 File names')
    w('')
    w('- Lower-case snake_case, named by **role**. A category prefix makes each stem unique across the pack: '
      '`fx_`, `ui_`, `power_`, `hud_`, `cursor_`, `marker_`, `glyph_`, `terrain_`, `sea_`, `shore_`, `hint_`, '
      '`help_`, `mode_card_`, `explainer_`. Sprites and meshes start with the unit or structure name. Audio files '
      'start with their source (`worker_die_01.wav`), UI sounds with `ui_`, ambience with `amb_`, outcome stings '
      'with `outcome_`.')
    w('- Numbered variants end in `_NN`, two digits, counting from 01 without gaps. A row with a numbered pattern '
      'is one asset id with interchangeable variants, unless its spec gives each number a meaning (states, pages, '
      'themes).')
    w('- No stem repeats a reference file stem, and no name contains an IP word. Ids and names come from '
      '[CONVENTIONS §8](../CONVENTIONS.md#8-naming).')
    w('- String tables are named by their BCP 47 language tag in lower case with `_` for `-` (`en_gb.json`); the '
      'tag itself is stored in the file.')
    w('- Maps follow `map_<mode>_<players>p_<nn>`; a thumbnail adds `_thumb`.')
    w('')
    w('### 2.3 Asset ids')
    w('')
    w('Asset ids are dotted and lower case. Numbered variants share one id; documents cite ids, never file names.')
    w('')
    w('| Id root | Used for | Example |')
    w('|---|---|---|')
    for r, u, e in [
        ('`sprite.`', 'sprite cards', '`sprite.worker.front`'), ('`fx.`', 'VFX textures', '`fx.glow_round`'),
        ('`ui.`', 'UI panels and title art', '`ui.title_wordmark`'), ('`icon.power.`, `icon.hud.`', 'power and HUD icons', '`icon.power.airstrike`'),
        ('`cursor.`, `marker.`', 'cursors and markers', '`marker.edge_compass`'), ('`glyph.`', 'input glyphs', '`glyph.pad.face_south`'),
        ('`terrain.`', 'ramps, sea tiles, wave gradients', '`terrain.ramp`'), ('`mesh.<group>.`', 'meshes', '`mesh.structure.spawn_node`'),
        ('`help.`, `anim.explainer`', 'help pictures and explainer scripts', '`help.hint.supply_crates`'),
        ('`sfx.<kind>.<source>.<event>`', 'sound-effect sample groups; kind is `unit`, `creature`, `weapon`, `building` or `crate`', '`sfx.unit.worker.attack`'),
        ('`sfx.crate.outcome.`', 'Supply-crate outcome stings', '`sfx.crate.outcome.shield`'), ('`sfx.ui.`', 'UI sounds', '`sfx.ui.match_ui.menu_select`'),
        ('`amb.`', 'ambience beds', '`amb.match_bed`'), ('`music.`', 'music', '`music.front_end.theme`'),
        ('`font.`', 'fonts', '`font.ui.primary`'), ('`strings.`', 'string tables', '`strings.en_gb`'),
        ('`map.`, `thumb.`', 'maps and thumbnails, one id per mode', '`map.spawn_control`'),
    ]:
        w('| %s | %s | %s |' % (r, u, e))
    w('')
    w('### 2.4 Source and runtime formats')
    w('')
    w('The artist delivers the source file under `content/`; the content build generates the runtime file in the '
      'pack. Every cooked texture is a DDS with the DX10 header; block-compressed textures are padded to whole '
      '4x4 blocks by the cooker. UI and HUD art is authored at twice its display size and reduced by the cooker; '
      'world textures are authored at the stated size. Audio sources are 48 kHz.')
    w('')
    w('| Profile | Source (artist delivers) | Runtime (content build generates) | Colour space | Alpha mode |')
    w('|---|---|---|---|---|')
    prof_rows = [
        ('cut-out sprite', 'PNG RGBA8, sRGB, cut-out alpha painted by the artist', 'BC7_UNORM_SRGB, full mips', 'sRGB', 'cut-out: alpha test at the consumer threshold (0.04 Workers, 0.02 other units)'),
        ('alpha sprite or picture', 'PNG RGBA8, sRGB, straight alpha', 'BC7_UNORM_SRGB, full mips (world) or no mips (UI)', 'sRGB', 'straight alpha blend'),
        ('additive VFX', 'PNG 8-bit greyscale, intensity on black', 'BC4_UNORM, full mips', 'linear', 'additive: black adds nothing, colour from the draw tint'),
        ('alpha mask', 'PNG 8-bit greyscale, white = opaque', 'BC4_UNORM, full mips', 'linear', 'mask as alpha over white, tinted at draw'),
        ('darkening layer', 'PNG 8-bit greyscale, white = full darkening', 'BC4_UNORM, full mips', 'linear', 'destination multiplied by (1 - texel)'),
        ('tinted icon', 'PNG RGBA8, sRGB, white or grey art', 'BC7_UNORM_SRGB, full mips', 'sRGB', 'straight alpha, tinted by vertex colour'),
        ('team cloth', 'PNG RGBA8, sRGB, team area painted pure red (255,0,0)', 'BC7_UNORM_SRGB, full mips, one recolour per team at load', 'sRGB', 'straight alpha; pure-red texels take the team colour'),
        ('UI picture', 'PNG RGB8 or RGBA8, sRGB', 'BC7_UNORM_SRGB, no mips (tiled strips keep mips and repeat)', 'sRGB', 'opaque or straight alpha'),
        ('CPU look-up table', 'PNG RGB8, sRGB', 'uncompressed R8G8B8A8_UNORM_SRGB, no mips, read on the CPU', 'sRGB', 'none'),
        ('sea tile', 'PNG RGB8, sRGB, seamless', 'BC7_UNORM_SRGB, full mips, wrap repeat', 'sRGB', 'opaque, multiplied by the sea light map'),
        ('thumbnail', 'PNG RGB8, sRGB, rendered by the map tool', 'BC7_UNORM_SRGB, no mips', 'sRGB', 'opaque'),
        ('mesh', 'glTF 2.0 binary `.glb`: metres, +Y up, no `NORMAL` (flat faces), `COLOR_0` per-face colour, named socket nodes', 'cooked Species mesh: face normals generated, colours attenuated at load', 'sRGB vertex colour', 'opaque; team parts painted mostly red'),
        ('sound effect', 'WAV, 48 kHz, 24-bit PCM, mono (2-D sounds may be stereo)', '16-bit PCM, 48 kHz, resident', 'n/a', 'n/a'),
        ('long audio and music', 'WAV, 48 kHz, 24-bit PCM, stereo for music', 'Opus stream, decoded on a worker thread', 'n/a', 'n/a'),
        ('font', 'OpenType font licensed for embedding', 'font file in the pack; glyphs rasterised at run time into R8_UNORM cache pages', 'n/a', 'coverage'),
        ('string table', 'UTF-8 JSON: string id to ICU MessageFormat text', 'cooked string table', 'n/a', 'n/a'),
        ('map', 'map definition from the map tool (text, Species map schema)', 'cooked map definition', 'n/a', 'n/a'),
        ('explainer script', 'JSON key-frame script that animates pictures by asset id', 'cooked explainer script', 'n/a', 'n/a'),
    ]
    for r in prof_rows:
        w('| %s | %s | %s | %s | %s |' % r)
    w('')
    w('### 2.5 Priorities and the first-playable assumption')
    w('')
    w('Each row has one priority: `skirmish-first-playable`, `launch` or `post-launch`.')
    w('')
    w('**First-playable assumption** (vision O8: one skirmish mode first). The first playable is **Spawn Control** '
      'for 2 players on the map slots `map_spawn_control_2p_01` and `map_spawn_control_2p_02`, with bots, and '
      'with Supply crates drawing from a core outcome pool: Reinforcements, Strike team (with its grenade and '
      'rocket weapons), Transport, airstrike, gun turret, Shield, Speed Boost and Fury. It needs:')
    w('')
    w('- units: Worker (all cards), Leader with its banners, Trooper, Transport, Bomber, Essence orbs;')
    w('- structures: Spawn Node, Relay, Transit Gate, gun turret (mount, head, gun barrel), Barrier Emitter, wall, '
      'tree, jump pad, Supply crate;')
    w('- UI: title wordmark, panels, menu highlight, check box, every cursor, the markers, the power-slot HUD for '
      'the core pool, the keyboard Tab glyph, the Spawn Control mode card, the primary font (Latin and Cyrillic) '
      'and the English string table;')
    w('- world: sky clouds, terrain ramp 01, sea tile 01, shore-wave gradient 01, the VFX of those units;')
    w('- audio: every sample group of those sources and weapons, the match interface sounds and the match '
      'ambience bed.')
    w('')
    w('Everything else that a match reaches is `launch`. `post-launch` holds the seasonal theme, alternative '
      'mesh bodies and the few rows whose role still needs confirmation. The design phase may move rows between '
      'priorities; the placeholder rule keeps the game building and running whatever is missing.')
    w('')
    w('### 2.6 Placeholders')
    w('')
    w('The content build generates a stand-in for every manifest row that has no authored file yet, at the same '
      'runtime path, so the game always builds and runs. Authored files replace stand-ins one by one.')
    w('')
    w('| Placeholder | What the content build generates |')
    w('|---|---|')
    for k, v in PLACEHOLDER_DEF:
        w('| %s | %s |' % (k, v))
    w('')

    # ---------------------------------------------------------------- §3 summary
    w('## 3 Summary')
    w('')
    w('| Category | Asset ids | Files | first-playable | launch | post-launch | Reference files replaced |')
    w('|---|---:|---:|---:|---:|---:|---:|')
    for c in CAT_ORDER:
        p = prio_by[c]
        w('| [%s](#%s) | %d | %d | %d | %d | %d | %d |' % (
            CAT_TITLE[c], ('%d-' % SECTION[c]) + re.sub(r'[^a-z0-9 -]', '', CAT_TITLE[c].lower()).replace(' ', '-'),
            len(by_cat[c]), files_by[c], p['skirmish-first-playable'], p['launch'], p['post-launch'], orig_by[c]))
    tp = collections.Counter()
    for c in CAT_ORDER:
        tp.update(prio_by[c])
    w('| **Total** | **%d** | **%d** | **%d** | **%d** | **%d** | **%d** |' % (
        total_ids, total_files, tp['skirmish-first-playable'], tp['launch'], tp['post-launch'], sum(orig_by.values())))
    w('')
    w('"Reference files replaced" counts the in-scope reference files whose role each category takes over; one '
      'Species file may replace several (138 bitmap font pages become 5 font files). Music, string tables and '
      'some help pictures have no one-to-one counterpart.')
    w('')

    # ---------------------------------------------------------------- texture categories
    intro = {
        'sprites': 'Camera-facing cards for the crowd and for small creatures and effects. Worker cards are white or light grey so that the team colour comes from the vertex tint; the renderer scales them with camera distance and turns them into billboards far away.',
        'vfx': 'Additive textures painted on black: overlapping layers saturate toward white and draw order does not matter. They are greyscale; colour comes from the draw tint (team colour, effect colour).',
        'ui': 'Front-end and window art. The palette is new; it keeps the token roles of the reference (an accent panel, a neutral panel, a highlight). UI pictures keep exact pixels: no mip-maps, linear filtering.',
        'hud': 'Power and Supply-crate outcome icons and the power-slot panel read-outs. Icons are white or grey art tinted at draw, drawn over a darkening halo; the same icon shows on the crate, in the power slot, on the cursor and on off-screen markers.',
        'cursors': 'World and menu cursors, off-screen and ground markers, Leader banners and Flag Chain flags. The runtime blurs cursors and markers into darkening shadows; banners and flags paint the team area pure red.',
        'glyphs': 'Platform-neutral input glyphs: shapes and labels describe the position of a control, never a console brand or trade dress.',
    }
    for c in ['sprites', 'vfx', 'ui', 'hud', 'cursors', 'glyphs']:
        w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
        w('')
        w(intro[c] + ' Brief: [art_and_audio_briefs.md](%s).' % by_cat[c][0]['brief_ref'])
        w('')
        L.extend(texture_rows(by_cat[c]))
        w('')

    # ---------------------------------------------------------------- terrain
    c = 'terrain'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('Ramps and wave gradients are CPU look-up tables, not GPU textures: the terrain builder reads a ramp by '
      'slope and height to colour each terrain triangle, and the sea renderer reads one wave-gradient row. Sea '
      'tiles are GPU textures, repeated and multiplied by the sea light map. Each map names one ramp, one sea tile '
      'and one wave gradient; number 01 of each is the map default. The seasonal theme forces ramp 06, sea tile 02 '
      'and wave gradient 04. Theme numbers carry no fixed biome; the environment brief assigns one look per number.')
    w('')
    L.extend(texture_rows(by_cat[c]))
    w('')

    # ---------------------------------------------------------------- meshes
    c = 'meshes'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('Flat-shaded low-poly meshes with per-face colours. Parts that take the team colour are painted '
      '"mostly red" (red more than twice green and twice blue) on the root node and its direct children; the '
      'renderer recolours them. Code fetches sockets by name, so every socket of a row must exist with exactly '
      'that name, as an empty node in the `.glb`. Budgets are the triangle counts of the reference meshes and are '
      'upper limits. Formats: `.glb` (glTF 2.0 binary) → cooked Species mesh; placeholder: primitive mesh with '
      'sockets.')
    w('')
    w('**Socket vocabulary.** `port_NN` manning slot where a Worker stands to operate a structure; `light_NN` lamp '
      'or flare; `status_NN` status light per port; `muzzle_NN` barrel end where shots start; `casing_eject_NN` '
      'where spent casings leave; `barrel_mount` where a barrel attaches; `kill_counter` kill read-out; `carry` '
      'carry point; `leg_NN` leg root; `segment_end` end of a leg part; `engine_NN` engine glow; `essence_link` '
      'end point of an essence link; `tail_NN` tail attachment; `gate_exit_NN` destination point of a gate; '
      '`gate_surface` gate surface centre; `gate_entrance` where units enter; `door` exit for spawned units; '
      '`fuel_point` fuel connection; `glow_NN` panel glow; `grab_NN` lift points; `egg_spawn` and `egg_lay` egg '
      'release points; `entrance` boarding point; `flag` flag pole tip; `grid_link` energy-grid connection; '
      '`beam_focus` beam origin; `window_NN` lit window; `thruster` engine exhaust; `field_anchor_NN` field end '
      'points; `eye_NN` eye glints; `launch_point` pod launch point; `stem` stem attachment; `arm_tip` pump-arm '
      'tip; `dish_direction` dish aiming point. Numbers keep their reference base: leg sockets count from 00, the '
      'others from 01.')
    w('')
    w('| Species file | Asset id | Role | Spec (budget, nodes, sockets) | Variants | Priority |')
    w('|---|---|---|---|---|---|')
    for g in by_cat[c]:
        if len(g['files']) == 1:
            f = g['files'][0]
            w('| `%s` | `%s` | %s | %s | single | %s |' % (f['path'], g['asset_id'], esc(g['role']), esc(f['spec']),
                                                      f.get('priority', g['priority'])))
        else:
            for i, f in enumerate(g['files'], 1):
                w('| `%s` | `%s` | %s | %s | %d of %d | %s |' % (f['path'], g['asset_id'], esc(g['role']) if i == 1 else '″',
                                                             esc(f['spec']), i, len(g['files']),
                                                             f.get('priority', g['priority'])))
    w('')

    # ---------------------------------------------------------------- help
    c = 'help'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('Help pictures carry no baked text: labels come from the string table, so one picture serves every '
      'language. Explainer animations are one script per mode that moves pictures on a canvas for a fixed loop; '
      'the scripts are re-authored in the Species explainer format and reference their pictures by asset id. '
      'Choreography (what moves when) is new; canvas size and loop length follow the reference.')
    w('')
    L.extend(texture_rows(by_cat[c]))
    w('')

    # ---------------------------------------------------------------- audio
    c = 'sfx'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('Sound effects are sample groups named by **source and event**: a group holds the variants one event picks '
      'from, and its file count matches the reference group, so the mix keeps the same variety. Behaviour (volume, '
      'pitch spread, distance, looping, priority) lives in the sound-event data of [10](../reference/10_audio_design.md), '
      'not in the files. Where one reference group serves several events, the Species group is named after its '
      'main binding and the other bindings are listed; two files are shared by two groups and listed once. '
      'Formats: WAV 48 kHz 24-bit → 16-bit PCM resident (Opus stream for files over 20 s). Placeholder: '
      'synthesised tone (voice lines: text-to-speech line).')
    w('')
    w('%d (source, event) bindings fire in a match and play these groups. A further %d events fire with no bound '
      'sound; they stay silent in the reference, so the event table lists them and no file is needed.' % (
          d['live_pairs'], d['silent_events']))
    w('')
    srcs = collections.OrderedDict()
    for g in by_cat[c]:
        srcs.setdefault(g['subcategory'], []).append(g)
    for sub, gs in srcs.items():
        nf = sum(len(g['files']) for g in gs)
        title = 'Supply-crate outcome stings' if sub == 'crate_outcome' else SRC_NAME.get(sub, sub)
        w('### %d.%d %s (%d files)' % (SECTION[c], list(srcs).index(sub) + 1, title, nf))
        w('')
        w('| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |')
        w('|---|---|---|---|---|---|---|')
        for g in gs:
            pat, n = pattern([f['path'] for f in g['files']])
            if not g['files']:
                pat = 'no own file: plays the shared files of `%s`' % shared_owner(groups, g)
            ev = g['asset_id'].split('.')[-1]
            base = re.sub(r'_[b-h]$', '', ev)
            desc = EVENT_DESC.get(base, base.replace('_', ' '))
            if base != ev:
                desc += ' (additional layer)'
            if sub == 'crate_outcome':
                desc = 'Supply crate yields %s' % ev.replace('_', ' ')
            also = ', '.join('`%s`' % x for x in g['bound'][1:]) or '—'
            w('| %s | `%s` | %s | %s | %s | %s | %s |' % (pat, g['asset_id'], esc(desc), also, esc(g['spec']),
                                                          variants_of(g), prio_of(g)))
        w('')

    for c, intro_txt in [
        ('ui_audio', 'Interface sounds: menus, selection, countdown, chat and the power-slot panel. They are 2-D '
                     '(not positional) except the unit highlight. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident. '
                     'Placeholder: synthesised tone.'),
        ('ambience', 'Ambience beds: 2-D loops under the match and under the lobby. Each bed has layers that play '
                     'together; long beds stream. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident or Opus stream '
                     '(over 20 s). Placeholder: silence.'),
    ]:
        w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
        w('')
        w(intro_txt)
        w('')
        w('| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |')
        w('|---|---|---|---|---|---|---|')
        for g in by_cat[c]:
            pat, n = pattern([f['path'] for f in g['files']])
            ev = g['asset_id'].split('.')[-1]
            base = re.sub(r'_[b-h]$', '', ev)
            desc = EVENT_DESC.get(base, base.replace('_', ' '))
            if base != ev:
                desc += ' (additional layer)'
            also = ', '.join('`%s`' % x for x in g['bound'][1:]) or '—'
            w('| %s | `%s` | %s | %s | %s | %s | %s |' % (pat, g['asset_id'], esc(desc), also, esc(g['spec']),
                                                          variants_of(g), prio_of(g)))
        w('')

    # ---------------------------------------------------------------- music
    c = 'music'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('The reference plays no music in a match: its music events are never triggered. Species commissions new '
      'music. This is a brief entry: the slots below are the proposed minimum, and the design phase sets the final '
      'list, its triggers and the music bus. The style follows the look: sparse, electronic, cool timbres that '
      'leave room for the crowd and the glow; no pastiche of any existing score.')
    w('')
    w('| File | Asset id | Role | Spec | Priority | Placeholder |')
    w('|---|---|---|---|---|---|')
    for g in by_cat[c]:
        for f in g['files']:
            w('| `%s` | `%s` | %s | %s | %s | %s |' % (f['path'], g['asset_id'], esc(g['role']), esc(f['spec']),
                                                       f.get('priority', g['priority']), g['placeholder']))
    w('')

    # ---------------------------------------------------------------- fonts
    c = 'fonts'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('Fonts are licensed families (open licence or commercial, licensed for embedding), delivered as font files '
      'and rasterised at run time into a glyph cache; there are no bitmap font atlases. The reference uses three '
      'text faces; the primary face also carries Japanese and Korean, which Species splits into companion files. '
      'The glyph renderer assigns faces to text roles ([ui_framework](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md)). '
      'Coverage is the minimum; a family with wider coverage is welcome. Placeholder: fallback font.')
    w('')
    w('| File | Asset id | Role | Coverage | Priority |')
    w('|---|---|---|---|---|')
    for g in by_cat[c]:
        f = g['files'][0]
        w('| `%s` | `%s` | %s | %s | %s |' % (f['path'], g['asset_id'], esc(g['role']), g['spec'].replace('coverage ', ''),
                                             g['priority']))
    w('')
    w('Each Latin face needs a regular weight at least; a bold weight is optional. Hinting and outlines must stay '
      'legible at the smallest HUD size (9 px at 1080p) and in the darkening text-outline pass.')
    w('')

    # ---------------------------------------------------------------- strings
    c = 'strings'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('All player-facing text is new, written in plain functional English and keyed by Species string ids '
      '(`ui.lobby.ready`, `mode.flag_chain.objective`). No reference text is copied. Pass 2 collects every string '
      'id the documents use; the counts below size the work from the reference text categories.')
    w('')
    w('| Category | String-id prefix | Reference entries |')
    w('|---|---|---:|')
    tot = 0
    for prefix, label, n in d['str_cats']:
        w('| %s | `%s.*` | %d |' % (label, prefix, n))
        tot += n
    w('| **Total** | | **%d** |' % tot)
    w('')
    w('| File | Asset id | Language | Priority | Placeholder |')
    w('|---|---|---|---|---|')
    langs = {code: lang for code, prio, lang in d['languages']}
    for g in by_cat[c]:
        f = g['files'][0]
        code = f['spec'].split()[-1]
        w('| `%s` | `%s` | %s (`%s`) | %s | %s |' % (f['path'], g['asset_id'], langs[code], code, g['priority'],
                                                    g['placeholder']))
    w('')
    w('The language set is the reference set of nine; the design phase confirms it. Japanese and Korean need the '
      'companion fonts of §%d.' % SECTION['fonts'])
    w('')

    # ---------------------------------------------------------------- maps
    c = 'maps'
    w('## %d %s' % (SECTION[c], CAT_TITLE[c]))
    w('')
    w('Maps are new layouts. One map slot exists per reference map, following the reference distribution per '
      'mode and player count, so every mode keeps its range of 2-, 3- and 4-player maps. Each map has one '
      'thumbnail, rendered automatically by the map tool. One 2-player and one 3-player Zone Control slot also host '
      'the two tutorials. Map formats: map definition (text, Species map schema) → cooked map definition; '
      'placeholder: generated test map. Thumbnail formats: PNG RGB8 rendered by the map tool → BC7 sRGB, no mips; '
      '800x600 px (4:3, display size 400x300).')
    w('')
    w('| Mode | Players | Map files | Thumbnail files | Priority |')
    w('|---|---|---|---|---|')
    maps = [g for g in by_cat[c] if g['subcategory'] == 'map']
    thumbs = {g['asset_id'].split('.')[1]: g for g in by_cat[c] if g['subcategory'] == 'thumbnail'}
    total_maps = 0
    for g in maps:
        mode = g['asset_id'].split('.')[1]
        byp = collections.OrderedDict()
        for f in g['files']:
            p = int(re.search(r'_(\d)p_', f['path']).group(1))
            byp.setdefault(p, []).append(f)
        for p, fs in byp.items():
            pat, n = pattern([f['path'] for f in fs])
            tfs = [f for f in thumbs[mode]['files'] if '_%dp_' % p in f['path']]
            tpat, tn = pattern([f['path'] for f in tfs])
            pr = collections.Counter(f.get('priority', g['priority']) for f in fs)
            prs = '<br>'.join('%s: %d' % (k, pr[k]) for k in PRIO_ORDER if pr[k])
            mname = g['role'].replace(' map slot (new layout)', '')
            w('| %s (`%s`, `%s`) | %d | %s | %s | %s |' % (mname, g['asset_id'], thumbs[mode]['asset_id'], p, pat, tpat, prs))
            total_maps += n
    w('| **Total** | | **%d maps** | **%d thumbnails** | |' % (total_maps, total_maps))
    w('')

    # ---------------------------------------------------------------- not created
    w('## 20 Not to be created')
    w('')
    w('These reference files and text entries need no Species counterpart. Counts only; nothing here is named.')
    w('')
    kinds = ['texture', 'mesh', 'audio', 'map']
    cls_counts = collections.OrderedDict()
    for orig, cls, kind in d['excluded']:
        cls_counts.setdefault(cls, collections.Counter())[kind] += 1
    for cls, n in d['str_out']:
        cls_counts.setdefault(cls, collections.Counter())['text'] += n
    w('| Class | Textures | Meshes | Audio files | Map files | Text entries | Total |')
    w('|---|---:|---:|---:|---:|---:|---:|')
    tk = collections.Counter()
    for cls in sorted(cls_counts, key=lambda k: -sum(cls_counts[k].values())):
        cc = cls_counts[cls]
        tk.update(cc)
        w('| %s | %s | %s | %s | %s | %s | %d |' % (cls, cc['texture'] or '', cc['mesh'] or '', cc['audio'] or '',
                                                   cc['map'] or '', cc['text'] or '', sum(cc.values())))
    w('| **Total** | **%d** | **%d** | **%d** | **%d** | **%d** | **%d** |' % (
        tk['texture'], tk['mesh'], tk['audio'], tk['map'], tk['text'], sum(tk.values())))
    w('')
    w('Bitmap font pages are not in this table: their role passes to the font files of §%d.' % SECTION['fonts'])
    w('')
    text = '\n'.join(L) + '\n'
    with open(os.path.join(a.out, 'GameAssets.md'), 'w', encoding='utf-8') as f:
        f.write(text)
    print('GameAssets.md', len(text.encode('utf-8')), 'bytes')
    write_readme(a.out, d, by_cat, files_by, prio_by, orig_by, total_ids, total_files, tk, cls_counts)


def shared_owner(groups, g):
    target = set(g.get('shared_files', []))
    for h in groups:
        if any(f['path'] in target for f in h['files']):
            return h['asset_id']
    return '?'


def write_readme(out, d, by_cat, files_by, prio_by, orig_by, total_ids, total_files, tk, cls_counts):
    L = []
    w = L.append
    w('# Assets')
    w('')
    w('> Status: Asset overview, pass 1 · Related: [GameAssets.md](GameAssets.md) · '
      '[asset_manifest.csv](asset_manifest.csv) · [art_and_audio_briefs.md](art_and_audio_briefs.md)')
    w('')
    w('This folder lists what the reference mechanics need as Species content: every sprite, texture, mesh, '
      'sound, font, string table and map, newly authored to the technical role its reference counterpart plays. '
      'Nothing is traced, re-coloured or edited from the reference game\'s art, audio or text.')
    w('')
    w('| File | What it is |')
    w('|---|---|')
    w('| [GameAssets.md](GameAssets.md) | The owner\'s list: every asset to create, by category, with its Species '
      'file name or numbered pattern, role, source and runtime format, spec, variants, priority and placeholder |')
    w('| [asset_manifest.csv](asset_manifest.csv) | The same set, one row per file: `asset_id, category, '
      'subcategory, consumers, role, target_format, spec, colour_space, alpha_mode, variants, priority, '
      'placeholder, brief_ref, species_filename, source_format`. Consumers are documents in pass 1; pass 2 '
      'replaces them with requirement IDs. The asset pipeline consumes every row and is not listed per row |')
    w('| [art_and_audio_briefs.md](art_and_audio_briefs.md) | The style guide for the look and feel to keep, and a '
      'brief per category |')
    w('')
    w('## Counts by category and priority')
    w('')
    w('| Category | Asset ids | Files | first-playable | launch | post-launch |')
    w('|---|---:|---:|---:|---:|---:|')
    tp = collections.Counter()
    for c in CAT_ORDER:
        p = prio_by[c]
        tp.update(p)
        w('| %s | %d | %d | %d | %d | %d |' % (CAT_TITLE[c], len(by_cat[c]), files_by[c], p['skirmish-first-playable'],
                                            p['launch'], p['post-launch']))
    w('| **Total** | **%d** | **%d** | **%d** | **%d** | **%d** |' % (total_ids, total_files, tp['skirmish-first-playable'],
                                                                   tp['launch'], tp['post-launch']))
    w('')
    w('The first-playable set is one skirmish mode, Spawn Control for 2 players, with the core Supply-crate pool '
      '([GameAssets.md §2.5](GameAssets.md#25-priorities-and-the-first-playable-assumption)). '
      '%d reference files and %d reference text entries need no Species counterpart '
      '([GameAssets.md §20](GameAssets.md#20-not-to-be-created)).' % (
          tk['texture'] + tk['mesh'] + tk['audio'] + tk['map'], tk['text']))
    w('')
    w('## Technical constraints per category')
    w('')
    w('| Category | Constraints the new assets keep |')
    w('|---|---|')
    for c, t in [
        ('sprites', 'Power-of-two cards, 32-128 px (square except the 32x64 side view); cut-out alpha (alpha test 0.04 for Workers, 0.02 for other units); white or light-grey bodies for vertex tinting; four hurt variants, a side view and an alternative faction card; full mip chains'),
        ('vfx', 'Additive on black, greyscale intensity, tinted at draw; strips 128x32 and 64x256, flares 128x128; wrap repeat where scrolled or tiled; the particle dot is magnified with NEAREST'),
        ('ui', 'Straight alpha or opaque, no mip-maps, linear filtering; authored at 2x display size; panel strips tile along their long axis; new palette with the same token roles'),
        ('hud', '128x128 icons (64x64 and 50x50 for small read-outs), white or grey art tinted at draw, darkening halo behind; one icon per power or outcome, reused on crates, slots, cursors and markers'),
        ('cursors', 'Cursor and marker art with run-time blurred darkening shadows; markers drawn as tinted alpha masks; banners and flags paint the team area pure red'),
        ('glyphs', 'Platform-neutral shapes and labels; no console trade dress; shadow glyphs are darkening layers'),
        ('terrain', 'Ramps 64x64 (x slope, y height) and wave gradients 400x10 are CPU tables, never uploaded; sea tiles 128x128 seamless, wrap repeat'),
        ('meshes', 'Flat-shaded, per-face colour, no normals in source; team parts mostly red on the root and its direct children; named sockets kept exactly; triangle budgets of the reference as upper limits; eight interchangeable Relics'),
        ('help', 'No baked text; hint pictures 200x200, help pages 150x150, mode cards 300x200 (display sizes); explainer canvases 800-1000 x 250 with fixed loop lengths'),
        ('sfx', 'Mono, 48 kHz; variant counts kept per group; loops seamless; behaviour in the sound-event data, not in the files; voice barks are new voice sets'),
        ('ui_audio', '2-D, short one-shots; mono or stereo'),
        ('ambience', '2-D layered loops; streamed when long'),
        ('music', 'New; stereo 48 kHz; streamed; slots proposed, final list set by the design phase'),
        ('fonts', 'Licensed families; glyph cache at run time; coverage Latin-1, Cyrillic, currency, Japanese kana and ideographs, Hangul'),
        ('strings', 'New text only; ICU MessageFormat; one table per language; the reference set of nine languages'),
        ('maps', 'New layouts; one slot per reference map by mode and player count; one auto-rendered 4:3 thumbnail per map'),
    ]:
        w('| %s | %s |' % (CAT_TITLE[c], t))
    w('')
    w('## Placeholder rule')
    w('')
    w('The content build generates a stand-in for every manifest row that has no authored file yet, at the same '
      'runtime path and with the same format, size, channel layout, sockets and loop flag as the row specifies. '
      'The game therefore always builds and runs, and every missing asset is visible in the build report. An '
      'authored file under `content/` replaces its stand-in; nothing else changes. The stand-in kinds:')
    w('')
    w('| Placeholder | What the content build generates |')
    w('|---|---|')
    for k, v in PLACEHOLDER_DEF:
        w('| %s | %s |' % (k, v))
    w('')
    w('The build also validates the other direction: every asset id that the sound-event data, the maps or the '
      'explainer scripts reference must exist in the manifest (logged, never fatal), and unused files are reported.')
    w('')
    text = '\n'.join(L) + '\n'
    with open(os.path.join(out, 'README.md'), 'w', encoding='utf-8') as f:
        f.write(text)
    print('README.md', len(text.encode('utf-8')), 'bytes')


if __name__ == '__main__':
    main()
