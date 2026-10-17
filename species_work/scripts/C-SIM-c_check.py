import sys, re, pathlib, csv
root = pathlib.Path(sys.argv[1])          # design_next
files = [pathlib.Path(p) for p in sys.argv[2:]]
dec_index = (root/'decisions'/'README.md').read_text(encoding='utf-8')
decs = set(re.findall(r'^\| (DEC-\d+) \|', dec_index, re.M))
status = dict(re.findall(r'^\| (DEC-\d+) \| .*? \| (Open|Resolved|Dropped) \|', dec_index, re.M))
adrs = set(m.group(1) for m in re.finditer(r'\[(ADR-\d{4})\]', (root/'adr'/'README.md').read_text(encoding='utf-8')))
allids = []
for f in files:
    t = f.read_text(encoding='utf-8')
    # strip code fences
    prose = re.sub(r'```.*?```', '', t, flags=re.S)
    defs = re.findall(r'\*\*(R-SIM-(\d+))\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)', t)
    nums = [int(n) for _, n, _ in defs]
    print(f'== {f.name}: {len(defs)} requirements, range {min(nums)}-{max(nums)}')
    if nums != sorted(nums): print('  NOT ascending')
    if any(n < 500 or n > 799 for n in nums): print('  OUT OF RANGE')
    allids += nums
    # cited R-SIM ids that are in my range but not defined anywhere in my files
    cited = set(int(x) for x in re.findall(r'R-SIM-(\d+)', t))
    # tuning keys defined
    keys = re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M)
    print(f'  tuning keys defined: {len(keys)}; dupes: {[k for k in set(keys) if keys.count(k)>1]}')
    # keys cited in section 2
    sec2 = t.split('## 2 Rules',1)[1].split('## 3 ',1)[0]
    for k in keys:
        if f'`{k}`' not in sec2: print('  key not cited in §2:', k)
    # DECs cited
    cd = set(re.findall(r'DEC-\d+', t))
    missing = sorted(cd - decs)
    print('  DECs cited:', len(cd), 'missing:', missing)
    ca = set(re.findall(r'ADR-\d{4}', t)); print('  ADRs:', sorted(ca), 'missing:', sorted(ca-adrs))
    # headings
    for h in ['## 1 Purpose and player experience','## 2 Rules','## 3 States and flows','## 4 Tuning table','## 5 Edge cases and interactions','## 6 Acceptance criteria','## 7 Dependencies','## 8 Open decisions']:
        if h not in t: print('  missing heading', h)
    for w in ['etc.', 'and so on', 'and others', 'various', 'similar to the above', '…', '...']:
        for m in re.finditer(re.escape(w), prose):
            line = prose[:m.start()].count('\n')+1
            print(f'  truncation word {w!r} near prose line {line}: {prose[m.start()-40:m.end()+20]!r}')
    # mermaid count
    print('  mermaid blocks:', t.count('```mermaid'))
    # requirement-level DEC citations: Open DECs cited inside requirement paragraphs
    reqtext = ' '.join(re.findall(r'\*\*R-SIM-\d+\*\* \([A-Z ]+\).*?(?=\n\*\*R-SIM-|\n### |\n## |\Z)', t, flags=re.S))
    globals().setdefault('REQ', '')
    globals()['REQ'] += reqtext
print('total requirements', len(allids), 'unique', len(set(allids)))
want = sys.stdin.read().split()
for d in want:
    if d not in REQ: print('decs_to_cite not cited in a requirement:', d, status.get(d))
