#!/usr/bin/env python3
"""S06 helper: list every path cited in a final document (code/, data/, contrib/, tools/,
targets/, docs/ prefixes, optionally followed by :Symbol) that is not in manifest.csv.
Brace groups like code/x.{h,cpp} and globs like icons/icon_*.bmp are expanded/ignored.
Usage: python3 -I S_06_check_citations.py <doc.md> <manifest.csv>"""
import csv, re, sys
DOC, MAN = sys.argv[1:3]
man = {r['path'] for r in csv.DictReader(open(MAN, encoding='utf-8'))}
dirs = set()
for p in man:
    parts = p.split('/')
    for i in range(1, len(parts)):
        dirs.add('/'.join(parts[:i]))
doc = open(DOC, encoding='utf-8').read()
bad = {}
for m in re.finditer(r'`((?:code|data|contrib|tools|targets|docs)/[^`:\s]*)', doc):
    p = m.group(1).rstrip('/.,;)')
    if '*' in p or '<' in p:
        continue
    b = re.match(r'(.*)\.\{([^}]*)\}$', p)
    cands = ['%s.%s' % (b.group(1), e) for e in b.group(2).split(',')] if b else [p]
    for c in cands:
        if c not in man and c not in dirs:
            bad[c] = bad.get(c, 0) + 1
print('unresolved:', len(bad))
for k, v in sorted(bad.items()):
    print(' ', k, v)
