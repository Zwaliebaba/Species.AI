#!/usr/bin/env python3
"""M2Economy.py: the economy model of the MVP's milestone M2 (one settlement with no neighbours).

It implements the settlement rules of Design/implementation/mvp_scope.md §2.3 and §2.4 (MVP-020 to MVP-038) with the
first guesses of its table 3, on the resource-site layout of Design/gdd/01_world_and_regions/world_and_sites.md
(WLD-060, WLD-062, WLD-064, WLD-080 to WLD-085, MVP-010), and judges a run against M2's first exit criterion (the
scope's table 2): a growth step at least every `pil.settlement.growth_step_minutes` (20 minutes) of play in the first
5 hours, and the plateau after 4 to 6 hours of play. A growth step is what the owner's answer O81 (M2 plan §9,
question 1) makes it: a structure completed, the population reaching a new multiple of 10 Workers, or the territory
taking in a resource site it did not hold. It also reports when the territory reaches the halfway line of a valley
border pair (WLD-064), which times M3's first contact, wanted in hour 2 to 3 of a run (the scope's §4), because
the same lever, `mvp.territory.radius_per_worker`, sets both.

It is the estimate of the M2 plan §3, written down so that its figures can be checked and its levers swept before
M2's task T10 measures the same thing on the game's libraries. It steps one second at a time, at the session pace:

  - the Wellspring turns `mvp.essence.orb_cost_ru` into a Worker every `mvp.essence.wellspring_interval_s` while the
    settlement is below its population limit (MVP-024, MVP-025; the orb's drift is not modelled);
  - the territory's radius follows MVP-026 and takes in a resource site when it reaches the site's centre;
  - each gathering port in the territory is worked by at most one gatherer, which mans it for `mvp.gather.port_ticks`
    and carries `mvp.gather.load_ru` to the nearest drop-off and back at `ref.unit.worker.speed` over
    `wld.travel.valley_path_factor` (MVP-031); the gatherers take the ports with the shortest cycles first;
  - the priorities share the Workers as MVP-036 states, construction releasing what its ports cannot use; guard duty,
    with no turret to man, keeps its share near the Wellspring;
  - construction is `mvp.construction.work_per_ru` Worker-seconds per resource unit at the site's ports, after the
    builders' walk (MVP-033); upkeep is continuous (MVP-035); the stores are capped (MVP-021, MVP-034).

The scripted steward is the greedy one of the M2 plan §3 and §5: a storehouse at each spot of Params.storehouse_spots
once the territory holds it and the stores hold its cost, then a dwelling once the population is within
Params.dwelling_margin of its limit and the stores hold the dwelling's cost; no turret. It is a policy, not the best
play, and T10's scripted steward states its own.

What it leaves out: walking contention, the port search's draw, pathing round structures, the orbs' drift and waiting,
the ramp-up of a new Worker's first trip, and every battle. Its figures say where the tuning starts, not where it ends.

Usage:
  python3 -I Tools/DesignModels/M2Economy.py [check | report | steps | sweep | variants]

  check     (default) compute every figure the M2 plan §3 states and compare it with STATED; prints one line per
            figure and exits with status 1 when any differs
  report    the run with the first guesses: the resource sites, a line every 30 minutes of play, and the verdict
  steps     every growth step of that run with the gap before it
  sweep     every set of O81's levers (M2 plan §9, question 2) but the last over their ranges: how many meet M2's
            exit as modelled, with a margin, and with M3's contact window, and the most preferred of them
  variants  the rule variants outside the levers: the Wellspring keeping back the next dwelling's cost

Inputs: class Params, each field naming its key or requirement. Outputs: text on standard output. Python 3 with the
standard library only; no input files. A run takes about 0.1 s, check about 2 s and sweep a few minutes.
"""
import itertools
import math
import sys
from dataclasses import dataclass, replace

HOUR_S = 3600.0
MINUTE_S = 60.0


