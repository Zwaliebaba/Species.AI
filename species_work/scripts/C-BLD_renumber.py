"""C-BLD: number requirement definitions in document order and rewrite every reference.

Usage: python3 -I C-BLD_renumber.py <file> [start]
Definitions are **BLD-NNN** or **BLD-@label@**. Every reference BLD-NNN or BLD-@label@
is rewritten through the same mapping. Fails on an undefined label or a duplicate definition.
"""
import re
import sys

path = sys.argv[1]
start = int(sys.argv[2]) if len(sys.argv) > 2 else 1
txt = open(path, encoding='utf-8').read()
defs = re.findall(r'\*\*BLD-(\d{3}|@[a-z0-9_]+@)\*\*', txt)
if len(defs) != len(set(defs)):
    sys.exit('duplicate definitions: ' + ', '.join(sorted({d for d in defs if defs.count(d) > 1})))
mapping = {d: f'{start + i:03d}' for i, d in enumerate(defs)}
missing = sorted(set(re.findall(r'BLD-(@[a-z0-9_]+@)', txt)) - set(mapping))
if missing:
    sys.exit('undefined labels: ' + ', '.join(missing))
undefined_num = sorted(set(re.findall(r'BLD-(\d{3})\b', txt)) - set(mapping))
if undefined_num:
    sys.exit('undefined numeric ids: ' + ', '.join(undefined_num))
out = re.sub(r'BLD-(\d{3}\b|@[a-z0-9_]+@)', lambda m: 'BLD-' + mapping[m.group(1)], txt)
open(path, 'w', encoding='utf-8').write(out)
print(f'{len(defs)} requirements numbered {start:03d} to {start + len(defs) - 1:03d}')
