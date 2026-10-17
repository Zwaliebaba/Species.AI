#!/usr/bin/env python3
"""G02a1 self-check for the units documents.

Usage: python3 -I G02a1_check.py <design_next dir> <file> [<file> ...]

Checks: requirement ids (range 001-299, unique, increasing per file), cited
requirement ids exist, tuning keys unique, cited ref.unit keys defined in the
given files or in sibling files of the folder, cited ref.* keys of other areas
defined somewhere in design_next, DEC ids exist, ADR ids exist, asset ids exist
in the manifest, template headings present, truncation words absent.
"""
import csv
import os
import re
import sys

root = sys.argv[1]
files = sys.argv[2:]

REQ_DEF = re.compile(r'\*\*R-UNIT-(\d{3})\*\*')
REQ_REF = re.compile(r'R-UNIT-(\d{3})')
KEY_DEF = re.compile(r'^\| `(ref\.[a-z0-9_.]+)`', re.M)
KEY_REF = re.compile(r'`(ref\.[a-z0-9_.*]+)`')
DEC_REF = re.compile(r'DEC-(\d{3})')
ADR_REF = re.compile(r'ADR-(\d{4})')
ASSET_REF = re.compile(r'`((?:sfx|sprite|mesh|fx|marker|icon)\.[a-z0-9_.]+)`')
HEADINGS = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows',
            '## 4 Tuning table', '## 5 Edge cases and interactions', '## 6 Acceptance criteria',
            '## 7 Dependencies', '## 8 Open decisions']
TRUNC = re.compile(r'\betc\.|and so on|and others|\bvarious\b|similar to the above|…|\.\.\.')

problems = []
all_defs = {}
texts = {}
for f in files:
    t = open(f, encoding='utf-8').read()
    texts[f] = t
    ids = [int(x) for x in REQ_DEF.findall(t)]
    for i in ids:
        if not 1 <= i <= 299:
            problems.append(f'{f}: id out of range R-UNIT-{i:03d}')
    if ids != sorted(ids):
        problems.append(f'{f}: ids not increasing')
    for k in KEY_DEF.findall(t):
        all_defs.setdefault(k, []).append(f)
    for h in HEADINGS:
        if h not in t:
            problems.append(f'{f}: missing heading {h}')
    for m in TRUNC.finditer(t):
        line = t.count('\n', 0, m.start()) + 1
        problems.append(f'{f}:{line}: truncation word {m.group(0)!r}')

all_ids = []
for f in files:
    all_ids += [int(x) for x in REQ_DEF.findall(texts[f])]
dups = {i for i in all_ids if all_ids.count(i) > 1}
if dups:
    problems.append(f'duplicate ids: {sorted(dups)}')
idset = set(all_ids)
for f in files:
    for x in REQ_REF.findall(texts[f]):
        if int(x) not in idset and int(x) < 300:
            problems.append(f'{f}: cites undefined R-UNIT-{x}')
for k, fs in all_defs.items():
    if len(fs) > 1:
        problems.append(f'duplicate key {k} in {fs}')

# keys defined anywhere in design_next
other_defs = set()
for dp, dn, fn in os.walk(root):
    for n in fn:
        if n.endswith('.md'):
            p = os.path.join(dp, n)
            if p in files:
                continue
            for k in KEY_DEF.findall(open(p, encoding='utf-8').read()):
                other_defs.add(k)
for f in files:
    for k in set(KEY_REF.findall(texts[f])):
        if '*' in k:
            continue
        if k not in all_defs and k not in other_defs:
            problems.append(f'{f}: cites undefined key {k}')

# decisions
dec_ids = set()
for dp, dn, fn in os.walk(os.path.join(root, 'decisions')):
    for n in fn:
        if n.endswith('.md'):
            for m in re.finditer(r'^### DEC-(\d{3}) ', open(os.path.join(dp, n), encoding='utf-8').read(), re.M):
                dec_ids.add(m.group(1))
adr_ids = set()
for n in os.listdir(os.path.join(root, 'adr')):
    m = re.match(r'(\d{4})-', n)
    if m:
        adr_ids.add(m.group(1))
assets = set()
with open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for r in csv.DictReader(fh):
        assets.add(r['asset_id'])
for f in files:
    for d in set(DEC_REF.findall(texts[f])):
        if d not in dec_ids:
            problems.append(f'{f}: unknown DEC-{d}')
    for a in set(ADR_REF.findall(texts[f])):
        if a not in adr_ids:
            problems.append(f'{f}: unknown ADR-{a}')
    for a in set(ASSET_REF.findall(texts[f])):
        if a not in assets:
            problems.append(f'{f}: unknown asset {a}')

for f in files:
    t = texts[f]
    print(f'{f}: {len(REQ_DEF.findall(t))} requirements, {len(KEY_DEF.findall(t))} keys, '
          f'{t.count("```mermaid")} diagrams, {os.path.getsize(f)} bytes')
print(f'total keys {len(all_defs)}, total requirements {len(all_ids)}')
for p in problems:
    print('PROBLEM', p)
sys.exit(1 if problems else 0)
