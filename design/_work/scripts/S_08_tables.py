"""S08 helper: generate the catalogue tables of 08_levels_and_level_design.md.

Usage:
  python3 -I S_08_tables.py <DRAFTS_DIR> <SRC_ROOT> fill <DOC.md>   # replace every <!-- S08:name --> ... <!-- /S08:name --> region
  python3 -I S_08_tables.py <DRAFTS_DIR> <SRC_ROOT> show <name>     # print one table

Inputs: drafts/X4_levels.csv, X4_level_buildings.csv, X4_thumbnails.csv, X4_mode_maplists.csv,
P06_level_building_usage.csv (cross-check), and the raw level files under <SRC_ROOT>/data/levels
(Assault side table and Blitzkrieg flag graph are recomputed from the raw Buildings blocks).
All paths are passed as arguments; the script never executes data.
"""
import sys, os, csv, re, collections

D, SRC = sys.argv[1], sys.argv[2]
CMD = sys.argv[3]


def load(n):
    return list(csv.DictReader(open(os.path.join(D, n), encoding='utf-8')))


L = load('X4_levels.csv')
B = load('X4_level_buildings.csv')
T = load('X4_thumbnails.csv')
M = load('X4_mode_maplists.csv')
P06 = load('P06_level_building_usage.csv')
TOP = [r for r in L if r['dir'] == 'levels' and r['kind'] == 'mp']
assert len(TOP) == 57, len(TOP)

FULL_MODES = {"Domination", "KingOfTheHill", "CaptureTheStatue", "Assault", "RocketRiot", "Blitzkrieg"}
MODE_SHORT = {"Domination": "DOM", "KingOfTheHill": "KOTH", "CaptureTheStatue": "CTS", "Assault": "ASLT",
              "RocketRiot": "RR", "Blitzkrieg": "BLITZ", "TankBattle": "TANK"}


