"""X-AST-2a: asset pass 2, part a. Applies the Phase 4 asset fixes to the manifest and the origin map.

Usage (run with python3 -I, paths as arguments):
  python3 -I X-AST-2a_apply.py --design design_next --origin species_work/asset_origin_map.csv \
      --new-assets species_work/p4_new_assets.csv [--dry-run]

Reads and rewrites design_next/assets/asset_manifest.csv (column order kept), updates the private
origin map, writes p4_new_assets.csv, and prints counts per category and priority.
It is idempotent: rows it adds are replaced, not duplicated, when it runs again.
"""
import argparse
import collections
import csv
import glob
import os
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument('--design', required=True)
ap.add_argument('--origin', required=True)
ap.add_argument('--new-assets', required=True)
ap.add_argument('--dry-run', action='store_true')
a = ap.parse_args()

ROOT = a.design
MAN = os.path.join(ROOT, 'assets', 'asset_manifest.csv')
COLS = ['asset_id', 'category', 'subcategory', 'consumers', 'role', 'target_format', 'spec', 'colour_space',
        'alpha_mode', 'variants', 'priority', 'placeholder', 'brief_ref', 'species_filename', 'source_format']

with open(MAN, encoding='utf-8', newline='') as fh:
    rd = csv.DictReader(fh)
    assert rd.fieldnames == COLS, rd.fieldnames
    rows = list(rd)
TAG = '[p4a]'  # never written; rows this script adds are recognised by NEW_IDS below

# ------------------------------------------------------------------ document texts (consumer evidence)
texts = {}
for p in glob.glob(os.path.join(ROOT, 'reference', '**', '*.md'), recursive=True) + \
        glob.glob(os.path.join(ROOT, 'architecture', 'components', '**', '*.md'), recursive=True):
    texts[os.path.relpath(p, ROOT).replace(os.sep, '/')] = open(p, encoding='utf-8').read()

WILD = re.compile(r'`([a-z][a-z0-9_]*\.[a-z0-9_.]+)\.\*`')  # at least two segments: a stem-wide `sfx.*` names no consumer
BRACE = re.compile(r'`([a-z][a-z0-9_.]*)\.\{([a-z0-9_, ]+)\}`')
wild_by_doc = {d: set(WILD.findall(t)) for d, t in texts.items()}
brace_by_doc = {}
for d, t in texts.items():
    s = set()
    for pre, alts in BRACE.findall(t):
        for x in alts.split(','):
            s.add(pre + '.' + x.strip())
    brace_by_doc[d] = s


def citing_docs(aid):
    out = set()
    pat = '`' + aid + '`'
    for d, t in texts.items():
        if pat in t or aid in brace_by_doc[d]:
            out.add(d)
            continue
        for w in wild_by_doc[d]:
            if aid.startswith(w + '.'):
                out.add(d)
                break
    return out


def exists(p):
    return os.path.exists(os.path.join(ROOT, p))


# stale consumer paths and the folders or split files that replaced them
REPL = {
    'reference/10_audio_design.md': sorted(glob.glob(os.path.join(ROOT, 'reference/10_audio_design/*.md'))),
    'reference/02_units/creatures_and_hazards.md': [os.path.join(ROOT, 'reference/02_units/creatures.md'),
                                                    os.path.join(ROOT, 'reference/02_units/hazards_and_projectiles.md')],
    'architecture/components/animation_and_vfx.md': sorted(glob.glob(os.path.join(ROOT, 'architecture/components/animation_and_vfx/*.md'))),
    'reference/06_controls_camera_and_commands.md': sorted(glob.glob(os.path.join(ROOT, 'reference/06_controls_camera_and_commands/*.md'))),
}
REPL = {k: [os.path.relpath(x, ROOT).replace(os.sep, '/') for x in v] for k, v in REPL.items()}

FUEL = ['mesh.structure.fuel_pump_housing', 'mesh.structure.fuel_pump_arm', 'mesh.structure.fuel_line_foot',
        'mesh.structure.fuel_line_span', 'mesh.structure.fuel_depot', 'fx.fuel_flow', 'sfx.building.fuel_line.flow',
        'sfx.building.fuel_depot.load_passenger']
MODE_OBJ = 'reference/03_buildings_and_world_objects/mode_objects.md'
ECON = 'reference/03_buildings_and_world_objects/economy_and_spawn_objects.md'
REMOVE = {('icon.power.rocket', 'reference/02_units/strike_team.md')}
ADD = {  # consumers named by the reports where the document states the use without the asset id
    'sfx.ui.power_input.confirm': ['reference/02_units/builder.md'],          # G02a2, G10: the Builder placement sound
    'sfx.ui.match_ui.unit_select': ['reference/02_units/transport.md'],      # G02a2, G06: the unit toggle (toggle_state)
}

# ------------------------------------------------------------------ field fixes on existing rows
BC = 'DDS (DX10 header) BC7_UNORM_SRGB, full mip chain'
BC4 = 'DDS (DX10 header) BC4_UNORM, full mip chain'
MESH_TGT = 'cooked Species mesh in the pack (face normals generated, colours attenuated at cook)'
MESH_ALPHA = ('opaque; team regions, where the mesh has any, marked by explicit tint weights: the `_SPECIES_TINT` '
              'vertex attribute or the `species_team_tint` material flag (ADR-0020)')
