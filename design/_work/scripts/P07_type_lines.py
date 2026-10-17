import sys, os
# usage: P07_type_lines.py <levels_dir> <TypeName> [prefix]
d, t = sys.argv[1], sys.argv[2]
prefix = sys.argv[3] if len(sys.argv) > 3 else 'mp_'
for fn in sorted(os.listdir(d)):
    if not fn.endswith('.txt') or not fn.startswith(prefix): continue
    inb = False
    with open(os.path.join(d, fn), encoding='latin-1') as f:
        for ln in f:
            tok = ln.split()
            if not tok: continue
            if tok[0] == 'Buildings_StartDefinition': inb = True; continue
            if tok[0] == 'Buildings_EndDefinition': inb = False; continue
            if inb and tok[0] == t:
                print(fn + '\t' + '\t'.join(tok))
