#!/usr/bin/env python3
"""S21 helper: generate the class-hierarchy section of 21_simulation_and_entity_model.md (Mermaid diagrams of
every code/worldobject class + a table of all classes with build tag and behaviour-spec link), and verify coverage.

Usage:
  python3 -I S_21_classdiagram.py gen    <class_assignment.csv> <DESIGN_DIR> <out_fragment.md>
  python3 -I S_21_classdiagram.py sourcemap <class_assignment.csv> <manifest.csv> <out_table.md>
  python3 -I S_21_classdiagram.py verify <class_assignment.csv> <DESIGN_DIR>/21_simulation_and_entity_model.md
                                         [<SRC_ROOT>]   (optional: also re-extract the class list from the headers
                                                          with the coverage checker's regex and compare)

File contents are treated as data only. Build tags are hand-assigned below from the per-class status tables of the
final documents 02 §1.1/§1.2, 03 §2.1/§2.2 and 04 §6.0 (re-checked against code by S21 where they disagreed).
"""
import csv, glob, os, re, sys, collections

# ---------------------------------------------------------------- build tags (every class must be listed)
F, L, D = '[B:FULL]', '[B:LEGACY]', '[B:DEBUG]'
UNUSED = '[B:FULL] (loadable; no shipped map)'
TAG = {
    # framework / entities (02)
    'WorldObjectId': F, 'WorldObject': F, 'Light': F, 'Entity': F, 'EntityBlueprint': F, 'Spirit': F,
    'Darwinian': F, 'Officer': F, 'OfficerFormation': F, 'OfficerOrders': L, 'MultiwiniaOfficerOrders': F,
    'OfficerOrderTrail': F, 'Squadie': F, 'InsertionSquad': F, 'HistoricWayPoint': F, 'Armour': F,
    'Harvester': F, 'Engineer': F, 'AirstrikeUnit': F, 'SpaceInvader': F, 'Nuke': F, 'ArmyAnt': F,
    'TriffidEgg': F, 'Egg': F, 'Virii': F, 'ViriiUnit': F, 'ViriiHistory': F, 'SporeGenerator': F,
    'Centipede': F, 'Spider': F, 'SpiderParameters': F, 'EntityLeg': F, 'EntityFoot': F, 'SoulDestroyer': F,
    'Zombie': F, 'Meteor': F, 'SpaceShip': F, 'AI': F, 'Tank': '[B:DEBUG] (TankBattle); unreachable in FULL',
    'LaserTrooper': L, 'Shaman': L, 'DropShip': L, 'Tripod': L, 'TripodNavData': L,
    'Tentacle': '[B:DEBUG] (via Portal / The Beast)', 'Lander': L,
    # buildings (03)
    'Building': F, 'BuildingPort': F, 'SpawnBuilding': F, 'SpawnBuildingLink': F, 'SpawnBuildingSpirit': F,
    'SpawnLink': F, 'MasterSpawnPoint': F, 'SpawnPoint': F, 'SpawnPopulationLock': F, 'GunTurret': F,
    'Teleport': F, 'RadarDish': F, 'ControlStation': F, 'JumpPad': F, 'PowerBuilding': F, 'SolarPanel': F,
    'Pylon': F, 'PylonEnd': F, 'PowerSplitter': F, 'Generator': L, 'PylonStart': L, 'Powerstation': L,
    'LaserFence': F, 'Wall': F, 'Tree': F, 'StaticShape': F, 'RestrictionZone': F, 'SmokeMarker': F,
    'EruptionMarker': F, 'MultiwiniaZone': F, 'Flag': F, 'CarryableBuilding': F, 'Statue': F, 'WallBuster': F,
    'TrunkPort': F, 'PulseBomb': F, 'FuelBuilding': F, 'FuelGenerator': F, 'FuelPipe': F, 'FuelStation': F,
    'EscapeRocket': F, 'AntHill': F, 'Triffid': F, 'Portal': D, 'Randomiser': F, 'AITarget': F,
    'AIObjective': F, 'AIObjectiveMarker': F, 'AISpawnPoint': L, 'ControlTower': UNUSED, 'Incubator': UNUSED,
    'CloneLab': UNUSED, 'ConstructionYard': L, 'DisplayScreen': L, 'ReceiverBuilding': L, 'SpiritProcessor': L,
    'ReceiverLink': L, 'ReceiverSpiritSpawner': L, 'SpiritReceiver': L,
    'UnprocessedSpirit': '[B:FULL] code path (soulrain level effect; no shipped map)',
    'SpiritStore': L, 'Factory': L, 'BlueprintBuilding': L, 'BlueprintStore': L, 'BlueprintConsole': L,
    'BlueprintRelay': L, 'Bridge': L, 'Cave': L, 'ChessBase': L, 'ChessPiece': L, 'FeedingTube': L,
    'DynamicBase': L, 'DynamicHub': L, 'DynamicNode': L, 'GodDish': L, 'Spam': L, 'Library': L,
    'ResearchItem': L, 'MineBuilding': L, 'MineCart': L, 'TrackLink': L, 'TrackJunction': L, 'TrackStart': L,
    'TrackEnd': L, 'Refinery': L, 'Mine': L, 'SafeArea': L, 'ScriptTrigger': L, 'FenceSwitch': L,
    'UpgradePort': L, 'PrimaryUpgradePort': L, 'Crate': F,
    # weapons / effects (04)
    'ThrowableWeapon': F, 'Grenade': F, 'FlameGrenade': F, 'AirStrikeMarker': F, 'ControllerGrenade': L,
    'Rocket': F, 'Laser': F, 'TurretShell': F, 'TurretEmptyShell': F, 'Fireball': F, 'Shockwave': F,
    'MuzzleFlash': F, 'Missile': '[B:LEGACY] (compiled, no caller)', 'LandMine': '[B:LEGACY] (compiled, no caller)',
    'PulseWave': F, 'SpamInfection': F, 'Lightning': F, 'Snow': F, 'DustBall': F, 'GunTurretTarget': F,
    'BoxKite': L,
}

