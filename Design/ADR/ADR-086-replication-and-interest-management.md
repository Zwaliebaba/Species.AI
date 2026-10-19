---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead; proposed by design-phase area A2, with owner gate B applied (O44)
---

# Replication: server-authoritative state replication with interest tiers, self-decodable datagrams, per-client caps and snapshot joins

## Context and Problem Statement

The server is authoritative, and clients receive only the state they may see (vision §5); one netcode serves the world and skirmish (O8, O10); replication is interest-managed from day one (O1). The reference sends commands only and has every participant simulate everything, so its bandwidth does not grow with the population; Species does not use that model for the world (DEC-200). A reference battle peaks at 2,000 to 6,000 simulated objects, and per-object replication of it needs hundreds of KB/s per client raw (DEC-201 item 7). A Species client in the settlement view sees its own settlement and its neighbours, up to 6 settlements (WLD-244), and the battles in which its settlement is a side; in the strategic view it sees the public layer and its settlement's seen layer as marks (WLD-241, WLD-242). The integrated host serves its remote stewards from a home connection (O10). DEC-205 asks how a player joins, leaves and returns; PIL-140 asks whether battle replays are feasible and who may watch them.

How does each client receive the world, within what budget, and how does it join?

## Decision Drivers

* Fog of war enforced by never sending hidden state to a remote client (PIL-084, D9), in live play and in replays.
* A per-client budget with graceful degradation instead of queue growth (DEC-201 item 12), and a lost datagram that costs one tick of a few figures, not a whole update.
* The integrated host's upload, and a hosted world's egress, at 16 and at 200 stewards.
* Join and reconnect in seconds whatever the world's age (DEC-205 item 1).
* One model for the world and for skirmish (O8).

## Considered Options

* A — one server-authoritative state replication model for the world and skirmish: interest management, quantised delta updates in self-decodable datagrams, client-side interpolation
* B — A for the world, and a deterministic command-stream model (every participant simulates) for skirmish only
* C — A, plus deterministic client-side simulation of selected content, re-anchored by snapshots

## Decision Outcome

Proposed option: "A", with crowd aggregation (DEC-201 option C) as its far tier. **Default while Proposed:** the decision below; its figures are the model's (`A2ServerBudget.py bandwidth`), computed, not measured.

