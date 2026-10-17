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
        code_part = []; comment_flag = False; i = 0; L = len(line); instr = False; blockbuf = []
        while i < L:
            ch = line[i]
            if in_block:
                if line.startswith('*/', i):
                    in_block = False; i += 2; continue
                comment_flag = True; blockbuf.append(ch); i += 1; continue
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
        if blockbuf:
            for lit in LIT_RE.findall(re.sub(r'"\s*TEXTURE_EXTENSION', 'bmp"', ''.join(blockbuf))):
                out.append((ln, lit, cur_func if func_depth is not None else (pending or cur_class or ''),
                            ' && '.join(x[0] for x in stack), {c: all(x[1][c] for x in stack) for c in cfgs}, True, raw.strip()))
            blockbuf = []
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
        if not tgt and L.endswith('.shp') and 'shapes/' + L in by_resname:
            tgt = by_resname['shapes/' + L]
        if tgt:
            add_ref(tgt, 'commented-out', how='literal inside a comment', **common)
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

# Icon stems passed to an "icons/%s.bmp" builder in the same file (ControlHelpSystem::RenderControlHelp /
# RenderControlHelpDPad, MultiwiniaHelp::RenderControlHelp(int,...)) -> confirmed references at the stem site.
ICON_STEM_BUILDERS = {
    'code/control_help.cpp': 'code/control_help.cpp:ControlHelpSystem::RenderControlHelp / RenderControlHelpDPad build "icons/%s.bmp"',
    'code/multiwiniahelp.cpp': 'code/multiwiniahelp.cpp:MultiwiniaHelp::RenderControlHelp(int,...) builds "icons/%s.bmp"',
}
for rel, rs in list(refs.items()):
    if not rel.startswith('data/icons/'):
        continue
    for r in list(rs):
        if r['kind'] == 'stem' and r['file'] in ICON_STEM_BUILDERS:
            add_ref(rel, 'rule:icon-stem', r['file'], r['sym'], r['line'], r['cond'], r['live'], r['built'],
                    'stem literal handed to ' + ICON_STEM_BUILDERS[r['file']], flags='mip=true mask=true')
for c in sites('code/lib/shader.cpp', 'shaders/%s'):
    add_ref('data/shaders/globals.h', 'rule:hlsl-include', c['file'], c['sym'], c['line'], c['cond'], c['live'], c['built'],
            '#include "globals.h" in deformDiamond/deformFullscreen/deformSphere.ps resolved by MyD3DXInclude::Open as shaders/<name>')

