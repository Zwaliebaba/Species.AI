"""R3a helper: structural adoptability checks over the reference documents.

Usage: python3 -I R3a_checks.py <lib.json> <design_next root> <out dir>
Writes text reports into <out dir>. Read-only over the library.
"""
import collections
import json
import os
import re
import sys

LIB = json.load(open(sys.argv[1], encoding='utf-8'))
ROOT = sys.argv[2]
OUT = sys.argv[3]
os.makedirs(OUT, exist_ok=True)

reqs, keys, cites, decs = LIB['reqs'], LIB['keys'], LIB['key_cites'], LIB['decs']


def is_ref(f):
  return f.startswith('reference')


def w(name, lines):
  with open(os.path.join(OUT, name), 'w', encoding='utf-8') as fh:
    fh.write('\n'.join(lines) + '\n')


# 1 cited keys that are not defined
undef = collections.defaultdict(list)
for c in cites:
  k = c['key']
  if k in keys:
    continue
  if any(ch in k for ch in '<>{}*'):
    continue
  # string ids under ui. may be strings; record anyway with kind note
  undef[k].append(f"{c['file']}:{c['line']}")
lines = []
for k in sorted(undef):
  refs = [x for x in undef[k] if is_ref(x)]
  if refs:
    lines.append(f'{k}\t{len(undef[k])}\t' + ', '.join(undef[k][:6]))
w('undef_keys.txt', lines)

# 2 tuning rows in reference docs with empty or dash value/unit
lines = []
for k, v in keys.items():
  if not is_ref(v['file']):
    continue
  row = v['row']
  val = row.get('value', '')
  unit = row.get('unit', '')
  dec = row.get('dec', '')
  req = row.get('requirement', '')
  flags = []
  if not val or val in ('—', '-', '–'):
    flags.append('no-value')
  if not unit or unit in ('—', '-', '–'):
    flags.append('no-unit')
  if not req or req in ('—', '-', '–'):
    flags.append('no-req')
  if 'unit' not in row:
    flags.append('no-unit-col')
  if flags:
    lines.append(f"{v['file']}:{v['line']}\t{k}\t{','.join(flags)}\tval={val[:40]}\tunit={unit[:20]}")
w('key_rows.txt', lines)

# 3 numbers in reference requirements with no key citation
num_rx = re.compile(r'(?<![\w.\-`])(\d+(?:\.\d+)?)(?![\w`])')
lines = []
per_file = collections.Counter()
for rid, r in reqs.items():
  if not is_ref(r['file']) or not rid.startswith('R-'):
    continue
  text = re.sub(r'`[^`]*`', '', r['text'])
  text = re.sub(r'\*\*[^*]+\*\*', '', text)
  text = re.sub(r'\b(?:R-)?[A-Z]{2,6}-\d{3}\b', '', text)
  text = re.sub(r'\b(?:DEC|ADR)-\d{3}\b', '', text)
  text = re.sub(r'§\s*[\d.]+', '', text)
  text = re.sub(r'reference/\d+', '', text)
  text = re.sub(r'\b(table|Table|slice|slot|bit|step|Step|phase|Phase|row|column|level|Level)\s+\d+', '', text)
  nums = [n for n in num_rx.findall(text) if n not in ('0', '1', '2')]
  has_key = '`ref.' in r['text']
  if nums and not has_key:
    per_file[r['file']] += 1
    lines.append(f"{r['file']}:{r['line']}\t{rid}\t{' '.join(nums[:8])}\t{r['text'][:160]!r}")
w('nums_without_key.txt', [f'{f}\t{n}' for f, n in per_file.most_common()] + [''] + lines)

# 4 Open DECs cited by reference docs
cited_by_ref = collections.defaultdict(set)
for rid, r in reqs.items():
  if not is_ref(r['file']):
    continue
  for d in re.findall(r'\bDEC-\d{3}\b', r['text']):
    cited_by_ref[d].add(r['file'])
lines = []
weak = re.compile(r'not (established|recorded|known|specified|defined|stated)|unspecified|undefined|unknown|design phase (decides|chooses|sets|defines)|to be decided|no default|not decided', re.I)
for d in sorted(cited_by_ref):
  e = decs.get(d)
  if not e:
    lines.append(f'{d}\tMISSING')
    continue
  if e['status'] != 'Open':
    continue
  flag = 'WEAK' if weak.search(e['specified']) else ''
  if len(e['specified']) < 60:
    flag += ' SHORT'
  lines.append(f"{d}\t{flag}\t{e['decide_by']}\t{'; '.join(sorted(cited_by_ref[d]))}\t{e['title']}\t{e['specified'][:300]}")
w('open_decs.txt', lines)

# 5 state diagrams: states without outgoing transitions
lines = []
for m in LIB['mermaid']:
  if not is_ref(m['file']):
    continue
  t = m['text']
  if 'stateDiagram' not in t:
    continue
  out = collections.defaultdict(set)
  inc = collections.defaultdict(set)
  states = set()
  composite = set()
  for ln in t.split('\n'):
    s = ln.strip()
    mm = re.match(r'^state\s+(?:"[^"]*"\s+as\s+)?(\w+)\s*\{', s)
    if mm:
      composite.add(mm.group(1))
    mm = re.match(r'^(\[\*\]|[\w.]+)\s*-->\s*(\[\*\]|[\w.]+)', s)
    if mm:
      a, b = mm.group(1), mm.group(2)
      out[a].add(b)
      inc[b].add(a)
      states.update([a, b])
      continue
    mm = re.match(r'^state\s+"[^"]*"\s+as\s+(\w+)', s)
    if mm:
      states.add(mm.group(1))
    mm = re.match(r'^(\w+)\s*:', s)
    if mm:
      states.add(mm.group(1))
  states.discard('[*]')
  sinks = [s for s in states if not out.get(s) and s not in composite]
  sources = [s for s in states if not inc.get(s) and s not in composite]
  if sinks or sources:
    lines.append(f"{m['file']}:{m['line']}\t[{m['section']}]\tno-exit={sorted(sinks)}\tno-entry={sorted(sources)}")
w('state_sinks.txt', lines)

# 6 per-file stats
lines = []
per = collections.defaultdict(lambda: {'reqs': 0, 'keys': 0, 'cites': 0})
for rid, r in reqs.items():
  if is_ref(r['file']) and rid.startswith('R-'):
    per[r['file']]['reqs'] += 1
for k, v in keys.items():
  if is_ref(v['file']):
    per[v['file']]['keys'] += 1
for f in sorted(per):
  hs = [h for _, h in LIB['headings'][f] if h.startswith('## ')]
  lines.append(f"{f}\treqs={per[f]['reqs']}\tkeys={per[f]['keys']}\t" + ' | '.join(hs))
for f in sorted(LIB['headings']):
  if is_ref(f) and f not in per:
    hs = [h for _, h in LIB['headings'][f] if h.startswith('## ')]
    lines.append(f'{f}\treqs=0\tkeys=0\t' + ' | '.join(hs))
w('per_file.txt', lines)
print('done')
