#!/usr/bin/env python3
"""X3 determinism agent: enumerate every random-number call site in the live code.

Usage: python3 -I X3_rng_sites.py <SRC_ROOT> <MANIFEST_CSV> <OUT_SYNCED_CSV> <OUT_UNSYNCED_CSV>

- Strips comments (keeps line numbers), evaluates #if/#ifdef nesting against the
  baseline macro set (Release|Win32 + TARGET_PC_FULLGAME), and records for every call:
  file, line, enclosing function, generator, argument text, build tag, compiled flag,
  heuristic context class and a heuristic "decides" target (assignment LHS / condition).
- Synced generators: syncrand, syncfrand, syncsfrand (+ underscored/Debug forms),
  FRAND/SFRAND (Vector3 wrappers -> _syncfrand/_syncsfrand), RandomNormalNumber,
  RandomApplyVariance (both draw syncfrand).
- Unsynced generators: frand, sfrand, AppRandom, AppSeedRandom, CRT rand/srand.
"""
import csv
import os
import re
import sys

SRC, MANIFEST, OUT_SYNC, OUT_UNSYNC = sys.argv[1:5]

# ---------------------------------------------------------------- baseline macros
DEFINED = {
    'TARGET_PC_FULLGAME', 'TARGET_MSVC', 'WIN32', '_WIN32', '__cplusplus', 'NDEBUG',
    '_CRT_SECURE_NO_DEPRECATE',
    'LAN_PLAY_ENABLED', 'WAN_PLAY_ENABLED', 'NETWORK_STATS_ENABLED',
    'INCLUDEGAMEMODE_DOMINATION', 'INCLUDEGAMEMODE_KOTH', 'INCLUDEGAMEMODE_CTS',
    'INCLUDEGAMEMODE_ROCKET', 'INCLUDEGAMEMODE_ASSAULT', 'INCLUDEGAMEMODE_BLITZ',
    'INCLUDE_CRATES_BASIC', 'INCLUDE_CRATES_ADVANCED', 'INCLUDE_TUTORIAL',
    'BLOCK_OLD_DARWINIA_OBJECTS', 'DUMP_DEBUG_LOG', 'DEBUG_RENDER_ENABLED',
    'FLOAT_NUMERICS', 'USE_CRASHREPORTING', 'HAVE_DSOUND',
    'USE_DISPLAY_LISTS', 'AUTHENTICATION_LEVEL', 'HAVE_REFRESH_RATES',
    'USE_PIXEL_EFFECT_GRID_OPTIMISATION', 'STRETCH_RECT',
    'WIN32_LEAN_AND_MEAN', 'NOMINMAX', 'DARWINIA_GAMETYPE', 'GRACE_TIME',
}
# Macros whose presence marks a build tag when a branch requires them
TAGS = {
    'TRACK_SYNC_RAND': 'DEBUG', 'TARGET_DEBUG': 'DEBUG', 'CHEATMENU_ENABLED': 'DEBUG',
    'SOUND_EDITOR': 'DEBUG', 'GESTURE_EDITOR': 'DEBUG', 'AVI_GENERATOR': 'DEBUG',
    'LOCATION_EDITOR': 'EDITOR', 'USE_DIRECT3D': 'D3D', 'TESTBED_ENABLED': 'DEAD',
    'USE_SEPULVEDA_HELP_TUTORIAL': 'DEAD', 'TARGET_OS_MACOSX': 'DEAD',
    'TARGET_OS_LINUX': 'DEAD', 'USE_LOADERS': 'DEAD', '_XBOX': 'DEAD',
    'INCLUDEGAMEMODE_TANKBATTLE': 'DEAD', 'INCLUDEGAMEMODE_PROLOGUE': 'DEAD',
    'INCLUDEGAMEMODE_CAMPAIGN': 'DEAD', 'TARGET_BETATEST_GROUP': 'DEAD',
    'MULTIWINIA_DEMOONLY': 'DEAD', 'TARGET_MULTIWINIA_DEMOONLY': 'DEAD',
    'INCLUDE_THEBEAST': 'DEBUG',
}
# INCLUDE_THEBEAST is defined in code/worldobject/crate.h only under TARGET_DEBUG.
# Files whose objects are unreachable in Multiwinia (Darwinia-only content), per P05/P06/P07
# drafts and Building::BuildingIsBlocked; 'mixed' = file holds both live and legacy classes.
LEGACY_FILES = {
    'blueprintstore', 'bridge', 'cave', 'chess', 'factory', 'feedingtube', 'generichub',
    'goddish', 'library', 'mine', 'researchitem', 'safearea', 'upgrade_port', 'switch',
    'scripttrigger', 'powerstation', 'constructionyard', 'displayscreen', 'spiritreceiver',
    'spiritstore', 'tripod', 'lander', 'dropship', 'shaman', 'tentacle', 'clonelab',
    'controltower', 'incubator', 'portal', 'global_internet', 'global_world', 'sepulveda',
    'demoendsequence', 'engineer', 'spam',
}
MIXED_FILES = {'generator', 'spam', 'darwinian', 'entity', 'location', 'rocket'}
def liveness(rel):
    base = os.path.splitext(os.path.basename(rel))[0]
    if rel.startswith('code/loaders/'):
        return 'DEAD(USE_LOADERS off)'
    if base in LEGACY_FILES and base not in MIXED_FILES:
        return 'LEGACY'
    if base in MIXED_FILES:
        return 'MIXED'
    return 'LIVE'


