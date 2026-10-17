"""C-DBG edit pass 4: small fidelity additions. Usage: python3 -I C-DBG_edit4.py <file>"""
import sys
p = sys.argv[1]
t = open(p, encoding='utf-8').read()
reps = [
 ("creates a Supply crate at (P.x, `dbg.commands.crate_height` (300 wu), P.z) with a new structure id and its world marker;",
  "creates a Supply crate at (P.x, `dbg.commands.crate_height` (300 wu), P.z) at render detail 1, with a new structure id and its world marker;"),
 ("its last acknowledged frame; the name of the lobby team whose participant id matches, in that team's colour; and its status:",
  "its last acknowledged frame; the name of the team kind (`TeamKind`) of the lobby team whose participant id matches, in that team's colour; and its status:"),
 ("Every node is timed whether or not the view shows it; the reference times",
  "The trees exist whenever markers are compiled in; the reference creates its tree only while its profile window is open. Every node is timed whether or not the view shows it; the reference times"),
]
for a, b in reps:
    n = t.count(a)
    if n != 1:
        print('MISS', n, a[:70]); continue
    t = t.replace(a, b)
open(p, 'w', encoding='utf-8').write(t)
print('ok')
