"""G03b self-check. Usage: python3 -I G03b_check.py <REPO>"""
import csv, json, os, re, sys, glob
REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
D = os.path.join(NEXT, 'reference/03_buildings_and_world_objects')
files = [os.path.join(D, 'defences_and_props.md'), os.path.join(D, 'trees_and_crate_structures.md')]
pkg = json.load(open(os.path.join(REPO, 'species_work/packages/G03b.json'), encoding='utf-8'))
texts = {f: open(f, encoding='utf-8').read() for f in files}
allt = '\n'.join(texts.values())
# requirement ids
ids = re.findall(r'\*\*R-BLDG-(\d{3})\*\*', allt)
print('requirements', len(ids), 'unique', len(set(ids)), 'min', min(ids), 'max', max(ids))
print('out of range', [i for i in ids if not 400 <= int(i) <= 799])
print('dup ids', sorted({i for i in ids if ids.count(i) > 1}))
for f, t in texts.items():
    seq = [int(i) for i in re.findall(r'\*\*R-BLDG-(\d{3})\*\*', t)]
    print(os.path.basename(f), 'reqs', len(seq), 'ascending', seq == sorted(seq), 'first', seq[0], 'last', seq[-1])
cited = sorted(set(i for i in re.findall(r'R-BLDG-(\d{3})', allt) if 400 <= int(i) <= 799) - set(ids))
print('cited R-BLDG 400-799 but undefined', cited)
# requirement references to other docs exist?
others = {}
for p in glob.glob(os.path.join(NEXT, '**/*.md'), recursive=True):
    if p in texts: continue
    for m in re.findall(r'\*\*((?:R-[A-Z]+|[A-Z]+)-\d{3})\*\*', open(p, encoding='utf-8').read()):
        others[m] = p
ext = sorted(set(re.findall(r'\b(R-(?:SIM|UNIT|ECON|MODE|CRATE|BOT|CTRL|MAP|META|AUD|TUT|VIS)-\d{3})\b', allt)))
print('external req refs not found', [e for e in ext if e not in others])
rb = sorted(set(i for i in re.findall(r'R-BLDG-(\d{3})', allt) if int(i) < 400))
print('README/G03a R-BLDG refs not found', [i for i in rb if 'R-BLDG-' + i not in others])
# tuning keys defined
defs = {}
for f, t in texts.items():
    for k in re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M):
        defs.setdefault(k, []).append(os.path.basename(f))
print('keys defined', len(defs)); print('dup keys', {k: v for k, v in defs.items() if len(v) > 1})
print('defined keys outside ref.bldg.<my objects>', [k for k in defs if not re.match(r'ref\.bldg\.(turret|barrier_emitter|wall|tree|scenery_mesh|exclusion_zone|smoke_column|volcano_vent|jump_pad|hive|spitter_plant|allegiance_scrambler)\.', k)])
other = {}
for p in glob.glob(os.path.join(NEXT, '**/*.md'), recursive=True):
    if p in texts: continue
    for k in re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', open(p, encoding='utf-8').read(), re.M):
        other.setdefault(k, p)
print('keys also defined elsewhere', [(k, os.path.relpath(other[k], NEXT)) for k in defs if k in other])
used = set(re.findall(r'(ref\.[a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)', allt))
undefined = sorted(k for k in used if k not in defs and k not in other)
print('cited keys not defined anywhere', undefined)
unusedk = sorted(k for k in defs if sum(1 for _ in re.finditer(re.escape(k) + r'(?![a-z0-9_])', allt)) < 2)
print('keys defined but never cited in a rule', unusedk)
# DEC ids
idx = open(os.path.join(NEXT, 'decisions/README.md'), encoding='utf-8').read()
known = set(re.findall(r'^\| (DEC-\d{3}) \|', idx, re.M))
status = dict(re.findall(r'^\| (DEC-\d{3}) \| \[[^\]]*\]\([^)]*\) \| (\w+) \|', idx, re.M))
decs = set(re.findall(r'DEC-\d{3}', allt)); print('decs cited', len(decs), 'unknown', sorted(decs - known))
for d in pkg['decs_to_cite']:
    dec = d.split()[0]
    if dec not in allt: print('NOT CITED', d)
for f, t in texts.items():
    m = re.search(r'## 8 Open decisions(.*)', t, re.S)
    listed = re.findall(r'^- (DEC-\d{3})', m.group(1), re.M) if m else []
    bad = [d for d in listed if status.get(d) != 'Open']
    res = re.search(r'Resolved decisions applied[^\n]*', m.group(1)).group(0) if m else ''
    res = res.split('Dropped entries')[0]
    badr = [d for d in re.findall(r'DEC-\d{3}', res) if status.get(d) != 'Resolved']
    opencited = sorted(d for d in set(re.findall(r'DEC-\d{3}', t)) if status.get(d) == 'Open')
    print(os.path.basename(f), 'open-list not Open:', bad, '| resolved-list not Resolved:', badr, '| Open cited but not in §8:', [d for d in opencited if d not in listed])
# ADRs
adr = set(re.findall(r'ADR-\d{4}', allt)); adrfiles = {os.path.basename(p)[:4] for p in glob.glob(os.path.join(NEXT, 'adr/0*.md'))}
print('ADRs', sorted(adr), 'missing', [a for a in adr if a[4:] not in adrfiles])
# assets
man = set(r['asset_id'] for r in csv.DictReader(open(os.path.join(NEXT, 'assets/asset_manifest.csv'), encoding='utf-8')))
ass = set(re.findall(r'`((?:sfx|fx|mesh|icon|marker|cursor|sprite|ui|help)\.[a-z0-9_.]+)`', allt))
print('asset-like ids not in manifest', sorted(a for a in ass - man))
print('assets_for_you not cited', [a for a in pkg['assets_for_you'] if a not in allt])
# truncation words
for w in ['etc.', 'and so on', 'and others', 'various', 'similar to the above', '…']:
    for f, t in texts.items():
        if w in t: print('TRUNCATION', repr(w), os.path.basename(f))
# headings and sizes
H = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table', 'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
for f, t in texts.items():
    have = re.findall(r'^## \d+ (.+)$', t, re.M)
    print(os.path.basename(f), 'headings missing', [h for h in H if h not in have], 'mermaid', t.count('```mermaid'), 'captions', len(re.findall(r'^\*Figure \d+', t, re.M)), 'size', len(t.encode()), 'AC', len(re.findall(r'^\*\*AC-', t, re.M)))
# links
for f, t in texts.items():
    for l in re.findall(r'\]\(([^)#]+)(?:#[^)]*)?\)', t):
        if l.startswith('http'): continue
        q = os.path.normpath(os.path.join(os.path.dirname(f), l))
        if not os.path.exists(q): print('LINK (target not yet written)', os.path.basename(f), l)
