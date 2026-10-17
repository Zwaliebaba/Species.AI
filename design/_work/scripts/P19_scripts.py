import sys, os, re, collections
d = sys.argv[1]
for f in sorted(os.listdir(d)):
    p = os.path.join(d, f)
    ops = collections.Counter(); says = []; lines=0
    for line in open(p, encoding='latin-1'):
        line = line.split('#')[0].replace('\r','').strip()
        if not line: continue
        lines += 1
        toks = re.split(r'[ \t:,]+', line)
        ops[toks[0]] += 1
        if toks[0].lower() == 'say' and len(toks) > 1: says.append(toks[1])
    print(f"{f}|{lines}|{';'.join(f'{k}x{v}' for k,v in sorted(ops.items()))}|{len(says)}|{says[0] if says else ''}..{says[-1] if says else ''}")
