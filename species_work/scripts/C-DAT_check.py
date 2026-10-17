"""C-DAT self-check for architecture/components/data_schemas/.

Usage: python3 -I C-DAT_check.py <REPO>
Checks: sizes; template headings; DAT ids in range, unique, ascending per file, every cited
DAT id defined; DEC and ADR ids exist; Open decs_to_cite cited by a requirement; asset ids in
the manifest; dat. keys unique and cited by a requirement; every cited key of another owner
defined somewhere; requirement ids of other documents exist; truncation words; links resolve
to a file on disk or a planned package target.
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
FOLDER = os.path.join(NEXT, 'architecture', 'components', 'data_schemas')
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-DAT.json'), encoding='utf-8'))
files = sorted(glob.glob(os.path.join(FOLDER, '*.md')))
texts = {f: open(f, encoding='utf-8').read() for f in files}
problems = []
HEADS = ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
         '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
         '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests',
         '## 8 Open decisions', '## 9 Tuning table']
lo, hi = (int(x) for x in pkg['id_ranges']['DAT'].split('-'))
all_ids = {}
req_text = ''
for f, txt in texts.items():
    name = os.path.basename(f)
    size = os.path.getsize(f)
    if size > 120 * 1024:
        problems.append(f'{name}: size {size} > 120 KB')
    for h in HEADS:
        if not re.search('^' + re.escape(h) + r'\s*$', txt, re.M):
            problems.append(f'{name}: missing heading {h}')
    ids = [int(i) for i in re.findall(r'\*\*DAT-(\d{3})\*\*', txt)]
    if ids != sorted(ids):
        problems.append(f'{name}: DAT ids not ascending')
    for n in ids:
        if n in all_ids:
            problems.append(f'DAT-{n:03d} defined in {name} and {all_ids[n]}')
        all_ids[n] = name
        if not lo <= n <= hi:
            problems.append(f'DAT-{n:03d} outside {lo}-{hi}')
    paras = [p for p in re.split(r'\n(?=\*\*DAT-\d{3}\*\*)', txt) if p.startswith('**DAT-')]
    paras = [re.split(r'\n#{2,4} ', p)[0] for p in paras]
    req_text += '\n'.join(paras) + '\n'
alltxt = '\n'.join(texts.values())
for r in sorted(set(int(x) for x in re.findall(r'DAT-(\d{3})', alltxt))):
    if r not in all_ids:
        problems.append(f'DAT-{r:03d} cited but not defined')

dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
for d in sorted(set(re.findall(r'DEC-\d{3}', alltxt))):
    if d not in dec_status:
        problems.append(f'{d} does not exist')
adr_files = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
for a in sorted(set(re.findall(r'ADR-\d{4}', alltxt))):
    if a not in adr_files:
        problems.append(f'{a} does not exist')
for d in pkg['decs_to_cite']:
    dec, status = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if status == 'Open' and dec not in req_text:
        problems.append(f'Open {dec} not cited by a requirement')
    if dec not in alltxt:
        problems.append(f'{d} not cited at all')
for dec in ['DEC-120', 'DEC-800', 'DEC-801', 'DEC-567']:
    if dec not in req_text:
        problems.append(f'{dec} not cited by a requirement')
# status claims in tables
for f, txt in texts.items():
    for m in re.finditer(r'^\| (DEC-\d{3}) \| (Open|Resolved|Dropped) \|', txt, re.M):
        if dec_status.get(m.group(1)) != m.group(2):
            problems.append(f'{os.path.basename(f)}: {m.group(1)} listed as {m.group(2)}, register says {dec_status.get(m.group(1))}')

assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])
roots = r'(?:sprite|fx|icon|cursor|marker|glyph|terrain|mesh|help|anim|sfx|amb|music|font|strings|map|thumb)'
for a in sorted(set(re.findall(r'`(' + roots + r'\.[a-z0-9_.]+)`', alltxt))):
    if a.endswith('.') or a.startswith('map.difficulty') or a in ('map.definition', 'map.index') or a.startswith('mesh.asset') or a in ('mesh.collision', 'mesh.sim_record', 'strings.table'):
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
for txt in texts.values():
    for line in txt.split('\n'):
        m = re.match(r'\|\s*`(dat\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            mine.append(m.group(1))
for k in mine:
    if len(defs[k]) > 1:
        problems.append(f'key {k} defined in {defs[k]}')
    if ('`' + k + '`') not in req_text:
        problems.append(f'key {k} not cited by a requirement')
for k in sorted(set(re.findall(r'`((?:ast|rnd|ref|ter|aen|ui|inp|cam|bld|vfx|dat|plt|dbg)\.[a-z0-9_.]+)`', alltxt))):
    if k.endswith('.'):
        continue
    if k not in defs:
        problems.append(f'key {k} cited but not defined anywhere')

reqdefs = set()
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    if os.path.dirname(f) == FOLDER:
        continue
    for m in re.finditer(r'\*\*((?:R-[A-Z]+|RND|TER|AEN|AST|INP|UI|VFX|CAM|BLD|PLT|DBG)-\d{3})\*\*', open(f, encoding='utf-8').read()):
        reqdefs.add(m.group(1))
for r in sorted(set(re.findall(r'\b((?:R-[A-Z]+|RND|TER|AEN|AST|INP|UI|VFX|CAM|BLD|PLT|DBG)-\d{3})\b', alltxt))):
    if r not in reqdefs:
        problems.append(f'{r} cited but defined nowhere')

for f, txt in texts.items():
    body = re.sub(r'```.*?```', '', txt, flags=re.S)
    for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(pat, body, re.I):
            problems.append(f'{os.path.basename(f)}: truncation "{m.group(0)}"')

planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(tgt.rstrip('/'))
        if tgt.endswith('/'):
            planned.add(tgt.rstrip('/') + '/README.md')
for f, txt in texts.items():
    for target in re.findall(r'\]\(([^)#\s]+)(?:#([^)]*))?\)', txt):
        t = target[0]
        if t.startswith('http'):
            continue
        tp = os.path.normpath(os.path.join(FOLDER, t))
        rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
        if not os.path.exists(tp) and rel not in planned:
            problems.append(f'{os.path.basename(f)}: link target neither on disk nor planned: {t}')

tot_ids = len(all_ids)
mermaid = len(re.findall(r'^```mermaid', alltxt, re.M))
tables = len(re.findall(r'^Table \d', alltxt, re.M))
print(f'files {len(files)}; requirements {tot_ids}; dat keys {len(mine)}; mermaid {mermaid}; captioned tables {tables}; '
      f'DECs {len(set(re.findall(r"DEC-[0-9]{3}", alltxt)))}; ADRs {len(set(re.findall(r"ADR-[0-9]{4}", alltxt)))}')
for f in files:
    print(' ', os.path.basename(f), os.path.getsize(f), len(re.findall(r'\*\*DAT-(\d{3})\*\*', texts[f])))
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
