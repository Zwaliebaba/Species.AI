#!/usr/bin/env python3
"""S25 synthesis: generate the catalogue tables of 25_textures_and_assets.md and assemble the document.

Usage:  python3 -I S_25_catalogue.py <OUT/_work dir> <OUT design dir>

Reads (never writes):
  <work>/drafts/X2a_textures.csv, X2a_shapes.csv, X2a_refs.csv, X2a_missing.csv   (visual assets, X2a)
  <work>/drafts/X2b_samples.csv                                                  (audio, X2b)
  <work>/drafts/P03_worldobject_classes.csv, P06_level_building_usage.csv, X4_levels.csv
  <work>/drafts/X2a_asset_catalogue_visual.md   (only the FindUnusedResources cross-check table rows)
  <work>/manifest.csv
  <work>/script_inputs/S_25_template.md          (hand-written prose with <!-- GEN:name --> placeholders)
Writes:
  <design>/25_textures_and_assets.md

Consumers and passes are recomputed from every individual reference in X2a_refs.csv (the X2a per-asset CSV
columns are truncated to four entries).  Flags that the X2a scan could not attach to a reference (the name is
held in a variable at the GetTexture call) are filled from a hand-verified table of call sites (FLAG_BY_SITE /
FLAG_BY_RULE below; each entry was checked in the cited function).
"""
import sys, os, re, csv, collections

WORK, DESIGN = sys.argv[1], sys.argv[2]
D = os.path.join(WORK, 'drafts')

def rd(name):
    with open(os.path.join(D, name), newline='', encoding='utf-8') as fh:
        return list(csv.DictReader(fh))

tex = rd('X2a_textures.csv')
shp = rd('X2a_shapes.csv')
refs_all = rd('X2a_refs.csv')
missing = rd('X2a_missing.csv')
samples = rd('X2b_samples.csv')
classes = {r['class']: r['base'] for r in rd('P03_worldobject_classes.csv')}
levels = rd('X4_levels.csv')
with open(os.path.join(WORK, 'manifest.csv'), newline='', encoding='utf-8') as fh:
    manifest = list(csv.DictReader(fh))

refs = collections.defaultdict(list)
for r in refs_all:
    refs[r['asset']].append(r)

# ------------------------------------------------------------------ helpers
def md(x):
    return str(x).replace('|', '\\|').replace('\n', ' ')

def group_of(p):
    if p.startswith('data/textures/unicode/'):
        return 'textures/unicode'
    if p.startswith('data/shapes/source/'):
        return 'shapes/source'
    return p.split('/')[1]

GROUP_TITLES = collections.OrderedDict([
    ('textures', 'data/textures (excluding font atlases)'),
    ('textures/unicode', 'data/textures/unicode (font atlases)'),
    ('sprites', 'data/sprites'), ('animations', 'data/animations (bitmaps and .anim panel scripts)'),
    ('terrain', 'data/terrain (colour ramps, water and wave tables)'), ('icons', 'data/icons'),
    ('thumbnails', 'data/thumbnails'), ('mwhelp', 'data/mwhelp (help images)'),
    ('sepulveda', 'data/sepulveda'), ('shaders', 'data/shaders (HLSL, Direct3D build only)'),
    ('shapes', 'data/shapes (.shp meshes)'), ('shapes/source', 'data/shapes/source (.max authoring files)'),
    ('sounds', 'data/sounds'), ('mwsounds', 'data/mwsounds')])

BUILD_LABEL = {'FULL': 'B:FULL', 'D3D': 'B:D3D', 'DEBUG': 'B:DEBUG', 'EDITOR': 'B:EDITOR', 'LEGACY': 'B:LEGACY',
               'DEAD': 'B:DEAD'}

def status_key(row):
    st, re_ = row['status'], row['reach']
    if st == 'referenced':
        return re_
    if re_ == 'DEAD':
        return 'unused-dead'
    if 'authoring' in re_ or 'Paint Shop' in re_:
        return 'authoring'
    return 'unused'

STATUS_TEXT = {'FULL': 'referenced [B:FULL]', 'LEGACY': 'referenced [B:LEGACY] only', 'D3D': 'referenced [B:D3D] only',
               'DEBUG': 'referenced [B:DEBUG] only', 'EDITOR': 'referenced [B:EDITOR] only',
               'unused-dead': 'unused (named only in dead/commented code or never-loaded data) [B:DEAD]',
               'unused': 'unused (never named)', 'authoring': 'unused (authoring source, never read)'}

def live_refs(row):
    k = status_key(row)
    rs = refs[row['path']]
    if k in BUILD_LABEL:
        out = [r for r in rs if r['build'] == k and r['strength'] == 'C']
        return out or [r for r in rs if r['build'] == k]
    if k == 'unused-dead':
        return [r for r in rs if r['build'] == 'DEAD' or r['kind'] == 'commented-out']
    return []

def split_site(site):
    m = re.match(r'(.*?):(.*):L(\d+)$', site)
    if not m:
        return site, '', ''
    return m.group(1), m.group(2), m.group(3)

# ---- consumer / pass mapping (same table as X2a_catalogue.py:consumer_of, re-implemented) ----
live_levels = set()
for lv in levels:
    mt = [t for t in lv.get('menu_types', '').replace(';', ' ').split() if t]
    if lv['file'].startswith('data/levels/mp_') and mt and set(mt) != {'TankBattle'}:
        live_levels.add(os.path.basename(lv['file']).lower())
mp_place = collections.defaultdict(int)
for r in rd('P06_level_building_usage.csv'):
    if r['level'].lower() in live_levels:
        mp_place[r['type'].lower()] += 1

def root_kind(cls):
    seen = 0
    while seen < 25:
        if cls in ('Building', 'Entity', 'WorldObject'):
            return cls
        b = classes.get(cls)
        if not b:
            return ''
        b = b.split(',')[0].replace('public', '').strip()
        if b == 'Unit':
            return 'Entity'
        cls = b; seen += 1
    return ''

