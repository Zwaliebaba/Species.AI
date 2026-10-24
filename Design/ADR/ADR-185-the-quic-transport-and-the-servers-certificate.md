---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T7; owner decisions O44, O108, O109)
---

# The QUIC transport and the server's certificate

## Context and Problem Statement

M4 runs the always-on world in its own server process, which the client connects to over `QuicTransport` (MVP-071, the [M4 plan](../implementation/m4_plan.md) T7). ADR-088 states the transport: one interface and two implementations, a reliable stream and QUIC datagrams with their fates, TLS 1.3 with a self-signed server certificate that the client pins, and the keep-alive and idle timeouts. ADR-089 pins MsQuic 2.6.3, Schannel build, and puts `msquic.dll` beside the executables.

Neither record says how the transport sits on MsQuic's threads, how a static library reaches a DLL that its programs do not link, or where the server's certificate comes from. This record decides those for M4, where the server and the client are on one computer (O109).

## Decision Drivers

* `QuicTransport` keeps `Transport`'s contract as `InProcessTransport` keeps it: no call waits, the reliable channel is ordered and bounded, each datagram's fate is reported once (ADR-088 items 1 and 2).
* MsQuic calls back on its own threads, and some of its calls wait for those threads.
* Only the transport reaches MsQuic, and only the pinned copy (PLT-100, ADR-089).
* The client refuses any server but the one it pins (ADR-088 item 4).
* No new dependency (R14).

## Considered Options

1. **The transport loads MsQuic itself.** NeuronCore's `QuicTransport` loads `msquic.dll` from beside the module that holds it, at its first use, and calls it through MsQuic's function table. The certificate is made with the system's own certificate functions and kept in the user's store.
2. **Every program links `msquic.lib`.** Each program and test project imports the package's targets and loads the DLL at start-up.
3. **A certificate file beside the world.** The server writes its certificate and key to the world's folder.

## Decision Outcome

Chosen option: 1.
- Option 2 makes every program that links NeuronCore, the content build and the frame bench among them, load MsQuic whether it connects or not, and needs the package's link step in each project.
- Option 3 writes a private key to a plain file, which the user's key storage provider exists to avoid.

What is built:

* **Loading.** `QuicLibrary` loads `msquic.dll` from the folder of the module that holds the code (the program, or the test library a test host loads), with `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32`, so Windows' own MsQuic is never taken. It opens version 2 of the function table and one registration of the low-latency profile. It sets the global Retry memory to 0 (ADR-088 item 4). It lives while any end or listener holds it, and its last holder closes it. No project links `msquic.lib`: NeuronCore restores the package for its header and the build's copy of the DLL.
* **Threads.** Each connection's state (`QuicLink`) is behind one lock, which MsQuic's callbacks take. No MsQuic function is called while the lock is held, because MsQuic may run a callback inside a call or wait for one. The end makes and closes every handle it owns: the client opens its stream on its own thread, at its first call after the handshake. The callbacks only record what happened. An exception on MsQuic's thread ends the process.
* **The reliable channel.** One bidirectional stream, opened by the client with `QUIC_STREAM_START_FLAG_IMMEDIATE`; the server takes the first and refuses any other (`PeerBidiStreamCount` 1, no unidirectional streams yet). Each message is framed by its length (ADR-093). A send waits in the end's buffer until the stream exists and leaves at the next call on the end. Send buffering is off, so a send completes when its bytes are acknowledged. The backlog, sent and not acknowledged, is bounded at 4 MiB, beyond which `Send` answers `Backlogged`. A receiver with more than 4 MiB not taken stops reading the stream, and QUIC's flow control then holds the sender; it reads again once it is down to 2 MiB. A stream whose framing breaks closes the connection with a protocol error (ADR-088 item 5).
* **Datagrams.** A state datagram is a QUIC DATAGRAM frame. Its fate is reported once: acknowledged, or lost at the first suspicion (`LOST_SUSPECT`), when it is discarded or canceled, or at once if the peer does not take datagrams. A later spurious acknowledgement changes nothing, because the sender has already given up the update as a baseline. A datagram larger than MsQuic's `MaxSendLength` is refused as too large. A received datagram that is empty or above 1,200 bytes is dropped and counted.
* **Closing.** `Close` ends the stream's sending side after the bytes already sent, and the connection once they are acknowledged. The end that closed waits at most 1 s for that when it is destroyed. An end destroyed without `Close` tells its peer. The reason is the peer's or the transport's: idle, refused, or unreachable is `ClosedByPeer`.
* **Timeouts.** The client sends a keep-alive every 100 ms and advertises an idle timeout of 15 s; the server advertises 30 s. QUIC takes the smaller (RFC 9000 §10.1), so a connection whose peer vanished closes after 15 s at both ends. The server's 30 s binds only a client that advertises none. The client's display of "waiting for the server" after 1 s without data is T10's.
* **The certificate.** `ServerCertificate::OpenOrCreate(name)` opens the certificate of subject `CN=name` in the user's "My" store, if it has its key and is valid for at least another day. Otherwise it makes one: a persisted ECDSA P-256 key named `name` in the user's software key storage provider, and a self-signed certificate for server authentication valid for 365 days. Its fingerprint is the SHA-256 of its encoding. The listener presents it through MsQuic's certificate-context credential.
* **The pin.** The client's credential turns the operating system's chain validation off and asks for the server's certificate (`QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION | QUIC_CREDENTIAL_FLAG_INDICATE_CERTIFICATE_RECEIVED`). It accepts only a certificate whose fingerprint is the one it was given, and refuses another with a bad-certificate alert. A handshake that completes without the certificate checked is refused too, so the pin cannot be skipped. `CertificateRefused()` reports it, and the reason is a protocol error. T9 publishes the fingerprint beside the world the server runs.
* **The interface.** The listener binds `127.0.0.1` on the port it is given, or one the system chooses (`Port()`). The client connects there. Remote stewards, IPv6 and UPnP (ADR-088 item 4) wait for the milestone that has them.
* **Figures.** `Stats()` gives the transport's own, as `InProcessTransport`'s; `PathStats()` gives MsQuic's statistics (round trip, congestion window, path MTU, packets and UDP bytes each way), whether the peer takes datagrams and the largest datagram MsQuic sends now. T11 measures the overhead per state datagram from them.

