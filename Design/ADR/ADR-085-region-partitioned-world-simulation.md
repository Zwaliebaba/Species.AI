---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Region-partitioned world simulation: regions that step the same tick, messages between ticks, battles anchored in one region

## Context and Problem Statement

The world must be partitioned into regions from day one so that regions can later move to separate server processes (vision §5, O1), and a world of 16 stewards must grow to one of 200 or more without a rewrite (O1, PIL-031). Area A1 made each region an island valley of four settlement sites, 5,120 wu on a side, joined to its neighbours only by land-bridge crossings 384 wu wide (WLD-020, WLD-021, WLD-024): every settlement, territory and structure lies inside one region. A1 bounds a battle to at most two regions, at a crossing at most 448 wu into the neighbour, and asks A2 and B4 which region simulates it (WLD-282). Battles always run in full; a settlement nobody watches runs the abstract economy (O3), and a watched one runs in full (PIL-057).

How do regions share one world, and what must hold at 16 stewards so that 200 needs no rewrite?

## Decision Drivers

* The same rules whether all regions run in one process or each in its own (O1).
* A region's state depends only on its own inputs, so it steps on any core, is cut and restored with the others and moves alone (ADR-081, ADR-083).
* Battles never split across a seam (O3, WLD-282).
* The cost of a world grows with its active load, not with its size (ADR-080).
* Fidelity switches that a steward cannot use to make a rival's settlement flip back and forth (PIL-057).

## Considered Options

* A — region owners that step the same tick in parallel, with every cross-region effect a message delivered at the next tick boundary, and battles anchored in one region with a lent battlefield
* B — one world owner for everything, regions as spatial indexes only
* C — overlapping region margins simulated twice and reconciled

## Decision Outcome

Proposed option: "A". **Default while Proposed:** items 1 to 8 below; item 9 is a proposal that is not in force until the owner decides it.