CLOTH_TGT = BC + '; the team colour is applied in the shader through the companion tint mask (AST-022)'
CLOTH_ALPHA = 'straight alpha; the team area takes the team colour by the weight of its companion tint mask'
CLOTH_SRC = 'PNG RGBA8, sRGB; team area painted as grey levels (its intensity), no key colour'

# per-file size corrections for whole 4x4 blocks (ADR-0017): file -> (old, new)
SIZE_FIX = {
    'content/textures/help/explainer/explainer_fuel_pump.png': ('181x256', '184x256'),
    'content/textures/help/explainer/explainer_fuel_pump_arm.png': ('50x256', '52x256'),
    'content/textures/help/explainer/explainer_launch_vessel.png': ('238x256', '240x256'),
    'content/textures/help/explainer/explainer_solar_array.png': ('158x256', '160x256'),
    'content/textures/help/explainer/explainer_turret_barrel.png': ('140x90', '140x92'),
    'content/textures/help/explainer/explainer_turret_base.png': ('140x110', '140x112'),
    'content/textures/help/explainer/explainer_core_device.png': ('207x256', '208x256'),
    'content/textures/help/explainer/explainer_zone_back.png': ('512x310', '512x312'),
    'content/textures/help/explainer/explainer_zone_front.png': ('512x310', '512x312'),
    'content/textures/help/help_general_page_03.png': ('150x149 px display size; author at 2x (300x298)',
                                                       '150x150 px display size; author at 2x (300x300)'),
    'content/textures/ui/ui_logo_banner.png': ('394x149 px display size; author at 2x (788x298)',
                                               '394x150 px display size; author at 2x (788x300)'),
    'content/textures/ui/ui_logo_emblem.png': ('277x194 px display size; author at 2x (554x388)',
                                               '278x194 px display size; author at 2x (556x388)'),
    'content/textures/ui/ui_title_wordmark.png': ('659x122 px display size; author at 2x (1318x244)',
                                                  '660x122 px display size; author at 2x (1320x244)'),
}
HINT_HOOK = ['help.hint.start_here', 'help.hint.spawn_nodes', 'help.hint.reinforcements', 'help.hint.supply_crates',
             'help.hint.zone_control', 'help.hint.relic_run_capture', 'help.hint.relic_run_lift',
             'help.hint.siege_core_device', 'help.hint.siege_attack', 'help.hint.siege_defend',
             'help.hint.siege_spawn_nodes', 'help.hint.siege_breach_charge', 'help.hint.launch_race_01',
             'help.hint.launch_race_02', 'help.hint.flag_chain_01', 'help.hint.flag_chain_02']

