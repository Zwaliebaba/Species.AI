#!/usr/bin/env python3
"""S_41: for 04-U2. Location::CreateRandomCrate (weighted mode) tests GetValue(pos.x, pos.z > 20.0), i.e. the
height at z = 0 or z = 1 along the map edge, and accepts the position iff that height is non-zero.
For every mp_ level, re-generate the heightmap with the P11 terrain port and report min/max of the edge heights
for x in [-100, X+100] (building position +- 100) at z = 0 and z = 1, and how many samples are exactly 0.
Usage: python3 -I S_41_crate_edge_check.py <SRC_ROOT> [step]"""
import sys, os, importlib.util
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('ref', os.path.join(here, 'P11_terrain_ref.py'))
ref = importlib.util.module_from_spec(spec); spec.loader.exec_module(ref)
src = sys.argv[1]; step = float(sys.argv[2]) if len(sys.argv) > 2 else 5.0
lv = os.path.join(src, 'data/levels')
print('level,worldX,samples,zero_samples,min_h,max_h')
for fn in sorted(os.listdir(lv)):
    if not fn.startswith('mp_'): continue
    land, tiles, flats, b = ref.parse(os.path.join(lv, fn))
    W = ref.build(land, tiles, flats)
    X = float(land['wx'])
    vals = []
    x = -100.0
    while x <= X + 100.0:
        for z in (0.0, 1.0):
            vals.append(W.value(x, z))
        x += step
    print(f'{fn},{land["wx"]},{len(vals)},{sum(1 for v in vals if v == 0.0)},{min(vals):.3f},{max(vals):.3f}')
