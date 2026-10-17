"""X4: level catalogue extractor for Multiwinia data/levels/** and data/thumbnails/**.

Usage:
    python3 -I X4_levels.py <SRC_ROOT> <OUT_DRAFTS_DIR> [--summary]

Reads (all relative to SRC_ROOT, never modified):
  data/levels/*.txt, data/levels/old/*.txt, data/thumbnails/*.bmp,
  data/levels/level_enum.txt, data/language/english.txt,
  data/game*.txt, data/purity/game.txt, data/scripts/*.txt,
  code/mapcrc.h, code/lib/universal_include.h, code/network/demo_limitations.cpp,
  code/UI/GameMenuWindow.cpp (tutorial map ids),
  tools/multiwinia_exclusions*.txt, tools/multiwinia_inclusions_*.txt
Writes:
  <OUT>/X4_levels.csv           one row per level file (map_/mission_/mp_, incl. old/)
  <OUT>/X4_level_buildings.csv  level x building-type counts (with per-team split)
  <OUT>/X4_thumbnails.csv       one row per data/thumbnails file
  <OUT>/X4_mode_maplists.csv    per game type, menu-listed maps in menu order (retail data set)

Parsing follows code/level_file.cpp:LevelFile::ParseMapFile / ParseMissionFile /
ParseMultiwiniaOptions, code/game_menu.cpp:GameMenu::AddMap / CreateMapList and
code/worldobject/building.cpp:Building::Read / BuildingIsBlocked.
File contents are treated purely as data.
"""
import sys, os, re, csv, fnmatch, struct, collections

# ---------------------------------------------------------------- constants (from code)
# code/worldobject/building.cpp:Building::GetTypeName (index order = enum order)
BUILDING_NAMES = ["Invalid", "Factory", "Cave", "RadarDish", "LaserFence", "ControlTower", "GunTurret",
    "Bridge", "Powerstation", "Tree", "Wall", "TrunkPort", "ResearchItem", "Library", "Generator",
    "Pylon", "PylonStart", "PylonEnd", "SolarPanel", "PowerSplitter", "TrackLink", "TrackJunction",
    "TrackStart", "TrackEnd", "Refinery", "Mine", "Yard", "DisplayScreen", "UpgradePort",
    "PrimaryUpgrade", "Incubator", "AntHill", "SafeArea", "Triffid", "SpiritReceiver", "ReceiverLink",
    "SpiritSpawner", "SpiritProcessor", "SpawnPoint", "SpawnPopulationLock", "SpawnPointMaster",
    "SpawnLink", "AITarget", "AISpawnPoint", "BlueprintStore", "BlueprintConsole", "BlueprintRelay",
    "ScriptTrigger", "Spam", "GodDish", "StaticShape", "FuelGenerator", "FuelPipe", "FuelStation",
    "EscapeRocket", "FenceSwitch", "DynamicHub", "DynamicNode", "FeedingTube", "MultiwiniaZone",
    "ChessPiece", "ChessBase", "CloneLab", "ControlStation", "Portal", "Crate", "Statue", "WallBuster",
    "PulseBomb", "RestrictionZone", "JumpPad", "AIObjective", "AIObjectiveMarker", "EruptionMarker",
    "SmokeMarker"]
# code/worldobject/building.cpp:Building::BuildingIsBlocked (BLOCK_OLD_DARWINIA_OBJECTS, on in FULL)
BLOCKED = {"BlueprintConsole", "BlueprintRelay", "BlueprintStore", "Bridge", "Cave", "ChessBase",
    "ChessPiece", "Crate", "DisplayScreen", "DynamicHub", "DynamicNode", "Factory", "FenceSwitch",
    "Generator", "GodDish", "Library", "Mine", "Portal", "Powerstation", "PrimaryUpgrade",
    "ReceiverLink", "SpiritSpawner", "Refinery", "ResearchItem", "SafeArea", "ScriptTrigger", "Spam",
    "SpiritProcessor", "SpiritReceiver", "Statue", "TrackEnd", "TrackJunction", "TrackLink",
    "TrackStart", "UpgradePort", "Yard", "PylonStart", "FeedingTube", "AISpawnPoint"}
# Building::CreateBuilding forces m_isGlobal=true for these; ParseBuildings asserts isGlobal for a subset
FORCED_GLOBAL = {"RadarDish", "ControlTower", "TrunkPort", "Incubator", "DynamicHub", "FenceSwitch"}
ASSERT_GLOBAL = {"TrunkPort", "ControlTower", "RadarDish", "Incubator", "FenceSwitch"}
# code/worldobject/entity.cpp:Entity::GetTypeName
ENTITY_NAMES = ["InvalidType", "LaserTroop", "Engineer", "Virii", "Squadie", "Egg", "SporeGenerator",
    "Lander", "Tripod", "Centipede", "SpaceInvader", "Spider", "Darwinian", "Officer", "ArmyAnt",
    "Armour", "SoulDestroyer", "TriffidEgg", "AI", "Shaman", "Harvester", "Nuke", "Tentacle", "Meteor",
    "Tank", "SpaceShip", "DropShip"]
