#!/usr/bin/env python3
"""S_41: find inline [U] tags in document bodies (outside the closing sections) that do not
reference an NN-U id, so 41_risks_and_open_questions.md can add them to the register.
Usage: python3 -I S_41_inline_u.py <DESIGN_DIR> <OUT_CSV>"""
import csv, glob, os, re, sys
design, out = sys.argv[1], sys.argv[2]
CLOSING = re.compile(r'open questions|unknowns|docs-vs-code|^ip items|ip register|source map|appendix', re.I)
rows = []
for fp in sorted(glob.glob(os.path.join(design, '[0-9][0-9]_*.md'))):
    name = os.path.basename(fp)
    if name.startswith(('40_', '41_', '90_', '91_', '99_')):
        continue
    doc = name[:2]
    section = ''
    in_closing = False
    in_code = False
    for ln, line in enumerate(open(fp, encoding='utf-8', errors='replace'), 1):
        if line.startswith('```'):
            in_code = not in_code
        if in_code:
            continue
        if line.startswith('## '):
            head = re.sub(r'^##\s+[\d.]*\s*', '', line).strip()
            in_closing = bool(CLOSING.search(head))
            section = head
        elif line.startswith('### ') and not in_closing:
            section = line[4:].strip()
        if in_closing or '[U]' not in line:
            continue
        if line.startswith('> Evidence:'):
            continue
        ids = re.findall(r'\b(\d\d-U\d+)\b', line)
        resolved = bool(re.search(r'Resolved', line))
        rows.append(dict(doc=doc, file=name, line=ln, section=section[:80], ids=';'.join(sorted(set(ids))),
                         resolved=int(resolved), text=re.sub(r'\s+', ' ', line.strip())[:400]))
with open(out, 'w', newline='', encoding='utf-8') as fh:
    w = csv.DictWriter(fh, fieldnames=['doc', 'file', 'line', 'section', 'ids', 'resolved', 'text'])
    w.writeheader(); w.writerows(rows)
from collections import Counter
c = Counter(r['doc'] for r in rows); u = Counter(r['doc'] for r in rows if not r['ids'])
for d in sorted(c): print(d, 'inline[U] lines', c[d], 'without id', u[d])
print('total', len(rows), 'without id', sum(u.values()))
