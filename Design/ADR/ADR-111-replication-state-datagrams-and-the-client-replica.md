---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T12)
---

# Replication at M1: the state datagrams, the acknowledged baselines and the client's replica

## Context and Problem Statement

Task T12 of the [M1 plan](../implementation/m1_plan.md) replaces M0's tick report with replication, for the whole battle until the seen layer exists at M3:
- per-client deltas against an acknowledged baseline, and the snapshot join (ADR-086 items 3 and 5);
- the client's replica of the two latest ticks, drawn one tick behind with interpolation, held up to 250 ms and never extrapolated (ADR-084 items 3 and 4);
- the presentation events (ADR-084 item 5);
- ADR-084's M1 test: the headless server running a recorded command stream gives the same per-tick hashes with presentation stubs and with an integrated client attached (DEC-222 item 4).

The owner checks it by running the sandbox: the window's title shows each side's figure count from the replica.

These decisions are open:
- which objects and fields a client receives, and how they are quantised;
- the layout of a state datagram within ADR-088's 1,200 bytes;
- what "the acknowledged baseline" is when a tick's update spans several datagrams that are lost one by one;
- how the join reaches the client before blob streams exist (ADR-093);
- where the replica lives, so that ADR-084's test can compare it with the server's state;
- how presentation events travel and when they expire.

## Decision Drivers

* The transport is the only path between server and client, the in-process one included (ADR-084 item 1, MVP-090).
* Each state datagram decodes alone, and a lost one costs only its own records for one tick (ADR-086 item 3).
* Replication never writes simulation state: attaching a client changes no hash (DEC-222 item 4).
* The MVP builds neither ADR-086's interest tiers nor its bandwidth caps (MVP scope §5.1); the in-process connection has no cap (ADR-086 item 4).
* What T13 to T17 draw (M1 plan table 2) is replicated now, so that the wire format does not move under them.

## Considered Options

* 1 — per-object acknowledged states: each object's delta is against the last state of it that an acknowledged datagram carried, and a tick's records are grouped by that tick so that each datagram names one baseline
* 2 — one baseline per client: the latest tick whose every datagram was acknowledged, with every record a delta against it
* 3 — whole state every tick, with no baseline

## Decision Outcome

Chosen option: "1".
- Option 2 loses a whole tick's baseline to one lost datagram of it. Over a lossy link the baseline then falls behind, and every record grows with it.
- Option 3 needs no acknowledgement, but costs every object's whole record every tick: 400 figures take 6.2 KB a tick in place of 1.5 KB (below).

* **What a client receives** (`GameShared/ReplicatedState`), in stable-id order: every figure, effect and Essence orb of the MVP's one region.
  - **A figure:** its team and kind; its position; its facing as a yaw of 10 bits (0 faces +z, 256 faces +x); its health; its task, the Worker's or Leader's behaviour state; a Leader's order (R-UNIT-164), which chooses its banner (R-UNIT-191); and five flags: dead, on the ground, under its team's crowd control, in formation, and its team's selected Leader.
  - **An effect of the effect pool** (a grenade, a shockwave ring, an order marker or a trail mark): its team, kind, position and yaw; a timer, which is a ring's active tick and every other kind's fuse or fade; and a ring's size in 1/64 s.
  - **An Essence orb:** its team, its position, its state (birth, floating or ascent) and that state's timer.
  - **Positions** are quantised to 1/16 wu in world coordinates: 24 signed bits across (±524,288 wu) and 20 up (±32,768 wu), clamped to those ranges.
  - **A team's own objects.** A client sees another team's order markers and trail marks not at all (R-UNIT-185, R-UNIT-186), and that team's crowd-control and selected flags cleared. "Its team" is the team whose commander is the client's steward (ADR-106).
  - **Moved into GameShared** so that the client reads them: `WorkerState`, `LeaderState` and `LeaderOrder` (in `FigureKind.h`), `EffectKind` and `OrbState`.
* **The state datagram** (`StateUpdate`, on the state channel, at most 1,200 bytes):
  - the message header (type and version, 3 bytes);
  - Species' 8-byte header of ADR-088 item 3: the update's tick in 48 bits and its baseline's age in 16, where 0 names no baseline;
  - a record count of 10 bits;
  - the records.
* **A record** is its id, its kind, its class and its fields.
  - **Id:** the first record's in 64 bits; each later one's as the gap from the one before, in 3, 7, 15 or 64 bits behind a 2-bit width.
  - **Kind:** whole, delta or removal.
  - **Delta:** a mask of the fields that changed, then those fields. The position travels as the move from the baseline's, in 7, 11, 15 or 25 bits an axis behind a 2-bit width.
  - **Class:** every record but a removal carries it, so a datagram decodes with no baseline and the replica applies it to one.
  - **Bit counts.** The bits each record takes are counted exactly, and the sender fills each datagram to its limit.
