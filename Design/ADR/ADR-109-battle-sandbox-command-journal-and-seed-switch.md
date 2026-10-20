---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T9)
---

# The battle sandbox, the command journal and the seed switch

## Context and Problem Statement

Task T9 of the [M1 plan](../implementation/m1_plan.md) makes the world that `Species.exe` starts at M1: the sandbox of the plan's §6, with its `mvp.sandbox.*` keys. A development switch sets its seed (MVP-005), and a development command restarts it. The task also writes the command journal in its first form (MVP-076) and runs the determinism tests of ADR-081, ADR-082 and ADR-083.

These decisions are open:
- how a sandbox is made, and who commands its sides;
- what the journal holds, how it is replayed, and where it is written;
- how the seed reaches the server from the command line;
- how the restart reaches the world.

## Decision Drivers

* One build reproduces a run from its start and its commands, bit for bit (MVP-093, ADR-081 Level 1).
* Every command takes the one validated path and executes at the next tick (MVP-094, ADR-087).
* The simulation key never leaves the server (ADR-082 item 1).
* A development feature is compiled out of a Release build (PLT-107), and the command line is read by PLT-022.
* Every number is a tuning value (MVP-099).

## Considered Options

* 1 — the journal as the arrivals a world's queues accept, with the stewards' connections, from the world's start; replayed by submitting them to the same queues before the same ticks
* 2 — the journal as the commands each queue executed, with their outcomes, taken after each tick
* 3 — the journal as every message a connection sent, refused ones included

## Decision Outcome

Chosen option: "1".
- Option 2 needs a queue to hand out what it executed, which it does not keep. It records nothing that option 1's replay does not make again: a command refused on execution is refused again.
- Option 3 records traffic that never reached the simulation, and a hostile client could fill it.

* **The sandbox.** `WorldServer::Desc::sandbox` makes every new world of the server the sandbox. At M1 both the client's integrated server and the headless server set it.
  - `StartWorld` makes a new world: it attaches region 0's terrain (ADR-102) and then calls `PopulateSandbox`.
  - The steward's side is team 0, at site `mvp.sandbox.steward_site` (0). The AI's side is team 1, at site `mvp.sandbox.ai_site` (3), diagonally opposite in WLD-040's local order (O70).
  - Each side is `mvp.sandbox.side_workers` (200) Workers, with no Leader, spawned within `mvp.sandbox.side_spread` (60 wu) of its site's centre.
  - The AI of team 1 is added as ADR-107 states.
  - A steward who connects to a sandbox becomes team 0's commander (ADR-106), so the latest steward commands it.
  - The Cladans' colours are presentation and arrive with the client's battle view.
* **The synthetic battle.** `PopulateSyntheticBattle` spawns `mvp.perf.synthetic_figures` (2,000) Workers, half at each sandbox site, with an AI on each side. It is ADR-080 item 2's synthetic battle. The tests and T10's scenes use it, and no server starts it.
* **The journal** (`CommandJournal`) holds a new world's start and every connection and accepted command since, in arrival order.
  - The start is the simulation key, the seed that passed, the first tick, the region count and the sandbox flag.
  - A connection entry stands for a steward's connection, which takes the world owner's next id and, in a sandbox, commands team 0. `ConnectSteward` does both, for the server and the replay alike.
  - A command entry holds the tick it executes in, its owner (0 for the world owner, *r* + 1 for region *r*), its sender, its sequence number and its bytes.
  - An AI's commands are not journaled: the replayed regions make them again. A command its queue refuses on arrival is not journaled either: it changes no state.
  - `ReplayJournal` starts the world and, before each tick, connects and submits that tick's entries in order, then steps the tick. So the replay passes through the same validation and the same keyed order as the run it records (ADR-087 item 4).
* **The journal file.** It holds a format version, the start and the entries, canonically (ADR-081 item 7).
  - A freezing server writes it beside its snapshots, as `commands.journal`, by the atomic replace of ADR-099. A file that does not read whole is refused.
  - The file holds the key, so it is kept as a snapshot is: never shown, sent or uploaded (ADR-082 item 1).
  - MVP-076 asks for the commands since the last snapshot. The first form holds those since the world started. A world restored from a snapshot has no journal: that comes with M2's periodic saving, which cuts the journal at each snapshot.
  - Neither M1 program has a snapshot folder yet, so no M1 run writes the file. The tests write it and replay it.