# Compiled-in code paths that cannot run in a Multiwinia match (manual, each verified by grep) ->
# reference build tag downgraded from FULL.
REACH = [
    # (file, symbol regex, line range or None, new tag, reason)
    ('code/taskmanager_interface_icons.cpp', r'.*', None, 'LEGACY',
     'TaskManagerInterfaceIcons is created only when !App::Multiplayer() (code/taskmanager_interface.cpp:TaskManagerInterface::CreateTaskManager)'),
    ('code/taskmanager_interface_gestures.cpp', r'.*', None, 'LEGACY',
     'TaskManagerInterfaceGestures only for single-player keyboard ControlMethod 0 (code/taskmanager_interface.cpp:TaskManagerInterface::CreateTaskManager)'),
    ('code/shaman_interface.cpp', r'.*', None, 'LEGACY',
     'ShamanInterface is shown only from TaskManagerInterfaceIcons (code/taskmanager_interface_icons.cpp) - single-player only'),
    ('code/global_internet.cpp', r'.*', None, 'LEGACY',
     'GlobalInternet is drawn only by GlobalWorld::Render (Darwinia world map; code/global_world.cpp:GlobalWorld::Render)'),
    ('code/global_world.cpp', r'.*Render.*|GlobalLocation.*|GlobalBuilding.*|SphereWorld.*', None, 'LEGACY',
     'GlobalWorld/SphereWorld rendering = Darwinia world map (Renderer branch m_locationId == -1, see P12 24.2)'),
    ('code/multiwiniahelp.cpp', r'MultiwiniaHelp::RenderControlHelp', (2781, 2994), 'LEGACY',
     'MultiwiniaHelp::RenderControlHelp() (no-arg) has no caller (grep code/)'),
    ('code/multiwiniahelp.cpp', r'MultiwiniaHelp::Render(BasicControlHelp|CurrentUnitHelp|CurrentTaskHelp)', None, 'LEGACY',
     'no live caller (calls commented out in code/taskmanager_interface_multiwinia.cpp, RenderBasicControlHelp never called)'),
    ('code/control_help.cpp', r'.*', None, 'LEGACY',
     'ControlHelpSystem::Render returns at once when App::Multiplayer(); RenderHelp also requires App::IsSinglePlayer() (code/control_help.cpp:ControlHelpSystem::Render, ::RenderHelp)'),
    ('code/clouds.cpp', r'Clouds::RenderFace', None, 'DEBUG',
     'm_faceTime is set only by Clouds::ShowDrSFace, called only from code/interface/cheat_window.cpp (cheat builds)'),
    ('code/interface/gesture_window.cpp', r'.*', None, 'DEBUG',
     'GestureWindow is created only by code/interface/debugmenu.cpp (debug menu)'),
    ('code/renderer.cpp', r'Renderer::RenderFrame', (795, 830), 'LEGACY',
     'Darwin logo overlay shown only via Script command (code/script.cpp sets m_renderDarwinLogo) - Darwinia cutscene scripts'),
    ('code/interface/helpandoptions_windows.cpp', r'CrateButton::.*|CrateHelp.*', None, 'DEBUG',
     'CrateHelpWindow reachable only through CrateHelpWindowButton (#ifdef INCLUDE_CRATE_HELP_WINDOW, TARGET_DEBUG)'),
]
DATA_REACH = [
    ('data/input/', 'LEGACY', 'icon bindings parsed by ControlBindings but InputManager::controlIcon has no caller (code/lib/input/input.cpp)'),
    ('data/purity/', 'DEAD', 'purity files are not loaded (PURITY_CONTROL never defined)'),
]

def apply_reach(r):
    if r['built'] == 'data':
        for pre, tag, why in DATA_REACH:
            if r['file'].startswith(pre):
                return tag, why
        return None
    for f_, rx, rng, tag, why in REACH:
        if r['file'] == f_ and re.match(rx, r['sym'] or '') and (rng is None or rng[0] <= r['line'] <= rng[1]):
            return tag, why
    return None

for rel, rs in refs.items():
    for r in rs:
        o = apply_reach(r)
        r['reach'] = o[1] if o else ''
        r['reach_tag'] = o[0] if o else ''

ORDER = ['FULL', 'D3D', 'DEBUG', 'EDITOR', 'LEGACY', 'DEAD']
_ref_build_compiled = ref_build
def ref_build(r):
    b = _ref_build_compiled(r)
    if b in ('FULL', 'D3D') and r.get('reach_tag'):
        return r['reach_tag']
    return b

def summarise(rel):
    rs = refs.get(rel, [])
    strong = [r for r in rs if ref_strong(r)]
    tags = sorted({ref_build(r) for r in strong}, key=ORDER.index)
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

# ----------------------------------------------------------------------------------
# 10. Packaging (tools/build_multiwinia_data.bat, tools/listfiles.pl, exclusion/inclusion lists)
# ----------------------------------------------------------------------------------
import fnmatch
TOOLS = os.path.join(SRC, 'tools')
def read_list(name):
    try:
        return [l.strip().replace('\\', '/') for l in open(os.path.join(TOOLS, name), encoding='latin-1').read().splitlines() if l.strip()]
    except FileNotFoundError:
        return []
all_data = []
for root, dirs, files in os.walk(DATA):
    for d in dirs:
        all_data.append(os.path.relpath(os.path.join(root, d), DATA).replace(os.sep, '/') + '/')
    for f in files:
        all_data.append(os.path.relpath(os.path.join(root, f), DATA).replace(os.sep, '/'))
