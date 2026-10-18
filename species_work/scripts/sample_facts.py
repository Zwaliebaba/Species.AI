"""Phase 5: sample the facts the fidelity reviewers check (prompt §8.1 Phase 5).

Usage: python3 -I sample_facts.py <REPO> [--seed 20261017] [--per-doc 20]

With the fixed seed, samples up to 20 lines per original document from its KEEP and MERGE routed
ranges, taking only lines that carry a number, a state transition or an enumeration member (a list
item or a table row). Writes species_work/fidelity_sample.csv (orig_file, line, text, routed_target).
"""
import csv
import os
import random
import re
import sys

REPO = sys.argv[1]
SEED = int(sys.argv[sys.argv.index('--seed') + 1]) if '--seed' in sys.argv else 20261017
PER = int(sys.argv[sys.argv.index('--per-doc') + 1]) if '--per-doc' in sys.argv else 20
WORK = os.path.join(REPO, 'species_work')
ORIG = os.path.join(REPO, 'design')
FACT = re.compile(r'\d|→|->|⇒|\btransition|\bbecomes\b|\bswitches to\b')
ENUM = re.compile(r'^\s*(?:[-*+]\s+\S|\d+\.\s+\S|\|(?!\s*:?-{3,}))')
SEP = re.compile(r'^\s*\|?\s*:?-{3,}')

rows = list(csv.DictReader(open(os.path.join(WORK, 'routing.csv'), encoding='utf-8', newline='')))
ranges = {}
for r in rows:
    if r['action'] in ('KEEP', 'MERGE'):
        ranges.setdefault(r['orig_file'], []).append((int(r['start_line']), int(r['end_line']), r['target_file']))

rng = random.Random(SEED)
out = []
for doc in sorted(ranges):
    path = os.path.join(ORIG, doc)
    if not os.path.exists(path):
        continue
    lines = open(path, encoding='utf-8').read().split('\n')
    cands, inside = [], False
    for lo, hi, target in sorted(ranges[doc]):
        for n in range(lo, min(hi, len(lines)) + 1):
            t = lines[n - 1]
            if t.lstrip().startswith(('```', '~~~')):
                inside = not inside
                continue
            if inside or t.lstrip().startswith('#') or SEP.match(t) or not t.strip():
                continue
            if FACT.search(t) or ENUM.match(t):
                cands.append((n, t.strip(), target))
    for n, t, target in sorted(rng.sample(cands, min(PER, len(cands)))):
        out.append({'orig_file': doc, 'line': n, 'text': t[:600], 'routed_target': target})

with open(os.path.join(WORK, 'fidelity_sample.csv'), 'w', encoding='utf-8', newline='') as f:
    w = csv.DictWriter(f, fieldnames=['orig_file', 'line', 'text', 'routed_target'], lineterminator='\n')
    w.writeheader()
    w.writerows(out)
print(f'{len(out)} facts from {len({o["orig_file"] for o in out})} documents (seed {SEED})')
