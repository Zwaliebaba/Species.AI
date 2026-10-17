#!/usr/bin/env python3
"""TerrainGenerator.py: the conformance generator of the terrain component.

An independent implementation of height-field generation, written from the terrain specification alone
(Design/architecture/components/terrain/: README.md §4.1 and §7, height_field_generation.md §4.2 to §4.9 and §7.2,
edges_normals_colours_and_sea.md §4.10 and §4.11). It reproduces the vector sets of height_field_generation.md §7.2,
which tests T1 to T3 and T7 to T13 of README.md §7.1 use, and it is the generator that new vector sets come from.

Usage:
  python3 Tools/DesignModels/TerrainGenerator.py [check | traceA | traceM1 | traceB | smooth | flatten | full | log2]

  check    (default) compute vector sets A to D, the tile generator, the decimal rounding and ceilLog2, and compare
           every value with the specification by exact equality; prints one line per test, exit status 1 on a mismatch
  traceA   set A: terrain values, the first ten events of tile 2 (table 7.2-C), the derived values of table 7.2-B and
           the probe samples
  traceM1  set A, tile 0 (method 1): the nine guide pins and the first four calls (table 7.2-D)
  traceB   set B (method 2): the first five calls (table 7.2-F), the highest sample and the height factor
  smooth   set C: one smooth evaluation (table 7.2-G)
  flatten  set D: the five flatten areas applied in order (table 7.2-H)
  full     traceA, then the simulation normals at the probe samples and two bilinear reads
  log2     ceilLog2 against the exact value for every integer from 1 to 69 999 (test T3)

Values are printed with repr(), which gives the shortest text that parses back to the same binary64 value. No input
files; the parameter sets are those of §7.2. Python binary64 arithmetic is IEEE-754 with rounding to nearest
(TER-061); binary32 rounding is done explicitly through struct (TER-066).
"""
import math
import struct
import sys


def f32(x):
  """Round a binary64 value to binary32 (nearest, ties to even) and widen it back (TER-066)."""
  return struct.unpack('<f', struct.pack('<f', x))[0]


# ---------------------------------------------------------------- numeric primitives (height_field_generation §4.2)
C_LOG2 = 1.0 / f32(math.log(2.0))


def rlog(x):
  return f32(math.log(x))


def log2r(x):
  return rlog(x) * C_LOG2


def ceil_log2(x):
  """ceilLog2 through the rounded logarithm (TER-065, TER-067)."""
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
  """A binary64 coordinate fraction to an unsigned 16-bit index, truncating toward zero; defined for v > -1
  (TER-071)."""
  assert v > -1.0
  return int(v) & 0xFFFF


# ---------------------------------------------------------------- tile generator (height_field_generation §4.3)
class TileRng:
  """The private generator of one tile (TER-068, TER-080)."""

  def __init__(self):
    self.state = 1

  def seed(self, s):
    self.state = s & 0xFFFFFFFF

  def next(self):
    self.state = (self.state * 214013 + 2531011) & 0xFFFFFFFF
    return (self.state >> 16) & 0x7FFF

  def signed(self, r):
    return (0.5 - self.next() / 32767.0) * r


