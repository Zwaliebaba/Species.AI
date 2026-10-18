#!/usr/bin/env python3
"""A2C_ServerBudgetChallenge.py: the A2 challenger's checks of the server-budget model (round 1).

It loads the proposer's model `Tools/DesignModels/A2ServerBudget.py` from the path given as the first argument (the
model is never copied or edited; it loads A1's model by its own path) and tests the assumptions behind the figures of
Design/architecture/04_solution_strategy.md §4.5 and ADR-079 to ADR-088:

  cost       per-figure cost multipliers (1, 1.25, 2, 4, and 6 for the per-frame reading of DBG-205), A1's uncapped
             stress, battles at the 1,000-figure cap, AI costs, and the vCPU factor; average-core budgets
  wall       the tick wall budget of ADR-080 item 6 (50 % of the period) with region steps scheduled as single-threaded
             jobs (longest first) on the host's workers, against the "35 % of the host" headline
  seen       the seen layer (fog of war for 256 settlements) that the model folds into a 30 us region fixed cost
  switch     fidelity switches without hysteresis: a steward toggling between the strategic and the settlement view
  bandwidth  baseline age under round trip and loss, a battle at the 1,000-figure cap, skirmish views at the design
             and stress populations, hitches under loss, per-client baseline history memory, first-join terrain
  latency    Monte Carlo command-to-display latency, discrete orders and direct control, mean and 95th percentile,
             with the client-side terms the model omits, against ADR-079's confirmation thresholds
  persist    staggered per-owner checkpoints: cross-owner events inside the window a crash restores inconsistently
  caps       the region body budget and the battle-size cap: body-budget denial and the defender share of a battle

Usage:
  python3 -I species_design_work/scripts/A2C_ServerBudgetChallenge.py Tools/DesignModels/A2ServerBudget.py [mode]

  mode is one of the names above, or all (default). Output: text on standard output. Standard library only; every
  random draw comes from random.Random with a fixed seed, so a run is reproducible.
"""
import importlib.util
import math
import random
import sys
from dataclasses import replace


def load_model(path):
  spec = importlib.util.spec_from_file_location('A2ServerBudget', path)
  mod = importlib.util.module_from_spec(spec)
  sys.modules['A2ServerBudget'] = mod
  spec.loader.exec_module(mod)
  return mod


M = None  # the A2 model, set in main


def f1(x):
  return f'{x:.1f}'


def f2(x):
  return f'{x:.2f}'


