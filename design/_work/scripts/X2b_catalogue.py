"""X2b: Multiwinia audio asset catalogue.

Usage:
  python3 -I X2b_catalogue.py <SRC multiwinia root> <OUT _work dir> [<notes.py>]

Reads (as data only, never executed):
  data/sounds/*.ogg, data/mwsounds/*.ogg   -> Ogg/Vorbis identification + comment headers, last granule
  data/sounds.txt                          -> blueprint parse (independent re-implementation of
                                              code/sound/soundsystem.cpp:LoadBlueprints/ParseSoundEvent/
                                              ParseSampleGroup), cross-checked against P16_sound_events.csv
  tools/sounds.txt, tools/menu_sounds.txt, tools/multiwinia_exclusions.txt
                                           -> archive membership (tools/build_multiwinia_data.bat logic)
  code/**/*.cpp|h                          -> Trigger*Event call sites (with comment / #if evaluation)
  OUT/_work/manifest.csv                   -> vcproj build membership of each code file
Writes:
  OUT/_work/drafts/X2b_samples.csv, X2b_sound_events.csv, X2b_callsites.csv, X2b_summary.txt,
  X2b_callsite_context.txt (review aid), X2b_md_*.md (generated markdown tables for the draft)
The optional notes.py is a plain dict literal file (parsed with ast.literal_eval, not imported).
"""
import ast
import collections
import csv
import fnmatch
import os
import re
import struct
import sys

# --------------------------------------------------------------------------------------------
# Baseline preprocessor model (build_matrix.md: Release|Win32 + TARGET_PC_FULLGAME)
# --------------------------------------------------------------------------------------------
BASELINE_DEFINES = {
    'TARGET_PC_FULLGAME': 1, 'WIN32': 1, 'TARGET_MSVC': 1, '_CRT_SECURE_NO_DEPRECATE': 1, 'NDEBUG': 1,
    'LAN_PLAY_ENABLED': 1, 'WAN_PLAY_ENABLED': 1, 'NETWORK_STATS_ENABLED': 1,
    'INCLUDEGAMEMODE_DOMINATION': 1, 'INCLUDEGAMEMODE_KOTH': 1, 'INCLUDEGAMEMODE_CTS': 1,
    'INCLUDEGAMEMODE_ROCKET': 1, 'INCLUDEGAMEMODE_ASSAULT': 1, 'INCLUDEGAMEMODE_BLITZ': 1,
    'INCLUDE_CRATES_BASIC': 1, 'INCLUDE_CRATES_ADVANCED': 1, 'INCLUDE_TUTORIAL': 1,
    'AUTHENTICATION_LEVEL': 1, 'BLOCK_OLD_DARWINIA_OBJECTS': 1, 'DUMP_DEBUG_LOG': 1,
    'DEBUG_RENDER_ENABLED': 1, 'FLOAT_NUMERICS': 1, 'USE_CRASHREPORTING': 1, 'HAVE_DSOUND': 1,
    'USE_DISPLAY_LISTS': 1, 'HAVE_REFRESH_RATES': 1, 'USE_PIXEL_EFFECT_GRID_OPTIMISATION': 1,
    'STRETCH_RECT': 1, '_MSC_VER': 1400,
}

OTHER_TYPES = ['Laser', 'Grenade', 'Rocket', 'AirStrikeBomb', 'Spirit', 'Sepulveda', 'Gesture', 'Ambience',
               'Music', 'Interface', 'CratePowerup', 'MultiwiniaInterface']
OTHER_ENUM = {'TypeLaser': 'Laser', 'TypeGrenade': 'Grenade', 'TypeRocket': 'Rocket',
              'TypeAirstrikeBomb': 'AirStrikeBomb', 'TypeSpirit': 'Spirit', 'TypeSepulveda': 'Sepulveda',
              'TypeGesture': 'Gesture', 'TypeAmbience': 'Ambience', 'TypeMusic': 'Music',
              'TypeInterface': 'Interface', 'TypeCratePowerup': 'CratePowerup',
              'TypeMultiwiniaInterface': 'MultiwiniaInterface'}

POS = {'0': '2D', '1': 'Stat', '2': 'Att', '3': 'Ed'}
LOOP = {'0': 'Once', '1': 'Loop', '2': 'LoopADSR'}
INST = {'0': 'Poly', '1': 'MonoRandom', '2': 'MonoNearest'}
TEAM = {'0': 'All', '1': 'MyTeam', '2': 'Enemy'}
SRCT = {'0': 'Sample', '1': 'Group'}


# --------------------------------------------------------------------------------------------
# Ogg / Vorbis header reader
# --------------------------------------------------------------------------------------------
def read_ogg(path):
    data = open(path, 'rb').read()
    res = {'bytes': len(data), 'ok': False}
    pos = 0
    packets = []
    cur = b''
    last_granule = None
    first_granule_audio = None
    pages = 0
    serials = set()
    while pos + 27 <= len(data):
        if data[pos:pos + 4] != b'OggS':
            nxt = data.find(b'OggS', pos + 1)
            if nxt < 0:
                break
            res.setdefault('resync', 0)
            res['resync'] += 1
            pos = nxt
            continue
        htype = data[pos + 5]
        granule = struct.unpack('<q', data[pos + 6:pos + 14])[0]
        serial = struct.unpack('<I', data[pos + 14:pos + 18])[0]
        serials.add(serial)
        nseg = data[pos + 26]
        segtab = data[pos + 27:pos + 27 + nseg]
        body = pos + 27 + nseg
        off = body
        for s in segtab:
            if len(packets) < 3:
                cur += data[off:off + s]
                if s < 255:
                    packets.append(cur)
                    cur = b''
            off += s
        if granule != -1:
            last_granule = granule
        pages += 1
        pos = off
    res['pages'] = pages
    res['streams'] = len(serials)
    if not packets or not packets[0].startswith(b'\x01vorbis'):
        res['error'] = 'no vorbis identification header'
        return res
    idh = packets[0]
    res['vorbis_version'] = struct.unpack('<I', idh[7:11])[0]
    res['channels'] = idh[11]
    res['rate'] = struct.unpack('<I', idh[12:16])[0]
    res['br_max'], res['br_nom'], res['br_min'] = struct.unpack('<iii', idh[16:28])
    res['blocksizes'] = (1 << (idh[28] & 15), 1 << (idh[28] >> 4))
    res['vendor'] = ''
    res['comments'] = []
    if len(packets) > 1 and packets[1].startswith(b'\x03vorbis'):
        c = packets[1]
        p = 7
        try:
            vl = struct.unpack('<I', c[p:p + 4])[0]; p += 4
            res['vendor'] = c[p:p + vl].decode('utf-8', 'replace'); p += vl
            n = struct.unpack('<I', c[p:p + 4])[0]; p += 4
            for _ in range(n):
                ln = struct.unpack('<I', c[p:p + 4])[0]; p += 4
                res['comments'].append(c[p:p + ln].decode('utf-8', 'replace')); p += ln
        except Exception:
            res['comments'].append('<comment header parse error>')
    res['granule'] = last_granule
    if last_granule is not None and res['rate']:
        res['duration'] = last_granule / float(res['rate'])
    res['ok'] = True
    return res


