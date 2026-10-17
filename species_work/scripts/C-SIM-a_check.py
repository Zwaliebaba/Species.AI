"""C-SIM-a self-check: ids, DEC references, tuning keys, truncation words, links, sizes.

Usage: python3 -I C-SIM-a_check.py <REPO>
"""
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
FOLDER = os.path.join(NEXT, 'reference', '12_simulation_model')
MINE = ['README.md', '02_match_lifecycle_and_state_reset.md',
        '03_entity_model_and_identity.md', '04_containers_and_update_order.md']
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-SIM-a.json'), encoding='utf-8'))

dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_ids = set(re.findall(r'^\| (DEC-\d{3}) \|', dec_index, re.M))
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
adr_ids = set(re.findall(r'(ADR-\d{4})', open(os.path.join(NEXT, 'adr', 'README.md'), encoding='utf-8').read()))

# tuning keys defined anywhere in design_next
defs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`(ref\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            defs.setdefault(m.group(1), []).append(os.path.relpath(f, NEXT))

problems = []
all_ids = []
alltext = ''
for name in MINE:
    path = os.path.join(FOLDER, name)
    txt = open(path, encoding='utf-8').read()
    alltext += txt
    size = os.path.getsize(path)
    if size > 120 * 1024:
        problems.append(f'{name}: {size} bytes > 120 KB')
    ids = re.findall(r'\*\*R-SIM-(\d{3})\*\*', txt)
    all_ids += [(name, int(i)) for i in ids]
    # cited ids exist somewhere in the folder
    # DEC and ADR references
    for d in set(re.findall(r'DEC-\d{3}', txt)):
        if d not in dec_ids:
            problems.append(f'{name}: {d} does not exist')
    for a in set(re.findall(r'ADR-\d{4}', txt)):
        if a not in adr_ids:
            problems.append(f'{name}: {a} does not exist')
    # truncation words outside code blocks
    body = re.sub(r'```.*?```', '', txt, flags=re.S)
    for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(pat, body, re.I):
            line = body[:m.start()].count('\n') + 1
            problems.append(f'{name}: truncation "{m.group(0)}" near body line {line}')
    # tuning keys
    used = set(re.findall(r'`(ref\.[a-z0-9_.]+)`', txt))
    for k in sorted(used):
        if k not in defs:
            problems.append(f'{name}: key {k} is not defined anywhere')
    for line in txt.split('\n'):
        m = re.match(r'\|\s*`(ref\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            k = m.group(1)
            if len(defs[k]) > 1:
                problems.append(f'{name}: key {k} defined in {defs[k]}')
            # cited by at least one requirement paragraph in this file
            req_paras = [p for p in re.split(r'\n(?=\*\*R-SIM-)', txt) if p.startswith('**R-SIM-')]
            if not any(('`' + k + '`') in p for p in req_paras):
                problems.append(f'{name}: key {k} is not cited by a requirement paragraph')
    # relative links
    for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
        if target.startswith('http'):
            continue
        tp = os.path.normpath(os.path.join(FOLDER, target))
        if not os.path.exists(tp):
            problems.append(f'{name}: link target missing (may be another package): {target}')

seen = {}
lo, hi = (int(x) for x in pkg['id_ranges']['R-SIM'].split('-'))
for name, n in all_ids:
    if not lo <= n <= hi:
        problems.append(f'{name}: R-SIM-{n:03d} outside {lo}-{hi}')
    if n in seen:
        problems.append(f'R-SIM-{n:03d} defined in {seen[n]} and {name}')
    seen[n] = name
# ids cited in my files exist in the folder
folder_text = ''.join(open(f, encoding='utf-8').read() for f in glob.glob(os.path.join(FOLDER, '*.md')))
defined = set(re.findall(r'\*\*(R-SIM-\d{3})\*\*', folder_text))
for name in MINE:
    txt = open(os.path.join(FOLDER, name), encoding='utf-8').read()
    for r in sorted(set(re.findall(r'R-SIM-\d{3}', txt))):
        if r not in defined:
            problems.append(f'{name}: cites {r}, which no file of the folder defines')
for d in pkg['decs_to_cite']:
    dec = d.split(' ')[0]
    if dec not in alltext:
        problems.append(f'decs_to_cite {d} not cited')
print(f'requirements: {len(all_ids)}; keys defined here: '
      f'{sum(1 for k, v in defs.items() for f in v if f.startswith("reference/12_simulation_model/") and os.path.basename(f) in MINE)}')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
