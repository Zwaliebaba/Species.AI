---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Transport: one interface with an in-process and a datagram implementation, schema-defined messages, build and content identity, and network diagnostics

## Context and Problem Statement

The integrated server runs in-process behind the same transport interface as a dedicated one (O10); community operators host dedicated servers first (O5). The reference sends unreliable datagrams with its own resends, self-describing messages that name every field, no encryption, no fragmentation and no bandwidth limit; a frame above 1,500 bytes is never delivered, one unrecognised message ends the process, and the join password travels in clear text (DEC-207). It compares no content or build identity between processes (DEC-204), and its network view shows only the health of its command frames (DEC-208). Identity, entitlement and the rendezvous and relay services for hosts at home are area D5's (DEC-248, DEC-249). R14 allows no third-party library in the executable.

Which transport and message format does Species use, how do server and client prove they run the same build and content, and what does the network report?

## Decision Drivers

* One code path whether the server is in-process or remote (O10, DEC-207 item 11).
* No dependency on a platform service for community servers (O5), and no third-party library (R14).
* Robust parsing of hostile input, encryption and no credential in clear (DEC-207 items 4 to 8).
* State replication at crowd scale cannot afford field names (DEC-201 item 11).
* A mismatch of build or content is refused with a reason, never played (DEC-204).
* Operators tune their servers with the numbers the developers use (DEC-208).

## Considered Options

* A — a platform transport with relay, NAT traversal and encryption, behind a Species interface
* B — an own encrypted datagram transport with reliable and unreliable channels and fragmentation, behind the same interface
* C — a stream transport for commands and a separate datagram channel for state

## Decision Outcome

Proposed option: "B", with option A of DEC-204 for identity and option B of DEC-208 for diagnostics. **Default while Proposed:** the decision below.

1. **One interface, two implementations.** `Transport` is the interface of every connection: connect, accept, send on a channel, receive, close, and its statistics. `InProcessTransport` joins the client and the integrated server by bounded queues of encoded buffers, through the same sequencing, acknowledgements, channels and budgets. `UdpTransport` is Species' own protocol over UDP. A relay hop for hosts behind a home router (DEC-249) is a third implementation behind the same interface when area D5 provides one.
2. **Channels.** A reliable, ordered channel for commands, presentation events that are guaranteed, results and replies, with cumulative and selective acknowledgements; an unreliable, sequenced channel for state updates, each acknowledged so that it becomes the client's baseline (ADR-086); a streamed transfer with flow control for blobs (a join's snapshot, terrain chunks by digest, a replay), never buffered whole (DEC-204 item 8, DEC-207 item 10).
3. **Datagrams.** At most 1,200 bytes of payload per datagram, asserted at the sender, with fragmentation above it; a keep-alive or an acknowledgement every tick period on an idle connection. The client shows that it waits for the server after 1 s without data and disconnects after 15 s; the server drops a silent connection after 30 s, and keeps the record of a departed connection for at most 10 minutes (DEC-205 item 9).
4. **Security.** After a handshake every datagram is encrypted and authenticated (an AEAD cipher, AES-GCM, with a per-connection key from an elliptic-curve key agreement), using the operating system's cryptography provider: on Windows the in-box CNG, which R14 allows. The server authenticates itself with a long-lived key, pinned by the server listing of area D5 or, for a server reached by its address, on first use with the key's fingerprint shown to the player. No credential and no password travels in clear. A second operating system's build names its provider in its own record (gate B).
5. **Messages.** Every message type has its own schema, versioned, in a bit-packed binary layout without field names, length-prefixed, with a hard limit on every count, length and depth; codecs are generated from the schemas (ADR-062). A malformed, oversized or unknown message is dropped and counted, never fatal (DEC-207 items 4 to 7); every parser is fuzzed in CI (ADR-069).
6. **Identity at the handshake** (DEC-204, option A). The client sends its build identity (ADR-057) with its protocol version and its simulation version, and the content manifest hash (ADR-060) of its simulation-relevant and presentation content; the server refuses a different protocol, simulation or content with a reason that the client shows. The world names its world template and generator versions (WLD-009). Downloading content from a server (DEC-204 option B, for later modding) is a later record and needs no change to this handshake.
7. **Diagnostics** (DEC-208, option B). Per connection: the measured round trip, jitter, loss, downstream and upstream rates against their budgets (ADR-086), the age of the newest state, the interpolation buffer, the reliable backlog and the rejected commands. Per server: tick-time percentiles, each region's step time and bodies, battles in progress, bandwidth per region and per client, and the owed ticks of ADR-083. They are shown in the developer overlay, written to the server's logs (ADR-067), and published to the world host through the server's administration interface. Every Debug build has a link simulator (latency, loss, reordering, a rate cap; soak target 10 % loss with reordering). Every server keeps a command journal of the last 10 minutes with the checkpoint before it, written out on a crash or at the world host's request, so that a defect reproduces on the same build (ADR-081 Level 1).

### Consequences

* Good, because community servers need no platform service and no third-party library, and the integrated server shares every line but the queue.
* Good, because every parser is bounded and fuzzed, and every connection is encrypted with in-box cryptography.
* Good, because a build or content mismatch is a clear refusal, and an operator sees the same numbers as a developer.
* Bad, because Species writes and maintains its own reliable transport and handshake, a security-sensitive part, which area D9 reviews.
* Bad, because players hosting at home need area D5's rendezvous or relay to be reachable; this record does not provide it.
* Neutral, because a second operating system's server needs its own cryptography provider.

### Confirmation

**Confirmed when:** the networking stage of the first playable passes 4 clients over 10 % loss and 150 ms round trip for 30 minutes; a client of another build or with an edited content file is refused with its reason; the fuzzer has run every message parser for 10⁷ inputs without a crash or an unbounded allocation; the integrated server runs the same test through `InProcessTransport` with identical per-tick hashes; and area D9's review of the handshake is recorded.

## Pros and Cons of the Options

### A — a platform transport

* Good, because relay and NAT traversal come with it.
* Bad, because it ties community servers to a platform service and adds a third-party library, which R14 does not allow.

### B — an own datagram transport

* Good, because it depends on nothing beyond the operating system and serves both stages.
* Bad, because its reliability, security and robustness are Species' to build and test.

### C — a stream for commands and datagrams for state

* Good, because a stream gives reliability for free.
* Bad, because a stream blocks behind a lost packet, and two transports double the code and the security surface.

## More Information

* Resolves DEC-204, DEC-207 and DEC-208 when the owner confirms it at gate B. Related: [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-067](ADR-067-logging.md), [ADR-069](ADR-069-test-framework.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-087](ADR-087-command-path-validation-and-timing.md).
* The type names `Transport`, `InProcessTransport` and `UdpTransport` are proposed here; area D3 confirms them in the replication component.
