#!/usr/bin/env python3
"""F1: list the rows of ORIG/90 §3 and the §2 rows whose IP column is "yes → …".

Usage: python3 -I F1_glossary_rows.py <ORIG/90_glossary.md> <out.tsv>

Output columns: section, original, kind, placeholder, note
(section is "3.2" … "3.8" for §3 rows, "2" for §2 IP rows).
Inputs are data only.
"""
import csv
import re
import sys


def cells(line):
    s = line.strip()
    if s.startswith('|'):
        s = s[1:]
    if s.endswith('|'):
        s = s[:-1]
    return [c.strip() for c in s.split(' | ')]


def main():
    src, out = sys.argv[1], sys.argv[2]
    rows = []
    section = None
    in2 = False
    with open(src, encoding='utf-8') as fh:
        for line in fh:
            line = line.rstrip('\n')
            if line.startswith('## 2 '):
                in2, section = True, '2'
                continue
            if line.startswith('## 3 '):
                in2 = False
                continue
            m = re.match(r'### (3\.\d) ', line)
            if m:
                section = m.group(1)
                continue
            if line.startswith('## ') and not line.startswith('## 2') and not line.startswith('## 3'):
                section = None
                continue
            if not section or not line.startswith('|') or line.startswith('|---'):
                continue
            c = cells(line)
            if c[0] in ('Term', 'Original term', 'Kind', 'Column', 'Tag'):
                continue
            if section == '2':
                if len(c) >= 5 and c[4].startswith('yes'):
                    ph = re.sub(r'^yes\s*→\s*', '', c[4])
                    rows.append(['2', c[0], c[1], ph, ''])
            elif section.startswith('3.') and section != '3.1' and len(c) >= 5:
                rows.append([section, c[0], c[1], c[2], c[4]])
    with open(out, 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['section', 'original', 'kind', 'placeholder', 'note'])
        wr.writerows(rows)
    print(len([r for r in rows if r[0] != '2']), 'section-3 rows;', len([r for r in rows if r[0] == '2']), 'section-2 IP rows')


if __name__ == '__main__':
    main()
