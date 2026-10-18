---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Commands: one validated path for every source, execution at the next tick, coalescing and rate limits

## Context and Problem Statement

Every change a player causes travels as a command (R-SIM-664), applied at the start of the tick that executes it (R-SIM-510). The reference forwards any command from any client and checks nothing but that a team is named: a modified client can order another team's figures, change the match options or start the match, and a missing field reads as a valid-looking default (DEC-202 items 1 to 3). Its continuous control state is latest-wins per tick, which loses the one-shot actions inside an earlier message (DEC-203 item 1). In Species the sources of commands are the game client of a present steward, the deputy and AI settlements on the server (PIL-043), the authenticated server API for status and orders (O6, PIL-075), and the world host's administration (PIL-133, D9). DEC-202 asks which checks a command passes; DEC-203 asks when it takes effect and how inputs are carried.

What path does every command take, and when does it execute?

## Decision Drivers

* No forged or out-of-phase command changes the world (DEC-202).
* The deputy, the AI and the O6 interface pass the same checks as a player (DEC-202 item 9).
* A command's effect is defined when its target has changed since it was issued (DEC-202 item 7, DEC-555).
* Latency that does not depend on a client's frame rate (DEC-203 item 3), and one-shot actions never lost.
* A flood is dropped and reported, never queued (DEC-202 item 10).

## Considered Options

* A — validate every command on arrival: identity, ownership or role, phase, argument ranges and ids
* B — A, plus revalidation when the command executes, with a defined no-op when its target has changed
* C — trust the client for low-risk commands and validate the rest

## Decision Outcome

Proposed option: "B" for validation, with DEC-203's option B for timing. **Default while Proposed:** the decision below.

1. **One path.** A command from any source (the client of a present steward, the deputy, an AI settlement, the server API, the world host's administration, a debug operator) is a typed message from an authenticated source, and enters the simulation only through the command queue of the state owner it addresses (the region of the settlement or expedition it names, the world owner for world-level commands such as declaring an attack, or the skirmish match; ADR-083 item 4), which applies its queue at the start of its step. No source has a shortcut: the deputy's and the AI's orders of the world game are commands that execute at the next tick like a steward's. Skirmish bots keep the reference rule that a bot's commands execute at once inside the step (R-SIM-690), because they are part of the simulation.
2. **On arrival** the server checks, and a failed check drops the command with a reason code: the sender's identity and session; its right over the addressed settlement, team, expedition or Leader (a steward commands only its own settlement, PIL-006), or its role for host and debug commands; the phase (lobby, match, world, battle) and the steward's presence where the command needs it; every field present, typed and in range, a missing or mistyped field rejecting the command (DEC-202 item 3); every id a stable id with its generation, never a pool slot and never an in-band sentinel (DEC-202 item 4).
3. **On execution**, at the start of its tick, the command is checked again against the state of that tick; a command whose target no longer exists or no longer qualifies (an empty group, a dead Leader, a site no longer free) does nothing, as its handler defines (DEC-555, R-SIM-715 item 2). Executing a command never issues a further player command (R-SIM-665).
4. **Timing.** A command executes in the first tick that starts after the server receives it; the client chooses no tick, and the server adds no input delay. Commands of one tick execute in the order of their arrival at the server, which keeps each connection's order. The server reports each player command's outcome (executed with its tick, or rejected with its reason) to its sender.
5. **Coalescing** (DEC-203 item 1). The continuous control state (cursor, stick vectors, the selection) is sent once per tick period and is latest-wins at the server, held when a tick receives none; every one-shot action (a click, an order, fire, a Power use) is a discrete command, delivered exactly once and in order on the reliable channel (ADR-088).
6. **Rate limits.** Per client: at most 20 discrete commands in a tick and 60 per second sustained (a token bucket), and one control state per tick; per steward on the server API: at most 10 calls per minute, which area D6 may refine. Excess is dropped, counted and reported to the sender and in the server's diagnostics; duplicates within a tick, by the sender's sequence number, execute once.
7. **Local feedback** (DEC-203, option B): the client shows the order marker, the selection and the route preview at once (ADR-084) and never predicts a figure's movement.
8. **Accountability.** Every command of the world host's administration and every debug command is recorded in the world record with its tick and its sender (PIL-133, D9); a session with any debug command carries the cheats-used marker (DBG-118).

### Consequences

* Good, because forgery, out-of-phase commands and missing fields fail on arrival, and a stale target fails safely on execution.
* Good, because the server API of O6 and the deputy are tested by the same checks as the game client.
* Good, because the latency is the wait for the next tick plus the network, independent of any frame rate: 187 ms on the integrated host and 267 ms at a 50 ms round trip, mean, in the model (ADR-079).
* Bad, because every handler states its result for a changed target, and every rejection has player-facing text (C2, C3).
* Bad, because the world's AI loses the reference bots' same-tick execution, so its orders take effect one tick later than a skirmish bot's.

### Confirmation

**Confirmed when:** the networking stage of the first playable rejects every forged command of a test client (another team's order, a lobby command in a match, a missing field, a pool slot as an id, a flood of 1,000 commands in a tick) with its reason and its counter; a crowd order for a group emptied between issue and execution does nothing; and a server-API order and the same order from the client produce the same world state.

## Pros and Cons of the Options

### A — validation on arrival

* Good, because it stops forgery.
* Bad, because a command valid at issue can be invalid at execution.

### B — arrival and execution

* Good, because it also defines every stale command's result.
* Bad, because every handler carries an execution-time check.

### C — partial trust

* Bad, because any trusted path is a cheat path in a world that persists.

## More Information

* Resolves DEC-202 and DEC-203 when the owner confirms it at gate B. Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md).
* Area D9 specifies the threat model and the community host's trust; area D6 the server API's versioning and its limits.
