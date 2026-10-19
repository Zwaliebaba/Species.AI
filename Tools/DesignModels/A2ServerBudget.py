#!/usr/bin/env python3
"""A2ServerBudget.py: the server-budget model of area A2 (architecture strategy).

It implements the budgets of Design/architecture/04_solution_strategy.md §4.5 and of the records ADR-079 to ADR-088
(tick rate, per-tick budgets and population caps, numeric model, random draws, process architecture, the client
boundary, region partitioning, replication, commands and transport), revised after challenge round 1 and after owner
gate B (O43 to O48; the QUIC datagram framing of MsQuic, O44 and ADR-088), and computes, at 16 and at 200 stewards:

  - server CPU per settlement (full and abstract fidelity) and per battle, per region and per world, at the evening
    peak of A1's planning and stress cases, with the seen layer, fidelity switches and keyed draws counted;
  - the tick's wall time on each stage's host (region steps as single-threaded jobs scheduled longest first, the
    serial world step, replication jobs, the consistent cut's copy), the cost multiple at which it reaches the wall
    budget and the deadline, at 10 and at 20 ticks per second and for 0.4, 0.6 and 1.0 reference cores per vCPU;
  - the region body budget from the reserved population limits and the per-side battle cap, and the population limit
    that each measured cost multiple allows;
  - bandwidth per client with crowds in view (a home view, a battle view, a battle at the side cap, a valley war,
    skirmish at its design and stress populations, the strategic view), at a one-tick and a two-tick baseline, at 10
    and at 20 ticks per second; the integrated host's upload and hosted egress; the first join's terrain;
  - storage: the static world, the live state and its consistent cut, backups and the pre-migration copy, the
    retained logs and replays, the growth per world week; server memory with the baseline history;
  - command-to-display latency by Monte Carlo (orders, direct control with and without prediction), at 10 and 20 Hz;
  - the share of the evening peak at which the world's battle cap holds an attack at its gate, for candidate caps.

Usage:
  python3 -I Tools/DesignModels/A2ServerBudget.py [check | report | cpu | wall | bandwidth | storage | latency | caps |
                                                   tables [python]]

  check      (default) compute every figure and every table of figures that the chapter states and compare them
             with STATED and STATED_TABLES; prints one line per figure and exits with status 1 when any differs
  report     every section below in order
  cpu        server CPU per settlement, per battle, per region and per world; the region body budget
  wall       the tick wall time per host, the cost multiples at the budget and at the deadline, vCPU factors, 20 Hz
  bandwidth  downstream per client by view and baseline age, upstream, the integrated host's upload, egress, joins
  storage    the storage of a world at 16 and at 200 stewards, its growth, the consistent cut, memory
  latency    command-to-display latency, mean and 95th percentile, at 10 and at 20 ticks per second
  caps       the world battle cap: share of the evening peak at the cap, for candidate caps
  tables     every table of figures of the chapter as Markdown rows; with python, as the STATED_TABLES literal

Inputs: class Params. Each field names the record or document it comes from; a value that a later area sets is
marked "placeholder" with that area. The per-figure CPU costs are budgets that the implementation must meet, not
measurements: the simulation-core milestone measures them first (ADR-080), and the multiples shown beside them
(1.25, 2, 4 and 6, the last the per-frame reading of DBG-205) say what a miss does. The world figures come from area
A1's model, Tools/DesignModels/A1WorldDensity.py, loaded from this folder by its path: the settlements and regions per
capacity (WLD-004), the battles in progress (WLD-284), the stewards present at the evening peak and the watched
settlements (WLD-244, WLD-280), and A1's placeholder plateau population of 600 (B1). When A1's model changes, this
model follows it. Every random value of the latency Monte Carlo comes from Python's seeded generator (seed 1), so a
run gives the same figures on every Python 3 version. Outputs: text tables on standard output. Python 3 with the
standard library only; no input files besides A1's model.
"""
import importlib.util
import math
import os
import random
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
  # CPU cost budgets, microseconds per object per tick on one reference core (ADR-080). The battle figure is the
  # whole-tick reading of DBG-205 (4.2 ms per tick at 2,000 units); the per-frame reading is 6 times it.
  cost_worker_settlement_us: float = 1.2  # a Worker or Leader at settlement work (gather, carry, build, guard)
  cost_figure_battle_us: float = 2.1      # a figure in a battle: search, combat movement, laser bolts, orbs
  cost_figure_march_us: float = 0.4       # a figure of an expedition outside a battle
  cost_settlement_abstract_us: float = 5.0  # placeholder, B2 and D2: one abstract step of 50 us every 10 ticks
  cost_ai_settlement_us: float = 20.0     # placeholder, B7 and D7: AI settlement or deputy; range 20 to 200
  cost_ai_battle_side_us: float = 30.0    # placeholder, B7: the command of one side of a battle
  cost_region_fixed_us: float = 30.0      # grids and the region's messages
  cost_world_fixed_us: float = 100.0      # the world owner: clock, relations, attack registry, keeper
  cost_world_per_settlement_us: float = 2.0
  cost_seen_us: float = 0.5               # placeholder, D9: one body's stamp of its settlement's seen layer
  seen_other_period_ticks: int = 5        # bodies outside battles update the seen layer every 5 ticks
  cost_draw_us: float = 0.03              # one keyed draw with SipHash-2-4 over 40 bytes (ADR-082)
  draws_battle_figure: float = 3.0        # simulation draws per battle figure per tick (R-SIM-630)
  draws_worker: float = 0.1               # per Worker at settlement work per tick
  draws_orb: float = 3.0                  # per Essence orb per tick
  switch_cost_us: float = 2.0             # materialising one figure when a settlement turns full fidelity
  fidelity_hold_s: float = 60.0           # a settlement stays full fidelity this long after its last view (ADR-085)
  watched_max_per_present: int = 6        # wld.scale.watched_max_per_present (WLD-244)
  cost_replication_us: float = 0.05       # encoding one figure's update for one client
  cost_interest_us: float = 40.0          # one client's interest set per tick
  cost_hash_us_per_kb: float = 0.4        # the canonical state hash (ADR-081), 2.5 GB/s
  hash_period_ticks: int = 10             # a Release server hashes each region every 10 ticks (ADR-081 item 8)
  cost_serialise_us_per_kb: float = 1.0   # the canonical serialisation of a cut, 1 GB/s, on the store thread
  cost_copy_us_per_kb: float = 0.1        # copying an owner's state at the cut, 10 GB/s, on the tick's workers
  cost_multipliers: tuple = (1.0, 1.25, 2.0, 4.0, 6.0)  # measured costs as multiples of the budgets
  confirm_tolerance: float = 1.25         # ADR-080's confirmation tolerance
  # bodies (ADR-080): reserved population limits and the per-side battle cap
  pop_limit_max: int = 600                # L: the most Workers and Leaders that B1 may give a settlement
  battle_side_max: int = 600              # S: the most figures of one side inside a battlefield (S = L)
  battle_bodies: int = 400                # pil.battle.bodies_typical (PIL-021)
  battle_bodies_floor: int = 200          # pil.battle.bodies_floor (PIL-021)
  region_step_budget_ms: float = 12.5     # one region's step on one reference core
  expedition_figures: int = 100           # placeholder, B4: mean figures of an expedition outside a battle
  orbs_per_battle: int = 40               # mean Essence orbs alive in a battle of 400
  cost_orb_us: float = 0.2
  # the tick's wall time (ADR-080 item 6)
  tick_wall_share: float = 0.5            # the budget: a tick's work within half its period
  serial_overhead_ms: float = 0.2         # message delivery and the barrier between phases, per tick
  # hosts: (worker threads that run jobs, reference cores per thread)
  vcpu_factor: float = 0.6                # reference cores per rented vCPU, until DEC-971's machine measures it
  vcpu_factors: tuple = (0.4, 0.6, 1.0)
  integrated_cores: float = 1.0           # the integrated server's average budget on the reference test machine
  # replication (ADR-086)
  quantum_near_wu: float = 1.0 / 16.0
  quantum_far_wu: float = 0.25
  max_speed_wus: float = 40.0             # the Worker speed (20 wu/s) under Speed Boost (x2)
  header_bits: int = 6
  tier2_hz: float = 2.5
  state_change_share: float = 0.03
  state_bytes: float = 4.0
  churn_share: float = 0.05
  spawn_bytes: float = 16.0
  shots_per_s: float = 1.0 / 3.0
  shot_bytes: float = 8.0
  orb_hz: float = 2.0
  orb_bytes: float = 2.5
  group_bytes: float = 12.0
  fixed_bytes_per_s: float = 900.0
  # A state update's records per QUIC datagram: the DATAGRAM frame carries at most 1,200 bytes (Species' 8-byte
  # header and 1,192 bytes of records), which fits MsQuic's largest datagram at its starting MTU of 1,288 bytes on
  # IPv6 (1,207 bytes), so no state datagram depends on path MTU discovery (ADR-088 item 3).
  payload_bytes: int = 1192
  # Bytes per state datagram besides its records, computed from the framing, not measured (M4 measures it):
  # IPv6 40 and UDP 8; the QUIC 1-RTT short header 14 (1 flags byte, the 9-byte connection id that MsQuic generates
  # without a server-id prefix, the 4-byte packet number that MsQuic writes; RFC 9000 §17.3.1); the DATAGRAM frame's
  # type and 2-byte length 3 (RFC 9221 §4); the AEAD tag 16 (RFC 9001 §5.3); Species' header 8 (the update's tick and
  # its baseline tick; sequencing and acknowledgement are QUIC's). On IPv4 it is 69; the model takes IPv6's 89.
  datagram_overhead_bytes: int = 89
  client_cap_bytes_per_s: float = 48000.0  # world client cap (ADR-086)
  skirmish_cap_bytes_per_s: float = 64000.0  # skirmish client cap (ADR-086)
  client_target_bytes_per_s: float = 32000.0
  upstream_control_bytes: float = 24.0
  upstream_commands_per_s: float = 0.5
  upstream_command_bytes: float = 24.0
  host_upload_budget_bytes_per_s: float = 500000.0  # integrated host upload budget default, 4 Mbit/s
  baseline_history_ticks: int = 32        # sent states kept per client until acknowledged
  baseline_bytes: float = 16.0
  join_blob_bytes_per_s: float = 256000.0  # a joining client's blob rate (terrain, snapshot)
  terrain_bytes_per_sample: float = 2.0   # 16-bit quantised heights to clients (D8 decides)
  # latency (ADR-079, ADR-084, ADR-087)
  frame_ms: float = 1000.0 / 60.0
  jitter_allowance_ms: float = 30.0
  net_jitter_mean_ms: float = 5.0         # exponential network jitter each way
  server_step_16_ms: float = 20.0         # wall time from a tick's start to its update leaving the server, 16
  latency_samples: int = 100000
  # storage (chapter §4.3.1; D4)
  settlement_record_kb: float = 8.0
  figure_record_bytes: float = 96.0
  figure_memory_bytes: float = 256.0
  checkpoint_period_s: float = 60.0       # world seconds between consistent cuts
  battle_checkpoint_period_s: float = 10.0  # a cut while a battle runs, at most every 10 s
  backups_kept: int = 7
  premigration_copies: int = 1            # kept per.migration.keep_days (14 real days) after a migration
  battle_record_kb: float = 2.0
  events_per_settlement_week: float = 5.0
  event_record_kb: float = 0.2
  report_entries_per_day: float = 200.0   # placeholder, B6
  report_entry_bytes: float = 48.0
  report_retention_days: float = 30.0
  replay_hz: float = 5.0
  replay_mask_bits: int = 2               # per figure record: seen by side 1, seen by side 2 (ADR-086)
  replay_decision_bytes: float = 1000.0   # the deputy's and AI's decisions with their standing orders, per battle
  replay_share: float = 0.6               # placeholder, B6: battles with an away steward on a side
  replay_retention_days: float = 7.0
  battle_minutes_mean: float = 8.0
  lifetime_days: float = 84.0             # wld.lifetime.default_dedicated (O39)
  dormant_per_year: tuple = (90, 730)     # placeholder, B6 and C2: dormant settlement records per year (16, 200)
  # world loop (A0, A1)
  battles_per_week: float = 6.0
  battles_per_week_stress: float = 10.0
  cap_candidates_16: tuple = (4, 6, 8)
  cap_candidates_200: tuple = (20, 26, 32)
  world_cap_min: int = 8
  region_cap: int = 2


