#!/usr/bin/env python3
"""A1WorldDensity.py: the world-density model of area A1 (world and regions).

It implements the world geometry and the life of a world as Design/gdd/01_world_and_regions/ (prefix WLD) states
them, and computes for a world of a given steward capacity: its regions, settlement sites, native AI settlements and
site margin; settlements per region; travel times between neighbours, across a region and across the world; the
contact rate (the world day on which territories meet, and the territory contacts of every settlement, in a fresh
world and over a year of joining, leaving, dormancy and returns); and the scale figures that area A2 sizes the server
with (bodies per region, battles in progress at the evening peak with their range, expeditions, the union of watched
settlements, terrain samples). Figures are given for 16 and 200 stewards, at both world paces (PIL-100, PIL-101), and
over several world seeds.

Usage:
  python3 -I Tools/DesignModels/A1WorldDensity.py [check | report | lifetime | battles | sweep | founding |
                                                   tables [python] | layout <capacity> [<seed>]]

  check     (default) compute every figure and every table of figures that the documents state and compare it with
            the stated value (STATED, STATED_TABLES); prints one line per figure and exits with status 1 when any
            differs
  report    world size, geometry, travel over seeds 1 to 10, contact in a fresh world, scale figures
  lifetime  a year of world days: launch, daily demand, active lifetimes, 21 days away on the map, dormancy, returns,
            the return queue and the stand-in AI settlements (WLD-124 to WLD-127), with the stand-ins and without
  battles   battles in progress at the evening peak and at the session pace: base, planning and stress cases, the
            99th percentile, the weekly maximum and the chance that an attack waits for a battle slot (WLD-284)
  sweep     first-contact days, lens width and valley-centre clearance over site spacing, the largest territory
            radius and the territory growth exponent
  founding  the founding rules compared in a fresh world: stewards with a steward neighbour, with and without
            stand-ins and the steward weight of WLD-140
  tables    every table of figures of the documents as Markdown rows; with python, as the STATED_TABLES literal
  layout    one generated layout (regions, sites, crossings) as text, for a capacity and a seed

Inputs: the tuning values of the documents (class Params; each field names its tuning key) and the values of other
areas that the model needs before those areas exist, each marked "placeholder" with the area that sets it; the
lifetime scenarios (class Scenario) are placeholders of B6 and C2. Outputs: text tables on standard output. Python 3
with the standard library only; no input files. Every random value is a counter-based SplitMix64 hash of (seed, keys),
so a figure depends only on its own inputs and the same on every Python version.

Geometry (WLD §2): a world is a compact blob of square regions, ringed by sea, inside an extent reserved for the
largest capacity; each region is a valley of four settlement sites in a 2 x 2 pattern that is offset and rotated as a
whole, an island whose coast falls to a sea channel along every side; a crossing (a land bridge) links one site on
each side of a shared region side. Valley neighbours share a lens split at its halfway line into two border areas,
one per side; a crossing's corridor is split at the region border the same way; the centre of every valley holds a
central resource site outside every territory.
"""
import math
import sys
from dataclasses import dataclass, replace


# ---------------------------------------------------------------- inputs
@dataclass(frozen=True)
class Params:
  # world and regions
  region_side_wu: float = 5120.0          # wld.region.side
  sites_per_region: int = 4               # wld.region.sites (the 2 x 2 valley; the model supports only 4)
  site_spacing_wu: float = 2080.0         # wld.site.spacing: valley neighbours, centre to centre
  valley_offset_wu: float = 64.0          # wld.site.valley_offset: the whole valley, per axis, at most
  valley_rotation_deg: float = 3.0        # wld.site.valley_rotation: the whole valley, at most
  coast_band_wu: float = 320.0            # wld.region.coast_band: half-width of the band where land falls to the sea
  channel_width_wu: float = 256.0         # wld.region.channel_width: sea along every region border outside crossings
  crossing_width_wu: float = 384.0        # wld.crossing.width
  crossing_mouth_wu: float = 1200.0       # wld.crossing.mouth_distance: from its site's centre
  core_radius_wu: float = 400.0           # wld.site.core_radius
  founding_radius_wu: float = 500.0       # wld.territory.founding_radius
  max_radius_wu: float = 1400.0           # wld.territory.max_radius
  sea_ring_regions: int = 1               # wld.world.sea_ring
  min_regions: int = 3                    # wld.world.min_regions
  blob_area_factor: float = 1.25          # wld.world.blob_area_factor
  blob_jitter: float = 0.35               # wld.world.blob_jitter
  capacity_limit: int = 256               # wld.world.capacity_limit: the extent is reserved for it
  cell_size_wu: float = 16.0              # wld.terrain.cell_size
  crossing_feather_wu: float = 96.0       # wld.terrain.crossing_feather: band of a bridge's feathered squares
  crossing_square_spacing_wu: float = 192.0  # wld.terrain.crossing_square_spacing: at most, between a bridge's squares
  # free sites, native AI settlements, the site margin (WLD-004, WLD-120 to WLD-127)
  free_sites_min: int = 2                 # pil.world.free_sites_min (PIL-068)
  ai_base: float = 6.0                    # wld.ai.base_count
  ai_per_steward: float = 0.25            # wld.ai.per_steward
  margin_per_steward: float = 0.25        # wld.world.site_margin
  contacts_min: int = 2                   # wld.density.contacts_min: the stand-in keeper's target per steward
  standin_delay_days: float = 3.0         # wld.density.standin_delay_days
  # founding score (WLD-140)
  w_steward: int = 6                      # wld.founding.steward_weight
  w_ai: int = 2                           # wld.founding.valley_weight
  w_crossing: int = 1                     # wld.founding.crossing_weight
  w_room: int = 1                         # wld.founding.room_weight
  # resources, in home-site units (WLD-080 to WLD-086)
  home_units: int = 8                     # wld.resource.home_inner + wld.resource.home_outer
  border_factor: int = 1                  # wld.resource.border_factor (each border area's resource site)
  crossing_factor: int = 1                # wld.resource.crossing_factor
  central_factor: int = 3                 # wld.resource.central_factor
  # travel (WLD-100 to WLD-104)
  valley_path_factor: float = 1.15        # wld.travel.valley_path_factor
  crossing_path_factor: float = 1.20      # wld.travel.crossing_path_factor
  march_speed_wus: float = 20.0           # wld.travel.march_speed (ref.unit.worker.speed); B4 confirms it
  formation_speed_wus: float = 12.0       # ref.unit.leader.speed x ref.unit.leader.formation_speed_factor
  warning_min: float = 10.0               # pil.conflict.warning_minutes (PIL-025), at both paces (WLD-102)
  session_factor: float = 12.0            # pil.pace.session_factor (PIL-101)
  # growth (placeholders until B1)
  plateau_days: float = 14.0              # pil.settlement.plateau_days (PIL-055)
  growth_exponent: float = 0.5            # placeholder, B1: territory radius r0 + (rmax - r0) (t / Tp)^g
  pop_plateau: int = 600                  # placeholder, B1: Workers and Leaders of a settlement at its plateau
  # world loop (A0 targets; placeholders until B4, B5, B6)
  battles_per_week: tuple = (2.0, 6.0, 10.0)  # pil.world.battles_per_week (PIL-065): low, middle, high
  bodies_per_battle: int = 400            # pil.battle.bodies_typical (PIL-021)
  battle_cap_region: int = 2              # wld.scale.battles_per_region_max
  battle_cap_share: float = 0.1           # wld.scale.battles_per_settlement: world cap = max(4, ceil(share x n))
  battle_cap_min: int = 4                 # wld.scale.battles_per_world_min
  gather_trips_per_day: float = 6.0       # placeholder, B2: gathering expeditions per settlement per world day
  gather_stay_min: float = 5.0            # placeholder, B2: minutes an expedition gathers at a resource site
  typical_active_min: float = 60.0        # pil.session.typical_active_minutes (PIL-102)
  typical_check_ins: float = 2.0          # pil.session.typical_check_ins (PIL-102)
  check_in_min: float = 5.0               # middle of pil.session.check_in_minutes (PIL-074)
  evening_window_h: float = 5.0           # placeholder, B6: the hours of a time zone in which present play falls
  dormancy_days: float = 21.0             # pil.session.dormancy_days (PIL-078, O34)


P = Params()

# The battle cases (WLD-284): battle length in minutes, share of attacks declared by present stewards, evening
# window in hours, battles that start together (a Cladan offensive, a valley war).
BATTLE_CASES = (('base', 8.0, 0.5, 5.0, 1), ('planning', 12.0, 0.8, 3.0, 2), ('stress', 12.0, 0.8, 3.0, 4))


