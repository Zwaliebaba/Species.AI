#!/usr/bin/env python3
"""X1a: survey of every shipped level file (map_*, mission_*, mp_*, old/*, purity/*).

Usage: python3 -I X1a_level_survey.py SRC_ROOT OUT_CSV OUT_ANOMALIES

Mirrors the tokeniser of contrib/systemiv/lib/filesys/text_stream_readers.cpp (comments '#',
first-CR truncation, separators ' \\t\\r\\n:,') and the block dispatch of code/level_file.cpp
(ParseMapFile / ParseMissionFile and every Parse* block reader) to count, per block and per field,
how many files of each file-set use it, and to flag lines the real parser would mis-handle.

File sets: mp (data/levels/mp_*.txt), map (data/levels/map_*.txt), mission (data/levels/mission_*.txt),
old (data/levels/old/*.txt, all mp_ maps), purity (data/purity/map_* and mission_*).
"""
import collections, csv, glob, os, sys

SEPS = set(' \t\r\n:,')

def read_lines(path):
    data = open(path, 'rb').read()
    if data[:2] == b'\xff\xfe':
        text = data[2:].decode('utf-16-le')
    elif data[:2] == b'\xfe\xff':
        text = data[2:].decode('utf-16-be')
    else:
        text = data.decode('latin-1')
    raw = text.split('\n')
    if text.endswith('\n'):
        raw = raw[:-1]
    out = []
    for n, l in enumerate(raw, 1):
        c = l
        i = c.find('#')
        if i >= 0: c = c[:i]
        i = c.find('\r')
        if i >= 0: c = c[:i]
        out.append((n, l, c))
    return out

def tokens(c):
    toks, cur = [], ''
    for ch in c:
        if ch in SEPS:
            if cur: toks.append(cur); cur = ''
        else:
            cur += ch
    if cur: toks.append(cur)
    return toks

def rest_of_line(c, ntok):
    """GetRestOfLine after ntok tokens were consumed (approximation: after the ntok-th token + 1 sep)."""
    i, k = 0, 0
    L = len(c)
    while k < ntok:
        while i < L and c[i] in SEPS: i += 1
        while i < L and c[i] not in SEPS: i += 1
        if i < L: i += 1   # the separator that GetNextToken overwrote
        k += 1
    while i < L and c[i] in SEPS: i += 1
    return c[i:]

# ---- code knowledge (from code/worldobject/building.cpp, entity.cpp and every ::Read) ----
BUILDING_NAMES = ["Invalid","Factory","Cave","RadarDish","LaserFence","ControlTower","GunTurret","Bridge",
 "Powerstation","Tree","Wall","TrunkPort","ResearchItem","Library","Generator","Pylon","PylonStart","PylonEnd",
 "SolarPanel","PowerSplitter","TrackLink","TrackJunction","TrackStart","TrackEnd","Refinery","Mine","Yard",
 "DisplayScreen","UpgradePort","PrimaryUpgrade","Incubator","AntHill","SafeArea","Triffid","SpiritReceiver",
 "ReceiverLink","SpiritSpawner","SpiritProcessor","SpawnPoint","SpawnPopulationLock","SpawnPointMaster",
 "SpawnLink","AITarget","AISpawnPoint","BlueprintStore","BlueprintConsole","BlueprintRelay","ScriptTrigger",
 "Spam","GodDish","StaticShape","FuelGenerator","FuelPipe","FuelStation","EscapeRocket","FenceSwitch",
 "DynamicHub","DynamicNode","FeedingTube","MultiwiniaZone","ChessPiece","ChessBase","CloneLab","ControlStation",
 "Portal","Crate","Statue","WallBuster","PulseBomb","RestrictionZone","JumpPad","AIObjective","AIObjectiveMarker",
 "EruptionMarker","SmokeMarker"]
BLOCKED_BUILDINGS = {"BlueprintConsole","BlueprintRelay","BlueprintStore","Bridge","Cave","ChessBase","ChessPiece",
 "Crate","DisplayScreen","DynamicHub","DynamicNode","Factory","FenceSwitch","Generator","GodDish","Library","Mine",
 "Portal","Powerstation","PrimaryUpgrade","ReceiverLink","SpiritSpawner","Refinery","ResearchItem","SafeArea",
 "ScriptTrigger","Spam","SpiritProcessor","SpiritReceiver","Statue","TrackEnd","TrackJunction","TrackLink",
 "TrackStart","UpgradePort","Yard","PylonStart","FeedingTube","AISpawnPoint"}
