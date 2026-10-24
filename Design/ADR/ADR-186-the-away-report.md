---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T5; owner decisions O6, O109, O110)
---

# The away report

## Context and Problem Statement

A steward who returns sees the away report first (MVP-074, PIL-073). Its summary fits on one page, readable in `pil.session.report_read_minutes` (2 minutes):
- the battles and their results;
- the Workers and stores lost and gained;
- the construction finished;
- the grievances recorded;
- the claims moved;
- the freezes.

The list of every deputy action with its standing order is one step away (PIL-026). The report and the settlement status are server messages, not client logic (PIL-075, O6). O110 has it open by itself on a return whose absence has news, and the latest report stays one press away on the HUD.

The region already keeps most of this with ticks:
- its battle results (ADR-161);
- its statistics' completed structures and claim ends (ADR-136, ADR-163);
- its moved areas (ADR-160);
- the world record's freezes (ADR-137);
- each absence's Workers, stores and deputy actions (ADR-184, ADR-185).

A grievance carries no tick, and its statistics entry names its holder but not whom it is against.

What does the report hold, how does the server build it, and when does it send it?

## Decision Drivers

* The report is built by the server from what the simulation recorded, so a later companion asks for it with the same call (PIL-075).
* It agrees with the server's statistics, which the M4 plan's T5 tests.
* It fits one reliable message, with the deputy's actions bounded.
* "News" is decided by the server from the report itself, as O110 defines it.

## Considered Options

1. **A report message built from the region's records, the absence keeping its grievances.** The absence also records each grievance added for or against its settlement while it lasts, with its tick. The server builds the report on a return or a request.
2. **The report kept as it happens.** The absence accumulates every line of the report as its events happen, so that it is the report.
3. **The client builds it.** The client compares its replica and statuses before and after the absence.

## Decision Outcome

Chosen option: 1.
- Option 2 copies the battle results, the statistics and the world record into the absence, two records of each fact that can disagree.
- Option 3 is client logic, which PIL-075 forbids, and a client that was not connected saw nothing.

What is built:

* **The grievances.** `AbsenceGrievance` (`GameLogic/Presence`): its tick, holder, against and cause. `GrievanceOf` (`GameLogic/Relations`), where every new grievance is made, records it in the open absence of either settlement (`RecordAbsenceGrievance`), at most `ABSENCE_GRIEVANCES_MAX` (256), the rest counted. `WORLD_STATE_VERSION` is 21 (`STATE_VERSION_WITH_ABSENCE_GRIEVANCES`). An absence of version 19 or 20 reads with none.
* **The shared values.** `FreezeCause`, `StandingOrder` and `DeputyActionKind` move to GameShared (`GameShared/AwayMessages`), because they are both saved state and on the wire, as M3's relation values are (ADR-165).
* **The messages.** `ReadAwayReportCommand` (the team) and `AwayReport`, of the message types `ReadAwayReport` and `AwayReport`, appended. The report holds:
  - its tick and team, whether there is an absence, and whether a return sent it (`onReturn`);
  - the absence's ticks and the Workers and stores at either end;
  - the dwellings, storehouses and turrets completed;
  - at most 64 battles: the other side, defended or not, won or not, how it ended and when, the Workers lost and the stores gained or lost, and whether an area moved;
  - at most 256 grievances, with their total;
  - at most 64 claims moved: a half's holder and claimant, or the central site's new holder;
  - at most 64 freezes: tick, cause and seconds;
  - at most 1,024 deputy actions, with their total.

  At every bound it encodes to about 23 KB, inside a reliable message's 64 KiB. `HasNews` is O110's news: a battle, a deputy action, a freeze, a grievance, a claim moved or a structure completed.
* **The builder.** `AwayReportOf(world, team, tick, onReturn)` (`GameLogic/AwayReport`) reads the settlement's latest closed absence (ADR-184) and keeps what falls in it, from the tick the steward went away up to the tick they came back:
  - the battle results that ended in it with the settlement as a side;
  - the statistics' structures completed by the settlement, and the central site won by its wait;
  - the territory events of halves moved to or from it;
  - the absence's grievances and deputy actions;
  - the world record's freezes from its first tick to its last. A freeze recorded at a restart carries the restart's tick, which is the first tick of the absence it opens.

  With no closed absence the report says so and holds nothing else.
* **The server.** A `ReadAwayReportCommand` is read, not queued, by a steward who commands the team, and answered after the next tick, as the epilogue is (ADR-169). `WorldServer::UpdatePresence`, on a return, marks each settlement each connected steward commands. Its report is sent after the tick only when it has news.

### Consequences

* Good, because the report is a server call that a companion can make (PIL-075), and each of its lines is a record the simulation already keeps.
* Good, because an absence without news opens nothing, so idling past the presence threshold does not interrupt the steward (O110).
* Bad, because a long absence's report is bounded: beyond 1,024 deputy actions or 256 grievances only the totals are told.
* Bad, because a version raise again; it migrates as every raise does (MVP-095).
* It forecloses a report of other settlements' battles; the valley status (ADR-165) and the results' public lines (ADR-168) remain for those.

### Confirmation

* `GameLogicTests/AwayReportTests`:
  - an absence with a raid on the settlement, its battle ended, a deputy action and a freeze gives a report whose battle, grievance, freeze, action, Workers and stores agree with the region's records, and which reads back whole;
  - an absence with nothing in it has no news;
  - the world server sends no report for a return without news, sends the report of a return whose absence holds a stall, after its tick, marked as a return's, and answers a request after the next tick with the same absence.
* `GameSharedTests/AwayMessageTests`: a report with every list at its bound fits a reliable message and reads back whole; a report without an absence has no news; an order that does not exist is malformed.
* T10's run checklist: the steward reads the summary within 2 minutes (Playtest 2's first line).

## Pros and Cons of the Options

### 1 — built from the records

* Good, because each fact has one record.
* Bad, because grievances needed a record of their own in the absence.

### 2 — kept as it happens

* Good, because the report is ready at the return.
* Bad, because it duplicates the region's records.

### 3 — the client builds it

* Good, because the server sends nothing new.
* Bad, because it is client logic, and blind to what the client did not see.

## More Information

* Owner decisions O6, O109, O110; MVP-074, MVP-098, PIL-026, PIL-073, PIL-075, PIL-131; the [M4 plan](../implementation/m4_plan.md), task T5.
* Amends [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) (where the freeze cause lives), [ADR-184](ADR-184-presence-and-absences.md) (the absence) and [ADR-099](ADR-099-world-snapshots.md) (the state version).
* The code: `GameShared/AwayMessages`, `GameShared/GameMessages`, `GameLogic/AwayReport`, `GameLogic/Presence`, `GameLogic/Relations`, `GameLogic/WorldRecord`, `GameLogic/WorldServer`, `GameLogic/WorldSnapshot`.
* **Amendment 2026-10-24 (M4 task T10, [ADR-191](ADR-191-the-away-report-and-the-pace-in-the-client.md)).** The client asks once for the latest report in an always-on world, and a return's report with news opens its page.
