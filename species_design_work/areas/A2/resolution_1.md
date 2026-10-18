# A2 resolution, round 1

> Proposer: A2-P · Date: 2026-10-18 · Answers: `species_design_work/areas/A2/challenge_1.md` (18 findings: 0 Critical, 6 High, 11 Medium, 1 Low; 5 "make it better" items) · Checks: `python3 -I species_design_work/scripts/A2C_ServerBudgetChallenge.py Tools/DesignModels/A2ServerBudget.py all` run on the round-0 model (every figure the challenge quotes reproduced); `wall` run again on the revised model (8 vCPU ×1.01, 16 vCPU ×2.24, against the revised model's ×1.02 and ×2.22). The challenger's `cost` and `caps` modes stop on the revised model, because the parameters `community_cores` and `region_body_budget` were renamed (the body budget is now derived, ADR-080 item 3); the revised model computes those checks itself.

## Summary

| Resolution | Findings | Make it better |
|---|---|---|
| Accepted | 16: F01, F02, F03, F04, F06, F07, F08, F09, F10, F11, F12, F13, F14, F15, F17, F18 | 2: MB-1, MB-5 |
| Rejected | 0 | 0 |
| Escalate (gate B) | 2: F05 (the second server operating system), F16 (identity, listing and relay) | 3: MB-2 (overload lever, amends PIL-057 and WLD-244), MB-3 (measure before gate B), MB-4 (the record-store engine) |

Two parts are accepted with a stated difference: F18 item 1 (A2 still writes `drop` rows naming an ADR, because its chapter has no requirement prefix and an `adapt` row must name a defined requirement; gate 12's `--final` mode tracks the conversion), and MB-3's timing (the design run writes no code; the measurement becomes the first task of the simulation-core milestone, and gate B confirms ranges, or the owner chooses to wait).

**Revised model** (`Tools/DesignModels/A2ServerBudget.py`, rewritten; `check`: 33 figures and 12 tables, 0 differences). New terms: the seen layer (0.5 µs per body per update, D9 placeholder), keyed draws (0.03 µs each, SipHash-2-4), fidelity switches with a 60 s hold, the tick's wall time (region steps as single-threaded jobs scheduled longest first, the serial world step, replication jobs, the consistent cut's copy), the cost multiples at the wall budget and the deadline, 0.4 to 1.0 reference cores per vCPU, 20 Hz columns everywhere, the reserved body budget, the population limit each measured multiple allows, skirmish views and caps, a two-tick baseline, the baseline history ring, the first join's terrain, side masks and a decision track in replays, no Windows codec in persisted data, the pre-migration copy, and a latency Monte Carlo (100,000 samples, seed 1) with the client terms.

## Findings

### A2-F01 — The stage-3 margin is 1.14 to 1.30 times, not about twice (High) — Accepted

The challenger is right, and with the terms of F07 counted it is worse: the 8-vCPU host reaches its 50 ms wall budget at ×1.02 the cost budgets (49.2 ms per tick at ×1, 7 workers and one vCPU for the tick, I/O and the store), and its 100 ms deadline at ×2.04. Changes:
- The model computes the wall time (`wall` mode, Table "wall") and its `check` covers it; ADR-080 item 6 states the wall budget (50 ms) and the deadline (100 ms) separately, and the host specifications by the multiple at each.
- **Margin rule** (ADR-080 item 8): a recommended host holds its stage's stress case within the wall budget at twice the cost budgets and within the deadline at four times. Stage 3: **16 vCPU** (22.7 ms; ×2.22 at the budget, ×4.46 at the deadline; ×1.48 at 0.4 reference cores per vCPU), 4 GB. Stage 2: **4 vCPU** recommended (20.1 ms; ×2.51), 2 vCPU the minimum (28.7 ms; ×1.75). Integrated: 12.0 ms on the tick thread and 2 workers (×4.20); its average budget of 1 reference core holds to ×3.1 (0.32 cores at ×1).
- The vCPU factor is a range until DEC-971 pins the machine: the wall table shows 0.4 per vCPU beside 0.6 (8 vCPU: 73.8 ms at 0.4, 29.5 ms at 1.0).
- The region step budget and the tolerance are consistent: the region bound under the caps (two battles at the side cap at two of its settlements, the other two settlements at work; F06) is 8.05 ms against a 12.5 ms step budget, headroom ×1.55, so +25 % stays inside it (10.06 ms).
- §4.4 "What 200 does not need" is rewritten: one process per world needs the 16-vCPU host; region hosts are the reserve beyond ×2.2.

### A2-F02 — The remedy for a cost overrun cuts battles, and the cost basis may be six times low (High) — Accepted

- ADR-080 item 2 states that 2.1 µs is the whole-tick reading of DBG-205 and gives the per-frame reading beside it (×6: 1.92 reference cores for the integrated worst case, 11.13 for 200 at stress; the sensitivity table shows ×1, ×1.25, ×2, ×4 and ×6).
- The remedy order in ADR-080's confirmation is reversed: (1) split a region step's read-only hot phases into parallel jobs (ADR-083 item 6), which restores the region step without changing a rule; (2) a larger host (the wall table); (3) the full-fidelity settlement cost: B1's population limit by the sensitivity table (at ×2, 450 with the side cap equal to the limit), and the overload lever of MB-2 if the owner approves it; (4) the battle side cap last, never below 200 per side (a battle of 400, PIL-021's typical) without an owner decision.
- Gate B is asked to confirm the population limit and the battle side cap as provisional values with ranges (limit 450 to 700, side cap 200 to 600) that the measurement fixes (owner question 1), or to wait for the measurement (MB-3).
- At ×6 no lever but a smaller settlement keeps the region step (limit 150), which is the case for measuring first; the chapter says so.

