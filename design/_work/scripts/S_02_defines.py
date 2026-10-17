"""S02: list every value #define in the entity source files covered by 02_units_and_entities.md.
Usage: python3 -I S_02_defines.py <multiwinia_src_root>
Prints a Markdown table: | Constant | Value | File |
"""
import os, re, sys
root = sys.argv[1]
files = ['worldobject','entity','darwinian','officer','spirit','entity_leg','insertion_squad','armour',
         'harvester','engineer','airstrike','nuke','armyant','triffid','egg','virii','sporegenerator',
         'centipede','spider','souldestroyer','meteor','spaceship','ai','tank','lasertrooper','shaman',
         'dropship','tripod','tentacle','lander']
rx = re.compile(r'^\s*#define\s+([A-Z_][A-Z0-9_]*)\s+([^/\n]+?)\s*(//.*)?$')
rows = []
for f in files:
    for ext in ('.h', '.cpp'):
        p = os.path.join(root, 'code', 'worldobject', f + ext)
        if not os.path.exists(p):
            continue
        for line in open(p, encoding='latin-1'):
            m = rx.match(line)
            if not m:
                continue
            name, val = m.group(1), m.group(2).strip()
            if name.startswith('_included') or name.endswith('_H') or name.startswith('INCLUDED'):
                continue
            rows.append((name, val, 'code/worldobject/' + f + ext))
print('| Constant | Value | File |')
print('|---|---|---|')
for r in rows:
    print('| `%s` | %s | `%s` |' % r)