# helper (non-WorldObject) classes: ownership relations drawn in the helper diagram [C] member declarations
HELPER_REL = [
    ('WorldObject', '*--', 'WorldObjectId', 'm_id'),
    ('Entity', '..>', 'EntityBlueprint', 'SetType stats'),
    ('Location', 'o--', 'Light', 'm_lights'),
    ('Building', '*--', 'BuildingPort', 'm_ports'),
    ('SpawnBuilding', '*--', 'SpawnBuildingLink', 'm_links'),
    ('SpawnBuildingLink', '*--', 'SpawnBuildingSpirit', 'm_spirits'),
    ('Officer', '*--', 'OfficerFormation', 'm_formationEntities'),
    ('Officer', '*--', 'Flag', 'm_flag'),
    ('Armour', '*--', 'Flag', 'm_flag m_deployFlag'),
    ('MultiwiniaZone', 'o--', 'Flag', 'm_blitzkriegFlag'),
    ('InsertionSquad', '*--', 'HistoricWayPoint', 'm_positionHistory'),
    ('Virii', '*--', 'ViriiHistory', 'm_positionHistory'),
    ('Spider', '*--', 'SpiderParameters', 'm_parameters'),
    ('Spider', '*--', 'EntityLeg', 'm_legs'),
    ('Tripod', '*--', 'EntityLeg', 'm_legs'),
    ('Tripod', '..>', 'TripodNavData', 'navigation'),
    ('EntityLeg', '*--', 'EntityFoot', 'm_foot'),
    ('Factory', '*--', 'SpiritStore', 'm_spiritStore'),
    ('MineBuilding', '*--', 'MineCart', 'm_carts'),
]

LEGACY_LIKE = ('[B:LEGACY]', '[B:DEBUG]')


def slug(title, seen):
    s = title.strip().lower()
    s = re.sub(r'[^\w\- ]', '', s, flags=re.U)
    s = s.replace(' ', '-')
    n = seen[s]
    seen[s] += 1
    return s if n == 0 else f'{s}-{n}'


def headings(path):
    out, seen, fence = [], collections.Counter(), False
    for line in open(path, encoding='utf-8'):
        if line.startswith('```'):
            fence = not fence
            continue
        if fence:
            continue
        m = re.match(r'^(#{1,6})\s+(.*?)\s*$', line)
        if m:
            title = m.group(2)
            out.append((len(m.group(1)), title, slug(title, seen)))
    return out