### A2-F03 — 10 Hz fails ADR-079's own latency confirmation once the client is counted (High) — Accepted

- The latency model is now the Monte Carlo with the client terms (input sampled at the next frame, one queued frame, half a scan-out, exponential network jitter of 5 ms each way, the server's step: 20 ms at 16, the 16-vCPU host's 22.7 ms at 200). Results, mean / p95: an order on the host 212 / 258 ms; remote at 50 ms 302 / 350 ms; at 150 ms 402 / 450 ms; at 20 Hz 127 / 152 and 217 / 245 ms.
- **MB-1 is adopted** (ADR-084 item 6): the client predicts the movement of the unit that its steward directly controls, with the simulation library's pure movement functions over the replica, and blends to the server's state over 100 ms; crowds stay interpolated, and every effect stays on the server. Predicted direct control: 42 / 53 ms on every client, at 10 and at 20 Hz.
- **Immediate send** (ADR-087 item 5): a changed continuous control state is sent at once, besides once per tick period, so direct control without prediction (a turret's aim before its prediction exists) has the latency of an order, not +0 to 100 ms.
- **Thresholds split** (ADR-079 confirmation): orders p95 at most 275 ms on the host and 375 ms remote at a 50 ms round trip (model 258 and 350); predicted direct control p95 at most 60 ms on every client (model 53); effects of direct control (hits) at the order thresholds.
- **20 Hz beside every budget**: CPU (Table "world_cpu" last column), the wall multiple at the 25 ms budget of 20 Hz (16 vCPU ×1.10, so a 20 Hz world of 200 needs about 32 vCPU for the 2× margin), host upload (15 remote stewards: 4.4 Mbit/s at 20 Hz, above the 4 Mbit/s default), bandwidth per view. Gate B confirms the 10 Hz budgets with these beside them, so a later move to 20 Hz does not reopen it.
- **Self-decodable datagrams** (ADR-086 item 3): each datagram of a tick's update carries whole records of a disjoint set of figures, each a delta against the same acknowledged baseline whose id it carries, so a loss costs only its figures for one tick, which interpolation over two states hides.

### A2-F04 — Staggered checkpoints restore an inconsistent world, and an integrated host can duplicate an army (High) — Accepted

- ADR-083 item 9 and §4.3.1 item 2: the stagger is replaced by a **consistent cut**: at one tick boundary, after every owner's step of that tick, every dirty owner's state, the messages produced in that tick for the next, and every id counter are copied (a memory copy on the tick's workers, 1.3 ms in reference cores at 200 stewards, spread over the workers), and the store thread writes the set as one transaction with a manifest that names the tick. A restore loads the newest complete manifest and re-delivers its messages; no other state exists, so no event can be half-restored and no id can be issued twice.
- Period: every 600 ticks (60 s of world time); in an integrated world, every 100 ticks (10 s) while a battle runs, so a host who ends the process can wind a battle back at most 10 s (model: 1.4 MB every 10 s, 141 KB/s at 16 stewards).
- Every unclean stop is recorded in the world record with the world time it lost and whether a battle ran (PIL-131, PIL-132), and the record extends the hash chain of F11, so it cannot be removed.
- ADR-083's confirmation adds the forced-kill test at every phase of a crossing and of a battle's end; D9's threat model gets the deliberate-kill case (cross-area request).

### A2-F05 — A second operating system cannot be built under AGENTS.md (High) — Escalate (gate B question 4), with the true statements accepted now

Accepted now, without assuming the owner's answer:
- Table 1 and ADR-083 item 1 say what is true under O21: the dedicated server is a Windows build; the server and simulation libraries reach the operating system only through the platform layer, so a port is a port of the platform layer and of the in-box services it wraps, never of the simulation; nothing claims that another operating system "can build" it.
- Persisted and transferred data do not depend on a Windows codec: records, backups and replays are stored uncompressed in the exact encoding (replays 3.6 MB per battle; 167 MB at 16 and 1.65 GB at 200 retained), and XPRESS stays for the client's content packs only (ADR-058).
- ADR-081 item 2 gains the portability rules of F13, and the transport uses a standard protocol (DTLS 1.2, F15) that another platform's library would speak on the wire.

Escalated: whether there is a second server operating system, when, and with which exceptions to `AGENTS.md` and R14; the options and costs are owner question 4. The challenger listed the nine items without a recommendation; the vision recommended Windows and Linux. A2 recommends option A (Windows in stage 2, the core kept portable), with option B as a later stage-2 milestone if the owner approves its exceptions.

### A2-F06 — The region body budget can be filled by a rival, and the battle-size cap can make a settlement unassailable (High) — Accepted

- ADR-080 item 3: the **region body budget is 4L + 2S**: each of the region's four sites reserves its population limit L (600), and visitors (figures of settlements of other regions) use only the remaining 2S (1,200), two battles' visiting sides. A settlement's growth is bounded only by its own limit, never by visitors; an expedition that would take a region's visitors above 2S waits at the region border on its own side, in its own region, where its settlement's reservation holds it. Body-budget denial is impossible.
- ADR-080 item 4: the battle cap is **per side**: at most S = 600 figures of one side inside a battlefield (every figure of the side within the bounds, B4 specifies the counting), so a battle has at most 1,200. With S = L, a settlement at its population limit can always be met by an equal force; the defender's advantage is B6's balance, not a cap.
- The bound of a region under these caps is 8.05 ms (F01); at L = 600 the settlements take 67 % of the body budget (WLD-281's 75 % holds).
- D9 and B6 get both scenarios (cross-area requests).

### A2-F07 — The model omits the seen layer, fidelity switches and a credible AI (Medium) — Accepted

- The model counts the seen layer (0.5 µs per body per update, every tick in battle and every 5 ticks otherwise, placeholder of D9): it is part of every per-figure cost now (a settlement at full fidelity 0.80 ms per tick, a battle of 400 1.15 ms). The world of 200 at its stress case is 185.5 ms per tick (1.85 reference cores) against 167.3 before.
- ADR-085 item 6: a settlement stays at full fidelity for at least 60 s after the last settlement view leaves it, and the model counts the switch cost (2 µs per figure materialised): 0.56 ms per tick at 200.
- The AI budget stays 20 µs per settlement per tick as a target, with its range in the model (100 µs: 206.0 ms per tick at 200; 200 µs: 231.6 ms); the 16-vCPU host holds the 200 µs case (×2.22 is computed at 20 µs; the AI's 46 ms more fit under its ×2 margin).

### A2-F08 — Bandwidth beyond the views of Table 9 (Medium) — Accepted

- Skirmish views at 2,000 and 4,000 units are in the model: 50.5 and 69.8 KB/s; skirmish has its own client cap of 64 KB/s (ADR-086 item 4), within which the design population fits at a two-tick baseline (54.5 KB/s); the in-process connection has no cap (its tiers still apply), so the host's own view of a skirmish is never trimmed.
- First join: a joining client's blob channel has its own budget of 256 KB/s; client terrain is 16-bit quantised heights (D8 decides the format) streamed by level of detail, the strategic level first: 6 s at 16 stewards and 62 s at 200; the 10 s rejoin target holds with the cache.
- The baseline history ring (32 ticks) is in the memory table: 28.7 MB at 16 (16 clients) and 84.2 MB at 200 (47 clients); server memory 60 and 382 MB.
- The 22-bit tier-1 figure holds at a one-tick baseline; the table gives the two-tick baseline beside it (+4 to +8 %), and the cap check uses the two-tick figure.

### A2-F09 — Simulation draws are predictable from a seed the host is shown (Medium) — Accepted

- ADR-082: simulation draws are keyed by a **secret 128-bit simulation key**, drawn by the server at world creation (or at a match's start), stored in the world's records, never shown, never replicated, separate from the public world seed that world generation keeps (WLD-161); the function is **SipHash-2-4**, a keyed function whose outputs do not reveal its key, over the domain, purpose, subject, tick and ordinal (40 bytes). Cost: 0.03 µs a draw, 1.35 ms per tick at 200 stewards in the stress case (0.7 %).
- An owner-level draw uses the owner's id as its subject, never 0; a second draw context for one subject, purpose and tick is a Debug assertion; no raw draw value is ever replicated.

### A2-F10 — Fog of war holds for remote clients only (Medium) — Accepted

- §4.3 and ADR-084 state the trust model: the integrated host holds the server in its process and is trusted with full information; fog of war protects every steward from every remote client, not from the host; attacks between stewards are off by default in integrated worlds (O33), and the join page tells a steward that the world host can see the whole world (C2 and D9 word it).
- Replays (ADR-086 item 6): one stream per battle with a 2-bit mask per figure record (seen by side one, seen by side two), served by the server to a steward of a side filtered by that side's seen layer at each tick, never downloaded raw; 3.6 MB per battle with the masks and the decision track.

### A2-F11 — An operator can edit the store and the record that should reveal it (Medium) — Accepted

- §4.3.1 item 6: every world-record entry and every cut's manifest extends a SHA-256 hash chain; each client keeps, per world, the newest chain hash it was shown (32 bytes) and, on rejoin, requires a chain that extends it; a rollback, a restore or a rewritten entry that the steward saw is detected and shown to that steward.
- Stated limits: the chain cannot detect an edit of the current state by an operator who controls the server (a state never shown to a client, or an edit followed by a new entry); D9 owns the community-trust wording.

### A2-F12 — Same-tick commands in arrival order let the host win every race (Medium) — Accepted

- ADR-087 item 4: the commands of one tick are grouped by sender, the senders ordered by a keyed permutation of the tick (ADR-082, domain 3, keyed by the tick and the sender's stable id), and each sender's own commands keep their order; latency no longer decides a same-tick race.
- Skirmish offers a match option that delays the host's commands by the median one-way latency of the remote players; B9 sets its default.

### A2-F13 — Level 2 hazards beyond floating point (Medium) — Accepted

- ADR-081 item 2 adds: simulation and record code uses fixed-width integer types only (no `long`, `long double` or `wchar_t`); every sort or selection whose result reaches an outcome is stable or uses a total order that ends with the stable id; no `std::hash` value and no pointer value reaches an outcome or an order; the checker and clang-tidy enforce them; these join the R18 candidates for gate C.
- ADR-081 item 6 notes that Level 2 between the client's integrated server and the Windows dedicated server holds by construction (one compiled server library, `/fp:precise` without contraction), which is what world generation needs today.

### A2-F14 — No way back from a bad patch; crash safety unstated (Medium) — Accepted

- §4.3.1 item 5: before a migration the store keeps a pre-migration copy of the whole world for 14 real days (156 MB at 200 stewards, in the storage table); every migration runs first on a copy and compares record counts and owner hashes before switching; a server of the previous release opens the pre-migration copy (the world days since the patch are lost, and the world record says so).
- §4.3.1 item 1: the write protocol (a cut is one transaction; each record set carries a checksum; the manifest is written last; with an engine, its own write-ahead log; with own files, a new file flushed with write-through and replaced atomically on the same volume); the store lives in the user-data folder of ADR-071, and the world host is warned when that folder is synchronised by a cloud client.
- The engine (ESE, own format or an R14 exception) is owner question 5 (MB-4).

### A2-F15 — Species designs its own encrypted handshake where Windows ships DTLS (Medium) — Accepted

- ADR-088 item 4: `UdpTransport` is secured by **DTLS 1.2 from the Schannel SSP through SSPI** (in-box on Windows 10 version 1607 and later, Windows 11 and Windows Server 2016 and later; verified on Microsoft Learn, 2026-10-18), with its stateless cookie against spoofed-source amplification; Species keeps its own reliability, channels and fragmentation above it; each server generates a self-signed certificate at first start, pinned by the server listing or on first use with its fingerprint shown. No own cryptographic protocol remains.
- The model's datagram overhead is now 77 bytes (IP and UDP 28; the DTLS 1.2 record with an AES-GCM suite 37: a 13-byte header, an 8-byte explicit nonce and a 16-byte tag; Species' header 12), up from the round-0 estimate of 60, which raises every view by 1 to 7 %: a battle view 23.2 KB/s, a skirmish at its design population 50.5 KB/s, the integrated host's upload for 15 remote stewards 2.8 Mbit/s, hosted egress at 200 stewards 584 GB per 30 days.

### A2-F16 — Identity, listing and relay (Medium) — Escalate (gate B question 6), with the R14-clean baseline accepted now

- Accepted now (ADR-088 item 4, MB-5): each player has an identity key (ECDSA P-256 in CNG, in the user's key store) that signs, inside the DTLS session, a server nonce bound to the server certificate's fingerprint; a server registers the key on first join, and a returning steward reclaims the settlement by it (ADR-086 item 5). Reachability for the first world-game milestone: UPnP port mapping through the in-box NAT API, IPv6 and a manual address; a host behind carrier-grade NAT is unreachable until a relay exists.
- Escalated: the identity, listing and relay provider (Steamworks SDK, Epic Online Services SDK, PlayFab Party, or an own service) with the storefront (ADR-072) and the publisher (ADR-073), owner question 6. The challenger and A2 agree on the R14-clean baseline for the first world-game milestone.

### A2-F17 — The client's composition root can reach the server's state (Medium) — Accepted

- ADR-083 item 2 and ADR-084 item 2: the integrated server is composed through one header of the server library, `IntegratedServer.h` (start, stop, a `Transport` endpoint); the checker allows the client executable that header and no other server header; every host tool and diagnostic reaches the server through the administration interface (ADR-088 item 7), as a remote operator's does.

### A2-F18 — Ledger and model consistency (Low) — Accepted, item 1 with a stated difference

1. Not changed: A2's chapter has no requirement prefix (the brief), and gate 12 rejects an `adapt` row that names no defined requirement (an ADR id is not one), so the rows stay `drop` with the ADR named as the replacement. The conversion is tracked: gate 12 reports the three adopted requirements that cite a dropped id, and under `--final` they fail until D2's WSIM requirements exist and the rows become `adapt` (cross-area request to D2, unchanged).
2. Accepted: the §2.2 row's note says that the tick source of R-SIM-510 is the server's tick loop (ADR-083 item 7), whose command batch is ordered by ADR-087 item 4.
3. Accepted: ADR-083 item 7 states skirmish game speed: the server runs world ticks within each tick period by accumulating the game speed with the fraction carried (R-SIM-548), at most 2 per period at Time Rush's 2.0, keeping one command frame per period (R-SIM-549); the CPU and bandwidth of the match double for the hold of 151 world ticks, which the skirmish budget covers (one match per server). DEC-264 stays D2's.
4. Accepted: the model's latency is the Monte Carlo now (the old half-frame text is gone), and the hash term uses `hash_period_ticks` (10) rather than the tick rate.
5. Accepted: replays carry a decision track: the deputy's and the AI's decisions with the standing order that permitted each (PIL-140), from the command journal of that side.

## Make it better

| Item | Resolution | What changed |
|---|---|---|
| MB-1 Predict only the steward's directly controlled units | Accepted | ADR-084 item 6; the client session may call the simulation library's pure movement and terrain-query functions (never an owner) for the predicted unit; predicted direct control 42 / 53 ms; ADR-079's confirmation splits its thresholds. Estimated 2 to 3 engineer-weeks (the challenger's estimate) |
| MB-2 An overload lever that spares battles | Escalate (owner question 7) | Proposed as ADR-085 item 9, not in force until the owner amends PIL-057 and WLD-244: under a sustained overload (the tick over its wall budget for 10 s), settlements watched only as neighbours fall to abstract fidelity with a stand-in presentation, least recently viewed first; a steward's own settlement and every battle never do. 101.0 ms of the 200 stress case's 185.5 ms per tick (54 %) in the revised model (the challenger's 57 % on the round-0 costs). A2 recommends approval, as the second lever after the host |
| MB-3 Measure before gate B | Escalate (owner question 1, option b) | Recommended instead: gate B confirms provisional ranges, and the measurement is the first task of the simulation-core milestone, before any content is tuned (ADR-080 confirmation). The design run writes no code, so measuring before gate B pauses the run by about one engineer-week |
| MB-4 The in-box ESE for the record store | Escalate (owner question 5) | Recommended if the dedicated server stays Windows in stage 2 (question 4, option A): ACID transactions, crash restore at start-up and online backup, R14-clean; the cut of F04 is one transaction; the records keep the exact, portable encoding inside the engine, and a world moves between servers by an export file in that encoding |
| MB-5 One in-box security stack | Accepted | ADR-088 item 4: Schannel DTLS 1.2 plus the identity key signing a nonce bound to the server certificate; no service, no R14 exception, no own protocol |

## Owner questions for gate B (one merged list, recommended option first)

1. **Scale budgets** (ADR-080 items 3 and 4). (a) Recommended: confirm as provisional values with ranges that the first task of the simulation-core milestone fixes: population limit L = 600 per settlement (range 450 to 700), battle side cap S = 600 (range 200 to 600; a battle at most 1,200), region body budget 4L + 2S = 3,600, region step 12.5 ms; world sizes as A1 set them (8 regions at 16, 77 at 200). (b) Wait to confirm them until a one-week measurement spike has run (MB-3). Trade-off: (a) keeps the run moving and fixes the values by measurement before content is tuned; the region bound is 8.05 ms against 12.5 (holds at +25 %), and at ×2 the limit falls to 450; (b) costs about a week and gives gate B measured costs. The challenger asked for (b) or ranges; A2 recommends (a).
2. **Battle caps** (WLD-285). Recommended: 2 per region and max(8, ⌈10 % of settlements⌉) per world: 8 at 16, 26 at 200. Alternative: A1's max(4, 10 %). Trade-off: 8 at 16 costs up to 4.6 ms per tick more and removes waiting in the planning case (4 % to 0 % of the evening peak; 11 % to 0 % at the session pace); at 200 the cap of 26 is reached in 1 % of the stress case's peak. No disagreement.
3. **Host specifications** (ADR-080 item 8). Recommended: stage 3, 16 vCPU and 4 GB (×2.22 the cost budgets at the 50 ms wall budget, ×4.46 at the deadline); stage 2, 4 vCPU (×2.51), with 2 vCPU as the minimum (×1.75); the integrated server, 1 reference core and 2 workers (0.32 cores at the stress case). Alternative: 8 vCPU for stage 3 with region hosts designed before stage 3 starts. Trade-off: 16 vCPU doubles the rented host but needs no inter-process protocol until beyond 200 stewards; 8 vCPU reaches its wall budget at ×1.02, so it depends on region hosts or the overload lever from the first measured overrun. The challenger offered both; A2 recommends 16 vCPU.
4. **Dedicated-server operating systems** (F05; O21; vision §6 item 10). (A) Recommended: a Windows dedicated server in stage 2; the server and simulation libraries keep every operating-system call in the platform layer, fixed-width types and total orders, and no Windows codec in persisted data, so a port is a platform-layer port. (B) A Linux dedicated server in stage 2, as an `AGENTS.md` change: Clang 18 or later with libc++ (or GCC 14 with libstdc++), built by Visual Studio's MSBuild Linux projects so that the `.slnx` and MSBuild stay; the R16 settings as `-std=c++23 -O2 -mavx2 -ffp-contract=off -fno-fast-math -Werror`; R14 exceptions for OpenSSL 3 (DTLS and HTTPS, from the distribution) and for the store engine if A's ESE is chosen (or the own store); an own crash handler; a Linux CI job and the Level 2 corpus on both builds; A2's estimate 7 to 11 engineer-weeks, then every change tested on two builds. (C) Defer Linux to stage 3, where the hosting operator chooses. Trade-off: most community hosts rent Linux, and Windows VPS plans usually cost more because of the Windows licence, so A narrows stage 2's community; B buys that reach for the exceptions and the weeks above. Where A2 and others differ: the vision recommended Windows and Linux; the challenger listed the costs without a recommendation; A2 recommends A, with B as a later stage-2 milestone if the owner approves its exceptions.
5. **Persistence technology** (MB-4, F14; DEC-250, DEC-251 are D4's). (a) Recommended if question 4 is A: the in-box Extensible Storage Engine (ESE) for stages 1 and 2: ACID transactions, crash restore and online backup, R14-clean, records in the portable exact encoding inside it, worlds moved by an export file. (b) Species' own journaled record store in plain C++: portable, but Species writes the write-ahead log, torn-write detection and restore (A2's estimate 3 to 4 engineer-weeks with tests). (c) SQLite: portable and proven, an R14 exception. Stage 3 may add a server database behind the same store interface by a later record. The challenger proposed (a); A2 agrees, conditional on question 4.
6. **Identity, listing and relay; storefront and publisher** (F16; ADR-072, ADR-073; DEC-248, DEC-249 are D5's). (a) Recommended for the first world-game milestone: the R14-clean baseline (an identity key per player, UPnP through the in-box NAT API, IPv6, a manual address; no relay, so hosts behind carrier-grade NAT are unreachable; no cross-server identity, so PIL-088's second-account detection is weak). Then, with the storefront, one provider: (b) the Steamworks SDK (identity and entitlement tickets, server browser and lobbies, a relay; an R14 exception; ties identity to one storefront); (c) the Epic Online Services SDK (cross-storefront identity, sessions and lobbies, relays; an R14 exception); (d) PlayFab Party with Lobby (a Microsoft NuGet package not on the R14 list; a metered service); (e) an own relay and listing service (no exception; the relay's egress is 2.8 Mbit/s for one relayed world of 15 remote stewards in battle views, 5.0 GB per 4-hour session). The owner names the storefront and, if known, the publishing entity. No disagreement on (a).
7. **Overload lever** (MB-2; amends PIL-057 and WLD-244). Recommended: approve, as an overload-only lever: under a sustained overload, settlements watched only as neighbours fall to abstract fidelity with a stand-in presentation; a steward's own settlement and every battle never do. Alternative: no lever; an overload slows world time. Trade-off: up to 101 ms of the 200 stress case's 185.5 ms per tick (54 %) is recoverable without touching battles, for a stand-in presentation (C3, the renderer) and the abstract model's accuracy holding for watched neighbours (B2).

For gate C: amend `AGENTS.md` R16's FMA sentence (MSVC stopped contracting under `/fp:precise` in Visual Studio 2022; the challenger confirmed it); R18 candidates: no CRT transcendental in simulation code, keyed draws only, no floating-point pragma or `/fp:contract`, no operating-system header outside the platform layer, fixed-width types, stable or total orders, no `std::hash` or pointer value in an outcome, owners hold all mutable simulation state.