### Consequences

* Good, because the client and the server run the same code over either transport, and only the transport knows MsQuic.
* Good, because the private key never leaves the user's key store, and a server keeps its certificate, and so its clients' pin, across restarts and worlds.
* Bad, because a reliable message sent in the frame the handshake completes waits for the end's next call, at most a frame or a tick.
* Bad, because the tests leave a certificate named "Species transport test" in the user's store of the machine that runs them, as a server leaves its own. It is reused on the next run.
* It forecloses nothing: blob streams (ADR-088 item 2) are unidirectional streams the end opens the same way, and a remote address replaces the loopback one.

### Confirmation

* `NeuronCoreTests/QuicTransportTests`, on the loopback interface in CI:
  - both channels both ways, with the order, the fates and the figures as `InProcessTransport`'s tests state them;
  - a thousand messages and one of the largest size in order;
  - either end closing after its last message;
  - a changed certificate refused before any message;
  - empty and oversized messages refused;
  - each of 200 datagrams given one fate when the server goes away;
  - a receiver that takes nothing holding the sender within its backlog, and every message arriving once it takes them.
* T11 confirms the rest of ADR-088 and ADR-089 on the reference test machine: the link simulator's soak, the overhead, the peer's datagram support and the pin re-checked on nuget.org.

## Pros and Cons of the Options

### 1 — the transport loads MsQuic

* Good, because nothing loads MsQuic until a connection needs it.
* Bad, because it calls through a function table rather than the import library.

### 2 — every program links it

* Good, because a missing DLL fails at start-up rather than at the first connection.
* Bad, because every program pays for it, and every project needs the package's link step.

### 3 — a certificate file

* Good, because the world's folder holds everything about it.
* Bad, because the private key lies in a plain file.

## More Information

* Owner decisions O44, O108, O109; MVP-071, MVP-090, PLT-100; the [M4 plan](../implementation/m4_plan.md), task T7.
* Amends [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md) (the idle timeouts' effect, the loopback interface at M4) and [ADR-089](ADR-089-msquic-transport-package.md) (MsQuic loaded by the transport, not linked).
* MsQuic's API as its 2.6.3 headers state it: `msquic.h` (the function table, `QUIC_SETTINGS`, the connection, stream and listener events, the datagram send states, `QUIC_PARAM_CONN_STATISTICS_V2`) and `msquic_winuser.h` (the address helpers, which need no socket library), read from the restored package on 2026-10-24.
* The code: `NeuronCore/QuicTransport`, `NeuronCore/ServerCertificate`, `NeuronCoreTests/QuicTransportTests`.
* **Amendment 2026-10-24 (M4 task T9, [ADR-187](ADR-187-the-always-on-worlds-server-process.md)).** The headless server's certificate is named `Species server`.
* **Amendment 2026-10-24 (M4 task T11, [ADR-189](ADR-189-the-link-simulator-and-the-transports-confirmations.md)).** `PathStats()` also gives MsQuic's stream payload bytes each way and its congestion events, from which the link scene computes the overhead per state datagram.
* **Amendment 2026-10-24 (M4 task T11, CI's first run of the transport's tests).** `Transport`'s contract that reliable messages sent before a `Close` reach the other end holds once the stream exists. A server end has no stream until its client's first message opens it, so a server end closed before then ends the connection at once, and what waited for the stream goes with it. The game never meets this: every client's first message is its join (ADR-186), and the server sends nothing it needs delivered before the join. `QuicTransportTests` closes after the client's first message, as a join's answer and close do.
