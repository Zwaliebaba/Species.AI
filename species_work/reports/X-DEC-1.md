# X-DEC-1 report — decision register: verdicts and fixes

## Files written

- `design_next/decisions/verdicts_and_fixes.md` — 117 entries, DEC-001 to DEC-117 (DEC-118 to DEC-199 unused). 117866 bytes, under the 120 KB limit, so the file is **not split**.
- `species_work/dec_map_X-DEC-1.csv` — one row per DEC (`dec_id, title, status, affects_prefix, affects_files, orig_refs`), `orig_refs` as `file:line-line` per cited table row.
- `species_work/reports/X-DEC-1.md` — this report.
- Scripts (run with `python3 -I`): `species_work/scripts/X-DEC-1_rowindex.py` (row-id → line index), `X-DEC-1_build.py` (validates field order, Decide-by rule, cross-references and Affects paths against package targets; writes the index table and the CSV), `X-DEC-1_coverage.py` (row → DEC coverage), `X-DEC-1_report.py` (this report).

No Artifact, Gamma, Google Drive, Docs or GitHub tool was used; nothing was published. No git write command was run.

## Counts

| Status | Entries |
|---|---|
| Resolved | 71 |
| Open | 34 |
| Dropped | 12 |
| **Total** | **117** |

Open entries all carry "Decide by" (design phase, playtest or prototype); Resolved and Dropped entries carry none. Of the Open entries, 7 are KEEP-HOOKS features (DEC-045 to DEC-051: map editor, replays, achievements and leaderboards, results screen, music, front-end features, tutorials).

Register layout: §1 headline verdicts (DEC-001–006), §2 simulation-integrity rules (DEC-007–016, all Resolved), §3 subsystem scope verdicts (DEC-017–029), §4 mechanic verdicts and keep-list (DEC-030–044), §5 hook-only features (DEC-045–051), §6 Dropped content (DEC-052–063), §7 fix-list (DEC-064–105), §8 design-critical uncertainties and fidelity risks (DEC-106–117).

## Gate checks run

- `scan_patterns.py forbidden_patterns.tsv` on the register: **0 hits**.
- `scan_patterns.py soft_patterns.tsv` on the register: **0 hits** (nothing to justify).
- Field order and status rules, DEC cross-references (all resolve inside the file), Affects paths (all are package target files): pass.
- Rows of the package not accounted for: none.

## Mapping: every package row to a DEC or another owner

Row keys: `Dn`, `Cn`, `Rn` (40 §2.1–2.2), `Gn`, `Bn`, `Pn`, `Kn`, `Fn` and `40-Un` are 40 rows; `4.6@line` is a 40 §4.6 row by line; `rankN` is 41 §1.3; `risk-Rnn` is 41 §2; `41@line` is a 41 §1.4 row or paragraph by line.

### D1–D10 (40 §1.2)

| Row | Accounted for by |
|---|---|
| D1 | DEC-001 |
| D2 | other agent: X-DEC-2a (networking model) |
| D3 | other agent: X-DEC-2c (numeric model) |
| D4 | other agent: X-DEC-2b (threading and process architecture) |
| D5 | DEC-029; shared with X-ADR (renderer backend) |
| D6 | DEC-002, DEC-003 |
| D7 | other agent: X-DEC-2c (sessions, lobby and services) |
| D8 | other agent: X-ADR (content pipeline) and X-DEC-2c (terrain runtime) |
| D9 | DEC-027; shared with X-ADR (audio engine) |
| D10 | DEC-045, DEC-046, DEC-047, DEC-048, DEC-049, DEC-052, DEC-053, DEC-054, DEC-055, DEC-059 |

### F1–F59 (40 §5.2)

