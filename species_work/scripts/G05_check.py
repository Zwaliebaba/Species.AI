# usage: G05_check.py <doc> <decisions_readme> <package_json>
import sys, re, json, pathlib
doc = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
idx = pathlib.Path(sys.argv[2]).read_text(encoding='utf-8')
pkg = json.loads(pathlib.Path(sys.argv[3]).read_text(encoding='utf-8'))
ids = re.findall(r'^\*\*(R-ECON-(\d{3}))\*\*', doc, re.M)
nums = [int(n) for _, n in ids]
print('requirements', len(ids), 'unique', len(set(nums)) == len(nums), 'range', min(nums), max(nums),
      'contiguous', nums == list(range(1, len(nums) + 1)))
refs = set(re.findall(r'R-ECON-(\d{3})', doc))
print('dangling refs', sorted(r for r in refs if int(r) not in nums))
known = dict(re.findall(r'^\| (DEC-\d{3}) \| .*?\| (Open|Resolved|Dropped) \|', idx, re.M))
cited = set(re.findall(r'DEC-\d{3}', doc))
print('cited DECs', len(cited), 'unknown', sorted(c for c in cited if c not in known))
req_text = '\n'.join(re.findall(r'^\*\*R-ECON-\d{3}\*\*.*(?:\n(?!\*\*R-ECON-|#).*)*', doc, re.M))
for d in pkg['decs_to_cite']:
    dec, st = d.split(' ', 1)
    if 'Open' in st and dec not in req_text:
        print('OPEN DEC NOT IN A REQUIREMENT', dec)
    if dec not in cited:
        print('NOT CITED AT ALL', d)
heads = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
         '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']
print('missing headings', [h for h in heads if h not in doc])
for w in [r'\betc\.', r'and so on', r'and others', r'\bvarious\b', r'similar to the above', '…', r'\.\.\.']:
    for m in re.finditer(w, doc, re.I):
        print('TRUNCATION?', w, doc[max(0, m.start()-40):m.end()+20].replace('\n', ' '))
print('mermaid blocks', len(re.findall(r'```mermaid', doc)), 'size', len(doc.encode('utf-8')))
print('open DECs cited', sorted(c for c in cited if known.get(c) == 'Open'))
