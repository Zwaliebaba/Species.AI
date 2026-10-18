"""R4b: tick and second pairs.

Usage: python3 -I R4b_ticks.py <design_next root> <out dir>
1. tuning rows in ticks whose notes give seconds: compare seconds x 10 with ticks.
2. prose pairs "N ticks ... X s" within one parenthesis or clause.
3. tuning rows whose unit is seconds (not real time), for a manual check of simulation timers.
"""
import json
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
keys = json.load(open(os.path.join(out, 'keys.json')))


def fnum(s):
  return float(s.replace(',', '').replace(' ', ''))


SEC = re.compile(r'(?<![\w.])(\d+(?:\.\d+)?)(?:\s*(?:to|–|-)\s*(\d+(?:\.\d+)?))?\s*s\b')
TICK = re.compile(r'(?<![\w.])(\d[\d ,]*?(?:\.\d+)?)(?:\s*(?:to|–|-)\s*(\d[\d ,]*?))?\s*ticks?\b')

rows1 = []
for k, v in sorted(keys.items()):
  if not v['unit'].startswith('tick'):
    continue
  val = v['value']
  m = re.match(r'^(\d+)(?:\s*(?:–|-|to)\s*(\d+))?$', val.replace(',', '').replace(' ', ''))
  if not m:
    continue
  lo = int(m.group(1))
  hi = int(m.group(2)) if m.group(2) else lo
  sm = SEC.search(v['notes'])
  if not sm:
    continue
  s_lo = float(sm.group(1))
  s_hi = float(sm.group(2)) if sm.group(2) else s_lo
  nominal = 'nominal' in v['notes'] or 'countdown' in v['notes']
  if abs(s_lo * 10 - lo) < 1e-6 and abs(s_hi * 10 - hi) < 1e-6:
    cls = 'exact'
  elif abs(s_lo * 10 - lo) <= 1.0001 and abs(s_hi * 10 - hi) <= 1.0001:
    cls = 'off-by-one-nominal' if nominal else 'off-by-one-unlabelled'
  else:
    cls = 'MISMATCH'
  rows1.append((cls, k, val, v['notes'], v['file']))

with open(os.path.join(out, 'tick_rows.txt'), 'w') as fh:
  for r in sorted(rows1):
    fh.write(' | '.join(r) + '\n')
from collections import Counter
print(Counter(r[0] for r in rows1))

# 2 prose pairs
FENCE = re.compile(r'^\s*```')
pairs = []
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d not in ('requirements_index',))
  for f in sorted(fs):
    if not f.endswith('.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    inside = False
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      if FENCE.match(line):
        inside = not inside
        continue
      if line.startswith('| `') and line.split('|')[1].strip().strip('`') in keys:
        continue
      for m in re.finditer(r'(\d[\d,]*)\s*ticks?\s*[,(;:]\s*(?:about |nominal(?:ly)? |that is |or )?(\d+(?:\.\d+)?)\s*s\b(?! of real)', line):
        t, s = int(m.group(1).replace(',', '')), float(m.group(2))
        if abs(s * 10 - t) > 1e-6:
          ctx = line[max(0, m.start() - 80):m.end() + 40]
          pairs.append((rel, i, t, s, 'nominal' in ctx, ctx))
      for m in re.finditer(r'(\d+(?:\.\d+)?)\s*s\b\s*(?:of [a-z ]+time\s*)?[,(;:]\s*(\d[\d,]*)\s*ticks?\b', line):
        s, t = float(m.group(1)), int(m.group(2).replace(',', ''))
        if abs(s * 10 - t) > 1e-6:
          ctx = line[max(0, m.start() - 80):m.end() + 40]
          pairs.append((rel, i, t, s, 'nominal' in ctx, ctx))
with open(os.path.join(out, 'tick_pairs.txt'), 'w') as fh:
  for r in pairs:
    fh.write(f'{r[0]}:{r[1]} ticks={r[2]} s={r[3]} nominal={r[4]}\n    {r[5]}\n')
print(len(pairs), 'prose pairs that differ')

# 3 second-valued keys
with open(os.path.join(out, 'second_keys.txt'), 'w') as fh:
  for k, v in sorted(keys.items()):
    u = v['unit']
    if re.match(r'^(s|seconds?|s of (game|match|simulation) time)\b', u) or u in ('s', 'seconds'):
      fh.write(f"{k} | {v['value']} | {u} | {v['notes'][:90]} | {v['req']} | {v['file']}\n")