SYS = [
    ('code/clouds.cpp', 'Clouds (sky; also menu backdrop)', '3a-3d'),
    ('code/landscape_renderer.cpp', 'LandscapeRenderer', '4a/4b'),
    ('code/landscape.cpp', 'Landscape', '4a/4b'),
    ('code/water_reflection.cpp', 'WaterReflectionEffect [D3D]', '5b'),
    ('code/water.cpp', 'Water', '5a/5b'),
    ('code/location.cpp', 'Location (weapons, lasers)', '10b'),
    ('code/particle_system.cpp', 'ParticleSystem', '15'),
    ('code/explosion.cpp', 'ExplosionManager', '14'),
    ('code/deform.cpp', 'DeformEffect [D3D]', 'after 15'),
    ('code/renderer.cpp', 'Renderer (logo / transitions)', '31'),
    ('code/markersystem.cpp', 'MarkerSystem', '16'),
    ('code/multiwiniahelp.cpp', 'MultiwiniaHelp (help/tutorial overlay)', '17'),
    ('code/animatedpanel', 'AnimatedPanel (mode help animation)', '17'),
    ('code/shaman_interface.cpp', 'ShamanInterface', '18'),
    ('code/interface/', 'Eclipse windows (menus/dialogs)', '19'),
    ('code/UI/', 'GameMenuWindow front-end', '19'),
    ('code/game_menu.cpp', 'GameMenu front-end', '19'),
    ('contrib/eclipse/', 'Eclipse window framework', '19'),
    ('code/gamecursor.cpp', 'GameCursor / MouseCursor', '20'),
    ('code/taskmanager_interface', 'Task-manager HUD', '23'),
    ('code/control_help.cpp', 'ControlHelpSystem', '25'),
    ('code/lib/text_renderer.cpp', 'TextRenderer (fonts)', 'all text'),
    ('code/multiwinia.cpp', 'Multiwinia match HUD / rules', '28'),
    ('code/rocket_status_panel.cpp', 'RocketStatusPanel (Rocket Riot HUD)', '28'),
    ('code/startsequence.cpp', 'StartSequence', '30'),
    ('code/demoendsequence.cpp', 'DemoEndSequence', '30'),
    ('code/loading_screen.cpp', 'LoadingScreen', 'loading loop'),
    ('code/team.cpp', 'Team (Virii strip build / render)', '7c'),
    ('code/global_world.cpp', 'GlobalWorld (Darwinia world map)', 'world-map branch'),
    ('code/global_internet.cpp', 'GlobalInternet (Darwinia world map)', 'world-map branch'),
    ('code/level_file.cpp', 'LevelFile (map defaults)', '4a/5a'),
    ('code/mapfile.cpp', 'MapFile (.mwm embeds terrain bitmaps)', '-'),
    ('code/location_editor.cpp', 'LocationEditor [EDITOR]', 'editor'),
    ('code/lib/FixedPipeline.cpp', 'FixedPipeline [D3D]', 'every D3D draw'),
    ('code/lib/shader.cpp', 'Shader include handler [D3D]', '-'),
    ('code/lib/3d_sprite.cpp', 'Render3DSprite', 'caller'),
    ('code/sepulveda.cpp', 'Sepulveda (Darwinia help)', '-'),
    ('code/loaders/', 'Loader mini-programs', '-'),
    ('code/tutorial', 'Darwinia tutorial', '-'),
    ('code/lib/resource.cpp', 'Resource (context regeneration)', '-'),
    ('code/app.cpp', 'App', '-'),
    ('code/main.cpp', 'main loop', '-'),
    ('code/script.cpp', 'Script (cut-scenes)', '-'),
    ('code/sound/', 'Sound system', '-'),
    ('code/network/', 'Network', '-'),
    ('code/unicode/', 'Unicode helpers', '-'),
]

def consumer_of(r, asset_dir):
    f, sym, _ = split_site(r['site'])
    if f.startswith('data/'):
        key = sym.lower()
        if f.startswith('data/levels/'):
            if key == 'landcolourfile':
                return 'LandscapeRenderer (terrain ramp)', '4a'
            if key in ('wavescolourfile', 'watercolourfile'):
                return 'Water (%s)' % sym, '5a/5b'
            return 'StaticShape (level %s)' % sym, '6'
        if f.startswith('data/animations/'):
            return 'AnimatedPanel keyframe (%s)' % os.path.basename(f), '17'
        if f.startswith('data/input/'):
            return 'ControlBindings icon (never drawn)', '-'
        return 'data ' + f, '-'
    if f.startswith('code/worldobject/'):
        cls = sym.split('::')[0] if '::' in sym else os.path.basename(f).rsplit('.', 1)[0]
        rk = root_kind(cls)
        meth = sym.split('::')[-1]
        if asset_dir == 'shapes':
            ps = {'Building': '6', 'Entity': '7a/7b', 'WorldObject': '10a'}.get(rk, '6/7')
        elif 'Alphas' in meth or 'Light' in meth or 'Port' in meth:
            ps = '8'
        elif cls == 'Darwinian':
            ps = '7d'
        else:
            ps = {'Entity': '7a/7b', 'Building': '6/8', 'WorldObject': '10a'}.get(rk, '6-10')
        place = ''
        if rk == 'Building':
            place = ', MP-map placements %d' % mp_place.get(cls.lower(), 0)
        return '%s (%s%s)' % (cls, rk or 'worldobject', place), ps
    for pre, name, ps in SYS:
        if f.startswith(pre):
            return name, ps
    return os.path.basename(f), '-'

# ---- flags ----
FLAG_BY_SITE = {   # name passed in a variable at the GetTexture call; checked in each function [C]
    'GameMenuWindow::SetupBuyMePage': 'mip=false mask=true',      # ImageButton -> code/UI/ImageButton.cpp:ImageButton::Render
    'GameTypeImageButton::Render': 'mip=false mask=false',
    'MapImageButton::Render': 'mip=false mask=false',
    'Water::RenderFlatWater': 'mip=true mask=true',
    'LandscapeRenderer::LandscapeRenderer': 'CPU',
    'Water::Water': 'CPU',
    'DeformParams': 'mip=false mask=false',                       # code/deform.cpp:DeformEffect::Stop L309
}
FLAG_BY_RULE = {
    'rule:map-thumbnail': 'mip=false mask=false', 'rule:map-thumbnail-tankbattle': 'mip=false mask=false',
    'rule:mode-image': 'mip=false mask=false',
    'rule:help-general': 'mip=false mask=true', 'rule:help-gametype': 'mip=false mask=true',
    'rule:anim-image': 'mip=true mask=true', 'rule:icon-stem': 'mip=true mask=true',
    'rule:darwinian-ouch': 'mip=true mask=true', 'rule:crate-icon': 'mip=true mask=true',
    'rule:research-icon': 'mip=true mask=false (pre-load); mip=true mask=true (draw)',
    'rule:research-gesture': 'mip=true mask=false (pre-load only)',
    'shadow-derived': 'mip=true mask=true',
}

def ref_flags(r, path):
    if r['flags']:
        return r['flags']
    f, sym, _ = split_site(r['site'])
    k = r['kind']
    if k in ('data', 'rule:terrain-default'):
        if '/terrain/' in path:
            return 'mip=true mask=true' if '/water_' in path else 'CPU'
        if f.startswith('data/animations/'):
            return 'mip=true mask=true'
        if f.startswith('data/input/'):
            return 'never drawn'
        return ''
    if k in FLAG_BY_RULE:
        return FLAG_BY_RULE[k]
    if k in ('literal', 'stem') and r['strength'] == 'C':
        if sym in FLAG_BY_SITE:
            return FLAG_BY_SITE[sym]
        if r['how'] == 'GetBinaryReader':
            return 'raw read → registered bitmap (draw: mip=true mask=true)'
        return 'mip=true mask=true'
    return ''

