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
                    and not cs.startswith(('return', 'else', '#', ':', ',')) and '=' not in code.split('(')[0] \
                    and not mf.group(1).startswith('m_') and not cs.endswith(','):
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

# ----------------------------------------------------------------------------------
# 6. Reference resolution
# ----------------------------------------------------------------------------------
refs = defaultdict(list)     # asset rel -> list of ref dicts
missing = []                 # names that look like assets but have no file
basename_idx = defaultdict(list); stem_idx = defaultdict(list)
for rel in assets:
    rn = res_name(rel); bn = rn.rsplit('/', 1)[-1]
    basename_idx[bn].append(rel); stem_idx[bn.rsplit('.', 1)[0]].append(rel)

file_cache = {}
def flines(rel):
    if rel not in file_cache:
        try:
            file_cache[rel] = open(os.path.join(SRC, rel), encoding='latin-1').read().split('\n')
        except Exception:
            file_cache[rel] = []
    return file_cache[rel]

GLP = re.compile(r'gl(?:TexParameter[if]|BlendFunc|TexEnv[if]|Enable|Disable)\s*\(([^)]*)\)')
def site_state(rel, ln):
    """GL texture/blend state set within the 4 lines before and 14 after a load/bind site [I]."""
    L = flines(rel); st = []
    for k in range(max(0, ln - 5), min(len(L), ln + 14)):
        for m in GLP.finditer(L[k]):
            a = re.sub(r'\s+', '', m.group(1)).replace('GL_TEXTURE_2D,', '').replace('GL_TEXTURE_', '').replace('GL_', '')
            fn = m.group(0).split('(')[0].replace('gl', '')
            if fn in ('Enable', 'Disable') and a not in ('BLEND', 'TEXTURE_2D', 'ALPHA_TEST', 'DEPTH_TEST', 'CULL_FACE'):
                continue
            if fn in ('Enable', 'Disable'):
                st.append('%s(%s)' % (fn, a))
            elif fn.startswith('TexParameter'):
                st.append(a.replace(',', '='))
            elif fn == 'BlendFunc':
                st.append('Blend(%s)' % a)
            else:
                st.append('TexEnv(%s)' % a.split(',')[-1])
    # de-duplicate keeping order
    seen = set(); out = []
    for x in st:
        if x not in seen:
            seen.add(x); out.append(x)
    return ' '.join(out)

CALL_RE = re.compile(r'(GetTextureWithAlpha|GetTextureWithFlags|GetTexture|GetBitmap|DoesTextureExist|GetShapeCopy|GetShape|DoesShapeExist|SetupTeamShape|GetBinaryReader|GetTextReader|Shader::Create|GetTextureInfo|DeleteTexture)\s*\(')
def call_flags(text, lit):
    """Classify the resource call wrapping a literal and its texture flags (Resource::GetTexture defaults
    mip=true, mask=true, compressed=false)."""
    idx = text.find('"' + lit)
    pre = text[:idx] if idx >= 0 else text
    calls = list(CALL_RE.finditer(pre))
    if not calls:
        return '', ''
    c = calls[-1].group(1)
    if c == 'GetTexture':
        after = text[idx + len(lit) + 2:] if idx >= 0 else ''
        m = re.match(r'\s*((?:,\s*[\w:!]+\s*)*)\)', after)
        args = [a.strip() for a in (m.group(1).split(',')[1:] if m else [])]
        mip = args[0] if len(args) > 0 else 'true'; mask = args[1] if len(args) > 1 else 'true'
        comp = args[2] if len(args) > 2 else 'false'
        return c, 'mip=%s mask=%s%s' % (mip, mask, ' compressed=%s' % comp if comp != 'false' else '')
    if c == 'GetTextureWithAlpha':
        return c, 'mip=true alpha=green->A (no mask)'
    return c, ''

def add_ref(asset, kind, where, sym, ln, cond, live, built, how, flags='', state='', strength='C'):
    refs[asset].append({'kind': kind, 'file': where, 'sym': sym, 'line': ln, 'cond': cond,
                        'live': live, 'built': built, 'how': how, 'flags': flags, 'state': state,
                        'strength': strength})

