#!/usr/bin/env python3
"""A1WorldDensity.py: the world-density model of area A1 (world and regions).

It implements the world geometry of Design/gdd/01_world_and_regions.md (prefix WLD) and computes, for a world of a
given steward capacity: the number of regions, settlement sites and AI settlements; settlements per region; travel
times between neighbours, across a region and across the world; the contact rate (when territories meet, how many
neighbours each settlement is in contact with, at several occupancies); and the scale figures that area A2 sizes the
server with (bodies per region, battles and expeditions in progress, watched settlements, terrain samples). Every
figure is given for 16 and for 200 stewards, at the always-on and at the session world pace (PIL-100, PIL-101).

Usage:
  python3 -I Tools/DesignModels/A1WorldDensity.py [check | report | sweep | layout <capacity> [<seed>] | ai]

  check    (default) compute the figures that the document states and compare them with the stated values; prints
           one line per figure and exits with status 1 when any differs
  report   the full report at 1 to 200 stewards: world size, geometry, travel, contact, scale
  sweep    first-contact days, lens width and valley-centre clearance over site spacing, largest territory
           radius and the territory growth exponent
  layout   the generated layout of one world (regions, sites, passes) as text, for a capacity and a seed
  ai       the AI-settlement rules compared: contact degree at 25, 50 and 100 % steward occupancy

Inputs: the tuning values of the document (class Params below; each field names its tuning key) and the values of
other areas that the model needs before those areas exist, each marked "placeholder" with its owner. Outputs: text
tables on standard output. Python 3 with the standard library only; no input files. The layout jitter is a
counter-based hash of (world seed, item, index) (SplitMix64), so a figure depends only on its own inputs.

Geometry (WLD §2): a world is a compact blob of square regions on a grid, ringed by sea; each region is a valley of
four settlement sites in a 2 x 2 pattern, enclosed by a border ridge that passes cross; valley neighbours share a
border area (the lens where their largest territories overlap); a pass links one site on each side of a shared region
side, and its corridor is a border area too; the centre of every valley holds a contested resource site.
"""
import math
import sys
from dataclasses import dataclass, replace


# ---------------------------------------------------------------- inputs
@dataclass(frozen=True)
class Params:
  # world and regions (WLD tuning keys)
  region_side_wu: float = 5120.0          # wld.region.side
  sites_per_region: int = 4               # wld.region.sites (the 2 x 2 valley; the model supports only 4)
  site_spacing_wu: float = 2080.0         # wld.site.spacing: valley neighbours, centre to centre
  site_jitter_wu: float = 128.0           # wld.site.jitter: per axis, uniform in +- this
  ridge_half_band_wu: float = 320.0       # wld.region.ridge_half_width
  pass_width_wu: float = 384.0            # wld.pass.width
  core_radius_wu: float = 400.0           # wld.site.core_radius
  founding_radius_wu: float = 500.0       # wld.territory.founding_radius
  max_radius_wu: float = 1400.0           # wld.territory.max_radius
  sea_ring_regions: int = 1               # wld.world.sea_ring
  min_regions: int = 3                    # wld.world.min_regions
  cell_size_wu: float = 16.0              # wld.terrain.cell_size
  chunk_side_wu: float = 1024.0           # wld.terrain.chunk_side
  # free sites and AI settlements (WLD)
  free_sites_min: int = 2                 # pil.world.free_sites_min (PIL-068)
  ai_base: float = 6.0                    # wld.ai.base_count
  ai_per_steward: float = 0.25            # wld.ai.per_steward
  # travel (WLD)
  valley_path_factor: float = 1.15        # wld.travel.valley_path_factor
  pass_path_factor: float = 1.20          # wld.travel.pass_path_factor
  march_speed_wus: float = 20.0           # ref.unit.worker.speed (reference/02); B4 sets the expedition speed
  formation_speed_wus: float = 12.0       # ref.unit.leader.speed x ref.unit.leader.formation_speed_factor
  warning_min: float = 10.0               # pil.conflict.warning_minutes (PIL-025)
  session_factor: float = 12.0            # pil.pace.session_factor (PIL-101)
  # growth (placeholders until B1)
  plateau_days: float = 14.0              # pil.settlement.plateau_days (PIL-055)
  growth_exponent: float = 0.5            # placeholder, B1: territory radius r0 + (rmax - r0) (t / Tp)^g
  pop_plateau: int = 600                  # placeholder, B1: Workers and Leaders of a settlement at its plateau
  # world loop (A0 targets, placeholders until B4, B5, B6)
  battles_per_week: tuple = (2.0, 6.0, 10.0)  # pil.world.battles_per_week (PIL-065), low, middle, high
  battle_mean_min: float = 8.0            # middle of pil.battle.typical_minutes (PIL-047)
  steward_attack_share: float = 0.5       # placeholder, B5: share of attacks declared by present stewards
  evening_window_h: float = 5.0           # placeholder, B6: the hours of a time zone in which present play falls
  gather_trips_per_day: float = 6.0       # placeholder, B2: gathering expeditions per settlement per world day
  gather_stay_min: float = 5.0            # placeholder, B2: minutes an expedition gathers at a resource site
  watched_per_present: int = 3            # wld.scale.watched_per_present
  watched_max_per_present: int = 6        # wld.scale.watched_max_per_present
  typical_active_min: float = 60.0        # pil.session.typical_active_minutes (PIL-102)
  typical_check_ins: float = 2.0          # pil.session.typical_check_ins (PIL-102)
  check_in_min: float = 5.0               # middle of pil.session.check_in_minutes (PIL-074)