@dataclass(frozen=True)
class Scenario:
  """A world's demand and churn over a year (placeholders of B6 and C2)."""
  name: str
  capacity: int
  launch_count: int        # stewards who arrive in the launch period
  launch_days: float
  demand_per_day: float    # new stewards per world day after the launch period
  mean_life_days: float    # mean active lifetime, exponential
  return_share: float = 0.25   # of dormant stewards who come back
  return_min_days: float = 7.0  # after dormancy, uniform between these
  return_max_days: float = 60.0


SCENARIOS = (
  Scenario('200, strong demand', 200, 200, 14.0, 2.0, 43.0),
  Scenario('200, weak demand', 200, 120, 14.0, 0.5, 43.0),
  Scenario('200, engaged', 200, 200, 14.0, 2.0, 120.0),
  Scenario('16, a friend every 4 days', 16, 16, 7.0, 0.25, 43.0),
  Scenario('16, a friend every 20 days', 16, 12, 7.0, 0.05, 60.0),
)
LIFETIME_SEEDS = (1, 2, 3, 4, 5)
LIFETIME_DAYS = (30, 90, 180, 365)
LAYOUT_SEEDS = tuple(range(1, 11))

# The figures that the document states. check compares the model with them.
STATED = {
  # capacity: (native AI, margin, sites, regions, land grid side, land samples in millions)
  'size_1': (9, 0, 12, 3, 2, 0.307),
  'size_16': (10, 4, 32, 8, 4, 0.819),
  'size_200': (56, 50, 308, 77, 10, 7.885),
  'extent_wu': 71680,
  # travel at march speed, minutes; over seeds 1 to 10 at 16 and 200: (min, median, max)
  'travel_lens_min': 0.9,
  'travel_valley_min': 2.0,
  'travel_crossing_min': (2.8, 3.0, 3.2),
  'travel_hop_min': (5.8, 5.9, 5.9),
  'diameter_16_min': (23, 24, 25),
  'diameter_200_min': (70, 74, 76),
  # contact (exact: the valley pattern is offset and rotated as a whole)
  'first_valley_day': 5.0,
  'first_crossing_day': 8.5,
  'lens_width_wu': 720,
  'centre_clearance_wu': 71,
  # with a growth that is linear in time (exponent 1): first valley and crossing contact days (WLD-064)
  'linear_growth_days': (8.4, 10.9),
  'centre_distance_wu': 1471,
  # a journey to a valley neighbour in world days: always-on pace, session pace (WLD-103)
  'journey_days': (0.0014, 0.017),
  # the solo world at full occupancy: (settlements per region, a steward's contacts)
  'fresh_1': (3.3, 2.0),
  # the strategic view's highest camera at 16 and 200 (WLD-240)
  'strategic_max_wu': (18432, 46080),
  # terrain chunks of a world of 16: land chunks, sea chunks of one value (WLD-166)
  'chunks_16': (200, 4700),
  # crossings over seeds 1 to 10 at 16 and 200: mouths (nearest, farthest) apart, a mouth's distance from the region
  # border (nearest, farthest), squares per bridge (fewest, most), flatten areas a region's layout needs at most
  # (WLD-162, WLD-282)
  'mouth_gap_wu': (435, 849),
  'mouth_border_wu': (205, 436),
  'bridge_squares': (4, 6),
  'layout_flatten_max': 24,
  # binary32 spacing at the far corner of the reserved extent (WLD-246)
  'binary32_spacing_wu': 0.0078,
  # sites without a crossing over seeds 1 to 10: (fewest, most) at 16 and at 200 (WLD-024)
  'no_crossing_16': (12, 16),
  'no_crossing_200': (40, 44),
  'territory_plateau_mwu2': 4.84,
  'own_units': 11,
  'holdable_units_max': 17,
  # the fresh world by steward occupancy is the table 'fresh' below, over seeds 1 to 10
  # lifetime, day 180, mean of 5 seeds: (stewards on the map, refused, settlements per region, a steward's contacts,
  # stewards with fewer than 2 contacts)
  'life_200_strong': (167, 22, 3.0, 2.8, 0.01),
  'life_200_weak': (50, 0, 2.0, 2.6, 0.01),
  'life_16_slow': (6, 0, 2.4, 2.4, 0.00),
  # battles in progress at the evening peak, 6 battles per settlement per week: p99 (base, planning, stress)
  'battles_16': (2, 4, 8),
  'battles_200': (5, 16, 20),
  'battles_16_session': (3, 6, 8),
  # weekly maximum, planning case, 6 a week (16, 200)
  'battles_week_max': (4, 16),
  # world battle cap (16, 200)
  'battle_cap': (4, 26),
  # union of watched settlements at the evening peak, full occupancy, over seeds 1 to 10: typical and at most, each
  # as (fewest, most)
  'watched_16': ((10, 11), (15, 16)),
  'watched_200': ((116, 118), (174, 177)),
  # bodies of a full region at the placeholder plateau population
  'bodies_region': 2400,
}

# Every table of figures in the documents, as `tables python` prints it (WLD-283).
STATED_TABLES = {
  'size': [
    ('1', '9', '0', '12', '3', '2', '307,200'),
    ('4', '9', '1', '16', '4', '3', '409,600'),
    ('8', '8', '2', '20', '5', '3', '512,000'),
    ('16', '10', '4', '32', '8', '4', '819,200'),
    ('50', '19', '13', '84', '21', '6', '2,150,400'),
    ('100', '33', '25', '160', '40', '8', '4,096,000'),
    ('200', '56', '50', '308', '77', '10', '7,884,800'),
    ('256', '70', '64', '392', '98', '12', '10,035,200'),
  ],
  'fresh': [
    ('16', '25 %', '17 to 18', '3 to 4', '2.12 to 2.25', '0.8 to 1.1', '2.5 to 3.0', '0 %', '75 to 100 %'),
    ('16', '50 %', '20 to 21', '2 to 3', '2.50 to 2.62', '1.4 to 1.7', '2.5 to 2.9', '0 %', '88 to 100 %'),
    ('16', '100 %', '26', '0', '3.25', '2.2 to 2.4', '2.4 to 2.8', '6 %', '81 to 100 %'),
    ('200', '25 %', '144 to 147', '38 to 41', '1.87 to 1.91', '1.2 to 1.3', '2.8 to 2.9', '0 %', '100 %'),
    ('200', '50 %', '182 to 189', '26 to 33', '2.36 to 2.45', '1.9 to 2.1', '2.9', '0 %', '100 %'),
    ('200', '100 %', '256', '0', '3.32', '2.8', '2.8 to 2.9', '1 %', '100 %'),
  ],
  'lifetime': [
    ('200, strong demand', '30', '195', '22 / 234', '3.26', '22 %', '2.81', '1 %', '2 %', '100 %'),
    ('200, strong demand', '90', '175', '22 / 356', '3.09', '26 %', '2.77', '0 %', '1 %', '100 %'),
    ('200, strong demand', '180', '167', '22 / 532', '3.04', '29 %', '2.75', '1 %', '2 %', '100 %'),
    ('200, strong demand', '365', '169', '25 / 903', '3.06', '28 %', '2.77', '1 %', '1 %', '100 %'),
    ('200, weak demand', '30', '117', '0 / 128', '2.54', '40 %', '2.74', '1 %', '2 %', '98 %'),
    ('200, weak demand', '90', '67', '0 / 155', '2.16', '60 %', '2.58', '1 %', '14 %', '97 %'),
    ('200, weak demand', '180', '50', '0 / 201', '2.02', '68 %', '2.61', '1 %', '11 %', '96 %'),
    ('200, weak demand', '365', '49', '0 / 297', '1.92', '67 %', '2.70', '2 %', '2 %', '97 %'),
    ('16, a friend every 4 days', '30', '15', '4 / 21', '3.10', '41 %', '2.48', '3 %', '5 %', '84 %'),
    ('16, a friend every 4 days', '90', '15', '8 / 36', '3.12', '42 %', '2.53', '5 %', '5 %', '89 %'),
    ('16, a friend every 4 days', '180', '14', '12 / 57', '3.08', '44 %', '2.61', '0 %', '4 %', '86 %'),
    ('16, a friend every 4 days', '365', '15', '30 / 106', '3.15', '41 %', '2.59', '1 %', '1 %', '91 %'),
    ('16, a friend every 20 days', '30', '10', '0 / 12', '2.70', '54 %', '2.48', '2 %', '13 %', '88 %'),
    ('16, a friend every 20 days', '90', '8', '0 / 15', '2.60', '65 %', '2.47', '0 %', '4 %', '85 %'),
    ('16, a friend every 20 days', '180', '6', '0 / 20', '2.40', '68 %', '2.39', '0 %', '14 %', '81 %'),
    ('16, a friend every 20 days', '365', '6', '0 / 30', '2.25', '69 %', '2.48', '0 %', '18 %', '88 %'),
  ],
  'battles': [
    ('16', 'evening peak', '2', '1 (1)', '2 (2)', '4 (4)', '4', '0 %'),
    ('16', 'evening peak', '6', '2 (2)', '4 (4)', '8 (8)', '4', '4 %'),
    ('16', 'evening peak', '10', '2 (2)', '6 (6)', '8 (8)', '4', '9 %'),
    ('16', 'session', '2', '2 (3)', '4 (4)', '4 (8)', '4', '2 %'),
    ('16', 'session', '6', '3 (5)', '6 (8)', '8 (12)', '4', '11 %'),
    ('16', 'session', '10', '4 (6)', '8 (10)', '12 (16)', '4', '24 %'),
    ('200', 'evening peak', '2', '3 (3)', '8 (8)', '12 (12)', '26', '0 %'),
    ('200', 'evening peak', '6', '5 (6)', '16 (16)', '20 (20)', '26', '0 %'),
    ('200', 'evening peak', '10', '8 (8)', '22 (22)', '28 (28)', '26', '0 %'),
  ],
  'travel': [
    ('halfway line', '1,040', '0.9', '1.4'),
    ('valley centre', '1,691', '1.4', '2.3'),
    ('valley neighbour', '2,392', '2.0', '3.3'),
    ('crossing neighbour', '3,401 to 3,899', '2.8 to 3.2', '4.7 to 5.4'),
    ('region hop', '6,971 to 7,111', '5.8 to 5.9', '9.7 to 9.9'),
    ('diameter 16', '27,541 to 29,907', '23.0 to 24.9', '38.3 to 41.5'),
    ('diameter 200', '84,583 to 91,349', '70.5 to 76.1', '117.5 to 126.9'),
  ],
  'scale': [
    ('16', '26', '8', '3.25', '1.0', '4', '10 to 11', '15 to 16', '0.82', '6.6', '19.7', '1.6'),
    ('200', '256', '77', '3.32', '9.9', '47', '116 to 118', '174 to 177', '7.88', '63.1', '189.2', '15.8'),
  ],
}