# code/worldobject/entity.cpp:Entity::EntityIsBlocked
ENTITY_BLOCKED = {"AI", "ArmyAnt", "DropShip", "Egg", "Lander", "LaserTroop", "Meteor", "Nuke", "Officer",
    "Shaman", "SpaceInvader", "Tentacle", "TriffidEgg", "Tripod", "InvalidType", "SpaceShip"}
# data/multiwinia.txt GAMETYPE order = Multiwinia::s_gameBlueprints order (code/multiwinia.h enum)
GAMETYPES = ["Domination", "KingOfTheHill", "CaptureTheStatue", "Assault", "RocketRiot", "Blitzkrieg", "TankBattle"]
# INCLUDEGAMEMODE_* in TARGET_PC_FULLGAME (TankBattle commented out) - code/lib/universal_include.h
FULL_MODES = {"Domination", "KingOfTheHill", "CaptureTheStatue", "Assault", "RocketRiot", "Blitzkrieg"}
# code/worldobject/crate.cpp:Crate::GetName
CRATE_NAMES = ["Squad", "Harvester", "Engineer", "Armour", "Airstrike", "Nuke", "Darwinians", "PersonalShield",
    "Subversion", "HotFeet", "GunTurret", "Infection", "MagicalForest", "Randomiser", "AntNest", "Plague",
    "Eggs", "Meteor", "DarkForest", "CrateMania", "SpawnMania", "Bitzkreig", "Rocketwinians", "Rage",
    "RocketTurret", "FlameTurret", "SpaceShip", "Triffids", "NapalmStrike", "SlowDown", "SpeedUp", "TheBeast"]
# code/global_world.cpp:GlobalResearch::GetTypeName
RESEARCH_NAMES = ["Darwinian", "Officer", "Squad", "Laser", "Grenade", "Rocket", "Controller", "AirStrike",
    "Armour", "TaskManager", "Engineer", "Harvester", "Nuke", "Subversion", "PersonalShield", "HotFeet",
    "GunTurret", "Shaman", "Infection", "MagicalForest", "DarkForest", "AntNest", "Plague", "Egg",
    "MeteorShower", "FireExtinguisher", "Rage", "Tank", "DropShip"]
# LevelFile::ParseMultiwiniaOptions LevelOptions keywords -> takes integer/identifier argument?
LEVELOPT_ARGS = {"nohandicap": 0, "instantspawnpointcapture": 0, "trunkportreinforcements": 1,
    "trunkportarmour": 1, "nogunturrets": 0, "attackerreinforcements": 1, "defenderreinforcements": 1,
    "defenderreinforcementtype": 1, "attackerreinforcementtype": 1, "forceradarteammatch": 0,
    "noradarcontrol": 0, "airstrikecapturezones": 1, "futurewinianteam": 1,
    "attackreinforcementoption": 1, "defendreinforcementoption": 1}
EFFECTS = ["snow", "dustwind", "soulrain", "lightning", "SpecialLighting"]   # strcmp (case-sensitive)
OPTION_KEYS = ["PopulationCap", "DefenderPopulationCap", "Effects", "GameTypes", "NumPlayers", "LevelOptions",
    "Name", "InvalidCrates", "Author", "Description", "PlayTime", "Difficulty", "AttackingTeams",
    "CoopMode", "ForceCoop", "CoopGroup1Positions", "CoopGroup2Positions", "OfficialLevel"]
# Section keywords recognised by LevelFile::ParseMapFile(TextReader*) (stricmp) and ParseMissionFile
MAP_SECTIONS = {"multiwiniaoptions", "landscape", "landscapetiles", "landflattenareas", "buildings", "lights",
    "cameramounts", "cameraanimations", "instantunits", "routes", "primaryobjectives"}
MISSION_SECTIONS = {"cameramounts", "cameraanimations", "buildings", "instantunits", "routes",
    "primaryobjectives", "runningprograms", "difficulty", "leaderboardstats"}

# ---------------------------------------------------------------- CRC (LevelFile::CalculeCRC; model from P11_mapcrc.py)
_TAB = []
for _i in range(256):
    _c = _i
    for _ in range(8):
        _c = (_c >> 1) ^ 0xEDB88320 if _c & 1 else _c >> 1
    _TAB.append(_c)

def map_crc(data):
    buf = (data + b'\xff')[:100001]
    c = 0
    for b in buf:
        c = _TAB[(c ^ b) & 0xff] ^ (c >> 8)
    return c & 0xffffffff

# ---------------------------------------------------------------- helpers
def rng(vals):
    vals = [v for v in vals if v is not None]
    if not vals:
        return ""
    lo, hi = min(vals), max(vals)
    f = lambda x: ("%g" % x)
    return f(lo) if lo == hi else f"{f(lo)}..{f(hi)}"

def fnum(s):
    try:
        return float(s)
    except Exception:
        return None

def inum(s):
    try:
        return int(float(s))
    except Exception:
        return None

def teamstr(counter):
    return ";".join(f"{k}:{v}" for k, v in sorted(counter.items(), key=lambda kv: (kv[0] == 255, kv[0])))

def read_lines(path):
    with open(path, 'rb') as f:
        raw = f.read()
    return raw, raw.decode('latin-1').splitlines()

