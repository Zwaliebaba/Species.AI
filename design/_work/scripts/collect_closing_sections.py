#!/usr/bin/env python3
"""Collect the standard closing sections ("Open questions and unknowns", "Docs-vs-code discrepancies", "IP items")
from every final document into three working files that feed 41_risks_and_open_questions.md.
Usage: python3 -I collect_closing_sections.py <DESIGN_DIR> <OUT_DIR>"""
import os, re, sys, glob
design, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
KINDS = {'unknowns': r'open questions|unknowns', 'discrepancies': r'docs-vs-code', 'ip': r'^ip items|ip register'}
acc = {k: [] for k in KINDS}
stats = []
for fp in sorted(glob.glob(os.path.join(design, '[0-9][0-9]_*.md'))):
    name = os.path.basename(fp)
    if name.startswith(('40_', '41_', '90_', '91_', '99_')):
        continue
    text = open(fp, encoding='utf-8', errors='replace').read()
    parts = re.split(r'(?m)^(?=## )', text)
    found = {}
    for part in parts:
        if not part.startswith('## '): continue
        head = re.sub(r'^##\s+[\d.]*\s*', '', part.split('\n', 1)[0]).strip().lower()
        for k, pat in KINDS.items():
            if re.search(pat, head):
                acc[k].append(f'\n<!-- from {name} -->\n### From {name}\n' + part.split('\n', 1)[1].rstrip() + '\n')
                found[k] = part.count('\n|') - 2
    stats.append((name, found, text.count('[U]'), text.count('[I]'), text.count('[C]')))
for k, chunks in acc.items():
    open(os.path.join(out, f'closing_{k}.md'), 'w', encoding='utf-8').write(f'# Collected "{k}" sections\n' + ''.join(chunks))
with open(os.path.join(out, 'closing_stats.csv'), 'w') as fh:
    fh.write('doc,unknown_rows,discrepancy_rows,ip_rows,U_tags,I_tags,C_tags\n')
    for name, f, u, i, c in stats:
        fh.write(f"{name},{f.get('unknowns','')},{f.get('discrepancies','')},{f.get('ip','')},{u},{i},{c}\n")
for s in stats: print(s[0], s[1], 'U', s[2], 'I', s[3], 'C', s[4])
