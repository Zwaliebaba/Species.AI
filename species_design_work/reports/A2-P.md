# Report A2-P (proposer, round 0)

> Area A2, architecture strategy · 2026-10-18

## Files written

| File | What |
|---|---|
| `Design/architecture/04_solution_strategy.md` | arc42 chapter 4, Draft: 4.1 technology decisions, 4.2 decomposition (processes, libraries, integrated and dedicated, one server tick, hand-offs), 4.3 quality goals with §4.3.1 persistence, 4.4 scale path, 4.5 model results (36 KB) |
| `Design/ADR/ADR-079-simulation-tick-rate.md` | 10 ticks per second for world and skirmish, whole ticks with the ten slices in order, abstract step every 10 ticks, a fixed conversion rule for any later change |
| `Design/ADR/ADR-080-per-tick-budgets-and-population-caps.md` | Per-figure cost budgets, a 10 ms region step, a 4,000-body region budget, battles of at most 1,000, caps 2 per region and max(8, 10 %) per world, integrated budget 1 core, host specifications |
| `Design/ADR/ADR-081-numeric-model-and-determinism-scope.md` | Strict binary64 under `/fp:precise` without contraction, own deterministic maths functions, three determinism levels, exact encoding, canonical state hash, fixed point in reserve |
| `Design/ADR/ADR-082-counter-based-simulation-random-draws.md` | Counter-based keyed draws (SplitMix64 output function chained over seed, domain, purpose, subject, tick, ordinal), shared with world generation |
| `Design/ADR/ADR-083-server-process-ownership-and-threading.md` | One headless portable server library, composition roots, lifetime scopes, state machines, owners per region, world and match, stable ids, region jobs, deadline-paced loop, loading, checkpoints |
| `Design/ADR/ADR-084-client-boundary-through-the-transport.md` | The transport as the only path (in-process included), client replica, presentation events, interpolation |
| `Design/ADR/ADR-085-region-partitioned-world-simulation.md` | Region owners on one tick, messages at tick boundaries, ownership by position, battles at crossings in their anchor region with a lent battlefield, border band, stage-3 deployment |
| `Design/ADR/ADR-086-replication-and-interest-management.md` | State replication with three interest tiers, 32 KB/s target and 48 KB/s cap per client, snapshot joins, replays as state streams |
| `Design/ADR/ADR-087-command-path-validation-and-timing.md` | One validated command path for every source, arrival and execution checks, next-tick execution, coalescing, rate limits |
| `Design/ADR/ADR-088-transport-messages-identity-and-diagnostics.md` | `Transport` with in-process and own encrypted UDP implementations, schema-defined messages, build and content handshake, diagnostics and a command journal |
| `Design/ADR/README.md` | Index rows for ADR-079 to ADR-088 |
| `Design/gdd/adoption_ledger/12_simulation_model_ledger.md` | A2's rows: 28 section rows of 01 and 07 and 12 exception rows; one explanatory paragraph |
| `Tools/DesignModels/A2ServerBudget.py` and its `README.md` entry | The server-budget model; `check` reproduces 37 figures and 10 tables |
| `species_design_work/areas/A2/proposal_notes.md` | Options, recommendations, assumptions, owner questions, adoption, the 20 decisions, cross-area requests, model results, names |

## Counts

- Requirements: 0. The brief gives chapter 4 no prefix, so the normative content is in the ten ADRs; area D2 restates it as WSIM requirements.
- Tuning keys: 0 defined (the budgets' keys belong to the components that enforce them: D2, D3, D4).
- Diagrams: 2 Mermaid (processes and transports; one server tick); gate 4 passes on all 286 blocks.
- ADRs: 10 new, ADR-079 to ADR-088, all Proposed until gate B.

## Adoption verdicts (ledger 12, 01 and 07)

144 requirements: adopt 92, adopt-skirmish 26, drop 26, adapt 0. Drops are the reference's sequencer schedule, slice pacing and catch-up, render-time extrapolation, the in-match frame's sequencer steps, the single sequential random stream and the sequencer replay hooks; each names its replacing ADR, and D2 turns the 10 drop rows into `adapt` once WSIM ids exist. The check reports 3 adopted requirements that cite a dropped id (expected, noted in the rows).

## DEC and ADR changes

- Proposed resolutions (register edited at Finalise): DEC-200 to DEC-208, DEC-221 to DEC-228, DEC-240 to DEC-242, all Resolved, none re-dated or dropped (notes §6). Inputs given to D2 (DEC-046, replays as state streams), D1 (DEC-220), D4 (DEC-250, DEC-251), D5 (DEC-248, DEC-249).
- ADRs: ADR-079 to ADR-088 added; ADR-084 amends the default of RND-002 and RND-018 (interpolation), for the renderer's owning area to apply.

## Cross-area requests

Notes §7: coordinator (names, ledger totals, register resolutions, gate questions), D1, D2 (WSIM requirements, adapt rows, sections 02 to 06, DEC-046), D3, D4, D5, D6, D7, D8, D9, D10, B1, B2, B4, B6, B7, C1, C3, the renderer's owner, E3.

## Names proposed

`RegionSimulation`, `WorldSimulation`, `Transport`, `InProcessTransport`, `UdpTransport`; terms: reference core, region body budget, lent battlefield, border band, interest tiers, command journal, anchor region.

## Findings for the coordinator

- AGENTS.md R16 says `/arch:AVX2` lets MSVC contract into FMA under `/fp:precise`; Microsoft's `/fp` documentation (checked 2026-10-18) says that contraction stopped by default in Visual Studio 2022 and needs `/fp:contract`. No setting changes; the wording is an owner amendment (owner question 7, gate C).
- The UCRT's transcendental functions choose implementations by CPU at run time (Microsoft Learn, `_set_FMA3_enable` and the floating-point support page), which is why ADR-081 requires own deterministic functions even for one build.

## Checks run

- `python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`: every gate PASS.
- `python3 -I Tools/DesignCheck/CheckDesign.py Design --design-phase` (with Mermaid): every gate PASS, 286 blocks, 0 failed.
- `python3 -I Tools/DesignModels/A2ServerBudget.py check`: 47 figures and tables, 0 differences; `A1WorldDensity.py check`: 0 differences.