ASSETISH = re.compile(r'^(?:data/)?(?:textures|sprites|shapes|animations|terrain|icons|thumbnails|mwhelp|shaders|sepulveda)/[^%\s]+$|^[\w\-]+\.(?:bmp|shp|anim|vs|ps|ps3)$', re.I)
for c in code_lits:
    L = c['lit'].replace('\\\\', '/').replace('\\', '/').lower()
    if L.startswith('data/'):
        L = L[5:]
    if not L or len(L) > 120:
        continue
    fn, flags = call_flags(c['text'], c['lit'])
    common = dict(where=c['file'], sym=c['sym'], ln=c['line'], cond=c['cond'], live=c['live'],
                  built=c['built'])
    state = ''
    def st():
        return site_state(c['file'], c['line'])
    if c['commented']:
        tgt = by_resname.get(L) or by_resname.get(L + '.bmp')
        if tgt:
            add_ref(tgt, 'commented-out', how='literal in // comment', **common)
        continue
    if L in by_resname:
        add_ref(by_resname[L], 'literal', how=fn or 'literal', flags=flags, state=st(), **common)
    elif L + '.bmp' in by_resname and '/' in L:
        add_ref(by_resname[L + '.bmp'], 'literal+ext', how=(fn or 'literal') + ' (ext appended)', flags=flags, state=st(), **common)
    elif L.startswith('shadow_') and L[7:] in by_resname:
        add_ref(by_resname[L[7:]], 'shadow-derived', how='runtime blurred copy name ' + L, flags=flags, state=st(), **common)
    elif '/' not in L and L in basename_idx and L.endswith('.shp'):
        add_ref(basename_idx[L][0], 'shape-name', how=fn or 'literal shape name', **common)
    elif '/' not in L and L in basename_idx:
        for t in basename_idx[L]:
            add_ref(t, 'basename', how=(fn or 'literal') + ' (bare file name; directory supplied elsewhere)', strength='I', **common)
    elif '/' not in L and L in stem_idx and len(L) >= 3 and re.match(r'^[a-z0-9_\-]+$', L):
        for t in stem_idx[L]:
            if assets[t]['dir'] in ('icons', 'mwhelp', 'thumbnails', 'animations', 'sprites', 'textures', 'terrain', 'shapes'):
                add_ref(t, 'stem', how='bare stem literal', strength='I', **common)
    elif ASSETISH.match(L) and '%' not in L and not L.endswith('/') and '*' not in L:
        missing.append(dict(name=L, **{k: v for k, v in common.items()}, how=fn))
    elif '%' in L and re.search(r'\.(bmp|shp|anim)$|^(textures|sprites|shapes|animations|terrain|icons|thumbnails|mwhelp|shaders|sepulveda)/', L):
        rx = '^' + re.sub(r'%[0-9\.\-]*[sdixXcu]', '(.+)', re.escape(L).replace('\\%', '%')) + '$'
        rx2 = re.compile(rx)
        hits = [r for r in assets if rx2.match(res_name(r)) or rx2.match(res_name(r).rsplit('/', 1)[-1])]
        for t in hits:
            add_ref(t, 'format-pattern', how='printf pattern "%s" (value set: see rule)' % c['lit'], strength='I', **common)
        if not hits:
            missing.append(dict(name=L, **{k: v for k, v in common.items()}, how='pattern matches no file'))

print('refs after literals', sum(len(v) for v in refs.values()), 'missing', len(missing))

# ----------------------------------------------------------------------------------
# 7. Data-file references (basename-with-extension tokens in data/**.txt and .anim)
# ----------------------------------------------------------------------------------
def level_class(rel):
    b = rel.rsplit('/', 1)[-1].lower()
    if rel.startswith('data/levels/old/'):
        return 'dead', 'data/levels/old/ is never listed (ListResources is non-recursive; code/game_menu.cpp:GameMenu::CreateMapList)'
    if rel.startswith('data/levels/mp_'):
        txt = open(os.path.join(SRC, rel), encoding='latin-1').read()
        m = re.search(r'GameTypes\s+(.*)', txt)
        gt = (m.group(1).strip() if m else '')
        if 'TankBattle' in gt:
            return 'debug', 'TankBattle map: GameTypeTankBattle not permitted without INCLUDEGAMEMODE_TANKBATTLE (code/app.cpp:App::IsGameModePermitted)'
        if gt.lower().startswith('none') or not gt:
            return 'special', 'mp map with GameTypes none (sandbox/benchmark/stress; only special targets)'
        return 'live', 'Multiwinia map listed by GameMenu::CreateMapList'
    if rel.startswith('data/levels/'):
        return 'legacy', 'Darwinia campaign map/mission file [B:LEGACY]'
    return 'live', ''

