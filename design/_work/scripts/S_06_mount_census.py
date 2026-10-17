#!/usr/bin/env python3
"""S06 helper: census of CameraMounts / CameraAnimations blocks in data/levels/*.txt.
Usage: python3 -I S_06_mount_census.py <SRC root>"""
import glob, os, re, sys
SRC = sys.argv[1]
files = sorted(glob.glob(os.path.join(SRC, 'data/levels/*.txt')))
mp = [f for f in files if os.path.basename(f).startswith('mp_')]
cnt = {'player0': 0, 'player1': 0, 'player2': 0, 'player3': 0, 'start': 0}
nomount = []
anim_nonempty = []
for f in files:
    txt = open(f, encoding='latin-1').read()
    m = re.search(r'CameraMounts_StartDefinition(.*?)CameraMounts_EndDefinition', txt, re.S)
    names = set()
    if m:
        for line in m.group(1).splitlines():
            t = line.split('#')[0].split()
            if t:
                names.add(t[0].lower())
    a = re.search(r'CameraAnimations_StartDefinition(.*?)CameraAnimations_EndDefinition', txt, re.S)
    if a and any(l.split('#')[0].strip() for l in a.group(1).splitlines()):
        anim_nonempty.append(os.path.basename(f))
    if f in mp:
        for k in cnt:
            if k in names:
                cnt[k] += 1
        if not any(n.startswith('player') for n in names):
            nomount.append(os.path.basename(f))
print('levels=%d mp=%d' % (len(files), len(mp)))
print('mp mounts:', cnt)
print('mp without player mounts:', nomount)
print('levels with non-empty CameraAnimations:', anim_nonempty)
