"""P6-RND: compute replacement terrain vector sets C and D from a seeded synthetic field (read-only helper).

Usage: python3 -I P6-RND_vectors.py <path to Tools/DesignModels/TerrainGenerator.py>
Set C: the smooth operation applied to set A (a seeded synthetic terrain) with synthetic parameters.
Set D: the five flatten areas on a flat field, with the Add value 18.7.
Writes nothing; prints the values.
"""
import importlib.util
import sys

spec = importlib.util.spec_from_file_location('tg', sys.argv[1])
tg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tg)

T_SMOOTH = 45.0
THR_SMOOTH = 18.3
CENTRE = (1350.0, 1350.0)
HALF = 300.0

terrain = tg.set_a()
terrain.generate()
w = terrain.world
cx, t, cz, size, threshold = tg.f32(CENTRE[0]), tg.f32(T_SMOOTH), tg.f32(CENTRE[1]), tg.f32(HALF), tg.f32(THR_SMOOTH)


def idx(r):
  return int((r - 0.0) * w.ix)


x1 = idx(cx - size) + 1
x2 = idx(cx + size) + 1
z1 = idx(cz - size) + 1
z2 = idx(cz + size) + 1
above = 0
differ = 0
first = None
for p in range(2):
  for z in range(z1, z2):
    for x in range(x1, x2):
      cur = w.get(x, z)
      nb = (w.get(x - 1, z), w.get(x + 1, z), w.get(x, z + 1), w.get(x, z - 1))
      v = tg.smooth_value(cur, t, threshold, nb)
      if cur > threshold:
        above += 1
        v0 = cur * (t / cur)
        alt = (v0 + (((nb[0] + nb[1]) + nb[2]) + nb[3])) / 6.0
        alt = cur if alt > cur else alt
        if alt != v:
          differ += 1
          inside = x - 1 >= x1 and z - 1 >= z1 and x + 1 < x2 and z + 1 < z2
          if first is None and p == 0 and inside and v < cur:
            sums = []
            s = v0
            for n in nb:
              s += n
              sums.append(s)
            first = dict(pass_=p + 1, x=x, z=z, cur=cur, W=nb[0], E=nb[1], N=nb[2], S=nb[3], v0=v0, sums=sums,
                         result=v, first=(v0 + (((nb[0] + nb[1]) + nb[2]) + nb[3])) / 6.0)
      w.put(x, z, v)
print('smooth area: centre', CENTRE, 'half-width', HALF, 't', repr(t), 'T', repr(threshold))
print('columns', x1, 'to', x2 - 1, 'rows', z1, 'to', z2 - 1)
print('above-threshold evaluations', above, 'differing', differ)
for k, v in (first or {}).items():
  print(' ', k, repr(v))

print('decimal rounding', [(d, repr(tg.f32(d))) for d in (15.76, -15.39, 1.2, -26.13, 18.7, 18.3)])

areas = [
  (60, 5, 60, 20, tg.FLATTEN_ABSOLUTE),
  (60, 18.7, 60, 8, tg.FLATTEN_ADD),
  (60, 4, 60, 20, tg.FLATTEN_SUBTRACT, 10),
  (60, 3, 60, 20, tg.FLATTEN_SUBTRACT2, 6),
  (60, 1, 60, 20, tg.FLATTEN_SMOOTH, 0.5),
]
d = tg.set_d()
for k, a in enumerate(areas, 1):
  d.flatten(*a)
  print(f'set D after ({k})', [[repr(v) for v in row] for row in tg.block(d)])
