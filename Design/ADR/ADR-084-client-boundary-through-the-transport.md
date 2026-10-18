---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# The client sees the world only through the transport: a replica, presentation events and interpolation

## Context and Problem Statement

The reference's integrity rules are fixed: presentation never writes simulation state, local-only state enters the simulation only as commands, and no presentation generator feeds it (R-SIM-655 to R-SIM-675). Its presentation reads live simulation objects and extrapolates positions between ticks (R-SIM-560 to R-SIM-564), and nothing enforces the rules. Species makes the server authoritative and the client a viewer of the state it may see (vision §5), runs the server in the host's own process in stage 1 (O10), and hides what a settlement cannot see from its client (PIL-084, WLD-242). The baseline renderer reads one immutable presentation snapshot per frame (RND-018) and extrapolates until this decision (RND-002). DEC-222 asks how the boundary is enforced; the first seed challenge of the design phase is that the integrated server must be separable and the in-process transport the real interface.

What is the only path between the server and any client, and what does the client draw from?

## Decision Drivers

* No client code can reach server state, whether the server is in the same process or not (O10).
* Fog of war is enforceable only if a client never receives what it may not see (PIL-084).
* The integrated host and a remote client look the same, so one client path is tested.
* Smooth motion from state that arrives at 10 ticks per second over a network with jitter (ADR-079).
* The integrity rules become build errors, not review findings (DEC-222 item 3).

## Considered Options

* A — a strict one-way boundary: encoded messages through the transport only, a client-side replica, an immutable snapshot and a typed event stream, interpolation between the two latest states; enforced by the build
* B — a shared object graph with read-only views for presentation, enforced by const access and review
* C — the reference model: direct access and extrapolation, by convention

## Decision Outcome

Proposed option: "A". **Default while Proposed:** the decision below.

1. **One path.** Everything that passes between a server and a client is an encoded message on a `Transport` (ADR-088): commands one way (ADR-087), state updates, presentation events and replies the other (ADR-086). The integrated server's transport is in-process: two bounded queues of encoded byte buffers between the client's and the server's threads, through the same sequencing, acknowledgement, channels and budgets as the socket transport. No pointer, reference or handle into server memory crosses it.
2. **Enforced by the build.** The server and simulation libraries cannot include a header of the renderer, audio, input, UI, camera or client session; the client's libraries cannot include a header of the server's internals or of the simulation's owners, only the message schemas. `Build\CheckProjectFiles.py` enforces both include allow-lists (BLD-041), and the server builds and runs with no presentation library linked (ADR-083).
3. **The replica.** The client keeps a replica: the decoded state of everything in its interest set (ADR-086), keyed by stable id, for the two latest ticks received. Presentation reads only the replica and the event stream, through the presentation snapshot of RND-018, which the client session builds once per rendered frame.
4. **Interpolation.** Each frame draws every replicated object at the interpolation between its two latest received states, at a display time one tick period plus a jitter allowance behind the newest state: the jitter allowance is 0 on the integrated host and 30 ms for a remote client, measured by the first playable and then fixed (ADR-079). When the next state is late, objects hold their latest state for up to 250 ms and the client then shows that it is waiting for the server; nothing is extrapolated. A figure that first appears is drawn from its first state; one that leaves the interest set fades out. The presentation snapshot carries the two tick numbers and the interpolation fraction in place of the extrapolation time (RND-002, RND-018, RND-019).
5. **Presentation events.** The server emits typed presentation events per tick (hits, deaths, Essence orb releases, sound cues, messages, achievement progress), filtered by the receiver's interest as the state is, each with its tick. The event contract names each event guaranteed (sent on the reliable channel: messages, results, warnings) or droppable (on the state channel: cosmetic cues); a droppable event older than 500 ms is not played.
6. **Local feedback.** The client shows the cursor, the selection highlight, order markers, route previews (from a pool of its own, R-SIM-662) and camera motion at once, from local input; it never predicts the movement of a figure. The server's result replaces any marker it contradicts.
7. **Skirmish** uses the same path, the same replica and the same interpolation.

### Consequences

* Good, because the integrated server is separable by construction: deleting the in-process transport and attaching the socket one changes nothing else.
* Good, because fog of war holds by construction: the client never holds what its settlement cannot see.
* Good, because one client path is tested for every stage, and every known divergence path of the reference becomes a build error.
* Bad, because the integrated host encodes and decodes its own updates, about 0.05 µs per figure per tick (ADR-086), and draws one tick behind the server.
* Bad, because a renderer that reads the replica cannot read a simulation field the server does not replicate; every presentation need is a replicated field or an event.
* Neutral, because motion is smooth where the reference snapped on a turn.

### Confirmation

**Confirmed when:** the build fails on a forbidden include in both directions; the headless server running a recorded command stream gives the same per-tick hashes with presentation stubs and with an integrated client attached (DEC-222 item 4); a client's replica is shown, by a test of the first world-game milestone, to hold no figure, structure or store outside its interest set; and the first playable runs its integrated and remote clients through the same client code.

## Pros and Cons of the Options

### A — the transport only

* Good, because separability, fog of war and one client path follow from it.
* Bad, because the host pays the encoding of its own view.

### B — shared read-only views

* Good, because it is cheaper inside one process.
* Bad, because it cannot serve a remote client, so Species would need A as well, and const access does not stop a cast.

### C — the reference model

* Bad, because it enforces nothing and leaves every known divergence path open.

## More Information

* Resolves DEC-222 when the owner confirms it at gate B. Amends the default of RND-002 and RND-018 (interpolation in place of extrapolation); the renderer's owning area applies it. Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md).
