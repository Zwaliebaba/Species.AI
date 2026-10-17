"""S23 helper: conformance oracle + survey built on scripts/P11_terrain_ref.py (the Python port of
Landscape::Init -> GenerateHeightMap -> LandscapeTile::Generate -> MergeTileIntoLandscape -> FlattenArea).

Usage:
  python3 -I S_23_terrain_oracle.py <SRC_ROOT> survey            # one CSV row per landscape-bearing level
  python3 -I S_23_terrain_oracle.py <SRC_ROOT> detail <file.txt>  # per-tile derived values for one level
  python3 -I S_23_terrain_oracle.py <SRC_ROOT> dump <file.txt> <out.f64>  # raw little-endian float64 grid, row-major (z outer)

Survey columns:
  file, cols, rows, cell, worldSize, tiles, flats, min, max,
  sha256_16           first 16 hex digits of SHA-256 over the world grid packed '<d' row-major (z outer, x inner)
  neg_tile_origin     tiles whose posX or posZ < 0
  off_edge_tiles      tiles whose footprint extends past the world grid
  k_lt_p              tiles where tile exponent k < guide power p (degenerate case)
  samp_frac_neg       merge-sampler calls with -1 < fx or fy < 0 (well-defined C++, x1 = 0, weight fx+1)
  samp_le_minus1      merge-sampler calls with fx or fy <= -1 (double->unsigned short UB; MSVC behaviour assumed)
  samp_far_wrap       merge-sampler calls whose x1/y1/x2/y2 >= tile size (wrap-to-column/row-0 rule)
  pow_calls           pow() calls made by tile generation (noise amplitude + lowland envelope)
  pow_fragile         pow() results within 1 double-ulp of a float32 rounding midpoint (a CRT that is
                      off by <=1 ulp could round these to a different float -> different terrain)
  pow_tie_exact       subset of pow_fragile where the exact mathematical result is representable in double
                      (integer exponent, e.g. pow(x,2) of a float x) and lies exactly on the midpoint: such a
                      call is reproducible iff the CRT returns the exact result
  exp_fragile, log_fragile   same test for exp() (2 per tile) and log() (GetPowerOfTwo)
"""
import sys, os, math, struct, hashlib, importlib.util
from fractions import Fraction

here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('ref', os.path.join(here, 'P11_terrain_ref.py'))
ref = importlib.util.module_from_spec(spec); spec.loader.exec_module(ref)

def f32(x): return struct.unpack('<f', struct.pack('<f', float(x)))[0]

def fragile(d):
    """True if double d lies within 1 ulp(double) of the midpoint between two adjacent floats."""
    if d == 0.0 or math.isinf(d) or math.isnan(d): return False
    f = f32(d)
    # neighbours of f in float32
    bits = struct.unpack('<I', struct.pack('<f', f))[0]
    up = struct.unpack('<f', struct.pack('<I', bits + 1))[0]
    dn = struct.unpack('<f', struct.pack('<I', bits - 1))[0]
    ulp = math.ulp(d)
    for nb in (up, dn):
        mid = (f + nb) / 2.0
        if abs(d - mid) <= ulp: return True
    return False

class Stats:
    def __init__(self): self.reset()
    def reset(self):
        self.pow_calls = self.pow_fragile = self.pow_tie = self.exp_fragile = self.log_fragile = 0
        self.frac_neg = self.le_m1 = self.far = 0
        self.sampling = False

S = Stats()

class InstrumentedMath:
    """Drop-in for the `math` module used inside P11_terrain_ref; counts CRT-fragile results."""
    def __getattr__(self, name): return getattr(math, name)
    @staticmethod
    def pow(a, b):
        r = math.pow(a, b); S.pow_calls += 1
        if fragile(r):
            S.pow_fragile += 1
            if float(b).is_integer() and 0 <= b <= 8 and Fraction(a) ** int(b) == Fraction(r): S.pow_tie += 1
        return r
    @staticmethod
    def exp(a):
        r = math.exp(a)
        if fragile(r): S.exp_fragile += 1
        return r
    @staticmethod
    def log(a):
        r = math.log(a)
        if fragile(r): S.log_fragile += 1
        return r

ref.math = InstrumentedMath()

_orig_value = ref.Map2D.value
def value_instrumented(self, x, y):
    if S.sampling:
        fx = (x - self.x0) * self.ix; fy = (y - self.y0) * self.iy
        for f, n in ((fx, self.cols), (fy, self.rows)):
            if -1.0 < f < 0.0: S.frac_neg += 1
            elif f <= -1.0: S.le_m1 += 1
            elif int(f) + 1 >= n: S.far += 1
    return _orig_value(self, x, y)
ref.Map2D.value = value_instrumented

def build_instrumented(land, tiles, flats):
    """Same as ref.build but flags tile-merge sampling (ref.build is reused unchanged)."""
    S.sampling = True
    try:
        return ref.build(land, tiles, flats)
    finally:
        S.sampling = False

def digest(W):
    h = hashlib.sha256()
    h.update(struct.pack('<%dd' % len(W.d), *W.d))
    return h.hexdigest()[:16]