def flag_cells(row, lrefs):
    fl = []
    for r in lrefs:
        v = ref_flags(r, row['path'])
        if v and v not in fl:
            fl.append(v)
    if not fl:
        return '-', '-'
    if all(v == 'CPU' for v in fl):
        return 'n/a (CPU look-up via GetBitmap)', ''
    if all(v == 'never drawn' for v in fl):
        return 'n/a (never drawn)', ''
    sigs = []
    for v in fl:
        if v in ('CPU', 'never drawn'):
            continue
        for part in v.split(';'):
            mm = re.search(r'mip=(true|false)', part)
            if not mm:
                continue
            alpha = 'alpha=green' in part
            mk = re.search(r'mask=(true|false)', part)
            mask = (mk.group(1) == 'true') if (mk and not alpha) else False
            sg = (mm.group(1), mask, alpha)
            if sg not in sigs:
                sigs.append(sg)
    def sig_txt(sg):
        return ('mip' if sg[0] == 'true' else 'no mip') + (', green→A' if sg[2] else (', mask' if sg[1] else ', no mask'))
    if not sigs:
        return '; '.join(fl), ''
    if len(sigs) == 1:
        mip = 'yes' if sigs[0][0] == 'true' else 'no'
        proc = sig_txt(sigs[0]).split(', ', 1)[1]
    else:
        mip = 'CONFLICT: ' + ' vs '.join(sig_txt(sg) for sg in sigs) + ' — first requester wins (§2.6)'
        proc = ''
    if 'CPU' in fl:
        proc = (proc + ', ' if proc else '') + 'also read on CPU'
    return mip, proc

GL_BY_RULE = {   # GL state at rule-built draw sites whose literal the X2a scan could not see (checked in code)
    'rule:map-thumbnail': 'MIN_FILTER=LINEAR MAG_FILTER=LINEAR',            # code/UI/MapImageButton.h:MapImageButton::Render
    'rule:map-thumbnail-tankbattle': 'MIN_FILTER=LINEAR MAG_FILTER=LINEAR',
    'rule:mode-image': 'MIN_FILTER=LINEAR MAG_FILTER=LINEAR Blend(SRC_ALPHA,ONE)',  # code/UI/GameTypeImageButton.h:GameTypeImageButton::Render
    'rule:help-general': 'MIN_FILTER=LINEAR MAG_FILTER=LINEAR Blend(SRC_ALPHA,ONE_MINUS_SRC_ALPHA)',  # code/UI/ImageButton.cpp:ImageButton::Render
    'rule:help-gametype': 'MIN_FILTER=LINEAR MAG_FILTER=LINEAR Blend(SRC_ALPHA,ONE_MINUS_SRC_ALPHA)',
}
WATER_GL = 'WRAP_S=REPEAT WRAP_T=REPEAT MIN_FILTER=LINEAR_MIPMAP_LINEAR MAG_FILTER=LINEAR TexEnv(MODULATE) unit1=water_lightmap'
def ref_gl(r, path):
    if r['gl_state']:
        return r['gl_state']
    if '/terrain/water_' in path:
        return WATER_GL                                                          # code/water.cpp:Water::RenderFlatWater
    if r['kind'] in GL_BY_RULE:
        return GL_BY_RULE[r['kind']]
    f, sym, _ = split_site(r['site'])
    if sym in ('GameMenuWindow::SetupBuyMePage',):
        return GL_BY_RULE['rule:help-general']
    if sym in ('MapImageButton::Render', 'GameTypeImageButton::Render'):
        return GL_BY_RULE['rule:map-thumbnail']
    return ''

ABBR = {'LINEAR_MIPMAP_LINEAR': 'LML', 'LINEAR': 'L', 'NEAREST': 'N', 'REPEAT': 'REPEAT', 'CLAMP': 'CLAMP'}
def gl_cells(lrefs, path=''):
    wraps, mins, mags, envs, blends = [], [], [], [], []
    for r in lrefs:
        g = ref_gl(r, path)
        for m in re.finditer(r'WRAP_(?:S|T|S/T)=(\w+)', g):
            if m.group(1) not in wraps: wraps.append(m.group(1))
        for m in re.finditer(r'MIN(?:_FILTER)?=(\w+)', g):
            v = ABBR.get(m.group(1), m.group(1))
            if v not in mins: mins.append(v)
        for m in re.finditer(r'MAG(?:_FILTER)?=(\w+)', g):
            v = ABBR.get(m.group(1), m.group(1))
            if v not in mags: mags.append(v)
        for m in re.finditer(r'TexEnv\((\w+)\)', g):
            if m.group(1) not in envs: envs.append(m.group(1))
        for m in re.finditer(r'Blend\((\w+),(\w+)\)', g):
            b = {'ONE': 'add', 'ONE_MINUS_SRC_ALPHA': 'alpha', 'ONE_MINUS_SRC_COLOR': 'darken'}.get(m.group(2), m.group(2))
            if b not in blends: blends.append(b)
    parts = []
    if wraps:
        parts.append('wrap ' + '/'.join(wraps))
    if mins:
        parts.append('MIN ' + '/'.join(mins))
    if mags:
        parts.append('MAG ' + '/'.join(mags))
    if envs:
        parts.append('env ' + '/'.join(envs))
    if any('unit1=water_lightmap' in ref_gl(r, path) for r in lrefs):
        parts.append('unit 1 = `water_lightmap` (COMBINE MODULATE)')
    return ('; '.join(parts) if parts else 'upload default'), blends

def sites_cell(lrefs, limit=None):
    code = collections.OrderedDict(); lv = []; other = collections.OrderedDict()
    for r in lrefs:
        f, sym, ln = split_site(r['site'])
        if f.startswith('data/levels/'):
            stem = os.path.basename(f).rsplit('.', 1)[0]
            tag = stem if not f.startswith('data/levels/old/') else 'old/' + stem
            if tag not in lv: lv.append(tag)
        elif f.startswith('data/'):
            key = '`%s:%s`' % (f, sym)
            other.setdefault(key, []).append(ln)
        else:
            key = '`%s:%s`' % (f, sym or '?')
            code.setdefault(key, [])
            if ln and ('L' + ln) not in code[key]:
                code[key].append('L' + ln)
    out = ['%s %s' % (k, ','.join(v)) if v else k for k, v in code.items()]
    out += ['%s ×%d' % (k, len(v)) for k, v in other.items()]
    if lv:
        out.append('levels (%d): %s' % (len(lv), ', '.join(lv)))
    return '; '.join(out) or '-'

def consumers_cell(row, lrefs):
    pairs = []
    for r in lrefs:
        c, p = consumer_of(r, row['dir'] if 'dir' in row else group_of(row['path']))
        if (c, p) not in pairs:
            pairs.append((c, p))
    cons = []; passes = []
    for c, p in pairs:
        if c not in cons: cons.append(c)
        if p not in passes and p != '-': passes.append(p)
    return '; '.join(cons) or '-', ', '.join(passes) or '-', pairs

def packed(a):
    a = a.replace('language.dat[english+french+german+italian+spanish+portuguese+russian+japanese+korean]',
                  'language.dat (every language list)')
    a = a.replace('init.dat + main.dat', 'init.dat+main.dat')
    return a

def pot_of(row):
    m = re.search(r'rounds up to (\d+)x(\d+)', row['pow2'])
    return (int(m.group(1)), int(m.group(2))) if m else None

def fmt_cell(row):
    if row['bpp']:
        b = int(row['bpp'])
        s = {4: 'P4 (16-colour palette)', 8: 'P8 (256-colour palette)', 24: 'BGR24', 32: 'BGRA32'}[b]
        if row['palette_colours_used'] and b <= 8:
            s += ', %s used' % row['palette_colours_used']
        s += ' → RGBA8'
        if row['loader_issues']:
            s += ' ⚠ ' + row['loader_issues']
        return s
    return row['format']