# ---------------------------------------------------------------- inputs
@dataclass(frozen=True)
class Params:
  # settlement and growth (MVP-020 to MVP-026)
  start_workers: int = 20                 # mvp.settlement.start_workers
  population_base: int = 40               # mvp.settlement.population_base
  dwelling_population: int = 30           # mvp.dwelling.population
  population_max: int = 300               # mvp.settlement.population_max
  orb_cost_ru: float = 20.0               # mvp.essence.orb_cost_ru
  wellspring_interval_s: float = 12.0     # mvp.essence.wellspring_interval_s
  founding_radius_wu: float = 500.0       # wld.territory.founding_radius
  max_radius_wu: float = 1400.0           # wld.territory.max_radius
  radius_per_worker_wu: float = 4.0       # mvp.territory.radius_per_worker
  # resources and gathering (MVP-030, MVP-031)
  ports_per_unit: int = 4                 # mvp.resource.ports_per_unit
  port_s: float = 10.0                    # mvp.gather.port_ticks (100) at ref.sim.tick_period (0.1 s)
  load_ru: float = 1.0                    # mvp.gather.load_ru
  worker_speed_wu_s: float = 20.0         # ref.unit.worker.speed
  path_factor: float = 1.05               # wld.travel.valley_path_factor (O71)
  gather_slowdown: float = 1.0            # the gathering cycle's factor for what the model leaves out (sweep: 1.25)
  # stores, structures and construction (MVP-021, MVP-032 to MVP-034)
  wellspring_capacity_ru: float = 1500.0  # mvp.store.wellspring_capacity_ru
  storehouse_capacity_ru: float = 1500.0  # mvp.store.storehouse_capacity_ru
  ports_dwelling: int = 6                 # mvp.construction.ports_dwelling
  ports_storehouse: int = 4               # mvp.construction.ports_storehouse
  work_per_ru: float = 1.0                # mvp.construction.work_per_ru (Worker-seconds per ru)
  dwelling_ru: float = 300.0              # mvp.cost.dwelling_ru
  dwelling_escalation: float = 1.15       # mvp.cost.dwelling_escalation
  storehouse_ru: float = 200.0            # mvp.cost.storehouse_ru
  # upkeep (MVP-035)
  upkeep_worker_ru_h: float = 6.0         # mvp.upkeep.worker_ru_per_hour
  upkeep_structure_ru_h: float = 30.0     # mvp.upkeep.structure_ru_per_hour
  # priorities (MVP-036)
  share_gather: float = 0.5               # mvp.priority.default_gather
  share_construct: float = 0.3            # mvp.priority.default_construct
  share_guard: float = 0.2                # mvp.priority.default_guard
  # the site's layout (WLD-040, WLD-064, WLD-082, MVP-010)
  site_spacing_wu: float = 2080.0         # wld.site.spacing
  inner_radius_wu: float = 350.0          # wld.resource.inner_radius, wld.resource.home_inner (3) at -60, 0, +60 deg
  outer_radius_wu: float = 800.0          # wld.resource.outer_radius, wld.resource.home_outer (5) at 0, +-36, +-72 deg
  extra_radius_wu: float = 1050.0         # wld.resource.extra_radius: the ninth home site, no crossing (MVP-010)
  halfway_wu: float = 1040.0              # WLD-064: a valley border pair's halfway line, where neighbours meet
  # the run and the exit (the scope's table 2, O81)
  run_hours: float = 8.0                  # mvp.run.length_hours
  step_minutes: float = 20.0              # pil.settlement.growth_step_minutes
  early_hours: float = 5.0                # M2's exit: the steps are judged in the first 5 hours
  plateau_min_hours: float = 4.0          # M2's exit: the plateau after 4 to 6 hours of play
  plateau_max_hours: float = 6.0
  plateau_quiet_minutes: float = 60.0     # M2 plan §5: no growth step for an hour of play ...
  plateau_share: float = 0.95             # ... with the Worker count within this share of the run's largest
  population_step: int = 10               # O81: the population step, a key T8 adds to the scope's table 3
  contact_min_hours: float = 2.0          # M3's target that the same levers set: first contact in hour 2 to 3 (the
  contact_max_hours: float = 3.0          # scope's §4 on the first guesses; M3 plan §3)
  # the scripted steward (a policy of this model; T10's steward states its own)
  dwelling_margin: int = 5                # a dwelling is placed within this many Workers of the limit
  build_distance_wu: float = 300.0        # a dwelling's distance from the Wellspring, outside the centre ring
  storehouse_spots: tuple = ((761.0, -54.0), (761.0, 54.0), (925.0, 0.0))  # (wu, deg): between the outer sites
  # a rule variant outside the levers (variants): while the population is within dwelling_margin of its limit, the
  # Wellspring keeps back the next dwelling's cost; 0 is the rule as MVP-024 states it
  wellspring_keeps_back: bool = False