# ---------------------------------------------------------------- counter-based randomness (SplitMix64)
def mix64(x):
  x = (x + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
  x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
  x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
  return x ^ (x >> 31)


def unit01(seed, *keys):
  """A value in [0, 1) that depends only on the seed and the keys."""
  h = mix64(seed & 0xFFFFFFFFFFFFFFFF)
  for k in keys:
    h = mix64(h ^ (k & 0xFFFFFFFFFFFFFFFF))
  return (h >> 11) / float(1 << 53)


def unit(seed, *keys):
  """A value in [-1, 1)."""
  return 2.0 * unit01(seed, *keys) - 1.0


class Stream:
  """A sequential stream of counter-based values, for the lifetime simulation."""

  def __init__(self, seed, purpose):
    self.seed, self.purpose, self.n = seed, purpose, 0

  def next(self):
    self.n += 1
    return unit01(self.seed, self.purpose, self.n)

  def expo(self, mean):
    return -mean * math.log(1.0 - self.next())

  def poisson(self, lam):
    k, p, limit = 0, 1.0, math.exp(-lam)
    while True:
      p *= self.next()
      if p < limit:
        return k
      k += 1


def round_half_up(x):
  return int(math.floor(x + 0.5))


# ---------------------------------------------------------------- world size (WLD-003, WLD-004, WLD-007)
def blob_grid_side(regions, p=P):
  return max(1, math.ceil(math.sqrt(regions * p.blob_area_factor)))


def size_core(capacity, p=P):
  ai0 = round_half_up(p.ai_base + p.ai_per_steward * capacity)
  margin = round_half_up(p.margin_per_steward * capacity)
  need = capacity + ai0 + p.free_sites_min + margin
  regions = max(p.min_regions, math.ceil(need / p.sites_per_region))
  sites = regions * p.sites_per_region
  ai = sites - capacity - p.free_sites_min - margin   # the rounding to whole regions goes to native AI settlements
  return ai, margin, sites, regions


def world_size(capacity, p=P, ai_override=None):
  """Native AI settlements, site margin, sites and regions of a world of `capacity` stewards (WLD-004)."""
  ai, margin, sites, regions = size_core(capacity, p)
  if ai_override is not None:
    margin += ai - ai_override
    ai = ai_override
  limit_regions = size_core(p.capacity_limit, p)[3]
  reserved_grid = blob_grid_side(limit_regions, p) + 2 * p.sea_ring_regions
  land_grid = blob_grid_side(regions, p)
  return dict(capacity=capacity, ai=ai, margin=margin, sites=sites, regions=regions, land_grid=land_grid,
              reserved_grid=reserved_grid, extent=reserved_grid * p.region_side_wu,
              land_samples=regions * (p.region_side_wu / p.cell_size_wu) ** 2)


# ---------------------------------------------------------------- layout (WLD-003, WLD-020 to WLD-066)
DIAGONAL = {0: 3, 1: 2, 2: 1, 3: 0}


class Layout:
  """One generated world: land regions, sites, crossings and the travel network."""

  def __init__(self, capacity, seed=1, p=P, ai_override=None):
    self.p, self.seed = p, seed
    self.size = world_size(capacity, p, ai_override)
    g = self.size['land_grid']
    self.cells = self.grow_blob(self.size['regions'], g)
    self.region_index = {c: i for i, c in enumerate(self.cells)}
    off = (self.size['reserved_grid'] - g) // 2
    self.cell_origin = [((i + off) * p.region_side_wu, (j + off) * p.region_side_wu) for i, j in self.cells]
    self.sites = []
    self.make_sites()
    self.valley_pairs, self.crossings = [], []
    self.make_pairs()
    self.nb = {s: self.make_neighbours(s) for s in range(len(self.sites))}
    self.graph = self.make_graph()

  def grow_blob(self, n, side):
    c = (side - 1) / 2.0
    cand = sorted((math.hypot(i - c, j - c) + self.p.blob_jitter * unit(self.seed, 1, i, j), i, j)
                  for i in range(side) for j in range(side))
    chosen = [(cand[0][1], cand[0][2])]
    taken = set(chosen)
    rest = [(i, j) for _, i, j in cand[1:]]
    while len(chosen) < n:
      for k, (i, j) in enumerate(rest):
        if any((i + di, j + dj) in taken for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1))):
          chosen.append((i, j))
          taken.add((i, j))
          del rest[k]
          break
    return chosen

  def make_sites(self):
    p = self.p
    h = p.site_spacing_wu / 2.0
    for r, (x0, z0) in enumerate(self.cell_origin):
      cx = x0 + p.region_side_wu / 2.0 + p.valley_offset_wu * unit(self.seed, 2, r, 0)
      cz = z0 + p.region_side_wu / 2.0 + p.valley_offset_wu * unit(self.seed, 2, r, 1)
      th = math.radians(p.valley_rotation_deg * unit(self.seed, 2, r, 2))
      ct, st = math.cos(th), math.sin(th)
      for k, (a, b) in enumerate(((-h, -h), (h, -h), (-h, h), (h, h))):
        self.sites.append((cx + a * ct - b * st, cz + a * st + b * ct, r, k))

  def site_of(self, region, local):
    return region * 4 + local

  def make_pairs(self):
    for r in range(len(self.cells)):
      for a, b in ((0, 1), (2, 3), (0, 2), (1, 3)):
        self.valley_pairs.append((self.site_of(r, a), self.site_of(r, b)))
    for r, (i, j) in enumerate(self.cells):
      east = self.region_index.get((i + 1, j))
      if east is not None:
        row = (i + j) % 2
        self.crossings.append((self.site_of(r, 1 + 2 * row), self.site_of(east, 0 + 2 * row)))
      north = self.region_index.get((i, j + 1))
      if north is not None:
        col = (i + j + 1) % 2
        self.crossings.append((self.site_of(r, 2 + col), self.site_of(north, 0 + col)))

  def opposites(self, s):
    """The sites that face site s across the channels of its region's two outward sides: the mirror site in the
    neighbouring region along x and along z (a crossing neighbour is one of them)."""
    _, _, r, k = self.sites[s]
    i, j = self.cells[r]
    out = []
    for di, dj, flip in ((-1 if k in (0, 2) else 1, 0, 1), (0, -1 if k in (0, 1) else 1, 2)):
      q = self.region_index.get((i + di, j + dj))
      if q is not None:
        out.append(self.site_of(q, k ^ flip))
    return out

  def make_neighbours(self, s):
    out = [(b, 'valley') for a, b in self.valley_pairs if a == s] + [(a, 'valley') for a, b in self.valley_pairs
                                                                       if b == s]
    out += [(b, 'crossing') for a, b in self.crossings if a == s] + [(a, 'crossing') for a, b in self.crossings
                                                                       if b == s]
    return out

  def dist(self, a, b):
    return math.hypot(self.sites[a][0] - self.sites[b][0], self.sites[a][1] - self.sites[b][1])

  def centre(self, r):
    x = sum(self.sites[self.site_of(r, k)][0] for k in range(4)) / 4.0
    z = sum(self.sites[self.site_of(r, k)][1] for k in range(4)) / 4.0
    return x, z

  def make_graph(self):
    p = self.p
    n = len(self.sites)
    g = {k: [] for k in range(n + len(self.cells))}
    for a, b in self.valley_pairs:
      w = self.dist(a, b) * p.valley_path_factor
      g[a].append((b, w))
      g[b].append((a, w))
    for a, b in self.crossings:
      w = self.dist(a, b) * p.crossing_path_factor
      g[a].append((b, w))
      g[b].append((a, w))
    for r in range(len(self.cells)):
      cx, cz = self.centre(r)
      for k in range(4):
        s = self.site_of(r, k)
        w = math.hypot(self.sites[s][0] - cx, self.sites[s][1] - cz) * p.valley_path_factor
        g[s].append((n + r, w))
        g[n + r].append((s, w))
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
    n = len(self.sites)
    total, count = 0.0, 0
    for r, (i, j) in enumerate(self.cells):
      for di, dj in ((1, 0), (0, 1)):
        q = self.region_index.get((i + di, j + dj))
        if q is not None:
          total += self.shortest_from(n + r)[n + q]
          count += 1
    return total / max(1, count)

  def bridges(self):
    """Per crossing: its two regions, the distances of its two mouths from the region border, and the centres of
    the feathered squares of its bridge, from mouth to mouth (WLD-162)."""
    p = self.p
    out = []
    for a, b in self.crossings:
      xa, za, ra, _ = self.sites[a]
      xb, zb, rb, _ = self.sites[b]
      d = math.hypot(xb - xa, zb - za)
      ux, uz = (xb - xa) / d, (zb - za) / d
      ma = (xa + ux * p.crossing_mouth_wu, za + uz * p.crossing_mouth_wu)
      mb = (xb - ux * p.crossing_mouth_wu, zb - uz * p.crossing_mouth_wu)
      ox, oz = self.cell_origin[rb]
      along_x = self.cells[ra][0] != self.cells[rb][0]
      border = ox if along_x else oz
      k = 0 if along_x else 1
      n = math.ceil(math.hypot(mb[0] - ma[0], mb[1] - ma[1]) / p.crossing_square_spacing_wu)
      pts = [(ma[0] + (mb[0] - ma[0]) * i / n, ma[1] + (mb[1] - ma[1]) * i / n) for i in range(n + 1)]
      out.append((ra, rb, border - ma[k], mb[k] - border, pts))
    return out

  def flatten_areas_max(self):
    """The most flatten areas that one region's layout needs: its 4 cores, its valley centre and every square of
    the bridges on its sides whose square and band reach into the region's square (WLD-162)."""
    p = self.p
    reach = p.crossing_width_wu / 2.0 + p.crossing_feather_wu
    per = {r: 5 for r in range(len(self.cells))}
    for ra, rb, _, _, pts in self.bridges():
      for r in (ra, rb):
        ox, oz = self.cell_origin[r]
        side = p.region_side_wu
        per[r] += sum(1 for x, z in pts if ox - reach < x < ox + side + reach and oz - reach < z < oz + side + reach)
    return max(per.values())

  def sites_without_crossing(self):
    linked = {a for a, _ in self.crossings} | {b for _, b in self.crossings}
    return len(self.sites) - len(linked)

  def crossing_lengths(self):
    return [self.dist(a, b) * self.p.crossing_path_factor for a, b in self.crossings]


