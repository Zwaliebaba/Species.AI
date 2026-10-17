#!/usr/bin/env python3
"""P10 helper: re-implements the Multiwinia input-preferences parser (code/lib/input/*)
to classify every line of the shipped input preference files, and lists, for every
ControlType in control_types.inc, its default bindings and its consumers in live code.

Usage: python3 -I P10_bindings.py <SRC root> <manifest.csv> <out_csv> <out_md> <out_lines_csv>
"""
import csv, os, re, sys

SRC, MANIFEST, OUT_CSV, OUT_MD, OUT_LINES = sys.argv[1:6]

# ---------------------------------------------------------------- control types
inc = open(os.path.join(SRC, 'code/lib/input/control_types.inc'), encoding='latin-1').read()
controls = []  # (index, name, typeName)
for line in inc.splitlines():
    s = line.strip()
    if s.startswith('//'):
        continue
    m = re.match(r'DEF_CONTROL_TYPE\(\s*(\w+)\s*,\s*(INPUT_TYPE_\w+)\s*\)\s*(//\s*(.*))?', s)
    if m:
        controls.append((len(controls), m.group(1), m.group(2), (m.group(4) or '').strip()))
TYPEVAL = {'INPUT_TYPE_FAIL': 0, 'INPUT_TYPE_BOOL': 1, 'INPUT_TYPE_ANALOG': 2,
           'INPUT_TYPE_1D': 6, 'INPUT_TYPE_2D': 10, 'INPUT_TYPE_ANY': 15}
ctrl_by_lower = {c[1].lower(): c for c in controls}

# ---------------------------------------------------------------- key names
kn_src = open(os.path.join(SRC, 'code/lib/input/keynames.cpp'), encoding='latin-1').read()
body = kn_src[kn_src.index('{', kn_src.index('s_keyNames')) + 1:kn_src.index('};')]
keynames = re.findall(r'"([^"]*)"', body)
assert len(keynames) == 225, len(keynames)

# ---------------------------------------------------------------- parser re-implementation
CONDS = [(1, 'down', 0), (1, 'up', 1), (1, 'pressed', 2), (2, 'release', 5), (2, 'position', 6),
         (2, 'move', 7), (2, 'zero', 8), (2, 'read', 9)]
CONDNAME = {0: 'down', 1: 'up', 2: 'pressed', 3: 'repeat', 5: 'release', 6: 'position',
            7: 'move', 8: 'zero', 9: 'read'}


def default_cond(name, typ):
    for t, n, c in CONDS:
        if (typ & t) == t and name == n:
            return c, t, ''
    # Bug reproduced: loop falls onto the NULL terminator (index 8 < NumInputConditions=10)
    return 0, 0, 'unknown condition "%s" silently parsed as COND_DOWN with type 0' % name

XI = [('lefttrigger', 6), ('righttrigger', 6), ('leftbumper', 1), ('rightbumper', 1),
      ('leftthumbstick', 11), ('rightthumbstick', 11), ('dpadleft', 1), ('dpadright', 1),
      ('dpadup', 1), ('dpaddown', 1), ('a', 1), ('b', 1), ('x', 1), ('y', 1), ('start', 1),
      ('back', 1), ('plug', 1), ('leftthumbstickleft', 1), ('leftthumbstickright', 1),
      ('leftthumbstickup', 1), ('leftthumbstickdown', 1), ('rightthumbstickleft', 1),
      ('rightthumbstickright', 1), ('rightthumbstickup', 1), ('rightthumbstickdown', 1),
      ('lefttriggerb', 1), ('righttriggerb', 1)]
XI_ANALOG = {'lefttrigger', 'righttrigger', 'leftthumbstick', 'rightthumbstick'}
MOUSE = {'axes': 10, 'left': 1, 'right': 1, 'middle': 1, 'wheel': 6, 'any': 1}


class R:
    def __init__(self, ok, typ=None, kind='', note='', dev=''):
        self.ok, self.typ, self.kind, self.note, self.dev = ok, typ, kind, note, dev


def p_simple_tail(tokens, idx):
    return idx >= len(tokens)


