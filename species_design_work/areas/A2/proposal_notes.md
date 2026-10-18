# A2 proposal notes (A2-P, revised after challenge round 1)

> Area A2, architecture strategy · Documents: `Design/architecture/04_solution_strategy.md` (Draft), `Design/ADR/ADR-079` to `ADR-088` (Proposed), ledger rows of `Design/gdd/adoption_ledger/12_simulation_model_ledger.md` (01 and 07) · Model: `Tools/DesignModels/A2ServerBudget.py` · Challenge round 1: `challenge_1.md`, answered in `resolution_1.md` · Date: 2026-10-18

## 1 Options considered

| Choice | Options | Chosen | Why |
|---|---|---|---|
| Tick rate (DEC-227) | A 10 Hz; B 20 Hz; C 30 Hz; D a higher full rate with a lower abstract rate | A, plus D's abstract cadence (every 10 ticks) and the prediction of the directly controlled unit; ADR-079 | Every reference tick value binds as written (O13, O17), so skirmish, the balance test bed, tests the reference rather than a conversion of hundreds of per-tick values, probabilities, factors and integration steps; half the CPU and bandwidth of B (1.85 against 3.71 reference cores at 200 stress; 23.2 against 36.9 KB/s battle view). Cost: orders 212 / 258 ms (mean / p95) on the host and 302 / 350 ms at a 50 ms round trip, against 127 / 152 and 217 / 245 at B; direct control predicted at 42 / 53 ms at either rate (MB-1). A conversion rule is fixed now, and the 20 Hz budgets stand beside the gate-B ones, so B stays one superseding ADR away. The register recommended D at 20 Hz; I disagree on the merits above |
| How the server executes a tick | Whole ticks with the 10 slices back to back; whole ticks without slices (re-express slice order in tick order); spread slices over frames | Back to back | Keeps every slice-dependent order of the reference (R-SIM-513, R-SIM-516) with no re-specification; slices cost nothing when run consecutively |
| Budgets (DEC-228) | A client-derived; B server-first; A+B with hard caps; C dynamic | A+B with caps, per-figure cost budgets, a 50 ms wall budget and a 100 ms deadline, hosts by a margin rule; ADR-080 | Both the integrated host and community hosts need a budget that can fail; C makes outcomes depend on hardware. The margin rule (×2 within the wall budget, ×4 within the deadline) answers A2-F01 |
| Region body budget (WLD-281) | a shared pool (round 0: 4,000); reserved 4L + 2S | 4L + 2S = 3,600 at L = S = 600 | Each site reserves its population limit, so no rival's waiting bodies block a settlement's growth (A2-F06); visitors use 2S; the region bound is 8.05 ms against a 12.5 ms step (×1.55 headroom) |
| Battle size cap | a total cap (round 0: 1,000); a per-side cap S | S = 600 per side, a battle at most 1,200 | With S = L a settlement at its limit can always be met by an equal force, so the cap cannot make a defender unassailable (A2-F06); a battle of 1,200 costs 3.32 ms |
| World battle cap | max(4, 10 %) (A1); max(6, 10 %); max(8, 10 %) | max(8, 10 %): 8 at 16, 26 at 200 | Removes waiting in the planning case at both paces (11 % to 0 % at the session pace) for up to 4.6 ms per tick; region cap 2 kept |
| Overrun remedy order | shrink battles first (round 0); split the region step, then the host, then the settlements' cost, battles last | battles last, never below 200 per side without the owner | O3 and PIL-021 put battles first (A2-F02); an in-region split restores a step without a rule change |
| Numeric model (DEC-240) | A reference; B strict binary64; C fixed point; D one build only | B with a three-level scope; ADR-081 | Decided by what needs reproducibility. Level 1 (one build, any CPU, any thread count) is required for tests, soak, defect reproduction, calibration and hand-over; the UCRT's transcendental functions dispatch by CPU, so even Level 1 needs own maths functions, which rules out A and D. Level 2 holds by construction between the integrated and the dedicated server (one compiled library) and is tested for world generation; fixed-width types and total orders added (A2-F13). Level 3 is not provided. C is held in reserve with a named trigger |
| Replays (PIL-140) | Command stream from the cut before the battle (0.4 MB); recorded state stream at 5 Hz with side masks and a decision track (3.6 MB) | State stream, served filtered by side (ADR-086 item 6) | Valid across client builds, needs no Level 2; the masks keep fog of war in replays (A2-F10); about 10 × the storage (167 MB at 16, 1.65 GB at 200, 7-day retention) |
| Random draws (DEC-242) | A one sequential stream; B counter-based; C stream per system | B with a secret 128-bit key and SipHash-2-4; ADR-082 | Parallel region steps need draws with no global order; nothing to persist but the key; the key is never shown, so no player predicts a draw (A2-F09); world generation keeps `Mix64` over the public seed |
| Process architecture (DEC-221, DEC-223 to DEC-226) | A roots, owners, region jobs, deadline loop; B registry and job graph; C reference | A with consistent cuts and `IntegratedServer.h`; ADR-083 | Explicit, testable ownership and order; one consistent world after any crash (A2-F04); the client cannot reach server state (A2-F17) |
| Simulation/presentation boundary (DEC-222) | A transport only, replica, interpolation; B shared read-only views; C reference | A with one predicted unit and a stated trust model; ADR-084 | Separability (seed challenge 1) and fog of war against remote clients by construction; the host's full view is stated (A2-F10) |
| Interpolation or extrapolation | Interpolate two latest states (1 tick + jitter delay); extrapolate from the latest (reference) | Interpolate, and predict only the directly controlled unit | No velocities on the wire, no snaps under network jitter; one tick of display delay, which prediction removes for direct control |
| Region partitioning and crossing battles (WLD-282) | A owners + messages at tick boundaries + anchored battles with a lent battlefield; B one world owner; C overlapping margins | A; ADR-085 | Same semantics in one process and many; A1's island valleys make seams bridges only |
| Stage 3 deployment | One process per world on 8 vCPU (round 0); one process on 16 vCPU; region hosts from the start | One process per world on 16 vCPU, region hosts as a reserve beyond ×2.2 | The 8-vCPU host reaches its wall budget at ×1.02; 16 vCPU holds ×2.22 (A2-F01) |
| Replication (DEC-200, DEC-201) | A state replication; B command-stream model for skirmish; C client re-simulation | A with three tiers, self-decodable datagrams, a 48 KB/s cap (64 in skirmish); ADR-086 | One netcode (O8, O10), fog of war, snapshot joins; a lost datagram costs only its figures for one tick |
| Commands (DEC-202, DEC-203) | A arrival checks; B arrival + execution; C partial trust | B, next-tick execution in a keyed sender order, immediate control states, coalescing, rate limits; ADR-087 | Stale-target results defined; one path for client, deputy, AI, API, admin; latency does not decide a same-tick race (A2-F12) |
| World AI command timing | Same-tick in-step (reference bots); next tick through validation | Next tick for the world AI and deputy; skirmish bots keep the reference | One path for every world source; skirmish stays the reference test bed |
| Transport (DEC-207) | A platform transport; B own datagram transport with own handshake (round 0); B inside the operating system's DTLS 1.2 | B inside Schannel's DTLS 1.2, with a player identity key; ADR-088 | No own cryptographic protocol (A2-F15), no platform service or third-party library (O5, R14) |
| Persistence approach | single save file; transactional record store; event-sourced | Record store written in consistent cuts, migration by data on a copy, a pre-migration copy, a hash chain (§4.3.1) | O1 rules out the file; event sourcing needs Level 3 |
| Persistence technology | own journaled store; SQLite (an R14 exception); in-box ESE; server DB for stage 3 | ESE if the dedicated server stays Windows in stage 2; escalated (question 5) | ACID, crash restore and backup in-box (MB-4); the records keep the portable exact encoding inside it |
| Dedicated-server operating systems | Windows only in stage 2; Windows and Linux in stage 2; Linux deferred to stage 3 | Windows in stage 2 with the core portable; escalated (question 4) | A Linux build needs an `AGENTS.md` change and R14 exceptions (A2-F05); the owner decides |