P = Params()

# The figures that the document states (WLD §1.3 Table 1, §2.15 Table 9). check compares the model with them.
STATED = {
  # capacity: (regions, sites, AI settlements, world side in wu, land samples in millions)
  'size_1': (3, 12, 9, 20480, 0.307),
  'size_16': (7, 28, 10, 25600, 0.717),
  'size_200': (65, 260, 58, 61440, 6.656),
  # travel, minutes at march speed: Wellspring to the lens, valley neighbour, pass neighbour, region hop
  'travel_lens_min': 0.9,
  'travel_valley_min': 2.0,
  'travel_pass_min': 3.0,
  'travel_hop_min': 5.9,
  # world diameter over the travel network, minutes at march speed (16, 200)
  'diameter_16_min': 23,
  'diameter_200_min': 66,
  # contact: first valley contact day, first pass contact day, late joiner's contact day (always-on), lens width
  'first_valley_day': 5.0,
  'first_pass_day': 8.5,
  'late_joiner_day': 0.6,
  'lens_width_wu': 720,
  'centre_clearance_wu': 71,
  'territory_plateau_mwu2': 4.84,
  # contact degree at plateau: mean territory contacts per settlement and per steward's settlement, by occupancy
  'degree_16': {0.25: (1.7, 3.0), 0.5: (2.2, 2.5), 1.0: (2.5, 2.5)},
  'degree_200': {0.25: (1.9, 2.0), 0.5: (2.5, 2.7), 1.0: (2.8, 2.8)},
  # settlements per region by occupancy (16, 200)
  'per_region_16': {0.25: 2.0, 0.5: 2.6, 1.0: 3.7},
  'per_region_200': {0.25: 1.7, 0.5: 2.4, 1.0: 4.0},
  # battles in progress, 99th percentile at the evening peak, at 2, 6 and 10 battles per settlement per week
  'battles_p99_16': (1, 2, 2),
  'battles_p99_200': (3, 5, 8),
  'battles_p99_16_session': (2, 3, 4),
  # expeditions in progress, mean, at 6 battles per settlement per week (16, 200)
  'expeditions_16': 1.0,
  'expeditions_200': 9.9,
  # stewards present at the evening peak of one time zone (16, 200)
  'present_16': 3.7,
  'present_200': 46.7,
  # watched settlements outside battles at the evening peak: typical and at most (16, 200)
  'watched_16': (11, 22),
  'watched_200': (140, 258),
  # world diameter of a solo world, minutes at march speed
  'diameter_1_min': 14,
  # bodies per full region at the placeholder plateau population
  'bodies_region': 2400,
}


