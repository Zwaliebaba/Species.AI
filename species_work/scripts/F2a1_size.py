"""Summarise routing_F2a1.csv: action counts and bytes, KEEP+MERGE bytes per package and per target file.
Usage: python3 -I F2a1_size.py <routing.csv>"""
import csv, sys
from collections import defaultdict
rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
act_n, act_b = defaultdict(int), defaultdict(int)
pkg, tgt = defaultdict(int), defaultdict(int)
for r in rows:
    act_n[r['action']] += 1; act_b[r['action']] += int(r['bytes'])
    if r['action'] in ('KEEP', 'MERGE', 'KEEP-HOOKS'):
        pkg[r['owner_package']] += int(r['bytes'])
        tgt[(r['owner_package'], r['target_file'])] += int(r['bytes'])
print('ACTIONS'); [print(f'  {a:16} {act_n[a]:4} rows {act_b[a]/1024:8.1f} KB') for a in sorted(act_n)]
print('PACKAGES (KEEP+MERGE+KEEP-HOOKS)'); [print(f'  {p:8} {b/1024:8.1f} KB') for p, b in sorted(pkg.items())]
print('TARGETS'); [print(f'  {p:8} {t:70} {b/1024:7.1f} KB') for (p, t), b in sorted(tgt.items())]
print('by orig/package')
op = defaultdict(int)
for r in rows:
    if r['action'] in ('KEEP', 'MERGE', 'KEEP-HOOKS'):
        op[(r['orig_file'][:2], r['owner_package'])] += int(r['bytes'])
[print(f'  {o} {p:8} {b/1024:7.1f} KB') for (o, p), b in sorted(op.items())]
