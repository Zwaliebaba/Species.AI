"""Package sizing for routing_F2a2.csv.

Usage: python3 -I F2a2_sizes.py <routing.csv> <headings.csv>
Prints, per owner_package: KEEP+MERGE bytes, KEEP-HOOKS bytes, and a per-H2 breakdown;
then counts per action, and lists TO-DEC / DEFER-DESIGN rows.
"""
import csv
import sys
from collections import defaultdict


def main():
    rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
    heads = [h for h in csv.DictReader(open(sys.argv[2], encoding='utf-8')) if h['level'] == '2']

    def h2_of(f, line):
        best = None
        for h in heads:
            if h['file'] == f and int(h['start_line']) <= line <= int(h['end_line']):
                best = h
        return (f[:2] + ' ' + best['heading'].split(' ', 1)[0]) if best else f[:2] + ' H1'

    km, kh = defaultdict(int), defaultdict(int)
    parts = defaultdict(lambda: defaultdict(int))
    for r in rows:
        p, a, b = r['owner_package'], r['action'], int(r['bytes'])
        if a in ('KEEP', 'MERGE'):
            km[p] += b
        elif a == 'KEEP-HOOKS':
            kh[p] += b
        else:
            continue
        parts[p][h2_of(r['orig_file'], int(r['start_line'])) + ('(H)' if a == 'KEEP-HOOKS' else '')] += b
    print('package | KEEP+MERGE KB | KEEP-HOOKS KB | total KB')
    for p in sorted(set(km) | set(kh)):
        print(f'{p} | {km[p] / 1024:.1f} | {kh[p] / 1024:.1f} | {(km[p] + kh[p]) / 1024:.1f}')
    print()
    for p in sorted(parts):
        print(p + ': ' + ', '.join(f'{k}={v / 1024:.1f}' for k, v in parts[p].items()))
    print()
    acts = defaultdict(int)
    for r in rows:
        acts[r['action']] += 1
    print('actions: ' + ', '.join(f'{a}={n}' for a, n in sorted(acts.items())))
    perfile = defaultdict(lambda: defaultdict(int))
    for r in rows:
        perfile[r['orig_file'][:2]][r['action']] += 1
    for f in sorted(perfile):
        print(f + ': ' + ', '.join(f'{a}={n}' for a, n in sorted(perfile[f].items())))
    print()
    for r in rows:
        if r['action'] in ('TO-DEC', 'DEFER-DESIGN', 'TO-ADR'):
            print(f"{r['action']} | {r['orig_file'][:2]} {r['anchor'][:40]} {r['start_line']}-{r['end_line']} | {r['note'][:150]}")


if __name__ == '__main__':
    main()
