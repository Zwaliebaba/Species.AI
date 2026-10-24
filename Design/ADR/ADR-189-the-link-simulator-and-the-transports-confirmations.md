---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T11; owner decisions O44, O108, O109)
---

# The link simulator and the transport's confirmations

## Context and Problem Statement

M4 confirms the transport records (the [M4 plan](../implementation/m4_plan.md) §2, T11):
- ADR-088: through the link simulator at 10 % loss and a 150 ms round trip for 30 minutes, no figure frozen for more than 2 ticks after a single lost datagram; the overhead per state datagram, from MsQuic's statistics, within 5 bytes of item 3's model; the same session through `InProcessTransport` with identical per-tick hashes; every message parser fuzzed for 10⁷ inputs;
- ADR-086: the same link, and a rejoin at minute 20 within 10 s;
- ADR-089: the pinned `msquic.dll` beside each executable, the peer's datagram support enabled, and the pin re-checked on nuget.org;
- ADR-069: the fuzzer of every parser in CI;
- ADR-071: `msquic.dll` and its licence notice in the install's list.

ADR-088 item 7 names the link simulator: a development tool under `Tools/` that relays a connection's UDP datagrams on the loopback interface with added latency, loss, reordering and a rate cap, through which every Debug build can route its connection. It never ships.

How is the simulator built, how is a freeze measured, and where do these runs live?

## Decision Drivers

* PLT-100: the platform layer stays socket-free, and only MsQuic opens a socket in the game (ADR-089).
* The measurement uses the server and transport code the headless server runs, not a copy.
* A freeze is the replica disagreeing with the server, not an inference from the datagram stream.
* The owner runs the 30-minute run on the reference test machine; CI runs a short one on every build.
* No new dependency (R14).

## Considered Options

1. **A Python relay and a scene of the performance tool.** `Tools/LinkSimulator/LinkSimulator.py` relays UDP with Python's socket module. `SpeciesPerformance.exe --scenes link` runs the session: the headless server's host with `QuicListener`, and a client of its own over `QuicTransport`, through the simulator. `Build/RunPerformance.py --link` starts the simulator, runs the scene, judges the lines and records them in `playtests/M4.md`.
2. **A C++ relay program under `Tools/`.** The relay is a tool project on Winsock.
3. **Loss inside the transport.** `QuicTransport` drops datagrams itself in a Debug build.

## Decision Outcome

Chosen option: 1.
- Option 2 brings the first socket code into the C++ tree, and the checkers' rule for it, for a tool that is a few hundred lines of Python. It also runs only on Windows, so it could not be tested where it is written.
- Option 3 loses only the datagrams the transport sends, not QUIC's acknowledgements, stream packets and handshake, and measures nothing of MsQuic's loss detection, retransmission or congestion control.

What is built:

* **The simulator.** `LinkSimulator.py --listen <port> --server <port>` relays one client's datagrams to the server on `127.0.0.1` and back, from a socket of its own, so the server sees one peer. Each direction is drawn separately:
  - every datagram is lost with `--loss-percent` (10);
  - one kept leaves after half of `--rtt-ms` (150);
  - with `--reorder-percent` (5), it leaves up to `--reorder-ms` (120) later, so later datagrams overtake it;
  - with `--rate-kbps`, each direction is a link of that rate, where a datagram that would wait more than a second is lost.

  A datagram on `--listen` from another address is a new client (a rejoin). One generator of `--seed` draws every fate. The figures go to `--stats` every second and at the end. `--self-test` relays 4,000 echoes and checks the loss, the round trip, the reordering and the rate limit. CI runs it.
