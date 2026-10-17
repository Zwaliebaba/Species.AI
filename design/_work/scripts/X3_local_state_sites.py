#!/usr/bin/env python3
"""X3 determinism agent: find references to LOCAL (per-peer, non-lockstepped) state from
simulation code. Same comment stripping / #if evaluation / function attribution as
X3_rng_sites.py (copied, scripts are run with python3 -I so no sibling imports).

Usage: python3 -I X3_local_state_sites.py <SRC_ROOT> <MANIFEST_CSV> <OUT_CSV>
"""
import csv, os, re, sys
SRC, MANIFEST, OUT = sys.argv[1:4]
# ---------------------------------------------------------------- baseline macros
DEFINED = {
    'TARGET_PC_FULLGAME', 'TARGET_MSVC', 'WIN32', '_WIN32', '__cplusplus', 'NDEBUG',
    '_CRT_SECURE_NO_DEPRECATE',
    'LAN_PLAY_ENABLED', 'WAN_PLAY_ENABLED', 'NETWORK_STATS_ENABLED',
    'INCLUDEGAMEMODE_DOMINATION', 'INCLUDEGAMEMODE_KOTH', 'INCLUDEGAMEMODE_CTS',
    'INCLUDEGAMEMODE_ROCKET', 'INCLUDEGAMEMODE_ASSAULT', 'INCLUDEGAMEMODE_BLITZ',
    'INCLUDE_CRATES_BASIC', 'INCLUDE_CRATES_ADVANCED', 'INCLUDE_TUTORIAL',
    'BLOCK_OLD_DARWINIA_OBJECTS', 'DUMP_DEBUG_LOG', 'DEBUG_RENDER_ENABLED',
    'FLOAT_NUMERICS', 'USE_CRASHREPORTING', 'HAVE_DSOUND', 'INCLUDE_THEBEAST',
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
}

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


DEF_FILES = set()
PATTERNS = [
    ('m_myTeamId', r'\bm_myTeamId\b'),
    ('GetMyTeam', r'\bGetMyTeam\s*\(|\bGetMyTaskManager\s*\('),
    ('camera', r'g_app->m_camera\b'),
    ('input', r'\bg_inputManager\b|\bg_target\b'),
    ('wallclock', r'\bGetHighResTime\s*\(|\bg_gameTime\b|\bg_advanceTime\b|\btime\s*\(\s*NULL\s*\)|GetAccurateTime\s*\('),
    ('prediction', r'\bg_predictionTime\b|GetPredictionTime\s*\('),
    ('prefs', r'\bg_prefsManager\b'),
    ('renderer', r'g_app->m_renderer\b'),
    ('ui', r'\bEclGetWindow\s*\(|m_taskManagerInterface\b|\bg_app->m_gameCursor\b|m_markerSystem\b|m_helpSystem\b|m_multiwiniaHelp\b'),
    ('pointer-order', r'std::(?:map|set|unordered_map|unordered_set)\s*<\s*[A-Za-z_]\w*\s*\*'),
    ('static-local', r'^\s*static\s+(?:double|float|int|bool|unsigned|Vector3|LList|DArray|std::)'),
    ('raw-transcendental', r'(?<![\w.>])(?:sin|cos|tan|asin|acos|atan|atan2|exp|log|log10|pow|sinf|cosf|tanf|asinf|acosf|atanf|atan2f|expf|logf|powf)\s*\('),
    ('raw-sqrt', r'(?<![\w.>])(?:sqrt|sqrtf)\s*\('),
    ('fmod', r'(?<![\w.>])(?:fmod|fmodf)\s*\('),
    ('float-decl', r'(?<![\w])float\s+[A-Za-z_]\w*\s*[=;,\[]'),
]
SIMFILES = re.compile(r'^code/(worldobject/|location\.cpp|team\.cpp|unit\.cpp|taskmanager\.cpp|multiwinia\.cpp|routing_system\.cpp|entity_grid|obstruction_grid|gametimer\.cpp|global_world\.cpp|landscape\.cpp|level_file\.cpp|location_input\.cpp|global_research|achievement_tracker\.cpp|explosion\.cpp|particle_system\.cpp|clouds\.cpp|water\.cpp|camera\.cpp|network/)')

compiled = {}
with open(MANIFEST, newline='') as fh:
    for row in csv.DictReader(fh):
        compiled[row['path']] = row.get('vcproj_build', '')

rows = []
for dp, dn, fns in os.walk(os.path.join(SRC, 'code')):
    dn.sort()
    for fn in sorted(fns):
        if not fn.endswith('.cpp'):
            continue
        full = os.path.join(dp, fn)
        rel = os.path.relpath(full, SRC).replace(os.sep, '/')
        if not SIMFILES.match(rel) or not compiled.get(rel):
            continue
        raw = open(full, encoding='latin-1').read()
        text = strip_comments(raw)
        lines = text.split('\n'); rawlines = raw.split('\n')
        stack = []; func = ''
        for ln, line in enumerate(lines, 1):
            s = line.strip()
            if s.startswith('#'):
                d = s[1:].strip(); kw = d.split(None, 1)[0] if d else ''; rest = d[len(kw):].strip()
                if kw in ('if', 'ifdef', 'ifndef'):
                    expr = ('defined(%s)' % rest.split()[0]) if kw == 'ifdef' else ('!defined(%s)' % rest.split()[0]) if kw == 'ifndef' else rest
                    val, ment = eval_cond(expr)
                    tags = {TAGS[m] for m in ment if m in TAGS}
                    if expr.strip() == '0': tags.add('DEAD')
                    stack.append([val, tags, bool(val), expr])
                elif kw == 'elif' and stack:
                    val, ment = eval_cond(rest); top = stack[-1]
                    top[1] |= {TAGS[m] for m in ment if m in TAGS}
                    top[0] = (not top[2]) and val if val is not None else None
                    top[2] = top[2] or bool(val)
                elif kw == 'else' and stack:
                    top = stack[-1]; top[0] = (not top[2]) if top[0] is not None or top[2] else None; top[2] = True
                elif kw == 'endif' and stack:
                    stack.pop()
                continue
            m = FUNC_RE.match(line)
            if m and not line.startswith((' ', '\t')) and m.group(1) not in KEYWORDS and not s.endswith(';'):
                func = m.group(1)
            active = all(e[0] for e in stack) if stack else True
            if not active:
                continue
            for name, pat in PATTERNS:
                if re.search(pat, line):
                    rows.append({'file': rel, 'line': ln, 'function': func, 'symbol': name,
                                 'context': classify(rel, func), 'statement': rawlines[ln-1].strip()[:140]})
with open(OUT, 'w', newline='') as fh:
    w = csv.DictWriter(fh, fieldnames=['file', 'line', 'function', 'symbol', 'context', 'statement'])
    w.writeheader(); w.writerows(rows)
from collections import Counter
print(len(rows), Counter((r['symbol'], r['context']) for r in rows).most_common(60))
