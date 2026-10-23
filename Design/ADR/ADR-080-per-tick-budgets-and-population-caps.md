---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead; proposed by design-phase area A2; items 3 to 5, 7 and 8 decided by the owner at gate B (O43, O46, O48)
---

# Per-tick budgets and population caps: per-figure cost budgets, reserved region bodies, per-side battle caps, a tick wall budget and named hosts

## Context and Problem Statement

The reference has no per-tick CPU budget and no engine population limit (DEC-228); its design population is 2,000 units per map and its stress population 4,000 (DBG-204), with a client guidance of 4.2 ms per tick at the design population on the reference test machine (DBG-205, DEC-971), which DBG-205 lets the design phase read as the cost of a whole tick or as the share of one frame. Species runs a server that simulates every battle in full whether anyone watches it (O3), a world of up to 16 stewards on the host's own machine in stage 1 (O10, O40) and up to 200 on hosted servers in stage 3 (O1, O5). Area A1 hands over the load: 26 settlements in 8 regions at 16 stewards and 256 in 77 at 200; up to 16 and 177 settlements watched at the evening peak; 4 battles in progress at 16 (6 at the session pace) and 16 at 200 in the planning case, and up to 8 (12) and 28 in the stress case (WLD-280, WLD-284); and it asks for the region body budget (WLD-281) and the battle caps (WLD-285). Pillar 1 needs battles of 400 figures and is at risk below 200 (PIL-021); pillar 6 rules out an unassailable settlement (O31, PIL-015).

What does a tick cost, what limits keep it within budget, and on what hardware?

## Decision Drivers

* A budget that can fail a test: per system, on a named machine, at a named population (DBG-205).
* The integrated host runs the server beside its own client at 60 frames per second (RND-454, DEC-228 item 6).
* A margin that survives the measurement of costs that are only targets today.
* Caps that neither a rival nor a defender can turn into a weapon (O2, O31).
* Battles are the last thing to shrink (PIL-021).

## Considered Options

* A — a client-derived budget: at most 25 % of a 16.7 ms frame per tick at 2,000 units on minimum specification, with the server budget per region derived from it
* B — a server-first budget: fixed milliseconds per region step on a stated host, with the client budget derived from it
* A and B together, with hard engine caps, per-figure cost budgets and a tick wall budget on named hosts
* C — no hard budget; population limited dynamically by measured cost

## Decision Outcome

Proposed option: "A and B together, with hard engine caps, per-figure cost budgets and a tick wall budget on named hosts". **Default while Proposed:** the decision below; its figures come from `Tools/DesignModels/A2ServerBudget.py` and are computed, not measured. The owner confirmed at gate B the population limit and the side cap as provisional values with ranges that the first measurement fixes (O43), the battle caps of item 5 (O46) and the hosts of items 7 and 8 (O48).

1. **Reference core.** Every CPU budget is stated in time on one core of the reference test machine (DEC-971). A vCPU of a rented host counts as 0.6 reference cores until measured, and every host figure is also given at 0.4 and 1.0.
2. **Cost budgets per object per tick**, which the implementation meets and the simulation-core milestone measures first:

   | Object | Budget, µs per tick | Basis |
   |---|---|---|
   | figure in a battle (search, combat movement, laser bolts, Essence orbs) | 2.1 | DBG-205 read as the cost of a whole tick: 4.2 ms per tick at 2,000 units. Read as one frame's share of a tick of six frames, it is 12.6 µs, 6 times more; the model shows that multiple |
   | Worker or Leader at settlement work (gathering, carrying, construction, guard duty) | 1.2 | the same search stagger without combat |
   | figure of an expedition outside a battle | 0.4 | route and formation following only |
   | a body's stamp of its settlement's seen layer (D9) | 0.5 per update | every tick in a battle, every 5 ticks otherwise |
   | a keyed simulation draw (ADR-082) | 0.03 | SipHash-2-4 over 40 bytes; 3 per battle figure, 0.1 per Worker at work, 3 per Essence orb, per tick |
   | settlement at abstract fidelity | 5 | one abstract step of 50 µs every 10 ticks (ADR-079) |
   | AI settlement or deputy | 60 | measured on the reference test machine (O74); first one 1 ms decision pass every 50 ticks, 20, and the model shows 100 and 200 |
   | command of one side of a battle | 30 | |
   | a figure materialised when its settlement turns full fidelity | 2 | at most once per settlement in 60 s (ADR-085 item 6) |
   | region fixed cost (grids, messages) | 30 | |