1. **Interest set.** The server computes each client's interest set every tick from the public layer and its settlement's seen layer (WLD-242), which area D9 specifies, intersected with the client's view: the settlement view's areas (WLD-244) or the strategic view. Nothing outside the interest set is sent, and the AI and the deputy read the same seen layer on the server (PIL-084).
2. **Three tiers.** Tier 1: figures inside the settlement view's footprint within 1,200 wu of the camera's ground point, at most 1,500, nearest first, sent every tick with positions quantised to 1/16 wu relative to their region's corner. Tier 2: the other figures of the interest set inside the settlement view's areas, every fourth tick, quantised to 1/4 wu. Tier 3: aggregates (an expedition's mark with a band of its body count, a battle's mark, a settlement's summary) once per second, and nothing per figure; the strategic view receives tier 3 only (WLD-245).
3. **Encoding and datagrams.** A state update is a per-client delta against a baseline that the client has acknowledged, of records in stable-id order with run-length skips, in a schema-defined bit-packed layout with no field names (DEC-201 item 11, ADR-088). A figure sends its position and, on change only, its task, health and facing; no velocity is sent, because the client interpolates (ADR-084). Laser bolts, grenades and other projectiles are spawn events with their start, target and tick; Essence orbs send their position twice per second; structures send state on change. A figure entering the interest set is sent whole once. Each QUIC datagram of a tick's update (ADR-088 item 2) is **self-decodable**: it carries whole records of a disjoint set of figures, each a delta against the same baseline, whose tick it names, so a lost datagram costs only its own figures for one tick, which interpolation over two states hides; a datagram that MsQuic reports acknowledged makes its records' states the client's baseline. The server keeps each client's sent states for 32 ticks (3.2 s); a figure whose acknowledged state is older is sent whole.
4. **Budgets.** Downstream per client in the world: a target of 32 KB/s for typical views and a cap of 48 KB/s; in skirmish a cap of 64 KB/s. The sender enforces the cap each tick (4.8 KB or 6.4 KB per tick): an update over the cap moves the farthest tier-1 figures to tier 2, then tier-2 figures to tier 3, and never queues. The in-process connection of the integrated host's own client has no cap; its tiers still apply. Upstream per client: at most 4 KB/s. A joining client's blob channel (terrain chunks and the first snapshot) has its own budget of 256 KB/s beside its cap. The integrated host has an upload budget, 4 Mbit/s by default and a host setting, divided equally among its remote clients up to each client's cap.
5. **Join, reconnect and leave** (DEC-205, option C). Every entry is a snapshot join: the client receives its interest set whole, then deltas; a reconnect is a join. A client receives terrain as 16-bit quantised heights (area D8 decides the format) by level of detail, the strategic level first, and keeps it in a cache keyed by chunk digest: a first join into a world of 16 streams its terrain in about 6 s, one into a world of 200 in about 62 s with the strategic view usable from the start, and a rejoin with the cache completes within 10 s. The world never pauses for a join. A returning steward reclaims the settlement from the deputy by the identity key of ADR-088 item 4, and a skirmish player its team from its bot, by a session token. When the integrated host's client closes, the world freezes (O4, PIL-131) and every remote client is disconnected with the freeze as its reason; no host migration exists. Skirmish spectating is a join with a delay of the spectator's updates of at least 30 s (DEC-205 item 10).
6. **Replays** (PIL-140; DEC-046 is area D2's). A battle replay is a recorded state stream: the server records, for each battle in which a side's steward is away, the tier-1 stream of an observer who sees the whole battlefield, at 5 updates per second, with a 2-bit mask on each figure record (seen by side one, seen by side two) and a decision track (each decision of the deputy and the AI with the standing order that permitted it, from that side's command journal), and keeps it for 7 world days. It is stored uncompressed in the exact encoding, so no codec of one operating system is needed to read it. The server streams a replay to a steward of a side, filtered by that side's seen layer at each tick, and never sends it raw; the client plays it with its own interpolation at up to 8 times speed (PIL-140). A command-stream replay would be about a tenth of the size (0.4 MB) but valid only on the release that recorded it and only under ADR-081's Level 2, so it is not used for players; the server's command journal serves defect reproduction.

### Consequences

* Good, because a client's bandwidth does not depend on the world's size: the same views cost the same at 16 and at 200 stewards, apart from the strategic view's marks (2.3 against 3.2 KB/s).
* Good, because the model's views fit at a two-tick baseline, the case after a lost acknowledgement: a home view 17.6 KB/s (18.4), a battle view of 400 figures 23.4 KB/s (24.7), a battle at the side cap of 1,200 figures 42.9 KB/s (45.5); a valley war of two battles and four settlements would need 63.8 KB/s and is sent within the cap with 747 of its 1,500 near figures at tier 1; a skirmish at its design population of 2,000 needs 51.0 KB/s (55.1) within its 64 KB/s cap, and one at its stress population of 4,000 is sent with 1,198 figures at tier 1.
* Good, because the integrated host's upload stays modest: 0.6 Mbit/s for 3 remote stewards at the evening peak, 2.8 Mbit/s for 15 remote stewards in battle views (4.5 at 20 Hz); a hosted world of 200 sends 8.8 Mbit/s at its evening peak and about 590 GB per 30 days; upstream is 1.1 KB/s per client.
* Good, because joins cost one snapshot, whatever the world's age, and a lost datagram never stalls the rest of the tick's update.
* Bad, because the server encodes per client (0.05 µs per figure per client per tick; 6.6 ms per tick at 200 stewards in the stress case) and keeps 32 ticks of sent states per client: 28.7 MB of server memory at 16 stewards and 84.2 MB at 200.
* Bad, because a valley war in view is shown with fewer full-rate figures than it has.
* Neutral, because replays cost storage instead of determinism: 3.6 MB per battle of 8 minutes with the masks and the decision track, 167 MB of retained replays at 16 stewards and 1.65 GB at 200.

### Confirmation

**Confirmed when** (amended 2026-10-19 for the MVP's milestones, O51 and O55): on the MVP's M3 build, a test client that receives every message the server sends to the steward's client holds no figure, structure or store outside the steward's seen layer (MVP-097); at M4, the client over MsQuic through the link simulator at 10 % loss and a 150 ms round trip for 30 minutes has no figure frozen for more than 2 ticks by a single lost datagram, and a rejoin at minute 20 completes within 10 s; and at the first milestone after the MVP that has remote stewards (the MVP builds neither the interest tiers nor the caps, its §5.1), 4 clients pass the same link within the per-client cap, a test client that requests or decodes state outside its interest set, live or in a replay, receives nothing beyond it, and a battle view of 400 figures measures within 10 % of the model's 23.4 KB/s.

## Pros and Cons of the Options

### A — state replication with tiers

* Good, because it meets O1, O8 and O10 with one netcode and keeps fog of war enforceable.
* Bad, because its bandwidth grows with the figures in view, which the tiers and the cap bound.

### B — a command-stream model for skirmish

* Good, because skirmish bandwidth would not depend on the unit count.
* Bad, because it doubles the netcode, puts bit-exactness on every client and needs O8 changed.

### C — client-side simulation of selected content

* Good, because it could lower the bandwidth of a watched battle.
* Bad, because it needs Level 2 on every client and a reconciliation; it is a later optimisation, not a starting model.

## More Information

* Resolves DEC-200, DEC-201 and DEC-205 (Resolved 2026-10-19). Related: [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md).
* The model's per-figure costs: a tier-1 figure that moves sends 22 bits per tick against a one-tick baseline and 24 against a two-tick one (8 or 9 bits per axis at 1/16 wu, and 6 bits of change mask and skip); a tier-2 figure 22 bits every fourth tick at 1/4 wu; 3 % of figures change state each tick, at 4 bytes. A QUIC datagram carries at most 1,192 bytes of records and 89 bytes besides them on IPv6 (69 on IPv4), as ADR-088 item 3 derives them from the framing: IPv6 40 (IPv4 20) and UDP 8, the QUIC short header 14, the DATAGRAM frame's type and length 3, the AEAD tag 16, and Species' header 8 (the update's tick and its baseline tick; sequencing and acknowledgement are QUIC's, DEC-207 item 2); computed, and measured at the MVP's M4.
* **Amendment 2026-10-18 (challenge round 1).** Item 3 makes datagrams self-decodable and keeps a 32-tick history (A2-F03, A2-F08); item 4 adds the skirmish cap, the uncapped in-process connection and the join blob budget (A2-F08); item 5 adds terrain by level of detail and reclaim by identity key (A2-F08, A2-F16); item 6 adds side masks, filtered serving, the decision track and uncompressed storage (A2-F10, A2-F18, A2-F05); the figures follow the revised model.
* **Amendment 2026-10-19 (owner gate B, O44).** State updates travel as QUIC datagrams of MsQuic (ADR-088 item 2), each acknowledged or reported lost by MsQuic, which makes an acknowledged update the baseline; the datagram overhead is 89 bytes on IPv6 (69 on IPv4) in place of DTLS's 77, which raises each view by 1 % to 2 % in the model (a battle view 23.4 KB/s against 23.2; hosted egress at 200 stewards 590 GB per 30 days against 584). The "Confirmed when" is re-pointed to the MVP's M3 and M4 and to the first milestone with remote stewards (O51, O55).