def eval_cond(expr):
    """Return (value or None if unknown, set_of_tag_macros_mentioned)."""
    mentioned = set(re.findall(r'[A-Za-z_][A-Za-z0-9_]*', expr))
    e = re.sub(r'defined\s*\(\s*([A-Za-z_]\w*)\s*\)', lambda m: '1' if m.group(1) in DEFINED else '0', expr)
    e = re.sub(r'defined\s+([A-Za-z_]\w*)', lambda m: '1' if m.group(1) in DEFINED else '0', e)
    e = e.replace('&&', ' and ').replace('||', ' or ')
    e = re.sub(r'!(?!=)', ' not ', e)
    def ident(m):
        name = m.group(0)
        if name in ('and', 'or', 'not'):
            return name
        return '1' if name in DEFINED else '0'
    e = re.sub(r'[A-Za-z_][A-Za-z0-9_]*', ident, e)
    try:
        return bool(eval(e, {'__builtins__': {}}, {})), mentioned
    except Exception:
        return None, mentioned

def strip_comments(text):
    out = []
    i, n = 0, len(text)
    in_block = False
    in_str = None
    while i < n:
        c = text[i]
        if in_block:
            if text.startswith('*/', i):
                in_block = False
                i += 2
                continue
            out.append('\n' if c == '\n' else ' ')
            i += 1
            continue
        if in_str:
            out.append(c)
            if c == '\\' and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if c == in_str or c == '\n':
                in_str = None
            i += 1
            continue
        if text.startswith('/*', i):
            in_block = True
            out.append('  ')
            i += 2
            continue
        if text.startswith('//', i):
            j = text.find('\n', i)
            if j == -1:
                j = n
            out.append(' ' * (j - i))
            i = j
            continue
        if c in '"\'':
            in_str = c
        out.append(c)
        i += 1
    return ''.join(out)

FUNC_RE = re.compile(r'^(?:[A-Za-z_][\w<>,\s\*&:]*?\s+[\*&]*)?((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\s*\(')
KEYWORDS = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'else', 'do', 'case', 'catch'}

SYNC_PATTERNS = [
    ('syncrand', r'\bsyncrand\s*\('), ('syncfrand', r'\bsyncfrand\s*\('),
    ('syncsfrand', r'\bsyncsfrand\s*\('), ('_syncrand', r'\b_syncrand\s*\('),
    ('_syncfrand', r'\b_syncfrand\s*\('), ('_syncsfrand', r'\b_syncsfrand\s*\('),
    ('FRAND', r'\bFRAND\s*\('), ('SFRAND', r'\bSFRAND\s*\('),
    ('RandomNormalNumber', r'\bRandomNormalNumber\s*\('),
    ('RandomApplyVariance', r'\bRandomApplyVariance\s*\('),
    ('DebugSyncRand', r'\bDebugSyncRand\s*\('), ('DebugSyncFrand', r'\bDebugSyncFrand\s*\('),
]
UNSYNC_PATTERNS = [
    ('frand', r'(?<![\w.>])frand\s*\('), ('sfrand', r'(?<![\w.>])sfrand\s*\('),
    ('AppRandom', r'\bAppRandom\s*\('), ('AppSeedRandom', r'\bAppSeedRandom\s*\('),
    ('rand', r'(?<![\w.>])rand\s*\('), ('srand', r'(?<![\w.>])srand\s*\('),
]
DEF_FILES = {'contrib/systemiv/lib/math/random_number.cpp', 'contrib/systemiv/lib/math/random_number.h',
             'code/lib/vector3.h', 'code/lib/vector3.cpp',
             'code/lib/math_utils.cpp'}  # math_utils.cpp holds an orphan MT19937 'syncrand' definition

