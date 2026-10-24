---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M0 task T10)
---

# World snapshots at M0: the file format, the store and the world's state version

## Context and Problem Statement

MVP-075 has the server save the world as a versioned snapshot every `mvp.persist.snapshot_seconds` and when it stops cleanly. It writes a temporary file, flushes it and puts it in place by an atomic rename, keeps the last `mvp.persist.snapshots_kept`, and at start loads the newest snapshot that validates. MVP-095 has the loader migrate every older version it accepts and refuse, with a message, a version it cannot migrate, never loading a snapshot in part. The snapshot stands in for the consistent cut of ADR-083 item 9 and for the journaled record store of O42, which the MVP defers. Saving starts at M2.

M0 task T10 builds the versioned snapshot of an empty world: written, flushed and renamed atomically, loaded and validated, with a snapshot of an unknown version refused. Its done-when is unit tests of a round trip, a torn write and a refused version.

The specifications leave open:
- the file's layout and how a damaged file is told from a whole one;
- where the engine's part ends and the game's begins;
- how a version is numbered and migrated;
- the files' names and folder;
- what a server does when a snapshot is refused;
- whether M0's composition roots save at all.

## Decision Drivers

* A snapshot is loaded whole or not at all (MVP-095), and a torn or changed file is found before any of it is decoded.
* The engine does not know the game's state (R9): it frames a world's bytes, and the game encodes and migrates them.
* A restored world is the world that was saved: the same state, key and tick, and from there the same ticks (ADR-085's "confirmed when" from M2).
* The writes go through the platform layer's atomic replace (PLT-089), the same path as every other file the game writes.

## Considered Options

* 1 — one file per snapshot: a fixed header, the world's state in its own versioned encoding, and a SipHash-2-4 check value over everything before it; an engine store that names, lists, keeps and loads the files; the game's encoder and loader
* 2 — the same file with a CRC-32 in place of SipHash
* 3 — one file overwritten in place, with no history

## Decision Outcome

Chosen option: "1".
- Option 2 adds a second checksum to the code for no gain: SipHash-2-4 is already NeuronCore's, and its 64 bits miss less than CRC-32's 32.
- Option 3 has no older snapshot to fall back on when the newest is refused, which MVP-075's "the newest that validates" needs.

* **The file** (NeuronServer, `Snapshot.h`). All integers are little-endian.
  - Bytes 0 to 7 hold the magic `SPCSNAP` and 0x1A.
  - Bytes 8 and 9 hold the format version, 1 (`SNAPSHOT_FORMAT_VERSION`); bytes 10 and 11 are reserved and 0.
  - Bytes 12 to 15 hold the world's state version, bytes 16 to 23 the first tick the restored world steps, and bytes 24 to 31 the length of the state.
  - Then the state, then 8 bytes of SipHash-2-4 of every byte before them under the public key `SNAPSHOT_CHECK_KEY`.
  - The check finds a torn, truncated or changed file. It authenticates nothing: the file lives on the server's own disk.
* **Decoding** refuses with one of seven reasons (`SnapshotError`), in this order, and decodes nothing of a refused file:
  - a file that is not a snapshot;
  - a file shorter than its header and check value;
  - a format version other than 1, read before anything that depends on it;
  - a length the file does not hold, as truncated or damaged;
  - a check value that does not match, as damaged.
  - The game's loader adds a state version it cannot migrate and a state that does not decode whole.
  - The reason is logged with the file's name, and the next older snapshot is tried.
* **The store** (`SnapshotStore`) keeps one world's snapshots in one folder.
  - A file is `world-<tick in 20 digits>.snapshot`, so the names sort by tick. A name of any other form is not a snapshot, so the temporary file of an unfinished write is never listed.
  - `Save` writes the file through `ReplaceFileAtomically` (PLT-089), which writes a temporary file, flushes it and renames it over the target. It then removes all but the newest `kept`. A failed write leaves every earlier snapshot.
  - `Snapshots` lists them newest first; `Load` reads and decodes one.
* **The world's state** (GameLogic, `WorldSnapshot.h`) is the game's own versioned encoding, at version 2 since M1 task T2 (`WORLD_STATE_VERSION`):
  - the simulation key;
  - the region count;
  - then the world owner's state and each region's, in the canonical encoding that ADR-097's state hash covers.
  - So a restored world hashes as the saved one did. The key is not in the hash, as ADR-082 item 1 requires, and is held in the snapshot because the snapshot is the world's own store at the MVP (ADR-082 item 1 allows the world's records to hold it).
  - The loader checks that the header's tick is the world owner's tick and each region's index and tick, and that the state ends where its last owner's does.
* **Versions and migration (MVP-095).** A change to any owner's state layout raises `WORLD_STATE_VERSION` and adds a step that migrates the previous version to it.
  - `RestoreWorld` runs the steps from the snapshot's version up to the current one.
  - A version below `WORLD_STATE_OLDEST_VERSION` or above the current one is refused.
  - Version 2 adds each region's teams and figures (ADR-101); its step reads a region of version 1 as one with no teams.
  - The format version changes only when the file's framing does, and this build reads format 1 only.