# --------------------------------------------------------------------------------------------
# sounds.txt parser (mirrors SoundSystem::LoadBlueprints semantics)
# --------------------------------------------------------------------------------------------
def remove_ext(name):
    # code/... RemoveExtension: strip from the LAST '.'
    i = name.rfind('.')
    return name[:i] if i >= 0 else name


def parse_sounds_txt(path):
    lines = open(path, encoding='latin-1').read().splitlines()
    sources = collections.OrderedDict()   # (group,type) -> [event dict]
    groups = collections.OrderedDict()    # name -> [samples]   (first definition wins for lookup)
    group_defs = collections.Counter()
    events = []
    i = 0

    def toks(s):
        return s.replace('\t', ' ').split()

    while i < len(lines):
        t = toks(lines[i]); i += 1
        if not t or t[0].startswith('#'):
            continue
        k = t[0].upper()
        if k in ('ENTITY', 'BUILDING', 'OTHER'):
            key = (k, t[1])
            sources.setdefault(key, [])
            while i < len(lines):
                tt = toks(lines[i]); i += 1
                if not tt:
                    continue
                if tt[0].upper() == 'END':
                    break
                if tt[0].upper() != 'EVENT':
                    continue
                ev = {'group': k, 'type': t[1], 'event': tt[1], 'params': collections.OrderedDict(),
                      'effects': [], 'line': i, 'fields': {}}
                while i < len(lines):
                    ft = toks(lines[i]); i += 1
                    if not ft:
                        continue
                    f = ft[0].upper()
                    if f == 'END':
                        break
                    if f == 'SOUNDNAME':
                        ev['raw_soundname'] = ft[1]
                        ev['soundname'] = remove_ext(ft[1].lower())
                    elif f in ('SOURCETYPE', 'POSITIONTYPE', 'INSTANCETYPE', 'LOOPTYPE', 'TEAMMATCHTYPE',
                               'MINDISTANCE', 'PRIORITY'):
                        ev['fields'][f] = ft[1]
                    elif f in ('VOLUME', 'FREQUENCY', 'ATTACK', 'SUSTAIN', 'RELEASE', 'LOOPDELAY'):
                        ev['params'][f] = ft[2:]
                    elif f == 'EFFECT':
                        eff = [ft[1], []]
                        while i < len(lines):
                            et = toks(lines[i]); i += 1
                            if not et:
                                continue
                            if et[0].upper() == 'END':
                                break
                            eff[1].append(et)
                        ev['effects'].append(eff)
                    else:
                        ev.setdefault('unknown', []).append(ft)
                sources[key].append(ev)
                events.append(ev)
        elif k == 'SAMPLEGROUP':
            name = t[1]
            group_defs[name.lower()] += 1
            samples = []
            while i < len(lines):
                tt = toks(lines[i]); i += 1
                if not tt:
                    continue
                if tt[0].upper() == 'END':
                    break
                samples.append(remove_ext(tt[1].lower()) if len(tt) > 1 else '')
            if name.lower() not in groups:
                groups[name.lower()] = {'name': name, 'samples': samples}
            else:
                groups[name.lower() + '#dup%d' % group_defs[name.lower()]] = {'name': name, 'samples': samples}
    return sources, groups, events, group_defs


def param_summary(p):
    if not p:
        return ''
    t = p[0]
    upd = '/once' if p[-1] == 'UpdateOncePerLoop' else ''
    if t == 'TypeFixedValue':
        return '%s%s' % (p[1], upd)
    if t == 'TypeRangedRandom':
        sm = ' s%s' % p[3] if float(p[3]) != 0 else ''
        return 'rnd %s..%s%s%s' % (p[1], p[2], sm, upd)
    if t == 'TypeLinked':
        sm = ' s%s' % p[5] if float(p[5]) != 0 else ''
        return '%s %s..%s->%s..%s%s%s' % (p[6], p[1], p[3], p[2], p[4], sm, upd)
    return ' '.join(p)


def event_summary(ev):
    f = ev['fields']
    parts = [POS.get(f.get('POSITIONTYPE', '2'), '?'), LOOP.get(f.get('LOOPTYPE', '0'), '?')]
    if f.get('INSTANCETYPE', '0') != '0':
        parts.append(INST[f['INSTANCETYPE']])
    if f.get('TEAMMATCHTYPE', '0') != '0':
        parts.append(TEAM[f['TEAMMATCHTYPE']])
    s = '/'.join(parts)
    s += ' md%s' % f.get('MINDISTANCE', '100')
    for k, lab in (('VOLUME', 'v'), ('FREQUENCY', 'f'), ('ATTACK', 'a'), ('SUSTAIN', 's'), ('RELEASE', 'r'),
                   ('LOOPDELAY', 'ld')):
        if k in ev['params']:
            s += ' %s=%s' % (lab, param_summary(ev['params'][k]))
    if ev['effects']:
        s += ' fx=' + '+'.join(e[0] for e in ev['effects'])
    return s


# --------------------------------------------------------------------------------------------
# Preprocessor / comment aware code scanner
# --------------------------------------------------------------------------------------------
def eval_pp(expr):
    e = expr.split('//')[0]
    e = re.sub(r'/\*.*?\*/', ' ', e)
    e = re.sub(r'defined\s*\(\s*(\w+)\s*\)', lambda m: ' 1 ' if m.group(1) in BASELINE_DEFINES else ' 0 ', e)
    e = re.sub(r'defined\s+(\w+)', lambda m: ' 1 ' if m.group(1) in BASELINE_DEFINES else ' 0 ', e)

    def ident(m):
        w = m.group(0)
        if w.isdigit():
            return w
        return ' %s ' % BASELINE_DEFINES.get(w, 0)
    e = re.sub(r'\b[A-Za-z_]\w*\b', ident, e)
    e = e.replace('&&', ' and ').replace('||', ' or ')
    e = re.sub(r'!(?!=)', ' not ', e)
    e = e.replace('\\', ' ')
    try:
        return bool(eval(e, {'__builtins__': {}}, {}))
    except Exception:
        return False


