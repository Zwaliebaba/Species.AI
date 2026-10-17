#!/usr/bin/env python3
"""S21 helper: generate the class-hierarchy section of 21_simulation_and_entity_model.md (Mermaid diagrams of
every code/worldobject class + a table of all classes with build tag and behaviour-spec link), and verify coverage.

Usage:
  python3 -I S_21_classdiagram.py gen    <class_assignment.csv> <DESIGN_DIR> <out_fragment.md>
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
    covered |= diagram(f'#### 2.3 Entity subclasses — {len(ent) + 1} classes (Entity and every class derived from it)',
                       [('WorldObject', '<|--', 'Entity', '')] +
                       [(rows[c]['base'], '<|--', c, '') for c in ent])
    tree = sorted(groups['BuildingTree'], key=lambda c: (rows[c]['base'] != 'Building', rows[c]['base'], c))
    covered |= diagram(f'#### 2.4 Building intermediate bases and their subclasses — {len(tree) + 1} classes',
                       [('WorldObject', '<|--', 'Building', '')] +
                       [(rows[c]['base'], '<|--', c, '') for c in tree])
    leaf = sorted(groups['BuildingLeaf'])
    covered |= diagram(f'#### 2.5 Building direct leaf subclasses — {len(leaf)} classes',
                       [('Building', '<|--', c, '') for c in leaf])
    oth = sorted(groups['Other'], key=lambda c: (rows[c]['base'], c))
    covered |= diagram(f'#### 2.6 Other WorldObject-derived classes (effects, weapons, spirits, order markers) and Unit '
                       f'subclasses — {len(oth) + 1} classes',
                       [(rows[c]['base'], '<|--', c, '') for c in oth], standalone=['WorldObject'])
    hel = sorted(groups['Helper'])
    covered |= diagram(f'#### 2.7 Helper classes with no WorldObject ancestry — {len(hel)} classes, with their owners',
                       [(a, rel, b, lab) for a, rel, b, lab in HELPER_REL], standalone=hel)
    nocover = sorted(set(rows) - covered)
    assert not nocover, nocover

    # ---- table
    w('#### 2.8 Every class: base, header, build tag, behaviour specification\n')
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


if __name__ == '__main__':
    if sys.argv[1] == 'gen':
        gen(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        sys.exit(verify(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None))
