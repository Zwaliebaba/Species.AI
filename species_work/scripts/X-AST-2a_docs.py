"""X-AST-2a: applies the pass-2a edits to GameAssets.md, README.md and art_and_audio_briefs.md.

Usage: python3 -I X-AST-2a_docs.py --design design_next --origin species_work/asset_origin_map.csv --census <scratch>/census.md
Reads the manifest written by X-AST-2a_apply.py. Every replacement states how often its old text must occur,
and the script stops without writing when one does not match. Run it once, on the pass-1 documents.
"""
import argparse
import collections
import csv
import os
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument('--design', required=True)
ap.add_argument('--origin', required=True)
ap.add_argument('--census', required=True, help='markdown of GameAssets §18.1, from X-AST-2a_strings.py --markdown')
a = ap.parse_args()
AD = os.path.join(a.design, 'assets')
man = list(csv.DictReader(open(os.path.join(AD, 'asset_manifest.csv'), encoding='utf-8')))
groups = collections.OrderedDict()
for r in man:
    groups.setdefault(r['asset_id'], []).append(r)
ERR = []


def rep(text, old, new, n=1, where=''):
    c = text.count(old)
    if c != n:
        ERR.append('%s: expected %d of %r, found %d' % (where, n, old[:90], c))
        return text
    return text.replace(old, new)


def rep_lines(text, scope, old, new, n, where=''):
    """Replace old by new inside lines that contain scope; exactly n lines must change."""
    lines = text.split('\n')
    k = 0
    for i, l in enumerate(lines):
        if scope in l and old in l:
            lines[i] = l.replace(old, new)
            k += 1
    if k != n:
        ERR.append('%s: scope %r old %r changed %d lines, expected %d' % (where, scope, old[:60], k, n))
    return '\n'.join(lines)


def table_end(lines, i):
    while i < len(lines) and lines[i].startswith('|'):
        i += 1
    return i


def insert_after(text, anchor, new_lines, where=''):
    lines = text.split('\n')
    idx = [i for i, l in enumerate(lines) if anchor in l]
    if len(idx) != 1:
        ERR.append('%s: anchor %r found %d times' % (where, anchor[:80], len(idx)))
        return text
    i = idx[0] + 1
    return '\n'.join(lines[:i] + new_lines + lines[i:])


def insert_table_end(text, anchor, new_lines, where=''):
    lines = text.split('\n')
    idx = [i for i, l in enumerate(lines) if anchor in l and l.startswith('|')]
    if len(idx) != 1:
        ERR.append('%s: table anchor %r found %d times' % (where, anchor[:80], len(idx)))
        return text
    i = table_end(lines, idx[0])
    return '\n'.join(lines[:i] + new_lines + lines[i:])


def esc(s):
    return s.replace('|', '\\|')


def pattern(paths):
    if len(paths) == 1:
        return '`%s`' % paths[0]
    m = [re.match(r'^(.*)_(\d\d)((?:_[a-z]+)?\.\w+)$', p) for p in paths]
    if all(m) and len(set((x.group(1), x.group(3)) for x in m)) == 1:
        nums = sorted(int(x.group(2)) for x in m)
        if nums == list(range(1, len(nums) + 1)):
            return '`%s_{01..%02d}%s` (%d files)' % (m[0].group(1), len(nums), m[0].group(3), len(nums))
    return None


def tex_rows(aid, fmt, per_member=False):
    g = groups[aid]
    pat = pattern([r['species_filename'] for r in g])
    specs = set(r['spec'] for r in g)
    if per_member or pat is None or len(specs) > 1:
        out = []
        for i, r in enumerate(g, 1):
            out.append('| `%s` | `%s` | %s | %s | %s | %s | %s | %s |' % (
                r['species_filename'], aid, esc(r['role']) if i == 1 else '″', fmt, esc(r['spec']),
                'single' if len(g) == 1 else '%d of %d' % (i, len(g)), r['priority'], r['placeholder']))
        return out
    r = g[0]
    return ['| %s | `%s` | %s | %s | %s | %s | %s | %s |' % (pat, aid, esc(r['role']), fmt, esc(r['spec']),
                                                          'single' if len(g) == 1 else str(len(g)), r['priority'],
                                                          r['placeholder'])]


def mesh_rows(aid, role_first_only=True):
    g = groups[aid]
    out = []
    for i, r in enumerate(g, 1):
        out.append('| `%s` | `%s` | %s | %s | %s | %s |' % (
            r['species_filename'], aid, esc(r['role']) if (i == 1 or not role_first_only) else '″', esc(r['spec']),
            'single' if len(g) == 1 else '%d of %d' % (i, len(g)), r['priority']))
    return out


EVENT = {'sfx.weapon.laser_bolt.spawn': 'every laser bolt fired (DEC-564)',
         'sfx.weapon.laser_bolt.hit_structure': 'a bolt hits a structure (DEC-564)',
         'sfx.weapon.laser_bolt.kill': 'a laser bolt or a turret round kills an actor (DEC-564)',
         'sfx.unit.trooper.throw_grenade': 'the Trooper throws the Strike team grenade (DEC-097)',
         'sfx.ui.match_ui.game_over': 'a result is declared: the game-over announcement (DEC-097, R-AUD-307)'}


def audio_row(aid):
    g = groups[aid]
    pat = pattern([r['species_filename'] for r in g])
    n = len(g)
    return '| %s | `%s` | %s | — | %s | %d variant%s | %s |' % (pat, aid, EVENT[aid], esc(g[0]['spec']), n,
                                                              '' if n == 1 else 's', g[0]['priority'])


def fmt_of(text, aid):
    for l in text.split('\n'):
        c = l.split(' | ')
        if l.startswith('|') and len(c) >= 8 and c[1] == '`%s`' % aid:
            return c[3]
    ERR.append('no formats cell for %s' % aid)
    return ''


# =================================================================== GameAssets.md
p = os.path.join(AD, 'GameAssets.md')
t = open(p, encoding='utf-8').read()
W = 'GameAssets'
t = rep(t, '> Status: Asset list, pass 1 (consumers named by document; pass 2 replaces them with requirement IDs)',
        '> Status: Asset list, pass 2a (consumers named by document; pass 2b replaces them with requirement IDs)', 1, W)

# --- 2.1 folders
t = rep(t, '| `content/textures/ui/` | UI panels, title wordmark, logos, front-end decoration, small UI icons |',
        '| `content/textures/ui/` | UI panels, title wordmark, logos, front-end decoration, small UI icons, the application icon |', 1, W)