FIELD = {  # asset id -> {field: (old substring or None for whole value, new)}
    'sprite.worker.front': {'spec': ('magnified with NEAREST on the front-end backdrop',
                                     'magnified on the front-end backdrop through the point-magnify clamp-to-edge sampler (ADR-0018)')},
    'sprite.visitor.front': {'alpha_mode': (None, 'cut-out (alpha test at the consumer threshold)'),
                             'spec': ('alpha-test threshold 0.02', 'drawn as a Worker card: alpha-test threshold 0.04 (RND-060, RND-316)'),
                             'source_format': (None, 'PNG RGBA8, sRGB; cut-out alpha painted by the artist')},
    'fx.particle_dot': {'spec': ('MAG NEAREST (crisp pixels) in particle batches',
                                 'magnified with crisp pixels in the particle batches through the point-magnify clamp-to-edge sampler (ADR-0018)')},
    'fx.sky_cloud': {'spec': ('blobby layer MAG LINEAR / MIN NEAREST, flat layer NEAREST; wrap repeat; small tile magnified on purpose',
                              'read through its `level0` view (mip level 0 only, RND-153); the blobby layers wrap with the linear-wrap sampler, '
                              'the flat layers wrap in the shader and sample through the point clamp-to-edge sampler (RND-154); '
                              'small tile magnified on purpose')},
    'fx.beam_short': {'spec': ('wrap repeat for the sky grid; drawn without mips by the sky and laser-bolt passes',
                               'the sky grid samples it with the linear clamp-to-edge sampler through all mip levels (RND-151); '
                               'the laser-bolt pass reads its `level0` view (RND-339)')},
    'fx.shard_edges': {'role': (None, 'Edge-line art reference for the shatter shards: not sampled at run time, because the shard '
                                      'edge lines come from a barycentric mask (VFX-229, ADR-0010); it fixes the look the mask reproduces')},
    'cursor.world': {'spec': ('blurred darkening shadow generated at run time',
                              'its blurred darkening shadow is the derived variant `cursor.world.shadow`, produced offline by the cooker')},
    'cursor.place': {'role': (None, 'Placement cursor for powers that place a structure; also the entering marker drawn at the '
                                    'entrance of a Transport or a Beam Dish (R-CTRL-371)')},
    'cursor.pointer': {'role': (None, 'Pointer-arrow image of the scroll-bar arrows and the selection arrows; the world cursor '
                                      '`cursor.world` draws the pointer itself (UI-465)')},
    'mesh.structure.structure_default': {'role': (None, 'Port pad: the pad mesh drawn under every port, whose `status` socket places '
                                                        'the port\'s status star (R-BLDG-017, RND-168); also the fallback body drawn '
                                                        'when a structure\'s mesh is missing (RND-163)'),
                                         'priority': (None, 'skirmish-first-playable')},
    'mesh.unit.worker_figure': {'role': ('team-recoloured mesh', 'team-tinted mesh')},
    'mesh.creature.spider_body': {'spec': ('leg_00, leg_01, leg_02, leg_03, leg_04, leg_05, leg_06, leg_07',
                                           'leg_01, leg_02, leg_03, leg_04, leg_05, leg_06, leg_07, leg_08')},
    'help.hint.camera_controls': {'spec': ('512x128 px display size; author at 2x (1024x256)',
                                           '304x128 px display size; author at 2x (608x256)')},
    'help.explainer_picture': {'target_format': (None, BC)},
    'terrain.ramp': {'spec': ('x = slope, y = height',
                              'x = slope (column 0 flat ground, column 63 vertical); y = height, the look-up row counted from '
                              'the bottom row of the image as viewed: the bottom row is the highest terrain, the top row sea '
                              'level and below (TER-040); the content build rejects another size (TER-250)')},
    'terrain.shore_wave_gradient': {'spec': ('the wave colour runs along x',
                                             'the wave colours are pixel row 1 counted from the bottom row of the image as viewed, '
                                             'one colour per column (TER-041)')},
    'uitex.logo_banner': {'role': (None, 'Large title logo on the main menu; no rule of a reference match draws it'),
                          'priority': (None, 'post-launch')},
    'marker.leader_banner': {'role': (None, 'Leader banner per order state: idle, follow, formation, gather (no in-scope rule draws '
                                            'gather; it keeps the state order complete)')},
}
for h in HINT_HOOK:
    FIELD[h] = {'role': ('__append__', '; objective-card hook art: no rule draws it in a reference match (DEC-050, R-TUT-013)'),
                'priority': (None, 'post-launch')}

# ------------------------------------------------------------------ new rows
NEW = []  # list of row dicts
NEW_REQ = {}  # asset id -> request source (for p4_new_assets.csv)
by_id = collections.OrderedDict()
for r in rows:
    by_id.setdefault(r['asset_id'], []).append(r)


def row(**kw):
    r = {c: '' for c in COLS}
    r.update(kw)
    return r


def members(aid, files, specs, **common):
    n = len(files)
    out = []
    for i, (f, s) in enumerate(zip(files, specs), 1):
        out.append(row(asset_id=aid, species_filename=f, spec=s,
                       variants='single file' if n == 1 else 'file %d of %d' % (i, n), **common))
    return out


def add(aid, req, rs):
    NEW.extend(rs)
    NEW_REQ[aid] = req


AUD_T = '16-bit PCM, 48 kHz, resident in the pack'
SRC_MONO = 'WAV, 48 kHz, 24-bit PCM, mono'
SRC_MS = 'WAV, 48 kHz, 24-bit PCM, mono or stereo'
AUD_BRIEF = 'art_and_audio_briefs.md#12-sound-effects-by-source-and-event'
AUD_DOCS = ['reference/10_audio_design/event_catalogue.md', 'reference/10_audio_design/music_hooks_and_sound_design_brief.md']
AEN = 'architecture/components/audio_engine.md'


def sfx(aid, sub, stem, n, role, spec, cons, prio, req, cat='Audio by source and event', src=SRC_MONO,
        ph='synthesised tone', brief=AUD_BRIEF, folder=None):
    folder = folder or 'content/audio/sfx/%s' % sub
    files = ['%s/%s.wav' % (folder, stem)] if n == 1 else ['%s/%s_%02d.wav' % (folder, stem, i) for i in range(1, n + 1)]
    add(aid, req, members(aid, files, [spec] * n, category=cat, subcategory=sub, consumers=';'.join(cons), role=role,
                          target_format=AUD_T, colour_space='n/a', alpha_mode='n/a', priority=prio, placeholder=ph,
                          brief_ref=brief, source_format=src))


TAKES = 'variant count and durations sized from the unbound reference takes for this moment (R-AUD-142)'
sfx('sfx.weapon.laser_bolt.spawn', 'laser_bolt', 'laser_bolt_spawn', 10,
    'Sample group for laser bolt spawn: every laser bolt fired (DEC-564)',
    'mono, 48 kHz; positional; one-shot; reference duration 0.23-0.67 s; ' + TAKES,
    AUD_DOCS + ['reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md', AEN],
    'skirmish-first-playable', 'G10 assets; item 82; DEC-564')