level_info = {}
for root, dirs, files in os.walk(os.path.join(DATA, 'levels')):
    for f in files:
        rel = os.path.relpath(os.path.join(root, f), SRC).replace(os.sep, '/')
        level_info[rel] = level_class(rel)

DATA_SKIP = ('data/language/',)
data_tok = re.compile(r'[A-Za-z0-9_\-\./]+\.(?:bmp|shp|anim)\b', re.I)
for root, dirs, files in os.walk(DATA):
    dirs.sort()
    for f in sorted(files):
        if not f.lower().endswith(('.txt', '.anim')):
            continue
        p = os.path.join(root, f)
        rel = os.path.relpath(p, SRC).replace(os.sep, '/')
        if rel.startswith(DATA_SKIP):
            continue
        for ln, line in enumerate(open(p, encoding='latin-1').read().split('\n'), 1):
            for m in data_tok.finditer(line):
                tok = m.group(0).lower().lstrip('./')
                key = (line.strip().split() or [''])[0]
                cls, why = level_info.get(rel, ('live', ''))
                live = {'FULL': cls == 'live', 'D3D': cls == 'live', 'DEBUG': cls in ('live', 'debug'), 'EDITOR': cls == 'live'}
                tgts = []
                if tok in by_resname:
                    tgts = [by_resname[tok]]
                elif tok.endswith('.shp') and 'shapes/' + tok in by_resname:
                    tgts = [by_resname['shapes/' + tok]]
                elif rel.startswith('data/levels/') and key.lower() in ('landcolourfile', 'wavescolourfile', 'watercolourfile'):
                    if 'terrain/' + tok in by_resname:
                        tgts = [by_resname['terrain/' + tok]]
                elif rel.startswith('data/animations/') and key.upper() == 'IMAGENAME':
                    if 'animations/' + tok in by_resname:
                        tgts = [by_resname['animations/' + tok]]
                    else:
                        missing.append(dict(name='animations/' + tok, where=rel, sym='IMAGENAME', ln=ln, cond='', live=live, built='data', how='anim keyframe image absent'))
                elif tok in basename_idx:
                    for t in basename_idx[tok]:
                        add_ref(t, 'data-basename', rel, key, ln, why, live, 'data', 'bare name in data file (directory from consumer)', strength='I')
                    continue
                for t in tgts:
                    add_ref(t, 'data', rel, key, ln, why, live, 'data', 'data key %s' % key)

# ----------------------------------------------------------------------------------
# 8. Rules: names built at run time (each rule = code site literal + value set) [C]
# ----------------------------------------------------------------------------------
def sites(file_, lit):
    return [c for c in code_lits if c['file'] == file_ and c['lit'] == lit and not c['commented']]

def apply_rule(rule_id, file_, lit, values, fmt, how, flags=''):
    ss = sites(file_, lit)
    if not ss:
        print('WARN rule site not found', rule_id, file_, lit)
    for c in ss:
        for v in values:
            name = (fmt % v).lower()
            tgt = by_resname.get(name)
            if tgt:
                add_ref(tgt, 'rule:' + rule_id, c['file'], c['sym'], c['line'], c['cond'], c['live'], c['built'],
                        how, flags=flags, state=site_state(c['file'], c['line']))
            else:
                missing.append(dict(name=name, where=c['file'], sym=c['sym'], ln=c['line'], cond=c['cond'],
                                    live=c['live'], built=c['built'], how='rule %s value %s' % (rule_id, v)))

def c_array(file_, func_marker, stop='};'):
    txt = open(os.path.join(SRC, file_), encoding='latin-1').read()
    i = txt.index(func_marker); j = txt.index(stop, i)
    return re.findall(r'"([^"]+)"', txt[i:j])