| Row | Accounted for by |
|---|---|
| F1 | DEC-002 |
| F2 | DEC-064 |
| F3 | DEC-065 |
| F4 | DEC-011 |
| F5 | DEC-066 |
| F6 | DEC-009 |
| F7 | DEC-047 |
| F8 | DEC-067 |
| F9 | DEC-047 |
| F10 | DEC-014 |
| F11 | DEC-068 |
| F12 | DEC-069 |
| F13 | DEC-070 |
| F14 | DEC-071 |
| F15 | DEC-072 |
| F16 | other agent: X-DEC-2a |
| F17 | DEC-073 |
| F18 | DEC-074 |
| F19 | DEC-075 |
| F20 | DEC-076 |
| F21 | DEC-077 |
| F22 | DEC-078 |
| F23 | DEC-048 |
| F24 | DEC-007 |
| F25 | DEC-079 |
| F26 | DEC-080 |
| F27 | DEC-081 |
| F28 | DEC-082 |
| F29 | DEC-008, DEC-010, DEC-011, DEC-012; shared with X-DEC-2a (modified-client part) |
| F30 | other agent: X-DEC-2a |
| F31 | other agent: X-DEC-2a (numeric part: X-DEC-2c) |
| F32 | DEC-083 |
| F33 | DEC-014 |
| F34 | DEC-084 |
| F35 | DEC-085 |
| F36 | DEC-086 |
| F37 | DEC-087 |
| F38 | DEC-088, DEC-089 |
| F39 | DEC-090 |
| F40 | DEC-091 |
| F41 | DEC-092 |
| F42 | DEC-093 |
| F43 | DEC-094 |
| F44 | DEC-095 |
| F45 | DEC-012 |
| F46 | DEC-011 |
| F47 | DEC-096 |
| F48 | DEC-097 |
| F49 | DEC-049 |
| F50 | DEC-098 |
| F51 | DEC-099 |
| F52 | DEC-100 |
| F53 | DEC-101 |
| F54 | DEC-102 |
| F55 | DEC-045 |
| F56 | DEC-103 |
| F57 | DEC-104 |
| F58 | DEC-105 |
| F59 | other agent: X-DEC-2a |

### Other rows of the package

