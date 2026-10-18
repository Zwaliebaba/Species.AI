"""R4b: ADR statuses in front matter, in the ADR README index and as cited in documents.

Usage: python3 -I R4b_adr.py <design_next root> <out dir>
"""
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
adr_dir = os.path.join(root, 'ADR')
status = {}
titles = {}
for f in sorted(os.listdir(adr_dir)):
  m = re.match(r'ADR-(\d{3})-.*\.md$', f)
  if not m:
    continue
  t = open(os.path.join(adr_dir, f), encoding='utf-8').read()
  s = re.search(r'^status:\s*(\S+)', t, re.M)
  status[m.group(1)] = s.group(1).lower() if s else '?'
  h = re.search(r'^# (.+)$', t, re.M)
  titles[m.group(1)] = h.group(1) if h else ''
res = []
# README index rows
readme = open(os.path.join(adr_dir, 'README.md'), encoding='utf-8').read().split('\n')
for i, line in enumerate(readme, 1):
  if line.startswith('|'):
    ids = re.findall(r'ADR-(\d{3})', line)
    st = re.findall(r'\b(Accepted|Proposed|Rejected|Superseded|Deprecated)\b', line)
    if len(set(ids)) == 1 and st:
      if st[0].lower() != status.get(ids[0]):
        res.append(f'README:{i} ADR-{ids[0]} index says {st[0]}, file says {status.get(ids[0])}')
# citations anywhere
STW = r'(Accepted|Proposed|Rejected|accepted|proposed|rejected)'
pat1 = re.compile(r'ADR-(\d{3})[^.;|]{0,40}?\(\s*' + STW + r'\b')
pat2 = re.compile(r'\b' + STW + r'\s+(?:ADRs?\s+)?\(?(?:\[)?ADR-(\d{3})')
pat3 = re.compile(r'ADR-(\d{3})\)?\s+(?:is|was|remains|stays)\s+(?:still\s+)?' + STW)
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d != 'requirements_index')
  for f in sorted(fs):
    if not f.endswith('.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      for m in pat1.finditer(line):
        a, s = m.group(1), m.group(2).lower()
        if status.get(a) != s:
          res.append(f'{rel}:{i} ADR-{a} cited as {s}, file {status.get(a)}: ...{line[max(0, m.start()-60):m.end()+40]}')
      for m in pat2.finditer(line):
        s, a = m.group(1).lower(), m.group(2)
        if status.get(a) != s:
          res.append(f'{rel}:{i} ADR-{a} cited as {s}, file {status.get(a)}: ...{line[max(0, m.start()-60):m.end()+40]}')
      for m in pat3.finditer(line):
        a, s = m.group(1), m.group(2).lower()
        if status.get(a) != s:
          res.append(f'{rel}:{i} ADR-{a} cited as {s}, file {status.get(a)}: ...{line[max(0, m.start()-60):m.end()+40]}')
      for a in re.findall(r'ADR-(\d{3})', line):
        if a not in status:
          res.append(f'{rel}:{i} ADR-{a} has no file')
with open(os.path.join(out, 'adr.txt'), 'w') as fh:
  for k in sorted(status):
    fh.write(f'ADR-{k} {status[k]} {titles[k]}\n')
  fh.write('\n')
  for r in res:
    fh.write(r + '\n')
print(len(res))
