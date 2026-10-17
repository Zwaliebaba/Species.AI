# G02b: structural checks for the creatures and hazards documents.
import re, sys, glob, csv, os
files = sys.argv[1:]
root = 'design_next'
texts = {f: open(f).read() for f in files}
# requirement ids
ids = []
for f, t in texts.items():
    for m in re.finditer(r'\*\*(R-UNIT-(\d{3}))\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)', t):
        ids.append((m.group(1), int(m.group(2)), f))
nums = [n for _, n, _ in ids]
print('requirements', len(ids), 'range', min(nums), max(nums))
bad = [i for i in ids if not 500 <= i[1] <= 899]
print('out of range', bad)
dup = sorted({i[0] for i in ids if [j[0] for j in ids].count(i[0]) > 1})
print('duplicate ids', dup)
defined = {i[0] for i in ids}
# other R-UNIT ids cited in my range must exist
for f, t in texts.items():
    for c in set(re.findall(r'R-UNIT-(\d{3})', t)):
        n = int(c)
        if 500 <= n <= 899 and 'R-UNIT-' + c not in defined:
            print('cited but undefined', f, 'R-UNIT-' + c)
# tuning keys
allrows = {}
for g in glob.glob(root + '/**/*.md', recursive=True):
    for m in re.finditer(r'^\| `(ref\.[a-z0-9_.]+)` \|', open(g).read(), re.M):
        allrows.setdefault(m.group(1), []).append(g)
for k, v in allrows.items():
    if len(v) > 1 and any(x in files for x in v):
        print('duplicate key', k, v)
for f, t in texts.items():
    body, _, table = t.partition('## 4 Tuning table')
    table = table.split('## 5 ')[0]
    own = re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', table, re.M)
    for k in own:
        if '`' + k + '`' not in body and k not in body:
            print('table key not cited in rules', f, k)
    cited = set(re.findall(r'(ref\.[a-z0-9_]+(?:\.[a-z0-9_]+)+)', body))
    for k in sorted(cited):
        if k.endswith('.') or '<kind>' in k: continue
        if k not in allrows:
            print('cited key not defined anywhere', f, k)
# DEC and ADR
decs = set()
for g in glob.glob(root + '/decisions/**/*.md', recursive=True):
    decs |= set(re.findall(r'^### (DEC-\d{3})', open(g).read(), re.M))
adrs = {os.path.basename(g)[:4] for g in glob.glob(root + '/adr/[0-9]*.md')}
for f, t in texts.items():
    for d in sorted(set(re.findall(r'DEC-\d{3}', t))):
        if d not in decs: print('unknown DEC', f, d)
    for a in sorted(set(re.findall(r'ADR-(\d{4})', t))):
        if a not in adrs: print('unknown ADR', f, a)
# assets
assets = {r['asset_id'] for r in csv.DictReader(open(root + '/assets/asset_manifest.csv'))}
for f, t in texts.items():
    for a in sorted(set(re.findall(r'`((?:mesh|sprite|sfx|fx|icon|marker|vo|music|ui)\.[a-z0-9_.]+)`', t))):
        if a not in assets: print('unknown asset', f, a)
# headings
need = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
        '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']
for f, t in texts.items():
    for h in need:
        if h not in t: print('missing heading', f, h)
    nocode = re.sub(r'```.*?```', '', t, flags=re.S)
    for w in ['etc.', 'and so on', 'and others', 'various', 'similar to the above', '…', '...']:
        if w in nocode: print('truncation word', f, repr(w))
    print(f, os.path.getsize(f), 'bytes', 'mermaid blocks', t.count('```mermaid'))
# open decs to cite
for d in ['DEC-084', 'DEC-091', 'DEC-107', 'DEC-113', 'DEC-311', 'DEC-453']:
    hit = [f for f, t in texts.items() if re.search(r'\*\*R-UNIT-\d{3}\*\*[^\n]*' + d, t)]
    print(d, 'cited by a requirement in', hit)