# (required extra tokens after the 7 base tokens, max extra tokens or None = unbounded list)
BUILDING_EXTRA = {
 "AITarget":(0,3),"AISpawnPoint":(4,6),"AIObjective":(1,1),"AIObjectiveMarker":(2,4),"AntHill":(1,1),
 "BlueprintStore":(1,1),"BlueprintConsole":(2,2),"BlueprintRelay":(2,2),"Bridge":(2,2),"ChessBase":(2,2),
 "CloneLab":(0,1),"ControlStation":(1,1),"ControlTower":(1,1),"Factory":(1,1),"FeedingTube":(1,1),
 "Generator":(1,1),"Pylon":(1,1),"PylonEnd":(1,1),"SolarPanel":(1,1),"PylonStart":(2,2),"PowerSplitter":(1,None),
 "DynamicHub":(5,5),"DynamicNode":(4,4),"GunTurret":(0,5),"Incubator":(0,1),"JumpPad":(2,2),"LaserFence":(1,3),
 "Mine":(1,1),"Refinery":(1,1),"TrackLink":(1,1),"TrackJunction":(1,None),"TrackStart":(2,2),"TrackEnd":(2,2),
 "MultiwiniaZone":(1,None),"Powerstation":(1,1),"PulseBomb":(1,None),"RadarDish":(0,None),"ResearchItem":(1,2),
 "RestrictionZone":(1,2),"FuelGenerator":(1,1),"FuelPipe":(1,1),"FuelStation":(1,1),"EscapeRocket":(5,5),
 "SafeArea":(3,3),"ScriptTrigger":(4,4),"SpawnPoint":(0,None),"SpawnLink":(0,None),"SpawnPointMaster":(0,None),
 "SpawnPopulationLock":(2,2),"SpiritReceiver":(1,1),"ReceiverLink":(1,1),"SpiritSpawner":(1,1),
 "SpiritProcessor":(1,1),"StaticShape":(2,2),"FenceSwitch":(5,6),"Tree":(8,11),"Triffid":(9,19),
 "TrunkPort":(0,1),"Wall":(0,1),
}
for n in BUILDING_NAMES[1:]:
    BUILDING_EXTRA.setdefault(n, (0, 0))   # base Building::Read only

ENTITY_NAMES = ["InvalidType","LaserTroop","Engineer","Virii","Squadie","Egg","SporeGenerator","Lander","Tripod",
 "Centipede","SpaceInvader","Spider","Darwinian","Officer","ArmyAnt","Armour","SoulDestroyer","TriffidEgg","AI",
 "Shaman","Harvester","Nuke","Tentacle","Meteor","Tank","SpaceShip","DropShip"]
BLOCKED_ENTITIES = {"AI","ArmyAnt","DropShip","Egg","Lander","LaserTroop","Meteor","Nuke","Officer","Shaman",
 "SpaceInvader","Tentacle","TriffidEgg","Tripod","InvalidType","SpaceShip"}

MW_OPTION_KEYS = ["PopulationCap","DefenderPopulationCap","Effects","GameTypes","NumPlayers","LevelOptions","Name",
 "InvalidCrates","Author","Description","PlayTime","Difficulty","AttackingTeams","CoopMode","ForceCoop",
 "CoopGroup1Positions","CoopGroup2Positions","OfficialLevel"]
LEVEL_OPTIONS = {"nohandicap":0,"instantspawnpointcapture":0,"trunkportreinforcements":1,"trunkportarmour":1,
 "nogunturrets":0,"attackerreinforcements":1,"defenderreinforcements":1,"defenderreinforcementtype":1,
 "attackerreinforcementtype":1,"forceradarteammatch":0,"noradarcontrol":0,"airstrikecapturezones":1,
 "futurewinianteam":1,"attackreinforcementoption":1,"defendreinforcementoption":1}
EFFECTS_CS = ["snow","dustwind","soulrain","lightning","SpecialLighting"]
GAMETYPES_CS = ["Domination","KingOfTheHill","CaptureTheStatue","Assault","RocketRiot","Blitzkrieg","TankBattle"]
LANDSCAPE_KEYS = ["cellSize","worldSizeX","worldSizeZ","outsideHeight","maxHeight","landColourFile",
 "wavesColourFile","waterColourFile"]