sfx('sfx.weapon.laser_bolt.hit_structure', 'laser_bolt', 'laser_bolt_hit_structure', 10,
    'Sample group for laser bolt hit_structure: a bolt hits a structure (DEC-564)',
    'mono, 48 kHz; positional; one-shot; reference duration 0.39-1.10 s; ' + TAKES,
    AUD_DOCS + ['reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md', AEN],
    'skirmish-first-playable', 'G10 assets; item 82; G04 missing assets 2; DEC-564')
sfx('sfx.weapon.laser_bolt.kill', 'laser_bolt', 'laser_bolt_kill', 8,
    'Sample group for laser bolt kill: a laser bolt or a turret round kills an actor (DEC-564)',
    'mono, 48 kHz; positional; one-shot; reference duration 0.22-0.75 s; ' + TAKES,
    AUD_DOCS + ['reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md',
                'reference/03_buildings_and_world_objects/defences_and_props.md', AEN],
    'skirmish-first-playable', 'G10 assets; item 82; G04 missing assets 2; DEC-564')
sfx('sfx.unit.trooper.throw_grenade', 'trooper', 'trooper_throw_grenade', 4,
    'Sample group for trooper throw_grenade: the Trooper throws the Strike team grenade (DEC-097)',
    'mono, 48 kHz; positional, follows the source; one-shot; about 0.5-1.0 s; variant count proposed '
    '(the reference set holds no take for this moment)',
    AUD_DOCS + ['reference/02_units/strike_team.md', AEN], 'skirmish-first-playable', 'G10 assets; item 82; DEC-097')
sfx('sfx.ui.match_ui.game_over', 'ui', 'ui_game_over', 2,
    'Sample group for match ui game_over: the game-over announcement when a result is declared, before the victory or '
    'defeat sting (R-AUD-307, DEC-097)',
    'mono or stereo, 48 kHz; 2-D (not positional); one-shot; announcer bus; reference duration 4.37-4.39 s; ' + TAKES,
    ['reference/10_audio_design/front_end_hud_voice_and_ambience.md'] + AUD_DOCS +
    ['reference/01_skirmish_rules_and_modes/README.md', AEN],
    'skirmish-first-playable', 'G01 missing assets 1; G10 assets; items 23, 82; DEC-097', cat='UI sounds', src=SRC_MS,
    ph='text-to-speech line', brief='art_and_audio_briefs.md#13-ui-sounds', folder='content/audio/ui')

# application icon (C-PLT, item 117)
ICON_SIZES = [16, 24, 32, 48, 64, 256]
add('uitex.app_icon', 'C-PLT missing assets 1; item 117',
    members('uitex.app_icon', ['content/textures/ui/ui_app_icon_%02d.png' % i for i in range(1, 7)],
            ['%dx%d px, icon size %d of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size' % (s, s, i)
             for i, s in enumerate(ICON_SIZES, 1)],
            category='UI panels and title', subcategory='ui', consumers='architecture/components/platform_layer.md',
            role='Application icon: the executable\'s icon resource, also the window icon (PLT-057); one member per icon size',
            target_format='Windows icon resource (.ico) holding the six sizes, built into the executable; not in the pack',
            colour_space='sRGB', alpha_mode='straight alpha', priority='skirmish-first-playable',
            placeholder='generated glyph', brief_ref='art_and_audio_briefs.md#5-ui-panels-title-and-logo',
            source_format='PNG RGBA8, sRGB; straight alpha'))

# input glyphs (C-INP, item 97)
TINT_SRC = 'PNG RGBA8, sRGB; white or grey art with straight alpha (tinted at draw)'
DARK_SRC = 'PNG 8-bit greyscale, white = full darkening'
INP = 'architecture/components/input.md'
for gid, label in [('stick_left', 'left stick'), ('stick_right', 'right stick'), ('dpad_horizontal', 'd-pad, left and right')]:
    add('glyph.pad.' + gid, 'C-INP missing assets; item 97',
        members('glyph.pad.' + gid, ['content/textures/icons/input/glyph_pad_%s.png' % gid],
                ['64x64 px display size; author at 2x (128x128); platform-neutral shape and label; no console trade dress'],
                category='Input glyphs', subcategory='input', consumers=INP, role='Gamepad %s glyph (INP-081)' % label,
                target_format=BC, colour_space='sRGB', alpha_mode='straight alpha, tinted by vertex colour',
                priority='launch', placeholder='generated glyph', brief_ref='art_and_audio_briefs.md#8-input-glyphs',
                source_format=TINT_SRC))
    add('glyph.pad.%s_shadow' % gid, 'C-INP missing assets (a shadow each); item 97',
        members('glyph.pad.%s_shadow' % gid, ['content/textures/icons/input/glyph_pad_%s_shadow.png' % gid],
                ['64x64 px display size; author at 2x (128x128)'],
                category='Input glyphs', subcategory='input', consumers=INP,
                role='Darkening shadow under the %s glyph' % label, target_format=BC4, colour_space='linear',
                alpha_mode='darkening layer (destination x (1 - texel))', priority='launch',
                placeholder='generated gradient', brief_ref='art_and_audio_briefs.md#8-input-glyphs',
                source_format=DARK_SRC))