P = Params()


# ---------------------------------------------------------------- the site
def border_site_distance(p=P):
  """The centre of a site's own valley border area (WLD-062, WLD-080): the centroid of the half of the lens of two
  discs of radius wld.territory.max_radius, wld.site.spacing apart, that lies nearer the site. Midpoint sum."""
  d, r = p.site_spacing_wu, p.max_radius_wu
  x0, x1, n = d - r, d / 2.0, 20000
  dx = (x1 - x0) / n
  area = moment = 0.0
  for i in range(n):
    x = x0 + (i + 0.5) * dx
    h = math.sqrt(max(0.0, r * r - (d - x) ** 2))
    area += h
    moment += x * h
  return moment / area


def resource_sites(p=P):
  """The site's 11 resource units (WLD-082, WLD-085, MVP-010): (name, distance wu, bearing deg from the outward
  diagonal, units). The valley neighbours lie at +-135 deg, so each border area's site does too."""
  sites = [(f'inner {a:+d}', p.inner_radius_wu, float(a), 1) for a in (-60, 0, 60)]
  sites += [(f'outer {a:+d}', p.outer_radius_wu, float(a), 1) for a in (-72, -36, 0, 36, 72)]
  sites.append(('extra 0', p.extra_radius_wu, 0.0, 1))
  b = border_site_distance(p)
  sites += [(f'border {a:+d}', b, float(a), 1) for a in (-135, 135)]
  return sites


def polar(distance, bearing_deg):
  a = math.radians(bearing_deg)
  return distance * math.cos(a), distance * math.sin(a)


def territory_radius(workers, p=P):
  """MVP-026, with the halfway line of WLD-064 (1,040 wu) never in the way of a resource site of its own."""
  return min(p.max_radius_wu, p.founding_radius_wu + p.radius_per_worker_wu * max(0, workers - p.start_workers))


def port_rates(held, dropoffs, p=P):
  """ru per second of each port of the held sites, worked by one gatherer, fastest first (MVP-031)."""
  rates = []
  for _, dist, bearing, units in held:
    sx, sz = polar(dist, bearing)
    near = min(math.hypot(sx - x, sz - z) for x, z in dropoffs)
    cycle = (p.port_s + 2.0 * near * p.path_factor / p.worker_speed_wu_s) * p.gather_slowdown
    rates += [p.load_ru / cycle] * (units * p.ports_per_unit)
  rates.sort(reverse=True)
  prefix = [0.0]
  for r in rates:
    prefix.append(prefix[-1] + r)
  return prefix


def dwelling_cost(built, p=P):
  return p.dwelling_ru * p.dwelling_escalation ** built


def limit_of(dwellings, p=P):
  return min(p.population_max, p.population_base + p.dwelling_population * dwellings)


# ---------------------------------------------------------------- the run
@dataclass
class Site:
  kind: str
  cost: float
  ports: int
  walk_s: float
  work_left: float
  spot: tuple = None