# ---------------------------------------------------------------- grids (edges_normals_colours_and_sea §4.10)
class Grid:
  """A rectangular grid with an origin, a cell size and an outside value; v[z][x]."""

  def __init__(self, cols, rows, x0, y0, cx, cy, outside):
    self.cols, self.rows = cols, rows
    self.x0, self.y0 = x0, y0
    self.cx, self.cy = cx, cy
    self.ix, self.iy = 1.0 / cx, 1.0 / cy
    self.outside = outside
    self.v = [[outside] * cols for _ in range(rows)]

  def get(self, x, z):
    """Integer read; outside the grid it returns the outside value (TER-072)."""
    x &= 0xFFFF
    z &= 0xFFFF
    if x >= self.cols or z >= self.rows:
      return self.outside
    return self.v[z][x]

  def put(self, x, z, val):
    """Integer write; outside the grid it changes nothing (TER-072)."""
    x &= 0xFFFF
    z &= 0xFFFF
    if x >= self.cols or z >= self.rows:
      return
    self.v[z][x] = val

  def sample(self, x, y):
    """Bilinear read with the weights-first sum (TER-073)."""
    fx = (x - self.x0) * self.ix
    fy = (y - self.y0) * self.iy
    x1 = trunc_u16(fx)
    x2 = (x1 + 1) & 0xFFFF
    y1 = trunc_u16(fy)
    y2 = (y1 + 1) & 0xFFFF
    fx -= math.floor(fx)
    fy -= math.floor(fy)
    if x1 >= self.cols:
      x1 = 0
    if x2 >= self.cols:
      x2 = 0
    if y1 >= self.rows:
      y1 = 0
    if y2 >= self.rows:
      y2 = 0
    w11 = (1 - fx) * (1 - fy)
    w12 = (1 - fx) * fy
    w21 = fx * (1 - fy)
    w22 = fx * fy
    return ((self.v[y1][x1] * w11 + self.v[y2][x1] * w12) + self.v[y1][x2] * w21) + self.v[y2][x2] * w22

  def highest(self):
    """Starts with get(0, 0); both loops are bounded by the row count (TER-076)."""
    best = self.get(0, 0)
    for z in range(self.rows):
      for x in range(self.rows):
        h = self.get(x, z)
        if h > best:
          best = h
    return best


# ---------------------------------------------------------------- tiles (height_field_generation §4.4 to §4.7)
class Tile:
  def __init__(self, pos_x, pos_y, pos_z, size, fd, hs, desired, method, seed, ls, power, guide=None):
    self.pos_x, self.pos_y, self.pos_z = pos_x, f32(pos_y), pos_z
    self.size = size
    self.fd, self.hs, self.desired = f32(fd), f32(hs), f32(desired)
    self.method, self.seed_value, self.ls = method, seed, f32(ls)
    self.power = power
    self.guide = guide            # guide[z][x], bytes 0 to 255
    self.rng = TileRng()

  def generate(self, cell, outside, trace=None):
    """TER-091: size the grid, pin the guide points, run the square and diamond passes, shift by position y."""
    req = int(self.size / cell)
    k = ceil_log2(req - 1)
    n = int(rpow(2.0, float(k))) + 1
    self.req, self.n = req, n
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
    iterations = ceil_log2(n) - p
    step = (n - 1) >> p
    self.first_step = step
    self.rng.seed(self.seed_value)
    rng = self.rng
    method = self.method

    def noise(h, b):
      """TER-105."""
      length = 256.0 * h / n
      r = rpow(length * 10.0, self.fd)
      d = rng.next()
      val = (0.5 - d / 32767.0) * r
      val *= comp
      val *= 0.1 + rpow(abs(f32(b)), self.ls) * 0.15
      return d, val

    def square(x, z, h):
      """TER-100 to TER-102: parents A, B, C, D."""
      a = g.get(x - h, z - h)
      b_ = g.get(x - h, z + h)
      c = g.get(x + h, z - h)
      d_ = g.get(x + h, z + h)
      choice = None
      if method == 0:
        base = (a + b_ + c + d_) * 0.25
      elif method == 1:
        choice = rng.next()
        base = (a + d_) * 0.5 if (choice & 1) else (b_ + c) * 0.5
      else:
        choice = rng.next()
        rr = choice & 0xFFFF
        base = a if rr < 0x3FFF else b_ if rr < 0x7FFF else c if rr < 0xBFFF else d_
      d, nz = noise(h, base)
      g.put(x, z, base + nz)
      if trace is not None:
        trace.append(('square', x, z, h, choice, base, d, nz, base + nz))

    def diamond(x, z, h):
      """TER-100 to TER-102: parents W, E, N, S."""
      w = g.get(x - h, z)
      e = g.get(x + h, z)
      nn = g.get(x, z + h)
      s = g.get(x, z - h)
      choice = None
      if method == 0:
        base = (w + e + s + nn) * 0.25
      elif method == 1:
        choice = rng.next()
        base = (w + e) * 0.5 if (choice & 1) else (nn + s) * 0.5
      else:
        choice = rng.next()
        rr = choice & 0xFFFF
        base = w if rr < 0x3FFF else e if rr < 0x7FFF else nn if rr < 0xBFFF else s
      d, nz = noise(h, base)
      g.put(x, z, base + nz)
      if trace is not None:
        trace.append(('diamond', x, z, h, choice, base, d, nz, base + nz))

    self.calls = 0
    for _ in range(iterations):          # TER-107: the write order of a pass
      for x in range(0, n - 1, step):
        for z in range(0, n - 1, step):
          h = step >> 1
          mx, mz = x + h, z + h
          square(mx, mz, h)
          self.calls += 1
          if z != 0:
            diamond(mx, z, h)
            self.calls += 1
          if x != 0:
            diamond(x, mz, h)
            self.calls += 1
      step >>= 1
    for x in range(n):
      for z in range(n):
        g.v[z][x] = g.v[z][x] + self.pos_y
    self.k, self.iterations = k, iterations


