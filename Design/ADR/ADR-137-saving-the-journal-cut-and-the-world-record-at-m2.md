---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T9)
---

# Saving, the journal cut and the world record at M2

## Context and Problem Statement

M2 is the first milestone whose world is continued across sessions (MVP-075). M2 task T9 builds what ADR-099 left to M2:
- the save every `mvp.persist.snapshot_seconds` of world time;
- the freeze record of an unclean stop with its clean-stop marker (PIL-131);
- freezes recorded in the world record instead of the log (ADR-095, ADR-096);
- the command journal cut at each snapshot, so that a defect is reproduced from a snapshot and the journal beside it (MVP-076);
- the version policy of MVP-095: the first version saved, and the migrations the loader carries from it.

Its done-when:
- a restored world gives the hashes of the world never stopped, at 1, 2 and 8 worker threads;
- a torn newest snapshot falls back to the one before;
- an unclean stop is recorded as a freeze;
- a version the loader cannot migrate is refused with a message;
- the snapshot's cost at a settlement's plateau is recorded.

The requirements leave open:
- where a freeze is held, given that a stall is wall time;
- when the journal is cut and where its pieces go;
- how a clean stop is told from a crash;
- what a version raise owes the worlds already saved.

## Decision Drivers

* The state hash covers what the simulation computes, and a replay of the journal on the same build gives the same hash on every tick (ADR-097, MVP-093). A stall, which depends on the machine's load, cannot be part of it.
* A crash loses at most the ticks since the last snapshot (MVP-075), and the snapshot and journal on disk are enough to replay up to it.
* Saving happens on the tick thread, between ticks, so it sees a whole tick (ADR-083 item 9).
* The owner's own worlds of M2's exit, saved across sessions, are the first that must never be lost to a version raise.

## Considered Options

For the freezes:
1. **A world record beside the state**, saved in the snapshot after the regions, under its check value but outside the state hash.
2. **Freezes in the world owner's state**, with each freeze journaled so that a replay makes it again.

For the journal:
3. **One journal file for each snapshot**, written when the next snapshot is saved, holding what was accepted from the earlier one's tick.
4. **One journal for the whole run**, rewritten at each snapshot.

For a clean stop:
5. **A marker file** that a freezing server writes after its final snapshot, naming that snapshot's tick, and that an opening server removes.
6. **A flag in the snapshot**, set in the final snapshot only.

## Decision Outcome

Chosen options: 1, 3 and 5.
- Option 2 puts wall time into the hash, and its journal would need an entry for every stall.
- Option 4 rewrites a file that grows for the whole run, and its replay always starts at the world's start.
- Option 6 rewrites the final snapshot to clear the flag when the world opens, or leaves an unclean stop after a clean one undetected.

* **The world record** (`WorldRecord`) is held by the world beside its state. It holds:
  - the freezes, each with its world tick, its cause and its whole seconds of wall time, 0 when the server knows none;
  - the calendar time at which its last snapshot was written.

  `SnapshotOf` writes it after the regions, and `RestoreWorld` reads it from version 9. The snapshot's check value covers it, and the state hash does not, so a world restored and stepped hashes as one never stopped. Wall time comes from `WorldServer::Desc::calendar`, which the composition roots give the platform clock's calendar time (PLT-053: for records, never for timing). The cause "server stall" is recorded from each stall that the host reports as a freeze (ADR-095), with the seconds the loop gave up.
* **Saving.** With a snapshot folder, the world server saves a snapshot after the tick whenever the next tick is a multiple of `mvp.persist.snapshot_seconds` at the tick period (600 ticks), and when it freezes. The store keeps `mvp.persist.snapshots_kept`. A save runs on the tick thread under the profile marker `world.snapshot.save`.
* **The journal cut** (MVP-076). The server's journal runs from the world's start, or from the snapshot it opened, to the next snapshot it saves.
  - At each save, the journal so far is written beside the snapshot or start it replays from, as `world-<tick in 20 digits>.journal`, and a new journal starts from the snapshot just saved.
  - A journal that starts from a snapshot is marked `restored` (journal format 5): its replay restores that snapshot and steps it with the journal's entries (`ReplayJournalOnto`).
  - A journal that starts before the oldest snapshot kept is removed with the snapshots, except the one just written, which may start from the world's start.
  - A world restored from a snapshot journals from it.
  - After a crash, the last snapshot has no journal: the ticks after it were lost with the world, and the snapshot before it, with its journal, replays up to it.
* **A clean stop** (PIL-131, MVP-075).
  - A freezing server writes the marker `clean-stop.marker`, holding the tick of its final snapshot in decimal, after that snapshot is saved.
  - An opening server reads the marker and removes it before it opens anything, so that a stop that leaves no marker is unclean.
  - When the snapshot it opens is not the one the marker names, the ticks run after that snapshot were lost. The world records a freeze of cause "unclean stop" at the snapshot's tick, as long as the calendar time since the snapshot was written. A torn final snapshot is such a case: the world opens from the one before, and the marker names the torn one.
* **Versions** (MVP-095).
  - Version 9 is the first that a server saves (`WORLD_STATE_FIRST_SAVED_VERSION`). No world of versions 1 to 8 was ever saved; their migration steps stay with their tests, because they cost nothing to keep.
  - From this change on, every change to an owner's state raises `WORLD_STATE_VERSION`.
  - Until the build that runs M2's exit (T19), a raise may refuse the version before it instead of migrating it: `WORLD_STATE_OLDEST_VERSION` rises with it, and a world saved during development opens new, with the refusal logged.
  - From the build of M2's exit, whose growth run the owner saves across sessions, every raise carries the step that migrates its predecessor.
  - A refused snapshot is logged with its reason, and the next older one is tried (ADR-099). The front end of T18 shows the steward the reason when a world cannot be continued.
