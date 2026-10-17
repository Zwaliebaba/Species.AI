"""Prompt §9.0 acceptance check for one Phase 3 package.

Usage: python3 -I accept_package.py <REPO> <PACKAGE_ID>

Checks: targets exist and are <= 120 KB (GameAssets.md and CSVs exempt); template
headings (reference §6.3 / component §6.4; README and glossary files exempt);
requirement ids inside the package's range and unique; tuning table present in
reference documents; Open decs_to_cite cited somewhere in the targets; zero
hard-fail pattern hits; report present. Prints PASS or the failures.
"""
import json
import os
import re
import subprocess
import sys

REPO, PID = sys.argv[1], sys.argv[2]
NEXT = os.path.join(REPO, 'design_next')
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', PID + '.json'), encoding='utf-8'))
REF_H = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table',
         'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
CMP_H = ['Responsibilities and boundaries', 'Interfaces', 'Data model', 'Behaviour and algorithms',
         'Concurrency and performance budget', 'Failure modes, diagnostics and telemetry',
         'Acceptance tests', 'Open decisions']
fails, texts = [], {}

for t in pkg['targets']:
    path = os.path.join(NEXT, t)
    if t.endswith('/'):
        continue
    if not os.path.exists(path):
        fails.append(f'missing target {t}')
        continue
    size = os.path.getsize(path)
    if size > 120 * 1024 and not t.endswith(('GameAssets.md', '.csv')):
        fails.append(f'{t} is {size // 1024} KB (> 120 KB)')
    txt = open(path, encoding='utf-8').read()
    texts[t] = txt
    base = os.path.basename(t)
    if base == 'README.md' or t.startswith('glossary/') or t.startswith('assets/'):
        continue
    heads = REF_H if t.startswith('reference/') else CMP_H
    have = re.findall(r'^##\s+(?:\d+\s+)?(.+?)\s*$', txt, re.M)
    for h in heads:
        if not any(x.lower().startswith(h.lower()) for x in have):
            fails.append(f'{t}: template heading "{h}" not found')

# extra files the agent wrote inside its target folders (splits) are checked for size too
for folder in {os.path.dirname(t) for t in pkg['targets'] if '/' in t}:
    d = os.path.join(NEXT, folder)
    if os.path.isdir(d):
        for f in os.listdir(d):
            rel = f'{folder}/{f}'
            if f.endswith('.md') and rel not in texts and os.path.getsize(os.path.join(d, f)) > 120 * 1024:
                fails.append(f'{rel} (extra file) is over 120 KB')

alltext = '\n'.join(texts.values())
for prefix, rng in pkg.get('id_ranges', {}).items():
    if prefix in ('DEC', 'ADR'):
        continue
    lo, hi = (int(x) for x in rng.split('-'))
    ids = re.findall(r'\*\*' + re.escape(prefix) + r'-(\d{3})\*\*', alltext)
    if not ids:
        fails.append(f'no {prefix} requirements found')
    seen = set()
    for i in ids:
        n = int(i)
        if not lo <= n <= hi:
            fails.append(f'{prefix}-{i} outside range {rng}')
        if n in seen:
            fails.append(f'{prefix}-{i} defined twice')
        seen.add(n)
for d in pkg.get('decs_to_cite', []):
    dec, status = d.split(' ')[0], d.split('(')[-1].rstrip(')')
    if status == 'Open' and dec not in alltext:
        fails.append(f'Open {dec} not cited')
pats = os.path.join(REPO, 'species_work', 'forbidden_patterns.tsv')
scan = os.path.join(REPO, 'species_work', 'scripts', 'scan_patterns.py')
files = [os.path.join(NEXT, t) for t in texts]
if files:
    out = subprocess.run([sys.executable, '-I', scan, pats] + files, capture_output=True, text=True).stdout.strip()
    if out:
        fails.append(f'hard-fail hits: {len(out.splitlines())} (first: {out.splitlines()[0][:120]})')
if not os.path.exists(os.path.join(REPO, 'species_work', 'reports', PID + '.md')):
    fails.append('report missing')
print(f'{PID}: ' + ('PASS' if not fails else 'FAIL'))
for f in fails:
    print('  -', f)
sys.exit(1 if fails else 0)
