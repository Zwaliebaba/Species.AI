#!/usr/bin/env python3
"""A1P_RegionPrototype.py: a first prototype of two neighbouring world regions (A1, resolution of A1-F16).

It builds two valleys side by side with the conformance terrain generator Tools/DesignModels/TerrainGenerator.py
(loaded by path) and the records that WLD-162 lists: one island base tile per region with a 3 x 3 guide grid, two
hill tiles in each region's outward corners, a level plateau per site (the proposed feathered flatten type 6 of
WLD-268 at the mean base height on the core's rim, read in a first generation pass and clamped to 20 to 120 wu), a
level area at each valley centre, and a land bridge across the sea channel between the two regions built as a chain
of feathered squares whose heights rise linearly from one mouth to the other. It then runs checks 1 to 6 of WLD-163
on the generated height field and reports, over a range of seeds, how often each check passes, the largest step at a
core's rim and how long a region takes. With "absolute", the feather bands are 0, which is the reference Absolute
type.

Usage: python3 -I species_design_work/scripts/A1P_RegionPrototype.py Tools/DesignModels/TerrainGenerator.py
       [seeds [height_scale fractal_dimension lowland_smoothing [absolute]]]

The cell is 32 wu (the world uses 16 wu) so that a pure-Python run of 100 seeds takes under two minutes; the checks
are structural (water, levels, slopes, reachability), which the cell size does not decide. A development tool.
Results on 2026-10-18: 100 of 100 seeds pass every check with the feathered type (floors 52 to 111 wu, rim steps 3
to 5 wu); with "absolute", 20 of 20 pass checks 1 to 6 with rim steps of 31 to 52 wu, above WLD-041's 16 wu.
"""
import importlib.util
import math
import sys
import time


def load(path):
  spec = importlib.util.spec_from_file_location('TerrainGenerator', path)
  mod = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(mod)
  return mod


SIDE = 5120.0
MARGIN = 640.0
CELL = 32.0
CHANNEL_HALF = 128.0
SPACING = 2080.0
CORE = 400.0
MOUTH = 1200.0
BRIDGE_HALF = 192.0
FLOOR_MIN, FLOOR_MAX = 20.0, 120.0
FEATHER_CORE = 320.0
FEATHER_BRIDGE = 96.0


def feather(w, cx, t, cz, size, band):
  i1, i2 = int((cx - size - band) / CELL), int((cx + size + band) / CELL) + 1
  j1, j2 = int((cz - size - band) / CELL), int((cz + size + band) / CELL) + 1
  for j in range(j1, j2):
    for i in range(i1, i2):
      d = max(abs(i * CELL - cx), abs(j * CELL - cz))
      if d <= size:
        w.put(i, j, t)
      elif d < size + band:
        cur = w.get(i, j)
        g = 1.0 - (d - size) / band
        w.put(i, j, cur + (t - cur) * g)