def smooth_value(cur, t, threshold, neighbours):
  """The smooth operation on one sample: above the threshold, cur × (t ÷ cur) plus the four neighbours in read order,
  divided by 6; never above cur (TER-143 to TER-145)."""
  v = t
  if cur > threshold:
    v = cur * (t / cur)
    for nb in neighbours:
      v += nb
    v /= 6.0
  return cur if v > cur else v


FLATTEN_ABSOLUTE, FLATTEN_ADD, FLATTEN_SUBTRACT, FLATTEN_SMOOTH, FLATTEN_SUBTRACT2 = 0, 1, 2, 3, 4


# ------------------------------------------------- terrain (README §4.1, height_field_generation §4.8 and §4.9)
class Terrain:
  def __init__(self, world_x, world_z, cell, outside, max_height, tiles, areas):
    self.cell = f32(cell)
    self.outside = f32(outside)
    self.max_height = f32(max_height)
    cols = int(math.ceil(world_x / self.cell)) & 0xFFFF
    rows = int(math.ceil(world_z / self.cell)) & 0xFFFF
    self.world = Grid(cols, rows, 0.0, 0.0, self.cell, self.cell, self.outside)
    self.tiles, self.areas = tiles, areas
    self.neg_frac_calls = 0

  def generate(self):
    """TER-060: every tile in record order is generated and merged, then the flatten areas in record order."""
    w = self.world
    for t in self.tiles:
      t.generate(self.cell, self.outside)
      self.merge(t)
    for a in self.areas:
      self.flatten(*a)
    self.size_x = self.cell * w.cols
    self.size_z = self.cell * w.rows

  def merge(self, t):
    """TER-120 to TER-122: remap the tile about the outside height and keep the higher sample."""
    w, cell, outside = self.world, self.cell, self.outside
    pos_xi = trunc_u16(t.pos_x / cell + 0.5)
    pos_zi = trunc_u16(t.pos_z / cell + 0.5)
    num_cells = int(t.size / cell)
    span = t.grid.highest() - outside + 0.001
    hf = (t.desired - outside) / span
    t.highest_after_shift, t.range, t.hf = t.grid.highest(), span, hf
    t.pos_xi, t.pos_zi, t.num_cells = pos_xi, pos_zi, num_cells
    tg = t.grid
    tg.x0, tg.y0 = float(t.pos_x), float(t.pos_z)
    tg.cx = tg.cy = t.size / float(t.n)
    tg.ix = tg.iy = 1.0 / tg.cx
    t.tile_cell = tg.cx
    for z in range(num_cells):
      wz = (z + pos_zi) * cell + 0.0
      for x in range(num_cells):
        wx = (x + pos_xi) * cell + 0.0
        fx = (wx - tg.x0) * tg.ix
        fy = (wz - tg.y0) * tg.iy
        if (-1 < fx < 0) or (-1 < fy < 0):
          self.neg_frac_calls += 1
        h1 = tg.sample(wx, wz)
        h1 = h1 - outside
        h1 = h1 * hf
        h1 = h1 + outside
        if h1 > w.get(x + pos_xi, z + pos_zi):
          w.put(x + pos_xi, z + pos_zi, h1)

  def flatten(self, cx, cy, cz, size, ftype=FLATTEN_ABSOLUTE, thr=40.0):
    """TER-140 to TER-145: one area, applied in place, row by row; the smooth operation makes two passes."""
    w = self.world
    cx, t, cz, size, threshold = f32(cx), f32(cy), f32(cz), f32(size), f32(thr)

    def idx(r):
      return int((r - 0.0) * w.ix)

    x1 = idx(cx - size) + 1
    x2 = idx(cx + size) + 1
    z1 = idx(cz - size) + 1
    z2 = idx(cz + size) + 1
    passes = 2 if ftype == FLATTEN_SMOOTH else 1
    for _ in range(passes):
      for z in range(z1, z2):
        for x in range(x1, x2):
          cur = w.get(x, z)
          v = t
          if ftype == FLATTEN_ADD:
            v = t + cur
          elif ftype == FLATTEN_SUBTRACT:
            v = cur - t if cur > threshold else cur
          elif ftype == FLATTEN_SUBTRACT2:
            v = cur - t if cur < threshold else cur
          elif ftype == FLATTEN_SMOOTH:
            v = smooth_value(cur, t, threshold, (w.get(x - 1, z), w.get(x + 1, z), w.get(x, z + 1), w.get(x, z - 1)))
          w.put(x, z, v)

  def normals(self):
    """The simulation normal grid (TER-170, TER-171)."""
    w = self.world
    c_ = self.cell
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
        h_n = w.get(x, z - 1)
        h_w = w.get(x + 1, z)
        h_e = w.get(x - 1, z)
        h_s = w.get(x, z + 1)
        if h_n == h_w and h_e == h_s and h_n == h_e:
          out[z][x] = (0.0, 1.0, 0.0)
          continue
        c = w.get(x, z)
        v_n = (0.0, c - h_n, c_)
        v_w = (-c_, c - h_w, 0.0)
        v_s = (0.0, c - h_s, -c_)
        v_e = (c_, c - h_e, 0.0)
        n_a = norm(cross(v_w, v_n))
        n_b = norm(cross(v_e, v_s))
        out[z][x] = norm((n_a[0] + n_b[0], n_a[1] + n_b[1], n_a[2] + n_b[2]))
    return out


