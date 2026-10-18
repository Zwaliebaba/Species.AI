"""R4b: non-numeric values cited in parentheses after a key, compared with the table value.

Usage: python3 -I R4b_tuning4.py <design_next root> <out dir>
"""
import json
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
keys = json.load(open(os.path.join(out, 'keys.json')))
rx = re.compile(r'`((?:ref|ter|rnd|ast|vfx|aen|ui|inp|cam|dat|plt|bld|dbg)\.[a-z0-9_.]+)`\s*\(([^)0-9]{1,30})\)')
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d != 'requirements_index')
  for f in sorted(fs):
    if not f.endswith('.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      for m in rx.finditer(line):
        k, c = m.group(1), m.group(2).strip().lower()
        if k not in keys:
          continue
        v = keys[k]['value'].lower().strip('`')
        if c in ('', 'presentation') or re.match(r'^(r-|dec-|adr-|see |§|table|reference|this|defined|owned|the owner|ast-|rnd-|vfx-|ui-)', c):
          continue
        if c not in v and v not in c:
          print(f'{rel}:{i} {k} cited ({c}) table [{keys[k]["value"]}]')
