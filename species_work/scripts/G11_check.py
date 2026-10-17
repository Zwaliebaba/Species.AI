#!/usr/bin/env python3
"""G11 self-check for reference/11_tutorial_and_help.md.

Usage: python3 -I G11_check.py <design_next dir> <file>

Checks: R-TUT ids (001-299, unique, increasing); cited R-TUT ids exist; tuning keys
unique and all ref.tut.*; every own key cited in a requirement paragraph; cited ref.*
keys of other documents defined somewhere in design_next (else warning); every cited
R-<PREFIX>-NNN of other documents exists in design_next (else warning); DEC, ADR and
asset ids exist; action ids exist in reference/06; every string id named in the string
tables has a text row; template headings; truncation words.
"""
import csv
import os
import re
import sys

root, f = sys.argv[1], sys.argv[2]
t = open(f, encoding='utf-8').read()
problems, warnings = [], []
ids = [int(x) for x in re.findall(r'\*\*R-TUT-(\d{3})\*\*', t)]
for i in ids:
    if not 1 <= i <= 299:
        problems.append(f'id out of range R-TUT-{i:03d}')
if ids != sorted(ids):
    problems.append('ids not increasing')
if len(ids) != len(set(ids)):
    problems.append('duplicate ids')
for x in re.findall(r'R-TUT-(\d{3})', t):
    if int(x) not in ids:
        problems.append(f'cites undefined R-TUT-{x}')
keys = re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M)
if len(keys) != len(set(keys)):
    problems.append('duplicate tuning keys')
for k in keys:
    if not k.startswith('ref.tut.'):
        problems.append(f'key outside own area {k}')
req_text = '\n'.join(re.findall(r'\*\*R-TUT-\d{3}\*\*.*?(?=\n\*\*R-TUT-|\n## |\n### |\Z)', t, re.S))
for k in keys:
    if f'`{k}`' not in req_text:
        problems.append(f'own key not cited in a requirement: {k}')
alltext = ''
for dp, dn, fn in os.walk(root):
    for n in fn:
        if n.endswith('.md') and os.path.join(dp, n) != os.path.abspath(f):
            alltext += open(os.path.join(dp, n), encoding='utf-8').read() + '\n'
cited = set(re.findall(r'`(ref\.[a-z0-9_.]+)`', t)) - set(keys)
for k in sorted(cited):
    if k.startswith('ref.tut.'):
        problems.append(f'ref.tut key cited but not defined: {k}')
    elif not re.search(r'^\| `' + re.escape(k) + r'` \|', alltext, re.M):
        warnings.append(f'foreign key not found as a table row: {k}')
for pre, num in sorted(set(re.findall(r'\b(R-(?:VIS|MODE|UNIT|BLDG|CRATE|ECON|CTRL|BOT|MAP|META|AUD|SIM))-(\d{3})\b', t))):
    if not re.search(r'\*\*' + pre + '-' + num + r'\*\*', alltext):
        warnings.append(f'cited {pre}-{num} not defined in design_next (yet)')
dec_index = open(os.path.join(root, 'decisions', 'README.md'), encoding='utf-8').read()
for d in sorted(set(re.findall(r'DEC-(\d{3})', t))):
    if f'| DEC-{d} |' not in dec_index:
        problems.append(f'unknown DEC-{d}')
adr_dir = os.listdir(os.path.join(root, 'adr'))
for a in sorted(set(re.findall(r'ADR-(\d{4})', t))):
    if not any(x.startswith(a + '-') for x in adr_dir):
        problems.append(f'unknown ADR-{a}')
assets = {r['asset_id'] for r in csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8'))}
for a in sorted(set(re.findall(r'`((?:help|anim|strings|sfx|map|icon|glyph|ui|mesh|fx|marker)\.[a-z0-9_.]+)`', t))):
    if a.startswith(('help.', 'anim.', 'strings.')) and a not in assets:
        problems.append(f'unknown asset {a}')
ctrl = ''.join(open(os.path.join(root, 'reference', '06_controls_camera_and_commands', n), encoding='utf-8').read()
               for n in os.listdir(os.path.join(root, 'reference', '06_controls_camera_and_commands')))
for a in sorted(set(re.findall(r'`(action\.[a-z_.0-9]+)`', t))):
    if f'`{a}`' not in ctrl:
        problems.append(f'unknown action id {a}')
defined = set(re.findall(r'^\| `((?:tutorial|ui\.help)\.[a-z0-9_.]+)` \|', t, re.M))
named = set(re.findall(r'`((?:tutorial\.tab|ui\.help\.(?:hint|explainer|page|mode))\.[a-z0-9_.]+)`', t))
for s in sorted(named - defined):
    if '<' not in s:
        problems.append(f'string id without a text row: {s}')
for h in ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
          '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']:
    if h not in t:
        problems.append(f'missing heading {h}')
body = re.sub(r'```.*?```', '', t, flags=re.S)
for m in re.finditer(r'\betc\.|and so on|and others|\bvarious\b|similar to the above|…|\.\.\.', body):
    problems.append(f'truncation word {m.group(0)!r}')
nmer = len(re.findall(r'```mermaid', t))
caps = len(re.findall(r'^Caption: ', t, re.M))
print(f'requirements {len(ids)}, keys {len(keys)}, mermaid {nmer}, captions {caps}, strings {len(defined)}, size {len(t.encode())}')
for p in problems:
    print('PROBLEM', p)
for w in warnings:
    print('warning', w)
sys.exit(1 if problems else 0)
