# Usage: python3 -I P12_shapes.py <shapes_dir> <code_dir>  -> CSV to stdout
# Parses .shp text: fragments (name,parent), markers, triangle counts (Triangles block or strips->tris), and greps code for references.
import sys, os, re
sd, cd = sys.argv[1], sys.argv[2]
code_text = {}
for root, _, files in os.walk(cd):
    for f in files:
        if f.endswith(('.cpp', '.h')):
            p = os.path.join(root, f)
            code_text[os.path.relpath(p, os.path.dirname(cd))] = open(p, encoding='latin-1').read().lower()
print('shape,fragments,max_depth,markers,triangles,format,marker_names,code_refs')
for f in sorted(os.listdir(sd)):
    if not f.endswith('.shp'): continue
    lines = open(os.path.join(sd, f), encoding='latin-1').read().splitlines()
    frags = {}; markers = []; tris = 0; fmt = set(); cur = None
    strip_verts = []
    i = 0
    while i < len(lines):
        t = lines[i].strip()
        m = re.match(r'(?i)fragment:\s*(\S+)', t)
        if m: cur = m.group(1); frags[cur] = 'sceneroot'
        m2 = re.match(r'(?i)parentname:\s*(\S+)', t)
        if m2 and cur is not None and cur in frags and not markers_pending if False else m2 and cur is not None: frags[cur] = m2.group(1).lower() if cur in frags else frags.get(cur)
        m = re.match(r'(?i)marker:\s*(\S+)', t)
        if m: markers.append(m.group(1)); cur = None
        m = re.match(r'(?i)triangles:\s*(\d+)', t)
        if m: tris += int(m.group(1)); fmt.add('tri')
        m = re.match(r'(?i)verts:\s*(\d+)', t)
        if m:
            fmt.add('strip'); n = int(m.group(1)); ids = []
            j = i + 1
            while len(ids) < n and j < len(lines):
                ids += re.findall(r'v(\d+)', lines[j]); j += 1
            for k in range(2, len(ids)):
                a, b, c = ids[k-2], ids[k-1], ids[k]
                if a != b and b != c and a != c: tris += 1
        i += 1
    def depth(n, seen=0):
        p = frags.get(n, 'sceneroot')
        return 1 if p in ('sceneroot', '') or seen > 50 else 1 + depth(next((k for k in frags if k.lower() == p), 'x'), seen + 1) if any(k.lower() == p for k in frags) else 1
    md = max([depth(n) for n in frags] or [0])
    key = f.lower()
    refs = sorted({os.path.basename(p) for p, s in code_text.items() if key in s or ('"' + key[:-4] + '"') in s})
    print(','.join(map(str, [f, len(frags), md, len(markers), tris, '+'.join(sorted(fmt)) or 'none', ';'.join(markers), ';'.join(refs)])))