# route arrows and the empty compass (C-RND-b, item 66)
RND_E = 'architecture/components/renderer/entity_and_overlay_renderers.md'
add('marker.route_arrow', 'C-RND-b missing assets 1; item 66',
    members('marker.route_arrow', ['content/textures/icons/markers/marker_route_arrow.png'],
            ['32x32 px; white arrow head pointing along +u'],
            category='Cursors and markers', subcategory='markers', consumers=RND_E,
            role='Arrow head drawn along each leg of a route line, pointing in the route\'s direction, in the colour of '
                 'the route\'s spots (RND-353)',
            target_format=BC, colour_space='sRGB', alpha_mode='straight alpha, tinted by vertex colour',
            priority='skirmish-first-playable', placeholder='generated glyph',
            brief_ref='art_and_audio_briefs.md#7-cursors-and-markers', source_format=TINT_SRC))
pc = by_id['marker.edge_compass'][0]
add('marker.edge_compass_empty', 'C-RND-b missing assets 2; item 66',
    members('marker.edge_compass_empty', ['content/textures/icons/markers/marker_edge_compass_empty.png'],
            [pc['spec'] + '; the compass ring with no arrow'],
            category=pc['category'], subcategory=pc['subcategory'], consumers=RND_E,
            role='Empty compass: the compass ring that the compass layer of an on-screen world marker draws instead of '
                 'the arrow (RND-357)',
            target_format=pc['target_format'], colour_space=pc['colour_space'], alpha_mode=pc['alpha_mode'],
            priority=pc['priority'], placeholder=pc['placeholder'], brief_ref=pc['brief_ref'],
            source_format=pc['source_format']))

# Abductor beam ring (G02b, item 35)
add('fx.abductor_beam_ring', 'G02b missing assets 1; item 35',
    members('fx.abductor_beam_ring', ['content/textures/fx/fx_abductor_beam_ring.png'],
            ['128x32 px; one soft ring band across v; wrap repeat around the ring (u)'],
            category='VFX textures', subcategory='fx',
            consumers=';'.join(['reference/02_units/hazards_and_projectiles.md', RND_E,
                                'architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md']),
            role='Beam ring of the Abductor\'s tractor beam: stacked rings in the Visitor faction colour, scrolling up '
                 'while Abducting and down while Dropping (R-UNIT-710); drawn with `actor_beam_alpha`',
            target_format=BC4, colour_space='linear', alpha_mode='alpha mask (white colour, mask as alpha, tinted at draw)',
            priority='launch', placeholder='generated gradient', brief_ref='art_and_audio_briefs.md#4-vfx-textures',
            source_format='PNG 8-bit greyscale mask, white = opaque'))

# derived variants (C-AST missing assets 1, C-RND-b missing assets 3 and 4, item 86)
CUR = 'reference/06_controls_camera_and_commands/cursor_and_markers.md'
AST = 'architecture/components/asset_pipeline_and_resources.md'
RND_V = 'architecture/components/renderer/visual_language.md'
SHADOWED = [('cursor.world', [CUR]), ('cursor.place', [CUR]), ('cursor.unavailable', [CUR]),
            ('cursor.move_target', [CUR]), ('cursor.highlight', [CUR]), ('cursor.select', [CUR]), ('cursor.enter', [CUR]),
            ('marker.crate_landing', ['reference/04_supply_crates_powers_and_weapons/crates_and_powers.md']),
            ('marker.barrage_target', ['reference/02_units/hazards_and_projectiles.md'])]
size_re = re.compile(r'^(\d+x\d+ px(?: display size; author at 2x \(\d+x\d+\))?)')


def key_of(path):
    return os.path.splitext(path)[0]


for pid, docs in SHADOWED:
    p = by_id[pid][0]
    m = size_re.match(p['spec'])
    size = m.group(1) if m else 'the size of its parent'
    add(pid + '.shadow', 'C-AST missing assets 1; C-RND-b missing assets 4; item 86',
        members(pid + '.shadow', [key_of(p['species_filename']) + '@shadow'],
                ['%s, the size of its parent; separable five-tap blur (2, 4, 7, 4, 2) scaled by `rnd.shadow.icon_blur_radius` '
                 '(RND-422)' % size],
                category=p['category'], subcategory='derived',
                consumers=';'.join(docs + [RND_E, RND_V, AST]),
                role='Blurred darkening shadow of `%s`, a derived variant produced offline by the cooker (`BlurredShadow`, '
                     'AST-029, RND-243)' % pid,
                target_format=BC4 + '; cooked entry `<parent key>@shadow`', colour_space='linear',
                alpha_mode='darkening layer (destination x (1 - texel))', priority=p['priority'],
                placeholder='derived by the cooker', brief_ref=p['brief_ref'],
                source_format='derived from the parent\'s source, no authored file (an artist MAY author it instead as '
                              'greyscale darkening art, RND-422)'))
