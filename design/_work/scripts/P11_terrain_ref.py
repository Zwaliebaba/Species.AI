"""P11 reference re-implementation of the Multiwinia landscape height-map pipeline
(LevelFile::ParseLandscapeData/ParseLandscapeTiles/ParseLandFlattenAreas ->
 Landscape::Init -> GenerateHeightMap -> LandscapeTile::Generate -> MergeTileIntoLandscape -> FlattenArea),
written from the P11 design draft to validate that the specification is complete.
Usage: python3 -I P11_terrain_ref.py <level.txt> [--dump out.csv]
Prints grid size, height stats and how many Buildings_StartDefinition entries sit above sea level (y>0)."""
import sys, math, struct

def f32(x):
    return struct.unpack('<f', struct.pack('<f', float(x)))[0]

def iv_atof(s):           # code/../math_stubs.cpp:iv_atof -> atof then float rounding
    return f32(float(s))

def to_u16(d):            # double -> unsigned short as MSVC x86 does it (truncate toward zero, keep low 16 bits) [I for negatives]
    return int(math.trunc(d)) & 0xFFFF

def i2u16(i):             # int -> unsigned short
    return i & 0xFFFF

class Map2D:
    """Array2D + SurfaceMap2D semantics (code/lib/2d_array.*, code/lib/2d_surface_map.*)."""
    def __init__(self, cols, rows, outside, x0=0.0, y0=0.0, cx=1.0, cy=1.0):
        self.cols, self.rows, self.outside = cols, rows, outside
        self.d = [outside] * (cols * rows)
        self.x0, self.y0, self.cx, self.cy = x0, y0, cx, cy
        self.ix, self.iy = 1.0 / cx, 1.0 / cy
    def get(self, x, y):
        x &= 0xFFFF; y &= 0xFFFF
        if x >= self.cols or y >= self.rows: return self.outside
        return self.d[y * self.cols + x]
    def put(self, x, y, v):
        x &= 0xFFFF; y &= 0xFFFF
        if x >= self.cols or y >= self.rows: return
        self.d[y * self.cols + x] = v
    def highest(self):          # GetHighestValue: note inner loop bound uses m_numRows
        h = self.get(0, 0)
        for y in range(self.rows):
            for x in range(self.rows):
                v = self.get(x, y)
                if v > h: h = v
        return h
    def value(self, x, y):      # GetValue: bilinear with uint16 index wrap-to-zero
        fx = (x - self.x0) * self.ix
        fy = (y - self.y0) * self.iy
        x1 = to_u16(fx); y1 = to_u16(fy)
        x2 = (x1 + 1) & 0xFFFF; y2 = (y1 + 1) & 0xFFFF
        fx -= math.floor(fx); fy -= math.floor(fy)
        if x1 >= self.cols: x1 = 0
        if x2 >= self.cols: x2 = 0
        if y1 >= self.rows: y1 = 0
        if y2 >= self.rows: y2 = 0
        return (self.get(x1, y1) * ((1 - fx) * (1 - fy)) + self.get(x1, y2) * ((1 - fx) * fy) +
                self.get(x2, y1) * (fx * (1 - fy)) + self.get(x2, y2) * (fx * fy))
    def index_x(self, rx): return int((rx - self.x0) * self.ix)   # GetMapIndexX (trunc toward 0)
    def index_y(self, ry): return int((ry - self.y0) * self.iy)
    def real_x(self, i): return i * self.cx + self.x0
    def real_y(self, i): return i * self.cy + self.y0

ONE_OVER_LOG2 = 1.0 / f32(math.log(2.0))
def get_power_of_two(x):  # LandscapeTile::GetPowerOfTwo = ceil(Log2(x)), Log2 uses iv_log (float-rounded)
    return int(math.ceil(f32(math.log(x)) * ONE_OVER_LOG2))