def parse(tokens):
    if not tokens:
        return R(False, note='empty spec')
    # 0 Conjoin
    if '&&' in tokens:
        parts, cur = [], []
        for t in tokens + ['&&']:
            if t == '&&':
                parts.append(cur); cur = []
            else:
                cur.append(t)
        typ, complex_seen, notes, devs = 1, False, [], set()
        for p in parts:
            r = parse(p)
            if not r.ok:
                return R(False, kind='conjoin', note='part "%s" failed: %s' % (' '.join(p), r.note))
            if r.typ is not None and r.typ > 1:
                if complex_seen:
                    return R(False, kind='conjoin', note='too many complex inputs')
                complex_seen, typ = True, r.typ
            if r.note:
                notes.append(r.note)
            devs.add(r.dev)
        return R(True, typ, 'conjoin', '; '.join(notes), '+'.join(sorted(devs)))
    # 1 Chord
    if '++' in tokens:
        parts, cur = [], []
        for t in tokens + ['++']:
            if t == '++':
                parts.append(cur); cur = []
            else:
                cur.append(t)
        devs = set()
        for nDown in range(len(parts)):
            toks = []
            for i, p in enumerate(parts):
                if i:
                    toks.append('&&')
                toks += p + (['down'] if i == nDown else ['pressed'])
            r = parse(toks)
            if not r.ok or (r.typ is not None and r.typ > 1):
                return R(False, kind='chord', note='chord expansion failed: ' + r.note)
            devs.add(r.dev)
        return R(True, 1, 'chord', 'expanded to %d conjunctions (one part "down", others "pressed")'
                 % len(parts), '+'.join(sorted(devs)))
    # 2 Invert
    if tokens[0].lower() == 'not' or tokens[0] == '!':
        r = parse(tokens[1:])
        if r.ok and r.typ != 1:
            return R(False, kind='invert', note='complex/non-BOOL input cannot be negated')
        if not r.ok:
            return R(False, kind='invert', note=r.note)
        return R(True, 1, 'invert', r.note, r.dev)
    # 3 Idle (SimpleInputDriver)
    if tokens[0].lower() == 'idle':
        ctl = {'atleast': 6, 'reaches': 6, 'no': 1}
        if len(tokens) < 2 or tokens[1] not in ctl:
            return R(False, kind='idle', note='bad idle control (unterminated table lookup = UB)')
        typ = ctl[tokens[1]]
        if len(tokens) < 3:
            return R(False, kind='idle', note='missing condition')
        if typ == 1:
            if tokens[2].lower() != 'longer':
                return R(False, kind='idle', note='expected "longer"')
        else:
            if not re.match(r'^[+-]?\d+', tokens[2]) or int(re.match(r'^[+-]?\d+', tokens[2]).group(0)) <= 0:
                return R(False, kind='idle', note='expected positive integer seconds')
        if len(tokens) > 3:
            return R(False, kind='idle', note='overstep')
        return R(True, typ, 'idle', '', 'idle')
    # 4 W32 key / mouse
    if tokens[0] in ('key', 'mouse'):
        if len(tokens) < 2:
            return R(False, kind='w32', note='missing control')
        if tokens[0] == 'key':
            kid = None
            for i in range(0, 224):  # 0..KEY_TILDE(223)
                if tokens[1].lower() == keynames[i].lower():
                    kid = i; break
            if kid is None and tokens[1].lower() == 'any':
                kid = 255
            if kid is None:
                return R(False, kind='w32', note='unknown key "%s"' % tokens[1])
            typ, dev = 1, 'key'
        else:
            if tokens[1] not in MOUSE:
                return R(False, kind='w32', note='unknown mouse control')
            typ, dev = MOUSE[tokens[1]], 'mouse'
        if len(tokens) < 3:
            return R(False, kind='w32', note='missing condition')
        cond, ctyp, note = default_cond(tokens[2], typ)
        if len(tokens) > 3:
            return R(False, kind='w32', note='overstep')
        extra = ''
        if dev == 'key' and kid == 255:
            extra = 'KEY_ANY(255) is never set by WndProc -> never fires'
        if dev == 'mouse' and tokens[1] == 'any':
            extra = 'MOUSE_ANY unhandled in getMouseInput -> never fires'
        if dev == 'mouse' and tokens[1] in ('axes', 'wheel') and cond not in (7, 9):
            extra = 'mouse axes/wheel only implement move/read -> never fires'
        if dev == 'key' and cond not in (0, 1, 2):
            extra = 'key driver only implements down/up/pressed -> never fires'
        return R(True, ctyp, 'w32', '; '.join(x for x in (note, extra) if x), dev)
    # 5 XInput (only if xinput1_3.dll loads)
    if tokens[0] == 'XInput':
        names = [n for n, _ in XI]
        if len(tokens) < 2 or tokens[1] not in names:
            return R(False, kind='xinput', note='unknown XInput control')
        typ = dict(XI)[tokens[1]]
        if len(tokens) < 3:
            return R(False, kind='xinput', note='missing condition')
        if typ == 1 and tokens[2] == 'repeat':
            cond, ctyp, note = 3, 1, ''
        else:
            cond, ctyp, note = default_cond(tokens[2], typ)
        rest = tokens[3:]
        if tokens[1] in XI_ANALOG:
            if rest:
                if len(rest) == 3 and rest[0] == 'with' and rest[1] == 'sensitivity' and \
                        re.match(r'^\d+', rest[2]) and 0 < int(re.match(r'^\d+', rest[2]).group(0)) <= 200:
                    note = (note + '; ' if note else '') + 'sensitivity %s' % rest[2]
                else:
                    return R(False, kind='xinput', note='bad optional tokens')
        elif rest:
            return R(False, kind='xinput', note='overstep')
        return R(True, ctyp, 'xinput', note, 'xinput')
    # 6 Prefs
    if tokens[0] == 'pref':
        return R(True, 1, 'pref', 'depends on preference key existing at parse time', 'pref')
    # 7 Value
    if tokens[0] == 'value':
        if len(tokens) == 2 and tokens[1] in ('Pecan', 'Hummingbird'):
            return R(True, None, 'value', 'spec.type left uninitialised', 'value')
        return R(False, kind='value', note='bad value')
    # 8 Alias
    if tokens[0] == 'alias':
        if len(tokens) == 2 and tokens[1].lower() in ctrl_by_lower:
            return R(True, 1, 'alias', 'alias of %s' % ctrl_by_lower[tokens[1].lower()][1], 'alias')
        return R(False, kind='alias', note='bad alias')
    return R(False, note='no driver accepts "%s"' % tokens[0])


