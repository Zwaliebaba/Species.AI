"""C-UI-a: shift requirement ids UI-<lo>..UI-<hi> up by one in the given files.

Usage: python3 -I C-UI-a_renumber.py <lo> <hi> <file> [<file> ...]
"""
import re
import sys

lo, hi = int(sys.argv[1]), int(sys.argv[2])
for path in sys.argv[3:]:
    s = open(path, encoding='utf-8').read()
    def sub(m):
        n = int(m.group(1))
        return f'UI-{n + 1:03d}' if lo <= n <= hi else m.group(0)
    s2 = re.sub(r'UI-(\d{3})\b', sub, s)
    open(path, 'w', encoding='utf-8').write(s2)
    print(path, sum(1 for _ in re.finditer(r'UI-(\d{3})\b', s) if lo <= int(_.group(1)) <= hi))