# ---------------------------------------------------------------- vector sets (height_field_generation §7.2)
ZERO_GUIDE = [[0, 0, 0], [0, 0, 0], [0, 0, 0]]
GUIDE_B = [[0, 0, 255], [0, 255, 255], [0, 255, 0]]


def set_a():
  """Table 7.2-A."""
  tiles = [
    Tile(204, -3.98, 260, 2264, 1.20, 4.00, 220, 1, 269491, 1.00, 2, ZERO_GUIDE),
    Tile(1274, 1.57, 132, 1014, 1.20, 4.00, 220, 1, 904945, 1.00, 2, ZERO_GUIDE),
    Tile(1129, 0.00, 985, 834, 1.20, 1.00, 300, 0, 1, 1.00, 0),
  ]
  return Terrain(2700, 2700, 15.76, -15.39, 0.0, tiles, [])


def tile_b():
  return Tile(100, 0.0, 100, 344, 5.0, 1.0, 391.0, 2, 1, 1.0, 2, GUIDE_B)


def set_b():
  return Terrain(1000, 1000, 20.0, -26.13, 0.0, [tile_b()], [])


SET_C = {'t': 62.0, 'threshold': 11.7, 'cur': 20.305899393655125,
         'neighbours': (6.921732493425324, 21.20888188060145, 27.02387309504114, 2.4006731601979148)}

SET_D_AREAS = [
  (60, 5, 60, 20, FLATTEN_ABSOLUTE),
  (60, 24.1, 60, 8, FLATTEN_ADD),
  (60, 4, 60, 20, FLATTEN_SUBTRACT, 10),
  (60, 3, 60, 20, FLATTEN_SUBTRACT2, 6),
  (60, 1, 60, 20, FLATTEN_SMOOTH, 0.5),
]