def acceptable(ctrl_type, spec_type):
    if spec_type is None:
        return None
    v = TYPEVAL[ctrl_type]
    if v & 1:
        v |= 6
    return (v & spec_type) == spec_type

SEPS = ' \t\n\r:,'


def tokenize_line(raw):
    line = raw.split('#', 1)[0].rstrip('\r\n')
    i = 0
    def nxt():
        nonlocal i
        while i < len(line) and line[i] in SEPS:
            i += 1
        if i >= len(line):
            return None
        j = i
        while j < len(line) and line[j] not in SEPS:
            j += 1
        tok = line[i:j]
        i = j + 1
        return tok
    def rest():
        nonlocal i
        k = i
        while k < len(line) and line[k] in SEPS:
            k += 1
        return line[k:] if k < len(line) else None
    c = nxt()
    if c is None:
        return None
    e = nxt()
    r = rest()
    return c, e, r


def process(relpath):
    rows = []
    bindings = {c[1]: [] for c in controls}
    icons = {}
    path = os.path.join(SRC, relpath)
    for ln, raw in enumerate(open(path, encoding='latin-1'), 1):
        t = tokenize_line(raw)
        if t is None:
            continue
        c, e, r = t
        status, detail, kind, dev, spec = '', '', '', '', (r or '').strip()
        if e not in ('=', '~'):
            status = 'IGNORED'; detail = 'Assignment not found (no "=" or "~")'
        elif c.lower() not in ctrl_by_lower:
            status = 'IGNORED'; detail = 'Control ID not found'
        else:
            ctl = ctrl_by_lower[c.lower()]
            if e == '~':
                if spec:
                    icons[ctl[1]] = spec; status = 'ICON'
                else:
                    status = 'IGNORED'; detail = 'Empty icon line'
            else:
                res = parse(spec.split())
                if not res.ok:
                    status = 'PARSE_FAIL'; detail = res.note; kind = res.kind
                else:
                    acc = acceptable(ctl[2], res.typ)
                    if acc is False:
                        status = 'BIND_FAIL'; detail = 'input type %s not acceptable for %s' % (res.typ, ctl[2])
                    else:
                        status = 'BOUND'; detail = res.note; kind = res.kind; dev = res.dev
                        bindings[ctl[1]].append((spec, dev, res.note))
        rows.append(dict(file=relpath, line=ln, control=c, op=e or '', spec=spec, status=status,
                         driver=kind, device=dev, detail=detail))
    return rows, bindings, icons

