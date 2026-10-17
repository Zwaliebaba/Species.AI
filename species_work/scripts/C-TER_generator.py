#!/usr/bin/env python3
"""C-TER conformance generator: an independent implementation of the terrain
generator exactly as architecture/components/terrain.md specifies it, used to
check the acceptance vectors of that document.

Run with:  python3 -I C-TER_generator.py [selftest|traceA|traceB|traceM1|full|log2]
No input files; the parameter sets are the anonymous sets of terrain.md §7.
"""
import math
import struct
import sys

def f32(x):
    """Round a binary64 value to binary32 (nearest-even) and widen it back."""
    return struct.unpack('<f', struct.pack('<f', x))[0]

# ---- numeric primitives (terrain.md §4.2) ---------------------------------
_C_LOG2 = 1.0 / f32(math.log(2.0))

def rlog(x):
    return f32(math.log(x))

def log2r(x):
    return rlog(x) * _C_LOG2

def pow2ceil(x):
    return int(math.ceil(log2r(x)))

def rpow(x, y):
    if x == 0.0 and y == 0.0:
        return 1.0
    return f32(math.pow(x, y))

def rexp(x):
    return f32(math.exp(x))

def rsqrt(x):
    return f32(math.sqrt(x))

def trunc_u16(v):
    """Reference conversion of a binary64 value to an unsigned 16-bit index,
    defined for v > -1 (truncation toward zero)."""
    assert v > -1.0
    return int(v) & 0xFFFF

# ---- tile generator (terrain.md §4.3) --------------------------------------
class TileRng:
    def __init__(self):
        self.state = 1
    def seed(self, s):
        self.state = s & 0xFFFFFFFF
    def next(self):
        self.state = (self.state * 214013 + 2531011) & 0xFFFFFFFF
        return (self.state >> 16) & 0x7FFF
    def signed(self, r):
        return (0.5 - self.next() / 32767.0) * r

class Grid:
    """Square or rectangular grid with origin, cell size and an outside value."""
    def __init__(self, cols, rows, x0, y0, cx, cy, outside):
        self.cols, self.rows = cols, rows
        self.x0, self.y0 = x0, y0
        self.cx, self.cy = cx, cy
        self.ix, self.iy = 1.0 / cx, 1.0 / cy
        self.outside = outside
        self.v = [[outside] * cols for _ in range(rows)]   # v[z][x]
    def get(self, x, z):
        x &= 0xFFFF; z &= 0xFFFF
        if x >= self.cols or z >= self.rows:
            return self.outside
        return self.v[z][x]
    def put(self, x, z, val):
        x &= 0xFFFF; z &= 0xFFFF
        if x >= self.cols or z >= self.rows:
            return
        self.v[z][x] = val
    def sample(self, x, y):
        fx = (x - self.x0) * self.ix
        fy = (y - self.y0) * self.iy
        x1 = trunc_u16(fx); x2 = (x1 + 1) & 0xFFFF
        y1 = trunc_u16(fy); y2 = (y1 + 1) & 0xFFFF
        fx -= math.floor(fx); fy -= math.floor(fy)
        if x1 >= self.cols: x1 = 0
        if x2 >= self.cols: x2 = 0
        if y1 >= self.rows: y1 = 0
        if y2 >= self.rows: y2 = 0
        w11 = (1 - fx) * (1 - fy)
        w12 = (1 - fx) * fy
        w21 = fx * (1 - fy)
        w22 = fx * fy
        return ((self.v[y1][x1] * w11 + self.v[y2][x1] * w12) + self.v[y1][x2] * w21) + self.v[y2][x2] * w22
    def highest(self):
        best = self.get(0, 0)
        for z in range(self.rows):
            for x in range(self.rows):          # rows for both bounds (reference scan)
                h = self.get(x, z)
                if h > best:
                    best = h
        return best

