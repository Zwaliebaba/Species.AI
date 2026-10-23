---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T8, owner decisions O53, O80, O98)
---

# The run, renown and the epilogue's data at M3

## Context and Problem Statement

M3's T8 gives a world its run and keeps what the epilogue shows:
- **The run** (MVP-060): `mvp.run.length_hours` (8) of play at the session pace. The world shows the time left, no attack may be declared in its last `mvp.run.quiet_minutes` (15), and battles in progress finish.
- **Renown** (MVP-061, PIL-125), never by stores taken: `mvp.renown.defense_held` (3) for each battle won as defender, `mvp.renown.claim_won` (3) for each claim battle won, `mvp.renown.grievance_eased` (1) for each 20 weight of grievance eased by tribute or a withdrawn claim, and `mvp.renown.structure_per_100ru` (1) for each 100 ru of structures completed. The standing of the four settlements is visible throughout (MVP-062).
- **The end** (MVP-063): the world becomes read-only, and the epilogue shows the final standing, each settlement's history and the steward's statistics.
- **The statistics** (MVP-064) gain the battles, claims and grievances, and, which M2's [ADR-156](ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md) left to M3, the tick each territory first reaches a halfway line, so that a saved world's contact hour can be judged.

The epilogue's page and the standing's display are T14's; T8 keeps their data.

Four things stand in the way:
- **The run is the world's**, while the rules it changes, the quiet minutes above all, are the regions'.
- **"Read-only".** Every owner's tick must still advance, for a world steps in step with its server's tick loop and a snapshot checks the regions' ticks against the world's ([ADR-099](ADR-099-world-snapshots.md)).
- **The statistics' kinds are written in a bounded field**, so new kinds widen it and change the layout of every entry.
- **The owner's M2 world** has no run and no renown, and must continue (O98).

## Decision Drivers

