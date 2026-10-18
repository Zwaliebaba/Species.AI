import csv, re, sys
# usage: R1b_scan.py review.md forbidden_patterns.tsv ip_tokens.txt
review, pats, toks = sys.argv[1:4]
lines = open(review, encoding='utf-8').read().split('\n')
rules = []
with open(pats, encoding='utf-8') as f:
    for r in csv.DictReader(f, delimiter='\t'):
        try:
            rules.append((re.compile(r['pattern'], re.I if 'i' in (r.get('flags') or '') else 0), r['class']))
        except re.error:
            pass
tokens = [t.strip().lower() for t in open(toks, encoding='utf-8') if t.strip()]
for i, line in enumerate(lines, 1):
    low = line.lower()
    hits = sorted({m.group(0) for rx, c in rules for m in rx.finditer(line)})
    th = sorted({t for t in tokens if len(t) > 3 and t in low})
    if hits or th:
        row = line.split('|')[1].strip() if line.startswith('|') else ''
        cls = line.split('|')[3].strip() if line.count('|') > 4 else ''
        print(f'{i}: row={row} cls={cls} pat={hits[:6]} tok={th[:6]}')
