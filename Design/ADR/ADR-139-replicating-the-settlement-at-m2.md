---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T11)
---

# Replicating the settlement at M2

## Context and Problem Statement

M2 task T11 puts the settlement on the wire, for what phase B draws and shows:
- its structures, with their construction progress and inactive state (MVP-033 to MVP-035);
- its stores, its population and limit, and its territory (MVP-022, MVP-026, MVP-030);
- the resource sites' manned ports (R-BLDG-017), the Workers' carried loads (MVP-031) and the orbs;
- the guaranteed event kinds on the reliable channel for MVP-102's notifications (ADR-084 item 5);
- T6's seen layer, for the fog (O81, the M2 plan §9 question 4).

The owner checks it by running a settlement world: the window's title shows the stores and the population from the replica.

These decisions are open:
- which objects a settlement adds to ADR-111's state, and which of their fields only the commander's client sees;
- how a territory and a seen layer, which are rasters of cells, travel;
- how a notice travels;
- how the owner starts a world with a settlement before T18's front end starts worlds.

## Decision Drivers

* ADR-111's codec and sender stay as they are: per-object acknowledged baselines, each datagram decoding alone.
* GameClient includes no GameLogic header (MVP-090), so the client cannot compute a territory from its radius: `ComputeTerritory` reads the region's terrain bounds and its other sites.
* A seen layer goes to its commander alone (MVP-097), and the settlement's detailed status stays a server read (MVP-098, ADR-136).
* A notice of MVP-102 must arrive, where a presentation event may be lost with its datagram.
* The bytes of a settlement at its plateau stay within ADR-086's caps.

## Considered Options

For the territory and the seen layer:
1. **Chunks of 8 by 8 cells** as replicated objects of 64 bits, each sent while it holds a cell.
2. **The territory's radius alone**, with the client computing the raster.
3. **A whole raster** as one reliable message on each change.

For the notices:
4. **A reliable message for each notice**, made from the region's events.
5. **The droppable `StructureCompleted` event** of ADR-132.

## Decision Outcome

Chosen options: 1 and 4.
- Option 2 needs the territory function in the client, which MVP-090's boundary forbids, and no radius gives a seen layer.
- Option 3 sends a territory of 320 by 320 cells, 12.8 KB, on every change, and each new Worker grows the radius. It would also need an order of its own against the state datagrams.
- Option 5 is lost with its datagram.

* **Three classes of replicated object** join ADR-111's figure, effect and orb. `StateUpdate::VERSION` is 3, and the class takes 3 bits.
  - **A structure** (`ReplicatedStructure`), for every structure of the region's pool, resource sites included:
    - sent whole only: its kind, its owning team or none (a resource site, WLD-084), its position and its front's yaw;
    - built and active, as two flags;
    - a construction site's progress in thousandths of the work it needs, as the status counts it (ADR-136);
    - a mask of its manned ports, port i as bit i, for its first 16 ports;
    - a turret's team or none and its health;
    - its aim: the head's yaw, and the barrels' up vector's tilt from vertical towards the head's front, signed in 10 bits of the yaw's steps, from which the client rebuilds that vector.
    A delta carries the flags, the progress, the ports, the turret's team and health, and the aim, each when it changed.
  - **A settlement** (`ReplicatedSettlement`): its local site, team and Cladan's colour; its stores and their capacity in 40 bits of thousandths of a resource unit; its population and limit; and its territory's radius in 1/16 wu in 20 bits. A delta carries the stores with the capacity, the population with the limit, and the radius.
  - **A chunk of cells** (`ReplicatedCells`): 8 by 8 cells of a settlement's territory or seen layer, over the region's square from its corner of least x and z. It holds its layer, its site, its place in chunks on each axis in 8 bits, and its cells in 64 bits, cell (x, z) as bit z × 8 + x. A chunk that holds no cell is not sent, so it is removed when its last cell goes.
  - **A Worker's load** is a sixth figure flag, set while it carries one (`Figure::carriedMilliRu`).