p = by_id['sprite.crawler.segment'][0]
add('sprite.crawler.segment.trail', 'C-AST missing assets 1; C-RND-b missing assets 3; item 66; item 86',
    members('sprite.crawler.segment.trail', [key_of(p['species_filename']) + '@trail'],
            ['32x512 px column cleared to black; copies of the segment art stacked from the top, copy i (from 0) '
             '(32 - floor(i / 2)) px square: 18 copies, 504 px in all (RND-320)'],
            category=p['category'], subcategory='derived',
            consumers=';'.join([RND_E, AST, 'reference/02_units/creatures.md']),
            role='Crawler trail texture sampled by the worm strip, a derived variant of `sprite.crawler.segment` produced '
                 'offline by the cooker (`StackedCopies`, AST-029)',
            target_format=BC4 + '; cooked entry `<parent key>@trail`', colour_space=p['colour_space'],
            alpha_mode=p['alpha_mode'], priority=p['priority'], placeholder='derived by the cooker',
            brief_ref=p['brief_ref'], source_format='derived from the parent\'s source, no authored file'))

# companion tint masks (C-AST missing assets 2, item 86)
for pid in ['marker.leader_banner', 'marker.flag_post_state']:
    ps = by_id[pid]
    files = [key_of(x['species_filename']) + '_tint.png' for x in ps]
    cons = sorted(set(c for x in ps for c in x['consumers'].split(';') if c) | {AST})
    add(pid + '_tint', 'C-AST missing assets 2; item 83; item 86',
        members(pid + '_tint', files, ['64x64 px, the size of its parent member'] * len(files),
                category=ps[0]['category'], subcategory='markers', consumers=';'.join(cons),
                role='Companion tint mask of `%s`, one per member in the parent\'s order: the team-tint weight of the cloth, '
                     '1 where the team colour applies fully (AST-022, ADR-0020)' % pid,
                target_format=BC4, colour_space='linear',
                alpha_mode='tint mask (team-tint weight read by the shader; never drawn on its own)',
                priority=ps[0]['priority'], placeholder='full-weight mask',
                brief_ref=ps[0]['brief_ref'], source_format='PNG 8-bit greyscale, white = full team tint'))

# map-editor camera-view marker mesh (G08b)
MESH_SRC = 'glTF 2.0 binary (.glb): no NORMAL attribute (flat faces), COLOR_0 per-face colour, metres, +Y up'
add('mesh.dev.camera_view', 'G08b missing assets',
    members('mesh.dev.camera_view', ['content/meshes/dev/camera_view.glb'],
            ['small camera-frustum marker; 1 node; no sockets; few triangles'],
            category='Meshes', subcategory='dev', consumers='reference/08_maps_and_level_design/map_editor_hooks.md',
            role='Camera-view marker that the map editor draws at each camera view and uses for picking (R-MAP-322, '
                 'R-MAP-380); editor builds only',
            target_format=MESH_TGT, colour_space='sRGB vertex colours', alpha_mode='opaque; no tint marking',
            priority='post-launch', placeholder='primitive mesh with sockets', brief_ref='art_and_audio_briefs.md#10-meshes',
            source_format=MESH_SRC))

# collision assets (C-AST missing assets 3, item 86; collidable meshes of R-BLDG-041 to R-BLDG-043)
COLLIDABLE = ['mesh.structure.spawn_node', 'mesh.structure.spawn_node_alt', 'mesh.structure.source_node',
              'mesh.structure.relay_mast', 'mesh.structure.transit_gate', 'mesh.structure.transit_gate_alt',
              'mesh.structure.command_console', 'mesh.structure.command_console_linked', 'mesh.structure.core_device',
              'mesh.structure.fuel_pump_housing', 'mesh.structure.fuel_line_foot', 'mesh.structure.fuel_depot',
              'mesh.structure.solar_array', 'mesh.structure.grid_pylon', 'mesh.structure.beam_dish',
              'mesh.structure.barrier_emitter', 'mesh.structure.turret_head', 'mesh.structure.turret_barrel',
              'mesh.structure.wall_section', 'mesh.structure.hive_mound', 'mesh.structure.spitter_plant_head',
              'mesh.scenery.prop', 'mesh.creature.abductor_hull']
