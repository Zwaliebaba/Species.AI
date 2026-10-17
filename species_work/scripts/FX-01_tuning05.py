"""FX-01 helper: rewrite the reference/05 tuning table (section 4.1).

Usage: python3 -I FX-01_tuning05.py <05_population_flow.md>
Drops the mirror rows whose values other documents own, edits the notes of the
rows the routes name, adds the Catch-up keys of DEC-124 and the table of keys
cited from other documents. Every edit must match exactly once.
"""
import sys

path = sys.argv[1]
text = open(path, encoding='utf-8').read()

DROP = '''ref.econ.mode_second_ticks ref.econ.spawn_control.elimination_mode_seconds
ref.econ.elimination.damage_factor ref.econ.corpse.health_on_death ref.econ.corpse.fade_ground_per_tick
ref.econ.corpse.fade_air_per_tick ref.econ.port.grid_cell_wu ref.econ.worker.idle_retarget_ticks
ref.econ.worker.port_search_range_wu ref.econ.catch_up.share_factor ref.econ.spawn_surge.active_ticks
ref.econ.crawler.team_limit ref.econ.hive.sortie_interval_ticks ref.econ.orb.spawn_lift_wu
ref.econ.orb.birth_hover_min_wu_s ref.econ.orb.ascent_rise_wu_s ref.econ.orb.wobble_phase_range
ref.econ.essence_grove.drop_rate ref.econ.seed_pod.essence_orbs ref.econ.seed_pod.essence_speed_wu_s
ref.econ.brood_drifter.orb_chance ref.econ.collector.idle_retarget_ticks ref.econ.collector.bot_decision_ticks
ref.econ.collector.speed_wu_s ref.econ.builder.idle_retarget_ticks ref.econ.crawler.orb_search_wu
ref.econ.ant.orb_search_wu ref.econ.centipede.orbs_per_segment ref.econ.centipede.eat_chance_divisor
ref.econ.start.bot_controller_request_wu ref.econ.start.bot_controller_position_wu
ref.econ.start.spawn_search_tries ref.econ.start.spawn_search_draws_per_try ref.econ.comeback.pool
ref.econ.crate.reinforcements_workers ref.econ.crate.reinforcements_speed_wu_s ref.econ.crate.drop_rate_s
ref.econ.crate.max_on_map_divisor_wu ref.econ.seed_pod.workers ref.econ.seed_pod.crawlers
ref.econ.seed_pod.ants ref.econ.seed_pod.centipede_size ref.econ.seed_pod.reaper_serpent_max
ref.econ.abductor.max_drop'''.split()

EDITS = [
    ("Values owned by another reference document are repeated here for the population-flow view and marked \"owner\" in the notes; the owning document's value governs.",
     "Values that another document owns are cited in the rules by that document's key and are not repeated here; the table after this one lists them."),
    ("| `ref.econ.spawn_node.port_count` | 5 | ports | owner: reference/03 |",
     "| `ref.econ.spawn_node.port_count` | 5 | ports | at the port sockets of the mesh |"),
    ("| uniform; 2.1–3.0 s; DEC-002 gives the closed range 20–30, whose lower end only a draw of exactly 0 reaches |",
     "| uniform; 2.1–3.0 s |"),
    ("| uniform; mean 30.5; 2.1–4.0 s; DEC-002 gives 20–40 |", "| uniform; mean 30.5; 2.1–4.0 s |"),
    ("| burst N = ⌊20 × H⌋: 20 to 38 | R-ECON-029, R-ECON-033 | DEC-081 |",
     "| burst N = ⌊20 × H⌋: 20 to 38 | R-ECON-029, R-ECON-033 | DEC-124 |"),
    ("| 0–90 in steps of 10; Spawn Control, Zone Control and Siege; typed default 0 elsewhere | R-ECON-034, R-ECON-037 | DEC-073, DEC-081 |",
     "| 0–90 in steps of 10; Spawn Control, Zone Control and Siege; typed default 0 elsewhere | R-ECON-034, R-ECON-037 | DEC-073, DEC-124 |\n"
     "| `ref.econ.catch_up.curve` | f(d) = d | × (h ÷ 100) | linear in the share deficit d; whether another rising curve, such as √d or d², plays better is the playtest item of DEC-124 | R-ECON-034, R-ECON-035 | DEC-124 |\n"
     "| `ref.econ.catch_up.max_multiplier` | 1 + h ÷ 100 | × burst base | 1.9 at the default option; approached as the team's share falls to 0 | R-ECON-035 | DEC-124 |\n"
     "| `ref.econ.catch_up.modes` | Spawn Control, Zone Control, Siege | modes | the modes with a Catch-up option; a map switches it off with its no-catch-up flag | R-ECON-037 | DEC-073, DEC-124 |"),
    ("| per Spawn Node in the second tutorial; owner: reference/11 |", "| per Spawn Node in the second tutorial |"),
    ("| defender cap; editor hook |", "| defender cap, in steps of 10; editor hook |"),
    ("| default; owner: reference/03 | R-ECON-063 | DEC-454 |", "| default | R-ECON-063 | DEC-454 |"),
    ("| uniform; 9.1–11.0 s; DEC-002 gives 90–110 |", "| uniform; 9.1–11.0 s |"),
    ("| halves the summed excess of team slot 0 | R-ECON-064 | DEC-454 |",
     "| halves the summed excess of team slot 0, by integer division | R-ECON-064 | DEC-454, DEC-851 |"),
    ("| uniform; (120 + U[−30, 30)) / 3 s = 30–50 s; DEC-002 gives 300–500 |",
     "| uniform; (120 + U[−30, 30)) / 3 s = 30–50 s |"),
    ("| option 1–10 in Launch Race and Flag Chain; a cap of 0 means no limit |",
     "| option 1–10 in Launch Race and Flag Chain; a map cap of 0 grants no Transport, and unlimited is its own explicit value (DEC-120) |"),
    ("| Workers per tick | owner: reference/03 |", "| Workers per tick | |"),
]

