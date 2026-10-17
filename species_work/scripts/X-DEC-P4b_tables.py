"""Write the X-DEC-P4b dec map and triage table.
Usage: python3 -I X-DEC-P4b_tables.py <dec_map.csv> <triage.csv>"""
import csv, sys

R12 = 'reference/12_simulation_model/'
DEC_MAP = [
    ('DEC-260', 'Order within an actor-grid cell and container iteration order', 'Resolved', 'R-SIM;R-BLDG',
     R12 + '05_space_routing_and_physics.md;' + R12 + '04_containers_and_update_order.md;reference/03_buildings_and_world_objects/README.md',
     '21_simulation_and_entity_model.md:1003-1068;21_simulation_and_entity_model.md:1100-1113;21_simulation_and_entity_model.md:735-979'),
    ('DEC-261', 'Multi-round Siege rebuilds the match world at a tick boundary', 'Resolved', 'R-SIM;R-MODE',
     R12 + '02_match_lifecycle_and_state_reset.md;reference/01_skirmish_rules_and_modes/siege.md',
     '22_determinism_and_networking.md:1759-1762'),
    ('DEC-262', 'Generations of laser-bolt and Essence-orb handles, and the team selection', 'Resolved', 'R-SIM',
     R12 + '03_entity_model_and_identity.md;' + R12 + '06_teams_and_player_commands.md',
     '21_simulation_and_entity_model.md:590-663'),
    ('DEC-263', 'Route and actor-group edge cases', 'Open', 'R-SIM',
     R12 + '05_space_routing_and_physics.md;' + R12 + '06_teams_and_player_commands.md',
     '21_simulation_and_entity_model.md:1175-1194;21_simulation_and_entity_model.md:1311-1548'),
    ('DEC-264', 'What runs while game speed is 0', 'Open', 'R-SIM;R-MODE',
     R12 + '01_tick_timing_and_main_loop.md;reference/01_skirmish_rules_and_modes/README.md',
     '22_determinism_and_networking.md:132-141;20_architecture.md:1169-1396'),
    ('DEC-265', 'Real duration of the speed-crate holds', 'Open', 'R-SIM;R-CRATE',
     R12 + '01_tick_timing_and_main_loop.md;reference/04_supply_crates_powers_and_weapons/crates_and_powers.md',
     '22_determinism_and_networking.md:132-141;20_architecture.md:1169-1396'),
    ('DEC-266', 'Decorative objects in the simulated pools', 'Open', 'R-SIM;VFX',
     R12 + '07_randomness_and_integrity.md;' + R12 + '03_entity_model_and_identity.md;architecture/components/animation_and_vfx/README.md',
     '22_determinism_and_networking.md:1213-1296;22_determinism_and_networking.md:1621-1671'),
]