P = Params()
CAPACITIES = (16, 200)
HOSTS = (   # name, label, capacity, case, threads, reference cores per thread (None: the vCPU factor)
  ('integrated', 'integrated, reference test machine: tick thread and 2 workers', 16, 'all_present', 3, 1.0),
  ('community', 'community, 2 vCPU: 2 workers', 16, 'all_present', 2, None),
  ('community4', 'community, 4 vCPU: 3 workers, 1 for tick, I/O and store', 16, 'all_present', 3, None),
  ('stage3_8', 'stage 3, 8 vCPU: 7 workers, 1 for tick, I/O and store', 200, 'stress', 7, None),
  ('stage3_16', 'stage 3, 16 vCPU: 15 workers', 200, 'stress', 15, None),
)


def body_budget(p=P):
  """Region body budget: the four sites' reserved population limits and two battles' visiting sides (ADR-080)."""
  return A1.P.sites_per_region * p.pop_limit_max + p.region_cap * p.battle_side_max


def battle_max(p=P):
  return 2 * p.battle_side_max


# ---------------------------------------------------------------- the world from A1
def world(capacity):
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
  w = world(capacity)
  case = next(c for c in A1.BATTLE_CASES if c[0] == case_name)
  return A1.battle_case(w['settlements'], b, case, A1.P, pace)['p99']


# ---------------------------------------------------------------- CPU (ADR-080)
def settlement_full_ms(p=P, k=1.0):
  """A settlement at full fidelity: its Workers, their seen-layer stamps and draws, its AI."""
  pop = A1.P.pop_plateau
  per = p.cost_worker_settlement_us + p.cost_seen_us / p.seen_other_period_ticks + p.draws_worker * p.cost_draw_us
  return (pop * per + p.cost_ai_settlement_us) * k / 1000.0


def settlement_abstract_ms(p=P, k=1.0):
  return (p.cost_settlement_abstract_us + p.cost_ai_settlement_us) * k / 1000.0


