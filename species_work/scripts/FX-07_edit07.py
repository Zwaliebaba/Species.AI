"""FX-07: scripted edits of reference/12/07 (routes, A-32, DEC citations, stale key). Usage: python3 -I FX-07_edit07.py <file>"""
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
cre = "[reference/02 creatures](../02_units/creatures.md)"
haz = "[reference/02 hazards](../02_units/hazards_and_projectiles.md)"
trees = "[reference/03 trees](../03_buildings_and_world_objects/trees_and_crate_structures.md)"
props = "[reference/03 defences and props](../03_buildings_and_world_objects/defences_and_props.md)"
reps = [
# RB-001 / RA-220 (1)
("Initialisation draws, in this order: the colours of unused team slots (a retry loop, bounded by DEC-014), the start positions of all six team slots in team-index order, the bot colours, the co-op pairing, the Siege sides, the initialisation of structures and bot controllers, and the starting Powers.",
 "Initialisation draws in the order of the steps of table 2.4-A of [02](02_match_lifecycle_and_state_reset.md) (R-SIM-016): the start positions of team slots 0 to 5 in order in step 3; the bot colours, the co-op pairing and the Siege sides in step 6; the map-placed structures in step 7; the bot controllers and the starting Powers of the teams in step 8; the colours of unused team slots in step 9, after team initialisation (a retry loop, bounded by DEC-014); and the actor-group offset table in step 11."),
# A-32
("the initial values of a bot controller (4);", "the initial values of a bot controller (2: the Beam Dish phase, then the main-cycle phase; [../07_bots/README.md](../07_bots/README.md) R-BOT-009);"),
("so each map-placed bot objective marker draws twice at load, and a Worker draws its rate twice per spawn.",
 "so each map-placed bot objective marker draws twice at load, and a Worker and a bot controller each draw their rate twice per spawn."),
# stale key
("holds `ref.sim.init.group_offset_table_size` (100, [02](02_match_lifecycle_and_state_reset.md)) values", "holds `ref.sim.group.offset_table_size` (100, [06](06_teams_and_player_commands.md)) values"),
("Keys cited here and defined elsewhere: `ref.sim.match.presentation_seed` and `ref.sim.init.group_offset_table_size` in [02](02_match_lifecycle_and_state_reset.md);",
 "Keys cited here and defined elsewhere: `ref.sim.match.presentation_seed` in [02](02_match_lifecycle_and_state_reset.md); `ref.sim.group.offset_table_size` in [06](06_teams_and_player_commands.md); `dbg.replay.capture_frame_rate` in [diagnostics, test and performance](../../architecture/components/diagnostics_test_and_performance.md); `ref.econ.essence_grove.team_slots` in [reference/05](../05_population_flow.md);"),
# RA-219 R-SIM-628
("In the reference every bot controller's spawn is such a search: it asks for the off-map point of `ref.sim.bot.controller_spawn_request` ([06](06_teams_and_player_commands.md)) and always consumes all 20 draws; where Species places the bot controller is DEC-087.",
 "In Species a bot controller makes no spawn-position search and no draw for its placement: it stands at its team's start position, or at the map centre ([../07_bots/README.md](../07_bots/README.md) R-BOT-008). The reference spawn of every bot controller is such a search: it asks for the off-map point of `ref.sim.bot.controller_spawn_request` ([06](06_teams_and_player_commands.md)) and always consumes all 20 draws; DEC-087 replaces it."),
# RA-062 Table 1 row
("| Worker: under crowd control | `NextUnit(0.5)`, `NextUnit(number of Workers under control)`, `NextUnit(2π)` | every 0.5 to 1 s | reference/02 |\n", ""),
# RA-099 owners
("| ant: leg shape | `NextUint` modulo 3 | every tick per ant — presentation | reference/02 |", "| ant: leg shape | `NextUint` modulo 3 | every tick per ant — presentation | " + cre + " |"),
("| ant: moving to a target | `NextSigned(0.2)` heading jitter | every tick | reference/02 |", "| ant: moving to a target | `NextSigned(0.2)` heading jitter | every tick | " + cre + " |"),
("| every tick — presentation; its slow-motion request is simulation ([01](01_tick_timing_and_main_loop.md)) | [reference/04](../04_supply_crates_powers_and_weapons/README.md) |",
 "| every tick — presentation; its slow-motion request is simulation ([01](01_tick_timing_and_main_loop.md)) | " + haz + " |"),
("| Crawler: nearby egg | ⌊`NextUnit(n)`⌋ | each search | reference/02 |", "| Crawler: nearby egg | ⌊`NextUnit(n)`⌋ | each search | " + cre + " |"),
# RA-151
("| tree: Essence orb drop | `NextUint` test, then `NextUnit(h)` for x, `NextUnit(2π)` for the yaw, then `NextUnit(6)` repeated until it names a valid team; the drop timer `NextUnit(5)` | each drop | reference/03 |",
 "| tree: Essence orb drop | `NextUint` test, then `NextUnit(h)` for x, `NextUnit(2π)` for the yaw, then one uniform draw of a team slot from `ref.econ.essence_grove.team_slots` (0 to 3; the reference re-draws a slot from 0 to 5 until it is neither faction slot, which gives the same distribution, DEC-014); the drop timer `NextUnit(5)` | each drop | " + trees + " R-BLDG-616 |"),
("| Blight Grove tree | `NextUnit(10)` every tick; its Wisp `NextSigned(1)` and `NextUnit(25)`; corruption `NextUnit(5)` | — | reference/04 |",
 "| Blight Grove tree | `NextUnit(10)` every tick; its Wisp `NextSigned(1)` and `NextUnit(25)`; corruption `NextUnit(5)` | — | " + trees + " |"),
("| volcano vent | `NextUint` every idle tick and `NextUnit(4)` | — presentation | reference/08 |", "| volcano vent | `NextUint` every idle tick and `NextUnit(4)` | — presentation | " + props + " |"),
("| Allegiance Scrambler effect | `NextUnit(2.5)`, `NextUint` per converted Worker | each conversion | reference/04 |", "| Allegiance Scrambler effect | `NextUnit(2.5)`, `NextUint` per converted Worker | each conversion | " + trees + " |"),
# DEC-266 Table 2
("snowflakes, Wisps, muzzle flashes and shell casings are decorative, but they live in the simulated effect pool, take simulation handles and, in the reference, draw from the simulation stream (R-SIM-622) |",
 "snowflakes, Wisps, muzzle flashes and shell casings are decorative, but they live in the simulated effect pool, take simulation handles and object serials, and, in the reference, draw from the simulation stream (R-SIM-622); their draws are presentation draws while the objects stay simulation objects (see DEC-266) |"),
# RA-062 Table 3 clause
("| a Worker on fire, under Speed Boost or with Sickness; opening a Supply crate; receiving a Shield, Speed Boost, Convert or Fury; taking and losing crowd control |",
 "| a Worker on fire, under Speed Boost or with Sickness; opening a Supply crate; receiving a Shield, Speed Boost, Convert or Fury |"),
# RA-219 R-SIM-663
("while the simulation reads it right after a successful Power use in the same tick (the turret and firebomb options of Supply crate outcomes, instant units and the bot's Leader); the panel's highlight timer is presentation state.",
 "while the simulation reads it right after a successful Power use in the same tick (the turret and firebomb options of Supply crate outcomes, instant units and the bot's Leader); the panel's highlight timer is presentation state. In Species a bot acts on the Power it has just started, never on the most-recent Power id ([../07_bots/03_decision_loop_and_leaders.md](../07_bots/03_decision_loop_and_leaders.md) R-BOT-090, DEC-087)."),
# R-SIM-669
("the Control Zone counter and the Transport weapon-cycle debounce that survive between matches (R-SIM-730);", "the Control Zone counter and the Transport weapon-cycle debounce, which are match state (R-SIM-730);"),
# RA-219 R-SIM-690
("The bot controller advances in the teams sub-step of the world step and issues no command (DEC-119).",
 "The bot controller advances in the teams sub-step of the world step. In its step it issues the catalogue commands of its own team, which execute at once and never enter a command batch or the network, and it applies its bot-only operations directly ([../07_bots/README.md](../07_bots/README.md) R-BOT-014, R-BOT-015; DEC-119, DEC-122); executing a bot action never issues a further command."),
# DEC-260 consistency R-SIM-692
("over structure-index order and over actor-grid result order (cell x, then cell z, then team, then slot) decide bot targets,",
 "over structure-index order and over actor-grid result order (cell x, then cell z, then team, then actor serial; [05](05_space_routing_and_physics.md) R-SIM-310, DEC-260) decide bot targets,"),
# DEC-653 R-SIM-715 item 7 + items 10, 11
("7. a score-weighted Supply crate drop whose total score is 0 takes the unweighted choice directly (R-SIM-716);",
 "7. a score-weighted Supply crate drop whose total score is 0 counts every score as equal (R-SIM-716);"),
("9. degenerate vector and geometric queries follow DEC-241.\n",
 "9. degenerate vector and geometric queries follow DEC-241;\n10. an actor group whose members reported no position in a tick keeps its centre and velocity ([06](06_teams_and_player_commands.md) R-SIM-427);\n11. a terrain ray that enters the grid from outside with a direction whose x component is 0 computes its entry parameter from the z components ([terrain](../../architecture/components/terrain.md) TER-016).\n"),
("**R-SIM-716** (MUST) A score-weighted Supply crate drop favours lower-scoring teams. When the scores it weights total 0, every team in the weighting has the same weight; the weighting never divides by a zero total and never produces a non-finite value (DEC-465). The drop position is DEC-114.",
 "**R-SIM-716** (MUST) A score-weighted Supply crate drop favours lower-scoring teams. When the scores it weights total 0, every score counts as equal: every weighted team has weight 1, and the share p that the drop's first draw tests is max(1 ÷ n, 1 − 1 ÷ n) for n weighted teams; the weighting never divides by the zero total and never produces a non-finite value (DEC-465, [reference/04](../04_supply_crates_powers_and_weapons/crates_and_powers.md) R-CRATE-106; see DEC-653). The drop position is DEC-114."),
# RB-002 / RA-220 (2) R-SIM-730
("**R-SIM-730** (MUST) No simulation state survives from one match into the next, and simulation code holds no mutable static state (DEC-011). In the reference two such values are read by the simulation: a process-wide count of Control Zones that the Flag Chain rules read and that only the start of a Zone Control match resets, so participants with different session histories diverge; and a Transport weapon-cycle debounce held in a function-level static, reached only through commands that an unmodified participant never sends.",
 "**R-SIM-730** (MUST) No simulation state survives from one match into the next, and simulation code holds no mutable static state (DEC-011). The Control Zone counter is match state that starts at 0 in every match ([02](02_match_lifecycle_and_state_reset.md) R-SIM-019): Zone Control's per-second recount is the only rule that recomputes it ([reference/01 Zone Control](../01_skirmish_rules_and_modes/zone_control.md)); Flag Chain initialisation adds 1 to it for each Flag post ([reference/03](../03_buildings_and_world_objects/mode_objects.md) R-BLDG-256, R-BLDG-269); and its readers are the reinforcement choice of a turret wave, which gives the airstrike power in place of the gun turret to a team that holds at least (counter + 1) ÷ 2 of the zones ([reference/05](../05_population_flow.md) R-ECON-104), the Zone Control win test and a Zone Control score term. The Transport weapon-cycle debounce belongs to each Transport for one match ([02](02_match_lifecycle_and_state_reset.md) R-SIM-021). In the reference the counter is process-wide and only the start of a Zone Control match recomputes it, so participants with different session histories diverge, and the debounce is a function-level static, reached only through commands that an unmodified participant never sends; DEC-011 removes both."),
# RA-320 / RB-266
("Both modes also drive the presentation frame delta at a fixed capture rate, while the real clock and every real-time schedule stay real.",
 "Both modes also drive the presentation frame delta at the fixed capture rate `dbg.replay.capture_frame_rate` (25 presentation frames per second, [diagnostics, test and performance](../../architecture/components/diagnostics_test_and_performance.md) DBG-241), while the real clock and every real-time schedule stay real."),
# edge case on bot Power
("- **Bots after a failed Power use.** A bot reads its team's most-recent-Power id right after a Power use; the id is meaningful only when the use succeeded ([reference/07](../07_bots/README.md), R-SIM-663).",
 "- **Bots after a failed Power use.** A bot acts on the Power it has just started; when the start fails it targets nothing, and it never reads the most-recent-Power id ([reference/07](../07_bots/03_decision_loop_and_leaders.md) R-BOT-090, R-SIM-663)."),
# dependencies and open decisions
("the owners of the draws in Table 1: [reference/01](../01_skirmish_rules_and_modes/README.md), [reference/02](../02_units/README.md), [reference/03](../03_buildings_and_world_objects/README.md),",
 "the owners of the draws in Table 1: [reference/01](../01_skirmish_rules_and_modes/README.md), [reference/02](../02_units/README.md) with [creatures](../02_units/creatures.md) and [hazards and projectiles](../02_units/hazards_and_projectiles.md), [reference/03](../03_buildings_and_world_objects/README.md) with [defences and props](../03_buildings_and_world_objects/defences_and_props.md) and [trees and crate structures](../03_buildings_and_world_objects/trees_and_crate_structures.md), [reference/05](../05_population_flow.md),"),
("- DEC-246 — terrain runtime model and terrain content identity.\n", "- DEC-246 — terrain runtime model and terrain content identity.\n- DEC-266 — decorative objects in the simulated pools (default: they stay in the effect pool and take object serials; their draws are presentation draws).\n"),
]
bad = 0
for a, b in reps:
    n = s.count(a)
    if n != 1:
        print("COUNT", n, "for:", a[:100].replace("\n", "|"))
        bad += 1
        continue
    s = s.replace(a, b)
if bad:
    sys.exit(1)
open(p, "w", encoding="utf-8").write(s)
print("ok", len(s.encode()))
