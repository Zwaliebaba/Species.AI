# Report A2-P (proposer, round 0 and revision after challenge round 1)

> Area A2, architecture strategy · 2026-10-18

## Round 1: revision (Step 3)

**Inputs.** `species_design_work/areas/A2/challenge_1.md` (18 findings: 0 Critical, 6 High, 11 Medium, 1 Low; 5 "make it better" items). The challenger's checks `species_design_work/scripts/A2C_ServerBudgetChallenge.py ... all` were run on the round-0 model (every figure the challenge quotes reproduced) and `wall` again on the revised model (8 vCPU ×1.01, 16 vCPU ×2.24, against the revised model's ×1.02 and ×2.22). Its `cost` and `caps` modes stop on the revised model because two parameters were renamed (`community_cores`, `region_body_budget`, now derived); the revised model computes those checks itself.

**Resolution** (`species_design_work/areas/A2/resolution_1.md`):

| Resolution | Findings | Make it better |
|---|---|---|
| Accepted | 16: F01, F02, F03, F04, F06, F07, F08, F09, F10, F11, F12, F13, F14, F15, F17, F18 | 2: MB-1, MB-5 |
| Rejected | 0 | 0 |
| Escalated to gate B | 2: F05 (second server operating system), F16 (identity, listing and relay) | 3: MB-2 (overload lever), MB-3 (measure before gate B), MB-4 (store engine) |

Two accepted items carry a stated difference: F18 item 1 (rows stay `drop` naming an ADR, because the chapter has no prefix and gate 12 rejects an `adapt` row with no defined requirement) and MB-3's timing (escalated as owner question 1 option b). **No Critical or High finding is open**: F01, F02, F03, F04 and F06 are answered by ADR and model changes; F05 is escalated with its true statements accepted now (a Windows build under O21, operating-system calls only in the platform layer, no Windows codec in persisted data) and three options with costs.

**What changed.**

