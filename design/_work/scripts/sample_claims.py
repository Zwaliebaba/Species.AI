#!/usr/bin/env python3
"""Phase 5: draw a reproducible random sample of [C]-tagged claims (lines that carry a [C] tag and at least one
code/ or data/ citation) from each final document, for independent verification.
Usage: python3 -I sample_claims.py <DESIGN_DIR> <OUT_CSV> [per_doc=12] [seed=20261017]"""
import csv, glob, os, random, re, sys
design, out = sys.argv[1], sys.argv[2]
per_doc = int(sys.argv[3]) if len(sys.argv) > 3 else 12
seed = int(sys.argv[4]) if len(sys.argv) > 4 else 20261017
rng = random.Random(seed)
CITE = re.compile(r'(?:code|data)/[\w\-./+]+')
rows = []
for p in sorted(glob.glob(os.path.join(design, '[0-9][0-9]_*.md'))):
    name = os.path.basename(p)
    if name.startswith(('90_', '91_', '99_')):
        continue
    lines = open(p, encoding='utf-8', errors='replace').read().split('\n')
    cands = []
    heading = ''
    in_code = False
    for i, l in enumerate(lines, 1):
        if l.startswith('```'): in_code = not in_code
        if in_code: continue
        if re.match(r'#{2,5} ', l): heading = l.lstrip('# ').strip()[:80]
        if '[C]' in l and CITE.search(l) and len(l) > 40:
            cands.append((i, heading, l.strip()))
    pick = rng.sample(cands, min(per_doc, len(cands)))
    for i, h, l in sorted(pick):
        rows.append([name, i, h, l[:1500], len(cands)])
with open(out, 'w', newline='', encoding='utf-8') as fh:
    w = csv.writer(fh); w.writerow(['doc', 'line', 'section', 'claim_text', 'candidates_in_doc']); w.writerows(rows)
print('sampled', len(rows), 'claims from', len({r[0] for r in rows}), 'documents; seed', seed)
