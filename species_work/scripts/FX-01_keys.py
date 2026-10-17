"""FX-01 helper: list tuning-key rows of the design root.

Usage: python3 -I FX-01_keys.py <design_root> <out.tsv>
Writes key, file, line, value, unit, notes, requirement, dec (tab separated).
"""
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
rows = []
for dp, dn, fn in os.walk(root):
    for n in fn:
        if not n.endswith('.md'):
            continue
        p = os.path.join(dp, n)
        lines = open(p, encoding='utf-8').read().split('\n')
        cols = None
        for i, line in enumerate(lines):
            if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
                head = [c.strip().lower() for c in line.strip().strip('|').split('|')]
                cols = head if head[:2] == ['key', 'value'] else None
                continue
            if cols is None:
                continue
            if not line.startswith('|'):
                cols = None
                continue
            if re.match(r'^\|\s*:?-{3,}', line):
                continue
            cells = [c.strip() for c in line.strip().strip('|').split('|')]
            m = re.match(r'^`([^`]+)`$', cells[0])
            if m:
                rows.append([m.group(1), os.path.relpath(p, root), str(i + 1)] + cells[1:7])
with open(out, 'w', encoding='utf-8') as f:
    for r in sorted(rows):
        f.write('\t'.join(r) + '\n')
print(len(rows), 'rows')