def tile_derived(t, cell, outside):
    req = int(t.size / cell)
    k = ref.get_power_of_two(req - 1)
    n = int(f32(math.pow(2, k))) + 1
    p = t.gp
    iters = ref.get_power_of_two(n) - p
    step0 = (n - 1) >> p
    fdm = 30.7 * f32(math.exp(-6.5 * t.fd)); smm = 15.353 * f32(math.exp(-3.1 * t.ls))
    comp = t.hs; comp *= fdm; comp *= smm
    posXi = ref.to_u16(t.posX / cell + 0.5); posZi = ref.to_u16(t.posZ / cell + 0.5)
    num = int(t.size / cell)
    return dict(req=req, k=k, n=n, p=p, iters=iters, step0=step0, fdm=fdm, smm=smm, comp=comp,
                posXi=posXi, posZi=posZi, numCells=num, tileCell=t.size / float(n))

def levels(src):
    lv = os.path.join(src, 'data', 'levels')
    for fn in sorted(os.listdir(lv)):
        p = os.path.join(lv, fn)
        if os.path.isfile(p) and (fn.startswith('mp_') or fn.startswith('map_')):
            yield fn, p

def survey(src):
    print('file,cols,rows,cell,worldSize,tiles,flats,min,max,sha256_16,neg_tile_origin,off_edge_tiles,k_lt_p,'
          'samp_frac_neg,samp_le_minus1,samp_far_wrap,pow_calls,pow_fragile,pow_tie_exact,exp_fragile,log_fragile')
    for fn, p in levels(src):
        land, tiles, flats, _b = ref.parse(p)
        if not tiles and land['wx'] == 2000 and land['cell'] == f32(12.0):
            continue
        S.reset()
        cell = land['cell']
        cols = int(math.ceil(land['wx'] / cell))
        neg = sum(1 for t in tiles if t.posX < 0 or t.posZ < 0)
        off = 0; klp = 0
        for t in tiles:
            d = tile_derived(t, cell, land['outside'])
            if d['posXi'] + d['numCells'] > cols or d['posZi'] + d['numCells'] > cols: off += 1
            if d['k'] < d['p']: klp += 1
        S.reset()
        W = build_instrumented(land, tiles, flats)
        print(f"{fn},{W.cols},{W.rows},{cell!r},{cell*W.cols:.3f},{len(tiles)},{len(flats)},"
              f"{min(W.d):.3f},{max(W.d):.3f},{digest(W)},{neg},{off},{klp},"
              f"{S.frac_neg},{S.le_m1},{S.far},{S.pow_calls},{S.pow_fragile},{S.pow_tie},{S.exp_fragile},{S.log_fragile}")

def detail(src, name):
    p = os.path.join(src, 'data', 'levels', name)
    land, tiles, flats, b = ref.parse(p)
    cell = land['cell']; outside = land['outside']
    cols = int(math.ceil(land['wx'] / cell)); rows = int(math.ceil(land['wz'] / cell))
    print(f"cellSize(f32)={cell!r} outsideHeight(f32)={outside!r} worldSizeX={land['wx']} worldSizeZ={land['wz']} maxHeight={land['maxh']}")
    print(f"grid={cols}x{rows} extent={cell*cols!r} x {cell*rows!r}")
    S.reset()
    W = build_instrumented(land, tiles, flats)
    for i, t in enumerate(tiles):
        d = tile_derived(t, cell, outside)
        hi = t.hm.highest()
        hr = hi - outside + 0.001
        hf = (t.dh - outside) / hr
        print(f"tile{i}: x={t.posX} y={t.posY!r} z={t.posZ} size={t.size} fd={t.fd!r} hs={t.hs!r} dh={t.dh!r} "
              f"method={t.method} seed={t.seed} ls={t.ls!r} p={t.gp} guideNonZero={sum(1 for g in t.guide if g)}")
        print(f"   reqWorldCells={d['req']} k={d['k']} n={d['n']} iterations={d['iters']} firstStep={d['step0']} "
              f"fracDimMod={d['fdm']!r} smoothMod={d['smm']!r} compHeightScale={d['comp']!r}")
        print(f"   posXi={d['posXi']} posZi={d['posZi']} numCells={d['numCells']} tileCell={d['tileCell']!r} "
              f"tileHighest(after posY)={hi!r} heightRange={hr!r} heightFactor={hf!r}")
    for i, a in enumerate(flats):
        print(f"flatten{i}: {a}")
    print(f"world min={min(W.d)!r} max={max(W.d)!r} sha256_16={digest(W)}")
    print(f"sampler: frac_neg={S.frac_neg} le_minus1={S.le_m1} far_wrap={S.far}; pow calls={S.pow_calls} fragile={S.pow_fragile} (exact ties {S.pow_tie}); "
          f"exp fragile={S.exp_fragile}; log fragile={S.log_fragile}")
    # a few probe samples (world sample indices) for hand checking
    for (x, z) in ((0, 0), (cols // 2, rows // 2), (cols // 4, rows // 4), (3 * cols // 4, rows // 3)):
        print(f"   h[{x},{z}] = {W.get(x, z)!r}")
    above = sum(1 for bb in b if W.value(bb[1], bb[2]) > 0.0)
    print(f"buildings above sea level: {above}/{len(b)}")

def dump(src, name, out):
    p = os.path.join(src, 'data', 'levels', name)
    land, tiles, flats, _b = ref.parse(p)
    W = ref.build(land, tiles, flats)
    with open(out, 'wb') as fh:
        fh.write(struct.pack('<%dd' % len(W.d), *W.d))
    print(f'{W.cols}x{W.rows} float64 written to {out}; sha256_16={digest(W)}')

if __name__ == '__main__':
    src, mode = sys.argv[1], sys.argv[2]
    if mode == 'survey': survey(src)
    elif mode == 'detail': detail(src, sys.argv[3])
    elif mode == 'dump': dump(src, sys.argv[3], sys.argv[4])
    else: print(__doc__)