* **The world server.** Given a snapshot folder (`WorldServer::Desc::snapshotFolder`), the server:
  - opens the newest snapshot that validates;
  - opens a new world when none validates;
  - saves a snapshot when it freezes (ADR-083 item 9's final cut). The number kept is `mvp.persist.snapshots_kept`, at least 1.
  - Without a folder, the world opens new each run.
* **M0's composition roots give no folder.** MVP-075 starts saving at M2, so neither the integrated server nor `SpeciesServer.exe` saves at M0, and M0 never leaves a world that M2's loader would need to migrate or refuse. M2 adds:
  - the saves folder, under the user-data root of PLT-016 for the integrated server;
  - the save every `mvp.persist.snapshot_seconds` of world time;
  - the freeze record of an unclean stop (PIL-131), which needs a marker of a clean stop.

### Consequences

* Good, because a torn or changed file is refused before any of it is decoded. A restored world hashes as the saved one did and steps the same ticks.
* Good, because the engine frames bytes and knows no game type, and the game's version and migration live beside the state they describe.
* Bad, because every snapshot is a whole world. That is right for one empty region at M0 and is measured at M2. ADR-083 item 9's incremental cut, writing only changed owners, comes with O42's store.
* Bad, because a snapshot written at one version of the state can only be read by a build that keeps its migration step. Every step is kept for as long as the MVP's worlds live.
* It forecloses: a snapshot loaded in part; a world state that the engine decodes; a snapshot file written other than by atomic replacement.

### Confirmation

* `NeuronServerTests` checks:
  - a snapshot round trip;
  - that every shorter file and every flipped bit is refused;
  - that bytes after the check value, a torn header and another file are refused with their reasons;
  - that format version 2 with a correct check value is refused;
  - that the store keeps the newest three by tick and does not list a temporary file;
  - that a torn newest snapshot is refused while the next older one loads.
* `GameLogicTests` checks:
  - that a restored world of three regions has the saved key, state and tick, and hashes as the saved world after ten more ticks;
  - that state versions 0 and 3 are refused (0 and 2 at M0);
  - that every shortened state, a state with bytes after it and a header tick the state does not hold are refused;
  - that a world server that stops saves its world, and the next one opens it at the same tick with the same key and state and runs on;
  - that a folder with no valid snapshot opens a new world.
* ADR-085's "confirmed when" from M2 (a restored world gives the hashes of a world never stopped) and ADR-083's from M4 (the forced kill) confirm the rest.

## Pros and Cons of the Options

### 1 — a framed file with a SipHash check, an engine store, the game's encoding

* Good, because one checksum serves the state hash, the draws and the files.
* Bad, because a file holds the whole world.

### 2 — CRC-32

* Good, because a CRC-32 is the usual check of a file format.
* Bad, because it is a second algorithm to carry, and weaker.

### 3 — one file in place

* Good, because there is one file.
* Bad, because a refused file leaves nothing to load.

## More Information

* Related: [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md).
* The code is in NeuronCore (`ListFiles`, `RemoveFile`), NeuronServer (`Snapshot`) and GameLogic (`WorldSnapshot`, the owners' `ReadState`, `WorldServer`).
* **Amendment 2026-10-20 (M1 task T2, [ADR-101](ADR-101-region-teams-figures-and-update-slices.md)).** `WORLD_STATE_VERSION` is 2: each region writes its teams and their figures after its queued commands, and the first migration step reads a version-1 region as one with no teams. Version 3 is refused.
* **Amendment 2026-10-20 (M1 task T3, [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md)).** `WORLD_STATE_VERSION` is 3: the world owner writes the world seed after the world tick, and the migration step reads a world of version 1 or 2 as one of seed 0. The terrain is not state: the world server builds it again from the restored seed, and refuses a snapshot whose seed gives none as it refuses a damaged one. Version 4 is refused.
* **Amendment 2026-10-20 (M1 task T4, [ADR-103](ADR-103-region-grids-draw-contexts-and-figure-movement.md)).** `WORLD_STATE_VERSION` is 4: each figure writes its movement state after T2's fields, and each region writes its actor grid's capacities and its cache after its teams. The migration step reads a figure of version 3 or older as Idle where it stood and registers it there, with no grid history. Version 5 is refused.
* **Amendment 2026-10-20 (M1 task T5, [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md)).** `WORLD_STATE_VERSION` is 5: each figure writes its route handle and route point after T4's fields, and each region writes its route store after its cache: its last handle and the routes still held (DEC-651, O71). The migration step reads a figure of version 4 or older as following no route, and a region as holding none. A region refuses a snapshot in which a figure holds a route that its store does not hold, or a route point outside its route. Version 6 is refused.
* **Amendment 2026-10-20 (M1 task T6, [ADR-105](ADR-105-combat-weapons-and-essence-orbs.md)).** `WORLD_STATE_VERSION` is 6: each Worker writes its threat, threat range, scared flag, threat-sound latch and grenade timer after T5's fields; each team writes its kill counter after its figures; and each region writes its laser-bolt, effect and Essence-orb pools after its routes. The migration step reads a figure of version 5 or older with no threat, the full threat range and its first grenade timer's base, and a region as holding no bolt, effect or orb. Version 7 is refused.
* **Amendment 2026-10-20 (M1 task T7, [ADR-106](ADR-106-leaders-formations-and-battle-commands.md)).** `WORLD_STATE_VERSION` is 7: each region writes its commanders before its queue and each team its selection and pointer; each figure writes its Leader, its follow timer and the Leader's orders and formation, and no longer its formation flag; each effect writes an order marker's route and a trail mark's creation tick. An older region reads with no commander, and its figures with no Leader and no order.
* **Amendment 2026-10-20 (M1 task T8, [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md)).** `WORLD_STATE_VERSION` is 8: each region writes its AIs after its commanders and its bot navigation nodes' counts after its orbs, and each Worker its nearest node. An older region reads with no AI and makes its nodes as a new region does.
* **Amendment 2026-10-21 (M2 task T4, [ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md)).** `WORLD_STATE_VERSION` is 9: each region writes its settlements in founding order and then its structure pool after its bot navigation nodes. An older region reads with neither. A region refuses a snapshot with two settlements on one site, a settlement whose team or Wellspring it does not hold, or a structure of a team it does not hold. `RestoreWorld` takes the structure records, as the world does (ADR-129).
* **Amendment 2026-10-21 (M2 task T5, [ADR-131](ADR-131-structure-step-collision-shapes-and-ports-at-m2.md)).** Version 9 also holds each figure's port, its structure's id and slot and the port's index, and the Worker's states ToPort and Manning; no world was saved between T4 and T5.
* **Amendment 2026-10-21 (M2 task T5, [ADR-132](ADR-132-placement-and-construction-at-m2.md)).** Version 9 also holds each structure's construction state (built, its Worker-ticks, its cost and whether it is active) and each settlement's construction queue.
* **Amendment 2026-10-22 (M2 task T5, [ADR-133](ADR-133-the-gun-turret-at-m2.md)).** Version 9 also holds a turret's own state after its structure's, and each effect's source; an effect of version 7 or 8 reads its kind at the width before the turret round, with no source.
* **Amendment 2026-10-22 (M2 task T6, [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)).** Version 9 also holds each orb's settlement, with the orb states `Drifting` and `Waiting`, each settlement's growth timer, and after its structures each region's territory events (WLD-066); an orb of an earlier version reads its state at the width before them. A settlement's territory and seen layer are derived, never held.
* **Amendment 2026-10-22 (M2 task T7, [ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).** Version 9 also holds each Worker's settlement task, idle reason, ticks at its gathering port and carried load, the Worker state `Carrying`, and each settlement's three shares, which must sum to 100, its upkeep's carry and its inactive steps' timer, and each structure's tick of completion.
* **Amendment 2026-10-22 (M2 task T9, [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md)).** Version 9 is the first that a server saves, and it holds the world record (the world's freezes and the calendar time of the snapshot) after the regions, under the check value but outside the state hash. Version 9 also holds each region's statistics and each settlement's population mark (ADR-136). The world server saves every `mvp.persist.snapshot_seconds` as well as when it freezes, writes the command journal beside the snapshot it replays from, and leaves the marker of a clean stop; an unclean stop is a freeze of the world record. Until M2's exit build a raise of the version may refuse its predecessor; from that build every raise migrates it.
* **Amendment 2026-10-22 (M2 task T18, [ADR-155](ADR-155-the-front-end-and-saved-worlds-at-m2.md)).** A world server opened only to continue (`restoreOnly`) opens a new world without its folder when no snapshot validates, so the snapshots it refused are kept, and reports each refusal with its reason (`WorldServer::Opened()`).
* **Amendment 2026-10-22 (M3 task T8, [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).** From state version 15 the world owner's values end with its run: the tick it ends at, whether it has ended and the tick it did. A world of an earlier version begins its run where it continues.
* **Amendment 2026-10-24 (M4 task T2, [ADR-180](ADR-180-the-always-on-pace.md)).** From state version 18 a snapshot names the world's pace after its region count, ahead of the world owner's values, because the world's record, read at its pace, must exist before its regions. A world of version 17 or older restores at the session pace.
* **Amendment 2026-10-24 (M4 task T3, [ADR-181](ADR-181-presence-and-absences.md)).** From state version 19 each region's state ends with its absences: the stewarded settlements' open and latest closed absences, with their Workers, stores and deputy actions. A region of version 18 or older reads with none.
* **Amendment 2026-10-24 (M4 task T4, [ADR-182](ADR-182-the-deputy.md)).** From state version 20 each region's state ends with its deputies, after its absences. A region of version 19 or older reads with none.
* **Amendment 2026-10-24 (M4 task T5, [ADR-183](ADR-183-the-away-report.md)).** From state version 21 each absence also holds the grievances recorded for or against its settlement while it lasted. An absence of version 19 or 20 reads with none.
* **Amendment 2026-10-24 (M4 task T6, [ADR-184](ADR-184-the-freezes-of-a-stopped-server-and-a-sleeping-computer.md)).** From state version 22 the world record writes a freeze's cause in two bits, for M4's causes; a record of version 21 or older reads its one-bit causes.