def alpha_cell(row, procs, blends):
    parts = []
    if row['bpp'] == '32':
        parts.append(row['alpha_in_file'].replace('A8 non-trivial ', 'A8 ').replace('min 0 max 255, ', ''))
    try:
        pk = int(row['pink_key_pixels'] or 0)
    except ValueError:
        pk = 0
    if pk:
        frac = float(row['pink_fraction'] or 0)
        parts.append('magenta key %s' % (('%.0f%%' % (100 * frac)) if frac >= 0.005 else ('%d px' % pk)))
    if 'green→A' in procs:
        parts.append('green→alpha')
    try:
        bf = float(row['black_fraction'] or 0)
    except ValueError:
        bf = 0
    if bf >= 0.25:
        parts.append('black bg %.0f%%' % (100 * bf))
    if blends:
        parts.append('drawn ' + '/'.join(blends))
    return '; '.join(parts) or 'opaque'

# ------------------------------------------------------------------ generated sections
GEN = {}

# per-file bitmap / anim / shader tables
tex_by_group = collections.OrderedDict((g, []) for g in GROUP_TITLES)
for row in tex:
    tex_by_group[group_of(row['path'])].append(row)
mp_unplaced = []
conflicts = []
for g, rows in tex_by_group.items():
    if not rows or g.startswith('shapes') or g in ('sounds', 'mwsounds'):
        continue
    out = ['| Path | Size (→ POT upload) | Format / channels | Alpha usage | Mip | Wrap / filter at draw [I] | Load site(s) | Consumer | Pass | Packed | Status |',
           '|---|---|---|---|---|---|---|---|---|---|---|']
    for row in rows:
        lr = live_refs(row)
        mip, procs = flag_cells(row, lr)
        glc, blends = gl_cells(lr, row['path'])
        cons, passes, pairs = consumers_cell(row, lr)
        p = row['path']
        if p.endswith('.bmp'):
            size = '%s×%s' % (row['width'], row['height'])
            pt = pot_of(row)
            if pt:
                size += ' → %d×%d' % pt
            fmtc = fmt_cell(row)
            alc = alpha_cell(row, procs, blends)
            if procs:
                mip = mip + ' · ' + procs
            if mip.startswith('n/a (CPU'):
                glc = 'n/a'
        elif p.endswith('.anim'):
            m = re.match(r'AnimatedPanel text \((\d+) x (\d+), t ([\d.]+)\.\.([\d.]+) s\)', row['format'])
            size = '%s×%s panel' % (m.group(1), m.group(2)) if m else '-'
            fmtc = 'AnimatedPanel text, %s–%s s' % (m.group(3), m.group(4)) if m else row['format']
            alc = row['extra'] or '-'
            mip, glc = '-', '-'
        else:
            size = '%s B' % row['bytes']
            fmtc = row['format']
            alc, mip, glc = '-', '-', '-'
            if p.endswith(('.vs', '.ps', '.ps3', '.h')):
                glc = 'D3D sampler state set by the effect'
        sk = status_key(row)
        if sk == 'FULL' and pairs and all('MP-map placements 0' in c for c, _ in pairs):
            mp_unplaced.append((p, cons))
        if mip.startswith('CONFLICT') and p.endswith('.bmp'):
            sig = mip.split(': ', 1)[1].split(' — ')[0]
            pk = int(row['pink_key_pixels'] or 0)
            if 'green→A' in sig:
                eff = 'visible: alpha mask (white, A = green) vs original RGB art'
            elif 'no mip' in sig:
                eff = 'mip chain present or absent; a no-mip draw that leaves MIN = LINEAR_MIPMAP_LINEAR samples an incomplete texture if the no-mip request came first'
            elif pk:
                eff = 'visible: magenta pixels transparent or opaque magenta'
            else:
                eff = 'none visible (file has no magenta pixels)'
            conflicts.append('| `%s` | %s | %d | %s | %s |' % (p, sig, pk, STATUS_TEXT[sk], eff))
        out.append('| `%s` | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |' % (
            p, size, md(fmtc), md(alc), md(mip), md(glc), md(sites_cell(lr)), md(cons), md(passes),
            md(packed(row['archive'])), STATUS_TEXT[sk]))
    GEN['tex:' + g] = '\n'.join(out)

GEN['conflict_table'] = '\n'.join(['| Texture | Distinct processing requested by its reachable call sites | Magenta pixels in file | Status | Effect of the first-requester rule |', '|---|---|---|---|---|'] + conflicts)
GEN['conflict_count'] = str(len(conflicts))

# .anim detail
out = ['| Panel script | Panel size | Time span | Objects | Keyframes | Images referenced (all under `data/animations/`) | Loaded by (game type) |', '|---|---|---|---|---|---|---|']
for row in tex_by_group['animations']:
    if not row['path'].endswith('.anim'):
        continue
    m = re.match(r'AnimatedPanel text \((\d+) x (\d+), t ([\d.]+)\.\.([\d.]+) s\)', row['format'])
    e = re.match(r'(\d+) objects, (\d+) keyframes, images: (.*)', row['extra'])
    out.append('| `%s` | %s×%s | %s–%s s | %s | %s | %s | `code/multiwiniahelp.cpp:MultiwiniaHelp::InitialiseAnimatedPanel` (%s) |' % (
        row['path'], m.group(1), m.group(2), m.group(3), m.group(4), e.group(1), e.group(2), e.group(3),
        os.path.basename(row['path'])[:-5]))
GEN['anim_detail'] = '\n'.join(out)

# shapes
shp_files = [r for r in shp if r['kind'] == 'shp']
max_files = [r for r in shp if r['kind'] == 'max']
out = ['| Path | Frags | Depth | Pos | Verts | Tris | Geom | Markers | Looked-up names | Load site(s) | Consuming class / system | Pass | Packed | Status |',
       '|---|---|---|---|---|---|---|---|---|---|---|---|---|---|']
for row in shp_files:
    lr = live_refs(row)
    cons, passes, pairs = consumers_cell(dict(row, dir='shapes'), lr)
    sk = status_key(row)
    if sk == 'FULL' and pairs and all('MP-map placements 0' in c for c, _ in pairs):
        mp_unplaced.append((row['path'], cons))
    warn = ''
    if row['positions'] and row['vertices'] and int(row['positions']) > int(row['vertices']):
        warn = ' ⚠ pos>verts'
    out.append('| `%s` | %s | %s | %s | %s%s | %s | %s | %s | %s | %s | %s | %s | %s | %s |' % (
        row['path'], row['fragments'], row['max_depth'], row['positions'], row['vertices'], warn, row['triangles'],
        row['geometry'], row['markers'], md(row['code_looked_up_names'] or '-'), md(sites_cell(lr)), md(cons),
        md(passes), md(packed(row['archive'])), STATUS_TEXT[sk]))
GEN['shape_table'] = '\n'.join(out)