## 2 Recommendations (beyond the vision)

| Recommendation | Benefit | Cost |
|---|---|---|
| Raise the world battle cap at 16 from A1's 4 to 8 | No attack waits in the planning case at either pace (from 4 % and 11 % of the evening peak) | 4.6 ms per tick at most on the integrated host |
| One process per world in stage 3 on 16 vCPU, region hosts as a reserve | Removes the riskiest stage-3 work (the inter-process protocol) from the launch path of the 200 tier, with a ×2.22 margin | A 16-vCPU host instead of 8 |
| Battle replays as recorded state streams with side masks | Replays survive patches during an absence, need no cross-build determinism and keep fog of war | 3.6 MB per battle; 1.65 GB rolling at 200 |
| Predict only the directly controlled unit (MB-1) | Direct control answers in 53 ms at p95 on every client without a faster tick | 2 to 3 engineer-weeks; pure movement functions in the simulation library |
| Consistent cuts every 60 s, every 10 s in integrated worlds during battles | One consistent world after any crash; a deliberate kill winds a battle back at most 10 s and is recorded | 141 KB/s of writes at 16 during battles |
| Command journal (last 10 min + the cut before it) on every server | Every crash or operator report reproduces on the same build | About 1 KB/s of journal writes; a ring buffer |
| A stall of 10 s or more recorded as a freeze | O4-consistent and visible to stewards; no silent fast-forward | None |
| Amend `AGENTS.md` R16's FMA sentence (MSVC stopped contracting under `/fp:precise` in VS 2022; the challenger confirmed it) | Correct engineering rules | An owner-approved edit at gate C |
| Propose the deterministic-core rules as R18 candidates at gate C: no CRT transcendental in simulation code; keyed draws only; no floating-point pragma or `/fp:contract`; no operating-system header outside the platform layer; fixed-width types; stable or total orders; no `std::hash` or pointer value in an outcome; owners hold all mutable simulation state | Conformance rules with a design source (O24) | Checker rules in `Build\CheckProjectFiles.py` |

