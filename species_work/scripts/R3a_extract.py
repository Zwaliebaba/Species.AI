"""R3a helper: extract requirements, tuning keys, DECs and state diagrams from the library.

Usage: python3 -I R3a_extract.py <design_next root> <output json>
Read-only over the library; writes only the output json.
"""
import json
import os
import re
import sys

ROOT = sys.argv[1]
OUT = sys.argv[2]

REQ_DEF = re.compile(r'^(?:[-*]\s+|\d+\.\s+|\|\s*)?\*\*((?:R-)?[A-Z]{2,6}-\d{3})\*\*')
REQ_CITE = re.compile(r'\b((?:R-)?[A-Z]{2,6}-\d{3})\b')
KEY_CITE = re.compile(r'`((?:ref|ter|rnd|ast|vfx|aen|ui|inp|cam|dat|plt|bld|dbg)\.[a-z0-9_.<>{}*-]+)`')
DEC_CITE = re.compile(r'\bDEC-(\d{3})\b')


def cells_of(line):
  s = line.strip()
  if s.startswith('|'):
    s = s[1:]
  if s.endswith('|'):
    s = s[:-1]
  return [c.strip() for c in re.split(r'(?<!\\)\|', s)]


def files(root):
  for d, _, fs in os.walk(root):
    if 'requirements_index' in d:
      continue
    for f in sorted(fs):
      if f.endswith('.md'):
        yield os.path.join(d, f)


reqs = {}
keys = {}
key_cites = []
decs = {}
headings = {}
mermaid = []

for path in files(ROOT):
  rel = os.path.relpath(path, ROOT)
  with open(path, encoding='utf-8') as fh:
    lines = fh.read().split('\n')
  headings[rel] = [(i + 1, l) for i, l in enumerate(lines) if l.startswith('#') and not l.startswith('#!')]
  # code fences
  in_code = False
  code_lang = None
  code_start = 0
  code_buf = []
  cols = None
  cur_req = None
  section = ''
  for i, line in enumerate(lines):
    if line.startswith('```'):
      if not in_code:
        in_code = True
        code_lang = line[3:].strip()
        code_start = i + 1
        code_buf = []
      else:
        in_code = False
        if code_lang == 'mermaid':
          mermaid.append({'file': rel, 'line': code_start, 'text': '\n'.join(code_buf), 'section': section})
      continue
    if in_code:
      code_buf.append(line)
      continue
    if line.startswith('#'):
      section = line.strip('# ').strip()
      cur_req = None
    # tuning tables
    if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
      head = [c.lower() for c in cells_of(line)]
      cols = head if head[:2] == ['key', 'value'] else None
      continue
    if cols is not None:
      if not line.startswith('|'):
        cols = None
      elif re.match(r'^\|\s*:?-{3,}', line):
        pass
      else:
        cells = cells_of(line)
        m = re.match(r'^`([^`]+)`$', cells[0])
        if m:
          row = {cols[j] if j < len(cols) else f'c{j}': cells[j] for j in range(len(cells))}
          keys[m.group(1)] = {'file': rel, 'line': i + 1, 'row': row, 'section': section}
        continue
    # requirement definitions
    m = REQ_DEF.match(line)
    if m:
      rid = m.group(1)
      reqs[rid] = {'file': rel, 'line': i + 1, 'text': line, 'section': section}
      cur_req = rid
    elif cur_req and line.strip() and not line.startswith('|') and not line.startswith('#'):
      reqs[cur_req]['text'] += '\n' + line
    elif not line.strip():
      cur_req = None
    for k in KEY_CITE.findall(line):
      key_cites.append({'file': rel, 'line': i + 1, 'key': k})
  # DEC entries
  if rel.startswith('decisions'):
    cur = None
    for i, line in enumerate(lines):
      m = re.match(r'^### DEC-(\d{3})\s+(.*)', line)
      if m:
        cur = 'DEC-' + m.group(1)
        decs[cur] = {'file': rel, 'line': i + 1, 'title': m.group(2), 'body': ''}
        continue
      if cur:
        if line.startswith('#'):
          cur = None
          continue
        decs[cur]['body'] += line + '\n'

for d in decs.values():
  m = re.search(r'- Status:\s*(\w+)', d['body'])
  d['status'] = m.group(1) if m else '?'
  m = re.search(r'- Specified behaviour:\s*(.*)', d['body'])
  d['specified'] = m.group(1) if m else ''
  m = re.search(r'- Decide by:\s*(.*)', d['body'])
  d['decide_by'] = m.group(1) if m else ''

with open(OUT, 'w', encoding='utf-8') as fh:
  json.dump({'reqs': reqs, 'keys': keys, 'key_cites': key_cites, 'decs': decs, 'headings': headings,
             'mermaid': mermaid}, fh, indent=1)
print(len(reqs), 'reqs', len(keys), 'keys', len(key_cites), 'key cites', len(decs), 'decs', len(mermaid), 'mermaid')
