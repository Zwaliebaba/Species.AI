#!/usr/bin/env python3
"""X2a asset catalogue (visual assets) for the Multiwinia design recovery.

Usage:  python3 -I X2a_catalogue.py <SRC multiwinia root> <OUT/_work dir> <scratch dir>

Reads (never writes) SRC.  Writes:
  <OUT>/drafts/X2a_textures.csv      one row per bitmap / shader / anim / psp file in scope
  <OUT>/drafts/X2a_shapes.csv        one row per .shp / .max file
  <OUT>/drafts/X2a_refs.csv          every resolved reference (asset, kind, site, condition)
  <OUT>/drafts/X2a_missing.csv       asset-like names referenced by code/data that have no file
  <OUT>/coverage/X2a_asset_catalogue_visual.csv
  <scratch>/X2a_generated_tables.md  markdown per-file tables (merged into the draft)
  <scratch>/X2a_summary.json         aggregate numbers used in the prose

Header parsing follows code/lib/bitmap.cpp:BitmapRGBA::LoadBmp; the .shp parser follows
code/lib/shape.cpp (Shape::Load, ShapeFragment ctor, ParseStripBlock, ParseTriangleBlock,
ShapeMarker ctor) with the TextReader separator set " \\t\\n\\r:," (text_stream_readers.cpp).
"""
import sys, os, re, csv, json, struct
from collections import defaultdict, OrderedDict

SRC, OUT, SCR = sys.argv[1], sys.argv[2], sys.argv[3]
DATA = os.path.join(SRC, 'data')

SCOPE_DIRS = ['textures', 'sprites', 'shapes', 'animations', 'terrain', 'icons',
              'thumbnails', 'mwhelp', 'shaders', 'sepulveda']

# ----------------------------------------------------------------------------------
# 1. Inventory
# ----------------------------------------------------------------------------------
assets = OrderedDict()          # rel path (as on disk, 'data/...') -> dict
for d in SCOPE_DIRS:
    base = os.path.join(DATA, d)
    for root, dirs, files in os.walk(base):
        dirs.sort()
        for f in sorted(files, key=str.lower):
            p = os.path.join(root, f)
            rel = os.path.relpath(p, SRC).replace(os.sep, '/')
            if d == 'sepulveda' and not f.lower().endswith('.bmp'):
                continue
            assets[rel] = {'path': rel, 'abs': p, 'dir': d, 'bytes': os.path.getsize(p),
                           'ext': f.rsplit('.', 1)[-1].lower() if '.' in f else ''}

def res_name(rel):
    """Resource-system name: path below data/, lower-cased (Resource lowercases)."""
    return rel[len('data/'):].lower()

by_resname = {res_name(r): r for r in assets}

# ----------------------------------------------------------------------------------
# 2. BMP header + pixel statistics (bitmap.cpp semantics)
# ----------------------------------------------------------------------------------
def pow2(n):
    return n > 0 and (n & (n - 1)) == 0

def next_pow2(n):
    p = 1
    while p < n:
        p <<= 1
    return p

