#!/usr/bin/env python3
"""Write WORK/dec_map_X-DEC-2b.csv for the deferred process, tick and roadmap decisions.

Usage: python3 -I X-DEC-2b_decmap.py <out.csv> <dec_file.md>
The DEC file is read only to check that every mapped DEC id has a heading there.
"""
import csv
import re
import sys

A = '20_architecture.md'
E = '21_simulation_and_entity_model.md'
N = '22_determinism_and_networking.md'
D = '32_debug_test_and_performance.md'
C = '40_carry_forward_assessment.md'
R = '41_risks_and_open_questions.md'

RC = 'reference/12_simulation_model/'
AC = 'architecture/components/'

ROWS = [
    ('DEC-220', 'Process roles and the headless server build',
     ['PLT', 'BLD', 'DBG', 'R-SIM'],
     [AC + 'platform_layer.md', AC + 'build_tooling_and_distribution.md',
      AC + 'diagnostics_test_and_performance.md', RC + '01_tick_timing_and_main_loop.md'],
     [(A, 28, 49), (A, 118, 131), (A, 547, 703), (A, 983, 1015), (C, 390, 390)]),
    ('DEC-221', 'Composition root, service lifetimes and the application state machine',
     ['PLT', 'UI', 'AEN', 'R-META', 'R-SIM'],
     [AC + 'platform_layer.md', AC + 'ui_framework/README.md', AC + 'audio_engine.md',
      'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md',
      RC + '02_match_lifecycle_and_state_reset.md'],
     [(A, 28, 49), (A, 50, 51), (A, 52, 117), (A, 118, 131), (A, 547, 703), (A, 704, 707),
      (A, 708, 754), (A, 755, 767), (A, 811, 814), (A, 815, 896), (A, 897, 898), (A, 899, 915),
      (A, 916, 921), (A, 959, 982), (A, 1027, 1039), (A, 1683, 1700), (C, 117, 117)]),
    ('DEC-222', 'Simulation and presentation boundary',
     ['R-SIM', 'RND', 'AEN', 'UI', 'CAM', 'BLD'],
     [RC + '07_randomness_and_integrity.md', RC + '01_tick_timing_and_main_loop.md',
      AC + 'renderer/README.md', AC + 'audio_engine.md', AC + 'ui_framework/README.md',
      AC + 'camera_and_sequences.md', AC + 'build_tooling_and_distribution.md',
      'decisions/deferred/networking.md'],
     [(A, 52, 117), (A, 118, 131), (A, 1397, 1567), (A, 1683, 1700), (E, 1644, 1661),
      (C, 46, 46), (C, 379, 380)]),
    ('DEC-223', 'World, region and match state ownership and reset',
     ['R-SIM', 'DAT'],
     [RC + '02_match_lifecycle_and_state_reset.md', RC + '03_entity_model_and_identity.md',
      RC + '07_randomness_and_integrity.md', AC + 'data_schemas/README.md',
      'decisions/deferred/numerics_lobby_terrain_services.md'],
     [(A, 547, 703), (A, 708, 754), (A, 755, 767), (A, 983, 1015), (A, 1683, 1700),
      (E, 1644, 1661), (C, 46, 46), (C, 117, 117), (C, 379, 380), (R, 403, 406)]),
    ('DEC-224', 'Threading and job model',
     ['R-SIM', 'PLT', 'RND', 'AEN', 'AST', 'DBG'],
     [RC + '04_containers_and_update_order.md', RC + '07_randomness_and_integrity.md',
      AC + 'platform_layer.md', AC + 'renderer/README.md', AC + 'audio_engine.md',
      AC + 'asset_pipeline_and_resources.md', AC + 'diagnostics_test_and_performance.md',
      'decisions/deferred/numerics_lobby_terrain_services.md'],
     [(A, 28, 49), (A, 118, 131), (A, 708, 754), (A, 755, 767), (A, 1397, 1567),
      (A, 1683, 1700), (E, 1644, 1661), (N, 992, 1010), (D, 879, 890), (C, 248, 249),
      (C, 255, 255), (C, 468, 468), (R, 398, 401), (R, 403, 406)]),
    ('DEC-225', 'Frame loop, simulation pacing and catch-up',
     ['R-SIM', 'R-META', 'PLT', 'DBG'],
     [RC + '01_tick_timing_and_main_loop.md',
      'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md',
      AC + 'platform_layer.md', AC + 'diagnostics_test_and_performance.md'],
     [(A, 28, 49), (A, 708, 754), (A, 983, 1015), (A, 1056, 1059), (A, 1060, 1074),
      (A, 1075, 1090), (A, 1151, 1164), (A, 1683, 1700), (D, 879, 890), (C, 248, 249),
      (R, 601, 601)]),
    ('DEC-226', 'Loading, streaming and asynchronous work',
     ['AST', 'RND', 'AEN', 'TER', 'R-META'],
     [AC + 'asset_pipeline_and_resources.md', AC + 'renderer/README.md', AC + 'audio_engine.md',
      AC + 'terrain.md', 'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md',
      'decisions/deferred/numerics_lobby_terrain_services.md'],
     [(A, 815, 896), (A, 983, 1015), (A, 1151, 1164), (A, 1397, 1567), (A, 1683, 1700),
      (N, 992, 1010), (C, 248, 249), (R, 403, 406)]),
    ('DEC-227', 'Simulation tick rate',
     ['R-SIM', 'R-MODE', 'R-UNIT', 'R-BLDG', 'R-CRATE', 'R-ECON', 'R-CTRL', 'R-BOT', 'R-AUD', 'R-TUT'],
     [RC + '01_tick_timing_and_main_loop.md', RC + '04_containers_and_update_order.md',
      'decisions/deferred/networking.md'],
     [(A, 28, 49), (A, 1075, 1090), (A, 1683, 1700), (D, 879, 890), (C, 248, 249)]),
    ('DEC-228', 'Per-tick CPU budget, population caps and stress populations',
     ['DBG', 'RND', 'R-ECON', 'R-SIM', 'R-MODE'],
     [AC + 'diagnostics_test_and_performance.md', AC + 'renderer/options_and_budgets.md',
      'reference/05_population_flow.md', RC + '05_space_routing_and_physics.md',
      'reference/01_skirmish_rules_and_modes/README.md'],
     [(E, 1644, 1661), (D, 879, 890), (C, 616, 675)]),
    ('DEC-229', 'Build order, milestones and work breakdown',
     ['R-SIM', 'R-MODE', 'R-ECON', 'R-BOT', 'RND', 'AEN', 'UI', 'INP', 'BLD', 'DBG', 'TER', 'DAT'],
     ['assets/README.md', 'reference/05_population_flow.md',
      'reference/01_skirmish_rules_and_modes/spawn_control.md', AC + 'diagnostics_test_and_performance.md',
      AC + 'build_tooling_and_distribution.md', 'decisions/deferred/networking.md'],
     [(C, 117, 117), (C, 616, 675)]),
]


def main():
    out, dec_file = sys.argv[1], sys.argv[2]
    text = open(dec_file, encoding='utf-8').read()
    heads = set(re.findall(r'^### (DEC-\d{3}) ', text, re.M))
    ids = [r[0] for r in ROWS]
    missing = [i for i in ids if i not in heads]
    extra = sorted(heads - set(ids))
    if missing or extra:
        sys.exit('mismatch: missing %s extra %s' % (missing, extra))
    with open(out, 'w', encoding='utf-8', newline='') as fh:
        w = csv.writer(fh, lineterminator='\n')
        w.writerow(['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs'])
        for dec, title, prefixes, files, refs in ROWS:
            w.writerow([dec, title, 'Open', ';'.join(prefixes), ';'.join(files),
                        ';'.join('%s:%d-%d' % r for r in refs)])
    print('wrote %d rows to %s' % (len(ROWS), out))


if __name__ == '__main__':
    main()