def run(p=P, samples=False):
  """One run of p.run_hours of play. Returns a dict of the figures and, with samples, a line every 30 minutes."""
  all_sites = sorted(resource_sites(p), key=lambda s: s[1])
  workers = p.start_workers
  stores = 0.0
  dwellings = storehouses = 0
  dropoffs = [(0.0, 0.0)]
  held = []
  queue = []
  spots_left = list(p.storehouse_spots)
  orb_timer = 0.0
  steps = []
  prefix = [0.0]
  radius = -1.0
  last_multiple = workers // p.population_step
  lines = []
  income_s = upkeep_s = 0.0
  gatherers = 0
  first = {}

  def refresh():
    nonlocal prefix
    prefix = port_rates(held, dropoffs, p)

  total = int(p.run_hours * HOUR_S)
  for t in range(total):
    # territory (MVP-026) and the sites it takes in
    r = territory_radius(workers, p)
    if r != radius:
      radius = r
      if radius >= p.halfway_wu:
        first.setdefault('contact', t)
      grew = False
      for s in all_sites:
        if s not in held and s[1] <= radius:
          held.append(s)
          steps.append((t, 'site', s[0], workers))
          first.setdefault('outer' if s[0].startswith('outer') else s[0].split()[0], t)
          grew = True
      if grew:
        refresh()
    limit = limit_of(dwellings, p)
    capacity = p.wellspring_capacity_ru + p.storehouse_capacity_ru * storehouses
    # the scripted steward: storehouses, then dwellings (MVP-032, MVP-034)
    if len(queue) < 8:
      if spots_left and spots_left[0][0] <= radius and stores >= p.storehouse_ru \
          and not any(q.kind == 'storehouse' for q in queue):
        spot = spots_left.pop(0)
        stores -= p.storehouse_ru
        walk = spot[0] * p.path_factor / p.worker_speed_wu_s
        work = p.storehouse_ru * p.work_per_ru
        queue.append(Site('storehouse', p.storehouse_ru, p.ports_storehouse, walk, work, spot))
      cost = dwelling_cost(dwellings + sum(q.kind == 'dwelling' for q in queue), p)
      planned = limit_of(dwellings + sum(q.kind == 'dwelling' for q in queue), p)
      if planned < p.population_max and workers >= planned - p.dwelling_margin and stores >= cost \
          and not any(q.kind == 'dwelling' for q in queue):
        stores -= cost
        walk = p.build_distance_wu * p.path_factor / p.worker_speed_wu_s
        queue.append(Site('dwelling', cost, p.ports_dwelling, walk, cost * p.work_per_ru))
    # the priorities (MVP-036): construction takes what its site's ports allow and releases the rest
    builders = min(queue[0].ports, round(workers * p.share_construct)) if queue else 0
    released = workers * p.share_construct - builders
    want = workers * p.share_gather + released * p.share_gather / (p.share_gather + p.share_guard)
    gatherers = min(int(round(want)), len(prefix) - 1)
    income_s = prefix[gatherers]
    structures = dwellings + storehouses
    upkeep_s = (workers * p.upkeep_worker_ru_h + structures * p.upkeep_structure_ru_h) / HOUR_S
    stores = min(capacity, stores + income_s) - upkeep_s
    stores = max(0.0, stores)
    # construction (MVP-033)
    if queue:
      q = queue[0]
      if q.walk_s > 0.0:
        q.walk_s -= 1.0
      else:
        q.work_left -= builders
        if q.work_left <= 0.0:
          queue.pop(0)
          if q.kind == 'dwelling':
            dwellings += 1
          else:
            storehouses += 1
            dropoffs.append(polar(*q.spot))
            refresh()
          steps.append((t, q.kind, f'{q.cost:.0f} ru', workers))
    # the Wellspring (MVP-024, MVP-025)
    orb_timer -= 1.0
    keep = 0.0
    if p.wellspring_keeps_back and workers >= limit - p.dwelling_margin and limit < p.population_max \
        and not any(q.kind == 'dwelling' for q in queue):
      keep = dwelling_cost(dwellings, p)
    if workers < limit and orb_timer <= 0.0 and stores - keep >= p.orb_cost_ru:
      stores -= p.orb_cost_ru
      workers += 1
      orb_timer = p.wellspring_interval_s
      if workers // p.population_step > last_multiple:
        last_multiple = workers // p.population_step
        steps.append((t, 'workers', str(workers), workers))
    if samples and t % int(30 * MINUTE_S) == 0:
      lines.append((t, workers, limit, stores, radius, len(prefix) - 1, gatherers, income_s * HOUR_S,
                    upkeep_s * HOUR_S, dwellings, storehouses))
  return judge(p, steps, workers, gatherers, income_s, upkeep_s, first, lines)