# ---------------------------------------------------------------- level parser
def parse_level(path):
    raw, lines = read_lines(path)
    L = {
        'size': len(raw), 'crc': map_crc(raw), 'sections': [], 'unknown_sections': [],
        'opts': collections.OrderedDict(), 'opt_raw': collections.defaultdict(list), 'unknown_opt_keys': [],
        'land': {}, 'tiles': [], 'flatten': [], 'lights': [], 'buildings': [], 'mounts': [], 'anims': [],
        'instant': [], 'routes': [], 'objectives': [], 'running': [], 'difficulty': None, 'leader': [],
    }
    sec = None
    cur_anim = None
    cur_route = None
    bblock = 0
    for raw_line in lines:
        s = raw_line.strip()
        if not s:
            continue
        toks = s.split()
        w = toks[0]
        wl = w.lower()
        if sec is None:
            if wl.endswith('_startdefinition'):
                sec = wl[:-len('_startdefinition')]
                L['sections'].append(sec)
                if sec == 'buildings':
                    bblock += 1
                if sec not in MAP_SECTIONS | MISSION_SECTIONS:
                    L['unknown_sections'].append(w)
            continue
        if wl == sec + '_enddefinition':
            sec = None
            cur_anim = None
            cur_route = None
            continue
        if w.startswith('#'):
            continue  # comment lines: the engine tokenises them, but every '#' line in shipped files is a header
        if sec == 'multiwiniaoptions':
            if w not in OPTION_KEYS and w.lower() not in [k.lower() for k in OPTION_KEYS]:
                L['unknown_opt_keys'].append(w)
            rest = toks[1:]
            L['opt_raw'][w.lower()].append(rest)
        elif sec == 'landscape':
            L['land'][wl] = toks[1] if len(toks) > 1 else ''
        elif sec == 'landscapetiles':
            t = toks
            tile = {'x': inum(t[0]), 'y': fnum(t[1]), 'z': inum(t[2]), 'size': inum(t[3]), 'frac': fnum(t[4]),
                    'hscale': fnum(t[5]), 'desired': fnum(t[6]), 'gen': inum(t[7]), 'seed': inum(t[8]),
                    'lowland': fnum(t[9]), 'gpow': inum(t[10]) if len(t) > 10 else None,
                    'guide': t[11] if len(t) > 11 else ''}
            L['tiles'].append(tile)
        elif sec == 'landflattenareas':
            t = toks
            fa = {'x': fnum(t[0]), 'y': fnum(t[1]), 'z': fnum(t[2]), 'size': fnum(t[3]),
                  'type': inum(t[4]) if len(t) > 4 else 0, 'thresh': fnum(t[5]) if len(t) > 5 else None}
            L['flatten'].append(fa)
        elif sec == 'lights':
            t = [fnum(x) for x in toks[:6]]
            L['lights'].append(t)
        elif sec == 'buildings':
            t = toks
            b = {'type': w, 'block': bblock, 'id': inum(t[1]) if len(t) > 1 else None,
                 'x': fnum(t[2]) if len(t) > 2 else None, 'z': fnum(t[3]) if len(t) > 3 else None,
                 'team': inum(t[4]) if len(t) > 4 else None, 'global': inum(t[7]) if len(t) > 7 else None,
                 'extra': t[8:]}
            L['buildings'].append(b)
        elif sec == 'cameramounts':
            L['mounts'].append(w)
        elif sec == 'cameraanimations':
            if cur_anim is None:
                cur_anim = {'name': w, 'nodes': []}
                L['anims'].append(cur_anim)
            elif wl == 'end':
                cur_anim = None
            else:
                cur_anim['nodes'].append(toks[:3])
        elif sec == 'instantunits':
            t = toks
            L['instant'].append({'type': w, 'team': inum(t[1]) if len(t) > 1 else None,
                                 'x': fnum(t[2]) if len(t) > 2 else None, 'z': fnum(t[3]) if len(t) > 3 else None,
                                 'count': inum(t[4]) if len(t) > 4 else 0,
                                 'inunit': inum(t[5]) if len(t) > 5 else None,
                                 'state': inum(t[6]) if len(t) > 6 else None,
                                 'spread': fnum(t[7]) if len(t) > 7 else None,
                                 'route': inum(t[10]) if len(t) > 10 else -1,
                                 'ntok': len(t)})
        elif sec == 'routes':
            if wl == 'route':
                cur_route = {'id': inum(toks[1]) if len(toks) > 1 else None, 'wps': []}
                L['routes'].append(cur_route)
            elif wl == 'end':
                cur_route = None
            elif cur_route is not None:
                cur_route['wps'].append(inum(w))
        elif sec == 'primaryobjectives':
            L['objectives'].append(" ".join(toks))
        elif sec == 'runningprograms':
            L['running'].append(w)
        elif sec == 'difficulty':
            if wl == 'createdasdifficulty' and len(toks) > 1:
                L['difficulty'] = toks[1]
        elif sec == 'leaderboardstats':
            L['leader'].append(" ".join(toks))
    # --- interpret MultiwiniaOptions like LevelFile::ParseMultiwiniaOptions (last assignment wins)
    o = L['opts']
    R = L['opt_raw']
    def last(k):
        v = R.get(k.lower())
        return v[-1] if v else None
    gt = last('GameTypes')
    o['gametypes_tokens'] = " ".join(gt) if gt else ""
    # LevelFile reads only the first token and strcmp-matches blueprint names
    first = gt[0] if gt else None
    o['gametype_levelfile'] = first if first in GAMETYPES else ("-1(" + first + ")" if first else "-1")
    # GameMenu::AddMap uses strcmp on key "GameTypes" (case-sensitive) and all tokens
    menu_types = []
    for k_raw in ['GameTypes']:
        pass
    o['menu_types'] = [t for t in (gt or []) if t in GAMETYPES]
    pc = R.get('populationcap')
    o['populationcap'] = pc[-1][0] if pc else ""
    o['populationcap_all'] = "|".join(x[0] for x in pc) if pc and len(pc) > 1 else ""
    d = last('DefenderPopulationCap'); o['defenderpopulationcap'] = d[0] if d else ""
    n = last('NumPlayers'); o['numplayers'] = n[0] if n else ""
    eff = []
    for line in R.get('effects', []):
        for e in line:
            if e in EFFECTS:
                eff.append(e)
            else:
                eff.append('?' + e)
    o['effects'] = " ".join(eff)
    lo = []
    for line in R.get('leveloptions', []):
        i = 0
        while i < len(line):
            k = line[i].lower()
            if k in LEVELOPT_ARGS:
                if LEVELOPT_ARGS[k] and i + 1 < len(line):
                    arg = line[i + 1]
                    if k.endswith('reinforcementtype'):
                        if arg.lower() not in [r.lower() for r in RESEARCH_NAMES]:
                            arg = arg + "(=-1:not a GlobalResearch type)"
                    lo.append(f"{line[i]}={arg}")
                    i += 2
                    continue
                lo.append(line[i])
            else:
                lo.append('?' + line[i])
            i += 1
    o['leveloptions'] = " ".join(lo)
    ic = []
    for line in R.get('invalidcrates', []):
        for c in line:
            ic.append(c if c.lower() in [x.lower() for x in CRATE_NAMES] else '?' + c)
    o['invalidcrates'] = " ".join(ic)
    p = last('PlayTime'); o['playtime'] = p[0] if p else ""
    df = last('Difficulty'); o['difficulty'] = df[0] if df else ""
    at = []
    for line in R.get('attackingteams', []):
        at += line
    o['attackingteams'] = " ".join(at)
    o['coopmode'] = 'Y' if 'coopmode' in R else ''
    o['forcecoop'] = 'Y' if 'forcecoop' in R else ''
    g1 = []; [g1.extend(x) for x in R.get('coopgroup1positions', [])]
    g2 = []; [g2.extend(x) for x in R.get('coopgroup2positions', [])]
    o['coopgroup1'] = " ".join(g1); o['coopgroup2'] = " ".join(g2)
    o['official'] = 'Y' if 'officiallevel' in R else ''
    nm = last('Name'); o['name'] = " ".join(nm) if nm else ""
    au = last('Author'); o['author'] = " ".join(au) if au else ""
    de = last('Description'); o['description'] = " ".join(de) if de else ""
    return L

