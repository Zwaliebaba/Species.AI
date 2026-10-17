#!/usr/bin/env python3
"""X1b: survey .shp files for format-level statistics.
Usage: python3 -I X1b_shp_survey.py <shapes_dir>
Reports keyword usage, per-file counts, anomalies relevant to the grammar."""
import sys, os, re, collections

SEP = re.compile(r'[ \t\n\r:,]+')

def toks(line):
    line = line.split('#', 1)[0]
    return [t for t in SEP.split(line) if t]

def main(d):
    kw = collections.Counter()
    first_kw = collections.Counter()
    anomalies = []
    rows = []
    for fn in sorted(os.listdir(d)):
        p = os.path.join(d, fn)
        if not os.path.isfile(p) or not fn.lower().endswith('.shp'):
            continue
        raw = open(p, 'rb').read()
        crlf = b'\r\n' in raw
        trailing_nl = raw.endswith(b'\n')
        text = raw.decode('latin-1')
        lines = text.split('\n')
        nfrag = nmark = nstrip_frag = ntri_frag = nnorm_nonzero = nmaterial = 0
        max_depth = 0
        parents = []
        names = []
        markers = []
        colours_over = 0
        has_alpha_col = 0
        in_frag = False
        mode = None
        for ln in lines:
            t = toks(ln)
            if not t:
                continue
            k = t[0].lower()
            first_kw[k if not k[0].isdigit() and not k.startswith('v') else ('<int>' if k[0].isdigit() else '<vN>')] += 1
            if k == 'fragment':
                nfrag += 1; names.append(t[1] if len(t) > 1 else '?'); mode = 'frag'
            elif k == 'marker':
                nmark += 1; markers.append(t[1] if len(t) > 1 else '?'); mode = 'mark'
            elif k == 'parentname':
                parents.append((mode, t[1] if len(t) > 1 else ''))
            elif k == 'depth':
                max_depth = max(max_depth, int(t[1]))
            elif k == 'strips':
                nstrip_frag += 1; mode = 'strips'
            elif k == 'triangles':
                ntri_frag += 1; mode = 'tris'
            elif k == 'normals':
                if int(t[1]) != 0: nnorm_nonzero += 1
                mode = 'norms'
            elif k == 'colours':
                mode = 'cols'
            elif k in ('vertices', 'positions'):
                mode = k
            elif k == 'material':
                nmaterial += 1
            elif mode == 'cols' and k[0].isdigit():
                vals = t[1:]
                if len(vals) > 3: has_alpha_col += 1
                if any(int(v) > 255 for v in vals[:3]): colours_over += 1
            if not k[0].isdigit() and not (k.startswith('v') and k[1:].isdigit()):
                kw[k] += 1
        rows.append((fn, nfrag, nmark, nstrip_frag, ntri_frag, nnorm_nonzero, nmaterial, max_depth, crlf, trailing_nl, colours_over, has_alpha_col))
        if nfrag and (nstrip_frag + ntri_frag) != nfrag:
            anomalies.append(f'{fn}: fragments={nfrag} but strips+tris blocks={nstrip_frag+ntri_frag}')
        if not trailing_nl:
            anomalies.append(f'{fn}: no trailing newline')
        if len(set(n.lower() for n in names)) != len(names):
            anomalies.append(f'{fn}: duplicate fragment names')
        for mode_, par in parents:
            if mode_ == 'frag' and par.lower() != 'sceneroot' and par.lower() not in [n.lower() for n in names]:
                anomalies.append(f'{fn}: fragment parent {par} not found')
    print('files', len(rows))
    print('keywords', dict(kw))
    print('first tokens', dict(first_kw))
    print('total fragments', sum(r[1] for r in rows), 'markers', sum(r[2] for r in rows))
    print('files with strips', sum(1 for r in rows if r[3]), 'files with triangles', sum(1 for r in rows if r[4]),
          'files mixing', sum(1 for r in rows if r[3] and r[4]))
    print('files with nonzero normals', sum(1 for r in rows if r[5]))
    print('files with Material lines', sum(1 for r in rows if r[6]))
    print('max frags per file', max(r[1] for r in rows), 'max markers per file', max(r[2] for r in rows))
    print('max marker depth', max(r[7] for r in rows))
    print('CRLF files', sum(1 for r in rows if r[8]), 'LF-only', sum(1 for r in rows if not r[8]))
    print('colour >255', sum(r[10] for r in rows), 'colour lines with 4 comps', sum(r[11] for r in rows))
    print('ANOMALIES:')
    for a in anomalies:
        print('  ', a)

if __name__ == '__main__':
    main(sys.argv[1])