def judge(p, steps, workers, gatherers, income_s, upkeep_s, first, lines):
  """The exit's first criterion (the scope's table 2, M2 plan §5): the largest gap between growth steps up to the end
  of the early hours or the plateau, whichever comes first, and the plateau: the first growth step that is followed
  by plateau_quiet_minutes of play with none while the Worker count is within plateau_share of the run's largest."""
  times = [t for t, _, _, _ in steps]
  quiet = p.plateau_quiet_minutes * MINUTE_S
  plateau = None
  for i, (t, _, _, count) in enumerate(steps):
    following = times[i + 1] if i + 1 < len(times) else p.run_hours * HOUR_S
    if following - t >= quiet and count >= p.plateau_share * workers:
      plateau = t
      break
  end = min(p.early_hours * HOUR_S, plateau if plateau is not None else p.run_hours * HOUR_S)
  gap = prev = 0
  gap_at = 0
  for t in times:
    if t > end:
      break
    if t - prev > gap:
      gap, gap_at = t - prev, prev
    prev = t
  if plateau is None or plateau > end:
    if end - prev > gap:
      gap, gap_at = end - prev, prev
  contact = first.get('contact')
  met_contact = contact is not None and p.contact_min_hours * HOUR_S <= contact <= p.contact_max_hours * HOUR_S
  met_gap = gap <= p.step_minutes * MINUTE_S
  met_plateau = plateau is not None and p.plateau_min_hours * HOUR_S <= plateau <= p.plateau_max_hours * HOUR_S
  reach = {}
  for t, kind, what, _ in steps:
    if kind == 'workers':
      reach.setdefault(int(what), t)
  return dict(steps=steps, workers=workers, gatherers=gatherers, income_h=income_s * HOUR_S,
              upkeep_h=upkeep_s * HOUR_S, plateau_s=plateau, gap_s=gap, gap_at_s=gap_at, met=met_gap and met_plateau,
              met_gap=met_gap, met_plateau=met_plateau, contact_s=contact, met_contact=met_contact, first=first,
              reach=reach, lines=lines)


# ---------------------------------------------------------------- figures
def hours(s):
  return None if s is None else round(s / HOUR_S, 1)


def minutes(s):
  return None if s is None else round(s / MINUTE_S)


def first_reach(res, workers):
  hits = [t for n, t in res['reach'].items() if n >= workers]
  return min(hits) if hits else None


