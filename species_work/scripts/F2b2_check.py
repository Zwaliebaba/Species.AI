"""Validate routing_F2b2.csv and print package sizing.

Usage: python3 -I F2b2_check.py <routing.csv> <drop_topics.md> <headings.csv>
"""
import csv, re, sys
from collections import Counter, defaultdict
rpath, dpath, hpath = sys.argv[1:4]
codes = set(re.findall(r'^\| `([A-Z]{2}-[A-Z0-9-]+)` \|', open(dpath, encoding='utf-8').read(), re.M))
rows = list(csv.DictReader(open(rpath, encoding='utf-8')))
pk = {'G00','G01','G02a','G02b','G03a','G03b','G04','G05','G06','G07','G08','G09','G10','G11','C-SIM','C-TER','C-RND','C-AST','C-VFX','C-AEN','C-UI','C-INP','C-CAM','C-DAT','C-PLT','C-BLD','C-DBG','X-GLO','X-AST'}
errs = []
for i, r in enumerate(rows, 2):
    a, own, note = r['action'], r['owner_package'], r['note']
    code = note.split(':', 1)[0] if ':' in note else ''
    if a.startswith('DROP') or a in ('DEFER-DESIGN', 'KEEP-HOOKS'):
        if code not in codes:
            errs.append(f'line {i}: {a} note lacks valid code: {note[:40]}')
        if a.startswith('DROP') and code[:2] not in ('DL', 'DP'):
            errs.append(f'line {i}: code family mismatch')
        if a == 'DROP-PROVENANCE' and not code.startswith('DP-'): errs.append(f'line {i}: DP expected')
        if a == 'DROP-LEGACY' and not code.startswith('DL-'): errs.append(f'line {i}: DL expected')
        if a == 'DEFER-DESIGN' and not code.startswith('DD-'): errs.append(f'line {i}: DD expected')
        if a == 'KEEP-HOOKS' and not code.startswith('KH-'): errs.append(f'line {i}: KH expected')
    if a.startswith('DROP'):
        if own or r['target_file'] or r['target_section']: errs.append(f'line {i}: DROP with target/owner')
    elif a in ('TO-DEC', 'DEFER-DESIGN'):
        if own != 'X-DEC': errs.append(f'line {i}: {a} owner {own}')
    elif a == 'TO-ADR':
        if own != 'X-ADR': errs.append(f'line {i}: TO-ADR owner {own}')
    elif a == 'TO-ASSETS':
        if own != 'X-AST': errs.append(f'line {i}: TO-ASSETS owner {own}')
    else:
        if own not in pk: errs.append(f'line {i}: bad owner {own}')
    if not a.startswith('DROP') and not r['target_file']: errs.append(f'line {i}: missing target')
    if r['target_file'].startswith('/') or 'design_next' in r['target_file']: errs.append(f'line {i}: target not relative')
    if '…' in note or 'etc.' in note: errs.append(f'line {i}: truncation word in note')
print('errors:', len(errs)); [print(' ', e) for e in errs[:40]]
print('rows', len(rows))
print('actions', dict(sorted(Counter(r['action'] for r in rows).items())))
size = defaultdict(int); hooks = defaultdict(int); byfile = defaultdict(lambda: defaultdict(int))
for r in rows:
    if r['action'] in ('KEEP', 'MERGE'):
        size[r['owner_package']] += int(r['bytes']); byfile[r['owner_package']][r['orig_file'][:2]] += int(r['bytes'])
    if r['action'] == 'KEEP-HOOKS':
        hooks[r['owner_package']] += int(r['bytes'])
for k in sorted(size, key=lambda k: -size[k]):
    print(f'{k:6} {size[k]:>8}  hooks {hooks.get(k,0):>6}  ' + ' '.join(f'{f}:{b}' for f, b in sorted(byfile[k].items())))
for k in hooks:
    if k not in size: print(f'{k:6} {0:>8}  hooks {hooks[k]:>6}')
# per-file action bytes (unique ranges by action)
fb = defaultdict(lambda: defaultdict(int))
for r in rows:
    fb[r['orig_file'][:2]][r['action']] += int(r['bytes'])
for f in sorted(fb):
    print(f, dict(sorted(fb[f].items())))