RESEARCH_NAMES = ["Darwinian","Officer","Squad","Laser","Grenade","Rocket","Controller","AirStrike","Armour",
 "TaskManager","Engineer","Harvester","Nuke","Subversion","PersonalShield","HotFeet","GunTurret","Shaman",
 "Infection","MagicalForest","DarkForest","AntNest","Plague","Egg","MeteorShower","FireExtinguisher","Rage",
 "Tank","DropShip"]
CONDITIONS = ["AlwaysTrue","BuildingOnline","BuildingOffline","ResearchOwned","NotInLocation","DebugKey","NeverTrue"]
CRATE_NAMES = None  # filled from crate.cpp by caller if available

MAP_BLOCKS = ["MultiwiniaOptions","Landscape","LandscapeTiles","LandFlattenAreas","Buildings","Lights",
 "CameraMounts","CameraAnimations","InstantUnits","Routes","PrimaryObjectives"]
MISSION_BLOCKS = ["CameraMounts","CameraAnimations","Buildings","InstantUnits","Routes","PrimaryObjectives",
 "RunningPrograms","Difficulty","LeaderboardStats"]
MISSION_ASSERT_BLOCKS = ["Landscape","LandscapeTiles","LandFlattenAreas","Lights"]


class Survey:
    def __init__(self):
        # key (block, field, detail) -> {set -> set(files)}, occurrences, token counts
        self.files = collections.defaultdict(lambda: collections.defaultdict(set))
        self.occ = collections.Counter()
        self.ntok = collections.defaultdict(list)
        self.status = {}
        self.anom = []

    def rec(self, fset, fname, block, field, detail='', status='parsed', ntok=None):
        k = (block, field, detail)
        self.files[k][fset].add(fname)
        self.occ[k] += 1
        if ntok is not None: self.ntok[k].append(ntok)
        self.status.setdefault(k, status)

    def A(self, fname, ln, msg):
        self.anom.append(f'{fname}:{ln}: {msg}')


