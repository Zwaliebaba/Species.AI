#!/usr/bin/env python3
"""G10 self-check for the audio design folder.

Usage: python3 -I G10_check.py <design_next dir> <file> [<file> ...]

Checks: R-AUD ids (001-499, unique); cited R-AUD ids exist; tuning keys
unique; every own key (ref.aud.*) cited by a requirement line; cited ref.*
keys of other documents defined somewhere in design_next; DEC and ADR ids
exist; Open DECs of the package cited by a requirement; asset ids exist in the
manifest; template headings; truncation words outside code blocks.
"""
import csv
import os
import re
import sys

root = sys.argv[1]
files = sys.argv[2:]
REQ_DEF = re.compile(r'\*\*R-AUD-(\d{3})\*\*')
REQ_REF = re.compile(r'R-AUD-(\d{3})')
KEY_DEF = re.compile(r'^\| `(ref\.[a-z0-9_.]+)`', re.M)
KEY_REF = re.compile(r'`(ref\.[a-z0-9_.]+)`')
DEC_REF = re.compile(r'DEC-(\d{3})')
ADR_REF = re.compile(r'ADR-(\d{4})')
ASSET_REF = re.compile(r'`((?:sfx|amb|music)\.[a-z0-9_.]+)`')
HEADINGS = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows',
            '## 4 Tuning table', '## 5 Edge cases and interactions', '## 6 Acceptance criteria',
            '## 7 Dependencies', '## 8 Open decisions']
TRUNC = re.compile(r'\betc\.|and so on|and others|\bvarious\b|similar to the above|…|\.\.\.')
OPEN = ['049', '071', '109', '317', '318', '319', '320', '321', '322', '565', '566', '567']
RESOLVED = ['004', '005', '006', '007', '027', '042', '072', '097', '323', '331', '458', '564',
            '568', '569', '570', '751']

problems = []
texts = {f: open(f, encoding='utf-8').read() for f in files}
defs = {}
for f, t in texts.items():
    for i in REQ_DEF.findall(t):
        if i in defs:
            problems.append('duplicate R-AUD-%s in %s and %s' % (i, defs[i], f))
        defs[i] = f
        if not (1 <= int(i) <= 499):
            problems.append('out of range R-AUD-%s' % i)
for f, t in texts.items():
    for i in set(REQ_REF.findall(t)):
        if i not in defs:
            problems.append('%s cites undefined R-AUD-%s' % (os.path.basename(f), i))
# keys
own = {}
for f, t in texts.items():
    for k in KEY_DEF.findall(t):
        if k in own:
            problems.append('duplicate key %s' % k)
        own[k] = f
alltext = '\n'.join(texts.values())
req_lines = [l for l in alltext.split('\n') if l.startswith('**R-AUD-')]
# a requirement "cites" a key when the key appears in its paragraph (the line and the table after it)
paras = re.split(r'\n(?=\*\*R-AUD-|#)', alltext)
req_paras = [p for p in paras if p.startswith('**R-AUD-')]
for k in own:
    if not any('`%s`' % k in p for p in req_paras):
        problems.append('own key not cited by a requirement: %s' % k)
# keys of other documents
others = set()
for dp, _d, names in os.walk(os.path.join(root, 'reference')):
    for n in names:
        if n.endswith('.md') and not dp.endswith('10_audio_design'):
            others.update(KEY_DEF.findall(open(os.path.join(dp, n), encoding='utf-8').read()))
for k in set(KEY_REF.findall(alltext)):
    if k not in own and k not in others:
        problems.append('key not defined anywhere: %s' % k)
# DEC, ADR
dec_index = open(os.path.join(root, 'decisions', 'README.md'), encoding='utf-8').read()
decs = set(re.findall(r'\| DEC-(\d{3}) \|', dec_index))
for d in set(DEC_REF.findall(alltext)):
    if d not in decs:
        problems.append('unknown DEC-%s' % d)
adrs = set(re.findall(r'^(\d{4})-', '\n'.join(os.listdir(os.path.join(root, 'adr'))), re.M))
for a in set(ADR_REF.findall(alltext)):
    if a not in adrs:
        problems.append('unknown ADR-%s' % a)
for d in OPEN:
    if not any('DEC-%s' % d in p for p in req_paras):
        problems.append('Open DEC-%s not cited by a requirement' % d)
for d in RESOLVED:
    if 'DEC-%s' % d not in alltext:
        problems.append('Resolved DEC-%s not cited' % d)
# assets
man = set(r['asset_id'] for r in csv.DictReader(open(os.path.join(root, 'assets', 'asset_manifest.csv'), newline='', encoding='utf-8')))
for a in set(ASSET_REF.findall(alltext)):
    if a not in man:
        problems.append('asset not in manifest: %s' % a)
# headings and truncation
for f, t in texts.items():
    if not f.endswith('README.md'):
        for h in HEADINGS:
            if h not in t:
                problems.append('%s lacks heading %s' % (os.path.basename(f), h))
    body = re.sub(r'```[\s\S]*?```', '', t)
    for m in TRUNC.finditer(body):
        problems.append('%s truncation word: %r' % (os.path.basename(f), m.group(0)))
    if len(t.encode('utf-8')) > 120 * 1024:
        problems.append('%s over 120 KB' % f)
print('requirements', len(defs), 'own keys', len(own), 'diagrams', alltext.count('```mermaid'))
for p in problems:
    print('PROBLEM', p)
print('problems', len(problems))
