#!/usr/bin/env python3
"""A2ServerBudget.py: the server-budget model of area A2 (architecture strategy).

It implements the budgets of Design/architecture/04_solution_strategy.md §4.5 and of the records ADR-079 to ADR-088
(tick rate, per-tick budgets and population caps, numeric model, random draws, process architecture, the client
boundary, region partitioning, replication, commands and transport), and computes, at 16 and at 200 stewards:

  - server CPU per settlement (full and abstract fidelity) and per battle, per region and per world, at the evening
    peak of A1's planning and stress cases, against the per-region step budget and the hardware of each stage;
  - bandwidth per client with crowds in view (a home view, a battle view, a valley war, the strategic view), the
    integrated host's upload and the hosted world's egress, at the chosen tick rate and at twice it;
  - storage: the static world, the live state, checkpoints and backups, the retained logs and replays, and the growth
    per world week and over a world lifetime;
  - command-to-display latency at the chosen tick rate and at twice it;
  - the share of the evening peak at which the world's battle cap holds an attack at its gate, for candidate caps.

Usage:
  python3 -I Tools/DesignModels/A2ServerBudget.py [check | report | cpu | bandwidth | storage | latency | caps |
                                                   tables [python]]

  check      (default) compute every figure and every table of figures that the chapter states and compare them
             with STATED and STATED_TABLES; prints one line per figure and exits with status 1 when any differs
  report     every section below in order
  cpu        server CPU per settlement, per battle, per region and per world; sensitivity to the cost budgets
  bandwidth  downstream per client by view, upstream, the integrated host's upload, hosted egress per month
  storage    the storage of a world at 16 and at 200 stewards, and its growth
  latency    command-to-display latency at 10 and at 20 ticks per second
  caps       the world battle cap: share of the evening peak at the cap, for candidate caps
  tables     every table of figures of the chapter as Markdown rows; with python, as the STATED_TABLES literal

Inputs: class Params. Each field names the record or document it comes from; a value that a later area sets is
marked "placeholder" with that area. The per-figure CPU costs are budgets that the implementation must meet, not
measurements: the simulation-core milestone measures them (ADR-080), and a miss is shown by the sensitivity rows.
The world figures come from area A1's model, Tools/DesignModels/A1WorldDensity.py, loaded from this folder by its
path: the settlements and regions per capacity (WLD-004), the battles in progress (WLD-284), the stewards present at
the evening peak and the watched settlements (WLD-244, WLD-280), and A1's placeholders (the plateau population of
600, B1). When A1's model changes, this model follows it. Outputs: text tables on standard output. Python 3 with the
standard library only; no input files besides A1's model.
"""
import importlib.util
import math
import os
import sys
from dataclasses import dataclass, replace

HERE = os.path.dirname(os.path.abspath(__file__))


def load_a1():
  """A1's world-density model, loaded by its explicit path (python3 -I puts no script folder on sys.path)."""
  spec = importlib.util.spec_from_file_location('A1WorldDensity', os.path.join(HERE, 'A1WorldDensity.py'))
  mod = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(mod)
  return mod


A1 = load_a1()