excl = read_list('sounds.txt') + read_list('menu_sounds.txt') + read_list('multiwinia_exclusions.txt')
short_ex = [e[3:].lower() for e in excl if e.startswith('**/')]
long_ex = [e.lower() for e in excl if not e.startswith('**/')]
def excluded_by(relpath):
    """listfiles.pl semantics: a path is pruned if its basename is a **/ short exclude, or the path (or an
    ancestor directory) matches a (case-insensitive, Windows bsd_glob) exclude pattern."""
    rp = relpath.lower(); parts = rp.split('/')
    for k in range(1, len(parts) + 1):
        sub = '/'.join(parts[:k])
        if parts[k - 1] in short_ex:
            return '**/' + parts[k - 1]
        for e in long_ex:
            if fnmatch.fnmatchcase(sub, e):
                return e
    return None
MAIN_EXT = ('tex', 'bmp', 'jpg', 'shp', 'txt', 'vs', 'ps', 'ps3', 'h', 'anim')
init_list = {x.lower() for x in read_list('init.list')} - {'textures/xboxlivearcade.bmp'}
lang_all = {x.lower() for x in read_list('language.list')}
lang_en = {x.lower() for x in read_list('englishlanguage.list')}
lang_per = {}
for ln_ in ('english', 'french', 'german', 'italian', 'spanish', 'portuguese', 'russian', 'japanese', 'korean'):
    for x in read_list(ln_ + 'language.list'):
        lang_per.setdefault(x.lower(), []).append(ln_)
def archive_of(rel):
    rp = rel[len('data/'):]; lo = rp.lower(); out = []
    if lo in init_list:
        out.append('init.dat')
    ex = excluded_by(rp)
    if ex is None and lo.rsplit('.', 1)[-1] in MAIN_EXT and 'sounds/' not in lo:
        out.append('main.dat')
    if lo in lang_all:
        out.append('language.dat[%s]' % ('+'.join(lang_per.get(lo, [])) or 'all-language build only'))
    if not out:
        if ex:
            return 'not packed (excluded by "%s")' % ex
        return 'not packed (extension .%s not in main.dat list)' % lo.rsplit('.', 1)[-1]
    return ' + '.join(out)

# ----------------------------------------------------------------------------------
# 11. Consumers, render passes, class bases, MP placements
# ----------------------------------------------------------------------------------
bases = {}
try:
    with open(os.path.join(OUT, 'drafts', 'P03_worldobject_classes.csv'), newline='') as fh:
        for row in csv.DictReader(fh):
            bases[row['class']] = row['base']
except FileNotFoundError:
    pass
def root_kind(cls):
    seen = 0
    while cls in bases and seen < 20:
        b = bases[cls]
        if b in ('Building', 'Entity', 'WorldObject', 'Unit'):
            return b
        cls = b; seen += 1
    return {'Building': 'Building', 'Entity': 'Entity', 'WorldObject': 'WorldObject'}.get(cls, '')
mp_place = defaultdict(int)
try:
    with open(os.path.join(OUT, 'drafts', 'P06_level_building_usage.csv'), newline='') as fh:
        for row in csv.DictReader(fh):
            lv = 'data/levels/' + row['level']
            if level_info.get(lv, ('',))[0] == 'live':
                mp_place[row['type'].lower()] += 1
except FileNotFoundError:
    pass