FILES = ['data/input/input_preferences.txt', 'data/input/input_preferences_darwinia.txt',
         'data/input/input_preferences_mac.txt', 'data/input/keyboards/de_DE.txt',
         'data/input/keyboards/fr_FR.txt']
results = {f: process(f) for f in FILES}

# ---------------------------------------------------------------- consumers in live code
live = set()
with open(MANIFEST, newline='') as fh:
    for row in csv.DictReader(fh):
        p = row['path']
        if p.startswith('code/') and row['vcproj_build'] == 'all' and re.search(r'\.(cpp|h|inc)$', p):
            live.add(p)

DEFINED = set('''TARGET_PC_FULLGAME TARGET_MSVC WIN32 NDEBUG LAN_PLAY_ENABLED WAN_PLAY_ENABLED
NETWORK_STATS_ENABLED INCLUDEGAMEMODE_DOMINATION INCLUDEGAMEMODE_KOTH INCLUDEGAMEMODE_CTS
INCLUDEGAMEMODE_ROCKET INCLUDEGAMEMODE_ASSAULT INCLUDEGAMEMODE_BLITZ INCLUDE_CRATES_BASIC
INCLUDE_CRATES_ADVANCED INCLUDE_TUTORIAL AUTHENTICATION_LEVEL BLOCK_OLD_DARWINIA_OBJECTS
DUMP_DEBUG_LOG DEBUG_RENDER_ENABLED FLOAT_NUMERICS USE_CRASHREPORTING HAVE_DSOUND
USE_DISPLAY_LISTS INCLUDE_THEBEAST HAVE_REFRESH_RATES USE_PIXEL_EFFECT_GRID_OPTIMISATION
STRETCH_RECT DARWINIA_GAMETYPE'''.split())
UNDEF = set('''TARGET_DEBUG _DEBUG USE_SEPULVEDA_HELP_TUTORIAL LOCATION_EDITOR TESTBED_ENABLED
TRACK_SYNC_RAND SOUND_EDITOR GESTURE_EDITOR AVI_GENERATOR CHEATMENU_ENABLED INCLUDE_CRATE_HELP_WINDOW
TARGET_OS_VISTA TARGET_OS_MACOSX TARGET_OS_LINUX USE_DIRECT3D MULTIPLAYER_DISABLED
INCLUDEGAMEMODE_TANKBATTLE INCLUDEGAMEMODE_PROLOGUE INCLUDEGAMEMODE_CAMPAIGN PERMITTED_MAPS
SPECTATOR_ONLY HIDE_INVALID_GAMETYPES TARGET_DEMOGAME TARGET_MULTIWINIA_DEMOONLY MULTIWINIA_DEMOONLY
DEMOBUILD DEMO2 EGAMESBUILD OBERONBUILD AMBROSIA_REGISTRATION BENCHMARK_AND_FTP USE_DARWINIA_MOD_SYSTEM
HAVE_XAUDIO _XBOX TEST_HARNESS_ENABLED ATTRACTMODE_ENABLED PURITY_CONTROL SCRIPT_TEST_ENABLED
PROFILER_ENABLED USE_LOADERS USE_FIXED FIXED64_NUMERICS USE_SOUND_DSPEFFECTS USE_SOUND_HW3D
USE_SOUND_MEMORY INVOKE_CALLBACK_FROM_SOUND_THREAD TARGET_PC_HARDWARECOMPAT TARGET_PC_PREVIEW
TARGET_ASSAULT_STRESSTEST TARGET_BETATEST_GROUP TARGET_BETATEST_GROUP_ALL TARGET_BETATEST_GROUP_A
TARGET_BETATEST_GROUP_B TARGET_BETATEST_GROUP_C USE_PENTIUM_COUNTER FOG_LOGGING FOG_PREFERENCE
GESTURE_EDITOR_ENABLED XBOX360 TARGET_XBOX TARGET_X360 SDL_LOADERS'''.split())


