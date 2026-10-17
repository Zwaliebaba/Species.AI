"""Print heading rows (level <= maxlevel) for one ORIG file from headings.csv.

Usage: python3 -I F2b2_heads.py <headings.csv> <file> [maxlevel] [prefix]
"""
import csv, sys
path, fname = sys.argv[1], sys.argv[2]
maxl = int(sys.argv[3]) if len(sys.argv) > 3 else 2
prefix = sys.argv[4] if len(sys.argv) > 4 else ''
with open(path, encoding='utf-8') as f:
    for r in csv.DictReader(f):
        if r['file'] != fname or int(r['level']) > maxl:
            continue
        if prefix and not r['heading'].startswith(prefix) and int(r['level']) > 2:
            continue
        print(f"{'  '*(int(r['level'])-1)}H{r['level']} {r['start_line']}-{r['end_line']} {r['bytes']:>7} | {r['heading'][:110]}")