SYS = [  # (file prefix, system, pass per P12 24.2 numbering)
    ('code/clouds.cpp', 'Sky/clouds (Clouds; also main-menu background)', '3a-3d'),
    ('code/landscape_renderer.cpp', 'Landscape (LandscapeRenderer)', '4a/4b'),
    ('code/landscape.cpp', 'Landscape', '4a/4b'),
    ('code/water_reflection.cpp', 'Water reflection [D3D]', '5b'),
    ('code/water.cpp', 'Water', '5a/5b'),
    ('code/location.cpp', 'Location (weapons, lasers)', '10b'),
    ('code/particle_system.cpp', 'ParticleSystem', '15'),
    ('code/explosion.cpp', 'ExplosionManager', '14'),
    ('code/deform.cpp', 'DeformEffect [D3D]', 'after 15 (deform)'),
    ('code/renderer.cpp', 'Renderer (logo / transitions)', '31'),
    ('code/markersystem.cpp', 'MarkerSystem (off-screen markers)', '16'),
    ('code/multiwiniahelp.cpp', 'MultiwiniaHelp (tutorial/help overlay)', '17'),
    ('code/animatedpanel', 'AnimatedPanel (game-mode help animation)', '17'),
    ('code/shaman_interface.cpp', 'ShamanInterface', '18'),
    ('code/interface/', 'Eclipse windows (menus/dialogs)', '19'),
    ('code/UI/', 'GameMenuWindow front-end (Eclipse)', '19'),
    ('code/game_menu.cpp', 'GameMenu front-end', '19'),
    ('contrib/eclipse/', 'Eclipse window framework', '19'),
    ('code/gamecursor.cpp', 'GameCursor / MouseCursor', '20'),
    ('code/taskmanager_interface', 'Task-manager HUD', '23'),
    ('code/control_help.cpp', 'ControlHelpSystem', '25'),
    ('code/lib/text_renderer.cpp', 'TextRenderer (fonts)', 'all text (13,17,19,21,28,29,33)'),
    ('code/multiwinia.cpp', 'Multiwinia match HUD', '28'),
    ('code/rocket_status_panel.cpp', 'RocketStatusPanel (Rocket Riot HUD)', '28'),
    ('code/startsequence.cpp', 'StartSequence', '30'),
    ('code/loading_screen.cpp', 'LoadingScreen', 'loading-screen loop (outside RenderFrame)'),
    ('code/team.cpp', 'Team (Virii sprite strip build)', '7c'),
    ('code/global_world.cpp', 'GlobalWorld (Darwinia world map)', 'GlobalWorld branch'),
    ('code/global_internet.cpp', 'GlobalInternet (Darwinia world map)', 'GlobalWorld branch'),
    ('code/level_file.cpp', 'LevelFile (map defaults)', '4a/5a (via terrain consumers)'),
    ('code/mapfile.cpp', 'MapFile (custom .mwm maps embed terrain bitmaps)', '-'),
    ('code/location_editor.cpp', 'LocationEditor [EDITOR]', 'editor'),
    ('code/lib/FixedPipeline.cpp', 'FixedPipeline [D3D] fixed-function emulation', 'every D3D draw'),
    ('code/lib/shader.cpp', 'Shader loader [D3D]', '-'),
    ('code/lib/3d_sprite.cpp', 'Render3DSprite', 'caller'),
    ('code/sepulveda.cpp', 'Sepulveda (Darwinia help) [DEAD]', '-'),
    ('code/loaders/', 'Loading-screen mini-programs [DEAD: USE_LOADERS]', '-'),
    ('code/app.cpp', 'App', '-'),
    ('code/main.cpp', 'main loop', '-'),
    ('code/script.cpp', 'Script (cutscenes)', '-'),
    ('code/sound/', 'Sound system', '-'),
]
def consumer_of(r, asset_dir):
    f = r['file']; sym = r['sym'] or ''
    if r['built'] == 'data':
        if f.startswith('data/levels/'):
            if r['sym'].lower() in ('landcolourfile',):
                return 'LandscapeRenderer (terrain colour ramp)', '4a'
            if r['sym'].lower() in ('wavescolourfile', 'watercolourfile'):
                return 'Water (%s)' % r['sym'], '5a/5b'
            return 'StaticShape building (level %s)' % r['sym'], '6'
        if f.startswith('data/animations/'):
            return 'AnimatedPanel keyframe (%s)' % f.rsplit('/', 1)[-1], '17'
        if f.startswith('data/input/'):
            return 'ControlBindings icon (never drawn)', '-'
        return 'data ' + f, '-'
    if f.startswith('code/worldobject/'):
        cls = sym.split('::')[0] if '::' in sym else os.path.basename(f)[:-4]
        rk = root_kind(cls)
        meth = sym.split('::')[-1]
        if asset_dir == 'shapes':
            ps = {'Building': '6 (buildings)', 'Entity': '7a/7b (units/entities)', 'WorldObject': '10a (effects)'}.get(rk, '6/7')
        else:
            if 'Alphas' in meth or 'Light' in meth or 'Port' in meth:
                ps = '8 (building alphas)'
            elif cls == 'Darwinian':
                ps = '7d (Darwinians)'
            elif rk == 'Entity':
                ps = '7a/7b'
            elif rk == 'Building':
                ps = '6/8'
            elif rk == 'WorldObject':
                ps = '10a (effects)'
            else:
                ps = '6-10'
        place = ''
        if rk == 'Building' and cls.lower() in mp_place:
            place = ', placed in %d live MP maps' % mp_place[cls.lower()]
        elif rk == 'Building':
            place = ', placed in 0 live MP maps (may still be spawned by code)'
        return '%s (%s%s)' % (cls, rk or 'worldobject', place), ps
    for pre, sysname, ps in SYS:
        if f.startswith(pre):
            return sysname, ps
    return f, '-'

