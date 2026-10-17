"""C-RND-a self-check: requirement ids, tuning keys, DEC/ADR/asset references,
reference keys, template headings, truncation words, links, sizes, Mermaid count.

Usage: python3 -I C-RND-a_check.py <REPO>
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
FOLDER = os.path.join(NEXT, 'architecture', 'components', 'renderer')
MINE = ['README.md', 'frame_graph_and_passes.md', 'world_renderers.md',
        'view_lighting_and_fog.md', 'shaders.md']
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-RND-a.json'), encoding='utf-8'))
HEADINGS = ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
            '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
            '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests',
            '## 8 Open decisions']

dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
adr_files = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])

# reference keys defined anywhere in design_next
ref_defs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`((?:ref|rnd|ter|vfx|ast|ui|cam|dat|plt|bld|dbg|aen|inp)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            ref_defs.setdefault(m.group(1), []).append(os.path.relpath(f, NEXT))

planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(tgt.rstrip('/'))
        if tgt.endswith('/'):
            planned.add(tgt.rstrip('/') + '/README.md')

problems = []
all_ids = []
alltext = ''
req_text = ''
mermaid = 0
total = 0
keys_here = []
for name in MINE:
    path = os.path.join(FOLDER, name)
    txt = open(path, encoding='utf-8').read()
    alltext += txt
    size = os.path.getsize(path)
    total += size
    if size > 120 * 1024:
        problems.append(f'{name}: {size} bytes > 120 KB')
    for h in HEADINGS:
        if not re.search('^' + re.escape(h) + r'\s*$', txt, re.M):
            problems.append(f'{name}: missing heading {h}')
    mermaid += len(re.findall(r'^```mermaid', txt, re.M))
    ids = re.findall(r'\*\*RND-(\d{3})\*\*', txt)
    all_ids += [(name, int(i)) for i in ids]
    paras = [p for p in re.split(r'\n(?=\*\*RND-\d{3}\*\*)', txt) if p.startswith('**RND-')]
    # a requirement paragraph ends at the next blank-line-separated heading
    paras = [re.split(r'\n#{2,4} ', p)[0] for p in paras]
    req_text += '\n'.join(paras)
    for d in set(re.findall(r'DEC-\d{3}', txt)):
        if d not in dec_status:
            problems.append(f'{name}: {d} does not exist')
    for a in set(re.findall(r'ADR-\d{4}', txt)):
        if a not in adr_files:
            problems.append(f'{name}: {a} does not exist')
    for a in set(re.findall(r'`((?:fx|terrain|mesh|sprite|ui(?!\.error|\.match)|marker|icon|help|sfx)\.[a-z0-9_.]+)`', txt)):
        if a not in assets:
            problems.append(f'{name}: asset {a} not in the manifest')
    body = re.sub(r'```.*?```', '', txt, flags=re.S)
    for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(pat, body, re.I):
            line = body[:m.start()].count('\n') + 1
            problems.append(f'{name}: truncation "{m.group(0)}" near body line {line}')
    for k in sorted(set(re.findall(r'`((?:ref|rnd)\.[a-z0-9_.]+)`', txt))):
        if k not in ref_defs:
            problems.append(f'{name}: key {k} is not defined anywhere')
    for line in txt.split('\n'):
        m = re.match(r'\|\s*`(rnd\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            k = m.group(1)
            keys_here.append(k)
            if len(ref_defs[k]) > 1:
                problems.append(f'{name}: key {k} defined in {ref_defs[k]}')
            if not any(('`' + k + '`') in p for p in paras):
                problems.append(f'{name}: key {k} is not cited by a requirement paragraph of this file')
    for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
        if target.startswith('http'):
            continue
        tp = os.path.normpath(os.path.join(FOLDER, target))
        rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
        if not os.path.exists(tp) and rel not in planned:
            problems.append(f'{name}: link target neither on disk nor a package target: {target}')

seen = {}
lo, hi = (int(x) for x in pkg['id_ranges']['RND'].split('-'))
for name, n in all_ids:
    if not lo <= n <= hi:
        problems.append(f'{name}: RND-{n:03d} outside {lo}-{hi}')
    if n in seen:
        problems.append(f'RND-{n:03d} defined in {seen[n]} and {name}')
    seen[n] = name
defined = {f'RND-{n:03d}' for _, n in all_ids}
for r in sorted(set(re.findall(r'RND-\d{3}', alltext))):
    if int(r[4:]) <= hi and r not in defined:
        problems.append(f'cites {r}, which no file of mine defines')
for d in pkg['decs_to_cite']:
    dec, status = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if status == 'Open' and dec not in req_text:
        problems.append(f'Open {dec} not cited by a requirement paragraph')
    if dec not in alltext:
        problems.append(f'{d} not cited at all')
r_sim = sorted(set(re.findall(r'R-SIM-\d{3}', alltext)))
sim_text = ''.join(open(f, encoding='utf-8').read() for f in glob.glob(os.path.join(NEXT, 'reference', '12_simulation_model', '*.md')))
for r in r_sim:
    if f'**{r}**' not in sim_text:
        problems.append(f'cites {r}, not defined in reference/12')
if len(keys_here) != len(set(keys_here)):
    problems.append('duplicate keys within my files')
print(f'files: {len(MINE)}; total bytes: {total}; requirements: {len(all_ids)}; '
      f'keys: {len(keys_here)}; mermaid: {mermaid}; R-SIM cited: {len(r_sim)}')
for name in MINE:
    print(f'  {name}: {os.path.getsize(os.path.join(FOLDER, name))} bytes')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