RESEARCH = c_array('code/global_world.cpp', 'char *GlobalResearch::GetTypeName')
crtxt = open(os.path.join(SRC, 'code/worldobject/crate.cpp'), encoding='latin-1').read()
ci = crtxt.index('char *Crate::GetName(int _type)'); cj = crtxt.index('\n}', ci)
CRATES = [m.group(1) for m in re.finditer(r'^\s*case [^:]+:\s*return "([^"]+)";', crtxt[ci:cj], re.M)] + ['Crate']
BLUEPRINTS = re.findall(r'^GAMETYPE\s+(\S+)', open(os.path.join(DATA, 'multiwinia.txt'), encoding='latin-1').read(), re.M)
eng = open(os.path.join(DATA, 'language/english.txt'), encoding='latin-1').read()
HELP_PAGES = sorted({(int(a) - 1, int(b)) for a, b in re.findall(r'^multiwinia_helppage_(\d)_(\d)\b', eng, re.M)})
MP_MAPS = sorted(r for r, (cls, _) in level_info.items() if cls in ('live',) and r.startswith('data/levels/mp_'))
TB_MAPS = sorted(r for r, (cls, _) in level_info.items() if cls == 'debug')

R = []
for f_ in ['code/taskmanager_interface_icons.cpp', 'code/taskmanager_interface_multiwinia.cpp',
           'code/taskmanager_interface_gestures.cpp', 'code/markersystem.cpp', 'code/gamecursor.cpp']:
    apply_rule('research-icon', f_, 'icons/icon_%s.bmp', RESEARCH, 'icons/icon_%s.bmp',
               'icons/icon_<GlobalResearch::GetTypeName(i)|Task::GetTaskName(type)>.bmp')
for f_ in ['code/taskmanager_interface_icons.cpp', 'code/taskmanager_interface_multiwinia.cpp',
           'code/taskmanager_interface_gestures.cpp', 'code/interface/gesture_window.cpp']:
    apply_rule('research-gesture', f_, 'icons/gesture_%s.bmp', RESEARCH, 'icons/gesture_%s.bmp',
               'icons/gesture_<research/task name>.bmp')
apply_rule('crate-icon', 'code/interface/helpandoptions_windows.cpp', 'icons/icon_%s.bmp', CRATES,
           'icons/icon_%s.bmp', 'icons/icon_<Crate::GetName(n)>.bmp (CrateHelpWindow)')
for f_ in ['code/UI/GameMenuWindow.cpp', 'code/UI/GameTypeImageButton.h']:
    apply_rule('mode-image', f_, 'mwhelp/mode_%s.bmp', BLUEPRINTS, 'mwhelp/mode_%s.bmp',
               'mwhelp/mode_<blueprint name from data/multiwinia.txt>.bmp, lower-cased', 'mip=false mask=false')
apply_rule('help-general', 'code/interface/helpandoptions_windows.cpp', 'mwhelp/general%d.bmp', range(1, 6),
           'mwhelp/general%d.bmp', 'GeneralHelpWindow page 1..MAX_PAGES(5) via ImageButton', 'mip=false mask=true')
apply_rule('help-gametype', 'code/interface/helpandoptions_windows.cpp', 'mwhelp/help_%d_%d.bmp', HELP_PAGES,
           'mwhelp/help_%d_%d.bmp', 'GameTypeHelpWindow page (t,p) exists iff phrase multiwinia_helppage_<t+1>_<p> exists', 'mip=false mask=true')
for f_ in ['code/UI/MapImageButton.h', 'code/UI/GameMenuWindow.cpp']:
    stems = [r.rsplit('/', 1)[-1][:-4] for r in MP_MAPS]
    apply_rule('map-thumbnail', f_, 'thumbnails/%s', stems, 'thumbnails/%s.bmp',
               'thumbnails/<level file stem>.bmp lower-cased, for each data/levels/mp_* map with a permitted GameTypes', 'mip=false mask=false')
    stems_tb = [r.rsplit('/', 1)[-1][:-4] for r in TB_MAPS]
    for c in sites(f_, 'thumbnails/%s'):
        for st_ in stems_tb:
            t = by_resname.get(('thumbnails/%s.bmp' % st_).lower())
            if t:
                add_ref(t, 'rule:map-thumbnail-tankbattle', c['file'], c['sym'], c['line'],
                        'runtime: TankBattle not permitted in FULL', {'FULL': False, 'D3D': False, 'DEBUG': True, 'EDITOR': False},
                        c['built'], 'thumbnail of a TankBattle map (game type selectable only in TARGET_DEBUG)', 'mip=false mask=false')