# ----------------------------------------------------------------------------------
# 12. Per-asset rows
# ----------------------------------------------------------------------------------
def site_str(r):
    return '%s:%s:L%d' % (r['file'], r['sym'] or '?', r['line'])

lookup_names = defaultdict(set)   # file -> names passed to LookupFragment/LookupMarker
for c in code_lits:
    if not c['commented'] and re.search(r'Lookup(Fragment|Marker)\s*\(\s*"' + re.escape(c['lit']) + '"', c['text']):
        lookup_names[c['file']].add(c['lit'])

STATUS_ORDER = ['FULL', 'D3D', 'DEBUG', 'EDITOR', 'LEGACY', 'DEAD']
rows = []
for rel, a in assets.items():
    info = a['info']
    rs, strong, tags = summarise(rel)
    weak = [r for r in rs if r not in strong]
    if a['ext'] == 'max':
        status, reach = 'unused', 'none (3ds Max authoring source)'
        ev = 'no loader for .max anywhere in code/; data/shapes/source is excluded from packing (tools/multiwinia_exclusions.txt "shapes/source")'
    elif a['ext'] == 'psp':
        status, reach = 'unused', 'none (Paint Shop Pro source)'
        ev = 'not a BMP; never requested; extension not packed into main.dat'
    elif tags and tags != ['DEAD']:
        status = 'referenced'
        reach = tags[0] if tags[0] != 'DEAD' else tags[1]
        ev = ''
    elif tags == ['DEAD']:
        status, reach = 'unused', 'DEAD'
        ev = 'only referenced from code that is not compiled / commented out'
    elif weak:
        status, reach = 'unknown', 'none'
        ev = 'only indirect evidence: ' + '; '.join(sorted({'%s %s:%s' % (r['kind'], r['file'], r['sym']) for r in weak if r['kind'] != 'format-pattern'}))[:300]
        if all(r['kind'] == 'format-pattern' for r in weak):
            status = 'unused'
            ev = 'name never produced: only matched by generic printf patterns whose value sets exclude it (see rules)'
    else:
        status, reach = 'unused', 'none'
        ev = 'no string, rule or data reference found'
    live_sites = [r for r in strong if ref_build(r) == reach] or strong
    sites_txt = []
    for r in live_sites:
        st_ = site_str(r)
        if st_ not in sites_txt:
            sites_txt.append(st_)
    cons = []; passes = []
    for r in live_sites:
        cn, ps = consumer_of(r, a['dir'])
        if cn not in cons:
            cons.append(cn)
        if ps not in passes:
            passes.append(ps)
    flags = sorted({r['flags'] for r in live_sites if r['flags']})
    states = []
    for r in live_sites:
        if r['state'] and r['state'] not in states:
            states.append(r['state'])
    reach_notes = sorted({r['reach'] for r in strong if r.get('reach')})
    if not ev:
        kinds = sorted({r['kind'] for r in live_sites})
        ev = '[C] ' + ', '.join(kinds)
        if reach_notes:
            ev += ' | reach: ' + '; '.join(reach_notes)
        dead = sorted({site_str(r) for r in strong if ref_build(r) == 'DEAD'})
        if dead:
            ev += ' | also dead/commented: ' + '; '.join(dead[:3])
    elif reach_notes:
        ev += ' | ' + '; '.join(reach_notes)
    a.update(status=status, reach=reach, sites=sites_txt, consumers=cons, passes=passes, flags=flags,
             states=states, evidence=ev, archive=archive_of(rel), nrefs=len(strong))
    if a['ext'] == 'shp':
        names = {f['name'].lower(): f['name'] for f in info['frag_list']}
        names.update({m['name'].lower(): m['name'] for m in info['marker_list']})
        files = {r['file'] for r in strong}
        lk = sorted({n for f_ in files for n in lookup_names.get(f_, set()) if n.lower() in names})
        a['lookups'] = lk