def computed(p=P):
  """The figures the M2 plan §3 and §9.1 state: the first guesses, the rule variant, and the two sets the sweep
  prefers, as modelled and with every gathering cycle shorter or longer."""
  base = run(p)
  inner_prefix = port_rates([s for s in resource_sites(p) if s[1] <= p.founding_radius_wu], [(0.0, 0.0)], p)
  kept = run(replace(p, wellspring_keeps_back=True))
  margin = replace(p, **MARGIN)
  m = {k: run(replace(margin, gather_slowdown=k)) for k in (0.8, 1.0, SWEEP_SLOWDOWN)}
  joint = replace(p, **JOINT)
  j = {k: run(replace(joint, gather_slowdown=k)) for k in (1.0, SWEEP_SLOWDOWN)}
  upkeep = {u: run(replace(joint, upkeep_worker_ru_h=u)) for u in (11.0, 14.0)}
  return {
    'border_site_wu': round(border_site_distance(p)),
    'founding_ports': len(inner_prefix) - 1,
    'founding_income_ru_h': round(inner_prefix[-1] * HOUR_S, -1),
    'outer_open_h': hours(base['first'].get('outer')),
    'max_gap_min': minutes(base['gap_s']),
    'max_gap_at_h': hours(base['gap_at_s']),
    'reach_280_h': hours(first_reach(base, 280)),
    'workers_at_run_end': base['workers'],
    'plateau_h': hours(base['plateau_s']),
    'contact_h': hours(base['contact_s']),
    'run_end_income_ru_h': round(base['income_h'], -2),
    'run_end_upkeep_ru_h': round(base['upkeep_h'], -2),
    'first_guesses_met': base['met'],
    'keeps_back_gap_min': minutes(kept['gap_s']),
    'margin_set': ', '.join(lever_text(margin, p)),
    'margin_gap_min': minutes(m[1.0]['gap_s']),
    'margin_plateau_h': hours(m[1.0]['plateau_s']),
    'margin_contact_h': hours(m[1.0]['contact_s']),
    'margin_slower_gap_min': minutes(m[SWEEP_SLOWDOWN]['gap_s']),
    'margin_slower_plateau_h': hours(m[SWEEP_SLOWDOWN]['plateau_s']),
    'margin_faster_plateau_h': hours(m[0.8]['plateau_s']),
    'joint_set': ', '.join(lever_text(joint, p)),
    'joint_gap_min': minutes(j[1.0]['gap_s']),
    'joint_plateau_h': hours(j[1.0]['plateau_s']),
    'joint_contact_h': hours(j[1.0]['contact_s']),
    'joint_end_income_ru_h': round(j[1.0]['income_h'], -2),
    'joint_end_upkeep_ru_h': round(j[1.0]['upkeep_h'], -2),
    'joint_slower_gap_min': minutes(j[SWEEP_SLOWDOWN]['gap_s']),
    'joint_slower_plateau_h': hours(j[SWEEP_SLOWDOWN]['plateau_s']),
    'joint_upkeep_11_gap_min': minutes(upkeep[11.0]['gap_s']),
    'joint_upkeep_14_gap_min': minutes(upkeep[14.0]['gap_s']),
    'joint_upkeep_14_income_above': upkeep[14.0]['income_h'] > upkeep[14.0]['upkeep_h'],
  }


STATED = {
  'border_site_wu': 894,
  'founding_ports': 12,
  'founding_income_ru_h': 920.0,
  'outer_open_h': 4.1,
  'max_gap_min': 45,
  'max_gap_at_h': 2.0,
  'reach_280_h': 7.6,
  'workers_at_run_end': 280,
  'plateau_h': None,
  'contact_h': 5.6,
  'run_end_income_ru_h': 4000.0,
  'run_end_upkeep_ru_h': 2000.0,
  'first_guesses_met': False,
  'keeps_back_gap_min': 54,
  'margin_set': 'mvp.territory.radius_per_worker 12, mvp.cost.dwelling_ru 150, mvp.essence.orb_cost_ru 15',
  'margin_gap_min': 15,
  'margin_plateau_h': 4.1,
  'margin_contact_h': 0.9,
  'margin_slower_gap_min': 19,
  'margin_slower_plateau_h': 6.0,
  'margin_faster_plateau_h': 3.0,
  'joint_set': 'mvp.territory.radius_per_worker 8, mvp.cost.dwelling_ru 200',
  'joint_gap_min': 20,
  'joint_plateau_h': 5.1,
  'joint_contact_h': 2.1,
  'joint_end_income_ru_h': 4000.0,
  'joint_end_upkeep_ru_h': 2200.0,
  'joint_slower_gap_min': 26,
  'joint_slower_plateau_h': None,
  'joint_upkeep_11_gap_min': 32,
  'joint_upkeep_14_gap_min': 70,
  'joint_upkeep_14_income_above': True,
}


