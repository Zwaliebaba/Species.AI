"""C-UI-b self-check of architecture/components/ui_framework/pages_and_screen_recipes.md.

Usage: python3 -I C-UI-b_check.py <REPO>
Checks requirement ids (range 300-499, unique, ascending), tuning keys (unique, each used,
every used own key defined, no ui.* key that a sibling file defines), cited foreign keys,
cited DEC/ADR/asset ids, cited R-*/RND-*/UI-* ids, the Open entries of decs_to_cite,
every assets_for_you id, template headings, truncation words, relative links,
Mermaid captions, string-id collisions of own keys, and size.
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
DIR = os.path.join(NEXT, 'architecture', 'components', 'ui_framework')
PATH = os.path.join(DIR, 'pages_and_screen_recipes.md')
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-UI-b.json'), encoding='utf-8'))
txt = open(PATH, encoding='utf-8').read()
problems = []

size = os.path.getsize(PATH)
print('size', size, 'bytes')
if size > 120 * 1000:
    problems.append('file above 120 000 bytes')

ids = re.findall(r'^\*\*(UI-(\d{3}))\*\* \((MUST|MUST NOT|SHOULD|SHOULD NOT|MAY)\)', txt, re.M)
nums = [int(n) for _, n, _ in ids]
print('requirements', len(ids))
if len(set(nums)) != len(nums):
    problems.append('duplicate requirement ids')
if nums != sorted(nums):
    problems.append('requirement ids not ascending')
if any(n < 300 or n > 499 for n in nums):
    problems.append('requirement id out of range')
defined = {i for i, _, _ in ids}

# sibling UI ids
sib = ''
for f in glob.glob(os.path.join(DIR, '*.md')):
    if os.path.abspath(f) != os.path.abspath(PATH):
        sib += open(f, encoding='utf-8').read()
sib_defined = set(re.findall(r'\*\*(UI-\d{3})\*\*', sib))
for c in sorted(set(re.findall(r'\bUI-\d{3}\b', txt))):
    n = int(c[3:])
    if 300 <= n <= 499:
        if c not in defined:
            problems.append('cited own UI id not defined: %s' % c)
    elif c not in sib_defined:
        problems.append('cited sibling UI id not (yet) defined: %s' % c)

# own tuning keys
rows = re.findall(r'^\| `(ui\.[a-z0-9_.]+)` \|', txt, re.M)
print('tuning keys', len(rows))
if len(rows) != len(set(rows)):
    problems.append('duplicate tuning keys')
body = re.sub(r'^\| `ui\.[^\n]*$', '', txt, flags=re.M)
own_prefixes = ('ui.pages.', 'ui.scroll_text.', 'ui.picker.', 'ui.map_row.', 'ui.chat_log.')
used = {k for k in re.findall(r'`(ui\.[a-z0-9_]+(?:\.[a-z0-9_]+)+)`', body) if k.startswith(own_prefixes)}
for k in sorted(set(rows) - used):
    problems.append('key defined but never used: %s' % k)
for k in sorted(used - set(rows)):
    problems.append('own-prefix key used but not defined: %s' % k)
# sibling-defined ui keys must not be redefined here
sib_keys = set(re.findall(r'^\|\s*`(ui\.[a-z0-9_.]+)`\s*\|', sib, re.M))
for k in rows:
    if k in sib_keys:
        problems.append('key also defined in a sibling file: %s' % k)
# own key prefix must not collide with an existing string id
alltxt = ''
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    if os.path.abspath(f) != os.path.abspath(PATH):
        alltxt += open(f, encoding='utf-8').read()
for p in own_prefixes:
    if re.search(r'`' + re.escape(p), alltxt):
        problems.append('own key prefix appears elsewhere: %s' % p)

# foreign keys cited must exist
foreign = set(re.findall(r'`((?:ref|rnd)\.[a-z0-9_]+(?:\.[a-z0-9_]+)+)`', txt))
defs = set()
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`((?:ref|rnd|ui)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            defs.add(m.group(1))
for k in sorted(foreign):
    if k not in defs:
        problems.append('cited key not defined anywhere: %s' % k)

dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
for d in sorted(set(re.findall(r'\bDEC-\d{3}\b', txt))):
    if d not in dec_status:
        problems.append('unknown DEC %s' % d)
# status claims in the §8 table
for d, st in re.findall(r'^\| (DEC-\d{3}) \| (Open|Resolved|Dropped) \|', txt, re.M):
    if dec_status.get(d) != st:
        problems.append('DEC status mismatch %s: %s vs %s' % (d, st, dec_status.get(d)))
for line in re.findall(r'^\| (DEC-\d{3}(?:, DEC-\d{3})+) \| (Open|Resolved|Dropped) \|', txt, re.M):
    for d in re.findall(r'DEC-\d{3}', line[0]):
        if dec_status.get(d) != line[1]:
            problems.append('DEC status mismatch %s: %s vs %s' % (d, line[1], dec_status.get(d)))
adr = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
for a in sorted(set(re.findall(r'\bADR-\d{4}\b', txt))):
    if a not in adr:
        problems.append('unknown ADR %s' % a)
assets = set()
with open(os.path.join(NEXT, 'assets', 'asset_manifest.csv'), encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])
pat = r'`((?:fx|sprite|sfx|amb|thumb|help|icon|glyph|cursor|anim|font|ui)\.[a-z0-9_.]+)`'
for a in sorted(set(re.findall(pat, txt))):
    if a.startswith('ui.') and (a.startswith(own_prefixes) or a in sib_keys):
        continue
    if a.startswith('ui.palette.') and a not in sib_keys:
        problems.append('sibling palette token not defined: %s' % a)
        continue
    if a not in assets and not a.startswith(('ui.debug', 'ui.common', 'ui.credits', 'ui.power_slots.')):
        problems.append('unknown asset (or string id) %s' % a)
for a in pkg['assets_for_you']:
    if '`%s`' % a not in txt:
        problems.append('asset for me not cited: %s' % a)

req_paras = re.findall(r'^\*\*UI-\d{3}\*\*[^\n]*(?:\n(?!\*\*UI-|#)[^\n]*)*', txt, re.M)
req_text = '\n'.join(req_paras)
for entry in pkg['decs_to_cite']:
    d, st = re.match(r'(DEC-\d{3}) \((\w+)\)', entry).groups()
    if d not in txt:
        problems.append('decs_to_cite not cited: %s' % d)
    elif st == 'Open' and d not in req_text:
        problems.append('Open DEC not cited by a requirement: %s' % d)

for r in sorted(set(re.findall(r'\b(R-[A-Z]+-\d{3}|RND-\d{3})\b', txt))):
    if not re.search(r'\*\*' + re.escape(r) + r'\*\*', alltxt):
        problems.append('cited id not defined elsewhere: %s' % r)

for h in ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
          '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
          '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests',
          '## 8 Open decisions']:
    if h not in txt:
        problems.append('missing heading %s' % h)
if 'Design-phase inputs' not in txt:
    problems.append('no Design-phase inputs')

for w in [r'\betc\.', r'and so on', r'and others', r'\bvarious\b', r'similar to the above', '…']:
    for m in re.finditer(w, txt):
        line = txt[:m.start()].count('\n') + 1
        problems.append('truncation word %r at line %d' % (w, line))

planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(os.path.normpath(os.path.join(NEXT, tgt.rstrip('/'))))
for link in sorted(set(re.findall(r'\]\(([^)#]+)(?:#[^)]*)?\)', txt))):
    if link.startswith('http'):
        continue
    p = os.path.normpath(os.path.join(os.path.dirname(PATH), link))
    if not os.path.exists(p) and p not in planned and p.replace('/README.md', '') not in planned:
        problems.append('link does not resolve: %s' % link)

blocks = re.findall(r'```mermaid\n(.*?)```', txt, re.S)
print('mermaid blocks', len(blocks))
pos = 0
for b in blocks:
    i = txt.index(b, pos)
    end = i + len(b) + 3
    pos = end
    after = txt[end:end + 200].lstrip('\n')
    if not after.startswith('Caption:'):
        problems.append('mermaid block without caption')

print('tables', len(re.findall(r'^\|[-| ]+\|$', txt, re.M)))
print('tests', len(re.findall(r'^\| (?:T|DT-)\d+ \|', txt, re.M)))
print('problems', len(problems))
for p in problems:
    print(' -', p)