def battle_ms(bodies=None, p=P, k=1.0):
  """A battle: its figures with their seen-layer stamps and draws every tick, its orbs, its two sides' command."""
  bodies = p.battle_bodies if bodies is None else bodies
  orbs = p.orbs_per_battle * bodies / p.battle_bodies
  per = p.cost_figure_battle_us + p.cost_seen_us + p.draws_battle_figure * p.cost_draw_us
  return (bodies * per + orbs * (p.cost_orb_us + p.draws_orb * p.cost_draw_us)
          + 2 * p.cost_ai_battle_side_us) * k / 1000.0


def figure_battle_us(p=P):
  return p.cost_figure_battle_us + p.cost_seen_us + p.draws_battle_figure * p.cost_draw_us


def figure_settlement_us(p=P):
  return p.cost_worker_settlement_us + p.cost_seen_us / p.seen_other_period_ticks + p.draws_worker * p.cost_draw_us


def region_worst_ms(p=P, k=1.0):
  """The busiest region: four settlements at full fidelity and two typical battles."""
  return (A1.P.sites_per_region * settlement_full_ms(p, k) + p.region_cap * battle_ms(None, p, k)
          + p.cost_region_fixed_us * k / 1000.0)


def region_budget_ms(p=P, k=1.0, limit=None, side=None):
  """The upper bound of a region step under the caps: two battles at their side caps, each at a settlement of the
  region (its population limit defending, a full side attacking), and the region's other two settlements at work."""
  lim = p.pop_limit_max if limit is None else limit
  side = p.battle_side_max if side is None else side
  in_battle = p.region_cap * (lim + side)
  at_work = (A1.P.sites_per_region - p.region_cap) * lim
  return (in_battle * figure_battle_us(p) + at_work * figure_settlement_us(p) + p.cost_region_fixed_us) * k / 1000.0


def pop_limit_for(k, p=P, tolerance=1.0, side=None):
  """The largest population limit, a multiple of 50, whose region bound holds the region step budget at k times the
  cost budgets and the given tolerance; side None keeps the battle side cap equal to the limit."""
  best = 0
  for lim in range(50, 2001, 50):
    if region_budget_ms(p, k * tolerance, lim, lim if side is None else side) <= p.region_step_budget_ms:
      best = lim
  return best


def replicated_figures(view):
  v = VIEWS[view]
  return v['t1'] + v['t2'] * P.tier2_hz / P.tick_hz


def scenario(capacity, scen, p=P):
  w = world(capacity)
  if scen == 'planning':
    return w['watched_typical'], battles(capacity, 'planning', p.battles_per_week), w['present'], 'battle'
  if scen == 'stress':
    nb = min(world_cap(w['settlements'], p), battles(capacity, 'stress', p.battles_per_week_stress))
    return w['watched_max'], nb, w['present'], 'war'
  if scen == 'session':
    return w['watched_max'], battles(capacity, 'planning', p.battles_per_week, 'session'), w['present'], 'battle'
  if scen == 'all_present':
    nb = min(world_cap(w['settlements'], p), battles(capacity, 'stress', p.battles_per_week_stress, 'session'))
    return w['settlements'], nb, capacity, 'battle'
  raise ValueError(scen)


def switch_ms(capacity, clients, p=P, k=1.0):
  """Fidelity switches per tick: each present steward's view moves at most 6 settlements, each held 60 s."""
  per_s = clients * p.watched_max_per_present / p.fidelity_hold_s
  return per_s * A1.P.pop_plateau * p.switch_cost_us * k / 1000.0 / p.tick_hz


def world_cpu_ms(capacity, scen, p=P, k=1.0):
  """Server CPU per tick of a whole world, in ms on one reference core, and its parts."""
  w = world(capacity)
  full, nb, clients, view = scenario(capacity, scen, p)
  n = w['settlements']
  parts = dict(
    full=full * settlement_full_ms(p, k),
    abstract=(n - full) * settlement_abstract_ms(p, k),
    battles=nb * battle_ms(None, p, k),
    expeditions=w['expeditions'] * p.expedition_figures * (p.cost_figure_march_us + p.cost_seen_us) * k / 1000.0,
    regions=w['regions'] * p.cost_region_fixed_us * k / 1000.0,
    world=(p.cost_world_fixed_us + n * p.cost_world_per_settlement_us) * k / 1000.0,
    switches=switch_ms(capacity, clients, p, k),
    replication=clients * (replicated_figures(view) * p.cost_replication_us + p.cost_interest_us) * k / 1000.0,
    persistence=(checkpoint_kb(capacity, full, nb, p) * p.cost_serialise_us_per_kb
                 / (p.checkpoint_period_s * p.tick_hz)
                 + w['regions'] * region_state_kb(p) * p.cost_hash_us_per_kb / p.hash_period_ticks) * k / 1000.0,
  )
  return sum(parts.values()), parts, dict(full=full, battles=nb, clients=clients)


def draws_ms(capacity, scen, p=P):
  """The keyed draws of one tick (ADR-082), ms: battle figures, Essence orbs and Workers at full fidelity."""
  full, nb, _, _ = scenario(capacity, scen, p)
  n = (nb * p.battle_bodies * p.draws_battle_figure + nb * p.orbs_per_battle * p.draws_orb
       + full * A1.P.pop_plateau * p.draws_worker)
  return n * p.cost_draw_us / 1000.0


def overload_lever_ms(capacity, p=P):
  """What the overload lever of ADR-085 item 9 recovers at the stress case: settlements watched only as neighbours
  of a present steward's own (every watched one but the present stewards' own) at abstract fidelity."""
  w = world(capacity)
  neighbours = w['watched_max'] - w['present']
  return neighbours * (settlement_full_ms(p) - settlement_abstract_ms(p))


def cores(ms_per_tick, p=P):
  return ms_per_tick / (1000.0 / p.tick_hz)


# ---------------------------------------------------------------- wall time (ADR-080 item 6, ADR-083 item 6)
def lpt(jobs, workers):
  loads = [0.0] * workers
  for j in sorted(jobs, reverse=True):
    i = loads.index(min(loads))
    loads[i] += j
  return max(loads)


def tick_phases(capacity, scen, p=P, k=1.0, cut=True):
  """The jobs of one tick, in reference ms: region steps (watched settlements and battles packed into the fewest
  regions, the worst for balance), the serial world step, the replication jobs per client, and on the tick of a
  consistent cut the copy of every region's state."""
  w = world(capacity)
  total, parts, ctx = world_cpu_ms(capacity, scen, p, k)
  regions = w['regions']
  full, nb = ctx['full'], ctx['battles']
  abstract = w['settlements'] - full
  exp_each = parts['expeditions'] / regions
  sw_each = parts['switches'] / regions
  hash_each = region_state_kb(p) * p.cost_hash_us_per_kb / p.hash_period_ticks * k / 1000.0
  jobs = []
  for _ in range(regions):
    f = min(A1.P.sites_per_region, full)
    full -= f
    a = min(A1.P.sites_per_region - f, abstract)
    abstract -= a
    b = min(p.region_cap, nb)
    nb -= b
    jobs.append(f * settlement_full_ms(p, k) + a * settlement_abstract_ms(p, k) + b * battle_ms(None, p, k)
                + p.cost_region_fixed_us * k / 1000.0 + exp_each + sw_each + hash_each)
  _, _, clients, view = scenario(capacity, scen, p)
  rep = [(replicated_figures(view) * p.cost_replication_us + p.cost_interest_us) * k / 1000.0] * clients
  ser = [region_state_kb(p) * p.cost_copy_us_per_kb * k / 1000.0] * regions if cut else []
  serial = parts['world'] + p.serial_overhead_ms
  return jobs, serial, rep, ser


