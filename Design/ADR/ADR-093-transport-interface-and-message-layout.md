---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M0 task T5)
---

# Transport interface, the in-process transport and the bit-packed message layout

## Context and Problem Statement

ADR-088 fixes the transport's shape at the level of a decision:
- one `Transport` interface with an in-process and a QUIC implementation (item 1);
- a reliable ordered channel, an unreliable sequenced channel of state datagrams of at most 1,200 bytes whose fates are reported, and a stream per blob (items 2 and 3);
- bit-packed, schema-defined, versioned messages, length-prefixed on a stream, with a hard limit on every count, length and depth, where a malformed, oversized or unknown message is dropped and counted (item 5).

ADR-090 puts the transport in NeuronCore and the game's message schemas and codecs in GameShared. M0 task T5 builds the interface, `InProcessTransport`, the framing and the codec skeleton that generated codecs will target (ADR-062), with tests that send, receive and close in both directions (MVP-090).

The specifications leave open:
- the calls of the interface and what each does at a close;
- the bounds of the in-process queues, and what a full one does;
- the bit order and the field forms of the layout;
- the message header, the size limits and the version rule;
- how a stream marks a message's length.

## Decision Drivers

* The client and the server run the same code over either transport (O10, DEC-207 item 11): the in-process transport keeps the channels, the order and the datagram fates of the remote one.
* No input crashes, reads outside its buffer, allocates without a bound or recurses without a limit (DEC-207 items 4 and 5).
* A codec costs only the bits its fields need (DEC-201 item 11), and the generator of ADR-062 can emit it field by field.
* Nothing waits: every call returns at once, so neither the frame loop nor the tick loop stalls on the other (PLT-067, ADR-083).

## Considered Options

* 1 — an interface per connection end that carries encoded buffers; an in-process pair of bounded queues behind one lock; a little-endian bit layout with bounded fields and byte-aligned strings
* 2 — a typed interface that sends and receives message structures, with encoding inside each transport
* 3 — option 1 with lock-free single-producer, single-consumer rings in place of the lock

## Decision Outcome

Chosen option: "1".
- Option 2 would put the codecs inside the transport and duplicate them in each implementation, against ADR-088's encoded buffers.
- Option 3 buys nothing measurable at M0's message rates, and it would make the drop-oldest rule and the datagram fates harder to get right. It stays open for when M4 measures the transport.

* **The interface** (`Neuron::Transport`) is one end of one connection:
  - `Send(channel, bytes)` queues an encoded message and returns the datagram's number on the state channel, 0 on the reliable one.
  - `Receive()` returns the next message of either channel in arrival order, or nothing.
  - `TakeDatagramOutcomes()` returns the fates of this end's datagrams in the order they were decided.
  - `Close()` closes both directions; `State()`, `Reason()` and `Stats()` report on the connection.
  - Every call returns at once. A send fails with `Closed`, `TooLarge`, `Empty` or `Backlogged`; a backlogged reliable message stays with its sender.
  - A server accepts through a `TransportListener`, and the in-process listener's `Connect` gives the client its end. Blob streams arrive with the first blob transfer, the join's snapshot, behind the same interface.
* **Close.** A close reaches the other end at once, as `ClosedByPeer`.
  - Messages already queued toward an end can still be received after the close, so a reliable message sent before it arrives, as a graceful stream shutdown delivers.
  - An end that is destroyed makes its unreceived datagrams lost for their sender.
  - `ProtocolError` is the reason a remote transport gives when the other end breaks the framing.
* **In-process queues.** The two ends share one link behind one mutex:
  - The reliable backlog toward an end is bounded at 4 MiB (`IN_PROCESS_RELIABLE_BACKLOG_BYTES`); a send above it fails with `Backlogged`.
  - The state queue holds at most 64 datagrams (`IN_PROCESS_STATE_QUEUE_DATAGRAMS`). A datagram sent to a full queue drops the oldest one queued, as a congested link would, and that datagram's fate is lost.
  - A datagram is acknowledged when the other end receives it, so the replication baseline of ADR-086 advances on the same events as over QUIC.
  - These bounds are chosen, not measured; M4 measures the transport and may move them.
