# Decision register: verdicts and fixes

> Status: Reference decisions · Range: DEC-001 to DEC-199 · Related: [decisions index](README.md), [conventions](../CONVENTIONS.md)

This file is the canonical register for the scope and behaviour decisions of the reference library. It holds the headline verdicts on the reference core loop, simulation timing and hook-only features; the simulation-integrity rules; the scope verdict for each reference subsystem; the verdicts on individual mechanics, including the keep-list; one decision for every item of the fix-list; and the design-critical uncertainties and gameplay-fidelity risks that need confirmation. Each entry states the behaviour the reference documents specify now. A **Resolved** entry states the corrected rule that the documents apply; an **Open** entry states the faithful reference behaviour as the default, with a recommendation; a **Dropped** entry records content the library does not specify. The networking model, numeric model, simulation threading, tick rate, sessions and services, lobby protocol, persistence and terrain runtime model are design-phase topics recorded under `decisions/deferred/`; engine-technology choices are ADRs under `adr/`. Other decision files cite the entries here instead of repeating them.

<!-- INDEX -->

## 1 Headline verdicts

### DEC-001 Reference core loop: crowd-and-territory economy
- Status: Open
- Affects: reference/00_reference_overview.md (R-VIS), reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: The reference game has no build queue and no currency. A team's army is the population that its held Spawn Nodes produce: a Spawn Node is captured by a single team with at least 3 operators on its 5 ports and no enemy on any port, activates 300 ticks after an ownership change, and produces in bursts and pauses up to a per-team population cap. Workers fight and re-plan on their own; Leaders act as signposts and formation leaders; Supply crates open to outcomes that are 90 % beneficial and 10 % hostile and feed six Power slots; six modes share one rules skeleton; bots play on a node graph with perfect information and no other advantage. The reference library specifies this loop in full and keeps it as the rule set of the skirmish mode.
- Options: A — the world game keeps the reference loop unchanged; B — the world game adds settlement building and a resource economy on top of essence-driven growth, while skirmish keeps the reference loop; C — the world game replaces the loop entirely.
- Recommendation: B. The vision brief overrides "no build queue and no currency" for the persistent world, where settlements gather resources, build and grow, but keeps a skirmish mode that is the combat balance test bed (vision O8). Keeping the reference loop authoritative for skirmish preserves the tuned combat and population behaviour; the design phase decides how settlement growth uses Essence orbs and whether a resource layer exists.
- Decide by: design phase

### DEC-002 Integer tick timers and the 11-tick mode second
- Status: Resolved
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/05_population_flow.md (R-ECON), reference/03_buildings_and_world_objects/README.md (R-BLDG), reference/04_supply_crates_powers_and_weapons/README.md (R-CRATE), reference/02_units/README.md (R-UNIT)
- Specified behaviour: Every rule timer, cooldown, lifetime and probability in the simulation is an integer number of ticks, or a per-second rate converted to ticks when the data loads; no simulation timer is a floating-point countdown. The **mode second** is an explicit named constant of 11 ticks (1.1 s at the reference tick rate of 10 ticks per second). The match clock, elimination countdowns, Relic timers, reinforcement waves and rank checks advance once per mode second. The reference tick counts are: one mode second 11; grace period 30; victory timers 101, 151 and 51 (written as 10, 15 and 5 mode seconds); Comeback Bonus interval 450; the 10-second production pause 101; burst request interval 2; rocket turret reload 15; Convert Turret reload 6; mortar turret reload 40; Spawn Node activation 300; Essence orb birth 61; Relay transit 10h + 1 ticks for h hops (11 for the first hop, 10 for each further hop). Seconds appear only as a derived display value next to the tick count.
- Options: A — integer tick counts with the mode second as an explicit 11-tick constant; B — real-time countdowns in seconds; C — integer ticks with a 10-tick second from the start.
- Recommendation: A. It keeps every tuned value of the reference valid for the first playable, removes floating-point accumulation from rule timing, and makes each timer testable: a table-driven test checks the tick count of every timer, and a static check rejects floating-point timer fields in simulation code. Re-tuning to a 10-tick second is a separate, later decision (DEC-003).

### DEC-003 Mode-second length after playtest
- Status: Open
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/05_population_flow.md (R-ECON)
- Specified behaviour: The mode second is 11 ticks (DEC-002). Every per-second rule, wave and elimination timer, the on-screen match clock and all balance values are tuned under it, so displayed seconds run 10 % slower than real seconds.
- Options: A — keep the 11-tick mode second permanently; B — change the mode second to 10 ticks and re-tune every value that depends on it; C — keep 11 ticks internally but display real seconds.
- Recommendation: A for the first playable, then an A/B playtest of 10-tick and 11-tick seconds. Changing the constant alone shifts all mode timings and the economy by 10 %, so B is acceptable only with a deliberate re-tune of the affected tuning keys.
- Decide by: playtest

### DEC-004 Reference tuning values adopted as the Species baseline
- Status: Resolved
- Affects: every reference document tuning table (R-VIS, R-MODE, R-UNIT, R-BLDG, R-CRATE, R-ECON, R-CTRL, R-BOT, R-MAP, R-META, R-AUD, R-TUT, R-SIM)
- Specified behaviour: The reference documents keep every numeric tuning value: stats, timings, caps and probabilities. Species may adopt these values as its starting values. Tuning tables carry no "provisional" flag on account of where a value comes from. A legal check of the adopted values is required before any commercial release.
- Options: A — adopt the reference values as the starting baseline; B — keep the structure and re-tune every number before use.
- Recommendation: A, by owner decision (2026-10-17). The keep-list in this register therefore means "same rule and same numbers" for the first tuning baseline; the design phase re-tunes freely afterwards.

### DEC-005 The reference library is the specification of record
- Status: Resolved
- Affects: every reference document (R-VIS, R-MODE, R-UNIT, R-BLDG, R-CRATE, R-ECON, R-CTRL, R-BOT, R-MAP, R-META, R-AUD, R-TUT, R-SIM)
- Specified behaviour: Where a reference document specifies a rule or a value, that statement is the reference behaviour. Species sets no parity target against any other edition or build of the reference game, and no requirement depends on such a comparison.
- Options: A — the reference library is authoritative; B — verify the library against another edition of the reference game before use.
- Recommendation: A. Species is a new game; the library is complete for its purpose, and the decisions in this register record every place where the reference behaviour is uncertain or changed.

### DEC-006 Reference quirks are kept as explicit, named rules
- Status: Resolved
- Affects: every reference document (R-VIS, R-MODE, R-UNIT, R-BLDG, R-CRATE, R-ECON, R-CTRL, R-BOT, R-MAP, R-META, R-AUD, R-TUT, R-SIM)
- Specified behaviour: A reference behaviour that looks accidental and is not corrected by a Resolved decision is specified as an explicit rule with a named tuning key or constant, never left implicit in an algorithm. Each such quirk is decided individually in this register (for example DEC-003, DEC-065, DEC-079, DEC-081 and DEC-094) before content tuning starts. Where both variants are cheap to build, both are playtested.
- Options: A — name and specify every kept quirk, and decide each one before tuning; B — correct quirks silently during implementation.
- Recommendation: A. The reference balance is tuned under these quirks (the 11-tick second, the doubled reinforcement advance, the binary catch-up bonus, the per-team cap divisor and the approximate grid queries), so changing one changes the game; naming them makes each change a visible decision and protects the keep and fix lists from late rework.

## 2 Simulation-integrity rules