3. **Reserved region bodies.** A settlement's population limit L, which B1 sets, is at most 600 Workers and Leaders, a provisional value in the range 450 to 700 that the first measurement fixes (O43). Each of a region's four sites reserves L inside the region's body budget, so a settlement's growth is bounded only by its own limit, never by visitors. Figures of settlements of other regions (visitors) use only the rest, two battles' visiting sides: the region body budget is 4L + 2S = 3,600 bodies at L = S = 600, of which the settlements take 67 % (WLD-281's 75 % holds). An expedition that would take a region's visitors above 2S waits at the region border on its own side, inside its own region. The **region step budget** is 12.5 ms on one reference core; the bound of a region under the caps (two battles at the side cap at two of its settlements, the other two settlements at work) is 8.05 ms, a headroom of 1.55 times, so a measured cost 25 % above the budgets still fits (10.06 ms). The bound allows L up to 700 at that tolerance; 600 keeps the margin.
4. **Battle size per side.** At most S = 600 figures of one side are inside a battlefield (every figure of the side within the bounds; B4 specifies the counting), a provisional value in the range 200 to 600 (O43), so a battle has at most 1,200 figures. With S equal to L, a settlement at its population limit can always be met by an equal force; the defender's advantage is B6's balance, never a cap. A figure that would exceed its side's cap waits at the battlefield's edge, inside its own region's visitor share. A battle of 400 costs 1.15 ms per tick, one of 1,200 3.32 ms, one of the floor's 200 0.60 ms.
5. **Battle caps** (WLD-285, O46): at most 2 battles in progress per region, and at most max(8, ⌈0.1 × the settlements at full occupancy⌉) per world: 8 at 16 stewards and 26 at 200. A1 recommended a world minimum of 4, and the owner chose 8 at gate B, which amends WLD-285; at 8 the share of the evening peak at the cap falls from 4 % to 0 % in the planning case and from 23 % to 3 % in the stress case at the always-on pace, and from 11 % to 0 % and from 37 % to 8 % at the session pace, for up to 4.6 ms more per tick.
6. **Tick wall budget.** The whole of a tick (every region step as a single-threaded job, scheduled longest first over the host's worker threads; the world owner's step after them; replication jobs; the copy of a consistent cut, ADR-083 item 9) finishes within 50 ms, half the tick period, on the host's hardware; 100 ms is the deadline beyond which owed ticks accumulate (ADR-083 item 7).
7. **Integrated server budget** (O48). The integrated server uses at most 1 reference core averaged over 10 s, on its tick thread and at most 2 worker threads, on the reference test machine, so the client keeps RND-454's 60 frames per second. Its worst case for 16 stewards (every steward present, every settlement watched, 8 battles) takes 0.32 reference cores and 12.0 ms of wall time per tick.
8. **Hosts, by a margin rule** (O48). A recommended host holds its stage's stress case within the wall budget at twice the cost budgets and within the deadline at four times:

   | Host | Wall ms at the budgets | Multiple at the 50 ms budget | At the deadline | At 0.4 reference cores per vCPU |
   |---|---|---|---|---|
   | stage 3, 200 stewards: 16 vCPU, 4 GB, 50 Mbit/s up (recommended) | 22.7 | 2.22 | 4.46 | 1.48 |
   | stage 3: 8 vCPU (not recommended) | 49.2 | 1.02 | 2.04 | 0.68 |
   | stage 2, 16 stewards all present: 4 vCPU, 1 GB, 10 Mbit/s up (recommended) | 20.1 | 2.51 | 5.05 | 1.67 |
   | stage 2: 2 vCPU (the minimum) | 28.7 | 1.75 | 3.52 | 1.17 |
   | integrated: the reference test machine, tick thread and 2 workers | 12.0 | 4.20 | 8.42 | — |

9. **Caps are hard and never drop a body.** A settlement at its population limit grows no further (B1); visitors beyond the visitor share wait in their own region; a side at its cap waits at the battlefield's edge; an attack beyond a battle cap waits at its gate (WLD-285); a client over its bandwidth cap is sent fewer full-rate figures (ADR-086). No cap removes, teleports or kills a figure, and every cap is a tuning value of the component that enforces it (D2, D3).
10. **Stress populations.** A region stress test runs the region bound (two battles of 1,200 at two of its settlements, two settlements of 600 at work); the world stress tests are A1's stress cases on the hosts of item 8 (26 battles in progress at 200, 8 at 16) and the integrated case of item 7; the client keeps the 4,000-unit stress map of DBG-204 and RND-456.

