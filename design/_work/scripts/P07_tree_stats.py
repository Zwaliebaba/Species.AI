import sys, collections
# reads output of P07_type_lines.py for Tree on stdin
rows = [l.rstrip('\n').split('\t') for l in sys.stdin if l.strip()]
def col(i, f=float):
    return [f(r[i]) for r in rows if len(r) > i]
names = ['file','type','id','x','z','team','fx','fz','global','height','budsize','pushOut','pushUp','iterations','seed','branchColour','leafColour','leafDrop','spiritDrop','evil']
print('n rows', len(rows))
for i, nm in enumerate(names):
    if i < 3: continue
    vals = [r[i] for r in rows if len(r) > i]
    if nm in ('branchColour','leafColour'):
        c = collections.Counter(vals)
        print(nm, 'distinct', len(c))
        for v, k in c.most_common(12):
            u = int(v) & 0xffffffff
            print('   ', v, k, 'RGBA=(%d,%d,%d,%d)' % (u & 255, (u >> 8) & 255, (u >> 16) & 255, (u >> 24) & 255))
        continue
    try:
        fv = [float(v) for v in vals]
    except ValueError:
        print(nm, 'nonnumeric'); continue
    c = collections.Counter(vals)
    print(nm, 'n=%d min=%g max=%g distinct=%d' % (len(fv), min(fv), max(fv), len(c)), 'top', c.most_common(6))
