#!/usr/bin/env python3
"""F1: render the ORIG self-test hit counts as Markdown tables for WORK/reports/F1.md.

Usage: python3 -I F1_report_tables.py <hits_orig_forbidden.tsv> <hits_orig_soft.tsv> <out.md>
"""
import csv
import sys
from collections import Counter, defaultdict


def esc(s):
    return s.replace('|', '\\|').replace('`', "'")


def main():
    hard, soft, out = sys.argv[1:4]
    with open(hard, encoding='utf-8', newline='') as fh:
        h = list(csv.DictReader(fh, delimiter='\t'))
    with open(soft, encoding='utf-8', newline='') as fh:
        s = list(csv.DictReader(fh, delimiter='\t'))
    L = []
    tot = Counter()
    n = Counter()
    zero = defaultdict(int)
    for r in h:
        tot[r['class']] += int(r['hits'])
        n[r['class']] += 1
        if r['hits'] == '0':
            zero[r['class']] += 1
    L.append('### A.1 Hard-fail list: totals per class (ORIG documents 00–99 and README)')
    L.append('')
    L.append('| Class | Patterns | Patterns with zero hits | Hits in ORIG |')
    L.append('|---|---|---|---|')
    for c in sorted(n):
        L.append('| %s | %d | %d | %d |' % (c, n[c], zero[c], tot[c]))
    L.append('| **all** | **%d** | **%d** | **%d** |' % (sum(n.values()), sum(zero.values()), sum(tot.values())))
    L.append('')
    L.append('### A.2 Hard-fail list: hit count of every non-identifier pattern')
    L.append('')
    L.append('| Class | Pattern | Flags | Hits | Files |')
    L.append('|---|---|---|---|---|')
    for r in h:
        if r['class'] == 'identifier':
            continue
        L.append('| %s | `%s` | %s | %s | %s |' % (r['class'], esc(r['pattern'][:150]) + ('…' if len(r['pattern']) > 150 else ''),
                                                  r['flags'] or 'cs', r['hits'], r['files']))
    L.append('')
    ident = [r for r in h if r['class'] == 'identifier']
    buckets = Counter()
    for r in ident:
        k = int(r['hits'])
        b = '0' if k == 0 else '1' if k == 1 else '2–9' if k < 10 else '10–99' if k < 100 else '100+'
        buckets[b] += 1
    L.append('### A.3 Identifier patterns (%d): distribution of hits' % len(ident))
    L.append('')
    L.append('| Hits per pattern | Patterns |')
    L.append('|---|---|')
    for b in ('0', '1', '2–9', '10–99', '100+'):
        L.append('| %s | %d |' % (b, buckets[b]))
    L.append('')
    L.append('Top 15 identifier patterns: ' + '; '.join('%s = %s' % (esc(r['note'].split(' (')[0]), r['hits'])
                                                    for r in sorted(ident, key=lambda x: -int(x['hits']))[:15]) + '.')
    zi = [r['note'].split(' (')[0] for r in ident if r['hits'] == '0']
    L.append('')
    L.append('Identifier patterns with zero hits: ' + (', '.join(esc(z) for z in zi) if zi else 'none') + '.')
    L.append('')
    L.append('### A.4 Soft-flag list: hit count of every pattern')
    L.append('')
    L.append('| Class | Pattern | Flags | Hits | Files |')
    L.append('|---|---|---|---|---|')
    for r in s:
        L.append('| %s | `%s` | %s | %s | %s |' % (r['class'], esc(r['pattern']), r['flags'] or 'cs', r['hits'], r['files']))
    L.append('')
    with open(out, 'w', encoding='utf-8', newline='') as fh:
        fh.write('\n'.join(L) + '\n')
    print('tables written')


if __name__ == '__main__':
    main()
