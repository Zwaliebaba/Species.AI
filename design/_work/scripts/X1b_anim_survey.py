#!/usr/bin/env python3
"""X1b: survey data/animations/*.anim against code/animatedpanel.cpp grammar.
Usage: python3 -I X1b_anim_survey.py <animations_dir>"""
import sys, os, re, collections

SEP = re.compile(r'[ \t\n\r:,]+')

def toks(line):
    line = line.split('#', 1)[0]
    return [t for t in SEP.split(line) if t]

def main(d):
    bmps = {f.lower() for f in os.listdir(d) if f.lower().endswith('.bmp')}
    used_all = set()
    for fn in sorted(os.listdir(d)):
        if not fn.lower().endswith('.anim'):
            continue
        hdr = {}
        objs = 0
        kfs = 0
        per_obj_kf = []
        imgs = collections.Counter()
        fields = collections.defaultdict(list)
        unknown = collections.Counter()
        depth = 0
        cur = None
        for ln in open(os.path.join(d, fn), encoding='latin-1'):
            t = toks(ln)
            if not t:
                continue
            k = t[0].upper()
            v = t[1] if len(t) > 1 else None
            if depth == 0:
                if k in ('WIDTH', 'HEIGHT', 'START', 'STOP'):
                    hdr[k] = v
                elif k == 'BEGIN' and v and v.lower() == 'object':
                    depth = 1; objs += 1; per_obj_kf.append(0)
                else:
                    unknown['top:' + k] += 1
            elif depth == 1:
                if k == 'BEGIN' and v and v.lower() == 'keyframe':
                    depth = 2; kfs += 1; per_obj_kf[-1] += 1
                elif k == 'END':
                    depth = 0
                else:
                    unknown['obj:' + k] += 1
            else:
                if k == 'END':
                    depth = 1
                elif k in ('TIMEINDEX', 'XPOS', 'YPOS', 'SCALE', 'ANGLE'):
                    fields[k].append(float(v))
                elif k in ('COLOUR', 'FLIPHORIZ', 'ALPHA'):
                    fields[k].append(int(v))
                elif k == 'IMAGENAME':
                    imgs[v.lower()] += 1
                else:
                    unknown['kf:' + k] += 1
        used_all |= set(imgs)
        print('==', fn, hdr, 'objects', objs, 'keyframes', kfs, 'kf/obj min/max', min(per_obj_kf), max(per_obj_kf))
        for k in ('TIMEINDEX', 'XPOS', 'YPOS', 'SCALE', 'ANGLE', 'ALPHA'):
            vals = fields[k]
            print('   %-9s min %8.2f max %8.2f' % (k, min(vals), max(vals)))
        print('   COLOUR values', dict(collections.Counter(fields['COLOUR'])), 'FLIPHORIZ', dict(collections.Counter(fields['FLIPHORIZ'])))
        print('   images', dict(imgs))
        missing = [i for i in imgs if i not in bmps]
        if missing:
            print('   MISSING images', missing)
        if unknown:
            print('   UNKNOWN tokens', dict(unknown))
    print('bmps in dir not referenced by any .anim:', sorted(bmps - used_all))

if __name__ == '__main__':
    main(sys.argv[1])
