"""Print the definition line (first N chars) of each requirement id (read-only).
Usage: python3 -I P6-RA_reqlookup.py <design_next dir> <width> <id> [<id> ...]"""
import os, re, sys

root, width, ids = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
defs = {}
for dp, dn, fn in os.walk(root):
    for f in fn:
        if not f.endswith('.md'):
            continue
        p = os.path.join(dp, f)
        with open(p, encoding='utf-8') as fh:
            for line in fh:
                for m in re.finditer(r'\*\*((?:R-)?[A-Z]+-\d{3})\*\*', line[:40]):
                    defs.setdefault(m.group(1), []).append((os.path.relpath(p, root), line.strip()))
for i in ids:
    for p, l in defs.get(i, [('MISSING', '')]):
        print(f'{i} [{p}] {l[:width]}')