# ---------------------------------------------------------------- counter-based jitter (SplitMix64)
def mix64(x):
  x = (x + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
  x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
  x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
  return x ^ (x >> 31)


def unit(seed, *keys):
  """A value in [-1, 1) that depends only on the seed and the keys."""
  h = mix64(seed)
  for k in keys:
    h = mix64(h ^ (k & 0xFFFFFFFFFFFFFFFF))
  return (h >> 11) / float(1 << 52) - 1.0


# ---------------------------------------------------------------- world size (WLD-004 to WLD-008)
def world_size(capacity, p=P, ai_override=None):
  """Regions, sites and AI settlements of a world of `capacity` stewards (WLD-005, WLD-121)."""
  ai = ai_override if ai_override is not None else max(0, int(round(p.ai_base + p.ai_per_steward * capacity)))
  need = capacity + ai + p.free_sites_min
  regions = max(p.min_regions, math.ceil(need / p.sites_per_region))
  sites = regions * p.sites_per_region
  if ai_override is None:
    ai = sites - capacity - p.free_sites_min   # the last region is filled with AI settlements (WLD-121)
  grid = blob_grid_side(regions) + 2 * p.sea_ring_regions
  side = grid * p.region_side_wu
  land_samples = regions * (p.region_side_wu / p.cell_size_wu) ** 2
  return dict(capacity=capacity, ai=ai, sites=sites, regions=regions, grid=grid, side=side,
              land_samples=land_samples, free=sites - ai)


def blob_grid_side(regions):
  """The side of the square that holds the compact land blob of `regions` cells (WLD-003)."""
  return max(1, math.ceil(math.sqrt(regions * 1.25)))


# ---------------------------------------------------------------- layout (WLD-003, WLD-020 to WLD-066)
class Layout:
  """One generated world: land regions, sites, passes, border areas and the travel network."""

  def __init__(self, capacity, seed=1, p=P, ai_override=None):
    self.p = p
    self.seed = seed
    self.size = world_size(capacity, p, ai_override)
    self.cells = self.grow_blob(self.size['regions'], blob_grid_side(self.size['regions']))
    self.region_index = {c: i for i, c in enumerate(self.cells)}
    self.sites = []        # (x, z, region index, local index)
    self.make_sites()
    self.valley_pairs = []  # (a, b) site indices, same region, orthogonal
    self.passes = []        # (a, b, (x, z)) site indices across a shared region side
    self.make_pairs()
    self.graph = self.make_graph()

  def grow_blob(self, n, side):
    """A compact blob of n cells: cells in order of distance from the centre, ties broken by the seed, kept
    connected by construction (WLD-003)."""
    c = (side - 1) / 2.0
    cand = []
    for i in range(side):
      for j in range(side):
        d = math.hypot(i - c, j - c) + 0.35 * unit(self.seed, 1, i, j)
        cand.append((d, i, j))
    cand.sort()
    chosen = [(cand[0][1], cand[0][2])]
    chosen_set = set(chosen)
    rest = [(i, j) for _, i, j in cand[1:]]
    while len(chosen) < n:
      for k, (i, j) in enumerate(rest):
        if any((i + di, j + dj) in chosen_set for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1))):
          chosen.append((i, j))
          chosen_set.add((i, j))
          del rest[k]
          break
    return chosen

  def make_sites(self):
    p = self.p
    off = (p.region_side_wu - p.site_spacing_wu) / 2.0
    for r, (i, j) in enumerate(self.cells):
      x0 = (i + p.sea_ring_regions) * p.region_side_wu
      z0 = (j + p.sea_ring_regions) * p.region_side_wu
      for k, (a, b) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
        jx = p.site_jitter_wu * unit(self.seed, 2, r, k, 0)
        jz = p.site_jitter_wu * unit(self.seed, 2, r, k, 1)
        x = x0 + off + a * p.site_spacing_wu + jx
        z = z0 + off + b * p.site_spacing_wu + jz
        self.sites.append((x, z, r, k))

  def site_of(self, region, local):
    return region * 4 + local

  def make_pairs(self):
    # valley neighbours: local 0-1, 2-3 (along x) and 0-2, 1-3 (along z)
    for r in range(len(self.cells)):
      for a, b in ((0, 1), (2, 3), (0, 2), (1, 3)):
        self.valley_pairs.append((self.site_of(r, a), self.site_of(r, b)))
    # passes: one per shared side; the row (or column) alternates with the parity of the cells (WLD-024)
    for r, (i, j) in enumerate(self.cells):
      east = self.region_index.get((i + 1, j))
      if east is not None:
        row = (i + j) % 2                       # 0: the sites with b = 0, 1: b = 1
        a = self.site_of(r, 1 + 2 * row)        # the east site of this region in that row
        b = self.site_of(east, 0 + 2 * row)     # the west site of the east region in that row
        self.passes.append((a, b))
      north = self.region_index.get((i, j + 1))
      if north is not None:
        col = (i + j + 1) % 2
        a = self.site_of(r, 2 + col)
        b = self.site_of(north, 0 + col)
        self.passes.append((a, b))

  def dist(self, a, b):
    xa, za = self.sites[a][:2]
    xb, zb = self.sites[b][:2]
    return math.hypot(xa - xb, za - zb)

  def make_graph(self):
    """The travel network: sites linked to their valley neighbours and their pass neighbours, and every site of a
    valley linked to its valley's central resource site (WLD-100 to WLD-104)."""
    p = self.p
    n = len(self.sites)
    g = {k: [] for k in range(n + len(self.cells))}
    for a, b in self.valley_pairs:
      w = self.dist(a, b) * p.valley_path_factor
      g[a].append((b, w))
      g[b].append((a, w))
    for a, b in self.passes:
      w = self.dist(a, b) * p.pass_path_factor
      g[a].append((b, w))
      g[b].append((a, w))
    for r, (i, j) in enumerate(self.cells):
      cx = (i + p.sea_ring_regions + 0.5) * p.region_side_wu
      cz = (j + p.sea_ring_regions + 0.5) * p.region_side_wu
      node = n + r
      for k in range(4):
        s = self.site_of(r, k)
        w = math.hypot(self.sites[s][0] - cx, self.sites[s][1] - cz) * p.valley_path_factor
        g[s].append((node, w))
        g[node].append((s, w))
    return g

  def shortest_from(self, src):
    import heapq
    best = {src: 0.0}
    heap = [(0.0, src)]
    while heap:
      d, u = heapq.heappop(heap)
      if d > best.get(u, math.inf):
        continue
      for v, w in self.graph[u]:
        nd = d + w
        if nd < best.get(v, math.inf):
          best[v] = nd
          heapq.heappush(heap, (nd, v))
    return best

  def diameter_wu(self):
    n = len(self.sites)
    worst = 0.0
    for s in range(n):
      d = self.shortest_from(s)
      worst = max(worst, max(d[t] for t in range(n)))
    return worst

  def region_hop_wu(self):
    """The mean travel distance between the central resource sites of two adjacent regions."""
    n = len(self.sites)
    total, count = 0.0, 0
    for r, (i, j) in enumerate(self.cells):
      for di, dj in ((1, 0), (0, 1)):
        q = self.region_index.get((i + di, j + dj))
        if q is not None:
          d = self.shortest_from(n + r)
          total += d[n + q]
          count += 1
    return total / max(1, count)

  # ------------------------------------------------------------ occupancy (WLD-121, WLD-140 to WLD-143)
  def place_ai(self, ai, rule='one_per_region'):
    """The AI settlements of world creation (WLD-121): one per region in ring order, then a second on the site
    diagonal to the first, then the rest; rule 'diagonal_pairs' puts two per region from the start."""
    order = list(range(len(self.cells)))          # cells are already in ring order (grow_blob)
    taken = []
    passes_per_site = [0] * len(self.sites)
    for a, b in self.passes:
      passes_per_site[a] += 1
      passes_per_site[b] += 1
    rounds = ((0,), (3,), (1,), (2,)) if rule == 'one_per_region' else ((0, 3), (1,), (2,))
    for locals_ in rounds:
      for r in order:
        for k in locals_:
          if len(taken) >= ai:
            return taken
          taken.append(self.site_of(r, k))
    return taken

  def neighbours(self, s):
    out = []
    for a, b in self.valley_pairs:
      if a == s:
        out.append((b, 'valley'))
      elif b == s:
        out.append((a, 'valley'))
    for a, b in self.passes:
      if a == s:
        out.append((b, 'pass'))
      elif b == s:
        out.append((a, 'pass'))
    return out

  def found_stewards(self, count, occupied, ranked=True):
    """Stewards found one after another (WLD-140): each takes the best-ranked free site, keeping the spare sites
    of PIL-068 free. Unranked founding takes free sites in a seed-shuffled order."""
    occ = set(occupied)
    free = [s for s in range(len(self.sites)) if s not in occ]
    placed = []
    nb = {s: self.neighbours(s) for s in range(len(self.sites))}
    for t in range(count):
      if ranked:
        def score(s):
          v = sum(1 for q, kind in nb[s] if q in occ and kind == 'valley')
          pz = sum(1 for q, kind in nb[s] if q in occ and kind == 'pass')
          return (-(2 * v + pz), self.sites[s][2], s)
        free.sort(key=score)
        pick = free[0]
      else:
        free.sort(key=lambda s: unit(self.seed, 7, s, t))
        pick = free[0]
      free.remove(pick)
      occ.add(pick)
      placed.append(pick)
    return placed

  def contact_degrees(self, occupied):
    """Territory contacts of each occupied settlement at plateau: its occupied valley neighbours and pass
    neighbours (WLD-060 to WLD-066)."""
    occ = set(occupied)
    deg = {}
    for s in occ:
      deg[s] = sum(1 for q, _ in self.neighbours(s) if q in occ)
    return deg