# ---------------------------------------------------------------- settlements on the map (WLD-120 to WLD-143)
class World:
  """Settlements on the sites of a layout: native AI settlements, stand-ins and stewards' settlements, with the
  founding choice of WLD-140 and the stand-in keeper of WLD-124 to WLD-127."""

  def __init__(self, layout, keeper=True, steward_weight=None):
    self.lay, self.p = layout, layout.p
    self.keeper = keeper
    self.w_steward = self.p.w_steward if steward_weight is None else steward_weight
    self.occ = {}            # site -> 'native', 'standin' or ('steward', id)
    self.lonely_since = {}   # steward site -> world day it first had fewer contacts than the target
    self.cap = layout.size['capacity']
    self.margin = layout.size['margin']
    self.place_natives()
    if keeper:
      self.creation_standins()

  # -- counts
  def stewards_on_map(self):
    return [s for s, v in self.occ.items() if isinstance(v, tuple)]

  def standins(self):
    return [s for s, v in self.occ.items() if v == 'standin']

  def free_sites(self):
    return [s for s in range(len(self.lay.sites)) if s not in self.occ]

  def standin_bound(self, places_held=0):
    """How many stand-ins the bounds of WLD-125 allow at once: never into PIL-068's reserve of free sites, never
    more settlements on the map than the capacity and the native AI settlements together, never more than the
    site margin."""
    st = len(self.stewards_on_map())
    places = self.cap - st - places_held
    by_reserve = len(self.free_sites()) + len(self.standins()) - places - self.p.free_sites_min
    return max(0, min(by_reserve, self.cap - st, self.margin))

  def standin_room(self, places_held=0):
    return self.standin_bound(places_held) - len(self.standins())

  # -- creation
  def place_natives(self):
    n = self.lay.size['ai']
    placed = 0
    for local in (0, 3, 1, 2):
      for r in range(len(self.lay.cells)):
        if placed >= n:
          return
        self.occ[self.lay.site_of(r, local)] = 'native'
        placed += 1

  def creation_standins(self):
    room = self.standin_room()
    for want in (1, 2):
      for r in range(len(self.lay.cells)):
        if room <= 0:
          return
        locs = [self.lay.site_of(r, k) for k in range(4)]
        here = [s for s in locs if s in self.occ]
        if len(here) >= want:
          continue
        free = [s for s in locs if s not in self.occ]
        if here:
          diag = self.lay.site_of(r, DIAGONAL[self.lay.sites[here[0]][3]])
          pick = diag if diag in free else free[0]
        else:
          pick = free[0]
        self.occ[pick] = 'standin'
        room -= 1

  # -- contacts
  def contacts(self, s):
    return sum(1 for q, _ in self.lay.nb[s] if q in self.occ)

  def steward_neighbour(self, s):
    return any(q in self.occ and isinstance(self.occ[q], tuple) for q, _ in self.lay.nb[s])

  # -- founding (WLD-140 to WLD-143)
  def score(self, s, beside=None):
    """WLD-140: the steward weight per neighbour that is a steward's settlement, valley or crossing; the valley
    weight per valley neighbour that is an AI settlement; the crossing weight per crossing neighbour that is an AI
    settlement; and the room weight per valley neighbour site that is free or a stand-in's, where a later steward
    can found beside."""
    sc = 0
    for q, kind in self.lay.nb[s]:
      v = self.occ.get(q)
      if kind == 'valley' and (v is None or v == 'standin'):
        sc += self.p.w_room
      if v is None:
        continue
      if isinstance(v, tuple):
        sc += self.w_steward
      else:
        sc += self.p.w_ai if kind == 'valley' else self.p.w_crossing
    if beside is not None and any(q == beside for q, _ in self.lay.nb[s]):
      sc += 100
    return sc

  def found(self, sid, day=0.0, beside=None):
    cands = self.free_sites() + self.standins()
    if not cands:
      return None
    best = min(cands, key=lambda s: (-self.score(s, beside), 1 if s in self.occ else 0, self.lay.sites[s][2], s))
    if self.occ.get(best) == 'standin':
      del self.occ[best]            # the stand-in steps back (WLD-126)
    self.occ[best] = ('steward', sid)
    return best

  def leave(self, s):
    del self.occ[s]
    self.lonely_since.pop(s, None)

  # -- the keeper (WLD-125)
  def keep(self, day, places_held=0, delay=None):
    if not self.keeper:
      return
    delay = self.p.standin_delay_days if delay is None else delay
    room = self.standin_room(places_held)
    if room < 0:
      # too many stand-ins: those with the fewest steward neighbours step back first (WLD-126)
      sts = sorted(self.standins(), key=lambda q: (sum(1 for r, _ in self.lay.nb[q]
                                                       if isinstance(self.occ.get(r), tuple)), -q))
      for q in sts[:-room]:
        del self.occ[q]
      room = 0
    for s in self.stewards_on_map():
      if self.contacts(s) >= self.p.contacts_min:
        self.lonely_since.pop(s, None)
        continue
      since = self.lonely_since.setdefault(s, day)
      if day - since < delay:
        continue
      free_nb = [q for q, kind in self.lay.nb[s] if q not in self.occ]
      free_nb.sort(key=lambda q: (0 if any(q == b for b, k in self.lay.nb[s] if k == 'valley') else 1, q))
      while free_nb and self.contacts(s) < self.p.contacts_min:
        if room <= 0:
          # no room in the bounds: a stand-in with no steward neighbour, the latest in region order, steps back so
          # that one can stand beside the steward (WLD-125)
          spare = [q for q in self.standins()
                   if not any(isinstance(self.occ.get(r), tuple) for r, _ in self.lay.nb[q])]
          if not spare:
            break
          del self.occ[max(spare)]
          room = 1
        self.occ[free_nb.pop(0)] = 'standin'
        room -= 1