# ---------------------------------------------------------------- the levers (O81)
LEVERS = (
  ('radius_per_worker_wu', 'mvp.territory.radius_per_worker', (4.0, 6.0, 8.0, 10.0, 12.0)),
  ('dwelling_ru', 'mvp.cost.dwelling_ru', (300.0, 250.0, 200.0, 150.0, 100.0)),
  ('dwelling_escalation', 'mvp.cost.dwelling_escalation', (1.15, 1.10, 1.05, 1.0)),
  ('orb_cost_ru', 'mvp.essence.orb_cost_ru', (20.0, 15.0, 12.0, 10.0)),
  ('upkeep_worker_ru_h', 'mvp.upkeep.worker_ru_per_hour', (6.0, 8.0, 10.0, 12.0)),
  ('ports_per_unit', 'mvp.resource.ports_per_unit', (4, 5, 6)),
)


def lever_text(q, p=P):
  return [f'{key} {getattr(q, name):g}' for name, key, _ in LEVERS if getattr(q, name) != getattr(p, name)]


SWEEP_SLOWDOWN = 1.25  # the margin the sweep asks for: the exit met with every gathering cycle this much longer


def preference(q, p=P):
  """O81's order: fewer levers moved, then none of the later ones, then the smaller moves."""
  moved = [i for i, (name, _, _) in enumerate(LEVERS) if getattr(q, name) != getattr(p, name)]
  distance = sum(values.index(getattr(q, name)) for name, _, values in LEVERS)
  return len(moved), max(moved, default=-1), distance


def sweep(p=P):
  """Every set of the levers but the last (mvp.resource.ports_per_unit, which changes the meshes, O81), each over its
  range. Returns the sets that meet M2's exit as modelled, most preferred first, as (Params, run, run with every
  gathering cycle SWEEP_SLOWDOWN times longer)."""
  ranges = [values for _, _, values in LEVERS[:-1]]
  names = [name for name, _, _ in LEVERS[:-1]]
  found = []
  for combo in itertools.product(*ranges):
    q = replace(p, **dict(zip(names, combo)))
    a = run(q)
    if a['met']:
      found.append((q, a, run(replace(q, gather_slowdown=SWEEP_SLOWDOWN))))
  found.sort(key=lambda f: preference(f[0], p))
  return found


# The sets the sweep prefers, held here so that check runs in seconds; sweep finds them again. MARGIN is the preferred
# set that meets M2's exit also with the margin; JOINT the preferred set that meets M2's exit and M3's contact window.
MARGIN = dict(radius_per_worker_wu=12.0, dwelling_ru=150.0, orb_cost_ru=15.0)
JOINT = dict(radius_per_worker_wu=8.0, dwelling_ru=200.0)


# ---------------------------------------------------------------- reports
def print_verdict(res, p=P):
  print(f"- largest gap between growth steps up to the plateau or 5 h: {minutes(res['gap_s'])} min, from "
        f"{hours(res['gap_at_s'])} h ({'met' if res['met_gap'] else 'missed'}; target {p.step_minutes:g} min)")
  print(f"- plateau: {hours(res['plateau_s'])} h ({'met' if res['met_plateau'] else 'missed'}; target "
        f"{p.plateau_min_hours:g} to {p.plateau_max_hours:g} h); {res['workers']} Workers at the end of the run")
  print(f"- M3's first contact (the territory at {p.halfway_wu:g} wu): {hours(res['contact_s'])} h"
        f" ({'met' if res['met_contact'] else 'missed'}; target {p.contact_min_hours:g} to {p.contact_max_hours:g} h)")
  print(f"- at the end: {res['gatherers']} gatherers, income {res['income_h']:.0f} ru/h, upkeep {res['upkeep_h']:.0f}"
        f" ru/h")


