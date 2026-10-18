"""Count the string ids that the reference documents and engine components cite, by family (GameAssets §18.1).

Usage: python3 -I P6-GLO_strings.py <design root> [family ...]
"""
import csv, os, re, sys
from collections import defaultdict, Counter

root = sys.argv[1]
show = sys.argv[2:]
roots = ['achievement', 'bot', 'building', 'camera', 'crate', 'creature', 'faction', 'game', 'map', 'match', 'mode',
         'power', 'team', 'tutorial', 'ui', 'unit']
two_level = {'match', 'mode', 'ui', 'tutorial'}
texts = {}
for dp, _, fs in os.walk(root):
  for fn in fs:
    if fn.endswith('.md'):
      p = os.path.join(dp, fn)
      texts[os.path.relpath(p, root).replace(os.sep, '/')] = open(p, encoding='utf-8').read()
# tuning keys
keys = set()
for t in texts.values():
  cols = False
  lines = t.split('\n')
  for i, line in enumerate(lines):
    if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
      cells = [c.strip().lower() for c in line.strip('|').split('|')]
      cols = cells[:2] == ['key', 'value']
      continue
    if not line.startswith('|'):
      cols = False
      continue
    if cols:
      m = re.match(r'^\|\s*`([^`]+)`', line)
      if m:
        keys.add(m.group(1))
rows = list(csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8', newline='')))
assets = {r['asset_id'] for r in rows}
excl = {'camera.automatic', 'camera.timeline', 'camera.advance', 'mode.options', 'maps.definition'}
rx = re.compile(r'`((?:' + '|'.join(roots) + r')(?:\.[a-z0-9_<>]+)+)`')
cites = defaultdict(Counter)
for f, t in texts.items():
  if not (f.startswith('reference/') or f.startswith('architecture/components/')):
    continue
  for m in rx.finditer(t):
    s = m.group(1)
    if s in keys or s in assets or s in excl:
      continue
    if any(k.startswith(s + '.') for k in ()):
      continue
    cites[s][f] += 1
fam = defaultdict(set)
for s in cites:
  parts = s.split('.')
  if parts[0] in two_level and len(parts) >= 3:
    fam[parts[0] + '.' + parts[1] + '.*'].add(s)
  else:
    fam[parts[0] + '.*'].add(s)
total = 0
for k in sorted(fam):
  files = Counter()
  for s in fam[k]:
    files.update(cites[s])
  top = sorted(files.items(), key=lambda kv: (-kv[1], kv[0]))
  total += len(fam[k])
  print(f'{k}\t{len(fam[k])}\t{top[0][0]}\t+{len(top) - 1}')
  if k in show:
    for s in sorted(fam[k]):
      print('   ', s, dict(cites[s]))
print('total', total)
