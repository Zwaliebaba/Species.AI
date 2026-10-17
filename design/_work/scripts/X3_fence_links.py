#!/usr/bin/env python3
"""X3: check LaserFence link integrity in shipped Multiwinia maps.

LaserFence::RenderAlphas (camera-culled) resets m_nextLaserFenceId to -1 when the link
target is missing or is not a LaserFence, while LaserFence::Advance branches on
m_nextLaserFenceId == -1. Any such dangling link in shipped data would make simulation
state depend on the local camera. This script lists every fence whose 'next' id is not -1
and does not name a LaserFence in the same map file.

Usage: python3 -I X3_fence_links.py <SRC_ROOT>
"""
import glob
import os
import sys

src = sys.argv[1]
total_fences = 0
bad = []
maps = sorted(glob.glob(os.path.join(src, 'data', 'levels', 'mp_*.txt')))
for path in maps:
    in_buildings = False
    types = {}
    fences = []
    for raw in open(path, encoding='latin-1'):
        line = raw.strip()
        if line.startswith('Buildings_StartDefinition'):
            in_buildings = True
            continue
        if line.startswith('Buildings_EndDefinition'):
            in_buildings = False
            continue
        if not in_buildings or not line or line.startswith('#'):
            continue
        tok = line.split()
        if len(tok) < 2:
            continue
        try:
            bid = int(tok[1])
        except ValueError:
            continue
        types[bid] = tok[0]
        if tok[0] == 'LaserFence' and len(tok) > 8:
            fences.append((bid, int(tok[8])))
    for bid, nxt in fences:
        total_fences += 1
        if nxt != -1 and types.get(nxt) != 'LaserFence':
            bad.append((os.path.basename(path), bid, nxt, types.get(nxt, 'MISSING')))
print('maps', len(maps), 'fences', total_fences, 'dangling', len(bad))
for b in bad:
    print('  %s fence %d -> %d (%s)' % b)
