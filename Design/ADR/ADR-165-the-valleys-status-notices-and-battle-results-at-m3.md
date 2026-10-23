---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T10)
---

# The valley's status, notices and battle results at M3

## Context and Problem Statement

M3 task T10 puts phase A's valley on the wire for the HUD of T11 to T14:
- the relations: each Cladan's grievances and anger (MVP-050);
- the claims and each area's holder (MVP-053), with O102's cooldown;
- the warning, the expeditions and the battles in progress (MVP-042 to MVP-046), with the battlefield that WLD-244 adds to the steward's areas;
- the battle results (MVP-048): whole for a side, the public line for every other steward;
- the renown standing (MVP-062) and the run's time left, quiet minutes and end (MVP-060, MVP-063);
- the notifications of MVP-102: contact, claims, the warning, battles starting and ending, the respite and the run.

Its done-when is ADR-084's test with four settlements and a battle: the steward's client holds nothing outside its settlement's seen layer while a battle is fought (MVP-097, PIL-084).

These decisions are open:
- whether the valley travels as replicated objects of ADR-111 or as a status the client asks for, as the settlement's does (ADR-136);
- how the notices of M3's kinds are raised and which stewards receive them;
- what a steward whose settlement was no side of a battle learns of it.

## Decision Drivers

* MVP-098: the settlement status is server data read through server messages, and the HUD already asks for it once a second (ADR-152).
* A grievance has no stable id of its own: it is named by its holder, target, cause and subject, and its weight changes every second as it fades (ADR-159).
* MVP-097: a client receives no state outside its settlement's seen layer, so another settlement's stores, losses and expeditions stay the server's.
* A notice must arrive, and the notices and a battle's result must arrive in their order.
* GameClient includes no GameLogic header (MVP-090): the values the wire carries must be GameShared's.

## Considered Options

For the valley's state:
1. **A valley status** answered to a request, as the settlement's is (`ReadValleyStatusCommand`, `ValleyStatus`).
2. **Replicated objects** of new classes: claims, areas, grievances, expeditions, battles, respites, renown and the run.

For the notices:
3. **Valley events** that the region raises in its step, beside its presentation events, and the world server turns into notices for the stewards they concern.
4. **New presentation event kinds** (`EffectEventKind`) that the world server also reads for its notices.

For another settlement's battles:
5. **The public line**: its sides, its claimed area, its end, its winner, its ticks and whether an area moved.
6. **Nothing** of a battle the steward's settlement is no side of.

## Decision Outcome

Chosen options: 1, 3 and 5.
- Option 2 needs a stable id for each grievance, which has none, and a delta every second for each fading weight; the HUD reads the valley at the status's pace anyway.
- Option 4 puts fields a notice needs, the other settlement and the area, into a presentation event's free fields, and the presentation events are droppable and filtered by sight.
- Option 6 contradicts MVP-048.