| Row | Accounted for by |
|---|---|
| C1 | DEC-013 |
| C2 | DEC-009 |
| C3 | DEC-008 |
| C4 | no DEC: method note (DP-METHOD) |
| C5 | DEC-015 |
| C6 | DEC-014 |
| C7 | other agent: X-ADR (loose-file override in development builds only) |
| C8 | DEC-011 |
| C9 | DEC-010 |
| C10 | other agent: X-DEC-2a |
| C11 | DEC-002, DEC-018 |
| C12 | DEC-041, DEC-060 |
| R1 | DEC-017 |
| R2 | DEC-018 |
| R3 | DEC-019, DEC-062 |
| R4 | DEC-020, DEC-043 |
| R5 | DEC-013, DEC-021 |
| R6 | DEC-022 |
| R7 | DEC-008, DEC-023 |
| R8 | DEC-024 |
| R9 | DEC-025 |
| R10 | DEC-045 |
| R11 | DEC-026, DEC-044, DEC-047 |
| R12 | DEC-027, DEC-049 |
| R15 | DEC-016, DEC-028 |
| R25 | DEC-024, DEC-051, DEC-053 |
| G1 | DEC-030 |
| G2 | DEC-030 |
| G3 | DEC-031 |
| G4 | DEC-031 |
| G5 | DEC-086 |
| G6 | DEC-032 |
| G7 | DEC-032 |
| G8 | DEC-033 |
| G9 | DEC-083 |
| G10 | DEC-055 |
| G11 | DEC-056 |
| G12 | DEC-008 |
| G13 | DEC-050 |
| G14 | DEC-096; shared with X-ADR (binding graph) |
| G15 | DEC-050 |
| G16 | DEC-081 |
| G17 | DEC-021 |
| G18 | DEC-021 |
| G19 | DEC-073 |
| G20 | DEC-082 |
| G21 | DEC-057 |
| G22 | DEC-018 |
| G23 | DEC-080 |
| G24 | DEC-085 |
| G25 | DEC-066 |
| G26 | DEC-058 |
| G27 | DEC-034 |
| G28 | DEC-035; shared with X-DEC-2a (reconnection) |
| G29 | DEC-036 |
| G30 | DEC-050; shared with X-DEC-2a (spectating) |
| G31 | DEC-050 |
| G32 | DEC-037 |
| G33 | DEC-051 |
| G34 | DEC-026; shared with X-DEC-2a (keep-alive) |
| G35 | DEC-054 |
| G36 | DEC-047 |
| G37 | DEC-050; shared with X-DEC-2c (matchmaking) |
| G38 | DEC-026; shared with X-DEC-2c (lobby) |
| G39 | other agent: X-DEC-2c (services) |
| G40 | DEC-048 |
| G41 | DEC-038 |
| G42 | DEC-039 |
| G43 | DEC-101 |
| G44 | DEC-040 |
| G45 | DEC-059 |
| G46 | other agent: X-ADR |
| G47 | other agent: X-ADR |
| G48 | DEC-029 |
| G49 | DEC-029 |
| G50 | DEC-029; shared with X-ADR |
| G51 | DEC-029; shared with X-ADR |
| G52 | other agent: X-ADR |
| G53 | DEC-027 |
| G54 | DEC-049 |
| G55 | DEC-027; shared with X-ADR |
| G56 | other agent: X-DEC-2a |
| G57 | DEC-046 |
| G58 | other agent: X-DEC-2a |
| G59 | other agent: X-DEC-2a |
| G60 | DEC-041 |
| G61 | DEC-041; shared with X-ADR |
| G62 | DEC-041; shared with X-ADR |
| G63 | DEC-041; shared with X-ADR |
| G64 | DEC-060; shared with X-ADR |
| G65 | DEC-060; shared with X-ADR |
| G66 | DEC-041; shared with X-ADR |
| G67 | other agent: X-DEC-2a (map identity) and X-ADR |
| G68 | DEC-061 |
| G69 | DEC-052 |
| G70 | DEC-045 |
| G71 | DEC-041; shared with X-ADR |
| G72 | other agent: X-ADR |
| B1 | DEC-002 |
| B2 | DEC-064 |
| B3 | DEC-002, DEC-018 |
| B4 | DEC-093 |
| B5 | DEC-090 |
| B6 | DEC-092 |
| B7 | DEC-094 |
| B8 | DEC-104 |
| B9 | DEC-042 |
| B10 | DEC-029 |
| B11 | DEC-016 |
| B12 | DEC-063 |
| B13 | DEC-013 |
| P2 | DEC-002 |
| P14 | DEC-014 |
| P15 | DEC-046 |
| 4.6@454 | DEC-002 |
| 4.6@455 | DEC-011 |
| 4.6@456 | DEC-007, DEC-010 |
| 4.6@457 | DEC-012, DEC-013 |
| 4.6@460 | DEC-014 |
| 4.6@467 | DEC-094 |
| K1 | DEC-001 |
| K2 | DEC-018 |
| K3 | DEC-018 |
| K4 | DEC-018 |
| K5 | DEC-030 |
| K6 | DEC-031 |
| K7 | DEC-032 |
| K8 | DEC-008 |
| K9 | DEC-021 |
| K10 | DEC-021 |
| K11 | DEC-021 |
| K12 | DEC-033 |
| K13 | DEC-018 |
| K14 | DEC-018 |
| K15 | DEC-001, DEC-017 |
| K16 | DEC-036 |
| K17 | DEC-035 |
| K18 | DEC-022 |
| K19 | DEC-017 |
| K20 | DEC-025 |
| K21 | DEC-024 |
| K22 | DEC-029 |
| K23 | DEC-027 |
| K24 | DEC-037 |
| K25 | other agent: X-DEC-2a |
| 40-U1 | DEC-106; shared with X-DEC-2a/2b (tick budget) |
| 40-U2 | DEC-003 |
| 40-U3 | DEC-079, DEC-081 |
| 40-U4 | DEC-080 |
| 40-U5 | DEC-082 |
| 40-U6 | other agent: X-DEC-2a |
| 40-U7 | other agent: X-DEC-2a |
| 40-U8 | other agent: X-DEC-2c |
| 40-U9 | other agent: X-DEC-2c |
| 40-U10 | DEC-094 |
| 40-U11 | other agent: X-DEC-2c (terrain runtime) |
| 40-U12 | DEC-047 |
| 40-U13 | no DEC: IP-register completeness only |
| 40-U14 | DEC-004 |
| 40-U15 | other agent: X-DEC-2c |
| 40-U16 | other agent: X-DEC-2c |
| 40-U17 | other agent: X-DEC-2c |
| rank1 | DEC-003 |
| rank2 | DEC-005 |
| rank3 | DEC-005 |
| rank4 | DEC-106 |
| rank5 | DEC-094 |
| rank6 | DEC-079 |
| rank7 | DEC-082 |
| rank8 | DEC-107 |
| rank9 | DEC-108 |
| rank10 | DEC-081 |
| rank11 | other agent: X-DEC-2a |
| rank12 | DEC-112 |
| rank13 | other agent: X-DEC-2a |
| rank14 | DEC-109 |
| rank15 | DEC-110 |
| risk-R01 | other agent: X-DEC-2c |
| risk-R02 | DEC-008, DEC-010 |
| risk-R03 | DEC-013 |
| risk-R04 | DEC-011 |
| risk-R05 | other agent: X-DEC-2a |
| risk-R06 | DEC-009, DEC-012 |
| risk-R07 | other agent: X-DEC-2a |
| risk-R08 | other agent: X-DEC-2a |
| risk-R09 | other agent: X-DEC-2a |
| risk-R10 | other agent: X-DEC-2c |
| risk-R11 | other agent: X-DEC-2a |
| risk-R12 | DEC-005 |
| risk-R13 | DEC-003, DEC-006 |
| risk-R14 | DEC-108, DEC-111 |
| risk-R15 | DEC-006, DEC-107 |
| risk-R16 | DEC-112 |
| risk-R17 | DEC-106 |
| risk-R18 | other agent: X-ADR |
| risk-R19 | DEC-015; shared with X-ADR |
| risk-R20 | DEC-012; shared with X-ADR |
| risk-R21 | DEC-099; shared with X-ADR |
| risk-R22 | no DEC: rename ledger and X-TOOLS |
| risk-R23 | no DEC: rename ledger and X-TOOLS |
| risk-R24 | no DEC: rename ledger and X-TOOLS |
| risk-R25 | no DEC: rename ledger and X-TOOLS |
| risk-R26 | other agent: X-DEC-2b (roadmap) |
| risk-R27 | other agent: X-DEC-2b (roadmap) |
| risk-R28 | other agent: X-DEC-2b (roadmap) |
| risk-R29 | other agent: X-DEC-2a |
| 41@89 | DEC-007, DEC-008, DEC-011, DEC-012 |
| 41@97 | DEC-113 |
| 41@98 | DEC-117 |
| 41@99 | DEC-115 |
| 41@100 | DEC-014 |
| 41@101 | DEC-117 |
| 41@102 | DEC-114 |
| 41@103 | DEC-014, DEC-114 |
| 41@104 | DEC-116 |
| 41@105 | DEC-094 |
| 41@106 | DEC-015 |
| 41@107 | DEC-031 |
| 41@108 | DEC-117 |
| 41@109 | no DEC: font-atlas identity is IP-only |
| 41@117 | DEC-064 |
| 41@118 | DEC-117 |
| 41@119 | DEC-117 |
| 41@120 | DEC-115 |
| 41@121 | DEC-022, DEC-114 |
| 41@122 | DEC-094 |
| 41@123 | DEC-015 |
| 41@124 | DEC-031 |
| 41@125 | DEC-049 |
| 41@126 | DEC-052 |
| 41@127 | DEC-041 |
| 41@129 | DEC-090 |