# ---------------------------------------------------------------- inputs
@dataclass(frozen=True)
class Params:
  # tick (ADR-079)
  tick_hz: float = 10.0                   # 1 / ref.sim.tick_period, kept (DEC-227)
  # CPU cost budgets, microseconds per object per tick on one reference core (ADR-080; measured at the
  # simulation-core milestone). The battle figure is the DEC-228 guidance: 4.2 ms per tick at 2,000 units.
  cost_worker_settlement_us: float = 1.2  # a Worker or Leader at settlement work (gather, carry, build, guard)
  cost_figure_battle_us: float = 2.1      # a figure in a battle: search, combat movement, laser bolts, orbs
  cost_figure_march_us: float = 0.4       # a figure of an expedition outside a battle: route and formation following
  cost_settlement_abstract_us: float = 5.0  # placeholder, B2 and D2: one abstract step of 50 us every 10 ticks
  cost_ai_settlement_us: float = 20.0     # placeholder, B7 and D7: AI settlement or deputy, a 1 ms pass every 50 ticks
  cost_ai_battle_side_us: float = 30.0    # placeholder, B7: the command of one side of a battle
  cost_region_fixed_us: float = 30.0      # grids, the region's messages, its seen-layer summary (WLD-243)
  cost_world_fixed_us: float = 100.0      # the world owner: clock, relations, attack registry, keeper
  cost_world_per_settlement_us: float = 2.0
  cost_replication_us: float = 0.05       # encoding one figure's update for one client
  cost_interest_us: float = 40.0          # one client's interest set per tick
  cost_hash_us_per_kb: float = 0.4        # the canonical state hash (ADR-081), 2.5 GB/s
  cost_serialise_us_per_kb: float = 1.0   # the canonical serialisation of a checkpoint (ADR-083), 1 GB/s
  cost_multipliers: tuple = (1.0, 2.0, 4.0)  # sensitivity: the measured costs at 1, 2 and 4 times the budgets
  battle_share: float = 0.25              # share of the hardware's tick budget that battles may take (PIL-021 check)
  figure_memory_bytes: float = 256.0      # one figure in server memory: state, route, search and grid entries
  baseline_bytes: float = 16.0            # one replicated figure's last acknowledged state, per client
  # bodies (ADR-080)
  battle_bodies: int = 400                # pil.battle.bodies_typical (PIL-021)
  battle_bodies_floor: int = 200          # pil.battle.bodies_floor (PIL-021)
  battle_bodies_max: int = 1000           # the battle-size cap proposed for gate B
  region_body_budget: int = 4000          # bodies a region step carries within its budget (the stress population)
  region_body_share: float = 0.75         # wld.scale.region_body_share (WLD-281)
  expedition_figures: int = 100           # placeholder, B4: mean figures of an expedition outside a battle
  orbs_per_battle: int = 40               # mean Essence orbs alive in a battle (200 deaths in 720 s, 130 s each)
  cost_orb_us: float = 0.2
  # per-region and per-tick budgets (ADR-080)
  region_step_budget_ms: float = 10.0     # one region's step on one reference core
  tick_wall_share: float = 0.5            # a tick's whole work finishes within this share of the tick period
  # hardware (ADR-080, gate B): reference-core equivalents available to the server
  integrated_cores: float = 1.0           # the integrated server on the reference test machine (DEC-971)
  community_cores: float = 1.2            # a community host: 2 vCPU at 0.6 reference cores each
  hosted_cores: float = 4.8               # a stage-3 host: 8 vCPU at 0.6 reference cores each
  # replication (ADR-086): quantisation, rates and tiers
  quantum_near_wu: float = 1.0 / 16.0     # position quantum of tier 1
  quantum_far_wu: float = 0.25            # position quantum of tier 2
  max_speed_wus: float = 40.0             # the Worker speed (20 wu/s) under Speed Boost (x2)
  header_bits: int = 6                    # change mask (2) and the run-length skip to the next handle (4)
  tier2_hz: float = 2.5                   # tier-2 update rate
  state_change_share: float = 0.03        # figures per tick whose task, health or facing changes
  state_bytes: float = 4.0
  churn_share: float = 0.05               # figures per second that enter or leave the interest set
  spawn_bytes: float = 16.0
  shots_per_s: float = 1.0 / 3.0          # a figure in combat: one shot per 3 s reload (DEC-201 item 4)
  shot_bytes: float = 8.0
  orb_hz: float = 2.0
  orb_bytes: float = 2.5
  group_bytes: float = 12.0               # tier-3 aggregate record, 1 per second
  fixed_bytes_per_s: float = 900.0        # structures 400, own settlement HUD data 300, public-layer marks 200
  payload_bytes: int = 1200               # datagram payload budget (DEC-207 item 2)
  datagram_overhead_bytes: int = 60       # IP and UDP 28, Species header and authentication tag 32
  client_cap_bytes_per_s: float = 48000.0  # downstream cap per client (ADR-086)
  client_target_bytes_per_s: float = 32000.0  # downstream target per client, typical views
  upstream_control_bytes: float = 24.0    # continuous control state per tick (DEC-203)
  upstream_commands_per_s: float = 0.5
  upstream_command_bytes: float = 24.0
  host_upload_budget_bytes_per_s: float = 500000.0  # integrated host upload budget default, 4 Mbit/s
  # latency (ADR-079, ADR-087)
  frame_ms: float = 1000.0 / 60.0
  jitter_allowance_ms: float = 30.0       # added to the interpolation delay of a remote client
  server_step_ms: float = 20.0            # wall time from a tick's start to its snapshot leaving the server
  rtt_cases_ms: tuple = (0.0, 50.0, 150.0)
  # storage (chapter §4.3; D4 specifies the store)
  settlement_record_kb: float = 8.0       # abstract record, structures, standing orders, timers
  figure_record_bytes: float = 96.0       # canonical record of one figure at full fidelity, exact binary64
  checkpoint_period_s: float = 60.0       # world seconds between checkpoints of a dirty region
  backups_kept: int = 7                   # daily backups of the live state
  battle_record_kb: float = 2.0           # one battle result in the world record
  events_per_settlement_week: float = 5.0  # contacts, claims, freezes and settings changes
  event_record_kb: float = 0.2
  report_entries_per_day: float = 200.0   # placeholder, B6: away-report entries per steward per world day
  report_entry_bytes: float = 48.0
  report_retention_days: float = 30.0
  replay_hz: float = 5.0                  # state-stream replay rate (PIL-140)
  replay_share: float = 0.6               # placeholder, B6: battles with an away steward on a side
  replay_retention_days: float = 7.0      # pil.session.long_absence_days
  replay_compression: float = 1.3         # in-box XPRESS Huffman on bit-packed records (ADR-058)
  battle_minutes_mean: float = 8.0        # the middle of pil.battle.typical_minutes (PIL-047)
  lifetime_days: float = 84.0             # wld.lifetime.default_dedicated (O39)
  dormant_per_year: tuple = (90, 730)     # placeholder, B6 and C2: dormant settlement records per year (16, 200)
  # world loop (A0, A1)
  battles_per_week: float = 6.0           # the middle of pil.world.battles_per_week, A1's planning rate
  battles_per_week_stress: float = 10.0
  cap_candidates_16: tuple = (4, 6, 8)
  cap_candidates_200: tuple = (20, 26, 32)
  world_cap_min: int = 8                  # proposed for gate B (A1's recommendation was 4)
  region_cap: int = 2                     # wld.scale.battles_per_region_max (WLD-285)


P = Params()
CAPACITIES = (16, 200)

# The figures the chapter states. check compares the model with them.
STATED = {
  'cpu_settlement_full_ms': 0.72,
  'cpu_settlement_abstract_ms': 0.025,
  'cpu_battle_ms': 0.91,
  'cpu_battle_max_ms': 2.18,
  'cpu_region_worst_ms': 4.73,
  'cpu_region_budget_ms': 8.43,
  'cpu_16_planning_ms': 13.0,
  'cpu_16_stress_ms': 28.3,
  'cpu_200_planning_ms': 113.1,
  'cpu_200_stress_ms': 167.3,
  'util_16_stress_x4': 1.13,
  'util_200_stress_x4': 6.69,
  'bw_home_kbs': 17.0,
  'bw_battle_kbs': 22.8,
  'bw_war_kbs': 62.3,
  'bw_strategic_kbs': 2.0,
  'bw_battle_20hz_kbs': 36.2,
  'upstream_kbs': 0.9,
  'host_upload_16_mbits': 2.7,
  'egress_200_gb_month': 575,
  'storage_16_mb': 146,
  'storage_200_mb': 1439,
  'growth_16_mb_week': 0.2,
  'growth_200_mb_week': 2.0,
  'replay_mb': 2.6,
  'latency_host_10_ms': 187,
  'latency_remote50_10_ms': 267,
  'latency_host_20_ms': 102,
  'world_cap_16': 8,
  'world_cap_200': 26,
  'egress_16_gb_month': 46,
  'memory_16_mb': 32,
  'memory_200_mb': 300,
  'replay_command_mb': 0.4,
  'tier1_within_cap_war': 806,
  'battles_400_integrated': 27,
  'cpu_200_stress_20hz_cores': 3.35,
}