# ---------------------------------------------------------------- contact timing (WLD-063, WLD-064)
def radius_at(days, p=P):
  f = min(1.0, max(0.0, days / p.plateau_days)) ** p.growth_exponent
  return p.founding_radius_wu + (p.max_radius_wu - p.founding_radius_wu) * f


def day_reaching(radius, p=P):
  if radius <= p.founding_radius_wu:
    return 0.0
  if radius > p.max_radius_wu:
    return math.inf
  f = (radius - p.founding_radius_wu) / (p.max_radius_wu - p.founding_radius_wu)
  return p.plateau_days * f ** (1.0 / p.growth_exponent)


def first_valley_contact_day(p=P):
  """Two valley neighbours founded together meet when each territory reaches half their spacing."""
  return day_reaching(p.site_spacing_wu / 2.0, p)


def pass_mouth_distance(p=P):
  off = (p.region_side_wu - p.site_spacing_wu) / 2.0
  return off - p.ridge_half_band_wu


def first_pass_contact_day(p=P):
  return day_reaching(pass_mouth_distance(p), p)


def late_joiner_contact_day(p=P):
  """A settlement founded next to a neighbour at plateau: contact when its radius reaches spacing - max radius."""
  return day_reaching(p.site_spacing_wu - p.max_radius_wu, p)