# ---------------------------------------------------------------- build / packaging model
def load_patterns(path):
    full, short = [], []
    if not os.path.isfile(path):
        return full, short
    for ln in open(path, encoding='latin-1'):
        ln = ln.strip().replace('\\', '/')
        if not ln:
            continue
        if ln.startswith('**/'):
            short.append(ln[3:])
        else:
            full.append(ln)
    return full, short

def excluded(rel, pats):
    full, short = pats
    parts = rel.split('/')
    if any(p in short for p in parts):
        return True
    for pat in full:
        # bsd_glob: '*' does not cross '/'; listfiles.pl compares case-insensitively (/i)
        if len(pat.split('/')) == len(parts) and fnmatch.fnmatch(rel.lower(), pat.lower()):
            return True
        # a pattern naming a directory prunes the whole subtree
        if rel.lower().startswith(pat.lower().rstrip('/') + '/') and '*' not in pat:
            return True
    return False

def load_inclusions(path):
    if not os.path.isfile(path):
        return set()
    return {ln.strip().replace('\\', '/').lower() for ln in open(path, encoding='latin-1') if ln.strip()}

def shipped(rel, excl, incl):
    return (not excluded(rel, excl)) or (rel.lower() in incl)

def parse_mapcrc(path):
    ids = {}
    for line in open(path, encoding='latin-1'):
        m = re.match(r'#define\s+MAPID_(\S+)\s+(0x[0-9a-fA-F]+)', line.strip())
        if m:
            ids[m.group(1)] = int(m.group(2), 16)
    return ids

def parse_permitted(path):
    """Return {target: [MAPID names]} for uncommented PERMITTED_MAPS defines per #ifdef TARGET_ block."""
    res = {}
    target = None
    depth = 0
    for line in open(path, encoding='latin-1'):
        s = line.strip()
        m = re.match(r'#ifdef\s+(TARGET_\w+)', s)
        if m and depth == 0:
            target = m.group(1); depth = 1; continue
        if s.startswith('#if'):
            if target: depth += 1
            continue
        if s.startswith('#endif') and target:
            depth -= 1
            if depth == 0: target = None
            continue
        if target and s.startswith('#define') and 'PERMITTED_MAPS' in s:
            res.setdefault(target, []).extend(re.findall(r'MAPID_(\w+)', s))
    return res