def mix(seed, *keys):
  h = (seed * 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
  for k in keys:
    h ^= (k + 0x632BE59BD9B4E019) & 0xFFFFFFFFFFFFFFFF
    h = (h * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    h ^= h >> 31
  return h


def u01(seed, *keys):
  return (mix(seed, *keys) >> 11) / float(1 << 53)


BASE_HS, BASE_FD, BASE_LS = 0.25, 2.0, 2.0


def build(tg, seed, ramp=True):
  regions = [(MARGIN, MARGIN), (MARGIN + SIDE, MARGIN)]
  tiles = []
  sites = []
  for r, (x0, z0) in enumerate(regions):
    guide = [[70, 80, 70], [80, 90, 80], [70, 80, 70]]
    tiles.append(tg.Tile(int(x0 + CHANNEL_HALF + 128), 0.0, int(z0 + CHANNEL_HALF + 128),
                         int(SIDE - 2 * (CHANNEL_HALF + 128)), BASE_FD, BASE_HS, 150.0, 1,
                         int(mix(seed, r, 1) & 0x7FFFFFFF), BASE_LS, 2, guide))
    cx, cz = x0 + SIDE / 2, z0 + SIDE / 2
    for a, b in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
      sites.append((cx + a * SPACING / 2, cz + b * SPACING / 2, r))
    # two hill tiles at the outward corners of this region (away from the shared side)
    out_x = x0 + 300 if r == 0 else x0 + SIDE - 300 - 640
    for k, hz in enumerate((z0 + 300, z0 + SIDE - 300 - 640)):
      tiles.append(tg.Tile(int(out_x), 0.0, int(hz), 640, 1.2, 1.0, 260.0 + 80 * u01(seed, r, 2, k), 0,
                           int(mix(seed, r, 3, k) & 0x7FFFFFFF), 1.0, 0, None))
  world_x = 2 * SIDE + 2 * MARGIN
  world_z = SIDE + 2 * MARGIN
  # first pass: generate the tiles only, read the base height at every site
  t1 = tg.Terrain(world_x, world_z, CELL, -10.0, 0.0, tiles, [])
  t1.generate()
  w = t1.world
  floors = []
  for (x, z, r) in sites:
    ring = [w.sample(x + CORE * math.cos(2 * math.pi * k / 16), z + CORE * math.sin(2 * math.pi * k / 16))
            for k in range(16)]
    floors.append(min(FLOOR_MAX, max(FLOOR_MIN, sum(ring) / len(ring))))
  centres = []
  for r, (x0, z0) in enumerate(regions):
    cx, cz = x0 + SIDE / 2, z0 + SIDE / 2
    centres.append((cx, cz, min(FLOOR_MAX, max(FLOOR_MIN, w.sample(cx, cz)))))
  a, b = sites[1], sites[4]
  ma = (a[0] + MOUTH, a[1])
  mb = (b[0] - MOUTH, b[1])
  ha = min(FLOOR_MAX, max(FLOOR_MIN, w.sample(*ma)))
  hb = min(FLOOR_MAX, max(FLOOR_MIN, w.sample(*mb)))
  # the proposed flatten type 6, a feathered level square: level inside its half-size, blended linearly to the
  # existing height across a band (the record's threshold field) outside it; applied in record order
  for (x, z, _), f in zip(sites, floors):
    feather(w, x, f, z, CORE, FEATHER_CORE)
  for cx, cz, h in centres:
    feather(w, cx, h, cz, 120.0, 200.0)
  if ramp:
    n = int((mb[0] - ma[0]) / BRIDGE_HALF) + 1
    for k in range(n + 1):
      f = k / n
      feather(w, ma[0] + (mb[0] - ma[0]) * f, ha + (hb - ha) * f, a[1], BRIDGE_HALF, FEATHER_BRIDGE)
  t2 = t1
  return t2, sites, floors, (ma, mb)


def normal_y(w, i, j):
  c = w.get(i, j)
  dx = (w.get(i + 1, j) - w.get(i - 1, j)) / (2 * CELL)
  dz = (w.get(i, j + 1) - w.get(i, j - 1)) / (2 * CELL)
  return 1.0 / math.sqrt(1.0 + dx * dx + dz * dz), c


def checks(t, sites, floors, mouths):
  w = t.world
  res = {}
  # a sea channel along the shared border outside the crossing
  bx = MARGIN + SIDE
  ok = True
  for j in range(int((MARGIN + 300) / CELL), int((MARGIN + SIDE - 300) / CELL)):
    z = j * CELL
    if abs(z - sites[1][1]) <= BRIDGE_HALF + FEATHER_BRIDGE + CELL:
      continue
    for i in range(int((bx - CHANNEL_HALF) / CELL), int((bx + CHANNEL_HALF) / CELL) + 1):
      if w.get(i, j) > 0.0:
        ok = False
  res['channel'] = ok
  # the crossing: dry and gentle along its centre line
  (ma, mb) = mouths
  ok = True
  for i in range(int(ma[0] / CELL), int(mb[0] / CELL) + 1):
    ny, h = normal_y(w, i, int(sites[1][1] / CELL))
    if h <= 1.0 or ny < 0.9:
      ok = False
  res['crossing'] = ok
  # site plateaus: level, in the height band
  ok = True
  for (x, z, _), f in zip(sites, floors):
    for j in range(int((z - CORE * 0.9) / CELL), int((z + CORE * 0.9) / CELL)):
      for i in range(int((x - CORE * 0.9) / CELL), int((x + CORE * 0.9) / CELL)):
        if math.hypot(i * CELL - x, j * CELL - z) > CORE * 0.9:
          continue
        ny, h = normal_y(w, i, j)
        if ny < 0.95 or not (FLOOR_MIN - 0.5 <= h <= FLOOR_MAX + 0.5):
          ok = False
  res['plateaus'] = ok
  # no water within 600 wu of a site
  ok = True
  for (x, z, _) in sites:
    for j in range(int((z - 600) / CELL), int((z + 600) / CELL) + 1):
      for i in range(int((x - 600) / CELL), int((x + 600) / CELL) + 1):
        if math.hypot(i * CELL - x, j * CELL - z) <= 600 and w.get(i, j) <= 0.0:
          ok = False
  res['dry_sites'] = ok
  # reachability over walkable land (height above 1 wu, normal y at least 0.8) from site 0 to every site
  start = (int(sites[0][0] / CELL), int(sites[0][1] / CELL))
  seen = {start}
  stack = [start]
  while stack:
    i, j = stack.pop()
    for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
      q = (i + di, j + dj)
      if q in seen or not (0 <= q[0] < w.cols and 0 <= q[1] < w.rows):
        continue
      ny, h = normal_y(w, *q)
      if h > 1.0 and ny >= 0.8:
        seen.add(q)
        stack.append(q)
  res['reachable'] = all((int(x / CELL), int(z / CELL)) in seen for (x, z, _) in sites)
  # step at the plateau edges: the largest height difference between neighbouring cells on a core's rim
  worst = 0.0
  for (x, z, _) in sites:
    for k in range(64):
      ang = 2 * math.pi * k / 64
      i = int((x + (CORE + CELL) * math.cos(ang)) / CELL)
      j = int((z + (CORE + CELL) * math.sin(ang)) / CELL)
      i0 = int((x + (CORE - CELL) * math.cos(ang)) / CELL)
      j0 = int((z + (CORE - CELL) * math.sin(ang)) / CELL)
      worst = max(worst, abs(w.get(i, j) - w.get(i0, j0)))
  res['rim_step_wu'] = worst
  return res


def main(argv):
  global BASE_HS, BASE_FD, BASE_LS
  tg = load(argv[1])
  global FEATHER_CORE, FEATHER_BRIDGE
  if len(argv) > 5:
    BASE_HS, BASE_FD, BASE_LS = float(argv[3]), float(argv[4]), float(argv[5])
  if len(argv) > 6 and argv[6] == 'absolute':
    FEATHER_CORE = FEATHER_BRIDGE = 0.0    # the reference Absolute type only
  seeds = range(1, (int(argv[2]) if len(argv) > 2 else 20) + 1)
  names = ('channel', 'crossing', 'plateaus', 'dry_sites', 'reachable')
  counts = {n: 0 for n in names}
  all_ok = 0
  steps = []
  t0 = time.time()
  for s in seeds:
    t, sites, floors, mouths = build(tg, s)
    r = checks(t, sites, floors, mouths)
    for n in names:
      counts[n] += 1 if r[n] else 0
    all_ok += 1 if all(r[n] for n in names) else 0
    steps.append(r['rim_step_wu'])
    print(f"seed {s}: " + ', '.join(f"{n} {'pass' if r[n] else 'FAIL'}" for n in names)
          + f"; floors {', '.join(f'{f:.0f}' for f in floors)}; rim step {r['rim_step_wu']:.0f} wu")
  dt = time.time() - t0
  n = len(seeds)
  print(f'\n{n} seeds, two regions each, cell {CELL:.0f} wu, {dt / n / 2:.1f} s of Python per region')
  for k in names:
    print(f'- {k}: {counts[k]} of {n}')
  print(f'- every check: {all_ok} of {n}; largest step at a core rim {max(steps):.0f} wu, median'
        f' {sorted(steps)[len(steps) // 2]:.0f} wu')
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
