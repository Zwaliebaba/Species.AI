"""X-DEC-R: apply the gate-G2 amendments to the decision register and the ADRs.

Usage: python3 -I X-DEC-R_apply.py <REPO> [--dry-run] [--compact-verdicts-index | --section-index]

Every edit is an exact (old, new) pair. An edit whose new text is already present counts as
applied before and is skipped; otherwise its old text must occur exactly once, and it is applied;
anything else is an error and nothing is written. Prints the size of every file touched.
"""
import importlib.util
import os
import re
import sys

REPO = sys.argv[1]
DRY = '--dry-run' in sys.argv
COMPACT = '--compact-verdicts-index' in sys.argv
SECTIONS = '--section-index' in sys.argv
HERE = os.path.dirname(os.path.abspath(__file__))
LIMIT = 120 * 1024


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(HERE, name + '.py'))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


V = load('X-DEC-R_edits_verdicts')
O = load('X-DEC-R_edits_other')

errors, log = [], []
texts = {}


def get(rel):
    if rel not in texts:
        texts[rel] = open(os.path.join(REPO, rel), encoding='utf-8').read()
    return texts[rel]


def apply(rel, tag, old, new):
    t = get(rel)
    if new in t:
        log.append(f'present  {tag}  ({os.path.basename(rel)})')
        return
    n_old = t.count(old)
    if n_old == 1:
        texts[rel] = t.replace(old, new)
        log.append(f'applied  {tag}  ({os.path.basename(rel)})')
    else:
        errors.append(f'{tag}: old text occurs {n_old} times in {rel}')


def slug(heading):
    return re.sub(r'[^\w\- ]', '', heading.lower()).replace(' ', '-')


# 1 verdicts file: edits, new section, index rows
for tag, old, new in V.EDITS:
    apply(V.F, tag, old, new)
t = get(V.F)
if '### DEC-118 ' not in t:
    if t.count(V.NEW_SECTION_ANCHOR) != 1:
        errors.append('verdicts: new-section anchor not unique')
    else:
        texts[V.F] = t.replace(V.NEW_SECTION_ANCHOR, V.NEW_SECTION_ANCHOR + V.NEW_SECTION)
        log.append('applied  A2/A4 new entries DEC-118, DEC-119')
t = get(V.F)
if SECTIONS:
    lines = t.split('\n')
    start = lines.index('## Index')
    end = next(i for i in range(start + 1, len(lines)) if lines[i].startswith('## '))
    secs = []
    for i, l in enumerate(lines):
        m = re.match(r'^## (\d+ .*)$', l)
        if not m:
            continue
        ids = []
        for l2 in lines[i + 1:]:
            if l2.startswith('## '):
                break
            m2 = re.match(r'^### (DEC-\d{3}) ', l2)
            if m2:
                ids.append(m2.group(1))
        rng = ids[0] if len(ids) == 1 else (', '.join(ids) if len(ids) == 2 else f'{ids[0]} to {ids[-1]}')
        secs.append(f'| [{m.group(1)}](#{slug(m.group(1))}) | {rng} | {len(ids)} |')
    block = ['## Index', '',
             'The generated [register index](README.md) lists every entry of this file with its title, status and link. The sections of this file:',
             '', '| Section | Entries | Count |', '|---|---|---|'] + secs + ['']
    if lines[start:end] != block:
        lines[start:end] = block
        log.append('applied  section index for the verdicts file (per-entry rows are in the generated register index)')
    t = '\n'.join(lines)
for dec, title, status, affects in ([] if SECTIONS else V.NEW_INDEX_ROWS):
    if re.search(rf'^\| \[?{dec}\b', t, re.M):
        continue
    lines = t.split('\n')
    last = max(i for i, l in enumerate(lines) if re.match(r'^\| \[?DEC-\d{3}', l))
    row = f'| [{dec}](#{slug(dec + " " + title)}) | {title} | {status} | {affects} |'
    lines.insert(last + 1, row)
    t = '\n'.join(lines)
    log.append(f'applied  index row {dec}')
texts[V.F] = t

if COMPACT:
    t = get(V.F)
    t2 = re.sub(r'^\| \[(DEC-\d{3})\]\(#[^)]+\) \|', r'| \1 |', t, flags=re.M)
    if t2 != t:
        log.append('applied  compact verdicts index (plain ids; the generated register index carries the links)')
    texts[V.F] = t2

# 2 other files
for rel, edits in O.EDITS.items():
    for tag, old, new in edits:
        apply(rel, tag, old, new)

for line in log:
    print(line)
for rel in sorted(texts):
    size = len(texts[rel].encode('utf-8'))
    flag = '  OVER 120 KB' if size > LIMIT else ''
    print(f'{size:8d}  {rel}{flag}')
for e in errors:
    print('ERROR', e)
if errors:
    sys.exit(1)
if not DRY:
    for rel, t in texts.items():
        open(os.path.join(REPO, rel), 'w', encoding='utf-8').write(t)
    print('written')
