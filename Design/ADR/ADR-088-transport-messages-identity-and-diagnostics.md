---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Transport: one interface with an in-process and a datagram implementation secured by the operating system's DTLS, schema-defined messages, player identity keys, build and content identity, and network diagnostics

## Context and Problem Statement

The integrated server runs in-process behind the same transport interface as a dedicated one (O10); community operators host dedicated servers first (O5). The reference sends unreliable datagrams with its own resends, self-describing messages that name every field, no encryption, no fragmentation and no bandwidth limit; a frame above 1,500 bytes is never delivered, one unrecognised message ends the process, and the join password travels in clear text (DEC-207). It compares no content or build identity between processes (DEC-204), and its network view shows only the health of its command frames (DEC-208). Identity, entitlement and the rendezvous and relay services for hosts at home are area D5's (DEC-248, DEC-249). R14 allows no third-party library in the executable. Windows ships DTLS 1.2 in its Schannel security provider from Windows 10 version 1607 and Windows Server 2016 on, and a key store and elliptic-curve signatures in CNG.

Which transport and message format does Species use, how is it secured, how do a returning player and a server know each other, how do server and client prove they run the same build and content, and what does the network report?

## Decision Drivers

* One code path whether the server is in-process or remote (O10, DEC-207 item 11).
* No dependency on a platform service for community servers (O5), and no third-party library (R14).
* Robust parsing of hostile input, encryption and no credential in clear (DEC-207 items 4 to 8), with no cryptographic protocol of Species' own.
* A returning steward reclaims the settlement without a platform account (DEC-205).
* State replication at crowd scale cannot afford field names (DEC-201 item 11).
* A mismatch of build or content is refused with a reason, never played (DEC-204).
* Operators tune their servers with the numbers the developers use (DEC-208).

## Considered Options

* A — a platform transport with relay, NAT traversal and encryption, behind a Species interface
* B — Species' own datagram transport with reliable and unreliable channels and fragmentation, secured by the operating system's DTLS 1.2, behind the same interface
* C — a stream transport for commands and a separate datagram channel for state

## Decision Outcome

Proposed option: "B", with option A of DEC-204 for identity and option B of DEC-208 for diagnostics. **Default while Proposed:** the decision below.

1. **One interface, two implementations.** `Transport` is the interface of every connection: connect, accept, send on a channel, receive, close, and its statistics. `InProcessTransport` joins the client and the integrated server by bounded queues of encoded buffers, through the same sequencing, acknowledgements, channels and interest tiers. `UdpTransport` is Species' own reliability over UDP inside DTLS (item 4). A relay hop for hosts behind a home router (DEC-249) is a third implementation behind the same interface when area D5 or the owner provides one (gate B).
2. **Channels.** A reliable, ordered channel for commands, presentation events that are guaranteed, results and replies, with cumulative and selective acknowledgements; an unreliable, sequenced channel for state updates, each acknowledged so that it can become the client's baseline (ADR-086); a streamed transfer with flow control for blobs (a join's snapshot, terrain chunks by digest, a replay), never buffered whole (DEC-204 item 8, DEC-207 item 10), within the blob budget of ADR-086 item 4.
3. **Datagrams.** At most 1,200 bytes of payload per datagram, asserted at the sender, with fragmentation above it, so that a datagram with its 77 bytes of IP, UDP, DTLS and Species headers stays below 1,300 bytes; a keep-alive or an acknowledgement every tick period on an idle connection. The client shows that it waits for the server after 1 s without data and disconnects after 15 s; the server drops a silent connection after 30 s, and keeps the record of a departed connection for at most 10 minutes (DEC-205 item 9).
4. **Security and identity.** `UdpTransport` runs inside a DTLS 1.2 session (RFC 6347) of the Schannel security support provider, reached through SSPI in its datagram mode, with an ECDHE-ECDSA AES-GCM cipher suite; before any handshake state exists, the server answers a new source address only with a stateless cookie (the DTLS HelloVerifyRequest), so a spoofed source draws no reply larger than its request. Each server creates a self-signed ECDSA P-256 certificate at its first start; a client pins it by the server listing (D5) or, for a server reached by its address, on first use with its fingerprint shown to the player, and refuses a changed certificate with a warning. Each player has an **identity key**, an ECDSA P-256 key pair created in the user's CNG key store at first launch, whose private half never leaves it; inside the DTLS session the client signs a server nonce bound to the server certificate's fingerprint, so a signature cannot be relayed to another server. A server keeps a key registry: it registers a key at a steward's first join, a returning steward reclaims the settlement by it (ADR-086 item 5), and a steward who has lost the key is bound to a new one by the world host through the administration interface, recorded in the world record (ADR-087 item 8). No password and no credential travels in clear, and Species implements no cryptographic protocol of its own. Reachability for the first world-game milestone: a port mapping requested through the in-box UPnP NAT API, IPv6, and a manual address; a host behind carrier-grade NAT is unreachable until a relay exists (gate B).
5. **Messages.** Every message type has its own schema, versioned, in a bit-packed binary layout without field names, length-prefixed, with a hard limit on every count, length and depth; codecs are generated from the schemas (ADR-062). A malformed, oversized or unknown message is dropped and counted, never fatal (DEC-207 items 4 to 7); every parser is fuzzed in CI (ADR-069).
6. **Build and content identity at the handshake** (DEC-204, option A). The client sends its build identity (ADR-057) with its protocol version and its simulation version, and the content manifest hash (ADR-060) of its simulation-relevant and presentation content; the server refuses a different protocol, simulation or content with a reason that the client shows. The world names its world template and generator versions (WLD-009). Downloading content from a server (DEC-204 option B, for later modding) is a later record and needs no change to this handshake.
7. **Diagnostics and administration** (DEC-208, option B). Per connection: the measured round trip, jitter, loss, downstream and upstream rates against their budgets (ADR-086), the age of the newest state, the interpolation buffer, the reliable backlog and the rejected commands. Per server: tick wall-time percentiles against the budget and the deadline (ADR-080 item 6), each region's step time and bodies, battles in progress, bandwidth per region and per client, the owed ticks and the stalls of ADR-083, and the newest consistent cut. They are shown in the developer overlay, written to the server's logs (ADR-067), and published through the server's **administration interface**, an authenticated connection on the same transport with its own message schemas; the integrated host's tools, a dedicated server's operator console and every host command use only that interface (ADR-083 item 2), and every command on it is recorded (ADR-087 item 8). Every Debug build has a link simulator (latency, loss, reordering, a rate cap; soak target 10 % loss with reordering). Every server keeps a command journal of the last 10 minutes with the consistent cut before it, written out on a crash or at the world host's request, so that a defect reproduces on the same build (ADR-081 Level 1).

