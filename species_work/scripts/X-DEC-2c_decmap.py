"""Write WORK/dec_map_X-DEC-2c.csv and check that every package input range is mapped.

Usage: python3 -I X-DEC-2c_decmap.py <package.json> <out.csv> <register.md>
"""
import csv
import json
import re
import sys
from pathlib import Path

F = "architecture/components/"
R12 = "reference/12_simulation_model/"
R09 = "reference/09_front_end_and_match_flow/"

ROWS = [
    ("DEC-240", "Simulation numeric model and determinism scope",
     ["R-SIM", "TER", "DAT", "BLD", "DBG", "PLT"],
     [R12 + "README.md", R12 + "01_tick_timing_and_main_loop.md", R12 + "07_randomness_and_integrity.md",
      F + "terrain.md", F + "data_schemas.md", F + "build_tooling_and_distribution.md",
      F + "diagnostics_test_and_performance.md", F + "platform_layer.md"],
     ["22_determinism_and_networking.md:1303-1393", "22_determinism_and_networking.md:1407-1536",
      "23_terrain_and_landscape.md:876-914", "31_platform_build_and_tools.md:921-933",
      "40_carry_forward_assessment.md:45-45", "40_carry_forward_assessment.md:119-119",
      "40_carry_forward_assessment.md:232-233", "40_carry_forward_assessment.md:346-346",
      "40_carry_forward_assessment.md:377-378", "40_carry_forward_assessment.md:453-453",
      "40_carry_forward_assessment.md:690-690", "40_carry_forward_assessment.md:696-697",
      "41_risks_and_open_questions.md:137-137", "41_risks_and_open_questions.md:434-434",
      "41_risks_and_open_questions.md:436-440", "41_risks_and_open_questions.md:456-456"]),
    ("DEC-241", "Numeric semantics that reference rules depend on",
     ["R-SIM", "R-UNIT", "R-BLDG", "DAT", "R-META"],
     [R12 + "03_entity_model_and_identity.md", R12 + "05_space_routing_and_physics.md",
      "reference/02_units/leader_and_formations.md", "reference/03_buildings_and_world_objects/",
      F + "data_schemas.md", R09 + "screen_flow_and_match_setup.md"],
     ["22_determinism_and_networking.md:244-266", "22_determinism_and_networking.md:1303-1393",
      "22_determinism_and_networking.md:1407-1536", "29_data_formats_reference.md:44-55"]),
    ("DEC-242", "Simulation random generator algorithm and stream derivation",
     ["R-SIM", "DBG", "TER"],
     [R12 + "07_randomness_and_integrity.md", F + "diagnostics_test_and_performance.md", F + "terrain.md"],
     ["40_carry_forward_assessment.md:119-119", "40_carry_forward_assessment.md:232-233",
      "40_carry_forward_assessment.md:377-378", "22_determinism_and_networking.md:1537-1551"]),
    ("DEC-243", "Lobby authority and the match-setup state",
     ["R-META", "R-MODE", "R-SIM", "UI"],
     [R09 + "screen_flow_and_match_setup.md", "reference/01_skirmish_rules_and_modes/README.md",
      R12 + "06_teams_and_player_commands.md", F + "ui_framework/pages_and_screen_recipes.md"],
     ["00_overview.md:292-292", "01_game_modes.md:661-661", "01_game_modes.md:663-665",
      "01_game_modes.md:1244-1244", "09_meta_systems.md:829-842", "09_meta_systems.md:852-870",
      "09_meta_systems.md:1031-1039", "22_determinism_and_networking.md:244-266",
      "22_determinism_and_networking.md:347-363", "22_determinism_and_networking.md:658-663",
      "40_carry_forward_assessment.md:49-49", "40_carry_forward_assessment.md:109-109",
      "40_carry_forward_assessment.md:489-490"]),
    ("DEC-244", "Session join, admission, reconnection and spectators",
     ["R-META", "R-SIM"],
     [R09 + "screen_flow_and_match_setup.md", R12 + "06_teams_and_player_commands.md"],
     ["09_meta_systems.md:589-602", "22_determinism_and_networking.md:610-611",
      "22_determinism_and_networking.md:612-657", "22_determinism_and_networking.md:664-679",
      "22_determinism_and_networking.md:680-693", "22_determinism_and_networking.md:753-762"]),
    ("DEC-245", "Session discovery, the advertised session record and the session browser",
     ["R-META", "UI"],
     [R09 + "screen_flow_and_match_setup.md", F + "ui_framework/pages_and_screen_recipes.md"],
     ["09_meta_systems.md:581-584", "09_meta_systems.md:585-588", "09_meta_systems.md:945-958",
      "09_meta_systems.md:984-1010", "09_meta_systems.md:1021-1030", "09_meta_systems.md:1068-1154",
      "22_determinism_and_networking.md:934-956", "22_determinism_and_networking.md:975-982",
      "22_determinism_and_networking.md:983-991", "40_carry_forward_assessment.md:178-178",
      "40_carry_forward_assessment.md:489-490"]),
    ("DEC-246", "Terrain runtime model and terrain content identity",
     ["TER", "R-MAP", "AST", "DAT", "R-SIM"],
     [F + "terrain.md", "reference/08_maps_and_level_design/rules_and_conventions.md",
      "reference/08_maps_and_level_design/map_editor_hooks.md", F + "asset_pipeline_and_resources.md",
      F + "data_schemas.md", R12 + "05_space_routing_and_physics.md"],
     ["22_determinism_and_networking.md:1537-1551", "23_terrain_and_landscape.md:876-914",
      "23_terrain_and_landscape.md:955-963", "40_carry_forward_assessment.md:50-50",
      "40_carry_forward_assessment.md:107-107", "40_carry_forward_assessment.md:692-692",
      "41_risks_and_open_questions.md:436-440", "41_risks_and_open_questions.md:449-453"]),
    ("DEC-247", "Terrain scale and large-world terrain inputs",
     ["TER", "R-MAP", "RND", "CAM", "R-SIM"],
     [F + "terrain.md", "reference/08_maps_and_level_design/rules_and_conventions.md",
      F + "renderer/world_renderers.md", F + "camera_and_sequences.md", R12 + "05_space_routing_and_physics.md"],
     ["23_terrain_and_landscape.md:964-1051"]),
    ("DEC-248", "Player identity and entitlement",
     ["PLT", "BLD", "R-META"],
     [F + "platform_layer.md", F + "build_tooling_and_distribution.md", R09 + "screen_flow_and_match_setup.md"],
     ["22_determinism_and_networking.md:680-693", "22_determinism_and_networking.md:934-956",
      "40_carry_forward_assessment.md:49-49", "40_carry_forward_assessment.md:133-134",
      "40_carry_forward_assessment.md:179-179", "40_carry_forward_assessment.md:244-245",
      "40_carry_forward_assessment.md:491-491", "40_carry_forward_assessment.md:698-698",
      "41_risks_and_open_questions.md:146-146", "41_risks_and_open_questions.md:424-429"]),
    ("DEC-249", "Directory, rendezvous, relay and matchmaking services",
     ["PLT", "R-META"],
     [F + "platform_layer.md", R09 + "screen_flow_and_match_setup.md"],
     ["09_meta_systems.md:945-958", "22_determinism_and_networking.md:612-657",
      "22_determinism_and_networking.md:983-991", "40_carry_forward_assessment.md:177-177",
      "40_carry_forward_assessment.md:244-245", "40_carry_forward_assessment.md:489-490",
      "40_carry_forward_assessment.md:689-689", "41_risks_and_open_questions.md:424-429"]),
    ("DEC-250", "Persisted data inventory and client-local storage",
     ["R-META", "R-SIM", "PLT", "DAT", "INP", "DBG"],
     [R09 + "options_localisation_and_hooks.md", R12 + "README.md", F + "platform_layer.md",
      F + "data_schemas.md", F + "input.md", F + "diagnostics_test_and_performance.md"],
     ["09_meta_systems.md:1068-1154", "22_determinism_and_networking.md:658-663",
      "22_determinism_and_networking.md:934-956", "22_determinism_and_networking.md:1537-1551",
      "40_carry_forward_assessment.md:109-109"]),
    ("DEC-251", "World-state persistence, schema migration and rollback",
     ["R-SIM", "DAT", "PLT", "DBG"],
     [R12 + "02_match_lifecycle_and_state_reset.md", F + "data_schemas.md", F + "platform_layer.md",
      F + "diagnostics_test_and_performance.md"],
     ["22_determinism_and_networking.md:244-266", "22_determinism_and_networking.md:753-762",
      "23_terrain_and_landscape.md:955-963", "40_carry_forward_assessment.md:377-378"]),
]