* **The sender** (`GameLogic/StateSender`), one per connection, on the tick thread. It keeps the last state of each object that an acknowledged datagram carried, with that datagram's tick, and the records of each datagram still in flight. Each tick:
  - **A changed object** goes as a delta against its acknowledged state, grouped by that state's tick, so each group's datagrams name their baseline. An object with no acknowledged state within 32 ticks (ADR-086 item 3) goes whole.
  - **A new object** goes whole. Whole records and removals ride with the largest group.
  - **An object that left** goes as a removal until a removal of it is acknowledged.
  - **A tick with no change** still sends one empty update (13 bytes), so that the client knows the server is not late.
  - **Fates.** An acknowledged datagram's states become the baselines, unless a newer one was acknowledged already. A lost or refused datagram changes nothing, and its records go again the next tick. A datagram with no fate after 32 ticks counts as lost.
  - **A replaced world.** When the sandbox restarts (ADR-109) the new world's ids start again: every acknowledged state is forgotten, so every object goes whole, every old object is removed, and nothing in flight counts.
* **The snapshot join** (ADR-086 item 5) is the first update's whole records on the state channel. It is sent again each tick until each datagram of it is acknowledged. The blob stream that ADR-093 gives the join arrives with M4's remote client.
* **Orbs every tick.** An orb is sent every tick it changes, as a figure is, until ADR-086's tiers exist. At ADR-086's twice per second, an orb interpolated between the two latest ticks would stand still for four ticks and jump on the fifth.
* **The replica** (`GameShared/StateReplica`) is the decoding half of the codec, beside it, so that the server's tests compare it with the server's state. It holds nothing of the server's.
  - **Views.** It holds the latest view, and the previous view as it was at the tick before.
  - **History.** It keeps every state it received for 64 ticks, twice the server's window, as the baselines a delta may name.
  - **A newer tick** moves the latest view to the previous one.
  - **A late datagram** of an older tick feeds only the history.
  - **A delta whose baseline is missing** is skipped and counted, which no test has seen happen.
  - **An object that enters whole** is left out of the previous view, so it is drawn from its first state.
* **Interpolation** (`GameClient/ReplicaTiming`).
  - The latest tick's state is drawn one tick period after it arrived, and the previous tick's as it arrives. Each figure's position and yaw are interpolated between them, the shorter way round.
  - The fraction never passes 1, so nothing is extrapolated.
  - When the next state is more than 250 ms late, the client shows that it waits for the server: in the title until the UI exists.
  - The integrated host's jitter allowance is 0. M4 measures a remote client's.
* **Presentation events** (`PresentationEvents`, on the state channel):
  - Each datagram carries a tick's events in order: kind, team, subject, position at 1/16 wu, vector and value as 32-bit floats, each field only when it is not zero. A lost datagram loses its events.
  - The selection and order events go only to the team's commander (ADR-106).
  - The client keeps an event until it is taken or until the display time is 500 ms past it (ADR-084 item 5).
  - Every event kind of the MVP is droppable. The guaranteed kinds (messages, warnings) come with M2's UI, on the reliable channel beside the command outcomes.
* **The tick report is retired** (ADR-096 amendment). Its type id stays unused. The client's tick is the replica's latest.
* **The title** shows the seed, the replica's tick and each team slot's live figures, from slot 0 to the highest seen, so a side with none left shows 0: `Species — seed 1 — tick 512 — figures 200 : 198`.
* **The host's sender** returns each state datagram's number (ADR-095 amendment), so the world server can match the fates the transport reports.
* **A restarted world.** A client that receives another seed's terrain builds it again; the same terrain again changes nothing (MVP-011).
* **Markers** (DBG-045 amendment): `net.replication.collect`, `net.replication.build` and `client.replica.apply`.

### Consequences

* Good, because the client and the server share one decoder: every tick of ADR-084's test checks that the replica equals the server's state, not only that the hashes agree.
* Good, because a lost datagram costs only its records, and no record ever waits for a whole tick to be acknowledged.
* Good, because what T13 to T17 draw is on the wire now: facing, task, order, flags, effects and orbs with their timers.
* Bad, because the server keeps an acknowledged state for every object of every client: about 40 bytes an object, 80 KB for the synthetic battle's 2,000 figures.
* Bad, because a stationary object that starts to move after 32 ticks goes whole, and is then drawn from its new state without interpolation for one tick.
* Bad, because the synthetic battle's mean of 9.1 KB a tick (91 KB/s) is above ADR-086's 64 KB/s skirmish cap. The in-process connection has no cap, and the remote client of M4 needs the tiers first.
* It forecloses: a per-client single baseline; a join that waits for a reliable snapshot before the first delta; extrapolation of any replicated object.