def set_d():
  return Terrain(120, 120, 12, -10, 0.0, [], [])


def block(terrain):
  """Samples (i, j) for i, j from 4 to 6, row by row (table 7.2-H)."""
  return [[terrain.world.get(i, j) for i in range(4, 7)] for j in range(4, 7)]


def smooth_trace(c):
  v = c['cur'] * (c['t'] / c['cur'])
  sums = []
  for nb in c['neighbours']:
    v += nb
    sums.append(v)
  return c['cur'] * (c['t'] / c['cur']), sums


# ---------------------------------------------------------------- expected values (height_field_generation §7.2)
EXPECTED_A_TILES = [
  # req, k, n, passes, first step, comp, highest, range, height factor, posXi, posZi, numCells, tile cell, calls
  (143, 8, 257, 7, 64, 0.03480018982936636, 11.67633525289311, 27.067335596215866, 8.696459963951211, 13, 16, 143,
   8.809338521400779, 261112),
  (64, 6, 65, 5, 16, 0.03480018982936636, 27.39792619447322, 42.78892653779597, 5.501189662596465, 81, 8, 64, 15.6,
   16120),
  (52, 6, 65, 7, 64, 0.00870004745734159, 39.5289374012995, 54.91993774462225, 5.742723194805618, 72, 62, 52,
   12.830769230769231, 16129),
]
EXPECTED_A_TERRAIN = {
  'cell size': 15.760000228881836, 'outside height': -15.390000343322754, 'fd modifier': 0.012578859674977138,
  'ls modifier': 0.6916403936557471, 'columns': 172, 'rows': 172, 'extent': 2710.720039367676,
  'lowest': -15.390000343322754, 'highest': 273.38014287056404, 'h(0, 0)': -15.390000343322754,
  'h(43, 43)': 64.11465376675797, 'h(86, 86)': 99.71780778328521, 'h(129, 57)': 80.84683847539341,
  'negative-fraction merge reads': 259,
}
EXPECTED_A_TILE2_EVENTS = [   # table 7.2-C: call, x, z, half-step, draw, base, noise, written
  ('square', 32, 32, 32, 41, -15.390000343322754, 54.918937744622255, 39.5289374012995),
  ('square', 16, 16, 16, 18467, -1.6602659071671901, -0.4416614889824001, -2.1019273961495903),
  ('square', 16, 48, 16, 6334, -1.6602659071671901, 2.1302986002057267, 0.47003269303853656),
  ('diamond', 16, 32, 16, 26500, 5.626760588716423, -5.800023064758535, -0.17326247604211176),
  ('square', 48, 16, 16, 19169, -1.6602659071671901, -0.5904718394818702, -2.2507377466490603),
  ('diamond', 32, 16, 16, 15724, 4.9465679787945245, 0.3372408885970395, 5.2838088673915635),
  ('square', 48, 48, 16, 11478, -1.6602659071671901, 1.0398706187680178, -0.6203952883991724),
  ('diamond', 48, 32, 16, 29358, 5.316951005732128, -7.072397743527255, -1.7554467377951273),
  ('diamond', 32, 48, 16, 26962, 5.9971436156540285, -6.421832127879571, -0.4246885122255426),
  ('square', 8, 8, 8, 24464, -12.067982106529463, -4.0803875962216285, -16.14836970275109),
]
EXPECTED_A_TILE0_PINS = [(64, 64), (128, 64), (192, 64), (64, 128), (128, 128), (192, 128), (64, 192), (128, 192),
                         (192, 192)]
