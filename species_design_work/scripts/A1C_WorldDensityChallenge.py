#!/usr/bin/env python3
"""A1C_WorldDensityChallenge.py: the A1 challenger's checks of the world-density model (round 1).

It loads the proposer's model `Tools/DesignModels/A1WorldDensity.py` from the path given as the first argument (the
model is never copied or edited) and tests the assumptions behind the figures of Design/gdd/01_world_and_regions.md:

  speed      travel limits of WLD-101 against the expedition speed that B4 has not set, and over 20 seeds
  jitter     what the per-site jitter of WLD-040 does to spacing, first contact, lens width and home-cell area
  late       the territory and resource sites of a late joiner beside settlements at plateau (WLD-061 to WLD-064)
  churn      a months-old world: stewards arrive and leave, settlements go dormant, places are held (WLD-005,
             WLD-063, WLD-121 to WLD-143; PIL-078); occupancy, ghosts, refused joiners, border areas by cohort
  battles    battles in progress against battle length, steward share, evening window and clustered attacks
  watched    the union of watched settlements at the peak, with and without expedition views (WLD-244)
  ridge      a Worker's straight walk over a border ridge against the travel network (WLD-021, WLD-100, WLD-102)
  extension  a capacity raise from 16 to 200 by WLD-008: blob growth, the origin, the AI share and contact
  ai         AI count against region count over capacities 1 to 256, a host who lowers the AI count, pass counts
  adjacency  how many stewards have another steward as a neighbour, by AI placement rule and founding ranking

Usage:
  python3 -I species_design_work/scripts/A1C_WorldDensityChallenge.py Tools/DesignModels/A1WorldDensity.py [mode]

  mode is one of the names above, or all (default). Output: text tables on standard output. Standard library only;
  every random draw comes from random.Random with a fixed seed, so a run is reproducible.
"""
import importlib.util
import math
import random
import sys
from dataclasses import replace


def load_model(path):
  spec = importlib.util.spec_from_file_location('A1WorldDensity', path)
  mod = importlib.util.module_from_spec(spec)
  sys.modules['A1WorldDensity'] = mod
  spec.loader.exec_module(mod)
  return mod


def pct(x):
  return f'{100.0 * x:.0f} %'


def fnum(x, nd=2):
  return '-' if isinstance(x, float) and math.isnan(x) else f'{x:.{nd}f}'


# ---------------------------------------------------------------- speed
def speed_check(m):
  P = m.P
  print('## C1 Travel limits against the expedition speed (WLD-101, WLD-104)\n')
  t = m.travel_table(P)
  lay16, lay200 = m.Layout(16, 1, P), m.Layout(200, 1, P)
  hop = max(lay16.region_hop_wu(), lay200.region_hop_wu())
  d16, d200 = lay16.diameter_wu(), lay200.diameter_wu()
  legs = (('valley neighbour', t['valley'], 2.5), ('pass neighbour', t['pass_'], 4.0), ('region hop', hop, 6.5),
          ('diameter at 16', d16, 25.0), ('diameter at 200', d200, 70.0))
  print('| leg | distance (wu) | limit (min) | lowest speed that meets it (wu/s) | 12 wu/s | 15 wu/s | 20 wu/s | 25 wu/s |')
  print('|---|---|---|---|---|---|---|---|')
  for name, dist, lim in legs:
    cells = []
    for v in (12.0, 15.0, 20.0, 25.0):
      mins = dist / v / 60.0
      cells.append(f'{mins:.1f}{"" if mins <= lim else " FAIL"}')
    print(f'| {name} | {dist:,.0f} | {lim:g} | {dist / (lim * 60.0):.1f} | ' + ' | '.join(cells) + ' |')
  # diameter over seeds
  rows = []
  for cap in (16, 200):
    vals = []
    for seed in range(1, 21):
      vals.append(m.Layout(cap, seed, P).diameter_wu() / 20.0 / 60.0)
    rows.append((cap, min(vals), sum(vals) / len(vals), max(vals)))
  print('\nWorld diameter at 20 wu/s over seeds 1 to 20 (the document states seed 1 only):\n')
  print('| capacity | min (min) | mean (min) | max (min) | limit (min) |')
  print('|---|---|---|---|---|')
  for cap, lo, mean, hi in rows:
    print(f'| {cap} | {lo:.1f} | {mean:.1f} | {hi:.1f} | {25 if cap == 16 else 70} |')
  print()


# ---------------------------------------------------------------- jitter and home cells
def region_geometry(lay, r):
  p = lay.p
  i, j = lay.cells[r]
  x0 = (i + p.sea_ring_regions) * p.region_side_wu
  z0 = (j + p.sea_ring_regions) * p.region_side_wu
  sites = [lay.sites[lay.site_of(r, k)][:2] for k in range(4)]
  centre = (x0 + p.region_side_wu / 2.0, z0 + p.region_side_wu / 2.0)
  return x0, z0, sites, centre


VALLEY_NB = {0: (1, 2), 1: (0, 3), 2: (0, 3), 3: (1, 2)}


