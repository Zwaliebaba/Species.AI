---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T6; owner decisions O4, O54, O108)
---

# The freezes of a stopped server and a sleeping computer

## Context and Problem Statement

An always-on world freezes whenever its server process is not running or the computer sleeps, and each freeze is recorded with its cause and its world time (MVP-071, PIL-131). Each appears in the away report (ADR-183).

M2 recorded two causes in the world record (ADR-137):
- a stall of 10 s or more, which the tick loop gives up (ADR-095);
- an unclean stop, from the snapshot the server opens.

A clean stop recorded nothing, because a session-paced world is meant to be frozen while its client is closed.

At the always-on pace both missing causes matter. The steward closes the server's console (O109), and the world is frozen until it starts again. The computer sleeps overnight, and the tick loop, when it wakes, sees a stall as long as the sleep and records it as a server stall, which tells the steward that the server failed when it did not.

How are the two freezes recorded, and how does a server tell a sleep from a stall?

## Decision Drivers

* Every freeze is recorded with its cause and its world time, and its length where the server knows it (PIL-131).
* A sleep is told apart from a server's own stall, because they mean different things to the steward.
* The world record's saved layout migrates (MVP-095).
* The platform layer is the only place that reaches the operating system (ADR-083 item 1).

## Considered Options

1. **The suspend notifications, read at the stall.** A platform watch counts the system's suspends. The world server reads the count at each stall: a stall across a new suspend is a sleep. A clean stop is a freeze of cause "server stopped", from the final snapshot's calendar time to the next start.
2. **The stall's length alone.** A stall above some hours is a sleep.
3. **The resume notification as its own freeze.** The watch reports each sleep's wall time, and the server records it apart from the tick loop's stall.

## Decision Outcome

Chosen option: 1.
- Option 2 mistakes a long hang for a sleep, and a short sleep for a stall.
- Option 3 records one gap twice, once as the sleep and once as the tick loop's stall, unless the two are matched, which is option 1 with more code.

What is built:

* **The causes.** `FreezeCause::ServerStopped` (2) and `FreezeCause::Sleep` (3), saved values never renumbered. `FREEZE_CAUSE_MAX` is 3, so a cause is written in two bits.
* **The world record.** `ReadWorldRecord` reads a record as its state version wrote it. From version 22 (`STATE_VERSION_WITH_FREEZE_CAUSES`) a cause has two bits; before, one (`FREEZE_CAUSE_MAX_BEFORE_M4`). `WORLD_STATE_VERSION` is 22.
* **The stopped server.** When `WorldServer::Open` continues a world that froze cleanly (its marker names the snapshot it opens, ADR-137), it records a freeze of cause "server stopped" at the snapshot's tick. The freeze lasts from the snapshot's calendar time to now, 0 seconds when either is unknown. This applies at both paces: a session-paced world's closed client is a freeze too (O4). An unclean stop is recorded as before.
* **The watch.** `Neuron::SuspendWatch` (`NeuronCore/SuspendWatch`) registers with `PowerRegisterSuspendResumeNotification` (`DEVICE_NOTIFY_CALLBACK`, `powrprof.lib`) when it is made, and unregisters when it ends. It counts each `PBT_APMSUSPEND`; the count is atomic and read on any thread. A refused registration leaves it at 0 (`Watching` is false), and every sleep is then recorded as a stall.
* **The sleep.** `WorldServer::Desc::suspensions` reads the count. The world server notes it when it opens and at each stall. A stall that freezes, 10 s or more, across a suspend the server has not yet counted is recorded as a sleep; one across none, as a server stall. A shorter stall across a suspend freezes nothing, and the suspend it crossed is spent. The headless server and the integrated server each own a watch and pass its count.

### Consequences

* Good, because the steward's away report says the computer slept, or the server was stopped, for how long, rather than that the server stalled.
* Good, because the watch is a few lines of the platform layer, and a server without it records as before.
* Bad, because a stall that happens to follow a suspend, in a sleep of less than 10 s, spends it, and a later real stall is then a stall again, as it should be. A stall during the first ticks after a long sleep is the sleep itself, so the two are not told apart within one gap.
* Bad, because every reopened session-paced world now has a freeze in its record. The record is not the HUD's; only the away report shows freezes.
* It forecloses nothing: a freeze cause for an operator's maintenance (O4) is a fifth value when dedicated servers exist.

### Confirmation

* `GameLogicTests/SavingTests`:
  - a clean stop's next start records a freeze of cause "server stopped", as long as the server was stopped by the calendar;
  - a freezing stall after a suspend is a sleep, one after none a stall, and a short stall across a suspend spends it;
  - a version-21 record reads its one-bit causes as they were, and the current record round-trips.
* `NeuronCoreTests/SuspendWatchTests`: the watch registers, a suspend counts once and a resume not at all, and a second watch registers after the first ends.
* T9's run checklist: the owner puts the computer to sleep with an always-on world running, and its away report names the sleep.

## Pros and Cons of the Options

### 1 — the notifications, read at the stall

* Good, because one gap is one freeze, with the right cause.
* Bad, because it needs the platform watch.

### 2 — the stall's length

* Good, because it needs nothing new.
* Bad, because it guesses.

### 3 — a freeze of the resume

* Good, because the sleep's wall time comes from the system.
* Bad, because the same gap is recorded twice.

## More Information

* Owner decisions O4, O54, O108, O109; MVP-071, MVP-075, MVP-095, PIL-076, PIL-131; the [M4 plan](../implementation/m4_plan.md), task T6.
* Amends [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) (a clean stop is now a freeze), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md) (a stall's cause) and [ADR-099](ADR-099-world-snapshots.md) (the state version).
* The code: `NeuronCore/SuspendWatch`, `GameShared/AwayMessages`, `GameLogic/WorldRecord`, `GameLogic/WorldServer`, `GameLogic/WorldSnapshot`, `GameLogic/IntegratedServer`, `SpeciesServer/SpeciesServer.cpp`.