# ---------------------------------------------------------------- cost
def report_cost():
  P = M.P
  print('## cost: sensitivity of world CPU (reference cores, average over a tick)\n')
  print('| Case | x1 | x1.25 | x2 | x4 | x6 (DBG-205 read per frame) | budget (cores) | breaks at |')
  print('|---|---|---|---|---|---|---|---|')
  for cap, scen, budget, label in ((16, 'all_present', P.integrated_cores, '16 all present, integrated (1.0)'),
                                   (16, 'all_present', P.community_cores, '16 all present, community 2 vCPU (1.2)'),
                                   (200, 'stress', P.hosted_cores, '200 stress, 8 vCPU (4.8)')):
    vals = [M.cores(M.world_cpu_ms(cap, scen, P, k)[0]) for k in (1, 1.25, 2, 4, 6)]
    brk = budget / vals[0]
    print(f'| {label} | ' + ' | '.join(f2(v) for v in vals) + f' | {budget:g} | x{brk:.2f} |')
  print()
  # A1's stress at 200 without the world cap, and battles at the 1,000-figure cap
  w200 = M.world(200)
  nb_uncapped = M.battles(200, 'stress', P.battles_per_week_stress)
  base, parts, ctx = M.world_cpu_ms(200, 'stress', P)
  extra_uncapped = (nb_uncapped - ctx['battles']) * M.battle_ms(None, P)
  big = replace(P, battle_bodies=P.battle_bodies_max, orbs_per_battle=P.orbs_per_battle * P.battle_bodies_max
                // P.battle_bodies)
  big_total = M.world_cpu_ms(200, 'stress', big)[0]
  print(f'- A1 stress at 200 uncapped: {nb_uncapped} battles (cap {ctx["battles"]}): +{f2(extra_uncapped)} ms per tick')
  print(f'- 200 stress with every battle at the 1,000-figure cap: {f1(big_total)} ms per tick, '
        f'{f2(M.cores(big_total))} cores ({100 * M.cores(big_total) / P.hosted_cores:.0f} % of the host), '
        f'against {f1(base)} ms at 400')
  w16 = M.world(16)
  nb16_session = M.battles(16, 'stress', P.battles_per_week_stress, 'session')
  print(f'- 16 session pace stress uncapped: {nb16_session} battles against the world cap {M.world_cap(w16["settlements"])}')
  # AI cost
  for ai in (20.0, 100.0, 200.0):
    p2 = replace(P, cost_ai_settlement_us=ai)
    t = M.world_cpu_ms(200, 'stress', p2)[0]
    print(f'- AI settlement or deputy at {ai:g} us per tick: 200 stress {f1(t)} ms per tick, {f2(M.cores(t))} cores')
  # region step budget against ADR-080's own +25 % tolerance
  head = P.region_step_budget_ms / M.region_budget_ms(P)
  print(f'- region at its body budget: {f2(M.region_budget_ms(P))} ms of the {P.region_step_budget_ms:g} ms step '
        f'budget: headroom x{head:.3f}, inside ADR-080\'s confirmation tolerance of x1.25 '
        f'({f2(M.region_budget_ms(P, 1.25))} ms at x1.25)')
  # vCPU factor
  for v in (0.4, 0.6, 0.8, 1.0):
    hw = 8 * v
    print(f'- vCPU = {v:g} reference cores: the 200 stress case takes {100 * M.cores(base) / hw:.0f} % of 8 vCPU')
  print()


# ---------------------------------------------------------------- wall
def region_jobs(capacity, scen, p, k):
  """Region step costs (ref-ms) with the watched settlements and battles packed into the fewest regions (the
  watched set of present stewards is their own valley, WLD-244), battles at most 2 per region, busiest first."""
  w = M.world(capacity)
  total, parts, ctx = M.world_cpu_ms(capacity, scen, p, k)
  R = w['regions']
  n = w['settlements']
  full = ctx['full']
  nb = ctx['battles']
  per_full = (M.settlement_full_ms(p, k) + p.cost_ai_settlement_us * k / 1000.0)
  per_abs = M.settlement_abstract_ms(p, k)
  per_battle = M.battle_ms(None, p, k)
  fixed = p.cost_region_fixed_us * k / 1000.0
  exp_each = parts['expeditions'] / R
  jobs = []
  f_left, a_left, b_left = full, n - full, nb
  for _ in range(R):
    f = min(4, f_left)
    f_left -= f
    a = min(4 - f, a_left)
    a_left -= a
    b = min(2, b_left)
    b_left -= b
    jobs.append(f * per_full + a * per_abs + b * per_battle + fixed + exp_each)
  serial = parts['world']
  parallel_tail = parts['replication'] + parts['persistence']
  return jobs, serial, parallel_tail, total


def lpt_makespan(jobs, workers):
  loads = [0.0] * workers
  for j in sorted(jobs, reverse=True):
    i = loads.index(min(loads))
    loads[i] += j
  return max(loads)


def wall_ms(capacity, scen, p, k, workers, speed):
  jobs, serial, tail, total = region_jobs(capacity, scen, p, k)
  regions = lpt_makespan(jobs, workers) / speed
  return regions + serial / speed + tail / (workers * speed), max(jobs) / speed, total


def report_wall():
  P = M.P
  budget = P.tick_wall_share * 1000.0 / P.tick_hz
  print(f'## wall: tick wall time against the {budget:g} ms budget of ADR-080 item 6\n')
  print('| Host | Workers x speed | x1 ms | x1.25 ms | x2 ms | breaks at | busiest region ms at x1 |')
  print('|---|---|---|---|---|---|---|')
  for label, cap, scen, workers, speed in (('integrated, 16 all present', 16, 'all_present', 3, 1.0),
                                           ('community 2 vCPU, 16 all present', 16, 'all_present', 2, 0.6),
                                           ('stage 3, 8 vCPU (8 workers)', 200, 'stress', 8, 0.6),
                                           ('stage 3, 8 vCPU (7 workers, 1 for tick, I/O, store)', 200, 'stress', 7, 0.6),
                                           ('stage 3, 16 vCPU', 200, 'stress', 15, 0.6)):
    vals = [wall_ms(cap, scen, P, k, workers, speed)[0] for k in (1, 1.25, 2)]
    lo, hi = 0.5, 20.0
    for _ in range(60):
      mid = (lo + hi) / 2
      if wall_ms(cap, scen, P, mid, workers, speed)[0] <= budget:
        lo = mid
      else:
        hi = mid
    bus = wall_ms(cap, scen, P, 1, workers, speed)[1]
    print(f'| {label} | {workers} x {speed:g} | ' + ' | '.join(f1(v) for v in vals) + f' | x{lo:.2f} | {f1(bus)} |')
  t200 = M.world_cpu_ms(200, 'stress', P)[0]
  print(f'\n- the chapter\'s "35 % of an 8-vCPU host" is {f1(t200)} ref-ms against 480 ref-ms per tick; the tick wall '
        f'budget allows {budget:g} ms x 8 vCPU x 0.6 = {budget * 8 * 0.6:g} ref-ms, so the same load is '
        f'{100 * t200 / (budget * 8 * 0.6):.0f} % of what a tick may use before load balance and serial parts')
  print()


# ---------------------------------------------------------------- seen layer
def report_seen():
  P = M.P
  print('## seen: the seen layer (fog of war per settlement) as a cost per body per update\n')
  print('Bodies in battle update it every tick; other bodies every 5 ticks. The model holds 30 us per region for it.\n')
  print('| Case | bodies in battle | other bodies | 0.25 us | 0.5 us | 1.0 us | model total ms |')
  print('|---|---|---|---|---|---|---|')
  for cap, scen in ((16, 'all_present'), (200, 'stress')):
    total, parts, ctx = M.world_cpu_ms(cap, scen, P)
    w = M.world(cap)
    inb = ctx['battles'] * P.battle_bodies
    other = ctx['full'] * M.A1.P.pop_plateau + w['expeditions'] * P.expedition_figures
    cells = []
    for c in (0.25, 0.5, 1.0):
      add = (inb * c + other * c / 5.0) / 1000.0
      cells.append(f'+{f1(add)} ({100 * add / total:.0f} %)')
    print(f'| {cap} {scen} | {inb:,} | {other:,.0f} | ' + ' | '.join(cells) + f' | {f1(total)} |')
  print()


# ---------------------------------------------------------------- fidelity switches
def report_switch():
  P = M.P
  print('## switch: fidelity switches without hysteresis\n')
  for cost_us in (2.0, 5.0):
    for period_s in (10.0, 30.0):
      w = M.world(200)
      per_switch_ms = M.A1.P.pop_plateau * cost_us / 1000.0
      switches_per_s = w['present'] * 6 * 2 / period_s
      ms_per_tick = switches_per_s * per_switch_ms / P.tick_hz
      t = M.world_cpu_ms(200, 'stress', P)[0]
      print(f'- 600 figures materialised at {cost_us:g} us each ({f1(per_switch_ms)} ms a switch); every present steward '
            f'(47) toggles strategic and settlement view every {period_s:g} s over 6 settlements: '
            f'{f1(switches_per_s)} switches/s, +{f1(ms_per_tick)} ms per tick ({100 * ms_per_tick / t:.0f} % of 200 stress)')
  print()


# ---------------------------------------------------------------- bandwidth
def axis_bits(move_wu, quantum):
  return math.ceil(math.log2(2.0 * move_wu / quantum + 1.0))


def baseline_age_ticks(rtt_ms, loss, p):
  """Mean age in ticks of the last acknowledged state when a tick's update is encoded: the ack of an update sent
  20 ms into tick n arrives after the round trip; a lost update or ack ages it by one more tick, geometrically."""
  tick_ms = 1000.0 / p.tick_hz
  base = math.ceil((p.server_step_ms + rtt_ms) / tick_ms)
  q = 1 - (1 - loss) ** 2  # update or its ack lost
  return base + q / (1 - q)


def view_bw(view, p, age=1.0):
  v = M.VIEWS[view]
  tick_s = 1.0 / p.tick_hz
  t1_bits = 2 * axis_bits(p.max_speed_wus * tick_s * age, p.quantum_near_wu) + p.header_bits
  t2_bits = 2 * axis_bits(p.max_speed_wus / p.tier2_hz * max(1.0, age * p.tier2_hz / p.tick_hz), p.quantum_far_wu) \
    + p.header_bits
  parts = dict(
    tier1=v['t1'] * (v['moving'] * t1_bits / 8.0 + p.state_change_share * p.state_bytes) * p.tick_hz,
    tier2=v['t2'] * v['moving'] * t2_bits / 8.0 * p.tier2_hz,
    churn=(v['t1'] + v['t2']) * p.churn_share * p.spawn_bytes,
    shots=v['combat'] * p.shots_per_s * p.shot_bytes,
    orbs=v['orbs'] * p.orb_hz * p.orb_bytes,
    groups=v['groups'] * p.group_bytes,
    fixed=p.fixed_bytes_per_s)
  payload = sum(parts.values())
  per_tick = payload / p.tick_hz
  dgrams = max(1, math.ceil(per_tick / p.payload_bytes))
  return payload + p.tick_hz * dgrams * p.datagram_overhead_bytes, dgrams


def report_bandwidth():
  P = M.P
  M.VIEWS['battle1000'] = dict(t1=1200, t2=600, moving=0.85, combat=1000, orbs=100, groups=20, label='battle of 1,000')
  M.VIEWS['skirmish'] = dict(t1=1500, t2=500, moving=0.85, combat=1000, orbs=100, groups=10,
                             label='skirmish, design population 2,000')
  M.VIEWS['skirmish4k'] = dict(t1=1500, t2=2500, moving=0.85, combat=2000, orbs=200, groups=10,
                               label='skirmish, stress population 4,000')
  print('## bandwidth: per client, KB/s, by baseline age (round trip and loss)\n')
  print('| View | model (1 tick) | 50 ms, 0 % | 150 ms, 0 % | 150 ms, 10 % | datagrams per tick | within 48 KB/s at 150 ms |')
  print('|---|---|---|---|---|---|---|')
  for view in ('home', 'battle', 'battle1000', 'war', 'skirmish', 'skirmish4k'):
    model = M.downstream(view, P)[0]
    cells = []
    for rtt, loss in ((50, 0.0), (150, 0.0), (150, 0.10)):
      age = baseline_age_ticks(rtt, loss, P)
      cells.append(view_bw(view, P, age)[0])
    dg = view_bw(view, P, 1.0)[1]
    print(f'| {M.VIEWS[view]["label"]} | {f1(model / 1000)} | ' + ' | '.join(f1(c / 1000) for c in cells)
          + f' | {dg} | {"yes" if cells[1] <= P.client_cap_bytes_per_s else "no"} |')
  print()
  print('Hitches under loss: a tick whose update is lost leaves the display without its next state for 70 ms '
        '(one tick less the 30 ms jitter allowance) unless every datagram decodes on its own.\n')
  for view in ('battle', 'war'):
    dg = view_bw(view, P, 1.0)[1]
    for loss in (0.01, 0.10):
      pt = 1 - (1 - loss) ** dg
      print(f'- {M.VIEWS[view]["label"]}, {dg} datagrams per tick, {loss:.0%} loss: {pt:.1%} of ticks lose their '
            f'update, one hitch every {0.1 / pt:.1f} s')
  print()
  print('Baseline history memory (the server keeps every unacknowledged state per client, 16 bytes a figure):\n')
  for ring in (8, 32):
    per_client = (M.VIEWS['war']['t1'] + M.VIEWS['war']['t2']) * P.baseline_bytes * ring / 1e6
    print(f'- {ring} ticks of history: {f1(per_client)} MB per client in a valley war; 47 clients '
          f'{per_client * 47:.0f} MB, 200 clients {per_client * 200:.0f} MB (the model: one baseline, 2.6 MB at 200)')
  print()
  print('First join: the world\'s baked terrain (WLD-166) streamed to a new client\n')
  for cap in (16, 200):
    w = M.world(cap)
    raw = w['land_samples'] * 8.0 / 1e6
    q16 = w['land_samples'] * 2.0 / 1.3 / 1e6
    print(f'- {cap} stewards: {f1(raw)} MB of binary64 heights: {raw * 1e6 / P.client_cap_bytes_per_s / 60:.1f} min at '
          f'the 48 KB/s client cap, {raw * 1e6 / P.host_upload_budget_bytes_per_s:.0f} s at the whole 4 Mbit/s host '
          f'budget; 16-bit heights compressed 1.3x: {f1(q16)} MB, {q16 * 1e6 / P.client_cap_bytes_per_s:.0f} s at '
          f'48 KB/s')
  t20 = replace(P, tick_hz=20.0)
  print(f'\n- integrated host upload, 15 remote in battle views at 20 Hz: '
        f'{15 * min(P.client_cap_bytes_per_s, M.downstream("battle", t20)[0]) * 8 / 1e6:.1f} Mbit/s '
        f'(10 Hz: {M.host_upload_mbits(15, "battle", P):.1f})')
  for view in ('battle1000', 'skirmish', 'skirmish4k', '_probe'):
    M.VIEWS.pop(view, None)
  print()


# ---------------------------------------------------------------- latency
def latency_samples(tick_hz, rtt_ms, direct, client_terms, step_ms, n=200000, seed=1):
  rng = random.Random(seed)
  tick = 1000.0 / tick_hz
  frame = 1000.0 / 60.0
  jitter_allow = 30.0 if rtt_ms > 0 else 0.0
  out = []
  for _ in range(n):
    t = 0.0
    if client_terms:
      t += rng.uniform(0, frame)              # input sampled at the next frame
    if direct:
      t += rng.uniform(0, tick)               # control state sent once per tick period (ADR-087 item 5)
    if rtt_ms > 0:
      t += rtt_ms / 2 + rng.expovariate(1 / 5.0)
    t += rng.uniform(0, tick)                 # wait for the next server tick
    t += step_ms                              # the server's step and encode
    if rtt_ms > 0:
      t += rtt_ms / 2 + rng.expovariate(1 / 5.0)
    t += tick + jitter_allow                  # interpolation reaches the new state
    if client_terms:
      t += rng.uniform(0, frame) + frame + frame / 2  # next frame, one queued frame, half a scan-out
    else:
      t += frame
    out.append(t)
  out.sort()
  return sum(out) / n, out[int(0.95 * n)]


def report_latency():
  print('## latency: command to display, ms (mean / 95th percentile)\n')
  print('ADR-079 confirmation: p95 at most 250 ms on the host and 350 ms remote at a 50 ms round trip.\n')
  print('| Client | tick | A2 terms, order | + client terms, order | + client terms, direct control | '
        '200 stress step (40 ms), direct |')
  print('|---|---|---|---|---|---|')
  for rtt in (0.0, 50.0, 150.0):
    for hz in (10.0, 20.0):
      step = 20.0 * 10.0 / hz
      a = latency_samples(hz, rtt, False, False, step)
      b = latency_samples(hz, rtt, False, True, step)
      c = latency_samples(hz, rtt, True, True, step)
      d = latency_samples(hz, rtt, True, True, 40.0 * 10.0 / hz)
      label = 'host' if rtt == 0 else f'remote {rtt:.0f} ms'
      print(f'| {label} | {hz:g} Hz | ' + ' | '.join(f'{m:.0f} / {q:.0f}' for m, q in (a, b, c, d)) + ' |')
  h = latency_samples(10.0, 0.0, True, True, 20.0)
  r = latency_samples(10.0, 50.0, True, True, 20.0)
  print(f'\n- host advantage in direct control at 10 Hz against a 50 ms remote: {r[0] - h[0]:.0f} ms mean')
  print()


# ---------------------------------------------------------------- persistence
def report_persist():
  P = M.P
  print('## persist: staggered checkpoints (every 600 ticks, phase by owner id)\n')
  period_s = P.checkpoint_period_s
  window_s = period_s / 3.0   # E|U1 - U2| for two uniform phases over one period
  print(f'Mean distance between two owners\' last checkpoints: {window_s:.0f} s of world time.\n')
  print('| Stewards | pace | battles per world hour | border crossings per hour | cross-owner events per hour '
        '| expected in the window per crash | P(at least one) |')
  print('|---|---|---|---|---|---|---|')
  for cap in (16, 200):
    w = M.world(cap)
    n = w['settlements']
    per_hour = n * P.battles_per_week / 2.0 / 168.0
    for label, factor in (('day mean', 1.0), ('evening peak', 0.2 + 0.8 * 24.0 / 3.0)):
      b = per_hour * factor
      crossings = b * (1.0 / 3.0) * 2.0      # a third of attacks go to a crossing neighbour, there and back
      events = 2 * b + crossings + b         # battle start (slot), battle end (result), crossings, declarations
      lam = events / 3600.0 * window_s
      print(f'| {cap} | {label} | {f1(b)} | {f1(crossings)} | {f1(events)} | {lam:.3f} | {1 - math.exp(-lam):.1%} |')
  print('\nA host who ends the process at a moment of choice (an integrated host, with the Windows Task Manager) reaches 100 %.\n')
  for cap in (16, 200):
    w = M.world(cap)
    full = w['watched_max']
    kb = M.checkpoint_kb(cap, full, M.world_cap(w['settlements']), P)
    print(f'- {cap} stewards: a consistent checkpoint of every owner at one tick is {kb / 1024:.1f} MB; at 1 GB/s '
          f'{kb / 1024:.1f} ms single-threaded, {kb / 1024 / (8 * 0.6 if cap == 200 else 3):.1f} ms on the host\'s '
          f'workers, once per 60 s')
  print()


# ---------------------------------------------------------------- caps
def report_caps():
  P = M.P
  pop = M.A1.P.pop_plateau
  print('## caps: the region body budget and the battle-size cap\n')
  free = P.region_body_budget - 4 * pop
  print(f'- a full region holds 4 x {pop} = {4 * pop:,} bodies of its settlements; {free:,} bodies of the '
        f'{P.region_body_budget:,} remain for visitors and battles; an attacker who keeps {free:,} figures in the '
        f'region (one battle of {P.battle_bodies_max:,} and {free - P.battle_bodies_max:,} waiting at its edge) stops '
        f'the growth of all 4 settlements (ADR-080 item 9)')
  for limit in (600, 750):
    att = P.battle_bodies_max - limit
    print(f'- a battle at a settlement of {limit} Workers and Leaders, all inside its battlefield, leaves '
          f'{att} attacker places under the 1,000 cap: {limit / att:.1f} defenders per attacker')
  print()


def main(argv):
  global M
  if len(argv) < 2:
    print(__doc__)
    return 2
  M = load_model(argv[1])
  mode = argv[2] if len(argv) > 2 else 'all'
  modes = dict(cost=report_cost, wall=report_wall, seen=report_seen, switch=report_switch,
               bandwidth=report_bandwidth, latency=report_latency, persist=report_persist, caps=report_caps)
  if mode == 'all':
    for fn in modes.values():
      fn()
  elif mode in modes:
    modes[mode]()
  else:
    print(__doc__)
    return 2
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