* **The seed switch.** `--seed <number>` sets a new world's seed, a decimal number of at most 64 bits, in a development build (table 3.3-A, PLT-107).
  - `ParseArguments` in NeuronCore reads it with `--loose-content` (ADR-108), from the arguments each program reads at start-up.
  - Each unknown argument is logged and ignored. In a Release build `--seed` and its value are unknown arguments.
  - With no seed the server draws one of 32 bits, as before (MVP-005).
* **The restart** is the development command `SandboxRestart` (0x010E), with one flag: a new seed or the same one. It does not enter a queue; the world server acts on it as it arrives, between ticks.
  - It first collects the outcomes of the refusals already made, then starts a new world from the next tick: the same or a drawn seed, a new key and a new journal. It connects every steward to the new world in connection order, and sends each the new terrain.
  - What arrives after it enters the new world. A command that the replaced world had accepted for the next tick goes with that world, without an outcome.
  - The restart's own outcome is `Applied`, or `NoEffect` when no seed the re-roll reaches gives a region. A server that runs no sandbox, or any Release build, refuses it as an unknown type.
* **The determinism tests** are the M1 parts of ADR-081's, ADR-082's and ADR-083's confirmations; see Confirmation.

### Consequences

* Good, because a defect seen in the sandbox reproduces from its journal on the same build, tick by tick.
* Good, because the replay takes the server's own path: the journal records arrivals, not effects.
* Bad, because the first form grows from the world's start, and a restored world has none until M2 cuts it at each snapshot.
* Bad, because a restart drops the commands accepted in the same tick before it without an outcome. The client reads no outcome yet, and the restart exists in development builds only.
* It forecloses: a journal that records effects rather than commands; a seed switch in a Release build.

### Confirmation

* `NeuronCoreTests` checks:
  - the seed in any position, its 64-bit bound and its spelling, and every unknown argument kept in order (PLT-022, PLT-107);
  - ADR-082 item 8's quality test at its full size: 100,663,296 values over 32 purposes, 1,024 sequential ticks, 192 sequential subjects and ordinals 0 to 15, with each bit and each byte balanced and successive values uncorrelated within six standard deviations.
* `NeuronServerTests` checks a stall of 2 s on the injected clock: the owed ticks run at most twice per period, none is given up, and tick 49 is on its deadline 1.9 s after the stall ended. The stall of 30 s recorded as a freeze is M0's test.
* `GameLogicTests` checks:
  - the sandbox's two sides, their sites, its AI and the steward's side;
  - the same hash every tick for the sandbox (300 ticks) and the synthetic battle (100 ticks) at 1, 2 and 8 worker threads as serially;
  - a cosmetic event emitted between figures' advances on every tick, which leaves every hash unchanged;
  - extra draws of each purpose in turn, through a tick's draw contexts, which leave every other purpose's values unchanged;
  - the journal file's round trip and its refusals;
  - a sandbox with a steward's commands, replayed from its journal with the same hash on every tick;
  - the journal written at a freeze, and none for the restored world;
  - the restart with the same seed and with a new one, and its refusal by a server that runs no sandbox.

## Pros and Cons of the Options

### 1 — accepted arrivals, replayed through the queues

* Good, because the replay runs the same validation, ordering and execution as the run it records.
* Bad, because an entry's effect is known only by replaying it.

### 2 — executed commands with outcomes

* Good, because the journal says what each command did.
* Bad, because the queue would have to keep what it executed, and the replay would still run them through it.

### 3 — every message received

* Good, because nothing a client sent is lost.
* Bad, because the journal grows with traffic the simulation never saw.

## More Information

* Related: [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md), [ADR-106](ADR-106-leaders-formations-and-battle-commands.md), [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md).
* The rules: MVP-005, MVP-076, MVP-093 and MVP-094 ([MVP scope](../implementation/mvp_scope.md)), PLT-022 and PLT-107 ([platform layer](../architecture/components/platform_layer.md)), the sandbox of the [M1 plan](../implementation/m1_plan.md) §6.
* The code is in GameLogic (`Sandbox`, `CommandJournal`, `WorldServer`), GameShared (`SandboxRestartCommand`), NeuronCore (`StartupOptions`) and the two programs.
