# Species design decisions

> Status: Species design decisions · Range: DEC-1000 to DEC-1999 · Related: [decisions index](README.md), [owner decisions](owner_decisions.md), [world and sites](../gdd/01_world_and_regions/world_and_sites.md), [MVP scope](../implementation/mvp_scope.md), [conventions](../CONVENTIONS.md)

This file records the decisions of the Species design and of the MVP built from it that the design does not settle: questions that building the MVP raises about a Final requirement, numbered with four digits from one thousand (CONVENTIONS §3). Each entry states the behaviour that the code builds until the owner decides. An **Open** entry is the owner's to decide, and a **Resolved** entry records the owner's decision.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-1000](#dec-1000-the-place-of-the-ninth-home-resource-site-on-the-mvps-terrain) | The place of the ninth home resource site on the MVP's terrain | Resolved | WLD, MVP |

## 1 World and sites

### DEC-1000 The place of the ninth home resource site on the MVP's terrain
- Status: Resolved
- Affects: gdd/01_world_and_regions/world_and_sites.md (WLD-042, WLD-082, WLD-085, `wld.resource.extra_bearing`), gdd/01_world_and_regions/terrain_time_and_views.md (WLD-163 check 7), implementation/mvp_scope.md (MVP-010); ADR-102, ADR-130
- Specified behaviour: The ninth home resource site of a site without a crossing stands at `wld.resource.extra_radius` (1,050 wu) from the site's centre, at a bearing of `wld.resource.extra_bearing` (+84°) from the outward diagonal instead of on it. Check 7 of WLD-163 counts no yield of a resource site that stands at or below the shore height or on ground steeper than walkable.
  - On the diagonal, the place lies 2,521 wu from the valley's centre towards the region's corner, where M1's island base tile is lowest (ADR-102). Over seeds 1 to 100 it is at or below `ref.unit.worker.shore_height` or steeper than `ref.sim.walk.min_normal_y` at 188 of the 400 sites, in 93 seeds.
  - The other ten resource sites of every site are dry and walkable on all of them.
  - At +84° and the same radius, every site of those seeds is walkable, has no sample at or below the shore height within 200 wu, and stands at least 315 wu from every other resource site and 237 wu outside each of its own valley border areas. Every seed therefore passes check 7 at its first attempt, as it passes the other checks.
  - The bearing lies between 80°, where one seed has water within 200 wu, and 86°, where the border areas come within 200 wu (WLD-082).
- Options:
  - A — as stated: the same radius at +84°, keeping the 11 units, every gathering distance and every seed's terrain;
  - B — the ninth on the diagonal as WLD-082 states, with the island's corners raised by the M1 terrain keys (`mvp.terrain.guide_rim`) until the place is dry and walkable on every seed; every seed's terrain changes, and M1's evidence on its seeds is measured again;
  - C — the MVP's sites hold 10 units, without the ninth home resource site, and MVP-010 and WLD-085 are amended;
  - D — WLD-082 as written, with check 7 re-rolling a region whose ninth site is wet; 93 of seeds 1 to 100 fail at their first attempt, so most seeds give another valley.
- Recommendation: A. It moves one value of the Final world design and nothing a steward can measure: every site keeps its 11 units at the same distances (WLD-042), and the valley of every seed that M1 measured stays as it was.
- Decide by: M2 task T10, before the growth scene tunes the economy on the 11 units.
- Resolution (2026-10-22, owner decision O87): option A. The ninth home resource site of a site without a crossing stands at `wld.resource.extra_radius` (1,050 wu) at `wld.resource.extra_bearing` (84°, now final) from the outward diagonal, as the specified behaviour states, and check 7 keeps counting no yield of a resource site in the sea or on a slope. Every site keeps its 11 units at the same distances, every seed's valley stays as M1 measured it, and M2 task T10's tuning, made on this place, stands (WLD-082 and MVP-010 amendments, [ADR-130](../ADR/ADR-130-settlements-structures-and-resource-sites-at-m2.md) amendment).
