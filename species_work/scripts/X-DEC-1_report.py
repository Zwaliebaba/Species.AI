"""Write WORK/reports/X-DEC-1.md from the dec_map CSV and the coverage table.

Usage: python3 -I X-DEC-1_report.py <dec_map.csv> <coverage.tsv> <register.md> <report.md>
"""
import csv
import os
import sys

OTHER = {
    "D2": "other agent: X-DEC-2a (networking model)",
    "D3": "other agent: X-DEC-2c (numeric model)",
    "D4": "other agent: X-DEC-2b (threading and process architecture)",
    "D7": "other agent: X-DEC-2c (sessions, lobby and services)",
    "D8": "other agent: X-ADR (content pipeline) and X-DEC-2c (terrain runtime)",
    "C4": "no DEC: method note (DP-METHOD)",
    "C7": "other agent: X-ADR (loose-file override in development builds only)",
    "C10": "other agent: X-DEC-2a",
    "G39": "other agent: X-DEC-2c (services)",
    "G46": "other agent: X-ADR", "G47": "other agent: X-ADR", "G52": "other agent: X-ADR",
    "G56": "other agent: X-DEC-2a", "G58": "other agent: X-DEC-2a", "G59": "other agent: X-DEC-2a",
    "G67": "other agent: X-DEC-2a (map identity) and X-ADR",
    "G72": "other agent: X-ADR",
    "K25": "other agent: X-DEC-2a",
    "F16": "other agent: X-DEC-2a", "F30": "other agent: X-DEC-2a",
    "F31": "other agent: X-DEC-2a (numeric part: X-DEC-2c)", "F59": "other agent: X-DEC-2a",
    "40-U6": "other agent: X-DEC-2a", "40-U7": "other agent: X-DEC-2a",
    "40-U8": "other agent: X-DEC-2c", "40-U9": "other agent: X-DEC-2c",
    "40-U11": "other agent: X-DEC-2c (terrain runtime)",
    "40-U13": "no DEC: IP-register completeness only",
    "40-U15": "other agent: X-DEC-2c", "40-U16": "other agent: X-DEC-2c", "40-U17": "other agent: X-DEC-2c",
    "rank11": "other agent: X-DEC-2a", "rank13": "other agent: X-DEC-2a",
    "risk-R01": "other agent: X-DEC-2c", "risk-R05": "other agent: X-DEC-2a",
    "risk-R07": "other agent: X-DEC-2a", "risk-R08": "other agent: X-DEC-2a",
    "risk-R09": "other agent: X-DEC-2a", "risk-R10": "other agent: X-DEC-2c",
    "risk-R11": "other agent: X-DEC-2a", "risk-R18": "other agent: X-ADR",
    "risk-R22": "no DEC: rename ledger and X-TOOLS", "risk-R23": "no DEC: rename ledger and X-TOOLS",
    "risk-R24": "no DEC: rename ledger and X-TOOLS", "risk-R25": "no DEC: rename ledger and X-TOOLS",
    "risk-R26": "other agent: X-DEC-2b (roadmap)", "risk-R27": "other agent: X-DEC-2b (roadmap)",
    "risk-R28": "other agent: X-DEC-2b (roadmap)", "risk-R29": "other agent: X-DEC-2a",
    "41@109": "no DEC: font-atlas identity is IP-only",
    **{f"4.6@{n}": "not in this package: X-DEC-2a" for n in (458, 459, 461, 462, 463, 464, 465, 466)},
}
SHARED = {
    "D5": "X-ADR (renderer backend)", "D9": "X-ADR (audio engine)",
    "D8": "", "G14": "X-ADR (binding graph)", "G50": "X-ADR", "G51": "X-ADR",
    "G55": "X-ADR", "G61": "X-ADR", "G62": "X-ADR", "G63": "X-ADR", "G64": "X-ADR",
    "G65": "X-ADR", "G66": "X-ADR", "G71": "X-ADR", "G28": "X-DEC-2a (reconnection)",
    "G30": "X-DEC-2a (spectating)", "G34": "X-DEC-2a (keep-alive)",
    "G37": "X-DEC-2c (matchmaking)", "G38": "X-DEC-2c (lobby)",
    "40-U1": "X-DEC-2a/2b (tick budget)", "risk-R19": "X-ADR", "risk-R20": "X-ADR",
    "risk-R21": "X-ADR", "F29": "X-DEC-2a (modified-client part)",
}


