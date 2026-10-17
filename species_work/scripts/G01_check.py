"""G01 self-check. Usage: python3 -I G01_check.py <REPO>"""
import csv, json, os, re, sys, glob
REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
pkg = json.load(open(os.path.join(REPO, 'species_work/packages/G01.json'), encoding='utf-8'))
files = [os.path.join(NEXT, t) for t in pkg['targets']]
texts = {f: open(f, encoding='utf-8').read() for f in files}
allt = '\n'.join(texts.values())
# requirement ids
ids = re.findall(r'\*\*R-MODE-(\d{3})\*\*', allt)
print('requirements', len(ids), 'unique', len(set(ids)), 'min', min(ids), 'max', max(ids))
dups = sorted({i for i in ids if ids.count(i) > 1}); print('dup ids', dups)
cited = set(re.findall(r'R-MODE-(\d{3})', allt)) - set(ids); print('cited but undefined R-MODE', sorted(cited))
# tuning keys defined (first column of tuning tables)
defs = {}
for f, t in texts.items():
    for k in re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M):
        defs.setdefault(k, []).append(os.path.basename(f))
print('keys defined', len(defs)); print('dup keys', {k: v for k, v in defs.items() if len(v) > 1})
# keys defined elsewhere in design_next
other = {}
for p in glob.glob(os.path.join(NEXT, '**/*.md'), recursive=True):
    if p in texts: continue
    for k in re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', open(p, encoding='utf-8').read(), re.M):
        other.setdefault(k, p)
clash = [k for k in defs if k in other]; print('keys also defined elsewhere', [(k, other[k]) for k in clash])
used = set(re.findall(r'`(ref\.[a-z0-9_.<>*]+)`', allt))
undefined = sorted(k for k in used if k not in defs and k not in other and '*' not in k and '<' not in k)
print('cited keys not defined anywhere', undefined)
# DEC ids
idx = open(os.path.join(NEXT, 'decisions/README.md'), encoding='utf-8').read()
known = set(re.findall(r'^\| (DEC-\d{3}) \|', idx, re.M))
decs = set(re.findall(r'DEC-\d{3}', allt)); print('decs cited', len(decs), 'unknown', sorted(decs - known))
for d in pkg['decs_to_cite']:
    dec = d.split()[0]
    if dec not in allt: print('NOT CITED', d)
# open decs per index
status = dict(re.findall(r'^\| (DEC-\d{3}) \| \[[^\]]*\]\([^)]*\) \| (\w+) \|', idx, re.M))
for f, t in texts.items():
    m = re.search(r'## 8 Open decisions(.*)', t, re.S)
    listed = re.findall(r'^- (DEC-\d{3})', m.group(1), re.M) if m else []
    bad = [d for d in listed if status.get(d) != 'Open']
    res = re.search(r'Resolved decisions applied[^\n]*', m.group(1)) if m else None
    badr = [d for d in re.findall(r'DEC-\d{3}', res.group(0))] if res else []
    badr = [d for d in badr if status.get(d) not in ('Resolved',) and d not in ('DEC-003', 'DEC-058')]
    print(os.path.basename(f), 'open-list not Open:', bad, '| resolved-list not Resolved:', badr)
# assets
man = set(r['asset_id'] for r in csv.DictReader(open(os.path.join(NEXT, 'assets/asset_manifest.csv'), encoding='utf-8')))
ass = set(re.findall(r'`((?:sfx|fx|mesh|help|icon|map|marker|anim|strings|ui|music|thumb)\.[a-z0-9_.]+)`', allt))
strings_like = {a for a in ass if a.startswith('ui.') and a not in man}
print('asset-like ids not in manifest', sorted(a for a in ass - man if not a.startswith('ui.')), 'ui.* (strings?)', len(strings_like))
# truncation words
for w in ['etc.', 'and so on', 'and others', 'various', 'similar to the above', '…']:
    for f, t in texts.items():
        if w in t: print('TRUNCATION', w, os.path.basename(f))
# headings
H = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table', 'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
for f, t in texts.items():
    have = re.findall(r'^## \d+ (.+)$', t, re.M)
    miss = [h for h in H if h not in have]
    print(os.path.basename(f), 'headings missing', miss, 'mermaid', t.count('```mermaid'), 'size', len(t.encode()))
