"""Regenerate the ADR index table rows in README.md from each ADR's H1 title and front-matter status.
Args: adr_dir"""
import os, re, sys
adr = sys.argv[1]
rows = []
for fn in sorted(os.listdir(adr)):
    m = re.match(r'ADR-(\d{3})-.*\.md$', fn)
    if not m:
        continue
    t = open(os.path.join(adr, fn), encoding='utf-8').read()
    status = re.search(r'^status:\s*(\S+)', t, re.M).group(1).capitalize()
    date = re.search(r'^date:\s*(\S+)', t, re.M).group(1)
    title = re.search(r'^# (.+)$', t, re.M).group(1).strip()
    rows.append(f'| [ADR-{m.group(1)}]({fn}) | {title} | {status} | {date} |')
p = os.path.join(adr, 'README.md')
lines = open(p, encoding='utf-8').read().split('\n')
start = next(i for i, l in enumerate(lines) if l.startswith('| Number | Title | Status | Date |'))
i = start + 2
while i < len(lines) and lines[i].startswith('| [ADR-'):
    i += 1
old = lines[start + 2:i]
lines[start + 2:i] = rows
open(p, 'w', encoding='utf-8').write('\n'.join(lines))
changed = [r for r in rows if r not in old]
print(f'{len(rows)} rows; {len(changed)} changed')
for r in changed:
    print('  ', r[:160])