* **The saves folder.** The integrated server takes its snapshot folder in its description and passes it to the world server. The saves folder under the user-data root of PLT-016, with the start-up option `--user-data` (PLT-025), moves to T18: until its front end starts a world that is no sandbox, the programs start only the battle sandbox, which saves nothing, and the root would have no caller.
* **The snapshot's cost** at a settlement's plateau is measured by T10's performance scenes, which run the growth scene to its plateau. It is recorded in `playtests/M2.md` beside the tick's budget (ADR-080).

### Consequences

* Good, because a restored world is the world never stopped, tick for tick, and a defect between two snapshots is replayed from the first with the journal beside it.
* Good, because every freeze the steward is told of (MVP-074) is in the world's own record and survives saving.
* Bad, because a crash loses the journal of the ticks since the last snapshot, as it loses the ticks themselves.
* Bad, because a save is a whole world written on the tick thread every minute. Its cost is measured at T10, and ADR-083 item 9's incremental cut, writing only changed owners, comes with O42's store.
* Bad, because the world record is not checked by any replay: a freeze recorded wrongly is found only by reading it.

### Confirmation

GameLogicTests (`SavingTests`):
- a snapshot every period, the newest three kept, with the journals from the kept snapshots beside them, and the snapshot of tick 100 with its journal replaying the server's hashes of ticks 100 to 149;
- a world restored at tick 100, its steward connecting again, giving the hashes of the same world never stopped at 1, 2 and 8 worker threads;
- a stall that froze the world and an unclean stop recorded with their ticks and seconds and saved, and no freeze for a clean stop;
- a torn newest snapshot falling back to the one before with an unclean stop, a snapshot of a later version refused for the one before it, and a folder holding only that one opening a new world.

GameLogicTests (`IntegratedServerTests`): an integrated server stopped on the real clock leaves the marker of a clean stop in its folder, and one started on the folder continues the world after the ticks it ran.

GameLogicTests (`SandboxTests`): a freezing world's journal written under its starting tick and read back, a journal's `restored` flag read back, and a restored world journaling from its snapshot.

## Pros and Cons of the Options

### 1 — a world record beside the state

* Good, because the hash stays a function of the simulation.
* Bad, because no replay checks it.

### 2 — freezes in the world owner's state

* Good, because one hash covers everything saved.
* Bad, because every stall must be journaled for a replay to hash alike.

### 3 — one journal for each snapshot

* Good, because each piece is written once and replays from a snapshot that is kept.
* Bad, because the last piece is lost in a crash.

### 4 — one journal for the run

* Good, because one file holds everything.
* Bad, because it grows for the whole run and is rewritten every minute.

### 5 — a marker file

* Good, because opening removes it at once, so the next stop is judged on its own.
* Bad, because it is one more file in the folder.

### 6 — a flag in the snapshot

* Good, because the snapshot says it all.
* Bad, because the flag cannot be cleared without writing the snapshot again.

## More Information

* **Amendment 2026-10-22 (M2 task T18, [ADR-155](ADR-155-the-front-end-and-saved-worlds-at-m2.md)).** The client's worlds are saved in a folder each under `saves\` of the user-data root, which the front end starts. A world opened only to continue (`WorldServer::Desc::restoreOnly`) never writes over a folder whose snapshots it refused: with none that validates, it opens new without the folder, saves nothing, keeps the marker, and the integrated server stops it and gives the refusal's reason, which the front end shows (MVP-095). `WorldServer::Opened()` gives whether the world was restored and each snapshot refused.
* **Amendment 2026-10-22 (M3 task T2, [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md)).** The command journal is format 6, whose start holds the count of AI settlements; a journal of format 5 reads as one with none.
* **Amendment 2026-10-22 (M3 task T4, [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md)).** `WORLD_STATE_VERSION` is 11: each region's grievances follow its scripted stewards. A region of version 10 reads with none, as every raise from M3's first reads version 10 (O98).
* **Amendment 2026-10-22 (M3 task T5, [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)).** `WORLD_STATE_VERSION` is 12: each region's claims and moved areas follow its grievances, and a territory event's kind takes a bit. A region of version 11 or 10 reads with no claim and no moved area, its events contacts.
* **Amendment 2026-10-22 (M3 task T6, [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md)).** `WORLD_STATE_VERSION` is 13: each region's expeditions, battles, battle results and respites follow its moved areas. A region of version 12 or earlier reads with none.
* **Amendment 2026-10-22 (M3 task T7, [ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md)).** `WORLD_STATE_VERSION` is 14: each region's AI settlements' decisions follow its respites. A region of version 13 or earlier makes them afresh for each AI settlement's scripted steward.
* **Amendment 2026-10-22 (M3 task T8, [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).** `WORLD_STATE_VERSION` is 15: the world owner's run after its commands, each region's renown after its AI settlements' decisions, and statistics kinds in a wider field. A world of version 14 or earlier begins its run where it continues, its renown at none. A world whose run has ended is still saved on its calendar, and opens ended.
* **Amendment 2026-10-23 (O102, [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)).** `WORLD_STATE_VERSION` is 16: each region's areas' last claim ends follow its renown. A region of version 15 or earlier reads with none.
* **Amendment 2026-10-24 (M4 task T2, [ADR-180](ADR-180-the-always-on-pace.md)).** The command journal is format 7, whose start holds the world's pace after its AI settlements; a journal of format 5 or 6 is read as a world at the session pace. `WORLD_STATE_VERSION` is 18: the snapshot names the pace after the region count.