t = rep(t, '| `content/textures/icons/markers/` | off-screen markers, ground markers, banners and flags |',
        '| `content/textures/icons/markers/` | off-screen markers, ground markers, route arrows, banners and flags with their tint masks |', 1, W)
t = rep(t, '| `content/ui/explainers/` | explainer animation scripts, one per mode |',
        '| `content/ui/explainers/` | explainer animation scripts, one per mode, each found by its `mode` field, never by its member order (DAT-284) |', 1, W)
t = rep(t, '| `content/meshes/<group>/` | flat-shaded meshes; groups `units`, `creatures`, `structures`, `projectiles`, `relics`, `scenery` |',
        '| `content/meshes/<group>/` | flat-shaded meshes; groups `units`, `creatures`, `structures`, `projectiles`, `relics`, `scenery`, and `dev` for the map editor\'s meshes |\n'
        '| `content/meshes/collision/` | collision assets: the hit geometry of every collidable mesh, authored as its own source (AST-044) |', 1, W)
t = rep(t, '| `content/strings/` | string tables, one per language |',
        '| `content/strings/` | string tables, one per language, and the development string table (development editions only) |', 1, W)
REC = [('modes', 'the match-option schema of each mode (`mode.options`)'),
       ('input', 'the default binding set and the keyboard-layout layers (`input.binding_set`, `input.keyboard_layouts`)'),
       ('actors', 'the actor stat table (`actor.stats`)'),
       ('crates', 'the Supply-crate outcome catalogue (`crate.outcomes`)'),
       ('audio', 'the sound sources with their events and layers (`sound.sources`), and the member weights of the sample groups (`sound.groups`, DAT-275)'),
       ('vfx', 'the effect definitions (`effects.definition`)'),
       ('timelines', 'the presentation timelines (`camera.timeline`)'),
       ('ui', 'page layouts, palettes, text styles and the font-role map (`interface.page_layout`, `interface.palette`, `interface.text_style`, `interface.font_roles`)'),
       ('editions', 'the edition manifests (`edition.manifest`)'),
       ('test', 'the soak configurations, selected only into the development and soak editions (`test.soak_configuration`)')]
t = insert_after(t, '| `content/thumbnails/` | map thumbnails, one per map |',
                 ['| `content/records/%s/` | %s |' % (f, d) for f, d in REC] +
                 ['', 'The `content/records/<family>/` folders hold the authored JSON data records of the record families of '
                  '[data schemas](../architecture/components/data_schemas/README.md) table 1-A (DAT-006). They are content '
                  'but not assets to create, so the manifest has no rows for them. A sample group\'s members are the '
                  'members of its asset in the order of its `variants` column; a group whose members are not equally '
                  'weighted lists its weights in `sound.groups` (DAT-275).'], W)

# --- 2.2 file names
t = insert_after(t, '- Maps follow `map_<mode>_<players>p_<nn>`; a thumbnail adds `_thumb`.',
                 ['- A **collision asset** (AST-044) is named after its render member with the suffix `_collision`, in '
                  '`content/meshes/collision/`; a **tint mask** (AST-022) is named after its parent member with the suffix `_tint`.',
                  '- A **derived variant** (AST-028) has no authored file: the cooker produces it offline from its parent\'s '
                  'source. Its row names its cooked entry, `<parent file without extension>@<variant id>` '
                  '(`content/textures/icons/cursors/cursor_world@shadow`).'], W)

# --- 2.3 ids
t = rep(t, '| `ui.` | UI panels and title art | `uitex.title_wordmark` |',
        '| `uitex.` | UI panels, title art and the application icon | `uitex.title_wordmark` |', 1, W)
t = rep(t, '| `mesh.<group>.` | meshes | `mesh.structure.spawn_node` |',
        '| `mesh.<group>.` | meshes; the group `collision` holds the collision assets, `dev` the editor meshes | `mesh.structure.spawn_node`, `mesh.collision.spawn_node` |', 1, W)
t = rep(t, '| `map.`, `thumb.` | maps and thumbnails, one id per mode | `mapfile.spawn_control` |',
        '| `mapfile.`, `thumb.` | maps and thumbnails, one id per mode | `mapfile.spawn_control` |\n'
        '| `<parent id>.<variant id>` | derived texture variants (AST-028): `shadow`, `trail` | `cursor.world.shadow` |\n'
        '| `<parent id>_tint` | companion tint masks of team-tinted cloth (AST-022) | `marker.leader_banner_tint` |', 1, W)

# --- 2.4 formats
t = rep(t, 'Every cooked texture is a DDS with the DX10 header; block-compressed textures are padded to whole 4x4 blocks by the cooker. ',
        'Every cooked texture is a DDS with the DX10 header. A block-compressed texture is a whole number of 4x4 blocks: '
        'its authored size (twice the display size for UI art) is a multiple of 4 in both dimensions, and the content '
        'build rejects any other size; the cooker never pads or resamples a source (ADR-0017, AST-068). ', 1, W)
t = rep(t, 'world textures are authored at the stated size. Audio sources are 48 kHz.',
        'world textures are authored at the stated size. Audio sources are 48 kHz. Samplers come from the closed set of '
        'ADR-0018: linear clamp-to-edge (the default), linear wrap, point clamp-to-edge, and point-magnify '
        'clamp-to-edge (point under magnification, linear under minification and between mips), which the particle '
        'dot and the front-end Worker figure use. Icons, markers and cursors are straight-alpha art that their passes '
        'may also draw with the additive blend (RND-393, RND-423). Nothing is recoloured at load, and derived variants '
        'are produced offline by the cooker (ADR-0018).', 1, W)
t = rep(t, '| team cloth | PNG RGBA8, sRGB, team area painted pure red (255,0,0) | BC7_UNORM_SRGB, full mips, one recolour per team at load | sRGB | straight alpha; pure-red texels take the team colour |',
        '| team cloth | PNG RGBA8, sRGB, team area painted as grey levels (its intensity), no key colour | BC7_UNORM_SRGB, full mips; no per-team copy | sRGB | straight alpha; the shader tints the team area by the weight of its companion tint mask (AST-022) |\n'
        '| tint mask | PNG 8-bit greyscale, white = full team tint, the size of its parent | BC4_UNORM, full mips | linear | team-tint weight read by the shader, never drawn on its own |\n'
        '| derived variant | none: the cooker derives it offline from its parent\'s source (`BlurredShadow`, `StackedCopies`) | its family\'s runtime format; shadows BC4_UNORM, full mips | as its family | as its family; shadows are darkening layers |\n'
        '| application icon | PNG RGBA8, sRGB, one file per icon size | Windows icon resource (.ico) built into the executable; not in the pack | sRGB | straight alpha |', 1, W)