def tree_text(row):
    pairs = [x.strip() for x in row['hierarchy'].split(';') if x.strip()]
    kids = collections.OrderedDict()
    order = []
    for pr in pairs:
        c, _, par = pr.partition('<')
        kids.setdefault(par.lower(), []).append(c)
        order.append(c)
    def rec(n, depth=0):
        ch = kids.get(n.lower(), [])
        if depth > 10 or not ch:
            return n
        return n + '{' + ', '.join(rec(c, depth + 1) for c in ch) + '}'
    if not pairs:
        return 'SceneRoot (no fragments)'
    return rec('SceneRoot').replace('SceneRoot{', 'SceneRoot{', 1) if 'sceneroot' in kids else '; '.join(pairs)

out = []
fam = collections.Counter(); fam_files = collections.defaultdict(set)
for row in shp_files:
    mk = [m.strip() for m in row['marker_names'].split(';') if m.strip()]
    byp = collections.OrderedDict()
    for m in mk:
        n, _, par = m.partition('@')
        byp.setdefault(par, []).append(n)
        f_ = re.sub(r'\d+$', '', n)
        fam[f_] += 1; fam_files[f_].add(os.path.basename(row['path']))
    mtxt = '; '.join('@%s: %s' % (par, ', '.join(ns)) for par, ns in byp.items()) or 'none'
    prob = (' ⚠ %s.' % row['parse_problems']) if row['parse_problems'] else ''
    out.append('- `%s` — %s fragment%s, depth %s: %s. Markers (%s): %s.%s' % (
        row['path'], row['fragments'], '' if row['fragments'] == '1' else 's', row['max_depth'], tree_text(row),
        row['markers'], mtxt, prob))
GEN['shape_hier'] = '\n'.join(out)
out = ['| Marker family (trailing digits removed) | Markers | Shapes using it |', '|---|---|---|']
for f_, n in sorted(fam.items(), key=lambda kv: (-kv[1], kv[0])):
    out.append('| `%s` | %d | %s |' % (f_, n, ', '.join(sorted(fam_files[f_]))))
GEN['marker_vocab'] = '\n'.join(out)
out = ['| Path | Bytes | Exported .shp (same stem, case-insensitive) |', '|---|---|---|']
for row in max_files:
    out.append('| `%s` | %s | %s |' % (row['path'], row['bytes'], ('`%s`' % row['source_pair']) if row['source_pair'] else 'none (no .shp with this stem)'))
GEN['max_table'] = '\n'.join(out)
geo = collections.Counter(r['geometry'] for r in shp_files)
st_s = collections.Counter(status_key(r) for r in shp_files)
tri = [int(r['triangles'] or 0) for r in shp_files]
big = sorted(shp_files, key=lambda r: -int(r['triangles'] or 0))[:5]
mostf = sorted(shp_files, key=lambda r: -int(r['fragments'] or 0))[:5]
mostm = sorted(shp_files, key=lambda r: -int(r['markers'] or 0))[:5]
GEN['shape_stats'] = '\n'.join([
    '| Statistic | Value |', '|---|---|',
    '| `.shp` files | %d |' % len(shp_files),
    '| `.max` authoring sources | %d (%d pair with a `.shp` by stem) |' % (len(max_files), sum(1 for r in max_files if r['source_pair'])),
    '| Geometry encoding | %s |' % ', '.join('%s %d' % kv for kv in sorted(geo.items())),
    '| Fragment blocks (engine instances, excl. SceneRoot) | %d |' % sum(int(r['fragments'] or 0) for r in shp_files),
    '| Max hierarchy depth | %d |' % max(int(r['max_depth'] or 0) for r in shp_files),
    '| Markers | %d |' % sum(int(r['markers'] or 0) for r in shp_files),
    '| Triangles (after strip conversion) total / max / mean | %d / %d / %.1f |' % (sum(tri), max(tri), sum(tri) / len(tri)),
    '| Largest by triangles | %s |' % ', '.join('`%s` (%s)' % (os.path.basename(r['path']), r['triangles']) for r in big),
    '| Most fragments | %s |' % ', '.join('`%s` (%s)' % (os.path.basename(r['path']), r['fragments']) for r in mostf),
    '| Most markers | %s |' % ', '.join('`%s` (%s)' % (os.path.basename(r['path']), r['markers']) for r in mostm),
    '| Status | %s |' % ', '.join('%s %d' % (STATUS_TEXT[k], v) for k, v in sorted(st_s.items())),
])

# audio
def audio_status(r):
    if r['status'] == 'unused':
        return 'unused'
    return 'live' if r['event_liveness'] == 'live' else 'dead'
for folder in ('sounds', 'mwsounds'):
    rows = [r for r in samples if r['folder'] == folder]
    out = ['| Path | Ch | Rate Hz | Duration s | KiB | kbps | Encoder | Archive | Status | Referenced by (events; sample groups) |',
           '|---|---|---|---|---|---|---|---|---|---|']
    for r in rows:
        ev = r['referenced_by'].replace(';', '; ')
        if r['via_groups']:
            ev += ' (group%s `%s`)' % ('s' if ';' in r['via_groups'] else '', r['via_groups'].replace(';', '`, `'))
        if r['orphan_groups']:
            ev = (ev + '; ' if ev else '') + 'orphan group `%s` (bound by no event)' % r['orphan_groups']
        vend = r['vendor'].replace('Xiph.Org libVorbis I ', 'libVorbis ').replace("AO; aoTuV b3 [20041120] (based on Xiph.Org's libVorbis)", 'aoTuV b3')
        out.append('| `%s` | %s | %s | %s | %d | %s | %s | %s | %s | %s |' % (
            r['path'], r['channels'], r['rate'], r['duration_s'], round(int(r['bytes']) / 1024), r['nominal_kbps'],
            vend, r['archive'], audio_status(r), md(ev or '-')))
    GEN['audio:' + folder] = '\n'.join(out)
ast = collections.Counter((r['folder'], audio_status(r)) for r in samples)
arch = collections.Counter((r['folder'], r['archive'].split('(')[0]) for r in samples)
dur = collections.defaultdict(float); byt = collections.defaultdict(int)
for r in samples:
    dur[r['folder']] += float(r['duration_s']); byt[r['folder']] += int(r['bytes'])
out = ['| Folder | Files | Total duration s | Bytes | Channels | Rates | Live | Dead (bound to never-fired events) | Unused (no binding) | menusounds.dat | sounds.dat | sounds2.dat | Not shipped |',
       '|---|---|---|---|---|---|---|---|---|---|---|---|---|']
for f in ('sounds', 'mwsounds'):
    rows = [r for r in samples if r['folder'] == f]
    rates = collections.Counter(r['rate'] for r in rows)
    out.append('| `data/%s` | %d | %.1f | %d | mono ×%d | %s | %d | %d | %d | %d | %d | %d | %d |' % (
        f, len(rows), dur[f], byt[f], len(rows), ', '.join('%s Hz ×%d' % kv for kv in sorted(rates.items())),
        ast[(f, 'live')], ast[(f, 'dead')], ast[(f, 'unused')], arch[(f, 'menusounds.dat')], arch[(f, 'sounds.dat')],
        arch[(f, 'sounds2.dat')], arch[(f, 'NOT-SHIPPED')]))