def scan_code_file(path):
    """Yield (lineno, code_text_without_comments, raw, active, ppstack_desc) for each line."""
    raw_lines = open(path, encoding='latin-1').read().split('\n')
    in_block = False
    stack = []  # list of [cond_text, this_branch_active, any_taken, parent_active]
    out = []
    for n, raw in enumerate(raw_lines, 1):
        # strip comments
        s = raw
        res = ''
        j = 0
        in_str = None
        while j < len(s):
            if in_block:
                k = s.find('*/', j)
                if k < 0:
                    j = len(s)
                    break
                in_block = False
                j = k + 2
                continue
            c = s[j]
            if in_str:
                res += c
                if c == '\\' and j + 1 < len(s):
                    res += s[j + 1]; j += 2; continue
                if c == in_str:
                    in_str = None
                j += 1
                continue
            if s.startswith('//', j):
                break
            if s.startswith('/*', j):
                in_block = True
                j += 2
                continue
            if c in '"\'':
                in_str = c
            res += c
            j += 1
        code = res
        st = code.strip()
        active = all(fr[1] for fr in stack)
        m = re.match(r'#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', st)
        if m:
            d, rest = m.group(1), m.group(2).strip()
            if d in ('ifdef', 'ifndef', 'if'):
                if d == 'ifdef':
                    v = rest.split()[0] in BASELINE_DEFINES if rest else False
                    desc = rest.split()[0] if rest else ''
                elif d == 'ifndef':
                    v = rest.split()[0] not in BASELINE_DEFINES if rest else True
                    desc = '!' + (rest.split()[0] if rest else '')
                else:
                    v = eval_pp(rest)
                    desc = rest
                stack.append([desc, v, v])
            elif d == 'elif' and stack:
                fr = stack[-1]
                v = (not fr[2]) and eval_pp(rest)
                fr[0] = 'elif ' + rest
                fr[1] = v
                fr[2] = fr[2] or v
            elif d == 'else' and stack:
                fr = stack[-1]
                fr[0] = 'else(' + fr[0] + ')'
                fr[1] = not fr[2]
                fr[2] = True
            elif d == 'endif' and stack:
                stack.pop()
        desc = ' && '.join(fr[0] for fr in stack if not fr[0].startswith('!_') and not fr[0].startswith('!INCLUDED')
                           and not fr[0].startswith('!_included'))
        out.append((n, code, raw, active, desc))
    return out


FUNC_RE = re.compile(r'^[A-Za-z_][\w\s\*&:<>,]*?\b((?:\w+::)+~?\w+)\s*\(')
FREE_FUNC_RE = re.compile(r'^(?:static\s+|inline\s+)*(?:const\s+)?[A-Za-z_][\w\*&<>]*(?:\s+[\*&]*[A-Za-z_][\w\*&<>]*)*?\s+[\*&]*(\w+)\s*\(')
KEYWORDS = {'if', 'while', 'for', 'switch', 'return', 'else', 'case', 'sizeof', 'delete', 'new', 'do'}
CLASS_RE = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*(?::\s*(?:public|protected|private)?\s*(\w+))?')
CALL_RE = re.compile(r'(Trigger(?:Entity|Building|Other)Event)\s*\(')
HDR_METHOD_RE = re.compile(r'^\s+(?:virtual\s+|static\s+|inline\s+)*[\w\*&<>:]+\s+[\*&]?(\w+)\s*\([^;]*$')


def split_args(s):
    """split the argument list starting right after '(' ; returns list of args (top level)."""
    depth = 0
    cur = ''
    args = []
    in_str = None
    for c in s:
        if in_str:
            cur += c
            if c == in_str:
                in_str = None
            continue
        if c in '"\'':
            in_str = c
            cur += c
            continue
        if c == '(':
            depth += 1
        elif c == ')':
            if depth == 0:
                args.append(cur.strip())
                return args, True
            depth -= 1
        if c == ',' and depth == 0:
            args.append(cur.strip())
            cur = ''
            continue
        cur += c
    args.append(cur.strip())
    return args, False


def find_callsites(src, files, vcproj):
    sites = []
    for rel in files:
        path = os.path.join(src, rel)
        scanned = scan_code_file(path)
        func = ''
        cls = ''
        hdr_method = ''
        for idx, (n, code, raw, active, desc) in enumerate(scanned):
            if not raw.startswith((' ', '\t')):
                fm = FUNC_RE.match(code)
                if fm and not code.rstrip().endswith(';'):
                    func = fm.group(1)
                else:
                    ff = FREE_FUNC_RE.match(code)
                    if ff and not code.rstrip().endswith(';') and ff.group(1) not in KEYWORDS and not code.startswith('#'):
                        func = ff.group(1)
            cm = CLASS_RE.match(code)
            if cm and not code.rstrip().endswith(';'):
                cls = cm.group(1)
            hm = HDR_METHOD_RE.match(code)
            if hm and rel.endswith('.h') and hm.group(1) not in KEYWORDS and not re.match(r'^\s+(?:else|return|delete|new)\b', code):
                hdr_method = hm.group(1)
            for m in CALL_RE.finditer(raw):
                fn = m.group(1)
                # is this occurrence inside a comment?
                commented = m.group(0) not in code
                # gather args possibly over several lines
                tail = raw[m.end():]
                k = idx
                args, closed = split_args(tail)
                while not closed and k + 1 < len(scanned) and k < idx + 4:
                    k += 1
                    tail += ' ' + scanned[k][2]
                    args, closed = split_args(tail)
                ctx_from = max(0, idx - 12)
                context = [scanned[x][2] for x in range(ctx_from, min(len(scanned), idx + 4))]
                f = func if rel.endswith('.cpp') else ('%s::%s' % (cls, hdr_method) if cls else hdr_method)
                if rel.endswith('.h') and func and not cls:
                    f = func
                sites.append({
                    'file': rel, 'line': n, 'func': f, 'call': fn, 'args': args, 'commented': commented,
                    'pp_active': active, 'pp': desc, 'vcproj': vcproj.get(rel, ''), 'context': context,
                })
    return sites