# Every table of figures in the chapter, as `tables python` prints it.
STATED_TABLES = {
  'unit_cpu': [
    ('settlement at full fidelity, 600 Workers', '0.72', '7.20', '0.72'),
    ('settlement at abstract fidelity, with its AI', '0.025', '0.25', '0.025'),
    ('battle of 400 figures', '0.91', '9.08', '0.91'),
    ('battle of 1,000 figures (the cap)', '2.18', '21.80', '2.18'),
    ('busiest region: 4 settlements at full fidelity, 2 battles', '4.73', '47.26', '4.73'),
    ('region at its body budget of 4,000, all in battle', '8.43', '84.30', '8.43'),
  ],
  'world_cpu': [
    ('16', 'evening peak, planning', '11', '4', '4', '8.1', '3.6', '1.2', '13.0', '0.13', '0.52', '13 %'),
    ('16', 'evening peak, stress', '16', '8', '4', '11.8', '7.3', '1.3', '20.4', '0.20', '0.82', '20 %'),
    ('16', 'session pace, planning', '16', '6', '4', '11.8', '5.4', '1.1', '18.4', '0.18', '0.73', '18 %'),
    ('16', 'all 16 present, stress', '26', '8', '16', '19.2', '7.3', '1.8', '28.3', '0.28', '1.13', '28 %'),
    ('200', 'evening peak, planning', '118', '16', '47', '87.3', '14.5', '11.2', '113.1', '1.13', '4.52', '24 %'),
    ('200', 'evening peak, stress', '177', '26', '47', '131.0', '23.6', '12.7', '167.3', '1.67', '6.69', '35 %'),
  ],
  'bandwidth': [
    ('home view: own settlement', '400', '800', '17.0', '24.7', 'yes'),
    ('battle view: 400 in battle', '600', '600', '22.8', '36.2', 'yes'),
    ('valley war: 2 battles, 4 settlements', '1,500', '2,000', '62.3', '96.3', 'no: 806 stay at tier 1'),
    ('strategic view, 16 stewards', '0', '0', '2.0', '2.6', 'yes'),
    ('strategic view, 200 stewards', '0', '0', '2.9', '3.5', 'yes'),
  ],
  'links': [
    ('integrated host, 3 remote stewards at the evening peak, battle views', '0.5'),
    ('integrated host, 15 remote stewards, battle views', '2.7'),
    ('integrated host, 15 remote stewards, valley-war views at the cap', '5.8'),
    ('community server of 16, all present, battle views', '2.9'),
    ('hosted world of 200, 47 present at the evening peak, battle views', '8.6'),
    ('hosted world of 200, 47 present, valley-war views', '18.0'),
  ],
  'storage': [
    ('16', '6.6', '1.4', '10.1', '2.2', '4.6', '120.7', '0.2', '146', '0.2', '23'),
    ('200', '63.1', '13.3', '93.0', '22.0', '57.6', '1188.7', '1.4', '1439', '2.0', '216'),
  ],
  'memory': [
    ('16', '3.3', '26.2', '0.9', '1.7', '32'),
    ('200', '29.8', '252.3', '2.6', '15.4', '300'),
  ],
  'battle_sizes': [
    ('200', '0.48', '51', '247'),
    ('400', '0.91', '27', '132'),
    ('1,000', '2.18', '11', '55'),
  ],
  'replays': [
    ('state stream at 5 Hz (chosen)', '2.6', 'any client of a release that reads the stream format'),
    ('command stream from the region checkpoint', '0.4', 'the same release only, Level 2'),
  ],
  'latency': [
    ('integrated host', '187', '102'),
    ('remote, 50 ms round trip', '267', '182'),
    ('remote, 150 ms round trip', '367', '282'),
  ],
  'caps': [
    ('16', 'evening peak', '4', '4 %', '23 %', '3.6'),
    ('16', 'evening peak', '6', '0 %', '3 %', '5.4'),
    ('16', 'evening peak', '8', '0 %', '3 %', '7.3'),
    ('16', 'session', '4', '11 %', '37 %', '3.6'),
    ('16', 'session', '6', '2 %', '8 %', '5.4'),
    ('16', 'session', '8', '0 %', '8 %', '7.3'),
    ('200', 'evening peak', '20', '0 %', '11 %', '18.2'),
    ('200', 'evening peak', '26', '0 %', '1 %', '23.6'),
    ('200', 'evening peak', '32', '0 %', '0 %', '29.1'),
  ],
}


# ---------------------------------------------------------------- the world from A1
def world(capacity):
  """Settlements at full occupancy, regions, stewards present at the evening peak, watched settlements (A1)."""
  s = A1.world_size(capacity)
  n = capacity + s['ai']
  _, peak = A1.present_share()
  present = max(1, int(round(capacity * peak)))
  watched = {16: A1.STATED['watched_16'], 200: A1.STATED['watched_200']}[capacity]
  return dict(capacity=capacity, settlements=n, regions=s['regions'], present=present,
              watched_typical=watched[0][1], watched_max=watched[1][1], land_samples=s['land_samples'],
              expeditions=A1.expedition_load(n, P.battles_per_week))


def world_cap(settlements, p=P):
  return max(p.world_cap_min, math.ceil(A1.P.battle_cap_share * settlements))


def battles(capacity, case_name, b, pace='always_on'):
  """Battles in progress at the evening peak (p99) for one of A1's cases (WLD-284)."""
  w = world(capacity)
  case = next(c for c in A1.BATTLE_CASES if c[0] == case_name)
  return A1.battle_case(w['settlements'], b, case, A1.P, pace)['p99']


# ---------------------------------------------------------------- CPU (ADR-080)
def settlement_full_ms(p=P, k=1.0):
  return A1.P.pop_plateau * p.cost_worker_settlement_us * k / 1000.0


def settlement_abstract_ms(p=P, k=1.0):
  return (p.cost_settlement_abstract_us + p.cost_ai_settlement_us) * k / 1000.0


