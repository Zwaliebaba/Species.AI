---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M0 task T6)
---

# Server host, tick loop and command queue: NeuronServer's first form

## Context and Problem Statement

ADR-079 item 2, ADR-083 items 2, 3, 6 and 7 and ADR-087 decide the server's loop and its command path:
- whole ticks;
- a composition root per process role;
- a world server's state machine;
- a tick thread with a worker pool;
- a loop that sleeps to its deadlines and catches up within bounds;
- one validated command path per state owner, executed at the next tick in a keyed sender order.

ADR-090 puts all of it in NeuronServer, in the `Neuron` namespace, which knows no game concept (R9). M0 task T6 builds NeuronServer's composition root, the deadline-paced loop at 10 ticks per second and the command path. Its done-when is that a headless test runs 600 ticks on the deadline (MVP-092, MVP-094).

The decisions leave open:
- where the engine ends and the game's state owners begin;
- how the catch-up rule reads at its edges;
- how a test runs a minute of ticks in no minute;
- what a queued command holds, and where its sequence number comes from;
- the message of the keyed sender order;
- which parts of ADR-087 M0 builds.

## Decision Drivers

* The integrated server and the headless server are one library and one host (O10, ADR-083 item 2).
* The engine knows no game concept (R9, ADR-090 item 3).
* Results are those of the serial order at any thread count (R-SIM-732, ADR-081 Level 1).
* A test of the loop is exact and fast, and never depends on a machine's timer.

## Considered Options

* 1 — a host that runs a `ServerWorld` interface; a pure schedule; one queue per owner that holds encoded commands; a fork-join worker pool
* 2 — a host that knows the regions and the world owner directly
* 3 — option 1 with each command decoded once on arrival and kept as a typed object

## Decision Outcome

Chosen option: "1".
- Option 2 would put game concepts in the engine (R9).
- Option 3 would keep decoded values that may refer to the buffer they came from. Decoding again at execution cannot fail on bytes that decoded once, and its cost is unmeasured at M0's command rates.

* **The host** (`Neuron::ServerHost`) is the composition root of one server run (ADR-083 item 2).
  - It creates the worker pool, the tick loop and the tick thread `server.tick`, in that order, and ends them in the reverse order.
  - It holds the connections it accepts from a `TransportListener` and numbers them from 1.
  - Its states are those of ADR-083 item 3: Stopped, Opening, Running, Freezing, then Stopped again. A world that fails to open leaves the host stopped and reaches the caller.
* **The seam** is `Neuron::ServerWorld`, which the game's world implements (T7). The host calls it in the order of ADR-083 item 6:
  - at the start of each tick, every new connection, every message and every datagram fate that arrived before the tick began, and every closed connection;
  - then the step of the tick, with the worker pool for the region jobs;
  - then the publication of the tick to every connection;
  - and outside the ticks, the opening, the freeze and every stall.
  - NeuronServer's own sources include no Windows header; `Build\CheckProjectFiles.py` rule 14 checks this for NeuronServer, GameShared and GameLogic (ADR-083 item 1).
* **The schedule** (`TickSchedule`) is pure arithmetic on clock readings.
  - Tick *n* is due at the anchor plus *n* periods, and never starts before its deadline.
  - A late loop starts its owed ticks no closer than half a period apart, twice the rate.
  - At up to 20 owed ticks it catches up. Above 20 the schedule starts again at the present: the tick that starts is due now, its number is the next one (no tick number is skipped), and its lateness is given up.
  - A stall of 10 s or more is a freeze with the cause "server stall" (PIL-131). The host passes every stall to the world, which records freezes in the world record from M2.
* **The loop** (`TickLoop`) sleeps through `PlatformClock::WaitUntil`, which never busy-waits.
  - A test passes its own wait, which moves a manual time source to the deadline (PLT-048). 600 ticks, a minute of world time, then run in no minute of real time, each at its exact deadline.
  - A second test on the real clock checks that no tick starts early.
  - The tick period is a parameter; the tuning record provides `ref.sim.tick_period` from T9 (O66).
* **The command queue** (`CommandQueue`) is the queue of one state owner (ADR-087 item 1).
  - On arrival it checks, in this order:
    - the size against `MESSAGE_MAX_BYTES`;
    - the header;
    - the type;
    - the version;
    - the type's own size limit;
    - the whole decoding;
    - the sender's right and the phase, as a check the game registers;
    - the sender's sequence number, which a sender may use once per tick.
  - A failure drops the command with one of six reasons, each counted and reported as an outcome. A refusal names the tick it would have executed in.
  - The queue keeps the bytes of a queued command, and executes it at the start of the next tick that the owner steps. A command submitted while a tick executes, by the deputy or an AI settlement, waits for the next tick like any other.
  - The handler reports an effect or none, for a target that has changed (ADR-087 item 3). Every outcome carries its tick.
