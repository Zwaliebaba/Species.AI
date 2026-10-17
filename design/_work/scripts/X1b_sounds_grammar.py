#!/usr/bin/env python3
"""X1b: validate data/sounds.txt against the state machine of
code/sound/soundsystem.cpp:LoadBlueprints/ParseSoundEvent/ParseSoundEffect/
ParseSampleGroup and code/sound/sound_parameter.cpp:SoundParameter::Read.
Also checks sample resolution (sounds/<n>.ogg then mwsounds/<n>.ogg) and the
packaging lists tools/sounds.txt and tools/menu_sounds.txt.
Usage: python3 -I X1b_sounds_grammar.py <multiwinia_root>"""
import sys, os, re, collections

SEP = re.compile(r'[ \t\n\r:,]+')
ENTITY = ["InvalidType", "LaserTroop", "Engineer", "Virii", "Squadie", "Egg", "SporeGenerator", "Lander", "Tripod",
          "Centipede", "SpaceInvader", "Spider", "Darwinian", "Officer", "ArmyAnt", "Armour", "SoulDestroyer",
          "TriffidEgg", "AI", "Shaman", "Harvester", "Nuke", "Tentacle", "Meteor", "Tank", "SpaceShip", "DropShip"]
OTHER = ["Laser", "Grenade", "Rocket", "AirStrikeBomb", "Spirit", "Sepulveda", "Gesture", "Ambience", "Music",
         "Interface", "CratePowerup", "MultiwiniaInterface"]
PTYPES = {"typefixedvalue": 1, "typerangedrandom": 3, "typelinked": 5}
LINKS = ["nothing", "heightaboveground", "xpos", "ypos", "zpos", "velocity", "cameradistance"]
UPD = ["updateconstantly", "updateoncePerloop".lower()]
FIELDS = ["SOUNDNAME", "SOURCETYPE", "POSITIONTYPE", "INSTANCETYPE", "LOOPTYPE", "TEAMMATCHTYPE", "PRIORITY",
          "MINDISTANCE", "VOLUME", "FREQUENCY", "ATTACK", "SUSTAIN", "RELEASE", "LOOPDELAY", "EFFECT"]

def toks(line):
    line = line.split('#', 1)[0]
    return [t for t in SEP.split(line) if t]

def building_names(root):
    src = open(os.path.join(root, 'code/worldobject/building.cpp'), encoding='latin-1').read()
    i = src.index('char *Building::GetTypeName')
    j = src.index('};', i)
    return re.findall(r'"([^"]+)"', src[i:j])

def effects(root):
    eff = []
    cur = None
    for ln in open(os.path.join(root, 'data/effects.txt'), encoding='latin-1'):
        t = toks(ln)
        if not t:
            continue
        if t[0].upper() == 'EFFECT':
            cur = [t[1], []]; eff.append(cur)
        elif t[0].upper() == 'END':
            cur = None
        elif cur is not None:
            cur[1].append((t[0], t[1], t[2], t[3], t[4]))
    return eff