* **Their ids.** A settlement and a chunk are no simulated object, so they take ids under an owner id that no simulation owner takes, `REPLICATION_OWNER_ID` (0xFFFF). A settlement's id is its site's; a chunk's holds its layer, site and place. Both sort after every simulated object, and each keeps its id for its life.
* **What the server collects** (`CollectReplicatedObjects`): after the figures, effects and orbs, every structure, every settlement, and the chunks of each settlement's territory and seen layer that hold a cell. It reads only the chunks within the bounds of the cells a raster has held, which `CellRaster` keeps as it holds them.
* **What a client sees** (`ViewForSteward`): every structure, settlement and territory chunk; a seen-layer chunk only of a settlement its steward commands (MVP-097); and another settlement's stores, capacity, population and limit at 0. Until M3's seen layer narrows the interest set, a client still receives the whole region (ADR-111).
* **The notices** (`SettlementNotice`, a new message on the reliable channel; ADR-084 item 5). Each holds:
  - its tick and kind, `StructureCompleted`, the one kind of M2 (M3 adds contact, claims, the warning and battles);
  - the team;
  - its subject's stable id, structure kind and position.
  The world server makes one from each `StructureCompleted` event of a team the steward commands. It sends them after the tick's command outcomes, in the order the region raised them; one that the backlog refuses waits, with those after it, for the next tick. The droppable event still draws the completion flash.
* **The client** keeps the notices until taken (`ClientSession::TakeNotices`) and finds its own settlement in the replica's latest view, the one whose team it commands (`OwnSettlement`). The title shows that settlement's stores and capacity in whole resource units and its Workers and limit in place of the sandbox's sides: `Species — seed 1 — tick 512 — stores 120 of 1500 — Workers 20 of 40`.
* **A settlement world before T18.** A Debug build's `--world`, in both programs, starts a new world with the steward's settlement at `mvp.sandbox.steward_site` in place of the sandbox, and `--seed` still names its seed. The integrated server's description takes the steward's site. T18's front end replaces the option's use, as it replaces the sandbox's.
* **Markers.** The collection stays under `net.replication.collect` (ADR-111).

### Consequences

* Good, because the client draws and shows the settlement from the server's own state: the same structures, the same rasters cell by cell, the same stores, and no second copy of a rule.
* Good, because a raster costs bytes only where it changes: a new Worker grows the territory by a few chunks, and a Worker that walks within it changes no chunk of the seen layer.
* Good, because the HUD's notifications cannot be lost, and the flash stays droppable.
* Bad, because a chunk of a full interior goes as 64 bits whole in the join: the plateau's 364 chunks take 4,554 bytes of its join of 10,660, counted by the codec's own bits.
* Bad, because the server reads every structure's state each tick to compare it with the client's, as it does each figure's; at a plateau that is 59 structures beside 300 figures.
* It forecloses: a client that computes its territory from the radius; a whole raster on the reliable channel; notices that are only presentation events.

**The measurement in this task.** These are bytes on the wire, which do not depend on the machine. They were measured on the local harness (g++ 13 at -O2, the authored tuning record, the committed meshes cooked) for the growth scene's seed 1 under its scripted steward (ADR-138). The world was replicated to its commander's client, which acknowledges every datagram at the next tick, for 600 ticks. The figures count the state datagrams; the presentation events are apart.

| World | Objects | Join | Then a tick: mean, median, 95th percentile, most |
|---|---|---|---|
| at work, 1 hour: 55 Workers | 226: 60 figures, 47 structures, 1 settlement, 118 chunks | 3,455 bytes in 3 datagrams | 340, 341, 364, 889 bytes |
| at its plateau, 4.5 hours: 300 Workers | 724: 300 figures, 59 structures, 1 settlement, 364 chunks | 10,660 bytes in 9 datagrams | 1,593, 1,591, 1,635, 1,678 bytes |

The plateau's 15.9 KB/s is within ADR-086's 64 KB/s skirmish cap, which the in-process connection does not apply.

### Confirmation