* **The keyed order** of a tick's senders is `SipHash24Words(key, {3, tick, sender})`: the domain 3 of ADR-082 item 5, the tick and the sender's stable id, as three little-endian 64-bit words.
  - Senders execute in ascending order of that value, ties broken by the sender's id, so the order is total (ADR-081 item 2).
  - A stable sort keeps each sender's commands in their arrival order.
  - The key is the world's simulation key, drawn with `DrawSecretKey` (the system's cryptographic generator) when the world is created (ADR-082 item 1).
* **Sequence numbers.** On a reliable channel, delivery is exactly once and in order, so the server numbers each connection's commands as they arrive, and the client counts the same. The sequence number on the wire comes with reconnection at M4, the first time a command can arrive twice.
* **SipHash-2-4** is in NeuronCore, in plain C++ with integer arithmetic only, tested against the 64 published vectors (ADR-082 item 2).
* **The worker pool** (`WorkerPool`) runs a phase's jobs on its workers and on the tick thread.
  - It returns when every job has run, and rethrows the first exception after all have finished.
  - The integrated server has at most 2 workers (ADR-080 item 7). With none, the tick thread runs every job.
* **Markers.** The loop and the host carry `server.tick`, `server.receive`, `server.step` and `server.publish`, added to the catalogue of DBG-045.
* **Not in T6**, each with where it arrives:
  - the rate limits of ADR-087 item 6, with the server API and remote clients after the MVP (ADR-087's confirmation);
  - the outcome message to the client, with the first client command at M1;
  - the skirmish host delay, with skirmish;
  - the consistent cuts of ADR-083 item 9, for which the MVP's snapshot (MVP-075) stands in.

### Consequences

* Good, because the integrated and the headless server are the same host with the same world behind it, and the engine names no game type.
* Good, because the loop's rules are tested exactly, at every edge, and a minute of ticks costs the suite nothing.
* Good, because no source can reach an owner but through its queue, and every refusal has a reason and a counter.
* Bad, because each command is decoded twice.
* Bad, because a stall of between 2.1 s and 10 s is given up without a record. Only a gap of 10 s or more is a freeze (PIL-131).
* It forecloses: a tick taken from a client's frame; a command that skips its owner's queue; an engine type that names a game concept.

### Confirmation

* `NeuronServerTests` runs:
  - the schedule's deadlines, its catch-up at twice the rate, its new start above 20 owed ticks and the 10 s freeze;
  - 600 ticks of the loop on the deadline, an overrun, a 30 s stall recorded as a freeze, no early start on the real clock, and a stop request;
  - 600 ticks of the host with three region jobs on two workers;
  - a client's connection, message, reply, datagram fate and close, and a world that fails to open;
  - the next-tick execution, the keyed order's shares over 1,000 ticks within 45 % to 55 %, each sender's order, the six refusals and a command whose target has gone.
* `NeuronCoreTests` checks SipHash-2-4 against the published vectors.
* A `CheckProjectFiles.py` fixture fails on a Windows header in NeuronServer.
* ADR-083's confirmation at M1 measures the loop under a real stall.

## Pros and Cons of the Options

### 1 — a world interface, a pure schedule, encoded commands, a fork-join pool

* Good, because each part is tested alone and the engine stays game-free.
* Bad, because the world interface grows as the server does.

### 2 — a host that knows the owners

* Good, because nothing stands between the host and the regions.
* Bad, because the engine would know game types.

### 3 — commands kept decoded

* Good, because a command is decoded once.
* Bad, because a decoded view into a message's buffer must outlive the buffer.

## More Information

* Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-093](ADR-093-transport-interface-and-message-layout.md).
* **The stall of ADR-083's confirmation.** ADR-083 item 7 catches up at most 20 owed ticks (2 s) and starts the schedule again beyond them. Its M1 confirmation asks that a 5 s stall catch up within 5 s at twice the rate. Under item 7, that stall is given up rather than caught up. This record follows item 7, and the owner kept its bound (O68): ADR-083's confirmation is amended so that M1 tests a stall of 2 s caught up within 2 s, and one of 30 s recorded as a freeze.
* The code is in NeuronServer (`ServerHost`, `TickSchedule`, `TickLoop`, `CommandQueue`, `WorkerPool`) and NeuronCore (`SipHash`, `SecureRandom`).
* **Amendment 2026-10-20 (M1 task T7, [ADR-106](ADR-106-leaders-formations-and-battle-commands.md)).** The outcome message exists: after each tick the world server sends each steward a `CommandReport` for each of its commands, executed or refused, with its sequence number and tick. `CommandQueue::Handles` tells the server which owner's queue takes a message's type.
* **Amendment 2026-10-20 (M1 task T9, [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md)).** The world server journals each connection and each command a queue accepts, with the tick it executes in, from a new world's start, and writes the journal beside its snapshots when it freezes. A replay submits the entries to the same queues before the same ticks.
* **Amendment 2026-10-20 (M1 task T12, [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md)).** `ConnectionSender::Send` returns the number of a state datagram, 0 on the reliable channel and none when the connection cannot take the message, so that the world server matches the fates of `DatagramFates` to what it sent.
* **Amendment 2026-10-22 (M2 task T9, [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md)).** The world records each stall that froze it in its world record, with the tick and the whole seconds the loop gave up, and the record is saved with the world.
