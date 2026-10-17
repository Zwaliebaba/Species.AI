"""C-DBG edit pass 3: fix cross-references and names. Usage: python3 -I C-DBG_edit3.py <file>"""
import sys
p = sys.argv[1]
t = open(p, encoding='utf-8').read()
reps = [
 ("only as a sequenced command (DBG-104).", "only as a sequenced command (DBG-118)."),
 ("| development builds (DBG-120) |", "| development builds (DBG-105) |"),
 ("the per-tick state hash of the test harness (DBG-160) is a test tool.", "the per-tick state hash of the test harness (DBG-144) is a test tool."),
 ("one row per match (DBG-172)", "one row per match (DBG-166)"),
 ("the process commit and working set", "the process's committed memory and working set"),
 ("`vfx.particles.advance`; `vfx.particles.render`; `vfx.shatter.advance`; `vfx.shatter.render`", "`effects.particles.advance`; `effects.particles.render`; `effects.shatter.advance`; `effects.shatter.render`"),
]
for a, b in reps:
    n = t.count(a)
    if n != 1:
        print('MISS', n, a[:70]); continue
    t = t.replace(a, b)
open(p, 'w', encoding='utf-8').write(t)
print('ok')
