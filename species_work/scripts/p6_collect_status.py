"""Phase 6: collect the fixers' status tables into species_work/fix_status.tsv.

Usage: python3 -I p6_collect_status.py <REPO>

Reads the `| Id | Status | Note |` table of every species_work/reports/P6-*.md and records, for each finding
id, the packet and its status (fixed, fixed-differently, rejected, request). Coordinator rows already in
fix_status.tsv are kept. Prints the ids of the packets' findings that no report mentions.
"""
import os
import re
import sys

REPO = sys.argv[1]
WORK = os.path.join(REPO, 'species_work')
CELL = re.compile(r'(?<!\\)\|')
ID = re.compile(r'^R\d[ab]?-\d+$')


def cells(line):
    return [c.strip() for c in CELL.split(line.strip())][1:-1]


status = {}
path = os.path.join(WORK, 'fix_status.tsv')
for line in open(path, encoding='utf-8').read().split('\n')[1:]:
    if line.strip():
        fid, st, *note = line.split('\t')
        status[fid] = (st, note[0] if note else '')

for name in sorted(os.listdir(os.path.join(WORK, 'reports'))):
    if not re.match(r'^P6-[A-Z0-9]+\.md$', name):
        continue
    pid = name[:-3]
    head = None
    for line in open(os.path.join(WORK, 'reports', name), encoding='utf-8').read().split('\n'):
        if not line.startswith('|'):
            head = None
            continue
        c = cells(line)
        if c and c[0].lower() == 'id' and len(c) >= 2 and c[1].lower() == 'status':
            head = c
            continue
        if head and c and ID.match(c[0].strip('*` ')):
            fid = c[0].strip('*` ')
            st = c[1].strip('*` ').lower().split()[0] if c[1].strip() else ''
            if status.get(fid, ('', ''))[1].startswith('coordinator'):
                continue
            status[fid] = (st, pid)

with open(path, 'w', encoding='utf-8') as fh:
    fh.write('id\tstatus\tnote\n')
    for fid in sorted(status, key=lambda f: (f.split('-')[0], int(f.split('-')[1]))):
        st, note = status[fid]
        fh.write(f'{fid}\t{st}\t{note}\n')

packet_ids = set()
for name in os.listdir(os.path.join(WORK, 'packets')):
    for line in open(os.path.join(WORK, 'packets', name), encoding='utf-8').read().split('\n'):
        c = cells(line) if line.startswith('| ') else []
        if c and ID.match(c[0]):
            packet_ids.add((c[0], name[:-3]))
missing = sorted(f'{fid} ({pid})' for fid, pid in packet_ids if fid not in status)
counts = {}
for st, _ in status.values():
    counts[st] = counts.get(st, 0) + 1
print('statuses:', counts)
print(f'{len(missing)} packet findings with no status:', ', '.join(missing[:60]))