def survey_file(S, path, fset, kind, fname):
    lines = read_lines(path)
    i = 0
    N = len(lines)
    blocks_seen = []

    def block_body(endname, allow_blank=True):
        nonlocal i
        body = []
        while i < N:
            ln, raw, c = lines[i]; i += 1
            t = tokens(c)
            if not t:
                body.append((ln, raw, c, t))
                continue
            if t[0].lower() == endname.lower():
                return body, True
            body.append((ln, raw, c, t))
        return body, False

    while i < N:
        ln, raw, c = lines[i]; i += 1
        t = tokens(c)
        if not t: continue
        w = t[0]
        wl = w.lower()
        if not wl.endswith('_startdefinition'):
            S.A(fname, ln, f'top-level token {w!r} is not a *_StartDefinition (AppDebugAssert(0) in debug; ignored in release)')
            S.rec(fset, fname, '(top-level)', w, status='unknown-top-level')
            continue
        bname = w[:-len('_StartDefinition')]
        blocks_seen.append(bname)
        canon = None
        valid = MAP_BLOCKS if kind == 'map' else MISSION_BLOCKS
        for b in valid:
            if b.lower() == bname.lower(): canon = b
        if canon is None:
            st = 'asserts-in-mission' if (kind == 'mission' and bname.lower() in [x.lower() for x in MISSION_ASSERT_BLOCKS]) else 'block-not-parsed-for-this-file-kind'
            S.A(fname, ln, f'block {w} not dispatched for a {kind} file ({st}); its lines are re-read as top-level tokens')
            S.rec(fset, fname, bname, '(block)', status=st)
            continue
        S.rec(fset, fname, canon, '(block)')
        endname = canon + '_EndDefinition'
        if canon == 'CameraAnimations':
            # nested: name line then nodes until End
            while i < N:
                ln2, raw2, c2 = lines[i]; i += 1
                t2 = tokens(c2)
                if not t2: continue
                if t2[0].lower() == endname.lower(): break
                S.rec(fset, fname, canon, 'animation', status='parsed', ntok=len(t2))
                while i < N:
                    ln3, raw3, c3 = lines[i]; i += 1
                    t3 = tokens(c3)
                    if not t3: continue
                    if t3[0].lower() == 'end': break
                    S.rec(fset, fname, canon, 'node:' + t3[0], ntok=len(t3))
                    if len(t3) < 3: S.A(fname, ln3, 'camera anim node with <3 tokens (CRASH)')
            continue
        if canon == 'Routes':
            while i < N:
                ln2, raw2, c2 = lines[i]; i += 1
                t2 = tokens(c2)
                if not t2: continue
                if t2[0].lower() == endname.lower(): break
                if t2[0].lower() == 'route':
                    S.rec(fset, fname, canon, 'Route', ntok=len(t2))
                    while i < N:
                        ln3, raw3, c3 = lines[i]; i += 1
                        t3 = tokens(c3)
                        if not t3: continue
                        if t3[0].lower() == 'end': break
                        wpn = {'0': 'waypoint:0(Type3DPos)', '1': 'waypoint:1(TypeGroundPos)', '2': 'waypoint:2(TypeBuilding)'}.get(t3[0], 'waypoint:' + t3[0])
                        S.rec(fset, fname, canon, wpn, ntok=len(t3))
                else:
                    S.rec(fset, fname, canon, t2[0], status='ignored-line')
            continue
        body, closed = block_body(endname)
        if not closed:
            S.A(fname, ln, f'block {w} not closed before EOF')
        for (ln2, raw2, c2, t2) in body:
            if not t2:
                if canon in ('Landscape', 'PrimaryObjectives') and raw2.strip(' \t\r') != '':
                    S.A(fname, ln2, f'comment-only line inside {canon} block: GetNextToken()==NULL then stricmp(NULL) (CRASH in parser)')
                elif canon in ('Landscape', 'PrimaryObjectives'):
                    S.A(fname, ln2, f'blank line inside {canon} block: GetNextToken()==NULL then stricmp(NULL) (CRASH in parser)')
                continue
            f0 = t2[0]
            if canon == 'MultiwiniaOptions':
                key = None
                for k in MW_OPTION_KEYS:
                    if k.lower() == f0.lower(): key = k
                if key is None:
                    S.rec(fset, fname, canon, f0, status='ignored-unknown-key', ntok=len(t2))
                    continue
                S.rec(fset, fname, canon, key, ntok=len(t2))
                if f0 != key:
                    S.A(fname, ln2, f'MultiwiniaOptions key {f0!r} differs in case from {key!r}: LevelFile accepts (stricmp) but GameMenu::AddMap uses strcmp -> ignored by the menu scanner')
                if key == 'LevelOptions':
                    j = 1
                    while j < len(t2):
                        o = t2[j]; j += 1
                        if o.lower() in LEVEL_OPTIONS:
                            arg = LEVEL_OPTIONS[o.lower()]
                            val = t2[j] if arg and j < len(t2) else ''
                            if arg and j >= len(t2): S.A(fname, ln2, f'LevelOptions {o} missing argument (CRASH)')
                            j += arg
                            if o.lower() in ('defenderreinforcementtype', 'attackerreinforcementtype') and val and val.lower() not in [r.lower() for r in RESEARCH_NAMES]:
                                S.A(fname, ln2, f'{o} {val!r} is not a GlobalResearch type name -> -1')
                            S.rec(fset, fname, canon, 'LevelOptions.' + o.lower(), detail=(val if arg else ''))
                        else:
                            S.rec(fset, fname, canon, 'LevelOptions.' + o, status='ignored-unknown-option')
                elif key == 'Effects':
                    for e in t2[1:]:
                        st = 'parsed' if e in EFFECTS_CS else 'ignored-unknown-effect(case-sensitive strcmp)'
                        S.rec(fset, fname, canon, 'Effects.' + e, status=st)
                elif key == 'GameTypes':
                    for gi, g in enumerate(t2[1:]):
                        st = 'parsed' if g in GAMETYPES_CS else 'ignored-unknown-gametype(case-sensitive strcmp)'
                        if gi > 0 and st == 'parsed': st = 'menu-only(LevelFile keeps first GameTypes token only)'
                        S.rec(fset, fname, canon, 'GameTypes.' + g, status=st)
                elif key == 'Difficulty':
                    S.rec(fset, fname, canon, 'Difficulty.value', detail=(t2[1] if len(t2) > 1 else ''))
                elif key == 'InvalidCrates':
                    for cr in t2[1:]:
                        st = 'parsed'
                        if CRATE_NAMES is not None and cr.lower() not in [x.lower() for x in CRATE_NAMES]:
                            st = 'ignored-unknown-crate-name'
                            S.A(fname, ln2, f'InvalidCrates {cr!r} does not match any Crate::GetName -> ignored')
                        S.rec(fset, fname, canon, 'InvalidCrates.' + cr, status=st)
                elif key in ('PopulationCap', 'DefenderPopulationCap', 'NumPlayers', 'PlayTime'):
                    S.rec(fset, fname, canon, key + '.value', detail=(t2[1] if len(t2) > 1 else 'MISSING'))
                elif key == 'AttackingTeams':
                    S.rec(fset, fname, canon, 'AttackingTeams.value', detail=' '.join(t2[1:]))
                elif key in ('CoopGroup1Positions', 'CoopGroup2Positions'):
                    S.rec(fset, fname, canon, key + '.value', detail=' '.join(t2[1:]))
            elif canon == 'Landscape':
                key = None
                for k in LANDSCAPE_KEYS:
                    if k.lower() == f0.lower(): key = k
                if key is None:
                    S.rec(fset, fname, canon, f0, status='ignored-unknown-key', ntok=len(t2))
                else:
                    S.rec(fset, fname, canon, key, ntok=len(t2))
                    if len(t2) < 2: S.A(fname, ln2, f'Landscape {key} without value (secondWord NULL -> CRASH)')
            elif canon == 'LandscapeTiles':
                guide = int(t2[10]) if len(t2) > 10 and t2[10].lstrip('-').isdigit() else None
                S.rec(fset, fname, canon, 'tile', detail='guidePower=%s' % guide, ntok=len(t2))
                need = 11 + (1 if guide and guide > 0 else 0)
                if len(t2) < need: S.A(fname, ln2, f'tile row has {len(t2)} tokens < {need} (CRASH)')
                if len(t2) > need: S.A(fname, ln2, f'tile row has {len(t2)} tokens > {need} (extra ignored)')
            elif canon == 'LandFlattenAreas':
                S.rec(fset, fname, canon, 'area', detail='%d tokens' % len(t2), ntok=len(t2))
                if len(t2) not in (4, 6): S.A(fname, ln2, f'flatten row has {len(t2)} tokens (expected 4 or 6)')
            elif canon == 'Lights':
                S.rec(fset, fname, canon, 'light', ntok=len(t2))
                if len(t2) < 6: S.A(fname, ln2, f'light row has {len(t2)} tokens < 6 (CRASH)')
            elif canon == 'CameraMounts':
                S.rec(fset, fname, canon, 'mount', ntok=len(t2))
                if len(t2) < 10: S.A(fname, ln2, f'camera mount row has {len(t2)} tokens < 10 (CRASH)')
            elif canon == 'Buildings':
                name = None
                for bn in BUILDING_NAMES:
                    if bn.lower() == f0.lower(): name = bn
                if name is None:
                    S.rec(fset, fname, canon, f0, status='ignored-unknown-type', ntok=len(t2))
                    S.A(fname, ln2, f'unknown building type {f0!r} (CreateBuilding -> NULL, line ignored)')
                    continue
                extra = len(t2) - 8
                st = 'blocked(BLOCK_OLD_DARWINIA_OBJECTS: line ignored)' if name in BLOCKED_BUILDINGS else 'parsed'
                if name == 'Invalid': st = 'ignored(Invalid)'
                S.rec(fset, fname, canon, name, detail=('dynamic' if kind == 'mission' else 'static'), status=st, ntok=len(t2))
                if st == 'parsed':
                    lo, hi = BUILDING_EXTRA[name]
                    if extra < lo: S.A(fname, ln2, f'{name} has {extra} extra tokens < required {lo} (CRASH)')
                    if extra < 0: S.A(fname, ln2, f'{name} has fewer than the 7 base tokens (CRASH)')
                    if hi is not None and extra > hi: S.A(fname, ln2, f'{name} has {extra} extra tokens > max {hi} (surplus ignored)')
                    if name == 'Tree' and len(t2) >= 16:
                        for col, idx in (('branchColour', 14), ('leafColour', 15)):
                            try:
                                v = int(t2[idx])
                                if v > 2147483647 or v < -2147483648:
                                    S.A(fname, ln2, f'Tree {col} {v} outside int32 (atoi overflow; MSVC atoi saturates [I])')
                            except ValueError:
                                pass
            elif canon == 'InstantUnits':
                name = None
                for en in ENTITY_NAMES:
                    if en.lower() == f0.lower(): name = en
                if name is None:
                    S.rec(fset, fname, canon, f0, status='ignored-unknown-type', ntok=len(t2))
                    continue
                st = 'blocked(EntityIsBlocked: line skipped)' if name in BLOCKED_ENTITIES else 'parsed'
                if name == 'Squadie' and len(t2) > 5 and t2[5] in ('0',):
                    st = 'skipped(InsertionSquadie with inAUnit=0)'
                S.rec(fset, fname, canon, name, status=st, ntok=len(t2))
                if st == 'parsed' and len(t2) < 8: S.A(fname, ln2, f'InstantUnit {name} has {len(t2)} tokens < 8 (CRASH)')
            elif canon == 'PrimaryObjectives':
                cond = None
                for cn in CONDITIONS:
                    if cn.lower() == f0.lower(): cond = cn
                S.rec(fset, fname, canon, cond or f0, status='parsed' if cond else 'unknown-condition', ntok=len(t2))
            elif canon == 'RunningPrograms':
                S.rec(fset, fname, canon, f0, ntok=len(t2))
            elif canon == 'Difficulty':
                S.rec(fset, fname, canon, f0, status='parsed' if f0.lower() == 'createdasdifficulty' else 'ignored', ntok=len(t2))
            elif canon == 'LeaderboardStats':
                S.rec(fset, fname, canon, f0, ntok=len(t2))
    return blocks_seen