def main() -> int:
    pkg = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    out = Path(sys.argv[2])
    reg = Path(sys.argv[3]).read_text(encoding="utf-8")
    ok = True
    # every DEC exists in the register with the same title and status Open
    for dec, title, *_ in ROWS:
        m = re.search(r"^### " + re.escape(dec) + r" (.+)$", reg, re.M)
        if not m or m.group(1).strip() != title:
            print(f"title mismatch for {dec}: {m.group(1) if m else None!r}")
            ok = False
    found = set(re.findall(r"^### (DEC-\d{3}) ", reg, re.M))
    if found != {r[0] for r in ROWS}:
        print("register/map DEC set differs:", sorted(found ^ {r[0] for r in ROWS}))
        ok = False
    # coverage of package inputs
    mapped = {ref for r in ROWS for ref in r[4]}
    for row in pkg["inputs"]:
        key = f"{row['file']}:{row['start_line']}-{row['end_line']}"
        if key not in mapped:
            print("unmapped input:", key)
            ok = False
    with out.open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["dec_id", "title", "status", "affects_prefix", "affects_files", "orig_refs"])
        for dec, title, prefixes, files, refs in ROWS:
            w.writerow([dec, title, "Open", ";".join(prefixes), ";".join(files), ";".join(refs)])
    print(f"wrote {len(ROWS)} rows; coverage {'complete' if ok else 'INCOMPLETE'}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
