import sys
path=sys.argv[1]
want=set(sys.argv[2].split(','))
sec=None; ev=None; out={}
for line in open(path,encoding='latin-1'):
    s=line.rstrip('\n')
    t=s.strip()
    if not s.startswith('\t') and t and not t.startswith('#'):
        sec=t; ev=None; continue
    if t.startswith('EVENT'):
        ev=t.split()[1]; 
        if sec in want: out.setdefault(sec,[]).append([ev,[]])
    elif t.startswith('SOUNDNAME') and sec in want and ev:
        out[sec][-1][1].append(t.split()[1])
for k,v in out.items():
    print(k+': '+'; '.join(e+'='+'/'.join(n) for e,n in v))
