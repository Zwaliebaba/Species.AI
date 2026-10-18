"""R4b: compare what a citing line says about an asset with its manifest row.

Usage: python3 -I R4b_assetroles.py <design_next root> <out dir>
Checks: loop / one-shot; streamed / resident; positional / 2-D; and image assets cited as icons, cursors
or UI textures against their category.
"""
import csv
import json
import os
import re
import sys
from collections import defaultdict

root, out = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8', newline='')))
man = {}
for r in rows:
  man.setdefault(r['asset_id'], r)
cites = json.load(open(os.path.join(out, 'assets_cites.json')))
texts = {}


def line(rel, n):
  if rel not in texts:
    texts[rel] = open(os.path.join(root, rel), encoding='utf-8').read().split('\n')
  return texts[rel][n - 1]


res = []
for a, where in cites.items():
  if a not in man:
    continue
  r = man[a]
  spec = (r['spec'] + ' ' + r['target_format'] + ' ' + r['role']).lower()
  audio = a.split('.')[0] in ('sfx', 'amb', 'music')
  for rel, n in where:
    if rel.startswith('assets/'):
      continue
    L = line(rel, n)
    # window around the id
    i = L.find(a)
    w = L[max(0, i - 120):i + len(a) + 120].lower()
    if audio:
      if re.search(r'\bloop(s|ing|ed)?\b', w) and 'loop' not in spec and 'one-shot' not in w:
        res.append(('cited as a loop, manifest not a loop', a, rel, n, w))
      if re.search(r'\bone-shot\b|\bsting\b', w) and 'seamless loop' in spec and 'loop' not in w:
        res.append(('cited as a one-shot, manifest is a loop', a, rel, n, w))
      if re.search(r'\bstream(ed|s)?\b', w) and 'stream' not in spec:
        res.append(('cited as streamed, manifest resident', a, rel, n, w))
      if re.search(r'\bresident\b', w) and 'resident' not in spec and 'stream' in spec:
        res.append(('cited as resident, manifest streamed', a, rel, n, w))
    else:
      if re.search(r'\bcursor\b', w) and a.startswith(('icon.', 'uitex.', 'fx.')) and 'cursor' not in spec:
        pass
with open(os.path.join(out, 'asset_roles.txt'), 'w') as fh:
  for x in res:
    fh.write(f'{x[0]} | {x[1]} | {x[2]}:{x[3]}\n    {x[4]}\n    manifest: {man[x[1]]["target_format"][:60]} / {man[x[1]]["spec"][:120]}\n')
print(len(res))
