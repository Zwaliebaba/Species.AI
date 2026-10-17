#!/usr/bin/env python3
"""Split every Phase-2 draft into level-2 sections and route them to target documents.

A section heading looks like `## [→ 02_units_and_entities] Darwinian` (one or more 2-digit doc numbers inside the brackets,
e.g. `[→ 06 / 24_rendering]`). Unrouted level-2 sections (Cross-partition notes, Contradictions, [U] Unknowns,
Docs-vs-code discrepancies, IP register, Appendix …) are collected into _meta_<kind>.md files.

Usage: python3 -I route_sections.py <DRAFTS_DIR> <OUT_DIR>
Writes <OUT_DIR>/<NN>.md per target doc and <OUT_DIR>/_index.csv (agent, heading, targets, bytes).
"""
import os, re, sys, glob, csv, collections

drafts, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
routed = collections.defaultdict(list)
meta = collections.defaultdict(list)
index = []
META_KINDS = [('cross', r'cross-partition'), ('contradictions', r'contradiction|open question'), ('unknowns', r'\[u\]|unknown'),
              ('discrepancies', r'docs-vs-code|discrepanc'), ('ip', r'\bip register|\bip\b'), ('appendix', r'appendix|dead|legacy code'),
              ('constants', r'constant'), ('determinism', r'determinism|sync')]
for fp in sorted(glob.glob(os.path.join(drafts, '*.md'))):
    agent = os.path.basename(fp)[:-3]
    text = open(fp, encoding='utf-8', errors='replace').read()
    parts = re.split(r'(?m)^(?=## )', text)
    for part in parts:
        if not part.startswith('## '):
            continue
        heading = part.split('\n', 1)[0][3:].strip()
        m = re.match(r'\[(?:→|->)\s*([^\]]+)\]\s*(.*)', heading)
        if m:
            nums = sorted(set(re.findall(r'\b(\d\d)(?:_|\b)', m.group(1))))
            for n in nums:
                routed[n].append(f'<!-- from {agent} -->\n{part.rstrip()}\n')
            index.append([agent, heading, ';'.join(nums), len(part)])
        else:
            kind = 'other'
            for k, pat in META_KINDS:
                if re.search(pat, heading, flags=re.I):
                    kind = k; break
            meta[kind].append(f'<!-- from {agent} -->\n{part.rstrip()}\n')
            index.append([agent, heading, 'meta:' + kind, len(part)])
for n, chunks in routed.items():
    open(os.path.join(out, f'{n}.md'), 'w', encoding='utf-8').write(f'# Routed draft sections for document {n}\n\n' + '\n'.join(chunks))
for k, chunks in meta.items():
    open(os.path.join(out, f'_meta_{k}.md'), 'w', encoding='utf-8').write(f'# Collected "{k}" sections from all drafts\n\n' + '\n'.join(chunks))
with open(os.path.join(out, '_index.csv'), 'w', newline='') as fh:
    w = csv.writer(fh); w.writerow(['agent', 'heading', 'targets', 'bytes']); w.writerows(index)
for n in sorted(routed): print(n, len(routed[n]), 'sections', sum(len(c) for c in routed[n]), 'bytes')
for k in sorted(meta): print('meta', k, len(meta[k]))