def balanced_arg(s, start):
    depth, j = 0, start
    while j < len(s):
        if s[j] == '(':
            depth += 1
        elif s[j] == ')':
            depth -= 1
            if depth == 0:
                return s[start + 1:j].strip()
        j += 1
    return s[start + 1:].strip()

def classify(path, func):
    f = func or ''
    low = f.lower()
    if path.startswith(('code/UI/', 'code/interface/', 'contrib/eclipse/')):
        return 'ui'
    if path.startswith('code/loaders/'):
        return 'loader'
    if path.startswith('code/sound/'):
        return 'sound'
    if 'render' in low or 'draw' in low or 'renderer' in path:
        return 'render'
    if path.endswith(('particle_system.cpp', 'explosion.cpp')):
        return 'vfx-system'
    if 'advance' in low or 'tick' in low:
        return 'advance'
    if any(k in low for k in ('initialise', '::init', 'read', 'setdetail', 'generate', 'create', 'spawn')):
        return 'init/create'
    if '::' in f:
        cls, meth = f.rsplit('::', 1)
        if cls.split('::')[-1] == meth or meth.startswith('~'):
            return 'init/create'
    return 'other'

def decides(stmt, col):
    pre = stmt[:col]
    m = re.search(r'([\w\.\->\[\]]+)\s*(?:[\+\-\*/]?=)(?!=)[^=;]*$', pre)
    if m:
        return 'assign:' + m.group(1)
    if re.search(r'\b(if|while)\s*\(', pre):
        return 'condition'
    m = re.search(r'(\w+)\s*\([^()]*$', pre)
    if m:
        return 'arg:' + m.group(1)
    if re.search(r'\breturn\b', pre):
        return 'return'
    return ''

compiled = {}
with open(MANIFEST, newline='') as fh:
    for row in csv.DictReader(fh):
        compiled[row['path']] = row.get('vcproj_build', '')