class Tile:
    def __init__(self, posX, posY, posZ, size, fd, hs, desired, method, seed, ls, power, guide=None):
        self.posX, self.posY, self.posZ = posX, f32(posY), posZ
        self.size = size
        self.fd, self.hs, self.desired = f32(fd), f32(hs), f32(desired)
        self.method, self.seed_value, self.ls = method, seed, f32(ls)
        self.power = power
        self.guide = guide            # guide[z][x], bytes 0..255
        self.rng = TileRng()

    def generate(self, cell, outside, trace=None):
        req = int(self.size / cell)
        k = pow2ceil(req - 1)
        n = int(rpow(2.0, float(k))) + 1
        self.n = n
        g = Grid(n, n, 0.0, 0.0, 1.0, 1.0, outside)
        self.grid = g
        p = self.power
        if p > 0:
            r = (1 << p) - 1
            scale = (n - 1) // (r + 1)
            for z in range(r):
                for x in range(r):
                    g.put((x + 1) * scale, (z + 1) * scale, float(self.guide[z][x]))
                    if trace is not None:
                        trace.append(('pin', (x + 1) * scale, (z + 1) * scale, float(self.guide[z][x])))
        fdm = 30.7 * rexp(-6.5 * self.fd)
        sm = 15.353 * rexp(-3.1 * self.ls)
        comp = self.hs
        comp *= fdm
        comp *= sm
        self.comp = comp
        iterations = pow2ceil(n) - p
        step = (n - 1) >> p
        self.rng.seed(self.seed_value)
        rng = self.rng
        method = self.method

        def noise(h, b):
            L = 256.0 * h / n
            R = rpow(L * 10.0, self.fd)
            d = rng.next()
            val = (0.5 - d / 32767.0) * R
            val *= comp
            val *= 0.1 + rpow(abs(f32(b)), self.ls) * 0.15
            return d, val

        def square(x, z, h):
            A = g.get(x - h, z - h); B = g.get(x - h, z + h)
            C = g.get(x + h, z - h); D = g.get(x + h, z + h)
            choice = None
            if method == 0:
                b = (A + B + C + D) * 0.25
            elif method == 1:
                choice = rng.next()
                b = (A + D) * 0.5 if (choice & 1) else (B + C) * 0.5
            else:
                choice = rng.next(); rr = choice & 0xFFFF
                b = A if rr < 0x3FFF else B if rr < 0x7FFF else C if rr < 0xBFFF else D
            d, nz = noise(h, b)
            g.put(x, z, b + nz)
            if trace is not None:
                trace.append(('square', x, z, h, choice, b, d, nz, b + nz))

        def diamond(x, z, h):
            W = g.get(x - h, z); E = g.get(x + h, z)
            N = g.get(x, z + h); S = g.get(x, z - h)
            choice = None
            if method == 0:
                b = (W + E + S + N) * 0.25
            elif method == 1:
                choice = rng.next()
                b = (W + E) * 0.5 if (choice & 1) else (N + S) * 0.5
            else:
                choice = rng.next(); rr = choice & 0xFFFF
                b = W if rr < 0x3FFF else E if rr < 0x7FFF else N if rr < 0xBFFF else S
            d, nz = noise(h, b)
            g.put(x, z, b + nz)
            if trace is not None:
                trace.append(('diamond', x, z, h, choice, b, d, nz, b + nz))

        self.calls = 0
        for _ in range(iterations):
            for x in range(0, n - 1, step):
                for z in range(0, n - 1, step):
                    h = step >> 1
                    mx, mz = x + h, z + h
                    square(mx, mz, h); self.calls += 1
                    if z != 0:
                        diamond(mx, z, h); self.calls += 1
                    if x != 0:
                        diamond(x, mz, h); self.calls += 1
            step >>= 1
        for x in range(n):
            for z in range(n):
                g.v[z][x] = g.v[z][x] + self.posY
        self.k, self.iterations = k, iterations

