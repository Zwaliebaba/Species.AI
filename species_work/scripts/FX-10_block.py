"""FX-10: extract or replace one fenced code block of a Markdown file.

Usage:
    python3 -I FX-10_block.py extract <md file> <n> <out file>     # n counts ```cpp blocks from 1
    python3 -I FX-10_block.py replace <md file> <n> <in file>
The fence lines are kept; only the body between them is read or replaced.
"""
import sys

mode, md, n, other = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
lines = open(md, encoding='utf-8').read().split('\n')
starts = [i for i, x in enumerate(lines) if x.strip() == '```cpp']
if n > len(starts):
    raise SystemExit(f'only {len(starts)} cpp blocks')
s = starts[n - 1]
e = next(i for i in range(s + 1, len(lines)) if lines[i].strip() == '```')
if mode == 'extract':
    with open(other, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines[s + 1:e]) + '\n')
else:
    body = open(other, encoding='utf-8').read().rstrip('\n').split('\n')
    lines[s + 1:e] = body
    with open(md, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines))
print(mode, md, n, 'lines', e - s - 1)
