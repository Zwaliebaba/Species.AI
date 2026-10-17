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