def battle_ms(bodies=None, p=P, k=1.0):
  bodies = p.battle_bodies if bodies is None else bodies
  orbs = p.orbs_per_battle * bodies / p.battle_bodies
  return (bodies * p.cost_figure_battle_us + 2 * p.cost_ai_battle_side_us + orbs * p.cost_orb_us) * k / 1000.0


def region_worst_ms(p=P, k=1.0):
  """The busiest region: its four settlements at full fidelity and two battles of the typical size."""
  return (A1.P.sites_per_region * settlement_full_ms(p, k) + p.region_cap * battle_ms(None, p, k)
          + p.cost_region_fixed_us * k / 1000.0)


def region_budget_ms(p=P, k=1.0):
  """A region at its body budget, every body at the battle cost (the upper bound of a region step)."""
  return (p.region_body_budget * p.cost_figure_battle_us + p.cost_region_fixed_us) * k / 1000.0


def replicated_figures(view):
  v = VIEWS[view]
  return v['t1'] + v['t2'] * P.tier2_hz / P.tick_hz


def world_cpu_ms(capacity, scenario, p=P, k=1.0):
  """Server CPU per tick of a whole world, in ms on one reference core, and its parts."""
  w = world(capacity)
  if scenario == 'planning':
    full, nb, clients, view = w['watched_typical'], battles(capacity, 'planning', p.battles_per_week), w['present'], \
      'battle'
  elif scenario == 'stress':
    full = w['watched_max']
    nb = min(world_cap(w['settlements'], p), battles(capacity, 'stress', p.battles_per_week_stress))
    clients, view = w['present'], 'war'
  elif scenario == 'session':          # the integrated world at the session pace, planning case
    full, nb, clients, view = w['watched_max'], battles(capacity, 'planning', p.battles_per_week, 'session'), \
      w['present'], 'battle'
  elif scenario == 'all_present':      # every steward of an integrated world present: every settlement watched
    full = w['settlements']
    nb = min(world_cap(w['settlements'], p), battles(capacity, 'stress', p.battles_per_week_stress, 'session'))
    clients, view = capacity, 'battle'
  else:
    raise ValueError(scenario)
  n = w['settlements']
  parts = dict(
    full=full * settlement_full_ms(p, k) + full * p.cost_ai_settlement_us * k / 1000.0,
    abstract=(n - full) * settlement_abstract_ms(p, k),
    battles=nb * battle_ms(None, p, k),
    expeditions=w['expeditions'] * p.expedition_figures * p.cost_figure_march_us * k / 1000.0,
    regions=w['regions'] * p.cost_region_fixed_us * k / 1000.0,
    world=(p.cost_world_fixed_us + n * p.cost_world_per_settlement_us) * k / 1000.0,
    replication=clients * (replicated_figures(view) * p.cost_replication_us + p.cost_interest_us) * k / 1000.0,
    persistence=checkpoint_kb(capacity, full, nb, p) * p.cost_serialise_us_per_kb / (p.checkpoint_period_s * p.tick_hz)
    * k / 1000.0 + w['regions'] * region_state_kb(p) * p.cost_hash_us_per_kb / p.tick_hz * k / 1000.0,
  )
  return sum(parts.values()), parts, dict(full=full, battles=nb, clients=clients)


def cores(ms_per_tick, p=P):
  return ms_per_tick / (1000.0 / p.tick_hz)


# ---------------------------------------------------------------- bandwidth (ADR-086)
VIEWS = {
  # t1: tier-1 figures (full rate), t2: tier-2 figures, moving share, figures in combat, orbs alive, tier-3 groups
  'home': dict(t1=400, t2=800, moving=0.8, combat=0, orbs=0, groups=20, label='home view: own settlement'),
  'battle': dict(t1=600, t2=600, moving=0.85, combat=400, orbs=40, groups=20, label='battle view: 400 in battle'),
  'war': dict(t1=1500, t2=2000, moving=0.9, combat=1200, orbs=150, groups=30, label='valley war: 2 battles, 4 '
              'settlements'),
  'strategic': dict(t1=0, t2=0, moving=0.0, combat=0, orbs=0, groups=40, label='strategic view, 16 stewards'),
  'strategic200': dict(t1=0, t2=0, moving=0.0, combat=0, orbs=0, groups=120, label='strategic view, 200 stewards'),
}


def axis_bits(max_move_wu, quantum_wu):
  return math.ceil(math.log2(2.0 * max_move_wu / quantum_wu + 1.0))


def downstream(view, p=P):
  """Bytes per second to one client, and its parts."""
  v = VIEWS[view]
  tick_s = 1.0 / p.tick_hz
  t1_bits = 2 * axis_bits(p.max_speed_wus * tick_s, p.quantum_near_wu) + p.header_bits
  t2_bits = 2 * axis_bits(p.max_speed_wus / p.tier2_hz, p.quantum_far_wu) + p.header_bits
  parts = dict(
    tier1=v['t1'] * (v['moving'] * t1_bits / 8.0 + p.state_change_share * p.state_bytes) * p.tick_hz,
    tier2=v['t2'] * v['moving'] * t2_bits / 8.0 * p.tier2_hz,
    churn=(v['t1'] + v['t2']) * p.churn_share * p.spawn_bytes,
    shots=v['combat'] * p.shots_per_s * p.shot_bytes,
    orbs=v['orbs'] * p.orb_hz * p.orb_bytes,
    groups=v['groups'] * p.group_bytes,
    fixed=p.fixed_bytes_per_s,
  )
  payload = sum(parts.values())
  per_tick = payload / p.tick_hz
  datagrams = p.tick_hz * max(1, math.ceil(per_tick / p.payload_bytes))
  parts['overhead'] = datagrams * p.datagram_overhead_bytes
  return sum(parts.values()), parts


def upstream(p=P):
  payload = p.upstream_control_bytes * p.tick_hz + p.upstream_commands_per_s * p.upstream_command_bytes
  return payload + p.tick_hz * p.datagram_overhead_bytes


