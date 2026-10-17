"""G00 self-check for reference/00_reference_overview.md.

Usage: python3 -I G00_check.py <doc.md> <decisions/README.md> <adr/README.md>

Checks: R-VIS ids unique and in 001-299; every cited R-VIS id is defined; tuning keys
unique; every key used in prose is defined in §4 and every defined key is used in §2;
every DEC and ADR id cited exists; template headings present; truncation words absent.
Prints counts and problems.
"""
import re
import sys

doc, dec_idx, adr_idx = sys.argv[1:4]
t = open(doc, encoding='utf-8').read()
decs = set(re.findall(r'\| (DEC-\d{3}) \|', open(dec_idx, encoding='utf-8').read()))
adrs = set(re.findall(r'\[(ADR-\d{4})\]', open(adr_idx, encoding='utf-8').read()))
problems = []

defined = re.findall(r'\*\*R-VIS-(\d{3})\*\*', t)
seen = set()
for d in defined:
    n = int(d)
    if not 1 <= n <= 299:
        problems.append(f'R-VIS-{d} out of range')
    if d in seen:
        problems.append(f'R-VIS-{d} defined twice')
    seen.add(d)
cited = set(re.findall(r'R-VIS-(\d{3})', t))
for c in sorted(cited - seen):
    problems.append(f'R-VIS-{c} cited but not defined')

sec = re.split(r'^## ', t, flags=re.M)
sections = {s.split('\n', 1)[0]: s for s in sec[1:]}
tuning = next(v for k, v in sections.items() if k.startswith('4 Tuning table'))
rules = next(v for k, v in sections.items() if k.startswith('2 Rules'))
rows = re.findall(r'^\| `(ref\.vis\.[\w.]+)` \|', tuning, re.M)
keys = set()
for k in rows:
    if k in keys:
        problems.append(f'tuning key {k} defined twice')
    keys.add(k)
used_all = set(re.findall(r'`(ref\.vis\.[\w.]+)`', t.replace(tuning, '')))
used_rules = set(re.findall(r'`(ref\.vis\.[\w.]+)`', rules))
for k in sorted(used_all - keys):
    problems.append(f'key {k} used but not in tuning table')
for k in sorted(keys - used_rules):
    problems.append(f'key {k} not used by any rule in §2')
# requirement column of the tuning table must name defined requirements
for line in tuning.splitlines():
    if line.startswith('| `ref.vis.'):
        cols = [c.strip() for c in line.strip('|').split('|')]
        for r in re.findall(r'R-VIS-(\d{3})', cols[4]):
            if r not in seen:
                problems.append(f'tuning row {cols[0]} names undefined R-VIS-{r}')
        if not re.search(r'R-VIS-\d{3}', cols[4]):
            problems.append(f'tuning row {cols[0]} has no requirement')

for d in sorted(set(re.findall(r'DEC-\d{3}', t))):
    if d not in decs:
        problems.append(f'{d} does not exist')
for a in sorted(set(re.findall(r'ADR-\d{4}', t))):
    if a not in adrs:
        problems.append(f'{a} does not exist')

heads = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table',
         'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
have = re.findall(r'^##\s+\d+\s+(.+?)\s*$', t, re.M)
for h in heads:
    if not any(x.startswith(h) for x in have):
        problems.append(f'heading {h} missing')

for w in [r'\betc\b', r'and so on', r'and others', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
    for m in re.finditer(w, t, re.I):
        line = t.count('\n', 0, m.start()) + 1
        problems.append(f'truncation word {m.group(0)!r} at line {line}')

mermaid = len(re.findall(r'^```mermaid', t, re.M))
captions = len(re.findall(r'^\*Figure \d+ —', t, re.M))
tables = len(re.findall(r'^\|---', t, re.M))
acs = len(re.findall(r'^\| AC-\d+ \|', t, re.M))
print(f'requirements {len(seen)}; tuning keys {len(keys)}; diagrams {mermaid} (captions {captions}); '
      f'tables {tables}; acceptance criteria {acs}; DECs cited {len(set(re.findall(r"DEC-[0-9]{3}", t)))}; '
      f'ADRs cited {len(set(re.findall(r"ADR-[0-9]{4}", t)))}; size {len(t.encode("utf-8"))} B')
for p in problems:
    print('PROBLEM', p)
sys.exit(1 if problems else 0)
