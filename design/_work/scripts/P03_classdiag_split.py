#!/usr/bin/env python3
"""P03 helper: split the class CSV (from P03_classdiag.py) into three Mermaid diagrams.
Usage: python3 -I P03_classdiag_split.py <classes.csv>"""
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1])))
base = {r['class']: r['base'] for r in rows}
def root(c):
    seen = set()
    while base.get(c) and c not in seen:
        seen.add(c)
        if base[c] in ('Entity', 'Building', 'Unit'):
            return base[c]
        c = base[c]
    return base.get(c) or None
groups = {'Entity': [], 'Building': [], 'Other': []}
for r in rows:
    c, b = r['class'], r['base']
    if c in ('Entity', 'Building'):
        groups['Other'].append(r); continue
    rt = root(c)
    if rt == 'Entity': groups['Entity'].append(r)
    elif rt == 'Building': groups['Building'].append(r)
    else: groups['Other'].append(r)
titles = {'Entity': 'Entity subclasses (WorldObject → Entity → …)',
          'Building': 'Building subclasses (WorldObject → Building → …)',
          'Other': 'WorldObject direct children (effects, weapons, spirits), Unit subclasses, and standalone helper classes'}
for g in ('Other', 'Entity', 'Building'):
    print(f'#### {titles[g]} — {len(groups[g])} classes')
    print('```mermaid')
    print('classDiagram')
    if g == 'Entity': print('    WorldObject <|-- Entity')
    if g == 'Building': print('    WorldObject <|-- Building')
    for r in groups[g]:
        if r['base']:
            print(f"    {r['base']} <|-- {r['class']}")
    for r in groups[g]:
        if not r['base']:
            print(f"    class {r['class']}")
    print('```')
    print()
