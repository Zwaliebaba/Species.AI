"""Phase 4 (item 104): rename the asset stems that collide with string-id prefixes.

Usage: python3 -I p4_rename_stems.py <REPO> [--apply]

`ui.<name>` asset ids become `uitex.<name>` and `map.<mode>` asset ids become `mapfile.<mode>`.
Only exact manifest asset ids are renamed (string ids such as `ui.lobby.ready` are never touched):
in assets/asset_manifest.csv (asset_id column) and wherever the id appears in backticks or as a
bare token in a .md file under design_next/, or in species_work/asset_origin_map.csv.
Prints the rename map and the number of replacements per file.
"""
import csv
import glob
import io
import os
import re
import sys
from collections import Counter

REPO = sys.argv[1]
APPLY = '--apply' in sys.argv
NEXT = os.path.join(REPO, 'design_next')
MAN = os.path.join(NEXT, 'assets', 'asset_manifest.csv')
RENAME_STEM = {'ui': 'uitex', 'map': 'mapfile'}

rows = list(csv.DictReader(open(MAN, encoding='utf-8', newline='')))
cols = list(rows[0].keys())
ids = sorted({r['asset_id'] for r in rows})
mapping = {a: RENAME_STEM[a.split('.')[0]] + a[len(a.split('.')[0]):] for a in ids if a.split('.')[0] in RENAME_STEM}
for a, b in mapping.items():
    if b in ids:
        sys.exit(f'collision: {b} already exists')
print(f'{len(mapping)} asset ids renamed:')
for a, b in mapping.items():
    print(f'  {a} -> {b}')

# token: the id not preceded by a word char, dot or hyphen, and not followed by a word char or ".<word>"
pat = re.compile(r'(?<![\w.-])(' + '|'.join(re.escape(a) for a in sorted(mapping, key=len, reverse=True)) + r')(?![\w-]|\.\w)')
counts = Counter()
for p in sorted(glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True)):
    t = open(p, encoding='utf-8').read()
    new, n = pat.subn(lambda m: mapping[m.group(1)], t)
    if n:
        counts[os.path.relpath(p, REPO)] = n
        if APPLY:
            open(p, 'w', encoding='utf-8').write(new)
for r in rows:
    if r['asset_id'] in mapping:
        r['asset_id'] = mapping[r['asset_id']]
        counts['design_next/assets/asset_manifest.csv'] += 1
if APPLY:
    buf = io.StringIO()
    w = csv.DictWriter(buf, fieldnames=cols, lineterminator='\n')
    w.writeheader()
    w.writerows(rows)
    open(MAN, 'w', encoding='utf-8', newline='').write(buf.getvalue())
origin = os.path.join(REPO, 'species_work', 'asset_origin_map.csv')
if os.path.exists(origin):
    t = open(origin, encoding='utf-8').read()
    new, n = pat.subn(lambda m: mapping[m.group(1)], t)
    counts['species_work/asset_origin_map.csv'] = n
    if APPLY and n:
        open(origin, 'w', encoding='utf-8').write(new)
for f, n in sorted(counts.items()):
    print(f'{n:5} {f}')
print('applied' if APPLY else 'dry run')
