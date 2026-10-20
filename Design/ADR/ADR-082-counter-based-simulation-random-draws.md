---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead; proposed by design-phase area A2 (gate B asked no question of it)
---

# Simulation random draws: counter-based values of a secret key, keyed by purpose, subject, tick and ordinal

## Context and Problem Statement

The reference draws all gameplay randomness from one sequential simulation generator per match (`SimRng`, R-SIM-605), keeps the presentation generators apart (R-SIM-607, R-SIM-608), fixes the draw order inside expressions (R-SIM-621) and leaves the algorithm and the stream structure to DEC-242 (R-SIM-606). A Species world is persistent and partitioned into regions that step in parallel and may later live in separate processes (O1, ADR-085). One sequential stream couples every region and system: one extra draw anywhere shifts every later draw everywhere, parallel phases would have to draw in a fixed global order, and a region's state would depend on the draws of others. Area A1 needs counter-based draws keyed by seed, purpose, region, attempt and index for world generation (WLD-161), from a world seed that is shown to the world host (WLD-006) so that a world's shape can be made again. Draws that a client could compute in advance would tell a player the outcome of every Supply crate and damage roll before it happens.

Which generator and which stream structure does the Species simulation use, and what keeps its draws unpredictable to players?

## Decision Drivers

* Region steps run in parallel and must give the serial result (R-SIM-732, ADR-083).
* A draw added in one system must not change another system's draws, so tests stay local and a region's state depends only on its own inputs.
* Nothing but the keys needs persisting (DEC-251 item 9), and a region moves between processes with its state alone.
* No player can predict a simulation draw, from the public world seed or from replicated values.
* Plain C++ with no library, the same on every build (ADR-081 Level 2); the reference draw forms and presentation generators stay as they are.

## Considered Options

* A — one sequential stream per match or world, with a current generator (PCG32 or xoshiro128++)
* B — counter-based values: each draw a keyed function of the draw's keys
* C — one sequential stream per system, each seeded from the world seed

## Decision Outcome

Proposed option: "B — counter-based values", with a secret key and a keyed function. **Default while Proposed:** the decision below.

