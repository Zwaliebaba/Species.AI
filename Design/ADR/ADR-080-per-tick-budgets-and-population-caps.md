---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Per-tick budgets and population caps: per-figure cost budgets, a region step budget, hard caps and named host specifications

## Context and Problem Statement

The reference has no per-tick CPU budget and no engine population limit (DEC-228); its design population is 2,000 units per map and its stress population 4,000 (DBG-204), with a client guidance of 4.2 ms per tick at the design population on the reference test machine (DBG-205, DEC-971). Species runs a server that simulates every battle in full whether anyone watches it (O3), a world of up to 16 stewards on the host's own machine in stage 1 (O10, O40) and up to 200 on hosted servers in stage 3 (O1, O5). Area A1 hands over the load: 26 settlements in 8 regions at 16 stewards and 256 in 77 at 200; up to 16 and 177 settlements watched at the evening peak; 4 battles in progress at 16 (6 at the session pace) and 16 at 200 in the planning case, and up to 8 (12) and 28 in the stress case (WLD-280, WLD-284); and it asks for the region body budget (WLD-281) and the battle caps (WLD-285). Pillar 1 needs battles of 400 figures and is at risk below 200 (PIL-021).

What does a tick cost, what limits keep it within budget, and on what hardware?

## Decision Drivers

* A budget that can fail a test: per system, on a named machine, at a named population (DBG-205).
* The integrated host runs the server beside its own client at 60 frames per second (RND-454, DEC-228 item 6).
* Community hosts are small machines (O5); hosted worlds of 200 must not need a rewrite (O1).
* Pillar 1's battle sizes (PIL-021) and pillar 6's waiting attacks (WLD-285).
* Caps that never make an outcome depend on host hardware (DEC-228 option C rejected).

## Considered Options

* A — a client-derived budget: at most 25 % of a 16.7 ms frame per tick at 2,000 units on minimum specification, with the server budget per region derived from it
* B — a server-first budget: fixed milliseconds per region step on a stated host, with the client budget derived from it
* A and B together, with hard engine caps and per-figure cost budgets
* C — no hard budget; population limited dynamically by measured cost

## Decision Outcome

Proposed option: "A and B together, with hard engine caps and per-figure cost budgets". **Default while Proposed:** the decision below; its figures come from `Tools/DesignModels/A2ServerBudget.py`, which computes them from the cost budgets and A1's load, and are computed, not measured.

1. **Reference core.** Every CPU budget is stated in time on one core of the reference test machine (DEC-971). A vCPU of a rented host counts as 0.6 reference cores until measured.
2. **Cost budgets per object per tick**, which the implementation meets and the simulation-core milestone measures:

   | Object | Budget, µs per tick | Basis |
   |---|---|---|
   | figure in a battle (search, combat movement, laser bolts, Essence orbs) | 2.1 | the DBG-205 guidance: 4.2 ms per tick at 2,000 units |
   | Worker or Leader at settlement work (gathering, carrying, construction, guard duty) | 1.2 | the same search stagger without combat |
   | figure of an expedition outside a battle | 0.4 | route and formation following only |
   | settlement at abstract fidelity | 5 | one abstract step of 50 µs every 10 ticks (ADR-079) |
   | AI settlement or deputy | 20 | one 1 ms decision pass every 50 ticks |
   | command of one side of a battle | 30 | |
   | region fixed cost (grids, messages, the seen-layer summary of WLD-243) | 30 | |

