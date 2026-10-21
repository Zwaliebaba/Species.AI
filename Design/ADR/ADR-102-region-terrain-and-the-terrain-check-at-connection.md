---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T3)
---

# Region terrain: the terrain library, the region's generation and the terrain check at connection

## Context and Problem Statement

M1's battle sandbox is fought on the valley of MVP-010. Task T3 of the [M1 plan](../implementation/m1_plan.md) builds the terrain generator of WLD-160 to WLD-163 for the MVP's one region, with one template, one biome and one lighting preset, no set pieces and no bridges (MVP-011). The server and the client generate the terrain from the world's seed with the same library and compare a terrain hash at connection; a client whose hash differs refuses the world with a message (MVP-011, the TER-009 amendment).

The terrain component specifies the height field of a skirmish map (TER-002 to TER-252), and A1 adds what a world needs (WLD-160 to WLD-166, WLD-268). These decisions are open:
- where the library and the region's generation live;
- what the terrain component lacks for a region;
- how a failed check re-rolls;
- which form of the terrain hash the client compares (TER-252's or WLD-166's, the M1 plan's open point);
- where the seed lives, and how a saved world gets its terrain back;
- how the server sends the terrain's identity, and what the client does with it.

## Decision Drivers

* The server and the client must build the same samples, bit for bit, from one function of the seed and the tuning record (TER-010, WLD-161).
* The engine knows no game concept (`AGENTS.md` R9): a region, a site and a valley are the game's.
* Every tuning value lives in data (MVP-099).
* Generation runs outside every tick (TER-242, WLD-165).
* The terrain never changes for the life of the world (MVP-011, TER-002).

## Considered Options

* 1 — the height field in NeuronCore, the region's generation in GameShared, the seed in the world owner's state, the region's chunk digests compared at connection
* 2 — the region's generation in GameLogic, the server baking the field and sending it to the client
* 3 — the terrain as state: the field in the snapshot and in the state hash

## Decision Outcome

Chosen option: "1".
- Option 2 is WLD-166's world game, with chunks delivered by digest (DEC-246). The MVP generates on both ends (MVP-011), and a client of GameLogic would link the server's simulation, which ADR-090 forbids.
- Option 3 puts 819,200 bytes of samples that never change into every snapshot and every tick's hash, where 64 bits of seed give them again.

* **The library** (NeuronCore `TerrainField`, `Sha256`). `TerrainField::Generate` is TER-060's generator: tiles, guide grids, flatten areas, the binary32-rounded functions of TER-065, the bilinear read of TER-073, the normals of TER-170 and the canonical digest of TER-252. It knows terrain records and nothing of a region.
  - It adds the origin of the grid, so a region's field covers its own square of the world (TER-002 amendment), WLD-268's generation method 3 (`UnbiasedParent`) and its flatten types 5 (`SmoothUnbiased`) and 6 (`Feathered`) (TER-035 amendment).
  - `Sha256` is FIPS 180-4, written here (R14), and is checked against the standard's vectors.
* **The region's generation** (GameShared `RegionTerrain`). GameShared is linked by both ends: the server through GameLogic and the client through GameClient.
  - `BuildRegionTerrain` runs one attempt of WLD-160 to WLD-162 for the region at grid cell `mvp.world.region_cell` (6) on both axes: its origin is 30,720 wu, which is aligned with the chunks as WLD-020 requires (the CAM-095 amendment's figure of 33,280 wu is corrected).
    - The site pattern of WLD-040, then one base tile over the region inside its coast bands, and two hill tiles in two corners chosen with the seed.
    - The first pass reads each core's rim and the valley centre.
    - The second pass adds the feathered squares at those levels.
    - Last, the 25 chunks' digests.
  - Every draw is WLD-161's counter-based value: SplitMix64's output function chained over the seed and the keys domain 2, purpose, region, attempt and index, as A1's model computes it (ADR-082 item 4). The purposes are `WorldDrawPurpose`, numbered from 1 and never renumbered.
  - The values are the record's: the `wld.*` keys of A1, the reference's sea level, shore height and walkable slope, and the temperate parameter set of WLD-162 as `mvp.terrain.*` (MVP scope table 3).
  - Two A1 keys spelled `centre` are renamed `center` (WLD-162 amendment), because the schema generator refuses a British stem in a field name (DAT-054, `AGENTS.md` R11).
  - The hill tiles stand `mvp.terrain.hill_inset` (300 wu) inside the region border, as WLD-162 states. A hill of 640 wu then reaches up to 204 wu into the feather band of the core in its corner, never nearer than 63 wu to its level square, so WLD-162's condition on the band could not hold together with the inset. The owner kept both the hill and the inset, and the condition is now the core's level square (O71, WLD-162 amendment): the band blends into the hill, and checks 3 to 6 pass.
* **The checks and the re-roll.** `CheckRegionTerrain` runs WLD-163's checks 1 and 3 to 6 on the region's samples. Check 2 has no crossing to check, check 7 waits for M2's yields, and check 8 runs from T5.
  - A region that fails tries the next attempt of the same seed, up to `wld.generation.max_attempts` (8), as WLD-163 says.
  - It then tries the next seed, for as many seeds, as MVP-011 says. The seed shown and stored is the one that passed (MVP-005); it gives the same terrain at its first passing attempt.
  - When no attempt passes, world creation fails with the checks of the last attempt.
  - Over seeds 1 to 100, every region passed at attempt 0 with site floors of 56.1 to 113.1 wu. This was measured by a sweep program over `BuildRegionTerrain` and `CheckRegionTerrain`, built with g++ 13 at -O1 on a 2.1 GHz Xeon container. The slowest attempt there took 177 ms, both passes and the chunk digests included.
* **The terrain hash** is WLD-166's form over the region: TER-252's digest of each 64 × 64 chunk of the region (z outer, x inner), and the SHA-256 of the 25 chunk digests in that order.
  - It is chosen over TER-252's digest of the whole field: the chunk digests are the identities that DEC-246 delivers and caches chunks by, so the world game reuses them.
  - It is the region's rather than the reserved extent's 4,900 chunks. Every chunk outside the MVP's region is sea at the outside height, which the region's own border strip already holds, so the extent's chunks add nothing that a comparison could catch. A world of more regions takes WLD-166's whole form.
* **The seed** is the world owner's state: `WorldSimulation` writes it after the world tick, so the state hash and the snapshot cover it (ADR-097, ADR-099).
  - `WORLD_STATE_VERSION` becomes 3. The migration step reads a world of versions 1 and 2 as one of seed 0 (ADR-099 amendment).
  - The golden run of DBG-147 changes once, by the 64 bits of seed 0. An independent model of the layout computes the same values.
* **The terrain is derived, not state.** `WorldServer::Open` builds it before the first tick (TER-242) and attaches it to the region; a region holds it as a shared, immutable `RegionTerrain` (TER-010). Neither the hash nor the snapshot holds it.
  - A restored world builds it again from its seed. A snapshot whose seed gives no terrain is refused like a damaged one, and the next older is tried.
  - A new world's seed is the steward's, through the server's description (MVP-005), or one the server draws from the system's generator. A drawn seed has 32 bits, so that it can be typed again.
  - A world of more regions, which only tests make, has terrain in region 0 only.
* **The terrain check at connection.** The server sends `WorldTerrain` (type id after `TickReport`, version 1, 43 bytes: the seed and the 32-byte digest) once to each connection on the reliable channel, before its first tick report. A send that the backlog refuses is tried at the next tick.
  - The client builds the region from that seed with its own library and its own tuning record, which its composition root reads from the content root as a remote client would.
  - It accepts the world when the seed and the digest agree.
  - Otherwise it refuses the world: it logs both digests, closes the connection, and shows the refusal in the window title until the UI has a message for it.
  - The client builds the region in the frame that receives the message, before the first frame that draws it (T6).

### Consequences

* Good, because one function of the seed and the record is the whole terrain, on the server, on the client and after a restore.
* Good, because the chunk digests are already the world game's chunk identities (WLD-166, DEC-246).
* Bad, because a client and a server built by compilers whose `pow`, `exp` or `log` differ in a result's last bit near a binary32 boundary would build different samples (TER-065 rounds to binary32 to make this rare, not impossible).
  - The expected digests in the tests come from the conformance generator (`Tools/DesignModels/TerrainGenerator.py`, extended from WLD-268's text) on CPython and glibc, so MSVC's CRT is held to them on every CI run.
  - The ARM64 development build (ADR-094) is held to them when its tests run.
* Bad, because the client's frame that builds the region stalls for as long as one attempt takes, as long as the client has no loading screen.
* Bad, because the hash and the snapshot now carry the seed, so the state version and the golden values changed.
* It forecloses: terrain as snapshot state; a client that accepts a server's terrain without building it; a world seed that is not part of the world's state.

### Confirmation

* `NeuronCoreTests` checks:
  - SHA-256 against FIPS 180-4's vectors;
  - the field's digests and normal digest for the conformance generator's record sets, method 3 and flatten types 5 and 6 among them;
  - a field that is the same at any origin.
* `GameSharedTests` checks:
  - A1's keyed values;
  - the site pattern;
  - one seed giving one terrain;
  - the levelled cores and centre;
  - a fixture per check that fails that check alone;
  - the chunk digests and the region's digest;
  - the `WorldTerrain` round trip.
* `GameLogicTests` checks:
  - a given seed's terrain and a drawn seed;
  - the terrain sent once to every connection;
  - a frozen world opening with the same terrain;
  - the seed kept by a snapshot;
  - versions 1 and 2 migrating to seed 0;
  - the new golden values.
* `GameClientTests` checks that the client accepts the terrain it builds again, and refuses a digest that differs, with its message, closing the connection.
* WLD-165's budget is measured on the reference test machine when M1 pins it (O58).

## Pros and Cons of the Options

### 1 — library in NeuronCore, region in GameShared, seed as state, region digest

* Good, because the engine's terrain serves a skirmish map unchanged, with an origin of (0, 0).
* Bad, because the client generates as the server does, which WLD-166's chunk delivery later replaces.

### 2 — server-side generation, the field sent

* Good, because the client trusts the server's samples and builds nothing.
* Bad, because the MVP has no chunk delivery, and the client would link the server's library.

### 3 — terrain as state

* Good, because a snapshot restores without generating.
* Bad, because the snapshot and the hash grow by the field, and the field never changes.

## More Information

* Related: [ADR-060](ADR-060-content-identity-by-hash.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-101](ADR-101-region-teams-figures-and-update-slices.md).
* The rules: TER-002 to TER-252 ([terrain](../architecture/components/terrain/README.md)), WLD-040, WLD-041 and WLD-160 to WLD-166 ([terrain, time and views](../gdd/01_world_and_regions/terrain_time_and_views.md)), WLD-268 ([skirmish maps and map tools](../gdd/01_world_and_regions/skirmish_maps_and_map_tools.md)), MVP-005, MVP-010 and MVP-011 ([MVP scope](../implementation/mvp_scope.md)).
* The code is in NeuronCore (`TerrainField`, `Sha256`), GameShared (`RegionTerrain`, `WorldTerrain`), GameLogic (`WorldSimulation`, `RegionSimulation`, `WorldServer`) and GameClient (`ClientSession`).
* **Amendment 2026-10-22 (M1's exit, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md) amendment).** The performance tool generates the valleys of seeds 1 to 100 on one thread, and the script reports the worst and the mean time, every attempt, check and re-roll included. It sets them against the smaller per-region share of WLD-165's budgets: 20 s for the 8 regions of a world of 16 stewards, and 120 s for the 77 regions of a world of 200 (WLD-004, table 3), which gives 1.56 s. The bake that WLD-165 includes is not built yet. The owner's next run on the reference test machine gives the figure the confirmation asks for.
* **Amendment 2026-10-21 (M1 task T14, [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md)).** `WorldTerrain` (version 2, 44 bytes) also names the team that the connection commands as a steward, or none, so that the client knows its own team for its picking and cursor.
* **Amendment 2026-10-20 (M1 task T5, [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md)).** A region's terrain carries its land route graph, built from its samples by the client and the server alike, and `CheckRegionTerrain` runs check 8 after checks 1 and 3 to 6. Over seeds 1 to 100 every region still passes at attempt 0; the slowest attempt now takes 250 ms, its graph included, measured as above.
