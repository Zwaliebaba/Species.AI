#!/usr/bin/env python3
"""G06 self-check for the controls document.

Usage: python3 -I G06_check.py <design_next dir> <file> [<file> ...]

Checks: R-CTRL ids (001-599, unique, increasing per file); cited R-CTRL ids
exist; tuning keys unique; every own key (ref.ctrl.*) cited by a requirement;
cited ref.* keys of other documents defined somewhere in design_next (warning
only for documents not yet written); DEC, ADR and asset ids exist; cited
PlayerCommand members exist in the command catalogue; template headings;
truncation words.
"""
import csv
import os
import re
import sys

root = sys.argv[1]
files = sys.argv[2:]
REQ_DEF = re.compile(r'\*\*R-CTRL-(\d{3})\*\*')
REQ_REF = re.compile(r'R-CTRL-(\d{3})')
KEY_DEF = re.compile(r'^\| `(ref\.[a-z0-9_.]+)`', re.M)
KEY_REF = re.compile(r'`((?:ref|rnd)\.[a-z0-9_.]+)`')
DEC_REF = re.compile(r'DEC-(\d{3})')
ADR_REF = re.compile(r'ADR-(\d{4})')
ASSET_REF = re.compile(r'`((?:sfx|sprite|mesh|fx|marker|icon|cursor|glyph|help|ui)\.[a-z0-9_.]+)`')
CMD_REF = re.compile(r'PlayerCommand::([A-Za-z]+)')
HEADINGS = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows',
            '## 4 Tuning table', '## 5 Edge cases and interactions', '## 6 Acceptance criteria',
            '## 7 Dependencies', '## 8 Open decisions']
TRUNC = re.compile(r'\betc\.|and so on|and others|\bvarious\b|similar to the above|…|\.\.\.')

problems, warnings = [], []
texts = {f: open(f, encoding='utf-8').read() for f in files}
own_keys = {}
all_ids = []
for f, t in texts.items():
    ids = [int(x) for x in REQ_DEF.findall(t)]
    all_ids += ids
    for i in ids:
        if not 1 <= i <= 599:
            problems.append(f'{f}: id out of range R-CTRL-{i:03d}')
    if ids != sorted(ids):
        problems.append(f'{f}: ids not increasing')
    for k in KEY_DEF.findall(t):
        own_keys.setdefault(k, []).append(f)
    if not f.endswith('README.md'):
        for h in HEADINGS:
            if h not in t:
                problems.append(f'{f}: missing heading {h}')
    body = re.sub(r'```.*?```', '', t, flags=re.S)
    for m in TRUNC.finditer(body):
        problems.append(f'{f}: truncation word {m.group(0)!r}')
dups = sorted({i for i in all_ids if all_ids.count(i) > 1})
if dups:
    problems.append(f'duplicate ids: {dups}')
idset = set(all_ids)
for f, t in texts.items():
    for x in REQ_REF.findall(t):
        if int(x) not in idset:
            problems.append(f'{f}: cites undefined R-CTRL-{x}')
for k, fs in own_keys.items():
    if len(fs) > 1:
        problems.append(f'duplicate key {k} in {fs}')
    if not k.startswith('ref.ctrl.'):
        problems.append(f'key outside own area: {k}')

# every own key cited by at least one requirement paragraph
alltext = '\n'.join(texts.values())
req_paras = re.findall(r'\*\*R-CTRL-\d{3}\*\*.*?(?=\n\*\*R-CTRL-|\n## |\n### |\Z)', alltext, re.S)
req_text = '\n'.join(req_paras)
for k in own_keys:
    if f'`{k}`' not in req_text:
        problems.append(f'key not cited by a requirement: {k}')
# tuning table rows: requirement column ids exist
for m in re.finditer(r'^\| `(ref\.ctrl\.[a-z0-9_.]+)` \|(.*)$', alltext, re.M):
    for x in REQ_REF.findall(m.group(2)):
        if int(x) not in idset:
            problems.append(f'tuning row {m.group(1)} cites undefined R-CTRL-{x}')

other_defs = set()
for dp, dn, fn in os.walk(root):
    for n in fn:
        p = os.path.join(dp, n)
        if n.endswith('.md') and os.path.abspath(p) not in {os.path.abspath(x) for x in files}:
            for k in re.findall(r'^\| `((?:ref|rnd)\.[a-z0-9_.]+)`', open(p, encoding='utf-8').read(), re.M):
                other_defs.add(k)
for k in sorted(set(KEY_REF.findall(alltext))):
    if k not in own_keys and k not in other_defs:
        warnings.append(f'cited key not defined yet: {k}')

dec_ids = set()
for dp, dn, fn in os.walk(os.path.join(root, 'decisions')):
    for n in fn:
        if n.endswith('.md'):
            dec_ids |= set(re.findall(r'^### DEC-(\d{3}) ', open(os.path.join(dp, n), encoding='utf-8').read(), re.M))
adr_ids = {re.match(r'(\d{4})-', n).group(1) for n in os.listdir(os.path.join(root, 'adr')) if re.match(r'\d{4}-', n)}
assets = set()
with open(os.path.join(root, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for r in csv.DictReader(fh):
        assets.add(r['asset_id'])
cmds = set(re.findall(r'`PlayerCommand::([A-Za-z]+)`',
                      open(os.path.join(root, 'reference', '12_simulation_model', '06_teams_and_player_commands.md'), encoding='utf-8').read()))
for d in sorted(set(DEC_REF.findall(alltext))):
    if d not in dec_ids:
        problems.append(f'unknown DEC-{d}')
for a in sorted(set(ADR_REF.findall(alltext))):
    if a not in adr_ids:
        problems.append(f'unknown ADR-{a}')
for a in sorted(set(ASSET_REF.findall(alltext))):
    if a not in assets and not a.startswith(('ui.power_slots', 'ui.match', 'ui.lobby', 'ui.help', 'ui.options', 'ui.debug', 'ui.camera', 'ui.controls')):
        problems.append(f'unknown asset {a}')
for c in sorted(set(CMD_REF.findall(alltext))):
    if c not in cmds:
        problems.append(f'unknown command PlayerCommand::{c}')

for f, t in texts.items():
    print(f'{f}: {len(REQ_DEF.findall(t))} requirements, {len(KEY_DEF.findall(t))} keys, '
          f'{t.count("```mermaid")} diagrams, {os.path.getsize(f)} bytes')
print(f'total keys {len(own_keys)}, total requirements {len(all_ids)}')
for w in warnings:
    print('WARN', w)
for p in problems:
    print('PROBLEM', p)
sys.exit(1 if problems else 0)
