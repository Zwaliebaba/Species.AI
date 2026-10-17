"""Checks for the C-SIM-b outputs. Usage: python3 -I C-SIM-b_check.py <decisions_dir> <adr_readme> <file> [<file> ...]"""
import re, sys, pathlib, json
dec_dir = pathlib.Path(sys.argv[1]); adr_readme = pathlib.Path(sys.argv[2]); files = [pathlib.Path(p) for p in sys.argv[3:]]
dec_text = (dec_dir / 'README.md').read_text(encoding='utf-8')
dec_status = dict(re.findall(r'^\| (DEC-\d+) \| .*? \| (Open|Resolved|Dropped) \|', dec_text, re.M))
adr_ids = set(re.findall(r'ADR-\d{4}', adr_readme.read_text(encoding='utf-8')))
heads = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
         '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']
trunc = re.compile(r'\betc\.|\band so on\b|\band others\b|\bvarious\b|similar to the above|…')
external_keys = {'ref.sim.tick_period', 'ref.sim.update_slices'}
all_ids = []; all_keys = []; defined_keys = set(); used_keys = set(); req_text = {}
problems = []
for f in files:
    t = f.read_text(encoding='utf-8'); lines = t.splitlines()
    for h in heads:
        if h not in lines: problems.append(f'{f.name}: missing heading {h}')
    ids = re.findall(r'^\*\*(R-SIM-(\d+))\*\* \((MUST|MUST NOT|SHOULD|SHOULD NOT|MAY)\)', t, re.M)
    for full, n, _ in ids:
        all_ids.append(int(n))
        if not 300 <= int(n) <= 499: problems.append(f'{f.name}: {full} out of range')
    # requirement blocks
    cur = None
    for ln in lines:
        m = re.match(r'^\*\*(R-SIM-\d+)\*\*', ln)
        if m: cur = m.group(1); req_text[cur] = ln + '\n'; continue
        if ln.startswith('#'): cur = None; continue
        if cur: req_text[cur] += ln + '\n'
    # tuning keys defined in tuning table rows
    in_tuning = False
    for ln in lines:
        if ln.startswith('## 4 Tuning table'): in_tuning = True; continue
        if ln.startswith('## 5'): in_tuning = False
        if in_tuning:
            m = re.match(r'^\| `(ref\.[a-z0-9_.]+)` \|', ln)
            if m: all_keys.append(m.group(1)); defined_keys.add(m.group(1))
    used_keys |= set(re.findall(r'`?(ref\.sim\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)', t))
    for i, ln in enumerate(lines, 1):
        if trunc.search(ln): problems.append(f'{f.name}:{i}: truncation word: {ln.strip()[:80]}')
    for d in set(re.findall(r'DEC-\d+', t)):
        if d not in dec_status: problems.append(f'{f.name}: unknown {d}')
        elif dec_status[d] == 'Dropped': problems.append(f'{f.name}: cites Dropped {d}')
    for a in set(re.findall(r'ADR-\d{4}', t)):
        if a not in adr_ids: problems.append(f'{f.name}: unknown {a}')
    # mermaid captions
    for i, ln in enumerate(lines):
        if ln.strip() == '```mermaid':
            j = i + 1
            while lines[j].strip() != '```': j += 1
            k = j + 1
            while k < len(lines) and not lines[k].strip(): k += 1
            if k >= len(lines) or not lines[k].startswith('*Figure'): problems.append(f'{f.name}:{i+1}: diagram without caption')
    # referenced requirement ids exist
    refs = set(re.findall(r'R-SIM-(\d+)', t))
    for r in refs:
        if 300 <= int(r) <= 499 and int(r) not in all_ids and not any(re.search(rf'^\*\*R-SIM-{r}\*\*', g.read_text(encoding="utf-8"), re.M) for g in files):
            problems.append(f'{f.name}: reference to undefined R-SIM-{r}')
        if not 300 <= int(r) <= 499: problems.append(f'{f.name}: reference to R-SIM-{r} outside my range')
dups = sorted({x for x in all_ids if all_ids.count(x) > 1})
if dups: problems.append(f'duplicate ids {dups}')
kd = sorted({k for k in all_keys if all_keys.count(k) > 1})
if kd: problems.append(f'duplicate keys {kd}')
undefined = sorted(k for k in used_keys if k not in defined_keys and k not in external_keys)
if undefined: problems.append(f'keys used but not defined: {undefined}')
unused = sorted(k for k in defined_keys if k not in {u for f in files for u in re.findall(r"(ref\.sim\.[a-z0-9_.]+)", re.split(r"## 4 Tuning table", f.read_text(encoding="utf-8"))[0])})
if unused: problems.append(f'keys defined but not cited in §2: {unused}')
pkg = json.load(open(sys.argv[3].split('design_next')[0] + 'species_work/packages/C-SIM-b.json'))
for e in pkg['decs_to_cite']:
    d, st = e.split(' ')[0], e.split('(')[1].rstrip(')')
    cited = [r for r, tx in req_text.items() if re.search(rf'\b{d}\b', tx)]
    if not cited: problems.append(f'{e} not cited in any requirement')
print('requirements:', len(all_ids), 'range', min(all_ids), '-', max(all_ids))
print('tuning keys:', len(all_keys))
print('\n'.join(problems) if problems else 'no problems')
