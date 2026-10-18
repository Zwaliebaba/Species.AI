---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Simulation random draws: counter-based values keyed by seed, purpose, subject, tick and ordinal

## Context and Problem Statement

The reference draws all gameplay randomness from one sequential simulation generator per match (`SimRng`, R-SIM-605), keeps the presentation generators apart (R-SIM-607, R-SIM-608), fixes the draw order inside expressions (R-SIM-621) and leaves the algorithm and the stream structure to DEC-242 (R-SIM-606). A Species world is persistent and partitioned into regions that step in parallel and may later live in separate processes (O1, ADR-085). One sequential stream couples every region and system: one extra draw anywhere shifts every later draw everywhere, parallel phases would have to draw in a fixed global order, and a region's state would depend on the draws of others. Area A1 needs counter-based draws keyed by seed, purpose, region, attempt and index for world generation (WLD-161).

Which generator and which stream structure does the Species simulation use?

## Decision Drivers

* Region steps run in parallel and must give the serial result (R-SIM-732, ADR-083).
* A draw added in one system must not change another system's draws, so tests stay local and a region's state depends only on its own inputs.
* Nothing but the seed needs persisting (DEC-251 item 9), and a region moves between processes with its state alone.
* One function serves the simulation and world generation (WLD-161), on every build (ADR-081 Level 2), in plain C++ with no library.
* The reference draw forms and the presentation generators stay as they are.

## Considered Options

* A — one sequential stream per match or world, with a current generator (PCG32 or xoshiro128++)
* B — counter-based values: each draw a hash of the seed and the draw's keys
* C — one sequential stream per system, each seeded from the world seed

## Decision Outcome

Proposed option: "B — counter-based values". **Default while Proposed:** the decision below.

1. **The mixing function** is the output function of SplitMix64 with its increment: `Mix64(x)`: x += 0x9E3779B97F4A7C15; x = (x ^ (x >> 30)) × 0xBF58476D1CE4E5B9; x = (x ^ (x >> 27)) × 0x94D049BB133111EB; return x ^ (x >> 31), in unsigned 64-bit arithmetic modulo 2⁶⁴. A **keyed value** of a seed *s* and keys *k*₁ to *k*ₙ (each an unsigned 64-bit integer) is h₀ = `Mix64`(*s*), hᵢ = `Mix64`(hᵢ₋₁ ^ *k*ᵢ), and the value is hₙ. `Tools/DesignModels/A1WorldDensity.py` (`unit01`) computes the same function.
2. **Simulation draws.** Every simulation draw is the keyed value of the world seed (WLD-006) or, in skirmish, the match seed (R-SIM-610), with the keys, in this order: the domain 1 (simulation); the **purpose**, a 32-bit enumerator that names the draw site family (for example a Worker's retarget, a laser shot's damage roll, a Supply crate's outcome), one per site family of R-SIM-630's table and of the Species rules; the **subject**, the 64-bit stable id of the object that draws (ADR-083), or 0 for a draw that belongs to no object (the match rules, the keeper, a world event); the **tick**, the 64-bit world tick or match tick; and the **ordinal**, the number of earlier draws of the same purpose by the same subject in the same tick.
3. **The draw context.** `SimRng` stays the type that code draws from, with the reference draw forms of R-SIM-605 and the x-then-y-then-z order of vector draws: a `SimRng` value is made for one subject, one purpose and one tick, holds the hash of its key prefix and its ordinal, and is never shared between subjects or threads. `NextUint` is the top 32 bits of the value; `NextUnit(r)` is (value >> 11) × 2⁻⁵³ × *r*, in [0, *r*); `NextSigned(r)` is (`NextUnit`(1) − 0.5) × *r*, in [−*r*/2, *r*/2); a uniform integer in [0, *n*) is ((value >> 32) × *n*) >> 32. One draw per statement, no draw in a constructor, an argument list or a loop condition (R-SIM-621, R-SIM-626), and every rejection loop capped (R-SIM-623).
4. **World generation** uses the same function with the domain 2 and A1's keys (WLD-161): the purpose, the region id, the attempt number and the item index, so a region's terrain and layout depend on no other region.
5. **Presentation generators** stay apart (R-SIM-607, R-SIM-608): each presentation consumer owns an instance of a sequential generator (PCG32), unreachable from simulation code, seeded from its own data where its output must look the same on every run (R-SIM-607).
6. **What is persisted** is the world seed and the world tick; no generator state exists to persist or to hand over.
7. **Quality.** A test of the simulation's test project draws 10⁸ values over the key patterns that the step uses (sequential ticks, sequential subjects, ordinals 0 to 15, every purpose) and checks their frequency, serial correlation and bit balance against stated bounds; a failure blocks the milestone.

### Consequences

* Good, because draws need no order between subjects, so parallel phases and parallel regions draw without coordination and give the serial result.
* Good, because a draw added to one purpose changes no other purpose's draws, and a region's state depends only on its own subjects.
* Good, because the world seed is the only random state, so persistence, hand-over and defect reproduction carry nothing else.
* Bad, because a draw context costs five mixes when it is made and each draw one more (about 1 ns a mix), where a sequential generator costs one step per draw; at the reference's draw rates (R-SIM-630) this is far below 1 % of a tick.
* Bad, because a subject's identity and the tick are part of every draw, so every object needs its stable id (ADR-083), and a rule that draws for an object before it has one is a defect.
* Neutral, because the reference's draw values are not reproduced, as R-SIM-606 already states; its distributions are.

### Confirmation

**Confirmed when:** the quality test of item 7 passes; the simulation-core milestone's synthetic battle gives identical hashes at 1, 2 and 8 worker threads (ADR-081); and a test adds a draw to one purpose and shows every other purpose's values unchanged.

## Pros and Cons of the Options

### A — one sequential stream

* Good, because it is the reference structure and the fastest draw.
* Bad, because it couples every region and system, forces a global draw order on parallel work and needs its state persisted and handed over.

### B — counter-based values

* Good, because draws are independent by key and stateless.
* Bad, because each draw needs its keys, and every object a stable id.

### C — one stream per system

* Good, because systems no longer shift each other.
* Bad, because the objects of one system still share one order across regions and threads.

## More Information

* Resolves DEC-242 when the owner confirms it at gate B; answers A1's request for WLD-161. Related: [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md).
* Golden hashes recorded under the default of DEC-242 (one PCG32 stream) are recorded again under this record, in a reviewed change (DBG-147).