### Consequences

* Good, because community servers need no platform service and no third-party library, and the integrated server shares every line but the queue.
* Good, because the security protocol is a published standard implemented by the operating system, so Species writes no cipher, key agreement or handshake, and another platform's DTLS library speaks the same protocol on the wire.
* Good, because a returning steward is recognised by a key that only that steward's machine holds, without an account, and a stolen signature is useless on another server.
* Good, because every parser is bounded and fuzzed, a build or content mismatch is a clear refusal, and an operator sees the same numbers as a developer.
* Bad, because Species still writes and maintains its own reliability, channels and fragmentation above DTLS, and area D9 reviews the identity flow and its key registry.
* Bad, because DTLS costs 37 bytes per datagram (a 13-byte header, an 8-byte explicit nonce and a 16-byte tag), about 1 % to 7 % of each view's bandwidth (ADR-086).
* Bad, because players hosting behind carrier-grade NAT cannot be reached until a relay exists; this record does not provide it, and a player's identity is per machine until D5's service exists.
* Neutral, because a second operating system's server needs its own DTLS provider and key store; the protocol and the keys are standard.

### Confirmation

**Confirmed when:** the networking stage of the first playable passes 4 clients over 10 % loss and 150 ms round trip for 30 minutes inside DTLS; a client of another build or with an edited content file is refused with its reason; a client that changes the server's certificate, replays a signature to another server or claims a registered settlement with another key is refused; a handshake from a spoofed address draws no reply larger than its request; the fuzzer has run every message parser for 10⁷ inputs without a crash or an unbounded allocation; the integrated server runs the same test through `InProcessTransport` with identical per-tick hashes; and area D9's review of the identity flow is recorded.

## Pros and Cons of the Options

### A — a platform transport

* Good, because relay and NAT traversal come with it.
* Bad, because it ties community servers to a platform service and adds a third-party library, which R14 does not allow.

### B — an own datagram transport inside the operating system's DTLS

* Good, because it depends on nothing beyond the operating system, serves both stages and leaves the cryptography to a standard implementation.
* Bad, because its reliability and robustness are Species' to build and test, and its reach stops at carrier-grade NAT without a relay.

### C — a stream for commands and datagrams for state

* Good, because a stream gives reliability for free.
* Bad, because a stream blocks behind a lost packet, and two transports double the code and the security surface.

## More Information

* Resolves DEC-204, DEC-207 and DEC-208 when the owner confirms it at gate B; the identity, listing and relay provider with the storefront (ADR-072) and the publisher (ADR-073) is owner question 6 of the solution strategy (§4.5), and DEC-248 and DEC-249 stay area D5's. Related: [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-067](ADR-067-logging.md), [ADR-069](ADR-069-test-framework.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-087](ADR-087-command-path-validation-and-timing.md).
* DTLS 1.2 in Schannel from Windows 10 version 1607 and Windows Server 2016, and its ECDHE-ECDSA AES-GCM cipher suites, are as Microsoft Learn states them ("TLS (Schannel SSP) changes in Windows 10 and Windows Server 2016"; "TLS Cipher Suites in Windows 10 v1607"), read on 2026-10-18.
* The type names `Transport`, `InProcessTransport` and `UdpTransport` are proposed here; area D3 confirms them in the replication component.
* **Amendment 2026-10-18 (challenge round 1).** Item 4 replaces the own AES-GCM handshake with Schannel DTLS 1.2 and adds the player identity key, the key registry and the in-box reachability (A2-F15, A2-F16, A2-MB-5); item 7 makes the administration interface the only path for host tools (A2-F17); the datagram overhead is 77 bytes.
