---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T8)
---

# Settlement commands, status and statistics at M2

## Context and Problem Statement

M2 task T8 makes the settlement's verbs commands and its state readable.
- The steward places a structure, rebuilds a destroyed turret and sets the three priorities. Each is a command of the one validated path of ADR-087 (MVP-094, MVP-101), and a refused placement must tell the steward why (MVP-032).
- The settlement status is server data, read through server messages, so that a later companion reads it without a change to the server (MVP-098, PIL-075).
- The server records each settlement's strength and Worker count every `mvp.stats.sample_minutes`, and every growth step (MVP-064). The M2 plan's §9, question 1, defines a growth step as a structure built, the population reaching a new multiple of a step held in data, or the territory taking in a resource site.

ADR-087's outcome says only whether an executed command had an effect, and the queue's six reasons cover refusals on arrival. A placement refused on execution has seven reasons of its own (ADR-132). The requirements leave open:
- how that reason reaches the steward;
- whether the status is pushed or asked for, and what it holds;
- where the statistics live, and how a growth step is detected.

## Decision Drivers

* An outcome stays one message per command, in the order the commands were sent (ADR-095).
* The engine knows no game concept (R9): a placement's reasons are the game's.
* The status is read the way a companion would read it (PIL-075), and costs nothing while nobody reads it.
* The statistics survive saving and continuing, because the owner's growth run spans sessions (M2 plan §5), and they are the same on every replay of a command stream (MVP-093).

## Considered Options

For the refusal's reason:
1. **A reason byte in every outcome**, which the engine passes unread and the game's handler fills.
2. **A message of its own** for each refused placement, beside the outcome.

For the status:
3. **A request answered by the server outside the queue**, with its outcome and then the status of the next tick.
4. **A status pushed** to the steward every second.

For the statistics:
5. **Entries in the region's state**, appended in each settlement's own step.
6. **A log file** beside the snapshots.

## Decision Outcome

Chosen options: 1, 3 and 5.
- Option 2 doubles the messages of a refusal and leaves the client to pair them.
- Option 4 sends 169 B a second with a full queue of eight sites (by its field widths), which the HUD, the only reader before M3, can ask for while it is open; a companion asks, as option 3 does.
- Option 6 is lost with a torn snapshot's tick, is not covered by the hash and is not replayed.

* **The reason.** An executed command's handler may return a `CommandExecution`, its effect and a reason byte, in place of its `CommandEffect`. The queue passes the reason to the outcome unread, with 0 for none and for every refusal on arrival. `CommandReport` version 2 carries it as 8 bits. A placement's or a rebuild's reason is its `PlacementRefusal`, which moves with `StructureKind` to GameShared as a wire value.
* **The commands.**
  - `PlaceStructure`: a team, a kind and a ground point.
  - `RebuildTurret`: a team and the turret's stable id.
  - `SetPriorities`: a team and three shares in percent.

  Each is permitted on arrival when its sender commands the team, a steward or an AI alike (DEC-122), so the scripted steward of T10 takes the same path. Shares that do not sum to 100, an id of 0 and a point that is not finite are malformed. On execution, a team with no settlement does nothing; a placement or rebuild runs the checks of ADR-132 and ADR-133, and a refusal is its reason.
* **The status.** `ReadSettlementStatus` names a team. The world server answers it itself, as it does the sandbox's restart (ADR-109): it checks the version, the size and the fields, then that the sender commands the team (otherwise `NotPermitted`) and that the team has a settlement (otherwise `NoEffect`). The outcome is reported at once with the next tick. After that tick, the `SettlementStatus` follows on the reliable channel. A status the backlog refuses goes after a later tick and carries its own tick.

  The status holds:
  - the stores and their capacity;
  - the population and its limit;
  - the growth state: growing, at the limit, or stores short of an orb, which is MVP-102's "cause when growth stops", and the ticks to the next orb;
  - the priorities, and the task counts of the Workers that follow no Leader;
  - the idle Workers by reason (MVP-037);
  - the inactive structures and the upkeep an hour (MVP-035);
  - the strength (MVP-038);
  - the construction sites in build order, with kind and progress in thousandths, at most 255.

  A request is no queued command, so the region's queue refuses one as an unknown type, and it is not journaled.
* **The statistics.** The region holds a list of entries, each with:
  - its tick and its settlement's site;
  - its kind: a sample, a structure built, a population step or a resource site taken in;
  - a detail: the structure's kind, the multiple reached or the site's index;
  - the settlement's Worker count, a Leader counting as the Worker it was;
  - its strength at the end of its own step.

  Each settlement's step ends by recording, in this order:
  1. each of its structures whose completion tick is this tick, in slot order (the Wellspring, placed by the world, never);
  2. each resource site its territory took in during the step, found by comparing the outer resource sites the territory holds before and after it grows (the central site, which needs a claim, never);
  3. each multiple of `mvp.stats.population_step` (10 Workers) above the settlement's mark, which only rises;
  4. a sample when the world tick is a multiple of `mvp.stats.sample_minutes` at the tick period.

  A region holds at most 65,535 entries. A run of the MVP records under 2,000: four settlements sampled every 5 minutes for 8 hours make 384, and their growth steps fewer than that again. The mark starts at the founding Workers' multiple, so founding is no step.
