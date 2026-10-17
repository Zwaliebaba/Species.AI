"""C-TER self-check of architecture/components/terrain.md.

Usage: python3 -I C-TER_check.py <REPO>
Checks requirement ids (range, uniqueness, order), tuning keys (unique, defined once,
each used by a requirement, every used ter.* key defined), cited DEC/ADR/asset ids,
cited ref./rnd. keys and R-*/RND-* ids, the Open entries of decs_to_cite, template
headings, truncation words, relative links, Mermaid blocks and size.
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
PATH = os.path.join(NEXT, 'architecture', 'components', 'terrain.md')
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-TER.json'), encoding='utf-8'))
txt = open(PATH, encoding='utf-8').read()
problems = []

size = os.path.getsize(PATH)
print('size', size, 'bytes')
if size > 120 * 1000:
    problems.append('file above 120 000 bytes')

# requirement ids
ids = re.findall(r'^\*\*(TER-(\d{3}))\*\* \((MUST|MUST NOT|SHOULD|SHOULD NOT|MAY)\)', txt, re.M)
nums = [int(n) for _, n, _ in ids]
print('requirements', len(ids))
if len(set(nums)) != len(nums):
    problems.append('duplicate requirement ids')
if nums != sorted(nums):
    problems.append('requirement ids not ascending')
if any(n < 1 or n > 399 for n in nums):
    problems.append('requirement id out of range')
defined = {i for i, _, _ in ids}
cited = set(re.findall(r'\bTER-\d{3}\b', txt))
missing = sorted(cited - defined)
if missing:
    problems.append('cited TER ids not defined: %s' % missing)

# tuning table
rows = re.findall(r'^\| `(ter\.[a-z0-9_.]+)` \|', txt, re.M)
print('tuning keys', len(rows))
if len(rows) != len(set(rows)):
    problems.append('duplicate tuning keys')
body = re.sub(r'^\| `ter\.[^\n]*$', '', txt, flags=re.M)
used = set(re.findall(r'`(ter\.[a-z0-9_.]+)`', body)) | set(re.findall(r'\b(ter\.[a-z0-9_]+(?:\.[a-z0-9_]+)+)', body))
unused = sorted(set(rows) - used)
undefined = sorted(used - set(rows))
if unused:
    problems.append('keys defined but never used: %s' % unused)
if undefined:
    problems.append('ter keys used but not defined: %s' % undefined)
# keys of other owners must not be defined here
for k in re.findall(r'^\| `((?:ref|rnd)\.[a-z0-9_.]+)` \|', txt, re.M):
    problems.append('foreign key defined here: %s' % k)

# other keys cited must exist somewhere in design_next
foreign = set(re.findall(r'`((?:ref|rnd)\.[a-z0-9_]+(?:\.[a-z0-9_]+)+)`', txt))
defs = set()
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`((?:ref|rnd)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            defs.add(m.group(1))
for k in sorted(foreign):
    if k not in defs:
        problems.append('cited key not defined anywhere: %s' % k)

# DEC, ADR, asset ids
dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
for d in sorted(set(re.findall(r'\bDEC-\d{3}\b', txt))):
    if d not in dec_status:
        problems.append('unknown DEC %s' % d)
adr = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
for a in sorted(set(re.findall(r'\bADR-\d{4}\b', txt))):
    if a not in adr:
        problems.append('unknown ADR %s' % a)
assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])
for a in sorted(set(re.findall(r'`((?:terrain|fx|mesh|sprite|ui|sfx|map|thumb)\.[a-z0-9_.]+)`', txt))):
    if a not in assets:
        problems.append('unknown asset %s' % a)
for a in pkg['assets_for_you']:
    if a not in txt:
        problems.append('asset for me not cited: %s' % a)

# decs_to_cite: Open entries cited by a requirement paragraph
req_paras = re.findall(r'^\*\*TER-\d{3}\*\*[^\n]*(?:\n(?!\*\*TER-|#)[^\n]*)*', txt, re.M)
req_text = '\n'.join(req_paras)
for entry in pkg['decs_to_cite']:
    d, st = re.match(r'(DEC-\d{3}) \((\w+)\)', entry).groups()
    if d not in txt:
        problems.append('decs_to_cite not cited: %s' % d)
    elif st == 'Open' and d not in req_text:
        problems.append('Open DEC not cited by a requirement: %s' % d)

# R-* and RND-* ids
allref = ''
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    if os.path.abspath(f) != os.path.abspath(PATH):
        allref += open(f, encoding='utf-8').read()
for r in sorted(set(re.findall(r'\b(R-[A-Z]+-\d{3}|RND-\d{3})\b', txt))):
    if not re.search(r'\*\*' + re.escape(r) + r'\*\*', allref):
        problems.append('cited id not defined elsewhere: %s' % r)

# headings
for h in ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
          '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
          '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests',
          '## 8 Open decisions']:
    if h not in txt:
        problems.append('missing heading %s' % h)
if 'Design-phase inputs' not in txt:
    problems.append('no Design-phase inputs')

# truncation words
for w in [r'\betc\.', r'and so on', r'and others', r'\bvarious\b', r'similar to the above', '…']:
    for m in re.finditer(w, txt):
        line = txt[:m.start()].count('\n') + 1
        problems.append('truncation word %r at line %d' % (w, line))

# links
planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(os.path.normpath(os.path.join(NEXT, tgt.rstrip('/'))))
for link in re.findall(r'\]\(([^)#]+)(?:#[^)]*)?\)', txt):
    if link.startswith('http'):
        continue
    p = os.path.normpath(os.path.join(os.path.dirname(PATH), link))
    if not os.path.exists(p) and p not in planned and p.replace('/README.md', '') not in planned:
        problems.append('link does not resolve: %s' % link)

# mermaid
blocks = re.findall(r'```mermaid\n(.*?)```', txt, re.S)
print('mermaid blocks', len(blocks))
for b in blocks:
    end = txt.index(b) + len(b) + 3
    after = txt[end:end + 200].lstrip('\n')
    if not after.startswith('Caption:'):
        problems.append('mermaid block without caption')

print('tables', len(re.findall(r'^Table \d', txt, re.M)))
print('problems', len(problems))
for p in problems:
    print(' -', p)