EXPECTED_A_TILE0_CALLS = [   # table 7.2-D: call, x, z, half-step, choice draw, base, noise draw, noise, written
  ('square', 32, 32, 32, 28114, -15.390000343322754, 13609, 7.16513913931825, -8.224861204004505),
  ('square', 32, 96, 32, 12878, -7.695000171661377, 28951, -16.901538996064705, -24.596539167726082),
  ('diamond', 32, 64, 32, 17157, -7.695000171661377, 20365, -5.354563557814331, -13.049563729475707),
  ('square', 32, 160, 32, 23403, -7.695000171661377, 14881, 2.020653458650265, -5.674346713011111),
]
EXPECTED_B = {'outside height': -26.1299991607666, 'req': 17, 'k': 4, 'n': 17, 'comp': 1.6309778697705958e-13,
              'passes': 3, 'first step': 4, 'calls': 952, 'events': 3817, 'highest': 263.7423632514278,
              'height factor': 1.4390076952556121}
EXPECTED_B_CALLS = [   # table 7.2-F
  ('square', 2, 2, 2, 41, -26.1299991607666, 18467, -0.10329567230285541, -26.233294833069458),
  ('square', 2, 6, 2, 6334, -26.1299991607666, 26500, -0.5015553966171525, -26.631554557383755),
  ('diamond', 2, 4, 2, 19169, 0.0, 15724, 0.0008134510092106354, 0.0008134510092106354),
  ('square', 2, 10, 2, 11478, -26.1299991607666, 29358, -0.6432491962051347, -26.773248356971735),
  ('diamond', 2, 8, 2, 26962, 0.0, 24464, -0.009966779196249498, -0.009966779196249498),
]
EXPECTED_C = {'v': 61.99999999999999, 'sums': [68.92173249342531, 90.13061437402676, 117.1544874690679,
                                               119.55516062926581],
              'result': 19.925860104877636, 'neighbours first': 19.92586010487764}
EXPECTED_D = [   # table 7.2-H, after each area
  [[5, 5, 5], [5, 5, 5], [5, 5, 5]],
  [[5, 5, 5], [5, 29.100000381469727, 5], [5, 5, 5]],
  [[5, 5, 5], [5, 25.100000381469727, 5], [5, 5, 5]],
  [[2, 2, 2], [2, 25.100000381469727, 2], [2, 2, 2]],
  [[-2.5, -2.0833333333333335, -2.5], [-2.0833333333333335, -0.9722222222222223, -1.3333333333333333],
   [-2.5, -1.3333333333333333, -3.611111111111111]],
]
EXPECTED_T1 = [41, 18467, 6334, 26500, 19169, 15724, 11478, 29358]
EXPECTED_T2 = [(15.76, 15.760000228881836), (-15.39, -15.390000343322754), (1.2, 1.2000000476837158),
               (-26.13, -26.1299991607666), (24.1, 24.100000381469727), (11.7, 11.699999809265137)]