### Consequences

* Good, because every budget can fail a test on a named machine, every cap has a number before content is tuned to it, and every recommended host has a stated margin.
* Good, because neither a rival's waiting bodies nor a defender's size can use a cap as a weapon: growth is reserved, and the battle cap is per side.
* Good, because pillar 1 is safe: 21 battles of 400 fit within a quarter of the integrated budget, and 209 within a quarter of the 16-vCPU host.
* Bad, because the cost budgets are targets, not measurements: at 4 times the budgets the integrated worst case takes 1.28 reference cores, above item 7, and a world of 200 needs 89.8 ms of wall time on 16 vCPU; at 6 times (the per-frame reading of DBG-205) the region step holds only settlements of 150.
* Bad, because the stage-3 host is 16 vCPU, twice the first estimate, which the margin rule requires.
* Neutral, because full-fidelity settlements dominate the cost (77 % of the stress case at 200), so the budget responds most to B1's population limit and to how many settlements are watched (WLD-244).

### Confirmation

**Confirmed when** (amended 2026-10-19 for the MVP's milestones, O43, O51 and O55): the first task of the simulation-core milestone, the MVP's M1 (DEC-229), before any content is tuned to these values, measures with M1's simulation on the reference test machine the battle of 400 figures of M1's exit and, in the test runner, a synthetic battle of 2,000 figures and a region at its bound, and every measured cost is within 25 % of its budget in item 2, or the items whose values O43 left provisional are amended with the measured figures within their ranges; and the first milestone after the MVP that hosts 16 stewards (PIL-110) runs the integrated stress case of item 7 within its budget. A measured cost above that tolerance is answered by these levers, in this order: (1) split the region step's read-only hot phases into parallel jobs (ADR-083 item 6); (2) a larger host by item 8's margin rule; (3) the full-fidelity settlement cost: B1's population limit by the model's sensitivity table (at 2 times the budgets, 450 with the side cap equal to it), and the overload lever of ADR-085 item 9 (O47); (4) the battle side cap last, never below 200 per side (a battle of 400, PIL-021's typical) without an owner decision. The tick rate is never lowered to absorb a cost.

## Pros and Cons of the Options

### A — client-derived budget

* Good, because it protects the skirmish and the integrated host on minimum-specification machines.
* Bad, because alone it says nothing about community hosts or 200 stewards.

### B — server-first budget

* Good, because it sets per-region limits that do not change with world size.
* Bad, because alone it can starve the integrated host's client.

### A and B together, with caps, cost budgets and a wall budget

* Good, because each side has a budget that can fail, the caps are hard, and the hosts are sized by the tick's wall time rather than by its average.
* Bad, because two budgets, a wall model and a margin rule must be measured and kept.

### C — no hard budget

* Bad, because battle outcomes would depend on the host's hardware, which a shared world cannot accept.

## More Information

* Resolves DEC-228 (Resolved 2026-10-19); the region body budget answers WLD-281 and the battle caps WLD-285. Gate B confirmed items 3 and 4 as provisional values with ranges (population limit 450 to 700, side cap 200 to 600) that the first measurement fixes (O43), item 5 (O46) and items 7 and 8 (O48).
* The model's figures (`A2ServerBudget.py cpu`, `wall`, `caps`): per tick, a settlement of 600 at full fidelity 0.80 ms; at abstract fidelity with its AI 0.025 ms; the busiest region (four settlements at full fidelity and two battles of 400) 5.53 ms; a world of 16 at its planning case 14.7 ms and with every steward present 32.0 ms; a world of 200 at its planning case 125.2 ms and at its stress case 185.5 ms.
* Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-086](ADR-086-replication-and-interest-management.md).
* **Amendment 2026-10-18 (challenge round 1).** The tick's wall time and the margin rule replace the average-core sizing (A2-F01): the stage-3 host is 16 vCPU and the stage-2 host 4 vCPU; the seen layer, keyed draws and fidelity switches are counted (A2-F07); the region body budget reserves each site's population limit and the battle cap is per side (A2-F06), which lowers the body budget from 4,000 to 3,600 and the most a settlement may hold from 750 to 600; the remedy order puts battles last (A2-F02).
* **Amendment 2026-10-19 (owner gate B, O43, O46, O47, O48).** Items 3 and 4 state L and S as provisional values with their ranges (O43); item 5 records the owner's world minimum of 8 battles, which amends WLD-285 (O46); items 7 and 8 record the host sizes (O48); remedy (3) of the confirmation names the overload lever as approved (O47). The "Confirmed when" is re-pointed to the MVP's M1 for the cost measurement, which O43 makes the first task before content is tuned, and to the first milestone after the MVP with 16 stewards for the integrated stress case (O51, O55); the MVP's own population limit of 300 (`mvp.settlement.population_max`, MVP-022) lies below this record's L, and M1's measurement may raise it.
* **Amendment 2026-10-20 (owner decision O69).** The reference test machine of the confirmation measures the `Release|ARM64` development build natively ([ADR-094](ADR-094-arm64-development-build.md)), with the owner's performance script in place of a CI job (DBG-208 and DBG-209 amendments); its figures bind the MVP's budgets, and the minimum-specification tier measures the x64 build before the first external build. M1's region at its bound has wandering Workers standing in for the two settlements at work, whose cost M2 measures again ([M1 plan](../implementation/m1_plan.md) T10).
* **Amendment 2026-10-20 (M1 task T10, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md)).** The instruments of the confirmation exist: the performance tool's scenes (the sandbox battle, the synthetic battle, the region bound with wandering Workers at work until M2, an empty region, the draws and the travel scene) and `Build\RunPerformance.py`, which records each figure of items 2, 3 and 7 against its budget in `playtests/M1.md`. A settlement at abstract fidelity, a materialised figure and a seen-layer stamp are not built at M1. The figures that confirm or amend this record come from the owner's first run on the reference test machine; ADR-110 lists indicative figures from another machine, which bind nothing.
* **Amendment 2026-10-20 (the owner's first run, M1 task T10).** The run of 2026-10-20 on the reference test machine pins it and measures items 2, 3 and 7 (`playtests/M1.md`, build 98ea6b70fa9e, `Release|ARM64`).
  - Every figure but one is within its budget. A figure in a battle costs 0.20 µs in the sandbox's battle and 0.33 µs in the synthetic one (2.1), a Worker at work 0.21 µs (1.2), a figure of an expedition 0.10 µs (0.4), a keyed draw 0.022 µs (0.03), and the command of one side 0.37 µs and 3.5 µs (30). The region's fixed cost is 8.3 µs (30), the region step at its bound 1.23 ms at the 95th percentile (12.5 ms, item 3), and the integrated server 0.0015 reference cores (1, item 7).
  - So the population limit L and the side cap S stay at 600, and `mvp.settlement.population_max` at 300. The measurement gives no lever a reason to move, and a value raised within O43's ranges is the owner's choice, not the measurement's. M2 measures the Worker at work again, with settlements in place of the wandering stand-in.
  - An AI settlement or deputy costs 23.8 µs per tick in the sandbox's battle, within 25 % of its 20 µs, and 52.9 µs in the synthetic battle, 2.6 times it, with 1,000 figures a side, above the side cap. None of the confirmation's levers lowers an AI's own cost, and the budget is not one of O43's provisional values, so the figure went to the owner (M1 plan T10). The owner accepts it (O74), and item 2's AI budget is 60 µs, the measured figure rounded up. The model's 200-steward stress case takes 185.5 ms of CPU per tick with the AI at 20 µs and 206.0 ms at 100 µs (`A2ServerBudget.py cpu`). The region bound runs no AI: four AIs at the synthetic figure would add 0.21 ms to its 1.23 ms.
  - So the first part of the confirmation holds. The record stays Proposed until the integrated stress case of item 7 has run.
* **Amendment 2026-10-22 (owner decision O100).** The reference test machine of the confirmation is the owner's x64 laptop, measuring `Release|x64`, in place of the ARM64 PC of the O69 amendment. A change of the machine re-baselines every result, so the figures above, from the ARM64 PC, stand as that machine's and the laptop's first run measures items 2, 3 and 7 again; the budgets, the population limit, the side cap and `mvp.settlement.population_max` stay unless that run moves them by the confirmation's levers.