def eval_if(expr):
    e = re.sub(r'//.*', '', expr).strip()
    unknown = False
    def dfn(m):
        nonlocal unknown
        n = m.group(1)
        if n in DEFINED: return ' 1 '
        if n in UNDEF: return ' 0 '
        unknown = True; return ' 1 '
    e = re.sub(r'defined\s*\(\s*(\w+)\s*\)', dfn, e)
    e = re.sub(r'defined\s+(\w+)', dfn, e)
    e = e.replace('&&', ' and ').replace('||', ' or ')
    e = re.sub(r'!(?!=)', ' not ', e)
    def ident(m):
        nonlocal unknown
        n = m.group(0)
        if n in ('and', 'or', 'not'): return n
        if n in DEFINED: return '1'
        if n in UNDEF: return '0'
        unknown = True; return '1'
    e = re.sub(r'[A-Za-z_]\w*', ident, e)
    try:
        v = bool(eval(e, {'__builtins__': {}}, {}))
    except Exception:
        return True, True
    return v, unknown


def strip_comments(text):
    out, i, n = [], 0, len(text)
    while i < n:
        if text.startswith('//', i):
            j = text.find('\n', i); i = n if j < 0 else j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            seg = text[i:(n if j < 0 else j + 2)]
            out.append('\n' * seg.count('\n')); i = n if j < 0 else j + 2
        elif text[i] == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1]); i = j + 1
        else:
            out.append(text[i]); i += 1
    return ''.join(out)

usage = {c[1]: set() for c in controls}
for p in sorted(live):
    if p.startswith('code/lib/input/'):
        continue
    try:
        text = strip_comments(open(os.path.join(SRC, p), encoding='latin-1').read())
    except FileNotFoundError:
        continue
    stack = []  # (active, taken)
    active = True
    for ln, line in enumerate(text.splitlines(), 1):
        s = line.strip()
        m = re.match(r'#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', s)
        if m:
            d, rest = m.group(1), m.group(2).strip()
            if d in ('ifdef', 'ifndef'):
                name = rest.split()[0] if rest else ''
                if name in DEFINED: v = True
                elif name in UNDEF: v = False
                else: v = True
                if d == 'ifndef' and (name in DEFINED or name in UNDEF): v = not v
                stack.append((active, v)); active = active and v
            elif d == 'if':
                v, _ = eval_if(rest); stack.append((active, v)); active = active and v
            elif d == 'elif':
                parent, taken = stack[-1] if stack else (True, False)
                v, _ = eval_if(rest)
                nv = (not taken) and v
                stack[-1] = (parent, taken or v); active = parent and nv
            elif d == 'else':
                parent, taken = stack[-1] if stack else (True, False)
                stack[-1] = (parent, True); active = parent and not taken
            elif d == 'endif':
                if stack:
                    parent, _ = stack.pop(); active = parent
            continue
        if not active:
            continue
        for mm in re.finditer(r'\bControl([A-Z]\w*)\b', line):
            nm = mm.group(1)
            if nm in usage:
                usage[nm].add(p)

# ---------------------------------------------------------------- outputs
main_rows, main_b, main_i = results['data/input/input_preferences.txt']
dw_rows, dw_b, dw_i = results['data/input/input_preferences_darwinia.txt']
mac_rows, mac_b, mac_i = results['data/input/input_preferences_mac.txt']
de_rows, de_b, _ = results['data/input/keyboards/de_DE.txt']
fr_rows, fr_b, _ = results['data/input/keyboards/fr_FR.txt']


def devclass(d):
    parts = d.split('+')
    if 'xinput' in parts:
        return 'pad'
    if 'key' in parts or 'mouse' in parts:
        return 'kbm'
    return 'other'


def mark(s, note):
    if 'silently parsed as COND_DOWN' in note:
        return s + ' [=down: unknown-condition fallback]'
    if 'never fires' in note:
        return s + ' [never fires]'
    return s


def fmt(blist, devs):
    return ' | '.join(mark(s, n) for s, d, n in blist if devclass(d) in devs)

