"""Print Markdown lists of TO-DEC, DEFER-DESIGN, TO-ADR and KEEP-HOOKS rows, and drop groups, from a routing CSV.

Usage: python3 -I F2b1_lists.py <routing.csv>
"""
import csv
import sys
from collections import defaultdict

rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))


def ref(r):
    return f"{r['orig_file'][:2]} L{r['start_line']}-{r['end_line']} `{r['anchor'][:48]}` ({round(int(r['bytes']) / 1024, 1)} KB)"


print('### TO-DEC rows (owner X-DEC)\n')
for r in rows:
    if r['action'] == 'TO-DEC':
        print(f"- {ref(r)}: **{r['target_section']}**. {r['note']}")
print('\n### DEFER-DESIGN rows (owner X-DEC; one Open DEC per topic, "Decide by: design phase")\n')
groups = defaultdict(list)
for r in rows:
    if r['action'] == 'DEFER-DESIGN':
        groups[r['note'].split(' ')[0]].append(r)
for code in sorted(groups):
    tot = sum(int(r['bytes']) for r in groups[code])
    print(f"\n**{code}** ({len(groups[code])} rows, {round(tot / 1024, 1)} KB)\n")
    for r in groups[code]:
        print(f"- {ref(r)}: {r['note'][len(code) + 1:]}")
print('\n### TO-ADR rows (owner X-ADR)\n')
for r in rows:
    if r['action'] == 'TO-ADR':
        print(f"- {ref(r)}: **{r['target_section']}**. {r['note']}")
print('\n### KEEP-HOOKS rows\n')
for r in rows:
    if r['action'] == 'KEEP-HOOKS':
        print(f"- {ref(r)}: {r['owner_package']} → `{r['target_file']}` {r['target_section']}. {r['note']}")
print('\n### Dropped groups\n')
d = defaultdict(list)
for r in rows:
    if r['action'].startswith('DROP'):
        d[(r['action'], r['note'].split(' ')[0])].append(r)
for (a, code), rs in sorted(d.items()):
    tot = sum(int(r['bytes']) for r in rs)
    secs = ', '.join(f"{r['orig_file'][:2]} {r['anchor'][:28]}" for r in rs)
    print(f"- **{a} {code}** ({len(rs)} rows, {round(tot / 1024, 1)} KB): {secs}")
