---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead; proposed by design-phase area A2 (gate B asked no question of it)
---

# Commands: one validated path for every source, execution at the next tick in a keyed sender order, immediate control states, coalescing and rate limits

## Context and Problem Statement

Every change a player causes travels as a command (R-SIM-664), applied at the start of the tick that executes it (R-SIM-510). The reference forwards any command from any client and checks nothing but that a team is named: a modified client can order another team's figures, change the match options or start the match, and a missing field reads as a valid-looking default (DEC-202 items 1 to 3). Its continuous control state is latest-wins per tick, which loses the one-shot actions inside an earlier message (DEC-203 item 1). In Species the sources of commands are the game client of a present steward, the deputy and AI settlements on the server (PIL-043), the authenticated server API for status and orders (O6, PIL-075), and the world host's administration (PIL-133, D9). The integrated host's commands reach the server in microseconds, a remote steward's after half a round trip, so an order by arrival would let the host win every race decided in one tick. DEC-202 asks which checks a command passes; DEC-203 asks when it takes effect and how inputs are carried.

What path does every command take, and when and in which order does it execute?

## Decision Drivers

* No forged or out-of-phase command changes the world (DEC-202).
* The deputy, the AI and the O6 interface pass the same checks as a player (DEC-202 item 9).
* A command's effect is defined when its target has changed since it was issued (DEC-202 item 7, DEC-555).
* Latency that does not depend on a client's frame rate (DEC-203 item 3), and one-shot actions never lost.
* Network latency, and the host's lack of it, does not decide a race inside one tick.
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
4. **Timing and order.** A command executes in the first tick that starts after the server receives it; the client chooses no tick, and the server adds no input delay. The commands of one tick are grouped by sender (a steward, the deputy of a settlement, an AI settlement, the world host, the server API caller); the senders execute in the ascending order of a keyed value of the tick and the sender's stable id (ADR-082 item 5, domain 3), which no client can compute and which changes every tick; each sender's own commands keep their order. A race inside one tick is therefore decided by neither latency nor sender id. In skirmish a match option delays the integrated host's commands by the median one-way latency of the remote players, measured in the lobby; B9 sets its default. The server reports each player command's outcome (executed with its tick, or rejected with its reason) to its sender.
5. **Control states and coalescing** (DEC-203 item 1). The continuous control state (cursor, stick vectors, the selection) is sent at once when it changes and at least once per tick period, and is latest-wins at the server, held when a tick receives none; so direct control that is not predicted (ADR-084 item 6) has the latency of an order. Every one-shot action (a click, an order, fire, a Power use) is a discrete command, delivered exactly once and in order on the reliable channel, a QUIC stream of MsQuic (ADR-088 item 2).
6. **Rate limits.** Per client: at most 20 discrete commands in a tick and 60 per second sustained (a token bucket), and at most 3 control states per tick period (3.0 KB/s upstream with the commands, within the 4 KB/s of ADR-086 item 4); per steward on the server API: at most 10 calls per minute, which area D6 may refine. Excess is dropped, counted and reported to the sender and in the server's diagnostics; duplicates within a tick, by the sender's sequence number, execute once.
7. **Local feedback** (DEC-203, option B): the client shows the order marker, the selection and the route preview at once, and predicts only the movement of the unit that its steward directly controls (ADR-084 item 6).
8. **Accountability.** Every command of the world host's administration and every debug command is recorded in the world record with its tick and its sender (PIL-133, D9); a session with any debug command carries the cheats-used marker (DBG-118).

### Consequences

* Good, because forgery, out-of-phase commands and missing fields fail on arrival, and a stale target fails safely on execution.
* Good, because the server API of O6 and the deputy are tested by the same checks as the game client.
* Good, because the latency is the wait for the next tick plus the network and the client's frames, independent of any frame rate at the server: mean / 95th percentile 212 / 258 ms on the integrated host and 302 / 350 ms at a 50 ms round trip in the model (ADR-079).
* Good, because a same-tick race is decided by a keyed order that favours no connection, so the integrated host gains nothing from its zero latency inside a tick.
* Bad, because every handler states its result for a changed target, and every rejection has player-facing text (C2, C3).
* Bad, because the world's AI loses the reference bots' same-tick execution, so its orders take effect one tick later than a skirmish bot's.
* Neutral, because a remote steward still sees its order half a round trip later than the host does; the keyed order removes the race, not the delay.

### Confirmation

**Confirmed when** (amended 2026-10-19 for the MVP's milestones, O51 and O55): on the MVP's M3 build the server rejects every forged command of a test client (another settlement's order, a missing field, a pool slot as an id) with its reason and its counter, a crowd order for a group emptied between issue and execution does nothing, and the steward and an AI settlement whose commands reach one tick win a contested action in shares within 45 % to 55 % over 1,000 ticks (MVP-094); at M4 the same test client is refused over MsQuic, and a deputy's order and the same order from the client produce the same world state; and at the first milestone after the MVP with the rate limits, skirmish and the server API of O6, which the MVP does not build (its §5.1), the server rejects a lobby command in a match and a flood of 1,000 commands in a tick with their reasons and counters, a server-API order and the same order from the client produce the same world state, and two remote senders win a contested action within the same shares whatever their latency.

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

* Resolves DEC-202 and DEC-203 (Resolved 2026-10-19). Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md).
* Area D9 specifies the threat model and the community host's trust; area D6 the server API's versioning and its limits.
* The latency figures are `Tools/DesignModels/A2ServerBudget.py latency`.
* **Amendment 2026-10-18 (challenge round 1).** Item 4 orders the senders of a tick by a keyed value instead of by arrival and adds the skirmish host delay (A2-F12); item 5 sends a changed control state at once (A2-F03); item 7 admits the predicted unit (A2-MB-1); the latency figures follow the revised model.
* **Amendment 2026-10-19 (after owner gate B, O44; O51, O55).** The reliable channel of item 5 is a QUIC stream of MsQuic (O44, ADR-088); nothing else in this record changes. The "Confirmed when" is re-pointed to the MVP's M3 and M4 and, for the rate limits, skirmish and the server API, to the first milestone after it.
* **Amendment 2026-10-23 (M3 task T9, [ADR-164](ADR-164-the-command-paths-confirmation-the-valley-run-and-the-valley-at-war-at-m3.md)).** The M3 part of the confirmation runs in CI as `GameLogicTests/CommandPathTests`. A test client of the world server in the valley has another settlement's declaration, claim, tribute and placement refused as not permitted, and a declaration and a claim each missing their last field refused as malformed, each counted by the region's queue. A declaration naming a pool slot in place of its Leader's id executes with no effect and the reason `AttackRefusal::NoLeader`. A crowd order for Workers who died before its tick does nothing. At the MVP no two senders command one object, so the contested action is the order of execution: over 1,000 ticks of the valley the steward's command executes before the AI settlement's in 48.8 % of them. The M4 part and the later one stand.