* **The Debug route.** A Debug build's `--link-port <port>` makes the client connect to an always-on world's server through that port. The pin stays the listing's fingerprint (ADR-187), because TLS runs end to end.
* **The link scene** (`Tools/SpeciesPerformance/LinkScene`), named only:
  - A valley of four settlements (seed 1, three AI settlements). The steward's settlement is run by its scripted steward and watched, so its figures move for the whole session. The world is served by `ServerHost` with `QuicListener` and the server's identity (ADR-186), as `SpeciesServer.exe` serves it.
  - The client joins over `QuicTransport` and asks for its statuses once a second, as the HUD asks.
  - Wrappers of the world and the listener record:
    - each tick's hash;
    - the latest steward's view, as `Publish` builds it (`ViewForSteward`);
    - the objects of each state datagram and its fate;
    - each server end's MsQuic statistics.
  - Each tick's comparison is made once the first datagram of a later tick arrives, so every datagram of the tick that arrived in time has been applied. The client compares its replica with the server's view of that tick. An object frozen at a tick is one the replica does not hold as the server's view holds it. A freeze runs until it holds it again, and its datagrams lost are those that carried the object, lost within it. A rejoin ends the freezes in progress uncounted.
  - The run is 30 minutes of the server's ticks with a rejoin at minute 20. The quick run is 300 ticks with a rejoin at tick 150.
  - After the session, the server's journal is replayed headless, and each tick's hash is compared.
* **The lines** (`RunPerformance.py --link`, judged):
  - a figure's longest freeze over one lost datagram that carried it is at most 2 ticks;
  - the overhead per state datagram is within 5 bytes of 73.25 bytes on IPv4. The overhead is every UDP byte the server sent, with 28 bytes of IPv4 and UDP per packet, less MsQuic's stream payload and the state datagrams' own bytes, over the state datagrams MsQuic reported sent, plus the 12.25 bytes of headers inside each datagram. A datagram counts at MsQuic's `QUIC_DATAGRAM_SEND_SENT` report (`QuicPathStats::datagramsSent`, `datagramBytesSent`), so one still queued, or canceled at a close, is not charged to the link. It is counted over each connection after its first 10 s. This charges every ACK-only packet, every probe and the stream frames' headers to the datagrams, so it is an upper bound. 73.25 is item 3's 69 with ADR-111's header: the 3-byte message header and the 10-bit record count above the 8 bytes item 3 counts;
  - a rejoin's first state is within 10 s of its connection's start;
  - every tick's hash equals the journal's replay;
  - the peer's datagram support is enabled at both ends.

  The freezes over two or more losses and over none, the datagrams, MsQuic's round trip, congestion window and events, the largest datagram it sends, the statuses answered and the simulator's figures are reported.
* **The record.** `playtests/M4.md`'s section "The link run". A missed line fails the run with exit code 1. The machine is neither read nor pinned, because these lines measure the link, not the machine's speed.
* **CI.** `RunPerformance.py --self-test` runs:
  - the simulator's self-test;
  - a quick link run through the simulator at its default link, which must join, rejoin, be answered, show datagrams enabled at both ends and replay to the same hashes;
  - the lines' and the record's rules on fixtures.
* **The fuzzer.** `GameSharedTests/MessageFuzzTests` gives each of the 39 game messages' parsers 4,000 inputs in CI:
  - half random bodies under the message's real header, up to a little above its bound;
  - half the encoding of a default message of the type, with a bit flipped, truncated or extended.

  Each is delivered or dropped with its reason. Its corpus is those encodings and the generator's pinned seed, as ADR-069 has it, so a failing input is found again. A default message holds empty lists, so the deep structures (a state update's records, a status's rows) are reached by the random bodies alone. ADR-088's 10⁷ inputs per parser are the same code at 10,000,000 inputs per message (3.9 × 10⁸ in all), run once and recorded in `playtests/M4.md`.
* **The pinned DLL.** `Build/CheckServerImports.py` fails when the server imports `msquic.dll`, which the transport loads itself and no project links (ADR-185). It also fails when the `msquic.dll` beside the built programs is not the pinned package's copy for their platform, byte for byte.
* **`QuicPathStats`** gains MsQuic's stream payload bytes each way, its congestion events, and the state datagrams it reported sent with their bytes.

### Consequences

