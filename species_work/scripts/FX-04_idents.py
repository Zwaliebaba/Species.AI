"""List backticked tokens without dots in the given files, with counts (FX-04 O23 survey)."""
import re, sys
from collections import Counter
c = Counter()
where = {}
for f in sys.argv[1:]:
    t = open(f, encoding='utf-8').read()
    for m in re.finditer(r'`([^`\n]+)`', t):
        tok = m.group(1)
        if '.' in tok and '::' not in tok and '(' not in tok:
            continue
        c[tok] += 1
        where.setdefault(tok, f.split('/')[-1])
for tok, n in sorted(c.items()):
    print(f'{n:4d}  {tok}  [{where[tok]}]')