| File | Change |
|---|---|
| `Tools/DesignModels/A2ServerBudget.py` | Rewritten: seen layer, keyed draws, fidelity switches with a 60 s hold, the tick's wall time (LPT over workers, serial world step, replication, the cut's copy), multiples at the wall budget and deadline, vCPU factors 0.4 / 0.6 / 1.0, ×1.25 to ×6 sensitivity with the population limit each allows, reserved body budget and side cap, skirmish views and caps, one- and two-tick baselines, the 32-tick history, the first join's terrain, side masks and a decision track in replays, uncompressed storage with the pre-migration copy, a latency Monte Carlo (100,000 samples, seed 1), 20 Hz columns; datagram overhead corrected to 77 bytes (DTLS 1.2 AES-GCM record 37 bytes); `check`: 33 figures and 12 tables, 0 differences |
| ADR-079 | 10 Hz kept; Monte Carlo latency; prediction and immediate send; split confirmation thresholds; 20 Hz budgets beside the gate-B ones; 60 s fidelity hold |
| ADR-080 | Reserved body budget 4L + 2S = 3,600, side cap 600, region step 12.5 ms (bound 8.05 ms, ×1.55), 50 ms wall budget and 100 ms deadline, the margin rule and the host table, the ×6 per-frame reading of DBG-205, remedies with battles last |
| ADR-081 | Fixed-width types, total orders, no `std::hash` or pointer in outcomes; Level 2 between integrated and dedicated by construction; hash at every cut |
| ADR-082 | Secret 128-bit simulation key and SipHash-2-4; owner id as subject; Debug assertion on a second context; generation keeps `Mix64` over the public seed; domain 3 for the command order |
| ADR-083 | Windows build per O21 with the platform layer as the only part another operating system replaces; `IntegratedServer.h`; id counter in the cut; in-region split as the first remedy; skirmish game speed; consistent cuts (60 s; 10 s in integrated worlds during battles), unclean stops recorded, the hash chain; forced-kill confirmation |
| ADR-084 | Predicted directly controlled unit with a 100 ms blend; client includes `IntegratedServer.h` only and the simulation's pure functions; uncapped in-process connection; trust model |
| ADR-085 | Visitor share at hand-over, side cap in battles, 60 s hold and switch cost, 16-vCPU stage 3, regions restored with the world's cut, the overload lever proposed as item 9 (not in force until the owner amends PIL-057 and WLD-244) |
| ADR-086 | Self-decodable datagrams, a 32-tick history, skirmish cap 64 KB/s, uncapped in-process connection, 256 KB/s join blobs, 16-bit terrain by level of detail, reclaim by identity key, side-masked replays served filtered with a decision track, uncompressed |
| ADR-087 | Keyed sender order within a tick, the skirmish host-delay option, control states sent at once, at most 3 per tick period, the predicted unit |
| ADR-088 | Schannel DTLS 1.2 with a stateless cookie, pinned self-signed server certificate, player identity key and key registry, UPnP/IPv6/manual address; the own handshake removed; the administration interface as the only host-tool path; 77-byte overhead |
| `Design/ADR/README.md` | Index titles of ADR-080 and ADR-082 to ADR-088 follow the revised headings |
| `Design/architecture/04_solution_strategy.md` | Revised throughout (53 KB): Table 1, Tables 2 to 5, both diagrams, §4.3.1 (cuts, write protocol, uncompressed storage, migration on a copy, the hash chain and its limits, ESE as owner question), §4.4 (hosts, 13 items true at 16, what 200 does not need, the gate B list), §4.5 (12 model tables pasted from `tables`, Tables 7 to 18) |
| Ledger 12 | §2.2 row: the tick source of R-SIM-510 and the keyed command order; R-SIM-605 and R-SIM-610 rows: the secret key; R-SIM-740 and R-SIM-741 rows: the consistent cut |
| `Tools/DesignModels/README.md` | The A2 entry: the `wall` mode, the new content, 33 figures and 12 tables |
| `species_design_work/areas/A2/proposal_notes.md` | §1 to §4 and §6 to §9 revised: options, recommendations, assumptions, the merged gate B list, DEC resolutions, cross-area requests, model headlines, names |

**Revised headline budgets** (at the cost budgets, computed, not measured): population limit 600 per settlement, side cap 600 (battle at most 1,200), region body budget 3,600, region step 12.5 ms (bound 8.05 ms); a world of 16 at its worst 0.32 reference cores and 12.0 ms of integrated wall time (×4.20), 60 MB memory, 207 MB storage; a world of 200 at stress 185.5 ms per tick (1.85 cores), 22.7 ms on 16 vCPU (×2.22 at the 50 ms budget, ×4.46 at the deadline), 382 MB memory, 2.05 GB storage; battle view 23.2 KB/s, skirmish 50.5 KB/s, caps 48 and 64 KB/s; host upload 2.8 Mbit/s for 15 remote stewards; egress 584 GB per 30 days at 200; orders 258 ms p95 on the host and 350 ms remote at 50 ms; predicted direct control 53 ms.

**Owner questions for gate B** (one merged list, recommended option first; notes §4): 1 scale budgets as provisional ranges (L 450 to 700, S 200 to 600), or wait for a one-week measurement; 2 battle caps max(8, 10 %); 3 hosts 16 vCPU at stage 3 and 4 vCPU (2 minimum) at stage 2; 4 dedicated-server operating systems: A Windows in stage 2 with the core portable (recommended), B Linux in stage 2 as an `AGENTS.md` change (named toolchain, MSBuild Linux projects, OpenSSL 3 and store-engine R14 exceptions, 7 to 11 engineer-weeks), C deferral to stage 3; 5 store engine: ESE if 4 = A, own store, SQLite; 6 identity, listing and relay with the storefront and publisher: the R14-clean baseline first, then Steamworks, EOS, PlayFab Party or an own relay; 7 the overload lever (amends PIL-057, WLD-244). Gate C: the R16 FMA wording and the R18 candidates.