class Tile:
    def __init__(self):
        self.seed_state = 1
    def rnd(self):            # LandscapeTile::LandscapeAppRandom: MSVC rand() LCG on a private state
        self.seed_state = (self.seed_state * 214013 + 2531011) & 0xFFFFFFFF
        return (self.seed_state >> 16) & 0x7FFF
    def sfrand(self, rng):    # Landscapesfrand
        return (0.5 - self.rnd() / 32767.0) * rng
    def noise(self, half, height):   # GenerateNoise
        length = 256.0 * half / self.n
        val = self.sfrand(f32(math.pow(length * 10.0, self.fd)))
        val *= self.comp
        h = abs(f32(height))
        p = 1.0 if (h == 0.0 and self.ls == 0.0) else (0.0 if h == 0.0 else f32(math.pow(h, self.ls)))
        val *= 0.1 + p * 0.15
        return val
    def choose(self, opts):
        m = self.method
        if m == 0:
            return sum(opts) * 0.25
        if m == 1:
            if self.rnd() & 1: return (opts[0] + opts[1]) * 0.5
            return (opts[2] + opts[3]) * 0.5
        r = self.rnd() & 0xFFFF
        if r < 0x3FFF: return opts[0]
        if r < 0x7FFF: return opts[1]
        if r < 0xBFFF: return opts[2]
        return opts[3]
    def diamond(self, x, z, h):
        g = self.hm.get
        opts = [g(x - h, z), g(x + h, z), g(x, z + h), g(x, z - h)]
        if self.method == 0: nh = (opts[0] + opts[1] + opts[3] + opts[2]) * 0.25
        elif self.method == 1:
            if self.rnd() & 1: nh = (opts[0] + opts[1]) * 0.5
            else: nh = (g(x, z + h) + g(x, z - h)) * 0.5
        else: nh = self.choose(opts)
        nh += self.noise(float(h), nh)
        self.hm.put(x, z, nh)
    def square(self, x, z, h):
        g = self.hm.get
        a, b, c, d = g(x - h, z - h), g(x - h, z + h), g(x + h, z - h), g(x + h, z + h)
        if self.method == 0: nh = (a + b + c + d) * 0.25
        elif self.method == 1:
            if self.rnd() & 1: nh = (a + d) * 0.5
            else: nh = (b + c) * 0.5
        else: nh = self.choose([a, b, c, d])
        nh += self.noise(float(h), nh)
        self.hm.put(x, z, nh)
    def generate(self, cell, outside):
        req_world = int(self.size / cell)
        k = get_power_of_two(req_world - 1)
        n = int(f32(math.pow(2, k))) + 1
        self.n = n
        self.hm = Map2D(n, n, outside)
        if self.gp > 0:
            res = (1 << self.gp) - 1
            scale = (n - 1) // (res + 1)
            for z in range(res):
                for x in range(res):
                    self.hm.put((x + 1) * scale, (z + 1) * scale, float(self.guide[z * res + x]))
        fdm = 30.7 * f32(math.exp(-6.5 * self.fd))
        smm = 15.353 * f32(math.exp(-3.1 * self.ls))
        self.comp = self.hs; self.comp *= fdm; self.comp *= smm
        iters = get_power_of_two(n) - self.gp
        step = (n - 1) >> self.gp
        self.seed_state = self.seed & 0xFFFFFFFF
        for _ in range(iters):
            if step > 0:
                for x in range(0, n - 1, step):
                    for z in range(0, n - 1, step):
                        h = step >> 1
                        mx, mz = x + h, z + h
                        self.square(mx, mz, h)
                        if z != 0: self.diamond(mx, z, h)
                        if x != 0: self.diamond(x, mz, h)
            step >>= 1
        for i in range(n * n): self.hm.d[i] += self.posY

def parse(path):
    land = dict(cell=f32(12.0), wx=2000, wz=2000, outside=-10.0, maxh=0.0)
    tiles, flats, buildings = [], [], []
    cur = None
    for raw in open(path, 'r', errors='replace'):
        s = raw.split('#')[0]
        toks = [t for t in s.replace(':', ' ').replace(',', ' ').split() if t]
        if not toks: continue
        w = toks[0].lower()
        if w.endswith('_startdefinition'): cur = w[:-16]; continue
        if w.endswith('_enddefinition'): cur = None; continue
        if cur == 'landscape' and len(toks) > 1:
            if w == 'cellsize': land['cell'] = iv_atof(toks[1])
            elif w == 'worldsizex': land['wx'] = int(toks[1])
            elif w == 'worldsizez': land['wz'] = int(toks[1])
            elif w == 'outsideheight': land['outside'] = iv_atof(toks[1])
            elif w == 'maxheight': land['maxh'] = f32(float(toks[1]))
        elif cur == 'landscapetiles':
            t = Tile()
            t.posX = int(toks[0]); t.posY = iv_atof(toks[1]); t.posZ = int(toks[2]); t.size = int(toks[3])
            t.fd = iv_atof(toks[4]); t.hs = iv_atof(toks[5]); t.dh = iv_atof(toks[6])
            t.method = int(toks[7]); t.seed = int(toks[8]); t.ls = iv_atof(toks[9]); t.gp = int(toks[10])
            t.guide = []
            if t.gp > 0:
                g = toks[11]
                res = (1 << t.gp) - 1
                t.guide = [(ord(g[2*i]) - 65) * 16 + (ord(g[2*i+1]) - 65) for i in range(res * res)]
            tiles.append(t)
        elif cur == 'landflattenareas':
            fa = dict(cx=iv_atof(toks[0]), cy=iv_atof(toks[1]), cz=iv_atof(toks[2]), size=iv_atof(toks[3]), type=0, thr=40.0)
            if len(toks) > 4:
                fa['type'] = int(toks[4]); fa['thr'] = iv_atof(toks[5])
            flats.append(fa)
        elif cur == 'buildings' and len(toks) > 3:
            try: buildings.append((toks[0], iv_atof(toks[2]), iv_atof(toks[3])))
            except ValueError: pass
    return land, tiles, flats, buildings

