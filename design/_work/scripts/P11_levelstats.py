"""P11 helper: survey data/levels/* for landscape / options usage.
Usage: python3 -I P11_levelstats.py <SRC_ROOT>
Prints per-file landscape summary and aggregate stats (sections, option keys, tile params, flatten types, guide grids)."""
import sys, os, re, collections
src = sys.argv[1]
lv = os.path.join(src, 'data/levels')
sections = collections.Counter()
optkeys = collections.Counter()
levelopts = collections.Counter()
effects = collections.Counter()
invalid = collections.Counter()
gametypes = collections.Counter()
flattypes = collections.Counter()
gpow = collections.Counter()
gen = collections.Counter()
nonzero_guides = []
rows = []
tiles_all = []
for fn in sorted(os.listdir(lv)):
    p = os.path.join(lv, fn)
    if not os.path.isfile(p): continue
    lines = open(p, 'r', errors='replace').read().splitlines()
    cur = None
    land = {}
    ntiles = 0; nflat = 0; nlights = 0
    for raw in lines:
        s = raw.split('#')[0].strip()
        if not s: continue
        toks = s.split()
        w = toks[0]
        if w.lower().endswith('_startdefinition'):
            cur = w[:-len('_StartDefinition')].lower(); sections[(cur)] += 1; continue
        if w.lower().endswith('_enddefinition'):
            cur = None; continue
        if cur == 'multiwiniaoptions':
            optkeys[w] += 1
            if w == 'LevelOptions':
                for t in toks[1:]:
                    if not re.match(r'^-?\d+$', t) : levelopts[t.lower()] += 1
            if w == 'Effects':
                for t in toks[1:]: effects[t] += 1
            if w == 'InvalidCrates':
                for t in toks[1:]: invalid[t] += 1
            if w == 'GameTypes':
                for t in toks[1:]: gametypes[t] += 1
        elif cur == 'landscape':
            if len(toks) > 1: land[w] = toks[1]
        elif cur == 'landscapetiles':
            ntiles += 1
            t = toks
            try:
                d = dict(x=int(t[0]), y=float(t[1]), z=int(t[2]), size=int(t[3]), fd=float(t[4]), hs=float(t[5]), dh=float(t[6]), gm=int(t[7]), seed=int(t[8]), ls=float(t[9]), gp=int(t[10]))
            except Exception as e:
                print('# parse error', fn, s); continue
            gpow[d['gp']] += 1; gen[d['gm']] += 1
            if d['gp'] > 0:
                g = t[11] if len(t) > 11 else ''
                d['guidelen'] = len(g)
                if set(g) != {'A'}: nonzero_guides.append((fn, d['gp'], len(g)))
            d['file'] = fn
            tiles_all.append(d)
        elif cur == 'landflattenareas':
            nflat += 1
            ft = int(toks[4]) if len(toks) > 4 else 0
            flattypes[ft] += 1
        elif cur == 'lights':
            nlights += 1
    rows.append((fn, land.get('worldSizeX'), land.get('worldSizeZ'), land.get('cellSize'), land.get('outsideHeight'), land.get('maxHeight',''), land.get('landColourFile',''), land.get('wavesColourFile',''), land.get('waterColourFile',''), ntiles, nflat, nlights))
print('file,worldSizeX,worldSizeZ,cellSize,outsideHeight,maxHeight,landColourFile,wavesColourFile,waterColourFile,tiles,flattenAreas,lights')
for r in rows: print(','.join(str(x) if x is not None else '' for x in r))
print('\n# sections', dict(sections))
print('# MultiwiniaOptions keys', dict(optkeys))
print('# LevelOptions', dict(levelopts))
print('# Effects', dict(effects))
print('# InvalidCrates', dict(invalid))
print('# GameTypes', dict(gametypes))
print('# flatten types', dict(flattypes))
print('# guide powers', dict(gpow))
print('# generation methods', dict(gen))
print('# non-zero guide grids', nonzero_guides)
import statistics
for k in ['size','fd','hs','dh','ls','y']:
    v = [d[k] for d in tiles_all]
    print(f'# tile {k}: min={min(v)} max={max(v)} distinct={len(set(v))}')
bad = [(d['file'], d['size']) for d in tiles_all]