def host_upload_mbits(remote_clients, view, p=P):
  return remote_clients * min(p.client_cap_bytes_per_s, downstream(view, p)[0]) * 8.0 / 1e6


def egress_gb_month(capacity, p=P):
  day_share, _ = A1.present_share()
  mean_present = capacity * day_share
  return mean_present * downstream('battle', p)[0] * 86400.0 * 30.0 / 1e9


# ---------------------------------------------------------------- storage (chapter §4.3)
def region_state_kb(p=P):
  """A full-fidelity region: four settlements of A1's plateau population, records exact (ADR-081)."""
  return A1.P.sites_per_region * (A1.P.pop_plateau * p.figure_record_bytes / 1024.0 + p.settlement_record_kb)


def checkpoint_kb(capacity, full, nb, p=P):
  w = world(capacity)
  figures = full * A1.P.pop_plateau + nb * p.battle_bodies
  return w['settlements'] * p.settlement_record_kb + figures * p.figure_record_bytes / 1024.0


def replay_mb(p=P):
  tick_bits = 2 * axis_bits(p.max_speed_wus / p.replay_hz, p.quantum_near_wu) + p.header_bits
  rate = (p.battle_bodies * 0.9 * tick_bits / 8.0 * p.replay_hz + p.battle_bodies * p.shots_per_s * p.shot_bytes
          + p.orbs_per_battle * p.orb_hz * p.orb_bytes + p.battle_bodies * p.churn_share * p.spawn_bytes)
  return rate * p.battle_minutes_mean * 60.0 / p.replay_compression / 1e6


def storage(capacity, p=P):
  """Megabytes of a world at the end of a lifetime of p.lifetime_days, and its growth per world week."""
  w = world(capacity)
  n = w['settlements']
  weeks = p.lifetime_days / 7.0
  battles_week = n * p.battles_per_week / 2.0
  terrain = w['land_samples'] * 8.0 / 1e6
  live = checkpoint_kb(capacity, w['watched_max'], world_cap(n, p), p) * 1024.0 / 1e6
  backups = p.backups_kept * live
  record_week = (battles_week * p.battle_record_kb + n * p.events_per_settlement_week * p.event_record_kb) * 1024 / 1e6
  reports = capacity * p.report_entries_per_day * p.report_entry_bytes * p.report_retention_days / 1e6
  replays = battles_week * p.replay_share * replay_mb(p) * p.replay_retention_days / 7.0
  dormant_week = {16: p.dormant_per_year[0], 200: p.dormant_per_year[1]}[capacity] / 52.0 * p.settlement_record_kb \
    * 1024 / 1e6
  parts = dict(terrain=terrain, live=live, backups=backups, record=record_week * weeks, reports=reports,
               replays=replays, dormant=dormant_week * weeks)
  checkpoint_rate_kbs = checkpoint_kb(capacity, w['watched_max'], world_cap(n, p), p) / p.checkpoint_period_s
  return sum(parts.values()), parts, record_week + dormant_week, checkpoint_rate_kbs


def replay_command_mb(p=P):
  """The command-stream alternative: the region's checkpoint at the battle's start and its inputs for the battle's
  length (steward commands and inbound region messages, 200 bytes per second); valid only on the same release."""
  return (region_state_kb(p) * 1024.0 + 200.0 * p.battle_minutes_mean * 60.0) / 1e6


def memory_mb(capacity, p=P):
  """Server memory of a world at its stress case: figures, terrain heights and normals, replication baselines."""
  w = world(capacity)
  figures = w['watched_max'] * A1.P.pop_plateau + world_cap(w['settlements'], p) * p.battle_bodies
  clients = capacity if capacity == 16 else w['present']
  parts = dict(figures=figures * p.figure_memory_bytes / 1e6,
               settlements=w['settlements'] * p.settlement_record_kb * 1024 / 1e6,
               terrain=w['land_samples'] * (8.0 + 24.0) / 1e6,
               replication=clients * (VIEWS['war']['t1'] + VIEWS['war']['t2']) * p.baseline_bytes / 1e6,
               checkpoint=checkpoint_kb(capacity, w['watched_max'], world_cap(w['settlements'], p), p) * 1024 / 1e6)
  return sum(parts.values()), parts