* The run ends on time with its standing and history (the M3 plan's T8 row).
* A world that ended stays read-only after a restart.
* Renown is derived from what happened, so that its provisional keys can be retuned (MVP-003).
* A world of version 14 or earlier continues.

## Considered Options

1. **The run held by the world owner and given to its regions**; renown kept as each settlement's counts of its sources; the statistics' new kinds written in a wider field from version 15.
2. **The run held by each region**, with its own end.
3. **Renown kept as points**, added as each source occurs.

## Decision Outcome

Chosen option: 1.
- **Option 2** puts a world's one run in as many places as it has regions.
- **Option 3** freezes the keys' values into the saved points, so retuning a key changes no world already played.

* **The run** (`WorldSimulation::Run`, world-owner state from version 15):
  - **Its fields.** The tick it ends at, whether it has ended, and the tick it did.
  - **Its length.** A new world's run ends `RunTicks` (`length_hours` at the tick period) after the world's first tick. The world gives its regions the end tick (`RegionSimulation::SetRunEnd`).
  - **The quiet minutes.** A region is in them from `quiet_minutes` before the end (`InQuietMinutes`). A declaration there is refused (`AttackRefusal::Quiet`, 8, after `NoSettlement`), and an AI settlement starts no muster and gives one up ([ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md) amended).
  - **Its end.** From the end tick, after each step, every expedition whose battle has not started ends as a withdrawal before its battle does (`EndUnfoughtExpeditions`). The run ends at the first step after which no battle is in progress, so battles in progress finish.
  - **Read-only.** A world whose run has ended steps only its tick and its regions' (`RegionSimulation::Pass`, which also clears their presentation events). The world server answers every command but a status request as `NotPermitted`, without queueing it, so no AI, steward or command changes the world again. Its snapshot holds the ended run, and the world opened from it is read-only too.
* **Renown** (`GameLogic/Renown`, region state from version 15): each settlement's record (`SettlementRenown`), made with it, counts:
  - **battles won as defender**, a raid withdrawn in its battle among them (MVP-045);
  - **claim battles won**;
  - **the weight of grievance eased**, in millionths, by tribute (the weight it removed) and by a withdrawn claim. A withdrawn claim counts its grievance's weight at the withdrawal, once the claim has stood `mvp.claim.uncontested_minutes`, as [DEC-1007](../decisions/species_design.md#dec-1007-renown-for-a-withdrawn-claim) states;
  - **the cost of its structures completed**, a turret's rebuild at what the rebuild cost.

  The points by source are derived (`RenownOf`): the counts times their keys, the eased weight in whole twenties and the cost in whole hundreds of ru. The standing orders the settlements by their total, the lower site first on a tie (`Standing`). Stores taken earn nothing.
* **The statistics** ([ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md) amended) gain seven kinds. The kind's bound is 10 from version 15, and an entry of an earlier version reads its kind in the bound of 3 it was written with. The new kinds, for the settlement each names:
  - **`HalfwayReached`**, its pair: the territory first reaching each of its valley border pairs' halfway lines, in its settlement's step. The region knows which it has recorded from its statistics, and knows them again when it is read.
  - **`GrievanceAdded`**, its cause: a new grievance of the holder's. A grievance that only grows records nothing.
  - **`ClaimMade`**, and its end: **`ClaimWithdrawn`**, **`ClaimLost`** or **`ClaimWon`** (a central site held after its wait is won), each with the area.
  - **`BattleEnded`**, for each side, with the place of its battle result.

  `IsGrowthStep` stays the M2 plan's three kinds, so the growth scene reads its steps and samples as before.
* **The epilogue's data** is the region's own state at the run's end: the standing and renown by source, the battle results, and the statistics of each settlement. Those are its strength and Worker count over the run, its growth steps, battles, claims and grievances. Nothing is copied for the epilogue.
* **The saved state.** `WORLD_STATE_VERSION` is 15:
  - the world owner writes its run after its commands;
  - each region writes its renown after its AI settlements' decisions;
  - statistics entries carry the wider kind.

  A world of version 14 or earlier begins its run where it continues, a full run from the tick it was saved at. Its settlements' renown starts at none, and its statistics keep their entries. The empty world's golden hash of DBG-147 moves with the run and the renown count, checked to be the only change.
* **What waits.** The time left, the standing, the epilogue and the front end's entries on the client are T10 and T14. The always-on pace's longer run is M4's.

### Consequences

* Good, because a run ends on time and stays ended across a restart, its history in the state itself.
* Good, because renown is derived, so Playtest 1 can retune its keys and see every world's standing move with them.
* Good, because the owner's M2 world continues with a whole run ahead of it.
* Bad, because an ended world still steps its tick and is saved on its calendar, a snapshot every `mvp.persist.snapshot_seconds` of a world that no longer changes.
* Bad, because a claim made and withdrawn before its wait earns no renown, which narrows MVP-061's letter (DEC-1007).
* Bad, because the halfway-line check runs in every settlement's step until the line is reached. That is a few hundred cell reads for each of its two pairs.
* It forecloses: renown earned by stores taken; a run that ends while a battle is in progress; a command executed in a world that has ended.

### Confirmation

GameLogicTests `RunTests`:
- **The run's end.** A run of 1,800 ticks from tick 0, which the region knows. The quiet minutes begin 300 ticks before the end, and refuse a declaration. The run ends at its end tick, and then fifty ticks change nothing in the region but its tick.
- **Battles finish.** An expedition still held at the end ends with no result; the run waits for the battle in progress and ends the tick after it.
- **Renown.**
  - A structure of 250 ru gives 2.
  - A claim withdrawn at once eases nothing; one withdrawn after its wait eases its grievance's weight, which with a tribute's 40 makes 3.
  - A raid held gives the defender 3; a claim battle won gives the claimant 3.
  - The standing orders the four settlements by their totals.
- **The statistics.** None at the founding radius; once at their largest, each settlement's two halfway lines, once each. A new grievance once, with its cause. Three claims made: one withdrawn, one lost, one won. A battle for each side by its result's place.
- **Read-only after a restart.** The world server answers a command as not permitted and a status request as applied once the run has ended. The world opened from its snapshot is ended, as it was, and stays so.
- **Saving.** The run, the renown, the statistics and the known halfway lines are restored. A region of version 14 reads with its renown at none, and an entry of version 14 reads its kind in the old bound.

`StateHashTests` holds DBG-147's new golden values. `StatisticsTests` allows a lone settlement's halfway lines among its entries. The whole GameLogic and GameShared suites pass.

## Pros and Cons of the Options

### 1 — the world's run, derived renown

* Good, because a world has one run, and every key stays a lever.
* Bad, because the regions hold a copy of the run's end that is not their state.

### 2 — a run in each region

* Good, because the region reads its own end.
* Bad, because one world's end is kept as many times as it has regions.

### 3 — renown as points

* Good, because the standing is read, not computed.
* Bad, because a retuned key leaves the points already earned at the old values.

## More Information

* Amends:
  - [ADR-099](ADR-099-world-snapshots.md): the world owner's run in the snapshot;
  - [ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md): the statistics' new kinds;
  - [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md): state version 15;
  - [ADR-156](ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md): the halfway lines recorded;
  - [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md): a new grievance recorded;
  - [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md): the claims recorded and eased weight counted;
  - [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md): the quiet minutes, the run's end and the battles recorded;
  - [ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md): no muster in the quiet minutes.
* Owner decisions O53, O80, O98; MVP-045, MVP-060 to MVP-064, MVP-075, MVP-095; PIL-122, PIL-125; DEC-1007; DBG-147.
* The code:
  - `GameLogic/Renown`, `Statistics`, `WorldSimulation` (`Run`, `RunTicks`), `RegionSimulation` (`Pass`, the run's end, `HalfwayReached`, the renown), `Battles`, `Claims`, `Relations`, `SettlementAi`, `WorldServer`, `WorldSnapshot`;
  - `GameShared/RelationMessages` (`AttackRefusal::Quiet`);
  - `Tools/SpeciesPerformance/GrowthScene`;
  - `GameLogicTests/RunTests`, `StateHashTests`, `StatisticsTests`, `TestSupport` (`RegionStateOf`).
