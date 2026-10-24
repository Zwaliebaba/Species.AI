---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (owner decision O112)
---

# Gatherers' home ports and their turns

## Context and Problem Statement

[ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md) gives gathering work while a resource site in the territory has a free port, and sends each searching Worker to the nearest one (O84). A gatherer leaves its port when it carries its load, so the port is free for the whole carry. Every Worker on guard duty near the Wellspring searches again every 1.1 to 2.0 s, so one of them takes the port within a tick or two and walks 250 to 450 wu to it. The carrier puts its load down, finds every port taken and becomes a guard, walking back to the Wellspring.

On the owner's M3 build two crossing streams of Workers ran between the Wellspring and the sites, and the Workers did not walk between the mines and the three storehouses beside them. The owner read this as Workers wandering (O112). MVP-031 has a gatherer carry its load to the nearest drop-off point "and return", which the relay never did.

The owner chose that a gatherer keeps its port while it carries and goes back to it (O112, option A). Built and measured as stated, that is worse than the relay:
- until a storehouse stands beside a site, the drop-off is the Wellspring, about 350 wu away;
- a port kept for one gatherer stands empty for that gatherer's whole round trip, about 32 s in every 42 s;
- the relay refills the port within seconds.

## Decision Drivers

* A gatherer walks only between its port and its drop-off point (MVP-031), and guards stay at the Wellspring.
* A port is never empty while a gatherer could stand at it: the economy must not slow down.
* M2's growth criteria hold, in O84's order: a growth step at least every 20 minutes of play and M3's first contact in hour 2 to 3, then the plateau in hours 4 to 6 (the MVP scope's §7 amendment of O84).
* The choice reads only the state, so every worker thread count gives the same hash (MVP-093), and a restored world decides alike.

## Considered Options

