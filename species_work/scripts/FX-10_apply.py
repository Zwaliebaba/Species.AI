"""FX-10: apply exact, unique-match replacements from a spec file.

Usage: python3 -I FX-10_apply.py <root dir> <spec file>

Spec format (plain text):
    @@@ <path relative to root>
    <<<
    old text (one or more lines)
    ===
    new text (zero or more lines)
    >>>
Each old text must occur exactly once in its file at the time it is applied. Edits of one file apply in
order; a file is written only when all its edits match, and nothing is written when any edit fails.
"""
import os
import sys

root, spec = sys.argv[1], sys.argv[2]
lines = open(spec, encoding='utf-8').read().split('\n')
edits = []  # (path, old, new)
path = None
i = 0
while i < len(lines):
    ln = lines[i]
    if ln.startswith('@@@ '):
        path = ln[4:].strip()
        i += 1
        continue
    if ln == '<<<':
        j = lines.index('===', i + 1)
        k = lines.index('>>>', j + 1)
        edits.append((path, '\n'.join(lines[i + 1:j]), '\n'.join(lines[j + 1:k])))
        i = k + 1
        continue
    i += 1

texts = {}
errors = []
for n, (p, old, new) in enumerate(edits, 1):
    full = os.path.join(root, p)
    if p not in texts:
        texts[p] = open(full, encoding='utf-8').read()
    c = texts[p].count(old)
    if c != 1:
        errors.append(f'edit {n} in {p}: {c} matches for {old[:80]!r}')
        continue
    texts[p] = texts[p].replace(old, new)
if errors:
    print('\n'.join(errors))
    sys.exit(1)
for p, t in texts.items():
    with open(os.path.join(root, p), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(t)
    print(f'{p}: {len(t.encode("utf-8"))} bytes')
print(f'{len(edits)} edits applied')