**Finding for the coordinator.** The round-0 datagram overhead (60 bytes) and my first round-1 figure (64 bytes) undercounted the DTLS 1.2 record with an AES-GCM suite (13-byte header, 8-byte explicit nonce, 16-byte tag); the model now uses 77 bytes, which raises each view by 1 % to 7 %. DTLS 1.2 in Schannel from Windows 10 version 1607 and its ECDHE-ECDSA AES-GCM suites were checked on Microsoft Learn on 2026-10-18.

**Checks run (round 1).**
- `python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`: every gate PASS.
- `python3 -I Tools/DesignCheck/CheckDesign.py Design --design-phase` (with Mermaid, both diagrams changed): every gate PASS, 286 blocks, 0 failed.
- `python3 -I Tools/DesignModels/A2ServerBudget.py check`: 33 figures and 12 tables, 0 differences; `A1WorldDensity.py check`: 0 differences.

## Round 0: proposal (Step 1)

### Files written

`Design/architecture/04_solution_strategy.md` (arc42 chapter 4, Draft); `Design/ADR/ADR-079` to `ADR-088` (Proposed until gate B) with their index rows in `Design/ADR/README.md`; A2's rows of `Design/gdd/adoption_ledger/12_simulation_model_ledger.md`; `Tools/DesignModels/A2ServerBudget.py` and its README entry; `species_design_work/areas/A2/proposal_notes.md`.

### Counts

- Requirements: 0. The brief gives chapter 4 no prefix, so the normative content is in the ten ADRs; area D2 restates it as WSIM requirements.
- Tuning keys: 0 defined (the budgets' keys belong to the components that enforce them: D2, D3, D4).
- Diagrams: 2 Mermaid (processes and transports; one server tick).
- ADRs: 10 new, ADR-079 to ADR-088, all Proposed until gate B.

### Adoption verdicts (ledger 12, 01 and 07)

144 requirements: adopt 92, adopt-skirmish 26, drop 26, adapt 0, in 28 section rows and 12 exception rows. Drops are the reference's sequencer schedule, slice pacing and catch-up, render-time extrapolation, the in-match frame's sequencer steps, the single sequential random stream and the sequencer replay hooks; each names its replacing ADR, and D2 turns the 10 drop rows into `adapt` once WSIM ids exist. The check reports 3 adopted requirements that cite a dropped id (expected, noted in the rows). Round 1 changed notes only, no verdict.

### DEC and ADR changes

- Proposed resolutions (register edited at Finalise): DEC-200 to DEC-208, DEC-221 to DEC-228, DEC-240 to DEC-242, all Resolved, none re-dated or dropped (notes §6, revised in round 1). Inputs given to D2 (DEC-046, DEC-264), D1 (DEC-220), D4 (DEC-250, DEC-251), D5 (DEC-248, DEC-249).
- ADRs: ADR-079 to ADR-088 added; ADR-084 amends the default of RND-002 and RND-018 (interpolation and one predicted unit), for the renderer's owning area to apply.

### Cross-area requests

Notes §7: coordinator, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, B1, B2, B4, B6, B7, B9, C1, C2, C3, the renderer's owner, E3.

### Names proposed

`RegionSimulation`, `WorldSimulation`, `Transport`, `InProcessTransport`, `UdpTransport`, `IntegratedServer.h`; terms: reference core, region body budget, visitor share, side cap, tick wall budget, margin rule, consistent cut, lent battlefield, border band, interest tiers, command journal, anchor region, simulation key, identity key, overload lever.

### Findings for the coordinator (round 0)

- `AGENTS.md` R16 says `/arch:AVX2` lets MSVC contract into FMA under `/fp:precise`; Microsoft's `/fp` documentation (checked 2026-10-18) says that contraction stopped by default in Visual Studio 2022 and needs `/fp:contract`; the challenger confirmed it. No setting changes; the wording is a gate C recommendation.
- The UCRT's transcendental functions choose implementations by CPU at run time, which is why ADR-081 requires own deterministic functions even for one build.
