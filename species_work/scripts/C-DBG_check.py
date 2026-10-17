"""C-DBG self-check for architecture/components/diagnostics_test_and_performance.md.

Usage: python3 -I C-DBG_check.py <REPO>
Checks: ids in range, unique, ascending; cited DBG ids defined; tuning keys unique across
design_next, each cited by a requirement paragraph, every cited key defined somewhere; DEC and ADR
ids exist; Open decs_to_cite cited by a requirement; other documents' requirement ids exist;
headings; truncation words; links resolve to a file on disk or a planned target; size; Mermaid count.
"""
import glob, json, os, re, sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
REL = 'architecture/components/diagnostics_test_and_performance.md'
PATH = os.path.join(NEXT, REL)
FOLDER = os.path.dirname(PATH)
pkg = json.load(open(os.path.join(REPO, 'species_work', 'packages', 'C-DBG.json'), encoding='utf-8'))
txt = open(PATH, encoding='utf-8').read()
problems = []
size = os.path.getsize(PATH)
if size > 120 * 1024:
    problems.append(f'size {size} > 120 KB')
for h in ['## 1 Responsibilities and boundaries', '## 2 Interfaces', '## 3 Data model',
          '## 4 Behaviour and algorithms', '## 5 Concurrency and performance budget',
          '## 6 Failure modes, diagnostics and telemetry', '## 7 Acceptance tests', '## 8 Open decisions']:
    if not re.search('^' + re.escape(h) + r'\s*$', txt, re.M):
        problems.append(f'missing heading {h}')
if not txt.startswith('# Diagnostics, test and performance\n\n> Status: Engine baseline · Prefix: DBG · ADRs:'):
    problems.append('status line')
ids = [int(i) for i in re.findall(r'\*\*DBG-(\d{3})\*\*', txt)]
lo, hi = (int(x) for x in pkg['id_ranges']['DBG'].split('-'))
if len(ids) != len(set(ids)):
    problems.append('duplicate DBG ids')
if ids != sorted(ids):
    problems.append('DBG ids not ascending')
for n in ids:
    if not lo <= n <= hi:
        problems.append(f'DBG-{n:03d} outside range')
for r in sorted(set(int(x) for x in re.findall(r'DBG-(\d{3})', txt))):
    if r not in ids:
        problems.append(f'DBG-{r:03d} cited but not defined')
paras = [p for p in re.split(r'\n(?=\*\*DBG-\d{3}\*\*)', txt) if p.startswith('**DBG-')]
paras = [re.split(r'\n#{2,4} ', p)[0] for p in paras]
req_text = '\n'.join(paras)
dec_index = open(os.path.join(NEXT, 'decisions', 'README.md'), encoding='utf-8').read()
dec_status = dict(re.findall(r'^\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
for d in sorted(set(re.findall(r'DEC-\d{3}', txt))):
    if d not in dec_status:
        problems.append(f'{d} does not exist')
adr_files = {('ADR-' + os.path.basename(f)[:4]) for f in glob.glob(os.path.join(NEXT, 'adr', '0*.md'))}
for a in sorted(set(re.findall(r'ADR-\d{4}', txt))):
    if a not in adr_files:
        problems.append(f'{a} does not exist')
for d in pkg['decs_to_cite']:
    dec, status = d.split(' ')[0], d.split('(')[1].rstrip(')')
    if status == 'Open' and dec not in req_text:
        problems.append(f'Open {dec} not cited by a requirement')
    if dec not in txt:
        problems.append(f'{d} not cited at all')
defs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    for line in open(f, encoding='utf-8'):
        m = re.match(r'\|\s*`((?:ref|rnd|ter|vfx|ast|ui|cam|dat|plt|bld|dbg|aen|inp)\.[a-z0-9_.]+)`\s*\|', line)
        if m:
            defs.setdefault(m.group(1), []).append(os.path.relpath(f, NEXT))
mine = [m.group(1) for m in re.finditer(r'(?m)^\|\s*`(dbg\.[a-z0-9_.]+)`\s*\|', txt)]
for k in mine:
    if len(defs[k]) > 1:
        problems.append(f'key {k} defined in {defs[k]}')
    if ('`' + k + '`') not in req_text:
        problems.append(f'key {k} not cited by a requirement')
for k in sorted(set(re.findall(r'`((?:dbg|ast|rnd|ref|ter|aen|cam|inp|vfx|dat|plt|bld)\.[a-z0-9_.]+)`', txt))):
    if k.endswith('.') or k not in defs:
        problems.append(f'key {k} cited but not defined anywhere')
reqdefs = {}
for f in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
    if f == PATH:
        continue
    for m in re.finditer(r'\*\*((?:R-[A-Z]+|RND|TER|AEN|AST|UI|INP|CAM|VFX|DAT|PLT|BLD)-\d{3})\*\*', open(f, encoding='utf-8').read()):
        reqdefs[m.group(1)] = f
for r in sorted(set(re.findall(r'\b((?:R-[A-Z]+|RND|TER|AEN|AST|UI|INP|CAM|VFX|DAT|PLT|BLD)-\d{3})\b', txt))):
    if r not in reqdefs:
        problems.append(f'{r} cited but defined nowhere')
body = re.sub(r'```.*?```', '', txt, flags=re.S)
for pat in [r'\betc\.', r'\band so on\b', r'\band others\b', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
    for m in re.finditer(pat, body, re.I):
        problems.append(f'truncation "{m.group(0)}"')
planned = set()
for pf in glob.glob(os.path.join(REPO, 'species_work', 'packages', '*.json')):
    for tgt in json.load(open(pf, encoding='utf-8')).get('targets', []):
        planned.add(tgt.rstrip('/'))
        if tgt.endswith('/'):
            planned.add(tgt.rstrip('/') + '/README.md')
for target in re.findall(r'\]\(([^)#\s]+)(?:#[^)]*)?\)', txt):
    if target.startswith('http'):
        continue
    tp = os.path.normpath(os.path.join(FOLDER, target))
    rel = os.path.relpath(tp, NEXT).replace(os.sep, '/')
    if not os.path.exists(tp) and rel not in planned:
        problems.append(f'link target neither on disk nor planned: {target}')
mermaid = len(re.findall(r'^```mermaid', txt, re.M))
tables = len(re.findall(r'^\|---', txt, re.M))
print(f'bytes {size}; requirements {len(ids)}; dbg keys {len(mine)}; mermaid {mermaid}; tables {tables}; '
      f'DECs {len(set(re.findall(r"DEC-[0-9]{3}", txt)))}; ADRs {len(set(re.findall(r"ADR-[0-9]{4}", txt)))}; '
      f'tests {len(re.findall(r"(?m)^\| T[0-9]+ \|", txt))}')
for p in problems:
    print(' -', p)
print('OK' if not problems else f'{len(problems)} problems')
