"""Report manifest consumers that name a requirement id the library does not define, or a removed stub, or a missing path."""
import csv, os, re, sys
root = sys.argv[1]
rows = list(csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8', newline='')))
defs = {}
for dp, _, fs in os.walk(root):
  for fn in fs:
    if fn.endswith('.md'):
      p = os.path.join(dp, fn)
      for line in open(p, encoding='utf-8'):
        for m in re.finditer(r'\*\*((?:R-)?[A-Z]{2,6}-\d{3})\*\*(.{0,40})', line):
          defs[m.group(1)] = m.group(2)
seen = set()
for r in rows:
  key = (r['asset_id'], r['consumers'])
  if key in seen:
    continue
  seen.add(key)
  for c in r['consumers'].split(';'):
    if not c:
      continue
    if '/' in c or c.endswith('.md'):
      if not os.path.exists(os.path.join(root, c.split('#')[0])):
        print('missing path', r['asset_id'], c)
    elif c not in defs:
      print('undefined', r['asset_id'], c)
    elif 'removed' in defs[c]:
      print('stub', r['asset_id'], c, defs[c])
