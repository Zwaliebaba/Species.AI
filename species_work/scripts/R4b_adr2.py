"""R4b: table rows that list ADR ids in one cell and statuses in another.

Usage: python3 -I R4b_adr2.py <design_next root>
"""
import os
import re
import sys

root = sys.argv[1]
status = {}
for f in os.listdir(os.path.join(root, 'ADR')):
  m = re.match(r'ADR-(\d{3})-.*\.md$', f)
  if m:
    t = open(os.path.join(root, 'ADR', f), encoding='utf-8').read()
    status[m.group(1)] = re.search(r'^status:\s*(\S+)', t, re.M).group(1).lower()
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d != 'requirements_index')
  for f in sorted(fs):
    if not f.endswith('.md') or (top.endswith('ADR') and f == 'README.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      if not line.startswith('|'):
        continue
      cells = [c.strip() for c in line.strip().strip('|').split('|')]
      for ci, c in enumerate(cells[:-1]):
        ids = re.findall(r'ADR-(\d{3})', c)
        if not ids or re.sub(r'[\[\]()\w./-]*ADR-\d{3}[\w./()-]*|,|and|\s', '', c):
          continue
        sc = cells[ci + 1]
        sts = re.findall(r'\b(Accepted|Proposed|Rejected)\b', sc)
        if not sts or re.sub(r'Accepted|Proposed|Rejected|,|and|\s', '', sc):
          if sts:
            print(f'{rel}:{i} MIXED status cell: {c} | {sc}')
          continue
        if len(sts) == 1:
          bad = [a for a in ids if status[a] != sts[0].lower()]
          if bad:
            print(f'{rel}:{i} {c} | {sc} -> wrong for ' + ', '.join(f'ADR-{a}={status[a]}' for a in bad))
        elif len(sts) == len(ids):
          bad = [(a, s) for a, s in zip(ids, sts) if status[a] != s.lower()]
          if bad:
            print(f'{rel}:{i} {c} | {sc} -> wrong for ' + ', '.join(f'ADR-{a}={status[a]} (cited {s})' for a, s in bad))
        else:
          print(f'{rel}:{i} COUNT MISMATCH {len(ids)} ids, {len(sts)} statuses: {c} | {sc}')
