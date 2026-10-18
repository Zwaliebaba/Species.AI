---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Server process architecture: one headless server library, composition roots and lifetimes, state owners, region jobs and a deadline-paced tick loop

## Context and Problem Statement

One server codebase runs integrated in the host's client and stand-alone on dedicated servers, behind the same transport interface, and the client never touches server state (O10, vision §5). The world is partitioned into regions from day one (O1, WLD-020), freezes while its server is not running (O4, O35), and keeps a stage-1 world running while the host's client stays open with its presentation suspended (PIL-077). The reference has one windowed process, process-wide state, an object reachable from everywhere, a single simulation thread, busy loops and a sequencer polled by whichever loop runs (DEC-221 to DEC-226). The deferred entries ask how the processes are composed and their state machines (DEC-221), who owns which state for how long (DEC-223), which threads run what (DEC-224), how the loops are paced (DEC-225) and how content is loaded and streamed (DEC-226). Process roles and the headless build (DEC-220) are area D1's; this record assumes its option A, separate build targets with a server library that links no presentation code.

How is the server built, composed, owned, threaded and paced, so that the integrated and the dedicated server are the same program?

## Decision Drivers

* One server library for the integrated server, the dedicated server and the test runner (O10, DEC-220).
* Every per-world and per-region object created and destroyed in one place, persisted without hidden state (DEC-223, DEC-251).
* Regions that step independently now and in separate processes later (O1, ADR-085).
* World time independent of any client's frame rate (DEC-220 item 9, DEC-225).
* Results independent of thread count and scheduling (ADR-081 Level 1).
* Server code that a second operating system's build can compile (gate B).

## Considered Options

* A — composition roots per process role with explicit lifetimes and state machines; one state owner per region, per world and per skirmish match; region steps as jobs; a fixed-rate server loop that sleeps to its deadline
* B — a service registry with run-time lifetime scopes; a fully parallel per-tick job graph across systems
* C — the reference structure: one root object, process-wide state, one simulation thread, client-driven stepping

## Decision Outcome

Proposed option: "A". **Default while Proposed:** the decision below.

