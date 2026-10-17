# G02b: list tuning keys cited in a document, split into defined-elsewhere and own.
import re, sys, glob
doc = sys.argv[1]
others = sys.argv[2:]
t = open(doc).read()
body = t.split('## 4 Tuning table')[0] if '## 4 Tuning table' in t else t
keys = sorted(set(re.findall(r'`(ref\.[a-z0-9_.]+[a-z0-9_])`', body)))
defined = {}
for g in others:
    for f in glob.glob(g, recursive=True):
        if f == doc: continue
        for m in re.finditer(r'^\| `(ref\.[a-z0-9_.]+)` \|', open(f).read(), re.M):
            defined.setdefault(m.group(1), f)
own_def = set(re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M))
for k in keys:
    where = defined.get(k)
    print(('ELSEWHERE ' + where) if where else ('OWN' + (' (in table)' if k in own_def else '')), k)
for k in sorted(own_def - set(keys)):
    print('TABLE-ONLY', k)
