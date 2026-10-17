"""P02: extract LANGUAGEPHRASE keys from given source files and resolve them in english.txt.
Usage: python3 -I P02_strings.py <english.txt> <out.csv> <src1> [src2 ...]"""
import sys, re, csv
eng, out, srcs = sys.argv[1], sys.argv[2], sys.argv[3:]
table = {}
with open(eng, 'r', encoding='latin-1') as f:
    for line in f:
        line = line.rstrip('\r\n')
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        m = re.match(r'^(\S+)\s+(.*)$', line)
        if m:
            table.setdefault(m.group(1), m.group(2).strip())
rows = []
seen = set()
pat = re.compile(r'LANGUAGEPHRASE\(\s*"([^"]+)"\s*\)')
for s in srcs:
    with open(s, 'r', encoding='latin-1') as f:
        for i, line in enumerate(f, 1):
            for k in pat.findall(line):
                key = (k, s.split('/multiwinia/')[-1])
                if key in seen:
                    continue
                seen.add(key)
                rows.append((k, s.split('/multiwinia/')[-1], i, table.get(k, '<<missing>>')))
with open(out, 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['key', 'file', 'first_line', 'english'])
    for r in rows:
        w.writerow(r)
print(len(rows))