def main(root):
    BUILD = building_names(root)
    EFF = effects(root)
    effnames = {e[0].lower(): e for e in EFF}
    lines = open(os.path.join(root, 'data/sounds.txt'), encoding='latin-1').read().split('\n')
    hazards = []
    stats = collections.Counter()
    fields = collections.Counter()
    ptype = collections.Counter()
    links = collections.Counter()
    upd = collections.Counter()
    upd_absent = 0
    effuse = collections.Counter()
    eff_nparams = collections.Counter()
    eff_param_name_mismatch = 0
    samplegroup_first = collections.Counter()
    groups = {}
    soundnames = []
    unknown_sources = []
    dup_sources = collections.Counter()
    events_per_source = collections.Counter()
    ints = collections.defaultdict(collections.Counter)
    i = 0
    n = len(lines)
    def nxt():
        nonlocal i
        i += 1
        return toks(lines[i - 1]) if i - 1 < n else None
    def read_param(t, where):
        nonlocal upd_absent
        if len(t) < 3 or t[1].upper() != 'PARAMETER':
            hazards.append(f'{where}: missing PARAMETER keyword: {t}')
            return
        pt = t[2].lower()
        ptype[t[2]] += 1
        k = PTYPES.get(pt)
        if k is None:
            hazards.append(f'{where}: unknown parameter type {t[2]}')
            return
        rest = t[3:]
        nums = rest[:k]
        if len(nums) < k:
            hazards.append(f'{where}: too few values {t}')
        rest = rest[k:]
        if pt == 'typelinked':
            if not rest:
                hazards.append(f'{where}: missing link name'); return
            links[rest[0]] += 1
            if rest[0].lower() not in LINKS:
                hazards.append(f'{where}: unknown link {rest[0]}')
            rest = rest[1:]
        if rest:
            upd[rest[0]] += 1
            if rest[0].lower() not in UPD:
                hazards.append(f'{where}: unknown update type {rest[0]}')
            if len(rest) > 1:
                hazards.append(f'{where}: extra tokens ignored {rest[1:]}')
        else:
            upd_absent += 1
    while i < n:
        t = nxt()
        if not t:
            continue
        g = t[0].upper()
        typ = t[1] if len(t) > 1 else None
        if g in ('ENTITY', 'BUILDING', 'OTHER'):
            stats[g] += 1
            names = {'ENTITY': ENTITY, 'BUILDING': BUILD, 'OTHER': OTHER}[g]
            if typ is None or typ.lower() not in [x.lower() for x in names]:
                unknown_sources.append(f'{g} {typ} (line {i})')
            dup_sources[(g, (typ or '').lower())] += 1
            # source block: loop ReadLine until END
            while i < n:
                t2 = nxt()
                if not t2:
                    continue
                w = t2[0].upper()
                if w == 'END':
                    break
                if w != 'EVENT':
                    hazards.append(f'line {i}: non-EVENT line inside source block treated as EVENT: {t2}')
                stats['EVENT'] += 1
                events_per_source[(g, typ)] += 1
                ev = t2[1] if len(t2) > 1 else None
                where0 = f'{g} {typ} EVENT {ev}'
                # event body: ReadLine + GetNextToken until END; blank line => NULL token => crash
                while True:
                    if i >= n:
                        hazards.append(f'{where0}: EOF inside EVENT'); break
                    t3 = nxt()
                    if not t3:
                        hazards.append(f'line {i}: BLANK/comment-only line inside EVENT ({where0}) -> NULL token'); continue
                    f = t3[0].upper()
                    if f == 'END':
                        break
                    fields[f] += 1
                    if f not in FIELDS:
                        hazards.append(f'line {i}: unknown field {f} in {where0}')
                    if f == 'SOUNDNAME':
                        soundnames.append((t3[1] if len(t3) > 1 else None, where0, i))
                    elif f in ('SOURCETYPE', 'POSITIONTYPE', 'INSTANCETYPE', 'LOOPTYPE', 'TEAMMATCHTYPE', 'PRIORITY'):
                        ints[f][t3[1]] += 1
                    elif f in ('VOLUME', 'FREQUENCY', 'ATTACK', 'SUSTAIN', 'RELEASE', 'LOOPDELAY'):
                        read_param(t3, f'line {i} {where0} {f}')
                    elif f == 'EFFECT':
                        en = t3[1] if len(t3) > 1 else None
                        effuse[en] += 1
                        if en is None or en.lower() not in effnames:
                            hazards.append(f'line {i}: unknown effect {en}')
                        bp = effnames.get((en or '').lower())
                        k = 0
                        while True:
                            t4 = nxt()
                            if not t4:
                                hazards.append(f'line {i}: BLANK line inside EFFECT -> NULL token'); continue
                            if t4[0].upper() == 'END':
                                break
                            if bp and k < len(bp[1]) and bp[1][k][0].lower() != t4[0].lower():
                                eff_param_name_mismatch += 1
                            read_param(t4, f'line {i} {where0} EFFECT {en} param {k}')
                            k += 1
                        eff_nparams[(en, k, len(bp[1]) if bp else None)] += 1
                        if k > 13:
                            hazards.append(f'line {i}: more than MAX_PARAMS=13 effect params')
        elif g == 'SAMPLEGROUP':
            stats['SAMPLEGROUP'] += 1
            members = []
            while True:
                t2 = nxt()
                if t2 is None:
                    hazards.append('EOF in SAMPLEGROUP'); break
                if not t2:
                    hazards.append(f'line {i}: BLANK line inside SAMPLEGROUP {typ} -> NULL token'); continue
                if t2[0].upper() == 'END':
                    break
                samplegroup_first[t2[0]] += 1
                members.append(t2[1].lower().rsplit('.', 1)[0] if len(t2) > 1 else None)
            if typ.lower() in groups:
                hazards.append(f'duplicate SAMPLEGROUP {typ}')
            groups[typ.lower()] = members
        else:
            stats['IGNORED_TOPLEVEL:' + g] += 1
    # sample resolution
    have = set()
    for d in ('sounds', 'mwsounds'):
        for f in os.listdir(os.path.join(root, 'data', d)):
            have.add((d, f.lower()))
    def resolve(name):
        if ('sounds', name + '.ogg') in have:
            return 'sounds'
        if ('mwsounds', name + '.ogg') in have:
            return 'mwsounds'
        return None
    missing_samples = []
    unresolved_groups = []
    st_map = {}
    referenced_files = set()
    for nm, where, ln in soundnames:
        stem = nm.lower().rsplit('.', 1)[0] if nm else None
        st_map.setdefault(where, []).append(stem)
    # source type per event isn't tracked per soundname order; check both interpretations
    for nm, where, ln in soundnames:
        stem = nm.lower().rsplit('.', 1)[0]
        if stem in groups:
            for m in groups[stem]:
                r = resolve(m)
                if r:
                    referenced_files.add(f'{r}/{m}.ogg')
                else:
                    missing_samples.append(f'group {stem} member {m}')
        else:
            r = resolve(stem)
            if r:
                referenced_files.add(f'{r}/{stem}.ogg')
            else:
                missing_samples.append(f'{where}: {stem} (line {ln})')
    for g, mem in groups.items():
        for m in mem:
            r = resolve(m)
            if r:
                referenced_files.add(f'{r}/{m}.ogg')
    print('top-level blocks:', dict(stats))
    print('fields:', dict(fields))
    for k, v in ints.items():
        print('  ', k, dict(v))
    print('param types:', dict(ptype))
    print('links:', dict(links))
    print('update types:', dict(upd), 'absent:', upd_absent)
    print('effect uses:', dict(effuse))
    print('effect param counts (name, given, blueprint):', dict(eff_nparams))
    print('effect param-name mismatches vs effects.txt order:', eff_param_name_mismatch)
    print('SAMPLEGROUP member first tokens:', dict(samplegroup_first))
    print('groups:', len(groups), 'members total:', sum(len(m) for m in groups.values()))
    print('unknown sources:', unknown_sources)
    print('duplicate source blocks:', [k for k, v in dup_sources.items() if v > 1])
    print('missing samples (%d):' % len(missing_samples))
    for m in missing_samples[:40]:
        print('   ', m)
    print('HAZARDS (%d):' % len(hazards))
    for h in hazards[:60]:
        print('   ', h)
    # packaging lists
    pk = {}
    for lst in ('sounds.txt', 'menu_sounds.txt'):
        ents = [l.strip() for l in open(os.path.join(root, 'tools', lst), encoding='latin-1') if l.strip()]
        pk[lst] = ents
        dirs = collections.Counter(e.split('/')[0] for e in ents)
        miss = [e for e in ents if (e.split('/')[0], e.split('/')[-1].lower()) not in have]
        print(f'tools/{lst}: {len(ents)} entries, dirs {dict(dirs)}, missing on disk {len(miss)} {miss[:10]}')
    all_ogg = {f'{d}/{f}' for d, f in have if f.endswith('.ogg')}
    in_lists = {e.lower() for e in pk['sounds.txt'] + pk['menu_sounds.txt']}
    print('ogg files on disk:', len(all_ogg), 'in sounds.txt+menu_sounds.txt:', len(in_lists & all_ogg),
          'remaining -> sounds2.dat candidates:', len(all_ogg - in_lists))
    print('referenced by data/sounds.txt:', len(referenced_files), 'referenced but not in tools lists:',
          len(referenced_files - in_lists))
    print('menu_sounds entries:', pk['menu_sounds.txt'])

if __name__ == '__main__':
    main(sys.argv[1])