## 3 Assumptions

1. The per-figure CPU costs of ADR-080 item 2 are achievable (2.1 µs per battle figure-tick is DBG-205 read as a whole tick's cost; read per frame it is 6 times more, shown as ×6). No measurement exists (DEC-106); the sensitivity rows show ×1.25, ×2, ×4 and ×6.
2. A vCPU of a rented host is 0.6 reference cores until measured; 0.4 and 1.0 are shown.
3. The reference test machine (DEC-971) has at least 4 cores, so the integrated server's tick thread and 2 workers leave the client its frame (RND-454).
4. B1's plateau population is A1's placeholder 600; B4's expedition is 100 figures outside battle; B6's away-report entries 200 per steward per day; 60 % of battles have an away steward on a side; dormant records 90 per year at 16 and 730 at 200; the seen layer costs 0.5 µs per body per update (D9).
5. Encoding assumptions of the bandwidth model: 3 % of figures change state per tick at 4 bytes; 5 % per second enter or leave the interest set at 16 bytes; one shot per 3 s per figure in combat at 8 bytes; 77 bytes per datagram (IP and UDP 28, the DTLS 1.2 AES-GCM record 37, Species' header 12).
6. The typical views: a home view of 400 near and 800 far figures; a battle view of 600 near (the battle of 400 and 200 own) and 600 far; a valley war of 1,500 near and 2,000 far; skirmish at 2,000 and at 4,000 units.
7. The server's step takes 20 ms of wall time at 16 stewards and the 16-vCPU host's 22.7 ms at 200; network jitter is exponential with a 5 ms mean each way; the jitter allowance is 30 ms.
8. A1's watched counts are the union over present stewards; when all 16 stewards of an integrated world are present, every settlement is watched (my stress case).
9. D2 will write WSIM requirements that the drop rows of ledger 12 can name, turning them into `adapt`.

## 4 Owner questions for gate B (one merged list, recommended option first)

1. **Scale budgets** (ADR-080 items 3 and 4). (a) Recommended: confirm as provisional values with ranges that the first task of the simulation-core milestone fixes: population limit L = 600 per settlement (range 450 to 700), battle side cap S = 600 (range 200 to 600; a battle at most 1,200), region body budget 4L + 2S = 3,600, region step 12.5 ms; world sizes as A1 set them (8 regions at 16, 77 at 200). (b) Wait to confirm them until a one-week measurement spike has run (MB-3). Trade-off: (a) keeps the run moving and fixes the values by measurement before content is tuned; the region bound is 8.05 ms against 12.5 (holds at +25 %), and at ×2 the limit falls to 450; (b) costs about a week and gives gate B measured costs. The challenger asked for (b) or ranges; I recommend (a).
2. **Battle caps** (WLD-285). Recommended: 2 per region and max(8, ⌈10 % of settlements⌉) per world: 8 at 16, 26 at 200. Alternative: A1's max(4, 10 %). Trade-off: 8 at 16 costs up to 4.6 ms per tick more and removes waiting in the planning case (4 % to 0 % of the evening peak; 11 % to 0 % at the session pace); at 200 the cap of 26 is reached in 1 % of the stress case's peak. No disagreement.
3. **Host specifications** (ADR-080 item 8). Recommended: stage 3, 16 vCPU and 4 GB (×2.22 the cost budgets at the 50 ms wall budget, ×4.46 at the deadline); stage 2, 4 vCPU (×2.51), with 2 vCPU as the minimum (×1.75); the integrated server, 1 reference core and 2 workers (0.32 cores at the stress case). Alternative: 8 vCPU for stage 3 with region hosts designed before stage 3 starts. Trade-off: 16 vCPU doubles the rented host but needs no inter-process protocol until beyond 200 stewards; 8 vCPU reaches its wall budget at ×1.02, so it depends on region hosts or the overload lever from the first measured overrun. The challenger offered both; I recommend 16 vCPU.
4. **Dedicated-server operating systems** (A2-F05; O21; vision §6 item 10). (A) Recommended: a Windows dedicated server in stage 2; the server and simulation libraries keep every operating-system call in the platform layer, fixed-width types and total orders, and no Windows codec in persisted data, so a port is a platform-layer port. (B) A Linux dedicated server in stage 2, as an `AGENTS.md` change: Clang 18 or later with libc++ (or GCC 14 with libstdc++), built by Visual Studio's MSBuild Linux projects so that the `.slnx` and MSBuild stay; the R16 settings as `-std=c++23 -O2 -mavx2 -ffp-contract=off -fno-fast-math -Werror`; R14 exceptions for OpenSSL 3 (DTLS and HTTPS, from the distribution) and for the store engine (ESE is Windows-only, so the own store or SQLite); an own crash handler; a Linux CI job and the Level 2 corpus on both builds; my estimate 7 to 11 engineer-weeks, then every change tested on two builds. (C) Defer Linux to stage 3, where the hosting operator chooses. Trade-off: most community hosts rent Linux, and Windows VPS plans usually cost more because of the Windows licence, so A narrows stage 2's community; B buys that reach for the exceptions and the weeks above. Where we differ: the vision recommended Windows and Linux; the challenger listed the costs without a recommendation; I recommend A, with B as a later stage-2 milestone if the owner approves its exceptions.
5. **Persistence technology** (MB-4, A2-F14; DEC-250, DEC-251 are D4's). (a) Recommended if question 4 is A: the in-box Extensible Storage Engine (ESE) for stages 1 and 2: ACID transactions, crash restore and online backup, R14-clean, records in the portable exact encoding inside it, worlds moved by an export file. (b) Species' own journaled record store in plain C++: portable, but Species writes the write-ahead log, torn-write detection and restore (3 to 4 engineer-weeks with tests). (c) SQLite: portable and proven, an R14 exception. Stage 3 may add a server database behind the same store interface by a later record. The challenger proposed (a); I agree, conditional on question 4.
6. **Identity, listing and relay; storefront and publisher** (A2-F16; ADR-072, ADR-073; DEC-248, DEC-249 are D5's). (a) Recommended for the first world-game milestone: the R14-clean baseline (an identity key per player, UPnP through the in-box NAT API, IPv6, a manual address; no relay, so hosts behind carrier-grade NAT are unreachable; no cross-server identity, so PIL-088's second-account detection is weak). Then, with the storefront, one provider: (b) the Steamworks SDK (identity and entitlement tickets, server browser and lobbies, a relay; an R14 exception; ties identity to one storefront); (c) the Epic Online Services SDK (cross-storefront identity, sessions and lobbies, relays; an R14 exception); (d) PlayFab Party with Lobby (a Microsoft NuGet package not on the R14 list; a metered service); (e) an own relay and listing service (no exception; the relay's egress is 2.8 Mbit/s for one relayed world of 15 remote stewards in battle views, 5.0 GB per 4-hour session). The owner names the storefront and, if known, the publishing entity. No disagreement on (a).
7. **Overload lever** (MB-2; ADR-085 item 9; amends PIL-057 and WLD-244). Recommended: approve, as an overload-only lever: under a sustained overload, settlements watched only as neighbours fall to abstract fidelity with a stand-in presentation; a steward's own settlement and every battle never do. Alternative: no lever; an overload slows world time. Trade-off: up to 101.0 ms of the 200 stress case's 185.5 ms per tick (54 %) is freed without touching battles, for a stand-in presentation (C3, the renderer) and the abstract model's accuracy holding for watched neighbours (B2).

For gate C: amend `AGENTS.md` R16's FMA sentence; the R18 candidates of §2.

## 5 Adoption (ledger 12, A2's scope: 01 tick timing and 07 randomness and integrity)

28 section rows and 12 exception rows (10 of the 40 rows are drops), 144 requirements: adopt 92, adopt-skirmish 26, drop 26, adapt 0 (A2's chapter has no requirement prefix, so a replacement named in an ADR is recorded as `drop` with the ADR as its replacement; D2 turns those rows into `adapt` when WSIM requirements exist).

| Scope | Verdict | Note |
|---|---|---|
| 01 §2.1 tick rate | adopt | 10 Hz kept (ADR-079) |
| 01 §2.2 tick and slices | adopt | whole ticks, slices back to back |
| 01 §2.3 tick schedule | drop | sequencer replaced by the server tick loop (ADR-083 item 7) |
| 01 §2.4 slice pacing and catch-up | drop | no client steps the simulation |
| 01 §2.5 tick clock and game speed | adopt-skirmish; R-SIM-547 adopt | the world has no game speed (PIL-085) |
| 01 §2.6 held world, pause, end of match | adopt-skirmish | |
| 01 §2.7 render-time extrapolation | drop | interpolation (ADR-084) |
| 01 §2.8 clocks | adopt | except the extrapolation-time row |
| 01 §2.9 render rate | adopt | |
| 01 §2.10 in-match frame | adopt; R-SIM-581, R-SIM-583 drop | client frame of ADR-084 |
| 07 §2.1 scope | adopt | Levels 1 and 2 of ADR-081 |
| 07 §2.2 random streams | adopt; R-SIM-605, R-SIM-606 drop; R-SIM-610, R-SIM-612 adopt-skirmish | keyed draws (ADR-082) |
| 07 §2.3 draw discipline | adopt; R-SIM-627 adopt-skirmish | |
| 07 §2.4 to §2.9 | adopt | |
| 07 §2.10 layering | adopt; R-SIM-672 drop | interpolation |
| 07 §2.11 | adopt | |
| 07 §2.12 bots and integrity | adopt-skirmish | world AI: B7 restates |
| 07 §2.13 to §2.15 | adopt | |
| 07 §2.16 settings, seasonal state, tutorials | adopt; R-SIM-726 adopt-skirmish | world: WLD-185 |
| 07 §2.17 | adopt | |
| 07 §2.18 replay hooks | adopt; R-SIM-740, R-SIM-741 drop | command journal (ADR-088 item 7) |

The check reports 3 adopted requirements that cite a dropped id (R-SIM-612 → R-SIM-533; R-SIM-700 → R-SIM-531; R-SIM-711 → R-SIM-564); each row's note says how they read, and D2's `adapt` rows will name the replacements. The other sections of ledger 12 (02 to 06) are D2's; cross-area request below.

## 6 Decisions (the 20 of the triage), proposed resolutions

| DEC | Proposed resolution | Implemented by |
|---|---|---|
| DEC-200 | Resolved: option A, one server-authoritative state replication model for the world and skirmish, with three interest tiers and crowd aggregation as the far tier | ADR-086 items 1 to 3; ADR-084 |
| DEC-201 | Resolved: option A (budget first) with C as the far tier: 32 KB/s target and 48 KB/s cap downstream (64 KB/s in skirmish; the in-process connection uncapped), 4 KB/s upstream, a 4 Mbit/s integrated-host upload budget, a 256 KB/s join blob budget; tier 1 at most 1,500 figures; self-decodable datagrams; degradation by tier, never queues | ADR-086 items 3 and 4; chapter Tables 11 and 12 |
| DEC-202 | Resolved: option B, validation on arrival and on execution for every source, one path | ADR-087 items 1 to 3, 6, 8 |
| DEC-203 | Resolved: option B, execution at the first tick after arrival in a keyed sender order, no extra input delay (a skirmish host-delay option, B9's default), control states sent at once on a change, interpolation one tick plus a 30 ms jitter allowance, coalescing of continuous state only, local feedback and the prediction of the directly controlled unit only | ADR-087 items 4, 5, 7; ADR-084 items 4 and 6 |
| DEC-204 | Resolved: option A, build identity with protocol and simulation versions and the content manifest hash compared at the handshake, refusal with a reason; designed for B later | ADR-088 item 6 |
| DEC-205 | Resolved: option C, snapshot join for every case, the identity key or a session token reclaims from the deputy or bot, terrain by level of detail with a digest cache, the integrated host's exit freezes the world and disconnects cleanly, no host migration; rejoin within 10 s | ADR-086 item 5; ADR-088 item 4 |
| DEC-206 | Resolved: option A, scoped: a canonical 64-bit hash per owner, every tick in test builds, at every consistent cut, hand-over and every 10 ticks in Release; field-level diff; refused hand-over on mismatch | ADR-081 item 8 |
| DEC-207 | Resolved: option B, an own datagram transport with reliable and unreliable channels, fragmentation and streamed blobs, inside Schannel's DTLS 1.2 with a stateless cookie, a pinned self-signed server certificate, a player identity key, schema-defined messages and fuzzed parsers | ADR-088 items 1 to 5 |
| DEC-208 | Resolved: option B, the developer metric set also published to world hosts through the administration interface; link simulator in Debug; a command journal with the cut before it on every server | ADR-088 item 7 |
| DEC-221 | Resolved: option A, composition roots per role, lifetime scopes (process, server run, world, region, battle, match, client session, connection), explicit state machines; the integrated server a child root composed through `IntegratedServer.h` only | ADR-083 items 2, 3 |
| DEC-222 | Resolved: option A, the transport as the only path, a client replica, typed presentation events, interpolation, one predicted unit, a stated trust model; include allow-lists both ways | ADR-084 |
| DEC-223 | Resolved: option A with B inside it: owners per region (`RegionSimulation`), world (`WorldSimulation`) and match (`MatchWorld`); stable 64-bit ids with the id counter in every cut; owners as the unit of hashing, of every consistent cut and of hand-over | ADR-083 items 4, 5, 9; ADR-085 item 1 |
| DEC-224 | Resolved: option A, a tick thread and a worker pool, region steps as parallel jobs, serial inside, in-region splitting of read-only phases as the first remedy when a step exceeds half its budget; network, store and logging on their own threads | ADR-083 item 6 |
| DEC-225 | Resolved: option A, a deadline-paced server loop, catch-up at most 2 per period up to 20 owed ticks, re-anchoring beyond, a 10 s stall recorded as a freeze; skirmish game speed as tick scheduling; clients render at their own rate | ADR-083 item 7 |
| DEC-226 | Resolved: option A for the world (streaming with progress and completion signals; a world runs when every region is loaded; generation never inside a tick), B allowed for skirmish | ADR-083 item 8 |
| DEC-227 | Resolved: option A (10 Hz) with D's lower abstract cadence (every 10 ticks) and a 60 s fidelity hold; the conversion rule for any later change is fixed, and the 20 Hz budgets stand beside the gate-B ones | ADR-079 |
| DEC-228 | Resolved: options A and B together with hard caps and per-figure cost budgets; region step 12.5 ms, region body budget 4L + 2S = 3,600, side cap 600, battle caps 2 per region and max(8, 10 %) per world, a 50 ms wall budget and a 100 ms deadline, integrated budget 1 core and 2 workers, hosts by the margin rule (16 vCPU at stage 3, 4 vCPU at stage 2), remedies with battles last | ADR-080 |
| DEC-240 | Resolved: option B (strict binary64, own deterministic maths functions, asserted environment, fixed-width types and total orders) with a three-level determinism scope; C in reserve with a named trigger | ADR-081 items 1 to 4, 6, 7, 9 |
| DEC-241 | Resolved: option B, observable semantics kept as explicit rules with vectors, incidental binary32 rounding dropped (the terrain generator's TER-065 kept) | ADR-081 item 5 |
| DEC-242 | Resolved: option B, counter-based values: SipHash-2-4 of a secret 128-bit simulation key over the domain, purpose, subject, tick and ordinal; world generation by `Mix64` over the public seed (WLD-161); the command order by the same keyed function; presentation generators PCG32 per consumer | ADR-082 |

None is re-dated or dropped. Each is "Resolved" pending gate B, since the ADRs are Proposed until then; the confirmation milestones (simulation core, first playable, first world-game milestone) are in each ADR for E3's roadmap.

## 7 Cross-area requests

| To | Request |
|---|---|
| Coordinator | Register the A2 names at Finalise (§9; CONVENTIONS §8); note that the chapter has no requirement prefix, so A2's ledger replacements are `drop` rows naming ADRs; refresh the ledger README totals; resolve DEC-200 to DEC-208, DEC-221 to DEC-228 and DEC-240 to DEC-242 in the register at Finalise; carry owner questions 1 to 7 to gate B and the R16 wording and the R18 candidates to gate C |
| D1 | DEC-220: build targets of one codebase (client with the server library; headless dedicated server; test runner), the server library linking no presentation code; `IntegratedServer.h` as the client's only server header and the administration interface for host tools (ADR-083 item 2); the integrated server as a child root with presentation-suspended mode (PIL-077); host specifications of ADR-080 item 8 in the operator documentation; the operating-system ADR after gate B question 4 |
| D2 | Write WSIM requirements for ADR-079 items 2 and 3, ADR-082, ADR-083 items 4 to 9 and ADR-085 (owners, messages, lent battlefield, border band, fidelity switch at an abstract-step boundary with the 60 s hold, the visitor share, the overload lever if approved) and turn A2's 10 drop rows into `adapt`; fill ledger 12 sections of 02 to 06; decide DEC-046 with the state-stream replay of ADR-086 item 6; DEC-264 (skirmish game speed as tick scheduling, ADR-083 item 7), DEC-455, DEC-266 on the whole-tick execution |
| D3 | The replication component on ADR-084, ADR-086 and ADR-088: tier rules, self-decodable encoding, caps, joins, the replay recorder and its side filter, the predicted unit's protocol; confirm the transport type names; extend `A2ServerBudget.py bandwidth` with measured encodings |
| D4 | The record store of §4.3.1: consistent cuts and their manifest, the write protocol, the pre-migration copy and migration on a copy, 7 backups, whole-world rollback, the hash chain, unclean stops in the world record, dormant records, archives of WLD-223; DEC-250, DEC-251 with gate B question 5; extend `A2ServerBudget.py storage` |
| D5 | DEC-248, DEC-249 with gate B question 6; the identity key and its registry, the certificate pinning of ADR-088 item 4; rendezvous and relay as a `Transport` implementation |
| D6 | The O6 server API as commands through ADR-087's path with its rate limit |
| D7 | AI runtime within 20 µs per settlement per tick and 30 µs per battle side per tick (ADR-080 item 2; the model shows 100 and 200); AI reads the server-side seen layer (ADR-086 item 1) |
| D8 | DEC-246/247: whether normals are held (252 MB at 200) or derived; chunks by digest; the client's 16-bit heights by level of detail (ADR-086 item 5); path-finding between regions over the bridges of ADR-085 |
| D9 | The seen layer and its cost (0.5 µs per body per update, a placeholder); the trust model's wording (ADR-084 item 8), the deliberate-kill case (ADR-083 item 9), the body-budget and battle-cap scenarios (ADR-080 items 3 and 4), what the hash chain promises (§4.3.1 item 8); the review of ADR-088's identity flow; accountability records of ADR-087 item 8 |
| D10 | DEC-971: the reference test machine (ADR-080 item 1 assumes at least 4 cores) and the vCPU factor; telemetry of ADR-088 item 7 with consent |
| B1 | Population limit at most 600 per settlement at the plateau (gate B range 450 to 700; ADR-080 item 3) |
| B2 | The abstract step every 10 ticks (or a multiple up to 100) within 5 µs per tick (ADR-079 item 3, ADR-080 item 2); the switch at a step boundary with the 60 s hold; the abstract model's accuracy for watched neighbours if the overload lever is approved |
| B4 | At most 600 figures per side inside a battlefield and how they are counted; reinforcements wait at the edge; battlefield at a crossing between the mouths (ADR-085 item 4); expedition size for the model |
| B6 | The defender's advantage as balance at equal side caps; away-report entries per day and the share of battles with an away steward (model placeholders) |
| B7 | World AI and deputy issue validated commands executing at the next tick (ADR-087 item 1); restate the integrity rules of reference/12 07 §2.12 for the world AI |
| B9 | The default of the skirmish host-delay option (ADR-087 item 4) |
| C1, C2, C3 | Immediate local feedback and the predicted unit's 100 ms blend (ADR-084 item 6); reason codes of rejected commands; the "waiting for the server" indicator (ADR-084 item 4); the join page's statement that the world host sees the whole world (ADR-084 item 8); a stand-in presentation if the overload lever is approved (ADR-085 item 9) |
| Renderer owner (E1 or D8) | Amend RND-002, RND-018, RND-019: interpolation fraction and two tick numbers in place of the extrapolation time, and the predicted unit (ADR-084 items 4 and 6) |
| E3 | Roadmap tasks for each ADR's "Confirmed when", with the cost measurement as the first task of the simulation-core milestone (ADR-080); first playable: the latency playtest with the split thresholds, the networking stage; first world-game milestone: integrated stress case, region hashes, crossing battle, forced-kill test |

## 8 Model results (for the rationale's "Model results")

`python3 -I Tools/DesignModels/A2ServerBudget.py check`: 33 figures and 12 tables, 0 differences. Headlines at the cost budgets:

| Figure | 16 stewards | 200 stewards |
|---|---|---|
| CPU per settlement, full / abstract | 0.80 / 0.025 ms per tick | same |
| CPU per battle of 400 / 1,200 | 1.15 / 3.32 ms per tick | same |
| Region bound / step budget | 8.05 / 12.5 ms (×1.55 headroom) | same |
| World CPU, planning | 14.7 ms per tick (0.15 cores) | 125.2 ms (1.25 cores) |
| World CPU, stress | 32.0 ms (all present, 0.32 cores) | 185.5 ms (1.85 cores; 3.71 at 20 Hz) |
| Tick wall time on the recommended host | 12.0 ms integrated (×4.20); 20.1 ms on 4 vCPU (×2.51) | 22.7 ms on 16 vCPU (×2.22; ×4.46 at the deadline); 49.2 ms on 8 vCPU (×1.02) |
| Population limit at ×2 / ×6 | 450 / 150 | same |
| Bandwidth per client: home / battle / side cap / war / strategic | 17.3 / 23.2 / 42.5 / 63.2 (capped at 48) / 2.1 KB/s | 17.3 / 23.2 / 42.5 / 63.2 / 3.1 KB/s |
| Skirmish, design / stress population | 50.5 / 69.8 KB/s (cap 64) | — |
| Server upload | 0.6 Mbit/s at the peak; 2.8 with 15 remote in battle; 3.0 on a community server | 8.7 Mbit/s at the peak; 584 GB per 30 days |
| First join's terrain | 6 s | 62 s |
| Storage after 84 world days | 207 MB (replays 167) | 2,052 MB (replays 1,645) |
| Consistent cut | 1.4 MB; 23 KB/s, 141 KB/s during battles | 12.7 MB; 216 KB/s |
| Growth per world week | 0.2 MB | 2.0 MB |
| Memory at stress | 60 MB | 382 MB |
| Latency, orders, mean / p95 | 212 / 258 ms host; 302 / 350 ms at 50 ms RTT | 304 / 352 ms at 50 ms RTT |
| Latency, predicted direct control | 42 / 53 ms | same |
| Keyed draws | — | 1.35 ms per tick (0.7 %) |
| Overload lever | — | up to 101.0 ms of 185.5 (54 %) |
| World battle cap | 8 | 26 |

## 9 Names proposed

`RegionSimulation` (region state owner), `WorldSimulation` (world state owner), `Transport`, `InProcessTransport`, `UdpTransport`, `IntegratedServer.h` (the integrated server's one header); terms: reference core (one core of the DEC-971 machine), region body budget, visitor share, side cap, tick wall budget, margin rule, consistent cut, lent battlefield, border band, interest tiers (tier 1, 2, 3), command journal, anchor region, simulation key, identity key, overload lever. None is a player-facing word.