def parse_demo_defaults(path):
    out = []
    for line in open(path, encoding='latin-1'):
        m = re.search(r'm_allowedMapCRCs\[\d+\]\s*=\s*MAPID_(\w+)', line)
        if m: out.append(m.group(1))
    return out

def parse_tutorial(path):
    out = {}
    txt = open(path, encoding='latin-1').read()
    for fn in ('SetupTutorial', 'SetupTutorial2'):
        m = re.search(r'void GameMenuWindow::' + fn + r'\(\)(.*?)\n}', txt, re.S)
        if m:
            ids = re.findall(r'MAPID_(\w+)', m.group(1))
            if ids: out[fn] = ids[0]
    return out

def load_lang(path):
    names = {}
    if not os.path.isfile(path):
        return names
    for ln in open(path, encoding='latin-1'):
        m = re.match(r'^(mp_\S+?)_name\s+(.*)$', ln.rstrip('\r\n'))
        if m:
            names[m.group(1).lower()] = m.group(2).strip()
    return names

def bmp_info(path):
    with open(path, 'rb') as f:
        h = f.read(54)
    if len(h) < 30 or h[:2] != b'BM':
        return ('?', '?', '?')
    w, hh = struct.unpack('<ii', h[18:26])
    bpp = struct.unpack('<H', h[28:30])[0]
    return (w, hh, bpp)

