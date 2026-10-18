# A2 proposal notes (A2-P, round 0)

> Area A2, architecture strategy · Documents: `Design/architecture/04_solution_strategy.md` (Draft), `Design/ADR/ADR-079` to `ADR-088` (Proposed), ledger rows of `Design/gdd/adoption_ledger/12_simulation_model_ledger.md` (01 and 07) · Model: `Tools/DesignModels/A2ServerBudget.py` · Date: 2026-10-18

## 1 Options considered

| Choice | Options | Chosen | Why |
|---|---|---|---|
| Tick rate (DEC-227) | A 10 Hz; B 20 Hz; C 30 Hz; D a higher full rate with a lower abstract rate | A, plus D's abstract cadence (every 10 ticks); ADR-079 | Every reference tick value binds as written (O13, O17), so skirmish, the balance test bed, tests the reference rather than a conversion of hundreds of per-tick values, probabilities, factors and integration steps; half the CPU and bandwidth of B (1.67 vs 3.35 cores at 200 stress; 22.8 vs 36.2 KB/s battle view). Cost: latency 187 ms host / 267 ms at 50 ms RTT vs 102 / 182 at B. The integrated host's latency equals the reference's steady state (wait 0–100 ms + 100 ms pacing). A conversion rule is fixed now so B stays one superseding ADR away; confirmation at the first playable by a latency measurement and a responsiveness playtest. The register recommended D at 20 Hz; I disagree on the merits above |
| How the server executes a tick | Whole ticks with the 10 slices back to back; whole ticks without slices (re-express slice order in tick order); spread slices over frames | Back to back | Keeps every slice-dependent order of the reference (R-SIM-513, R-SIM-516) with no re-specification; slices cost nothing when run consecutively |
| Budgets (DEC-228) | A client-derived; B server-first; A+B with hard caps; C dynamic | A+B with caps and per-figure cost budgets; ADR-080 | Both the integrated host and community hosts need a budget that can fail; C makes outcomes depend on hardware |
| Region body budget (WLD-281) | 3,200 (A1's minimum for 4 × 600 at 75 %); 4,000 (the DBG-204 stress population); 6,000 | 4,000 | A region is the size of a reference map, so the reference stress population is the natural per-region test figure; 4,000 bodies all in battle cost 8.43 ms, inside the 10 ms step; allows B1 up to 750 per settlement |
| Battle size cap | none (bodies budget only); 800; 1,000; 2,000 (reference design population) | 1,000 | 2.5 × typical; a battle of 1,000 costs 2.18 ms; 2,000 would let one battle take half a region's body budget and leave no room for two battles plus four settlements |
| World battle cap | max(4, 10 %) (A1); max(6, 10 %); max(8, 10 %) | max(8, 10 %) at 16 → 8; 26 at 200 | Removes waiting in the planning case at both paces (11 % → 0 % at the session pace) for 3.6 ms per tick; region cap 2 kept |
| Numeric model (DEC-240) | A reference; B strict binary64; C fixed point; D one build only | B with a three-level scope; ADR-081 | Not inherited from the reference: decided by what needs reproducibility. Level 1 (one build, any CPU, any thread count) is required for tests, soak, defect reproduction, calibration, and hand-over; the UCRT's transcendental functions dispatch by CPU and change with Windows versions (Microsoft Learn, checked 2026-10-18), so even Level 1 needs own maths functions, which rules out A and D. Level 2 (all executables of a release) is needed for world generation (WLD-161) and keeps a second-OS server possible; under `/fp:precise` MSVC has generated no contraction since VS 2022, so B fits AGENTS.md as written. Level 3 (across releases) is not provided: migration by data, replays as state streams. C costs a rewrite of PLT/TER for a guarantee only generation and diagnostics need; held in reserve with a named trigger |
| Replays (PIL-140) | Command stream from a region checkpoint (0.4 MB per battle); recorded state stream at 5 Hz (2.6 MB) | State stream (ADR-086 item 6) | Valid across client builds, needs no Level 2, robust across a patch during an absence; costs 6 × the storage (121 MB at 16, 1.19 GB at 200, 7-day retention) |
| Random draws (DEC-242) | A one sequential stream; B counter-based; C stream per system | B; ADR-082, SplitMix64-chained keyed values | Parallel region steps and in-region jobs need draws with no global order; nothing to persist or hand over; one function for simulation and A1's generation |
| Process architecture (DEC-221, DEC-223 to DEC-226) | A roots, owners, region jobs, deadline loop; B registry and job graph; C reference | A; ADR-083 | Explicit, testable ownership and order; regions scale over cores now and processes later |
| Simulation/presentation boundary (DEC-222) | A transport only, replica, interpolation; B shared read-only views; C reference | A; ADR-084 | Separability (seed challenge 1) and fog of war by construction; the host pays ~0.05 µs per figure per tick to encode its own view |
| Interpolation or extrapolation | Interpolate two latest states (1 tick + jitter delay); extrapolate from the latest (reference) | Interpolate | No velocities on the wire (saves bandwidth), no snaps under network jitter; costs one tick of display delay |
| Region partitioning and crossing battles (WLD-282) | A owners + messages at tick boundaries + anchored battles with a lent battlefield; B one world owner; C overlapping margins | A; ADR-085 | Same semantics in one process and many; A1's island valleys make seams bridges only. Crossing battle anchor = region of the border area where it starts (each border area belongs to one region, WLD-062) |
| Stage 3 deployment | One process per world; region hosts from the start | One process per world first, region hosts as a reserve | The model: 200 stewards at stress = 1.67 reference cores, 300 MB; split only when measured costs exceed about 2 × budget, or for > 200 |
| Replication (DEC-200, DEC-201) | A state replication; B command-stream model for skirmish; C client re-simulation | A with three tiers and a 48 KB/s cap; ADR-086 | One netcode (O8, O10), fog of war, snapshot joins |
| Commands (DEC-202, DEC-203) | A arrival checks; B arrival + execution; C partial trust | B, next-tick execution, coalescing, rate limits; ADR-087 | Stale-target results defined; one path for client, deputy, AI, API, admin |
| World AI command timing | Same-tick in-step (reference bots); next tick through validation | Next tick for the world AI and deputy; skirmish bots keep the reference | One path for every world source (deputy trust, O6 parity); skirmish stays the reference test bed |
| Transport (DEC-207) | A platform transport; B own datagram transport; C stream + datagrams | B with in-box CNG crypto; ADR-088 | No platform service or third-party library (O5, R14); relay for home hosts is D5's |
| Persistence approach | single save file; transactional record store; event-sourced | Record store, checkpoints, migration by data (§4.3.1) | O1 rules out the file; event sourcing needs Level 3, which no float model gives across releases |
| Persistence technology | own journaled record format (plain C++); SQLite (needs R14); Windows ESE (in-box, Windows only); server DB for stage 3 | own format for stages 1–2; DB for stage 3 by a later record | R14 and a second server OS; escalated to gate B (question 6) |

## 2 Recommendations (beyond the vision)

| Recommendation | Benefit | Cost |
|---|---|---|
| Raise the world battle cap at 16 from A1's 4 to 8 | No attack waits in the planning case at either pace (from 4 % and 11 % of the evening peak) | 3.6 ms per tick at most on the integrated host |
| One process per world in stage 3, region hosts as a reserve | Removes the riskiest stage-3 work (the inter-process protocol) from the launch path of the 200 tier | None at the budgets; at > 2 × measured cost a 200-world needs the split or a bigger host |
| Battle replays as recorded state streams | Replays survive patches during an absence and need no cross-build determinism | 2.6 MB per battle; 1.19 GB rolling at 200 |
| Command journal (last 10 min + checkpoint) on every server | Every crash or operator report reproduces on the same build | ~1 KB/s of journal writes; a ring buffer |
| A stall of 10 s or more recorded as a freeze | O4-consistent and visible to stewards; no silent fast-forward | none |
| Amend AGENTS.md R16's FMA sentence (MSVC stopped contracting under `/fp:precise` in VS 2022) | Correct engineering rules | An owner-approved edit at gate C |
| Propose the deterministic-core rules as R18 candidates at gate C (no CRT transcendental in simulation code; keyed draws only; no floating-point pragma or `/fp:contract`; no OS header in server/simulation libraries; owners hold all mutable simulation state) | Conformance rules with a design source (O24) | Checker rules in `Build\CheckProjectFiles.py` |

## 3 Assumptions

1. The per-figure CPU costs of ADR-080 item 2 are achievable (2.1 µs per battle figure-tick is the DEC-228 guidance; 1.2 µs for settlement work is my estimate). No measurement exists (DEC-106); the sensitivity rows show 2× and 4×.
2. A vCPU of a rented host is 0.6 reference cores until measured.
3. The reference test machine (DEC-971) has at least 4 cores, so the integrated server's 1 core and 2 workers leave the client its frame (RND-454).
4. B1's plateau population is A1's placeholder 600; B4's expedition is 100 figures outside battle; B6's away-report entries 200 per steward per day; 60 % of battles have an away steward on a side; dormant records 90 per year at 16 and 730 at 200 (from A1's lifetime scenarios).
5. Encoding assumptions of the bandwidth model: 3 % of figures change state per tick at 4 bytes; 5 % per second enter or leave the interest set at 16 bytes; one shot per 3 s per figure in combat at 8 bytes; 60 bytes of datagram overhead.
6. The typical views: a home view of 400 near and 800 far figures; a battle view of 600 near (the battle of 400 and 200 own) and 600 far; a valley war of 1,500 near and 2,000 far.
7. The server's step takes 20 ms of wall time to the snapshot at 10 Hz; the jitter allowance is 30 ms.
8. A1's watched counts are the union over present stewards; when all 16 stewards of an integrated world are present, every settlement is watched (my stress case).
9. D2 will write WSIM requirements that the drop rows of ledger 12 can name, turning them into `adapt`.

## 4 Owner questions for gate B (recommended option first)

1. **Scale budgets.** Recommended: population limit at most 750 Workers and Leaders per settlement (B1 sets the value within it), battles of at most 1,000 figures (typical 400, floor 200), a region body budget of 4,000 bodies, world sizes per capacity as A1 set them (8 regions at 16, 77 at 200, extent reserved for 256). Alternative: lower caps (600 per settlement, 800 per battle) for more margin. Trade-off: the recommended caps fit with margin at the cost budgets (0.28 reference cores at 16 worst, 1.67 at 200 stress) but leave less room if the measured costs are 2–4× the budgets; ADR-080's confirmation lowers them by the measured ratio before content is tuned.
2. **Battle caps.** Recommended: 2 per region; per world max(8, ⌈10 % of settlements⌉): 8 at 16, 26 at 200. Alternative: A1's max(4, 10 %). Trade-off: 8 at 16 costs 3.6 ms per tick more and removes waiting in the planning case (4 % → 0 % evening, 11 % → 0 % session); at 200 the cap of 26 is reached in 1 % of the evening peak of the stress case.
3. **Dedicated-server operating systems.** Recommended: Windows and Linux; the Windows server first (stage 2), the Linux build before or during stage 2 by its own ADR, with the server and simulation libraries portable C++23 from the first line (ADR-083 item 1). Alternative: Windows only (O21's default). Trade-off: Linux reaches most community hosts but needs an AGENTS.md change for a second compiler and its build (MSBuild-driven, no CMake), a CI job, and a cryptography provider for the transport that R14 allows (CNG is Windows-only); ADR-081's Level 2 makes its simulation agree with Windows by test, not by hope.
4. **Online services and identity backend.** Recommended: no Species-run service needed to play in stages 1 and 2 beyond a small server listing with rendezvous and relay (DEC-249), with player identity from the storefront's account (DEC-248), which also gives PIL-088's one person, one newcomer protection across community servers. Alternative: Species accounts on a Species identity service from stage 1. Trade-off: storefront identity is cheapest and global but ties identity to a storefront and its SDK, which R14 does not list today, so the owner must allow it; an own service avoids the SDK but must be run, secured and GDPR-compliant from launch. D5 specifies it.
5. **Storefronts (ADR-072) and the publishing entity (ADR-073).** Recommended: one PC storefront first that provides account identity, entitlement and a dedicated-server distribution channel, so community hosts can install the server; the owner names it, and the publishing entity if known (it decides ADR-073's signing eligibility). Trade-off: each storefront's identity SDK is an R14 decision; a storefront without identity services leaves question 4 to an own service.
6. **Persistence technology (escalated).** Recommended: Species' own journaled record store in plain C++ for stages 1 and 2 (§4.3.1 item 8), with a server database behind the same interface for stage 3 by a later record. Alternatives: SQLite embedded (an R14 exception); Windows ESE (in-box, so R14-compatible, but Windows-only and blocks question 3's Linux build). Trade-off: the own store keeps R14 and portability but is Species code to write and test (two checkpoints per owner, 7 backups; the model's writes are 23 KB/s at 16 and 216 KB/s at 200); SQLite saves that work at the cost of R14's closed list.
7. **AGENTS.md R16 wording (for gate C).** Recommended: amend the sentence that says `/arch:AVX2` lets MSVC contract into FMA under `/fp:precise`; Microsoft's documentation says contraction stopped by default in VS 2022. No setting changes.

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
| DEC-201 | Resolved: option A (budget first) with C as the far tier: 32 KB/s target and 48 KB/s cap downstream, 4 KB/s upstream, a 4 Mbit/s integrated-host upload budget; tier-1 at most 1,500 figures; degradation by tier, never queues | ADR-086 item 4; model Table 9 |
| DEC-202 | Resolved: option B, validation on arrival and on execution for every source, one path | ADR-087 items 1 to 3, 6, 8 |
| DEC-203 | Resolved: option B, execution at the first tick after arrival, no extra input delay, interpolation delay one tick plus a 30 ms jitter allowance, coalescing of continuous state only, local feedback without movement prediction | ADR-087 items 4, 5, 7; ADR-084 item 4 |
| DEC-204 | Resolved: option A, build identity with protocol and simulation versions and the content manifest hash compared at the handshake, refusal with a reason; designed for B later | ADR-088 item 6 |
| DEC-205 | Resolved: option C, snapshot join for every case, identity or session token reclaims from the deputy or bot, the integrated host's exit freezes the world and disconnects cleanly, no host migration; rejoin within 10 s | ADR-086 item 5 |
| DEC-206 | Resolved: option A, scoped: a canonical 64-bit hash per owner, every tick in test builds, at checkpoints, hand-overs and every 10 ticks in Release; field-level diff; refused hand-over on mismatch | ADR-081 item 8 |
| DEC-207 | Resolved: option B, an own encrypted datagram transport with reliable and unreliable channels, fragmentation, streamed blobs, in-box CNG cryptography, schema-defined messages, fuzzed parsers | ADR-088 items 1 to 5 |
| DEC-208 | Resolved: option B, the developer metric set also published to world hosts; link simulator in Debug; a command journal on every server | ADR-088 item 7 |
| DEC-221 | Resolved: option A, composition roots per role, lifetime scopes (process, server run, world, region, battle, match, client session, connection), explicit state machines; the integrated server a child root | ADR-083 items 2, 3 |
| DEC-222 | Resolved: option A, the transport as the only path, a client replica, typed presentation events, interpolation; include allow-lists both ways | ADR-084 |
| DEC-223 | Resolved: option A with B inside it: owners per region (`RegionSimulation`), world (`WorldSimulation`) and match (`MatchWorld`); stable 64-bit ids; owners as the unit of persistence, hashing and hand-over | ADR-083 items 4, 5; ADR-085 item 1 |
| DEC-224 | Resolved: option A, a tick thread and a worker pool, region steps as parallel jobs, serial inside, in-region splitting only of read-only phases when measured; network, store and logging on their own threads | ADR-083 item 6 |
| DEC-225 | Resolved: option A, a deadline-paced server loop, catch-up at most 2 per period up to 20 owed ticks, re-anchoring beyond, a 10 s stall recorded as a freeze; clients render at their own rate | ADR-083 item 7 |
| DEC-226 | Resolved: option A for the world (streaming with progress and completion signals; a world runs when every region is loaded; generation never inside a tick), B allowed for skirmish | ADR-083 item 8 |
| DEC-227 | Resolved: option A (10 Hz) with D's lower abstract cadence (every 10 ticks); the conversion rule for any later change is fixed | ADR-079 |
| DEC-228 | Resolved: options A and B together with hard caps and per-figure cost budgets; region step 10 ms, region body budget 4,000, battle size 1,000, battle caps 2 per region and max(8, 10 %) per world, integrated budget 1 core and 2 workers, host specifications | ADR-080 |
| DEC-240 | Resolved: option B (strict binary64, own deterministic maths functions, asserted environment) with a three-level determinism scope; C in reserve with a named trigger | ADR-081 items 1 to 4, 6, 7, 9 |
| DEC-241 | Resolved: option B, observable semantics kept as explicit rules with vectors, incidental binary32 rounding dropped (the terrain generator's TER-065 kept) | ADR-081 item 5 |
| DEC-242 | Resolved: option B, counter-based keyed values (SplitMix64 output function chained over seed, domain, purpose, subject, tick, ordinal); presentation generators PCG32 per consumer | ADR-082 |

None is re-dated or dropped. Each is "Resolved" pending gate B, since the ADRs are Proposed until then; the confirmation milestones (first playable, simulation core, first world-game milestone) are in each ADR for E3's roadmap.

## 7 Cross-area requests

| To | Request |
|---|---|
| Coordinator | Register the A2 names at Finalise: `RegionSimulation`, `WorldSimulation`, `Transport`, `InProcessTransport`, `UdpTransport` (CONVENTIONS §8); note that the chapter has no requirement prefix, so A2's ledger replacements are `drop` rows naming ADRs; refresh the ledger README totals; resolve DEC-200 to DEC-208, DEC-221 to DEC-228 and DEC-240 to DEC-242 in the register at Finalise; carry owner questions 1 to 7 to gate B (7 to gate C) |
| D1 | DEC-220: build targets of one codebase (client with the server library; headless dedicated server; test runner), the server library linking no presentation code (ADR-083 item 1); the integrated server as a child root with presentation-suspended mode (PIL-077); host specifications of ADR-080 item 8 in the operator documentation; the second-OS ADR after gate B |
| D2 | Write WSIM requirements for ADR-079 items 2 and 3, ADR-082, ADR-083 items 4 to 9 and ADR-085 (owners, messages, lent battlefield, border band, fidelity switch at an abstract-step boundary) and turn A2's 10 drop rows into `adapt`; fill ledger 12 sections of 02 to 06 (recommendations: 02 §2.5 per-match reset holds per owner; 04 slice partitioning and update order adopt within each owner; 03 §2.4 handles stay owner-local beside the stable ids of ADR-083 item 5); decide DEC-046 with the state-stream replay of ADR-086 item 6; DEC-455, DEC-264, DEC-266 on the whole-tick execution |
| D3 | The replication component on ADR-084, ADR-086 and ADR-088: tier rules, encoding, budgets, joins, the replay recorder; confirm the transport type names; extend the bandwidth model (`A2ServerBudget.py bandwidth`) with measured encodings |
| D4 | The record store of §4.3.1 (two checkpoints per owner, 7 backups, migration by schema version, whole-world rollback, restore visible to stewards, dormant records, archives of WLD-223); DEC-250, DEC-251; extend `A2ServerBudget.py storage` |
| D5 | DEC-248, DEC-249 with owner questions 4 and 5; the server key pinning of ADR-088 item 4; rendezvous and relay as a `Transport` implementation |
| D6 | The O6 server API as commands through ADR-087's path with its rate limit |
| D7 | AI runtime within 20 µs per settlement per tick and 30 µs per battle side per tick (ADR-080 item 2); AI reads the server-side seen layer (ADR-086 item 1) |
| D8 | DEC-246/247: whether normals are held (252 MB at 200) or derived; chunks by digest; path-finding between regions over the bridges of ADR-085 |
| D9 | The seen layer; review of ADR-088's handshake; accountability records of ADR-087 item 8 |
| D10 | DEC-971: the reference test machine (ADR-080 item 1 assumes at least 4 cores); telemetry of ADR-088 item 7 with consent |
| B1 | Population limit at most 750 per settlement at the plateau (ADR-080 item 3); show Essence waiting when the region is at its body budget (PIL-054) |
| B2 | The abstract step every 10 ticks (or a multiple up to 100) within 5 µs per tick (ADR-079 item 3, ADR-080 item 2); the switch at a step boundary |
| B4 | Battles of at most 1,000 figures; reinforcements wait at the edge; battlefield at a crossing between the mouths (ADR-085 item 4); expedition size for the model |
| B6 | Away-report entries per day and the share of battles with an away steward (model placeholders) |
| B7 | World AI and deputy issue validated commands executing at the next tick (ADR-087 item 1); restate the integrity rules of reference/12 07 §2.12 for the world AI |
| C1, C3 | Immediate local feedback (ADR-084 item 6); reason codes of rejected commands; the "waiting for the server" indicator (ADR-084 item 4) |
| Renderer owner (E1 or D8) | Amend RND-002, RND-018, RND-019: interpolation fraction and two tick numbers in place of the extrapolation time (ADR-084 item 4) |
| E3 | Roadmap tasks for each ADR's "Confirmed when" (simulation core: cost measurement and Level 1 replays; first playable: latency playtest, networking stage; first world-game milestone: integrated stress case, region hashes, crossing battle) |

## 8 Model results (for the rationale's "Model results")

`python3 -I Tools/DesignModels/A2ServerBudget.py check`: 37 figures and 10 tables, 0 differences. Headlines at the cost budgets:

| Figure | 16 stewards | 200 stewards |
|---|---|---|
| CPU per settlement, full / abstract | 0.72 / 0.025 ms per tick | same |
| CPU per battle of 400 / 1,000 | 0.91 / 2.18 ms per tick | same |
| World CPU, planning | 13.0 ms per tick (0.13 cores) | 113.1 ms (1.13 cores) |
| World CPU, stress | 28.3 ms (all present, 0.28 cores) | 167.3 ms (1.67 cores; 35 % of 8 vCPU) |
| At 4 × cost | 1.13 cores | 6.69 cores |
| Busiest region / at body budget | 4.73 / 8.43 ms (budget 10) | same |
| Bandwidth per client: home / battle / war / strategic | 17.0 / 22.8 / 62.3 (capped at 48) / 2.0 KB/s | 17.0 / 22.8 / 62.3 / 2.9 KB/s |
| Server upload | 0.5 Mbit/s at the peak; 2.7 with 15 remote in battle | 8.6 Mbit/s at the peak; 575 GB per 30 days |
| Storage after 84 world days | 146 MB (replays 121) | 1,439 MB (replays 1,189) |
| Growth per world week | 0.2 MB | 2.0 MB |
| Memory at stress | 32 MB | 300 MB |
| Latency, mean | 187 ms host; 267 ms at 50 ms RTT | same |
| World battle cap | 8 | 26 |

## 9 Names proposed

`RegionSimulation` (region state owner), `WorldSimulation` (world state owner), `Transport`, `InProcessTransport`, `UdpTransport`; terms: reference core (one core of the DEC-971 machine), region body budget, lent battlefield, border band, interest tiers (tier 1, 2, 3), command journal, anchor region. None is a player-facing word.
