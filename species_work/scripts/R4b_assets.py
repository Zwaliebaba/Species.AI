"""R4b: asset ids cited anywhere in design_next against the manifest.

Usage: python3 -I R4b_assets.py <design_next root> <out dir>
Writes assets_missing.txt (cited ids not in the manifest), assets_cites.json (id -> citing lines),
assets_uncited.txt (manifest ids cited by no document outside assets/).
"""
import csv
import json
import os
import re
import sys
from collections import defaultdict

root, out = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8', newline='')))
ids = defaultdict(list)
for r in rows:
  ids[r['asset_id']].append(r)
stems = sorted({a.split('.')[0] for a in ids}, key=len, reverse=True)
rx = re.compile(r'(?<![\w.`/-])`?((?:' + '|'.join(stems) + r')\.[a-z0-9_]+(?:\.[a-z0-9_*<>{}]+)*)`?(?![\w-])')
keys = json.load(open(os.path.join(out, 'keys.json')))
cites = defaultdict(list)
missing = []
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d != 'requirements_index')
  for f in sorted(fs):
    if not f.endswith('.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      for m in rx.finditer(line):
        a = m.group(1).rstrip('.')
        if a in keys:
          continue
        if re.search(r'[*<>{}]', a):
          cites['PATTERN:' + a].append((rel, i))
          continue
        # file names such as fx.glow_cloudy.png are not ids; strip a trailing file extension
        a2 = re.sub(r'\.(png|dds|wav|ogg|gltf|glb|json|ttf|otf|csv|txt|hlsl|h)$', '', a)
        cites[a2].append((rel, i))
        if a2 not in ids:
          missing.append((rel, i, a2, line.strip()[:200]))
json.dump({k: v for k, v in cites.items()}, open(os.path.join(out, 'assets_cites.json'), 'w'))
with open(os.path.join(out, 'assets_missing.txt'), 'w') as fh:
  for m in missing:
    fh.write(f'{m[0]}:{m[1]} {m[2]}\n    {m[3]}\n')
outside = {a for a, v in cites.items() if any(not r.startswith('assets/') for r, _ in v)}
with open(os.path.join(out, 'assets_uncited.txt'), 'w') as fh:
  for a in sorted(ids):
    if a not in outside:
      r = ids[a][0]
      fh.write(f"{a} | {r['category']} | {r['role'][:80]} | consumers {r['consumers'][:80]}\n")
print(len(ids), 'ids;', len(cites), 'cited tokens;', len(missing), 'missing cites;',
      sum(1 for a in ids if a not in outside), 'ids cited only inside assets/ or nowhere')