def home_cell_areas(lay, r, step=32.0, central_clear=270.0, core_clear=600.0):
  """Per local site of region r: home-cell area (WLD-061: in the region outside the ridge bands, within the largest
  radius, nearer than either valley neighbour, minus both lenses, minus the clearances of WLD-060 and WLD-083), the
  area of each of its two lenses that it could hold (clipped by the same clearances), and the area claimed by two home
  cells at once (diagonal overlap)."""
  p = lay.p
  x0, z0, s, c = region_geometry(lay, r)
  rmax = p.max_radius_wu
  lo, hi = p.ridge_half_band_wu, p.region_side_wu - p.ridge_half_band_wu
  home = [0.0] * 4
  lens = {}
  overlap = 0.0
  a2 = step * step
  z = z0 + lo + step / 2
  while z < z0 + hi:
    x = x0 + lo + step / 2
    while x < x0 + hi:
      if math.hypot(x - c[0], z - c[1]) > central_clear:
        d = [math.hypot(x - s[k][0], z - s[k][1]) for k in range(4)]
        owners = []
        for k in range(4):
          if d[k] > rmax:
            continue
          if any(d[q] < core_clear for q in range(4) if q != k):
            continue
          nb = VALLEY_NB[k]
          in_lens = [q for q in nb if d[q] <= rmax]
          if in_lens:
            for q in in_lens:
              key = (min(k, q), max(k, q))
              lens[key] = lens.get(key, 0.0) + a2 / 2.0   # counted from both sides
            continue
          if all(d[k] <= d[q] for q in nb):
            owners.append(k)
        for k in owners:
          home[k] += a2
        if len(owners) > 1:
          overlap += a2
      x += step
    z += step
  return home, lens, overlap