def wall_ms(capacity, scen, threads, speed, p=P, k=1.0, cut=True):
  jobs, serial, rep, ser = tick_phases(capacity, scen, p, k, cut)
  return (lpt(jobs, threads) + serial + lpt(rep, threads) + (lpt(ser, threads) if ser else 0.0)) / speed


def multiple_at(capacity, scen, threads, speed, limit_ms, p=P):
  """The cost multiple at which the tick's wall time reaches limit_ms (bisection)."""
  lo, hi = 0.01, 64.0
  for _ in range(60):
    mid = (lo + hi) / 2
    if wall_ms(capacity, scen, threads, speed, p, mid) <= limit_ms:
      lo = mid
    else:
      hi = mid
  return lo


def host_speed(h, p=P, factor=None):
  return h[5] if h[5] is not None else (p.vcpu_factor if factor is None else factor)


# ---------------------------------------------------------------- bandwidth (ADR-086)
VIEWS = {
  'home': dict(t1=400, t2=800, moving=0.8, combat=0, orbs=0, groups=20, label='home view: own settlement'),
  'battle': dict(t1=600, t2=600, moving=0.85, combat=400, orbs=40, groups=20, label='battle view: 400 in battle'),
  'battle_cap': dict(t1=1200, t2=600, moving=0.85, combat=1200, orbs=120, groups=20,
                     label='battle at the side cap: 1,200 in battle'),
  'war': dict(t1=1500, t2=2000, moving=0.9, combat=1200, orbs=150, groups=30, label='valley war: 2 battles, 4 '
              'settlements'),
  'skirmish': dict(t1=1500, t2=500, moving=0.85, combat=1600, orbs=150, groups=10, cap='skirmish',
                   label='skirmish, design population 2,000'),
  'skirmish_stress': dict(t1=1500, t2=2500, moving=0.85, combat=3000, orbs=300, groups=10, cap='skirmish',
                          label='skirmish, stress population 4,000'),
  'strategic': dict(t1=0, t2=0, moving=0.0, combat=0, orbs=0, groups=40, label='strategic view, 16 stewards'),
  'strategic200': dict(t1=0, t2=0, moving=0.0, combat=0, orbs=0, groups=120, label='strategic view, 200 stewards'),
}


def axis_bits(max_move_wu, quantum_wu):
  return math.ceil(math.log2(2.0 * max_move_wu / quantum_wu + 1.0))


def view_cap(view, p=P):
  return p.skirmish_cap_bytes_per_s if VIEWS[view].get('cap') == 'skirmish' else p.client_cap_bytes_per_s


def downstream(view, p=P, age_ticks=1, v=None):
  """Bytes per second to one client, and its parts, with the baseline age_ticks old."""
  v = VIEWS[view] if v is None else v
  tick_s = 1.0 / p.tick_hz
  t1_bits = 2 * axis_bits(p.max_speed_wus * tick_s * age_ticks, p.quantum_near_wu) + p.header_bits
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


def tier1_within_cap(view, p=P):
  v = VIEWS[view]
  lo, hi = 0, v['t1']
  cap = view_cap(view, p)
  while lo < hi:
    mid = (lo + hi + 1) // 2
    if downstream(view, p, v=dict(v, t1=mid, t2=v['t2'] + v['t1'] - mid))[0] <= cap:
      lo = mid
    else:
      hi = mid - 1
  return lo


def upstream(p=P):
  payload = p.upstream_control_bytes * p.tick_hz + p.upstream_commands_per_s * p.upstream_command_bytes
  return payload + p.tick_hz * p.datagram_overhead_bytes


def host_upload_mbits(remote_clients, view, p=P):
  return remote_clients * min(view_cap(view, p), downstream(view, p)[0]) * 8.0 / 1e6


def egress_gb_month(capacity, p=P):
  day_share, _ = A1.present_share()
  return capacity * day_share * downstream('battle', p)[0] * 86400.0 * 30.0 / 1e9


def join_terrain_s(capacity, p=P):
  return world(capacity)['land_samples'] * p.terrain_bytes_per_sample / p.join_blob_bytes_per_s


# ---------------------------------------------------------------- storage and memory (chapter §4.3.1)
def region_state_kb(p=P):
  return A1.P.sites_per_region * (A1.P.pop_plateau * p.figure_record_bytes / 1024.0 + p.settlement_record_kb)


def checkpoint_kb(capacity, full, nb, p=P):
  w = world(capacity)
  figures = full * A1.P.pop_plateau + nb * p.battle_bodies
  return w['settlements'] * p.settlement_record_kb + figures * p.figure_record_bytes / 1024.0


def cut_kb(capacity, p=P):
  """A consistent cut of every owner at one tick, at the stress case (ADR-083 item 9)."""
  w = world(capacity)
  return checkpoint_kb(capacity, w['watched_max'], world_cap(w['settlements'], p), p)


def replay_mb(p=P):
  tick_bits = 2 * axis_bits(p.max_speed_wus / p.replay_hz, p.quantum_near_wu) + p.header_bits + p.replay_mask_bits
  rate = (p.battle_bodies * 0.9 * tick_bits / 8.0 * p.replay_hz + p.battle_bodies * p.shots_per_s * p.shot_bytes
          + p.orbs_per_battle * p.orb_hz * p.orb_bytes + p.battle_bodies * p.churn_share * p.spawn_bytes)
  return (rate * p.battle_minutes_mean * 60.0 + p.replay_decision_bytes) / 1e6


def replay_command_mb(p=P):
  return (region_state_kb(p) * 1024.0 + 200.0 * p.battle_minutes_mean * 60.0) / 1e6


def storage(capacity, p=P):
  """Megabytes of a world at the end of a lifetime of p.lifetime_days, its growth per world week, and the
  write rate of consistent cuts every 60 s in KB/s."""
  w = world(capacity)
  n = w['settlements']
  weeks = p.lifetime_days / 7.0
  battles_week = n * p.battles_per_week / 2.0
  terrain = w['land_samples'] * 8.0 / 1e6
  live = cut_kb(capacity, p) * 1024.0 / 1e6
  backups = p.backups_kept * live
  record_week = (battles_week * p.battle_record_kb + n * p.events_per_settlement_week * p.event_record_kb) * 1024 / 1e6
  reports = capacity * p.report_entries_per_day * p.report_entry_bytes * p.report_retention_days / 1e6
  replays = battles_week * p.replay_share * replay_mb(p) * p.replay_retention_days / 7.0
  dormant_week = {16: p.dormant_per_year[0], 200: p.dormant_per_year[1]}[capacity] / 52.0 * p.settlement_record_kb \
    * 1024 / 1e6
  premigration = p.premigration_copies * (terrain + live + record_week * weeks + reports)
  parts = dict(terrain=terrain, live=live, backups=backups, premigration=premigration, record=record_week * weeks,
               reports=reports, replays=replays, dormant=dormant_week * weeks)
  return sum(parts.values()), parts, record_week + dormant_week, cut_kb(capacity, p) / p.checkpoint_period_s


