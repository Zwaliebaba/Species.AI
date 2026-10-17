#!/usr/bin/env python3
"""S_41: companion to the coordinator's C_nuke_sea_check.py, for 02-U12 / 02-U5 / 04-U3.
For every mp_ level, re-generate the heightmap with the P11 terrain port and estimate the success
probability of the two other unbounded re-roll loops:
  * SpaceShip::FindNewTarget  - pos = (SFRAND(X), SFRAND(Z)), accept when height >= 30
    (SFRAND(r) is in [-r/2, r/2]; negative coordinates convert to an out-of-range uint16 index -> index 0)
  * Crate::RunSpaceShip       - pos = (FRAND(X), FRAND(Z)), accept when height >= 10
Also reports the height at (0,0) (start value of both loops / the random-mode crate template).
Usage: python3 -I S_41_spaceship_land_check.py <SRC_ROOT> [step]"""
import sys, os, importlib.util
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('ref', os.path.join(here, 'P11_terrain_ref.py'))
ref = importlib.util.module_from_spec(spec); spec.loader.exec_module(ref)
src = sys.argv[1]; step = float(sys.argv[2]) if len(sys.argv) > 2 else 25.0
lv = os.path.join(src, 'data/levels')
print('level,worldX,worldZ,ship_samples,ship_ok,ship_p,crate_samples,crate_ok,crate_p,h00')
for fn in sorted(os.listdir(lv)):
    if not fn.startswith('mp_'): continue
    land, tiles, flats, b = ref.parse(os.path.join(lv, fn))
    W = ref.build(land, tiles, flats)
    X = float(land['wx']); Z = float(land['wz'])
    n1 = ok1 = 0
    x = -X / 2
    while x <= X / 2:
        z = -Z / 2
        while z <= Z / 2:
            n1 += 1
            if W.value(x, z) >= 30.0: ok1 += 1
            z += step
        x += step
    n2 = ok2 = 0
    x = 0.0
    while x <= X:
        z = 0.0
        while z <= Z:
            n2 += 1
            if W.value(x, z) >= 10.0: ok2 += 1
            z += step
        x += step
    print(f'{fn},{land["wx"]},{land["wz"]},{n1},{ok1},{ok1 / n1:.4f},{n2},{ok2},{ok2 / n2:.4f},{W.value(0.0, 0.0):.2f}')
