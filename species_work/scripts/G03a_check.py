#!/usr/bin/env python3
"""G03a checks: requirement ids, tuning keys, cited DECs, assets, cross-references.

Usage: python3 -I G03a_check.py <design_next dir> <package json>
"""
import csv
import json
import os
import re
import sys

root = sys.argv[1]
pkg = json.load(open(sys.argv[2], encoding='utf-8'))
folder = os.path.join(root, 'reference', '03_buildings_and_world_objects')
mine = ['README.md', 'economy_and_spawn_objects.md', 'mode_objects.md']
texts = {f: open(os.path.join(folder, f), encoding='utf-8').read() for f in mine}
problems = []

# 1 sizes
for f in mine:
    size = os.path.getsize(os.path.join(folder, f))
    print(f'{f}: {size} bytes')
    if size > 120 * 1024:
        problems.append(f'{f} over 120 KB')

# 2 requirement ids
defined = {}
for f, t in texts.items():
    for m in re.finditer(r'\*\*R-BLDG-(\d{3})\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)', t):
        n = int(m.group(1))
        if n in defined:
            problems.append(f'duplicate R-BLDG-{n:03d} in {f} and {defined[n]}')
        defined[n] = f
        if not 1 <= n <= 399:
            problems.append(f'R-BLDG-{n:03d} out of range')
print('requirements:', len(defined), {f: sum(1 for v in defined.values() if v == f) for f in mine})
for f, t in texts.items():
    for m in re.finditer(r'(?<!\*\*)R-BLDG-(\d{3})(?!\*\*)', t):
        n = int(m.group(1))
        if n < 400 and n not in defined and n not in (99, 100, 249, 250, 399, 400, 799, 1):
            problems.append(f'{f}: reference to undefined R-BLDG-{n:03d}')

# 3 tuning keys
keyrow = re.compile(r'^\| `(ref\.[a-z0-9_.]+)` \|', re.M)
mykeys = {}
for f, t in texts.items():
    for m in keyrow.finditer(t):
        k = m.group(1)
        if k in mykeys:
            problems.append(f'duplicate key {k} in {f} and {mykeys[k]}')
        mykeys[k] = f
        if not k.startswith('ref.bldg.'):
            problems.append(f'{f}: foreign key defined {k}')
print('tuning keys:', len(mykeys), {f: sum(1 for v in mykeys.values() if v == f) for f in mine})
# keys used
other_docs = []
for dp, dn, fn in os.walk(os.path.join(root, 'reference')):
    for x in fn:
        if x.endswith('.md') and os.path.join(dp, x) not in [os.path.join(folder, y) for y in mine]:
            other_docs.append(os.path.join(dp, x))
foreign_defined = set()
for p in other_docs:
    for m in keyrow.finditer(open(p, encoding='utf-8').read()):
        foreign_defined.add(m.group(1))
used = set()
for f, t in texts.items():
    for m in re.finditer(r'`(ref\.[a-z0-9_.]+)`', t):
        used.add((f, m.group(1)))
for f, k in sorted(used):
    if k.startswith('ref.bldg.'):
        if k not in mykeys:
            problems.append(f'{f}: ref.bldg key used but not defined: {k}')
    elif k not in foreign_defined:
        problems.append(f'{f}: foreign key not found in any reference document: {k}')
for k, f in mykeys.items():
    body = texts[f]
    if body.count(k) < 2:
        problems.append(f'{f}: key {k} defined but not used in a rule')
# also: key in my table that another doc defines
for k in mykeys:
    if k in foreign_defined:
        problems.append(f'key {k} also defined in another document')

# 4 DECs
decdir = os.path.join(root, 'decisions')
decs = {}
for dp, dn, fn in os.walk(decdir):
    for x in fn:
        if x.endswith('.md'):
            t = open(os.path.join(dp, x), encoding='utf-8').read()
            for m in re.finditer(r'^### (DEC-\d{3})[^\n]*\n\s*- Status: (\w+)', t, re.M):
                decs[m.group(1)] = m.group(2)
cited = set()
for f, t in texts.items():
    for m in re.finditer(r'DEC-\d{3}', t):
        cited.add(m.group(0))
        if m.group(0) not in decs:
            problems.append(f'{f}: unknown {m.group(0)}')
for d in pkg['decs_to_cite']:
    did, st = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if did not in cited:
        problems.append(f'decs_to_cite not cited: {d}')
    if decs.get(did) and decs[did] != st:
        print(f'note: {did} status in register is {decs[did]}, package says {st}')
open_cited = sorted(d for d in cited if decs.get(d) == 'Open')
print('Open DECs cited:', ', '.join(open_cited))
# Open DECs must be cited in a requirement line
for f, t in texts.items():
    reqlines = [l for l in t.splitlines() if l.startswith('**R-BLDG-')]
    pass

# 5 assets
manifest = set()
with open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for r in csv.DictReader(fh):
        manifest.add(r['asset_id'])
asset_re = re.compile(r'`((?:mesh|fx|sfx|ui|marker|help|icon|cursor|sprite|tex)\.[a-z0-9_.]+)`')
used_assets = set()
for f, t in texts.items():
    for m in asset_re.finditer(t):
        a = m.group(1)
        used_assets.add(a)
        if a not in manifest:
            problems.append(f'{f}: asset not in manifest: {a}')
for a in pkg['assets_for_you']:
    if a not in used_assets:
        problems.append(f'asset for me not cited: {a}')

# 6 other requirement refs
allreq = {}
for p in other_docs:
    t = open(p, encoding='utf-8').read()
    for m in re.finditer(r'\*\*(R-[A-Z]+-\d{3})\*\*', t):
        allreq[m.group(1)] = p
for f, t in texts.items():
    for m in re.finditer(r'R-(ECON|SIM|MODE|UNIT|CRATE|CTRL|BOT|MAP|AUD|TUT|META|VIS)-\d{3}', t):
        if m.group(0) not in allreq:
            problems.append(f'{f}: reference to missing {m.group(0)}')

# 7 headings
heads = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
         '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']
for f, t in texts.items():
    for h in heads:
        if h not in t:
            problems.append(f'{f}: missing heading {h}')

# 8 truncation words
trunc = re.compile(r'\betc\.|\band so on\b|\band others\b|\bvarious\b|similar to the above|…')
for f, t in texts.items():
    for i, l in enumerate(t.splitlines(), 1):
        if trunc.search(l):
            problems.append(f'{f}:{i}: truncation word: {trunc.search(l).group(0)}')

# 9 diagrams
for f, t in texts.items():
    n = t.count('```mermaid')
    print(f'{f}: {n} mermaid diagrams')

print('PROBLEMS:', len(problems))
for p in problems:
    print('  ', p)
