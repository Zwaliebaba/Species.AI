---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Simulation tick rate: 10 ticks per second for the world and for skirmish, run as whole ticks on the server

## Context and Problem Statement

Every duration of the reference library is a count of ticks of `ref.sim.tick_period` (0.1 s, 10 ticks per second, R-SIM-500), and every reference tuning table holds tick-denominated values: durations, cool-downs, per-tick probabilities, per-tick movement and decay, the 11-tick mode second (R-SIM-503, DEC-002, O17). The vision pillars require one tick for both forms of play (PIL-001, PIL-085), and A0 leaves its rate to A2 (DEC-227). The reference runs each tick as ten update slices spread over a participant's render frames (R-SIM-510 to R-SIM-536); Species runs an authoritative server, integrated in the host's client first and dedicated later (O10), whose clients render replicated state (DEC-200). The tick rate multiplies the server's CPU per region and the replication bandwidth per client (DEC-227 item 6) and sets the command latency a player feels (DEC-203). The reference's steered special units (the Strike team, the Transport) are directly controlled with the stick or the pointer (R-CTRL-100), the case most sensitive to latency.

Which tick rate does Species simulate at, and how does the server execute a tick?

## Decision Drivers

* Skirmish is the first playable and the combat balance test bed, and it adopts the reference crowd rules as written (O8, O20, PIL-002); the reference values are the Species starting values (O13), and the mode second of O17 is stated in ticks.
* Server CPU at 200 stewards (O1) and on the integrated host's own machine (O10), and the host's home upload in stage 1.
* Command-to-display latency for crowd orders and for the direct control of special units (PIL-010, DEC-203).
* One tick for both forms of play (PIL-085), and an abstract economy that can be cheaper than the full simulation (O3).
* A rate fixed by measurement, not by assumption (DEC-227 item 8).

## Considered Options

* A — keep 10 ticks per second
* B — 20 ticks per second
* C — 30 ticks per second
* D — 20 or 30 ticks per second for the full simulation, and a lower rate for the abstract economy model

## Decision Outcome

Proposed option: "A — keep 10 ticks per second", with option D's lower cadence for the abstract economy and the prediction of directly controlled units of ADR-084. **Default while Proposed:** the decision below; its figures are those of `Tools/DesignModels/A2ServerBudget.py`, computed, not measured.

1. **One tick rate.** The full simulation of the world (battles, watched settlements, expeditions, the movement of every figure) and of skirmish advances in ticks of `ref.sim.tick_period` (0.1 s, 10 ticks per second). Every tick-denominated reference value keeps its value and its meaning, the 11-tick mode second of O17 included, and no tuning table is converted.
2. **Whole ticks, slices in order.** The server executes each tick whole: the commands of the tick (ADR-087), the tick-clock update, then the ten update slices of R-SIM-510 to R-SIM-516 back to back in slice order, in one step of each state owner (ADR-083). The slice partitioning of the sliced pools, the fixed slices of R-SIM-513 and the same-tick advance rule of R-SIM-516 therefore keep every observable order of the reference. No client steps the simulation, and no update slice is spread over render frames: the reference's slice pacing and catch-up (R-SIM-530 to R-SIM-536) and its sequencer schedule (R-SIM-520 to R-SIM-524) are replaced by the server tick loop of ADR-083 item 7.
3. **The abstract economy steps every 10 ticks.** A settlement at abstract fidelity (O3) advances in abstract steps of 10 ticks (1 s of world time), on a phase fixed by its settlement id so that the steps of a region spread over the ten ticks of each second. B2 and D2 may lengthen the step to a multiple of 10 ticks of at most 100 ticks; a fidelity switch happens at the tick boundary that ends an abstract step, and a settlement stays at full fidelity for at least 60 s after the last settlement view leaves it (ADR-085 item 6).
4. **What the client draws.** Clients interpolate replicated state one tick plus a jitter allowance behind the newest (ADR-084 item 4), and predict only the movement of the unit that their steward directly controls (ADR-084 item 6); the reference's render-time extrapolation (R-SIM-560 to R-SIM-565) is not used. A changed control state is sent at once (ADR-087 item 5).
5. **A later change of rate** (a superseding ADR) rescales the reference values by one rule, which this record fixes now so that the decision can be reversed without a design pass (R-SIM-505): a duration of *n* ticks becomes round-half-up(*n* × *r*′ ÷ *r*) ticks, and at least 1; a per-tick probability *p* becomes 1 − (1 − *p*)^(*r* ÷ *r*′); a per-tick factor *f* (a decay or a blend) becomes *f*^(*r* ÷ *r*′); a per-tick increment (a distance, a speed step, a rate) is multiplied by *r* ÷ *r*′; where *r* is the old and *r*′ the new number of ticks per second. Every converted value moves to a Species tuning key, and its requirement is adapted in the ledger. The gate-B budgets carry their 20 Hz figures beside them (ADR-080, ADR-086), so such a change does not reopen them.