# --------------------------------------------------------------------------------------------
# Type tables from code
# --------------------------------------------------------------------------------------------
def read_type_tables(src):
    ent_cpp = open(os.path.join(src, 'code/worldobject/entity.cpp'), encoding='latin-1').read()
    bld_cpp = open(os.path.join(src, 'code/worldobject/building.cpp'), encoding='latin-1').read()
    m = re.search(r'char \*Entity::GetTypeName.*?\{(.*?)\};', ent_cpp, re.S)
    ent_names = re.findall(r'"(\w+)"', m.group(1))
    m = re.search(r'char \*Building::GetTypeName.*?\{(.*?)\};', bld_cpp, re.S)
    bld_names = re.findall(r'"(\w+)"', m.group(1))
    ent_cls = dict(re.findall(r'case Entity::Type(\w+)\s*:\s*entity = new (\w+)', ent_cpp))
    bld_cls = dict(re.findall(r'case Type(\w+)\s*:\s*building = new (\w+)', bld_cpp))
    # entity enum order
    ent_h = open(os.path.join(src, 'code/worldobject/entity.h'), encoding='latin-1').read()
    m = re.search(r'enum\s*\{\s*(TypeInvalid.*?NumEntityTypes)', ent_h, re.S)
    ent_enum = re.findall(r'\b(Type\w+)', m.group(1))
    bld_h = open(os.path.join(src, 'code/worldobject/building.h'), encoding='latin-1').read()
    m = re.search(r'enum\s*\{\s*(TypeInvalid.*?NumBuildingTypes)', bld_h, re.S)
    bld_enum = re.findall(r'\b(Type\w+)', m.group(1))
    name_to_class = {}
    for i, nm in enumerate(ent_names):
        en = ent_enum[i][4:] if i < len(ent_enum) else ''
        name_to_class[('ENTITY', nm)] = ent_cls.get(en, '')
    for i, nm in enumerate(bld_names):
        en = bld_enum[i][4:] if i < len(bld_enum) else ''
        name_to_class[('BUILDING', nm)] = bld_cls.get(en, '')
    m = re.search(r'bool Building::BuildingIsBlocked.*?return\((.*?)\);', bld_cpp, re.S)
    bblocked = set(re.findall(r'Building::Type(\w+)', m.group(1)))
    m = re.search(r'bool Entity::EntityIsBlocked.*?return\((.*?)\);', ent_cpp, re.S)
    eblocked = set(re.findall(r'Entity::Type(\w+)', m.group(1)))
    blocked = set()
    for i, nm in enumerate(ent_names):
        if i < len(ent_enum) and ent_enum[i][4:] in eblocked:
            blocked.add(('ENTITY', nm))
    for i, nm in enumerate(bld_names):
        if i < len(bld_enum) and bld_enum[i][4:] in bblocked:
            blocked.add(('BUILDING', nm))
    return name_to_class, blocked, ent_enum, bld_enum


def class_hierarchy(src):
    parent = {}
    for root, _d, fs in os.walk(os.path.join(src, 'code')):
        for f in fs:
            if not f.endswith('.h'):
                continue
            txt = open(os.path.join(root, f), encoding='latin-1').read()
            for m in re.finditer(r'^\s*class\s+(\w+)\s*:\s*(?:public|protected|private)\s+(\w+)', txt, re.M):
                parent.setdefault(m.group(1), m.group(2))
    return parent


def ancestors(cls, parent):
    out = []
    while cls in parent:
        cls = parent[cls]
        out.append(cls)
    return out


def method_bodies(src, files):
    """(class, method) -> comment-stripped body text, for out-of-line definitions in compiled .cpp files."""
    bodies = {}
    for rel in files:
        if not rel.endswith('.cpp'):
            continue
        txt = open(os.path.join(src, rel), encoding='latin-1').read()
        txt = re.sub(r'/\*.*?\*/', ' ', txt, flags=re.S)
        txt = re.sub(r'//[^\n]*', '', txt)
        for m in re.finditer(r'^[A-Za-z_][^\n;]*?\b(\w+)::(~?\w+)\s*\([^;{]*?\)\s*(?:const\s*)?(?::[^{;]*)?\{', txt, re.M | re.S):
            start = m.end() - 1
            depth = 0
            j = start
            while j < len(txt):
                if txt[j] == '{':
                    depth += 1
                elif txt[j] == '}':
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            bodies.setdefault((m.group(1), m.group(2)), (rel, txt[start:j + 1]))
    return bodies


def base_site_applies(type_cls, site_cls, method, parent, bodies):
    """True if a call inside site_cls::method is reached for objects of type_cls (no non-chaining override)."""
    if type_cls == site_cls:
        return True, ''
    chain = [type_cls] + ancestors(type_cls, parent)
    if site_cls not in chain:
        return False, 'not a subclass'
    for k in chain:
        if k == site_cls:
            return True, ''
        if (k, method) in bodies:
            body = bodies[(k, method)][1]
            if re.search(r'\b\w+::%s\s*\(' % re.escape(method), body):
                continue
            return False, 'overridden by %s::%s without base call' % (k, method)
    return False, 'chain broken'


def list_sound_events(src):
    """class -> list of event names its ListSoundEvents() offers (own PutData + base call)."""
    res = {}
    base = {}
    for root, _d, fs in os.walk(os.path.join(src, 'code')):
        if '/unicode' in root:
            continue
        for f in fs:
            if not f.endswith('.cpp'):
                continue
            txt = open(os.path.join(root, f), encoding='latin-1').read()
            for m in re.finditer(r'void\s+(\w+)::ListSoundEvents\s*\([^)]*\)\s*\{(.*?)\n\}', txt, re.S):
                body = re.sub(r'//[^\n]*', '', m.group(2))
                body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
                res[m.group(1)] = re.findall(r'PutData\s*\(\s*"(\w+)"', body)
                b = re.search(r'(\w+)::ListSoundEvents', body)
                if b:
                    base[m.group(1)] = b.group(1)
    return res, base


