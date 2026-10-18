"""R4b: extract every tuning table row, then find citations of a key with a value that differs.

Usage: python3 -I R4b_tuning.py <design_next root> <out dir>
"""
import json
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
FENCE = re.compile(r'^\s*```')


def cells_of(line):
  return [c.strip() for c in re.split(r'(?<!\\)\|', line.strip().strip('|'))]


def files():
  for top, dirs, fs in os.walk(root):
    dirs[:] = sorted(d for d in dirs if d not in ('.git', 'requirements_index'))
    for f in sorted(fs):
      if f.endswith('.md'):
        p = os.path.join(top, f)
        yield os.path.relpath(p, root), open(p, encoding='utf-8').read()


texts = dict(files())
keys = {}
table_lines = set()
for f, t in texts.items():
  lines = t.split('\n')
  cols = None
  for i, line in enumerate(lines):
    if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
      head = [c.lower() for c in cells_of(line)]
      cols = head if head[:2] == ['key', 'value'] else None
      continue
    if cols is None:
      continue
    if not line.startswith('|'):
      cols = None
      continue
    if re.match(r'^\|\s*:?-{3,}', line):
      continue
    c = cells_of(line)
    m = re.match(r'^`([^`]+)`$', c[0])
    if not m:
      continue
    d = dict(zip(cols, c))
    keys[m.group(1)] = dict(file=f, line=i + 1, value=d.get('value', ''), unit=d.get('unit', ''),
                            notes=d.get('range or notes', ''), req=d.get('requirement', ''), dec=d.get('dec', ''))
    table_lines.add((f, i + 1))

json.dump(keys, open(os.path.join(out, 'keys.json'), 'w'), indent=1)
print(len(keys), 'keys')

NUM = re.compile(r'(?<![\w.])[-−]?\d+(?:[.,]\d+)*(?:\s*/\s*\d+)?')


def nums(s):
  r = []
  for m in NUM.finditer(s):
    x = m.group(0).replace('−', '-').replace(',', '')
    if '/' in x:
      a, b = x.split('/')
      try:
        r.append(float(a) / float(b))
      except Exception:
        pass
      r.append(x)
      continue
    try:
      r.append(float(x))
    except ValueError:
      pass
  return [v for v in r if not isinstance(v, str)]


key_rx = re.compile(r'`((?:ref|ter|rnd|ast|vfx|aen|ui|inp|cam|dat|plt|bld|dbg)\.[a-z0-9_.]+)`')
hits = []
for f, t in texts.items():
  inside = False
  for i, line in enumerate(t.split('\n'), 1):
    if FENCE.match(line):
      inside = not inside
      continue
    if (f, i) in table_lines:
      continue
    for m in key_rx.finditer(line):
      k = m.group(1)
      if k not in keys:
        continue
      after = line[m.end():m.end() + 60]
      before = line[max(0, m.start() - 40):m.start()]
      cited = None
      # "`key` (40 ticks, 4 s)" or "`key` = 40" or "`key`, 40"
      ma = re.match(r'\s*(?:\(|=|is |of |, )\s*([^)`;|]*)', after)
      if ma and re.search(r'\d', ma.group(1)):
        cited = ('after', ma.group(1)[:50])
      else:
        mb = re.search(r'(\d[\d.,]*\s*[a-zA-Z%/ ]{0,24})\(\s*$', before)
        if mb:
          cited = ('before', mb.group(1))
      if not cited:
        continue
      kv = keys[k]['value']
      vn = nums(kv)
      cn = nums(cited[1])
      if not cn:
        continue
      ok = any(abs(a - b) < 1e-9 for a in cn for b in vn)
      hits.append(dict(file=f, line=i, key=k, cited=cited[1].strip(), side=cited[0], value=kv,
                       unit=keys[k]['unit'], ok=ok, text=line.strip()[:300]))

json.dump(hits, open(os.path.join(out, 'cites.json'), 'w'), indent=1)
bad = [h for h in hits if not h['ok']]
print(len(hits), 'value citations;', len(bad), 'without a matching number')
with open(os.path.join(out, 'cites_bad.txt'), 'w') as fh:
  for h in bad:
    fh.write(f"{h['file']}:{h['line']} {h['key']} cited [{h['cited']}] ({h['side']}) table [{h['value']} {h['unit']}]\n")
