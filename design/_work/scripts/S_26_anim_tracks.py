"""S26: summarise every object track of every .anim file (data only, no execution of file content).
Usage: python3 -I S_26_anim_tracks.py <anim_file> [...]
Output: markdown tables, one per file."""
import sys, os

COL = {0: 'none', 1: 'friendly', 2: 'enemy', 3: 'invisible'}

def parse(path):
    hdr = {'WIDTH': 600, 'HEIGHT': 300, 'START': 0.0, 'STOP': 30.0}
    objs = []
    cur_obj = None
    cur_kf = None
    with open(path, encoding='latin-1') as f:
        for raw in f:
            toks = raw.split()
            if not toks:
                continue
            k = toks[0].upper()
            if k == 'BEGIN' and len(toks) > 1:
                if toks[1].lower() == 'object':
                    cur_obj = []
                elif toks[1].lower() == 'keyframe':
                    cur_kf = {'TIMEINDEX': 0.0, 'IMAGENAME': 'darwinian.bmp', 'XPOS': 100.0, 'YPOS': 100.0,
                              'COLOUR': 0, 'SCALE': 0.3, 'ANGLE': 0.0, 'FLIPHORIZ': 0, 'ALPHA': 255}
                continue
            if k == 'END':
                if cur_kf is not None:
                    cur_obj.append(cur_kf); cur_kf = None
                elif cur_obj is not None:
                    objs.append(cur_obj); cur_obj = None
                continue
            if cur_kf is not None and len(toks) > 1:
                v = toks[1]
                if k in ('IMAGENAME',):
                    cur_kf[k] = v
                elif k in ('COLOUR', 'FLIPHORIZ', 'ALPHA'):
                    cur_kf[k] = int(float(v))
                elif k in cur_kf:
                    cur_kf[k] = float(v)
            elif cur_obj is None and k in hdr and len(toks) > 1:
                hdr[k] = float(toks[1]) if k in ('START', 'STOP') else int(float(toks[1]))
    return hdr, objs

def rng(vals, fmt='{:.0f}'):
    lo, hi = min(vals), max(vals)
    return fmt.format(lo) if abs(hi - lo) < 1e-9 else (fmt.format(lo) + '→' + fmt.format(hi) if vals[0] <= vals[-1] else fmt.format(lo) + '–' + fmt.format(hi))

for path in sys.argv[1:]:
    hdr, objs = parse(path)
    name = os.path.basename(path)
    nk = sum(len(o) for o in objs)
    print(f"#### `{name}` — canvas {hdr['WIDTH']}×{hdr['HEIGHT']}, loop {hdr['START']:.2f}–{hdr['STOP']:.2f} s, {len(objs)} objects, {nk} keyframes\n")
    print('| # | Image(s) | Colour role(s) | Keys | First–last key (s) | X range | Y range | Scale range | Angle range (deg) | Alpha range | Flip |')
    print('|---|---|---|---|---|---|---|---|---|---|---|')
    for i, o in enumerate(objs):
        ks = sorted(o, key=lambda kf: kf['TIMEINDEX'])
        imgs = []
        for kf in o:
            n = kf['IMAGENAME'].replace('.bmp', '')
            if n not in imgs: imgs.append(n)
        cols = []
        for kf in o:
            c = COL.get(kf['COLOUR'], str(kf['COLOUR']))
            if c not in cols: cols.append(c)
        t = [kf['TIMEINDEX'] for kf in ks]
        dead = sum(1 for x in t if x > hdr['STOP'])
        tt = f"{t[0]:.2f}–{t[-1]:.2f}" + (f" ({dead} beyond STOP)" if dead else '')
        xs = [kf['XPOS'] for kf in ks]; ys = [kf['YPOS'] for kf in ks]
        sc = [kf['SCALE'] for kf in ks]; an = [kf['ANGLE'] for kf in ks]; al = [kf['ALPHA'] for kf in ks]
        fl = sorted(set(kf['FLIPHORIZ'] for kf in ks))
        def mm(v, f='{:.0f}'):
            lo, hi = min(v), max(v)
            return f.format(lo) if abs(hi-lo) < 1e-9 else f.format(lo) + '..' + f.format(hi)
        print(f"| {i} | {', '.join(imgs)} | {', '.join(cols)} | {len(o)} | {tt} | {mm(xs)} | {mm(ys)} | {mm(sc,'{:.2f}')} | {mm(an)} | {mm(al)} | {','.join(map(str,fl))} |")
    print()
