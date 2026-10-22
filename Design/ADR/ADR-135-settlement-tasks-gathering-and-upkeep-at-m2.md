---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T7)
---

# Settlement tasks, gathering and upkeep at M2

## Context and Problem Statement

A settlement's Workers gather, build and stand guard (MVP-031, MVP-033, MVP-036).
- A gatherer finds a free gathering port in its territory, the nearest first (O84). It mans the port for `mvp.gather.port_ticks`, then carries `mvp.gather.load_ru` to the nearest drop-off point. A full store refuses the load, and the gatherer waits.
- The steward sets three shares, gathering, construction and guard duty, and the task search keeps the shares of the Workers that follow no Leader near them. A task with no work releases its share to the others.
- Guard duty mans turret ports, or else gathers Workers near the Wellspring.
- An idle Worker wanders, and the settlement panel shows why it is idle (MVP-037).

M2 task T7 builds these on the reference Worker's task search (R-UNIT-087 to R-UNIT-097), with T5's ports (ADR-131) and T6's territory (ADR-134). The requirements leave open:
- how a Worker's task is held;
- what "approach the shares" computes;
- how far a gatherer walks, and by what path;
- where a load is put down;
- what the idle reasons are, when guard duty always has somewhere to stand.

## Decision Drivers

* A Worker's choice reads only the state, so that every worker thread count gives the same hash (MVP-093).
* The shares hold across a settlement of 20 Workers or 300, so the choice is made Worker by Worker, as each searches.
* A gatherer's walk of 350 wu or more crosses the valley's terrain, so it needs the land route graph (R-SIM-345).
* Every number is a key (MVP-099).

## Considered Options

1. **A task held by each Worker and chosen at its own task search**: the task furthest below its share among the Workers that follow no Leader.
2. **A settlement-level assignment** made each tick, which hands Workers to tasks.
3. **Weighted draws**: a Worker draws its task with the shares as weights.

## Decision Outcome

Chosen option: 1. Option 2 moves Workers the reference's task search would not, and reassigns Workers mid-walk. Option 3 approaches the shares only on average, and a settlement of 20 sees whole Workers off its shares for minutes.

* **The task.** A Worker holds its `WorkerTask`, Gather, Construct, Guard or none, and an `IdleReason`. A gatherer also holds the ticks it has manned its port and the load it carries. The settlement holds its three shares in percent, summing to 100, from `mvp.priority.default_*` at its founding (T8's command sets them).
* **The search.** An idle Worker of a settlement that follows no Leader makes the settlement's search after its orders and its Leader search (R-UNIT-087), before the random-position search.
  - The work of each task:
    - gathering: the nearest free port, by R-BLDG-023's availability, at a resource site in the territory;
    - construction: the nearest free port at the first queued site;
    - guard duty: the nearest free port at a built, active turret of the settlement in its territory. Guard duty always has work: with no turret port, a guard stands within `mvp.guard.gather_radius` (100 wu) of the Wellspring.

    The central resource site is gathered only while its settlement holds it, which needs a claim (WLD-086), so M2 gathers there never.
  - **The shares.** A task with no work releases its share to the tasks with work, in proportion to theirs. The Worker takes the task with work whose deficit, share × Workers − count × the shares with work, is largest and positive, in WorkerTask's order on a tie. The arithmetic is in integers, and the counts are of the settlement's other living Workers that follow no Leader.
  - It walks to the task's port along a route of the land route graph, the route's end the port itself. A guard with no port walks towards a point drawn within the gather radius of the Wellspring (`GuardPoint`), or wanders where it is when it is near already.
* **Gathering.** A manning gatherer counts `mvp.gather.port_ticks` at a resource site's port, then takes `mvp.gather.load_ru` and becomes `Carrying` (a new Worker state, 8). It walks along a route to the nearest drop-off point of its settlement, the Wellspring's door or the entrance of a built, active storehouse. There it deposits its load. A full store refuses the load whole (MVP-030), and the carrier waits there with the reason `StoreFull` until the store has room. Then it is idle with its task, and searches on its next tick. A carrier, like a manning Worker, searches for nothing (table 2.5-A).
* **Construction** is ADR-132's, at the first queued site's ports. When the site is built its Workers find their ports unmanned (ADR-132) and search again with their task.
* **Idle reasons.** A Worker that finds no task gives one reason:
  - with no share on a task with work: the reason of the largest share without work, `QueueEmpty` for construction with an empty queue, otherwise `NoFreePort`;
  - `ShareMet` when every task with work has its share.
  - A carrier at a full store gives `StoreFull`.

  Guard duty always has work, and the shares of the tasks with work cover every Worker. So a reason other than `StoreFull` appears only when guard duty has no share and every task with a share has no work. `ShareMet` cannot arise from the rule as built; it stays for T8's status, should a later rule cap guard duty.
* **The wire.** The replicated task widens to four bits for the state `Carrying` (ADR-111).
* **State.** The Worker's task, idle reason, ticks at its port and load, and the settlement's shares are part of version 9 (ADR-099).

### Consequences

* Good, because each Worker's choice is a function of the state when it searches, and a settlement keeps its shares whole Worker by whole Worker.
* Good, because a gatherer reaches any port in its territory by the route graph, as an order's crowd does.
* Bad, because a Worker keeps its task until its search runs: a manning guard at a turret holds it however the shares change, and a gatherer finishes its trip first.
* Bad, because a reservation turns a second Worker away only within its tick (R-BLDG-025), so a few more Workers than a site's ports may walk to it and be turned away there.

### Confirmation

GameLogicTests (`TaskTests`):
- a gatherer at the Wellspring taking the nearest free port about 350 wu away, beyond the idle search's 100 wu, along a route;
- gatherers manning, carrying whole loads and depositing them;
- a carrier waiting at a full store with `StoreFull` and depositing once the store has room;
- the default shares with an empty queue: about 14 of 20 Workers gathering and 6 on guard, then constructors once a dwelling is placed;
- `QueueEmpty` for construction alone with an empty queue, and `NoFreePort` for the gatherers beyond the ports;
- guards manning a turret's four ports and the rest gathered near the Wellspring;
- constructors building the first queued site to completion;
- a settlement at work hashing alike every tick at 0, 1, 2 and 8 worker threads.

## Pros and Cons of the Options

### 1 — a task chosen at each Worker's search

* Good, because it is the reference's search, extended, and reads only the state.
* Bad, because a share changes the Workers only as they search.

### 2 — a settlement-level assignment

* Good, because the shares are exact on every tick.
* Bad, because it takes Workers off their walks and their ports.

### 3 — weighted draws

* Good, because it is one draw for each search.
* Bad, because a small settlement drifts off its shares.