1. **The server library** is headless, portable C++23: it includes no Windows header and reaches the operating system (sockets, files, threads, the clock of ADR-074, cryptography) only through the platform layer (PLT). It holds the state owners, the tick loop and its worker pool, command validation (ADR-087), interest management and replication (ADR-086), the persistence interface, the server API of O6 and the AI runtime. The client links it for integrated hosting and for skirmish; the dedicated server and the headless test runner link it with no presentation code (DEC-220 option A, D1).
2. **Composition roots** (DEC-221, option A). Each process role has one composition root that creates every service in a documented order and destroys them in the reverse order (PLT-038, PLT-043); no service is reached through a global. The lifetime scopes are: process; server run (one world from its opening to its freeze, or one skirmish lobby to its results); world (persisted across server runs); region; battle (inside its region); match (a skirmish match world, `MatchWorld`); client session (one connection of a client to a server); connection (on the server, one client). The integrated server's root is a child of the client's root: starting and stopping it never tears down the client, and stopping it freezes the world (O4, PIL-131).
3. **State machines.** The client: Boot → FrontEnd → Connecting → Loading → InWorld or InMatch → Leaving → FrontEnd, with quit accepted in every state and one teardown for every exit (PLT-042); the integrated host's client has, in InWorld, the presentation-suspended mode of PIL-077. A world server: Stopped → Opening (load and validate the world, every region) → Running → Freezing (the final checkpoint and the freeze record) → Stopped. A skirmish server: Lobby → Loading → Match → Results → Lobby. A state machine's transitions are the only way a lifetime scope begins or ends.
4. **State owners** (DEC-223, option A with B inside it). One owner holds every mutable simulation value of its scope: a region owner (`RegionSimulation`) per region, holding its settlements, figures, structures, battles, grids and routes; one world owner (`WorldSimulation`) per world, holding the world tick, the seed, the Cladans' grievances and relations, the register of claims and declared attacks, the battle slots, the keeper (WLD-125) and the world record; and one owner per skirmish match (`MatchWorld`). Simulation code holds no mutable static state (R-SIM-730). Each owner may store its objects in typed pools or a component store; either way it serialises canonically, with no hidden state, and it is the unit of persistence, of hashing (ADR-081) and of hand-over (ADR-085).
5. **Identity.** Every simulated object of a world has a stable 64-bit id, unique in the world for its life: the creating owner's id in its high 16 bits and that owner's creation counter in the low 48 bits. Messages, persistence, replication and random draws (ADR-082) name objects by stable id; pool slots and generational handles (R-SIM-131 to R-SIM-143) stay inside their owner.
6. **Threads** (DEC-224, option A). A server has a tick thread and a worker pool. In each tick the tick thread delivers the messages of the previous tick (ADR-085), runs every region owner's step as one job (regions in parallel, each serial inside in the reference update order of ADR-079), then the world owner's step, then the replication encoding per client as jobs, then hands the checkpoint buffers of the tick to the store thread. Network I/O runs on its own threads behind lock-free queues (DEC-207 item 9); logging is asynchronous (ADR-067). Inside one region step, work is split into parallel jobs only for a phase that reads the previous state and writes per-object outputs, merged in a fixed order, and only once a measurement shows the region step above half its budget (ADR-080). The integrated server runs its tick thread and at most 2 workers (ADR-080 item 7). The result of every tick is that of the serial order at any thread count (R-SIM-732).
7. **Pacing** (DEC-225, option A). The tick loop schedules tick *n* at the start time plus *n* tick periods and sleeps to that deadline; it never busy-waits and never takes its rate from a client frame. A tick that overruns lets the next start at once; the loop then runs owed ticks at most twice per period, up to 20 owed ticks (2 s). Beyond 20, the schedule is anchored again at the present, and the real time lost is not world time (O4); a gap of 10 s or more is recorded as a freeze with the cause "server stall" (PIL-131). Clients render at their own rate from replicated state (ADR-084) and never step the simulation.
8. **Loading and streaming** (DEC-226, option A for the world, B allowed for skirmish). A world opens by loading its records and its baked terrain chunks (WLD-166) on the worker pool with progress reported to the world host, and runs only when every region is loaded, so that every region steps the same tick. World generation and extension (WLD-165) run as background jobs, never inside a tick. A client streams terrain chunks by digest and content packs asynchronously with progress and explicit completion signals (DEC-226 items 2 and 3); a skirmish may show a blocking loading view. No load pauses a region, and the server keeps its clients' connections alive through every load.
9. **Checkpoints.** At the end of the tick in which a dirty region's checkpoint falls due, every 600 ticks (60 s of world time) on a phase fixed by its region id, the region owner serialises itself into a buffer, and the store thread writes it; freezing writes a final checkpoint of every owner, so a clean stop loses nothing and a crash at most 60 s of world time and one write. The store engine, its schema, migration, backups and rollback are area D4's (DEC-250, DEC-251).

### Consequences

* Good, because the integrated server is literally the dedicated server's library, started by a child root, so O10 holds by construction.
* Good, because region steps are independent jobs with messages between ticks, which is the same model across processes (ADR-085).
* Good, because world time depends only on the server's own loop, at every pace and in every stage.
* Bad, because every service and owner must be wired explicitly in its root, and every object needs a stable id from its creation.
* Bad, because a region's step is single-threaded until measured otherwise, so one region's cost is bounded by ADR-080's region budget rather than by the core count.
* Neutral, because skirmish keeps the reference's single match world, now as one owner of the same server.

### Confirmation

**Confirmed when:** the simulation-core milestone shows that a world and a skirmish match stepped in one process give the same per-tick hashes as each stepped alone (DEC-223 item 4); quit is tested from every state of both state machines; a forced stall of 5 s catches up within 5 s at most twice the rate, and one of 30 s is recorded as a freeze; and the dedicated server builds and runs its test world with no presentation library linked.

## Pros and Cons of the Options

### A — roots, owners, region jobs, a deadline loop

* Good, because ownership, order and pacing are explicit and testable, and the region split scales across cores and processes.
* Bad, because a region's step does not use more than one core until a phase is split.

### B — a registry and a full job graph

* Good, because it can reach a higher throughput per region.
* Bad, because order-dependent rules become hard to keep and to verify, and the registry hides dependencies.

### C — the reference structure

* Bad, because it contradicts the headless server, the separation of O10 and a world time independent of frame rates.

## More Information

* Resolves DEC-221, DEC-223, DEC-224, DEC-225 and DEC-226 when the owner confirms it at gate B; DEC-220 stays area D1's, and this record is its input. Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md).
* The type names `RegionSimulation` and `WorldSimulation` are proposed here; area D2 confirms them in the world simulation component.