t = rep(t, '| cooked Species mesh: face normals generated, colours attenuated at load | sRGB vertex colour | opaque; team parts painted mostly red |',
        '| cooked Species mesh: face normals generated, colours attenuated once at cook (RND-160) | sRGB vertex colour | opaque; team regions marked by explicit tint weights: the `_SPECIES_TINT` vertex attribute or the `species_team_tint` material flag (AST-034, ADR-0020) |\n'
        '| collision mesh | glTF 2.0 binary `.glb`: triangle primitives, no materials, metres, +Y up; authored as its own source | cooked `Collision` entry, simulation-relevant (DAT-278) | n/a | n/a |', 1, W)
t = rep(t, '| long audio and music | WAV, 48 kHz, 24-bit PCM, stereo for music | Opus stream, decoded on a worker thread | n/a | n/a |',
        '| long audio and music | WAV, 48 kHz, 24-bit PCM, stereo for music | Opus stream, decoded on a worker thread: every member of a group whose longest member is over 20 s (`aen.residency.stream_threshold`), and every music group (AEN-072) | n/a | n/a |', 1, W)

# --- 2.5 priorities
t = rep(t, '- structures: Spawn Node, Relay, Transit Gate, gun turret (mount, head, gun barrel), Barrier Emitter, wall, tree, jump pad, Supply crate;',
        '- structures: Spawn Node, Relay, Transit Gate, gun turret (mount, head, gun barrel), Barrier Emitter, wall, tree, jump pad, Supply crate, the port pad, and the collision assets of the collidable ones;', 1, W)
t = rep(t, 'the keyboard Tab glyph, the Spawn Control mode card, the primary font (Latin and Cyrillic) and the English string table;',
        'the keyboard Tab glyph, the Spawn Control mode card, the route arrow, the primary font (Latin and Cyrillic), the English string table, the development string table and the application icon;', 1, W)
t = rep(t, '- audio: every sample group of those sources and weapons, the match interface sounds and the match ambience bed.',
        '- audio: every sample group of those sources and weapons, with the laser-bolt fire, structure-hit and kill groups and the grenade throw, the match interface sounds with the game-over announcement, and the match ambience bed.', 1, W)
t = rep(t, '`post-launch` holds the seasonal theme, alternative mesh bodies and the few rows whose role still needs confirmation.',
        '`post-launch` holds the seasonal theme, alternative mesh bodies with their collision assets, the objective-card hint pictures (hook art, DEC-050), the map editor\'s camera-view marker and the few rows whose role still needs confirmation.', 1, W)

# --- 2.6 placeholders
t = insert_after(t, '| generated test map | flat map with the objects the mode needs for its player count |',
                 ['| full-weight mask | tint mask of weight 1 over the opaque area of its parent art |',
                  '| derived by the cooker | the variant derived from its parent\'s stand-in or authored file |',
                  '| primitive collision | collision triangles at the bounding shape of its render member |',
                  '',
                  'An edition that its edition manifest marks for release contains no placeholder: the content build fails '
                  'such an edition and lists the rows that have no authored file (AST-085). Every other edition may '
                  'contain placeholders.'], W)

# --- 4 unit sprites
t = rep_lines(t, '`sprite.visitor.front`', '| PNG RGBA8 → BC7 sRGB, mips | 128x128 px; alpha-test threshold 0.02 |',
              '| PNG RGBA8 (cut-out) → BC7 sRGB, mips | 128x128 px; drawn as a Worker card: alpha-test threshold 0.04 (RND-060, RND-316) |', 1, W)
t = rep_lines(t, '`sprite.worker.front`', 'magnified with NEAREST on the front-end backdrop',
              'magnified on the front-end backdrop through the point-magnify clamp-to-edge sampler (ADR-0018)', 1, W)
t = insert_after(t, '| `sprite.crawler.segment` |', tex_rows('sprite.crawler.segment.trail',
                 'derived from `sprite.crawler.segment` → BC4, mips'), W)

# --- 5 VFX
t = rep_lines(t, '`fx.sky_cloud`', 'blobby layer MAG LINEAR / MIN NEAREST, flat layer NEAREST; wrap repeat; small tile magnified on purpose',
              groups['fx.sky_cloud'][0]['spec'].split('16x16 px; ', 1)[1], 1, W)
t = rep_lines(t, '`fx.beam_short`', 'wrap repeat for the sky grid; drawn without mips by the sky and laser-bolt passes',
              groups['fx.beam_short'][0]['spec'].split('128x32 px; ', 1)[1], 1, W)
t = rep_lines(t, '`fx.particle_dot`', 'MAG NEAREST (crisp pixels) in particle batches',
              groups['fx.particle_dot'][0]['spec'].split('16x16 px; ', 1)[1], 1, W)
t = rep_lines(t, '`fx.shard_edges`', '| Edge-line pattern decaled on shatter shards |', '| %s |' % groups['fx.shard_edges'][0]['role'], 1, W)
t = insert_table_end(t, '`fx.terrain_grid`', tex_rows('fx.abductor_beam_ring', fmt_of(t, 'fx.tree_canopy')), W)

# --- 6 UI
for aid, old, new in [('uitex.logo_banner', '394x149 px display size; author at 2x (788x298)', '394x150 px display size; author at 2x (788x300)'),
                      ('uitex.title_wordmark', '659x122 px display size; author at 2x (1318x244)', '660x122 px display size; author at 2x (1320x244)'),
                      ('uitex.logo_emblem', '277x194 px display size; author at 2x (554x388)', '278x194 px display size; author at 2x (556x388)')]:
    t = rep_lines(t, '`%s`' % aid, old, new, 1, W)
t = rep_lines(t, '`uitex.logo_banner`', '| Large title logo on the main menu |', '| %s |' % groups['uitex.logo_banner'][0]['role'], 1, W)
t = rep_lines(t, '`uitex.logo_banner`', '| single | launch | generated panel |', '| single | post-launch | generated panel |', 1, W)
t = rep(t, 'UI pictures keep exact pixels: no mip-maps, linear filtering. Brief',
        'UI pictures keep exact pixels: no mip-maps, linear filtering. Authored sizes are multiples of 4 (ADR-0017). Brief', 1, W)
t = insert_table_end(t, '`uitex.backdrop_wheel`', tex_rows('uitex.app_icon', 'PNG RGBA8 per size → icon resource in the executable', per_member=True), W)