GEN['audio_stats'] = '\n'.join(out)

# directory statistics
out = ['| Directory | Files | Bytes | Bit depths | POT / NPOT | 32-bit with alpha | Magenta-key files | FULL | LEGACY only | D3D only | DEBUG only | EDITOR only | Unused (dead refs) | Unused (never named) | Authoring source | Not packed |',
       '|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|']
tot = collections.Counter()
allrows = tex + shp
by_g = collections.OrderedDict((g, []) for g in GROUP_TITLES)
for r in allrows:
    by_g[group_of(r['path'])].append(r)
for g, rows in by_g.items():
    if not rows:
        continue
    bpp = collections.Counter(r['bpp'] for r in rows if r.get('bpp'))
    p2 = sum(1 for r in rows if r.get('pow2') == 'yes'); np_ = sum(1 for r in rows if (r.get('pow2') or '').startswith('no'))
    al = sum(1 for r in rows if 'non-trivial' in (r.get('alpha_in_file') or ''))
    pk = sum(1 for r in rows if (r.get('pink_key_pixels') or '0') not in ('', '0'))
    sc = collections.Counter(status_key(r) for r in rows)
    notp = sum(1 for r in rows if r['archive'].startswith('not packed'))
    b = sum(int(r['bytes']) for r in rows)
    for k, v in sc.items(): tot[k] += v
    tot['files'] += len(rows); tot['bytes'] += b; tot['notp'] += notp
    out.append('| `data/%s` | %d | %d | %s | %s | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d |' % (
        g, len(rows), b, ', '.join('%s-bit ×%d' % (k, v) for k, v in sorted(bpp.items(), key=lambda kv: int(kv[0]))) or 'n/a',
        ('%d / %d' % (p2, np_)) if bpp else 'n/a', al, pk, sc['FULL'], sc['LEGACY'], sc['D3D'], sc['DEBUG'], sc['EDITOR'],
        sc['unused-dead'], sc['unused'], sc['authoring'], notp))
out.append('| **visual total** | %d | %d | | | | | %d | %d | %d | %d | %d | %d | %d | %d | %d |' % (
    tot['files'], tot['bytes'], tot['FULL'], tot['LEGACY'], tot['D3D'], tot['DEBUG'], tot['EDITOR'], tot['unused-dead'],
    tot['unused'], tot['authoring'], tot['notp']))
GEN['dir_stats'] = '\n'.join(out)

# NPOT table
npot = collections.defaultdict(list)
for r in tex:
    pt = pot_of(r)
    if pt:
        npot[('%s×%s' % (r['width'], r['height']), '%d×%d' % pt)].append(r['path'])
out = ['| Stored size | Uploaded size when POT is required | Texel ratio | Files | Directories |', '|---|---|---|---|---|']
for (a, b), ps in sorted(npot.items(), key=lambda kv: -len(kv[1])):
    w, h = map(int, a.split('×')); W, H = map(int, b.split('×'))
    dirs = collections.Counter(group_of(p) for p in ps)
    out.append('| %s | %s | ×%.2f | %d | %s |' % (a, b, (W * H) / (w * h), len(ps), ', '.join('`data/%s` %d' % kv for kv in sorted(dirs.items()))))
GEN['npot_table'] = '\n'.join(out)

# font inventory
def rng_class(lo):
    v = int(lo, 16)
    if v < 0x100: return 'Latin-1 U+0020–00FF'
    if 0x400 <= v < 0x500: return 'Cyrillic U+0410–044F (2 files per font)'
    if 0x20a0 <= v < 0x20b0: return 'Currency U+20A0–20AF'
    if 0x3000 <= v < 0x3100: return 'CJK punctuation + kana U+3000–30FF'
    if 0x4e00 <= v < 0xa000: return 'CJK ideographs U+4E00–9EFF (one file per 256 code points)'
    if 0xac00 <= v < 0xd800: return 'Hangul U+AC00–D7FF (one file per 256 code points)'
    return 'other'
inv = collections.OrderedDict()
for r in tex_by_group['textures/unicode']:
    b = os.path.basename(r['path'])
    font, lo, hi = b.split('.')[:3]
    k = (font, rng_class(lo))
    inv.setdefault(k, []).append(r)
out = ['| Font | Range | Files | Size × bit depth | Packed (retail `language.dat`) |', '|---|---|---|---|---|']
for (font, rc), rows in inv.items():
    sz = collections.Counter('%s×%s %s-bit' % (r['width'], r['height'], r['bpp']) for r in rows)
    pk = collections.Counter(packed(r['archive']) for r in rows)
    out.append('| `%s` | %s | %d | %s | %s |' % (font, rc, len(rows), ', '.join('%s ×%d' % kv for kv in sz.items()),
                                              '; '.join('%s ×%d' % kv for kv in pk.items())))
GEN['font_inventory'] = '\n'.join(out)

# missing names
COMPASS_NOTE = ('compiled in FULL but **unreachable** [C]: the only callers pass `lineNormal` from `code/markersystem.cpp:MarkerSystem::Render`, '
                'which is (0,−1) on screen or a normalised vector off screen; `code/lib/vector2.cpp:Vector2::Normalise` maps a zero vector to (0,1), '
                'so the `_screenDX == 0 && _screenDY == 0` branch never runs; `RenderCompassEmpty` has no caller')
MISSING_NOTE = {   # reachability of each requesting site (checked in code; see §1.3 overrides)
    'MultiwiniaHelp::LoadIcons': '[B:LEGACY] `HelpDarwinians` tab: its "appropriate" test is hard-coded false, so the icon is never bound',
    'MultiwiniaHelp::RenderCurrentUnitHelp': '[B:LEGACY] no live caller (the calls in `code/taskmanager_interface_multiwinia.cpp` are commented out)',
    'MultiwiniaHelp::RenderBasicControlHelp': '[B:LEGACY] no live caller (the calls in `code/taskmanager_interface_multiwinia.cpp` are commented out)',
    'Renderer::RenderFlatTexture': '[B:LEGACY] its only call is commented out',
    'Renderer::RenderLogo': '[B:DEAD] its only call is inside `#ifdef PROMOTIONAL_BUILD` (never defined)',
    'code/shaman_interface.cpp': '[B:LEGACY] ShamanInterface is shown only from `TaskManagerInterfaceIcons` (single-player)',
    'HelpImageButton::Render': '[B:LEGACY] `PageHelp` has no live entry; guarded by `DoesTextureExist` (pattern `mwhelp/helppage_%d_%d.bmp` matches no file)',
    'NextHelpButton::MouseUp': '[B:LEGACY] `PageHelp` only; guarded by `DoesTextureExist`',
    'PrevHelpButton::MouseUp': '[B:LEGACY] `PageHelp` only; guarded by `DoesTextureExist`',
    'MapDetailsButton::Render': '[B:FULL] guarded by `DoesTextureExist` → silently not drawn (pattern `icons/menu_%dplayer.bmp` matches no file)',
    'TaskManagerInterfaceMultiwinia::TaskManagerInterfaceMultiwinia': '[B:FULL] pre-load loop guarded by `DoesTextureExist` → skipped (only 8 gesture icons exist)',
    'TaskManagerInterfaceIcons::TaskManagerInterfaceIcons': '[B:FULL] pre-load loop guarded by `DoesTextureExist` (and the `GetTexture` is commented out)',
    'TaskManagerInterfaceGestures::TaskManagerInterfaceGestures': '[B:LEGACY] single-player keyboard `ControlMethod` 0 only; guarded by `DoesTextureExist`',
    'TaskManagerInterfaceGestures::RenderResearch': '[B:LEGACY] single-player keyboard `ControlMethod` 0 only',
    'Render': '[B:DEAD] `GestureWindow` body under `GESTURE_EDITOR && USE_SEPULVEDA_HELP_TUTORIAL`; created only by the debug menu',
    'Team::Initialise': 'not missing: run-time bitmap registered with `AddBitmap` (§2.13)',
    'Team::RenderVirii': 'not missing: run-time bitmap (§2.13)',
    'ViriiUnit::Render': 'not missing: run-time bitmap (§2.13)',
}
grp = collections.OrderedDict()
for r in missing:
    nm = r['requested_name']
    key = nm
    grp.setdefault(key, []).append(r)