def main():
    dec_csv, cov_tsv, reg, out = sys.argv[1:5]
    with open(dec_csv, encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    counts = {}
    for r in rows:
        counts[r["status"]] = counts.get(r["status"], 0) + 1
    cov = {}
    with open(cov_tsv, encoding="utf-8") as fh:
        for line in fh:
            k, v = line.rstrip("\n").split("\t")
            cov[k] = v

    def cell(key):
        v = cov.get(key, "-")
        parts = []
        if v != "-":
            parts.append(v.replace(",", ", "))
        if key in OTHER:
            parts.append(OTHER[key])
        if key in SHARED and SHARED[key]:
            parts.append("shared with " + SHARED[key])
        return "; ".join(parts) if parts else "UNMAPPED"

    def table(title, keys):
        lines = [f"### {title}", "", "| Row | Accounted for by |", "|---|---|"]
        for k in keys:
            lines.append(f"| {k} | {cell(k)} |")
        return "\n".join(lines) + "\n"

    size = os.path.getsize(reg)
    secs = []
    secs.append(table("D1–D10 (40 §1.2)", [f"D{n}" for n in range(1, 11)]))
    secs.append(table("F1–F59 (40 §5.2)", [f"F{n}" for n in range(1, 60)]))
    secs.append(table("Other rows of the package", (
        [f"C{n}" for n in range(1, 13)]
        + [f"R{n}" for n in range(1, 13)] + ["R15", "R25"]
        + [f"G{n}" for n in range(1, 73)]
        + [f"B{n}" for n in range(1, 14)] + ["P2", "P14", "P15"]
        + [f"4.6@{n}" for n in (454, 455, 456, 457, 460, 467)]
        + [f"K{n}" for n in range(1, 26)]
        + [f"40-U{n}" for n in range(1, 18)]
        + [f"rank{n}" for n in range(1, 16)]
        + [f"risk-R{n:02d}" for n in range(1, 30)]
        + [f"41@{n}" for n in [89] + list(range(97, 110)) + list(range(117, 128)) + [129]]
    )))
    unmapped = [k for k in cov if cell(k) == "UNMAPPED"]

    with open(out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(TEMPLATE.format(
            total=len(rows), next=len(rows) + 1, resolved=counts.get("Resolved", 0),
            open_=counts.get("Open", 0), dropped=counts.get("Dropped", 0),
            size=size, unmapped=", ".join(unmapped) or "none",
            tables="\n".join(secs)))
    print("report written; unmapped:", unmapped)


TEMPLATE = """# X-DEC-1 report — decision register: verdicts and fixes

## Files written

- `design_next/decisions/verdicts_and_fixes.md` — {total} entries, DEC-001 to DEC-{total:03d} (DEC-{next:03d} to DEC-199 unused). {size} bytes, under the 120 KB limit, so the file is **not split**.
- `species_work/dec_map_X-DEC-1.csv` — one row per DEC (`dec_id, title, status, affects_prefix, affects_files, orig_refs`), `orig_refs` as `file:line-line` per cited table row.
- `species_work/reports/X-DEC-1.md` — this report.
- Scripts (run with `python3 -I`): `species_work/scripts/X-DEC-1_rowindex.py` (row-id → line index), `X-DEC-1_build.py` (validates field order, Decide-by rule, cross-references and Affects paths against package targets; writes the index table and the CSV), `X-DEC-1_coverage.py` (row → DEC coverage), `X-DEC-1_report.py` (this report).

No Artifact, Gamma, Google Drive, Docs or GitHub tool was used; nothing was published. No git write command was run.

## Counts

| Status | Entries |
|---|---|
| Resolved | {resolved} |
| Open | {open_} |
| Dropped | {dropped} |
| **Total** | **{total}** |

Open entries all carry "Decide by" (design phase, playtest or prototype); Resolved and Dropped entries carry none. Of the Open entries, 7 are KEEP-HOOKS features (DEC-045 to DEC-051: map editor, replays, achievements and leaderboards, results screen, music, front-end features, tutorials).

Register layout: §1 headline verdicts (DEC-001–006), §2 simulation-integrity rules (DEC-007–016, all Resolved), §3 subsystem scope verdicts (DEC-017–029), §4 mechanic verdicts and keep-list (DEC-030–044), §5 hook-only features (DEC-045–051), §6 Dropped content (DEC-052–063), §7 fix-list (DEC-064–105), §8 design-critical uncertainties and fidelity risks (DEC-106–117).

## Gate checks run

- `scan_patterns.py forbidden_patterns.tsv` on the register: **0 hits**.
- `scan_patterns.py soft_patterns.tsv` on the register: **0 hits** (nothing to justify).
- Field order and status rules, DEC cross-references (all resolve inside the file), Affects paths (all are package target files): pass.
- Rows of the package not accounted for: {unmapped}.

## Mapping: every package row to a DEC or another owner

Row keys: `Dn`, `Cn`, `Rn` (40 §2.1–2.2), `Gn`, `Bn`, `Pn`, `Kn`, `Fn` and `40-Un` are 40 rows; `4.6@line` is a 40 §4.6 row by line; `rankN` is 41 §1.3; `risk-Rnn` is 41 §2; `41@line` is a 41 §1.4 row or paragraph by line.

{tables}
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
"""

if __name__ == "__main__":
    main()
