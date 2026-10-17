"""Owner decision O22: move the ADRs to `ADR/` as `ADR-NNN-<slug>.md` and rewrite every citation.

Usage: python3 -I p4_rename_adrs.py <REPO> [--apply]

- design_next/adr/00NN-<slug>.md  ->  design_next/ADR/ADR-0NN-<slug>.md (git mv), README.md moves too;
- in every text file under design_next/, prompts/, vision/, AGENTS.md, .github/ and the species_work files the
  Phase 4 agents read: each old file name becomes the new one, `adr/` becomes `ADR/` in paths, and the id
  `ADR-00NN` becomes `ADR-0NN` (three digits, same number).
Prints the counts per file; without --apply nothing is written or moved.
"""
import glob
import os
import re
import subprocess
import sys

REPO = sys.argv[1]
APPLY = '--apply' in sys.argv
OLD = os.path.join(REPO, 'design_next', 'adr')
NEW = os.path.join(REPO, 'design_next', 'ADR')
src_dir = OLD if os.path.isdir(OLD) else NEW
files = sorted(f for f in os.listdir(src_dir) if re.match(r'^\d{4}-.+\.md$', f))
fmap = {f: 'ADR-' + f[1:4] + f[4:] for f in files if f.startswith('0')}
assert len(fmap) == len(files), 'an ADR number does not start with 0'

ID = re.compile(r'\bADR-0(\d{3})\b')
PATH = re.compile(r'(?<![\w-])adr/')
NAME = re.compile('|'.join(re.escape(f) for f in sorted(fmap, key=len, reverse=True)))

targets = []
for root in ('design_next', 'prompts', 'vision', '.github'):
    targets += glob.glob(os.path.join(REPO, root, '**', '*'), recursive=True)
targets += [os.path.join(REPO, 'AGENTS.md')]
targets += glob.glob(os.path.join(REPO, 'species_work', '*.md')) + glob.glob(os.path.join(REPO, 'species_work', '*.csv'))
targets += glob.glob(os.path.join(REPO, 'species_work', 'packages', 'FX-*.json'))
targets += [os.path.join(REPO, 'species_work', 'reports', r) for r in ('X-ADR-P4.md', 'X-ARB.md', 'X-ROUTE-a.md', 'X-ROUTE-b.md',
                                                                       'X-DEC-P4a.md', 'X-DEC-P4b.md', 'X-AST-2a.md')]
total = 0
for p in sorted(set(targets)):
    if not os.path.isfile(p) or p.endswith(('.png', '.dds', '.wav')):
        continue
    try:
        t = open(p, encoding='utf-8').read()
    except UnicodeDecodeError:
        continue
    n1 = len(NAME.findall(t)); t = NAME.sub(lambda m: fmap[m.group(0)], t)
    n2 = len(PATH.findall(t)); t = PATH.sub('ADR/', t)
    n3 = len(ID.findall(t)); t = ID.sub(r'ADR-\1', t)
    if n1 + n2 + n3:
        total += n1 + n2 + n3
        print(f'{n1:4} names {n2:4} paths {n3:5} ids  {os.path.relpath(p, REPO)}')
        if APPLY:
            open(p, 'w', encoding='utf-8', newline='').write(t)
if APPLY and src_dir == OLD:
    subprocess.run(['git', '-C', REPO, 'mv', 'design_next/adr', 'design_next/ADR_tmp'], check=True)
    subprocess.run(['git', '-C', REPO, 'mv', 'design_next/ADR_tmp', 'design_next/ADR'], check=True)
    for old, new in fmap.items():
        subprocess.run(['git', '-C', REPO, 'mv', f'design_next/ADR/{old}', f'design_next/ADR/{new}'], check=True)
print(f'{len(fmap)} ADR files; {total} replacements; applied: {APPLY}')
