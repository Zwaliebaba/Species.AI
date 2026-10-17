"""Summarise a routing CSV: counts per action, KEEP+MERGE bytes per owner package (and per file and target section).

Usage: python3 -I F2b1_sizes.py <routing.csv>
"""
import csv
import sys
from collections import Counter, defaultdict

rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
acts = Counter(r['action'] for r in rows)
print('ACTIONS', dict(sorted(acts.items())), 'total', len(rows))
abytes = defaultdict(int)
for r in rows:
    abytes[r['action']] += int(r['bytes'])
print('ACTION BYTES', {k: round(v / 1024, 1) for k, v in sorted(abytes.items())})
pk = defaultdict(int)
pkf = defaultdict(lambda: defaultdict(int))
pks = defaultdict(lambda: defaultdict(int))
for r in rows:
    if r['action'] in ('KEEP', 'MERGE', 'KEEP-HOOKS'):
        b = int(r['bytes'])
        pk[(r['owner_package'], r['action'] == 'KEEP-HOOKS')] += b
        pkf[r['owner_package']][r['orig_file'][:2]] += b
        pks[r['owner_package']][(r['target_file'], r['target_section'])] += b
for (p, hooks), b in sorted(pk.items()):
    print('PKG', p, 'hooks' if hooks else 'keep+merge', round(b / 1024, 1), 'KB', {k: round(v / 1024, 1) for k, v in pkf[p].items()})
for p in sorted(pks):
    print('==', p)
    for (t, s), b in sorted(pks[p].items(), key=lambda x: -x[1]):
        print('   ', round(b / 1024, 1), t, '|', s)
ta = sum(int(r['bytes']) for r in rows if r['action'] == 'TO-ASSETS')
print('TO-ASSETS bytes', round(ta / 1024, 1))