class Terrain:
    def __init__(self, worldX, worldZ, cell, outside, maxHeight, tiles, areas):
        self.cell = f32(cell)
        self.outside = f32(outside)
        self.maxHeight = f32(maxHeight)
        cols = int(math.ceil(worldX / self.cell)) & 0xFFFF
        rows = int(math.ceil(worldZ / self.cell)) & 0xFFFF
        self.world = Grid(cols, rows, 0.0, 0.0, self.cell, self.cell, self.outside)
        self.tiles, self.areas = tiles, areas
        self.neg_frac_calls = 0

    def generate(self):
        w = self.world
        for t in self.tiles:
            t.generate(self.cell, self.outside)
            self.merge(t)
        for a in self.areas:
            self.flatten(*a)
        self.sizeX = self.cell * w.cols
        self.sizeZ = self.cell * w.rows

    def merge(self, t):
        w, cell, outside = self.world, self.cell, self.outside
        posXi = trunc_u16(t.posX / cell + 0.5)
        posZi = trunc_u16(t.posZ / cell + 0.5)
        numCells = int(t.size / cell)
        rng_ = t.grid.highest() - outside + 0.001
        hf = (t.desired - outside) / rng_
        t.highest_after_shift, t.range, t.hf = t.grid.highest(), rng_, hf
        t.posXi, t.posZi, t.numCells = posXi, posZi, numCells
        tg = t.grid
        tg.x0, tg.y0 = float(t.posX), float(t.posZ)
        tg.cx = tg.cy = t.size / float(t.n)
        tg.ix = tg.iy = 1.0 / tg.cx
        t.tilecell = tg.cx
        for z in range(numCells):
            wz = (z + posZi) * cell + 0.0
            for x in range(numCells):
                wx = (x + posXi) * cell + 0.0
                fx = (wx - tg.x0) * tg.ix
                fy = (wz - tg.y0) * tg.iy
                if (-1 < fx < 0) or (-1 < fy < 0):
                    self.neg_frac_calls += 1
                h1 = tg.sample(wx, wz)
                h1 = h1 - outside
                h1 = h1 * hf
                h1 = h1 + outside
                if h1 > w.get(x + posXi, z + posZi):
                    w.put(x + posXi, z + posZi, h1)

    def flatten(self, cx, cy, cz, size, ftype=0, thr=40.0):
        w = self.world
        cx, t, cz, size, T = f32(cx), f32(cy), f32(cz), f32(size), f32(thr)
        def idx(r):
            return int((r - 0.0) * w.ix)
        x1 = idx(cx - size) + 1; x2 = idx(cx + size) + 1
        z1 = idx(cz - size) + 1; z2 = idx(cz + size) + 1
        passes = 2 if ftype == 3 else 1
        for _ in range(passes):
            for z in range(z1, z2):
                for x in range(x1, x2):
                    cur = w.get(x, z); v = t
                    if ftype == 1: v = t + cur
                    elif ftype == 2: v = cur - t if cur > T else cur
                    elif ftype == 4: v = cur - t if cur < T else cur
                    elif ftype == 3:
                        if cur > T:
                            v = cur * (t / cur)
                            v += w.get(x - 1, z); v += w.get(x + 1, z)
                            v += w.get(x, z + 1); v += w.get(x, z - 1)
                            v /= 6.0
                        if v > cur: v = cur
                    w.put(x, z, v)

    def normals(self):
        w = self.world; c_ = self.cell
        out = [[None] * w.cols for _ in range(w.rows)]
        def norm(v):
            l2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2]
            if l2 > 1e-10:
                inv = 1.0 / rsqrt(l2)
                return (v[0] * inv, v[1] * inv, v[2] * inv)
            return (0.0, 0.0, 1.0)
        def cross(a, b):
            return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
        for z in range(w.rows):
            for x in range(w.cols):
                hN = w.get(x, z - 1); hW = w.get(x + 1, z); hE = w.get(x - 1, z); hS = w.get(x, z + 1)
                if hN == hW and hE == hS and hN == hE:
                    out[z][x] = (0.0, 1.0, 0.0); continue
                c = w.get(x, z)
                vN = (0.0, c - hN, c_); vW = (-c_, c - hW, 0.0)
                vS = (0.0, c - hS, -c_); vE = (c_, c - hE, 0.0)
                nA = norm(cross(vW, vN)); nB = norm(cross(vE, vS))
                out[z][x] = norm((nA[0] + nB[0], nA[1] + nB[1], nA[2] + nB[2]))
        return out

# ---- parameter sets (terrain.md §7) ----------------------------------------
Z3 = [[0, 0, 0], [0, 0, 0], [0, 0, 0]]

def set_a():
    tiles = [
        Tile(204, -3.98, 260, 2264, 1.20, 4.00, 220, 1, 269491, 1.00, 2, Z3),
        Tile(1274, 1.57, 132, 1014, 1.20, 4.00, 220, 1, 904945, 1.00, 2, Z3),
        Tile(1129, 0.00, 985, 834, 1.20, 1.00, 300, 0, 1, 1.00, 0),
    ]
    return Terrain(2700, 2700, 15.76, -15.39, 0.0, tiles, [])

GUIDE_B = [[0, 0, 255], [0, 255, 255], [0, 255, 0]]

def tile_b(posY=0.0, desired=391.0):
    return Tile(100, posY, 100, 344, 5.0, 1.0, desired, 2, 1, 1.0, 2, GUIDE_B)