def lens_width(p=P):
  return 2 * p.max_radius_wu - p.site_spacing_wu


def centre_clearance(p=P):
  """How far the valley centre lies outside the largest territory of its four sites."""
  return p.site_spacing_wu / math.sqrt(2.0) - p.max_radius_wu


def territory_area_at_plateau(p=P, step=16.0):
  """The area a settlement of a full valley holds at plateau: its home cell (nearer to it than to any valley
  neighbour, inside the largest radius, outside the ridge band) plus one half of each lens, by sampling."""
  off = (p.region_side_wu - p.site_spacing_wu) / 2.0
  sx, sz = off, off
  n_s = [(off + p.site_spacing_wu, off), (off, off + p.site_spacing_wu), (off + p.site_spacing_wu, off +
         p.site_spacing_wu)]
  lo = p.ridge_half_band_wu
  hi = p.region_side_wu - p.ridge_half_band_wu
  area = 0.0
  z = lo + step / 2
  while z < hi:
    x = lo + step / 2
    while x < hi:
      d = math.hypot(x - sx, z - sz)
      if d <= p.max_radius_wu and all(d <= math.hypot(x - a, z - b) for a, b in n_s):
        area += step * step
      x += step
    z += step
  return area


# ---------------------------------------------------------------- travel (WLD-100 to WLD-106)
def minutes(wu, speed):
  return wu / speed / 60.0


def travel_table(p=P):
  off = (p.region_side_wu - p.site_spacing_wu) / 2.0
  valley = p.site_spacing_wu * p.valley_path_factor
  pass_ = 2 * off * p.pass_path_factor
  to_border = p.site_spacing_wu / 2.0        # from a Wellspring to the lens at contact
  centre = p.site_spacing_wu / math.sqrt(2.0) * p.valley_path_factor
  return dict(valley=valley, pass_=pass_, to_border=to_border, centre=centre)


# ---------------------------------------------------------------- battles, expeditions, watched (WLD-280 to WLD-284)
def poisson_quantile(lam, q=0.99):
  k, pk, cum = 0, math.exp(-lam), math.exp(-lam)
  while cum < q:
    k += 1
    pk *= lam / k
    cum += pk
  return k


def battle_load(settlements, battles_per_week, p=P, pace='always_on', regions=1):
  """Battles in progress: mean over the week, and the 99th percentile at the evening peak of one time zone
  (always-on), or during a session (session pace: the world runs only while its host plays)."""
  per_week = settlements * battles_per_week / 2.0           # every battle has two settlement sides
  if pace == 'session':
    per_hour = per_week * p.session_factor / 168.0           # world-loop rates are 12 times faster per world hour
    lam = per_hour * p.battle_mean_min / 60.0
    return dict(per_week=per_week, mean=lam, p99=poisson_quantile(lam), per_region_p99=poisson_quantile(
      lam / regions))
  per_hour = per_week / 168.0
  peak = per_hour * ((1 - p.steward_attack_share) + p.steward_attack_share * 24.0 / p.evening_window_h)
  lam_mean = per_hour * p.battle_mean_min / 60.0
  lam_peak = peak * p.battle_mean_min / 60.0
  return dict(per_week=per_week, mean=lam_mean, peak_mean=lam_peak, p99=poisson_quantile(lam_peak),
              per_region_p99=poisson_quantile(lam_peak / regions))


def expedition_load(settlements, battles_per_week, p=P):
  """Expeditions in progress on average: attack expeditions (from declaration to their return) and gathering
  expeditions to contested resource sites."""
  t = travel_table(p)
  attack_min = max(p.warning_min, minutes(t['to_border'], p.march_speed_wus)) + p.battle_mean_min + minutes(
    t['valley'], p.march_speed_wus)
  attacks_per_min = settlements * battles_per_week / 2.0 / (7 * 24 * 60)
  gather_min = 2 * minutes(t['centre'], p.march_speed_wus) + p.gather_stay_min
  gathers_per_min = settlements * p.gather_trips_per_day / (24 * 60)
  return dict(attack=attacks_per_min * attack_min, gather=gathers_per_min * gather_min,
              attack_min=attack_min, gather_min=gather_min)


def present_share(p=P):
  """Share of a steward's day that the typical pattern is present, and share during the evening window."""
  present_min = p.typical_active_min + p.typical_check_ins * p.check_in_min
  return present_min / (24 * 60), present_min / (p.evening_window_h * 60)


def watched(capacity, settlements, p=P):
  """Watched settlements outside battles at the evening peak (WLD-244): typical and at most, never more than the
  settlements of the world."""
  _, peak_share = present_share(p)
  present = capacity * peak_share
  return (min(settlements, int(round(present * p.watched_per_present))),
          min(settlements, int(round(present * p.watched_max_per_present))))


