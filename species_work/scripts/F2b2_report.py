"""Emit the generated parts of WORK/reports/F2b2.md (Markdown) to stdout.

Usage: python3 -I F2b2_report.py <routing.csv>
"""
import csv, sys
from collections import Counter, defaultdict
rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
heads = {}
if len(sys.argv) > 2:
    for h in csv.DictReader(open(sys.argv[2], encoding='utf-8')):
        heads[(h['file'], int(h['start_line']))] = h['heading']


def sec(r):
    h = heads.get((r['orig_file'], int(r['start_line'])), '')
    tok = h.split(' ')[0] if h else ''
    if tok and tok[0].isdigit():
        return '§' + tok
    return h if h else 'intro'


def status(note):
    o, rs = 'Open' in note, 'Resolved' in note
    return 'Resolved' if rs and not o else 'Open' if o and not rs else 'Open and Resolved items' if o and rs else 'per item'

num = lambda f: f[:2]
out = []
out.append('## Counts per action\n')
files = sorted({num(r['orig_file']) for r in rows})
c = defaultdict(Counter)
for r in rows:
    c[r['action']][num(r['orig_file'])] += 1
acts = ['KEEP', 'MERGE', 'KEEP-HOOKS', 'TO-DEC', 'DEFER-DESIGN', 'TO-ADR', 'TO-ASSETS', 'DROP-LEGACY', 'DROP-PROVENANCE']
out.append('| Action | ' + ' | '.join(files) + ' | Total |')
out.append('|---|' + '---:|' * (len(files) + 1))
for a in acts:
    out.append(f'| {a} | ' + ' | '.join(str(c[a][f]) for f in files) + f' | {sum(c[a].values())} |')
tot = Counter(num(r['orig_file']) for r in rows)
out.append('| **Rows** | ' + ' | '.join(str(tot[f]) for f in files) + f' | {len(rows)} |\n')
out.append('## Package sizing (KEEP + MERGE input bytes from documents 26-32)\n')
size = defaultdict(int); hooks = defaultdict(int); src = defaultdict(Counter)
for r in rows:
    if r['action'] in ('KEEP', 'MERGE'):
        size[r['owner_package']] += int(r['bytes']); src[r['owner_package']][num(r['orig_file'])] += int(r['bytes'])
    if r['action'] == 'KEEP-HOOKS':
        hooks[r['owner_package']] += int(r['bytes'])
out.append('| Package | KEEP + MERGE bytes | KEEP-HOOKS bytes | From | Over 150 KB? |')
out.append('|---|---:|---:|---|---|')
for k in sorted(size, key=lambda k: -size[k]):
    flag = 'no (at the limit, see below)' if size[k] + hooks[k] > 145000 else 'no'
    out.append(f'| {k} | {size[k]:,} | {hooks[k]:,} | ' + ', '.join(f'{f} ({b:,})' for f, b in sorted(src[k].items())) + f' | {flag} |')
out.append('')
for a, title in (('TO-DEC', 'TO-DEC rows'), ('DEFER-DESIGN', 'DEFER-DESIGN rows'), ('TO-ADR', 'TO-ADR rows')):
    sel = [r for r in rows if r['action'] == a]
    out.append(f'## {title} ({len(sel)})\n')
    for r in sel:
        if a == 'TO-DEC':
            tail = f" — {status(r['note'])}"
        elif a == 'DEFER-DESIGN':
            tail = f" — {r['note'].split(':')[0]}"
        else:
            tail = ''
        out.append(f"- {num(r['orig_file'])} {sec(r)} (L{r['start_line']}-{r['end_line']}): {r['target_section']}{tail}")
    out.append('')
print('\n'.join(out))