* Good, because the run measures the code the steward's computer runs: the headless server's host, the world server, MsQuic's loss detection, retransmission and congestion control, and the client's replica.
* Good, because the simulator was tested where it was written, and CI runs a short version on every build.
* Bad, because the scene runs in real time: 30 minutes for the owner, and about 40 s of every CI build.
* Bad, because the overhead counts acknowledgements and probes with the datagrams. A result above the line needs the packets per datagram, which the record reports, to tell the model's error from the acknowledgements'.
* It forecloses nothing: a scene of four clients at the first milestone with remote stewards runs the same simulator on four ports.

### Confirmation

* `LinkSimulator.py --self-test` in CI; `RunPerformance.py --self-test`'s link checks in CI.
* `NeuronCoreTests/StartupOptionsTests`: `--link-port` in Debug builds only.
* The owner's `Build\RunPerformance.py --link` on the reference test machine, recorded in `playtests/M4.md`, confirms or amends ADR-086's and ADR-088's M4 lines.

## Pros and Cons of the Options

### 1 — a Python relay and a scene

* Good, because the relay has no build and is tested on any machine.
* Bad, because Python's timers wake about a millisecond late, which is within a tick's 100 ms.

### 2 — a C++ relay

* Good, because one language serves the tree.
* Bad, because it is the tree's first socket code, for a tool.

### 3 — loss in the transport

* Good, because no relay runs.
* Bad, because it does not test how QUIC detects and resends what is lost.

## More Information

* Owner decisions O44, O108, O109; MVP-071, MVP-090, PLT-100; the [M4 plan](../implementation/m4_plan.md), task T11.
* Amends [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md) (item 7's simulator, the overhead's model with ADR-111's header), [ADR-086](ADR-086-replication-and-interest-management.md) (how M4's line is measured), [ADR-089](ADR-089-msquic-transport-package.md) (the pin re-checked, the DLL checked) and [ADR-185](ADR-185-the-quic-transport-and-the-servers-certificate.md) (the path statistics).
* The pin, re-checked on nuget.org on 2026-10-24: `Microsoft.Native.Quic.MsQuic.Schannel` 2.6.3 is the latest version, published 2026-10-13, MIT, with no deprecation or advisory. 2.6.1 and 2.6.2 are deprecated for critical bugs (GHSA-2mxg-prvg-9jfg). Read from nuget.org's registration index.
* CI's first quick link run through the simulator on MsQuic (300 ticks, Debug build on the Windows runner, 2026-10-24) found:
  - a figure's longest freeze over one lost datagram was 1 tick, over 1,207 such freezes, and over two or more 2 ticks;
  - the rejoin's first state came 0.69 s after its connection started;
  - all 352 hashes equal the replay's;
  - datagrams were enabled at both ends, and MsQuic's round trip was 162.7 ms.

  It also showed that MsQuic puts several DATAGRAM frames in one packet (0.93 packets a datagram), which item 3's model of one datagram per packet does not count. Its overhead, computed then from the datagrams the world handed to MsQuic, came out negative, because datagrams queued under congestion or canceled at the rejoin were charged to the link. The overhead now counts only the datagrams MsQuic reports sent. CI's next quick run (run 313) put it at 87.5 bytes over 124 datagrams, with 1.36 packets a datagram, so that about a third of a packet a datagram, its acknowledgements and probes, is charged to it; its freezes over one lost datagram were again at most 1 tick, over 797.
* The scene's logic was run before CI on a Linux build of the portable libraries, with a stand-in for MsQuic over `InProcessTransport` that loses 10 % of the state datagrams. Over 300 ticks:
  - every freeze over one lost datagram was 1 tick, and every one over two was 2;
  - there was no other freeze;
  - the rejoin completed;
  - the 301 hashes equalled the replay's.

  It measured nothing of MsQuic.
* The code: `Tools/LinkSimulator/LinkSimulator.py`, `Tools/SpeciesPerformance/LinkScene`, `Build/RunPerformance.py`, `Build/CheckServerImports.py`, `GameSharedTests/MessageFuzzTests`, `NeuronCore/StartupOptions`, `GameClient/AlwaysOnClient`.
