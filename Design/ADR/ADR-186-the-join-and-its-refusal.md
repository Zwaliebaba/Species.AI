---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T8; owner decisions O108, O109)
---

# The join and its refusal

## Context and Problem Statement

From M4 a client connects to a headless server it did not start in its own process (MVP-071). The server can be of another build than the client: the owner rebuilds while an always-on world runs in its console, or starts a Debug client against a Release server. Its content can differ too: the owner edits a record, and the content build re-cooks it beside the executables, while the running server keeps what it loaded. A client of another build reads the server's messages wrongly, and one of other content predicts costs and draws terrain from other numbers.

ADR-088 item 6 says what the client sends at its connection: its build identity (ADR-057), its protocol and simulation versions, and its content manifest hash (ADR-060). The server refuses a different protocol, simulation or content with a reason the client shows. Neither ADR-057's generated identity nor a protocol version exists in code. The content's simulation identity does (`LooseContent::Identity`, AST-100).

What does a client send, what does a server compare, and what happens to a connection before and after?

## Decision Drivers

* A client of another build or with other simulation-relevant content is refused with its reason (the [M4 plan](../implementation/m4_plan.md) T8).
* A refused client changes nothing in the world: no steward, no journal entry, no presence (ADR-181).
* The same code runs over both transports (ADR-088 item 1).
* No new build step, which ADR-057's generated identity needs and M4 does not.

## Considered Options

1. **A join message, answered, with the programs' stamp as the build.** The client's first message carries the protocol version, a stamp of the build's two programs and the content identity. The server holds the connection out of the world until it arrives, then answers it and admits it or refuses it.
2. **ADR-057's generated identity.** A build step writes the revision and a build number into a header that both programs compile.
3. **Compare at the transport.** QUIC's ALPN names a protocol version, and a mismatch fails the handshake.

## Decision Outcome

Chosen option: 1.
- Option 2 is the shipping design, but the revision misses a build from an uncommitted tree, which is how the owner builds between pulls. It needs a generated header the checkers and clang-tidy must learn.
- Option 3 refuses without a reason the client can show, and the in-process transport has no ALPN.

What is built:

* **The identity.** `JoinIdentity` (`GameShared/JoinMessages`):
  - `GAME_PROTOCOL_VERSION`, 1, raised by any change to a message the two exchange;
  - the build: the SHA-256 over the name, size and last write time of `Species.exe` and `SpeciesServer.exe` beside the program (`Neuron::FilesStamp`, `BUILD_PROGRAMS`). Both programs of one build stamp the same files. A program rebuilt after its server started stamps differently. An executable cannot be rewritten while it runs, so the stamp a server took at its start stays true;
  - the content: the simulation content identity of the content folder mounted (AST-100).
  `OwnJoinIdentity` composes it. `Species.exe`, `SpeciesServer.exe` and the frame bench compute it once at start, after mounting the content.
* **The join.** `JoinCommand`, the client's first message on the reliable channel, sent when `ClientSession` is made; the server does not number it, so commands are numbered from 0 after it, as before (ADR-124). `JoinAnswer`, the server's first message on the connection: the refusal and the server's own identity. Neither changes layout, so any two builds read each other's join. An answer of an unknown refusal is malformed.
* **The comparison.** `JoinRefusalOf` names the first difference: the protocol, then the build, then the content (`JoinRefusal::Protocol`, `Build`, `Content`, saved on the wire and never renumbered). A join that does not read is a protocol refusal.
* **The server.** A `WorldServer` given its identity (`Desc::identity`) holds each new connection as joining. It drops everything from a joining connection but its join, sends it nothing, and journals no connection for it, so it is no steward's presence. An accepted join makes it a steward at that tick, as connecting did, and its answer goes before anything else it is sent. A refused join is answered, logged with both identities, and closed. A server given no identity takes each connection as it connects, as the tests' servers do.
* **Closing.** `ConnectionSender::Close` closes one connection after the reliable messages already sent to it; the host drops it at the tick that finds it closed (ADR-095).
* **The client.** `ClientSession` sends the join when it is given an identity, and on a refusal records it with the server's identity (`JoinRefused`), stops reading, closes and refuses every command. Until T10's front end shows the message, the title shows the refusal (`JoinRefusedTitle`) and the log both identities.
* **The simulation version.** It is not sent. The client runs no simulation; what it shares with the server's rules is code, which the build stamp covers, and data, which the content identity covers. `WORLD_STATE_VERSION` belongs to GameLogic, which the client does not link.

### Consequences

* Good, because a stale server, the owner's most likely mismatch, is refused with its reason before the client reads a message of it. T9's client then stops it and starts its own.
* Good, because the in-process path runs the same join, and its two ends always agree.
* Bad, because a copy of the build whose files have other times is another build, and two builds of identical code are told apart. The cost is a restart of the server.
* Bad, because the protocol version is raised by hand; a forgotten raise is still caught by the build stamp while both programs come from one folder.
* It forecloses nothing: ADR-057's generated identity replaces the stamp in `OwnJoinIdentity`, and the identity key's signature (ADR-088 item 4) joins the join.

### Confirmation

* `GameSharedTests/JoinMessageTests`: the join and the answer round-trip at their bounds, an unknown refusal fails the reading, and the refusal is the first difference.
* `GameLogicTests/JoinTests`: a joining connection is sent nothing and counts for nothing until its join; an accepted join is answered first, and its commands are numbered from 0. A join of another protocol, build or content, or one cut short, is answered with its refusal and closed, and its connection is never journaled.
* `GameClientTests/ClientSessionTests`: the join goes first and is not numbered; a refusal closes the session and shows its reason.
* `NeuronCoreTests/FilesTests`: the stamp is unchanged with the files and changes when one is written again.
* T9's run checklist: a client rebuilt while its server runs is refused, and the owner sees why.

## Pros and Cons of the Options

### 1 — a join with the programs' stamp

* Good, because it needs no build step and catches every rebuild.
* Bad, because the stamp is of the files, not of what they contain.

### 2 — the generated identity

* Good, because it is ADR-057's shipping form.
* Bad, because it misses uncommitted builds and needs a generated header.

### 3 — the transport compares

* Good, because nothing reaches the world before the transport agrees.
* Bad, because it gives no reason, and only QUIC has it.

## More Information

* Owner decisions O108, O109; MVP-071, MVP-072, MVP-090; the [M4 plan](../implementation/m4_plan.md), task T8.
* Amends [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md) (item 6's identity at M4), [ADR-057](ADR-057-version-identity.md) (the interim build identity), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md) (the sender closes a connection) and [ADR-124](ADR-124-battle-verbs-at-m1.md) (the join is not numbered).
* The code: `GameShared/JoinMessages`, `NeuronCore/Files` (`FilesStamp`), `NeuronServer/ServerHost`, `GameLogic/WorldServer`, `GameLogic/IntegratedServer`, `GameClient/ClientSession`, `Species/Species.cpp`, `SpeciesServer/SpeciesServer.cpp`, `Tools/SpeciesFrameBench/SpeciesFrameBench.cpp`.
