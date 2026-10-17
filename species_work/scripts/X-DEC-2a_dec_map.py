#!/usr/bin/env python3
"""Build WORK/dec_map_X-DEC-2a.csv from the X-DEC-2a package and the range-to-DEC map below.

Usage:
  python3 -I X-DEC-2a_dec_map.py <package.json> <out.csv> [<report_table.md>]

The package's `inputs` list is indexed 0..N-1 in file order. MAP assigns each input index the
DEC ids it fed; UNUSED lists the indexes that fed no DEC (with the reason, for the report).
Every index must appear in exactly one of MAP or UNUSED.
"""
import csv
import json
import sys

DECS = {
    200: ("Replication and authority model",
          ["R-SIM", "R-BOT", "R-MODE", "PLT"],
          ["reference/12_simulation_model/06_teams_and_player_commands.md",
           "reference/12_simulation_model/07_randomness_and_integrity.md",
           "reference/07_bots.md",
           "reference/01_skirmish_rules_and_modes/README.md",
           "architecture/components/platform_layer.md"]),
    201: ("Replication bandwidth and population budget",
          ["R-ECON", "R-SIM", "DBG"],
          ["reference/05_population_flow.md",
           "reference/12_simulation_model/03_entity_model_and_identity.md",
           "architecture/components/diagnostics_test_and_performance.md"]),
    202: ("Command validation and authority",
          ["R-SIM", "R-CTRL", "R-META", "DBG"],
          ["reference/12_simulation_model/06_teams_and_player_commands.md",
           "reference/06_controls_camera_and_commands.md",
           "reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md",
           "architecture/components/diagnostics_test_and_performance.md"]),
    203: ("Input timing, coalescing and latency",
          ["R-SIM", "R-CTRL", "INP"],
          ["reference/12_simulation_model/01_tick_timing_and_main_loop.md",
           "reference/06_controls_camera_and_commands.md",
           "architecture/components/input.md"]),
    204: ("Content and version identity",
          ["R-MAP", "AST", "BLD", "DAT"],
          ["reference/08_maps_and_level_design/rules_and_conventions.md",
           "reference/08_maps_and_level_design/map_editor_hooks.md",
           "architecture/components/asset_pipeline_and_resources.md",
           "architecture/components/build_tooling_and_distribution.md",
           "architecture/components/data_schemas.md"]),
    205: ("Join, reconnect, leave and session continuity",
          ["R-MODE", "R-META"],
          ["reference/01_skirmish_rules_and_modes/README.md",
           "reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md"]),
    206: ("Divergence detection and integrity checking",
          ["R-SIM", "DBG"],
          ["reference/12_simulation_model/07_randomness_and_integrity.md",
           "reference/12_simulation_model/02_match_lifecycle_and_state_reset.md",
           "architecture/components/diagnostics_test_and_performance.md"]),
    207: ("Transport, message format and robustness",
          ["PLT", "DAT", "R-MAP"],
          ["architecture/components/platform_layer.md",
           "architecture/components/data_schemas.md",
           "reference/08_maps_and_level_design/map_editor_hooks.md"]),
    208: ("Network diagnostics and test tooling",
          ["DBG"],
          ["architecture/components/diagnostics_test_and_performance.md"]),
}

# input index -> DEC ids it fed
MAP = {
    0: [200, 204, 205, 206], 1: [200, 202], 2: [200, 205], 3: [204], 4: [207],
    5: [200, 203, 205, 206, 207], 6: [203], 7: [208], 8: [207, 202], 9: [207],
    10: [201, 207], 11: [207, 202, 201], 12: [201, 207, 202], 13: [207], 14: [207, 202],
    15: [205, 206, 207], 16: [205], 17: [203], 18: [207, 203], 19: [207, 208],
    20: [205], 21: [207], 22: [207], 23: [207, 208], 24: [206], 25: [203], 26: [205],
    27: [206, 208], 28: [201], 29: [201, 208], 30: [208], 31: [208], 32: [207],
    33: [202, 204, 206, 207], 34: [205, 206, 207, 208], 35: [207], 36: [206],
    37: [201, 203], 38: [205], 39: [204], 40: [208], 41: [206], 42: [206], 43: [206],
    44: [206], 45: [206], 46: [206, 208], 47: [200], 48: [203], 49: [200, 203, 207],
    50: [205], 51: [205], 52: [205], 53: [205], 54: [206], 55: [204], 56: [207, 203],
    57: [200], 58: [200, 201], 59: [200], 60: [200], 61: [200, 201, 205], 62: [200, 201],
    63: [200, 203], 64: [200], 65: [200], 66: [200], 67: [202, 206], 68: [204],
    69: [204, 207, 205], 70: [200], 71: [206], 72: [200], 73: [200], 74: [200, 203],
    75: [206, 202, 204], 76: [200], 77: [200, 204], 78: [203, 202], 79: [200],
    80: [205, 207, 203, 206], 84: [201, 203], 85: [200, 207], 86: [200, 205],
    87: [203], 88: [202], 89: [206, 204], 90: [203], 91: [201], 92: [205], 93: [203],
    94: [207], 95: [206], 96: [202, 207, 205], 97: [204], 98: [203], 99: [203],
    100: [207, 203, 205], 101: [200], 102: [205, 206], 103: [204], 104: [202],
}

UNUSED = {
    81: "derived spatial-cache results: simulation determinism owned by reference/12, not a networking question",
    82: "single-threaded simulation and parallel-pragma hazard: threading model (process_tick_and_roadmap.md)",
    83: "blank line closing the table",
}


def main(argv):
    if len(argv) not in (3, 4):
        print(__doc__, file=sys.stderr)
        return 2
    with open(argv[1], encoding="utf-8") as fh:
        inputs = json.load(fh)["inputs"]
    n = len(inputs)
    seen = set(MAP) | set(UNUSED)
    if set(MAP) & set(UNUSED) or seen != set(range(n)):
        missing = sorted(set(range(n)) - seen)
        print(f"mapping incomplete or overlapping: missing {missing}", file=sys.stderr)
        return 1
    refs = {d: [] for d in DECS}
    for i, decs in MAP.items():
        r = inputs[i]
        ref = f"{r['file']}:{r['start_line']}-{r['end_line']}"
        for d in decs:
            if d not in DECS:
                print(f"unknown DEC {d} at input {i}", file=sys.stderr)
                return 1
            if ref not in refs[d]:
                refs[d].append(ref)

    def key(ref):
        f, rng = ref.rsplit(":", 1)
        return (f, int(rng.split("-")[0]))

    with open(argv[2], "w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["dec_id", "title", "status", "affects_prefix", "affects_files", "orig_refs"])
        for d in sorted(DECS):
            title, prefixes, files = DECS[d]
            w.writerow([f"DEC-{d}", title, "Open", ";".join(prefixes), ";".join(files),
                        ";".join(sorted(refs[d], key=key))])
    if len(argv) == 4:
        with open(argv[3], "w", encoding="utf-8", newline="\n") as fh:
            fh.write("| # | Range | Anchor | DECs fed |\n|---|---|---|---|\n")
            for i, r in enumerate(inputs):
                rng = f"{r['file']}:{r['start_line']}-{r['end_line']}"
                fed = ", ".join(f"DEC-{d}" for d in MAP[i]) if i in MAP else f"none: {UNUSED[i]}"
                fh.write(f"| {i} | `{rng}` | {r.get('anchor', '')[:48]} | {fed} |\n")
    print(f"{n} inputs; {len(MAP)} mapped, {len(UNUSED)} unused; {len(DECS)} DECs")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