# --------------------------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------------------------
def main():
    src, outw = sys.argv[1], sys.argv[2]
    notes = {}
    if len(sys.argv) > 3:
        notes = ast.literal_eval(open(sys.argv[3], encoding='utf-8').read())
    drafts = os.path.join(outw, 'drafts')
    summary = []

    # ---------------- audio files
    files = []
    for folder in ('sounds', 'mwsounds'):
        d = os.path.join(src, 'data', folder)
        for f in sorted(os.listdir(d), key=str.lower):
            files.append((folder, f))
    info = {}
    for folder, f in files:
        info[(folder, f)] = read_ogg(os.path.join(src, 'data', folder, f))

    # ---------------- packaging (tools/build_multiwinia_data.bat)
    def read_list(p):
        return [x.strip().replace('\\', '/') for x in open(os.path.join(src, p), encoding='latin-1').read().splitlines()
                if x.strip()]
    sounds_list = read_list('tools/sounds.txt')
    menu_list = read_list('tools/menu_sounds.txt')
    excl = read_list('tools/multiwinia_exclusions.txt')
    disk_rel = {('%s/%s' % (fo, f)).lower(): (fo, f) for fo, f in files}
    pack = {}
    excluded_by = {}
    for key in disk_rel:
        for pat in excl:
            if pat.startswith('**/'):
                continue
            if fnmatch.fnmatch(key, pat.lower()):
                excluded_by[key] = pat
    sl = set(x.lower() for x in sounds_list)
    ml = set(x.lower() for x in menu_list)
    for key in disk_rel:
        arch = []
        if key in ml:
            arch.append('menusounds.dat')
        if key in sl:
            arch.append('sounds.dat')
        if key not in sl and key not in ml and key not in excluded_by:
            arch.append('sounds2.dat')
        pack[key] = arch
    list_missing = [x for x in sounds_list + menu_list if x.lower() not in disk_rel]
    list_dups = [x for x, c in collections.Counter([y.lower() for y in sounds_list + menu_list]).items() if c > 1]
    summary.append('tools/sounds.txt entries: %d (missing on disk: %s)' % (len(sounds_list), list_missing))
    summary.append('tools/menu_sounds.txt entries: %d' % len(menu_list))
    summary.append('entries in both lists / duplicated: %s' % list_dups)
    summary.append('excluded by multiwinia_exclusions.txt: %s' % sorted(excluded_by.items()))
    for a in ('menusounds.dat', 'sounds.dat', 'sounds2.dat'):
        summary.append('archive %s: %d files' % (a, sum(1 for v in pack.values() if a in v)))
    summary.append('not shipped (excluded): %d' % sum(1 for v in pack.values() if not v))

    # ---------------- sounds.txt
    sources, groups, events, group_defs = parse_sounds_txt(os.path.join(src, 'data/sounds.txt'))
    summary.append('sources: %d (%s)' % (len(sources), dict(collections.Counter(g for g, _t in sources))))
    summary.append('EVENT blocks: %d (%s)' % (len(events), dict(collections.Counter(e['group'] for e in events))))
    summary.append('SAMPLEGROUP definitions: %d, distinct names: %d, duplicate names: %s' % (
        sum(group_defs.values()), len(group_defs), [k for k, v in group_defs.items() if v > 1]))
    summary.append('sample entries in groups: %d' % sum(len(g['samples']) for g in groups.values()))
    # verify against P16 CSV
    p16 = os.path.join(drafts, 'P16_sound_events.csv')
    if os.path.exists(p16):
        rows = list(csv.DictReader(open(p16)))
        mism = []
        for r, ev in zip(rows, events):
            a = (r['group'], r['object_type'], r['event'], r['SOUNDNAME'].lower(), r['LOOPTYPE'], r['POSITIONTYPE'])
            b = (ev['group'], ev['type'], ev['event'], ev.get('raw_soundname', '').lower(),
                 ev['fields'].get('LOOPTYPE', ''), ev['fields'].get('POSITIONTYPE', ''))
            if a != b:
                mism.append((a, b))
        summary.append('P16 cross-check: P16 rows %d vs X2b blocks %d, mismatches %d %s' % (
            len(rows), len(events), len(mism), mism[:5]))

    # sample name resolution
    on_disk = {}
    for fo, f in files:
        stem = remove_ext(f.lower())
        on_disk.setdefault(stem, []).append((fo, f))

    def resolve(name):
        # CachedSample::CachedSample: "sounds/<n>" then "mwsounds/<n>"; Resource::GetSoundStreamDecoder appends .ogg
        for fo in ('sounds', 'mwsounds'):
            for (ffo, f) in on_disk.get(name, []):
                if ffo == fo and f.lower() == name + '.ogg':
                    return (fo, f)
        return None

    # references
    ref_by = collections.defaultdict(set)     # (fo,f) -> set of "GROUP Type Event"
    ref_via = collections.defaultdict(set)    # (fo,f) -> set of group names
    group_users = collections.defaultdict(set)
    missing_refs = []
    event_samples = {}
    for ev in events:
        evname = '%s %s %s' % (ev['group'], ev['type'], ev['event'])
        sn = ev.get('soundname', '')
        st = ev['fields'].get('SOURCETYPE', '0')
        res_files = []
        if st == '1':
            g = groups.get(sn)
            if g is None:
                missing_refs.append((evname, 'group', sn))
            else:
                group_users[sn].add(evname)
                for s in g['samples']:
                    r = resolve(s)
                    if r is None:
                        missing_refs.append((evname, 'sample-in-group ' + sn, s))
                    else:
                        ref_by[r].add(evname)
                        ref_via[r].add(g['name'])
                        res_files.append(r)
        else:
            r = resolve(sn)
            if r is None:
                missing_refs.append((evname, 'sample', sn))
            else:
                ref_by[r].add(evname)
                res_files.append(r)
        ev['resolved'] = res_files
    unused_groups = [g['name'] for k, g in groups.items() if '#dup' not in k and k not in group_users]
    orphan_group_members = collections.defaultdict(set)
    for k, g in groups.items():
        if k in group_users:
            continue
        for s in g['samples']:
            r = resolve(s)
            if r:
                orphan_group_members[r].add(g['name'])
            else:
                missing_refs.append(('(unused group %s)' % g['name'], 'sample-in-group', s))
    summary.append('missing references (name -> no file): %s' % missing_refs)
    summary.append('unused sample groups (%d): %s' % (len(unused_groups), unused_groups))

    # unused_samples.sh emulation (Darwinia-era: only data/sounds, names from SOUNDNAME + SAMPLE lines)
    txt = open(os.path.join(src, 'data/sounds.txt'), encoding='latin-1').read().splitlines()
    names = set()
    for l in txt:
        if re.search(r'soundname ', l, re.I):
            names.add(re.sub(r'[ \t]*SOUNDNAME[ \t]*', '', l).strip().lower())
        if re.search(r'sample ', l, re.I):
            names.add(re.sub(r'[ \t]*SAMPLE[ \t]*', '', l).strip().lower())
    sh_unused = sorted(remove_ext(f.lower()) for fo, f in files if fo == 'sounds' and remove_ext(f.lower()) not in names)
    summary.append('unused_samples.sh emulation (data/sounds only, CR stripped): %d names on disk not in sounds.txt' % len(sh_unused))
    sg_names = set(k for k in group_defs)
    snd_names = set(e.get('soundname', '') for e in events)
    summary.append('unused_sample_groups.sh emulation: groups not used as SOUNDNAME: %s' % sorted(sg_names - snd_names))

    # ---------------- code call sites
    vcproj = {}
    man = os.path.join(outw, 'manifest.csv')
    for r in csv.DictReader(open(man)):
        vcproj[r['path']] = r['vcproj_build']
    code_files = []
    for root, _d, fs in os.walk(os.path.join(src, 'code')):
        for f in fs:
            if f.endswith(('.cpp', '.h')):
                rel = os.path.relpath(os.path.join(root, f), src).replace(os.sep, '/')
                if rel.startswith('code/sound/soundsystem.'):
                    continue
                code_files.append(rel)
    code_files.sort()
    sites = find_callsites(src, code_files, vcproj)
    name_to_class, blocked, ent_enum, bld_enum = read_type_tables(src)
    parent = class_hierarchy(src)
    lse, lse_base = list_sound_events(src)
    bodies = method_bodies(src, [f for f in code_files if vcproj.get(f)])
    overridden_log = []

    def lse_for(cls):
        out = []
        c = cls
        seen = set()
        while c and c not in seen:
            seen.add(c)
            out += lse.get(c, [])
            c = lse_base.get(c)
        return out

    with open(os.path.join(drafts, 'X2b_callsites.csv'), 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['file', 'line', 'function', 'call', 'object_arg', 'event_arg', 'other_type', 'team_arg',
                    'commented', 'pp_active', 'pp_conditions', 'vcproj_build'])
        for s in sites:
            a = s['args'] + ['', '', '', '']
            other = ''
            team = ''
            if s['call'] == 'TriggerOtherEvent':
                mm = re.search(r'SoundSourceBlueprint::(\w+)', a[2])
                other = OTHER_ENUM.get(mm.group(1), mm.group(1)) if mm else a[2]
                team = a[3]
            s['other'] = other
            s['ev'] = a[1]
            s['obj'] = a[0]
            w.writerow([s['file'], s['line'], s['func'], s['call'], a[0], a[1], other, team,
                        'yes' if s['commented'] else '', 'yes' if s['pp_active'] else 'NO', s['pp'], s['vcproj']])
    with open(os.path.join(drafts, 'X2b_callsite_context.txt'), 'w') as fh:
        for s in sites:
            fh.write('=== %s:%d %s %s(%s) commented=%s active=%s pp=%s vcproj=%s\n' % (
                s['file'], s['line'], s['func'], s['call'], ', '.join(s['args']), s['commented'], s['pp_active'],
                s['pp'], s['vcproj']))
            for l in s['context']:
                fh.write('    ' + l.rstrip() + '\n')
    summary.append('call sites: %d (commented %d, pp-inactive %d, not in vcproj %d)' % (
        len(sites), sum(1 for s in sites if s['commented']), sum(1 for s in sites if not s['pp_active']),
        sum(1 for s in sites if not s['vcproj'])))

    # ---------------- map sites to (group,type,event)
    # manual site->targets rules (dynamic names / non-this emitters), verified by reading the code:
    synth = notes.get('SYNTH_SITES', [])          # list of [group,type,event,site-desc,status]
    site_override = notes.get('SITE_TARGETS', {})  # "file:line" -> list of [group,type] or [group,type,event]
    live_sites = collections.defaultdict(list)
    def site_status(s):
        if s['commented']:
            return 'commented-out'
        if not s['vcproj']:
            return 'not-compiled(vcproj)'
        if not s['pp_active']:
            return 'not-compiled(#if %s)' % s['pp']
        return notes.get('SITE_STATUS', {}).get('%s:%d' % (s['file'], s['line']), 'FULL')
    all_types = {}
    for (g, t) in name_to_class:
        all_types[(g, t)] = name_to_class[(g, t)]
    pair_sites = collections.defaultdict(list)
    for s in sites:
        key = '%s:%d' % (s['file'], s['line'])
        st = site_status(s)
        evlit = s['ev'].strip()
        lit = evlit[1:-1] if evlit.startswith('"') and evlit.endswith('"') else None
        ref = '%s:%s:L%d' % (s['file'], s['func'] or '?', s['line'])
        targets = []
        if key in site_override:
            for tg in site_override[key]:
                if len(tg) == 3:
                    targets.append((tg[0], tg[1], tg[2]))
                elif lit:
                    targets.append((tg[0], tg[1], lit))
        elif s['call'] == 'TriggerOtherEvent':
            if lit:
                targets.append(('OTHER', s['other'], lit))
        else:
            grp = 'ENTITY' if s['call'] == 'TriggerEntityEvent' else 'BUILDING'
            if lit and s['obj'].strip() == 'this':
                fcls, _, meth = (s['func'] or '').partition('::')
                restrict = notes.get('BASE_APPLIES', {}).get(key)
                for (g, t), c in all_types.items():
                    if g != grp or not c:
                        continue
                    if restrict is not None and '%s|%s' % (g, t) not in restrict:
                        continue
                    ok, why = base_site_applies(c, fcls, meth, parent, bodies)
                    if ok:
                        targets.append((g, t, lit))
                    elif why.startswith('overridden'):
                        overridden_log.append('%s:%d %s -> %s %s: %s' % (s['file'], s['line'], s['func'], g, t, why))
        s['targets'] = targets
        for tg in targets:
            pair_sites[tg].append((ref, st))
    for row in synth:
        pair_sites[(row[0], row[1], row[2])].append((row[3], row[4]))
    summary.append('base-class sites suppressed by overrides: %s' % overridden_log)

    # ---------------- events table
    data_pairs = collections.OrderedDict()
    for ev in events:
        data_pairs.setdefault((ev['group'], ev['type'], ev['event'].lower()), []).append(ev)
    # code pairs without data (case-insensitive event names)
    code_pairs = collections.OrderedDict()
    for (g, t, e), lst in pair_sites.items():
        code_pairs.setdefault((g, t, e.lower()), (e, lst))
    reach = notes.get('TYPE_REACH', {})
    descr = notes.get('EVENT_DESC', {})
    rows = []
    keys = list(data_pairs.keys())
    for k in code_pairs:
        if k not in data_pairs:
            # only keep code-only pairs for types that have a blueprint in data, or generic-base pairs
            keys.append(k)
    seen = set()
    for k in keys:
        if k in seen:
            continue
        seen.add(k)
        g, t, el = k
        evs = data_pairs.get(k, [])
        evname = evs[0]['event'] if evs else code_pairs[k][0]
        sitel = code_pairs.get(k, (None, []))[1]
        # filter generic-base-class sites for code-only pairs on types with no data at all -> skip noise
        if not evs and (g, t) not in sources:
            continue
        smp = []
        for ev in evs:
            for r in ev['resolved']:
                p = 'data/%s/%s' % r
                if p not in smp:
                    smp.append(p)
        tr = reach.get('%s|%s' % (g, t), '')
        if any(st == 'FULL' for _r, st in sitel):
            fired = 'yes' if (tr.startswith('FULL') or not tr) else 'site-live/type-%s' % tr.split(':')[0]
        elif sitel:
            fired = 'no(' + ','.join(sorted(set(st.split('(')[0] for _r, st in sitel))) + ')'
        else:
            fired = 'no(no-site)'
        if not evs:
            gd = notes.get('GENERIC_DESC', {}).get('%s|%s' % (g, evname), '')
        rows.append({
            'group': g, 'object_type': t, 'event': evname, 'in_data': 'yes' if evs else 'no',
            'blocks': len(evs),
            'blueprint': ' || '.join('[%s %s] %s' % (SRCT.get(ev['fields'].get('SOURCETYPE', '0')), ev.get('soundname', ''),
                                                      event_summary(ev)) for ev in evs),
            'n_samples': len(smp), 'samples': ';'.join(smp),
            'code_sites': ' ; '.join('%s [%s]' % (r, st) for r, st in sitel),
            'fired_in_FULL': fired,
            'type_reach': reach.get('%s|%s' % (g, t), ''),
            'listed_in_ListSoundEvents': '',
            'gameplay_trigger': descr.get('%s|%s|%s' % (g, t, evname), '') if evs else gd,
        })
        cls = name_to_class.get((g, t), '')
        if cls:
            rows[-1]['listed_in_ListSoundEvents'] = 'yes' if evname.lower() in [x.lower() for x in lse_for(cls)] else 'no'
    with open(os.path.join(drafts, 'X2b_sound_events.csv'), 'w', newline='') as fh:
        cols = ['group', 'object_type', 'event', 'in_data', 'blocks', 'blueprint', 'n_samples', 'samples',
                'code_sites', 'fired_in_FULL', 'type_reach', 'listed_in_ListSoundEvents', 'gameplay_trigger']
        w = csv.DictWriter(fh, fieldnames=cols)
        w.writeheader()
        for r in rows:
            w.writerow(r)
    summary.append('event pairs: %d (with data %d, code-only %d)' % (
        len(rows), sum(1 for r in rows if r['in_data'] == 'yes'), sum(1 for r in rows if r['in_data'] == 'no')))
    summary.append('data pairs with no live FULL site: %s' % [
        '%s %s %s' % (r['group'], r['object_type'], r['event']) for r in rows if r['in_data'] == 'yes' and r['fired_in_FULL'] != 'yes'])
    summary.append('pairs missing description: %d' % sum(1 for r in rows if not r['gameplay_trigger']))

    # liveness of each event pair for sample reachability
    live_pair = set()
    for r in rows:
        if r['in_data'] == 'yes' and r['fired_in_FULL'] == 'yes':
            live_pair.add('%s %s %s' % (r['group'], r['object_type'], r['event']))
    summary.append('live data pairs: %d of %d' % (len(live_pair), sum(1 for r in rows if r['in_data'] == 'yes')))

    # ---------------- samples table
    srows = []
    for fo, f in files:
        inf = info[(fo, f)]
        key = ('%s/%s' % (fo, f)).lower()
        refs = sorted(ref_by.get((fo, f), []))
        status = 'referenced' if refs else 'unused'
        livestat = ''
        if refs:
            livestat = 'live' if any(x in live_pair for x in refs) else 'dead-data'
        # all Vorbis comments; personal-name fields are redacted (ENGINEER/ARTIST/PERFORMER/COPYRIGHT values)
        tags = []
        for c in inf.get('comments', []):
            k, eq, v = c.partition('=')
            if eq and k.upper() in ('ENGINEER', 'ARTIST', 'PERFORMER', 'COPYRIGHT', 'CONTACT'):
                v = '<redacted>'
            tags.append(k + eq + v)
        srows.append({
            'path': 'data/%s/%s' % (fo, f), 'folder': fo, 'name': remove_ext(f.lower()),
            'channels': inf.get('channels', ''), 'rate': inf.get('rate', ''),
            'duration_s': '%.3f' % inf['duration'] if inf.get('duration') is not None else '',
            'bytes': inf['bytes'], 'nominal_kbps': '%.0f' % (inf['br_nom'] / 1000.0) if inf.get('br_nom', 0) > 0 else '',
            'vendor': inf.get('vendor', ''), 'tags': ' | '.join(tags),
            'referenced_by': ';'.join(refs), 'via_groups': ';'.join(sorted(ref_via.get((fo, f), []))),
            'orphan_groups': ';'.join(sorted(orphan_group_members.get((fo, f), []))),
            'status': status, 'event_liveness': livestat,
            'archive': ';'.join(pack[key]) if pack[key] else 'NOT-SHIPPED(%s)' % excluded_by.get(key, '?'),
            'unused_samples_sh': 'listed' if fo == 'sounds' and remove_ext(f.lower()) in sh_unused else '',
            'pcm_bytes_decoded': inf['granule'] * 2 * inf['channels'] if inf.get('granule') else '',
        })
    with open(os.path.join(drafts, 'X2b_samples.csv'), 'w', newline='') as fh:
        cols = ['path', 'folder', 'name', 'channels', 'rate', 'duration_s', 'bytes', 'nominal_kbps', 'vendor', 'tags',
                'referenced_by', 'via_groups', 'orphan_groups', 'status', 'event_liveness', 'archive',
                'unused_samples_sh', 'pcm_bytes_decoded']
        w = csv.DictWriter(fh, fieldnames=cols)
        w.writeheader()
        for r in srows:
            w.writerow(r)

    # ---------------- statistics
    def stats(rs, label):
        n = len(rs)
        dur = sum(float(r['duration_s']) for r in rs if r['duration_s'])
        by = sum(int(r['bytes']) for r in rs)
        ch = collections.Counter(r['channels'] for r in rs)
        rt = collections.Counter(r['rate'] for r in rs)
        vend = collections.Counter(r['vendor'] for r in rs)
        kb = collections.Counter(r['nominal_kbps'] for r in rs)
        return '%s: files %d, total %.1f s (%.2f min), %d bytes (%.2f MiB), channels %s, rates %s, nominal kbps %s, vendors %s' % (
            label, n, dur, dur / 60, by, by / 1048576.0, dict(ch), dict(rt), dict(kb), dict(vend))
    summary.append(stats(srows, 'ALL'))
    summary.append(stats([r for r in srows if r['folder'] == 'sounds'], 'data/sounds'))
    summary.append(stats([r for r in srows if r['folder'] == 'mwsounds'], 'data/mwsounds'))
    summary.append(stats([r for r in srows if r['status'] == 'referenced'], 'referenced'))
    summary.append(stats([r for r in srows if r['status'] == 'unused'], 'unused'))
    summary.append(stats([r for r in srows if r['event_liveness'] == 'live'], 'live'))
    summary.append(stats([r for r in srows if r['event_liveness'] == 'dead-data'], 'dead-data'))
    for a in ('menusounds.dat', 'sounds.dat', 'sounds2.dat'):
        summary.append(stats([r for r in srows if a in r['archive']], 'archive ' + a))
    summary.append(stats([r for r in srows if r['archive'].startswith('NOT')], 'not shipped'))
    summary.append('unused but shipped: %d' % sum(1 for r in srows if r['status'] == 'unused' and not r['archive'].startswith('NOT')))
    summary.append('referenced but not shipped: %s' % [r['path'] for r in srows if r['status'] == 'referenced' and r['archive'].startswith('NOT')])
    summary.append('live but not shipped: %s' % [r['path'] for r in srows if r['event_liveness'] == 'live' and r['archive'].startswith('NOT')])
    longest = sorted(srows, key=lambda r: -float(r['duration_s'] or 0))[:15]
    summary.append('longest: %s' % [(r['path'], r['duration_s']) for r in longest])
    shortest = sorted(srows, key=lambda r: float(r['duration_s'] or 0))[:5]
    summary.append('shortest: %s' % [(r['path'], r['duration_s']) for r in shortest])
    summary.append('files with tags: %s' % [(r['path'], r['tags']) for r in srows if r['tags']])
    summary.append('non-44100: %s' % [(r['path'], r['rate']) for r in srows if str(r['rate']) != '44100'])
    summary.append('non-mono: %s' % [(r['path'], r['channels']) for r in srows if str(r['channels']) != '1'])
    summary.append('header errors: %s' % [(k, v.get('error')) for k, v in info.items() if not v['ok']])
    summary.append('multi-stream/resync: %s' % [(k, v.get('streams'), v.get('resync')) for k, v in info.items() if v.get('streams', 1) != 1 or v.get('resync')])
    summary.append('vorbis_version!=0: %s' % [k for k, v in info.items() if v.get('vorbis_version')])
    summary.append('largest decoded PCM: %s' % sorted(((r['pcm_bytes_decoded'] or 0), r['path']) for r in srows)[-3:])
    open(os.path.join(drafts, 'X2b_summary.txt'), 'w').write('\n'.join(summary) + '\n')

    # ---------------- markdown helpers
    def md_esc(s):
        return str(s).replace('|', '\\|')
    def short_ev(e):
        return ' '.join(e.split(' ')[1:])
    with open(os.path.join(drafts, 'X2b_md_samples.md'), 'w') as fh:
        for fo in ('sounds', 'mwsounds'):
            rs = [r for r in srows if r['folder'] == fo]
            fh.write('\n### `data/%s/` (%d files)\n\n' % (fo, len(rs)))
            fh.write('Columns: duration s; size KiB; archive (`menusounds.dat` / `sounds.dat` / `sounds2.dat` / NOT-SHIPPED(exclusion pattern)); status (live = referenced by an event fired in FULL, dead = referenced only by unreachable/never-fired events, unused = no blueprint reference); referenced by (sample group -> events).\n\n')
            fh.write('| Path | s | KiB | Archive | Status | Referenced by |\n|---|---|---|---|---|---|\n')
            for r in rs:
                if r['status'] == 'referenced':
                    st = 'live' if r['event_liveness'] == 'live' else 'dead'
                    evs = '; '.join(short_ev(e) for e in r['referenced_by'].split(';'))
                    ref = ('`%s` -> ' % r['via_groups'].replace(';', '`, `') if r['via_groups'] else '') + evs
                else:
                    st = 'unused'
                    ref = ('orphan group `%s`' % r['orphan_groups']) if r['orphan_groups'] else ''
                arch = r['archive']
                fh.write('| `%s` | %s | %.0f | %s | %s | %s |\n' % (r['path'], r['duration_s'], int(r['bytes']) / 1024.0,
                                                                  md_esc(arch), st, md_esc(ref)))

    def site_short(sites):
        out = []
        for part in sites.split(' ; '):
            if not part:
                continue
            ref, _, st = part.rpartition(' [')
            st = st.rstrip(']')
            out.append('`%s`' % ref + ('' if st == 'FULL' else ' [%s]' % st))
        return '<br>'.join(out) if out else '—'
    with open(os.path.join(drafts, 'X2b_md_events.md'), 'w') as fh:
        fh.write('| # | Source | Event | Data blocks: [Sample\\|Group name] pos/loop/inst/team md=min-distance v=volume f=pitch a/s/r/ld=ADSR,loop-delay fx | Files | Fired in FULL | Gameplay trigger | Code trigger site(s) |\n')
        fh.write('|---|---|---|---|---|---|---|---|\n')
        i = 0
        for r in rows:
            if r['in_data'] != 'yes':
                continue
            i += 1
            fh.write('| %d | %s %s | %s | %s | %d | %s | %s | %s |\n' % (
                i, r['group'], r['object_type'], r['event'], md_esc(r['blueprint']).replace(' || ', '<br>'),
                r['n_samples'], r['fired_in_FULL'], md_esc(r['gameplay_trigger']), md_esc(site_short(r['code_sites']))))
    with open(os.path.join(drafts, 'X2b_md_events_codeonly.md'), 'w') as fh:
        agg = collections.OrderedDict()
        for r in rows:
            if r['in_data'] == 'yes':
                continue
            k = (r['group'], r['event'])
            a = agg.setdefault(k, {'types': [], 'sites': [], 'fired': set(), 'desc': r['gameplay_trigger']})
            a['types'].append(r['object_type'])
            a['fired'].add(r['fired_in_FULL'])
            for part in r['code_sites'].split(' ; '):
                if part and part not in a['sites']:
                    a['sites'].append(part)
        fh.write('| # | Group | Event (fired, no data -> silent) | Object types (with a blueprint in sounds.txt) | Fired in FULL | Meaning | Code site(s) |\n|---|---|---|---|---|---|---|\n')
        for i, (k, a) in enumerate(agg.items(), 1):
            fh.write('| %d | %s | %s | %s | %s | %s | %s |\n' % (i, k[0], k[1], ', '.join(a['types']), '/'.join(sorted(a['fired'])),
                                                          md_esc(a['desc']), md_esc(site_short(' ; '.join(a['sites'])))))
    # ---------------- coverage fragment
    cov = os.path.join(outw, 'coverage', 'X2b_asset_catalogue_audio.csv')
    with open(cov, 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['path', 'depth', 'reason', 'sections'])
        for r in srows:
            sec = ['25.B Per-directory audio catalogue (every file)', '10.I Statistics']
            if r['status'] == 'referenced':
                sec.append('10.C Complete event table')
                if r['archive'].startswith('NOT'):
                    sec.append('10.F Music')
            else:
                sec.append('10.J Unused samples dead data and the tools/ checking scripts')
            w.writerow([r['path'], 'full', '', ';'.join(sec)])
        for path, sec in (
                ('data/sounds.txt', '10.A Method and verification of P16;10.B How a sample name becomes a file;10.C Complete event table;10.D Events fired by code that have no data;10.J Unused samples'),
                ('tools/sounds.txt', '10.B How a sample name becomes a file (packaging);25.A Audio assets in the asset pipeline'),
                ('tools/menu_sounds.txt', '10.B How a sample name becomes a file (packaging);10.F Music;25.A Audio assets in the asset pipeline'),
                ('tools/unused_samples.sh', '10.J Unused samples dead data and the tools/ checking scripts'),
                ('tools/unused_sample_groups.sh', '10.J Unused samples dead data and the tools/ checking scripts'),
                ('tools/build_multiwinia_data.bat', '10.B How a sample name becomes a file (packaging)'),
                ('tools/multiwinia_exclusions.txt', '10.B How a sample name becomes a file (packaging);10.F Music'),
                ('tools/listfiles.pl', '10.B How a sample name becomes a file (packaging)'),
                ('tools/patch.full.list', '10.B How a sample name becomes a file (packaging)'),
                ('code/sound/sample_cache.cpp', '10.B How a sample name becomes a file'),
                ('code/lib/resource.cpp', '10.B How a sample name becomes a file'),
                ('contrib/systemiv/lib/tosser/hash_table.cpp', '10.B How a sample name becomes a file'),
                ('contrib/systemiv/lib/filesys/filesys_utils.cpp', '10.B How a sample name becomes a file')):
            w.writerow([path, 'full', '', sec])
        for f in sorted(set(s['file'] for s in sites)):
            w.writerow([f, 'skimmed', 'only Trigger*Event / StopAllSounds call sites and their enclosing functions read', '10.C Complete event table;10.D Events fired by code that have no data'])
    print('\n'.join(summary))


if __name__ == '__main__':
    main()
