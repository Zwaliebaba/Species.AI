# Species design decisions

> Status: Species design decisions · Range: DEC-1000 to DEC-1999 · Related: [decisions index](README.md), [owner decisions](owner_decisions.md), [world and sites](../gdd/01_world_and_regions/world_and_sites.md), [MVP scope](../implementation/mvp_scope.md), [conventions](../CONVENTIONS.md)

This file records the decisions of the Species design and of the MVP built from it that the design does not settle: questions that building the MVP raises about a Final requirement, numbered with four digits from one thousand (CONVENTIONS §3). Each entry states the behaviour that the code builds until the owner decides. An **Open** entry is the owner's to decide, and a **Resolved** entry records the owner's decision.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-1000](#dec-1000-the-place-of-the-ninth-home-resource-site-on-the-mvps-terrain) | The place of the ninth home resource site on the MVP's terrain | Resolved | WLD, MVP |
| [DEC-1001](#dec-1001-the-provisional-looks-of-the-settlement) | The provisional looks of the settlement | Open | MVP |

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

## 2 Presentation

### DEC-1001 The provisional looks of the settlement
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-082, MVP-083, and table 3's `mvp.structure.*`, `mvp.wellspring.*`, `mvp.orb.cloudy_size`, `mvp.orb.cloudy_alpha` and `mvp.worker.load_color`); ADR-140
- Specified behaviour: The client draws the settlement with the first values of table 3 (owner decision O89). MVP-083 states that a construction site rises with its progress, that an inactive structure is dark and that a completed structure flashes once, and MVP-082 that a carried load is visible, but not how far, how dark, how large or in what colour.
  - A construction site is sunk by `mvp.structure.rise_depth_factor` (1.0) × its mesh's height × the share of its work still to come, so it rises out of the ground as it is built.
  - An inactive structure's lit colour is multiplied by `mvp.structure.inactive_brightness` (0.35), and it shows no ownership lights.
  - A completed structure flashes once: a camera-facing `fx.starburst` of half-size `mvp.structure.flash_size` (40 wu) in its team's colour, its own half-size above the structure's place, fading over `mvp.structure.flash_seconds` (0.5 s) from the tick of its event.
  - The Wellspring glows: `mvp.wellspring.glow_blobs` (5) quads of `fx.glow_cloudy`, of half-size `mvp.wellspring.glow_size` (22 wu) and alpha `mvp.wellspring.glow_alpha` (0.45) in its team's colour, circle the centre of its crystal at `mvp.wellspring.glow_radius` (12 wu), one turn every `mvp.wellspring.glow_turn_seconds` (24 s).
  - An orb drifting home or waiting at its Wellspring is wrapped in a quad of `fx.glow_cloudy` of half-size `mvp.orb.cloudy_size` (16 wu) and alpha `mvp.orb.cloudy_alpha` (0.5) in its settlement's colour.
  - A Worker that carries a load shows `sprite.worker.carry_load` over its body in `mvp.worker.load_color` (0.6, 0.9, 1.0).
- Options:
  - A — the values as stated, which the owner retunes after M2 task T12's run, as the two lights were retuned after M1's runs (O75, O83);
  - B — values that the owner sets before the run.
- Recommendation: A. Each look is one key of the client's presentation record, read at start-up, so a retune changes no code and nothing the simulation computes.
- Decide by: the owner's run of M2 task T12; the likeness review of PIL-027 at M2's end judges the result.
