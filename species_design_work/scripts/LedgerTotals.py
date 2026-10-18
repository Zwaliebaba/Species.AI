"""Refresh the totals table of Design/gdd/adoption_ledger/README.md from the effective verdicts.

Usage: python3 -I LedgerTotals.py <REPO>

Runs the design checker's ledger gate (gate 12) to get each R- id's effective verdict, counts them per reference
document, and rewrites the README's totals table in place; the rest of the README is kept.
"""
import collections
import importlib.util
import os
import re
import sys

REPO = os.path.abspath(sys.argv[1])
spec = importlib.util.spec_from_file_location('CheckDesign', os.path.join(REPO, 'Tools', 'DesignCheck', 'CheckDesign.py'))
cd = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cd)

root = os.path.join(REPO, 'Design')
check = cd.Check(root, skip_mermaid=True, design_phase=True)
check.gate_requirements()   # the ledger gate needs the defined requirement ids
check.gate_ledger()
ids, _ = cd.reference_requirements(check.text)
verdict = getattr(check, 'ledger_verdicts', {})
VERDICTS = ['adopt', 'adopt-skirmish', 'adapt', 'drop', 'pending']
per = collections.OrderedDict()
for rid, (f, _) in sorted(ids.items()):
    doc = (f.split('/')[1] if f.startswith('reference/') else f.split('/')[0]).removesuffix('.md')
    per.setdefault(doc, collections.Counter())[verdict.get(rid, 'pending')] += 1

per = collections.OrderedDict(sorted(per.items()))
path = os.path.join(root, 'gdd', 'adoption_ledger', 'README.md')
text = open(path, encoding='utf-8').read()
old = re.search(r'^\| Reference document \|.*?(?=\n\n|\Z)', text, re.M | re.S)
sections = {}
for doc in per:
    lf = os.path.join(root, 'gdd', 'adoption_ledger', f'{doc}_ledger.md')
    t = open(lf, encoding='utf-8').read() if os.path.exists(lf) else ''
    owner = re.search(r'Owner: area (\w+)', t)
    sections[doc] = (owner.group(1) if owner else '?', len(re.findall(r'^\| `[^`]+ §[\d.]+`', t, re.M)))
rows = ['| Reference document | Owner | Sections | Requirements | ' + ' | '.join(VERDICTS) + ' |',
        '|---|---|---|---|' + '---|' * len(VERDICTS)]
tot = collections.Counter()
for doc, c in per.items():
    owner, ns = sections[doc]
    rows.append(f'| [{doc}]({doc}_ledger.md) | {owner} | {ns} | {sum(c.values())} | ' + ' | '.join(str(c[v]) for v in VERDICTS) + ' |')
    tot.update(c)
    tot['_sections'] += ns
rows.append(f'| **Total** | | **{tot["_sections"]}** | **{sum(tot[v] for v in VERDICTS)}** | '
            + ' | '.join(f'**{tot[v]}**' for v in VERDICTS) + ' |')
text = text[:old.start()] + '\n'.join(rows) + text[old.end():]
open(path, 'w', encoding='utf-8').write(text)
print(', '.join(f'{v} {tot[v]}' for v in VERDICTS))