apply_rule('cloud-face', 'code/clouds.cpp', 'sepulveda/sepulveda%d.bmp', range(1, 6), 'sepulveda/sepulveda%d.bmp',
           'Clouds::RenderFace picIndex 1..3 random, 4 eyes shut, 5 mouth open', 'mip=true mask=false (current pic); default flags (fading previous pic)')
apply_rule('darwinian-ouch', 'code/worldobject/darwinian.cpp', 'sprites/darwinian_ouch%d.bmp', range(1, 4),
           'sprites/darwinian_ouch%d.bmp', 'dead airborne Darwinian: index 1 + uid%10 when uid%10 < 3')
apply_rule('armyant-shape', 'code/worldobject/armyant.cpp', 'armyant%d.shp', [2, 3], 'shapes/armyant%d.shp',
           'ArmyAnt::SetShape LOD shapes i=1,2 -> armyant2/3.shp')
# unicode atlases: ListResources("textures/unicode/", "<font>.*.*.bmp") for each TextRenderer::Initialise name
for c in sites('code/lib/text_renderer.cpp', 'textures/unicode/%s'):
    for rel in assets:
        rn = res_name(rel)
        m = re.match(r'textures/unicode/(editor|speccy|square)\.([0-9a-f]+)\.([0-9a-f]+)\.bmp$', rn)
        if m:
            add_ref(rel, 'rule:font-atlas', c['file'], c['sym'], c['line'], c['cond'], c['live'], c['built'],
                    'TextRenderer::BuildUnicodeArray lists textures/unicode/<font>.*.*.bmp for font %s (code/app.cpp:App::SetLanguage)' % m.group(1),
                    flags='mip=true mask=true', state='MIN=LINEAR_MIPMAP_LINEAR MAG=LINEAR (set in DrawText*)')
# animation .anim -> IMAGENAME bitmaps are handled by the data scan; .anim files by literals in multiwiniahelp.cpp
# terrain: level-file keys handled by the data scan; defaults + fallback:
for lit, why in [('landscape_default.bmp', 'LevelFile defaults (level_file.cpp) when no landColourFile'),
                 ('waves_default.bmp', 'LevelFile defaults when no wavesColourFile'),
                 ('water_default.bmp', 'LevelFile defaults when no waterColourFile')]:
    for c in sites('code/level_file.cpp', lit):
        add_ref(by_resname['terrain/' + lit], 'rule:terrain-default', c['file'], c['sym'], c['line'], c['cond'], c['live'], c['built'],
                why + '; consumed as terrain/<name> by LandscapeRenderer/Water')
print('refs after rules', sum(len(v) for v in refs.values()), 'missing', len(missing))

# ----------------------------------------------------------------------------------
# 9. Per-asset status
# ----------------------------------------------------------------------------------
STRONG = ('literal', 'literal+ext', 'shadow-derived', 'shape-name', 'data', 'manual')
def ref_strong(r):
    return r['strength'] == 'C' and (r['kind'] in STRONG or r['kind'].startswith('rule:'))

def ref_build(r):
    """Build tag of a single reference."""
    if r['kind'] == 'commented-out':
        return 'DEAD'
    if r['built'] == '':
        return 'DEAD'
    lv = r['live']
    if lv.get('FULL'):
        return 'FULL'
    if lv.get('D3D'):
        return 'D3D'
    if lv.get('DEBUG'):
        return 'DEBUG'
    if lv.get('EDITOR'):
        return 'EDITOR'
    return 'DEAD'

def summarise(rel):
    rs = refs.get(rel, [])
    strong = [r for r in rs if ref_strong(r)]
    tags = sorted({ref_build(r) for r in strong}, key=['FULL', 'D3D', 'DEBUG', 'EDITOR', 'DEAD'].index)
    return rs, strong, tags

if os.environ.get('X2A_REVIEW'):
    want = os.environ['X2A_REVIEW']
    for rel, a in assets.items():
        rs, strong, tags = summarise(rel)
        if want == 'weak' and (tags and tags[0] == 'FULL'):
            continue
        if want not in ('weak', 'all') and want not in rel:
            continue
        print('##', rel, tags)
        seen = set()
        for r in rs:
            k = (r['kind'], r['file'], r['sym'])
            if k in seen:
                continue
            seen.add(k)
            print('   ', r['strength'], r['kind'], r['file'], r['line'], r['sym'], '|', r['cond'][:60], '|', ref_build(r), '|', r['how'][:70])
    sys.exit(0)