# source, request, outcome, dec_id, cite_in, document_fix
TRIAGE = [
    ('item 9; C-SIM-b 1', 'order within an actor-grid cell: DEC-016 (cell slot last key) vs DEC-650 (registration schedule has no effect)', 'new', 'DEC-260',
     '05_space_routing_and_physics.md R-SIM-307, R-SIM-310; 04_containers_and_update_order.md R-SIM-211, R-SIM-239 (table 2.9-A port occupant row); reference/03 README (port occupant rule)',
     'R-SIM-307 last sentence and R-SIM-310: the within-cell key is ascending actor serial (R-SIM-137), not cell slot order (DEC-260); table 2.9-A port-occupant row: "... then team 0 to 5, then actor serial"; reference/03 README port-occupant rule the same; DEC-016 and DEC-650 wording via hand-off H1 and the DEC-650 amendment'),
    ('item 16; C-SIM-a 4', 'DEC-016 "containers iterate in stable-id order" vs DEC-455 slot order with last-freed-first reuse', 'merged', 'DEC-260',
     '04_containers_and_update_order.md R-SIM-211',
     'R-SIM-211 cites DEC-260 next to DEC-016; DEC-016 text via hand-off H1; DEC-455 option B wording via hand-off H6'),
    ('item 15; C-SIM-a 1', 'multi-round Siege rebuild at a tick boundary (R-SIM-030); DEC-066 records only the participant-dependent point', 'new', 'DEC-261',
     '02_match_lifecycle_and_state_reset.md R-SIM-030; reference/01 siege.md (multi-round rule)',
     'R-SIM-030 cites DEC-261 (keep DEC-066 for selectability and the serial-counter reset); DEC-066 text via hand-off H3'),
    ('item 15; C-SIM-a 2', 'per-slot generation for laser-bolt and Essence-orb handles (R-SIM-127)', 'new', 'DEC-262',
     '03_entity_model_and_identity.md R-SIM-127', 'R-SIM-127 cites DEC-262 next to DEC-028'),
    ('item 15; C-SIM-a 3', 'team selection as a generational handle (R-SIM-142)', 'merged', 'DEC-262',
     '03_entity_model_and_identity.md R-SIM-142; 06_teams_and_player_commands.md (team selection fields, R-SIM-439)',
     'R-SIM-142 cites DEC-262; 06 states that the selected actor is a generational handle (C-SIM-a cross-document request)'),
    ('item 15; C-SIM-a 5', 'placement of the actor-group offset table step (DEC-652 "a fixed step")', 'amend', 'DEC-652',
     '02_match_lifecycle_and_state_reset.md table 2.4-A step 11', 'none: DEC-652 now names the last step of match initialisation'),
    ('item 14; C-SIM-a DEC-011', 'stale Source Node id has no effect; DEC-011 wording', 'amend', 'DEC-011',
     '02_match_lifecycle_and_state_reset.md R-SIM-024', 'DEC-011 lives in verdicts_and_fixes.md (X-DEC-P4a): replacement text in hand-off H2'),
    ('item 10; C-SIM-b 2', 'BuildRoute when no corridor test runs (R-SIM-351, AC-SIM-05-09)', 'new', 'DEC-263',
     '05_space_routing_and_physics.md R-SIM-351', 'R-SIM-351 cites DEC-263 (Open DEC must be cited by a requirement)'),
    ('item 10; C-SIM-b 3', 'actor group with no reporting member in slice 0 (R-SIM-427): add to DEC-653', 'amend', 'DEC-653',
     '06_teams_and_player_commands.md R-SIM-427', 'R-SIM-427 cites DEC-653 item 10'),
    ('item 10; C-SIM-b 4', 'ActorGroup::Attack with no candidate (R-SIM-428)', 'merged', 'DEC-263',
     '06_teams_and_player_commands.md R-SIM-428', 'R-SIM-428 cites DEC-263'),
    ('item 10; C-SIM-b 5', 'obstacle push on a structure base point ends through DEC-241 item 6 (R-SIM-382)', 'amend', 'DEC-241',
     '05_space_routing_and_physics.md R-SIM-382', 'none: R-SIM-382 already cites DEC-241 item 6 and DEC-014; DEC-241 item 6 now records the dependency'),
    ('item 10; C-SIM-b 6', 'co-op setup colour command issued from the setup path (R-SIM-420)', 'merged', 'DEC-119',
     '06_teams_and_player_commands.md R-SIM-420; reference/01 README R-MODE-020', 'none: R-SIM-420 and R-MODE-020 already state that setup issues no command (DEC-119)'),
    ('item 11; C-SIM-c 1', 'game-speed tick scheduling at speed 0 (R-SIM-548, R-SIM-549)', 'new', 'DEC-264',
     '01_tick_timing_and_main_loop.md R-SIM-549', 'R-SIM-549 cites DEC-264 next to DEC-225'),
    ('item 11; C-SIM-c 2', 'real duration of the speed-crate holds under DEC-092 (R-SIM-557)', 'new', 'DEC-265',
     '01_tick_timing_and_main_loop.md R-SIM-557; reference/04 crates_and_powers.md (Time Dilation and Time Rush rules)', 'R-SIM-557 and the reference/04 speed-outcome rules cite DEC-265'),
    ('item 11; C-SIM-c 3', 'presentation frame delta while paused (R-SIM-571) vs one presentation clock', 'new', 'DEC-955',
     '01_tick_timing_and_main_loop.md R-SIM-573; camera_and_sequences.md CAM-030', 'R-SIM-573 cites DEC-955; R-SIM-571 stays the reference behaviour'),
    ('item 11; C-SIM-c 4', 'decorative objects in simulated pools take serials', 'new', 'DEC-266',
     '07_randomness_and_integrity.md (Table 2 row and the cosmetic-draw rule); 03_entity_model_and_identity.md R-SIM-138', 'one requirement of 07 (the Table 2 classification rule) cites DEC-266; DEC-222 item 7 cross-reference added'),
    ('item 11; C-SIM-c 5', 'Essence orb hover draws kept as simulation draws', 'merged', 'DEC-266',
     '07_randomness_and_integrity.md Table 1', 'none: recorded in DEC-266 and DEC-222 item 7'),
    ('item 11, 12; C-SIM-c 6', 'DEC-116 (crate drops continue in the grace period) vs the start hold (no world tick runs while held)', 'amend', 'DEC-116',
     'reference/04 crates_and_powers.md R-CRATE-102; reference/01 README R-MODE-044; 00_reference_overview.md R-VIS-052',
     'DEC-116 and DEC-036 live in verdicts_and_fixes.md (X-DEC-P4a): replacement text in hand-off H4; R-MODE-044 and R-VIS-052: replace "Supply crate drops are not suspended during the grace period" by the R-CRATE-102 statement (not gated by the grace period, but no world tick runs while the world is held, so the first drop roll follows the grace countdown)'),
    ('item 13', 'mode-second constant: 01 R-SIM-503 names it without a key', 'none', '',
     '', 'R-SIM-503 cites `ref.mode.mode_second` (defined by reference/01 R-MODE-002); X-ROUTE-b'),
    ('item 17', 'order of initialisation draws (R-SIM-612 vs 02 table 2.4-A); readers of the Control Zone counter (R-SIM-730 vs R-SIM-019)', 'none', '',
     '', 'cross-document fix, no DEC: 07 R-SIM-612 and R-SIM-730 align with 02 (C-SIM-a cross-document requests); routed by X-ROUTE-b'),
]

with open(sys.argv[1], 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs'])
    w.writerows(DEC_MAP)
with open(sys.argv[2], 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['source', 'request', 'outcome', 'dec_id', 'cite_in', 'document_fix'])
    w.writerows(TRIAGE)
print(len(DEC_MAP), 'dec rows;', len(TRIAGE), 'triage rows')