# ---------------------------------------------------------------- report and check
def occupancy_degrees(capacity, share, p=P, seed=1, ranked=True, rule='one_per_region', ai_override=None):
  lay = Layout(capacity, seed, p, ai_override)
  ai_sites = lay.place_ai(lay.size['ai'], rule)
  stewards = int(round(capacity * share))
  st = lay.found_stewards(stewards, ai_sites, ranked)
  occ = ai_sites + st
  deg = lay.contact_degrees(occ)
  vals = sorted(deg.values())
  mean = sum(vals) / len(vals) if vals else 0.0
  low = sum(1 for v in vals if v < 2) / len(vals) if vals else 0.0
  st_vals = [deg[s] for s in st]
  st_mean = sum(st_vals) / len(st_vals) if st_vals else 0.0
  st_low = sum(1 for v in st_vals if v < 1) / len(st_vals) if st_vals else 0.0
  return dict(occupied=len(occ), mean=mean, below_two=low, steward_mean=st_mean, steward_isolated=st_low,
              per_region=len(occ) / lay.size['regions'])


def fmt(x, nd=1):
  return f'{x:,.{nd}f}'


def report(p=P):
  print('# A1 world-density model: report\n')
  print('## World size (WLD-004 to WLD-008)\n')
  print('| stewards | AI | sites | regions | grid (regions per side) | world side (wu) | land samples (M) |'
        ' heights binary64 (MB) | normals (MB) | heights 16-bit (MB) |')
  print('|---|---|---|---|---|---|---|---|---|---|')
  for cap in (1, 4, 8, 16, 50, 100, 200):
    s = world_size(cap, p)
    m = s['land_samples']
    print(f"| {cap} | {s['ai']} | {s['sites']} | {s['regions']} | {s['grid']} | {fmt(s['side'], 0)} |"
          f" {m / 1e6:.3f} | {m * 8 / 1e6:.1f} | {m * 24 / 1e6:.1f} | {m * 2 / 1e6:.1f} |")
  print('\n## Geometry\n')
  print(f'- lens (border area) width at plateau: {fmt(lens_width(p), 0)} wu')
  print(f'- valley centre outside the largest territory by {fmt(centre_clearance(p), 0)} wu')
  print(f'- pass mouth at {fmt(pass_mouth_distance(p), 0)} wu from its site')
  print(f'- territory of a settlement in a full valley at plateau: {territory_area_at_plateau(p) / 1e6:.2f} M wu2')
  print(f'- core area {math.pi * p.core_radius_wu ** 2 / 1e6:.2f} M wu2; founding territory '
        f'{math.pi * p.founding_radius_wu ** 2 / 1e6:.2f} M wu2')
  print('\n## Travel (WLD-100 to WLD-106), minutes; travel runs at the tick rate at both paces\n')
  t = travel_table(p)
  print('| leg | distance (wu) | march 20 wu/s | formation 12 wu/s |')
  print('|---|---|---|---|')
  for name, key in (('Wellspring to the lens at contact', 'to_border'), ('valley neighbour', 'valley'),
                    ('pass neighbour', 'pass_'), ('site to valley centre', 'centre')):
    print(f'| {name} | {fmt(t[key], 0)} | {minutes(t[key], p.march_speed_wus):.1f} |'
          f' {minutes(t[key], p.formation_speed_wus):.1f} |')
  for cap in (16, 200):
    lay = Layout(cap, 1, p)
    hop = lay.region_hop_wu()
    dia = lay.diameter_wu()
    print(f'| region hop ({cap}) | {fmt(hop, 0)} | {minutes(hop, p.march_speed_wus):.1f} |'
          f' {minutes(hop, p.formation_speed_wus):.1f} |')
    print(f'| world diameter ({cap}) | {fmt(dia, 0)} | {minutes(dia, p.march_speed_wus):.1f} |'
          f' {minutes(dia, p.formation_speed_wus):.1f} |')
  hold = p.warning_min - minutes(t['to_border'], p.march_speed_wus)
  print(f'\nWarning {p.warning_min:.0f} min against the {minutes(t["to_border"], p.march_speed_wus):.1f} min'
        f' to the lens: the hold of the attack lifecycle is {hold:.1f} min (always-on); at the session pace a'
        f' warning divided by {p.session_factor:.0f} is {p.warning_min / p.session_factor * 60:.0f} s')
  print('\n## Contact (WLD-060 to WLD-066), world days at the always-on pace (session: x 2 h)\n')
  print(f'- first valley contact: day {first_valley_contact_day(p):.1f}'
        f' ({first_valley_contact_day(p) * 24 / p.session_factor:.1f} h of world time at the session pace)')
  print(f'- first pass contact: day {first_pass_contact_day(p):.1f}')
  print(f'- late joiner next to a plateau neighbour: contact on day {late_joiner_contact_day(p):.2f}')
  print('\n| stewards | occupancy | occupied | per region | mean contacts | share < 2 | steward mean |'
        ' stewards isolated |')
  print('|---|---|---|---|---|---|---|---|')
  for cap in (1, 16, 200):
    for share in (0.25, 0.5, 1.0):
      o = occupancy_degrees(cap, share, p)
      print(f"| {cap} | {share:.0%} | {o['occupied']} | {o['per_region']:.2f} | {o['mean']:.2f} |"
            f" {o['below_two']:.0%} | {o['steward_mean']:.2f} | {o['steward_isolated']:.0%} |")
  print('\n## Scale handed to A2 (WLD-280 to WLD-284)\n')
  print('| stewards | settlements | regions | bodies per full region | battles/week | in progress, mean |'
        ' evening peak p99 | per region p99 | session p99 | expeditions, mean | present at peak | watched |')
  print('|---|---|---|---|---|---|---|---|---|---|---|---|')
  _, peak_share = present_share(p)
  for cap in (16, 200):
    s = world_size(cap, p)
    n = s['capacity'] + s['ai']
    for b in p.battles_per_week:
      a = battle_load(n, b, p, 'always_on', s['regions'])
      ss = battle_load(n, b, p, 'session', s['regions']) if cap <= 16 else None
      e = expedition_load(n, b, p)
      sess = str(ss['p99']) if ss else 'n/a'
      print(f"| {cap} | {n} | {s['regions']} | {p.sites_per_region * p.pop_plateau} | {a['per_week']:.0f}"
            f" (b={b:.0f}) | {a['mean']:.2f} | {a['p99']} | {a['per_region_p99']} | {sess} |"
            f" {e['attack'] + e['gather']:.1f} | {cap * peak_share:.1f} | {watched(cap, n, p)} |")