PREFER = {'Entity': 'base class of all units', 'SpaceInvader': 'SpaceInvader — behaviour'}


def best_anchor(name, heads):
    best = None
    for level, title, anchor in heads:
        if name in PREFER and PREFER[name] in title:
            return (-1, level, anchor, title)
        if level < 2:
            continue
        plain = re.sub(r'[`*]', '', title)
        plain = re.sub(r'^[0-9.]+\s+', '', plain)
        if not re.search(r'\b' + re.escape(name) + r'\b', plain):
            continue
        if plain == name:
            score = 0
        elif plain.startswith(name + ' ') or plain.startswith(name + ' —') or plain.startswith(name + ','):
            score = 1
        elif plain.startswith(name):
            score = 2
        else:
            score = 3
        cand = (score, level, anchor, title)
        if best is None or cand[:2] < best[:2]:
            best = cand
    return best


def load(csv_path):
    rows = list(csv.DictReader(open(csv_path, encoding='utf-8')))
    return {r['class']: r for r in rows}


def root(c, rows):
    seen = set()
    while c in rows and rows[c]['base'] and c not in seen:
        seen.add(c)
        b = rows[c]['base']
        if b in ('Entity', 'Building', 'Unit', 'WorldObject'):
            return b
        c = b
    return None


def gen(csv_path, design, out_path):
    rows = load(csv_path)
    missing = sorted(set(rows) - set(TAG))
    extra = sorted(set(TAG) - set(rows))
    assert not missing and not extra, (missing, extra)
    docname = {'02': '02_units_and_entities.md', '03': '03_buildings_and_world_objects.md',
               '04': '04_crates_powerups_and_weapons.md'}
    heads = {d: headings(os.path.join(design, f)) for d, f in docname.items()}

    groups = collections.defaultdict(list)
    for c, r in rows.items():
        if c in ('Entity', 'Building'):
            continue
        rt = root(c, rows)
        if rt == 'Entity':
            groups['Entity'].append(c)
        elif rt == 'Building':
            # intermediate bases (classes that are themselves a base) and their subtrees vs direct leaves
            if r['base'] == 'Building' and not any(x['base'] == c for x in rows.values()):
                groups['BuildingLeaf'].append(c)
            else:
                groups['BuildingTree'].append(c)
        elif rt in ('WorldObject', 'Unit'):
            groups['Other'].append(c)
        elif c != 'WorldObject':
            groups['Helper'].append(c)
    out = []
    w = out.append

    def annot(cls):
        t = TAG[cls]
        if t.startswith('[B:LEGACY]'):
            return f'    <<legacy>> {cls}'
        if t.startswith('[B:DEBUG]'):
            return f'    <<debug>> {cls}'
        return None

    def diagram(title, edges, standalone=(), header=()):
        w(title + '\n')
        w('```mermaid')
        w('classDiagram')
        for h in header:
            w(h)
        names = set()
        for a, rel, b, lab in edges:
            w(f'    {a} {rel} {b}' + (f' : {lab}' if lab else ''))
            names.update((a, b))
        for s in standalone:
            if s not in names:
                w(f'    class {s}')
                names.add(s)
        for n in sorted(names):
            if n in TAG:
                a = annot(n)
                if a:
                    w(a)
        w('```\n')
        return names

    covered = set()
    ent = sorted(groups['Entity'])
    covered |= diagram(f'### 2.3 Entity subclasses — {len(ent) + 1} classes (Entity and every class derived from it)',
                       [('WorldObject', '<|--', 'Entity', '')] +
                       [(rows[c]['base'], '<|--', c, '') for c in ent])
    tree = sorted(groups['BuildingTree'], key=lambda c: (rows[c]['base'] != 'Building', rows[c]['base'], c))
    covered |= diagram(f'### 2.4 Building intermediate bases and their subclasses — {len(tree) + 1} classes',
                       [('WorldObject', '<|--', 'Building', '')] +
                       [(rows[c]['base'], '<|--', c, '') for c in tree])
    leaf = sorted(groups['BuildingLeaf'])
    covered |= diagram(f'### 2.5 Building direct leaf subclasses — {len(leaf)} classes',
                       [('Building', '<|--', c, '') for c in leaf])
    oth = sorted(groups['Other'], key=lambda c: (rows[c]['base'], c))
    covered |= diagram(f'### 2.6 Other WorldObject-derived classes (effects, weapons, spirits, order markers) and Unit '
                       f'subclasses — {len(oth) + 1} classes',
                       [(rows[c]['base'], '<|--', c, '') for c in oth], standalone=['WorldObject'])
    hel = sorted(groups['Helper'])
    covered |= diagram(f'### 2.7 Helper classes with no WorldObject ancestry — {len(hel)} classes, with their owners',
                       [(a, rel, b, lab) for a, rel, b, lab in HELPER_REL], standalone=hel)
    nocover = sorted(set(rows) - covered)
    assert not nocover, nocover

    # ---- table
    w('### 2.8 Every class: base, header, build tag, behaviour specification\n')
    w('Generated by `_work/scripts/S_21_classdiagram.py` from `_work/class_assignment.csv` (the class list is the one the '
      'coverage checker extracts from `code/worldobject/*.h`). "Spec" links to the heading in the final document that '
      'carries the class\'s behaviour specification. Build tags: `_work/build_matrix.md` §7.\n')
    w('| # | Class | Base | Header | Build tag | Spec |')
    w('|---|---|---|---|---|---|')
    unresolved = []
    for i, c in enumerate(sorted(rows, key=str.lower), 1):
        r = rows[c]
        d = r['doc']
        ba = best_anchor(c, heads[d])
        if ba:
            link = f'[{d} — {re.sub(r"[`|]", "", ba[3])}]({docname[d]}#{ba[2]})'
        else:
            link = f'[{d}]({docname[d]})'
            unresolved.append(c)
        base = f'`{r["base"]}`' if r['base'] else '—'
        w(f'| {i} | `{c}` | {base} | `{r["header"]}` | {TAG[c]} | {link} |')
    w('')
    cnt = collections.Counter(TAG[c].split(' ')[0] for c in rows)
    w('Totals: ' + ', '.join(f'{k} {v}' for k, v in sorted(cnt.items())) + f'; {len(rows)} classes.\n')
    open(out_path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print('groups', {k: len(v) for k, v in groups.items()}, 'unresolved spec anchors:', unresolved)


def verify(csv_path, doc_path, src=None):
    rows = load(csv_path)
    names = set(rows)
    if src:
        cls = set()
        for h in sorted(glob.glob(os.path.join(src, 'code', 'worldobject', '*.h'))):
            txt = open(h, encoding='latin-1').read()
            txt = re.sub(r'//[^\n]*', '', txt)
            txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
            cls |= {m.group(1) for m in re.finditer(r'\bclass\s+(\w+)\s*(?::\s*([^{;]+))?\{', txt)}
        print('header classes', len(cls), 'csv classes', len(names), 'diff', sorted(cls ^ names))
        names |= cls
    text = open(doc_path, encoding='utf-8').read()
    merm = '\n'.join(re.findall(r'```mermaid(.*?)```', text, flags=re.S))
    miss = sorted(n for n in names if not re.search(r'\b' + re.escape(n) + r'\b', merm))
    print(f'{len(names)} classes; {len(names) - len(miss)} present in mermaid blocks; missing: {miss}')
    return 0 if not miss else 1


# ---------------------------------------------------------------- source map (core files + every worldobject header)
CORE = [
    ('code/location.h', 'Location: members, containers, API declarations'),
    ('code/location.cpp', 'Location: Init order, containers, per-slice Advance, Spawn*/Get* resolvers, IsWalkable, GetWalkingDistance, IsFriend, weather, Bang, teams/colours/positions'),
    ('code/team.h', 'Team, LobbyTeam, TeamControls declarations; team types'),
    ('code/team.cpp', 'Team::Advance order, NewUnit/NewEntity, selection, Eliminate, TeamControls pack/sample'),
    ('code/unit.h', 'Unit declaration (base of InsertionSquad, AirstrikeUnit, ViriiUnit)'),
    ('code/unit.cpp', 'Unit::AdvanceEntities, Advance, formation offsets, FollowRoute, destructor'),
    ('code/globals.h', 'NUM_TEAMS, NUM_SLICES_PER_FRAME'),
    ('code/entity_grid.h', 'EntityGrid declaration'),
    ('code/entity_grid.cpp', 'EntityGrid cells, footprint, exact and approximate queries'),
    ('code/entity_grid_cache.h', 'EntityGridCacheElement, ENTITYGRIDCACHE_MAXAGE'),
    ('code/entity_grid_cache.cpp', 'cache hit rule, eviction'),
    ('code/obstruction_grid.h', 'ObstructionGrid declaration'),
    ('code/obstruction_grid.cpp', 'CalculateAll, CalculateBuildingArea (fence rasterising), GetBuildings'),
    ('code/routing_system.h', 'RoutingSystem, Land/Sea, RouteNode, RoutingTableEntry, WayPoint, Route'),
    ('code/routing_system.cpp', 'graph construction, GenerateRoute, route rendering'),
    ('code/lib/matrix_cache.h', 'SymmetricMatrixCache used by the routing walkability caches'),
    ('code/lib/slice_darray.h', 'SliceDArray declaration'),
    ('code/lib/slice_darray.cpp', 'GetNextSliceBounds'),
    ('code/lib/2d_array.h', 'Array2D declaration'),
    ('code/lib/2d_array.cpp', 'Array2D accessors, outside value'),
    ('code/lib/2d_surface_map.h', 'SurfaceMap2D declaration'),
    ('code/lib/2d_surface_map.cpp', 'SurfaceMap2D indexing (nearest, bilinear)'),
    ('code/lib/auto_vector.h', 'auto_vector container'),
    ('contrib/systemiv/lib/tosser/llist.h', 'LList declaration'),
    ('contrib/systemiv/lib/tosser/llist.cpp', 'LList semantics'),
    ('contrib/systemiv/lib/tosser/darray.h', 'DArray declaration'),
    ('contrib/systemiv/lib/tosser/darray.cpp', 'DArray lowest-free-slot policy'),
    ('contrib/systemiv/lib/tosser/fast_darray.h', 'FastDArray declaration'),
    ('contrib/systemiv/lib/tosser/fast_darray.cpp', 'FastDArray LIFO free list, growth copy'),
    ('contrib/systemiv/lib/tosser/bounded_array.h', 'BoundedArray'),
    ('contrib/systemiv/lib/tosser/bounded_array.cpp', 'BoundedArray bodies'),
    ('contrib/systemiv/lib/tosser/btree.h', 'BTree declaration'),
    ('contrib/systemiv/lib/tosser/btree.cpp', 'BTree pre-order conversion'),
    ('contrib/systemiv/lib/tosser/hash_table.h', 'HashTable declaration'),
    ('contrib/systemiv/lib/tosser/hash_table.cpp', 'HashTable probing, hash function'),
    ('contrib/systemiv/lib/tosser/sorting_hash_table.h', 'SortingHashTable declaration'),
    ('contrib/systemiv/lib/tosser/sorting_hash_table.cpp', 'SortingHashTable ordered walk'),
    ('contrib/systemiv/lib/tosser/directory.h', 'Directory container'),
    ('contrib/systemiv/lib/tosser/directory.cpp', 'Directory bodies'),
    ('code/worldobject/worldobject.cpp', 'WorldObjectId (allocation, comparison), WorldObject base, BounceOffLandscape, Light'),
    ('code/worldobject/entity.cpp', 'Entity factory, base Advance, AdvanceInAir/InWater/Dead, PushFromObstructions, EnterTeleports, FollowRoute, RayHit'),
    ('code/worldobject/building.cpp', 'Building base Advance, Initialise, PushFromBuilding, CreateBuilding'),
    ('code/worldobject/weapons.cpp', 'Laser conversion (destroy + respawn), throwable gravity'),
    ('code/worldobject/tree.cpp', 'Tree::Generate hit sphere from render-time generation; CalculateAll caller'),
    ('code/worldobject/crate.cpp', 'Crate render-side orientation; CalculateAll callers; runtime building ids'),
    ('code/worldobject/radardish.cpp', 'teleport disable/enable and grid re-insertion'),
    ('code/worldobject/teleport.cpp', 'teleport entry disables entities'),
    ('code/worldobject/insertion_squad.cpp', 'InsertionSquad::DirectControl local-state leak L1'),
    ('code/worldobject/darwinian.cpp', 'SearchForThreats (cache user), approximate count users, unique-id gameplay uses'),
    ('code/worldobject/spawnpoint.cpp', 'SpawnPopulationLock approximate count user'),
    ('code/worldobject/officer.cpp', 'route generation callers; CalculateBoundingSphere (local)'),
    ('code/worldobject/armour.cpp', 'sea routing user; approximate count users'),
    ('code/worldobject/statue.cpp', 'route generation caller; runtime AI target id'),
    ('code/worldobject/wallbuster.cpp', 'route generation caller'),
    ('code/worldobject/gunturret.cpp', 'approximate query users; render writes'),
    ('code/worldobject/laserfence.cpp', 'render-side link reset'),
    ('code/worldobject/entity_leg.cpp', 'wall-clock leg timing (sim-resident cosmetic)'),
    ('code/gametimer.h', 'g_predictionTime, SERVER_ADVANCE_PERIOD macros'),
    ('code/gametimer.cpp', 'GetPredictionTime, server advance period'),
    ('code/network/clienttoserver.cpp', 'ProcessAllServerLetters: the only caller of Location::Advance; command application'),
    ('code/network/server.cpp', '10 Hz server tick'),
    ('code/network/iframe.cpp', 'debug state hash (what is and is not hashed)'),
    ('code/main.cpp', 'DoEnterLocation: ResetUniqueIdSeed before new Location; LocationGameLoop render-frame calls'),
    ('code/global_world.cpp', 'GenerateBuildingId'),
    ('code/level_file.cpp', 'building and instant-unit parsing (owners by position, route columns), level effects'),
    ('code/location_input.cpp', 'local preview routes (slot sharing), FenceSwitch local switch'),
    ('code/gamecursor.cpp', 'RenderCrosshair type-assignment typo; m_walkableRoute reader'),
    ('code/particle_system.h', 'ParticleSystem container'),
    ('code/particle_system.cpp', 'cosmetic particle advance'),
    ('code/multiwinia.h', 'MULTIWINIA_NUM_TEAMCOLOURS'),
    ('code/multiwinia.cpp', 'colour tables, InitialiseTeam, Multiwinia::Advance at slice 8'),
    ('code/multiwiniahelp.cpp', 'tutorial director inserting crates from the render loop (leak L5)'),
    ('code/taskmanager_interface_multiwinia.cpp', 'local-state leaks L3, L4'),
    ('code/camera.cpp', 'free-look flag read by L1'),
    ('data/stats.txt', 'entity stat rows loaded by EntityBlueprint (positional)'),
    ('data/input/input_preferences.txt', 'CameraFreeMovement binding (L1 trigger)'),
    ('data/levels/mp_*.txt', 'level buildings, instant units, effects (scanned for route ids and soulrain)'),
]


def sourcemap(csv_path, manifest_path, out_path):
    rows = load(csv_path)
    man = {r['path'].lower() for r in csv.DictReader(open(manifest_path, encoding='utf-8'))}
    import fnmatch
    by_hdr = collections.defaultdict(list)
    for c, r in rows.items():
        by_hdr[r['header']].append(c)
    out = ['| File | Role in this document |', '|---|---|']
    bad = []
    seen = set()
    for path, role in CORE:
        ok = any(fnmatch.fnmatchcase(m, path.lower()) for m in man) if '*' in path else path.lower() in man
        if not ok:
            bad.append(path)
        seen.add(path)
        out.append(f'| `{path}` | {role} |')
    for h in sorted(by_hdr):
        if h in seen:
            continue
        if h.lower() not in man:
            bad.append(h)
        out.append(f'| `{h}` | declares {", ".join("`" + c + "`" for c in sorted(by_hdr[h]))} (class diagram §2) |')
    open(out_path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print(len(out) - 2, 'rows; not in manifest:', bad)


if __name__ == '__main__':
    if sys.argv[1] == 'gen':
        gen(sys.argv[2], sys.argv[3], sys.argv[4])
    elif sys.argv[1] == 'sourcemap':
        sourcemap(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        sys.exit(verify(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None))