def fmt(x):
    return repr(x)

def main(argv):
    mode = argv[1] if len(argv) > 1 else 'selftest'
    if mode == 'log2':
        bad = [x for x in range(1, 70000) if pow2ceil(x) != (x - 1).bit_length()]
        print('log2 mismatches 1..69999:', len(bad), bad[:10])
        return
    if mode in ('traceA', 'full', 'selftest'):
        T = set_a()
        print('cell', fmt(T.cell), 'outside', fmt(T.outside), 'cols', T.world.cols, 'rows', T.world.rows)
        fdm = 30.7 * rexp(-6.5 * f32(1.2)); sm = 15.353 * rexp(-3.1 * 1.0)
        print('fdm', fmt(fdm), 'sm', fmt(sm))
        tr = []
        t2 = T.tiles[2]
        t2.generate(T.cell, T.outside, tr)
        print('tile2 n', t2.n, 'comp', fmt(t2.comp), 'calls', t2.calls)
        for e in tr[:10]:
            print('  ', e)
        T = set_a(); T.generate()
        for i, t in enumerate(T.tiles):
            print('tile', i, 'n', t.n, 'k', t.k, 'iter', t.iterations, 'comp', fmt(t.comp), 'highest', fmt(t.highest_after_shift),
                  'range', fmt(t.range), 'hf', fmt(t.hf), 'posXi', t.posXi, 'posZi', t.posZi, 'numCells', t.numCells,
                  'tilecell', fmt(t.tilecell), 'calls', t.calls)
        w = T.world
        allv = [v for row in w.v for v in row]
        print('world min', fmt(min(allv)), 'max', fmt(max(allv)), 'sizeX', fmt(T.sizeX))
        for (x, z) in [(0, 0), (43, 43), (86, 86), (129, 57)]:
            print('h[%d,%d] =' % (x, z), fmt(w.get(x, z)))
        print('negative-fraction merge samples', T.neg_frac_calls)
        print('land fraction', sum(1 for v in allv if v > 0) / len(allv))
        if mode == 'full':
            nm = T.normals()
            for (x, z) in [(43, 43), (86, 86), (129, 57), (0, 0)]:
                print('n[%d,%d] =' % (x, z), nm[z][x])
            print('h at (500.5, 700.25) =', fmt(w.sample(500.5, 700.25)))
            print('h at (-5, 100) =', fmt(w.sample(-5.0, 100.0)))
    if mode in ('traceM1', 'selftest'):
        T = set_a()
        t0 = T.tiles[0]
        tr = []
        t0.generate(T.cell, T.outside, tr)
        print('method1 tile0 events (pins first):')
        for e in tr[:13]:
            print('  ', e)
        print('  calls', t0.calls, 'events', 9 + 4 * t0.calls)
    if mode in ('traceB', 'selftest'):
        tb = tile_b()
        tr = []
        tb.generate(20.0, f32(-26.13), tr)
        print('set B comp', fmt(tb.comp), 'n', tb.n, 'calls', tb.calls, 'events', 9 + 4 * tb.calls)
        for e in tr[9:14]:
            print('  ', e)
        hb = tb.grid.highest()
        print('set B highest with posY 0:', fmt(hb))
        target = 263.7423632514278
        print('implied posY:', fmt(target - hb), 'f32(8.74)=', fmt(f32(8.74)))
        for cand in [8.74, 8.75, 8.7, 8.73]:
            t3 = tile_b(posY=cand); t3.generate(20.0, f32(-26.13))
            h3 = t3.grid.highest()
            o = f32(-26.13)
            print('  posY', cand, 'highest', fmt(h3), 'hf(desired 391)', fmt((f32(391.0) - o) / (h3 - o + 0.001)))
    if mode == 'smooth':
        cur = 20.305899393655125; t = f32(62.0)
        v = cur * (t / cur)
        print('v0', fmt(v))
        for nb in [6.921732493425324, 21.20888188060145, 27.02387309504114, 2.4006731601979148]:
            v += nb; print('  ', fmt(v))
        print('result', fmt(v / 6.0))
        c = cur * (t / cur)
        alt = (c + (((6.921732493425324 + 21.20888188060145) + 27.02387309504114) + 2.4006731601979148)) / 6.0
        print('neighbours-first', fmt(alt))

if __name__ == '__main__':
    main(sys.argv)