# --- 8 cursors and markers
t = rep(t, 'The runtime blurs cursors and markers into darkening shadows; banners and flags paint the team area pure red.',
        'The cooker derives the blurred darkening shadows of the shadowed cursors and of the crate landing and barrage '
        'markers offline, as derived variants; banners and flags carry the team area as grey levels, with a companion '
        'tint mask each.', 1, W)
t = rep_lines(t, '`cursor.world`', 'blurred darkening shadow generated at run time',
              'its blurred darkening shadow is the derived variant `cursor.world.shadow`, produced offline by the cooker', 1, W)
t = rep_lines(t, '`cursor.place`', '| Placement cursor for powers that place a structure |', '| %s |' % groups['cursor.place'][0]['role'], 1, W)
t = rep_lines(t, '`cursor.pointer`', '| Pointer arrow for menus; also the scroll-bar arrow |', '| %s |' % groups['cursor.pointer'][0]['role'], 1, W)
t = rep_lines(t, '`marker.leader_banner`', '| Leader banner per order state: idle, follow, formation, gather |',
              '| %s |' % esc(groups['marker.leader_banner'][0]['role']), 1, W)
for aid in ('marker.leader_banner', 'marker.flag_post_state'):
    t = rep_lines(t, '`%s`' % aid, 'PNG RGBA8 (team red) → BC7 sRGB, mips, per-team', 'PNG RGBA8 (team area grey) → BC7 sRGB, mips; tint mask', 4, W)
    t = rep_lines(t, '`%s`' % aid, 'cloth texture; team area pure red', 'cloth texture; team area as grey levels, with its companion tint mask `%s_tint`' % aid, 4, W)
t = insert_after(t, '`content/textures/icons/markers/marker_edge_compass.png`',
                 tex_rows('marker.edge_compass_empty', fmt_of(t, 'marker.edge_compass')), W)
t = insert_after(t, '`content/textures/icons/markers/leader_banner_gather.png`',
                 tex_rows('marker.leader_banner_tint', 'PNG grey (mask) → BC4, mips', per_member=True), W)
t = insert_after(t, '`content/textures/icons/markers/flag_post_raised.png`',
                 tex_rows('marker.flag_post_state_tint', 'PNG grey (mask) → BC4, mips', per_member=True), W)
new8 = tex_rows('marker.route_arrow', fmt_of(t, 'cursor.world'))
for pid in ['cursor.world', 'cursor.place', 'cursor.unavailable', 'cursor.move_target', 'cursor.highlight', 'cursor.select',
            'cursor.enter', 'marker.crate_landing', 'marker.barrage_target']:
    new8 += tex_rows(pid + '.shadow', 'derived from `%s` → BC4, mips' % pid)
t = insert_table_end(t, '`content/textures/icons/cursors/cursor_world.png`', new8, W)

# --- 9 input glyphs
new9 = []
for g in ['stick_left', 'stick_right', 'dpad_horizontal']:
    new9 += tex_rows('glyph.pad.' + g, fmt_of(t, 'glyph.pad.face_south'))
    new9 += tex_rows('glyph.pad.%s_shadow' % g, fmt_of(t, 'glyph.pad.face_shadow'))
t = insert_table_end(t, '`glyph.key.tab`', new9, W)

# --- 10 terrain
t = rep_lines(t, '`terrain.ramp`', '| 64x64 px; x = slope, y = height |',
              '| 64x64 px; %s |' % groups['terrain.ramp'][1]['spec'].split('64x64 px; ', 1)[1], 1, W)
t = rep_lines(t, '`terrain.shore_wave_gradient`', '| 400x10 px; the wave colour runs along x |',
              '| 400x10 px; %s |' % groups['terrain.shore_wave_gradient'][1]['spec'].split('400x10 px; ', 1)[1], 1, W)

# --- 11 meshes
t = rep(t, 'Parts that take the team colour are painted "mostly red" (red more than twice green and twice blue) on the root node and its direct children; the renderer recolours them.',
        'Parts that take the team colour carry explicit tint weights on the root node and its direct children: the '
        '`_SPECIES_TINT` vertex attribute or the `species_team_tint` material flag (AST-034, ADR-0020). The renderer '
        'tints them in the shader and nothing is recoloured at load; an authoring helper may propose weights for art made '
        'in the "mostly red" key-colour style (red more than twice green and twice blue), which the artist reviews.', 1, W)
t = rep(t, 'Formats: `.glb` (glTF 2.0 binary) → cooked Species mesh; placeholder: primitive mesh with sockets.',
        'Formats: `.glb` (glTF 2.0 binary) → cooked Species mesh, colours attenuated once at cook (RND-160); placeholder: '
        'primitive mesh with sockets. Indexed sockets are numbered from 01 without gaps (AST-038).', 1, W)
t = rep_lines(t, '`mesh.unit.worker_figure`', 'team-recoloured mesh', 'team-tinted mesh', 1, W)
t = rep_lines(t, '`mesh.creature.spider_body`', 'leg_00, leg_01, leg_02, leg_03, leg_04, leg_05, leg_06, leg_07',
              'leg_01, leg_02, leg_03, leg_04, leg_05, leg_06, leg_07, leg_08', 1, W)
t = rep_lines(t, '`mesh.structure.structure_default`', '| default structure body assigned before a structure sets its own mesh |',
              '| %s |' % groups['mesh.structure.structure_default'][0]['role'], 1, W)
t = rep_lines(t, '`mesh.structure.structure_default`', '| single | post-launch |', '| single | skirmish-first-playable |', 1, W)
t = insert_table_end(t, 'content/meshes/scenery/scenery_prop_07.glb', mesh_rows('mesh.dev.camera_view'), W)
coll = [aid for aid in groups if aid.startswith('mesh.collision.')]
sub = ['', '### 11.1 Collision assets', '',
       'Every collidable mesh, that is every mesh that a structure\'s sphere, mesh or ray test reads ([buildings and world '
       'objects](../reference/03_buildings_and_world_objects/README.md) R-BLDG-041 to R-BLDG-043), has a collision asset: '
       'a separate glTF 2.0 file of triangle primitives with no materials, authored as its own source and never generated '
       'from render geometry, cooked into a simulation-relevant `Collision` entry ([asset pipeline](../architecture/components/asset_pipeline_and_resources.md) AST-044). '
       'A collision asset has one member per member of its render mesh, in the same order, and the render mesh\'s import '
       'record names it. The structures that the hit tests never read (Launch Vessel, Control Zone, carryable objects, bot '
       'navigation nodes and the other kinds of table 2.12-A that are never hit), the jump pad, trees and the socket-only '
       'turret mount have none. Placeholder: primitive collision.', '',
       '| Species file | Asset id | Role | Spec | Variants | Priority |', '|---|---|---|---|---|---|']
