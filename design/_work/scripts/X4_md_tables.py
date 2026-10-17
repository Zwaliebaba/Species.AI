"""X4 helper: render markdown tables for drafts/X4_level_catalogue.md from the X4 CSVs.
Usage: python3 -I X4_md_tables.py <OUT_DRAFTS_DIR> <table-name>
Tables: master, options, objects, thumbs, darwinia, old, modes, buildings_matrix
"""
import sys, os, csv, collections

def load(d, n):
    return list(csv.DictReader(open(os.path.join(d, n))))

def md(header, rows):
    out = ["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
    for r in rows:
        out.append("| " + " | ".join(str(x).replace('|', '/') for x in r) + " |")
    return "\n".join(out)

def short(f):
    return f.replace('data/levels/', '').replace('data/thumbnails/', '')

FULL_MODES = {"Domination", "KingOfTheHill", "CaptureTheStatue", "Assault", "RocketRiot", "Blitzkrieg"}

def fullcol(r):
    modes = set(r['menu_types'].split())
    if not modes:
        return 'never listed'
    if not (modes & FULL_MODES):
        return 'mode not compiled'
    return 'listed' if r['ships_retail'] == 'Y' else 'permitted, data absent'

def klass(r):
    f = os.path.basename(r['file']).lower()
    tags = []
    if 'stress' in f: tags.append('stress-test')
    if 'benchmark' in f: tags.append('benchmark')
    if 'sandbox' in f: tags.append('sandbox/test')
    if 'tankbattle' in f: tags.append('tankbattle [B:DEBUG mode]')
    if not r['menu_types']: tags.append('hidden (no valid GameTypes)')
    if r['ships_retail'] != 'Y': tags.append('not in retail data')
    if not tags: tags.append('official, retail')
    return "; ".join(tags)

def main():
    d, t = sys.argv[1], sys.argv[2]
    L = load(d, 'X4_levels.csv')
    top = [r for r in L if r['dir'] == 'levels' and r['kind'] == 'mp']
    if t == 'master':
        rows = []
        for r in top:
            mid = r['crc']
            perm = []
            for k, lab in [('permitted_pc_preview', 'PREV'), ('permitted_pc_hardwarecompat', 'HW'),
                           ('permitted_assault_stresstest', 'STRESS'), ('permitted_betatest_group_a', 'βA'),
                           ('permitted_betatest_group_b', 'βB'), ('permitted_betatest_group_c', 'βC')]:
                if r[k] == 'Y': perm.append(lab)
            ships = [lab for k, lab in [('ships_retail', 'retail'), ('ships_pcpreview', 'prev'), ('ships_hwcompat', 'hw'),
                                         ('ships_assaultstress', 'stress'), ('ships_beta_a', 'βA'), ('ships_beta_b', 'βB'),
                                         ('ships_beta_c', 'βC')] if r[k] == 'Y']
            rows.append([short(r['file']), r['english_name'] or '—', r['mapid_define'], mid, r['level_enum_id'] or '—',
                         r['gametypes_tokens'] or '—', r['numplayers'] or '—',
                         f"{r['worldSizeX']}×{r['worldSizeZ']} / {r['cellSize']} ({r['grid_cells']})",
                         short(r['thumbnail']) or '— (default.bmp)', " ".join(ships) or '—',
                         fullcol(r), " ".join(perm) or '—', 'Y' if r['demo_default_allowed'] else '',
                         r['tutorial'].replace('SetupTutorial2', 'Tutorial2').replace('SetupTutorial', 'Tutorial1'), klass(r)])
        print(md(['File', 'English name (IP)', 'MAPID macro', 'CRC (=map id)', 'level_enum id', 'GameTypes', 'Players',
                  'World size / cell (grid)', 'Thumbnail', 'Data shipped in', 'FULLGAME menu (retail data)', 'PERMITTED_MAPS of other targets',
                  'Demo default', 'Tutorial', 'Class'], rows))
    elif t == 'options':
        rows = []
        for r in top:
            pc = r['populationcap'] + (f" (lines: {r['populationcap_all']})" if r['populationcap_all'] else '')
            rows.append([short(r['file']), pc or '—', r['defenderpopulationcap'] or '—', r['playtime'] or '—', r['difficulty'] or '—',
                         r['effects'] or '—', r['leveloptions'] or '—', r['invalidcrates'] or '—', r['attackingteams'] or '—',
                         ('Coop' if r['coopmode'] else '') + (' ForceCoop' if r['forcecoop'] else '') or '—',
                         (r['coopgroup1'] + ' / ' + r['coopgroup2']) if (r['coopgroup1'] or r['coopgroup2']) else '—',
                         r['official'] or '—', r['name'] or '—'])
        print(md(['File', 'PopulationCap', 'DefenderPopCap', 'PlayTime', 'Difficulty', 'Effects', 'LevelOptions', 'InvalidCrates',
                  'AttackingTeams', 'Coop', 'CoopGroup1 / 2 positions', 'OfficialLevel', 'Name'], rows))
    elif t == 'objects':
        rows = []
        for r in top:
            rows.append([short(r['file']), r['buildings_total'], r['spawnpoints_by_team'] or '—',
                         f"{r['spawnlinks']}/{r['spawnpointmasters']}", f"{r['spawnpoplocks']}" + (f" ({r['spawnpoplock_radius_maxpop']})" if r['spawnpoplock_radius_maxpop'] else ''),
                         r['trunkports_by_team'] or '—', r['mwzones_by_team'] or '—', r['mwzone_sizes'] or '—',
                         r['escaperockets_by_team'] or '—', r['fuelstations_by_team'] or '—',
                         (r['pulsebombs'] + ' @' + r['pulsebomb_starttime']) if r['pulsebombs'] != '0' else '—',
                         r['controlstations_by_team'] or '—', r['gunturrets_by_team'] or '—', r['radardishes'],
                         f"{r['walls']}/{r['laserfences']}", r['jumppads'], r['restrictionzones'],
                         f"{r['aitargets']}/{r['aiobjectives']}/{r['aiobjectivemarkers']}", r['trees'],
                         r['staticshapes'] or '—', r['instantunits'] or '—', r['camera_mount_names'] or '—'])
        print(md(['File', 'Bldgs', 'SpawnPoint by pos', 'SpawnLink/Master', 'SpawnPopLock (radius/max)', 'TrunkPort by pos',
                  'MultiwiniaZone by pos', 'Zone sizes', 'EscapeRocket', 'FuelStation', 'PulseBomb @startTime', 'ControlStation',
                  'GunTurret', 'RadarDish', 'Wall/LaserFence', 'JumpPad', 'RestrictionZone', 'AITarget/AIObjective/Marker', 'Trees',
                  'StaticShape shapes', 'InstantUnits type@pos×n', 'Camera mounts'], rows))
    elif t == 'landscape':
        rows = []
        for r in L:
            if r['kind'] == 'mission': continue
            rows.append([short(r['file']), r['worldSizeX'], r['cellSize'], r['grid_cells'], r['outsideHeight'], r['maxHeight'] or '—',
                         r['landColourFile'].replace('landscape_', '').replace('.bmp', ''),
                         r['wavesColourFile'].replace('waves_', '').replace('.bmp', ''),
                         r['waterColourFile'].replace('water_', '').replace('.bmp', ''),
                         r['tiles'], r['tile_size'], r['tile_fracdim'], r['tile_heightscale'], r['tile_desiredheight'],
                         r['tile_genmethods'], r['tile_guidepower'], r['tile_lowland'], r['flatten_areas'], r['flatten_types'] or '—',
                         r['lights']])
        print(md(['File', 'worldSize', 'cellSize', 'grid', 'outsideHeight', 'maxHeight', 'landColour', 'waves', 'water', 'tiles',
                  'tile size', 'fracDim', 'heightScale', 'desiredHeight', 'genMethods', 'guidePower', 'lowland', 'flatten', 'flatten types', 'lights'], rows))
    elif t == 'thumbs':
        T = load(d, 'X4_thumbnails.csv')
        rows = []
        for r in T:
            ships = " ".join(lab for k, lab in [('ships_retail', 'retail'), ('ships_pcpreview', 'prev'), ('ships_hwcompat', 'hw'),
                                                 ('ships_assaultstress', 'stress'), ('ships_beta_a', 'βA'), ('ships_beta_b', 'βB'),
                                                 ('ships_beta_c', 'βC')] if r[k] == 'Y')
            rows.append([short(r['file']), r['size_bytes'], f"{r['width']}×{r['height']}×{r['bpp']}bpp", short(r['level_file']) or '—',
                         r['note'] or '', ships or '—'])
        print(md(['Thumbnail', 'Bytes', 'Format', 'Level (case-insensitive stem match)', 'Note', 'Data shipped in'], rows))
    elif t == 'darwinia':
        rows = []
        for r in L:
            if r['kind'] not in ('map', 'mission'): continue
            rows.append([short(r['file']), r['kind'], r['pairing'], r['sections'], r['buildings_total'], r['blocked_buildings_FULL'],
                         r['unknown_building_types'] or '—', r['camera_mounts'] or '—', r['camera_anims'] or '—', r['instantunit_total'],
                         r['routes'], r['primary_objectives'][:80] or '—', r['created_as_difficulty'] or '—', r['referenced_by'] or '—'])
        print(md(['File', 'Kind', 'Paired with', 'Sections', 'Bldgs', 'Blocked in FULL', 'Unknown types', 'Cam mounts', 'Cam anims',
                  'Instant units', 'Routes', 'Primary objectives', 'CreatedAsDifficulty', 'Referenced by'], rows))
    elif t == 'old':
        rows = []
        for r in L:
            if r['dir'] != 'levels/old': continue
            rows.append([short(r['file']), r['english_name'] or '—', r['gametypes_tokens'] or ('(empty GameTypes line)' if 'multiwiniaoptions' in r['sections'] else '(no options block)'),
                         r['numplayers'] or '—', r['populationcap'] or '—', r['worldSizeX'], r['cellSize'], r['tiles'], r['buildings_total'],
                         r['blocked_buildings_FULL'], r['spawnpoints_by_team'] or '—', r['instantunits'] or '—',
                         r['leveloptions'] or '—', r['mapid_define'] or '—'])
        print(md(['File', 'English name', 'GameTypes', 'Players', 'PopCap', 'worldSize', 'cell', 'tiles', 'Bldgs', 'Blocked in FULL',
                  'SpawnPoint by pos', 'InstantUnits', 'LevelOptions', 'CRC equals a MAPID?'], rows))
    elif t == 'modes':
        M = load(d, 'X4_mode_maplists.csv')
        by = collections.defaultdict(list)
        for r in M:
            if r['scope'] == 'retail' or r['gametype'] == 'TankBattle':
                by[(r['gametype_index'], r['gametype'], r['scope'])].append(r)
        for (gi, g, sc), lst in sorted(by.items(), key=lambda kv: int(kv[0][0])):
            print(f"\n**{gi} {g}** ({sc}, {len(lst)} maps)\n")
            print(md(['Menu index', 'File', 'Name', 'Players', 'Difficulty', 'PlayTime'],
                     [[r['menu_index'], short(r['file']), r['english_name'], r['numplayers'], r['difficulty'] or '—', r['playtime'] or '—'] for r in lst]))
    elif t == 'buildings_matrix':
        B = load(d, 'X4_level_buildings.csv')
        files = [r['file'] for r in top]
        cnt = collections.defaultdict(dict)
        types = set()
        for r in B:
            if r['file'] in files:
                cnt[r['building_type']][r['file']] = int(r['count'])
                types.add(r['building_type'])
        rows = []
        for ty in sorted(types):
            tot = sum(cnt[ty].values())
            rows.append([ty, tot, len(cnt[ty]), ", ".join(f"{short(f)[3:-4]}:{c}" for f, c in sorted(cnt[ty].items()))])
        print(md(['Building type', 'Total (mp_* top level)', 'Files', 'Per file'], rows))

if __name__ == '__main__':
    main()