# ---------------------------------------------------------------- check
def check():
  results = []

  def test(name, got, want):
    results.append((name, got == want))
    if got != want:
      print(f'  mismatch in {name}:\n    got  {got!r}\n    want {want!r}')

  rng = TileRng()
  rng.seed(1)
  test('T1 tile generator after Seed(1)', [rng.next() for _ in range(8)], EXPECTED_T1)
  draw_zero = TileRng()
  draw_zero.next = lambda: 0
  test('T1 Signed(R) for a draw of 0', draw_zero.signed(3.0), 1.5)
  test('T2 decimal rounding', [f32(a) for a, _ in EXPECTED_T2], [b for _, b in EXPECTED_T2])
  test('T3 ceilLog2 for 1 to 69 999', [x for x in range(1, 70000) if ceil_log2(x) != (x - 1).bit_length()], [])

  terrain = set_a()
  t2 = terrain.tiles[2]
  trace = []
  t2.generate(terrain.cell, terrain.outside, trace)
  events = [(e[0], e[1], e[2], e[3], e[6], e[5], e[7], e[8]) for e in trace[:10]]
  test('T8 set A tile 2, first ten events (table 7.2-C)', events, EXPECTED_A_TILE2_EVENTS)
  test('T8 set A tile 2, calls and events', (t2.calls, 3 * t2.calls), (16129, 48387))

  terrain = set_a()
  t0 = terrain.tiles[0]
  trace = []
  t0.generate(terrain.cell, terrain.outside, trace)
  test('T9 set A tile 0, guide pins (table 7.2-D)', [(e[1], e[2], e[3]) for e in trace[:9]],
       [(x, z, 0.0) for x, z in EXPECTED_A_TILE0_PINS])
  test('T9 set A tile 0, first four calls (table 7.2-D)', [tuple(e) for e in trace[9:13]],
       [tuple(c) for c in EXPECTED_A_TILE0_CALLS])
  test('T9 set A tile 0, calls and events', (t0.calls, 9 + 4 * t0.calls), (261112, 1044457))

  terrain = set_a()
  terrain.generate()
  got = [(t.req, t.k, t.n, t.iterations, t.first_step, t.comp, t.highest_after_shift, t.range, t.hf, t.pos_xi,
          t.pos_zi, t.num_cells, t.tile_cell, t.calls) for t in terrain.tiles]
  test('T7 set A derived values (table 7.2-B)', got, EXPECTED_A_TILES)
  w = terrain.world
  values = [v for row in w.v for v in row]
  got = {'cell size': terrain.cell, 'outside height': terrain.outside,
         'fd modifier': 30.7 * rexp(-6.5 * f32(1.2)), 'ls modifier': 15.353 * rexp(-3.1 * 1.0),
         'columns': w.cols, 'rows': w.rows, 'extent': terrain.size_x, 'lowest': min(values), 'highest': max(values),
         'h(0, 0)': w.get(0, 0), 'h(43, 43)': w.get(43, 43), 'h(86, 86)': w.get(86, 86),
         'h(129, 57)': w.get(129, 57), 'negative-fraction merge reads': terrain.neg_frac_calls}
  test('T7 and T11 set A terrain values and probe samples', got, EXPECTED_A_TERRAIN)

  terrain = set_b()
  tb = terrain.tiles[0]
  trace = []
  tb.generate(terrain.cell, terrain.outside, trace)
  test('T10 set B, first five calls (table 7.2-F)', [tuple(e) for e in trace[9:14]],
       [tuple(c) for c in EXPECTED_B_CALLS])
  terrain = set_b()
  terrain.generate()
  tb = terrain.tiles[0]
  got = {'outside height': terrain.outside, 'req': tb.req, 'k': tb.k, 'n': tb.n, 'comp': tb.comp,
         'passes': tb.iterations, 'first step': tb.first_step, 'calls': tb.calls, 'events': 9 + 4 * tb.calls,
         'highest': tb.highest_after_shift, 'height factor': tb.hf}
  test('T10 set B tile values (table 7.2-E)', got, EXPECTED_B)

  v0, sums = smooth_trace(SET_C)
  c = SET_C
  result = smooth_value(c['cur'], f32(c['t']), f32(c['threshold']), c['neighbours'])
  first = (v0 + (((c['neighbours'][0] + c['neighbours'][1]) + c['neighbours'][2]) + c['neighbours'][3])) / 6.0
  test('T12 set C smooth sample (table 7.2-G)', {'v': v0, 'sums': sums, 'result': result, 'neighbours first': first},
       EXPECTED_C)

  terrain = set_d()
  rest_ok = True
  got = []
  for area in SET_D_AREAS:
    terrain.flatten(*area)
    got.append(block(terrain))
    rest_ok &= all(terrain.world.get(i, j) == -10.0 for j in range(10) for i in range(10)
                   if not (4 <= i <= 6 and 4 <= j <= 6))
  test('T13 set D flatten areas in order (table 7.2-H)', got, EXPECTED_D)
  test('T13 set D, every other sample stays -10', rest_ok, True)

  for name, ok in results:
    print(f'{"PASS" if ok else "FAIL"}  {name}')
  failed = sum(1 for _, ok in results if not ok)
  print(f'{len(results) - failed} of {len(results)} tests pass')
  return 1 if failed else 0