# max -> shp pairing
shp_stems = {res_name(r).rsplit('/', 1)[-1][:-4]: r for r in assets if r.endswith('.shp')}
for rel, a in assets.items():
    if a['ext'] == 'max':
        st_ = res_name(rel).rsplit('/', 1)[-1][:-4]
        a['pair'] = shp_stems.get(st_, '')

# ----------------------------------------------------------------------------------
# 13. CSV outputs
# ----------------------------------------------------------------------------------
def j(xs, n=6):
    xs = list(xs)
    return '; '.join(xs[:n]) + (' (+%d more)' % (len(xs) - n) if len(xs) > n else '')

tex_cols = ['path', 'dir', 'bytes', 'format', 'width', 'height', 'bpp', 'palette_colours_used', 'alpha_in_file',
            'pink_key_pixels', 'pink_fraction', 'black_fraction', 'mean_luminance', 'pow2', 'loader_issues', 'archive',
            'status', 'reach', 'load_sites', 'flags_at_load', 'gl_state_near_site', 'consumer', 'render_pass', 'evidence', 'extra']
with open(os.path.join(OUT, 'drafts', 'X2a_textures.csv'), 'w', newline='') as fh:
    w = csv.writer(fh); w.writerow(tex_cols)
    for rel, a in assets.items():
        if a['ext'] in ('shp', 'max'):
            continue
        i_ = a['info']
        if a['ext'] == 'bmp':
            fmt = 'BMP %d-bit %s' % (i_['bpp'], 'palettised' if i_['bpp'] <= 8 else ('BGR' if i_['bpp'] == 24 else 'BGRA'))
            extra = ''
            vals = [i_['w'], i_['h'], i_['bpp'], i_.get('palette_used', ''), i_['alpha'], i_['pink'],
                    '%.3f' % i_['pink_frac'], '%.3f' % i_['black_frac'], '%.1f' % i_['mean_lum'], i_['pow2'], j(i_['issues'], 3)]
        elif a['ext'] == 'anim':
            fmt = 'AnimatedPanel text (%s x %s, t %s..%s s)' % (i_['w'], i_['h'], i_['start'], i_['stop'])
            extra = '%d objects, %d keyframes, images: %s' % (i_['objects'], i_['keyframes'], ', '.join(i_['images']))
            vals = [i_['w'], i_['h'], '', '', '', '', '', '', '', '', '']
        else:
            fmt = {'vs': 'HLSL vertex shader (vs_main)', 'ps': 'HLSL pixel shader (ps_main)', 'ps3': 'HLSL ps_3_0 pixel shader (ps_main)',
                   'h': 'HLSL include', 'psp': 'Paint Shop Pro image'}.get(a['ext'], a['ext'])
            extra = ''
            vals = [''] * 11
        w.writerow([rel, a['dir'], a['bytes'], fmt] + vals + [a['archive'], a['status'], a['reach'], j(a['sites']),
                   j(a['flags'], 3), j(a['states'], 2), j(a['consumers'], 4), j(a['passes'], 4), a['evidence'], extra])

shp_cols = ['path', 'kind', 'bytes', 'fragments', 'max_depth', 'positions', 'vertices', 'colours', 'triangles', 'geometry',
            'markers', 'marker_names', 'hierarchy', 'parse_problems', 'code_looked_up_names', 'archive', 'status', 'reach',
            'load_sites', 'consumer', 'render_pass', 'evidence', 'source_pair']