1. **Owners.** Each region of a world has one region owner (ADR-083) that holds everything inside the region's square: its four sites and their settlements, every figure, structure, Essence orb and effect there, its border areas, its resource sites and its battles. One world owner holds the world-level state: the world tick, the public world seed and the simulation key (ADR-082), Cladans, grievances and relations, the claims register, declared attacks and the battle slots of WLD-285, the keeper, the stewards, presence and the world record.
2. **One tick, then messages.** Every owner steps world tick *t*. An owner affects another only by a message produced during tick *t* and delivered at the start of tick *t* + 1, in the order of the sending owner's id, then the sender's sequence; a region step reads no other owner's state. The messages are: an object crossing a region border (its full record, by stable id), the lending and return of a battlefield (item 4), the border band of item 5, events for the world owner (contacts, battle results, attack arrivals and grievance causes; WLD-066, PIL-046) and the world owner's orders to regions (a declared attack's warning and gate, a respite, a keeper's placement, a dormancy). Every cross-region effect therefore takes exactly one tick, 100 ms, in one process and across processes alike. The messages in flight at a tick boundary are part of every consistent cut (ADR-083 item 9).
3. **Ownership by position.** An object belongs to the region whose square holds its position at the end of a tick, unless it is inside the battlefield of a battle, which owns it until it leaves that battlefield or the battle ends. A figure that crosses the region border on a crossing is handed over at the tick boundary; a crossing is the only land link between two regions (WLD-002), so hand-overs occur only on bridges. A hand-over that would take the receiving region's visitors above its visiting share (ADR-080 item 3) is not sent: the expedition waits at the border on its own side.
4. **Battles at crossings (WLD-282).** A battle runs in its anchor region: for a battle at a settlement, the defender's region; for a battle in a crossing border area, the region of that border area (each border area belongs to one site and that site's region, WLD-062); for a battle that a world event starts, the region of the territory it enters. Its battlefield, which B4 bounds at a crossing to the land between the two mouths, reaches at most 448 wu into the neighbouring region; that part is **lent** to the anchor region from the tick after the battle starts until the tick after it ends: the neighbour steps nothing inside it and hands over every object of its own that enters it. The battle counts against its anchor region's cap (WLD-285) and holds at most the side cap of ADR-080 item 4 per side.
5. **Border band.** At the end of each tick, each region sends its neighbour the stable ids, sides and positions of its figures on a bridge within 64 wu of their shared border. A battle trigger across a border (two hostile expeditions meeting on a bridge) is detected by the anchor region from that band, one tick late, and the battle starts at the next tick. 64 wu is more than the 4 wu a figure moves in a tick at twice the Worker's speed, so no contact passes unseen.
6. **Fidelity inside the region.** Each settlement's economy runs at full or at abstract fidelity inside its region owner (O3, PIL-057), and switches at the tick boundary that ends an abstract step (ADR-079), through the one settlement record of DEC-251 item 10. A settlement that becomes watched switches to full fidelity at the next such boundary; one that stops being watched stays at full fidelity for at least 60 s of world time after the last settlement view leaves it, so a view that sweeps back and forth switches a settlement at most once a minute. A switch to full fidelity materialises the settlement's figures at about 2 µs per figure (0.56 ms per tick at 200 stewards when every present steward's view moves; ADR-080). B2 and D2 specify the switch, the calibration and its accuracy.
7. **Per-client updates are assembled from per-region parts.** Each region owner encodes, after its step, its share of the update of every client whose interest set reaches into it (ADR-086); the client's update for the tick is the concatenation of these parts with the world owner's. A client's interest never depends on how regions are distributed.
8. **Stage 3.** A world of 50 to 200 or more stewards runs every region in one server process on the stage-3 host of ADR-080 item 8 (1.85 reference cores for 200 at the stress case; 22.7 ms per tick on 16 vCPU, ×2.22 the cost budgets at the 50 ms wall budget). Beyond that margin, or for worlds larger than 200, it distributes its region owners over region host processes, with the world owner and the client connections in a front process; the message semantics of item 2 are unchanged, and a region moves between hosts by its state in a consistent cut and its hash at a tick boundary, during which the world's tick waits at most 1 s, inside the catch-up allowance of ADR-083. The protocol between processes, the region-to-host assignment and the front process are a later record of stage 3.
9. **Overload lever (proposed; not in force until the owner amends PIL-057 and WLD-244 at gate B).** When the tick's wall time has exceeded its 50 ms budget (ADR-080 item 6) for 100 consecutive ticks (10 s), the server moves settlements that are watched only as a neighbour (no present steward's own settlement, and no side of a battle) to abstract fidelity, least recently viewed first, one per abstract step, until the wall time is below 80 % of the budget; their clients draw them with a stand-in presentation (the settlement's structures and an animated crowd that the abstract state drives, C3 and the renderer specify it) and say so. A steward's own settlement and every battle never move. When the wall time has stayed below 80 % for 60 s, they return to full fidelity in the reverse order. At the 200 stress case this lever recovers up to 101.0 ms of the 185.5 ms per tick (54 %); its cost is the stand-in presentation and the abstract model's accuracy for watched neighbours (B2).

### Consequences

* Good, because the semantics are the same for 1 process and for many, so the scale path to 200 and beyond changes the deployment, not the rules.
* Good, because a region's step, its part of a cut, its hash and its hand-over involve only its owner and its messages.
* Good, because A1's island valleys keep seams to bridges only, and battles never span a seam.
* Good, because the 60 s hold bounds fidelity switches to one a minute per settlement, so a view swept across a rival's settlement costs at most one switch and the gap of PIL-057.
* Bad, because every cross-region effect waits one tick, and an object that crosses a border cannot act on the far side in the tick it crosses.
* Bad, because the lent battlefield and the border band are two mechanisms that D2 and B4 must implement and test at the first world-game milestone, when a world of 16 has crossings.
* Bad, if the owner approves item 9, because a watched neighbour can show a stand-in instead of its Workers during an overload.
* Neutral, because a world of 16 stewards has 8 regions and runs them on the integrated host's worker threads; nothing in this record waits for stage 3.

### Confirmation

**Confirmed when:** the first world-game milestone (PIL-110) shows, on a world of 16 stewards, that stepping its regions on 1, 2 and 8 threads gives identical per-tick hashes; that a battle at a crossing runs whole in its anchor region with figures of both regions; that a figure crossing a bridge is handed over with its hash checked; that a world cut, restored and stepped gives the hashes of the world that was never stopped; and that a view swept across a settlement every 5 s for 10 minutes switches it at most once a minute.

## Pros and Cons of the Options

### A — region owners, messages, anchored battles

* Good, because it gives a region-local state and one-tick semantics everywhere.
* Bad, because of the one-tick delay at every seam and the two mechanisms of items 4 and 5.

### B — one world owner

* Good, because it has no seams.
* Bad, because it cannot step regions in parallel or move them between processes without a rewrite, against O1.

### C — overlapping margins

* Good, because it hides seams.
* Bad, because two owners simulating one area must agree bit for bit and reconcile, which costs more than the bridges it serves.

## More Information

* Answers WLD-281 (with ADR-080) and WLD-282; part of DEC-223's resolution (ADR-083). Item 9 is owner question 7 of the solution strategy (§4.5). Related: [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-086](ADR-086-replication-and-interest-management.md).
* The figures are `Tools/DesignModels/A2ServerBudget.py report` (`overload_lever_ms`, `switch_ms`).
* **Amendment 2026-10-18 (challenge round 1).** Item 6 adds the 60 s hold and the switch cost (A2-F07); item 3 adds the visiting share (A2-F06); item 8 uses the revised host and margin (A2-F01); item 9 proposes the overload lever as an owner question (A2-MB-2); a region is restored with the world's consistent cut (A2-F04).