BLDG_README = 'reference/03_buildings_and_world_objects/README.md'
DAT_G = 'architecture/components/data_schemas/entity_audio_geometry_and_panel_schemas.md'
budget_re = re.compile(r'<= (\d+) triangles budget')
for mid in COLLIDABLE:
    ps = by_id[mid]
    name = {'mesh.scenery.prop': 'scenery_prop'}.get(mid, mid.split('.')[-1])
    cid = 'mesh.collision.' + name
    n = len(ps)
    files = ['content/meshes/collision/%s.glb' % name] if n == 1 else \
        ['content/meshes/collision/%s_%02d.glb' % (name, i) for i in range(1, n + 1)]
    specs = []
    for x in ps:
        b = budget_re.search(x['spec'])
        specs.append('triangle primitives only, no materials, no sockets; the nodes of its render member that the hit '
                     'tests read, at that member\'s bounds; %s' % ('<= %s triangles budget' % b.group(1) if b else
                                                                   'at most its render member\'s triangle count'))
    bdocs = [c for c in ps[0]['consumers'].split(';') if c.startswith('reference/03_')]
    if mid == 'mesh.creature.abductor_hull':
        bdocs = ['reference/03_buildings_and_world_objects/defences_and_props.md']
        role = ('Collision asset of `mesh.creature.abductor_hull` where a map places it as a scenery mesh: the hit '
                'geometry that the sphere, mesh and ray tests read (R-BLDG-041, R-BLDG-042, R-BLDG-459, AST-044)')
    else:
        role = ('Collision asset of `%s`%s: the hit geometry that the structure\'s sphere, mesh and ray tests read '
                '(R-BLDG-041 to R-BLDG-043, AST-044)' % (mid, ', one per member in its order' if n > 1 else ''))
    cons = [BLDG_README] + [d for d in bdocs if d != BLDG_README] + [AST, DAT_G]
    add(cid, 'C-AST missing assets 3; item 86',
        members(cid, files, specs, category='Meshes', subcategory='collision', consumers=';'.join(cons), role=role,
                target_format='cooked `Collision` entry in the pack, flagged simulation-relevant: triangles and bounding '
                              'sphere per node (DAT-278)',
                colour_space='n/a', alpha_mode='n/a', priority=ps[0]['priority'], placeholder='primitive collision',
                brief_ref='art_and_audio_briefs.md#10-meshes',
                source_format='glTF 2.0 binary (.glb): triangle primitives, no materials, metres, +Y up; authored as its own '
                              'source, never generated from render geometry (AST-044)'))

# development string table (C-DBG)
add('strings.dev_en_gb', 'C-DBG missing assets (development string table)',
    members('strings.dev_en_gb', ['content/strings/dev_en_gb.json'], ['en-GB only; the `ui.dev.*` captions'],
            category='String table', subcategory='string_table',
            consumers='architecture/components/diagnostics_test_and_performance.md',
            role='Development string table: the developer-facing captions in the base language only; development '
                 'editions only, never a player edition (ADR-0056)',
            target_format='cooked string table in the pack of the development editions', colour_space='n/a',
            alpha_mode='n/a', priority='skirmish-first-playable', placeholder='pseudo-localised text',
            brief_ref='art_and_audio_briefs.md#17-player-facing-text',
            source_format='UTF-8 JSON, string id -> ICU MessageFormat text'))

NEW_IDS = set(NEW_REQ)

# ------------------------------------------------------------------ apply
log = collections.Counter()
out = [r for r in rows if r['asset_id'] not in NEW_IDS]  # idempotent re-run
for r in out:
    aid = r['asset_id']
    # 1 consumers
    cons = [c for c in r['consumers'].split(';') if c]
    new = []
    for c in cons:
        if exists(c):
            new.append(c)
        elif c in REPL:
            hit = [f for f in REPL[c] if ('`' + aid + '`') in texts[f] or aid in brace_by_doc[f] or
                   any(aid.startswith(w + '.') for w in wild_by_doc[f])]
            new.extend(hit or [REPL[c][0]])
            log['stale path replaced'] += 1
        else:
            print('UNKNOWN STALE PATH', aid, c, file=sys.stderr)
            new.append(c)
    if aid in FUEL or aid.startswith('sfx.building.fuel_pump.'):
        if MODE_OBJ in new and ('`' + aid + '`') not in texts[MODE_OBJ]:
            new = [ECON if x == MODE_OBJ else x for x in new]
            log['fuel-chain consumer corrected'] += 1
    new = [x for x in new if (aid, x) not in REMOVE]
    extra = sorted((citing_docs(aid) | set(ADD.get(aid, []))) - set(new))
    if extra:
        log['rows gaining consumers'] += 1
    new += extra
    seen = []
    for x in new:
        if x not in seen:
            seen.append(x)
    r['consumers'] = ';'.join(seen)
    # 2 field fixes
    for fld, (old, val) in FIELD.get(aid, {}).items():
        if old is None:
            r[fld] = val
        elif old == '__append__':
            if not r[fld].endswith(val):
                r[fld] += val
        elif old in r[fld]:
            r[fld] = r[fld].replace(old, val)
        elif val not in r[fld]:
            print('FIELD FIX MISSED', aid, fld, old, file=sys.stderr)
        log['field fixes'] += 1
    f = r['species_filename']
    if f in SIZE_FIX:
        o, n = SIZE_FIX[f]
        if o in r['spec']:
            r['spec'] = r['spec'].replace(o, n)
            log['size fixes'] += 1
        elif n not in r['spec']:
            print('SIZE FIX MISSED', f, file=sys.stderr)
    # 3 team tint (ADR-0020) and cook-time attenuation
    if r['category'] == 'Meshes' and r['subcategory'] not in ('collision', 'dev'):
        r['target_format'] = r['target_format'].replace('colours attenuated at load', 'colours attenuated at cook')
        if r['alpha_mode'] == 'opaque (team parts painted mostly red)':
            r['alpha_mode'] = MESH_ALPHA
            log['mesh tint rows'] += 1
    if aid in ('marker.leader_banner', 'marker.flag_post_state'):
        r['target_format'] = CLOTH_TGT
        r['alpha_mode'] = CLOTH_ALPHA
        r['source_format'] = CLOTH_SRC
        r['spec'] = r['spec'].replace('team area pure red',
                                      'team area as grey levels, with its companion tint mask `%s_tint`' % aid)
        log['cloth rows'] += 1
