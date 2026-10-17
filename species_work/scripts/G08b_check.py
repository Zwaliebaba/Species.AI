"""G08b self-check. Usage: python3 -I G08b_check.py <REPO>"""
import csv, glob, json, os, re, sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
F = os.path.join(NEXT, 'reference/08_maps_and_level_design/map_editor_hooks.md')
pkg = json.load(open(os.path.join(REPO, 'species_work/packages/G08b.json'), encoding='utf-8'))
t = open(F, encoding='utf-8').read()
problems = []

# requirement ids
ids = [int(i) for i in re.findall(r'\*\*R-MAP-(\d{3})\*\*', t)]
print('requirements', len(ids), 'min', min(ids), 'max', max(ids))
if len(ids) != len(set(ids)): problems.append('duplicate ids')
if ids != sorted(ids): problems.append('ids not ascending')
bad = [i for i in ids if not 300 <= i <= 499]
if bad: problems.append(f'out of range {bad}')
cited_own = sorted({int(i) for i in re.findall(r'R-MAP-(\d{3})', t) if 300 <= int(i) <= 499} - set(ids))
if cited_own: problems.append(f'cited own ids not defined {cited_own}')

# external requirement ids exist somewhere
defs = {}
for p in glob.glob(os.path.join(NEXT, '**/*.md'), recursive=True):
    if os.path.abspath(p) == os.path.abspath(F): continue
    for m in re.findall(r'\*\*((?:R-[A-Z]+|[A-Z]+)-\d{3})\*\*', open(p, encoding='utf-8').read()):
        defs[m] = p
ext = sorted(set(re.findall(r'\b((?:R-(?:SIM|UNIT|ECON|MODE|CRATE|BOT|CTRL|MAP|META|AUD|TUT|VIS|BLDG))-\d{3}|RND-\d{3})\b', t)))
ext = [e for e in ext if not (e.startswith('R-MAP-') and 300 <= int(e[-3:]) <= 499)]
missing = [e for e in ext if e not in defs]
print('external requirement refs', len(ext), 'not found', missing)

# tuning keys
keys = re.findall(r'^\| `((?:ref|rnd)\.[a-z0-9_.]+)` \|', t, re.M)
print('tuning keys', len(keys))
if len(keys) != len(set(keys)): problems.append('duplicate keys ' + str({k for k in keys if keys.count(k) > 1}))
if any(not k.startswith('ref.map.editor.') for k in keys): problems.append('key outside ref.map.editor.')
other = {}
for p in glob.glob(os.path.join(NEXT, '**/*.md'), recursive=True):
    if os.path.abspath(p) == os.path.abspath(F): continue
    for k in re.findall(r'^\| `((?:ref|rnd)\.[a-z0-9_.]+)` \|', open(p, encoding='utf-8').read(), re.M):
        other.setdefault(k, p)
dup_else = [k for k in keys if k in other]
if dup_else: problems.append(f'keys also defined elsewhere {dup_else}')
body = t.split('## 4 Tuning table')[0]
uncited = [k for k in keys if not re.search(re.escape(k) + r'(?![a-z0-9_])', body)]
if uncited: problems.append(f'keys not cited in rules {uncited}')
used = sorted(set(re.findall(r'`((?:ref|rnd)\.[a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)`', t)))
undefined = [k for k in used if k not in keys and k not in other]
print('cited keys not defined anywhere (owner may still be writing):', undefined)

# DECs
idx = open(os.path.join(NEXT, 'decisions/README.md'), encoding='utf-8').read()
status = dict(re.findall(r'^\| (DEC-\d{3}) \| \[[^\]]*\]\([^)]*\) \| (\w+) \|', idx, re.M))
decs = sorted(set(re.findall(r'DEC-\d{3}', t)))
unknown = [d for d in decs if d not in status]
if unknown: problems.append(f'unknown DECs {unknown}')
for d in pkg['decs_to_cite']:
    if d.split()[0] not in t: problems.append('NOT CITED ' + d)
m = re.search(r'## 8 Open decisions(.*)', t, re.S)
listed = re.findall(r'^- (DEC-\d{3})', m.group(1), re.M)
notopen = [d for d in listed if status.get(d) != 'Open']
opencited = [d for d in decs if status.get(d) == 'Open' and d not in listed]
res = re.search(r'Resolved decisions applied[^\n]*', m.group(1)).group(0).split('Dropped entries')[0]
notres = [d for d in re.findall(r'DEC-\d{3}', res) if status.get(d) != 'Resolved']
rescited = [d for d in decs if status.get(d) == 'Resolved' and d not in res]
print('DECs cited', decs)
if notopen: problems.append(f'§8 list not Open {notopen}')
if opencited: problems.append(f'Open cited but not in §8 {opencited}')
if notres: problems.append(f'resolved line not Resolved {notres}')
if rescited: problems.append(f'Resolved cited but not in resolved line {rescited}')
# every Open DEC is cited by a requirement paragraph
reqparas = re.findall(r'\*\*R-MAP-\d{3}\*\*.*?(?=\n\*\*R-MAP-|\n## |\Z)', t, re.S)
for d in listed:
    if not any(d in p for p in reqparas): problems.append(f'Open {d} not cited by a requirement')

# ADRs
adrs = sorted(set(re.findall(r'ADR-\d{4}', t)))
adrfiles = {os.path.basename(p)[:4] for p in glob.glob(os.path.join(NEXT, 'adr/0*.md'))}
if [a for a in adrs if a[4:] not in adrfiles]: problems.append('missing ADR')
print('ADRs', adrs)

# assets
man = {r['asset_id'] for r in csv.DictReader(open(os.path.join(NEXT, 'assets/asset_manifest.csv'), encoding='utf-8'))}
ass = sorted(set(re.findall(r'`((?:terrain|sfx|fx|mesh|icon|marker|cursor|sprite|thumb|map|help|glyph|font)\.[a-z0-9_.]+)`', t)))
print('asset ids', ass, 'not in manifest', [a for a in ass if a not in man])

# links
for href in re.findall(r'\]\(([^)#]+)(?:#[^)]*)?\)', t):
    if href.startswith('http'): continue
    path = os.path.normpath(os.path.join(os.path.dirname(F), href))
    if not os.path.exists(path): print('link target missing (may be written later):', href)

# truncation and headings
for w in ['etc.', 'and so on', 'and others', 'various', 'similar to the above', '…']:
    if w in t: problems.append('truncation ' + w)
H = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table',
     'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
have = re.findall(r'^## \d+ (.+)$', t, re.M)
if have != H: problems.append(f'headings {have}')
print('diagrams', t.count('```mermaid'), 'captions', len(re.findall(r'^Caption: ', t, re.M)))
print('size', os.path.getsize(F))
print('PROBLEMS' if problems else 'OK', problems)
