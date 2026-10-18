"""Map every tuning key defined in a tuning table to its requirement column (read-only).
Usage: python3 -I P6-RA_keymap.py <design_next dir> <key> [<key> ...]
With no keys, prints nothing. Prints: key | file | value | requirement column."""
import os, re, sys

root = sys.argv[1]
keys = sys.argv[2:]
rows = {}
for dp, dn, fn in os.walk(root):
    for f in fn:
        if not f.endswith('.md'):
            continue
        p = os.path.join(dp, f)
        with open(p, encoding='utf-8') as fh:
            for line in fh:
                if not line.startswith('|'):
                    continue
                cells = [c.strip() for c in line.strip().strip('|').split('|')]
                if len(cells) < 5:
                    continue
                m = re.fullmatch(r'`([a-z0-9_.]+)`', cells[0])
                if not m:
                    continue
                k = m.group(1)
                rows.setdefault(k, []).append((os.path.relpath(p, root), cells[1], cells[4] if len(cells) > 4 else ''))
for k in keys:
    for r in rows.get(k, [('MISSING', '', '')]):
        print(f'{k} | {r[0]} | {r[1]} | {r[2]}')
