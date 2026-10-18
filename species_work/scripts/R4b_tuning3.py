"""R4b: value citations in non-tuning table cells and "`key` (default N)" forms.

Usage: python3 -I R4b_tuning3.py <design_next root> <out dir>
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
root, out = sys.argv[1], sys.argv[2]
keys = json.load(open(os.path.join(out, 'keys.json')))
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'R4b_tuning2.py')).read()
ns = {}
exec(src.split('bad = []')[0].replace("out = sys.argv[1]", "out = %r" % out), ns)
nums, match = ns['nums'], ns['match']

key_rx = re.compile(r'`((?:ref|ter|rnd|ast|vfx|aen|ui|inp|cam|dat|plt|bld|dbg)\.[a-z0-9_.]+)`')
res = []
for top, dirs, fs in os.walk(root):
  dirs[:] = sorted(d for d in dirs if d != 'requirements_index')
  for f in sorted(fs):
    if not f.endswith('.md'):
      continue
    p = os.path.join(top, f)
    rel = os.path.relpath(p, root)
    for i, line in enumerate(open(p, encoding='utf-8').read().split('\n'), 1):
      if not line.startswith('|'):
        continue
      cells = [c.strip() for c in re.split(r'(?<!\\)\|', line.strip().strip('|'))]
      m0 = re.match(r'^`([^`]+)`$', cells[0]) if cells else None
      if m0 and m0.group(1) in keys and keys[m0.group(1)]['file'] == rel and keys[m0.group(1)]['line'] == i:
        continue
      for ci, c in enumerate(cells):
        ks = key_rx.findall(c)
        if len(ks) != 1 or ks[0] not in keys:
          continue
        k = ks[0]
        # a cell that is only the key: the value may be in a neighbouring cell
        if re.fullmatch(r'`[^`]+`', c):
          for nb in (ci - 1, ci + 1):
            if 0 <= nb < len(cells):
              v = cells[nb]
              if re.fullmatch(r'[-−]?[\d][\d ,.]*\s*[a-zA-Z%°/²]*(\s*\([^)]*\))?', v) and not key_rx.search(v):
                if not match(nums(v), nums(keys[k]['value'])):
                  res.append((rel, i, k, v, keys[k]['value'], keys[k]['unit']))
        else:
          md = re.search(r'`' + re.escape(k) + r'`\s*\(default\s+([^)]*)\)', c)
          if md and not match(nums(md.group(1)), nums(keys[k]['value'])):
            res.append((rel, i, k, md.group(1), keys[k]['value'], keys[k]['unit']))
with open(os.path.join(out, 'cites_table.txt'), 'w') as fh:
  for r in res:
    fh.write(f'{r[0]}:{r[1]} {r[2]} cell [{r[3]}] table [{r[4]} | {r[5]}]\n')
print(len(res))
