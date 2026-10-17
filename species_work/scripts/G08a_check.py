"""G08a self-check. Usage: python3 -I G08a_check.py <REPO>"""
import csv, json, os, re, sys, glob
REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
pkg = json.load(open(os.path.join(REPO, 'species_work/packages/G08a.json'), encoding='utf-8'))
files = [os.path.join(NEXT, t) for t in pkg['targets']]
texts = {f: open(f, encoding='utf-8').read() for f in files}
allt = '\n'.join(texts.values())
main = texts[files[1]]
ids = re.findall(r'\*\*R-MAP-(\d{3})\*\*', allt)
print('requirements', len(ids), 'unique', len(set(ids)), 'min', min(ids), 'max', max(ids))
print('dup ids', sorted({i for i in ids if ids.count(i) > 1}))
print('out of range', [i for i in ids if not 1 <= int(i) <= 299])
cited = set(re.findall(r'R-MAP-(\d{3})', allt)) - set(ids); print('cited but undefined R-MAP', sorted(cited))
# requirement keyword present
for m in re.finditer(r'\*\*R-MAP-(\d{3})\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)', allt): pass
nokw = [i for i in ids if not re.search(r'\*\*R-MAP-%s\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)' % i, allt)]
print('ids without keyword', nokw)
defs = {}
for f, t in texts.items():
    for k in re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M):
        defs.setdefault(k, []).append(os.path.basename(f))
print('keys defined', len(defs)); print('dup keys', {k: v for k, v in defs.items() if len(v) > 1})
print('keys not ref.map.*', [k for k in defs if not k.startswith('ref.map.')])
other = {}
for p in glob.glob(os.path.join(NEXT, '**/*.md'), recursive=True):
    if p in texts: continue
    for k in re.findall(r'^\| `((?:ref|rnd|ter|cam|dat)\.[a-z0-9_.]+)` \|', open(p, encoding='utf-8').read(), re.M):
        other.setdefault(k, p)
print('keys also defined elsewhere', [(k, other[k]) for k in defs if k in other])
used = set(re.findall(r'`((?:ref|rnd)\.[a-z0-9_.<>*]+)`', allt))
print('cited keys not defined anywhere', sorted(k for k in used if k not in defs and k not in other and '*' not in k and '<' not in k))
# each own key cited by a requirement paragraph
reqtext = '\n'.join(p for p in re.split(r'\n\s*\n', main) if re.match(r'\*\*R-MAP-\d{3}\*\*', p.strip()) or p.strip().startswith('|') or p.strip().startswith('1.'))
rq = re.split(r'(?=\*\*R-MAP-\d{3}\*\*)', main)
uncited = []
for k in defs:
    hit = any(k in seg for seg in rq[1:] if not seg.startswith('## 4'))
    # exclude tuning table section
    body = main.split('## 4 Tuning table')[0]
    if ('`%s`' % k) not in body: uncited.append(k)
print('own keys not cited in §2', uncited)
idx = open(os.path.join(NEXT, 'decisions/README.md'), encoding='utf-8').read()
known = set(re.findall(r'^\| (DEC-\d{3}) \|', idx, re.M))
status = dict(re.findall(r'^\| (DEC-\d{3}) \| \[[^\]]*\]\([^)]*\) \| (\w+) \|', idx, re.M))
decs = set(re.findall(r'DEC-\d{3}', allt)); print('decs cited', len(decs), 'unknown', sorted(decs - known))
body2 = main.split('## 3 States and flows')[0]
for d in pkg['decs_to_cite']:
    dec = d.split()[0]
    st = status.get(dec)
    inreq = any(dec in seg for seg in re.split(r'(?=\*\*R-MAP-\d{3}\*\*)', body2)[1:])
    if dec not in allt: print('NOT CITED', d, st)
    elif st == 'Open' and not inreq: print('OPEN NOT IN A REQUIREMENT', d)
m = re.search(r'## 8 Open decisions(.*)', main, re.S)
listed = re.findall(r'^- (DEC-\d{3})', m.group(1), re.M)
print('open-list not Open:', [d for d in listed if status.get(d) != 'Open'])
opencited = sorted(d for d in decs if status.get(d) == 'Open')
print('Open DECs cited', opencited, 'missing from §8 list', [d for d in opencited if d not in listed])
adrs = set(re.findall(r'ADR-\d{4}', allt))
adridx = open(os.path.join(NEXT, 'adr/README.md'), encoding='utf-8').read()
print('ADRs cited', sorted(adrs), 'unknown', sorted(a for a in adrs if a not in adridx))
man = set(r['asset_id'] for r in csv.DictReader(open(os.path.join(NEXT, 'assets/asset_manifest.csv'), encoding='utf-8')))
ass = set(re.findall(r'`((?:sfx|fx|mesh|icon|map|marker|thumb|terrain)\.[a-z0-9_.]+)`', allt))
ass = {a for a in ass if not a.startswith('map.difficulty') and not a.startswith('map.<')}
print('asset ids cited', len(ass), 'not in manifest', sorted(a for a in ass - man))
print('assets_for_you not cited', [a for a in pkg['assets_for_you'] if a not in allt])
for w in ['etc.', 'and so on', 'and others', 'various', 'similar to the above', '…']:
    for f, t in texts.items():
        if w in t: print('TRUNCATION', repr(w), os.path.basename(f))
H = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table', 'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
for f, t in texts.items():
    have = re.findall(r'^## \d+ (.+)$', t, re.M)
    miss = [h for h in H if h not in have]
    print(os.path.basename(f), 'headings missing', miss, 'mermaid', t.count('```mermaid'), 'figure captions', len(re.findall(r'^Figure \d', t, re.M)), 'size', len(t.encode()))
# links
for f, t in texts.items():
    for l in re.findall(r'\]\(([^)#]+)(?:#[^)]*)?\)', t):
        if l.startswith('http'): continue
        p = os.path.normpath(os.path.join(os.path.dirname(f), l))
        if not os.path.exists(p): print('missing link target', os.path.basename(f), l)
# AC ids
acs = re.findall(r'\*\*AC-MAP-(\d+)\*\*', main); print('ACs', len(acs))