with open(OUT_LINES, 'w', newline='') as fh:
    w = csv.DictWriter(fh, fieldnames=['file', 'line', 'control', 'op', 'spec', 'status', 'driver', 'device', 'detail'])
    w.writeheader()
    for f in FILES:
        for r in results[f][0]:
            w.writerow(r)

KB = ('kbm',)
PAD = ('pad',)
OTHER = ('other',)
hdr = ['idx', 'control', 'type', 'comment', 'kbm_default', 'xinput_default', 'other_default', 'icon',
       'darwinia_kbm_diff', 'darwinia_xinput_diff', 'mac_kbm_diff', 'de_DE_primary_override',
       'fr_FR_primary_override', 'live_consumers']
rows = []
for idx, name, typ, comment in controls:
    mb = main_b[name]
    kbm = fmt(mb, KB)
    pad = fmt(mb, PAD)
    other = fmt(mb, OTHER)
    dkbm, dpad = fmt(dw_b[name], KB), fmt(dw_b[name], PAD)
    mkbm = fmt(mac_b[name], KB)
    rows.append(dict(idx=idx, control=name, type=typ.replace('INPUT_TYPE_', ''), comment=comment,
                     kbm_default=kbm, xinput_default=pad, other_default=other,
                     icon=main_i.get(name, ''),
                     darwinia_kbm_diff='' if dkbm == kbm else (dkbm or '(none)'),
                     darwinia_xinput_diff='' if dpad == pad else (dpad or '(none)'),
                     mac_kbm_diff='' if mkbm == kbm else (mkbm or '(none)'),
                     de_DE_primary_override=fmt(de_b[name], KB), fr_FR_primary_override=fmt(fr_b[name], KB),
                     live_consumers=';'.join(sorted(x.replace('code/', '') for x in usage[name]))))
with open(OUT_CSV, 'w', newline='') as fh:
    w = csv.DictWriter(fh, fieldnames=hdr)
    w.writeheader(); w.writerows(rows)

# Markdown table
def esc(s):
    return s.replace('|', '\\|') if s else '—'
with open(OUT_MD, 'w') as fh:
    fh.write('| # | ControlType | Type | Source comment | Keyboard / mouse default (input_preferences.txt) | Xbox (XInput) default | Darwinia-scheme diff (KB/M ; pad) | Live consumers (code/) |\n')
    fh.write('|---|---|---|---|---|---|---|---|\n')
    for r in rows:
        diff = ''
        if r['darwinia_kbm_diff'] or r['darwinia_xinput_diff']:
            diff = 'KB/M: %s ; pad: %s' % (r['darwinia_kbm_diff'] or 'same', r['darwinia_xinput_diff'] or 'same')
        fh.write('| %d | `%s` | %s | %s | %s | %s | %s | %s |\n' % (
            r['idx'], r['control'], r['type'], esc(r['comment']), esc(r['kbm_default'] + ((' ; other: ' + r['other_default']) if r['other_default'] else '')),
            esc(r['xinput_default']), esc(diff), esc(r['live_consumers'].replace(';', ', '))))
    fh.write('\n')
    fh.write('\n#### Icons (`~` lines) in input_preferences.txt (last per control wins)\n\n| Control | Icon |\n|---|---|\n')
    for k, v in main_i.items():
        fh.write('| `%s` | `%s` |\n' % (k, v))
    fh.write('\n### Lines of the shipped preference files that do not create a binding\n\n')
    fh.write('| File:line | Control | Op | Spec | Status | Reason |\n|---|---|---|---|---|---|\n')
    for f in FILES:
        for r in results[f][0]:
            if r['status'] not in ('BOUND', 'ICON') or r['detail']:
                fh.write('| `%s:%d` | `%s` | `%s` | `%s` | %s | %s |\n' % (
                    f.replace('data/input/', ''), r['line'], r['control'], r['op'], r['spec'].replace('|', '\\|'),
                    r['status'], r['detail'] or '—'))

# Summary to stdout
for f in FILES:
    st = {}
    for r in results[f][0]:
        st[r['status']] = st.get(r['status'], 0) + 1
    print(f, st)
print('controls', len(controls))
print('controls with no binding in main file:', [r['control'] for r in rows if not (r['kbm_default'] or r['xinput_default'] or r['other_default'])])
print('controls with no live consumer:', [r['control'] for r in rows if not r['live_consumers']])
