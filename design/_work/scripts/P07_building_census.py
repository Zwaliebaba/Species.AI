import sys, os, re, collections
# usage: P07_building_census.py <levels_dir> [type ...]
d = sys.argv[1]
want = set(t.lower() for t in sys.argv[2:])
cnt = collections.defaultdict(collections.Counter)
lines_by_type = collections.defaultdict(list)
for fn in sorted(os.listdir(d)):
    if not fn.endswith('.txt'): continue
    inb = False
    with open(os.path.join(d, fn), encoding='latin-1') as f:
        for ln in f:
            s = ln.strip()
            if not s: continue
            tok = s.split()
            if tok[0] == 'Buildings_StartDefinition': inb = True; continue
            if tok[0] == 'Buildings_EndDefinition': inb = False; continue
            if inb and not tok[0].startswith('#'):
                cnt[tok[0]][fn] += 1
                if tok[0].lower() in want:
                    lines_by_type[tok[0]].append((fn, s))
for t in sorted(cnt):
    mp = sum(v for k, v in cnt[t].items() if k.startswith('mp_'))
    other = sum(v for k, v in cnt[t].items() if not k.startswith('mp_'))
    mpfiles = sorted(k for k in cnt[t] if k.startswith('mp_'))
    print(f"{t}\tmp={mp}\tother={other}\tmpfiles={len(mpfiles)}\t{','.join(mpfiles)[:300]}")
for t, L in lines_by_type.items():
    print('=====', t)
    for fn, s in L[:400]:
        print(fn, '|', s)