def jitter_check(m):
  P = m.P
  print('## C2 What the per-site jitter does (WLD-040, WLD-042, WLD-062, WLD-064)\n')
  ds, days, lens_w, cdist, pday = [], [], [], [], []
  into_clear = 0
  for seed in range(1, 11):
    lay = m.Layout(200, seed, P)
    for a, b in lay.valley_pairs:
      d = lay.dist(a, b)
      ds.append(d)
      days.append(m.day_reaching(d / 2.0, P))
      lens_w.append(2 * P.max_radius_wu - d)
      if d < P.max_radius_wu + P.core_radius_wu + 200.0:
        into_clear += 1
    for r in range(len(lay.cells)):
      _, _, s, c = region_geometry(lay, r)
      for k in range(4):
        cdist.append(math.hypot(s[k][0] - c[0], s[k][1] - c[1]))
    for a, b in lay.passes:
      for s_ in (a, b):
        x, z, r, k = lay.sites[s_]
        i, j = lay.cells[r]
        lx = x - (i + P.sea_ring_regions) * P.region_side_wu
        lz = z - (j + P.sea_ring_regions) * P.region_side_wu
        other = lay.sites[b if s_ == a else a]
        if abs(other[0] - x) > abs(other[1] - z):
          mouth = (P.region_side_wu - P.ridge_half_band_wu - lx) if other[0] > x else (lx - P.ridge_half_band_wu)
        else:
          mouth = (P.region_side_wu - P.ridge_half_band_wu - lz) if other[1] > z else (lz - P.ridge_half_band_wu)
        pday.append(m.day_reaching(mouth, P))

  def stats(v):
    v = sorted(v)
    return v[0], v[len(v) // 2], v[-1]

  print('| figure (200 stewards, seeds 1 to 10) | document | min | median | max |')
  print('|---|---|---|---|---|')
  for name, doc, v, fmt in (('valley spacing (wu)', '2,080', ds, '{:,.0f}'),
                            ('first valley contact (world day)', '5', days, '{:.1f}'),
                            ('valley lens width (wu)', '720', lens_w, '{:,.0f}'),
                            ('site to valley centre (wu)', '1,471', cdist, '{:,.0f}'),
                            ('first pass contact (world day)', '9 (8.5)', pday, '{:.1f}')):
    lo, med, hi = stats(v)
    print(f'| {name} | {doc} | {fmt.format(lo)} | {fmt.format(med)} | {fmt.format(hi)} |')
  print(f'\nValley pairs closer than 2,000 wu, where a held lens reaches inside the neighbour\'s core clearance'
        f' (WLD-060: 400 + 200 wu): {into_clear} of {len(ds)} ({pct(into_clear / len(ds))})')
  early = sum(1 for d in days if d < 4.0) / len(days)
  late = sum(1 for d in days if d > 6.0) / len(days)
  print(f'Pairs whose first contact falls before world day 4: {pct(early)}; after world day 6: {pct(late)}')
  # home cells
  homes, holdable, overlaps = [], [], 0.0
  for seed in (1, 2, 3):
    lay = m.Layout(200, seed, P)
    for r in range(len(lay.cells)):
      home, lens, ov = home_cell_areas(lay, r)
      overlaps += ov
      for k in range(4):
        homes.append(home[k])
        tot = home[k] + sum(v for key, v in lens.items() if k in key)
        holdable.append(tot)
  nominal = m.Layout(1, 1, replace(P, site_jitter_wu=0.0))
  h0, l0, _ = home_cell_areas(nominal, 0)
  lo, med, hi = stats(homes)
  lo2, med2, hi2 = stats(holdable)
  print(f'\nHome cell (no lens held), M wu2: nominal {h0[0] / 1e6:.2f}; jittered min {lo / 1e6:.2f}, median'
        f' {med / 1e6:.2f}, max {hi / 1e6:.2f} (max / min {hi / lo:.2f})')
  print(f'Home cell plus both valley lenses, M wu2: nominal {(h0[0] + sum(v for k_, v in l0.items() if 0 in k_)) / 1e6:.2f};'
        f' jittered min {lo2 / 1e6:.2f}, median {med2 / 1e6:.2f}, max {hi2 / 1e6:.2f} (max / min {hi2 / lo2:.2f})')
  print(f'Area inside two home cells at once (diagonal sites, not covered by any rule): {overlaps / 1e6:.3f} M wu2'
        f' over 195 regions\n')


# ---------------------------------------------------------------- late joiner geometry
def late_check(m):
  P = m.P
  print('## C3 A late joiner beside settlements at plateau (WLD-061 to WLD-064, WLD-081, PIL-029)\n')
  nominal = m.Layout(1, 1, replace(P, site_jitter_wu=0.0))
  home, lens, _ = home_cell_areas(nominal, 0, step=16.0)
  h = home[0]
  l1 = lens[(0, 1)]
  corridor = P.pass_width_wu * 2 * P.ridge_half_band_wu
  print('| settlement | holds | territory at plateau (M wu2) | resource sites in home-site units | vs late joiner |')
  print('|---|---|---|---|---|')
  rows = (('late joiner beside two plateau neighbours', 'home cell only', h, 8),
          ('fresh-world settlement (typical)', 'home cell + 1 lens', h + l1, 9),
          ('incumbent beside a late joiner', 'home cell + 2 lenses', h + 2 * l1, 10),
          ('incumbent, interior site, holds its pass', 'home cell + 2 lenses + 1 pass', h + 2 * l1 + corridor, 12))
  for name, holds, area, res in rows:
    print(f'| {name} | {holds} | {area / 1e6:.2f} | {res} | {res / 8:.2f} x |')
  print(f'\n(+3 for a valley\'s central resource site, which any settlement may claim: up to 15 units, 1.9 x the late'
        f' joiner\'s 8. No site has two passes: see C9.)')
  print(f'\nGrowth (radius r0 + (rmax - r0) (t / 14)^0.5): radius on world day 3 = {m.radius_at(3.0, P):,.0f} wu;'
        f' halfway line (1,040 wu) on world day {m.day_reaching(1040.0, P):.1f}; pass mouth (1,200 wu) on day'
        f' {m.day_reaching(1200.0, P):.1f}.')
  for g in (0.5, 0.75, 1.0):
    q = replace(P, growth_exponent=g)
    print(f'- growth exponent {g}: halfway line on day {m.day_reaching(1040.0, q):.1f}; a newcomer protection shorter'
          f' than that loses both valley lenses to plateau neighbours')
  print()


# ---------------------------------------------------------------- churn: a months-old world
class ChurnWorld:
  """A world over a year of world days at the always-on pace. Stewards arrive (a launch burst, then a daily Poisson
  demand), found at the best-ranked free site (WLD-140), play for an exponential active lifetime, are away on the map
  under the deputy for 21 days (PIL-078), then dormant: the site frees and the place is held for `place_days`
  (WLD-005). A border area is held by the first side to reach its line once both sites are occupied, never by the
  side facing a newcomer under protection (WLD-063, as WLD-064 intends), and becomes unheld when its holder leaves."""

  VALLEY_REACH = 5.0
  PASS_REACH = 8.5

  def __init__(self, m, capacity, seed, launch_count, launch_days, demand, mean_life, place_days=42.0,
               dormancy_days=21.0, newcomer_days=3.0, count_absent=True):
    self.m = m
    self.rng = random.Random(seed * 7919 + capacity)
    self.lay = m.Layout(capacity, seed, m.P)
    self.cap = capacity
    self.ai = set(self.lay.place_ai(self.lay.size['ai']))
    n = len(self.lay.sites)
    self.nb = {s: self.lay.neighbours(s) for s in range(n)}
    self.occ = {s: ('ai', -1, 0.0) for s in self.ai}   # site -> (kind, steward id, founded day)
    self.st = {}                                        # id -> dict
    self.edges = [(a, b, 'valley') for a, b in self.lay.valley_pairs] + [(a, b, 'pass') for a, b in self.lay.passes]
    self.holder = [None] * len(self.edges)
    self.edge_of = {}
    for e, (a, b, _) in enumerate(self.edges):
      self.edge_of.setdefault(a, []).append(e)
      self.edge_of.setdefault(b, []).append(e)
    self.launch_count, self.launch_days = launch_count, launch_days
    self.demand, self.mean_life = demand, mean_life
    self.place_days, self.dormancy_days, self.newcomer_days = place_days, dormancy_days, newcomer_days
    self.count_absent = count_absent
    self.refused = 0
    self.arrived = 0
    self.next_id = 0
    self.launch_carry = 0.0

  def places_used(self, day):
    used = 0
    for s in self.st.values():
      if s['state'] in ('active', 'absent'):
        used += 1 if (s['state'] == 'active' or self.count_absent) else 0
      elif s['state'] == 'dormant' and day < s['release']:
        used += 1
    return used

  def score(self, site):
    v = sum(1 for q, kind in self.nb[site] if q in self.occ and kind == 'valley')
    pz = sum(1 for q, kind in self.nb[site] if q in self.occ and kind == 'pass')
    return (-(2 * v + pz), self.lay.sites[site][2], site)

  def found(self, day):
    free = [s for s in range(len(self.lay.sites)) if s not in self.occ]
    pick = min(free, key=self.score)
    sid = self.next_id
    self.next_id += 1
    life = self.rng.expovariate(1.0 / self.mean_life)
    self.st[sid] = dict(site=pick, founded=day, stop=day + life, state='active', release=math.inf)
    self.occ[pick] = ('st', sid, day)

  def protection_end(self, site):
    kind, sid, founded = self.occ[site]
    return founded + self.newcomer_days if kind == 'st' else -math.inf

  def update_edges(self, day):
    for e, (a, b, kind) in enumerate(self.edges):
      if self.holder[e] is not None:
        if self.holder[e] not in self.occ:
          self.holder[e] = None
        else:
          continue
      if a not in self.occ or b not in self.occ:
        continue
      reach = self.VALLEY_REACH if kind == 'valley' else self.PASS_REACH
      ea = max(self.occ[a][2] + reach, self.protection_end(b))
      eb = max(self.occ[b][2] + reach, self.protection_end(a))
      first = min(ea, eb)
      if day >= first:
        self.holder[e] = a if (ea < eb or (ea == eb and a < b)) else b

  def step(self, day):
    for s in self.st.values():
      if s['state'] == 'active' and day >= s['stop']:
        s['state'] = 'absent'
      if s['state'] == 'absent' and day >= s['stop'] + self.dormancy_days:
        s['state'] = 'dormant'
        s['release'] = s['stop'] + self.dormancy_days + self.place_days
        del self.occ[s['site']]
    if day < self.launch_days:
      self.launch_carry += self.launch_count / self.launch_days
      arrivals = int(self.launch_carry)
      self.launch_carry -= arrivals
    else:
      arrivals = self.poisson(self.demand)
    for _ in range(arrivals):
      self.arrived += 1
      if self.places_used(day) < self.cap:
        self.found(day)
      else:
        self.refused += 1
    self.update_edges(day)

  def poisson(self, lam):
    k, p, L = 0, 1.0, math.exp(-lam)
    while True:
      p *= self.rng.random()
      if p < L:
        return k
      k += 1

  def ai_lens_share(self):
    held, by_ai = 0, 0
    for e, (a, b, kind) in enumerate(self.edges):
      if kind != 'valley' or self.holder[e] is None:
        continue
      if a not in self.occ or b not in self.occ or self.holder[e] not in self.occ:
        continue
      ka, kb = self.occ[a][0], self.occ[b][0]
      if {ka, kb} == {'ai', 'st'}:
        held += 1
        if self.occ[self.holder[e]][0] == 'ai':
          by_ai += 1
    return by_ai / held if held else float('nan')

  def snapshot(self, day):
    active = sum(1 for s in self.st.values() if s['state'] == 'active')
    absent = sum(1 for s in self.st.values() if s['state'] == 'absent')
    dormant_held = sum(1 for s in self.st.values() if s['state'] == 'dormant' and day < s['release'])
    on_map = list(self.occ)
    deg = {s: sum(1 for q, _ in self.nb[s] if q in self.occ) for s in on_map}
    st_sites = [s for s in on_map if self.occ[s][0] == 'st']
    zero = sum(1 for s in st_sites if deg[s] == 0)
    human = sum(1 for s in st_sites if any(q in self.occ and self.occ[q][0] == 'st' for q, _ in self.nb[s]))
    held_units = {}
    for e, (a, b, kind) in enumerate(self.edges):
      h = self.holder[e]
      if h is not None:
        held_units[h] = held_units.get(h, 0) + (1 if kind == 'valley' else 2)
    early = [8 + held_units.get(s, 0) for s in st_sites if self.occ[s][2] < self.launch_days]
    late = [8 + held_units.get(s, 0) for s in st_sites if self.occ[s][2] >= 28.0]
    late_valley_lost = 0
    late_n = 0
    for s in st_sites:
      if self.occ[s][2] >= 28.0:
        late_n += 1
        lost = all(self.holder[e] not in (None, s) for e in self.edge_of.get(s, []) if self.edges[e][2] == 'valley'
                   and self.holder[e] is not None)
        has_valley = [e for e in self.edge_of.get(s, []) if self.edges[e][2] == 'valley' and self.holder[e] is not None]
        if has_valley and lost:
          late_valley_lost += 1
    free_sites = len(self.lay.sites) - len(on_map)
    good_free = sum(1 for s in range(len(self.lay.sites)) if s not in self.occ and -self.score(s)[0] >= 2)
    return dict(day=day, active=active, absent=absent, dormant_held=dormant_held,
                free_places=self.cap - self.places_used(day), free_sites=free_sites, good_free=good_free,
                refused=self.refused, arrived=self.arrived, on_map=len(on_map),
                per_region=len(on_map) / self.lay.size['regions'],
                ai_share=len(self.ai) / len(on_map), mean_deg=sum(deg.values()) / max(1, len(deg)),
                st_zero=zero / max(1, len(st_sites)), st_no_human=1 - human / max(1, len(st_sites)),
                ghosts=absent / max(1, active + absent),
                early_units=(sum(early) / len(early)) if early else float('nan'),
                late_units=(sum(late) / len(late)) if late else float('nan'),
                ai_lens_share=self.ai_lens_share(),
                late_lost_all=late_valley_lost / max(1, late_n), late_n=late_n)


def churn_run(m, capacity, seeds, days=(30, 90, 180, 365), **kw):
  out = {d: [] for d in days}
  for seed in seeds:
    w = ChurnWorld(m, capacity, seed, **kw)
    for day in range(0, max(days) + 1):
      w.step(float(day))
      if day in out:
        out[day].append(w.snapshot(float(day)))
  avg = {}
  for d, snaps in out.items():
    avg[d] = {}
    for k in snaps[0]:
      vals = [s[k] for s in snaps if not (isinstance(s[k], float) and math.isnan(s[k]))]
      avg[d][k] = sum(vals) / len(vals) if vals else float('nan')
  return avg


def churn_check(m):
  print('## C4 A months-old world: arrivals, churn, dormancy and held places (WLD-005, WLD-063, WLD-140; PIL-078)\n')
  print('Mean active lifetime 43 world days is the A0 challenger\'s churn of half the stewards a month; 120 days is an'
        ' engaged community. Newcomer protection 3 world days (B6 has not set it). Five seeds each.\n')
  scen = (
    ('200, strong demand', 200, dict(launch_count=200, launch_days=14, demand=2.0, mean_life=43.0)),
    ('200, strong demand, no place held', 200, dict(launch_count=200, launch_days=14, demand=2.0, mean_life=43.0,
                                                    place_days=0.0)),
    ('200, weak demand', 200, dict(launch_count=120, launch_days=14, demand=0.5, mean_life=43.0)),
    ('200, engaged, strong demand', 200, dict(launch_count=200, launch_days=14, demand=2.0, mean_life=120.0)),
    ('200, engaged, strong demand, no place held', 200, dict(launch_count=200, launch_days=14, demand=2.0,
                                                             mean_life=120.0, place_days=0.0)),
    ('16, friends, a joiner every 4 days', 16, dict(launch_count=16, launch_days=7, demand=0.25, mean_life=43.0)),
    ('16, same, no place held', 16, dict(launch_count=16, launch_days=7, demand=0.25, mean_life=43.0, place_days=0.0)),
    ('16, friends, a joiner every 20 days', 16, dict(launch_count=12, launch_days=7, demand=0.05, mean_life=60.0)),
  )
  print('| scenario | day | active | away on map (ghost share) | dormant, place held | free places | free sites'
        ' (beside a settlement) | refused / arrived | settlements per region | AI share | mean contacts |'
        ' stewards with no contact | stewards with no steward neighbour | resource units: launch cohort, joined after'
        ' day 28 | joined after day 28 who hold no valley lens | AI-steward lenses held by the AI |')
  print('|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|')
  for name, cap, kw in scen:
    res = churn_run(m, cap, (1, 2, 3, 4, 5), **kw)
    for d in (30, 90, 180, 365):
      s = res[d]
      print(f"| {name} | {d} | {s['active']:.1f} | {s['absent']:.1f} ({pct(s['ghosts'])}) | {s['dormant_held']:.1f} |"
            f" {s['free_places']:.1f} | {s['free_sites']:.0f} ({s['good_free']:.0f}) |"
            f" {s['refused']:.0f} / {s['arrived']:.0f} | {s['per_region']:.2f} | {pct(s['ai_share'])} |"
            f" {s['mean_deg']:.2f} | {pct(s['st_zero'])} | {pct(s['st_no_human'])} |"
            f" {fnum(s['early_units'])}, {fnum(s['late_units'])} | {pct(s['late_lost_all'])} |"
            f" {pct(s['ai_lens_share']) if not math.isnan(s['ai_lens_share']) else '-'} |")
  print()


# ---------------------------------------------------------------- battles in progress
def quantile_compound(m, lam, k, q):
  return k * m.poisson_quantile(lam / k, q)


def battles_check(m):
  P = m.P
  print('## C5 Battles in progress against the model\'s assumptions (WLD-280; PIL-047, PIL-065)\n')
  print('Concurrency at the evening peak of one time zone; k is the number of battles that start together (a'
        ' Cladan offensive, a valley war); "week max" is the quantile 1 - 1/n over the n battle-length windows of a'
        ' week\'s peak hours. 400 bodies per battle is PIL-021\'s typical battle.\n')
  print('| stewards | battles per settlement per week | battle length (min) | steward share | evening window (h) | k |'
        ' mean at peak | p99 | p99.9 | week max | bodies in battle at week max |')
  print('|---|---|---|---|---|---|---|---|---|---|---|')
  cases = ((8.0, 0.5, 5.0, 1), (12.0, 0.5, 5.0, 1), (8.0, 0.8, 5.0, 1), (8.0, 0.5, 3.0, 1), (8.0, 0.5, 5.0, 2),
           (12.0, 0.8, 3.0, 2), (12.0, 0.8, 3.0, 4))
  for cap, n in ((16, 26), (200, 258)):
    for b in (6.0, 10.0):
      for dur, share, win, k in cases:
        per_hour = n * b / 2.0 / 168.0
        peak = per_hour * ((1 - share) + share * 24.0 / win)
        lam = peak * dur / 60.0
        windows = 7 * win * 60.0 / dur
        wq = 1 - 1.0 / windows
        p99 = quantile_compound(m, lam, k, 0.99)
        p999 = quantile_compound(m, lam, k, 0.999)
        wmax = quantile_compound(m, lam, k, wq)
        print(f'| {cap} | {b:g} | {dur:g} | {share:g} | {win:g} | {k} | {lam:.2f} | {p99} | {p999} | {wmax} |'
              f' {wmax * 400:,} |')
  print('\nSession pace, 16 stewards (an integrated world: every battle runs on the host\'s PC):\n')
  print('| battles per settlement per week | battle length (min) | k | mean | p99 | p99.9 |')
  print('|---|---|---|---|---|---|')
  for b in (6.0, 10.0):
    for dur, k in ((8.0, 1), (12.0, 1), (12.0, 2)):
      lam = 26 * b / 2.0 * P.session_factor / 168.0 * dur / 60.0
      print(f'| {b:g} | {dur:g} | {k} | {lam:.2f} | {quantile_compound(m, lam, k, 0.99)} |'
            f' {quantile_compound(m, lam, k, 0.999)} |')
  print()


# ---------------------------------------------------------------- watched union
def watched_check(m):
  P = m.P
  print('## C6 Watched settlements: the union, and expedition views (WLD-244, WLD-280; PIL-057)\n')
  _, peak_share = m.present_share(P)
  rng = random.Random(11)
  print('| stewards | occupancy | stewards on map | present at peak | model typical; at most | union typical | union at'
        ' most | union at most + 1 expedition view each | + 3 expedition views each |')
  print('|---|---|---|---|---|---|---|---|---|')
  for cap in (16, 200):
    for share in (0.25, 1.0):
      lay = m.Layout(cap, 1, P)
      ai = lay.place_ai(lay.size['ai'])
      st = lay.found_stewards(int(round(cap * share)), ai)
      occ = set(ai) | set(st)
      present_n = max(1, int(round(len(st) * peak_share)))
      diag = {0: 3, 1: 2, 2: 1, 3: 0}
      res = {'typ': 0.0, 'max': 0.0, 'e1': 0.0, 'e3': 0.0}
      draws = 200
      occ_list = sorted(occ)
      for _ in range(draws):
        pres = rng.sample(st, present_n)
        typ, top = set(), set()
        for s in pres:
          typ.add(s)
          top.add(s)
          for q, kind in lay.neighbours(s):
            if q in occ:
              top.add(q)
              if kind == 'valley':
                typ.add(q)
          r, k = lay.sites[s][2], lay.sites[s][3]
          dq = lay.site_of(r, diag[k])
          if dq in occ:
            top.add(dq)
        e1, e3 = set(top), set(top)
        for s in pres:
          e1.add(rng.choice(occ_list))
          for _ in range(3):
            e3.add(rng.choice(occ_list))
        res['typ'] += len(typ)
        res['max'] += len(top)
        res['e1'] += len(e1)
        res['e3'] += len(e3)
      mt = m.watched(cap, len(occ), P)
      print(f"| {cap} | {pct(share)} | {len(st)} | {present_n} | {mt[0]}; {mt[1]} | {res['typ'] / draws:.0f} |"
            f" {res['max'] / draws:.0f} | {res['e1'] / draws:.0f} | {res['e3'] / draws:.0f} |")
  print()


# ---------------------------------------------------------------- ridge crossing
def worker_ridge_seconds(h, half=320.0, speed=20.0, dt=0.1, k=3.0, fmin=0.3, fmax=2.0):
  """Seconds for a Worker to walk straight over a ridge band (a triangular profile of rise h over `half` wu each
  side), with the slope factor of R-UNIT-042 (f = 1 + (h_now - h_next) / k, clamped) and no slope block (R-UNIT-105)."""
  g = h / half
  t, x = 0.0, 0.0
  while x < 2 * half:
    step = speed * dt
    rise = g * step if x < half else -g * step
    f = min(fmax, max(fmin, 1.0 - rise / k))
    x += speed * f * dt
    t += dt
  return t


def walk_cost_straight(flat, h, half=320.0, uphill=1.5):
  g = h / half
  seg = half * math.sqrt(1 + g * g)
  return flat + seg * (1 + uphill * g) + seg


def ridge_check(m):
  P = m.P
  print('## C7 A border ridge against a Worker\'s straight walk (WLD-021, WLD-100, WLD-102; R-UNIT-042, R-SIM-351)\n')
  for h in (160.0, 300.0, 500.0):
    t = worker_ridge_seconds(h)
    print(f'- ridge rise {h:.0f} wu: a Worker crosses the 640 wu band in {t:.0f} s (32 s on flat ground);'
          f' WalkCost of the band {walk_cost_straight(0.0, h):,.0f} wu against 640 wu flat')
  lay = m.Layout(16, 1, replace(P, site_jitter_wu=0.0))
  pairs = []
  for r, (i, j) in enumerate(lay.cells):
    east = lay.region_index.get((i + 1, j))
    if east is None:
      continue
    linked = {(a, b) for a, b in lay.passes} | {(b, a) for a, b in lay.passes}
    for ka in (1, 3):
      for kb in (0, 2):
        a, b = lay.site_of(r, ka), lay.site_of(east, kb)
        if (a, b) in linked:
          continue
        pairs.append((a, b))
    break
  print('\n| pair across one region border, not linked by a pass | straight (wu) | over the ridge at 20 wu/s, rise 300'
        ' (min) | travel network (wu) | network ETA (min) | warning that the target actually gets, of 10 min |')
  print('|---|---|---|---|---|---|')
  for a, b in pairs:
    d = lay.dist(a, b)
    walk = (d - 640.0) / 20.0 + worker_ridge_seconds(300.0)
    net = lay.shortest_from(a)[b]
    eta = net / 20.0
    got = 600.0 - (eta - walk)
    print(f'| site {lay.sites[a][3]} to site {lay.sites[b][3]} of the next region | {d:,.0f} | {walk / 60:.1f} |'
          f' {net:,.0f} | {eta / 60:.1f} | {max(0.0, got) / 60:.1f} min |')
  print('\nA Speed Boost (R-UNIT-123: 40 then 80 wu/s) taken after a warning sent on an ETA at 20 wu/s leaves the'
        f' target {10 / 2:.0f} min or {10 / 4:.1f} min of its 10.\n')


# ---------------------------------------------------------------- capacity raise
class GrownLayout:
  pass


def extend_blob(m, base_cells, target, seed, centre):
  chosen = list(base_cells)
  chosen_set = set(chosen)
  while len(chosen) < target:
    cand = set()
    for i, j in chosen:
      for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        c = (i + di, j + dj)
        if c not in chosen_set:
          cand.add(c)
    best = min(cand, key=lambda c: (math.hypot(c[0] - centre[0], c[1] - centre[1]) + 0.35 * m.unit(seed, 1, c[0], c[1]),
                                    c))
    chosen.append(best)
    chosen_set.add(best)
  return chosen


def layout_with_cells(m, capacity, cells, seed, ai_count):
  P = m.P

  class L(m.Layout):
    def grow_blob(self, n, side):
      return list(cells)

  lay = L(capacity, seed, P, ai_override=ai_count)
  return lay


def contact_after(lay, ai_sites, stewards):
  st = lay.found_stewards(stewards, ai_sites)
  deg = lay.contact_degrees(list(ai_sites) + st)
  vals = list(deg.values())
  st_vals = [deg[s] for s in st]
  return (sum(vals) / len(vals), sum(st_vals) / max(1, len(st_vals)),
          sum(1 for v in st_vals if v == 0) / max(1, len(st_vals)))


def extension_check(m):
  P = m.P
  print('## C8 The path from 16 to 200 by a capacity raise (WLD-001, WLD-003, WLD-008, WLD-120)\n')
  seed = 1
  base = m.Layout(16, seed, P)
  g0 = m.blob_grid_side(7)
  centre = ((g0 - 1) / 2.0, (g0 - 1) / 2.0)
  grown = extend_blob(m, base.cells, 65, seed, centre)
  imin = min(c[0] for c in grown)
  jmin = min(c[1] for c in grown)
  imax = max(c[0] for c in grown)
  jmax = max(c[1] for c in grown)
  print(f'A 16-steward world (7 regions on grid cells 0 to {g0 - 1}) raised to 65 regions, each new region grown onto a'
        f' sea cell that touches the blob in the order of WLD-003 (distance from the original centre plus the jitter):'
        f' land cells run from i = {imin} to {imax} and j = {jmin} to {jmax}. WLD-001 puts the extent\'s corner at'
        f' (0, 0) with one sea cell on every side, so the original regions start at x = 5,120 wu; after the raise the'
        f' land reaches {max(0, -imin)} cells and {max(0, -jmin)} cells below the original land, so every stored'
        f' position of the world must move by'
        f' ({max(0, -imin) * 5120:,} wu, {max(0, -jmin) * 5120:,} wu) for the corner to stay at (0, 0).\n')
  fresh = m.Layout(200, seed, P)
  fresh_ai = fresh.place_ai(fresh.size['ai'])
  raised = layout_with_cells(m, 200, grown, seed, 10)
  raised_ai = raised.place_ai(10)
  ai_regions_fresh = len({fresh.sites[s][2] for s in fresh_ai})
  ai_regions_raised = len({raised.sites[s][2] for s in raised_ai})
  print('| world of 200 | AI settlements | regions with an AI settlement | occupancy | mean contacts |'
        ' a steward\'s contacts | stewards with no contact |')
  print('|---|---|---|---|---|---|---|')
  for name, lay, ai, nreg in (('created at 200', fresh, fresh_ai, ai_regions_fresh),
                              ('raised from 16 (AI count fixed, WLD-120)', raised, raised_ai, ai_regions_raised)):
    for share in (0.25, 0.5, 1.0):
      mean, st_mean, zero = contact_after(lay, ai, int(round(200 * share)))
      print(f'| {name} | {len(ai)} | {nreg} of 65 | {pct(share)} | {mean:.2f} | {st_mean:.2f} | {pct(zero)} |')
  print()


# ---------------------------------------------------------------- AI count, host choices, passes
def ai_check(m):
  P = m.P
  print('## C9 AI count, regions without AI, a host who lowers the AI count, passes per site (WLD-004, WLD-042,'
        ' WLD-120, WLD-121)\n')
  short = []
  for cap in range(1, 257):
    s = m.world_size(cap, P)
    if s['ai'] < s['regions']:
      short.append((cap, s['ai'], s['regions']))
  print(f'Capacities at which AI settlements are fewer than regions (some valleys start empty): {len(short)} of 256;'
        f' the first is {short[0][0] if short else "none"}; at 200: {m.world_size(200, P)["ai"]} AI for'
        f' {m.world_size(200, P)["regions"]} regions; at 256: {m.world_size(256, P)["ai"]} AI for'
        f' {m.world_size(256, P)["regions"]} regions.')
  half_up = 0
  for cap in range(1, 257):
    a0 = math.floor(P.ai_base + P.ai_per_steward * cap + 0.5)
    r_half = max(P.min_regions, math.ceil((cap + a0 + P.free_sites_min) / P.sites_per_region))
    if r_half != m.world_size(cap, P)['regions']:
      half_up += 1
  print(f'Capacities whose region count differs between Python round() (ties to even) and rounding half up:'
        f' {half_up}.\n')
  print('| 16 stewards, AI count set by the host | occupancy | mean contacts | a steward\'s contacts |'
        ' stewards with no contact |')
  print('|---|---|---|---|---|')
  for ai_n in (10, 6, 2):
    for share in (0.25, 0.5, 1.0):
      lay = m.Layout(16, 1, P, ai_override=ai_n)
      ai = lay.place_ai(ai_n)
      mean, st_mean, zero = contact_after(lay, ai, int(round(16 * share)))
      print(f'| {ai_n} | {pct(share)} | {mean:.2f} | {st_mean:.2f} | {pct(zero)} |')
  print('\n| stewards | sites with 0 passes | 1 pass | 2 passes | most resource units a site can hold (8 + 2 lenses +'
        ' 2 per pass): 0 passes, 1 pass |')
  print('|---|---|---|---|---|')
  for cap in (1, 16, 200):
    lay = m.Layout(cap, 1, P)
    cnt = [0, 0, 0, 0, 0]
    per = [0] * len(lay.sites)
    for a, b in lay.passes:
      per[a] += 1
      per[b] += 1
    for v in per:
      cnt[v] += 1
    print(f'| {cap} | {cnt[0]} ({pct(cnt[0] / len(per))}) | {cnt[1]} | {cnt[2]} | 10, 12 |')
  print()


def found_variant(lay, count, ai_sites, steward_bonus):
  """Ranked founding (WLD-140) with an extra weight for a neighbour that is a steward's settlement."""
  occ = {s: 'ai' for s in ai_sites}
  nb = {s: lay.neighbours(s) for s in range(len(lay.sites))}
  placed = []
  for _ in range(count):
    def score(s):
      v = sum((2 + (steward_bonus if occ[q] == 'st' else 0)) for q, kind in nb[s] if q in occ and kind == 'valley')
      pz = sum((1 + (steward_bonus / 2 if occ[q] == 'st' else 0)) for q, kind in nb[s] if q in occ and kind == 'pass')
      return (-(v + pz), lay.sites[s][2], s)
    free = [s for s in range(len(lay.sites)) if s not in occ]
    pick = min(free, key=score)
    occ[pick] = 'st'
    placed.append(pick)
  return placed, occ


def adjacency_check(m):
  P = m.P
  print('## C10 Do stewards meet each other? Steward neighbours in a fresh world (WLD-121, WLD-140; Table B of A0)\n')
  print('| stewards | founded | AI placement | ranking | stewards with no steward neighbour | with no steward valley'
        ' neighbour | mean contacts |')
  print('|---|---|---|---|---|---|---|')
  for cap, counts in ((16, (4, 8, 16)), (200, (50, 100, 200))):
    for n in counts:
      for rule in ('one_per_region', 'diagonal_pairs'):
        for bonus, label in ((0, 'WLD-140'), (3, 'a steward valley neighbour scores 5, an AI one 2')):
          lay = m.Layout(cap, 1, P)
          ai = lay.place_ai(lay.size['ai'], rule)
          st, occ = found_variant(lay, n, ai, bonus)
          none, no_valley = 0, 0
          for s in st:
            nbs = lay.neighbours(s)
            if not any(occ.get(q) == 'st' for q, _ in nbs):
              none += 1
            if not any(occ.get(q) == 'st' for q, kind in nbs if kind == 'valley'):
              no_valley += 1
          deg = lay.contact_degrees(list(occ))
          print(f'| {cap} | {n} | {rule} | {label} | {pct(none / n)} | {pct(no_valley / n)} |'
                f' {sum(deg.values()) / len(deg):.2f} |')
  print()


def main(argv):
  if len(argv) < 2:
    print(__doc__)
    return 2
  m = load_model(argv[1])
  mode = argv[2] if len(argv) > 2 else 'all'
  checks = dict(speed=speed_check, jitter=jitter_check, late=late_check, churn=churn_check, battles=battles_check,
                watched=watched_check, ridge=ridge_check, extension=extension_check, ai=ai_check,
                adjacency=adjacency_check)
  if mode == 'all':
    print('# A1 challenge model, round 1\n')
    for fn in checks.values():
      fn(m)
    return 0
  if mode in checks:
    checks[mode](m)
    return 0
  print(__doc__)
  return 2


if __name__ == '__main__':
  sys.exit(main(sys.argv))
