"""C-VFX self-check: requirement ids, tuning keys, DEC/ADR/asset references, cited keys and
requirement ids, template headings, truncation words, links, sizes and Mermaid count.

Usage: python3 -I C-VFX_check.py <REPO>
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
FOLDER = os.path.join(NEXT, 'architecture', 'components', 'animation_and_vfx')
MINE = {'README.md': (1, 99), 'explainers_sprites_and_procedural_animation.md': (100, 199),
        'particles_shatter_and_screen_effects.md': (200, 299)}
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-VFX.json'), encoding='utf-8'))
HEADINGS = ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
            '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
            '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests',
            '## 8 Open decisions', '## 9 Tuning table']

dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
adr_files = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])

key_defs = {}
req_defs = set()
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    txt = open(f, encoding='utf-8').read()
    for line in txt.split('\n'):
        m = re.match(r'\|\s*`((?:ref|rnd|ter|vfx|ast|ui|cam|dat|plt|bld|dbg|aen|inp)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            key_defs.setdefault(m.group(1), []).append(os.path.relpath(f, NEXT))
    for r in re.findall(r'\*\*((?:R-[A-Z]+|RND|TER|VFX|UI|CAM|DBG|AST|AEN|DAT|PLT|BLD|INP)-\d{3})\*\*', txt):
        req_defs.add(r)

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
for name, (lo, hi) in MINE.items():
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
    ids = [int(i) for i in re.findall(r'\*\*VFX-(\d{3})\*\*', txt)]
    if ids != sorted(ids):
        problems.append(f'{name}: requirement ids not in increasing order')
    for i in ids:
        if not lo <= i <= hi:
            problems.append(f'{name}: VFX-{i:03d} outside {lo}-{hi}')
    all_ids += [(name, i) for i in ids]
    paras = [p for p in re.split(r'\n(?=\*\*VFX-\d{3}\*\*)', txt) if p.startswith('**VFX-')]
    paras = [re.split(r'\n#{2,4} ', p)[0] for p in paras]
    ptext = '\n'.join(paras)
    req_text += ptext
    for d in set(re.findall(r'DEC-\d{3}', txt)):
        if d not in dec_status:
            problems.append(f'{name}: {d} does not exist')
    for a in set(re.findall(r'ADR-\d{4}', txt)):
        if a not in adr_files:
            problems.append(f'{name}: {a} does not exist')
    for a in set(re.findall(r'`((?:fx|terrain|mesh|sprite|marker|icon|cursor|help|anim|sfx)\.[a-z0-9_.]+)`', txt)):
        if a.endswith('.') or a.endswith('.*'):
            continue
        if a not in assets:
            problems.append(f'{name}: asset {a} not in the manifest')
    body = re.sub(r'```.*?```', '', txt, flags=re.S)
    for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(pat, body, re.I):
            line = body[:m.start()].count('\n') + 1
            problems.append(f'{name}: truncation "{m.group(0)}" near body line {line}')
    for k in sorted(set(re.findall(r'`((?:ref|rnd|ter|vfx)\.[a-z0-9_.]+)`', txt))):
        if k.endswith('.'):
            continue
        if k not in key_defs:
            problems.append(f'{name}: key {k} is not defined anywhere')
    for line in txt.split('\n'):
        m = re.match(r'\|\s*`(vfx\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            k = m.group(1)
            keys_here.append(k)
            if len(key_defs[k]) > 1:
                problems.append(f'{name}: key {k} defined in {key_defs[k]}')
            if ('`' + k + '`') not in ptext and not re.search(r'`' + re.escape(k.rsplit('.', 1)[0]) + r'\.\*`', ptext):
                problems.append(f'{name}: key {k} is not cited by a requirement paragraph of this file')
    for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
        if target.startswith('http'):
            continue
        tp = os.path.normpath(os.path.join(FOLDER, target))
        rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
        if not os.path.exists(tp) and rel not in planned:
            problems.append(f'{name}: link target neither on disk nor a package target: {target}')
    for k in sorted(set(re.findall(r'`(vfx\.[a-z0-9_.]+)`', txt))):
        if k not in key_defs and not k.endswith('.'):
            problems.append(f'{name}: vfx key {k} used but not in a tuning table')

seen = {}
for name, n in all_ids:
    if n in seen:
        problems.append(f'VFX-{n:03d} defined in {seen[n]} and {name}')
    seen[n] = name
defined = {f'VFX-{n:03d}' for _, n in all_ids}
for r in sorted(set(re.findall(r'\bVFX-\d{3}\b', alltext))):
    if r not in defined and r not in ('VFX-099', 'VFX-100', 'VFX-199', 'VFX-200', 'VFX-299', 'VFX-001'):
        problems.append(f'cites {r}, which is not defined')
for r in sorted(set(re.findall(r'\b(?:R-[A-Z]+|RND|TER)-\d{3}\b', alltext))):
    if r not in req_defs:
        problems.append(f'cites {r}, not defined in the set')
for d in pkg['decs_to_cite']:
    dec, status = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if status == 'Open' and dec not in req_text:
        problems.append(f'Open {dec} not cited by a requirement paragraph')
    if dec not in alltext:
        problems.append(f'{d} not cited at all')
for a in pkg['assets_for_you']:
    if ('`' + a + '`') not in alltext:
        problems.append(f'asset {a} for me is not cited')
if len(keys_here) != len(set(keys_here)):
    dup = sorted({k for k in keys_here if keys_here.count(k) > 1})
    problems.append(f'duplicate keys within my files: {dup}')
print(f'files: {len(MINE)}; total bytes: {total}; requirements: {len(all_ids)}; '
      f'keys: {len(keys_here)}; mermaid: {mermaid}')
for name in MINE:
    txt = open(os.path.join(FOLDER, name), encoding='utf-8').read()
    print(f'  {name}: {os.path.getsize(os.path.join(FOLDER, name))} bytes; '
          f'{len(re.findall(r"[*][*]VFX-(\d{3})[*][*]", txt))} requirements; '
          f'{len(re.findall(r"^[|][ ]*`vfx[.]", txt, re.M))} keys; '
          f'{len(re.findall(chr(94) + "```mermaid", txt, re.M))} mermaid; '
          f'{len(re.findall(r"^[|]---", txt, re.M))} tables')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