for aid in coll:
    sub += mesh_rows(aid)
lines = t.split('\n')
i12 = [i for i, l in enumerate(lines) if l.startswith('## 12 ')]
lines = lines[:i12[0] - 1] + sub + lines[i12[0] - 1:]
t = '\n'.join(lines)

# --- 12 help
t = rep_lines(t, '`help.explainer_picture`', 'PNG RGBA8 → BC7 sRGB, no mips', 'PNG RGBA8 → BC7 sRGB, mips', 25, W)
for f, o, n in [('explainer_fuel_pump.png', '181x256', '184x256'), ('explainer_fuel_pump_arm.png', '50x256', '52x256'),
                ('explainer_launch_vessel.png', '238x256', '240x256'), ('explainer_solar_array.png', '158x256', '160x256'),
                ('explainer_turret_barrel.png', '140x90', '140x92'), ('explainer_turret_base.png', '140x110', '140x112'),
                ('explainer_core_device.png', '207x256', '208x256'), ('explainer_zone_back.png', '512x310', '512x312'),
                ('explainer_zone_front.png', '512x310', '512x312')]:
    t = rep_lines(t, '/%s`' % f, o + ' px', n + ' px', 1, W)
t = rep_lines(t, 'help_general_page_03.png', '150x149 px display size; author at 2x (300x298)',
              '150x150 px display size; author at 2x (300x300)', 1, W)
t = rep_lines(t, '`help.hint.camera_controls`', '512x128 px display size; author at 2x (1024x256)',
              '304x128 px display size; author at 2x (608x256)', 1, W)
HINTS = [x for x in groups if x.startswith('help.hint.') and x != 'help.hint.camera_controls']
for h in HINTS:
    role = groups[h][0]['role']
    base = role.split('; objective-card hook art')[0]
    t = rep_lines(t, '`%s`' % h, '| %s |' % base, '| %s |' % role, 1, W)
    t = rep_lines(t, '`%s`' % h, '| single | launch | diagram card |', '| single | post-launch | diagram card |', 1, W)
t = rep(t, 'Choreography (what moves when) is new; canvas size and loop length follow the reference.',
        'Choreography (what moves when) is new; canvas size and loop length follow the reference. The explainer panel '
        'samples its pictures linearly with mip-maps (R-META-231), so they carry full mip chains. The objective-card hint '
        'pictures are hook art: no rule of a reference match draws them (DEC-050, R-TUT-013).', 1, W)

# --- 13 audio
t = rep(t, 'Formats: WAV 48 kHz 24-bit → 16-bit PCM resident (Opus stream for files over 20 s).',
        'Formats: WAV 48 kHz 24-bit → 16-bit PCM resident, or an Opus stream for every member of a group whose longest '
        'member is over 20 s (`aen.residency.stream_threshold`, AEN-072); the resident groups hold about 173 MiB (AEN-304).', 1, W)
t = rep(t, '182 (source, event) bindings fire in a match and play these groups. A further 98 events fire with no bound sound; they stay silent in the reference, so the event table lists them and no file is needed.',
        '182 (source, event) bindings of the reference fire in a match and play these groups. Of the events that fire with '
        'no bound sound in the reference, four gain new groups here by Resolved decisions (the laser-bolt fire, '
        'structure-hit and kill groups of DEC-564 and the Trooper grenade throw of DEC-097), the game-over announcement '
        'is a UI sound (§14), and the victory and defeat events play the music stings of §16; every other one stays '
        'silent and needs no file ([event catalogue](../reference/10_audio_design/event_catalogue.md) R-AUD-141 to R-AUD-143).', 1, W)
t = rep(t, '### 13.3 Trooper (48 files)', '### 13.3 Trooper (52 files)', 1, W)
t = rep(t, '### 13.36 laser bolt (36 files)', '### 13.36 laser bolt (64 files)', 1, W)
t = insert_after(t, '`sfx.unit.trooper.exit_gate`', [audio_row('sfx.unit.trooper.throw_grenade')], W)
t = insert_after(t, '`sfx.weapon.laser_bolt.hit_bark`', [audio_row(x) for x in
                 ['sfx.weapon.laser_bolt.spawn', 'sfx.weapon.laser_bolt.hit_structure', 'sfx.weapon.laser_bolt.kill']], W)
t = rep(t, 'Interface sounds: menus, selection, countdown, chat and the power-slot panel.',
        'Interface sounds: menus, selection, countdown, chat, the power-slot panel and the game-over announcement.', 1, W)
t = insert_after(t, '`sfx.ui.match_ui.chat_message`', [audio_row('sfx.ui.match_ui.game_over')], W)
t = rep(t, 'Each bed has layers that play together; long beds stream. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident or Opus stream (over 20 s).',
        'Each bed has layers that play together. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident, or an Opus stream for '
        'every member of a group whose longest member is over 20 s (AEN-072).', 1, W)

# --- 18 strings (§18.1 comes from X-AST-2a_strings.py --markdown)
t = rep(t, '| team and faction names | `match.team.*` | 11 |', '| team colour and faction names | `team.colour.*`, `faction.*` | 11 |', 1, W)
t = rep(t, 'Pass 2 collects every string id the documents use; the counts below size the work from the reference text categories.',
        'The first table sizes the work from the reference text categories; §18.1 lists the string ids that the documents cite.', 1, W)
t = insert_table_end(t, '`strings.ko_kr`',
                     ['| `content/strings/dev_en_gb.json` | `strings.dev_en_gb` | English (United Kingdom), development captions `ui.dev.*` only; in development editions only, never in a player edition | skirmish-first-playable | pseudo-localised text |'], W)
census = open(a.census, encoding='utf-8').read().rstrip('\n').split('\n')
t = insert_after(t, 'The language set is the reference set of nine; the design phase confirms it.', [''] + census, W)

# --- 20 not created
t = rep(t, '| unused (bound to no sound event) |  |  | 192 |  |  | 192 |\n', '', 1, W)
t = insert_after(t, '| editor only (map editor windows) |  |  |  |  | 164 | 164 |',
                 ['| unused (bound to no sound event) |  |  | 162 |  |  | 162 |'], W)
t = rep(t, '| **Total** | **115** | **201** | **434** | **67** | **281** | **1098** |',
        '| **Total** | **115** | **201** | **404** | **67** | **281** | **1068** |', 1, W)