### Consequences

* Good, because every reference rule and tuning value binds as written, so skirmish tests the reference balance rather than a conversion of it, and the 11-tick mode second of O17 keeps its meaning.
* Good, because the server costs half as much CPU as at 20 ticks per second: a world of 200 at its stress case takes 1.85 reference cores against 3.71 at 20 Hz, and on the 16-vCPU host of ADR-080 its tick reaches the 50 ms wall budget at 2.22 times the cost budgets, where 20 Hz reaches its 25 ms budget at 1.10 times; a battle view costs 23.2 KB/s against 36.9, and the integrated host's upload for 15 remote stewards is 2.8 Mbit/s against 4.4, above its 4 Mbit/s default.
* Good, because the server, the integrated server and every test run execute whole ticks in one order, which keeps the step reproducible (ADR-081).
* Bad, because an order's latency is higher than at 20 Hz. The model's Monte Carlo, with the client's input sampling, a queued frame and half a scan-out, network jitter of 5 ms each way and the server's step, gives mean / 95th percentile: on the integrated host 212 / 258 ms (127 / 152 at 20 Hz); for a remote client at a 50 ms round trip 302 / 350 ms (217 / 245); at 150 ms 402 / 450 ms. The integrated host feels as the reference does, whose steady state is a 0 to 100 ms wait for the next tick plus a 100 ms pacing delay (R-SIM-534).
* Good, because the latency that a player feels most, steering a unit directly, does not depend on the tick: the predicted unit moves 42 / 53 ms after the input on every client, at either rate (ADR-084 item 6).
* Neutral, because the reference's update slices survive only as an order inside a tick; their purpose of spreading cost over frames is gone.

### Confirmation

**Confirmed when:** the first playable (O20) on the integrated server, over a playtest of at least 6 players, measures at the 95th percentile at most 275 ms from an order to its display on the host and at most 375 ms for a remote client at a 50 ms round trip, at most 60 ms from a direct-control input to the predicted unit's movement on every client, and the effects of direct control (hits) within the order thresholds; the players rate crowd orders and the direct control of a Strike team as responsive; and the simulation-core milestone has measured the tick cost of ADR-080. If a criterion fails, a superseding ADR moves the full simulation to 20 ticks per second with the conversion rule of item 5, on the 20 Hz budgets that ADR-080 and ADR-086 state, before content is tuned beyond the reference values.

## Pros and Cons of the Options

### A — 10 ticks per second

* Good, because no reference value is converted and nothing of the balance test bed changes.
* Good, because it halves the server CPU and the replication bandwidth against B.
* Bad, because an order reaches the display about 85 ms later than at B; prediction removes this for directly controlled units only.

### B — 20 ticks per second

* Good, because it halves the display delay and the wait for the next tick; it divides 60 frames per second evenly.
* Bad, because every tick-denominated value of every reference tuning table is converted, per-tick probabilities, factors and integration steps included, and the skirmish no longer tests the reference rules as written.
* Bad, because it doubles the server CPU (3.71 reference cores at 200 stewards, stress case) and halves the tick's wall budget, so a world of 200 needs about twice the stage-3 host for the same margin, and it raises a battle view's bandwidth by 59 %.

### C — 30 ticks per second

* Good, because it gives the lowest latency.
* Bad, because it triples the server CPU and converts every value as B does, for a crowd game whose orders tolerate 100 to 200 ms beyond their immediate local feedback (DEC-203).

### D — a higher full rate and a lower abstract rate

* Good, because the abstract economy is cheap at a low rate; this record keeps that part (item 3).
* Bad, because its full rate is B's or C's, with their conversion and cost.

## More Information

* Resolves DEC-227 when the owner confirms it at gate B. Related: [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md) (budgets), [ADR-083](ADR-083-server-process-ownership-and-threading.md) (the tick loop), [ADR-084](ADR-084-client-boundary-through-the-transport.md) (interpolation and prediction), [ADR-087](ADR-087-command-path-validation-and-timing.md) (command timing).
* The latency figures are `A2ServerBudget.py latency`: 100,000 samples from a seeded generator; the server's step is 20 ms at 16 stewards and the 16-vCPU host's 22.7 ms at 200 (a remote client at 50 ms in a world of 200: 304 / 352 ms).
* World-game timers in world days convert to ticks by WLD-202 at this rate.
* **Amendment 2026-10-18 (challenge round 1).** The latency model gained the client terms and percentiles (A2-F03); the confirmation splits its thresholds for orders and for direct control, and adds the prediction of ADR-084 (MB-1); the 20 Hz figures stand beside the budgets.
