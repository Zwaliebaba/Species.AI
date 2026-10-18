"""Phase 6: merge the reviewers' findings into one fix list (prompt §8.1 Phase 6).

Usage: python3 -I p6_merge_findings.py <REPO>

Reads every species_work/review/<ID>.md, takes the rows of its findings table (the table whose header
is `# | Severity | File | Where | Finding | Proposed fix`), normalises the file to a path under
design_next/ (or the repository root), and writes species_work/fix_list.md: one section per owning
area, rows ordered by severity, then file and `Where`, each tagged with its reviewer and row (`R2a-14`),
so findings on the same place sit next to each other and the fixer settles them at once.
"""
import collections
import os
import re
import sys

REPO = sys.argv[1]
WORK = os.path.join(REPO, 'species_work')
NEXT = os.path.join(REPO, 'design_next')
CELL = re.compile(r'(?<!\\)\|')
SEV = {'high': 0, 'medium': 1, 'low': 2}


def cells(line):
    parts = [c.strip() for c in CELL.split(line.strip())]
    return parts[1:-1] if line.strip().startswith('|') else parts


def area(path):
    p = path.replace('\\', '/').strip('`').split('#')[0].strip()
    p = re.sub(r'^(?:\./)?design_next/', '', p)
    if p.startswith(('AGENTS.md', 'CLAUDE.md', 'vision/', 'prompts/', 'Tools/', '.github/')):
        return 'root', p
    if os.path.exists(os.path.join(NEXT, p)):
        parts = p.split('/')
        if parts[0] == 'reference' and len(parts) > 1:
            return 'reference/' + parts[1].removesuffix('.md'), p
        if parts[0] == 'architecture' and len(parts) > 2:
            return 'components/' + parts[2].removesuffix('.md'), p
        return parts[0].removesuffix('.md'), p
    return 'unresolved', p


rows = []
for name in sorted(os.listdir(os.path.join(WORK, 'review'))):
    if not name.endswith('.md'):
        continue
    rid = name[:-3]
    text = open(os.path.join(WORK, 'review', name), encoding='utf-8').read().split('\n')
    in_table = False
    for line in text:
        c = cells(line) if line.strip().startswith('|') else []
        if len(c) >= 6 and c[0] == '#' and c[1].lower() == 'severity':
            in_table = True
            continue
        if in_table and c and re.match(r'^:?-{3,}', c[0]):
            continue
        if in_table and len(c) >= 6:
            sev = c[1].strip('*').lower()
            a, p = area(re.split(r',\s*|;\s*|<br>', c[2])[0])  # the first named file owns the row
            rows.append({'id': f'{rid}-{c[0]}', 'sev': sev, 'area': a, 'file': p, 'where': c[3],
                         'finding': c[4], 'fix': ' | '.join(c[5:])})
        elif in_table and not line.strip().startswith('|'):
            in_table = False

groups = collections.OrderedDict()
for r in sorted(rows, key=lambda r: (r['area'], SEV.get(r['sev'], 3), r['file'], r['where'])):
    groups.setdefault(r['area'], []).append(r)

counts = collections.Counter(r['sev'] for r in rows)
out = ['# Phase 6 fix list', '',
       f'{len(rows)} findings from {len({r["id"].split("-")[0] for r in rows})} reviews: '
       + ', '.join(f'{counts[s]} {s.capitalize()}' for s in ('high', 'medium', 'low')) + '.', '',
       '| Area | High | Medium | Low |', '|---|---|---|---|']
for a, rs in groups.items():
    c = collections.Counter(r['sev'] for r in rs)
    out.append(f'| {a} | {c["high"]} | {c["medium"]} | {c["low"]} |')
for a, rs in groups.items():
    out += ['', f'## {a}', '', '| Id | Severity | File | Where | Finding | Proposed fix | Status |', '|---|---|---|---|---|---|---|']
    for r in rs:
        out.append(f'| {r["id"]} | {r["sev"].capitalize()} | {r["file"]} | {r["where"]} | {r["finding"]} | {r["fix"]} | open |')
open(os.path.join(WORK, 'fix_list.md'), 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print(f'{len(rows)} findings, {len(groups)} areas: ' + ', '.join(f'{counts[s]} {s}' for s in ('high', 'medium', 'low')))