# --- 3 summary
CATS = ['Unit sprites', 'VFX textures', 'UI panels and title', 'HUD, supply-crate and power icons', 'Cursors and markers',
        'Input glyphs', 'Terrain ramps, water and waves', 'Meshes', 'Help images and explainer animations',
        'Audio by source and event', 'UI sounds', 'Ambience', 'Music', 'Fonts', 'String table', 'Maps and thumbnails']
ANCH = {'Unit sprites': '4-unit-sprites', 'VFX textures': '5-vfx-textures', 'UI panels and title': '6-ui-panels-and-title',
        'HUD, supply-crate and power icons': '7-hud-supply-crate-and-power-icons', 'Cursors and markers': '8-cursors-and-markers',
        'Input glyphs': '9-input-glyphs', 'Terrain ramps, water and waves': '10-terrain-ramps-water-and-waves',
        'Meshes': '11-meshes', 'Help images and explainer animations': '12-help-images-and-explainer-animations',
        'Audio by source and event': '13-audio-by-source-and-event', 'UI sounds': '14-ui-sounds', 'Ambience': '15-ambience',
        'Music': '16-music', 'Fonts': '17-fonts', 'String table': '18-string-table', 'Maps and thumbnails': '19-maps-and-thumbnails'}
cat_of = {r['species_filename']: r['category'] for r in man}
orig = collections.Counter()
for o in csv.DictReader(open(a.origin, encoding='utf-8')):
    f = o['species_filename']
    if f in cat_of and o['original_count'].isdigit() and o['original_status'] != 'player-facing':  # text entries are not files
        orig[cat_of[f]] += int(o['original_count'])
ids = collections.defaultdict(set)
files = collections.Counter()
pr = collections.defaultdict(collections.Counter)
for r in man:
    ids[r['category']].add(r['asset_id'])
    files[r['category']] += 1
    pr[r['category']][r['priority']] += 1
assert set(files) == set(CATS), set(files) ^ set(CATS)
summ = ['| Category | Asset ids | Files | first-playable | launch | post-launch | Reference files replaced |',
        '|---|---:|---:|---:|---:|---:|---:|']
T = collections.Counter()
for c in CATS:
    vals = [len(ids[c]), files[c], pr[c]['skirmish-first-playable'], pr[c]['launch'], pr[c]['post-launch'], orig[c]]
    T.update(dict(zip('abcdef', vals)))
    summ.append('| [%s](#%s) | %d | %d | %d | %d | %d | %d |' % (c, ANCH[c], *vals))
summ.append('| **Total** | **%d** | **%d** | **%d** | **%d** | **%d** | **%d** |' % tuple(T[k] for k in 'abcdef'))
lines = t.split('\n')
s0 = lines.index('## 3 Summary')
h = next(i for i in range(s0, len(lines)) if lines[i].startswith('| Category |'))
e = table_end(lines, h)
lines = lines[:h] + summ + lines[e:]
t = '\n'.join(lines)
t = rep(t, 'Music, string tables and some help pictures have no one-to-one counterpart.',
        'Music, string tables, some help pictures and every row added in pass 2a without a reference take (the '
        'application icon, the new glyphs, markers and the Abductor beam ring, the tint masks, the collision assets, '
        'the derived variants and the Trooper grenade throw) have no one-to-one counterpart. "Files" counts manifest rows: '
        'the %d derived-variant rows have no authored file, because the cooker produces them from their parents.'
        % sum(1 for r in man if r['subcategory'] == 'derived'), 1, W)
GA = t

# =================================================================== README.md
p2 = os.path.join(AD, 'README.md')
r_ = open(p2, encoding='utf-8').read()
W = 'README'
r_ = rep(r_, '> Status: Asset overview, pass 1', '> Status: Asset overview, pass 2a', 1, W)
r_ = rep(r_, 'Consumers are documents in pass 1; pass 2 replaces them with requirement IDs.',
         'Consumers are documents in passes 1 and 2a; pass 2b replaces them with requirement IDs.', 1, W)
lines = r_.split('\n')
h = next(i for i, l in enumerate(lines) if l.startswith('| Category | Asset ids |'))
e = table_end(lines, h)
rs = ['| Category | Asset ids | Files | first-playable | launch | post-launch |', '|---|---:|---:|---:|---:|---:|']
for row in summ[2:-1]:
    c = row.split(' | ')
    rs.append('| %s | %s |' % (re.sub(r'^\| \[(.*)\]\(#.*\)$', r'\1', c[0]), ' | '.join(c[1:6])))
rs.append('| **Total** | ' + ' | '.join(summ[-1].split(' | ')[1:6]) + ' |')
lines = lines[:h] + rs + lines[e:]
r_ = '\n'.join(lines)
r_ = rep(r_, '817 reference files and 281 reference text entries need no Species counterpart',
         '787 reference files and 281 reference text entries need no Species counterpart', 1, W)
r_ = rep(r_, 'the particle dot is magnified with NEAREST |',
         'the particle dot is magnified through the point-magnify clamp-to-edge sampler (ADR-0018) |', 1, W)
r_ = rep(r_, '| Cursors and markers | Cursor and marker art with run-time blurred darkening shadows; markers drawn as tinted alpha masks; banners and flags paint the team area pure red |',
         '| Cursors and markers | Straight-alpha art, drawn additively or alpha-blended as the pass states (RND-393); blurred darkening shadows are derived variants that the cooker produces offline; compass and disc shadows are tinted alpha masks; banners and flags carry the team area as grey levels with a companion tint mask (AST-022) |', 1, W)
r_ = rep(r_, 'team parts mostly red on the root and its direct children;',
         'team regions marked by explicit tint weights on the root and its direct children (ADR-0020); a collision asset for every collidable mesh (AST-044);', 1, W)
r_ = rep(r_, '| UI panels and title | Straight alpha or opaque, no mip-maps, linear filtering; authored at 2x display size;',
         '| UI panels and title | Straight alpha or opaque, no mip-maps, linear filtering; authored at 2x display size, a multiple of 4 in both dimensions (ADR-0017); the application icon is a Windows icon resource with six sizes;', 1, W)
r_ = rep(r_, '| Help images and explainer animations | No baked text; hint pictures 200x200, help pages 150x150, mode cards 300x200 (display sizes);',
         '| Help images and explainer animations | No baked text; hint pictures 200x200, help pages 150x150, mode cards 300x200 (display sizes); explainer pictures with full mip chains;', 1, W)
r_ = rep(r_, '| Ambience | 2-D layered loops; streamed when long |',
         '| Ambience | 2-D layered loops; a group streams when its longest member is over 20 s (AEN-072) |', 1, W)