* **Layout.** A message is:
  - a 16-bit type id and an 8-bit schema version, then the type's fields;
  - packed least significant bit first, with no names, tags or alignment;
  - unsigned and signed fields of 0 to 64 bits;
  - a bounded field (a count, a length or an enumeration) in the bits its maximum needs, where a larger value is malformed;
  - byte strings and UTF-8 strings as a bounded length and then the bytes, starting on a byte boundary so the reader can hand them out without copying;
  - text that is not well-formed UTF-8 (RFC 3629) is malformed;
  - the last byte padded with zero bits;
  - nested values counted against a depth limit that each field states.
* **Reading.** The reader fails at the first read that breaks a bound, and every later read returns zero. A codec therefore decodes to the end and asks `Finish` once, which also fails on anything left over but zero padding, and on a nested value left open.
* **Dispatch.** `MessageDispatcher` checks the message's size against `MESSAGE_MAX_BYTES` (64 KiB) and its type's own limit, the header, the type and the version. A message reaches its handler only once it has decoded whole. Each drop is counted by reason: unknown, wrong version, oversized or malformed.
* **Versions.** The version must match exactly until the schema language states how one version reads another. Generated codecs satisfy the `MessageCodec` concept (`TYPE`, `VERSION`, `MAX_BYTES`, `Encode`, `Decode`).
* **Stream framing.**
  - On a reliable stream each message is preceded by its length, a minimal unsigned LEB128 of at most 3 bytes, not zero and at most the limit.
  - A length that breaks this cannot be skipped: the reader stops, and the connection closes with `ProtocolError`.
  - The reader holds at most one message and the bytes after it.
  - The in-process transport passes whole buffers and needs no framing; `QuicTransport` frames its command stream with it.
* **Fuzzing.**
  - The first form is a seeded test of 100,000 inputs, random buffers and single-bit mutations of a valid message.
  - Each input must be delivered whole or dropped with a reason.
  - The CI fuzzer of ADR-069 with ADR-088's 10⁷ inputs runs every parser from M4, when the generated codecs exist.
* **Sockets.** `Build/CheckProjectFiles.py` rule 14 rejects a Winsock header anywhere, because MsQuic owns the socket (ADR-089).

### Consequences

* Good, because the integrated server and a remote one differ in one class, and the replication baseline reads the same fates from both.
* Good, because no input to the dispatcher can do more than be counted, and the reader can hand out strings without copying.
* Good, because a field of n bits costs n bits, and a generated codec is one call per field.
* Bad, because one lock per connection serialises the two ends' calls; at M0's rates this is invisible, and M4 measures it.
* Bad, because an exact version match makes every schema change a protocol change until the schema language states compatibility.
* Neutral, because the reliable backlog refuses rather than grows: the command path must keep a message it could not send, which the command queue of ADR-087 does anyway.
* It forecloses: a transport that interprets messages; a parser that throws or asserts on input; field names on the wire.

### Confirmation

* `NeuronCoreTests`:
  - `TransportTests` checks both directions on both channels, the order across two threads, a close from either end, the bounded backlog, the drop of the oldest datagram, the refusals, the fates of an end that goes away, and the listener.
  - `MessageCodecTests` checks the round trip of every field form, every bound of the reader, the dispatcher's reasons, the random inputs, and the framing under any split and every bad length.
* A `CheckProjectFiles.py` fixture fails on a socket header.
* At M4, ADR-088's confirmation runs the same session over both transports with identical per-tick hashes.

## Pros and Cons of the Options

### 1 — encoded buffers, one lock, the bit layout

* Good, because the transport stays a carrier and every implementation shares the codecs.
* Bad, because the lock is a cost to measure.

### 2 — a typed interface

* Good, because a caller never sees bytes.
* Bad, because each implementation encodes, and the in-process one would skip the encoding that the remote one tests.

### 3 — lock-free rings

* Good, because neither end ever waits for the other's lock.
* Bad, because dropping the oldest datagram and reporting its fate need both ends to agree on one queue, which a ring makes harder than a lock.

## More Information

* Related: [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-069](ADR-069-test-framework.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md), [ADR-089](ADR-089-msquic-transport-package.md), [ADR-090](ADR-090-solution-and-project-layout.md).
* The code is in NeuronCore: `Transport`, `InProcessTransport`, `BitWriter`, `BitReader`, `MessageCodec` and `StreamFraming`.