for old, new in EDITS:
    n = text.count(old)
    if n != 1:
        sys.exit(f'edit matches {n} times: {old[:70]}')
    text = text.replace(old, new)

lines = text.split('\n')
out, dropped = [], []
for line in lines:
    hit = next((k for k in DROP if line.startswith(f'| `{k}` |')), None)
    if hit:
        dropped.append(hit)
        continue
    out.append(line)
missing = sorted(set(DROP) - set(dropped))
if missing or len(dropped) != len(DROP):
    sys.exit(f'drop mismatch: missing {missing}, dropped {len(dropped)} of {len(DROP)}')
text = '\n'.join(out)

CITED = '''
Keys cited from other documents (their values are defined there):

| Owning document | Keys | Used by |
|---|---|---|
| [01 README](01_skirmish_rules_and_modes/README.md), [spawn_control.md](01_skirmish_rules_and_modes/spawn_control.md) | `ref.mode.mode_second`, `ref.mode.spawn_control.elimination_mode_seconds` | R-ECON-003, R-ECON-090 |
| [02 README](02_units/README.md) | `ref.unit.corpse.fade_start`, `ref.unit.corpse.fade_ground_per_tick`, `ref.unit.corpse.fade_air_per_tick` | R-ECON-011 |
| [worker.md](02_units/worker.md) | `ref.unit.worker.retarget_ticks`, `ref.unit.worker.port_search_range` | R-ECON-014, R-ECON-015 |
| [collector.md](02_units/collector.md), [builder.md](02_units/builder.md) | `ref.unit.collector.retarget_ticks`, `ref.unit.collector.bot_think_ticks`, `ref.unit.collector.speed`, `ref.unit.builder.retarget_ticks` | R-ECON-072, R-ECON-079, R-ECON-080, R-ECON-082 |
| [essence_orbs.md](02_units/essence_orbs.md) | `ref.unit.essence_orb.birth_hover_max`, `ref.unit.essence_orb.ascent_rise_min`, `ref.unit.essence_orb.ascent_rise_range`, `ref.unit.essence_orb.wobble_phase_range` | R-ECON-068, R-ECON-071 |
| [creatures.md](02_units/creatures.md) | `ref.unit.crawler.team_limit`, `ref.unit.crawler.orb_search_range`, `ref.unit.ant.search_range`, `ref.unit.centipede.eat_chance_divisor`, `ref.unit.centipede.orbs_per_segment`, `ref.unit.brood_drifter.prefertilise_one_in`, `ref.unit.reaper_serpent.alive_limit`, `ref.unit.seed_pod.workers_min`, `ref.unit.seed_pod.workers_spread`, `ref.unit.seed_pod.crawlers_min`, `ref.unit.seed_pod.crawlers_spread`, `ref.unit.seed_pod.ants_min`, `ref.unit.seed_pod.ants_spread`, `ref.unit.seed_pod.centipede_size_min`, `ref.unit.seed_pod.centipede_size_spread`, `ref.unit.seed_pod.orbs_min`, `ref.unit.seed_pod.orbs_spread`, `ref.unit.seed_pod.orb_speed_min`, `ref.unit.seed_pod.orb_speed_spread` | R-ECON-050, R-ECON-067, R-ECON-083 to R-ECON-085, R-ECON-117 |
| [hazards_and_projectiles.md](02_units/hazards_and_projectiles.md) | `ref.unit.abductor.drop_cap` | R-ECON-117 |
| [trees_and_crate_structures.md](03_buildings_and_world_objects/trees_and_crate_structures.md) | `ref.bldg.hive.release_interval_ticks` | R-ECON-056 |
| [crates_and_powers.md](04_supply_crates_powers_and_weapons/crates_and_powers.md) | `ref.crate.drop.period_default_s`, `ref.crate.drop.map_width_per_crate`, `ref.crate.reinforcements.count_base`, `ref.crate.reinforcements.count_spread`, `ref.crate.reinforcements.speed_min`, `ref.crate.reinforcements.speed_spread`, `ref.crate.spawn_surge.timer_ticks`, `ref.crate.essence_grove.drop_rate_base`, `ref.crate.essence_grove.drop_rate_spread`, `ref.power.comeback.pool`, `ref.power.comeback.pool_basic` | R-ECON-046, R-ECON-067, R-ECON-114, R-ECON-115, R-ECON-117, R-ECON-120 |
| [12 tick timing](12_simulation_model/01_tick_timing_and_main_loop.md) | `ref.sim.tick_period`, `ref.sim.update_slices` | R-ECON-005 |
| [12 containers](12_simulation_model/04_containers_and_update_order.md) | `ref.sim.spawn.orb_lift` | R-ECON-066 |
| [12 space](12_simulation_model/05_space_routing_and_physics.md) | `ref.sim.actor_grid.cell_size` | R-ECON-013 |
| [12 teams](12_simulation_model/06_teams_and_player_commands.md) | `ref.sim.max_teams`, `ref.sim.team.eliminate_damage_factor` | R-ECON-010 |
'''
anchor = '\n### 4.2 Derived throughput'
if text.count(anchor) != 1:
    sys.exit('anchor 4.2 not found once')
text = text.replace(anchor, CITED + anchor)
open(path, 'w', encoding='utf-8').write(text)
print(f'dropped {len(dropped)} rows, applied {len(EDITS)} edits')
