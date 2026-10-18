"""Compare the consumers column of the asset manifest with the citations in the library.

Usage: python3 -I P6-GLO_consumers.py <design root> [--write]
Reports, per asset id: consumers that no longer cite it, and requirements that cite it but are not consumers.
"""
import csv, os, re, sys
from collections import defaultdict

root = sys.argv[1]
man_path = os.path.join(root, 'assets', 'asset_manifest.csv')
rows = list(csv.DictReader(open(man_path, encoding='utf-8', newline='')))
ids = sorted({r['asset_id'] for r in rows}, key=len, reverse=True)
idset = set(ids)
stems = sorted({a.split('.')[0] for a in ids}, key=len, reverse=True)
asset_rx = re.compile(r'`((?:' + '|'.join(map(re.escape, stems)) + r')\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)`')
def_rx = re.compile(r'\*\*((?:R-)?[A-Z]{2,6}-\d{3})\*\*')

req_cites = defaultdict(set)   # asset -> req ids citing it
doc_cites = defaultdict(set)   # asset -> doc paths citing it outside any requirement
req_exists = {}
for dirpath, _, files in os.walk(root):
  for fn in files:
    if not fn.endswith('.md'):
      continue
    p = os.path.join(dirpath, fn)
    rel = os.path.relpath(p, root).replace(os.sep, '/')
    if not (rel.startswith('reference/') or rel.startswith('architecture/components/')):
      continue
    lines = open(p, encoding='utf-8').read().split('\n')
    cur = None
    fence = False
    for line in lines:
      if line.startswith('```'):
        fence = not fence
      m = def_rx.search(line)
      if m and not fence:
        cur = m.group(1)
        req_exists[cur] = rel
      elif not line.strip() or line.startswith('#'):
        cur = None
      for a in asset_rx.findall(line):
        if a not in idset:
          continue
        if cur:
          req_cites[a].add(cur)
        else:
          doc_cites[a].add(rel)

by_id = defaultdict(list)
for r in rows:
  by_id[r['asset_id']].append(r)
stale = 0
missing = 0
for a in sorted(by_id):
  cons = set(c for c in by_id[a][0]['consumers'].split(';') if c)
  reqs = {c for c in cons if '/' not in c and not c.endswith('.md')}
  docs = cons - reqs
  gone = sorted(c for c in reqs if c not in req_cites[a])
  gone_docs = sorted(d for d in docs if d not in doc_cites[a] and not any(rr == d for rr in req_exists.values() for x in [0] if False))
  new = sorted(req_cites[a] - reqs)
  if gone or new or gone_docs:
    print(f'{a}: not citing {gone} {gone_docs}; citing but absent {new}')
    stale += len(gone)
    missing += len(new)
print('stale', stale, 'missing', missing)
