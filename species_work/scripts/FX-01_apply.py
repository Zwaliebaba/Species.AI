"""FX-01 helper: apply exact old/new replacement pairs to a file.

Usage: python3 -I FX-01_apply.py <target.md> <pairs.txt>
pairs.txt holds blocks separated by lines '=== OLD', '=== NEW' and '=== END'.
Every OLD text must occur exactly once in the target; otherwise nothing is written.
"""
import sys

target, pairs = sys.argv[1], sys.argv[2]
text = open(target, encoding='utf-8').read()
raw = open(pairs, encoding='utf-8').read().split('\n')
blocks, cur, mode = [], None, None
for line in raw:
    if line == '=== OLD':
        cur, mode = {'old': [], 'new': []}, 'old'
    elif line == '=== NEW':
        mode = 'new'
    elif line == '=== END':
        blocks.append(('\n'.join(cur['old']), '\n'.join(cur['new'])))
        cur, mode = None, None
    elif mode:
        cur[mode].append(line)
bad = []
for old, new in blocks:
    n = text.count(old)
    if n != 1:
        bad.append(f'{n}x: {old[:100]}')
        continue
    text = text.replace(old, new)
if bad:
    sys.exit('not applied:\n' + '\n'.join(bad))
open(target, 'w', encoding='utf-8').write(text)
print(f'applied {len(blocks)} replacements')