def build(land, tiles, flats):
    cell = land['cell']; outside = land['outside']
    cols = int(math.ceil(land['wx'] / cell)); rows = int(math.ceil(land['wz'] / cell))
    W = Map2D(cols, rows, outside, 0.0, 0.0, cell, cell)
    for t in tiles:
        t.generate(cell, outside)
        posXi = to_u16(t.posX / W.cx + 0.5)
        posZi = to_u16(t.posZ / W.cy + 0.5)
        num = int(t.size / W.cx)
        hr = t.hm.highest() - outside + 0.001
        hf = (t.dh - outside) / hr
        tm = t.hm
        tm.x0 = float(t.posX); tm.y0 = float(t.posZ)
        tm.cx = t.size / float(tm.cols); tm.cy = t.size / float(tm.rows)
        tm.ix = 1.0 / tm.cx; tm.iy = 1.0 / tm.cy
        for z in range(num):
            tz = W.real_y(z + posZi)
            for x in range(num):
                tx = W.real_x(x + posXi)
                h1 = (tm.value(tx, tz) - outside) * hf + outside
                if h1 > W.get(x + posXi, z + posZi):
                    W.put(x + posXi, z + posZi, h1)
    for a in flats:
        x1 = W.index_x(a['cx'] - a['size']) + 1; x2 = W.index_x(a['cx'] + a['size']) + 1
        z1 = W.index_y(a['cz'] - a['size']) + 1; z2 = W.index_y(a['cz'] + a['size']) + 1
        passes = 2 if a['type'] == 3 else 1
        for _ in range(passes):
            for z in range(z1, z2):
                for x in range(x1, x2):
                    h = a['cy']; cur = W.get(x, z); t = a['type']
                    if t == 1: h += cur
                    elif t == 2: h = cur - h if cur > a['thr'] else cur
                    elif t == 4: h = cur - h if cur < a['thr'] else cur
                    elif t == 3:
                        if cur > a['thr']:
                            h = cur * (h / cur)
                            h += W.get(x - 1, z) + W.get(x + 1, z) + W.get(x, z + 1) + W.get(x, z - 1)
                            h /= 6.0
                        if h > cur: h = cur
                    W.put(x, z, h)
    return W

if __name__ == '__main__':
    land, tiles, flats, buildings = parse(sys.argv[1])
    W = build(land, tiles, flats)
    vals = W.d
    print(f'grid {W.cols}x{W.rows} cell={W.cx!r} worldSize={W.cx*W.cols:.3f}x{W.cy*W.rows:.3f} min={min(vals):.3f} max={max(vals):.3f} tiles={len(tiles)} flats={len(flats)}')
    above = [b for b in buildings if W.value(b[1], b[2]) > 0.0]
    print(f'buildings above sea level: {len(above)}/{len(buildings)}')
    below = [(b[0], round(W.value(b[1], b[2]), 2)) for b in buildings if W.value(b[1], b[2]) <= 0.0]
    print('below/at sea:', below[:12])
    if '--dump' in sys.argv:
        out = open(sys.argv[sys.argv.index('--dump') + 1], 'w')
        for z in range(W.rows):
            out.write(','.join(f'{W.get(x, z):.4f}' for x in range(W.cols)) + '\n')
