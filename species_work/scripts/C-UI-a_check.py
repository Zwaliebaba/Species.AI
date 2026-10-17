"""C-UI-a self-check: requirement ids, tuning keys, DEC/ADR/asset references, cited
requirement ids of other documents, template headings, truncation words, links, sizes.

Usage: python3 -I C-UI-a_check.py <REPO>
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
FOLDER = os.path.join(NEXT, 'architecture', 'components', 'ui_framework')
MINE = ['README.md', 'core_and_widgets.md', 'layout_rendering_input_and_localisation.md']
OTHER = 'pages_and_screen_recipes.md'
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-UI-a.json'), encoding='utf-8'))
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

key_defs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`((?:ref|rnd|ter|vfx|ast|ui|cam|dat|plt|bld|dbg|aen|inp)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            key_defs.setdefault(m.group(1), []).append(os.path.relpath(f, NEXT))

req_defs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    txt = open(f, encoding='utf-8').read()
    for r in re.findall(r'\*\*((?:R-[A-Z]+|RND|UI|TER|AST|VFX|AEN|INP|CAM|DAT|PLT|BLD|DBG)-\d{3})\*\*', txt):
        req_defs.setdefault(r, []).append(os.path.relpath(f, NEXT))

planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(tgt.rstrip('/'))
        if tgt.endswith('/'):
            planned.add(tgt.rstrip('/') + '/README.md')

problems = []
ids = []
alltext = ''
req_text = ''
mermaid = 0
my_keys = []
for name in MINE:
    path = os.path.join(FOLDER, name)
    txt = open(path, encoding='utf-8').read()
    alltext += txt
    size = os.path.getsize(path)
    if size > 120 * 1024:
        problems.append(f'{name}: {size} bytes > 120 KB')
    for h in HEADINGS:
        if not re.search('^' + re.escape(h) + r'\s*$', txt, re.M):
            problems.append(f'{name}: missing heading {h}')
    mermaid += len(re.findall(r'^```mermaid', txt, re.M))
    for i in re.findall(r'\*\*UI-(\d{3})\*\*', txt):
        ids.append((name, int(i)))
    paras = [p for p in re.split(r'\n(?=\*\*UI-\d{3}\*\*)', txt) if p.startswith('**UI-')]
    paras = [re.split(r'\n#{2,4} ', p)[0] for p in paras]
    req_text += '\n'.join(paras)
    for d in set(re.findall(r'DEC-\d{3}', txt)):
        if d not in dec_status:
            problems.append(f'{name}: {d} does not exist')
    for a in set(re.findall(r'ADR-\d{4}', txt)):
        if a not in adr_files:
            problems.append(f'{name}: {a} does not exist')
    for a in set(re.findall(r'`((?:fx|terrain|mesh|sprite|ui|marker|icon|help|sfx|font|strings|glyph|cursor|amb|anim)\.[a-z0-9_.]+)`', txt)):
        if a.startswith('ui.') and a.split('.')[1] in ('core', 'chrome', 'message_dialog', 'window', 'button', 'value_control', 'field', 'increment', 'scroll', 'check', 'dropdown', 'dialog', 'options_screen', 'menu_control', 'page_button', 'page_back', 'page_title', 'fill_box', 'page_dropdown', 'page_field', 'palette', 'layout', 'scale', 'access', 'text', 'font', 'glyph', 'lang', 'budget', 'title', 'common', 'chat', 'pages', 'scroll_text', 'picker', 'map_row', 'chat_log'):
            continue
        if a.endswith('.') or a.endswith('*'):
            continue
        if a not in assets:
            problems.append(f'{name}: asset {a} not in the manifest')
    body = re.sub(r'```.*?```', '', txt, flags=re.S)
    for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(pat, body, re.I):
            line = body[:m.start()].count('\n') + 1
            problems.append(f'{name}: truncation "{m.group(0)}" near body line {line}')
    for k in sorted(set(re.findall(r'`((?:ref|rnd|ui)\.[a-z0-9_]+\.[a-z0-9_.]+)`', txt))):
        if k.startswith('ui.') and k.split('.')[1] in ('title', 'common', 'chat', 'dialog') and k not in key_defs:
            continue  # string ids
        if k not in key_defs:
            problems.append(f'{name}: key {k} is not defined anywhere')
    for line in txt.split('\n'):
        m = re.match(r'\|\s*`(ui\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            k = m.group(1)
            my_keys.append(k)
            if len(key_defs.get(k, [])) > 1:
                problems.append(f'{name}: key {k} defined in {key_defs[k]}')
    for r in sorted(set(re.findall(r'\b((?:R-[A-Z]+|RND|TER|AST|VFX|AEN|INP|CAM|DAT|PLT|BLD|DBG)-\d{3})\b', txt))):
        if r not in req_defs:
            problems.append(f'{name}: cites {r}, which no document defines')
    for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
        if target.startswith('http'):
            continue
        tp = os.path.normpath(os.path.join(FOLDER, target))
        rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
        if not os.path.exists(tp) and rel not in planned:
            problems.append(f'{name}: link target neither on disk nor a package target: {target}')

seen = {}
lo, hi = (int(x) for x in pkg['id_ranges']['UI'].split('-'))
prev = 0
for name, n in ids:
    if not lo <= n <= hi:
        problems.append(f'{name}: UI-{n:03d} outside {lo}-{hi}')
    if n in seen:
        problems.append(f'UI-{n:03d} defined in {seen[n]} and {name}')
    seen[n] = name
    if n < prev:
        problems.append(f'{name}: UI-{n:03d} not ascending')
    prev = n
defined = {f'UI-{n:03d}' for _, n in ids}
other_txt = open(os.path.join(FOLDER, OTHER), encoding='utf-8').read() if os.path.exists(os.path.join(FOLDER, OTHER)) else ''
other_defined = set(re.findall(r'\*\*(UI-\d{3})\*\*', other_txt))
for r in sorted(set(re.findall(r'UI-\d{3}', alltext))):
    n = int(r[3:])
    if n <= hi and r not in defined:
        problems.append(f'cites {r}, which no file of mine defines')
    if n > hi and r not in other_defined:
        problems.append(f'cites {r}, which pages_and_screen_recipes.md does not define')
for d in pkg['decs_to_cite']:
    dec, status = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if status == 'Open' and dec not in req_text:
        problems.append(f'Open {dec} not cited by a requirement paragraph')
    if dec not in alltext:
        problems.append(f'{d} not cited at all')
for a in pkg['assets_for_you']:
    if a not in alltext:
        problems.append(f'asset {a} of assets_for_you not cited')
if len(my_keys) != len(set(my_keys)):
    dup = sorted({k for k in my_keys if my_keys.count(k) > 1})
    problems.append(f'duplicate keys within my files: {dup}')
# every owned key cited by some requirement paragraph
for k in sorted(set(my_keys)):
    if ('`' + k + '`') not in req_text:
        problems.append(f'key {k} not cited by any requirement paragraph')
# missing ids in sequence (informational)
gaps = [n for n in range(1, max(seen) + 1) if n not in seen]
print(f'requirements: {len(ids)}; keys: {len(my_keys)}; mermaid: {mermaid}; gaps: {gaps}')
for name in MINE:
    print(f'  {name}: {os.path.getsize(os.path.join(FOLDER, name))} bytes')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