def memory_mb(capacity, p=P):
  w = world(capacity)
  figures = w['watched_max'] * A1.P.pop_plateau + world_cap(w['settlements'], p) * p.battle_bodies
  clients = capacity if capacity == 16 else w['present']
  history = clients * p.baseline_history_ticks * (VIEWS['war']['t1'] + VIEWS['war']['t2']) * p.baseline_bytes
  parts = dict(figures=figures * p.figure_memory_bytes / 1e6, terrain=w['land_samples'] * (8.0 + 24.0) / 1e6,
               history=history / 1e6,
               records=(w['settlements'] * p.settlement_record_kb + cut_kb(capacity, p)) * 1024 / 1e6)
  return sum(parts.values()), parts


def affordable_battles(bodies, hw_cores, p=P):
  return int(0.25 * hw_cores * 1000.0 / p.tick_hz // battle_ms(bodies, p))


# ---------------------------------------------------------------- latency (ADR-079, ADR-084, ADR-087)
def latency(tick_hz, rtt_ms, kind, step_ms, p=P):
  """Mean and 95th percentile of command-to-display latency, ms. kind: 'order' (a discrete command, sent at once),
  'direct' (direct control without prediction, its control state sent at once on a change, ADR-087 item 5) or
  'predicted' (the steward's directly controlled unit drawn from the client's prediction, ADR-084 item 6)."""
  rng = random.Random(1)
  tick = 1000.0 / tick_hz
  frame = p.frame_ms
  out = []
  for _ in range(p.latency_samples):
    t = rng.uniform(0, frame)                            # input sampled at the next frame
    if kind == 'predicted':
      t += rng.uniform(0, frame) + frame + frame / 2     # the next frame, one queued frame, half a scan-out
      out.append(t)
      continue
    if rtt_ms > 0:
      t += rtt_ms / 2 + rng.expovariate(1 / p.net_jitter_mean_ms)
    t += rng.uniform(0, tick)                            # the wait for the next server tick
    t += step_ms
    if rtt_ms > 0:
      t += rtt_ms / 2 + rng.expovariate(1 / p.net_jitter_mean_ms)
    t += tick + (p.jitter_allowance_ms if rtt_ms > 0 else 0.0)   # interpolation reaches the new state
    t += rng.uniform(0, frame) + frame + frame / 2
    out.append(t)
  out.sort()
  return sum(out) / len(out), out[int(0.95 * len(out))]


def step_ms_for(capacity, p=P, tick_hz=10.0):
  if capacity == 16:
    return p.server_step_16_ms * 10.0 / tick_hz
  h = next(x for x in HOSTS if x[0] == 'stage3_16')
  return wall_ms(200, 'stress', h[4], host_speed(h, p), p, 1.0, cut=False)


# ---------------------------------------------------------------- the chapter's tables
def r1(x):
  return f'{x:.1f}'


def r2(x):
  return f'{x:.2f}'


def pct(x):
  return f'{100 * x:.0f} %'


def doc_tables(p=P):
  t = {}
  t['unit_cpu'] = [
    ('settlement at full fidelity, 600 Workers', r2(settlement_full_ms(p)), r2(settlement_full_ms(p) * p.tick_hz)),
    ('settlement at abstract fidelity, with its AI', f'{settlement_abstract_ms(p):.3f}',
     f'{settlement_abstract_ms(p) * p.tick_hz:.2f}'),
    ('battle of 400 figures', r2(battle_ms(None, p)), r2(battle_ms(None, p) * p.tick_hz)),
    (f'battle at the side cap: {battle_max(p):,} figures', r2(battle_ms(battle_max(p), p)),
     r2(battle_ms(battle_max(p), p) * p.tick_hz)),
    ('busiest region: 4 settlements at full fidelity, 2 battles of 400', r2(region_worst_ms(p)),
     r2(region_worst_ms(p) * p.tick_hz)),
    (f'region bound: 2 battles of {battle_max(p):,}, 2 settlements at work ({body_budget(p):,} bodies)',
     r2(region_budget_ms(p)),
     r2(region_budget_ms(p) * p.tick_hz)),
  ]
  rows = []
  for cap, scen, label in ((16, 'planning', 'evening peak, planning'), (16, 'stress', 'evening peak, stress'),
                           (16, 'session', 'session pace, planning'), (16, 'all_present', 'all 16 present, stress'),
                           (200, 'planning', 'evening peak, planning'), (200, 'stress', 'evening peak, stress')):
    total, parts, ctx = world_cpu_ms(cap, scen, p)
    rows.append((str(cap), label, str(ctx['full']), str(ctx['battles']), str(ctx['clients']), r1(parts['full']),
                 r1(parts['battles']), r1(total - parts['full'] - parts['battles']), r1(total), r2(cores(total)),
                 r2(2 * cores(total))))
  t['world_cpu'] = rows
  rows = []
  for h in HOSTS:
    name, label, cap, scen, threads, _ = h
    sp = host_speed(h, p)
    w10 = wall_ms(cap, scen, threads, sp, p)
    budget10 = p.tick_wall_share * 1000.0 / p.tick_hz
    m_budget = multiple_at(cap, scen, threads, sp, budget10, p)
    m_dead = multiple_at(cap, scen, threads, sp, 1000.0 / p.tick_hz, p)
    m20 = multiple_at(cap, scen, threads, sp, budget10 / 2, p)
    lo = multiple_at(cap, scen, threads, host_speed(h, p, p.vcpu_factors[0]), budget10, p)
    rows.append((label, r1(w10), f'x{m_budget:.2f}', f'x{m_dead:.2f}', f'x{m20:.2f}',
                 '—' if h[5] is not None else f'x{lo:.2f}'))
  t['wall'] = rows
  rows = []
  for k in p.cost_multipliers:
    i_hw = next(x for x in HOSTS if x[0] == 'integrated')
    s3 = next(x for x in HOSTS if x[0] == 'stage3_8')
    s16 = next(x for x in HOSTS if x[0] == 'stage3_16')
    rows.append((f'x{k:g}', r2(cores(world_cpu_ms(16, 'all_present', p, k)[0])),
                 r1(wall_ms(16, 'all_present', i_hw[4], i_hw[5], p, k)),
                 r2(cores(world_cpu_ms(200, 'stress', p, k)[0])),
                 r1(wall_ms(200, 'stress', s3[4], host_speed(s3, p), p, k)),
                 r1(wall_ms(200, 'stress', s16[4], host_speed(s16, p), p, k)),
                 f'{pop_limit_for(k, p):,}', f'{pop_limit_for(k, p, side=p.battle_side_max):,}'))
  t['sensitivity'] = rows
  rows = []
  for view in ('home', 'battle', 'battle_cap', 'war', 'skirmish', 'skirmish_stress', 'strategic', 'strategic200'):
    b10 = downstream(view, p)[0]
    b2 = downstream(view, p, age_ticks=2)[0]
    b20 = downstream(view, replace(p, tick_hz=2 * p.tick_hz))[0]
    cap = view_cap(view, p)
    fits = 'yes' if b2 <= cap else f'no: {tier1_within_cap(view, p):,} stay at tier 1'
    rows.append((VIEWS[view]['label'], f"{VIEWS[view]['t1']:,}", f"{VIEWS[view]['t2']:,}", r1(b10 / 1000.0),
                 r1(b2 / 1000.0), r1(b20 / 1000.0), f'{cap / 1000:.0f}', fits))
  t['bandwidth'] = rows
  w16, w200 = world(16), world(200)
  p20 = replace(p, tick_hz=2 * p.tick_hz)
  t['links'] = [
    ('integrated host, 3 remote stewards at the evening peak, battle views',
     r1(host_upload_mbits(w16['present'] - 1, 'battle', p)), r1(host_upload_mbits(w16['present'] - 1, 'battle', p20))),
    ('integrated host, 15 remote stewards, battle views', r1(host_upload_mbits(15, 'battle', p)),
     r1(host_upload_mbits(15, 'battle', p20))),
    ('integrated host, 15 remote stewards, valley-war views at the cap', r1(host_upload_mbits(15, 'war', p)),
     r1(host_upload_mbits(15, 'war', p20))),
    ('integrated skirmish host, 1 remote player at the design population', r1(host_upload_mbits(1, 'skirmish', p)),
     r1(host_upload_mbits(1, 'skirmish', p20))),
    ('community server of 16, all present, battle views', r1(host_upload_mbits(16, 'battle', p)),
     r1(host_upload_mbits(16, 'battle', p20))),
    ('hosted world of 200, 47 present at the evening peak, battle views',
     r1(host_upload_mbits(w200['present'], 'battle', p)), r1(host_upload_mbits(w200['present'], 'battle', p20))),
    ('hosted world of 200, 47 present, valley-war views', r1(host_upload_mbits(w200['present'], 'war', p)),
     r1(host_upload_mbits(w200['present'], 'war', p20))),
  ]
  rows = []
  for cap in CAPACITIES:
    total, parts, growth, ckpt = storage(cap, p)
    rows.append((str(cap), r1(parts['terrain']), r1(parts['live']), r1(parts['backups']), r1(parts['premigration']),
                 r1(parts['record']), r1(parts['reports']), r1(parts['replays']), r1(parts['dormant']),
                 f'{total:.0f}', r1(growth), f'{ckpt:.0f}'))
  t['storage'] = rows
  rows = []
  for cap in CAPACITIES:
    total, parts = memory_mb(cap, p)
    rows.append((str(cap), r1(parts['figures']), r1(parts['terrain']), r1(parts['history']), r1(parts['records']),
                 f'{total:.0f}'))
  t['memory'] = rows
  rows = []
  for bodies in (p.battle_bodies_floor, p.battle_bodies, battle_max(p)):
    rows.append((f'{bodies:,}', r2(battle_ms(bodies, p)), str(affordable_battles(bodies, p.integrated_cores, p)),
                 str(affordable_battles(bodies, 16 * p.vcpu_factor, p))))
  t['battle_sizes'] = rows
  t['replays'] = [
    ('state stream at 5 Hz with side masks and the decision track (chosen)', r1(replay_mb(p)),
     'each side, filtered by its seen layer; any client of a release that reads the format'),
    ('command stream from the cut before the battle', r1(replay_command_mb(p)), 'the same release only, Level 2'),
  ]
  rows = []
  for label, cap, rtt in (('integrated host', 16, 0.0), ('remote, 50 ms round trip', 16, 50.0),
                          ('remote, 150 ms round trip', 16, 150.0), ('remote at 50 ms, world of 200', 200, 50.0)):
    cells = [label]
    for kind in ('order', 'direct', 'predicted'):
      for hz in (10.0, 20.0):
        m, q = latency(hz, rtt, kind, step_ms_for(cap, p, hz), p)
        cells.append(f'{m:.0f} / {q:.0f}')
    rows.append(tuple(cells))
  t['latency'] = rows
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
        rows.append((str(cap), 'session' if pace == 'session' else 'evening peak', str(c), pct(shares[0]),
                     pct(shares[1]), r1(c * battle_ms(None, p))))
  t['caps'] = rows
  return t


TABLE_HEADS = {
  'unit_cpu': '| Load | ms per tick | ms of CPU per second |',
  'world_cpu': '| Stewards | Case | Settlements at full fidelity | Battles in progress | Clients | Settlements ms |'
               ' Battles ms | Other ms | Total ms per tick | Reference cores | Reference cores at 20 Hz |',
  'wall': '| Host and threads | Wall ms per tick at the budgets | Multiple at the 50 ms budget |'
          ' Multiple at the 100 ms deadline | Multiple at the 25 ms budget of 20 Hz | Multiple at 0.4 per vCPU |',
  'sensitivity': '| Measured cost | 16 all present, reference cores | Integrated wall ms |'
                 ' 200 stress, reference cores | 8 vCPU wall ms | 16 vCPU wall ms |'
                 ' Population limit, side cap equal | Population limit, side cap 600 |',
  'bandwidth': '| View | Tier-1 figures | Tier-2 figures | KB/s, 1-tick baseline | KB/s, 2-tick baseline |'
               ' KB/s at 20 Hz | Cap KB/s | Within the cap at a 2-tick baseline |',
  'links': '| Link | Mbit/s at 10 Hz | Mbit/s at 20 Hz |',
  'storage': '| Stewards | Terrain MB | Live state MB | Backups MB | Pre-migration copy MB | World record MB |'
             ' Away reports MB | Replays MB | Dormant MB | Total MB | Growth MB per world week |'
             ' Cut writes KB/s |',
  'memory': '| Stewards | Figures MB | Terrain heights and normals MB | Baseline history MB |'
            ' Records and cut buffer MB | Total MB |',
  'battle_sizes': '| Battle bodies | ms per tick | Battles within 25 % of the integrated budget |'
                  ' Battles within 25 % of a 16-vCPU host |',
  'replays': '| Replay form | MB per battle of 8 minutes | Viewable by |',
  'latency': '| Client | Order, 10 Hz | Order, 20 Hz | Direct control without prediction, 10 Hz |'
             ' The same, 20 Hz | Predicted direct control, 10 Hz | Predicted, 20 Hz |',
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


def print_table(t, k):
  print(TABLE_HEADS[k])
  print('|' + '---|' * len(t[k][0]))
  for row in t[k]:
    print('| ' + ' | '.join(row) + ' |')
  print()


# ---------------------------------------------------------------- reports
def report_cpu(p=P):
  t = doc_tables(p)
  print(f'## Server CPU (ADR-080), ms per tick on one reference core at {p.tick_hz:g} ticks per second\n')
  print(f'- region body budget {body_budget(p):,} = 4 x {p.pop_limit_max} reserved + {p.region_cap} x'
        f' {p.battle_side_max} visiting sides; a battle at most {battle_max(p):,}; region step budget'
        f' {p.region_step_budget_ms:g} ms; the region bound {region_budget_ms(p):.2f} ms, headroom'
        f' x{p.region_step_budget_ms / region_budget_ms(p):.2f}; design limit with the tolerance'
        f' {pop_limit_for(1.0, p, p.confirm_tolerance)}\n')
  print_table(t, 'unit_cpu')
  print_table(t, 'world_cpu')
  for cap, scen in ((16, 'planning'), (16, 'stress'), (16, 'session'), (16, 'all_present'), (200, 'planning'),
                    (200, 'stress')):
    total, parts, ctx = world_cpu_ms(cap, scen, p)
    print(f'- {cap} {scen}: ' + ', '.join(f'{k} {v:.2f}' for k, v in parts.items()) + f'; total {total:.2f}')
  for ai in (20.0, 100.0, 200.0):
    pa = replace(p, cost_ai_settlement_us=ai)
    print(f'- AI at {ai:g} us per settlement per tick: 200 stress {world_cpu_ms(200, "stress", pa)[0]:.1f} ms per tick')
  print()


def report_wall(p=P):
  t = doc_tables(p)
  print('## Tick wall time (ADR-080 item 6): region jobs longest first, serial world step, replication, the cut\n')
  print_table(t, 'wall')
  print_table(t, 'sensitivity')
  for f in p.vcpu_factors:
    s3 = next(x for x in HOSTS if x[0] == 'stage3_8')
    print(f'- {f:g} reference cores per vCPU: 8-vCPU wall {wall_ms(200, "stress", s3[4], f, p):.1f} ms;'
          f' average {cores(world_cpu_ms(200, "stress", p)[0]) / (8 * f):.0%} of the host')
  print()


def report_bandwidth(p=P):
  t = doc_tables(p)
  print('## Bandwidth per client (ADR-086)\n')
  print_table(t, 'bandwidth')
  print_table(t, 'links')
  print(f'- upstream per client: {upstream(p) / 1000:.2f} KB/s')
  for cap in CAPACITIES:
    print(f'- hosted egress, {cap} stewards: {egress_gb_month(cap, p):.0f} GB per 30 days; first join terrain at'
          f' {p.join_blob_bytes_per_s / 1000:.0f} KB/s: {join_terrain_s(cap, p):.0f} s')
  print()


def report_storage(p=P):
  t = doc_tables(p)
  print('## Storage and memory (chapter §4.3.1, D4)\n')
  print_table(t, 'storage')
  print_table(t, 'memory')
  print_table(t, 'replays')
  for cap in CAPACITIES:
    kb = cut_kb(cap, p)
    copy_ms = kb * p.cost_copy_us_per_kb / 1000
    ser_ms = kb * p.cost_serialise_us_per_kb / 1000
    print(f'- consistent cut, {cap} stewards: {kb / 1024:.1f} MB; the copy {copy_ms:.1f} ms in reference cores on the'
          f' tick\'s workers, the serialisation {ser_ms:.1f} ms on the store thread')
  print()


def report_latency(p=P):
  t = doc_tables(p)
  print('## Command-to-display latency, mean / 95th percentile, ms (ADR-079, ADR-084, ADR-087)\n')
  print_table(t, 'latency')


def report_caps(p=P):
  t = doc_tables(p)
  print('## World battle cap (WLD-285): share of the evening peak at the cap\n')
  print_table(t, 'caps')
  print_table(t, 'battle_sizes')


def report(p=P):
  print('# A2 server-budget model: report\n')
  for cap in CAPACITIES:
    w = world(cap)
    print(f"- {cap} stewards: {w['settlements']} settlements, {w['regions']} regions, {w['present']} present at the"
          f" evening peak, watched {w['watched_typical']} typical and {w['watched_max']} at most;"
          f" world battle cap {world_cap(w['settlements'], p)}")
  print()
  report_cpu(p)
  report_wall(p)
  report_bandwidth(p)
  report_storage(p)
  report_latency(p)
  report_caps(p)


# ---------------------------------------------------------------- check
def computed(p=P):
  w16, w200 = world(16), world(200)
  s3 = next(x for x in HOSTS if x[0] == 'stage3_8')
  s16 = next(x for x in HOSTS if x[0] == 'stage3_16')
  return {
    'body_budget': body_budget(p),
    'battle_max': battle_max(p),
    'cpu_settlement_full_ms': round(settlement_full_ms(p), 2),
    'cpu_battle_ms': round(battle_ms(None, p), 2),
    'cpu_region_budget_ms': round(region_budget_ms(p), 2),
    'region_headroom': round(p.region_step_budget_ms / region_budget_ms(p), 2),
    'cpu_16_stress_ms': round(world_cpu_ms(16, 'all_present', p)[0], 1),
    'cpu_200_stress_ms': round(world_cpu_ms(200, 'stress', p)[0], 1),
    'cores_200_stress': round(cores(world_cpu_ms(200, 'stress', p)[0]), 2),
    'wall_8vcpu_ms': round(wall_ms(200, 'stress', s3[4], host_speed(s3, p), p), 1),
    'wall_16vcpu_ms': round(wall_ms(200, 'stress', s16[4], host_speed(s16, p), p), 1),
    'multiple_8vcpu_budget': round(multiple_at(200, 'stress', s3[4], host_speed(s3, p), 50.0, p), 2),
    'multiple_16vcpu_budget': round(multiple_at(200, 'stress', s16[4], host_speed(s16, p), 50.0, p), 2),
    'multiple_16vcpu_deadline': round(multiple_at(200, 'stress', s16[4], host_speed(s16, p), 100.0, p), 2),
    'region_headroom_design_limit': pop_limit_for(1.0, p, p.confirm_tolerance),
    'pop_limit_x2': pop_limit_for(2.0, p),
    'bw_battle_kbs': round(downstream('battle', p)[0] / 1000, 1),
    'bw_skirmish_kbs': round(downstream('skirmish', p)[0] / 1000, 1),
    'upstream_kbs': round(upstream(p) / 1000, 1),
    'host_upload_15_mbits': round(host_upload_mbits(15, 'battle', p), 1),
    'egress_200_gb_month': round(egress_gb_month(200, p)),
    'join_terrain_200_s': round(join_terrain_s(200, p)),
    'storage_16_mb': round(storage(16, p)[0]),
    'storage_200_mb': round(storage(200, p)[0]),
    'cut_200_mb': round(cut_kb(200, p) / 1024, 1),
    'memory_200_mb': round(memory_mb(200, p)[0]),
    'replay_mb': round(replay_mb(p), 1),
    'world_cap_16': world_cap(w16['settlements'], p),
    'world_cap_200': world_cap(w200['settlements'], p),
    'cut_rate_battle_16_kbs': round(cut_kb(16, p) / p.battle_checkpoint_period_s),
    'draws_200_stress_ms': round(draws_ms(200, 'stress', p), 2),
    'overload_lever_200_ms': round(overload_lever_ms(200, p), 1),
    'overload_lever_200_share': round(overload_lever_ms(200, p) / world_cpu_ms(200, 'stress', p)[0], 2),
  }


# The figures the chapter and the records state. check compares the model with them.
STATED = {
  'body_budget': 3600,
  'battle_max': 1200,
  'cpu_settlement_full_ms': 0.8,
  'cpu_battle_ms': 1.15,
  'cpu_region_budget_ms': 8.05,
  'region_headroom': 1.55,
  'cpu_16_stress_ms': 32.0,
  'cpu_200_stress_ms': 185.5,
  'cores_200_stress': 1.85,
  'wall_8vcpu_ms': 49.2,
  'wall_16vcpu_ms': 22.7,
  'multiple_8vcpu_budget': 1.02,
  'multiple_16vcpu_budget': 2.22,
  'multiple_16vcpu_deadline': 4.46,
  'region_headroom_design_limit': 700,
  'pop_limit_x2': 450,
  'bw_battle_kbs': 23.4,
  'bw_skirmish_kbs': 51.0,
  'upstream_kbs': 1.1,
  'host_upload_15_mbits': 2.8,
  'egress_200_gb_month': 590,
  'join_terrain_200_s': 62,
  'storage_16_mb': 207,
  'storage_200_mb': 2052,
  'cut_200_mb': 12.7,
  'memory_200_mb': 382,
  'replay_mb': 3.6,
  'world_cap_16': 8,
  'world_cap_200': 26,
  'cut_rate_battle_16_kbs': 141,
  'draws_200_stress_ms': 1.35,
  'overload_lever_200_ms': 101.0,
  'overload_lever_200_share': 0.54,
}

# Every table of figures in the chapter, as `tables python` prints it.
STATED_TABLES = {
  'unit_cpu': [
    ('settlement at full fidelity, 600 Workers', '0.80', '8.02'),
    ('settlement at abstract fidelity, with its AI', '0.025', '0.25'),
    ('battle of 400 figures', '1.15', '11.48'),
    ('battle at the side cap: 1,200 figures', '3.32', '33.23'),
    ('busiest region: 4 settlements at full fidelity, 2 battles of 400', '5.53', '55.32'),
    ('region bound: 2 battles of 1,200, 2 settlements at work (3,600 bodies)', '8.05', '80.50'),
  ],
  'world_cpu': [
    ('16', 'evening peak, planning', '11', '4', '4', '8.8', '4.6', '1.3', '14.7', '0.15', '0.29'),
    ('16', 'evening peak, stress', '16', '8', '4', '12.8', '9.2', '1.4', '23.4', '0.23', '0.47'),
    ('16', 'session pace, planning', '16', '6', '4', '12.8', '6.9', '1.2', '20.9', '0.21', '0.42'),
    ('16', 'all 16 present, stress', '26', '8', '16', '20.8', '9.2', '2.0', '32.0', '0.32', '0.64'),
    ('200', 'evening peak, planning', '118', '16', '47', '94.6', '18.4', '12.3', '125.2', '1.25', '2.50'),
    ('200', 'evening peak, stress', '177', '26', '47', '141.9', '29.8', '13.7', '185.5', '1.85', '3.71'),
  ],
  'wall': [
    ('integrated, reference test machine: tick thread and 2 workers', '12.0', 'x4.20', 'x8.42', 'x2.09', '—'),
    ('community, 2 vCPU: 2 workers', '28.7', 'x1.75', 'x3.52', 'x0.87', 'x1.17'),
    ('community, 4 vCPU: 3 workers, 1 for tick, I/O and store', '20.1', 'x2.51', 'x5.05', 'x1.25', 'x1.67'),
    ('stage 3, 8 vCPU: 7 workers, 1 for tick, I/O and store', '49.2', 'x1.02', 'x2.04', 'x0.50', 'x0.68'),
    ('stage 3, 16 vCPU: 15 workers', '22.7', 'x2.22', 'x4.46', 'x1.10', 'x1.48'),
  ],
  'sensitivity': [
    ('x1', '0.32', '12.0', '1.85', '49.2', '22.7', '900', '1,150'),
    ('x1.25', '0.40', '15.0', '2.32', '61.4', '28.3', '700', '800'),
    ('x2', '0.64', '23.9', '3.71', '98.1', '45.1', '450', '350'),
    ('x4', '1.28', '47.6', '7.42', '195.9', '89.8', '200', '0'),
    ('x6', '1.92', '71.3', '11.13', '293.6', '134.5', '150', '0'),
  ],
  'bandwidth': [
    ('home view: own settlement', '400', '800', '17.6', '18.4', '25.2', '48', 'yes'),
    ('battle view: 400 in battle', '600', '600', '23.4', '24.7', '37.4', '48', 'yes'),
    ('battle at the side cap: 1,200 in battle', '1,200', '600', '42.9', '45.5', '69.1', '48', 'yes'),
    ('valley war: 2 battles, 4 settlements', '1,500', '2,000', '63.8', '68.0', '98.6', '48', 'no: 747 stay at tier 1'),
    ('skirmish, design population 2,000', '1,500', '500', '51.0', '55.1', '85.0', '64', 'yes'),
    ('skirmish, stress population 4,000', '1,500', '2,500', '70.5', '73.7', '104.6', '64', 'no: 1,198 stay at tier 1'),
    ('strategic view, 16 stewards', '0', '0', '2.3', '2.3', '3.2', '48', 'yes'),
    ('strategic view, 200 stewards', '0', '0', '3.2', '3.2', '4.1', '48', 'yes'),
  ],
  'links': [
    ('integrated host, 3 remote stewards at the evening peak, battle views', '0.6', '0.9'),
    ('integrated host, 15 remote stewards, battle views', '2.8', '4.5'),
    ('integrated host, 15 remote stewards, valley-war views at the cap', '5.8', '5.8'),
    ('integrated skirmish host, 1 remote player at the design population', '0.4', '0.5'),
    ('community server of 16, all present, battle views', '3.0', '4.8'),
    ('hosted world of 200, 47 present at the evening peak, battle views', '8.8', '14.1'),
    ('hosted world of 200, 47 present, valley-war views', '18.0', '18.0'),
  ],
  'storage': [
    ('16', '6.6', '1.4', '10.1', '14.8', '2.2', '4.6', '167.1', '0.2', '207', '0.2', '23'),
    ('200', '63.1', '13.3', '93.0', '156.0', '22.0', '57.6', '1645.3', '1.4', '2052', '2.0', '216'),
  ],
  'memory': [
    ('16', '3.3', '26.2', '28.7', '1.7', '60'),
    ('200', '29.8', '252.3', '84.2', '15.4', '382'),
  ],
  'battle_sizes': [
    ('200', '0.60', '41', '397'),
    ('400', '1.15', '21', '209'),
    ('1,200', '3.32', '7', '72'),
  ],
  'replays': [
    ('state stream at 5 Hz with side masks and the decision track (chosen)', '3.6',
     'each side, filtered by its seen layer; any client of a release that reads the format'),
    ('command stream from the cut before the battle', '0.4', 'the same release only, Level 2'),
  ],
  'latency': [
    ('integrated host', '212 / 258', '127 / 152', '212 / 258', '127 / 152', '42 / 53', '42 / 53'),
    ('remote, 50 ms round trip', '302 / 350', '217 / 245', '302 / 350', '217 / 245', '42 / 53', '42 / 53'),
    ('remote, 150 ms round trip', '402 / 450', '317 / 345', '402 / 450', '317 / 345', '42 / 53', '42 / 53'),
    ('remote at 50 ms, world of 200', '304 / 352', '229 / 258', '304 / 352', '229 / 258', '42 / 53', '42 / 53'),
  ],
  'caps': [
    ('16', 'evening peak', '4', '4 %', '23 %', '4.6'),
    ('16', 'evening peak', '6', '0 %', '3 %', '6.9'),
    ('16', 'evening peak', '8', '0 %', '3 %', '9.2'),
    ('16', 'session', '4', '11 %', '37 %', '4.6'),
    ('16', 'session', '6', '2 %', '8 %', '6.9'),
    ('16', 'session', '8', '0 %', '8 %', '9.2'),
    ('200', 'evening peak', '20', '0 %', '11 %', '23.0'),
    ('200', 'evening peak', '26', '0 %', '1 %', '29.8'),
    ('200', 'evening peak', '32', '0 %', '0 %', '36.7'),
  ],
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
  print(f'\n{len(STATED)} figures and {len(STATED_TABLES)} tables, {bad} differences')
  return bad


def main(argv):
  mode = argv[1] if len(argv) > 1 else 'check'
  if mode == 'check':
    return 1 if check() else 0
  modes = dict(report=report, cpu=report_cpu, wall=report_wall, bandwidth=report_bandwidth, storage=report_storage,
               latency=report_latency, caps=report_caps)
  if mode in modes:
    modes[mode]()
  elif mode == 'tables':
    print_tables(as_python=len(argv) > 2 and argv[2] == 'python')
  else:
    print(__doc__)
    return 2
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
