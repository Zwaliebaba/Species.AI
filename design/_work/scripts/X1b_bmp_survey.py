#!/usr/bin/env python3
"""X1b: survey every .bmp under a data dir for loader compatibility with
code/lib/bitmap.cpp:BitmapRGBA::LoadBmp.
Usage: python3 -I X1b_bmp_survey.py <data_dir> [--detail <subdir>]
Prints per-directory histograms of (bpp, infoheader size, compression),
non-power-of-two counts, palette-size/offset anomalies, row-padding hazards,
top-down images, pink (255,0,255) key usage and alpha usage in 32-bit files."""
import sys, os, struct, collections

def pot(n):
    return n > 0 and (n & (n - 1)) == 0

def analyse(path):
    b = open(path, 'rb').read()
    r = {'path': path, 'size': len(b)}
    if len(b) < 26 or b[:2] != b'BM':
        r['bad'] = 'no BM magic'
        return r
    bfSize, _, _, off = struct.unpack_from('<IHHI', b, 2)
    bisz = struct.unpack_from('<I', b, 14)[0]
    r['off'] = off
    r['bisz'] = bisz
    if bisz == 12:
        w, h, planes, bpp = struct.unpack_from('<HHHH', b, 18)
        comp = 0
        clrused = 0
        ncol_code = (off - 26) // 3
    else:
        w, h, planes, bpp, comp, sizeimg, xppm, yppm, clrused, clrimp = struct.unpack_from('<iiHHIIiiII', b, 18)
        ncol_code = (off - 54) // 4
    r.update(w=w, h=h, bpp=bpp, comp=comp, clrused=clrused, ncol_code=ncol_code)
    r['topdown'] = h < 0
    h = abs(h)
    # row byte size as the parser reads it (no padding for 4/8 bit)
    if bpp == 4:
        parser_row = (w + 1) // 2
    elif bpp == 8:
        parser_row = w
    elif bpp == 24:
        parser_row = w * 3 + ((4 - w * 3) & 3)
    elif bpp == 32:
        parser_row = w * 4
    else:
        parser_row = None
    true_row = ((w * bpp + 31) // 32) * 4
    r['pad_hazard'] = parser_row is not None and parser_row != true_row
    # header-size mismatch: parser starts pixel data straight after palette
    if bisz == 40:
        parser_pix_start = 14 + 40 + ncol_code * 4
    elif bisz == 12:
        parser_pix_start = 14 + 12 + ncol_code * 3
    else:
        parser_pix_start = None
    r['offset_hazard'] = parser_pix_start is not None and parser_pix_start != off
    r['pot'] = pot(w) and pot(h)
    # pink and alpha scans for 24/32 bit uncompressed
    pink = 0
    alpha_non255 = 0
    if comp == 0 and bpp in (24, 32) and off + true_row * h <= len(b):
        step = bpp // 8
        for y in range(h):
            row = off + y * true_row
            for x in range(w):
                p = row + x * step
                bl, g, rr = b[p], b[p + 1], b[p + 2]
                if rr == 255 and g == 0 and bl == 255:
                    pink += 1
                if step == 4 and b[p + 3] != 255:
                    alpha_non255 += 1
    elif comp == 0 and bpp in (4, 8):
        ncol = ncol_code
        pal = [b[54 + i * 4: 54 + i * 4 + 3] for i in range(min(ncol, 256))] if bisz == 40 else []
        pink_idx = {i for i, c in enumerate(pal) if c == b'\xff\x00\xff'}
        if pink_idx:
            for y in range(h):
                row = off + y * true_row
                if bpp == 8:
                    for x in range(w):
                        if row + x < len(b) and b[row + x] in pink_idx:
                            pink += 1
                else:
                    for x in range(w):
                        byte = b[row + x // 2] if row + x // 2 < len(b) else 0
                        idx = (byte >> 4) if x % 2 == 0 else (byte & 15)
                        if idx in pink_idx:
                            pink += 1
    r['pink'] = pink
    r['alpha_non255'] = alpha_non255
    return r

def main(d, detail=None):
    per_dir = collections.defaultdict(collections.Counter)
    rows = []
    for root, dirs, files in os.walk(d):
        for fn in files:
            if fn.lower().endswith('.bmp'):
                p = os.path.join(root, fn)
                r = analyse(p)
                rel = os.path.relpath(p, d)
                r['rel'] = rel
                top = rel.split(os.sep)[0] if os.sep in rel else '.'
                if top == 'textures' and rel.count(os.sep) >= 2:
                    top = os.sep.join(rel.split(os.sep)[:2])
                if top == 'levels' and rel.count(os.sep) >= 2:
                    top = os.sep.join(rel.split(os.sep)[:2])
                r['top'] = top
                rows.append(r)
                if 'bad' in r:
                    per_dir[top]['BAD'] += 1
                    continue
                per_dir[top][f"{r['bpp']}bpp/h{r['bisz']}/c{r['comp']}"] += 1
                per_dir[top]['nonPOT'] += 0 if r['pot'] else 1
                per_dir[top]['padHazard'] += 1 if r['pad_hazard'] else 0
                per_dir[top]['offsetHazard'] += 1 if r['offset_hazard'] else 0
                per_dir[top]['topdown'] += 1 if r['topdown'] else 0
                per_dir[top]['withPink'] += 1 if r['pink'] else 0
                per_dir[top]['withAlpha'] += 1 if r['alpha_non255'] else 0
                per_dir[top]['files'] += 1
    for top in sorted(per_dir):
        c = per_dir[top]
        print(top, dict(sorted((k, v) for k, v in c.items() if v)))
    print('TOTAL files', len(rows))
    print('--- hazards (pad/offset/topdown/bad/compressed):')
    for r in rows:
        if 'bad' in r or r.get('pad_hazard') or r.get('offset_hazard') or r.get('topdown') or r.get('comp'):
            print('  ', r['rel'], {k: r.get(k) for k in ('bpp', 'bisz', 'comp', 'w', 'h', 'off', 'ncol_code', 'pad_hazard', 'offset_hazard', 'topdown', 'bad')})
    if detail:
        print('--- detail for', detail)
        for r in sorted(rows, key=lambda r: r['rel']):
            if r['rel'].startswith(detail):
                print('  ', r['rel'], r.get('w'), r.get('h'), r.get('bpp'), 'pink=%d' % r.get('pink', 0), 'alpha<255=%d' % r.get('alpha_non255', 0))

if __name__ == '__main__':
    det = None
    if len(sys.argv) > 3 and sys.argv[2] == '--detail':
        det = sys.argv[3]
    main(sys.argv[1], det)