def sweep(p=P):
  print('| spacing (wu) | max radius (wu) | growth exponent | first valley day | first pass day | lens (wu) |'
        ' centre clearance (wu) |')
  print('|---|---|---|---|---|---|---|')
  for s in (1920.0, 2080.0, 2240.0, 2400.0):
    for rmax in (1300.0, 1400.0, 1500.0):
      for g in (0.5, 0.75, 1.0):
        q = replace(p, site_spacing_wu=s, max_radius_wu=rmax, growth_exponent=g)
        print(f'| {s:.0f} | {rmax:.0f} | {g} | {first_valley_contact_day(q):.1f} | {first_pass_contact_day(q):.1f} |'
              f' {lens_width(q):.0f} | {centre_clearance(q):.0f} |')


def ai_rules(p=P):
  print('| stewards | rule | AI | occupancy | mean contacts | share < 2 | steward mean | stewards isolated |')
  print('|---|---|---|---|---|---|---|---|')
  for cap in (1, 4, 16, 200):
    for rule, ranked in (('one_per_region', True), ('diagonal_pairs', True), ('one_per_region', False)):
      for share in (0.25, 0.5, 1.0):
        o = occupancy_degrees(cap, share, p, 1, ranked, rule)
        label = rule + ('' if ranked else ', unranked founding')
        print(f"| {cap} | {label} | {world_size(cap, p)['ai']} | {share:.0%} | {o['mean']:.2f} |"
              f" {o['below_two']:.0%} | {o['steward_mean']:.2f} | {o['steward_isolated']:.0%} |")


def layout_text(capacity, seed, p=P):
  lay = Layout(capacity, seed, p)
  s = lay.size
  print(f"capacity {capacity}, seed {seed}: {s['regions']} regions, {s['sites']} sites, {s['ai']} AI, grid"
        f" {s['grid']} x {s['grid']}, world side {s['side']:.0f} wu")
  g = blob_grid_side(s['regions'])
  for j in reversed(range(g)):
    print(' '.join('#' if (i, j) in lay.region_index else '.' for i in range(g)))
  print(f'{len(lay.valley_pairs)} valley pairs, {len(lay.passes)} passes')
  for k, (x, z, r, loc) in enumerate(lay.sites[:8]):
    print(f'site {k}: region {r} local {loc} at ({x:.0f}, {z:.0f})')


