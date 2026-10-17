#!/usr/bin/env python3
"""Assign every code/worldobject class to the final document that must carry its behaviour-spec heading.
Entity-derived -> 02, Building-derived -> 03, other WorldObject-derived (weapons/effects) -> 04,
helper classes with no WorldObject ancestry -> the document of the main class declared in the same header.
Usage: python3 -I class_assignment.py <P03_worldobject_classes.csv> <out.csv>"""
import csv, sys, collections
rows = list(csv.DictReader(open(sys.argv[1])))
base = {r['class']: (r['base'] or '').split(',')[0].replace('public', '').strip() for r in rows}
hdr = {r['class']: r['header'] for r in rows}
def root(c, seen=()):
    b = base.get(c, '')
    if c in ('Entity',): return 'Entity'
    if c in ('Building',): return 'Building'
    if c in ('WorldObject',): return 'WorldObject'
    if not b or b not in base or c in seen: return b or ''
    return root(b, seen + (c,))
doc_of = {}
for c in base:
    r = root(c)
    doc_of[c] = {'Entity': '02', 'Building': '03', 'WorldObject': '04'}.get(r, '')
# special cases: base classes themselves
doc_of['Entity'] = '02'; doc_of['Building'] = '03'; doc_of['WorldObject'] = '02'
by_hdr = collections.defaultdict(list)
for c, d in doc_of.items():
    if d: by_hdr[hdr[c]].append(d)
for c, d in list(doc_of.items()):
    if not d:
        cand = collections.Counter(by_hdr.get(hdr[c], [])).most_common(1)
        doc_of[c] = cand[0][0] if cand else '02'
with open(sys.argv[2], 'w', newline='') as fh:
    w = csv.writer(fh); w.writerow(['class', 'base', 'header', 'doc'])
    for c in sorted(doc_of): w.writerow([c, base[c], hdr[c], doc_of[c]])
cnt = collections.Counter(doc_of.values()); print(cnt)
for d in sorted(cnt): print(d, ' '.join(sorted(c for c in doc_of if doc_of[c] == d)))