def md(header, rows):
    out = ["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
    for r in rows:
        out.append("| " + " | ".join(str(x).replace('|', '/') if str(x) != '' else '—' for x in r) + " |")
    return "\n".join(out)


def code(p):
    return f"`{p}`" if p else '—'


def stem(f):
    return os.path.basename(f)[:-4]


def ships(r):
    return " ".join(lab for k, lab in [('ships_retail', 'retail'), ('ships_pcpreview', 'prev'), ('ships_hwcompat', 'hw'),
                                        ('ships_assaultstress', 'stress'), ('ships_beta_a', 'βA'), ('ships_beta_b', 'βB'),
                                        ('ships_beta_c', 'βC')] if r.get(k) == 'Y')


def fullmenu(r):
    modes = set(r['menu_types'].split())
    if not modes:
        return 'never listed'
    if not (modes & FULL_MODES):
        return 'mode not compiled'
    return 'listed' if r['ships_retail'] == 'Y' else 'permitted, data absent'


def klass(r):
    f = os.path.basename(r['file']).lower()
    tags = []
    if 'stress' in f: tags.append('stress test')
    if 'benchmark' in f: tags.append('benchmark')
    if 'sandbox' in f: tags.append('sandbox')
    if 'tankbattle' in f: tags.append('TankBattle [B:DEBUG]')
    if f == 'mp_cts_3p_3.txt': tags.append('cut map')
    if not r['menu_types']: tags.append('hidden (GameTypes none)')
    if r['ships_retail'] != 'Y': tags.append('not in retail data')
    if not tags: tags.append('official retail')
    return "; ".join(tags)


# ---------- building counts ----------
cnt = collections.defaultdict(dict)       # file -> type -> count
byteam = collections.defaultdict(dict)    # file -> type -> by_team string
for r in B:
    cnt[r['file']][r['building_type']] = int(r['count'])
    byteam[r['file']][r['building_type']] = r['by_team']
TYPES = sorted({t for r in TOP for t in cnt[r['file']]})

# cross-check vs P06 (lower-case type names, file basename)
p06 = collections.defaultdict(int)
for r in P06:
    p06[(r['type'].lower(), r['level'])] += int(r['count'])
mism = 0
for r in TOP:
    for t, c in cnt[r['file']].items():
        if p06.get((t.lower(), os.path.basename(r['file'])), 0) != c:
            mism += 1
CROSS = f"cross-check against `_work/drafts/P06_level_building_usage.csv`: {mism} mismatching (type, map) pairs"

# ---------- raw-file parsing (Assault sides, Blitzkrieg flags) ----------
def raw_buildings(fname):
    p = os.path.join(SRC, fname)
    rows = []
    inb = False
    for line in open(p, encoding='latin-1'):
        s = line.split('#')[0].strip()
        if not s:
            continue
        tok = re.split(r'[\s,]+', s)
        w = tok[0].lower()
        if w == 'buildings_startdefinition':
            inb = True; continue
        if w == 'buildings_enddefinition':
            inb = False; continue
        if inb:
            rows.append(tok)
    return rows


def teamlab(t):
    return 'n' if t == '255' else 'p' + t


def notable(r):
    c = cnt[r['file']]
    parts = []
    for t, lab in [('MultiwiniaZone', 'Zone'), ('TrunkPort', 'TrunkPort'), ('EscapeRocket', 'Rocket'), ('FuelStation', 'FuelStn'),
                   ('PulseBomb', 'PulseBomb'), ('ControlStation', 'CtrlStn'), ('GunTurret', 'Turret'), ('RadarDish', 'Radar'),
                   ('Wall', 'Wall'), ('LaserFence', 'Fence'), ('JumpPad', 'JumpPad'), ('RestrictionZone', 'RestrZone'),
                   ('SpawnPopulationLock', 'PopLock'), ('ControlTower', 'CtrlTower'), ('Incubator', 'Incubator'),
                   ('Statue', 'Statue(blocked)'), ('WallBuster', 'WallBuster'), ('EruptionMarker', 'Eruption'),
                   ('SmokeMarker', 'Smoke'), ('Tree', 'Tree'), ('StaticShape', 'Shape')]:
        if c.get(t):
            parts.append(f"{lab} {c[t]}")
    return ", ".join(parts)


def T_identity():
    rows = []
    for r in TOP:
        perm = [lab for k, lab in [('permitted_pc_preview', 'PREV'), ('permitted_pc_hardwarecompat', 'HW'),
                                   ('permitted_assault_stresstest', 'STRESS'), ('permitted_betatest_group_a', 'βA'),
                                   ('permitted_betatest_group_b', 'βB'), ('permitted_betatest_group_c', 'βC')] if r[k] == 'Y']
        tut = r['tutorial'].replace('SetupTutorial2', 'Tutorial 2').replace('SetupTutorial', 'Tutorial 1')
        rows.append([code(r['file']), r['size_bytes'], r['mapid_define'], r['crc'], r['mapid_check'], r['level_enum_id'],
                     ships(r), fullmenu(r), " ".join(perm), 'Y' if r['demo_default_allowed'] else '', tut])
    return md(['File', 'Bytes', 'MAPID macro (`code/mapcrc.h`)', 'CRC = map id', 'CRC check', 'level_enum id',
               'Data shipped in', 'FULL menu (retail data)', 'PERMITTED_MAPS of other targets', 'Demo default', 'Tutorial'], rows)


def T_master():
    rows = []
    for r in TOP:
        cap = r['populationcap'] or '—'
        if r['defenderpopulationcap'] and r['defenderpopulationcap'] != '-1':
            cap += f" / def {r['defenderpopulationcap']}"
        modes = " ".join(MODE_SHORT.get(m, m) for m in r['gametypes_tokens'].split()) or '—'
        rows.append([code(r['file']), r['english_name'], modes, r['numplayers'],
                     f"{r['worldSizeX']}×{r['worldSizeZ']}", r['cellSize'], r['grid_cells'], r['tiles'], r['flatten_areas'],
                     r['spawnpoints_by_team'] or '0', notable(r), cap, r['invalidcrates'], r['instantunits'],
                     code(r['thumbnail']) if r['thumbnail'] else '`data/thumbnails/default.bmp` (fallback)',
                     'Y' if r['ships_retail'] == 'Y' else 'N', klass(r)])
    return md(['File', 'English name (IP)', 'Modes', 'Players', 'World (wu)', 'cellSize', 'Grid', 'Tiles', 'Flatten',
               'SpawnPoints by pos', 'Notable objects (counts)', 'PopCap / DefenderPopCap', 'InvalidCrates',
               'Starting units type@pos×n', 'Thumbnail', 'Retail', 'Class'], rows)


def T_options():
    rows = []
    for r in TOP:
        pc = r['populationcap'] + (f" (lines {r['populationcap_all']})" if r['populationcap_all'] else '')
        rows.append([code(r['file']), r['gametypes_tokens'], r['numplayers'], pc, r['defenderpopulationcap'], r['playtime'],
                     r['difficulty'], r['effects'], r['leveloptions'], r['invalidcrates'], r['attackingteams'],
                     ('CoopMode' if r['coopmode'] else '') + (' ForceCoop' if r['forcecoop'] else ''),
                     (r['coopgroup1'] + ' / ' + r['coopgroup2']) if (r['coopgroup1'] or r['coopgroup2']) else '',
                     r['official'], r['name']])
    return md(['File', 'GameTypes', 'NumPlayers', 'PopulationCap', 'DefenderPopulationCap', 'PlayTime', 'Difficulty', 'Effects',
               'LevelOptions', 'InvalidCrates', 'AttackingTeams', 'Coop', 'CoopGroup1 / 2', 'OfficialLevel', 'Name'], rows)


ABBR = {'AIObjective': 'AIO', 'AIObjectiveMarker': 'AIOM', 'AITarget': 'AIT', 'ControlStation': 'CS', 'ControlTower': 'CT',
        'EruptionMarker': 'ER', 'EscapeRocket': 'Rkt', 'FuelGenerator': 'FG', 'FuelPipe': 'FP', 'FuelStation': 'FS',
        'GunTurret': 'GT', 'Incubator': 'Inc', 'JumpPad': 'JP', 'LaserFence': 'LF', 'MultiwiniaZone': 'Zone',
        'PowerSplitter': 'PSp', 'PulseBomb': 'PB', 'Pylon': 'Py', 'PylonEnd': 'PyE', 'RadarDish': 'RD',
        'RestrictionZone': 'RZ', 'SmokeMarker': 'SM', 'SolarPanel': 'Sol', 'SpawnLink': 'SL', 'SpawnPoint': 'SP',
        'SpawnPointMaster': 'SPM', 'SpawnPopulationLock': 'SPL', 'StaticShape': 'Shp', 'Statue': 'Stat', 'Tree': 'Tree',
        'TrunkPort': 'TP', 'Wall': 'Wall', 'WallBuster': 'WB'}


def T_legend():
    return md(['Abbrev.', 'Building type'], [[ABBR[t], t] for t in TYPES])


def T_objects():
    rows = []
    for r in TOP:
        c = cnt[r['file']]
        rows.append([code(r['file'])] + [c.get(t, '') for t in TYPES] + [r['buildings_total']])
    return md(['File'] + [ABBR[t] for t in TYPES] + ['Total rows'], rows) + "\n\n" + CROSS + " [C]."


def T_typetotals():
    rows = []
    for t in TYPES:
        files = [r for r in TOP if cnt[r['file']].get(t)]
        tot = sum(cnt[r['file']][t] for r in files)
        teams = collections.Counter()
        for r in files:
            for part in byteam[r['file']][t].split(';'):
                if part:
                    k, v = part.split(':')
                    teams['neutral' if k == '255' else 'pos' + k] += int(v)
        rows.append([t, tot, len(files), ", ".join(f"{k} {v}" for k, v in sorted(teams.items()))])
    return md(['Building type', 'Rows in the 57 mp_* files', 'Files', 'Owner field (start position / neutral)'], rows)


def T_landscape():
    rows = []
    for r in L:
        if r['kind'] == 'mission':
            continue
        rows.append([code(r['file']), r['worldSizeX'] + ('' if r['worldSizeX'] == r['worldSizeZ'] else '×' + r['worldSizeZ']),
                     r['cellSize'], r['grid_cells'], r['outsideHeight'], r['maxHeight'],
                     r['landColourFile'].replace('landscape_', '').replace('.bmp', ''),
                     r['wavesColourFile'].replace('waves_', '').replace('.bmp', ''),
                     r['waterColourFile'].replace('water_', '').replace('.bmp', ''),
                     r['tiles'], r['tile_size'], r['tile_posY'], r['tile_fracdim'], r['tile_heightscale'], r['tile_desiredheight'],
                     r['tile_genmethods'], r['tile_guidepower'], r['tile_lowland'], r['flatten_areas'], r['flatten_types'],
                     r['lights'], r['light_colours']])
    return md(['File', 'worldSize', 'cellSize', 'grid', 'outsideHeight', 'maxHeight', 'land ramp', 'waves', 'water', 'tiles',
               'tile size', 'tile posY', 'fracDim', 'heightScale', 'desiredHeight', 'genMethods', 'guidePower', 'lowland',
               'flatten', 'flatten type:n', 'lights', 'light colours (r,g,b)'], rows)


def T_camunits():
    rows = []
    for r in TOP:
        rows.append([code(r['file']), r['camera_mount_names'], r['camera_anims'], r['instantunits'], r['instantunit_total'],
                     r['instantunits_on_routes'], r['routes'], r['primary_objectives'] or '(none → NeverTrue added)',
                     'A' if 'cameraanimations' in r['sections'] else 'B'])
    return md(['File', 'Camera mounts', 'Camera anims', 'InstantUnits type@pos×n', 'Units total', 'Units on routes', 'Routes',
               'PrimaryObjectives', 'Layout'], rows)


def T_modes():
    by = collections.defaultdict(list)
    for r in M:
        if r['scope'] == 'retail' or r['gametype'] == 'TankBattle':
            by[(int(r['gametype_index']), r['gametype'], r['scope'])].append(r)
    out = []
    for (gi, g, sc), lst in sorted(by.items()):
        out.append(f"**{gi} {g}** ({'retail data' if sc == 'retail' else 'source tree only'}, {len(lst)} maps)\n")
        out.append(md(['Menu index', 'File', 'English name (IP)', 'Players', 'Difficulty', 'PlayTime (min)'],
                      [[r['menu_index'], code(r['file']), r['english_name'], r['numplayers'], r['difficulty'], r['playtime']]
                       for r in lst]))
        out.append('')
    return "\n".join(out).rstrip()


def T_assault():
    rows = []
    for r in TOP:
        if 'Assault' not in r['gametypes_tokens'] and 'assault' not in r['file']:
            continue
        bl = raw_buildings(r['file'])
        n = int(r['numplayers'] or 0)
        att = set(r['attackingteams'].split())
        defs = [str(i) for i in range(n) if str(i) not in att]
        pb = [b for b in bl if b[0].lower() == 'pulsebomb']
        pbs = "; ".join(f"id {b[1]} {teamlab(b[4])} t={float(b[8]):.0f}s links {' '.join(b[9:]) or '—'}" for b in pb)
        cs = [b for b in bl if b[0].lower() == 'controlstation']
        csl = " ".join(f"{b[1]}({teamlab(b[4])}→{b[8]})" for b in cs)
        tp = byteam[r['file']].get('TrunkPort', '')
        rows.append([code(r['file']), n, " ".join(sorted(att)) or '—', " ".join(defs) or '—', pbs, csl, tp,
                     r['defenderpopulationcap'], r['playtime'], r['leveloptions'], r['invalidcrates'],
                     r['coopgroup1'] + (' / ' + r['coopgroup2'] if r['coopgroup2'] else ''), r['instantunits']])
    return md(['File', 'Players', 'Attacker pos', 'Defender pos', 'PulseBomb (id, owner, startTime, links)',
               'ControlStations id(owner→controlBuildingId)', 'TrunkPorts pos:n', 'DefenderPopCap', 'PlayTime',
               'LevelOptions', 'InvalidCrates', 'Co-op groups', 'Starting units'], rows)


def T_blitz():
    rows = []
    for r in TOP:
        if r['gametypes_tokens'] != 'Blitzkrieg':
            continue
        bl = raw_buildings(r['file'])
        z = [b for b in bl if b[0].lower() == 'multiwiniazone']
        desc = "; ".join(f"{b[1]}:{teamlab(b[4])} r{float(b[8]):.0f}→{','.join(b[9:]) or '—'}" for b in z)
        rows.append([code(r['file']), r['numplayers'], len(z), desc, byteam[r['file']].get('TrunkPort', ''),
                     r['spawnpoints_by_team'], r['leveloptions']])
    return md(['File', 'Players', 'Flags', 'Flag id:owner radius→links (file order)', 'TrunkPorts', 'SpawnPoints', 'LevelOptions'], rows)


def T_zones():
    rows = []
    for r in TOP:
        g = r['gametypes_tokens']
        if g not in ('KingOfTheHill', 'CaptureTheStatue'):
            continue
        bl = raw_buildings(r['file'])
        z = [b for b in bl if b[0].lower() == 'multiwiniazone']
        desc = "; ".join(f"{b[1]}:{teamlab(b[4])} r{float(b[8]):g} @({float(b[2]):.0f},{float(b[3]):.0f})" for b in z)
        rows.append([code(r['file']), MODE_SHORT[g], r['numplayers'], len(z), desc])
    return md(['File', 'Mode', 'Players', 'Zones', 'Zone id:owner radius @(x,z)'], rows)


def T_rr():
    rows = []
    for r in TOP:
        if r['gametypes_tokens'] != 'RocketRiot':
            continue
        bt = byteam[r['file']]
        rows.append([code(r['file']), r['numplayers'], bt.get('EscapeRocket', ''), bt.get('FuelStation', ''),
                     bt.get('FuelGenerator', ''), cnt[r['file']].get('FuelPipe', ''), bt.get('TrunkPort', ''),
                     bt.get('Pylon', ''), cnt[r['file']].get('SolarPanel', ''), cnt[r['file']].get('PowerSplitter', ''),
                     bt.get('PylonEnd', ''), r['spawnpoints_by_team'], r['instantunits'], r['leveloptions'], r['invalidcrates']])
    return md(['File', 'Players', 'EscapeRocket', 'FuelStation', 'FuelGenerator', 'FuelPipe', 'TrunkPort', 'Pylon',
               'SolarPanel', 'PowerSplitter', 'PylonEnd', 'SpawnPoints', 'Starting units', 'LevelOptions', 'InvalidCrates'], rows)


def T_thumbs():
    rows = []
    for r in T:
        rows.append([code(r['file']), r['size_bytes'], f"{r['width']}×{r['height']}×{r['bpp']}bpp", code(r['level_file']),
                     r['note'], ships(r)])
    return md(['Thumbnail', 'Bytes', 'Format', 'Level (case-insensitive stem)', 'Note', 'Data shipped in'], rows)


def T_darwinia():
    rows = []
    for r in L:
        if r['kind'] not in ('map', 'mission'):
            continue
        pair = " ".join(code('data/levels/' + p) for p in r['pairing'].split() if p.endswith('.txt'))
        rows.append([code(r['file']), r['kind'], pair or r['pairing'], r['sections'], r['buildings_total'],
                     r['blocked_buildings_FULL'], r['unknown_building_types'], r['camera_mounts'], r['camera_anims'],
                     r['instantunit_total'], r['routes'], r['primary_objectives'][:70], r['created_as_difficulty'],
                     " ".join(code(x) for x in r['referenced_by'].split())])
    return md(['File', 'Kind', 'Paired with', 'Sections', 'Bldgs', 'Blocked in FULL', 'Unknown types', 'Cam mounts', 'Cam anims',
               'Instant units', 'Routes', 'Primary objectives (truncated at 70 chars)', 'CreatedAsDifficulty', 'Referenced by'], rows)


def T_old():
    rows = []
    for r in L:
        if r['dir'] != 'levels/old':
            continue
        gt = r['gametypes_tokens'] or ('(empty GameTypes line)' if 'multiwiniaoptions' in r['sections'] else '(no options block)')
        rows.append([code(r['file']), r['english_name'], gt, r['numplayers'], r['populationcap'], r['worldSizeX'], r['cellSize'],
                     r['tiles'], r['buildings_total'], r['blocked_buildings_FULL'], r['spawnpoints_by_team'], r['instantunits'],
                     r['leveloptions'], r['effects'], r['invalidcrates']])
    return md(['File', 'English name', 'GameTypes', 'Players', 'PopCap', 'worldSize', 'cell', 'tiles', 'Bldgs',
               'Blocked in FULL', 'SpawnPoint by pos', 'InstantUnits', 'LevelOptions', 'Effects', 'InvalidCrates'], rows)


def T_levelenum():
    rows = []
    p = os.path.join(SRC, 'data/levels/level_enum.txt')
    present = {os.path.basename(r['file']).lower() for r in TOP}
    for line in open(p, encoding='latin-1'):
        tok = line.split()
        if len(tok) >= 2:
            f = tok[0]
            ex = f.lower() in present
            rows.append([tok[1], f"`data/levels/{f}`" if ex else f, 'in tree' if ex else 'absent (PDLC)'])
    return md(['Id', 'Map file', 'Status'], rows)


def T_inventory():
    c = collections.Counter((r['dir'], r['kind']) for r in L)
    rows = [['`data/levels/mp_*.txt`', c[('levels', 'mp')], 'Multiwinia map (single file)', '[B:FULL]'],
            ['`data/levels/map_*.txt`', c[('levels', 'map')], 'Darwinia location map', '[B:LEGACY]'],
            ['`data/levels/mission_*.txt`', c[('levels', 'mission')], 'Darwinia mission overlay', '[B:LEGACY]'],
            ['`data/levels/level_enum.txt`', 1, 'achievement level index', '[B:FULL] code; excluded from retail data'],
            ['`data/levels/old/mp_*.txt`', c[('levels/old', 'mp')], 'superseded Multiwinia maps', 'never loaded'],
            ['`data/thumbnails/*.bmp`', len(T), 'map-select thumbnails (56 `mp_*` + `default.bmp`)', '[B:FULL]']]
    return md(['Location', 'Files', 'Kind', 'Build tag'], rows)


TABLES = {k[2:]: v for k, v in globals().items() if k.startswith('T_') and callable(v)}

if CMD == 'show':
    print(TABLES[sys.argv[4]]())
elif CMD == 'fill':
    doc = sys.argv[4]
    text = open(doc, encoding='utf-8').read()
    for name, fn in TABLES.items():
        pat = re.compile(r'(<!-- S08:%s -->)(.*?)(<!-- /S08:%s -->)' % (name, name), re.S)
        if pat.search(text):
            text = pat.sub(lambda m: m.group(1) + "\n" + fn() + "\n" + m.group(3), text)
    open(doc, 'w', encoding='utf-8').write(text)
    print('filled', doc, CROSS)
