#!/usr/bin/env python3
"""S06 helper: build Source-map rows for (a) every data asset path quoted in
06_player_control_and_input.md (icons/, textures/, mwhelp/, sprites/, shapes) and (b) the
camera mounts defined by each data/levels/mp_*.txt; every row is checked against manifest.csv.
Usage: python3 -I S_06_sourcemap_assets.py <SRC root> <doc.md> <manifest.csv> <out.md>"""
import csv, glob, os, re, sys
SRC, DOC, MAN, OUT = sys.argv[1:5]
man = {r['path'] for r in csv.DictReader(open(MAN, encoding='utf-8'))}
doc = open(DOC, encoding='utf-8').read()
secs = []
for m in re.finditer(r'^## (\d+) ', doc, re.M):
    secs.append((m.start(), m.group(1)))


def section_of(pos):
    s = '?'
    for start, num in secs:
        if start <= pos:
            s = num
    return s

assets = {}
for m in re.finditer(r'`((?:icons|textures|mwhelp|sprites)/[A-Za-z0-9_\-]+\.bmp|[a-z_]+\.shp)`', doc):
    p = m.group(1)
    path = 'data/' + p if not p.endswith('.shp') else 'data/shapes/' + p
    assets.setdefault(path, set()).add(section_of(m.start()))
rows, missing = [], []
for path in sorted(assets):
    if path in man:
        rows.append('| `%s` | art referenced by the input/cursor/task-bar UI (§%s) |' % (path, ', §'.join(sorted(assets[path], key=lambda x: int(x) if x.isdigit() else 99))))
    else:
        missing.append(path)
lv = []
for f in sorted(glob.glob(os.path.join(SRC, 'data/levels/mp_*.txt'))):
    rel = 'data/levels/' + os.path.basename(f)
    txt = open(f, encoding='latin-1').read()
    m = re.search(r'CameraMounts_StartDefinition(.*?)CameraMounts_EndDefinition', txt, re.S)
    names = []
    if m:
        for line in m.group(1).splitlines():
            t = line.split('#')[0].split()
            if t:
                names.append(t[0])
    if rel in man:
        lv.append('| `%s` | camera mounts used at match start (§9.8): %s |' % (rel, ', '.join('`%s`' % n for n in names) if names else 'none'))
    else:
        missing.append(rel)
open(OUT, 'w', encoding='utf-8').write('\n'.join(rows + lv) + '\n')
print('asset rows', len(rows), 'level rows', len(lv), 'missing', missing)
