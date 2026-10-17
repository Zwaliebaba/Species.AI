"""C-AST self-check for architecture/components/asset_pipeline_and_resources.md.

Usage: python3 -I C-AST_check.py <REPO>
Checks: ids in range and unique; references to AST ids defined; tuning keys unique in
design_next, each cited by a requirement paragraph, every cited ast./rnd./ref. key
defined somewhere; DEC and ADR ids exist; Open decs_to_cite cited by a requirement;
asset ids in the manifest; other documents' requirement ids exist; headings; truncation
words; links resolve to a file on disk or a planned package target; size.
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
REL = 'architecture/components/asset_pipeline_and_resources.md'
PATH = os.path.join(NEXT, REL)
FOLDER = os.path.dirname(PATH)
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-AST.json'), encoding='utf-8'))
txt = open(PATH, encoding='utf-8').read()
problems = []

size = os.path.getsize(PATH)
if size > 120 * 1024:
    problems.append(f'size {size} > 120 KB')

for h in ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
          '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
          '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests',
          '## 8 Open decisions', '## 9 Tuning table']:
    if not re.search('^' + re.escape(h) + r'\s*$', txt, re.M):
        problems.append(f'missing heading {h}')

ids = [int(i) for i in re.findall(r'\*\*AST-(\d{3})\*\*', txt)]
lo, hi = (int(x) for x in pkg['id_ranges']['AST'].split('-'))
if len(ids) != len(set(ids)):
    problems.append('duplicate AST ids')
if ids != sorted(ids):
    problems.append('AST ids not ascending in document order')
for n in ids:
    if not lo <= n <= hi:
        problems.append(f'AST-{n:03d} outside {lo}-{hi}')
for r in set(int(x) for x in re.findall(r'AST-(\d{3})', txt)):
    if r not in ids:
        problems.append(f'AST-{r:03d} cited but not defined')

paras = [p for p in re.split(r'\n(?=\*\*AST-\d{3}\*\*)', txt) if p.startswith('**AST-')]
paras = [re.split(r'\n#{2,4} ', p)[0] for p in paras]
req_text = '\n'.join(paras)

dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
for d in sorted(set(re.findall(r'DEC-\d{3}', txt))):
    if d not in dec_status:
        problems.append(f'{d} does not exist')
adr_files = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
for a in sorted(set(re.findall(r'ADR-\d{4}', txt))):
    if a not in adr_files:
        problems.append(f'{a} does not exist')
for d in pkg['decs_to_cite']:
    dec, status = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if status == 'Open' and dec not in req_text:
        problems.append(f'Open {dec} not cited by a requirement')
    if dec not in txt:
        problems.append(f'{d} not cited at all')

assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])
roots = r'(?:sprite|fx|ui|icon|cursor|marker|glyph|terrain|mesh|help|anim|sfx|amb|music|font|strings|map|thumb)'
for a in sorted(set(re.findall(r'`(' + roots + r'\.[a-z0-9_.]+)`', txt))):
    if a.startswith('ui.error.'):
        continue
    if a.endswith('.*') or a.endswith('.'):
        continue
    if a not in assets:
        problems.append(f'asset {a} not in the manifest')

defs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`((?:ref|rnd|ter|vfx|ast|ui|cam|dat|plt|bld|dbg|aen|inp)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            defs.setdefault(m.group(1), []).append(os.path.relpath(f, NEXT))
mine = []
for line in txt.split('\n'):
    m = re.match(r'\|\s*`(ast\.[a-z0-9_.]+)`\s*\|', line)
    if m:
        mine.append(m.group(1))
for k in mine:
    if len(defs[k]) > 1:
        problems.append(f'key {k} defined in {defs[k]}')
    if ('`' + k + '`') not in req_text:
        problems.append(f'key {k} not cited by a requirement')
for k in sorted(set(re.findall(r'`((?:ast|rnd|ref|ter)\.[a-z0-9_.]+)`', txt))):
    if k.endswith('.') or k not in defs:
        problems.append(f'key {k} cited but not defined anywhere')

reqdefs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    if f == PATH:
        continue
    for m in re.finditer(r'\*\*((?:R-[A-Z]+|RND|TER)-\d{3})\*\*', open(f, encoding='utf-8').read()):
        reqdefs[m.group(1)] = f
for r in sorted(set(re.findall(r'\b((?:R-[A-Z]+|RND|TER)-\d{3})\b', txt))):
    if r not in reqdefs:
        problems.append(f'{r} cited but defined nowhere')

body = re.sub(r'```.*?```', '', txt, flags=re.S)
for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
    for m in re.finditer(pat, body, re.I):
        problems.append(f'truncation "{m.group(0)}"')

planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(tgt.rstrip('/'))
        if tgt.endswith('/'):
            planned.add(tgt.rstrip('/') + '/README.md')
for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
    if target.startswith('http'):
        continue
    tp = os.path.normpath(os.path.join(FOLDER, target))
    rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
    if not os.path.exists(tp) and rel not in planned:
        problems.append(f'link target neither on disk nor planned: {target}')

mermaid = len(re.findall(r'^```mermaid', txt, re.M))
tables = len(re.findall(r'^Table \d', txt, re.M))
print(f'bytes {size}; requirements {len(ids)}; ast keys {len(mine)}; mermaid {mermaid}; '
      f'captioned tables {tables}; DECs {len(set(re.findall(r"DEC-[0-9]{3}", txt)))}; '
      f'ADRs {len(set(re.findall(r"ADR-[0-9]{4}", txt)))}')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