def bmp_info(path):
    b = open(path, 'rb').read()
    r = {'fmt': 'bmp'}
    if len(b) < 26 or b[:2] != b'BM':
        r['error'] = 'not BM'
        return r
    bfOffBits = struct.unpack_from('<I', b, 10)[0]
    biSize = struct.unpack_from('<I', b, 14)[0]
    r['hdr'] = biSize
    issues = []
    if biSize == 12:
        w, h, planes, bpp = struct.unpack_from('<HHHH', b, 18)
        comp = 0
        ncol = (bfOffBits - 26) // 3
        palstart, palstride = 26, 3
    else:
        w, h, planes, bpp, comp = struct.unpack_from('<iiHHI', b, 18)
        if biSize != 40:
            issues.append('biSize=%d (loader accepts only 40/12: AppDebugAssert, garbage in release)' % biSize)
        ncol = (bfOffBits - 54) // 4
        palstart, palstride = 14 + biSize, 4
        if biSize == 40 and (bfOffBits - 54) % 4:
            issues.append('bfOffBits not palette-aligned')
    r.update(w=w, h=abs(h), bpp=bpp, comp=comp, ncol=max(ncol, 0), off=bfOffBits)
    if h < 0:
        issues.append('top-down (negative height) unsupported')
    if comp != 0:
        issues.append('compression=%d (loader requires BI_RGB)' % comp)
    if bpp not in (4, 8, 24, 32):
        issues.append('bpp %d unsupported' % bpp)
    h = abs(h)
    # The loader reads the palette straight after the header and pixel data straight after the
    # palette (it never seeks to bfOffBits); row padding is skipped only for 24-bit.
    if bpp == 4 and (w % 8):
        issues.append('4-bit width %d not multiple of 8: loader ignores row padding -> rows shear' % w)
    if bpp == 8 and (w % 4):
        issues.append('8-bit width %d not multiple of 4: loader ignores row padding -> rows shear' % w)
    if bpp == 4 and (w % 2):
        issues.append('odd 4-bit width')
    pal = []
    for i in range(min(max(ncol, 0), 256)):
        o = palstart + i * palstride
        if o + 3 > len(b):
            break
        pal.append((b[o + 2], b[o + 1], b[o]))
    # pixel decode (as stored; rows bottom-up)
    data_off = palstart + max(ncol, 0) * palstride  # what the loader does
    if data_off != bfOffBits:
        issues.append('loader pixel offset %d != bfOffBits %d' % (data_off, bfOffBits))
    off = bfOffBits
    pink = 0; black = 0; npx = w * h; amin = 255; amax = 0; acount_partial = 0; azero = 0
    lum_sum = 0; gsum_vs_lum = 0
    colours_used = set()
    try:
        if bpp in (4, 8):
            stride = ((w * bpp + 31) // 32) * 4
            counts = [0] * 256
            for y in range(h):
                row = b[off + y * stride: off + y * stride + (w * bpp + 7) // 8]
                if bpp == 8:
                    for v in row:
                        counts[v] += 1
                else:
                    for v in row:
                        counts[v >> 4] += 1
                        counts[v & 15] += 1
                    if w % 2:
                        counts[row[-1] & 15] -= 1
            for i, c in enumerate(counts):
                if not c:
                    continue
                rgb = pal[i] if i < len(pal) else (0, 0, 0)
                colours_used.add(rgb)
                if rgb == (255, 0, 255):
                    pink += c
                if rgb == (0, 0, 0):
                    black += c
                lum_sum += c * (rgb[0] + rgb[1] + rgb[2]) / 3.0
            r['palette_used'] = sum(1 for c in counts if c)
        elif bpp in (24, 32):
            bpx = bpp // 8
            stride = ((w * bpp + 31) // 32) * 4
            for y in range(h):
                row = b[off + y * stride: off + y * stride + w * bpx]
                B = row[0::bpx]; G = row[1::bpx]; R = row[2::bpx]
                lum_sum += (sum(B) + sum(G) + sum(R)) / 3.0
                # pink / black counts
                for i in range(0, len(R)):
                    rr = R[i]; gg = G[i]; bb = B[i]
                    if rr == 255 and gg == 0 and bb == 255:
                        pink += 1
                    elif rr == 0 and gg == 0 and bb == 0:
                        black += 1
                if bpp == 32:
                    A = row[3::bpx]
                    if A:
                        amin = min(amin, min(A)); amax = max(amax, max(A))
                        azero += A.count(0)
                        acount_partial += sum(1 for a in A if 0 < a < 255)
    except Exception as e:  # pragma: no cover
        issues.append('decode error %s' % e)
    r['pink'] = pink
    r['pink_frac'] = pink / npx if npx else 0
    r['black_frac'] = black / npx if npx else 0
    r['mean_lum'] = lum_sum / npx if npx else 0
    if bpp == 32:
        if amin == 255:
            r['alpha'] = 'A8 present, all 255 (opaque)'
        elif amax == 0:
            r['alpha'] = 'A8 present, all 0'
        else:
            r['alpha'] = 'A8 non-trivial (min %d max %d, %d%% zero, %d%% partial)' % (
                amin, amax, round(100 * azero / npx), round(100 * acount_partial / npx))
    else:
        r['alpha'] = 'none in file'
    r['pow2'] = 'yes' if pow2(w) and pow2(h) else 'no (GL upload rounds up to %dx%d when NPOT unsupported/ManuallyScaleTextures; D3D always)' % (next_pow2(w), next_pow2(h))
    r['issues'] = issues
    return r

# ----------------------------------------------------------------------------------
# 3. SHP parser (shape.cpp semantics)
# ----------------------------------------------------------------------------------
SEP = set(' \t\n\r:,')

def tokens(line):
    out = []; cur = []
    for ch in line:
        if ch in SEP:
            if cur:
                out.append(''.join(cur)); cur = []
        else:
            cur.append(ch)
    if cur:
        out.append(''.join(cur))
    return out

def shp_info(path):
    lines = open(path, encoding='latin-1').read().split('\n')
    n = len(lines); i = 0
    frags = []; markers = []; problems = []
    def nextline():
        nonlocal i
        if i >= n:
            return None
        l = lines[i]; i += 1
        return tokens(l)
    while i < n:
        t = nextline()
        if not t:
            continue
        kw = t[0].lower()
        if kw == 'fragment':
            name = t[1] if len(t) > 1 else 'unknown'
            fr = {'name': name, 'parent': None, 'pos': 0, 'col': 0, 'vert': 0, 'tri': 0,
                  'strips': 0, 'fmt': '', 'norm': 0}
            while True:
                t = nextline()
                if t is None:
                    break
                if not t:
                    continue
                k = t[0].lower(); a = t[1] if len(t) > 1 else None
                if k == 'parentname':
                    fr['parent'] = a
                elif k == 'positions':
                    cnt = int(a); fr['pos'] = cnt; exp = 0
                    while exp < cnt:
                        tt = nextline()
                        if tt is None:
                            problems.append('EOF in positions'); break
                        if tt and tt[0][:1].isdigit():
                            if int(tt[0]) != exp:
                                problems.append('%s: position id mismatch -> block aborted (positions discarded)' % name)
                                fr['pos'] = 0
                                break
                            exp += 1
                elif k in ('normals', 'colours', 'vertices'):
                    cnt = int(a); exp = 0
                    fr[{'normals': 'norm', 'colours': 'col', 'vertices': 'vert'}[k]] = cnt
                    while exp < cnt:
                        tt = nextline()
                        if tt is None:
                            problems.append('EOF in %s' % k); break
                        if tt and tt[0][:1].isdigit():
                            exp += 1
                elif k == 'strips':
                    ns = int(a); fr['strips'] = ns; fr['fmt'] = 'strips'; expected = 0
                    while expected < ns:
                        tt = nextline()
                        if tt is None:
                            break
                        if tt and tt[0].lower() == 'strip':
                            # ParseStripBlock
                            t2 = nextline()
                            if t2 and t2[0].lower() == 'material':
                                t2 = nextline()
                            nv = int(t2[1])
                            got = 0; v1 = v2 = -1
                            while got < nv:
                                t3 = nextline()
                                if t3 is None:
                                    break
                                for tok in t3:
                                    v3 = int(tok[1:])
                                    if got >= 2 and v1 != v2 and v2 != v3 and v1 != v3:
                                        fr['tri'] += 1
                                    v1, v2 = v2, v3
                                    got += 1
                            expected += 1
                    break
                elif k == 'triangles':
                    nt = int(a); fr['fmt'] = 'triangles'; got = 0
                    while got < nt:
                        tt = nextline()
                        if tt is None:
                            break
                        got += 1
                    fr['tri'] = nt
                    break
            frags.append(fr)
        elif kw == 'marker':
            name = t[1] if len(t) > 1 else '?'
            mk = {'name': name, 'parent': None, 'depth': None}
            while True:
                tt = nextline()
                if tt is None:
                    break
                if not tt:
                    continue
                k = tt[0].lower()
                if k == 'parentname':
                    mk['parent'] = tt[1] if len(tt) > 1 else None
                elif k == 'depth':
                    mk['depth'] = int(tt[1])
                elif k == 'markerend':
                    break
            markers.append(mk)
    # hierarchy checks (Shape::Load)
    names = {f['name'].lower(): f for f in frags}
    if len(names) != len(frags):
        problems.append('duplicate fragment names (debug assert)')
    for f in frags:
        p = (f['parent'] or 'unknown')
        if p.lower() != 'sceneroot' and p.lower() not in names:
            problems.append('fragment %s parent %s not found (debug assert)' % (f['name'], p))
    def depth_of(fname):
        d = 1; cur = names.get(fname.lower()); guard = 0
        while cur and (cur['parent'] or '').lower() != 'sceneroot' and guard < 200:
            cur = names.get((cur['parent'] or '').lower()); d += 1; guard += 1
        return d
    for m in markers:
        pn = (m['parent'] or '')
        if pn.lower() == 'sceneroot':
            exp = 1
        elif pn.lower() in names:
            exp = depth_of(pn) + 1
        else:
            problems.append('marker %s parent %s not found' % (m['name'], pn)); continue
        if m['depth'] != exp:
            problems.append('marker %s Depth %s != computed %d' % (m['name'], m['depth'], exp))
    if len(frags) > 100:
        problems.append('>100 fragments')
    if len(markers) > 100:
        problems.append('>100 markers')
    tree = []
    for f in frags:
        tree.append('%s<%s' % (f['name'], f['parent'] or '?'))
    md = max([depth_of(f['name']) for f in frags] or [0])
    return {
        'fragments': len(frags), 'max_depth': md,
        'positions': sum(f['pos'] for f in frags), 'vertices': sum(f['vert'] for f in frags),
        'colours': sum(f['col'] for f in frags), 'triangles': sum(f['tri'] for f in frags),
        'format': '+'.join(sorted({f['fmt'] for f in frags if f['fmt']})) or 'none',
        'hierarchy': '; '.join(tree), 'markers': len(markers),
        'marker_names': '; '.join('%s@%s' % (m['name'], m['parent']) for m in markers),
        'problems': problems, 'frag_list': frags, 'marker_list': markers,
    }

# ----------------------------------------------------------------------------------
# 4. ANIM parser (code/animatedpanel.cpp:AnimatedPanel::Read / AnimatedPanelKeyframe::Read)
# ----------------------------------------------------------------------------------
def anim_info(path):
    txt = open(path, encoding='latin-1').read()
    imgs = re.findall(r'IMAGENAME\s+(\S+)', txt)
    w = re.search(r'WIDTH\s+(\S+)', txt); h = re.search(r'HEIGHT\s+(\S+)', txt)
    st = re.search(r'START\s+(\S+)', txt); sp = re.search(r'STOP\s+(\S+)', txt)
    return {'fmt': 'anim', 'w': w.group(1) if w else '', 'h': h.group(1) if h else '',
            'start': st.group(1) if st else '', 'stop': sp.group(1) if sp else '',
            'objects': len(re.findall(r'BEGIN\s+Object', txt)),
            'keyframes': len(re.findall(r'BEGIN\s+Keyframe', txt)),
            'images': sorted(set(i.lower() for i in imgs))}

for rel, a in assets.items():
    if a['ext'] == 'bmp':
        a['info'] = bmp_info(a['abs'])
    elif a['ext'] == 'shp':
        a['info'] = shp_info(a['abs'])
    elif a['ext'] == 'anim':
        a['info'] = anim_info(a['abs'])
    else:
        a['info'] = {'fmt': a['ext']}

json.dump({r: {k: v for k, v in a['info'].items() if k not in ('frag_list', 'marker_list')}
           for r, a in assets.items()},
          open(os.path.join(SCR, 'X2a_info.json'), 'w'), indent=0)
print('inventory', len(assets))

# ----------------------------------------------------------------------------------
# 5. Code scan: string literals with enclosing symbol and #if condition stack
# ----------------------------------------------------------------------------------
manifest = {}
with open(os.path.join(OUT, 'manifest.csv'), newline='') as fh:
    for row in csv.DictReader(fh):
        manifest[row['path']] = row['vcproj_build']

BASE_DEFS = {'TARGET_PC_FULLGAME': 1, 'TARGET_MSVC': 1, 'WIN32': 1, '_WIN32': 1, 'NDEBUG': 1,
             'LAN_PLAY_ENABLED': 1, 'WAN_PLAY_ENABLED': 1, 'NETWORK_STATS_ENABLED': 1,
             'INCLUDEGAMEMODE_DOMINATION': 1, 'INCLUDEGAMEMODE_KOTH': 1, 'INCLUDEGAMEMODE_CTS': 1,
             'INCLUDEGAMEMODE_ROCKET': 1, 'INCLUDEGAMEMODE_ASSAULT': 1, 'INCLUDEGAMEMODE_BLITZ': 1,
             'INCLUDE_CRATES_BASIC': 1, 'INCLUDE_CRATES_ADVANCED': 1, 'INCLUDE_TUTORIAL': 1,
             'AUTHENTICATION_LEVEL': 1, 'BLOCK_OLD_DARWINIA_OBJECTS': 1, 'DUMP_DEBUG_LOG': 1,
             'DEBUG_RENDER_ENABLED': 1, 'FLOAT_NUMERICS': 1, 'USE_CRASHREPORTING': 1,
             'USE_DISPLAY_LISTS': 1, 'HAVE_DSOUND': 1, 'HAVE_REFRESH_RATES': 1,
             'USE_PIXEL_EFFECT_GRID_OPTIMISATION': 1, 'STRETCH_RECT': 1, 'TEXTURE_EXTENSION': 1,
             '_MSC_VER': 1400, 'TARGET_OS_WINDOWS': 0}
D3D_DEFS = dict(BASE_DEFS, USE_DIRECT3D=1, _CRT_NON_CONFORMING_SWPRINTFS=1)
DEBUG_DEFS = dict(BASE_DEFS, TARGET_DEBUG=1, CHEATMENU_ENABLED=1, SOUND_EDITOR=1, GESTURE_EDITOR=1,
                  INCLUDEGAMEMODE_TANKBATTLE=1, INCLUDEGAMEMODE_PROLOGUE=1, INCLUDEGAMEMODE_CAMPAIGN=1,
                  INCLUDE_CRATE_HELP_WINDOW=1, INCLUDE_THEBEAST=1, TRACK_SYNC_RAND=1, AVI_GENERATOR=1)
del DEBUG_DEFS['TARGET_PC_FULLGAME']
EDITOR_DEFS = dict(BASE_DEFS, LOCATION_EDITOR=1)

def eval_cond(expr, defs):
    e = expr.split('//')[0].split('/*')[0].strip()
    e = re.sub(r'defined\s*\(\s*(\w+)\s*\)', lambda m: '1' if m.group(1) in defs else '0', e)
    e = re.sub(r'defined\s+(\w+)', lambda m: '1' if m.group(1) in defs else '0', e)
    e = e.replace('&&', ' and ').replace('||', ' or ')
    e = re.sub(r'!(?!=)', ' not ', e)
    def ident(m):
        w = m.group(0)
        if w in ('and', 'or', 'not'):
            return w
        return str(defs.get(w, 0))
    e = re.sub(r'\b[A-Za-z_]\w*\b', ident, e)
    e = re.sub(r'(\d+)[uUlL]+', r'\1', e)
    try:
        return bool(eval(e, {'__builtins__': {}}, {}))
    except Exception:
        return None

FUNC_RE = re.compile(r'^\s*(?:[\w:<>\*&~,]+\s+)*?([~\w]+(?:::[~\w]+)*)\s*\(([^;]*)$')
CTRL = {'if', 'for', 'while', 'switch', 'return', 'else', 'sizeof', 'case', 'do', 'catch',
        'defined', 'delete', 'new', 'throw', 'sprintf', 'strcpy', 'strcat'}
LIT_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')

def scan_code_file(path, rel):
    """Return list of (line_no, literal, symbol, cond_text, live{FULL,D3D,DEBUG,EDITOR}, commented, linetext)."""
    out = []
    try:
        lines = open(path, encoding='latin-1').read().split('\n')
    except Exception:
        return out
    stack = []          # list of [cond_text, {cfg: active_bool}, taken{cfg}]
    cfgs = {'FULL': BASE_DEFS, 'D3D': D3D_DEFS, 'DEBUG': DEBUG_DEFS, 'EDITOR': EDITOR_DEFS}
    in_block = False
    cur_func = ''; cur_class = ''; depth = 0; func_depth = None; pending = None; class_pending = False
    for ln, raw in enumerate(lines, 1):
        line = raw
        s = line.strip()
        if not in_block and s.startswith('#'):
            m = re.match(r'#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', s)
            if m:
                kw, rest = m.group(1), m.group(2).strip()
                if kw in ('ifdef', 'ifndef', 'if'):
                    if kw == 'ifdef':
                        txt = rest.split()[0] if rest else ''
                        vals = {c: (txt in d) for c, d in cfgs.items()}
                    elif kw == 'ifndef':
                        txt = '!' + (rest.split()[0] if rest else '')
                        vals = {c: (txt[1:] not in d) for c, d in cfgs.items()}
                    else:
                        txt = rest
                        vals = {c: eval_cond(rest, d) for c, d in cfgs.items()}
                    stack.append([txt, vals, dict(vals)])
                elif kw == 'elif' and stack:
                    top = stack[-1]
                    vals = {}
                    for c, d in cfgs.items():
                        v = eval_cond(rest, d)
                        vals[c] = (not top[2][c]) and v if v is not None else None
                        if vals[c]:
                            top[2][c] = True
                    top[0] = 'elif ' + rest; top[1] = vals
                elif kw == 'else' and stack:
                    top = stack[-1]
                    top[1] = {c: (None if top[2][c] is None else not top[2][c]) for c in cfgs}
                    top[0] = 'else of ' + top[0]
                elif kw == 'endif' and stack:
                    stack.pop()
            continue
        # strip comments, keep literal positions
        code_part = []; comment_flag = False; i = 0; L = len(line); instr = False
        while i < L:
            ch = line[i]
            if in_block:
                if line.startswith('*/', i):
                    in_block = False; i += 2; continue
                comment_flag = True; i += 1; continue
            if instr:
                code_part.append(ch)
                if ch == '\\' and i + 1 < L:
                    code_part.append(line[i + 1]); i += 2; continue
                if ch == '"':
                    instr = False
                i += 1; continue
            if ch == '"':
                instr = True; code_part.append(ch); i += 1; continue
            if ch == "'" and i + 2 < L:
                j = line.find("'", i + 1 if line[i + 1] != '\\' else i + 3)
                if j > 0:
                    code_part.append(line[i:j + 1]); i = j + 1; continue
            if line.startswith('//', i):
                rest = line[i + 2:]
                if '"' in rest:
                    for lit in LIT_RE.findall(rest.replace('" TEXTURE_EXTENSION', 'bmp"')):
                        out.append((ln, lit, cur_func if func_depth is not None else (pending or cur_class or ''), ' && '.join(x[0] for x in stack),
                                    {c: all(x[1][c] for x in stack) for c in cfgs}, True, raw.strip()))
                break
            if line.startswith('/*', i):
                in_block = True; i += 2; continue
            code_part.append(ch); i += 1
        code = ''.join(code_part)
        # function tracking (heuristic): a header line (name + '(' , no ';') arms `pending`;
        # the next '{' opened while not inside a function makes it the current symbol.
        cs = code.strip()
        mcls = re.match(r'^\s*(?:class|struct)\s+(\w+)(?!.*;)', code)
        if mcls and func_depth is None:
            cur_class = mcls.group(1); pending = None; class_pending = True
        if func_depth is None:
            mf = FUNC_RE.match(code)
            if mf and mf.group(1).split('::')[-1] not in CTRL and not cs.endswith(';') \
                    and not cs.startswith(('return', 'else', '#')) and '=' not in code.split('(')[0]:
                name = mf.group(1)
                if '::' not in name and cur_class and rel.endswith('.h'):
                    name = cur_class + '::' + name
                pending = name; class_pending = False
            elif cs.endswith(';'):
                pending = None
        for ch in code:
            if ch == '{':
                if func_depth is None and pending and not class_pending:
                    cur_func = pending; func_depth = depth; pending = None
                elif class_pending:
                    class_pending = False
                depth += 1
            elif ch == '}':
                depth -= 1
                if func_depth is not None and depth <= func_depth:
                    func_depth = None
        code2 = re.sub(r'"\s*TEXTURE_EXTENSION', 'bmp"', code)
        for lit in LIT_RE.findall(code2):
            out.append((ln, lit, cur_func if func_depth is not None else (pending or cur_class or ''),
                        ' && '.join(x[0] for x in stack),
                        {c: all(x[1][c] for x in stack) for c in cfgs}, comment_flag and False, raw.strip()))
    return out

code_lits = []      # dicts
CODE_ROOTS = ['code', 'contrib/eclipse']
for cr in CODE_ROOTS:
    for root, dirs, files in os.walk(os.path.join(SRC, cr)):
        dirs.sort()
        for f in sorted(files):
            if not f.endswith(('.cpp', '.h', '.inc')):
                continue
            p = os.path.join(root, f)
            rel = os.path.relpath(p, SRC).replace(os.sep, '/')
            built = manifest.get(rel, '?')
            for (ln, lit, sym, cond, live, commented, txt) in scan_code_file(p, rel):
                code_lits.append({'file': rel, 'line': ln, 'lit': lit, 'sym': sym, 'cond': cond,
                                  'live': live, 'commented': commented, 'text': txt,
                                  'built': built})
print('code literals', len(code_lits))

if os.environ.get('X2A_DEBUG_LITS'):
    pat = os.environ['X2A_DEBUG_LITS'].lower()
    for c in code_lits:
        if pat in c['lit'].lower():
            print(c['file'], c['line'], c['sym'], '|', c['cond'], c['live'], c['commented'], c['built'], '|', c['lit'])
    sys.exit(0)
