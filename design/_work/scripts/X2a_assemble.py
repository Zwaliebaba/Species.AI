#!/usr/bin/env python3
"""Assemble drafts/X2a_asset_catalogue_visual.md from hand-written parts and generated tables.
Usage: python3 -I X2a_assemble.py <scratch dir> <OUT/_work dir>"""
import sys, os
scr, out = sys.argv[1], sys.argv[2]
A = open(os.path.join(scr, 'X2a_partA.md')).read()
G = open(os.path.join(scr, 'X2a_generated_tables.md')).read()
C = open(os.path.join(scr, 'X2a_partC.md')).read()
m = '## [→ 25_textures_and_assets] 25.11'
g1, g2 = G.split(m, 1); g2 = m + g2
k = C.index('## [→ 25_textures_and_assets] 25.12')
doc = A.rstrip() + '\n\n' + g1.rstrip() + '\n\n' + C[:k].rstrip() + '\n\n' + g2.rstrip() + '\n\n' + C[k:]
open(os.path.join(out, 'drafts', 'X2a_asset_catalogue_visual.md'), 'w').write(doc)
print(len(doc))