# ---------------------------------------------------------------- main
def main():
    src, out = sys.argv[1], sys.argv[2]
    summary = '--summary' in sys.argv
    lv = os.path.join(src, 'data/levels')
    th = os.path.join(src, 'data/thumbnails')
    ids = parse_mapcrc(os.path.join(src, 'code/mapcrc.h'))
    crc_to_name = {v: k for k, v in ids.items()}
    permitted = parse_permitted(os.path.join(src, 'code/lib/universal_include.h'))
    demo_default = parse_demo_defaults(os.path.join(src, 'code/network/demo_limitations.cpp'))
    tutorial = parse_tutorial(os.path.join(src, 'code/UI/GameMenuWindow.cpp'))
    lang = load_lang(os.path.join(src, 'data/language/english.txt'))
    enum = {}
    ep = os.path.join(lv, 'level_enum.txt')
    if os.path.isfile(ep):
        for ln in open(ep, encoding='latin-1'):
            t = ln.split()
            if len(t) >= 2: enum[t[0].lower()] = t[1]
    T = os.path.join(src, 'tools')
    builds = {
        'retail': (load_patterns(os.path.join(T, 'multiwinia_exclusions.txt')), set()),
        'pcpreview': (load_patterns(os.path.join(T, 'multiwinia_exclusions_beta_test.txt')),
                      load_inclusions(os.path.join(T, 'multiwinia_inclusions_previewcopy.txt'))),
        'hwcompat': (load_patterns(os.path.join(T, 'multiwinia_exclusions_hwcompat.txt')),
                     load_inclusions(os.path.join(T, 'multiwinia_inclusions_hwcompat.txt'))),
        'assaultstress': (load_patterns(os.path.join(T, 'multiwinia_exclusions_hwcompat.txt')),
                          load_inclusions(os.path.join(T, 'multiwinia_inclusions_assaultstresstest.txt'))),
        'beta_a': (load_patterns(os.path.join(T, 'multiwinia_exclusions_beta_test.txt')),
                   load_inclusions(os.path.join(T, 'multiwinia_inclusions_beta_test_group_a.txt'))),
        'beta_b': (load_patterns(os.path.join(T, 'multiwinia_exclusions_beta_test.txt')),
                   load_inclusions(os.path.join(T, 'multiwinia_inclusions_beta_test_group_b.txt'))),
        'beta_c': (load_patterns(os.path.join(T, 'multiwinia_exclusions_beta_test.txt')),
                   load_inclusions(os.path.join(T, 'multiwinia_inclusions_beta_test_group_c.txt'))),
    }
    # references to level files from data/*.txt, data/purity, data/scripts
    ref_files = []
    for d in ['data', 'data/purity', 'data/scripts']:
        p = os.path.join(src, d)
        if os.path.isdir(p):
            for fn in sorted(os.listdir(p)):
                fp = os.path.join(p, fn)
                if os.path.isfile(fp) and fn.endswith('.txt'):
                    ref_files.append((d + '/' + fn, open(fp, encoding='latin-1').read().lower()))
    thumbs = {}
    if os.path.isdir(th):
        for fn in sorted(os.listdir(th)):
            thumbs[fn.lower()] = fn

    files = []
    for d in ['', 'old']:
        p = os.path.join(lv, d)
        for fn in sorted(os.listdir(p), key=str.lower):
            fp = os.path.join(p, fn)
            if not os.path.isfile(fp) or not fn.endswith('.txt') or fn == 'level_enum.txt':
                continue
            files.append(('data/levels/' + (d + '/' if d else '') + fn, fp, d, fn))

    rows = []
    brows = []
    parsed = {}
    for rel, fp, d, fn in files:
        L = parse_level(fp)
        parsed[rel] = L
        stem = fn[:-4]
        kind = 'mp' if fn.startswith('mp_') else ('map' if fn.startswith('map_') else ('mission' if fn.startswith('mission_') else 'other'))
        o = L['opts']
        r = collections.OrderedDict()
        r['file'] = rel
        r['dir'] = 'levels/old' if d else 'levels'
        r['kind'] = kind
        r['size_bytes'] = L['size']
        r['crc'] = '0x%08x' % L['crc']
        key = stem.upper()
        if d == '':
            exp = ids.get(key)
            r['mapid_define'] = ('MAPID_' + key) if exp is not None else ''
            r['mapid_check'] = '' if exp is None else ('OK' if exp == L['crc'] else 'MISMATCH')
        else:
            nm = crc_to_name.get(L['crc'])
            r['mapid_define'] = ('crc==MAPID_' + nm) if nm else ''
            r['mapid_check'] = 'n/a(old)'
        r['level_enum_id'] = enum.get(fn.lower(), '') if d == '' else ''
        r['english_name'] = lang.get(stem.lower(), '')
        r['sections'] = " ".join(L['sections'])
        r['unknown_sections'] = " ".join(L['unknown_sections'])
        # pairing
        if kind == 'map':
            loc = stem[4:]
            r['pairing'] = " ".join(sorted(x[3] for x in files if x[3].startswith('mission_' + loc) and x[2] == d))
        elif kind == 'mission':
            cands = [x[3] for x in files if x[3].startswith('map_') and stem.startswith('mission_' + x[3][4:-4]) and x[2] == d]
            # longest match (pattern_buffer vs others)
            cands.sort(key=len, reverse=True)
            r['pairing'] = cands[0] if cands else ''
        else:
            r['pairing'] = 'single-file (mission=null)'
        refs = [n for n, txt in ref_files if fn.lower() in txt]
        r['referenced_by'] = " ".join(refs)
        # options
        for k in ['name', 'gametypes_tokens', 'gametype_levelfile']:
            r[k] = o.get(k, '')
        r['menu_types'] = " ".join(o.get('menu_types', []))
        r['menu_listed_dir'] = 'Y' if (d == '' and kind == 'mp' and o.get('menu_types')) else ''
        for k in ['numplayers', 'populationcap', 'populationcap_all', 'defenderpopulationcap', 'effects',
                  'leveloptions', 'invalidcrates', 'playtime', 'difficulty', 'attackingteams', 'coopmode',
                  'forcecoop', 'coopgroup1', 'coopgroup2', 'official', 'author', 'description']:
            r[k] = o.get(k, '')
        r['unknown_option_keys'] = " ".join(L['unknown_opt_keys'])
        # landscape
        la = L['land']
        r['worldSizeX'] = la.get('worldsizex', '')
        r['worldSizeZ'] = la.get('worldsizez', '')
        r['cellSize'] = la.get('cellsize', '')
        try:
            import math
            r['grid_cells'] = "%dx%d" % (math.ceil(float(r['worldSizeX']) / float(r['cellSize'])),
                                         math.ceil(float(r['worldSizeZ']) / float(r['cellSize'])))
        except Exception:
            r['grid_cells'] = ''
        r['outsideHeight'] = la.get('outsideheight', '')
        r['maxHeight'] = la.get('maxheight', '')
        r['landColourFile'] = la.get('landcolourfile', '')
        r['wavesColourFile'] = la.get('wavescolourfile', '')
        r['waterColourFile'] = la.get('watercolourfile', '')
        tl = L['tiles']
        r['tiles'] = len(tl) if 'landscapetiles' in L['sections'] else ''
        r['tile_size'] = rng([t['size'] for t in tl])
        r['tile_fracdim'] = rng([t['frac'] for t in tl])
        r['tile_heightscale'] = rng([t['hscale'] for t in tl])
        r['tile_desiredheight'] = rng([t['desired'] for t in tl])
        r['tile_posY'] = rng([t['y'] for t in tl])
        r['tile_genmethods'] = " ".join(str(x) for x in sorted({t['gen'] for t in tl if t['gen'] is not None}))
        r['tile_lowland'] = rng([t['lowland'] for t in tl])
        r['tile_guidepower'] = " ".join(str(x) for x in sorted({t['gpow'] for t in tl if t['gpow'] is not None}))
        fl = L['flatten']
        r['flatten_areas'] = len(fl) if 'landflattenareas' in L['sections'] else ''
        r['flatten_types'] = teamstr(collections.Counter(f['type'] for f in fl))
        r['flatten_size'] = rng([f['size'] for f in fl])
        r['lights'] = len(L['lights']) if 'lights' in L['sections'] else ''
        r['light_colours'] = " | ".join("(%g,%g,%g)" % tuple(x[3:6]) for x in L['lights'] if len(x) >= 6 and None not in x)
        # buildings
        B = L['buildings']
        tc = collections.Counter(b['type'] for b in B)
        r['buildings_total'] = len(B)
        r['building_blocks'] = len([s for s in L['sections'] if s == 'buildings'])
        r['buildings_in_2nd_block'] = sum(1 for b in B if b['block'] >= 2)
        r['building_types'] = len(tc)
        r['blocked_buildings_FULL'] = sum(v for k, v in tc.items() if k in BLOCKED)
        r['unknown_building_types'] = " ".join(sorted(k for k in tc if k not in BUILDING_NAMES))
        def by_team(tp):
            return teamstr(collections.Counter(b['team'] for b in B if b['type'] == tp))
        r['spawnpoints'] = tc.get('SpawnPoint', 0)
        r['spawnpoints_by_team'] = by_team('SpawnPoint')
        teams_b = {b['team'] for b in B if b['team'] is not None and b['team'] not in (255, -1)}
        teams_i = {u['team'] for u in L['instant'] if u['team'] is not None and u['team'] not in (255, -1)}
        r['team_positions_used'] = " ".join(str(x) for x in sorted(teams_b | teams_i))
        r['spawnpoplocks'] = tc.get('SpawnPopulationLock', 0)
        r['spawnpoplock_radius_maxpop'] = " ".join(sorted({f"{b['extra'][0]}/{b['extra'][1]}" for b in B
                                                          if b['type'] == 'SpawnPopulationLock' and len(b['extra']) >= 2}))
        r['spawnlinks'] = tc.get('SpawnLink', 0)
        r['spawnpointmasters'] = tc.get('SpawnPointMaster', 0)
        r['trunkports_by_team'] = by_team('TrunkPort')
        r['mwzones'] = tc.get('MultiwiniaZone', 0)
        r['mwzones_by_team'] = by_team('MultiwiniaZone')
        r['mwzone_sizes'] = " ".join(sorted({b['extra'][0] for b in B if b['type'] == 'MultiwiniaZone' and b['extra']}))
        r['mwzones_with_links'] = sum(1 for b in B if b['type'] == 'MultiwiniaZone' and len(b['extra']) > 1)
        r['escaperockets_by_team'] = by_team('EscapeRocket')
        r['fuelstations_by_team'] = by_team('FuelStation')
        r['pulsebombs'] = tc.get('PulseBomb', 0)
        r['pulsebomb_starttime'] = " ".join(b['extra'][0] for b in B if b['type'] == 'PulseBomb' and b['extra'])
        r['pulsebomb_links'] = " ".join(str(len(b['extra']) - 1) for b in B if b['type'] == 'PulseBomb' and b['extra'])
        r['controlstations_by_team'] = by_team('ControlStation')
        r['gunturrets_by_team'] = by_team('GunTurret')
        r['radardishes'] = tc.get('RadarDish', 0)
        r['aitargets'] = tc.get('AITarget', 0)
        r['aiobjectives'] = tc.get('AIObjective', 0)
        r['aiobjectivemarkers'] = tc.get('AIObjectiveMarker', 0)
        r['restrictionzones'] = tc.get('RestrictionZone', 0)
        r['walls'] = tc.get('Wall', 0)
        r['laserfences'] = tc.get('LaserFence', 0)
        r['jumppads'] = tc.get('JumpPad', 0)
        r['trees'] = tc.get('Tree', 0)
        r['staticshapes'] = " ".join(sorted({b['extra'][1] for b in B if b['type'] == 'StaticShape' and len(b['extra']) > 1}))
        # camera
        r['camera_mounts'] = len(L['mounts']) if 'cameramounts' in L['sections'] else ''
        r['camera_mount_names'] = " ".join(L['mounts'])
        r['camera_anims'] = len(L['anims']) if 'cameraanimations' in L['sections'] else ''
        r['camera_anim_names'] = " ".join(f"{a['name']}({len(a['nodes'])})" for a in L['anims'])
        # instant units
        IU = L['instant']
        r['instantunit_lines'] = len(IU)
        r['instantunit_total'] = sum((u['count'] or 0) for u in IU)
        agg = collections.Counter()
        for u in IU:
            agg[(u['type'], u['team'])] += (u['count'] or 0)
        r['instantunits'] = " ".join(f"{t}@{tm}x{c}" + ("[blocked]" if t in ENTITY_BLOCKED else "")
                                     for (t, tm), c in sorted(agg.items(), key=lambda kv: (kv[0][0], kv[0][1] if kv[0][1] is not None else -9)))
        r['instantunits_on_routes'] = sum(1 for u in IU if u['route'] not in (-1, None))
        r['routes'] = len(L['routes'])
        r['route_waypoints'] = sum(len(x['wps']) for x in L['routes'])
        r['primary_objectives'] = " || ".join(L['objectives'])
        r['running_programs'] = len(L['running'])
        r['created_as_difficulty'] = L['difficulty'] or ''
        r['leaderboard_stats'] = " | ".join(L['leader'])
        # thumbnail (code/UI/MapImageButton.h: thumbnails/<lowercased stem>.bmp)
        tkey = (stem + '.bmp').lower()
        r['thumbnail'] = ('data/thumbnails/' + thumbs[tkey]) if (kind == 'mp' and d == '' and tkey in thumbs) else ''
        # packaging / permission
        relData = rel[len('data/'):]
        for bname, (ex, inc) in builds.items():
            r['ships_' + bname] = 'Y' if shipped(relData, ex, inc) else ''
        mid = key if d == '' else None
        for tgt in ['TARGET_PC_PREVIEW', 'TARGET_PC_HARDWARECOMPAT', 'TARGET_ASSAULT_STRESSTEST',
                    'TARGET_BETATEST_GROUP_A', 'TARGET_BETATEST_GROUP_B', 'TARGET_BETATEST_GROUP_C']:
            lst = permitted.get(tgt)
            r['permitted_' + tgt.replace('TARGET_', '').lower()] = ('Y' if mid and mid in lst else '') if lst else 'all'
        r['permitted_pc_fullgame'] = 'all' if 'TARGET_PC_FULLGAME' not in permitted else ''
        r['demo_default_allowed'] = 'Y' if mid and mid in demo_default else ''
        r['tutorial'] = " ".join(k for k, v in tutorial.items() if mid and v == mid)
        rows.append(r)
        # building rows
        for t, c in sorted(tc.items()):
            brows.append({'file': rel, 'building_type': t, 'count': c,
                          'by_team': teamstr(collections.Counter(b['team'] for b in B if b['type'] == t)),
                          'known_type': 'Y' if t in BUILDING_NAMES else 'N(skipped by CreateBuilding)',
                          'blocked_in_FULL': 'Y' if t in BLOCKED else '',
                          'forced_global': 'Y' if t in FORCED_GLOBAL else '',
                          'non_global_in_file': sum(1 for b in B if b['type'] == t and t in ASSERT_GLOBAL and b['global'] == 0),
                          'in_2nd_block': sum(1 for b in B if b['type'] == t and b['block'] >= 2)})

    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, 'X4_levels.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        for r in rows: w.writerow(r)
    with open(os.path.join(out, 'X4_level_buildings.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(brows[0].keys()))
        w.writeheader()
        for r in brows: w.writerow(r)
    # thumbnails
    level_stems = {x[3][:-4].lower(): x[0] for x in files if x[2] == ''}
    old_stems = {x[3][:-4].lower(): x[0] for x in files if x[2] == 'old'}
    trows = []
    for lk, fn in sorted(thumbs.items()):
        stem = lk[:-4]
        wdt, hgt, bpp = bmp_info(os.path.join(th, fn))
        lvl = level_stems.get(stem, '')
        note = ''
        if stem == 'default':
            note = 'fallback (MapImageButton) when thumbnails/<stem>.bmp missing'
        elif not lvl:
            note = ('level only in data/levels/old: ' + old_stems[stem]) if stem in old_stems else 'no level file in tree'
        relData = 'thumbnails/' + fn
        trows.append({'file': 'data/thumbnails/' + fn, 'size_bytes': os.path.getsize(os.path.join(th, fn)),
                      'width': wdt, 'height': hgt, 'bpp': bpp, 'level_file': lvl, 'note': note,
                      **{('ships_' + b): ('Y' if shipped(relData, ex, inc) else '') for b, (ex, inc) in builds.items()}})
    with open(os.path.join(out, 'X4_thumbnails.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(trows[0].keys()))
        w.writeheader()
        for r in trows: w.writerow(r)
    # per-mode menu lists (retail data set). GameMenu::CreateMapList: names come lower-cased from main.dat,
    # base order ~alphabetical (OrderedInsert), then stable bubble sort by NumPlayers, then difficulty rank.
    def drank(dv):
        r = 0
        if 'basic' in dv: r = 1
        if 'inter' in dv: r = 2
        if 'advan' in dv: r = 3
        return r
    mrows = []
    for gi, g in enumerate(GAMETYPES):
        cand = [r for r in rows if r['dir'] == 'levels' and r['kind'] == 'mp' and g in r['menu_types'].split()]
        for scope in ['all_in_tree', 'retail']:
            lst = [r for r in cand if scope == 'all_in_tree' or r['ships_retail'] == 'Y']
            lst.sort(key=lambda r: os.path.basename(r['file']).lower())
            lst.sort(key=lambda r: (inum(r['numplayers']) or 0, drank(r['difficulty'])))
            for i, r in enumerate(lst):
                mrows.append({'gametype_index': gi, 'gametype': g, 'mode_in_FULL': 'Y' if g in FULL_MODES else 'N(INCLUDEGAMEMODE_TANKBATTLE off)',
                              'scope': scope, 'menu_index': i, 'file': r['file'], 'english_name': r['english_name'],
                              'numplayers': r['numplayers'], 'difficulty': r['difficulty'], 'playtime': r['playtime']})
    with open(os.path.join(out, 'X4_mode_maplists.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(mrows[0].keys()))
        w.writeheader()
        for r in mrows: w.writerow(r)
    if summary:
        print('levels:', len(rows), 'building rows:', len(brows), 'thumbs:', len(trows), 'mode rows:', len(mrows))
        print('permitted:', permitted)
        print('demo default:', demo_default, 'tutorial:', tutorial)
        print('mapid mismatches:', [r['file'] for r in rows if r['mapid_check'] == 'MISMATCH'])
        print('no mapid (top-level):', [r['file'] for r in rows if r['dir'] == 'levels' and not r['mapid_define']])

if __name__ == '__main__':
    main()
