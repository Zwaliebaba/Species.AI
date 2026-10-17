"""G04 self-check: ids, DEC references, tuning keys, assets, headings, truncation, links, sizes.

Usage: python3 -I G04_check.py <REPO>
"""
import csv, glob, json, os, re, sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
FOLDER = os.path.join(NEXT, 'reference', '04_supply_crates_powers_and_weapons')
RANGES = {'README.md': (1, 99), 'crates_and_powers.md': (100, 399), 'weapons_and_projectiles.md': (400, 599)}
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'G04.json'), encoding='utf-8'))
dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
adr_ids = set(re.findall(r'(ADR-\d{4})', open(os.path.join(NEXT, 'adr', 'README.md'), encoding='utf-8').read()))
assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), newline='', encoding='utf-8') as f:
    for row in csv.DictReader(f):
        assets.add(row['asset_id'])
defs = {}
reqdefs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    txt = open(f, encoding='utf-8').read()
    rel = os.path.relpath(f, NEXT)
    for m in re.finditer(r'^\|\s*`(ref\.[a-z0-9_.]+)`\s*\|', txt, re.M):
        defs.setdefault(m.group(1), []).append(rel)
    for m in re.finditer(r'\*\*((?:R-[A-Z]+|[A-Z]+)-\d{3})\*\*', txt):
        reqdefs.setdefault(m.group(1), []).append(rel)
planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(tgt.rstrip('/'))
        if tgt.endswith('/'):
            planned.add(tgt.rstrip('/') + '/README.md')
HEAD = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
        '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']
problems = []
alltext = ''
nreq = 0; nkeys = 0; ndiag = 0; mykeys = set()
asset_pat = re.compile(r'`((?:sfx|fx|icon|mesh|marker|sprite|cursor|glyph|help|ui)\.[a-z0-9_.]+)`')
for name, (lo, hi) in RANGES.items():
    path = os.path.join(FOLDER, name)
    txt = open(path, encoding='utf-8').read()
    alltext += txt
    size = os.path.getsize(path)
    if size > 120 * 1024:
        problems.append(f'{name}: {size} bytes > 120 KB')
    for h in HEAD:
        if h not in txt:
            problems.append(f'{name}: heading missing: {h}')
    ids = [int(i) for i in re.findall(r'\*\*R-CRATE-(\d{3})\*\*', txt)]
    nreq += len(ids)
    for i in ids:
        if not lo <= i <= hi:
            problems.append(f'{name}: R-CRATE-{i:03d} outside {lo}-{hi}')
    ndiag += txt.count('```mermaid')
    caps = len(re.findall(r'^Caption: ', txt, re.M))
    if caps != txt.count('```mermaid'):
        problems.append(f'{name}: {txt.count("```mermaid")} diagrams but {caps} captions')
    for d in set(re.findall(r'DEC-\d{3}', txt)):
        if d not in dec_status:
            problems.append(f'{name}: {d} does not exist')
    for a in set(re.findall(r'ADR-\d{4}', txt)):
        if a not in adr_ids:
            problems.append(f'{name}: {a} does not exist')
    body = re.sub(r'```.*?```', '', txt, flags=re.S)
    for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(pat, body, re.I):
            problems.append(f'{name}: truncation "{m.group(0)}" near body line {body[:m.start()].count(chr(10)) + 1}')
    for k in sorted(set(re.findall(r'`(ref\.[a-z0-9_.]+)`', txt))):
        if k not in defs and not k.endswith('.') and '*' not in k:
            problems.append(f'{name}: key {k} is not defined anywhere')
    req_paras = [p for p in re.split(r'\n(?=\*\*R-CRATE-)', txt) if p.startswith('**R-CRATE-')]
    for m in re.finditer(r'^\|\s*`(ref\.[a-z0-9_.]+)`\s*\|', txt, re.M):
        k = m.group(1); nkeys += 1; mykeys.add(k)
        if len(defs[k]) > 1:
            problems.append(f'{name}: key {k} defined in {defs[k]}')
        if not any(('`' + k + '`') in p for p in req_paras):
            problems.append(f'{name}: key {k} is not cited by a requirement paragraph')
    for a in sorted(set(asset_pat.findall(txt))):
        if a not in assets and not a.startswith(('ui.power_slots', 'ui.match', 'help.mode')):
            problems.append(f'{name}: asset {a} not in the manifest')
    for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
        if target.startswith('http'):
            continue
        tp = os.path.normpath(os.path.join(FOLDER, target))
        rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
        if not os.path.exists(tp) and rel not in planned:
            problems.append(f'{name}: link target missing: {target}')
    for r in sorted(set(re.findall(r'\b((?:R-[A-Z]+)-\d{3})\b', txt))):
        if r not in reqdefs:
            problems.append(f'{name}: cites {r}, which no file defines (yet)')
seen = {}
for name in RANGES:
    for i in re.findall(r'\*\*R-CRATE-(\d{3})\*\*', open(os.path.join(FOLDER, name), encoding='utf-8').read()):
        if i in seen:
            problems.append(f'R-CRATE-{i} twice: {seen[i]} and {name}')
        seen[i] = name
for d in pkg['decs_to_cite']:
    dec = d.split(' ')[0]
    if dec not in alltext:
        problems.append(f'decs_to_cite {d} not cited')
    elif dec_status.get(dec) == 'Open':
        # cited inside a requirement paragraph?
        paras = [p for p in re.split(r'\n(?=\*\*R-CRATE-)', alltext) if p.startswith('**R-CRATE-')]
        if not any(dec in p.split('\n\n')[0] for p in paras):
            problems.append(f'Open {dec} not cited by a requirement paragraph')
for dec in ['DEC-121', 'DEC-120', 'DEC-081']:
    if dec not in alltext:
        problems.append(f'{dec} not cited')
print(f'requirements: {nreq}; keys defined: {nkeys}; diagrams: {ndiag}')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
