# Verdicts, keep-list and fix-list (continued)

> Continuation of [verdicts_and_fixes.md](verdicts_and_fixes.md) for DEC-120 onwards (the first file holds DEC-001 to DEC-119 and is at the 120 KB limit). Same format; the generated index is [README.md](README.md).

| Id | Title | Status | Affects |
|---|---|---|---|
| DEC-120 | Map values: absent, zero and unlimited are distinct | Resolved | reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas.md (DAT), reference/05_population_flow.md (R-ECON) |
| DEC-121 | Ownership of Brood creatures | Open | reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/creatures_and_hazards.md (R-UNIT) |

### DEC-120 Map values: absent, zero and unlimited are distinct
- Status: Resolved
- Owner decision: O19 (2026-10-17): explicit values.
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas.md (DAT), reference/05_population_flow.md (R-ECON)
- Specified behaviour: In the typed map schema, every per-map limit (for example the Transport cap and the population cap) has three distinct forms. An **absent** value means the lobby default applies. **0** means none is allowed. **Unlimited** is its own explicit value, never encoded as 0 or as a negative number. Map values override lobby defaults (DEC-065).
- Options: A — explicit values; B — 0 means unlimited, as the reference reads caps.
- Recommendation: A. A map designer can express "none" and "unlimited" without magic numbers, and the schema validator rejects ambiguous values.

### DEC-121 Ownership of Brood creatures
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/creatures_and_hazards.md (R-UNIT)
- Specified behaviour: The creatures of the Brood outcome belong to the team that placed the power.
- Options: A — they belong to the placing team; B — they belong to the Rogue faction.
- Recommendation: decide in playtest: A makes Brood a weapon, B makes it a pure hazard, consistent with the other hostile outcomes (DEC-107).
- Decide by: playtest
