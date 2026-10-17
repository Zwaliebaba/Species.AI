"""Count build tags and keywords per heading section (level <= maxlevel).
Usage: python3 -I F2a1_tags.py <headings.csv> <origdir> <file> <maxlevel> [extra_regex...]"""
import csv, re, sys
hcsv, odir, fname, maxl = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
extra = sys.argv[5:]
with open(f'{odir}/{fname}', encoding='utf-8') as f:
    lines = f.read().split('\n')
pats = [('LEG', r'\[B:LEGACY'), ('DEAD', r'\[B:DEAD'), ('D3D', r'\[B:D3D'), ('DBG', r'\[B:DEBUG'),
        ('DEMO', r'\[B:DEMO'), ('FULL', r'\[B:FULL'), ('camp', r'(?i)campaign'), ('tank', r'(?i)tank ?battle'),
        ('dwn', r'Darwinia\b'), ('quirk', r'(?i)quirk|defect|bug\b'), ('lock', r'(?i)lockstep|sync')]
pats += [(e, e) for e in extra]
rx = [(n, re.compile(p)) for n, p in pats]
with open(hcsv, encoding='utf-8') as f:
    for r in csv.DictReader(f):
        if r['file'] != fname or int(r['level']) > maxl:
            continue
        a, b = int(r['start_line']), int(r['end_line'])
        seg = lines[a - 1:b]
        cnt = []
        for n, p in rx:
            c = sum(len(p.findall(l)) for l in seg)
            if c:
                cnt.append(f'{n}={c}')
        print(f"{'  '*(int(r['level'])-2)}H{r['level']} {a}-{b} {r['bytes']}B {r['heading'][:70]} | {' '.join(cnt)}")
