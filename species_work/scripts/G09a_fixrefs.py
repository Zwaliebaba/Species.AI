"""G09a: apply exact, context-anchored text replacements to a file.
Usage: python3 -I G09a_fixrefs.py <file> <pairs.tsv>
pairs.tsv: old<TAB>new per line; each old must occur exactly once."""
import sys
path, pairs = sys.argv[1], sys.argv[2]
text = open(path, encoding='utf-8').read()
bad = 0
for line in open(pairs, encoding='utf-8'):
    line = line.rstrip('\n')
    if not line or line.startswith('#'):
        continue
    old, new = line.split('\t')
    n = text.count(old)
    if n != 1:
        print('COUNT', n, repr(old)); bad += 1; continue
    text = text.replace(old, new)
if bad:
    print('aborted, nothing written'); sys.exit(1)
open(path, 'w', encoding='utf-8').write(text)
print('ok')
