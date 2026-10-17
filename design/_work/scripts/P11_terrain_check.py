"""P11 helper: run P11_terrain_ref on every map and count, per building type, how many sit at/below sea level.
Usage: python3 -I P11_terrain_check.py <SRC_ROOT>"""
import sys, os, collections, importlib.util
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('ref', os.path.join(here, 'P11_terrain_ref.py'))
ref = importlib.util.module_from_spec(spec); spec.loader.exec_module(ref)
src = sys.argv[1]
lv = os.path.join(src, 'data/levels')
tot = collections.Counter(); below = collections.Counter()
for fn in sorted(os.listdir(lv)):
    if not (fn.startswith('mp_') or fn.startswith('map_')): continue
    land, tiles, flats, b = ref.parse(os.path.join(lv, fn))
    W = ref.build(land, tiles, flats)
    for t, x, z in b:
        tot[t] += 1
        if W.value(x, z) <= 0.0: below[t] += 1
for t in sorted(tot, key=lambda k: -tot[k]):
    print(f'{t},{tot[t]},{below[t]}')
