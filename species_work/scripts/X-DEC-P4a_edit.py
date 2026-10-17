"""Apply exact old->new replacements to a file; every old text must occur exactly once.
Usage: python3 -I X-DEC-P4a_edit.py <file> <edits.json>
edits.json: a list of [old, new] pairs."""
import json, sys
p, e = sys.argv[1], sys.argv[2]
t = open(p, encoding='utf-8').read()
b0 = len(t.encode())
for old, new in json.load(open(e, encoding='utf-8')):
    n = t.count(old)
    if n != 1:
        sys.exit(f'{p}: expected 1 occurrence, found {n}: {old[:90]!r}')
    t = t.replace(old, new)
open(p, 'w', encoding='utf-8').write(t)
print(p, b0, '->', len(t.encode()), f'({len(t.encode()) - b0:+d})')