r_ = rep(r_, '| String table | New text only; ICU MessageFormat; one table per language; the reference set of nine languages |',
         '| String table | New text only; ICU MessageFormat; one table per language; the reference set of nine languages; a development string table in development editions only |', 1, W)
r_ = rep(r_, '| generated test map | flat map with the objects the mode needs for its player count |',
         '| generated test map | flat map with the objects the mode needs for its player count |\n'
         '| full-weight mask | tint mask of weight 1 over the opaque area of its parent art |\n'
         '| derived by the cooker | the variant derived from its parent\'s stand-in or authored file |\n'
         '| primitive collision | collision triangles at the bounding shape of its render member |', 1, W)
r_ = rep(r_, 'The build also validates the other direction: every asset id that the sound-event data, the maps or the explainer scripts reference must exist in the manifest (logged, never fatal), and unused files are reported.',
         'An edition that its edition manifest marks for release contains no placeholder: the content build fails such an '
         'edition and lists the rows that have no authored file (AST-085). The build also validates the other direction: '
         'an asset id that the sound-event data, the maps or the explainer scripts reference and the manifest lacks is a '
         'build error (AST-049, DEC-015), while a file missing at run time is a diagnostic, never fatal; unused files are reported.', 1, W)
RD = r_

# =================================================================== art_and_audio_briefs.md
p3 = os.path.join(AD, 'art_and_audio_briefs.md')
b = open(p3, encoding='utf-8').read()
W = 'briefs'
b = rep(b, '> Status: Style guide and category briefs, pass 1', '> Status: Style guide and category briefs, pass 2a', 1, W)
b = rep(b, '- The renderer attenuates mesh colours at load (x 0.85), team-recoloured parts further (x 0.7), the Transport hull strongly (x 0.1) and the green team\'s Worker figure by half (x 0.5).',
        '- The mesh cooker attenuates every mesh colour once, at cook (x 0.85, RND-160); the renderer scales team-tinted parts further (x 0.7), the Transport hull strongly (x 0.1) and the green team\'s Worker figure by half (x 0.5).', 1, W)
b = rep(b, '- Shadows of cursors, markers and HUD icons are blurred copies; the content build may generate them from the base art, or an artist may author them as greyscale.',
        '- Shadows of cursors, of the crate landing marker and of the barrage marker are blurred copies of their art: derived variants that the cooker produces offline, never at load or at run time (RND-422, ADR-0018), or greyscale darkening art that an artist authors instead. The compass and team-disc shadows and the glyph shadows are authored art.', 1, W)
b = rep(b, '- Meshes mark team parts by painting them mostly red (red more than twice green and twice blue) on the root node and its direct children; the renderer replaces that colour with the team colour while keeping its brightness. Cloth banners and flags paint the team area pure red (255, 0, 0).',
        '- Meshes mark team parts with explicit tint weights on the root node and its direct children: the `_SPECIES_TINT` vertex attribute or the `species_team_tint` material flag (AST-034, ADR-0020). The renderer tints them in the shader while keeping their brightness; the cooker never infers a weight from a colour. An artist who works in the "mostly red" key-colour style (red more than twice green and twice blue) runs the authoring helper, which writes explicit weights for review.\n'
        '- Cloth banners and flags carry the team area as grey levels (its intensity) and a companion tint mask whose white marks full team tint (AST-022). Nothing is recoloured at load and no per-team copy exists; the look is unchanged.', 1, W)
b = rep(b, '- Some textures are magnified with nearest-neighbour filtering on purpose: the particle dot, the flat cloud layer, the full-screen pixel passes and the front-end backdrop figure. Their art is designed as clean pixel shapes at the stated size.',
        '- Some textures are magnified with point filtering on purpose, for a blocky pixel look (RND-432): the particle dot and the front-end backdrop figure through the point-magnify clamp-to-edge sampler (linear under minification), the flat cloud layers, and the captured frame of the screen transition. Their art is designed as clean pixel shapes at the stated size.', 1, W)
b = rep(b, '- Front-end pictures keep exact pixels: no mip-maps, linear filtering, authored at twice the display size.',
        '- Front-end pictures are authored at twice their display size and drawn with linear filtering. Menu art has no mip-maps; the explainer pictures carry full mip chains, because the explainer panel samples them with mip-maps (R-META-231).\n'
        '- Block-compressed art is a whole number of 4x4 blocks: its authored size is a multiple of 4 in both dimensions, and the content build rejects any other size (ADR-0017).', 1, W)
b = rep(b, '**15 files, 12 asset ids.**', '**16 rows, 13 asset ids.**', 1, W)
b = rep(b, '- The Visitor card is a distinct faction figure with soft alpha (threshold 0.02).',
        '- The Visitor card is a distinct faction figure, drawn as a Worker card with a hard cut-out edge (threshold 0.04).\n'
        '- The Crawler trail is a derived variant: the cooker stacks 18 shrinking copies of the Crawler card into a 32x512 column (RND-320); nothing is authored for it.', 1, W)
b = rep(b, '**18 files, 18 asset ids**, all additive greyscale on black except the tree canopy mask (an alpha mask) and the shatter-shard edge pattern (a cut-out decal).',
        '**19 files, 19 asset ids**, all additive greyscale on black except the tree canopy mask and the Abductor beam ring (alpha masks) and the shatter-shard edge pattern, an art reference for the edge lines that the shard shader draws from a barycentric mask (VFX-229); it is not sampled at run time.', 1, W)
b = rep(b, 'canopy mask 128x32; shard edge pattern 128x128.', 'canopy mask 128x32; shard edge pattern 128x128; Abductor beam ring 128x32.', 1, W)
b = rep(b, '- Tiled and scrolled textures (beams in the sky grid, barrier fields, fuel flow, terrain grid, cloud tile) tile seamlessly along their repeat axis.',
        '- Tiled and scrolled textures (barrier fields, fuel flow, terrain grid, cloud tile, the Abductor beam ring) tile seamlessly along their repeat axis. The sky grid draws the short beam once per line, clamped (RND-151).', 1, W)
b = rep(b, '**15 files, 13 asset ids**:', '**21 files, 14 asset ids**:', 1, W)
b = rep(b, 'a check-box tick, a lock icon and the front-end backdrop wheel.',
        'a check-box tick, a lock icon, the front-end backdrop wheel and the application icon.', 1, W)