out = ['| Requested name (resource name, no file under `data/`) | Site(s) | Compiled in | Reachability / effect | How the name is built |', '|---|---|---|---|---|']
for nm, rows in grp.items():
    sites = []; builds = []; notes = []; hows = []
    for r in rows:
        f, sym, ln = split_site(r['site'])
        s = '`%s:%s` L%s' % (f, sym or '?', ln)
        if s not in sites: sites.append(s)
        if r['build'] not in builds: builds.append(r['build'])
        n = ''
        if 'compass_empty' in nm:
            n = COMPASS_NOTE
        elif sym in MISSING_NOTE:
            n = MISSING_NOTE[sym]
        elif f in MISSING_NOTE:
            n = MISSING_NOTE[f]
        elif r['condition']:
            n = 'compiled only when ' + r['condition']
        if n and n not in notes: notes.append(n)
        if r['how'] and r['how'] not in hows: hows.append(r['how'])
    shown = nm.replace(', ', '`, `')
    out.append('| `%s` | %s | %s | %s | %s |' % (shown, '; '.join(sites), ', '.join(builds), md('; '.join(notes) or '-'), md('; '.join(hows) or '-')))
GEN['missing_table'] = '\n'.join(out)

# unused / non-FULL lists
order = [('unused', 'Never named anywhere (unused)'), ('unused-dead', 'Named only in non-compiled or commented-out code, or in data that is never loaded (unused, [B:DEAD])'),
         ('LEGACY', 'Compiled into FULL but unreachable in a Multiwinia match ([B:LEGACY])'), ('D3D', 'Direct3D build only ([B:D3D])'),
         ('DEBUG', 'Debug / cheat builds only ([B:DEBUG])'), ('EDITOR', 'Editor build only ([B:EDITOR])')]
out = []
for k, title in order:
    rows = [r for r in allrows if status_key(r) == k]
    out.append('#### %s — %d files\n' % (title, len(rows)))
    out.append('| Path | Packed | Evidence |\n|---|---|---|')
    for r in rows:
        ev = r['evidence']
        out.append('| `%s` | %s | %s |' % (r['path'], md(packed(r['archive'])), md(ev)))
    out.append('')
auth = [r for r in allrows if status_key(r) == 'authoring']
out.append('#### Authoring sources (never read by the game) — %d files\n' % len(auth))
out.append('The %d `.max` files are listed with their exported `.shp` in §4.7; the remaining authoring file is `%s` (%s).\n' % (
    sum(1 for r in auth if r['path'].endswith('.max')), [r['path'] for r in auth if not r['path'].endswith('.max')][0],
    md([r['evidence'] for r in auth if not r['path'].endswith('.max')][0])))
GEN['unused_lists'] = '\n'.join(out)

out = ['| Path | Only consumers (building classes with no placement in a live Multiwinia map) |', '|---|---|']
for p, c in mp_unplaced:
    out.append('| `%s` | %s |' % (p, md(c)))
GEN['mp_unplaced'] = '\n'.join(out)
GEN['mp_unplaced_count'] = str(len(mp_unplaced))

# FindUnusedResources cross-check rows, copied from the X2a draft
xa = open(os.path.join(D, 'X2a_asset_catalogue_visual.md'), encoding='utf-8').read()
sec = xa.split('25.11 Cross-check with `tools/FindUnusedResources.sh`', 1)[1].split('\n## ', 1)[0]
GEN['fur_table'] = '\n'.join(l for l in sec.splitlines() if l.startswith('|'))

