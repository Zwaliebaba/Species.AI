"""Check that every requirement id and DEC id cited in the ADR files is defined.
Args: design_root adr_dir"""
import re, sys, os
root, adr = sys.argv[1], sys.argv[2]
defined, decs = set(), set()
for dp, dn, fns in os.walk(root):
    for fn in fns:
        if fn.endswith('.md'):
            t = open(os.path.join(dp, fn), encoding='utf-8').read()
            defined.update(re.findall(r'\*\*((?:R-[A-Z]+|[A-Z]{2,4})-\d{3})\*\*', t))
            decs.update(re.findall(r'^### (DEC-\d{3})', t, re.M))
def expand(a, b):
    p, x = a.rsplit('-', 1)
    q, y = b.rsplit('-', 1)
    return [f'{p}-{n:03d}' for n in range(int(x), int(y) + 1)] if p == q else [a, b]
bad = 0
for fn in sorted(os.listdir(adr)):
    if not fn.endswith('.md'):
        continue
    t = open(os.path.join(adr, fn), encoding='utf-8').read()
    ids = set(re.findall(r'\b((?:R-[A-Z]+|TER|RND|AST|VFX|AEN|UI|INP|CAM|DAT|PLT|BLD|DBG)-\d{3})\b', t))
    for a, b in re.findall(r'\b((?:R-[A-Z]+|[A-Z]{2,4})-\d{3}) to ((?:R-[A-Z]+|[A-Z]{2,4})-\d{3})\b', t):
        pass
    for i in sorted(ids):
        if i not in defined:
            print(f'{fn}: undefined {i}'); bad += 1
    for d in sorted(set(re.findall(r'\bDEC-\d{3}\b', t))):
        if d not in decs:
            print(f'{fn}: undefined {d}'); bad += 1
print('checked; problems:', bad)
