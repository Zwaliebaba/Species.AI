import sys, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8')
timer_table = """### 4.4 Economy timers at a glance

| Timer | Ticks | Seconds (derived) | Key or rule |
|---|---|---|---|
| production request interval | 2 | 0.2 | `ref.econ.spawn_node.request_interval_ticks` |
| first production timer | 21–40 | 2.1–4.0 | `ref.econ.spawn_node.first_request_ticks` |
| burst pause | 101–150, mean 125.5 | 10.1–15.0 | `ref.econ.spawn_node.pause_ticks` |
| first ownership evaluation | 1–20 | 0.1–2.0 | `ref.econ.spawn_node.first_evaluation_ticks` |
| later ownership evaluations | 21–30 | 2.1–3.0 | `ref.econ.spawn_node.evaluation_interval_ticks` |
| activation delay | 300 | 30.0 | `ref.econ.spawn_node.activation_ticks` |
| spawn-network transit | 11 for the first hop, 10 for each later hop, less same-tick hand-offs | about 1.0–1.1 per hop | R-ECON-039 |
| mode second (wave countdown step, HUD countdown) | 11 | 1.1 | `ref.econ.mode_second_ticks` |
| reinforcement wave, interval 60 | 660; first gap 649 | 66.0; 64.9 | R-ECON-100 |
| Spawn Control elimination | 330 (30 mode seconds) | 33.0 | `ref.econ.spawn_control.elimination_mode_seconds` |
| Comeback Bonus | 450; first 51 | 45.0; 5.1 | `ref.econ.comeback.interval_ticks`, `ref.econ.comeback.first_ticks` |
| Spawn Surge | 50 or 51 | 5.0–5.1 | `ref.econ.spawn_surge.active_ticks` |
| Essence orb Birth | 61 at least | 6.1 | `ref.econ.orb.birth_ticks` |
| Essence orb Floating | 301–500 | 30.1–50.0 | `ref.econ.orb.floating_ticks` |
| Essence orb Ascent | 901 | 90.1 | `ref.econ.orb.ascent_ticks` |
| Collector and Builder idle search | 11 | 1.1 | `ref.econ.collector.idle_retarget_ticks`, `ref.econ.builder.idle_retarget_ticks` |
| Collector delivery | 1 per orb | 0.1 | `ref.econ.collector.delivery_per_tick` |
| bot Collector decision | 11 | 1.1 | `ref.econ.collector.bot_decision_ticks` |
| Worker idle re-plan | 11–20 | 1.1–2.0 | `ref.econ.worker.idle_retarget_ticks` |
| corpse fade | 25 on the ground, 50 in the air | 2.5, 5.0 | R-ECON-011 |
| Population Limiter recount cycle | 91–110 | 9.1–11.0 | `ref.econ.population_limiter.recount_cycle_ticks` |
| Hive orb-to-ants conversion | 10–11 | about 1 | `ref.econ.hive.conversion_ticks` |
| Crawler egg hatch | 151 | 15.1 | `ref.econ.crawler_egg.hatch_ticks` |
| starting-power lock | 300 | 30.0 | `ref.econ.start.power_lock_ticks` |

## 5 Edge cases and interactions
"""
edits = [
 ("## 5 Edge cases and interactions\n", timer_table),
 ("and the Spawn Control score and elimination timers of reference/01 (see DEC-002, DEC-003).",
  "and the Spawn Control score and the Spawn Control elimination countdown of `ref.econ.spawn_control.elimination_mode_seconds` (30 mode seconds, 330 ticks) that reference/01 specifies (see DEC-002, DEC-003)."),
 ("| `ref.econ.elimination.damage_factor` |",
  "| `ref.econ.spawn_control.elimination_mode_seconds` | 30 | mode seconds | 330 ticks without Spawn Nodes; owner: reference/01 | R-ECON-003 | DEC-002 |\n| `ref.econ.elimination.damage_factor` |"),
]
for old, new in edits:
    n = t.count(old)
    if n != 1:
        print('MISS', n, old[:70]); continue
    t = t.replace(old, new)
p.write_text(t, encoding='utf-8')
print('done')