* **State.** The statistics and each settlement's population mark are part of version 9 (ADR-099). The golden hashes of DBG-147 change by the empty list each region writes after its territory events.

### Consequences

* Good, because every settlement verb, the steward's and the scripted steward's, is one command with one outcome that says why it did nothing.
* Good, because a status is computed only when asked for, and the HUD and a check-in read the same message.
* Good, because the statistics are saved, hashed and replayed with the world: two runs of one command stream record the same entries.
* Bad, because a status request takes a sequence number but no journal entry, so a replayed journal's sequence numbers have gaps where requests were, as the sandbox's restarts already leave.
* Bad, because a structure built and a resource site taken in during the same tick are recorded as two growth steps, though the steward sees them together. The growth scene of T10 counts steps by their ticks.

### Confirmation

NeuronServerTests (`CommandQueueTests`): an executed command's reason reaches its outcome, and a refusal on arrival has none.

GameSharedTests (`SettlementMessageTests`):
- a status listing 255 sites is exactly its bound and reads back as written;
- shares that do not sum to 100, a turret id of 0 and a point that is not finite are malformed;
- version 2 of the report carries its reason.

GameLogicTests (`SettlementCommandTests`):
- a placement refused as not buildable, outside the territory, in the centre ring, unaffordable or overlapping gives its reason, and one that passes pays and queues;
- an AI's placement takes the same path;
- the priorities set by command, and nothing set for a team with no settlement;
- a rebuild by stable id, refused for a structure that is no turret site and for empty stores;
- six refusals on arrival counted with their tick, a status request among them as an unknown type;
- through the world server: a status request answered with its outcome and then the status of the next tick, a request for another team not permitted, and a placement's refusal reaching the steward.

GameLogicTests (`StatisticsTests`):
- a sample at tick 0 and every period;
- a dwelling built, the multiples from 30 to 150 Workers each once, and each resource site the growing territory takes in;
- the same entries from one recorded command stream on two runs and at two worker threads.

## Pros and Cons of the Options

### 1 — a reason byte in every outcome

* Good, because a refusal stays one message, and the engine reads none of it.
* Bad, because every outcome grows by a byte.

### 2 — a message of its own for each refusal

* Good, because the report keeps its version.
* Bad, because the client pairs two messages for one command.

### 3 — a request answered outside the queue

* Good, because it is a companion's API call (PIL-075), and nothing is computed while nobody asks.
* Bad, because the HUD asks again for each fresh status.

### 4 — a status pushed every second

* Good, because the client asks for nothing.
* Bad, because it is sent while no panel shows it, and a companion would need another path.

### 5 — entries in the region's state

* Good, because they are saved, hashed and replayed with the world.
* Bad, because the snapshot carries them: 202 bits an entry, at most 51 kB for 2,000 entries.

### 6 — a log file beside the snapshots

* Good, because the snapshot stays smaller.
* Bad, because the log and the world can disagree after a crash, and no hash covers it.

## More Information

* **Amendment 2026-10-22 (M3 task T5, [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)).** A claim, its withdrawal and tribute are commands of the same path, registered with the settlement's; their refusals are their outcomes' reasons, a `ClaimRefusal` or a `TributeRefusal`.
* **Amendment 2026-10-22 (M3 task T6, [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md)).** The declaration of an attack and its withdrawal are commands of the same path, registered with the settlement's; their refusals are their outcomes' reasons, an `AttackRefusal`.
* **Amendment 2026-10-22 (M3 task T8, [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).** The statistics also record the halfway lines a territory first reaches, each new grievance, each claim made and its end, and each battle for both its sides; the kind's bound is 10 from state version 15, and an entry of an earlier version reads its kind in the bound of 3.
* **Amendment 2026-10-23 (M3 task T10, [ADR-165](ADR-165-the-valleys-status-notices-and-battle-results-at-m3.md)).** Beside the settlement's status, a steward asks for its valley's (`ReadValleyStatusCommand`, `ValleyStatus`): the run, the renown, the areas, the claims, its relations, its expeditions, the battles and the respites, read outside the queue in the same way and answered after the settlement's.
* **Amendment 2026-10-23 (owner decision O106, [ADR-178](ADR-178-the-stewards-growth-hold.md)).** The growth hold (`HoldGrowthCommand`) is a command of the same path, registered with the settlement's. The status's growth state `Held` comes before any other cause, and `SettlementStatus` is version 2.
