#!/usr/bin/env python3
"""C-PLT: turn the symbolic requirement tokens of the platform-layer draft into PLT ids.

Usage: python3 -I C-PLT_number.py <draft.md> <output.md>

A definition is written **{{name}}** and a reference {{name}}. Definitions are numbered
PLT-001 upwards in the order in which they appear. Every reference must have a definition,
and every definition must be unique.
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
text = open(src, encoding='utf-8').read()
defs = re.findall(r'\*\*\{\{([a-z0-9_]+)\}\}\*\*', text)
dup = sorted({d for d in defs if defs.count(d) > 1})
if dup:
    sys.exit(f'duplicate definitions: {dup}')
ids = {name: f'PLT-{i:03d}' for i, name in enumerate(defs, start=1)}
if len(ids) > 299:
    sys.exit('more than 299 requirements')
missing = sorted({r for r in re.findall(r'\{\{([a-z0-9_]+)\}\}', text) if r not in ids})
if missing:
    sys.exit(f'references without a definition: {missing}')
out = re.sub(r'\{\{([a-z0-9_]+)\}\}', lambda m: ids[m.group(1)], text)
open(dst, 'w', encoding='utf-8').write(out)
print(f'{len(ids)} requirements numbered PLT-001 to PLT-{len(ids):03d}')