1. **The simulation key.** Every world has a 128-bit simulation key, drawn by its server from the operating system's cryptographic generator when the world is created, stored in the world's records, never shown to anyone, never sent to a client and never written to a log, a replay or a journal outside the world's own store. A skirmish match has one, drawn at its start and kept in its setup on the server. The public world seed (WLD-006) keys world generation only.
2. **The function.** A simulation draw is SipHash-2-4 (Aumasson and Bernstein, 2012; a keyed function whose outputs do not reveal its key) with the simulation key, over a 40-byte message of five unsigned 64-bit fields in little-endian order: the domain 1 (simulation); the **purpose**, an enumerator that names the draw site family (for example a Worker's retarget, a laser shot's damage roll, a Supply crate's outcome), one per site family of R-SIM-630's table and of the Species rules; the **subject**, the 64-bit stable id of the object that draws (ADR-083 item 5), or the owner's own id for a draw that belongs to no object (the match rules, the keeper, a world event), never 0; the **tick**, the world tick or match tick; and the **ordinal**, the number of earlier draws of the same purpose by the same subject in the same tick. Species implements it in the engine maths library in plain C++, with the published test vectors as its unit tests.
3. **The draw context.** `SimRng` stays the type that code draws from, with the reference draw forms of R-SIM-605 and the x-then-y-then-z order of vector draws: a `SimRng` value is made for one subject, one purpose and one tick, holds its ordinal, and is never shared between subjects or threads; making a second context for the same subject, purpose and tick is an assertion failure in Debug and test builds. `NextUint` is the top 32 bits of the 64-bit value; `NextUnit(r)` is (value >> 11) × 2⁻⁵³ × *r*, in [0, *r*); `NextSigned(r)` is (`NextUnit`(1) − 0.5) × *r*, in [−*r*/2, *r*/2); a uniform integer in [0, *n*) is ((value >> 32) × *n*) >> 32. One draw per statement, no draw in a constructor, an argument list or a loop condition (R-SIM-621, R-SIM-626), and every rejection loop capped (R-SIM-623). No raw draw value is ever replicated or shown; presentation that needs variety per object derives it from the object's stable id with a presentation generator.
4. **World generation** keeps A1's keyed values over the public world seed (WLD-161): the output function of SplitMix64 with its increment (`Mix64`(x): x += 0x9E3779B97F4A7C15; x = (x ^ (x >> 30)) × 0xBF58476D1CE4E5B9; x = (x ^ (x >> 27)) × 0x94D049BB133111EB; return x ^ (x >> 31), modulo 2⁶⁴) chained over the seed and the keys domain 2, purpose, region id, attempt number and item index, as `Tools/DesignModels/A1WorldDensity.py` (`unit01`) computes it; a world's shape is public by design, so its draws need no secret.
5. **Other keyed orders.** The order of the senders' commands within a tick (ADR-087 item 4) is a SipHash-2-4 value of the simulation key over the domain 3, the tick and the sender's stable id.
6. **Presentation generators** stay apart (R-SIM-607, R-SIM-608): each presentation consumer owns an instance of a sequential generator (PCG32), unreachable from simulation code, seeded from its own data where its output must look the same on every run.
7. **What is persisted** is the simulation key, the public world seed and the world tick; no generator state exists to persist or to hand over.
8. **Quality.** A test of the simulation's test project draws 10⁸ values over the key patterns that the step uses (sequential ticks, sequential subjects, ordinals 0 to 15, every purpose) and checks their frequency, serial correlation and bit balance against stated bounds; a failure blocks the milestone.

### Consequences

* Good, because draws need no order between subjects, so parallel phases and parallel regions draw without coordination and give the serial result.
* Good, because a draw added to one purpose changes no other purpose's draws, and a region's state depends only on its own subjects.
* Good, because no player can predict a draw: the key is never shown, and recovering it from observed outcomes is as hard as breaking SipHash.
* Bad, because a draw costs about 0.03 µs instead of about 0.001 µs for a sequential generator step; at the reference's draw rates (R-SIM-630) that is 1.35 ms per tick for a world of 200 at its stress case, 0.7 % of its tick.
* Bad, because a subject's identity and the tick are part of every draw, so every object needs its stable id from its creation (ADR-083), and a rule that draws for an object before it has one is a defect.
* Neutral, because the reference's draw values are not reproduced, as R-SIM-606 already states; its distributions are. A defect reproduction needs the world's store (which holds the key), never a client's data.

### Confirmation

**Confirmed when** (amended 2026-10-19 for the MVP's milestones, O51 and O55): at the MVP's M1, the simulation-core milestone, the SipHash-2-4 implementation passes its published test vectors, the quality test of item 8 passes, the battle sandbox and the synthetic battle give identical hashes at 1, 2 and 8 worker threads (ADR-081), and a test adds a draw to one purpose and shows every other purpose's values unchanged (MVP-093); and on the M3 build a test client given everything the steward's client receives cannot compute a later draw (it never receives the key).

## Pros and Cons of the Options

### A — one sequential stream

* Good, because it is the reference structure and the fastest draw.
* Bad, because it couples every region and system, forces a global draw order on parallel work and needs its state persisted and handed over.

### B — counter-based values of a secret key

* Good, because draws are independent by key, stateless and unpredictable without the key.
* Bad, because each draw needs its keys, and every object a stable id.

### C — one stream per system

* Good, because systems no longer shift each other.
* Bad, because the objects of one system still share one order across regions and threads.

## More Information

* Resolves DEC-242 (Resolved 2026-10-19); answers A1's request for WLD-161. Related: [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-087](ADR-087-command-path-validation-and-timing.md).
* Golden hashes recorded under the default of DEC-242 (one PCG32 stream) are recorded again under this record, in a reviewed change (DBG-147).
* **Amendment 2026-10-18 (challenge round 1).** Simulation draws use a secret simulation key and SipHash-2-4 instead of `Mix64` over the public seed, because `Mix64` is invertible and the seed is shown to the host (A2-F09); an owner-level draw uses its owner's id as its subject; a second context is an assertion; the command order of ADR-087 uses the same function.
* **Amendment 2026-10-19 (after owner gate B; O51, O55).** Gate B changed nothing in this record. The "Confirmed when" is re-pointed from the simulation-core milestone to the MVP's M1, which builds the keyed draws (MVP-093), and to its M3 build for the client test; the record stays Proposed until that evidence exists.
* **Amendment 2026-10-20 (M1 task T9, [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md)).** The M1 part of the confirmation runs in CI: the published SipHash-2-4 vectors, item 8's quality test at 100,663,296 values over 32 purposes, the sandbox's and the synthetic battle's hashes at 1, 2 and 8 worker threads, and extra draws of each purpose through a tick's draw contexts leaving every other purpose's values unchanged. The M3 part, a client that cannot compute a later draw, remains.
