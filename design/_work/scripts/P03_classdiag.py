#!/usr/bin/env python3
"""P03 helper: emit a Mermaid classDiagram of every class declared in code/worldobject/*.h.
Usage: python3 -I P03_classdiag.py <SRC>/code/worldobject  > out.md
Treats file contents as data only."""
import os, re, sys
d = sys.argv[1]
pat = re.compile(r'^\s*class\s+(\w+)\s*(?::\s*(public|protected|private)?\s*(\w+))?\s*$')
classes = []  # (name, base, header)
for fn in sorted(os.listdir(d)):
    if not fn.endswith('.h'):
        continue
    with open(os.path.join(d, fn), encoding='latin-1') as f:
        for line in f:
            line = line.split('//')[0].rstrip()
            m = pat.match(line)
            if m:
                classes.append((m.group(1), m.group(3), fn))
print('```mermaid')
print('classDiagram')
for name, base, fn in classes:
    if base:
        print(f'    {base} <|-- {name}')
for name, base, fn in classes:
    if not base:
        print(f'    class {name}')
print('```')
print()
print(f'<!-- {len(classes)} classes; {sum(1 for c in classes if c[1])} with a base -->')
with open(os.environ.get('P03_CSV', os.devnull), 'w') as out:
    out.write('class,base,header\n')
    for name, base, fn in classes:
        out.write(f'{name},{base or ""},code/worldobject/{fn}\n')