sync_rows, unsync_rows = [], []
roots = ['code', 'contrib/systemiv/lib', 'contrib/eclipse']
for root in roots:
    for dp, dn, fns in os.walk(os.path.join(SRC, root)):
        dn.sort()
        for fn in sorted(fns):
            if not fn.endswith(('.cpp', '.h', '.c', '.inl')):
                continue
            full = os.path.join(dp, fn)
            rel = os.path.relpath(full, SRC).replace(os.sep, '/')
            try:
                raw = open(full, encoding='latin-1').read()
            except OSError:
                continue
            text = strip_comments(raw)
            lines = text.split('\n')
            rawlines = raw.split('\n')
            stack = []  # entries: (active_bool_or_None, tags:set, taken:bool)
            func = ''
            for ln, line in enumerate(lines, 1):
                s = line.strip()
                if s.startswith('#'):
                    d = s[1:].strip()
                    kw = d.split(None, 1)[0] if d else ''
                    rest = d[len(kw):].strip()
                    if kw in ('if', 'ifdef', 'ifndef'):
                        if kw == 'ifdef':
                            expr = 'defined(%s)' % rest.split()[0] if rest else '0'
                        elif kw == 'ifndef':
                            expr = '!defined(%s)' % rest.split()[0] if rest else '1'
                        else:
                            expr = rest
                        val, ment = eval_cond(expr)
                        tags = {TAGS[m] for m in ment if m in TAGS}
                        if expr.strip() == '0':
                            tags.add('DEAD')
                        stack.append([val, tags, bool(val), expr])
                    elif kw == 'elif' and stack:
                        val, ment = eval_cond(rest)
                        top = stack[-1]
                        top[1] = {TAGS[m] for m in ment if m in TAGS} | top[1]
                        top[0] = (not top[2]) and val if val is not None else None
                        top[2] = top[2] or bool(val)
                        top[3] = rest
                    elif kw == 'else' and stack:
                        top = stack[-1]
                        top[0] = (not top[2]) if top[0] is not None or top[2] else None
                        top[2] = True
                        top[3] = '!(' + top[3] + ')'
                    elif kw == 'endif' and stack:
                        stack.pop()
                    continue
                m = FUNC_RE.match(line)
                if m and not line.startswith((' ', '\t')) and m.group(1) not in KEYWORDS and not s.endswith(';'):
                    func = m.group(1)
                active = all(e[0] for e in stack) if stack else True
                unknown = any(e[0] is None for e in stack)
                conds = ' && '.join(e[3] for e in stack)
                if active:
                    build = 'FULL'
                elif unknown:
                    build = 'UNKNOWN'
                else:
                    tagset = set()
                    for e in stack:
                        if not e[0]:
                            tagset |= e[1]
                    build = '|'.join(sorted(tagset)) if tagset else 'DEAD'
                is_comp = 'Y' if compiled.get(rel, '') else 'N'
                for pats, out in ((SYNC_PATTERNS, sync_rows), (UNSYNC_PATTERNS, unsync_rows)):
                    for name, pat in pats:
                        for mm in re.finditer(pat, line):
                            col = mm.start()
                            paren = line.find('(', mm.start())
                            arg = balanced_arg(line, paren)
                            kind = 'call'
                            if rel in DEF_FILES:
                                kind = 'definition/wrapper'
                            if re.match(r'\s*(?:unsigned\s+long|double|int|void)\s+\w+\s*\(', line) and line.strip().endswith(';'):
                                kind = 'declaration'
                            stmt = rawlines[ln - 1].strip() if ln - 1 < len(rawlines) else s
                            win = ' '.join(lines[max(0, ln - 1):min(len(lines), ln + 8)])
                            tgt = decides(line, col)
                            var = tgt.split(':', 1)[1].split('.')[-1].split('->')[-1] if tgt.startswith('assign:') else ''
                            feeds = ''
                            if 'CreateParticle' in win and (not var or re.search(r'CreateParticle[^;]*\b' + re.escape(var) + r'\b', win)):
                                feeds = 'particle?'
                            if 'TriggerEntityEvent' in win or 'TriggerBuildingEvent' in win or 'TriggerOtherEvent' in win:
                                feeds = (feeds + ' sound?').strip()
                            out.append({
                                'file': rel, 'line': ln, 'function': func, 'generator': name,
                                'argument': arg[:80], 'context': classify(rel, func),
                                'decides': decides(line, col), 'build': build,
                                'preproc_conditions': conds[:120], 'compiled': is_comp,
                                'kind': kind, 'statement': stmt[:140],
                                'liveness': liveness(rel), 'cosmetic_hint': feeds,
                            })

FIELDS = ['file', 'line', 'function', 'generator', 'argument', 'context', 'decides',
          'cosmetic_hint', 'liveness', 'build', 'compiled', 'kind', 'preproc_conditions', 'statement']
for path, rows in ((OUT_SYNC, sync_rows), (OUT_UNSYNC, unsync_rows)):
    with open(path, 'w', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=FIELDS)
        w.writeheader()
        for r in rows:
            w.writerow(r)

def summary(rows, label):
    live = [r for r in rows if r['kind'] == 'call' and r['build'] == 'FULL' and r['compiled'] == 'Y']
    print(label, 'total rows', len(rows), 'live calls', len(live))
    from collections import Counter
    print('  by generator', dict(Counter(r['generator'] for r in live)))
    print('  by context', dict(Counter(r['context'] for r in live)))
    print('  by build (all calls)', dict(Counter(r['build'] for r in rows if r['kind'] == 'call')))
    print('  not compiled', len([r for r in rows if r['compiled'] == 'N']))
summary(sync_rows, 'SYNCED')
summary(unsync_rows, 'UNSYNCED')