* **The values the wire shares with the saved state.** `GrievanceCause`, `ClaimState`, `ExpeditionState` and `BattleEnd`, with their bounds, move from GameLogic to `GameShared/RelationMessages.h`, so that the client reads them; their values are unchanged.
* **The valley status** (`ValleyStatus`, MVP-098). A steward who commands the team asks with `ReadValleyStatusCommand`; the world server reads the request outside the queue, as it reads a settlement's (ADR-136), and answers after the next tick, after the settlement's status. A world whose run has ended still answers it. It holds:
  - the tick and the team;
  - the run's end tick, the first tick of its quiet minutes, and whether it has ended;
  - each settlement's renown by source, as the tuning derives it (ADR-163);
  - each of the nine areas' holder and the first tick a claim may be made on it again (O102);
  - every claim, at most `VALLEY_CLAIMS_MAX` (36, the region's bound);
  - the grievances its Cladan holds and those held against it, each with its cause, subject, flags and weight in millionths, at most `VALLEY_GRIEVANCES_MAX` (255) in the region's order;
  - the expeditions it sent and those declared on it: the warning's countdown is its declaration tick and `mvp.conflict.warning_minutes`;
  - every battle in progress, with its objective, its battlefield and each side's losses against the loss cap only when its settlement is a side; another steward's battle shows its sides, claim and start alone;
  - every respite.
  At its bound it is 5,492 bytes, a reliable message far under `MESSAGE_MAX_BYTES`.
* **Valley events** (`ValleyEvent`, `RegionSimulation::Raise`), never state, cleared with the presentation events at each step and pass. The rules raise one at each:
  - contact (MVP-027), the first of the pair acting;
  - claim made, withdrawn, lost or won, the claimant acting against the half's pair neighbour or the central site's holder, at the area's resource site (`ResourceSiteOfArea`, now public in `Claims`);
  - attack declared, the warning, and an expedition ended before its battle, withdrawn, emptied or unfought at the run's end;
  - battle started and ended, at its objective, the end's detail being whether the attacker won;
  - respite started.
* **The notices** (`SettlementNotice` version 2, ADR-139). `NoticeKind` adds these kinds and the run's quiet minutes and end. A notice gains the other settlement's site, whether the steward's settlement acted, and a detail; contact and the run's notices name no subject. The world server gives a valley event to the steward who commands the settlement that acted or the other one, and a claim on the central site to every steward whose own claim on it stands. The quiet minutes and the run's end go once to every steward as each begins, but not again to a world continued in either.
* **The battle results** (`BattleResultNotice`). Each steward is sent one after its tick's notices for every battle that ended: whole for a steward whose settlement was a side (the Workers each side lost, the stores and Essence moved, the grievance, the respite and the renown it gave its winner), the public line for every other. The claimed area comes from the claim's end in the same tick. The notices and results wait in one queue of encoded messages, so a refused send keeps their order.
* **The client** keeps the latest valley status (`ClientSession::LatestValleyStatus`) and the results until taken (`TakeBattleResults`). Until T11 draws M3's cards, the HUD shows the notices of structures alone.
* **A figure that changes teams goes whole** (ADR-111). T10's test found that an expedition's figures, which leave their settlement's team for the expedition's and come back (ADR-161), kept their old team in the replica, since a figure's delta carries no team. `MakeRecord` now sends a figure whose team differs from its baseline whole.

### Consequences

* Good, because no state outside what the steward's settlement may know reaches its client: another settlement's stores, losses and expeditions stay on the server, and the test proves it while two battles are fought.
* Good, because the HUD reads the valley the way it reads the settlement, once a second, with no second copy of a rule on the client.
* Good, because the notices arrive once, in order, with their results.
* Bad, because the battlefield of WLD-244 and the warning's countdown arrive at the status's pace, up to a second after the battle's start; the notice of the start arrives at its tick.
* Bad, because a figure that changes teams costs a whole record once.
* It forecloses: grievances and battles as replicated objects; a notice that rides a presentation event; a battle's figures sent to a steward who is no side of it.

### Confirmation

* `GameSharedTests`: the valley status and a battle result round-trip at their bounds, and a claim or expedition of id 0, a raid that moved an area or an end before the start do not decode; a notice of version 2 round-trips at its bound, and only contact and the run's name no subject; a figure of another team goes whole.
* `GameLogicTests`:
  - **ADR-084's test with four settlements and a battle** (`Adr084AValleyClientHoldsNothingOutsideItsSeenLayerInABattle`). The valley at war (ADR-164), saved and continued by a world server, with two raids in progress, one on the steward's settlement. For 400 ticks the client's replica equals the server's view and holds no other team's figure outside its seen layer but its own expedition's, and no other settlement's stores. Every valley status lists only its settlement's grievances and expeditions, and another battle's public line. Both battles end with their results, whole for its own and the public line for the other, and only its own battle's end is noticed.
  - each claim's path raises its valley event, and a step clears them;
  - the run's quiet minutes and end are each noticed once.
* `GameClientTests`: the session keeps the latest valley status and takes the results once, in order.
* The owner's checks wait for T11 to T14, which show what T10 sends.

## Pros and Cons of the Options

### 1 — a valley status

* Good, because it is ADR-136's path, its narrowing is one function, and a grievance needs no id.
* Bad, because its state is up to a second old.

### 2 — replicated objects

* Good, because a change arrives at its tick.
* Bad, because grievances would need ids of their own, and each fading weight a delta every second.

### 3 — valley events

* Good, because each carries the fields its notice needs, and the presentation events stay as they are.
* Bad, because the region keeps a second list of each tick's events.

### 4 — presentation event kinds

* Good, because the list exists.
* Bad, because a droppable event would carry what must arrive, in fields named for other things.

### 5 — the public line

* Good, because MVP-048 asks for it and it names nothing the seen layer hides.
* Bad, because a steward learns that two settlements fought where it cannot see, which WLD-242's public layer allows.

### 6 — nothing

* Good, because it sends less.
* Bad, because MVP-048 gives every steward the public line.

## More Information

* Amends:
  - [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md): a figure whose team changed goes whole;
  - [ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md): the valley's status beside the settlement's;
  - [ADR-139](ADR-139-replicating-the-settlement-at-m2.md): the notice's version 2 and M3's kinds.
* Related: [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md) to [ADR-164](ADR-164-the-command-paths-confirmation-the-valley-run-and-the-valley-at-war-at-m3.md) (phase A).
* Owner decisions O98, O102, O101; MVP-027, MVP-042, MVP-048, MVP-050, MVP-053, MVP-060 to MVP-063, MVP-090, MVP-097, MVP-098, MVP-102; PIL-084; WLD-242, WLD-244.
* The code:
  - `GameShared/RelationMessages` (the moved values, `ReadValleyStatusCommand`, `ValleyStatus`, `BattleResultNotice`), `SettlementMessages` (`NoticeKind`, `SettlementNotice`), `GameMessages`, `ReplicatedState` (`MakeRecord`);
  - `GameLogic/Relations` (`ValleyEvent`), `RegionSimulation` (`Raise`, `ValleyEvents`), `Claims` (`ResourceSiteOfArea`), `Battles`, `SettlementCommands` (`ValleyStatusOf`, `ResultNoticeOf`), `WorldServer`;
  - `GameClient/ClientSession`, `SettlementHud`;
  - `GameSharedTests/RelationMessageTests`, `SettlementMessageTests`, `StateUpdateTests`; `GameLogicTests/ReplicationTests`, `ClaimTests`, `RunTests`; `GameClientTests/ClientSessionTests`.
* **Amendment 2026-10-23 (M3 task T13, [ADR-168](ADR-168-the-battle-result-card-at-m3.md)).** The HUD takes the session's results each frame: the settlement's own wait on the result card, every other's public line is a card among the notices.