def affordable_battles(bodies, hw_cores, p=P):
  """Battles of a size that fit in p.battle_share of the hardware's tick budget."""
  return int(p.battle_share * hw_cores * 1000.0 / p.tick_hz // battle_ms(bodies, p))


def tier1_within_cap(view, p=P):
  """The most tier-1 figures of a view that the per-client cap carries, the rest at tier 2."""
  v = VIEWS[view]
  lo, hi = 0, v['t1']
  while lo < hi:
    mid = (lo + hi + 1) // 2
    VIEWS['_probe'] = dict(v, t1=mid, t2=v['t2'] + v['t1'] - mid)
    if downstream('_probe', p)[0] <= p.client_cap_bytes_per_s:
      lo = mid
    else:
      hi = mid - 1
  VIEWS.pop('_probe', None)
  return lo


# ---------------------------------------------------------------- latency (ADR-079, ADR-087)
def latency_ms(tick_hz, rtt_ms, p=P):
  """Mean command-to-display latency: the wait for the next tick, half the round trip each way, the interpolation
  delay (one tick, plus the jitter allowance for a remote client) and half a frame of presentation."""
  tick_ms = 1000.0 / tick_hz
  jitter = p.jitter_allowance_ms if rtt_ms > 0 else 0.0
  step = p.server_step_ms * 10.0 / tick_hz
  return tick_ms / 2.0 + rtt_ms + step + tick_ms + jitter + p.frame_ms


# ---------------------------------------------------------------- the chapter's tables
def r1(x):
  return f'{x:.1f}'


def r2(x):
  return f'{x:.2f}'


def doc_tables(p=P):
  t = {}
  # Table 4.5-A: CPU per settlement and per battle
  t['unit_cpu'] = [
    ('settlement at full fidelity, 600 Workers', r2(settlement_full_ms(p)), r2(settlement_full_ms(p) * p.tick_hz),
     r2(cores(settlement_full_ms(p)) * 100)),
    ('settlement at abstract fidelity, with its AI', f'{settlement_abstract_ms(p):.3f}',
     f'{settlement_abstract_ms(p) * p.tick_hz:.2f}', f'{cores(settlement_abstract_ms(p)) * 100:.3f}'),
    ('battle of 400 figures', r2(battle_ms(None, p)), r2(battle_ms(None, p) * p.tick_hz),
     r2(cores(battle_ms(None, p)) * 100)),
    ('battle of 1,000 figures (the cap)', r2(battle_ms(p.battle_bodies_max, p)),
     r2(battle_ms(p.battle_bodies_max, p) * p.tick_hz), r2(cores(battle_ms(p.battle_bodies_max, p)) * 100)),
    ('busiest region: 4 settlements at full fidelity, 2 battles', r2(region_worst_ms(p)),
     r2(region_worst_ms(p) * p.tick_hz), r2(cores(region_worst_ms(p)) * 100)),
    ('region at its body budget of 4,000, all in battle', r2(region_budget_ms(p)),
     r2(region_budget_ms(p) * p.tick_hz), r2(cores(region_budget_ms(p)) * 100)),
  ]
  # Table 4.5-B: CPU per world
  rows = []
  for cap, scen, label in ((16, 'planning', 'evening peak, planning'), (16, 'stress', 'evening peak, stress'),
                           (16, 'session', 'session pace, planning'), (16, 'all_present', 'all 16 present, stress'),
                           (200, 'planning', 'evening peak, planning'), (200, 'stress', 'evening peak, stress')):
    total, parts, ctx = world_cpu_ms(cap, scen, p)
    hw = p.integrated_cores if cap == 16 else p.hosted_cores
    x4 = world_cpu_ms(cap, scen, p, 4.0)[0]
    rows.append((str(cap), label, str(ctx['full']), str(ctx['battles']), str(ctx['clients']), r1(parts['full']),
                 r1(parts['battles']), r1(total - parts['full'] - parts['battles']), r1(total), r2(cores(total)),
                 r2(cores(x4)), f'{100 * cores(total) / hw:.0f} %'))
  t['world_cpu'] = rows
  # Table 4.5-C: bandwidth per client
  rows = []
  for view in ('home', 'battle', 'war', 'strategic', 'strategic200'):
    b10, parts = downstream(view, p)
    b20 = downstream(view, replace(p, tick_hz=2 * p.tick_hz))[0]
    fits = 'yes' if b10 <= p.client_cap_bytes_per_s else f'no: {tier1_within_cap(view, p):,} stay at tier 1'
    rows.append((VIEWS[view]['label'], f"{VIEWS[view]['t1']:,}", f"{VIEWS[view]['t2']:,}", r1(b10 / 1000.0),
                 r1(b20 / 1000.0), fits))
  t['bandwidth'] = rows
  # Table 4.5-D: upload, egress
  w16, w200 = world(16), world(200)
  t['links'] = [
    ('integrated host, 3 remote stewards at the evening peak, battle views', r1(host_upload_mbits(w16['present'] - 1,
                                                                                                  'battle', p))),
    ('integrated host, 15 remote stewards, battle views', r1(host_upload_mbits(15, 'battle', p))),
    ('integrated host, 15 remote stewards, valley-war views at the cap', r1(host_upload_mbits(15, 'war', p))),
    ('community server of 16, all present, battle views', r1(host_upload_mbits(16, 'battle', p))),
    ('hosted world of 200, 47 present at the evening peak, battle views',
     r1(host_upload_mbits(w200['present'], 'battle', p))),
    ('hosted world of 200, 47 present, valley-war views', r1(host_upload_mbits(w200['present'], 'war', p))),
  ]
  # Table 4.5-E: storage
  rows = []
  for cap in CAPACITIES:
    total, parts, growth, ckpt = storage(cap, p)
    rows.append((str(cap), r1(parts['terrain']), r1(parts['live']), r1(parts['backups']), r1(parts['record']),
                 r1(parts['reports']), r1(parts['replays']), r1(parts['dormant']), f'{total:.0f}', r1(growth),
                 f'{ckpt:.0f}'))
  t['storage'] = rows
  # Table 4.5-E2: memory
  rows = []
  for cap in CAPACITIES:
    total, parts = memory_mb(cap, p)
    rows.append((str(cap), r1(parts['figures']), r1(parts['terrain']), r1(parts['replication']),
                 r1(parts['settlements'] + parts['checkpoint']), f'{total:.0f}'))
  t['memory'] = rows
  # Table 4.5-E3: the battle sizes of PIL-021
  rows = []
  for bodies in (p.battle_bodies_floor, p.battle_bodies, p.battle_bodies_max):
    rows.append((f'{bodies:,}', r2(battle_ms(bodies, p)), str(affordable_battles(bodies, p.integrated_cores, p)),
                 str(affordable_battles(bodies, p.hosted_cores, p))))
  t['battle_sizes'] = rows
  # Table 4.5-E4: replays
  t['replays'] = [
    ('state stream at 5 Hz (chosen)', r1(replay_mb(p)), 'any client of a release that reads the stream format'),
    ('command stream from the region checkpoint', r1(replay_command_mb(p)), 'the same release only, Level 2'),
  ]
  # Table 4.5-F: latency
  rows = []
  for rtt in p.rtt_cases_ms:
    rows.append(('integrated host' if rtt == 0 else f'remote, {rtt:.0f} ms round trip', f'{latency_ms(10, rtt, p):.0f}',
                 f'{latency_ms(20, rtt, p):.0f}'))
  t['latency'] = rows
  # Table 4.5-G: battle caps
  rows = []
  for cap, candidates, paces in ((16, p.cap_candidates_16, ('always_on', 'session')),
                                 (200, p.cap_candidates_200, ('always_on',))):
    w = world(cap)
    for pace in paces:
      for c in candidates:
        pc = replace(A1.P, battle_cap_min=c, battle_cap_share=0.0)
        shares = []
        for name, b in (('planning', p.battles_per_week), ('stress', p.battles_per_week_stress)):
          case = next(x for x in A1.BATTLE_CASES if x[0] == name)
          shares.append(A1.battle_case(w['settlements'], b, case, pc, pace)['at_cap'])
        rows.append((str(cap), 'session' if pace == 'session' else 'evening peak', str(c), f'{100 * shares[0]:.0f} %',
                     f'{100 * shares[1]:.0f} %', r1(c * battle_ms(None, p))))
  t['caps'] = rows
  return t


TABLE_HEADS = {
  'unit_cpu': '| Load | ms per tick | ms of CPU per second | % of one reference core |',
  'world_cpu': '| Stewards | Case | Settlements at full fidelity | Battles in progress | Clients | Settlements ms |'
               ' Battles ms | Other ms | Total ms per tick | Reference cores | Cores at 4 x cost | Of the stage host |',
  'bandwidth': '| View | Tier-1 figures | Tier-2 figures | KB/s at 10 Hz | KB/s at 20 Hz | Within the 48 KB/s cap |',
  'links': '| Link | Mbit/s |',
  'storage': '| Stewards | Terrain MB | Live state MB | Backups MB | World record MB | Away reports MB | Replays MB |'
             ' Dormant MB | Total MB | Growth MB per world week | Checkpoint KB/s |',
  'latency': '| Client | 10 Hz, ms | 20 Hz, ms |',
  'memory': '| Stewards | Figures MB | Terrain heights and normals MB | Replication baselines MB |'
            ' Records and checkpoint buffer MB | Total MB |',
  'battle_sizes': '| Battle bodies | ms per tick | Battles within 25 % of the integrated budget |'
                  ' Battles within 25 % of the stage-3 host |',
  'replays': '| Replay form | MB per battle of 8 minutes | Viewable by |',
  'caps': '| Stewards | Pace | World cap | At the cap, planning | At the cap, stress |'
          ' Battle CPU at the cap, ms per tick |',
}


def print_tables(p=P, as_python=False):
  t = doc_tables(p)
  if as_python:
    print('STATED_TABLES = {')
    for k, rows in t.items():
      print(f"  '{k}': [")
      for row in rows:
        print('    (' + ', '.join(repr(c) for c in row) + '),')
      print('  ],')
    print('}')
    return
  for k, rows in t.items():
    print(f'### {k}\n')
    print(TABLE_HEADS[k])
    print('|' + '---|' * len(rows[0]))
    for row in rows:
      print('| ' + ' | '.join(row) + ' |')
    print()


# ---------------------------------------------------------------- reports
def report_cpu(p=P):
  print('## Server CPU (ADR-080), ms per tick on one reference core at', f'{p.tick_hz:g} ticks per second\n')
  t = doc_tables(p)
  print(TABLE_HEADS['unit_cpu'])
  print('|---|---|---|---|')
  for row in t['unit_cpu']:
    print('| ' + ' | '.join(row) + ' |')
  print()
  print(TABLE_HEADS['world_cpu'])
  print('|' + '---|' * 12)
  for row in t['world_cpu']:
    print('| ' + ' | '.join(row) + ' |')
  print()
  print('Parts of each world case (ms per tick):\n')
  for cap, scen in ((16, 'planning'), (16, 'stress'), (16, 'session'), (16, 'all_present'), (200, 'planning'),
                    (200, 'stress')):
    total, parts, ctx = world_cpu_ms(cap, scen, p)
    print(f'- {cap} {scen}: ' + ', '.join(f'{k} {v:.2f}' for k, v in parts.items()) + f'; total {total:.2f}')
  print('\nSensitivity: world CPU in reference cores at 1, 2 and 4 times the cost budgets\n')
  for cap, scen in ((16, 'all_present'), (200, 'stress')):
    vals = [cores(world_cpu_ms(cap, scen, p, k)[0]) for k in p.cost_multipliers]
    print(f'- {cap} {scen}: ' + ', '.join(f'x{k:g} {v:.2f}' for k, v in zip(p.cost_multipliers, vals)))
  print(f'\n- region step budget {p.region_step_budget_ms:g} ms: the busiest region takes {region_worst_ms(p):.2f} ms,'
        f' a region at its body budget {region_budget_ms(p):.2f} ms; at 2 x cost {region_budget_ms(p, 2):.2f} ms')
  print(f'- tick period {1000 / p.tick_hz:.0f} ms; wall share {p.tick_wall_share:.0%}')
  print()


def report_bandwidth(p=P):
  print('## Bandwidth per client (ADR-086)\n')
  for view in VIEWS:
    total, parts = downstream(view, p)
    print(f"- {VIEWS[view]['label']}: {total / 1000:.1f} KB/s (" + ', '.join(f'{k} {v / 1000:.2f}'
                                                                           for k, v in parts.items()) + ')')
  print(f'- upstream per client: {upstream(p) / 1000:.2f} KB/s')
  for cap in CAPACITIES:
    print(f'- hosted egress, {cap} stewards: {egress_gb_month(cap, p):.0f} GB per 30 days')
  t = doc_tables(p)
  print()
  for row in t['links']:
    print(f'- {row[0]}: {row[1]} Mbit/s')
  print()


def report_storage(p=P):
  print('## Storage (chapter §4.3, D4)\n')
  for cap in CAPACITIES:
    total, parts, growth, ckpt = storage(cap, p)
    print(f'- {cap} stewards, after {p.lifetime_days:g} world days: {total:.0f} MB (' +
          ', '.join(f'{k} {v:.1f}' for k, v in parts.items()) + f'); growth {growth:.2f} MB per world week;'
          f' checkpoint writes {ckpt:.0f} KB/s')
  print(f'- one battle replay, state stream at {p.replay_hz:g} Hz: {replay_mb(p):.1f} MB; command stream'
        f' {replay_command_mb(p):.1f} MB')
  for cap in CAPACITIES:
    total, parts = memory_mb(cap, p)
    print(f'- server memory, {cap} stewards, stress: {total:.0f} MB (' + ', '.join(f'{k} {v:.1f}'
                                                                              for k, v in parts.items()) + ')')
  for bodies in (p.battle_bodies_floor, p.battle_bodies, p.battle_bodies_max):
    print(f'- battles of {bodies}: {affordable_battles(bodies, p.integrated_cores, p)} within 25 % of the integrated'
          f' budget, {affordable_battles(bodies, p.hosted_cores, p)} within 25 % of the stage-3 host')
  print()


def report_latency(p=P):
  print('## Command-to-display latency, mean (ADR-079, ADR-087)\n')
  for rtt in p.rtt_cases_ms:
    print(f'- round trip {rtt:.0f} ms: {latency_ms(10, rtt, p):.0f} ms at 10 Hz, {latency_ms(20, rtt, p):.0f} ms at'
          ' 20 Hz')
  print()


def report_caps(p=P):
  print('## World battle cap (WLD-285): share of the evening peak at the cap\n')
  t = doc_tables(p)
  print(TABLE_HEADS['caps'])
  print('|---|---|---|---|---|---|')
  for row in t['caps']:
    print('| ' + ' | '.join(row) + ' |')
  print()


def report(p=P):
  print('# A2 server-budget model: report\n')
  for cap in CAPACITIES:
    w = world(cap)
    print(f"- {cap} stewards: {w['settlements']} settlements, {w['regions']} regions, {w['present']} present at the"
          f" evening peak, watched {w['watched_typical']} typical and {w['watched_max']} at most;"
          f" world battle cap {world_cap(w['settlements'], p)}")
  print()
  report_cpu(p)
  report_bandwidth(p)
  report_storage(p)
  report_latency(p)
  report_caps(p)


# ---------------------------------------------------------------- check
def computed(p=P):
  w16, w200 = world(16), world(200)
  return {
    'cpu_settlement_full_ms': round(settlement_full_ms(p), 2),
    'cpu_settlement_abstract_ms': round(settlement_abstract_ms(p), 3),
    'cpu_battle_ms': round(battle_ms(None, p), 2),
    'cpu_battle_max_ms': round(battle_ms(p.battle_bodies_max, p), 2),
    'cpu_region_worst_ms': round(region_worst_ms(p), 2),
    'cpu_region_budget_ms': round(region_budget_ms(p), 2),
    'cpu_16_planning_ms': round(world_cpu_ms(16, 'planning', p)[0], 1),
    'cpu_16_stress_ms': round(world_cpu_ms(16, 'all_present', p)[0], 1),
    'cpu_200_planning_ms': round(world_cpu_ms(200, 'planning', p)[0], 1),
    'cpu_200_stress_ms': round(world_cpu_ms(200, 'stress', p)[0], 1),
    'util_16_stress_x4': round(cores(world_cpu_ms(16, 'all_present', p, 4.0)[0]), 2),
    'util_200_stress_x4': round(cores(world_cpu_ms(200, 'stress', p, 4.0)[0]), 2),
    'bw_home_kbs': round(downstream('home', p)[0] / 1000, 1),
    'bw_battle_kbs': round(downstream('battle', p)[0] / 1000, 1),
    'bw_war_kbs': round(downstream('war', p)[0] / 1000, 1),
    'bw_strategic_kbs': round(downstream('strategic', p)[0] / 1000, 1),
    'bw_battle_20hz_kbs': round(downstream('battle', replace(p, tick_hz=20.0))[0] / 1000, 1),
    'upstream_kbs': round(upstream(p) / 1000, 1),
    'host_upload_16_mbits': round(host_upload_mbits(15, 'battle', p), 1),
    'egress_200_gb_month': round(egress_gb_month(200, p)),
    'storage_16_mb': round(storage(16, p)[0]),
    'storage_200_mb': round(storage(200, p)[0]),
    'growth_16_mb_week': round(storage(16, p)[2], 1),
    'growth_200_mb_week': round(storage(200, p)[2], 1),
    'replay_mb': round(replay_mb(p), 1),
    'latency_host_10_ms': round(latency_ms(10, 0, p)),
    'latency_remote50_10_ms': round(latency_ms(10, 50, p)),
    'latency_host_20_ms': round(latency_ms(20, 0, p)),
    'world_cap_16': world_cap(w16['settlements'], p),
    'world_cap_200': world_cap(w200['settlements'], p),
    'egress_16_gb_month': round(egress_gb_month(16, p)),
    'memory_16_mb': round(memory_mb(16, p)[0]),
    'memory_200_mb': round(memory_mb(200, p)[0]),
    'replay_command_mb': round(replay_command_mb(p), 1),
    'tier1_within_cap_war': tier1_within_cap('war', p),
    'battles_400_integrated': affordable_battles(p.battle_bodies, p.integrated_cores, p),
    'cpu_200_stress_20hz_cores': round(2 * cores(world_cpu_ms(200, 'stress', p)[0]), 2),
  }


def check(p=P):
  bad = 0
  for k, v in computed(p).items():
    want = STATED.get(k)
    ok = want == v
    bad += not ok
    print(f"{'ok  ' if ok else 'DIFF'} {k}: model {v}, stated {want}")
  t = doc_tables(p)
  for k, rows in STATED_TABLES.items():
    got = t.get(k)
    ok = got == rows
    bad += not ok
    print(f"{'ok  ' if ok else 'DIFF'} table {k}: {len(rows)} rows")
    if not ok:
      for a, b in zip(got or [], rows):
        if a != b:
          print(f'       model  {a}\n       stated {b}')
  for k in t:
    if k not in STATED_TABLES:
      bad += 1
      print(f'DIFF table {k}: not in STATED_TABLES')
  print(f'\n{len(STATED) + len(STATED_TABLES)} figures and tables, {bad} differences')
  return bad


def main(argv):
  mode = argv[1] if len(argv) > 1 else 'check'
  if mode == 'check':
    return 1 if check() else 0
  if mode == 'report':
    report()
  elif mode == 'cpu':
    report_cpu()
  elif mode == 'bandwidth':
    report_bandwidth()
  elif mode == 'storage':
    report_storage()
  elif mode == 'latency':
    report_latency()
  elif mode == 'caps':
    report_caps()
  elif mode == 'tables':
    print_tables(as_python=len(argv) > 2 and argv[2] == 'python')
  else:
    print(__doc__)
    return 2
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