def report(p=P):
  print('## The site (WLD-082, WLD-085, MVP-010)\n')
  print('| resource site | distance (wu) | bearing (deg) | ports | cycle to the Wellspring (s) | ru/h per port |')
  print('|---|---|---|---|---|---|')
  for name, d, b, u in sorted(resource_sites(p), key=lambda s: s[1]):
    cycle = p.port_s + 2.0 * d * p.path_factor / p.worker_speed_wu_s
    print(f'| {name} | {d:.0f} | {b:+.0f} | {u * p.ports_per_unit} | {cycle:.1f} | {HOUR_S / cycle:.0f} |')
  res = run(p, samples=True)
  print('\n## The run with the first guesses\n')
  print('| h | Workers | limit | stores (ru) | radius (wu) | ports | gatherers | income (ru/h) | upkeep (ru/h) |'
        ' dwellings | storehouses |')
  print('|---|---|---|---|---|---|---|---|---|---|---|')
  for t, w, lim, s, r, ports, g, inc, upk, dw, sh in res['lines']:
    print(f'| {t / HOUR_S:.1f} | {w} | {lim} | {s:.0f} | {r:.0f} | {ports} | {g} | {inc:.0f} | {upk:.0f} | {dw} |'
          f' {sh} |')
  print()
  print_verdict(res, p)


def report_steps(p=P):
  res = run(p)
  prev = 0
  for t, kind, what, _ in res['steps']:
    print(f'{t / HOUR_S:5.2f} h  +{(t - prev) / MINUTE_S:5.1f} min  {kind:10} {what}')
    prev = t
  print()
  print_verdict(res, p)


def report_sweep(p=P):
  found = sweep(p)
  total = math.prod(len(values) for _, _, values in LEVERS[:-1])
  margin = [f for f in found if f[2]['met']]
  joint = [f for f in found if f[1]['met_contact']]
  both = [f for f in joint if f[2]['met']]
  print(f'Of {total} sets (mvp.resource.ports_per_unit held), {len(found)} meet M2\'s exit as modelled,'
        f' {len(margin)} also'
        f' with every gathering cycle {SWEEP_SLOWDOWN:g} times longer, {len(joint)} also M3\'s contact window, and'
        f' {len(both)} all three. Most preferred first (O81\'s order):\n')
  print('| set | gap (min) | plateau (h) | contact (h) | with the margin: gap (min) | plateau (h) | contact (h) |')
  print('|---|---|---|---|---|---|---|')
  for q, a, b in found[:16]:
    print(f"| {', '.join(lever_text(q, p))} | {minutes(a['gap_s'])} | {hours(a['plateau_s'])} |"
          f" {hours(a['contact_s'])} |"
          f" {minutes(b['gap_s'])} | {hours(b['plateau_s'])} | {hours(b['contact_s'])} |")
  for label, held, sets in (('MARGIN', MARGIN, margin), ('JOINT', JOINT, joint)):
    same = bool(sets) and sets[0][0] == replace(p, **held)
    print(f"\n{label} {'is' if same else 'is not'} the preferred set{'' if same else '; update it and STATED'}.")
  return 0


def report_variants(p=P):
  for label, q in (('MVP-024 as written', p), ('the Wellspring keeps back the next dwelling\'s cost', replace(
      p, wellspring_keeps_back=True))):
    print(f'### {label}')
    print_verdict(run(q), p)
    print()


# ---------------------------------------------------------------- check
def check(p=P):
  bad = 0
  for k, v in computed(p).items():
    want = STATED.get(k)
    ok = want == v
    bad += not ok
    print(f"{'ok  ' if ok else 'DIFF'} {k}: model {v}, stated {want}")
  return bad


def main(argv):
  mode = argv[1] if len(argv) > 1 else 'check'
  if mode == 'check':
    return 1 if check() else 0
  modes = dict(report=report, steps=report_steps, sweep=report_sweep, variants=report_variants)
  if mode in modes:
    return modes[mode]() or 0
  print(__doc__)
  return 2


if __name__ == '__main__':
  sys.exit(main(sys.argv))