3. **Region step budget.** One region's step takes at most 10 ms on one reference core. The **region body budget** is 4,000 bodies, the stress population of DBG-204, at which a region whose every body fights costs 8.43 ms. The settlements of a region take at most 75 % of it (WLD-281): 3,000 bodies, so the population limit that B1 sets at the plateau is at most 750 Workers and Leaders per settlement for four settlements per region; A1's placeholder of 600 takes 60 %.
4. **Battle size.** A battle has at most 1,000 figures of both sides together, 2.5 times the typical 400 of PIL-021; B4's reinforcement rules keep a battle inside it, and an expedition that would take a battle above it waits at the battlefield's edge. A battle of 400 costs 0.91 ms per tick, one of 1,000 2.18 ms, one of the floor's 200 0.48 ms.
5. **Battle caps** (WLD-285): at most 2 battles in progress per region, and at most max(8, ⌈0.1 × the settlements at full occupancy⌉) per world: 8 at 16 stewards and 26 at 200. A1 recommended a world minimum of 4; at 8 the model's share of the evening peak at the cap falls from 4 % to 0 % in the planning case and from 23 % to 3 % in the stress case at the always-on pace, and from 11 % to 0 % and from 37 % to 8 % at the session pace, for 3.6 ms more per tick.
6. **Tick wall budget.** The whole of a tick (every region step in parallel, the world owner's step, replication and persistence) finishes within 50 % of the tick period, 50 ms, on the host's hardware; ADR-083 states what happens on an overrun.
7. **Integrated server budget.** The integrated server uses at most 1 reference core averaged over 10 s, on its tick thread and at most 2 worker threads, on the reference test machine, so the client keeps RND-454's 60 frames per second. The model's worst case for 16 stewards, every steward present with every settlement watched and 8 battles, is 0.28 reference cores.
8. **Host specifications** for gate B: a community server of 16 stewards (stage 2) runs on 2 vCPU (1.2 reference cores), 1 GB of memory for the server and 10 Mbit/s of upload; a hosted world of 200 (stage 3) runs on 8 vCPU (4.8 reference cores), 2 GB and 50 Mbit/s. At the cost budgets the model's stress cases take 0.28 reference cores at 16 stewards (24 % of the community host) and 1.67 at 200 (35 % of the stage-3 host).
9. **Caps are hard and never drop a body.** A region at its body budget refuses growth (Essence waits at the Wellspring until bodies fall below it); a settlement at its population limit grows no further (B1); a battle at its size waits reinforcements at the edge; an attack beyond a battle cap waits at its gate (WLD-285); a client over its bandwidth cap is sent fewer full-rate figures (ADR-086). No cap removes, teleports or kills a figure, and every cap is a tuning value of the component that enforces it (D2, D3).
10. **Stress populations.** A region stress test runs 4,000 bodies in one region (four settlements at the population limit and two battles of 500 attackers); the world stress tests are A1's stress cases on the hosts of item 8 (26 battles in progress at 200, 8 at 16) and the integrated case of item 7; the client keeps the 4,000-unit stress map of DBG-204 and RND-456.

### Consequences

* Good, because every budget can fail a test on a named machine, and every cap has a number before content is tuned to it.
* Good, because the margins are wide at the cost budgets: a world of 200 at its stress case takes 1.67 reference cores, so stage 3 can start with one server process per world (ADR-085), and pillar 1 is safe: 27 battles of 400 fit within a quarter of the integrated budget, and 132 within a quarter of the stage-3 host.
* Bad, because the cost budgets are targets, not measurements: at 4 times the budgets the integrated worst case takes 1.13 reference cores, above item 7, and a world of 200 takes 6.69, above item 8's host. The milestone measurement decides whether the caps fall.
* Bad, because a region at its body budget makes Essence wait, which B1 must show to the steward as a cause (PIL-054).
* Neutral, because full-fidelity settlements dominate the cost (78 % of the stress case at 200), so the budget responds most to B1's population limit and to how many settlements are watched (WLD-244).

### Confirmation

**Confirmed when:** the simulation-core milestone (DEC-229) measures, on the reference test machine, a synthetic battle of 2,000 figures and a region of 4,000 bodies, and every measured cost is within 25 % of its budget in item 2; and the first world-game milestone (PIL-110) runs the integrated stress case of item 7 within its budget. A cost above its budget by more than 25 % lowers the region body budget and the battle size by the ratio of budget to measurement, in a reviewed change, before any content is tuned to them; the tick rate is never lowered to absorb it.

## Pros and Cons of the Options

### A — client-derived budget

* Good, because it protects the skirmish and the integrated host on minimum-specification machines.
* Bad, because alone it says nothing about community hosts or 200 stewards.

### B — server-first budget

* Good, because it sets per-region limits that do not change with world size.
* Bad, because alone it can starve the integrated host's client.

### A and B together, with caps and cost budgets

* Good, because each side has a budget that can fail, and the caps are hard.
* Bad, because two budgets must be measured and kept.

### C — no hard budget

* Bad, because battle outcomes would depend on the host's hardware, which a shared world cannot accept.

## More Information

* Resolves DEC-228 when the owner confirms it at gate B; the region body budget answers WLD-281 and the battle caps WLD-285. The owner questions of gate B carry items 3, 4, 5 and 8.
* The model's figures (`A2ServerBudget.py cpu`, `caps`): per tick, a settlement of 600 at full fidelity 0.72 ms; at abstract fidelity with its AI 0.025 ms; the busiest region (four settlements at full fidelity and two battles) 4.73 ms; a world of 16 at its planning case 13.0 ms and with every steward present 28.3 ms; a world of 200 at its planning case 113.1 ms and at its stress case 167.3 ms.
* Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-086](ADR-086-replication-and-interest-management.md).