## Decisions taken on interpretation (for the coordinator to confirm)

1. **40 "Decide" rows** (F5, F14, F25, F26, F27, F34, F40, F43, F53 and F38 item 1) are **Open**. Following §6.5, "Specified behaviour" is the faithful reference behaviour, and 40's stated default is the **Recommendation** (DEC-066, 071, 079, 080, 081, 084, 091, 094, 101, 089).
2. **F35 / G24** (Builder orbs: "give them a destination or remove the behaviour") is **Open** (DEC-085), because 40 names two alternatives without choosing; I recommend removal for skirmish.
3. **F20** ("Flight invulnerable or terminal") is Resolved as **both** invulnerable and terminal (DEC-076); option B records the terminal-only reading.
4. **F2 / B2**: 40 names one fixed period of 11 ticks. For recounts written with a 2-second period I specified **21 ticks** by the same rule (DEC-064). G01 and G03a should confirm it against 03 §1.2.
5. **41 rank 7** (special teams in win checks) is closed by the F28 fix (DEC-082, **Resolved**), not an Open confirm, because 40 recommends a fix. **Rank 12** (bot promotion stall) is fixed by F37 (DEC-087), and the remaining calibration question is DEC-112 (Open). **Rank 14** is split: the mechanism is fixed by F48 (DEC-097), and the radius value is DEC-109 (Open, default ≈316 wu). The package note listed ranks 7, 12 and 14 as Open.
6. **F9** (Flag Sweep time test) is folded into the achievements hook DEC-047, because achievements are redesigned. **F10** (bot colour re-roll) is folded into DEC-014 (bounded algorithms). **F29** is split across DEC-008 (strike-team look-around), DEC-010 (difficulty preference), DEC-011 (zone counter) and DEC-012 (tree hit spheres).
7. **40-U14** is DEC-004, **Resolved by owner decision** (G1-4).
8. **41 ranks 2–3 and risk R12** (evidence base) have no Species consequence; they are recorded as DEC-005 (the library is the specification of record).
9. **41 §1.4 quirks with no recommendation** became Open DECs with the reference default: Abductor wander quadrant (DEC-113), weighted crate drop position (DEC-114), lobby research settings overridden (DEC-115), crates during the grace period (DEC-116). The settled facts are DEC-117 (Resolved). Facts already covered elsewhere are cited by DEC-014, 015, 022, 031, 041, 049, 052, 064, 090 and 094.