**The measurement in this task.** The figures come from the local harness (g++ 13 at -O2, the authored tuning record) and from a client that acknowledges every datagram. They are bytes on the wire, which do not depend on the machine.

| Scene | Join | Then a tick: mean, median, 95th percentile, most | Events, mean a tick |
|---|---|---|---|
| sandbox, 400 figures, 3,000 ticks | 6,219 bytes in 6 datagrams | 1,470, 1,429, 2,103, 2,490 bytes | 0 bytes: the sandbox's battle barely fights (ADR-107) |
| synthetic battle, 2,000 figures, 3,000 ticks | 31,014 bytes in 26 datagrams | 9,056, 9,164, 11,697, 12,790 bytes | 169 bytes |

The sandbox's 14.7 KB/s is within ADR-086's 23.4 KB/s for a battle view of 400 figures, which assumed the tiers. No delta in either run missed its baseline.

### Confirmation

* `GameSharedTests` round-trips every record kind and class, applies each delta to its baseline, counts each record's bits against the written ones, fills datagrams of 2,000 whole records to their limit, refuses a delta without a baseline, a short datagram and a delta of no field, and drives the replica through a lost tick, a late datagram, a missing baseline and a re-entry.
* `GameLogicTests` has three integration tests and the sender's own tests:
  - **ADR-084's M1 test.** The sandbox with an attached client, which steers its side by what its replica shows, gives the headless replay's hash on every one of 300 ticks, and its replica equals the server's state after every tick.
  - **A lossy link.** One state datagram in three lost, and a client that reads every third tick: no baseline is missing, every hash matches the replay, and the replica converges once the link is clean.
  - **A restart.** The new world's state is held from the tick of the restart.
  - **The sender.** Its baselines, the 32-tick window, removals and the team's own fields.
* `GameClientTests` checks the title's figure counts, the interpolation one tick behind, the hold and the wait, and the expiry of events.
* The owner's run on `Debug|ARM64` with `--seed 1` (M1 plan T12):
  - the title shows both sides at 200, and their counts fall once the sides fight;
  - after two minutes, the log line of the session's close reports no record without its baseline;
  - Ctrl+Shift+F11 still rebuilds the device;
  - the program closes cleanly.

## Pros and Cons of the Options

### 1 — per-object acknowledged states

* Good, because each record's baseline is as new as the transport allows.
* Bad, because the sender keeps a state per object and client, and a tick after a loss takes one more datagram for the older group.

### 2 — one baseline per client

* Good, because the sender keeps one tick of states per client.
* Bad, because one lost datagram of a tick costs the whole tick's baseline.

### 3 — whole state every tick

* Good, because it needs no acknowledgement and no history.
* Bad, because it costs about four times the bytes, and more for crowds that stand still.

## More Information

* **Left to later tasks.**
  - The client's own team and each team's colour, which T14's commands and T16's palette need.
  - A Leader's order target, for T17's flag pointer.
  - The server's tick cost, for T15's overlay.
  - The seen layer that narrows the interest set (M3, MVP-097).
  - The tiers and caps of ADR-086 items 2 and 4.
  - The blob stream of the join (M4).
* Related: [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md), [ADR-093](ADR-093-transport-interface-and-message-layout.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md), [ADR-106](ADR-106-leaders-formations-and-battle-commands.md), [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md).
* The rules: MVP-090, MVP-097 (interim), DEC-222 item 4, R-UNIT-185, R-UNIT-186, R-UNIT-191.
* The code is in GameShared (`ReplicatedState`, `StateUpdate`, `StateReplica`, `PresentationEvents`), GameLogic (`StateSender`, `WorldServer`), GameClient (`ClientSession`, `ReplicaTiming`) and NeuronServer (`ServerHost`).
* **Amendment 2026-10-21 (M1 task T17b, owner decision O78, [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md)).** A Leader's order target is replicated, for the flag pointer: `ReplicatedFigure::target` is the first point after the start of a Leader's Waypoint order route and none otherwise, sent for every team as field bit 64 of the figure delta, a presence bit and, when present, a whole position; a whole figure record carries the presence bit too. `StateUpdate::VERSION` is 2.