# source map: core implementation files (hand-written roles) + every file that names an asset (from X2a_refs.csv)
CORE = collections.OrderedDict([
    ('code/lib/bitmap.cpp', 'BMP decoder (`LoadBmp` and line readers), pixel operations, BMP/PNG writers, texture upload `ConvertToTexture(Async)`'),
    ('code/lib/bitmap.h', '`BitmapRGBA` class, `RGBAColour` pixel layout, header structs'),
    ('code/lib/resource.cpp', 'resource manager: archives, loose-file lookup, texture cache and flags, bitmap store, shapes, display lists, flush/regenerate, `ListResources`, sound look-up'),
    ('code/lib/resource.h', 'resource manager declarations, flag values `WithMipmaps`/`WithMask`/`WithAlpha`/`WithCompression`, cache container types'),
    ('code/lib/math_utils.cpp', '`RequirePowerOfTwoTextures`, `RequireSquareTextures`, `nearestPowerOfTwo`'),
    ('code/lib/rgb_colour.cpp', '`RGBAColour` equality (colour key compares all four channels)'),
    ('code/lib/rgb_colour.h', '`RGBAColour` constructor (alpha defaults to 255)'),
    ('code/lib/vector2.cpp', '`Vector2::Normalise` (zero → (0,1)) — makes the `compass_empty` branch unreachable'),
    ('code/lib/shape.cpp', '`.shp` parser, hierarchy build, strip conversion, display lists, hit tests'),
    ('code/lib/shape.h', '`Shape`, `ShapeFragment`, `ShapeMarker` declarations'),
    ('code/lib/texture.cpp', 'D3D9 `Texture` wrapper [B:D3D]'),
    ('code/lib/texture.h', '`TextureParams`, `TF_*` flags [B:D3D]'),
    ('code/lib/text_renderer.cpp', 'font atlas discovery (`BuildUnicodeArray`), legacy 2× up-scale (`BuildOpenGlState`), glyph look-up'),
    ('code/lib/text_renderer.h', '`TextRenderer` declarations'),
    ('code/lib/poster_maker.cpp', 'screenshot capture → PNG (only image writer path in FULL)'),
    ('code/lib/universal_include.h', '`TEXTURE_EXTENSION` = "bmp"; build macros gating loaders'),
    ('code/lib/shader.cpp', 'HLSL load + `#include` handler (`globals.h`) [B:D3D]'),
    ('code/lib/FixedPipeline.cpp', 'loads `FixedFunc0–2` shader pairs [B:D3D]'),
    ('code/lib/opengl_directx.cpp', 'D3D9 GL emulation: texture upload with `compressed` flag, device reset → `FlushOpenGlState` [B:D3D]'),
    ('contrib/systemiv/lib/tosser/hash_table.cpp', 'case-folding `HashTable` used for archive entries, bitmap store, shapes, display lists'),
    ('contrib/unrar/unrar.cpp', '`UncompressedArchive`: whole-archive RAR decompression into `MemMappedFile`s'),
    ('code/app.cpp', 'archive mount order (`App::App`, `App::Initialise`, `App::LoadSounds`), `GL_MAX_TEXTURE_SIZE`, font selection (`App::SetLanguage`)'),
    ('code/UI/GameMenuWindow.cpp', '`PreloadTextures` (first requester of `laser.bmp`/`clouds.bmp`), thumbnail and mode-image pre-loads'),
    ('code/taskmanager_interface.cpp', '`CreateTaskManager` decides which task-manager (and so which icon pre-loads) exists'),
    ('code/team.cpp', 'builds `sprites/viriifull.bmp`'),
    ('code/water.cpp', 'water/waves ramps, `water_lightmap` generation'),
    ('code/landscape_renderer.cpp', 'landscape colour ramp (CPU), grid overlay texture'),
    ('code/mapfile.cpp', 'custom `.mwm` maps: embedded terrain bitmaps registered with `AddBitmap`'),
    ('code/worldobject/flag.cpp', 'team-tinted flag/banner textures (`ConvertRedChannel`)'),
    ('code/gamecursor.cpp', 'cursor bitmaps and blurred `shadow_*` copies'),
    ('code/markersystem.cpp', 'marker icons, green→alpha masks, `compass_empty` request'),
    ('code/worldobject/statue.cpp', 'Capture-the-Statue mesh choice via `syncrand() % 8`, optional `carryspecialshape.shp`'),
    ('code/worldobject/entity.cpp', 'team-coloured shape copies (`SetupTeamShape`, `ConvertShapeColoursToTeam`)'),
    ('code/sound/sample_cache.cpp', 'sample name → `sounds/` then `mwsounds/` look-up; mono check; LRU cache'),
    ('code/sound/soundsystem.cpp', '`sounds.txt` blueprint parse (sample names lower-cased, extension stripped)'),
    ('code/sound/sound_stream_decoder.cpp', 'Ogg Vorbis (and unreachable WAV) decoding'),
    ('tools/build_multiwinia_data.bat', 'packs `data/` into the `.dat` archives'),
    ('tools/listfiles.pl', 'exclusion-list expansion used by the data build'),
    ('tools/copyfiles.pl', 'list-driven copy used by the data build'),
    ('tools/init.list', 'contents of `init.dat`'),
    ('tools/language.list', 'all-language `language.dat` contents (font atlases)'),
    ('tools/englishlanguage.list', 'English-only `language.dat`'),
    ('tools/russianlanguage.list', 'Russian `language.dat` (adds Cyrillic `editor`/`square` atlases)'),
    ('tools/multiwinia_exclusions.txt', 'files never packed into `main.dat`/`sounds2.dat`'),
    ('tools/sounds.txt', '`sounds.dat` manifest (also an exclusion list)'),
    ('tools/menu_sounds.txt', '`menusounds.dat` manifest (also an exclusion list)'),
    ('tools/FindUnusedResources.sh', 'Darwinia-era unused-asset finder (§6.4)'),
    ('tools/FindUnusedResourcesExceptions.txt', 'its exception list'),
    ('tools/unused_samples.sh', 'Darwinia-era unused-sample finder (§5)'),
    ('tools/unused_sample_groups.sh', 'Darwinia-era unused-sample-group finder (§5)'),
    ('tools/patch.full.list', 'Darwinia patch list (not used by Multiwinia builds)'),
    ('data/sounds.txt', 'sound blueprints that bind the audio files (§5, [10](10_audio_design.md))'),
    ('docs/OpenGLState.txt', 'stale texture-state notes (discrepancies)'),
    ('docs/RussianFontRequest.txt', 'Cyrillic font history (discrepancies)'),
    ('docs/landscape lightmap.txt', 'stale lightmap notes (discrepancies)'),
])
ref_files = collections.OrderedDict()
for r in refs_all:
    f = split_site(r['site'])[0]
    ref_files.setdefault(f, collections.Counter())[r['build']] += 1
out = ['| File | Role in this document |', '|---|---|']
for f, role in CORE.items():
    extra = ''
    if f in ref_files:
        extra = ' Also names assets: %s.' % ', '.join('%s %d' % kv for kv in sorted(ref_files[f].items()))
    out.append('| `%s` | %s%s |' % (f, role, extra))
for f in sorted(ref_files):
    if f in CORE:
        continue
    c = ref_files[f]
    kind = 'level file naming terrain ramps / static-shape meshes' if f.startswith('data/levels/') else (
        'panel script naming animation bitmaps' if f.startswith('data/animations/') else (
        'input bindings naming icons (never drawn)' if f.startswith('data/input/') else 'requests assets'))
    out.append('| `%s` | %s (references by build: %s) |' % (f, kind, ', '.join('%s %d' % kv for kv in sorted(c.items()))))
GEN['source_map'] = '\n'.join(out)
GEN['source_map_count'] = str(len(out) - 2)

# ------------------------------------------------------------------ assemble
tpl = open(os.path.join(WORK, 'script_inputs', 'S_25_template.md'), encoding='utf-8').read()
def sub(m):
    k = m.group(1)
    if k not in GEN:
        raise SystemExit('unknown placeholder ' + k)
    return GEN[k]
doc = re.sub(r'<!-- GEN:([\w:/]+) -->', sub, tpl)

# contents
heads = []
for line in doc.splitlines():
    m = re.match(r'^(##|###) (.+)$', line)
    if m and not line.startswith('## Contents'):
        t = m.group(2)
        anchor = re.sub(r'[^\w\- ]', '', t.lower()).strip().replace(' ', '-')
        heads.append(('  ' if m.group(1) == '###' else '') + '- [%s](#%s)' % (t, anchor))
doc = doc.replace('<!-- CONTENTS -->', '\n'.join(heads))
open(os.path.join(DESIGN, '25_textures_and_assets.md'), 'w', encoding='utf-8').write(doc)

# ------------------------------------------------------------------ coverage self-check
DIRS = ['data/textures/', 'data/sprites/', 'data/shapes/', 'data/animations/', 'data/terrain/', 'data/shaders/',
        'data/icons/', 'data/thumbnails/', 'data/mwhelp/', 'data/sepulveda/', 'data/sounds/', 'data/mwsounds/']
cited = {s.split(':')[0].lower() for s in re.findall(r'`([^`\n]+)`', doc)}
need = [r['path'] for r in manifest if any(r['path'].lower().startswith(d) for d in DIRS)]
miss = [p for p in need if p.lower() not in cited]
print('document bytes', len(doc.encode('utf-8')), '| catalogue files required', len(need), '| missing', len(miss))
print('MP-unplaced FULL assets', len(mp_unplaced))
for p in miss[:20]:
    print('MISSING', p)
sys.exit(1 if miss else 0)
