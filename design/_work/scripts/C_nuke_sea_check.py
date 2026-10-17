"""Coordinator check for 04-U1: for every mp_ level, estimate the fraction of the Crate::RunNuke sampling rectangle
[0, worldX-100) x [0, worldZ-100) whose landscape height is <= 0 (the loop's exit condition), using the P11 terrain port.
A fraction of 0 means the re-roll loop in code/worldobject/crate.cpp:Crate::RunNuke would never terminate.
Usage: python3 -I C_nuke_sea_check.py <SRC_ROOT> [step]"""
import sys, os, importlib.util
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('ref', os.path.join(here, 'P11_terrain_ref.py'))
ref = importlib.util.module_from_spec(spec); spec.loader.exec_module(ref)
src = sys.argv[1]; step = float(sys.argv[2]) if len(sys.argv) > 2 else 20.0
lv = os.path.join(src, 'data/levels')
print('level,worldX,worldZ,samples,sea_samples,sea_fraction')
for fn in sorted(os.listdir(lv)):
    if not fn.startswith('mp_'): continue
    land, tiles, flats, b = ref.parse(os.path.join(lv, fn))
    W = ref.build(land, tiles, flats)
    wx = int(land['wx'] - 100.0); wz = int(land['wz'] - 100.0)
    n = sea = 0
    x = 0.0
    while x < wx:
        z = 0.0
        while z < wz:
            n += 1
            if W.value(x, z) <= 0.0: sea += 1
            z += step
        x += step
    print(f'{fn},{land["wx"]},{land["wz"]},{n},{sea},{sea / n:.4f}')