## Facts the rewrite agents should confirm in their own ranges

- DEC-021: the crate-opening condition is stated as in the keep-list ("strict majority within 35 wu; at least 51 ticks with at least 50 Workers"). G04 should state the exact rule from 04 §1.7.
- DEC-022: decision periods 200/101/11 ticks and randomness 100/50/0 % are mapped to easy/standard/hard in that order. G07 should confirm.
- DEC-036 and DEC-116: the keep-list says the world is frozen during ready-up, and 41 says crates drop during the grace period. Both are stated. G01 and G09a should reconcile which phase is frozen.
- DEC-077: the valid bot-mode values 0–2 are inferred from the `>=` correction.
- DEC-090: the Abductor speed default is the effective value 244. The alternative is the listed 500.
- 41 R15 also names "Siege Solar Array knock-out" and "Zone Control concentric zones". My inputs hold no facts on them, so I wrote no DEC. They are left to X-DEC-3 (41 §3) or X-DEC-4a.

## Cross-document requests

- **Coordinator / CONVENTIONS.md §3**: it says decisions live in `decisions/decision_register.md`. The register is split per package (`decisions/verdicts_and_fixes.md`, `decisions/deferred/…`). The line should point to `decisions/README.md`.
- **Coordinator**: the register links to `decisions/README.md` (the G2-generated index). It cites the deferred files by plain path, not as links: `decisions/deferred/networking.md`, `process_tick_and_roadmap.md` and `numerics_lobby_terrain_services.md`. Networking cross-references are written as "see DEC-2xx, networking" (DEC-008).
- **G01, C-SIM-c**: DEC-002 requires the mode second as a named constant with a tuning key in reference/12, plus a tick-count table for every timer listed in DEC-002.
- **G03a, G04, G05, G06, G07, G10**: each correction DEC (DEC-087, 088, 096, 097, 102) says "every correction the owning document lists". The owning document must therefore list every corrected defect as a rule.

## Names needed that are not in the ledger

These are descriptive phrases, not proper names. I propose them for the glossary:

- **mode second**: the 11-tick rule second. Proposed identifier `kModeSecondTicks`; it fits `ref.mode.*` or `ref.sim.*`.
- **Control Zone counter**: the per-match zone count that Flag Chain initialisation adds to.
- **launch spectacle**: Workers turning to watch a launching Launch Vessel.
- **bot-mode option**: the option whose valid values are 0 to 2.
- **actor-group offset table**.
- **Vehicle Duel vehicle**: used only in Dropped DEC-062. Its kind has no ledger term.
- **seasonal hats**: art of the seasonal theme.

## Dropped as out of scope (one line each)

- DEC-052, campaign and Advisor narrator: no match reaches them.
- DEC-053, script interpreter: no match opens a script.
- DEC-054, loader mini-programs, intro and attract mode: not player-visible.
- DEC-055, gestures and icon bar: unreachable.
- DEC-056, alternative control scheme: always switched off.
- DEC-057, Rift Horror and inactive crate rewards: never occur.
- DEC-058, Vehicle Duel: debug-only.
- DEC-059, alternative-renderer effects: never run in a match.
- DEC-060, benchmark and video capture: absent from playable builds.
- DEC-061, profiles and mods: not part of the reference product.
- DEC-062, unreachable actor kinds.
- DEC-063, reference map shapes and identities: IP with no rule.