* `GameSharedTests`:
  - each new class round-trips whole with every field at its bound, and as a delta of each field;
  - a Worker's load is a flag;
  - the replication-only ids sort after every simulated id;
  - a notice round-trips at its bound, and one with no subject is malformed.
* `GameLogicTests`:
  - **ADR-084's test with a settlement at work.** A world with the steward's settlement and an attached client, which places a dwelling once its replica shows the stores to pay for it, gives the headless replay's hash on every tick. Its replica equals the server's state after every tick. It receives one notice of the dwelling's completion and sees a Worker's load, the population, the dwelling's limit, its territory and its seen layer.
  - **A commander's fields.** The chunks of a raster hold each of its cells once, and the seen layer and the stores and population go to the commander's client alone.
* `GameClientTests`: the title shows the client's own settlement and not another's, and a notice is taken once. `NeuronCoreTests`: `--world` takes no value and exists in Debug builds only.
* The owner's run on `Debug|ARM64` with `--world --seed 1` (the M2 plan's T11):
  - the title shows the stores and the Workers from the replica;
  - the stores rise as the gatherers come back, and the Workers rise by one every 12 s once the stores hold an orb's cost;
  - after two minutes, the log line of the session's close reports no record without its baseline;
  - without `--world` the sandbox runs as before;
  - the program closes cleanly.

## Pros and Cons of the Options

### 1 — chunks of cells

* Good, because the client holds the server's raster exactly, and only the chunks that change travel.
* Bad, because the sender keeps an acknowledged state for each chunk, as it does for each object.

### 2 — the radius alone

* Good, because it is 20 bits.
* Bad, because the client would need GameLogic's territory function and the region's data, and a seen layer has no radius.

### 3 — a whole raster

* Good, because it is one message per change.
* Bad, because a change of one cell sends all of them, and the message would need an order of its own against the state.

### 4 — a reliable message for each notice

* Good, because a notice arrives once, in order, and its message decodes alone.
* Bad, because a notice the backlog refuses delays those after it.

### 5 — the droppable event

* Good, because it exists.
* Bad, because a lost datagram loses the notification.

## More Information

* Related: [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-132](ADR-132-placement-and-construction-at-m2.md), [ADR-133](ADR-133-the-gun-turret-at-m2.md), [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md), [ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md), [ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md).
* The rules: MVP-016, MVP-022, MVP-026, MVP-030, MVP-031, MVP-033 to MVP-035, MVP-090, MVP-097, MVP-102, R-BLDG-017, R-BLDG-408, R-BLDG-416, R-BLDG-422; owner decisions O81 and O88.
* The code: GameShared (`ReplicatedState`, `StateUpdate`, `SettlementMessages`), GameLogic (`StateSender`, `Territory`, `WorldServer`, `IntegratedServer`), GameClient (`ClientSession`), NeuronCore (`StartupOptions`) and the two programs.
* ADR-111 is amended with this record.
* **Amendment 2026-10-22 (M2 task T12, [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md)).** A Debug build's `--watch` with `--world` gives the steward's settlement to the scripted steward of ADR-138 from the world's start, and the steward commands nothing; the journal's start keeps the watched flag for a world as for the sandbox.
* **Amendment 2026-10-22 (M3 task T3, [ADR-158](ADR-158-the-seen-layer-and-its-readers-at-m3.md)).** Another settlement's structures go to a client only where its settlement's seen layer holds them. The Wellsprings and the resource sites are WLD-242's public layer and go everywhere, without their manned ports where the client does not see them.
* **Amendment 2026-10-23 (M3 task T10, [ADR-165](ADR-165-the-valleys-status-notices-and-battle-results-at-m3.md)).** `SettlementNotice` is version 2: M3's kinds (contact, claims, the warning, an attack ended before its battle, battles starting and ending, the respite and the run), the other settlement concerned, whether the steward's settlement acted, and a detail. The region raises them as valley events, and a battle's result follows its end as a `BattleResultNotice`, both in one queue of encoded messages.