def check(p=P):
  bad = 0

  def same(name, got, want, tol=0.0):
    nonlocal bad
    ok = abs(got - want) <= tol if isinstance(want, float) else got == want
    print(f"{'ok  ' if ok else 'DIFF'} {name}: model {got}, document {want}")
    if not ok:
      bad += 1

  for cap in (1, 16, 200):
    s = world_size(cap, p)
    want = STATED[f'size_{cap}']
    same(f'regions at {cap}', s['regions'], want[0])
    same(f'sites at {cap}', s['sites'], want[1])
    same(f'AI settlements at {cap}', s['ai'], want[2])
    same(f'world side at {cap} (wu)', int(s['side']), want[3])
    same(f'land samples at {cap} (M)', round(s['land_samples'] / 1e6, 3), want[4], 0.0005)
  t = travel_table(p)
  same('Wellspring to the lens (min)', round(minutes(t['to_border'], p.march_speed_wus), 1),
       STATED['travel_lens_min'], 0.05)
  same('valley neighbour (min)', round(minutes(t['valley'], p.march_speed_wus), 1), STATED['travel_valley_min'],
       0.05)
  same('pass neighbour (min)', round(minutes(t['pass_'], p.march_speed_wus), 1), STATED['travel_pass_min'], 0.05)
  lay16, lay200 = Layout(16, 1, p), Layout(200, 1, p)
  same('region hop (min)', round(minutes(lay16.region_hop_wu(), p.march_speed_wus), 1), STATED['travel_hop_min'],
       0.05)
  same('diameter at 16 (min)', round(minutes(lay16.diameter_wu(), p.march_speed_wus)),
       STATED['diameter_16_min'])
  same('diameter at 200 (min)', round(minutes(lay200.diameter_wu(), p.march_speed_wus)),
       STATED['diameter_200_min'])
  same('first valley contact (day)', round(first_valley_contact_day(p), 1), STATED['first_valley_day'], 0.05)
  same('first pass contact (day)', round(first_pass_contact_day(p), 1), STATED['first_pass_day'], 0.05)
  same('late joiner contact (day)', round(late_joiner_contact_day(p), 1), STATED['late_joiner_day'], 0.05)
  same('lens width (wu)', int(lens_width(p)), STATED['lens_width_wu'])
  same('valley centre clearance (wu)', int(round(centre_clearance(p))), STATED['centre_clearance_wu'])
  same('territory at plateau (M wu2)', round(territory_area_at_plateau(p) / 1e6, 2), STATED['territory_plateau_mwu2'],
       0.005)
  for cap in (16, 200):
    for share, (mean, st_mean) in STATED[f'degree_{cap}'].items():
      o = occupancy_degrees(cap, share, p)
      same(f'mean contacts at {cap}, {share:.0%}', round(o['mean'], 1), mean, 0.05)
      same(f'steward contacts at {cap}, {share:.0%}', round(o['steward_mean'], 1), st_mean, 0.05)
      same(f'settlements per region at {cap}, {share:.0%}', round(o['per_region'], 1),
           STATED[f'per_region_{cap}'][share], 0.05)
  s16, s200 = world_size(16, p), world_size(200, p)
  n16, n200 = 16 + s16['ai'], 200 + s200['ai']
  for k, b in enumerate(p.battles_per_week):
    same(f'battles p99 at 16, {b:.0f} a week', battle_load(n16, b, p)['p99'], STATED['battles_p99_16'][k])
    same(f'battles p99 at 200, {b:.0f} a week', battle_load(n200, b, p)['p99'], STATED['battles_p99_200'][k])
    same(f'battles p99 at 16, session pace, {b:.0f} a week', battle_load(n16, b, p, 'session')['p99'],
         STATED['battles_p99_16_session'][k])
  mid = p.battles_per_week[1]
  e16, e200 = expedition_load(n16, mid, p), expedition_load(n200, mid, p)
  same('expeditions at 16', round(e16['attack'] + e16['gather'], 1), STATED['expeditions_16'], 0.05)
  same('expeditions at 200', round(e200['attack'] + e200['gather'], 1), STATED['expeditions_200'], 0.05)
  _, peak_share = present_share(p)
  same('present at the peak, 16', round(16 * peak_share, 1), STATED['present_16'], 0.05)
  same('present at the peak, 200', round(200 * peak_share, 1), STATED['present_200'], 0.05)
  for cap, n in ((16, n16), (200, n200)):
    typ, top = watched(cap, n, p)
    same(f'watched settlements at {cap}, typical', typ, STATED[f'watched_{cap}'][0])
    same(f'watched settlements at {cap}, at most', top, STATED[f'watched_{cap}'][1])
  same('diameter at 1 (min)', round(minutes(Layout(1, 1, p).diameter_wu(), p.march_speed_wus)),
       STATED['diameter_1_min'])
  same('bodies per full region', p.sites_per_region * p.pop_plateau, STATED['bodies_region'])
  print(f'{bad} difference(s)')
  return bad


def main(argv):
  mode = argv[1] if len(argv) > 1 else 'check'
  if mode == 'check':
    return 1 if check() else 0
  if mode == 'report':
    report()
    return 0
  if mode == 'sweep':
    sweep()
    return 0
  if mode == 'ai':
    ai_rules()
    return 0
  if mode == 'layout':
    cap = int(argv[2]) if len(argv) > 2 else 16
    seed = int(argv[3]) if len(argv) > 3 else 1
    layout_text(cap, seed)
    return 0
  print(__doc__)
  return 2


if __name__ == '__main__':
  sys.exit(main(sys.argv))