def fresh_world(capacity, share, seed=1, p=P, keeper=True, steward_weight=None):
  """Stewards found one after another in a new world (WLD-140), the keeper running after each founding."""
  w = World(Layout(capacity, seed, p), keeper, steward_weight)
  n = int(round(capacity * share))
  for i in range(n):
    w.found(i)
    w.keep(0.0, delay=0.0)
  occ = list(w.occ)
  sts = w.stewards_on_map()
  mean = sum(w.contacts(s) for s in occ) / max(1, len(occ))
  with_steward = sum(1 for s in sts if w.steward_neighbour(s)) / max(1, len(sts))
  below = sum(1 for s in sts if w.contacts(s) < p.contacts_min) / max(1, len(sts))
  st_mean = sum(w.contacts(s) for s in sts) / max(1, len(sts))
  return dict(occupied=len(occ), mean=mean, steward_mean=st_mean, with_steward=with_steward, below_min=below,
              standins=len(w.standins()), per_region=len(occ) / w.lay.size['regions'])


# ---------------------------------------------------------------- a year of world days (WLD-124 to WLD-127, WLD-143)
def lifetime(sc, seed, p=P, keeper=True, days=LIFETIME_DAYS):
  lay = Layout(sc.capacity, seed, p)
  w = World(lay, keeper)
  arrive, life, ret = Stream(seed, 101), Stream(seed, 102), Stream(seed, 103)
  st = {}            # id -> dict(state, site, stop, dormant_day, return_day)
  queue = []         # returning stewards waiting for a place
  refused = arrived = 0
  carry = 0.0
  out = {}
  for day in range(0, max(days) + 1):
    d = float(day)
    # leaving: active -> away on the map -> dormant (PIL-078)
    for sid, s in st.items():
      if s['state'] == 'active' and d >= s['stop']:
        s['state'] = 'away'
      if s['state'] == 'away' and d >= s['stop'] + p.dormancy_days:
        s['state'] = 'dormant'
        w.leave(s['site'])
        s['site'] = None
        if ret.next() < sc.return_share:
          s['return_day'] = d + sc.return_min_days + ret.next() * (sc.return_max_days - sc.return_min_days)
      if s['state'] == 'dormant' and s.get('return_day') is not None and d >= s['return_day']:
        s['state'] = 'waiting'
        s['return_day'] = None
        queue.append(sid)
    # arrivals
    if d < sc.launch_days:
      carry += sc.launch_count / sc.launch_days
      new = int(carry)
      carry -= new
    else:
      new = arrive.poisson(sc.demand_per_day)
    on_map = len(w.stewards_on_map())
    # returns first (WLD-143), then new stewards
    still = []
    for sid in queue:
      if on_map < sc.capacity:
        st[sid]['site'] = w.found(sid, d)
        st[sid]['state'] = 'active'
        st[sid]['stop'] = d + life.expo(sc.mean_life_days)
        on_map += 1
      else:
        still.append(sid)
    queue = still
    for _ in range(new):
      arrived += 1
      if on_map < sc.capacity:
        sid = len(st)
        st[sid] = dict(state='active', site=None, stop=d + life.expo(sc.mean_life_days))
        st[sid]['site'] = w.found(sid, d)
        on_map += 1
      else:
        refused += 1
    w.keep(d)
    if day in days:
      occ = list(w.occ)
      sts = w.stewards_on_map()
      out[day] = dict(
        active=sum(1 for s in st.values() if s['state'] == 'active'),
        away=sum(1 for s in st.values() if s['state'] == 'away'),
        on_map=len(sts), waiting=len(queue), refused=refused, arrived=arrived,
        free=len(w.free_sites()), standins=len(w.standins()),
        per_region=len(occ) / lay.size['regions'],
        ai_share=(len(occ) - len(sts)) / max(1, len(occ)),
        mean=sum(w.contacts(s) for s in occ) / max(1, len(occ)),
        steward_mean=sum(w.contacts(s) for s in sts) / max(1, len(sts)),
        below_min=sum(1 for s in sts if w.contacts(s) < p.contacts_min) / max(1, len(sts)),
        no_contact=sum(1 for s in sts if w.contacts(s) == 0) / max(1, len(sts)),
        with_steward=sum(1 for s in sts if w.steward_neighbour(s)) / max(1, len(sts)))
  return out


def lifetime_mean(sc, p=P, keeper=True, seeds=LIFETIME_SEEDS):
  runs = [lifetime(sc, s, p, keeper) for s in seeds]
  return {d: {k: sum(r[d][k] for r in runs) / len(runs) for k in runs[0][d]} for d in runs[0]}


# ---------------------------------------------------------------- contact timing (WLD-063, WLD-064)
def day_reaching(radius, p=P):
  if radius <= p.founding_radius_wu:
    return 0.0
  if radius > p.max_radius_wu:
    return math.inf
  f = (radius - p.founding_radius_wu) / (p.max_radius_wu - p.founding_radius_wu)
  return p.plateau_days * f ** (1.0 / p.growth_exponent)


def first_valley_contact_day(p=P):
  return day_reaching(p.site_spacing_wu / 2.0, p)


def first_crossing_contact_day(p=P):
  return day_reaching(p.crossing_mouth_wu, p)


def lens_width(p=P):
  return 2 * p.max_radius_wu - p.site_spacing_wu


def centre_clearance(p=P):
  return p.site_spacing_wu / math.sqrt(2.0) - p.max_radius_wu


def territory_area_at_plateau(p=P, step=16.0):
  """A settlement's home cell with its own two valley border areas at plateau, in a full valley, by sampling."""
  off = (p.region_side_wu - p.site_spacing_wu) / 2.0
  sx = sz = off
  others = [(off + p.site_spacing_wu, off), (off, off + p.site_spacing_wu),
            (off + p.site_spacing_wu, off + p.site_spacing_wu)]
  lo, hi = p.coast_band_wu, p.region_side_wu - p.coast_band_wu
  area = 0.0
  z = lo + step / 2
  while z < hi:
    x = lo + step / 2
    while x < hi:
      d = math.hypot(x - sx, z - sz)
      if d <= p.max_radius_wu and all(d <= math.hypot(x - a, z - b) for a, b in others):
        area += step * step
      x += step
    z += step
  return area


def own_units(p=P):
  """The resource units every site holds by default (WLD-085): its home sites, the resource sites of its own two
  valley border areas and of its own crossing border area or, without a crossing, one more home resource site."""
  return p.home_units + 2 * p.border_factor + p.crossing_factor


def holdable_units_max(p=P):
  """What a settlement can hold at most (WLD-086): its own units, its neighbours' border areas on its own borders
  won by claims, and its valley's central resource site."""
  return own_units(p) + 2 * p.border_factor + p.crossing_factor + p.central_factor


# ---------------------------------------------------------------- travel (WLD-100 to WLD-104)
def minutes(wu, speed):
  return wu / speed / 60.0


def travel_table(p=P):
  return dict(to_border=p.site_spacing_wu / 2.0, valley=p.site_spacing_wu * p.valley_path_factor,
              centre=p.site_spacing_wu / math.sqrt(2.0) * p.valley_path_factor)


def travel_over_seeds(capacity, seeds=LAYOUT_SEEDS, p=P):
  cross, hop, dia = [], [], []
  for s in seeds:
    lay = Layout(capacity, s, p)
    cross += lay.crossing_lengths()
    hop.append(lay.region_hop_wu())
    dia.append(lay.diameter_wu())
  return cross, hop, dia


