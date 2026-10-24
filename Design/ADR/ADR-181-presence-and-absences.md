---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T3; owner decisions O54, O108, O109)
---

# Presence and absences

## Context and Problem Statement

MVP-072 makes the steward present while a client is connected and has given input within `mvp.presence.idle_minutes` (5 minutes), and away otherwise; a world at the session pace has no away state. While the steward is away the deputy runs the settlement (MVP-073, T4), and a returning steward sees the away report of the absence (MVP-074, T5).

Presence decides what the simulation does, because the deputy acts only while the steward is away. So it must:
- reach the simulation in a deterministic way;
- be saved;
- replay from the command journal (MVP-076).

Its inputs are not simulation state: a connection, and the time of the steward's last input. Only the server knows them.

The client sends commands, which are input, and requests (the settlement's and the valley's status, every second), which are not. A steward who only moves the camera sends neither.

How does presence enter the simulation, what counts as input, and what does the region keep of an absence?

## Decision Drivers

* No client can make its steward away or present by a forged command: presence is the server's fact, as a connection is (ADR-095).
* A replay of the journal gives the same presence at the same ticks, and a snapshot holds it.
* A steward who plays, camera only included, is never away.
* The deputy (T4) and the away report (T5) read one record of each absence.

## Considered Options

1. **A host fact between ticks.** The world server decides presence before each tick from its connections and their input. It applies a change to the world between ticks, as it applies a connection, and journals it as a presence entry. The region keeps each absence as a record.
2. **A command.** A presence command from the client, through the command queue, validated as any other.
3. **Derived at read time.** The region keeps no presence. The deputy and the report read the server's connections and input times.

## Decision Outcome

Chosen option: 1.
- Option 2 lets a client declare itself away, and an away state that a disconnection causes has no client to send it.
- Option 3 puts host state into the simulation's decisions, so a replay could not reproduce them.

What is built:

* **What counts as input.** The world server notes each steward connection's input tick (`WorldServer::Steward::inputTick`). It is set:
  - at the connection;
  - by each command a queue takes;
  - by each `PresenceReportCommand` (`GameShared/AwayMessages`, the message type `PresenceReport`, appended).

  The report has no fields. It is read, not queued, and answered as a request is, so it is in no queue and no journal. A request for a status, the valley's status or the epilogue is not input. The client sends the report when its input router sees input and no report has gone for `PRESENCE_REPORT_SECONDS` (15 s), so that camera-only play counts. The client's part is built with the always-on world's client (T10).
* **The rule.** `WorldServer::UpdatePresence`, before each tick's step: in a world at the always-on pace that is neither the sandbox nor watched, the steward is present while a steward connection's input tick is less than the idle minutes' ticks behind the tick, and away otherwise. A world at the session pace is never away. The idle minutes are `tick`-paced, 3,000 ticks at the committed 5 minutes.
* **The absence.** `Absence` (`GameLogic/Presence`) holds:
  - its team and the tick it opened;
  - the tick it closed, none while it lasts;
  - the settlement's Workers (`Population`) and stores at either end;
  - the deputy's actions (T4) in the order they were taken, at most `DEPUTY_ACTIONS_MAX` (4,096), the rest counted.

  `SetStewardPresence(world, away)` opens an absence for each stewarded settlement at the world's next tick, or closes the open one there. A closed absence replaces the settlement's previous closed one, so the region holds at most an open and a closed absence for each settlement: the latest closed one is the report the HUD asks for again (O109, T5). `StewardAway` and `OpenAbsence` read it.
* **The journal.** Format 8: each entry may be a presence change (`JournalEntry::presence`, `away`), written after its connection flag. The replay applies it before the tick's step, after the tick's connections and commands, as the server does. A journal of format 7 or older holds none.
* **The state.** The region writes its absences as a new last section. `WORLD_STATE_VERSION` is 19 (`STATE_VERSION_WITH_ABSENCES`). A region of version 18 or older reads with none, its steward present. The reader refuses an absence of a tick the region has not reached, of a team with no settlement, or a second open or closed absence of one team.

A world at the always-on pace that opens with nobody connected is away from its first tick: the client connects a moment after the server starts (T9). That absence closes at the connection with nothing in it, and an absence with no news opens no report (O109, T5).

### Consequences

* Good, because presence is reproducible from the journal and the snapshot, and no client can set it.
* Good, because the deputy and the report have one record per absence, with the Workers and stores at both ends that the report's summary needs.
* Bad, because a short absence replaces the previous closed one, including the absence of a restart; the HUD's "latest report" is always the latest absence, not the latest with news.
* Bad, because the client must report camera-only input, or a steward who only watches becomes away after the idle minutes. A battle watched without input makes the steward away; the deputy then commands the battle (O109).
* It forecloses presence per steward: the MVP has one steward, and `SetStewardPresence` applies to every stewarded settlement of the world. Several stewards will need presence per steward, which the journal entry would then name.

### Confirmation

* `GameLogicTests/PresenceTests`:
  - an always-on world with no client is away from its first tick;
  - the connection makes it present;
  - its input ages to away at exactly the idle ticks;
  - a presence report and a command make it present again, and a status request does not;
  - the client leaving makes it away;
  - a session-paced world has no absence;
  - the journal holds the changes at their ticks, its replay hashes alike on every tick with the same absences, and the absences save and restore.
* `GameSharedTests/AwayMessageTests`: the report is its 3-byte header and decodes.
* T10's run checklist: the owner idles past the idle minutes and comes back.

## Pros and Cons of the Options

### 1 — a host fact between ticks

* Good, because it is deterministic, saved and unforgeable.
* Bad, because the journal gains an entry kind.

### 2 — a command

* Good, because the command path exists.
* Bad, because a client could set its own presence, and a disconnection sends nothing.

### 3 — derived at read time

* Good, because no state changes.
* Bad, because the simulation would read the host, and nothing would replay.

## More Information

* Owner decisions O54, O108, O109; MVP-072, MVP-076, PIL-070; the [M4 plan](../implementation/m4_plan.md), task T3.
* Amends [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) (the journal's format) and [ADR-099](ADR-099-world-snapshots.md) (the state version).
* The code: `GameShared/AwayMessages`, `GameShared/GameMessages`, `GameLogic/Presence`, `GameLogic/RegionSimulation`, `GameLogic/CommandJournal`, `GameLogic/WorldServer`, `GameLogic/WorldSnapshot`.
* **Amendment 2026-10-24 (M4 task T5, [ADR-183](ADR-183-the-away-report.md)).** Each absence also records the grievances added for or against its settlement while it lasts (`AbsenceGrievance`), for the away report; `StandingOrder` and `DeputyActionKind` live in GameShared's `AwayMessages.h`. `WORLD_STATE_VERSION` is 21.
