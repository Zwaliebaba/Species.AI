---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead (owner gate B); proposed by design-phase area A2
---

# Simulation numeric model: strict binary64 with deterministic maths functions, a stated determinism scope and a canonical state hash

## Context and Problem Statement

The reference holds simulation state in binary64, rounds every transcendental and square-root result to binary32, and relies on every participant running one build (DEC-240); a few of its rules depend on numeric semantics such as truncation, degenerate-vector fallbacks and accumulated timers (DEC-241). Species is server-authoritative (vision §5, ADR-086): no two processes simulate one world live, so bit-exact agreement between processes is not needed for play. It is needed elsewhere: tests and soak runs compare per-tick state hashes; an operator's reported defect is reproduced on a developer's machine from a checkpoint and the commands that followed it; the abstract economy is calibrated against the full simulation (O3); a region moves between server processes in stage 3 (O1); A1's world generator must give the same world on any server (WLD-161, AC-21); and a dedicated server for a second operating system, built with a second compiler, is the gate-B recommendation. AGENTS.md fixes `/fp:precise` and `/arch:AVX2` in every project (R16) and asks for an ADR before a deterministic core. Two facts were checked on 2026-10-18 in Microsoft's documentation: since Visual Studio 2022, `/fp:precise` generates no floating-point contraction unless `/fp:contract` is given (https://learn.microsoft.com/cpp/build/reference/fp-specify-floating-point-behavior); and the Universal CRT selects among implementations of its transcendental functions by the CPU's instruction sets at run time, so their results can differ between computers and between Windows versions (https://learn.microsoft.com/cpp/c-runtime-library/floating-point-support, https://learn.microsoft.com/cpp/c-runtime-library/reference/get-fma3-enable-set-fma3-enable).

Which number representation and floating-point rules does the simulation use, which results must agree between which builds, and how is a disagreement found?

## Decision Drivers

* Reproducible tests, soak runs and defect reproduction from the first month (DEC-240 item 9).
* A second server build of the same release, with a second compiler, kept possible (gate B).
* The engine baseline is binary64 (PLT-134, PLT-135, TER-061); rewriting it to fixed point is a large cost for a small team.
* No player-facing feature may stop working because two builds disagree in one bit.
* The AGENTS.md settings stand as they are (R16, §3).

## Considered Options

* A — the reference model: binary64 state, binary32-rounded transcendental results, one build per session
* B — strict binary64: simulation code in its own library under a strict floating-point model, an own deterministic maths library, the floating-point environment asserted
* C — fixed point: integer simulation state with table-driven or CORDIC trigonometry
* D — no cross-process bit-exactness: reproducibility within one build only

## Decision Outcome

Proposed option: "B — strict binary64", scoped by the determinism levels below, with C held in reserve. **Default while Proposed:** the decision below.

1. **Representation.** Simulation state is IEEE-754 binary64 for positions (world units in world coordinates), velocities, stats (DEC-090) and every other continuous quantity; every timer is an integer count of ticks (R-SIM-501); counters and ids are integers. At the far corner of the reserved extent (71,680 wu, WLD-001) binary64 has a spacing of 1.5 × 10⁻¹¹ wu, so the simulation needs no origin shifting; replication sends region-relative quantised positions (ADR-086) and rendering is camera-relative (WLD-246).
2. **Compiler model.** Every project compiles `/fp:precise` and `/arch:AVX2` as AGENTS.md requires, and no simulation, server or engine maths project sets `/fp:contract`, `/fp:fast` or a floating-point pragma; `Build\CheckProjectFiles.py` checks both. Under these settings MSVC preserves source order and rounding and generates no contraction, so every basic operation is correctly rounded binary64. A second compiler builds the same libraries with contraction off and no fast-math option, and its build is accepted only through the cross-build test of item 6.
3. **Deterministic maths functions.** Simulation code, world generation and every content-build step whose output feeds simulation state call no CRT transcendental function (`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `exp`, `exp2`, `log`, `log2`, `log10`, `pow`, `cbrt`, `hypot`, the hyperbolic functions and `erf`). The engine maths library (PLT-134) provides deterministic implementations of each: argument reduction and polynomial evaluation in binary64, in a fixed order written in the library, tested against golden vectors. `sqrt`, `floor`, `ceil`, `trunc`, `fmod`, `fma` written explicitly, minimum and maximum are exact IEEE operations and stay allowed. The terrain generator keeps the binary32 rounding of TER-065 over these functions, so the conformance vectors of the terrain component hold. The project-file checker rejects a CRT transcendental call in a simulation project.
4. **Floating-point environment.** Simulation runs in the default environment: round to nearest, ties to even, no flush to zero, no denormals-are-zero, exceptions masked. Every tick entry and every worker that runs simulation code asserts it in Debug and test builds and checks it in Release, where a difference is restored and logged. No third-party code runs inside a tick (R-SIM-720).
5. **Numeric semantics (DEC-241, option B).** The observable semantics that reference rules depend on stay explicit rules with acceptance vectors: truncation toward zero for every float-to-integer conversion (DEC-241 item 1), the degenerate-vector and degenerate-query results of PLT-137 and PLT-138, the tolerance values of DEC-241 item 8 (never used for ordering, hashing or set membership), integer tick timers for every countdown (item 3), and the formation volley timing restated on the integer tick counter (item 4). The incidental binary32 rounding of the reference (transcendental and square-root results, binary32 fields and locals) is dropped from simulation rules; every simulation-feeding decimal is parsed once, at content build, by one correctly rounded parser and rejected when malformed (item 12); lobby values are integers or fixed-point values with ranges (item 13).
6. **Determinism scope.**

   | Level | What agrees bit for bit | Required for | Status |
   |---|---|---|---|
   | 1 | one build, on any supported x64 CPU and at any thread count: the same state and inputs give the same state every tick | tests and golden hashes, soak runs, an operator's defect reproduced from a checkpoint and its command journal, the abstract model's calibration (O3), a region handed between processes of one build | required, tested from the simulation-core milestone |
   | 2 | every executable of one release: the client's integrated server, the Windows dedicated server and any later server for a second operating system | A1's world generation on any server (WLD-161), a region handed between hosts of different builds, cross-build defect reproduction | required for world generation; tested for the step from the first second build; a failure is a defect, never an outage, because no player-facing feature depends on Level 2 for the step |
   | 3 | builds of different releases | nothing: persisted state migrates by data transformation (D4), and replays, journals and golden hashes belong to their release | not provided |

   Battle replays are recorded state streams (ADR-086), so they need no level. Iteration over unordered containers never reaches an outcome (R-SIM-680, DEC-016), and every parallel phase produces the result of the serial order (R-SIM-732, ADR-083).
7. **Exact encoding.** Every value that is persisted, transferred between processes or hashed is encoded exactly: a defined little-endian binary layout, binary64 as its 64 bits, never decimal text (DEC-240 item 10).
8. **Canonical state hash (DEC-206, option A scoped).** Each state owner (region, world and skirmish match; ADR-083) computes a 64-bit hash of its canonical serialisation. Debug, test, CI and soak runs compute it every tick; a Release server computes it at every checkpoint, at both ends of every region hand-over, and once every 10 ticks for each region, staggered, into its command journal. Where two runs must agree (a test, a defect reproduction, a hand-over) the first differing tick produces a field-level diff of both states (DBG). A hand-over whose hashes differ is refused and the region stays with its sender; a Release server never stops its world over a hash.
9. **Fixed point in reserve.** If the cross-build test of a second compiler fails and its cause cannot be removed within items 2 to 5, a superseding ADR moves positions and movement integration to fixed point in region-local coordinates before that build is released. Level 1 does not depend on it.

### Consequences

* Good, because Level 1 holds by construction: no CPU-dependent library path, no contraction, no unordered iteration, and one environment.
* Good, because the engine baseline stays binary64, and the reference rules keep their arithmetic apart from the dropped binary32 rounding.
* Good, because a second operating system's server build is possible without a numeric rewrite, and the cost of Level 2 is a compiler flag and a test.
* Bad, because Species writes and tests its own transcendental functions, and every simulation call site uses them; the checker enforces it.
* Bad, because dropping the binary32 rounding changes reference results in their last bits wherever a rule used a rounded function, so golden vectors are Species vectors, recorded once (DBG-147).
* Neutral, because Level 3 is ruled out deliberately: a patch never re-simulates history.

### Confirmation

**Confirmed when:** the simulation-core milestone (DEC-229) replays a synthetic battle of 2,000 figures with identical per-tick hashes on two CPU generations of each of two x64 vendors and with 1, 2 and 8 worker threads (Level 1); the deterministic functions pass their golden vectors on those machines; a test that adds a cosmetic effect leaves every hash unchanged; and, once a second build exists, the cross-build test replays the same corpus with identical hashes on both builds (Level 2).

## Pros and Cons of the Options

### A — the reference model

* Good, because it needs no new maths library.
* Bad, because the CRT's CPU-dependent paths break even Level 1 across computers, and binary32 rounding narrows the dependence without removing it (DEC-240 item 5).

### B — strict binary64

* Good, because it gives Level 1 by construction and Level 2 with discipline, inside the AGENTS.md settings.
* Bad, because it needs an own maths library and a checker rule.

### C — fixed point

* Good, because it is bit-exact across compilers and architectures by construction.
* Bad, because it rewrites the engine baseline (PLT, TER) and every reference rule's arithmetic for a guarantee that Species needs only for world generation and for diagnostics.

### D — one build only

* Good, because it is the cheapest.
* Bad, because without own maths functions it does not even give Level 1 across computers, and it closes the second server build.

## More Information

* Resolves DEC-240, DEC-241 and DEC-206 when the owner confirms it at gate B. Related: [ADR-082](ADR-082-counter-based-simulation-random-draws.md) (random draws), [ADR-083](ADR-083-server-process-ownership-and-threading.md) (ownership and threads), [ADR-085](ADR-085-region-partitioned-world-simulation.md) (hand-over), [ADR-086](ADR-086-replication-and-interest-management.md) (replays as state streams).
* AGENTS.md R16 states that `/arch:AVX2` lets MSVC contract `a*b+c` into an FMA under `/fp:precise`; the Microsoft documentation cited above says that this stopped with Visual Studio 2022. The settings of R16 stand either way; the difference is reported to the owner for an amendment of R16's wording.
* The deterministic-core rules of AGENTS.md R16 (integer ticks, ordered iteration, named random streams, no wall-clock time) apply to the simulation libraries; the design phase proposes them as R18 candidates at gate C (O24).