out.extend(NEW)

# checks: one row per file, every row has a consumer that exists, BC sizes are multiples of 4
files = collections.Counter(r['species_filename'] for r in out)
dup = [f for f, n in files.items() if n > 1]
assert not dup, dup
for r in out:
    for c in r['consumers'].split(';'):
        assert c and exists(c), (r['asset_id'], c)
dims = re.compile(r'(?:author at 2x \((\d+)x(\d+)\))|^(\d+)x(\d+) px(?! display)|; (\d+)x(\d+) px')
for r in out:
    if 'BC' not in r['target_format']:
        continue
    m = re.search(r'author at 2x \((\d+)x(\d+)\)', r['spec']) or re.search(r'(?:^|; )(\d+)x(\d+) px', r['spec'])
    if m:
        w, h = int(m.group(1)), int(m.group(2))
        if w % 4 or h % 4:
            print('NOT A MULTIPLE OF 4:', r['asset_id'], r['species_filename'], r['spec'][:60], file=sys.stderr)

if not a.dry_run:
    with open(MAN, 'w', encoding='utf-8', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=COLS, lineterminator='\n')
        w.writeheader()
        w.writerows(out)

# ------------------------------------------------------------------ origin map
TAKE_MAP = {
    'sfx.weapon.laser_bolt.spawn': ['data/mwsounds/Laser_Cock%d.ogg' % i for i in range(1, 11)],
    'sfx.weapon.laser_bolt.hit_structure': ['data/sounds/laser_hit_building.ogg'] +
                                           ['data/sounds/laser_hit_building%d.ogg' % i for i in range(2, 11)],
    'sfx.weapon.laser_bolt.kill': ['data/sounds/laser_hitent%d.ogg' % i for i in range(1, 9)],
    'sfx.ui.match_ui.game_over': ['data/mwsounds/GameOver1.ogg', 'data/mwsounds/GameOver2.ogg'],
}
with open(a.origin, encoding='utf-8', newline='') as fh:
    ord_ = csv.DictReader(fh)
    ocols = ord_.fieldnames
    orows = list(ord_)
new_files = {r['species_filename']: r for r in NEW}
orows = [o for o in orows if not (o['species_filename'] in new_files and o['original_count'] == '0')]
by_path = {o['original_paths']: o for o in orows}
mapped = set()
for aid, paths in TAKE_MAP.items():
    fl = [r['species_filename'] for r in NEW if r['asset_id'] == aid]
    assert len(fl) == len(paths), aid
    for f, pth in zip(fl, paths):
        o = by_path[pth]
        o['species_filename'] = f
        o['asset_id'] = aid
        o['original_status'] = 'unbound take'
        mapped.add(f)
for r in NEW:
    if r['species_filename'] not in mapped:
        orows.append({'species_filename': r['species_filename'], 'asset_id': r['asset_id'], 'original_paths': '',
                      'original_count': '0', 'original_status': ''})
if not a.dry_run:
    with open(a.origin, 'w', encoding='utf-8', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=ocols, lineterminator='\n')
        w.writeheader()
        w.writerows(orows)

# ------------------------------------------------------------------ p4_new_assets.csv
if not a.dry_run:
    with open(a.new_assets, 'w', encoding='utf-8', newline='') as fh:
        w = csv.writer(fh, lineterminator='\n')
        w.writerow(['asset_id', 'consumer_file', 'cited_by_request'])
        for aid in sorted(NEW_IDS):
            cons = next(r['consumers'].split(';') for r in NEW if r['asset_id'] == aid)
            for c in cons:
                w.writerow([aid, c, NEW_REQ[aid]])

# ------------------------------------------------------------------ report
cat_ids = collections.defaultdict(set)
cat_files = collections.Counter()
cat_prio = collections.defaultdict(collections.Counter)
for r in out:
    cat_ids[r['category']].add(r['asset_id'])
    cat_files[r['category']] += 1
    cat_prio[r['category']][r['priority']] += 1
print('rows', len(out), 'ids', len({r['asset_id'] for r in out}), 'new ids', len(NEW_IDS), 'new rows', len(NEW))
for k, v in sorted(log.items()):
    print(' ', k, v)
tot = collections.Counter()
for c in cat_files:
    p = cat_prio[c]
    print('| %s | %d | %d | %d | %d | %d |' % (c, len(cat_ids[c]), cat_files[c], p['skirmish-first-playable'],
                                              p['launch'], p['post-launch']))
    tot.update(p)
print('total prio', dict(tot))
cols = collections.Counter()
for o in orows:
    if o['species_filename'].startswith('content/'):
        cols['origin rows with a Species file'] += 1
print(dict(cols), 'distinct', len({o['species_filename'] for o in orows if o['species_filename'].startswith('content/')}))
print('excluded classes:', collections.Counter(o['species_filename'] for o in orows if o['species_filename'].startswith('(')).most_common(3))
