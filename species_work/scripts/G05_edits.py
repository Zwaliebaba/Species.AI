import sys, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8')
edits = [
 ("one Worker is spawned on each of its ports, directly in the operating-port state",
  "`ref.econ.start.operators_per_port` (1) Worker is spawned on each of its ports, directly in the operating-port state"),
 ("the orb rises slowly (its vertical hover is at least U[0, 0.5) wu/s)",
  "the orb rises slowly (its vertical hover is at least `ref.econ.orb.birth_hover_min_wu_s`, U[0, 0.5) wu/s)"),
 ("| creation | 4 | wobble phase U[0, 10), then three axis wobble rates U[0, 2) |",
  "| creation | 4 | wobble phase `ref.econ.orb.wobble_phase_range` U[0, 10), then three axis wobble rates `ref.econ.orb.wobble_rate_range` U[0, 2) |"),
 ("| wobble, every tick in Birth, Floating and Ascent | 3 | three wobble offsets U[−0.5, 0.5) |",
  "| wobble, every tick in Birth, Floating and Ascent | 3 | three wobble offsets `ref.econ.orb.wobble_offset_range` U[−0.5, 0.5) |"),
 ("with velocity (door front + (U[−0.5, 0.5), U[−0.5, 0.5), U[−0.5, 0.5))) × `ref.econ.spawn_node.door_speed` (10), in wu/s.",
  "with velocity (door front + j) × `ref.econ.spawn_node.door_speed` (10), in wu/s, where j has three components, each drawn from `ref.econ.spawn_node.door_jitter` (U[−0.5, 0.5))."),
 ("with its vertical component replaced by U[0, 1), multiplied by",
  "with its vertical component replaced by a draw from `ref.econ.gate.spawn_vertical_range` (U[0, 1)), multiplied by"),
 ("makes `ref.econ.start.spawn_search_tries` (10) tries of 2 draws each,",
  "makes `ref.econ.start.spawn_search_tries` (10) tries of `ref.econ.start.spawn_search_draws_per_try` (2) draws each,"),
 ("with an eat chance of its size ÷ 2 (reference/02).",
  "with an eat chance of its size ÷ `ref.econ.centipede.eat_chance_divisor` (2) (reference/02)."),
 ("| drawn uniformly from team slots 0 to 3, so possibly a slot not in play (see DEC-305) |",
  "| drawn uniformly from `ref.econ.essence_grove.team_slots` (team slots 0 to 3), so possibly a slot not in play (see DEC-305) |"),
 ("*Figure 3 — Essence orb states in a match. The Reaper Serpent removes the orb of each actor it kills before it exists; a Builder's orbs stay Attached until the Builder dies.*",
  "*Figure 3 — Essence orb states in a match. An actor that the Reaper Serpent kills leaves no orb, and a Builder's orbs stay Attached until the Builder dies.*"),
 ("**R-ECON-{transit_sametick}** (MUST) Structures advance in structure order within a tick, and an orb handed to a structure that comes later in structure order than the handing structure receives that structure's advance in the same tick.",
  "**R-ECON-{transit_sametick}** (MUST) Structures advance in structure order within a tick. When a request or a hand-off places an orb on an edge of a structure that comes later in structure order than the structure making the request or the hand-off, the orb receives that structure's advance in the same tick."),
 ("Given a node 1 hop from the Source Node with the Source Node later in structure order, then the Worker appears 11 ticks after the request;",
  "Given a node 1 hop from the Source Node, then the Worker appears 11 ticks after the request when the Source Node comes earlier in structure order than the node, and 10 ticks after it when the Source Node comes later;"),
]
for old, new in edits:
    n = t.count(old)
    if n != 1:
        print('MISS', n, old[:70]); continue
    t = t.replace(old, new)
p.write_text(t, encoding='utf-8')
print('done')
