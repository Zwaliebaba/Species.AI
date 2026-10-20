---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M0 task T8)
---

# Keyed draws and the state hash: SimRng, the draw ledger and the owners' canonical state

## Context and Problem Statement

ADR-082 fixes the simulation's draws: SipHash-2-4 of the world's secret key over the domain, the purpose, the subject, the tick and the ordinal, with the draw forms of item 3.

ADR-081 item 8 gives each state owner a 64-bit hash of its canonical serialisation. Debug and test runs compute it every tick, and DBG-147 keeps golden hashes that change only with a reviewed change.

M0 task T8 builds both. Its done-when is that the hash of 600 ticks from one seed is stable across runs (MVP-093, DBG-147).

The decisions leave open:
- where `SimRng` lives, given that purposes are the game's;
- how a second context for the same keys is caught;
- which hash function and key the state hash uses;
- what the canonical state of an owner is;
- whether the simulation key is part of it;
- how the owners' hashes combine.

## Decision Drivers

* The engine names no game concept (R9), and the draw purposes are the game's.
* The state hash is the same on every build of one release that runs the same code (ADR-081 Level 1, and Level 2 by construction at M0, whose state is integers).
* The simulation key is never shown, sent or logged (ADR-082 item 1).
* One encoding for state, so the hash and the snapshot of T10 cover the same bytes.

## Considered Options

* 1 — `SimRng` in NeuronCore with a numeric purpose; a per-owner draw ledger in development builds; SipHash-2-4 under a public zero key over each owner's state in the bit layout of ADR-093
* 2 — option 1 with FNV-1a or xxHash64 as the state hash
* 3 — option 1 with one hash over the whole world's bytes instead of one per owner

## Decision Outcome

Chosen option: "1".
- Option 2 adds a second hash function where SipHash is already tested against its published vectors.
- Option 3 cannot compare one region at a hand-over, which ADR-081 item 8 needs (ADR-085 item 8).

* **`SimRng`** (NeuronCore, beside SipHash, ADR-090 table 1) is a draw context for one purpose, one subject and one tick.
  - Each value is `SipHash24Words(key, {1, purpose, subject, tick, ordinal})`, and the ordinal counts the context's draws.
  - The forms are those of ADR-082 item 3: `NextValue`, `NextUint`, `NextUnit`, `NextSigned` and `NextBelow` for a count of at most 2³².
  - The purpose is a number: the game's `DrawPurpose` enumeration arrives with M1's first draw site. A subject is a stable id and never 0.
* **The draw ledger** (`DrawLedger`) is one owner's record of the contexts made in its current tick.
  - In development builds a second context for the same purpose, subject and tick is a development check failure (ADR-082 item 3).
  - Release builds pass no ledger and check nothing.
* **Canonical state.** Each owner writes its mutable values with the `BitWriter` of ADR-093, in a fixed order:
  - the world owner: the world tick, its id counter and its queued commands;
  - a region: its index, its tick clock, its id counter and its queued commands;
  - a queued command: its sender, its sequence number and its bytes, in arrival order (`CommandQueue::WriteState`).
  - At M0 every field is a whole number of bytes on a byte boundary, so the state is the little-endian bytes of its fields (ADR-081 item 7).
  - The simulation key is no part of it: a snapshot saves the key apart (T10), and no hash covers it.
* **The state hash** is `SipHash24` under a public key of zeros (`STATE_HASH_KEY`) over an owner's state.
  - The key is no secret: the hash detects divergence and authenticates nothing.
  - `HashWorld` gives one hash per owner, the world owner's first and then each region's in index order. The world's hash is SipHash over the owners' ids (16 bits) and hashes (64 bits) in that order.
* **When.** In development builds the world server hashes the world after every tick and keeps the last hashes, logged at trace. A Release server hashes at its saves (T10), as ADR-081 item 8 says.
* **The golden run** steps a world from a fixed key, the seed, for 600 ticks, with one region and with three.
  - It records the last world hash and a chain: SipHash of every tick's world hash as little-endian words.
  - The values were computed by an independent Python model of the layout and of SipHash, and the C++ build gives the same values.
* **Checker.** `Build\CheckProjectFiles.py` rule 14 rejects `SimRng.h` outside NeuronCore, GameLogic and the test projects (VFX-005, UI-002, DEC-222). The simulation's other headers are GameLogic's, which rules 11 and 13 already keep from the client.
* **Not in T8:**
  - the quality test of ADR-082 item 8 at 10⁸ values. T8 runs its first form at 2²⁰ values, with bounds above six standard deviations; the full test is the M1 confirmation.
  - the field-level diff of ADR-081 item 8, with the first state that has fields to compare.

### Consequences

* Good, because a draw depends on its own keys only, so parallel regions draw without coordinating, and the hash shows it: the same at 0, 1, 2 and 8 workers.
* Good, because one encoding serves the hash and the snapshot.
* Bad, because a golden value changes with every change to an owner's state layout. DBG-147 makes each such change a reviewed one that names its rule.
* Bad, because the key-of-zeros hash is no integrity check: a snapshot needs its own (T10).
* It forecloses: a draw from a sequential generator in the simulation; a state hash that covers the simulation key.

### Confirmation

* `NeuronCoreTests` checks:
  - four reference draws;
  - the forms' ranges;
  - a purpose's values unchanged by draws of another;
  - the balance of 2²⁰ values over the step's key patterns;
  - the ledger's second context.
* `GameLogicTests` checks the golden hash and chain of 600 ticks with one and three regions, the same hashes at 0, 1, 2 and 8 workers, a hash that covers the state and not the key, and an issued id that changes only its owner's hash.
* A `CheckProjectFiles.py` fixture fails on `SimRng.h` in GameClient.
* ADR-082's and ADR-081's confirmations at M1 run the quality test at 10⁸ values and the battle sandbox's hashes.

## Pros and Cons of the Options

### 1 — SipHash under a zero key, per owner

* Good, because one tested function serves draws, orders and hashes.
* Bad, because SipHash costs more than a non-keyed hash per byte; at M0's few bytes per owner the cost is not measurable.

### 2 — FNV-1a or xxHash64

* Good, because either is faster per byte.
* Bad, because either is a second function to implement and test.

### 3 — one hash for the world

* Good, because it is one call.
* Bad, because a region cannot be compared alone.

## More Information

* Related: [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-093](ADR-093-transport-interface-and-message-layout.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md).
* The code is in NeuronCore (`SimRng`), NeuronServer (`CommandQueue::WriteState` and `ReadState`) and GameLogic (`StateHash`, the owners' `WriteState`).
* **Amendment 2026-10-20 (M1 task T4, [ADR-103](ADR-103-region-grids-draw-contexts-and-figure-movement.md)).** The game keeps each draw context for its whole tick (`TickDraws`): the first use of a purpose for a subject makes the context, and every later use in that tick continues it, so the ledger's rule that a second context is a defect still holds when one rule draws twice.
