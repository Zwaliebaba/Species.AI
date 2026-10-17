"""G09a acceptance checks. Usage: python3 -I G09a_check.py <repo root> <file>..."""
import re, sys, csv, glob, os, json
root, files = sys.argv[1], sys.argv[2:]
nx = os.path.join(root, 'design_next')
texts = {f: open(f, encoding='utf-8').read() for f in files}
allt = '\n'.join(texts.values())
ok = True
def bad(*a):
    global ok; ok = False; print('FAIL', *a)
# requirement ids
defs = []
for f, t in texts.items():
    for m in re.finditer(r'\*\*(R-META-(\d{3}))\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)', t):
        defs.append((m.group(1), int(m.group(2)), f))
ids = [d[0] for d in defs]
if len(ids) != len(set(ids)): bad('duplicate requirement ids')
for i, n, f in defs:
    if not (1 <= n <= 399): bad('out of range', i)
refs = set(re.findall(r'R-META-\d{3}', allt))
for r in sorted(refs - set(ids)): bad('dangling ref', r)
print('requirements', len(ids))
# tuning keys
own = re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', allt, re.M)
if len(own) != len(set(own)): bad('duplicate keys', [k for k in set(own) if own.count(k) > 1])
others = ''
for p in glob.glob(os.path.join(nx, '**', '*.md'), recursive=True):
    if os.path.abspath(p) in [os.path.abspath(f) for f in files]: continue
    others += open(p, encoding='utf-8').read() + '\n'
odef = set(re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', others, re.M))
for k in sorted(set(own) & odef): bad('key also defined elsewhere', k)
cited = set(k.rstrip('.') for k in re.findall(r'`(ref\.[a-z0-9_.]*[a-z0-9_])`', allt))
for k in sorted(cited - set(own)):
    if k in odef: continue
    if any(o.startswith(k + '.') for o in odef | set(own)): continue
    bad('cited key not defined anywhere', k)
print('tuning keys', len(own))
# DEC, ADR, assets
decs_known = set(re.findall(r'^### (DEC-\d{3})', '\n'.join(open(p, encoding='utf-8').read() for p in glob.glob(os.path.join(nx, 'decisions', '**', '*.md'), recursive=True)), re.M))
for d in sorted(set(re.findall(r'DEC-\d{3}', allt)) - decs_known): bad('unknown DEC', d)
adrs = set(re.findall(r'(\d{4})-', ' '.join(os.listdir(os.path.join(nx, 'adr')))))
for a in sorted(set(re.findall(r'ADR-(\d{4})', allt)) - adrs): bad('unknown ADR', a)
assets = set()
with open(os.path.join(nx, 'assets', 'asset_manifest.csv'), newline='', encoding='utf-8') as fh:
    for row in csv.DictReader(fh): assets.add(row['asset_id'])
prefixes = ('sprite.', 'ui.', 'fx.', 'help.', 'map.', 'thumb.', 'strings.', 'cursor.', 'amb.', 'sfx.', 'anim.')
cand = set(re.findall(r'`([a-z]+\.[a-z0-9_.]+)`', allt))
used_assets = sorted(c for c in cand if c in assets)
print('asset ids cited', len(used_assets))
# decs_to_cite
pkg = json.load(open(os.path.join(root, 'species_work', 'packages', 'G09a.json')))
reqtext = '\n'.join(p for p in re.split(r'\n\n', allt) if p.startswith('**R-META-'))
for e in pkg['decs_to_cite']:
    d, st = e.split(' ', 1)
    if 'Open' in st and d not in reqtext: bad('Open DEC not cited in a requirement', d)
    if d not in allt: bad('DEC not cited at all', d)
# template headings
for f, t in texts.items():
    if f.endswith('README.md'): continue
    for h in ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table', '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']:
        if h not in t: bad('missing heading', f, h)
# truncation words
for f, t in texts.items():
    for w in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
        for m in re.finditer(w, t):
            line = t[:m.start()].count('\n') + 1
            bad('truncation', os.path.basename(f), line, w)
# mermaid captions: each block preceded by a caption line
for f, t in texts.items():
    for m in re.finditer(r'```mermaid', t):
        prev = t[:m.start()].rstrip().split('\n')[-1]
        if not prev.startswith('Diagram '): bad('mermaid without caption', os.path.basename(f), prev[:60])
print('mermaid blocks', allt.count('```mermaid'))
for f, t in texts.items(): print(os.path.basename(f), len(t.encode('utf-8')), 'bytes')
print('OK' if ok else 'FAILURES')
