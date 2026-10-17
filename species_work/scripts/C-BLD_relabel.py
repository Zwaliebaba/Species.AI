"""C-BLD: replace given literal substrings in a file, each exactly once (or a stated count).

Usage: python3 -I C-BLD_relabel.py <file> <pairs.tsv>
pairs.tsv: old<TAB>new<TAB>expected_count
"""
import sys
path, pairs = sys.argv[1], sys.argv[2]
txt = open(path, encoding='utf-8').read()
for line in open(pairs, encoding='utf-8'):
    line = line.rstrip('\n')
    if not line:
        continue
    old, new, cnt = line.split("\t")
    old, new = old.replace("\\n", "\n"), new.replace("\\n", "\n")
    n = txt.count(old)
    if n != int(cnt):
        sys.exit(f'{old!r}: found {n}, expected {cnt}')
    txt = txt.replace(old, new)
open(path, 'w', encoding='utf-8').write(txt)
print('ok')