def spread(vals):
  v = sorted(vals)
  return v[0], v[len(v) // 2], v[-1]


# ---------------------------------------------------------------- battles (WLD-284, WLD-285)
def poisson_quantile(lam, q=0.99):
  if lam <= 0:
    return 0
  k, pk, cum = 0, math.exp(-lam), math.exp(-lam)
  while cum < q:
    k += 1
    pk *= lam / k
    cum += pk
  return k


def poisson_tail(lam, k):
  """P(N >= k) for N ~ Poisson(lam)."""
  if k <= 0:
    return 1.0
  pk, cum = math.exp(-lam), math.exp(-lam)
  for i in range(1, k):
    pk *= lam / i
    cum += pk
  return max(0.0, 1.0 - cum)


def battle_cap(settlements, p=P):
  return max(p.battle_cap_min, math.ceil(p.battle_cap_share * settlements))


def battle_case(settlements, b, case, p=P, pace='always_on'):
  """Battles in progress for one case: the expected number at the evening peak (or during a session), its 99th
  percentile and its weekly maximum, with battles starting in clusters of k (compound Poisson), and the share of
  time at which the world's battle cap is reached."""
  _, dur, share, window, k = case
  per_hour = settlements * b / 2.0 / 168.0
  if pace == 'session':
    lam = per_hour * p.session_factor * dur / 60.0
    windows = 7 * 24 * 60.0 / dur
  else:
    lam = per_hour * ((1 - share) + share * 24.0 / window) * dur / 60.0
    windows = 7 * window * 60.0 / dur
  p99 = k * poisson_quantile(lam / k, 0.99)
  wmax = k * poisson_quantile(lam / k, 1.0 - 1.0 / windows)
  cap = battle_cap(settlements, p)
  at_cap = poisson_tail(lam / k, math.ceil(cap / k))
  return dict(mean=lam, p99=p99, week_max=wmax, cap=cap, at_cap=at_cap)


def expedition_load(settlements, b, p=P):
  t = travel_table(p)
  attack_min = max(p.warning_min, minutes(t['to_border'], p.march_speed_wus)) + 8.0 + minutes(t['valley'],
                                                                                              p.march_speed_wus)
  attacks = settlements * b / 2.0 / (7 * 24 * 60) * attack_min
  gathers = settlements * p.gather_trips_per_day / (24 * 60) * (2 * minutes(t['centre'], p.march_speed_wus)
                                                                + p.gather_stay_min)
  return attacks + gathers


def present_share(p=P):
  present_min = p.typical_active_min + p.typical_check_ins * p.check_in_min
  return present_min / (24 * 60), present_min / (p.evening_window_h * 60)


def watched_union(capacity, seed=1, p=P, draws=200):
  """The union of the settlements that present stewards' settlement views make watched at the evening peak
  (WLD-244): typically the own settlement and its valley neighbours; at most also the diagonal site of the valley
  and the two sites that face it across the channels (its crossing neighbour among them), the six whose territory
  comes within the view's margin of 800 wu. Full steward occupancy."""
  w = World(Layout(capacity, seed, p), keeper=True)
  for i in range(capacity):
    w.found(i)
  sts = sorted(w.stewards_on_map())
  _, peak = present_share(p)
  n = max(1, int(round(len(sts) * peak)))
  s_typ = s_top = 0
  rng = Stream(seed, 201)
  for _ in range(draws):
    pool = list(sts)
    pres = []
    for _ in range(n):
      pres.append(pool.pop(int(rng.next() * len(pool))))
    typ, top = set(), set()
    for s in pres:
      typ.add(s)
      top.add(s)
      for q, kind in w.lay.nb[s]:
        if kind == 'valley' and q in w.occ:
          typ.add(q)
          top.add(q)
      for q in [w.lay.site_of(w.lay.sites[s][2], DIAGONAL[w.lay.sites[s][3]])] + w.lay.opposites(s):
        if q in w.occ:
          top.add(q)
    s_typ += len(typ)
    s_top += len(top)
  return s_typ / draws, s_top / draws, n


# ---------------------------------------------------------------- reports
def pct(x):
  return f'{100 * x:.0f} %'


def span(vals, fmt):
  """The range of a figure over seeds, as the document writes it: one value, or 'low to high'."""
  lo, hi = fmt.format(min(vals)), fmt.format(max(vals))
  return lo if lo == hi else f'{lo} to {hi}'


def report(p=P):
  print('# A1 world-density model: report\n')
  print('## World size (WLD-004 to WLD-008)\n')
  print('| stewards | native AI | site margin | sites | regions | land grid | reserved extent (wu) | land samples (M) |'
        ' heights binary64 (MB) | normals (MB) | heights 16-bit (MB) |')
  print('|---|---|---|---|---|---|---|---|---|---|---|')
  for cap in (1, 4, 8, 16, 50, 100, 200, 256):
    s = world_size(cap, p)
    m = s['land_samples']
    print(f"| {cap} | {s['ai']} | {s['margin']} | {s['sites']} | {s['regions']} | {s['land_grid']} |"
          f" {s['extent']:,.0f} | {m / 1e6:.3f} | {m * 8 / 1e6:.1f} | {m * 24 / 1e6:.1f} | {m * 2 / 1e6:.1f} |")
  print('\n## Geometry (exact: the valley pattern moves as a whole)\n')
  print(f'- lens at plateau {lens_width(p):.0f} wu, split at its halfway line into two border areas')
  print(f'- valley centre outside the largest territory by {centre_clearance(p):.0f} wu')
  print(f'- territory at plateau (home cell and its own two border areas) {territory_area_at_plateau(p) / 1e6:.2f}'
        f' M wu2; core {math.pi * p.core_radius_wu ** 2 / 1e6:.2f} M wu2')
  print(f'- resource units: {own_units(p)} held by every site by default, at most {holdable_units_max(p)}'
        f' ({holdable_units_max(p) / own_units(p):.2f} x)')
  print('\n## Travel at 20 wu/s over seeds 1 to 10 (min, median, max), minutes\n')
  t = travel_table(p)
  print(f"- to the halfway line {minutes(t['to_border'], 20):.1f}; to the valley centre"
        f" {minutes(t['centre'], 20):.1f}; valley neighbour {minutes(t['valley'], 20):.1f}")
  for cap in (16, 200):
    cr, hop, dia = travel_over_seeds(cap, p=p)
    print(f'- capacity {cap}: crossing neighbour {", ".join(f"{minutes(v, 20):.1f}" for v in spread(cr))};'
          f' region hop {", ".join(f"{minutes(v, 20):.1f}" for v in spread(hop))};'
          f' diameter {", ".join(f"{minutes(v, 20):.1f}" for v in spread(dia))}')
  print('\n## Contact\n')
  print(f'- first valley contact day {first_valley_contact_day(p):.1f}, first crossing contact day'
        f' {first_crossing_contact_day(p):.1f}, for every settlement, late joiners included')
  print('\n| stewards | occupancy | settlements | per region | stand-ins | mean contacts | a steward\'s contacts |'
        ' stewards below 2 | stewards with a steward neighbour |')
  print('|---|---|---|---|---|---|---|---|---|')
  for cap in (1, 16, 200):
    for share in (0.25, 0.5, 1.0):
      if cap == 1 and share < 1.0:
        continue
      o = fresh_world(cap, share, 1, p)
      print(f"| {cap} | {pct(share)} | {o['occupied']} | {o['per_region']:.2f} | {o['standins']} | {o['mean']:.2f} |"
            f" {o['steward_mean']:.2f} | {pct(o['below_min'])} | {pct(o['with_steward'])} |")
  print('\n## Scale handed to A2 (WLD-280 to WLD-285)\n')
  for cap in (16, 200):
    s = world_size(cap, p)
    n = cap + s['ai']
    ty, top, pres = watched_union(cap, 1, p)
    print(f'- {cap} stewards: {n} settlements at full occupancy in {s["regions"]} regions;'
          f' bodies per full region {p.sites_per_region * p.pop_plateau}; expeditions in progress'
          f' {expedition_load(n, 6.0, p):.1f} at 6 battles a week; present at the peak {pres};'
          f' watched union {ty:.0f} typical, {top:.0f} at most')
  print()
  battles(p)


def battles(p=P):
  print('### Battles in progress (evening peak of one time zone; session pace for 16)\n')
  print('| stewards | pace | battles per settlement per week | case | mean | p99 | weekly max | bodies at weekly max |'
        ' world cap | share of time at the cap |')
  print('|---|---|---|---|---|---|---|---|---|---|')
  for cap, paces in ((16, ('always_on', 'session')), (200, ('always_on',))):
    n = cap + world_size(cap, p)['ai']
    for pace in paces:
      for b in p.battles_per_week:
        for case in BATTLE_CASES:
          r = battle_case(n, b, case, p, pace)
          print(f"| {cap} | {pace} | {b:g} | {case[0]} | {r['mean']:.2f} | {r['p99']} | {r['week_max']} |"
                f" {r['week_max'] * p.bodies_per_battle:,} | {r['cap']} | {pct(r['at_cap'])} |")
  print()


def lifetime_report(p=P):
  print('# A year of world days (5 seeds each)\n')
  print('| scenario | keeper | day | active | away on map | on map | waiting returns | refused / arrived |'
        ' free sites | stand-ins | per region | AI share | mean contacts | a steward\'s contacts | stewards below 2 |'
        ' with no contact | with a steward neighbour |')
  print('|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|')
  for sc in SCENARIOS:
    for keeper in (True, False):
      res = lifetime_mean(sc, p, keeper)
      for d in LIFETIME_DAYS:
        s = res[d]
        print(f"| {sc.name} | {'on' if keeper else 'off'} | {d} | {s['active']:.1f} | {s['away']:.1f} |"
              f" {s['on_map']:.1f} | {s['waiting']:.1f} | {s['refused']:.0f} / {s['arrived']:.0f} |"
              f" {s['free']:.0f} | {s['standins']:.1f} | {s['per_region']:.2f} | {pct(s['ai_share'])} |"
              f" {s['mean']:.2f} | {s['steward_mean']:.2f} | {pct(s['below_min'])} | {pct(s['no_contact'])} |"
              f" {pct(s['with_steward'])} |")


def sweep(p=P):
  print('| spacing (wu) | max radius (wu) | growth exponent | first valley day | first crossing day | lens (wu) |'
        ' centre clearance (wu) |')
  print('|---|---|---|---|---|---|---|')
  for s in (1920.0, 2080.0, 2240.0, 2400.0):
    for rmax in (1300.0, 1400.0, 1500.0):
      for g in (0.5, 0.75, 1.0):
        q = replace(p, site_spacing_wu=s, max_radius_wu=rmax, growth_exponent=g)
        print(f'| {s:.0f} | {rmax:.0f} | {g} | {first_valley_contact_day(q):.1f} |'
              f' {first_crossing_contact_day(q):.1f} | {lens_width(q):.0f} | {centre_clearance(q):.0f} |')


def founding(p=P):
  print('| stewards | occupancy | rule | mean contacts | stewards below 2 | with a steward neighbour |')
  print('|---|---|---|---|---|---|')
  for cap in (16, 200):
    for share in (0.25, 0.5, 1.0):
      for name, keeper, weight in (('WLD-140 with stand-ins', True, None), ('no stand-ins', False, None),
                                   ('stand-ins, steward weight 2', True, 2)):
        o = fresh_world(cap, share, 1, p, keeper, weight)
        print(f"| {cap} | {pct(share)} | {name} | {o['mean']:.2f} | {pct(o['below_min'])} |"
              f" {pct(o['with_steward'])} |")


def layout_text(capacity, seed, p=P):
  lay = Layout(capacity, seed, p)
  s = lay.size
  print(f"capacity {capacity}, seed {seed}: {s['regions']} regions, {s['sites']} sites, {s['ai']} native AI,"
        f" margin {s['margin']}, land grid {s['land_grid']}, reserved extent {s['extent']:.0f} wu")
  g = s['land_grid']
  for j in reversed(range(g)):
    print(' '.join('#' if (i, j) in lay.region_index else '.' for i in range(g)))
  print(f'{len(lay.valley_pairs)} valley pairs, {len(lay.crossings)} crossings')


# ---------------------------------------------------------------- the document's tables (check compares them)
def doc_tables(p=P):
  """Every table of figures that the document states, as rows of formatted cells. `tables` prints them as
  Markdown; `check` compares them with STATED_TABLES."""
  t = {}
  rows = []
  for cap in (1, 4, 8, 16, 50, 100, 200, 256):
    s = world_size(cap, p)
    rows.append((str(cap), str(s['ai']), str(s['margin']), str(s['sites']), str(s['regions']), str(s['land_grid']),
                 f"{s['land_samples']:,.0f}"))
  t['size'] = rows
  rows = []
  for cap in (16, 200):
    for share in (0.25, 0.5, 1.0):
      runs = [fresh_world(cap, share, sd, p) for sd in LAYOUT_SEEDS]
      rows.append((str(cap), pct(share), span([o['occupied'] for o in runs], '{:.0f}'),
                   span([o['standins'] for o in runs], '{:.0f}'), span([o['per_region'] for o in runs], '{:.2f}'),
                   span([o['mean'] for o in runs], '{:.1f}'), span([o['steward_mean'] for o in runs], '{:.1f}'),
                   pct(max(o['below_min'] for o in runs)),
                   span([100 * o['with_steward'] for o in runs], '{:.0f}') + ' %'))
  t['fresh'] = rows
  rows = []
  for sc in SCENARIOS:
    if sc.name == '200, engaged':
      continue
    on = lifetime_mean(sc, p, True)
    off = lifetime_mean(sc, p, False)
    for d in LIFETIME_DAYS:
      a, b = on[d], off[d]
      rows.append((sc.name, str(d), f"{a['on_map']:.0f}", f"{a['refused']:.0f} / {a['arrived']:.0f}",
                   f"{a['per_region']:.2f}", pct(a['ai_share']), f"{a['steward_mean']:.2f}", pct(a['below_min']),
                   pct(b['below_min']), pct(a['with_steward'])))
  t['lifetime'] = rows
  rows = []
  for cap, paces in ((16, ('always_on', 'session')), (200, ('always_on',))):
    n = cap + world_size(cap, p)['ai']
    for pace in paces:
      for b in p.battles_per_week:
        cells = [str(cap), 'session' if pace == 'session' else 'evening peak', f'{b:g}']
        for case in BATTLE_CASES:
          r = battle_case(n, b, case, p, pace)
          cells.append(f"{r['p99']} ({r['week_max']})")
        r = battle_case(n, b, BATTLE_CASES[1], p, pace)
        cells += [str(r['cap']), pct(r['at_cap'])]
        rows.append(tuple(cells))
  t['battles'] = rows
  rows = []
  t_ = travel_table(p)
  rows.append(('halfway line', f"{t_['to_border']:,.0f}", f"{minutes(t_['to_border'], 20):.1f}",
               f"{minutes(t_['to_border'], 12):.1f}"))
  rows.append(('valley centre', f"{t_['centre']:,.0f}", f"{minutes(t_['centre'], 20):.1f}",
               f"{minutes(t_['centre'], 12):.1f}"))
  rows.append(('valley neighbour', f"{t_['valley']:,.0f}", f"{minutes(t_['valley'], 20):.1f}",
               f"{minutes(t_['valley'], 12):.1f}"))
  cr16, hop16, dia16 = travel_over_seeds(16, p=p)
  cr200, hop200, dia200 = travel_over_seeds(200, p=p)
  for name, vals in (('crossing neighbour', cr16 + cr200), ('region hop', hop16 + hop200), ('diameter 16', dia16),
                     ('diameter 200', dia200)):
    lo, med, hi = spread(vals)
    rows.append((name, f'{lo:,.0f} to {hi:,.0f}', f'{minutes(lo, 20):.1f} to {minutes(hi, 20):.1f}',
                 f'{minutes(lo, 12):.1f} to {minutes(hi, 12):.1f}'))
  t['travel'] = rows
  rows = []
  for cap in (16, 200):
    s = world_size(cap, p)
    n = cap + s['ai']
    wu = [watched_union(cap, sd, p) for sd in LAYOUT_SEEDS]
    pres = wu[0][2]
    m = s['land_samples']
    rows.append((str(cap), str(n), str(s['regions']), f"{n / s['regions']:.2f}", f'{expedition_load(n, 6.0, p):.1f}',
                 str(pres), span([round(v[0]) for v in wu], '{:.0f}'), span([round(v[1]) for v in wu], '{:.0f}'),
                 f'{m / 1e6:.2f}', f'{m * 8 / 1e6:.1f}', f'{m * 24 / 1e6:.1f}', f'{m * 2 / 1e6:.1f}'))
  t['scale'] = rows
  return t


def print_tables(p=P, as_python=False):
  t = doc_tables(p)
  if as_python:
    print('STATED_TABLES = {')
    for k, rows in t.items():
      print(f'  {k!r}: [')
      for r in rows:
        print(f'    {r!r},')
      print('  ],')
    print('}')
    return
  for k, rows in t.items():
    print(f'## {k}\n')
    for r in rows:
      print('| ' + ' | '.join(r) + ' |')
    print()


# ---------------------------------------------------------------- check
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
    for k, key in enumerate(('ai', 'margin', 'sites', 'regions', 'land_grid')):
      same(f'{key} at {cap}', s[key], want[k])
    same(f'land samples at {cap} (M)', round(s['land_samples'] / 1e6, 3), want[5], 0.0005)
  same('reserved extent (wu)', int(world_size(16, p)['extent']), STATED['extent_wu'])
  t = travel_table(p)
  same('Wellspring to the halfway line (min)', round(minutes(t['to_border'], p.march_speed_wus), 1),
       STATED['travel_lens_min'], 0.05)
  same('valley neighbour (min)', round(minutes(t['valley'], p.march_speed_wus), 1), STATED['travel_valley_min'],
       0.05)
  cr16, hop16, dia16 = travel_over_seeds(16, p=p)
  cr200, hop200, dia200 = travel_over_seeds(200, p=p)
  for k, v in enumerate(spread(cr16 + cr200)):
    same(f'crossing neighbour (min) {("min", "median", "max")[k]}', round(minutes(v, p.march_speed_wus), 1),
         STATED['travel_crossing_min'][k], 0.05)
  for k, v in enumerate(spread(hop16 + hop200)):
    same(f'region hop (min) {("min", "median", "max")[k]}', round(minutes(v, p.march_speed_wus), 1),
         STATED['travel_hop_min'][k], 0.05)
  for cap, dia in ((16, dia16), (200, dia200)):
    for k, v in enumerate(spread(dia)):
      same(f'diameter at {cap} (min) {("min", "median", "max")[k]}', round_half_up(minutes(v, p.march_speed_wus)),
           STATED[f'diameter_{cap}_min'][k])
  same('first valley contact (day)', round(first_valley_contact_day(p), 1), STATED['first_valley_day'], 0.05)
  same('first crossing contact (day)', round(first_crossing_contact_day(p), 1), STATED['first_crossing_day'], 0.05)
  same('lens width (wu)', int(lens_width(p)), STATED['lens_width_wu'])
  same('valley centre clearance (wu)', int(round(centre_clearance(p))), STATED['centre_clearance_wu'])
  same('territory at plateau (M wu2)', round(territory_area_at_plateau(p) / 1e6, 2), STATED['territory_plateau_mwu2'],
       0.005)
  q = replace(p, growth_exponent=1.0)
  same('linear growth: first valley and crossing contact days',
       (round(first_valley_contact_day(q), 1), round(first_crossing_contact_day(q), 1)), STATED['linear_growth_days'])
  same('valley centre from each site centre (wu)', int(round(p.site_spacing_wu / math.sqrt(2.0))),
       STATED['centre_distance_wu'])
  jd = minutes(t['valley'], p.march_speed_wus) / (24 * 60)
  same('journey to a valley neighbour (world days)', (round(jd, 4), round(jd * p.session_factor, 3)),
       STATED['journey_days'])
  o = fresh_world(1, 1.0, 1, p)
  same('solo world: settlements per region and a steward\'s contacts', (round(o['per_region'], 1),
                                                                        round(o['steward_mean'], 1)), STATED['fresh_1'])
  same('strategic view highest camera at 16 and 200 (wu)',
       tuple(int(0.9 * world_size(c, p)['land_grid'] * p.region_side_wu) for c in (16, 200)),
       STATED['strategic_max_wu'])
  s16 = world_size(16, p)
  per_side = int(s16['extent'] / 1024)
  land = s16['regions'] * int(p.region_side_wu / 1024) ** 2
  same('chunks of a world of 16: land, sea', (land, per_side * per_side - land), STATED['chunks_16'])
  straight = [v / p.crossing_path_factor - 2 * p.crossing_mouth_wu for v in cr16 + cr200]
  gap = (int(round(min(straight))), int(round(max(straight))))
  same('crossing mouths apart (wu), nearest and farthest', gap, STATED['mouth_gap_wu'])
  lays = [Layout(cap, sd, p) for cap in (16, 200) for sd in LAYOUT_SEEDS]
  bridges = [br for lay in lays for br in lay.bridges()]
  to_border = [v for br in bridges for v in br[2:4]]
  same('a mouth from the region border (wu), nearest and farthest',
       (int(round(min(to_border))), int(round(max(to_border)))), STATED['mouth_border_wu'])
  counts = [len(br[4]) for br in bridges]
  same('squares per bridge, fewest and most', (min(counts), max(counts)), STATED['bridge_squares'])
  same('flatten areas a region\'s layout needs, at most', max(lay.flatten_areas_max() for lay in lays),
       STATED['layout_flatten_max'])
  same('binary32 spacing at 71,680 wu', round(2.0 ** (math.floor(math.log2(s16['extent'])) - 23), 4),
       STATED['binary32_spacing_wu'], 0.00005)
  for cap in (16, 200):
    counts = [Layout(cap, sd, p).sites_without_crossing() for sd in LAYOUT_SEEDS]
    same(f'sites without a crossing at {cap}, fewest and most', (min(counts), max(counts)),
         STATED[f'no_crossing_{cap}'])
  same('own resource units', own_units(p), STATED['own_units'])
  same('holdable resource units', holdable_units_max(p), STATED['holdable_units_max'])
  for key, name in (('life_200_strong', '200, strong demand'), ('life_200_weak', '200, weak demand'),
                    ('life_16_slow', '16, a friend every 20 days')):
    sc = next(s for s in SCENARIOS if s.name == name)
    d = lifetime_mean(sc, p)[180]
    want = STATED[key]
    same(f'{name}, day 180: stewards on the map', int(round(d['on_map'])), want[0])
    same(f'{name}, day 180: refused', int(round(d['refused'])), want[1])
    same(f'{name}, day 180: settlements per region', round(d['per_region'], 1), want[2], 0.05)
    same(f'{name}, day 180: a steward\'s contacts', round(d['steward_mean'], 1), want[3], 0.05)
    same(f'{name}, day 180: stewards below 2 contacts', round(d['below_min'], 2), want[4], 0.005)
  for cap, key, pace in ((16, 'battles_16', 'always_on'), (200, 'battles_200', 'always_on'),
                         (16, 'battles_16_session', 'session')):
    n = cap + world_size(cap, p)['ai']
    for k, case in enumerate(BATTLE_CASES):
      same(f'battles p99 at {cap}, {pace}, {case[0]}', battle_case(n, 6.0, case, p, pace)['p99'], STATED[key][k])
  for k, cap in enumerate((16, 200)):
    n = cap + world_size(cap, p)['ai']
    r = battle_case(n, 6.0, BATTLE_CASES[1], p)
    same(f'weekly maximum at {cap}, planning', r['week_max'], STATED['battles_week_max'][k])
    same(f'battle cap at {cap}', r['cap'], STATED['battle_cap'][k])
  for cap in (16, 200):
    wu = [watched_union(cap, sd, p) for sd in LAYOUT_SEEDS]
    same(f'watched union at {cap}, typical, fewest and most over seeds',
         (min(round(v[0]) for v in wu), max(round(v[0]) for v in wu)), STATED[f'watched_{cap}'][0])
    same(f'watched union at {cap}, at most, fewest and most over seeds',
         (min(round(v[1]) for v in wu), max(round(v[1]) for v in wu)), STATED[f'watched_{cap}'][1])
  same('bodies per full region', p.sites_per_region * p.pop_plateau, STATED['bodies_region'])
  tables = doc_tables(p)
  for name, rows in STATED_TABLES.items():
    got = tables.get(name, [])
    same(f'table {name}: rows', len(got), len(rows))
    for k, (g, want) in enumerate(zip(got, rows)):
      same(f'table {name}: row {k + 1} ({want[0]})', g, want)
  print(f'{bad} difference(s)')
  return bad


def main(argv):
  mode = argv[1] if len(argv) > 1 else 'check'
  if mode == 'check':
    return 1 if check() else 0
  if mode == 'report':
    report()
  elif mode == 'lifetime':
    lifetime_report()
  elif mode == 'battles':
    battles()
  elif mode == 'sweep':
    sweep()
  elif mode == 'founding':
    founding()
  elif mode == 'tables':
    print_tables(as_python=len(argv) > 2 and argv[2] == 'python')
  elif mode == 'layout':
    layout_text(int(argv[2]) if len(argv) > 2 else 16, int(argv[3]) if len(argv) > 3 else 1)
  else:
    print(__doc__)
    return 2
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