def main():
    global CRATE_NAMES
    root, outcsv, outanom = sys.argv[1], sys.argv[2], sys.argv[3]
    if len(sys.argv) > 4:
        CRATE_NAMES = [x.strip() for x in open(sys.argv[4]).read().split('\n') if x.strip()]
    sets = [
        ('mp', 'map', sorted(glob.glob(os.path.join(root, 'data/levels/mp_*.txt')))),
        ('map', 'map', sorted(glob.glob(os.path.join(root, 'data/levels/map_*.txt')))),
        ('mission', 'mission', sorted(glob.glob(os.path.join(root, 'data/levels/mission_*.txt')))),
        ('old', 'map', sorted(glob.glob(os.path.join(root, 'data/levels/old/*.txt')))),
        ('purity', 'map', sorted(glob.glob(os.path.join(root, 'data/purity/map_*.txt')))),
        ('purity', 'mission', sorted(glob.glob(os.path.join(root, 'data/purity/mission_*.txt')))),
    ]
    S = Survey()
    nfiles = collections.Counter()
    order = collections.Counter()
    for fset, kind, files in sets:
        for p in files:
            rel = os.path.relpath(p, root)
            nfiles[fset] += 1
            seen = survey_file(S, p, fset, kind, rel)
            order[(fset, ' > '.join(seen))] += 1
    cols = ['mp', 'map', 'mission', 'old', 'purity']
    with open(outcsv, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['block', 'field', 'detail', 'status', 'n_files_total'] + ['n_' + c for c in cols] +
                   ['occurrences', 'min_tokens', 'max_tokens', 'example_file'])
        w.writerow(['(files)', '(files in set)', '', '', sum(nfiles.values())] + [nfiles[c] for c in cols] + ['', '', '', ''])
        for k in sorted(S.files, key=lambda k: (k[0], k[1], k[2])):
            per = S.files[k]
            allf = set().union(*per.values())
            nt = S.ntok.get(k, [])
            ex = sorted(allf)[0]
            w.writerow([k[0], k[1], k[2], S.status[k], len(allf)] + [len(per.get(c, ())) for c in cols] +
                       [S.occ[k], min(nt) if nt else '', max(nt) if nt else '', ex])
    with open(outanom, 'w') as f:
        f.write('# block order per file set (count)\n')
        for (fs, o), n in sorted(order.items()):
            f.write(f'{fs}\t{n}\t{o}\n')
        f.write('\n# anomalies\n')
        for a in S.anom:
            f.write(a + '\n')
    print('files per set:', dict(nfiles))
    print('rows:', len(S.files), 'anomalies:', len(S.anom))

if __name__ == '__main__':
    main()
