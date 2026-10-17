"""Write dec_map_X-DEC-P4a.csv and p4_dec_triage_X-DEC-P4a.csv.
Usage: python3 -I X-DEC-P4a_tables.py <species_work_dir>"""
import csv, os, sys
W = sys.argv[1]
R01 = 'reference/01_skirmish_rules_and_modes/'
R02 = 'reference/02_units/'
R03 = 'reference/03_buildings_and_world_objects/'
R04 = 'reference/04_supply_crates_powers_and_weapons/'
R05 = 'reference/05_population_flow.md'
R06 = 'reference/06_controls_camera_and_commands/'
R07 = 'reference/07_bots/'
R08 = 'reference/08_maps_and_level_design/rules_and_conventions.md'
R09 = 'reference/09_front_end_and_match_flow/'
R11 = 'reference/11_tutorial_and_help.md'
SRC = '(Phase 4 triage of the Phase 3 reports {}; see species_work/p4_dec_triage_X-DEC-P4a.csv)'
DECMAP = [
 ('DEC-122', 'Bot and deputy actions outside the player command catalogue', 'Open', 'R-BOT;R-SIM;R-UNIT;R-BLDG',
  [R07 + 'README.md', 'reference/12_simulation_model/06_teams_and_player_commands.md', R02 + 'worker.md', R02 + 'leader_and_formations.md', R02 + 'transport.md', R02 + 'strike_team.md', R02 + 'collector.md', R02 + 'builder.md', R03 + 'mode_objects.md'], 'G07 request 1, item 50'),
 ('DEC-123', 'Tutorial for the first playable', 'Open', 'R-TUT;R-MODE;R-MAP;R-VIS;R-BOT',
  [R11, R01 + 'README.md', R08, 'reference/00_reference_overview.md', R07 + '05_mode_strategies_and_tutorials.md', 'assets/GameAssets.md'], 'G11 request 1, item 72'),
 ('DEC-850', 'Match-rule gap rules', 'Open', 'R-MODE', [R01 + 'relic_run.md', R01 + 'flag_chain.md'], 'G01 requests 1 and 2'),
 ('DEC-851', 'Population-flow rule details', 'Open', 'R-ECON', [R05], 'G05 requests 8 and 10'),
 ('DEC-852', 'Game-speed divisors in Worker rules', 'Open', 'R-UNIT', [R02 + 'README.md', R02 + 'worker.md'], 'G02a1 request 1, item 31'),
 ('DEC-853', 'Worker and Trooper rule details', 'Open', 'R-UNIT', [R02 + 'worker.md', R02 + 'README.md', R02 + 'strike_team.md'], 'G02a1 requests 2 to 4, G02a2 request 3, item 31'),
 ('DEC-854', 'Transport, Collector and Strike team rule details', 'Open', 'R-UNIT', [R02 + 'transport.md', R02 + 'collector.md', R02 + 'strike_team.md'], 'G02a2 requests 4, 6, 7, 9 and 10, item 27'),
 ('DEC-855', 'Creature and hazard rule quirks', 'Open', 'R-UNIT', [R02 + 'creatures.md', R02 + 'hazards_and_projectiles.md'], 'G02b requests 1 to 5 and 7, item 35'),
 ('DEC-856', 'Creature presentation quirks', 'Open', 'R-UNIT', [R02 + 'hazards_and_projectiles.md', R02 + 'creatures.md'], 'G02b request 6, item 35'),
 ('DEC-857', 'Attempt caps of the bounded searches', 'Resolved', 'R-UNIT;R-BOT', [R02 + 'hazards_and_projectiles.md', R07 + '04_powers_and_unit_policies.md'], 'G02b interpretations (attempt caps), G07 request 5, item 34'),
 ('DEC-858', 'Empty outcome lists', 'Open', 'R-CRATE;R-ECON', [R04 + 'README.md', R04 + 'crates_and_powers.md', R05], 'G04 request 1, G05 request 5, item 5'),
 ('DEC-859', 'Tick counts chosen for reference durations', 'Open', 'R-CRATE;R-BLDG;R-MODE', [R04 + 'crates_and_powers.md', R04 + 'weapons_and_projectiles.md', R03 + 'defences_and_props.md', R01 + 'siege.md'], 'G04 requests 4, 5 and 7, G03b request 3, G01 request 5'),
 ('DEC-860', 'Structure rule details', 'Open', 'R-BLDG', [R03 + 'mode_objects.md', R03 + 'economy_and_spawn_objects.md'], 'G03a requests 1, 3, 4 and 5, item 43'),
 ('DEC-861', 'Bot rule quirks', 'Open', 'R-BOT', [R07 + '05_mode_strategies_and_tutorials.md', R07 + '01_navigation_graph.md'], 'G07 requests 6 to 8'),
 ('DEC-862', 'Bot rule details', 'Open', 'R-BOT', [R07 + 'README.md', R07 + '02_priority_model.md', R07 + '03_decision_loop_and_leaders.md', R07 + '04_powers_and_unit_policies.md'], 'G07 requests 2 to 4, 9 and 10, item 51'),
 ('DEC-863', 'Control details of the corrected command set', 'Resolved', 'R-CTRL', [R06 + 'actions_and_bindings.md', R06 + 'selection_and_power_slots.md'], 'G06 requests 1, 2, 3 and 5, item 58'),
 ('DEC-864', 'Pointer fallback and camera smoothing', 'Open', 'R-CTRL', [R06 + 'actions_and_bindings.md', R06 + 'camera.md'], 'G06 requests 4 and 6, item 58'),
 ('DEC-865', 'Map data without a reference case', 'Open', 'R-MAP', [R08], 'G08a requests 1, 2, 3, 6, 7 and 9, item 63'),
 ('DEC-866', 'Population-cap values of a map', 'Open', 'R-MAP', [R08], 'G08a requests 5 and 8, item 63'),
 ('DEC-867', 'Closed player slots', 'Open', 'R-META;R-MAP', [R09 + 'screen_flow_and_match_setup.md', R08], 'G09a request 1, G08a request 4, items 63 and 69'),
 ('DEC-868', 'Front-end reference quirks', 'Open', 'R-META;R-MAP', [R09 + 'screen_flow_and_match_setup.md', R09 + 'front_end_presentation.md', R08], 'G09a requests 2 to 4, item 69'),
 ('DEC-869', 'Reference sound options without a Species counterpart', 'Open', 'R-META', [R09 + 'options_localisation_and_hooks.md'], 'G09b request 1, item 71'),
 ('DEC-870', 'Tutorial director details', 'Open', 'R-TUT', [R11], 'G11 request 4'),
 ('DEC-871', 'Essence orb break operation', 'Dropped', 'R-UNIT;VFX', [R02 + 'essence_orbs.md', 'architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md'], 'G02a2 request 5'),
]
with open(os.path.join(W, 'dec_map_X-DEC-P4a.csv'), 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs'])
    for d, t, s, p, files, src in DECMAP:
        w.writerow([d, t, s, p, ';'.join(files), SRC.format(src)])

T = []
def t(source, request, outcome, dec, cite='', fix=''):
    T.append([source, request, outcome, dec, cite, fix])
ARB_RT = 'see arbitration A-8 (item 48, sub-point 2)'
# ---- G00
t('G00', 'no DEC requests', 'none', '', '', '')
# ---- G01
t('G01-1', 'Relic spawn window arithmetic: exact or integer division', 'new', 'DEC-850', 'relic_run.md R-MODE-306', 'R-MODE-306: state that 60 × S ÷ Z and window ÷ 2 are exact real-valued quotients; cite DEC-850')
t('G01-2', 'Flag Chain Home post under-attack status: trigger and rank under DEC-078', 'new', 'DEC-850', 'flag_chain.md R-MODE-462', 'R-MODE-462: cite DEC-850 next to DEC-078')
t('G01-3', 'DEC-082 in Relic Run and Launch Race: add the files to Affects', 'amend', 'DEC-082', 'relic_run.md R-MODE-307', 'Affects now names the whole reference/01 folder')
t('G01-4', 'repeated result declaration adds the kill total again (DEC-047)', 'none', 'DEC-047', 'README R-MODE-057', '')
t('G01-5', 'multi-round Siege tick conversions (fade-out 10, next-round display 110 ticks)', 'new', 'DEC-859', 'siege.md R-MODE-362, R-MODE-372', 'siege.md tuning rows ref.mode.siege.fade_out_ticks and ref.mode.siege.next_round_display: add DEC-859 to the DEC column; R-MODE-362 cites DEC-859')
t('G01-6', 'DEC-017 says 39 option definitions; 38 remain', 'amend', 'DEC-017', '', 'reference/00_reference_overview.md: the mirror line of the option count says 38')
# ---- G02a1
t('G02a1-1', 'game-speed divisors in Worker rules under tick scheduling', 'new', 'DEC-852', 'README R-UNIT-007; worker.md R-UNIT-079, R-UNIT-082, R-UNIT-103', 'cite DEC-852 next to DEC-653 in each; README §8 lists DEC-852')
t('G02a1-2', 'fire-damage kill over-count', 'new', 'DEC-853', 'worker.md R-UNIT-119', 'R-UNIT-119: cite DEC-853 next to DEC-006')
t('G02a1-3', 'Trooper per-actor rate under floating-point stats', 'new', 'DEC-853', 'README R-UNIT-014', 'R-UNIT-014: cite DEC-853')
t('G02a1-4', 'range of the Worker Hive and Spitter Plant check', 'new', 'DEC-853', 'worker.md R-UNIT-098', 'R-UNIT-098 step 5: cite DEC-853')
# ---- G02a2
t('G02a2-1', 'DEC-085 contradicts builder.md: the Builder delivers to friendly Spawn Nodes', 'amend', 'DEC-085', 'builder.md R-UNIT-447, R-UNIT-457; reference/05 R-ECON-082', 'apply arbitration A-3: R-ECON-082 and the other reference/05 rows it lists, the reference/00 glossary row for the Builder, the builder.md §8 title "Builder Essence-orb delivery to friendly Spawn Nodes"; DEC-085 is now Resolved with the delivery rule')
t('G02a2-2', 'DEC-080 status mismatch (treated as Open, no cap on delivery)', 'amend', 'DEC-080', 'collector.md R-UNIT-425', 'DEC-080 is right (Resolved, owner decision O15). collector.md R-UNIT-425 and its §8 row, reference/05 R-ECON-074, its population-event row "Collector delivery" and its §8 line, and builder.md R-UNIT-457 ("As for the Collector"): a Collector delivery is checked against the receiving team\'s population cap, and an orb that would exceed it is not converted (DEC-080, DEC-453)')
t('G02a2-3', 'Trooper reload under floating-point stats (new Open)', 'merged', 'DEC-853', 'strike_team.md R-UNIT-341', 'merged with G02a1-3 into DEC-853 item 3; R-UNIT-341: cite DEC-853')
t('G02a2-4', 'Transport braking below zero', 'new', 'DEC-854', 'transport.md R-UNIT-379', 'R-UNIT-379: cite DEC-854')
t('G02a2-5', 'Essence orb broken marker never set (Dropped candidate)', 'new', 'DEC-871', 'essence_orbs.md R-UNIT-308', 'R-UNIT-308: cite DEC-871 (Dropped)')
t('G02a2-6', 'Collector suck speed near the collector point (clarification)', 'new', 'DEC-854', 'collector.md R-UNIT-431', 'R-UNIT-431: cite DEC-854')
t('G02a2-7', 'Collector status text while Harvesting and delivering', 'new', 'DEC-854', 'collector.md R-UNIT-445', 'R-UNIT-445: cite DEC-854 next to DEC-006')
t('G02a2-8', 'DEC-453 misses the Builder deliveries', 'amend', 'DEC-453', 'builder.md R-UNIT-457', '')
t('G02a2-9', 'Flag Chain bot turret search radius 1.2 × Flag post size (clarification)', 'new', 'DEC-854', 'transport.md R-UNIT-406', 'G01 or G03a confirm the default Flag post size that gives ref.unit.transport.bot_turret_search_default (200 wu); R-UNIT-406: cite DEC-854')
t('G02a2-10', 'Strike team direct-control vectors (clarification)', 'new', 'DEC-854', 'strike_team.md R-UNIT-358', 'reference/06 (G06) and input.md (C-INP) state the scaling of direct_move_vector and direct_target_vector consistently with R-UNIT-358; R-UNIT-358: cite DEC-854')
# ---- G02b
t('G02b-1', 'Crawler bite cadence (20 per tick)', 'new', 'DEC-855', 'creatures.md R-UNIT-532', 'R-UNIT-532: cite DEC-855')
t('G02b-2', 'spider avoidance neighbour draw', 'new', 'DEC-855', 'creatures.md R-UNIT-576', 'R-UNIT-576: cite DEC-855')
t('G02b-3', 'Abductor double time step', 'new', 'DEC-855', 'hazards_and_projectiles.md R-UNIT-705', 'R-UNIT-705: cite DEC-855 next to DEC-006')
t('G02b-4', 'barrage missile self-kill credit', 'new', 'DEC-855', 'hazards_and_projectiles.md R-UNIT-726', 'R-UNIT-726: cite DEC-855')
t('G02b-5', 'barrage missile ignores its own death', 'new', 'DEC-855', 'hazards_and_projectiles.md R-UNIT-723', 'R-UNIT-723: cite DEC-855')
t('G02b-6', 'creature presentation quirks (four)', 'new', 'DEC-856', 'hazards_and_projectiles.md R-UNIT-718; creatures.md R-UNIT-528, R-UNIT-547, R-UNIT-593', 'cite DEC-856 in each')
t('G02b-7', 'Reaper Serpent fallback roam range 100 or 200 wu', 'new', 'DEC-855', 'creatures.md R-UNIT-520', 'R-UNIT-520: cite DEC-855')
t('G02b-interp-caps', 'attempt caps 256 and 2000 of DEC-014 (Species values)', 'new', 'DEC-857', 'hazards_and_projectiles.md R-UNIT-702, R-UNIT-706, R-UNIT-722', 'cite DEC-857 next to DEC-014; tuning rows of the three keys: add DEC-857')
t('G02b-interp-durations', 'fixed durations by the binary64 countdown conversion', 'none', 'DEC-241', 'README R-UNIT-005', '')
t('G02b-interp-other', 'Abductor speed law, centipede trailing blend, directions not in the input, Wisp alpha (readings)', 'none', '', 'hazards_and_projectiles.md R-UNIT-705; creatures.md R-UNIT-552, R-UNIT-597', 'readings of the input for the fidelity review; no DEC')
t('G02b-xdoc-affects', 'DEC Affects fields name creatures_and_hazards.md', 'amend', 'DEC-014;DEC-019;DEC-084;DEC-090;DEC-091;DEC-092;DEC-105;DEC-107;DEC-113;DEC-121;DEC-311;DEC-453;DEC-460', '', 'Affects repointed to creatures.md or hazards_and_projectiles.md; dec_map.csv affects_files: coordinator')
t('G02b-xdoc-303-111', 'DEC-303 and DEC-111 add hazards_and_projectiles.md', 'amend', 'DEC-303;DEC-111', 'hazards_and_projectiles.md R-UNIT-728', '')
t('G02b-xdoc-092', 'DEC-092 says the gravity of Crawler eggs; it is the Seed Pod', 'amend', 'DEC-092', '', 'reference/12/05 R-SIM-363: the per-tick gravity belongs to the Seed Pod, not the Crawler egg (C-SIM-b owner)')
# ---- G03a
t('G03a-1', 'tie in the carryable ownership recount', 'new', 'DEC-860', 'mode_objects.md R-BLDG-275', 'R-BLDG-275: cite DEC-860')
t('G03a-2', 'Beam Dish receiver tie (first of equal distances)', 'none', 'DEC-016', 'economy_and_spawn_objects.md R-BLDG-157', '')
t('G03a-3', 'Launch Vessel damage floor', 'new', 'DEC-860', 'mode_objects.md R-BLDG-324', 'R-BLDG-324: cite DEC-860')
t('G03a-4', 'teleport exit waypoint jitter axes', 'new', 'DEC-860', 'economy_and_spawn_objects.md R-BLDG-149', 'R-BLDG-149: state that the single jitter draw offsets the waypoint across the exit direction, in the horizontal plane; cite DEC-860')
t('G03a-5', 'Launch Vessel attacker picks may repeat', 'new', 'DEC-860', 'mode_objects.md R-BLDG-325', 'R-BLDG-325: cite DEC-860')
t('G03a-6', 'cloth flag wobble phase constant', 'none', '', 'mode_objects.md R-BLDG-272', 'R-BLDG-272: cite VFX-139 for the per-column phases vfx.cloth.wobble_phases (1.5, 1.1, 1.3), which C-VFX defines')
t('G03a-7', 'DEC-459 (Open) against DEC-561 (Resolved)', 'amend', 'DEC-459', 'mode_objects.md R-BLDG-311, R-BLDG-317', 'DEC-459 is now Resolved through DEC-561; R-BLDG-311: state that the map validation of DEC-561 keeps a Core Device with no linked Command Console out of every match (see DEC-459)')
# ---- G03b
t('G03b-1', 'DEC-311: turret searches skip the bot controller, the Hive any-enemy search does not', 'amend', 'DEC-311', 'defences_and_props.md R-BLDG-411; trees_and_crate_structures.md R-BLDG-635', '')
t('G03b-2', 'DEC-088: elaboration of the corrected turret line of sight', 'none', 'DEC-088', 'defences_and_props.md R-BLDG-413', '')
t('G03b-3a', 'DEC-064 scope: add the turret ownership and target timers (defences_and_props.md)', 'none', 'DEC-064', 'defences_and_props.md R-BLDG-401, R-BLDG-408, R-BLDG-411', 'see arbitration A-12: DEC-064 covers only the recounts re-armed from the match clock, and its text now says so. The turret timers are 11 ticks either way (a 1.0 s countdown also gives 11 ticks, DEC-002); R-BLDG-401 follows the narrowed R-BLDG-003 of A-12 and cites DEC-064 only if the turret timers are re-armed from the match clock')
t('G03b-3b', 'turret hosing window keyed as 10 ticks', 'new', 'DEC-859', 'defences_and_props.md R-BLDG-415', 'tuning row ref.bldg.turret.hose_window_ticks: add DEC-859 to the DEC column')
t('G03b-4', 'tree Essence-orb team: one draw U{0..3} equals the reference re-draw', 'none', 'DEC-013;DEC-014', 'trees_and_crate_structures.md', 'reference/12/07 Table 1 (C-SIM-c owner): describe the single U{0..3} draw of the tree Essence-orb team, not the repeated draw')
t('G03b-readings', 'five readings for a reviewer (Barrier Emitter energy control, turret idle sweep, flame-turret guard, Hive search kinds, offspring height)', 'none', '', 'defences_and_props.md R-BLDG-412, R-BLDG-417, R-BLDG-439; trees_and_crate_structures.md', 'readings of the input for the fidelity review; no DEC')
# ---- G04
t('G04-1', 'empty candidate lists after exclusions', 'new', 'DEC-858', 'README R-CRATE-003; crates_and_powers.md R-CRATE-121, R-CRATE-124, R-CRATE-126', 'cite DEC-858 in each')
t('G04-2', 'DEC-465 against DEC-653 item 7 (zero-score weighted drop)', 'amend', 'DEC-465;DEC-653', 'crates_and_powers.md R-CRATE-106', 'DEC-465 now states the p test; DEC-653 item 7: hand-off H-1 to X-DEC-P4b')
t('G04-3', 'DEC-550 airstrike wording', 'amend', 'DEC-550', 'README R-CRATE-017, R-CRATE-021', 'DEC-550 also drops the Sickness from its one-shot list, which is never a Power (README table 2.2-A)')
t('G04-4', 'opening progress in integer units (note for DEC-002)', 'new', 'DEC-859', 'crates_and_powers.md R-CRATE-115', 'R-CRATE-115 and its tuning rows: cite DEC-859 next to DEC-002')
t('G04-5', 'neutral crate life 600 ticks (confirm)', 'new', 'DEC-859', 'crates_and_powers.md R-CRATE-119', 'arbitration A-10 settles the Strike team life (601 ticks, a single-precision counter) but not the precision of the crate counter, so 600 stays as the default and DEC-859 item 2 names 601 as the alternative; R-CRATE-119: cite DEC-859')
t('G04-6', 'a forced outcome makes no selection draw (DEC-652)', 'amend', 'DEC-652', 'crates_and_powers.md R-CRATE-121', 'hand-off H-2 to X-DEC-P4b')
t('G04-7', 'shockwave active ticks A = ⌈10 × size⌉ − 1 (note for DEC-241 item 3)', 'new', 'DEC-859', 'weapons_and_projectiles.md R-CRATE-470', 'R-CRATE-470: cite DEC-859')
t('G04-8', 'rocket life ticks against the rocket-turret key (10 or 11)', 'none', '', 'weapons_and_projectiles.md R-CRATE-431; defences_and_props.md', ARB_RT + '; no DEC: the reference settles it')
t('G04-9', 'known items DEC-304 (every team) and DEC-116 (start hold)', 'merged', 'DEC-304;DEC-116', '', 'see item 1 and item 12')
# ---- G05
t('G05-1', 'DEC-304 wording: Spawn Surge affects every team', 'amend', 'DEC-304', 'reference/05 R-ECON-045, R-ECON-046', '')
t('G05-2', 'Transport cap of 0 under map values (new Open DEC requested)', 'merged', 'DEC-120', 'reference/05 R-ECON-093, R-ECON-098', 'owner decision O19 (DEC-120) decides it: 0 means none. R-ECON-093 step 3: drop "or the Transport cap is 0" (a cap of 0 grants no Transport; unlimited is its own explicit value); R-ECON-098: "a numeric Transport cap of 1 or more", as R-CRATE-125 states')
t('G05-3', 'DEC-065 consequence on the three Flag Chain maps (40 to 100 Workers, cap 2 to 3)', 'amend', 'DEC-065', 'reference/05 R-ECON-091', 'G08a confirms the per-map values')
t('G05-4', 'integer tick ranges one tick above DEC-002', 'amend', 'DEC-002', 'reference/05 tuning table', 'reference/05: drop the tuning-table notes that quote the older DEC-002 lower bounds')
t('G05-5', 'starting-power fallback: no power when no outcome qualifies', 'new', 'DEC-858', 'reference/05 R-ECON-110', 'R-ECON-110: cite DEC-858 in place of "see DEC-014, DEC-021"')
t('G05-6', 'second-tutorial lock checks the tutorial player team (DEC-103)', 'amend', 'DEC-103', 'reference/05 R-ECON-052', '')
t('G05-7', 'port occupancy binding (DEC-088, DEC-094, DEC-016)', 'none', 'DEC-088;DEC-094;DEC-016', 'reference/05 R-ECON-012, R-ECON-013', 'G03a: state the port occupant as R-ECON-012 and R-ECON-013 do: the first operating Worker in grid order among those operating that port of that structure within 5 wu')
t('G05-8', 'Flag Chain airstrike threshold: either condition', 'new', 'DEC-851', 'reference/05 R-ECON-104', 'R-ECON-104: cite DEC-851')
t('G05-9', 'grace period: no economy timer advances', 'merged', 'DEC-116', 'reference/05 R-ECON-004', '')
t('G05-10', 'Population Limiter overpopulation halving: integer division', 'new', 'DEC-851', 'reference/05 R-ECON-064', 'R-ECON-064: cite DEC-851 next to DEC-454')
# ---- G06
t('G06-1', 'default bindings of demote and scrap (DEC-086)', 'new', 'DEC-863', 'actions_and_bindings.md R-CTRL-044, R-CTRL-045', '')
t('G06-2', 'clickable power bar details (DEC-083)', 'new', 'DEC-863', 'selection_and_power_slots.md R-CTRL-226, R-CTRL-230', '')
t('G06-3', 'secondary-button priority order (DEC-023)', 'new', 'DEC-863', 'actions_and_bindings.md R-CTRL-050', '')
t('G06-4', 'pointer fallback sphere', 'new', 'DEC-864', 'actions_and_bindings.md R-CTRL-058', 'R-CTRL-058: cite DEC-864')
t('G06-5', 'lock countdown outline pass (render defect)', 'new', 'DEC-863', 'selection_and_power_slots.md R-CTRL-232', '')
t('G06-6', 'smoothing overshoot not clamped', 'new', 'DEC-864', 'camera.md R-CTRL-454', 'R-CTRL-454: cite DEC-864 next to DEC-006')
t('G06-7', 'automatic-camera setting has no options widget', 'amend', 'DEC-050', 'camera.md R-CTRL-480', 'R-CTRL-480: cite DEC-050')
# ---- G07
t('G07-1', 'bot actions outside the player command catalogue', 'new', 'DEC-122', 'README R-BOT-014, R-BOT-015', 'R-BOT-015 and README §8: cite DEC-122 for the design-phase decision')
t('G07-2', 'bot controller position (DEC-087 gives none)', 'new', 'DEC-862', 'README R-BOT-008', 'DEC-087 points to DEC-862; R-BOT-008: cite DEC-862')
t('G07-3', 'promotion Power expiry (DEC-087)', 'new', 'DEC-862', '04_powers_and_unit_policies.md R-BOT-107', 'R-BOT-107: cite DEC-862')
t('G07-4', 'standard priority with no usable link: neighbour mean 0', 'new', 'DEC-862', '02_priority_model.md R-BOT-060', 'R-BOT-060: cite DEC-862; DEC-653 item 6: hand-off H-3')
t('G07-5', 'bounded placement loops: cap 100, nothing placed', 'new', 'DEC-857', '04_powers_and_unit_policies.md R-BOT-105, R-BOT-117', 'R-BOT-105: cite DEC-857 next to DEC-014')
t('G07-6', 'defence-flag propagation in one pass', 'new', 'DEC-861', '05_mode_strategies_and_tutorials.md R-BOT-129', 'R-BOT-129: cite DEC-861')
t('G07-7', 'tagged zone never cleared', 'new', 'DEC-861', '01_navigation_graph.md R-BOT-036', 'R-BOT-036: cite DEC-861')
t('G07-8', 'Worker nearest node never cleared', 'new', 'DEC-861', '01_navigation_graph.md R-BOT-043', 'R-BOT-043: cite DEC-861')
t('G07-9', 'pins on a shared nearest node: last writer wins', 'new', 'DEC-862', '02_priority_model.md R-BOT-070', 'R-BOT-070: cite DEC-862 next to DEC-016')
t('G07-10', 'random selection and stored crossings (consequence of DEC-087)', 'new', 'DEC-862', '03_decision_loop_and_leaders.md R-BOT-088', 'R-BOT-088: cite DEC-862')
t('G07-11', 'DEC-087 and DEC-112 wording on the Siege bot Transport behaviour', 'amend', 'DEC-087;DEC-112', 'transport.md R-UNIT-405', 'transport.md R-UNIT-405: replace "The Siege behaviour runs in every Siege match (DEC-087)" by a statement that the bot\'s Siege Transport objective list is filled (R-BOT-127), so the mobile-bomb branch runs (DEC-087)')
# ---- G08a
t('G08a-1', 'a mode listed twice in a map mode list', 'new', 'DEC-865', 'rules_and_conventions.md R-MAP-013', 'R-MAP-013: cite DEC-865')
t('G08a-2', 'the Essence-orb rain map effect', 'new', 'DEC-865', 'rules_and_conventions.md R-MAP-092', 'R-MAP-092: cite DEC-865')
t('G08a-3', 'map routes never followed', 'new', 'DEC-865', 'rules_and_conventions.md R-MAP-120, R-MAP-121', 'R-MAP-121: cite DEC-865')
t('G08a-4', 'structures and starting forces at a closed slot start position', 'new', 'DEC-867', 'rules_and_conventions.md R-MAP-046, edge case 3', 'edge case 3 and R-MAP-046: cite DEC-867')
t('G08a-5', 'unlimited defender population cap', 'new', 'DEC-866', 'rules_and_conventions.md R-MAP-061, R-MAP-062', 'R-MAP-062: cite DEC-866')
t('G08a-6', 'non-square maps', 'new', 'DEC-865', 'rules_and_conventions.md R-MAP-080, R-MAP-048, R-MAP-190, edge case 10', 'R-MAP-080: cite DEC-865')
t('G08a-7', 'reinforcement power option values restricted to Powers', 'new', 'DEC-865', 'rules_and_conventions.md R-MAP-072', 'R-MAP-072: cite DEC-865')
t('G08a-8', 'population cap 0 locks every Spawn Node and Transit Gate', 'new', 'DEC-866', 'rules_and_conventions.md R-MAP-062', '')
t('G08a-9', 'Trooper starting-forces record not in a group', 'new', 'DEC-865', 'rules_and_conventions.md R-MAP-131', 'R-MAP-131: cite DEC-865')
t('G08a-xdoc-5', 'R-MODE-084 lets a repeated cap line count (item 60)', 'amend', 'DEC-801', 'README R-MODE-084', 'DEC-801 is right. R-MODE-084: a second population-cap line is a content-build error (DEC-801, DEC-015), as R-ECON-053 and R-MAP-061 state')
t('G08a-xdoc-8', 'R-ECON-052 and the glossary divide by the map player count (item 59)', 'amend', 'DEC-079', 'reference/05 R-ECON-052, R-ECON-053', 'DEC-079 is right. R-ECON-052 rule 3: the divisor is the number of open slots; R-ECON-053: a map without a population cap states unlimited explicitly (R-MAP-061, DEC-120); glossary entry "population cap" and reference/00: divided by the number of open slots')
# ---- G08b
t('G08b', 'no DEC requests', 'none', '', '', '')
# ---- G09a
t('G09a-1', 'closed slots (DEC-079 detail)', 'new', 'DEC-867', 'screen_flow_and_match_setup.md R-META-090', 'R-META-090: cite DEC-867')
t('G09a-2', 'chat rename parsing quirk', 'new', 'DEC-868', 'screen_flow_and_match_setup.md R-META-101', 'R-META-101: cite DEC-868 next to DEC-006')
t('G09a-3', 'studio-mark fade jump', 'new', 'DEC-868', 'front_end_presentation.md R-META-219, edge case 2', 'R-META-219: cite DEC-868')
t('G09a-4', 'play-time format 0:60', 'new', 'DEC-868', 'screen_flow_and_match_setup.md R-META-038; rules_and_conventions.md R-MAP-021', 'R-MAP-021: replace "as hours and minutes" by \'as "0:" followed by the minutes in two digits (R-META-038)\'; cite DEC-868 in both')
t('G09a-5', 'Siege side-row taken test differs for drawing and pressing', 'merged', 'DEC-102', 'screen_flow_and_match_setup.md R-META-080', 'merged with C-UI-b request 4 (defect F18 of the UI framework, now named in DEC-102). R-META-080: one availability rule for drawing and pressing, computed in the update; drop "counting the local slot when the row is pressed and not counting it when the row is drawn" (UI-416, UI-417)')
t('G09a-xdoc-2', 'G01 still writes "0 means no limit" (item 67)', 'amend', 'DEC-120', 'README R-MODE-074, R-MODE-056', 'DEC-120 is right. R-MODE-074, R-MODE-056 and the time-limit and crate-period key notes: "no limit" and "none" are explicit option values, never 0 (R-META-048)')
# ---- G09b
t('G09b-1', 'reference sound options without a Species counterpart', 'new', 'DEC-869', 'options_localisation_and_hooks.md R-META-429, R-META-430, R-META-431', 'cite DEC-869 in each')
t('G09b-2', 'DEC-044 against DEC-570 (separate volumes)', 'amend', 'DEC-044', 'options_localisation_and_hooks.md R-META-434', '')
t('G09b-3', 'DEC-800 option B wording (settings writer and comment lines)', 'none', 'DEC-800', '', 'already corrected: DEC-800 option B says the writer re-emits blank and comment lines')
t('G09b-adr', 'ADR-0063 against DEC-099 (item 70)', 'none', 'DEC-099', 'options_localisation_and_hooks.md R-META-463, R-META-464, R-META-465', 'X-ADR-P4 amends ADR-0063 (item 108 drops its sign-off exception); DEC-099 unchanged')
# ---- G10
t('G10-1', 'sting for an owner change to no team (detail of DEC-097)', 'amend', 'DEC-097', 'event_model_and_sound_data.md R-AUD-025; event_catalogue.md R-AUD-134, R-AUD-137', '')
t('G10-2', 'DEC-565 names the dropped Leader absorb mode', 'amend', 'DEC-565', '', '')
t('G10-xdoc-097', 'G04 R-CRATE-117 and G09 R-META-454 contradict DEC-097 (item 81)', 'amend', 'DEC-097', 'crates_and_powers.md R-CRATE-117; options_localisation_and_hooks.md R-META-454', 'DEC-097 is right. R-CRATE-117: while progress drains, the bucket tones play downwards (R-AUD-031); R-META-454 item 1: the sample-memory level is not honoured, every sample group uses all its members (R-AUD-064)')
t('G10-xdoc-323', 'tree.burning loop and DEC-323 (G03b question)', 'none', 'DEC-323', 'event_catalogue.md R-AUD-039', '')
# ---- G11
t('G11-1', 'tutorial for the first playable (owner-level)', 'new', 'DEC-123', 'reference/11 R-TUT-001; README R-MODE-083', 'reference/11 §8 and R-MODE-083: cite DEC-123')
t('G11-2', 'objective cards (DEC-050) and the 16 help.hint pictures', 'amend', 'DEC-050', 'reference/11 R-TUT-013', 'DEC-050 is right. Coordinator re-routes the objective-card section of the front-end source as hooks (G09b hooks file or reference/11), or X-AST moves the 16 objective-card rows to post-launch hook art (item 73)')
t('G11-3', 'where the tutorial director runs (DEC-103)', 'amend', 'DEC-103', 'reference/11 R-TUT-061, R-TUT-062', 'reference/12/06 (C-SIM-b owner): add the tutorial events to the command catalogue (item 74)')
t('G11-4', 'readings for review (hint alpha, right-edge clamp, unset step timer, first evaluation after the grace period)', 'new', 'DEC-870', 'reference/11 R-TUT-023 to R-TUT-029, R-TUT-061, R-TUT-065, R-TUT-070 to R-TUT-075', 'cite DEC-870 in R-TUT-061 and R-TUT-065')
t('G11-5', 'Launch Race help page names fuel 70; the rule is 33', 'none', '', 'mode_objects.md R-BLDG-325', '')
# ---- phase4_dec_fixes items in scope
t('item 1', 'DEC-304: every team', 'amend', 'DEC-304', 'reference/05 R-ECON-045', 'see G05-1')
t('item 2', 'what a map Transport cap of 0 means', 'merged', 'DEC-120', 'reference/05 R-ECON-093', 'see G05-2')
t('item 3', 'DEC-065: record the consequence for three Flag Chain maps', 'amend', 'DEC-065', 'reference/05 R-ECON-091', 'see G05-3')
t('item 4', 'DEC-002: tick ranges one tick lower than the replay', 'amend', 'DEC-002', '', 'see G05-4')
t('item 5', 'DEC-014 and DEC-103: empty starting-power draw and tutorial team', 'amend', 'DEC-014;DEC-103;DEC-858', 'reference/05 R-ECON-052, R-ECON-110', 'DEC-014 points to DEC-858; DEC-103 names the tutorial player team')
t('item 6', 'DEC-021: "Forest" is the Essence Grove', 'amend', 'DEC-021', 'crates_and_powers.md R-CRATE-132', '')
t('item 9 and 16', 'DEC-016 against DEC-650 and DEC-455 (order within a cell)', 'merged', 'DEC-016', '', 'assigned to X-DEC-P4b by the brief (one Resolved entry); DEC-016 left unchanged for its hand-off text')
t('item 12', 'DEC-116 against the start hold', 'amend', 'DEC-116;DEC-036', 'README R-MODE-044; crates_and_powers.md R-CRATE-102; reference/00 R-VIS-052', 'DEC-116 is Resolved: no drop happens while the world is held for the grace period. R-MODE-044 and R-VIS-052: replace "Supply crate drops are not suspended during the grace period; only a paused match stops them" by "No world tick runs during the grace period, so no Supply crate drops in it (DEC-116)"; R-CRATE-102: drop "Drops that the grace period does not stop are the reference default of DEC-116" and cite DEC-116; glossary "Supply crate" row unchanged')
t('item 14', 'DEC-011 wording on the stale Source Node id', 'amend', 'DEC-011', 'reference/12/02 R-SIM-024', '')
t('item 19', 'DEC-107, DEC-120, DEC-121 read by G04, G08a, C-DAT', 'none', 'DEC-107;DEC-120;DEC-121', '', 'checked: G04 applies all three (R-CRATE-126, R-CRATE-125, R-CRATE-198); G08a applies DEC-120 and points to R-CRATE-029/030 for the exclusions; the map schema of data_schemas cites DEC-120')
t('item 20', 'DEC-078 no-group colour', 'none', 'DEC-078', 'README R-MODE-017', 'see arbitration A-1: DEC-078 needs no change; R-MODE-017 replacement in A-1')
t('item 22', 'DEC-017: 38 option definitions', 'amend', 'DEC-017', '', 'see G01-6')
t('item 24', 'O20 has no DEC entry', 'amend', 'DEC-229;DEC-123', '', 'hand-off H-4 to X-DEC-P4b (owner-decision line in DEC-229); the tutorial question is DEC-123')
t('item 25', 'DEC-085 against builder.md', 'amend', 'DEC-085', 'builder.md R-UNIT-457', 'see arbitration A-3 and G02a2-1')
t('item 27', 'G02a2: 10 DEC requests or clarifications', 'merged', 'DEC-853;DEC-854;DEC-871;DEC-085;DEC-453', '', 'see rows G02a2-1 to G02a2-10')
t('item 31', 'G02a1: four new DECs', 'merged', 'DEC-852;DEC-853', '', 'see rows G02a1-1 to G02a1-4')
t('item 34', 'DEC-014: G02b search caps 256 and 2000', 'new', 'DEC-857', 'hazards_and_projectiles.md R-UNIT-702, R-UNIT-706, R-UNIT-722', 'see G02b-interp-caps; DEC-014 cites DEC-857')
t('item 35', 'G02b: seven DEC requests', 'merged', 'DEC-855;DEC-856', '', 'see rows G02b-1 to G02b-7')
t('item 43', 'G03a: DEC-459 against DEC-561, seven requests', 'merged', 'DEC-459;DEC-860', '', 'see rows G03a-1 to G03a-7')
t('item 47', 'DEC-465 against DEC-653 item 7; DEC-550 airstrike wording', 'amend', 'DEC-465;DEC-550;DEC-653', '', 'see G04-2 and G04-3; hand-off H-1')
t('item 50', 'bot actions without a catalogue command (owner-level)', 'new', 'DEC-122', 'README R-BOT-014, R-BOT-015', 'see G07-1')
t('item 51', 'DEC-087: bot controller position, promotion-Power expiry, 100 placement attempts', 'amend', 'DEC-087;DEC-862;DEC-857', 'README R-BOT-008; R-BOT-105, R-BOT-107', 'see G07-2, G07-3 and G07-5')
t('item 52', 'node and marker recount periods 21 or 20 ticks (DEC-064)', 'none', 'DEC-064', 'reference/03 README table 2.7-A; reference/07 R-BOT keys', 'see arbitration A-12: 20 ticks for bot nodes and markers, which DEC-064 does not cover; reference/03 README (R-BLDG-003, table 2.7-A) changes, not reference/07. DEC-064 text narrowed to the recounts re-armed from the match clock, naming the bot countdowns as not covered')
t('item 58', 'G06: seven DEC requests', 'merged', 'DEC-863;DEC-864;DEC-050', '', 'see rows G06-1 to G06-7')
t('item 59', 'DEC-079: R-ECON-052 and the glossary still divide by the player count', 'amend', 'DEC-079', 'reference/05 R-ECON-052', 'see G08a-xdoc-8')
t('item 60', 'DEC-801: R-MODE-084 lets a repeated cap line count', 'amend', 'DEC-801', 'README R-MODE-084', 'see G08a-xdoc-5')
t('item 63', 'G08a: nine DEC requests', 'merged', 'DEC-865;DEC-866;DEC-867', '', 'see rows G08a-1 to G08a-9')
t('item 67', 'DEC-120: G01 still writes "0 means no limit"', 'amend', 'DEC-120', 'README R-MODE-074', 'see G09a-xdoc-2')
t('item 69', 'G09a: five DEC requests', 'merged', 'DEC-867;DEC-868;DEC-102', '', 'see rows G09a-1 to G09a-5')
t('item 70', 'ADR-0063 against DEC-099', 'none', 'DEC-099', '', 'see G09b-adr')
t('item 71', 'DEC-044, DEC-570, DEC-800 and the sound options', 'amend', 'DEC-044;DEC-570;DEC-800;DEC-869', '', 'DEC-044 narrowed; DEC-570 unchanged; DEC-800 already correct; sound options DEC-869')
t('item 72', 'tutorial mode for the first playable (owner-level)', 'new', 'DEC-123', 'reference/11 R-TUT-001', 'see G11-1')
t('item 73', 'DEC-050 objective cards and the routing of the front-end objective section', 'amend', 'DEC-050', 'reference/11 R-TUT-013', 'see G11-2')
t('item 74', 'DEC-103: the tutorial director placement', 'amend', 'DEC-103', 'reference/11 R-TUT-061', 'see G11-3')
t('item 81', 'DEC-097, DEC-565 and the no-team sting', 'amend', 'DEC-097;DEC-565', '', 'see G10-1, G10-2 and G10-xdoc-097')
t('item 89', 'DEC-567: an omitted DSP parameter takes its declared default', 'amend', 'DEC-567', 'audio_engine.md AEN-071; data_schemas README DAT-012', 'DEC-567 lives in a file of X-DEC-P4a and was amended here; X-DEC-P4b also lists the item')
t('item 93', 'DEC-102: the percent sign is an ordinary character in text fields', 'amend', 'DEC-102', '', 'DEC-102 lives in a file of X-DEC-P4a and was amended here; X-DEC-P4b also lists the item')
t('item 98', 'DEC-102 and ADR-0032 against R-META-080, R-CTRL-223, R-META-406', 'amend', 'DEC-102', 'screen_flow_and_match_setup.md R-META-080; selection_and_power_slots.md R-CTRL-223; options_localisation_and_hooks.md R-META-406', 'DEC-102 now names the one availability rule of a colour or side row. R-META-080: see G09a-5; R-CTRL-223: the Power slots panel produces its screen zones in its advance, not while drawing, with the same one-frame delay (UI-461); R-META-406: add the settings save of the colour or side row press (UI-419, arbitration A-22); R-META-044: arbitration A-17')
t('item 48 (A-9)', 'cancelled Transport: destroyed outside a tutorial (arbitration A-9)', 'amend', 'DEC-551', 'transport.md R-UNIT-370; README R-CRATE-020', 'DEC-551 now limits the Transport case to a tutorial; transport.md R-UNIT-370 and its rows follow A-9')
t('item 48 (A-10)', 'Strike team life 601 ticks (arbitration A-10)', 'none', 'DEC-859', 'strike_team.md R-UNIT-338', 'apply A-10; DEC-859 item 2 cites the 601-tick count as the single-precision alternative for the crate life')
t('item 101', 'DEC-024 and DEC-803: building focus, approach and developer fly mode', 'amend', 'DEC-024', 'camera_and_sequences.md CAM-053 to CAM-058, CAM-080 to CAM-084; reference/06 camera.md R-CTRL-457; map_editor_hooks.md R-MAP-309', 'DEC-024 amended; DEC-803 is X-DEC-P4b\'s. R-CTRL-457: the building-focus mode and the debug fly mode exist only as the timeline operations and the developer tool of the camera component (CAM-053 to CAM-058, CAM-080 to CAM-084; DEC-024); R-MAP-309 cites CAM-080')
with open(os.path.join(W, 'p4_dec_triage_X-DEC-P4a.csv'), 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f)
    w.writerow(['source', 'request', 'outcome', 'dec_id', 'cite_in', 'document_fix'])
    w.writerows(T)
from collections import Counter
print(len(DECMAP), 'dec_map rows;', len(T), 'triage rows;', dict(Counter(r[2] for r in T)))