1. **The home port kept by one gatherer** (O112's option A as stated). The port's keeper is saved on the port, and no other Worker takes it while its gatherer carries.
2. **The home port shared by its turns.** A port takes as many gatherers as fit one gatherer's round, and a gatherer whose port is manned waits its turn beside it.
3. **A home site with a queue.** A gatherer belongs to a resource site, takes any free port there and waits at the site when none is free.
4. **The relay as it is**, with only the explanation.

## Decision Outcome

Chosen option: 2. It keeps option 1's lanes and fills each port as the relay did.
- **Option 1.** Measured on the growth scene's seeds 1 to 8, the settlement had 40 Workers after the first hour against 60, and 82 after four hours. It never reached its plateau within the 8-hour run, and its largest gap between growth steps was 43.7 minutes.
- **Option 3.** It needs a rule for which waiting gatherer takes which port, and gatherers crowd the site rather than their port.
- **Option 4** leaves the crossing streams that the owner reported.

What is built:

* **The state.** A Worker holds its home port: the structure's id and slot and the port's index (`Figure::homeStructureId`, `homeStructureSlot`, `homePortIndex`).
  - The region writes them for every figure of every team, in team and slot order, as a new last section of its state after M4's deputies: a bit, then the three values when there is a home port.
  - `WORLD_STATE_VERSION` is 23 (`STATE_VERSION_WITH_HOME_PORTS`). A region of version 22 or older reads with no home port, and its gatherers take one as they next gather, so the owner's world continues (O98).
* **The turns** (`PortTurns`). A resource site's port takes ⌈(p + w) ÷ p⌉ gatherers, where:
  - p is `mvp.gather.port_ticks`;
  - w is the ticks of the walk from the site to its settlement's nearest drop-off point and back, at `ref.unit.worker.speed`.

  It is 1 while the settlement has no drop-off point. It is computed when a Worker searches, so a storehouse built beside a site, or turned inactive, changes it from the next search.
* **The choice** (`GatherPort`). A gatherer is a living Worker of a settlement's task that follows no Leader and no order, holds the gathering task and has a home port.
  - A Worker that gathers goes back to its home port while that port's other gatherers are fewer than its turns.
  - Otherwise it takes the port with the fewest gatherers among the ports with room. The nearest to it wins a tie, then slot and port order.
  - Gathering has work while a port has room at a resource site in the territory, or at the central resource site while its settlement holds it. A Worker that takes another task, or none, has no home port.
* **The wait** (`ToPortStep`, R-UNIT-069 amended). A gatherer walking to its home port while another Worker mans it no longer becomes Idle.
  - It walks on, and stands `ref.unit.worker.arrival_slow_radius` (10 wu) from the port until the port is free, then mans it.
  - If two arrive together, the structure's occupant rule keeps one, and the other searches again and waits.
* **The load** (`GatherAtPort`). A gatherer that picks up its load makes the port it stands at its home port. This covers a gatherer of a world saved before version 18.
* **The tuning.** `mvp.gather.port_ticks` is 215, from 100: see the measurement below. [DEC-1012](../decisions/species_design.md#dec-1012-the-settlements-pace-after-the-gatherers-home-ports) puts the trade between the opening and the plateau to the owner.

### The measurement

All figures are means over the growth scene's seeds 1 to 8, each run for `mvp.run.length_hours`. The settlement is run by its scripted steward on the committed meshes and records. The seeds differ by less than 0.1 h. The runs used a Linux build of the game's libraries and the performance tool, g++ 13 at `-O2` with the development checks on, as [ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md)'s tuning was measured.

| Variant | Workers after 1 h | 100 Workers at | Plateau (4 to 6 h) | Largest gap (≤ 20 min) | First halfway line (hour 2 to 3) |
|---|---|---|---|---|---|
| The relay (ADR-135) | 60 | 1.92 h | 4.30 h | 15.5 min | 2.66 h |
| Option 1, port ticks 100 | 40 | 5.17 h | none in 8 h | 43.7 min | 6.74 h |
| Option 2, port ticks 100 | 73 | 1.25 h | 2.56 h | 14.4 min | 1.78 h |
| Option 2, port ticks 150 | 69 | 1.58 h | 3.16 h | 16.2 min | 2.20 h |
| Option 2, port ticks 175 | 62 | 1.75 h | 3.54 h | 17.1 min | 2.46 h |
| Option 2, port ticks 200 | 57 | 2.00 h | 3.99 h | 18.0 min | 2.74 h |
| **Option 2, port ticks 215** | 54 | 2.17 h | 4.28 h | 18.5 min | 2.93 h |
| Option 2, port ticks 230 | 51 | 2.25 h | 4.57 h | 19.1 min | 3.10 h |
| Option 2, escalation 1.25 | 70 | 1.25 h | 3.02 h | 14.4 min | 1.85 h |
| Option 2, escalation 1.40 | 70 | 1.25 h | 3.90 h | 27.8 min | 1.94 h |
| Option 2, orb cost 45 ru | 49 | 1.92 h | 3.57 h | 23.3 min | 2.55 h |

At 100 port ticks the ports are manned far more of the time. On the tests' valley after an hour, 10.0 of 12 inner ports are manned on average, against 5.7 under the relay. The plateau then comes at 2.56 h, and the first halfway line at 1.78 h, before hour 2.

Of the three levers tried, only the time at the port meets all of M2's criteria. 215 is the smallest value tried that holds all three. The step at 0.1 s per tick costs the same as the relay's: 350 to 460 µs at 300 Workers on that build.

### Consequences

* Good, because each gatherer walks only between its port and its drop-off point, and comes back (MVP-031). A storehouse beside a site shortens the round, so fewer gatherers fill the port and more Workers stay free.
* Good, because guards no longer cross the territory to take a port from a carrier.
* Good, because every growth criterion of M2 holds on the scene, with the same cost per step.
* Bad, because the first hour is slower than under the relay at the same priorities: 54 Workers against 60, at 2.15 times as long at the port for each load.
* Bad, because the largest gap between growth steps rises from 15.5 to 18.5 minutes, nearer the 20-minute limit.
* Bad, because M3's valley run (`playtests/M3.md`) and the owner's M2 runs were measured with the relay; their figures stand for the relay until they are run again.
* Bad, because a version raise again changes the state the owner's world holds; it migrates, as every raise since version 10 has (O98).
* It forecloses: a resource port kept for one gatherer alone, and the relay's reassignment of a port while its gatherer carries.

### Confirmation

* `GameLogicTests/TaskTests`:
  - every gatherer walks only to its home port;
  - a carrier goes back to the port it carried from;
  - a gatherer waits its turn beside a manned port, and none is sent away for want of a port;
  - Workers moved to guard duty have no home port;
  - the home ports are saved, and a region of version 22 reads with none;
  - with more Workers than the ports' turns take, the rest have no free port.
* `GameLogicTests/ValleyTests`: the AI settlement's battle AI continues alike from a snapshot taken before its first main cycle that promotes a formation Leader. The crowd's gatherers now leave the Wellspring for their ports, so the later cycle that the test waited for no longer promotes.
* `GameLogicTests/GrowthSceneTests`: the scene's first hour stays inside its bounds.
* `GameSharedTests/TuningTests`: the committed record holds 215 port ticks.
* The owner's run: the Workers walk between the sites and their storehouses, and the opening's pace is judged ([DEC-1012](../decisions/species_design.md#dec-1012-the-settlements-pace-after-the-gatherers-home-ports)).

## Pros and Cons of the Options

### 1 — one keeper per port

* Good, because each port belongs to one Worker, which the client could show.
* Bad, because a port waits through its gatherer's whole round, which halves the opening's gathering and stalls the run.

### 2 — the port shared by its turns

* Good, because the port is manned while the lanes stay fixed.
* Bad, because the turns are an estimate of the round, which slopes and crowds lengthen; rounding up keeps the port full at the price of a short wait.

### 3 — a home site with a queue

* Good, because a waiting gatherer takes whichever of its site's ports frees first.
* Bad, because it needs a rule between the waiting gatherers, and a crowd stands at each site.

### 4 — the relay

* Good, because nothing changes, and M2's tuning stands as measured.
* Bad, because the streams the owner reported stay, and MVP-031's return is never made.

## More Information

* Owner decisions O84, O98, O112; DEC-1012; MVP-031, MVP-036, MVP-037; R-UNIT-069, R-BLDG-023.
* Amends [ADR-131](ADR-131-structure-step-collision-shapes-and-ports-at-m2.md) (a Worker walking to its port) and [ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md) (the gathering search).
* The code: `GameLogic/SettlementTasks`, `GameLogic/FigureAdvance`, `GameLogic/Figure`, `GameLogic/RegionSimulation`, `GameLogic/WorldSnapshot`, `GameData/Records/tuning/mvp.json`.
