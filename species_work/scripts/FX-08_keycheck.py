"""FX-08: check tuning-key citations.

Usage: python3 -I FX-08_keycheck.py <design_root> <file.md> [<file.md> ...] [--removed key1,key2,...]

Prints every backticked tuning key cited in the given files that no tuning table of the design
root defines, and, for --removed, every file of the design root that still cites a removed key.
"""
import glob
import os
import re
import sys

root = sys.argv[1]
args = sys.argv[2:]
removed = []
if '--removed' in args:
    i = args.index('--removed')
    removed = [k for k in args[i + 1].split(',') if k]
    args = args[:i] + args[i + 2:]

PREFIXES = ('ref', 'ter', 'rnd', 'ast', 'vfx', 'aen', 'ui', 'inp', 'cam', 'dat', 'plt', 'bld', 'dbg')
KEY = re.compile(r'`((?:' + '|'.join(PREFIXES) + r')\.[a-z0-9_]+(?:\.[a-z0-9_<>*]+)+)`')
ROW = re.compile(r'^\|\s*`([a-z0-9_.]+)`\s*\|')

defined = set()
texts = {}
for p in glob.glob(os.path.join(root, '**', '*.md'), recursive=True):
    t = open(p, encoding='utf-8').read()
    texts[p] = t
    head = None
    for line in t.split('\n'):
        if line.startswith('| key |'):
            head = True
            continue
        if head and line.startswith('|'):
            m = ROW.match(line)
            if m:
                defined.add(m.group(1))
        elif not line.startswith('|'):
            head = None

bad = 0
for f in args:
    t = open(f, encoding='utf-8').read()
    for n, line in enumerate(t.split('\n'), 1):
        for k in KEY.findall(line):
            if '*' in k or '<' in k or k.endswith('.'):
                continue
            if k not in defined:
                bad += 1
                print(f'{f}:{n} cites undefined key {k}')
print(f'undefined citations: {bad}')
for k in removed:
    for p, t in sorted(texts.items()):
        if f'`{k}`' in t:
            print(f'removed key {k} still cited in {os.path.relpath(p, root)}')
