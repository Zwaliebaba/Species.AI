---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T2; owner decisions O54, O109)
---

# The always-on pace

## Context and Problem Statement

From M4 a new world may be created at the always-on pace (MVP-071), and a world's pace is fixed for its life (PIL-104). The MVP scope's tuning table states every value at the session pace, and its pace column says how each converts at the always-on pace (the scope's §1.3). A `scaled` duration is multiplied by `mvp.pace.session_factor` (24), and a `scaled` amount per event or per hour is divided by it. `tick` and `—` values are the same at both paces, among them movement, battles, the warning and the presence threshold.

Fourteen values of the `mvp.*` record are `scaled`:
- the Wellspring's interval;
- the gathered load;
- the construction work per ru;
- the two upkeep rates and the inactive step;
- the respite;
- the grievance pressure and fade;
- the claim's uncontested wait and cooldown;
- the run's length and quiet minutes;
- the statistics' sample period.

Every reader of them in the simulation reads them through its region's or its world's record (`RegionSimulation::Tuning`). No world has run at the always-on pace yet.

How does a world read the record at its pace, and where is its pace kept?

## Decision Drivers

* One conversion, in one place, applied once, so that no reader can miss it and no `scaled` value is converted twice.
* The pace is world state: a world saved at the always-on pace continues and replays at it (MVP-075, MVP-076).
* The owner's M2 and M3 worlds, saved at versions 10 to 17, continue at the session pace (MVP-095, the M4 plan's §4).
* The client converts the same values as the server when it shows them (T10).

## Considered Options

1. **A paced copy of the record that the world owns.** `PacedTuning` converts the fourteen values once. The world owner holds the copy, and its regions read it.
2. **A conversion at each reader.** Each of the fourteen reads multiplies or divides by the factor when its world is always-on.
3. **A second record.** An `always_on` tuning record beside `mvp.json`, authored with the converted values.

## Decision Outcome

Chosen option: 1.
- Option 2 spreads the rule over fourteen sites, and a fifteenth `scaled` value would need a sixteenth edit somewhere else.
- Option 3 doubles every economy value's authoring, and the two records drift.

What is built:

* **The pace.** `WorldPace` (`GameShared/WorldPace`): `Session` (0) and `AlwaysOn` (1), a saved value that is never renumbered. `PaceName` and `PaceNamed` spell it as a world's description will (`session`, `always_on`).
* **The conversion.** `PacedTuning(record, pace)` returns the record unchanged at the session pace, or when the factor is not above 0. At the always-on pace it returns the record with the fourteen values of the scope's table 3 converted, and nothing else. The function and its list are the only place in the code that knows which values are `scaled`.
* **The world.** `WorldSimulation::Desc::pace` sets the pace. The world owner holds `PacedTuning` of the record it is given and hands that copy to its regions, so every region, AI and scripted steward reads the record at the world's pace. Its run's end is `RunTicks` of the copy.
  - `WorldSimulation::Pace` and `WorldSimulation::Tuning` read them back.
  - `WorldServer::Tuning` is still the record as the server read it, unconverted: it gives the tick period and the snapshot period, both `tick`.
* **The start.** `WorldStart::pace` (`GameLogic/CommandJournal`) and `WorldServer::Desc::pace` set a new world's pace. A restored world keeps its own.
* **The journal.** Format 7 writes the start's pace after its AI settlements. Formats 5 and 6 are read as worlds at the session pace.
* **The snapshot.** The pace is written after the region count, ahead of the world owner's state, because the world's record copy must exist before its regions do. `WORLD_STATE_VERSION` is 18 (`STATE_VERSION_WITH_PACE`). A snapshot of version 17 or older restores at the session pace, the only pace there was. No state hash covers the pace, as none covers the key or the region count. Two worlds that differ only in pace hash alike until their first `scaled` timer runs.

At the always-on pace a load of 1/24 ru is held as 42 thousandths of a resource unit (`MilliRuOf`), 0.8 % more than 41.67, so an always-on economy runs that much faster than 1/24 of the session's. The other conversions fit their fields exactly.

Measured on the tests' valley of seed 1 (the local Linux build of the game's libraries, `-O1`): a lone settlement at site 2 reaches 30 Workers at tick 9,239 at the session pace and at tick 207,562 at the always-on pace, 22.47 times as many. With a factor of 4 it reaches them at tick 35,264, 3.817 times. The ratio is below the factor because walking to a port and gathering there take the same ticks at both paces: the first loads' walk is a fixed cost that the always-on economy amortizes over 24 times as many ticks. The ratio rises towards the factor (17.9 times for the first Worker grown, 22.6 for the eleventh).

### Consequences

* Good, because every reader of the record converts by construction, a new `scaled` value is one line of `PacedTuning`, and the client can call the same function.
* Good, because a world's pace saves and replays with it, and every world saved before M4 continues as it was.
* Bad, because each world holds a copy of the record, a few kilobytes, and the world can be neither copied nor moved, as its regions hold the copy by reference.
* Bad, because the gathered load rounds to whole thousandths at the always-on pace.
* Bad, because tick-paced activities make the always-on economy slightly faster than 1/24 of the session's in its first hours. Playtest 2 and the away run of T12 judge the always-on economy as it is, not by the session's evidence.
* It forecloses per-value paces beyond the table's three kinds: a fourth kind is a new branch of `PacedTuning`.

### Confirmation

* `GameSharedTests/WorldPaceTests`:
  - each of the fourteen values is multiplied or divided by the committed factor of 24;
  - with them put back, the always-on record cooks to the session record's bytes, so nothing else converted;
  - the session pace reads the record unchanged;
  - the pace's names read back.
* `GameLogicTests/PaceTests`:
  - an always-on valley's Wellspring interval is 288 s and its run's end 24 times the session's;
  - its snapshot restores at its pace and runs on with the same hashes;
  - its journal's start names the pace;
  - a world in version 17's layout restores at the session pace with the same hashes;
  - a lone settlement at a factor of 4 reaches 30 Workers at more than 3.6 and at most 4 times the session's ticks;
  - an always-on valley hashes alike at 1, 2 and 8 worker threads over 600 ticks.
* Playtest 2 runs a world at this pace for 8 world days (the scope's §6.2).

## Pros and Cons of the Options

### 1 — a paced copy the world owns

* Good, because the conversion is one function, applied once.
* Bad, because the world carries a copy of the record.

### 2 — a conversion at each reader

* Good, because no record is copied.
* Bad, because fourteen sites must each remember the pace, and nothing finds the one that forgot.

### 3 — a second record

* Good, because each pace's values can be tuned apart.
* Bad, because every economy value is authored twice, and the scope's single table stops being the source.

## More Information

* Owner decisions O54, O109; MVP-070, MVP-071, MVP-095; PIL-100, PIL-101, PIL-103, PIL-104; the [M4 plan](../implementation/m4_plan.md), task T2.
* Amends [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) (the journal's format) and [ADR-099](ADR-099-world-snapshots.md) (the state version).
* The code: `GameShared/WorldPace`, `GameLogic/WorldSimulation`, `GameLogic/WorldSnapshot`, `GameLogic/CommandJournal`, `GameLogic/WorldServer`.
