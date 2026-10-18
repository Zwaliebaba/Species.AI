"""R4b: manifest consumers against the requirements that cite each asset id.

Usage: python3 -I R4b_consumers.py <design_next root> <out dir>
A requirement cites an asset when its paragraph (from **ID** to the next **ID** or heading) holds the id.
"""
import csv
import os
import re
import sys
from collections import defaultdict

root, out = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8', newline='')))
consumers = defaultdict(set)
for r in rows:
  for c in r['consumers'].split(';'):
    if c.strip():
      consumers[r['asset_id']].add(c.strip())
ids = set(consumers) | {r['asset_id'] for r in rows}
stems = sorted({a.split('.')[0] for a in ids}, key=len, reverse=True)
arx = re.compile(r'`((?:' + '|'.join(stems) + r')\.[a-z0-9_.]+)`')
rid = re.compile(r'^\*\*((?:R-)?[A-Z]{2,6}-\d{3})\*\*')
cited_by = defaultdict(set)
req_where = {}
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d != 'requirements_index')
  for f in sorted(fs):
    if not f.endswith('.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    if not (rel.startswith('reference/') or rel.startswith('architecture/')):
      continue
    cur = None
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      m = rid.match(line)
      if m:
        cur = m.group(1)
        req_where[cur] = f'{rel}:{i}'
      elif line.startswith('#'):
        cur = None
      if cur:
        for a in arx.findall(line):
          a = a.rstrip('.')
          if a in ids:
            cited_by[a].add(cur)
missing = []
stale = []
for a in sorted(ids):
  for r in sorted(cited_by[a] - consumers[a]):
    missing.append((a, r))
  for r in sorted(consumers[a] - cited_by[a]):
    if r in req_where:
      stale.append((a, r))
    else:
      stale.append((a, r + ' (no such requirement)'))
with open(os.path.join(out, 'consumers.txt'), 'w') as fh:
  fh.write('# requirement cites the asset; manifest consumers do not list it\n')
  for a, r in missing:
    fh.write(f'MISSING {a} <- {r} ({req_where.get(r)})\n')
  fh.write('# manifest consumer that does not cite the asset in its paragraph\n')
  for a, r in stale:
    fh.write(f'STALE {a} -> {r} ({req_where.get(r.split()[0])})\n')
print(len(missing), 'missing;', len(stale), 'stale')