# ---------------------------------------------------------------- trace modes
def trace_a(full):
  terrain = set_a()
  print('cell', repr(terrain.cell), 'outside', repr(terrain.outside), 'cols', terrain.world.cols,
        'rows', terrain.world.rows)
  print('fd modifier', repr(30.7 * rexp(-6.5 * f32(1.2))), 'ls modifier', repr(15.353 * rexp(-3.1 * 1.0)))
  trace = []
  t2 = terrain.tiles[2]
  t2.generate(terrain.cell, terrain.outside, trace)
  print('tile 2 n', t2.n, 'comp', repr(t2.comp), 'calls', t2.calls)
  for e in trace[:10]:
    print('  ', e)
  terrain = set_a()
  terrain.generate()
  for i, t in enumerate(terrain.tiles):
    print('tile', i, 'n', t.n, 'k', t.k, 'passes', t.iterations, 'comp', repr(t.comp), 'highest',
          repr(t.highest_after_shift), 'range', repr(t.range), 'hf', repr(t.hf), 'posXi', t.pos_xi, 'posZi', t.pos_zi,
          'numCells', t.num_cells, 'tile cell', repr(t.tile_cell), 'calls', t.calls)
  w = terrain.world
  values = [v for row in w.v for v in row]
  print('world min', repr(min(values)), 'max', repr(max(values)), 'extent', repr(terrain.size_x))
  for x, z in [(0, 0), (43, 43), (86, 86), (129, 57)]:
    print('h[%d,%d] =' % (x, z), repr(w.get(x, z)))
  print('negative-fraction merge reads', terrain.neg_frac_calls)
  print('land fraction', sum(1 for v in values if v > 0) / len(values))
  if full:
    nm = terrain.normals()
    for x, z in [(43, 43), (86, 86), (129, 57), (0, 0)]:
      print('n[%d,%d] =' % (x, z), nm[z][x])
    print('h at (500.5, 700.25) =', repr(w.sample(500.5, 700.25)))
    print('h at (-5, 100) =', repr(w.sample(-5.0, 100.0)))


def trace_m1():
  terrain = set_a()
  t0 = terrain.tiles[0]
  trace = []
  t0.generate(terrain.cell, terrain.outside, trace)
  print('method 1, tile 0 events (pins first):')
  for e in trace[:13]:
    print('  ', e)
  print('  calls', t0.calls, 'events', 9 + 4 * t0.calls)


def trace_b():
  terrain = set_b()
  tb = terrain.tiles[0]
  trace = []
  tb.generate(terrain.cell, terrain.outside, trace)
  print('set B comp', repr(tb.comp), 'n', tb.n, 'calls', tb.calls, 'events', 9 + 4 * tb.calls)
  for e in trace[9:14]:
    print('  ', e)
  terrain = set_b()
  terrain.generate()
  print('set B highest', repr(terrain.tiles[0].highest_after_shift), 'height factor', repr(terrain.tiles[0].hf))


def trace_smooth():
  v0, sums = smooth_trace(SET_C)
  c = SET_C
  print('v0', repr(v0))
  for s in sums:
    print('  ', repr(s))
  print('result', repr(smooth_value(c['cur'], f32(c['t']), f32(c['threshold']), c['neighbours'])))
  n = c['neighbours']
  print('neighbours first', repr((v0 + (((n[0] + n[1]) + n[2]) + n[3])) / 6.0))


def trace_flatten():
  terrain = set_d()
  for k, area in enumerate(SET_D_AREAS, 1):
    terrain.flatten(*area)
    print(f'after ({k})', [[repr(v) for v in row] for row in block(terrain)])


def main(argv):
  mode = argv[1] if len(argv) > 1 else 'check'
  if mode == 'check':
    return check()
  if mode == 'log2':
    bad = [x for x in range(1, 70000) if ceil_log2(x) != (x - 1).bit_length()]
    print('ceilLog2 mismatches for 1 to 69 999:', len(bad), bad[:10])
    return 1 if bad else 0
  if mode in ('traceA', 'full'):
    trace_a(mode == 'full')
  elif mode == 'traceM1':
    trace_m1()
  elif mode == 'traceB':
    trace_b()
  elif mode == 'smooth':
    trace_smooth()
  elif mode == 'flatten':
    trace_flatten()
  else:
    print(__doc__)
    return 2
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
