import sys, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8')
edits = [
 ("The Comeback Bonus is on by default in every mode, and the host can turn it off (see DEC-073).",
  "The Comeback Bonus is on by default in every mode; the host can turn it off in Spawn Control, Zone Control, Relic Run and Flag Chain (see DEC-073)."),
 ("| `ref.econ.orb.ascent_rise_wu_s` | 2–4 | wu/s | owner: reference/02 |",
  "| `ref.econ.orb.ascent_rise_wu_s` | 2–4 | wu/s | 2 + U[0, 2), one draw per tick; owner: reference/02 |"),
 ("| `ref.econ.orb.birth_max_height_wu` | 10 | wu | above the creation-time ground height |",
  "| `ref.econ.orb.birth_max_height_wu` | 10 | wu | above the creation-time ground height; above it the pull-down draws U[0, 10) per tick |"),
]
for old, new in edits:
    n = t.count(old)
    if n != 1:
        print('MISS', n, old[:70]); continue
    t = t.replace(old, new)
p.write_text(t, encoding='utf-8')
print('done')