with open(os.path.join(OUT, 'drafts', 'X2a_shapes.csv'), 'w', newline='') as fh:
    w = csv.writer(fh); w.writerow(shp_cols)
    for rel, a in assets.items():
        if a['ext'] not in ('shp', 'max'):
            continue
        i_ = a['info']
        if a['ext'] == 'shp':
            vals = [i_['fragments'], i_['max_depth'], i_['positions'], i_['vertices'], i_['colours'], i_['triangles'],
                    i_['format'], i_['markers'], i_['marker_names'], i_['hierarchy'], j(i_['problems'], 3), ', '.join(a.get('lookups', []))]
            pair = ''
        else:
            vals = [''] * 12
            pair = a.get('pair', '')
        w.writerow([rel, a['ext'], a['bytes']] + vals + [a['archive'], a['status'], a['reach'], j(a['sites']),
                   j(a['consumers'], 4), j(a['passes'], 3), a['evidence'], pair])

with open(os.path.join(OUT, 'drafts', 'X2a_refs.csv'), 'w', newline='') as fh:
    w = csv.writer(fh)
    w.writerow(['asset', 'kind', 'strength', 'site', 'build', 'condition', 'reach_note', 'how', 'flags', 'gl_state'])
    for rel in assets:
        for r in refs.get(rel, []):
            if r['kind'] == 'format-pattern':
                continue
            w.writerow([rel, r['kind'], r['strength'], site_str(r), ref_build(r), r['cond'], r.get('reach', ''),
                        r['how'], r['flags'], r['state']])

seen_m = set()
with open(os.path.join(OUT, 'drafts', 'X2a_missing.csv'), 'w', newline='') as fh:
    w = csv.writer(fh); w.writerow(['requested_name', 'site', 'build', 'condition', 'how'])
    for m in missing:
        key = (m['name'], m['where'], m['ln'])
        if key in seen_m:
            continue
        seen_m.add(key)
        b = 'DEAD' if m.get('built') == '' else next((t for t in ['FULL', 'D3D', 'DEBUG', 'EDITOR'] if m['live'].get(t)), 'DEAD')
        w.writerow([m['name'], '%s:%s:L%s' % (m['where'], m.get('sym', ''), m['ln']), b, m.get('cond', ''), m.get('how', '')])

# coverage
with open(os.path.join(OUT, 'coverage', 'X2a_asset_catalogue_visual.csv'), 'w', newline='') as fh:
    w = csv.writer(fh); w.writerow(['path', 'depth', 'reason', 'sections'])
    for rel, a in assets.items():
        sec = '25.4 Per-file catalogue (%s)' % a['dir']
        if a['ext'] == 'max':
            w.writerow([rel, 'skimmed', 'binary 3ds Max source: size and pairing recorded, contents not parsed (never loaded at run time)', sec])
        elif a['ext'] == 'psp':
            w.writerow([rel, 'skimmed', 'Paint Shop Pro source image: not loadable by BitmapRGBA, never referenced; header not parsed', sec])
        else:
            w.writerow([rel, 'full', '', sec])

# summary for prose
summ = {'n': len(assets), 'by_dir': defaultdict(lambda: defaultdict(int))}
for rel, a in assets.items():
    d = a['dir'] + ('/unicode' if '/unicode/' in rel else '')
    summ['by_dir'][d]['files'] += 1
    summ['by_dir'][d]['status:' + a['status']] += 1
    summ['by_dir'][d]['reach:' + a['reach']] += 1
    if a['ext'] == 'bmp':
        i_ = a['info']
        summ['by_dir'][d]['bpp%d' % i_['bpp']] += 1
        summ['by_dir'][d]['pow2' if i_['pow2'] == 'yes' else 'npot'] += 1
        if 'non-trivial' in i_['alpha']:
            summ['by_dir'][d]['alpha8'] += 1
        if i_['pink']:
            summ['by_dir'][d]['pinkkey'] += 1
        summ['by_dir'][d]['bytes'] += a['bytes']
json.dump(summ, open(os.path.join(SCR, 'X2a_summary.json'), 'w'), indent=1, default=lambda o: dict(o))
json.dump({rel: {k: v for k, v in a.items() if k not in ('abs',) and k != 'info'} for rel, a in assets.items()},
          open(os.path.join(SCR, 'X2a_assets.json'), 'w'), indent=0, default=str)
print('written', len(rows))