b = rep(b, '- All UI art is authored at 2x display size, opaque or straight alpha, no mip-maps.',
        '- All UI art is authored at 2x display size, opaque or straight alpha, no mip-maps.\n'
        '- The application icon is the executable\'s and the window\'s icon: six PNG files, one per size (16, 24, 32, 48, 64 and 256 px), each drawn for its size, which the build combines into one Windows icon resource.', 1, W)
b = rep(b, '**28 files, 22 asset ids**: nine cursors, eleven markers (off-screen compass and its shadow, team disc and its shadow, Supply crate and Relic markers, crate landing and barrage target ground markers, flag pointer, jump-pad arrow, order trail), four Leader banners and four Flag Chain flag states.',
        '**47 rows, 35 asset ids**: nine cursors, thirteen markers (off-screen compass, its empty variant and its shadow, team disc and its shadow, Supply crate and Relic markers, crate landing and barrage target ground markers, flag pointer, jump-pad arrow, order trail, route arrow), four Leader banners and four Flag Chain flag states with a tint mask each, and nine blurred shadows that the cooker derives.', 1, W)
b = rep(b, '- Cursors are 128x128 (the pointer 64x64), white art; the runtime adds a blurred darkening shadow. Each cursor state must be distinguishable by shape alone.',
        '- Cursors are 128x128 (the pointer 64x64), white art; the cooker derives the blurred darkening shadow of every cursor of the set except the turret-aim crosshair, offline. Each cursor state must be distinguishable by shape alone.\n'
        '- The route arrow marks the direction along route lines (RND-353); the empty compass is the compass ring without its arrow, which on-screen markers draw (RND-357).', 1, W)
b = rep(b, 'Both are cloth textures with the team area in pure red, read on a moving cloth at small size.',
        'Both are cloth textures that carry the team area as grey levels with a companion tint mask (white = full team tint), read on a moving cloth at small size.', 1, W)
b = rep(b, '**13 files, 13 asset ids**: four face buttons by position (south, east, west, north), two shoulder buttons, two triggers, three shadow layers and the keyboard Tab key.',
        '**19 files, 19 asset ids**: four face buttons by position (south, east, west, north), two shoulder buttons, two triggers, two sticks, the horizontal d-pad, six shadow layers and the keyboard Tab key.', 1, W)
b = rep(b, '- 64x64 for face buttons, 128x128 for the Tab key,', '- 64x64 for face buttons, sticks, the d-pad and their shadows, 128x128 for the Tab key,', 1, W)
b = rep(b, '- A ramp is a 64x64 table: x is slope, y is height.',
        '- A ramp is a 64x64 table: x is slope (column 0 flat ground, column 63 vertical), y is height, read with row 0 at the bottom of the image as viewed: the bottom row is the highest terrain and the top row sea level (TER-040). Any other size is rejected.', 1, W)
b = rep(b, '- A shore-wave gradient is a 400x10 table; the sea renderer reads one row along the wave.',
        '- A shore-wave gradient is a 400x10 table; the sea renderer reads pixel row 1 counted from the bottom row (the second row from the bottom), one colour per column (TER-041).', 1, W)
b = rep(b, '**71 files, 53 asset ids**: unit figures and bodies (7), projectiles (4), creatures (13), structures (32), eight Relics and seven scenery pieces.',
        '**104 files, 77 asset ids**: 71 render meshes in 53 asset ids (unit figures and bodies 7, projectiles 4, creatures 13, structures 32, eight Relics and seven scenery pieces), 32 collision assets in 23 asset ids, and the map editor\'s camera-view marker.', 1, W)
b = rep(b, '- Team parts painted mostly red on the root node and its direct children.',
        '- Team parts carry explicit tint weights (the `_SPECIES_TINT` attribute or the `species_team_tint` material flag) on the root node and its direct children; see §2.5.\n'
        '- Indexed sockets are numbered from 01 without gaps (`port_01`, `leg_01` to `leg_08`).\n'
        '- Every collidable mesh has a collision asset: triangle-only geometry with no materials, authored as its own source at the render member\'s bounds. The hit tests read it, never the render mesh.', 1, W)
b = rep(b, 'the camera-controls hint 512x128 (display sizes; author at 2x).',
        'the camera-controls hint 304x128 (display sizes; author at 2x).\n'
        '- The 16 objective-card hint pictures are hook art: no rule of a reference match draws them (DEC-050), so they are post-launch.', 1, W)
b = rep(b, 'The scripts reference their pictures by asset id; the pictures are cut-outs with soft alpha.',
        'The scripts reference their pictures by asset id; the pictures are cut-outs with soft alpha and full mip chains.', 1, W)
b = rep(b, '**863 files in 167 sample groups**', '**895 files in 171 sample groups**', 1, W)
b = rep(b, '- Each group keeps the reference variant count, so the mix keeps its variety:',
        '- Four groups give sound to moments the reference leaves silent, by Resolved decisions: the laser-bolt fire, structure-hit and kill groups (DEC-564) and the Trooper grenade throw (DEC-097). Their variant counts follow the reference set\'s unbound takes for those moments where it has them.\n'
        '- Each group keeps the reference variant count, so the mix keeps its variety:', 1, W)
b = rep(b, '**14 files in 12 groups**: menu hover, select and back, unit select and highlight, state toggle, invalid action, countdown tick, chat message, selection circle, and the power-slot panel sounds.',
        '**16 files in 13 groups**: menu hover, select and back, unit select and highlight, state toggle, invalid action, countdown tick, chat message, selection circle, the power-slot panel sounds, and the game-over announcement (two takes, played before the victory or defeat sting).', 1, W)
b = rep(b, 'Long beds stream.', 'A bed group streams when its longest member is over 20 s (AEN-072).', 1, W)
b = rep(b, '**9 string tables** (en-GB first, then French, German, Italian, Spanish, Portuguese, Russian, Japanese, Korean).',
        '**9 string tables** (en-GB first, then French, German, Italian, Spanish, Portuguese, Russian, Japanese, Korean), and a development string table (en-GB only, development editions only).', 1, W)
b = rep(b, '- ICU MessageFormat for counts, plurals and names.',
        '- ICU MessageFormat for counts, plurals and names.\n'
        '- [GameAssets §18.1](GameAssets.md#181-string-ids-cited-by-the-documents) lists the string ids that the documents cite, by family.', 1, W)
BR = b

if ERR:
    print('\n'.join(ERR), file=sys.stderr)
    sys.exit(1)
open(p, 'w', encoding='utf-8').write(GA)
open(p2, 'w', encoding='utf-8').write(RD)
open(p3, 'w', encoding='utf-8').write(BR)
print('written', len(GA), len(RD), len(BR))
