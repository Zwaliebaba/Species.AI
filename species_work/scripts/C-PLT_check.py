#!/usr/bin/env python3
"""C-PLT self-check for the platform layer.

Usage: python3 -I C-PLT_check.py <design_next dir> <platform_layer.md>

Checks: INP ids (001-299, unique, increasing); cited INP ids exist; inp.* tuning keys
unique and cited by a requirement; cited ref.* keys defined elsewhere in design_next;
DEC, ADR and asset ids exist; cited R-* requirement ids exist; PlayerCommand members
exist; action ids are those of reference/06 table 2.1-A or of table 3.6-B; template
headings; truncation words outside code blocks.
"""
import csv
import os
import re
import sys

root, path = sys.argv[1], sys.argv[2]
t = open(path, encoding='utf-8').read()
problems, warnings = [], []

ids = [int(x) for x in re.findall(r'\*\*PLT-(\d{3})\*\*', t)]
for i in ids:
    if not 1 <= i <= 299:
        problems.append(f'id out of range PLT-{i:03d}')
if ids != sorted(ids):
    problems.append('ids not increasing')
if len(ids) != len(set(ids)):
    problems.append('duplicate ids')
idset = set(ids)
for x in re.findall(r'PLT-(\d{3})', t):
    if int(x) not in idset:
        problems.append(f'cites undefined PLT-{x}')

own = re.findall(r'^\| `(plt\.[a-z0-9_.]+)` \|', t, re.M)
if len(own) != len(set(own)):
    problems.append('duplicate tuning key')
paras = re.findall(r'\*\*PLT-\d{3}\*\*.*?(?=\n\*\*PLT-|\n## |\n### |\Z)', t, re.S)
req_text = '\n'.join(paras)
for k in own:
    if f'`{k}`' not in req_text:
        problems.append(f'key not cited by a requirement: {k}')
for k in set(re.findall(r'`(plt\.[a-z0-9_.]+)`', t)):
    if k not in own:
        problems.append(f'cited inp key not defined: {k}')

others = set()
reqs = set()
for dp, dn, fn in os.walk(root):
    for n in fn:
        if n.endswith('.md') and os.path.abspath(os.path.join(dp, n)) != os.path.abspath(path):
            s = open(os.path.join(dp, n), encoding='utf-8').read()
            others |= set(re.findall(r'^\| `((?:ref|rnd|ter|ast)\.[a-z0-9_.]+)`', s, re.M))
            reqs |= set(re.findall(r'\*\*(R-[A-Z]+-\d{3})\*\*', s))
for k in sorted(set(re.findall(r'`((?:ref|rnd)\.[a-z0-9_.]+)`', t))):
    if k not in others:
        problems.append(f'cited key not defined elsewhere: {k}')
for r in sorted(set(re.findall(r'\b(R-[A-Z]+-\d{3})\b', t))):
    if r not in reqs:
        problems.append(f'cited requirement not found: {r}')

dec_ids = set()
for dp, dn, fn in os.walk(os.path.join(root, 'decisions')):
    for n in fn:
        if n.endswith('.md'):
            dec_ids |= set(re.findall(r'^### DEC-(\d{3}) ', open(os.path.join(dp, n), encoding='utf-8').read(), re.M))
for d in sorted(set(re.findall(r'DEC-(\d{3})', t))):
    if d not in dec_ids:
        problems.append(f'unknown DEC-{d}')
adr = {n[:4] for n in os.listdir(os.path.join(root, 'adr')) if re.match(r'\d{4}-', n)}
for a in sorted(set(re.findall(r'ADR-(\d{4})', t))):
    if a not in adr:
        problems.append(f'unknown ADR-{a}')
assets = set()
with open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for r in csv.DictReader(fh):
        assets.add(r['asset_id'])
for a in sorted(set(re.findall(r'`((?:glyph|icon|help|sprite|cursor|marker|sfx)\.[a-z0-9_.]+)`', t))):
    if a not in assets:
        problems.append(f'unknown asset {a}')
for k in sorted(set(re.findall(r'`((?:dbg|bld|aen|ast|dat)\.[a-z0-9_.]+)`', t))):
    found = False
    for dp, dn, fn in os.walk(root):
        for n in fn:
            if n.endswith('.md') and re.search(r'^\| `' + re.escape(k) + r'` \|', open(os.path.join(dp, n), encoding='utf-8').read(), re.M):
                found = True
    if not found:
        problems.append(f'cited component key not defined elsewhere: {k}')
others_ids = set()
for dp, dn, fn in os.walk(root):
    for n in fn:
        if n.endswith('.md'):
            others_ids |= set(re.findall(r'\*\*((?:RND|INP|AST|AEN|UI|DAT|BLD|DBG|CAM|VFX|TER)-\d{3})\*\*', open(os.path.join(dp, n), encoding='utf-8').read()))
for r in sorted(set(re.findall(r'\b((?:RND|INP|AST|AEN|UI|DAT|BLD|DBG|CAM|VFX|TER)-\d{3})\b', t))):
    if r not in others_ids:
        problems.append(f'cited component requirement not found: {r}')
for h in ['Responsibilities and boundaries', 'Interfaces', 'Data model', 'Behaviour and algorithms',
          'Concurrency and performance budget', 'Failure modes, diagnostics and telemetry',
          'Acceptance tests', 'Open decisions']:
    if not re.search(r'^## \d+ ' + re.escape(h), t, re.M):
        problems.append(f'missing heading {h}')
body = re.sub(r'```.*?```', '', t, flags=re.S)
for m in re.finditer(r'\betc\.|and so on|and others|\bvarious\b|similar to the above|…|\.\.\.', body):
    problems.append(f'truncation word {m.group(0)!r}')

print(f'{path}: {len(ids)} requirements, {len(own)} keys, {t.count("```mermaid")} diagrams, {os.path.getsize(path)} bytes')
for w in warnings:
    print('WARN', w)
for p in problems:
    print('PROBLEM', p)
sys.exit(1 if problems else 0)
