"""FX-07: scripted edits of reference/12/06 (routes, A-2, DEC citations, O23 names). Usage: python3 -I FX-07_edit06.py <file>"""
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
sel = "../06_controls_camera_and_commands/selection_and_power_slots.md"
reps = [
# RA-258 lobby slot open field
("| achievement mask | 0 | the achievement hook mask ([../09_front_end_and_match_flow/options_localisation_and_hooks.md](../09_front_end_and_match_flow/options_localisation_and_hooks.md)) |\n",
 "| achievement mask | 0 | the achievement hook mask ([../09_front_end_and_match_flow/options_localisation_and_hooks.md](../09_front_end_and_match_flow/options_localisation_and_hooks.md)) |\n| open | true | whether the slot is open; a closed slot stays unused for the whole match (R-SIM-461) |\n"),
# RB-006 / DEC-262
("**R-SIM-401** (MUST) A team's current selection changes only through the execution of a command (R-SIM-450) and through the effects that executing a command has inside the same simulation step; no other simulation rule and no presentation or input code writes it (DEC-008, DEC-119).",
 "**R-SIM-401** (MUST) A team's current selection changes only through the execution of a command (R-SIM-450) and through the effects that executing a command has inside the same simulation step; no other simulation rule and no presentation or input code writes it (DEC-008, DEC-119). The selected actor is held as a generational handle and resolved again on every use, so a selection whose actor has been removed resolves to none and never designates a later occupant of the same slot ([03_entity_model_and_identity.md](03_entity_model_and_identity.md) R-SIM-142, DEC-028; see DEC-262)."),
("2. sets the team's current selection to (g, a, s);", "2. sets the team's current selection to (g, a, s), with the actor held as a generational handle (R-SIM-401);"),
# A-2
("**R-SIM-407** (MUST) Eliminating a team changes the health of every member of its actor groups and of every ungrouped actor by −`ref.sim.team.eliminate_damage_factor` × that actor's health stat (2 × health), stops all of its Powers, and sets every Transit Gate it owns to the no-team value.",
 "**R-SIM-407** (MUST) Eliminating a team changes the health of every member of its actor groups and of every ungrouped actor by −`ref.sim.team.eliminate_damage_factor` × that actor's current health (2 × current health), as unresistable damage, stops all of its Powers, and sets every Transit Gate it owns to the no-team value."),
("| `ref.sim.team.eliminate_damage_factor` | 2 | × health stat | | R-SIM-407 | — |", "| `ref.sim.team.eliminate_damage_factor` | 2 | × current health | | R-SIM-407 | — |"),
("Eliminating a team gives every one of its actors a health change of −2 × its health stat,", "Eliminating a team gives every one of its actors a health change of −2 × its current health,"),
# RA-120 DEC-121
("| creature created by a Supply crate outcome | the team that placed the Power, except the Reaper Serpent and the Hive, which belong to the Rogue faction (DEC-107) | never | — |",
 "| creature created by a Supply crate outcome | the team that placed the Power, except the Reaper Serpent and the Hive, which belong to the Rogue faction (DEC-107); the creatures of the Brood outcome belong to the placing team (see DEC-121) | never | — |"),
# IsFriend
("IsFriend(a, b):\n    if a is the no-team value or b is the no-team value: return false\n    if the match is co-op: return coopGroup[a] == coopGroup[b]\n    return a == b",
 "IsFriend(_a, _b):\n    if _a is the no-team value or _b is the no-team value: return false\n    if the match is co-op: return coopGroup[_a] == coopGroup[_b]\n    return _a == _b"),
# SetColour -> SetColor
("a human's own choice arrives through the `SetColour` command (R-SIM-451).", "a human's own choice arrives through the `SetColor` command (R-SIM-451)."),
("In the reference the local participant then also sends a `SetColour` command for its own team with the same colour,", "In the reference the local participant then also sends a `SetColor` command for its own team with the same colour,"),
("2. then, for each participant in participant order (the order in which the participants joined the session), its latest `ControlState`, then its latest `Select`, then its latest `SetColour`, each only if one arrived in that tick.\n\nA participant's earlier `ControlState`, `Select` or `SetColour` of the same tick is discarded without merging. Because `Select` and `SetColour` set state,",
 "2. then, for each participant in participant order (the order in which the participants joined the session), its latest `ControlState`, then its latest `Select`, then its latest `SetColor`, each only if one arrived in that tick.\n\nA participant's earlier `ControlState`, `Select` or `SetColor` of the same tick is discarded without merging. Because `Select` and `SetColor` set state,"),
("| `PlayerCommand::SetColour` | team, colour id |", "| `PlayerCommand::SetColor` | team, colour id |"),
("    P->>S: Select and SetColour, at any time\n    Note over S: keeps only the latest ControlState, Select and SetColour of each participant",
 "    P->>S: Select and SetColor, at any time\n    Note over S: keeps only the latest ControlState, Select and SetColor of each participant"),
("    S->>W: then for each participant in join order its ControlState, Select and SetColour", "    S->>W: then for each participant in join order its ControlState, Select and SetColor"),
# RB-005
("**R-SIM-422** (MUST) On a map with fewer than four players, each unused slot draws a colour from 0 to 3 that no player slot holds,",
 "**R-SIM-422** (MUST) On a map with fewer than four players, each unused slot draws a colour from 0 to 3 that no player slot holds, by the free-colour draw of R-SIM-420 step 3: colour = (simulation draw mod `ref.sim.team.player_colour_count`) div `ref.sim.team.coop_colour_divisor`, which gives 0 to 3, and only the colour changes,"),
("| `ref.sim.team.player_colour_count` | 8 | colours | colour ids 0 to 7 | R-SIM-415, R-SIM-416, R-SIM-420, R-SIM-451 | — |",
 "| `ref.sim.team.player_colour_count` | 8 | colours | colour ids 0 to 7 | R-SIM-415, R-SIM-416, R-SIM-420, R-SIM-422, R-SIM-451 | — |"),
("| `ref.sim.team.coop_colour_divisor` | 2 | — | random free colour (draw mod 8) div 2, in a fallback step that never applies | R-SIM-420 | DEC-451 |",
 "| `ref.sim.team.coop_colour_divisor` | 2 | — | random free colour (draw mod 8) div 2: the unused-slot colours, and a co-op fallback step that never applies | R-SIM-420, R-SIM-422 | DEC-450, DEC-451 |"),
# DEC-653 R-SIM-427
("When no member reported a position, the division is not evaluated and the centre and velocity keep their values.",
 "When no member reported a position, the division is not evaluated and the centre and velocity keep their values (see DEC-653)."),
# DEC-263 R-SIM-428 + O23
("**R-SIM-428** (MUST) `ActorGroup::Attack(position, withGrenade)`:", "**R-SIM-428** (MUST) `ActorGroup::Attack(_position, _withGrenade)`:"),
("With no candidate, no member attacks and no candidate draw is made.", "With no candidate, no member attacks and no candidate draw is made (see DEC-263)."),
# RA-258 R-SIM-433
("Starting a match fills every empty slot below the map's player count with a bot team: the host requests (player count − filled slots) bot teams through the session ([../09_front_end_and_match_flow/screen_flow_and_match_setup.md](../09_front_end_and_match_flow/screen_flow_and_match_setup.md)) and then issues `StartMatch` (R-SIM-460).",
 "Starting a match fills every empty open slot below the map's player count with a bot team, and a closed slot gets none (R-SIM-461): the host requests one bot team for every unused open slot below the player count through the session ([../09_front_end_and_match_flow/screen_flow_and_match_setup.md](../09_front_end_and_match_flow/screen_flow_and_match_setup.md)) and then issues `StartMatch` (R-SIM-460)."),
# RA-217 R-SIM-434
("**R-SIM-434** (MUST) The bot is not a participant. When a bot team is initialised in the match world, the simulation spawns one bot controller (`BotController`) for it, placed inside the map (DEC-087; the position is specified in [../07_bots/README.md](../07_bots/README.md)). In the reference the spawn asks for the off-map point (−100, 0, −100) of `ref.sim.bot.controller_spawn_request`, so the spawn search of [05_space_routing_and_physics.md](05_space_routing_and_physics.md) makes all its draws (20) and falls back to (20, 0, 20) at the map's corner. The Rogue and Visitor factions always receive a bot controller as well (DEC-558).",
 "**R-SIM-434** (MUST) The bot is not a participant. When a bot team is initialised in the match world, the simulation spawns one bot controller (`BotController`) for it, inside the map, at the position of [../07_bots/README.md](../07_bots/README.md) R-BOT-008: its team's start position, or the centre of the map for the Rogue and Visitor factions and for a slot with no start position; the placement makes no spawn search and no draw (DEC-087). `ref.sim.bot.controller_spawn_request` records only the reference request, the off-map point (−100, 0, −100), for which the spawn search of [05_space_routing_and_physics.md](05_space_routing_and_physics.md) makes all its draws (20) and falls back to (20, 0, 20) at the map's corner; it is not the Species placement. The Rogue and Visitor factions always receive a bot controller as well (DEC-558)."),
# RA-217 R-SIM-435
("**R-SIM-435** (MUST) A bot controller is simulated identically on every participant as part of the simulation step. It sends no network traffic (DEC-200): it acts through the same simulation operations that the commands of §2.11 execute, and each of its actions passes the same validation path as a player command (DEC-119, DEC-202). It reads the full simulation state ([../07_bots/README.md](../07_bots/README.md)).",
 "**R-SIM-435** (MUST) A bot controller is simulated identically on every participant as part of the simulation step, and it sends nothing over the network (DEC-200). A bot is an authorised command source of its own team (DEC-119): in its bot step it issues the actions that the player command catalogue expresses (targeting a Power, stopping a Power and aiming a Beam Dish) as catalogue commands, which execute at once through the command's operation, with the command's validation and no-op rules (DEC-202), and never enter the command batch of a tick ([../07_bots/README.md](../07_bots/README.md) R-BOT-014). Every other bot action is a bot-only operation of R-BOT-015, which applies the named simulation operation directly after checking that the object still exists and belongs to the bot's team (see DEC-122). It reads the full simulation state."),
# RA-258 R-SIM-437
("**R-SIM-437** (MUST) A request for a bot team is ignored once the team count has reached the map's player count, and bot teams count toward the \"server full\" test for humans (DEC-244).",
 "**R-SIM-437** (MUST) A request for a bot team is ignored once every open slot below the map's player count is held, and bot teams count toward the \"server full\" test for humans, which counts open slots only: a session is full once every open slot is held, and a closed slot is never offered to a joining player (R-SIM-461, DEC-244)."),
# RA-217 R-SIM-438
("Bot teams send none: their bot controller acts directly (R-SIM-435).", "Bot teams send none: a bot issues its catalogue commands inside its own bot step and applies its bot-only operations directly (R-SIM-435)."),
# O23 fields + RA-200 d
("| `move_target` | one-shot flag |", "| `moveTarget` | one-shot flag |"),
("| `direct_move` | flag | a direct movement vector is present |", "| `directMove` | flag | a direct movement vector is present |"),
("| `primary_fire` | one-shot flag |", "| `primaryFire` | one-shot flag |"),
("| `secondary_fire` | one-shot flag | secondary fire at the pointer target |", "| `secondaryFire` | one-shot flag | secondary fire at the pointer target |"),
("| `target_directed` | one-shot flag | a directed target vector is present |", "| `targetDirected` | one-shot flag | a directed target vector is present |"),
("| `secondary_fire_directed` | one-shot flag | directed secondary fire |", "| `secondaryFireDirected` | one-shot flag | directed secondary fire |"),
("| `camera_tracking` | flag | the sender's camera is tracking the selected unit |", "| `cameraTracking` | flag | the sender's camera is tracking the selected unit |"),
("| `special_orders` | one-shot flag | the special-orders modifier is held |",
 "| `specialOrders` | one-shot flag | the special-orders action `action.order.special` fired; it is set when the action goes down, never held as a modifier ([../06_controls_camera_and_commands/actions_and_bindings.md](../06_controls_camera_and_commands/actions_and_bindings.md) R-CTRL-051, R-CTRL-101) |"),
("| `end_set_target` | one-shot flag | the set-target action ended |", "| `endSetTarget` | one-shot flag | the set-target action ended |"),
("| `primary_held` | flag | the primary pointer button is held |", "| `primaryHeld` | flag | the primary pointer button is held |"),
("| `pointer_position` | position |", "| `pointerPosition` | position |"),
("| `direct_move_vector` | world-space (x, z) | the movement-stick vector; present with `direct_move` |", "| `directMoveVector` | world-space (x, z) | the movement-stick vector; present with `directMove` |"),
("| `direct_target_vector` | world-space (x, z) | the aiming-stick vector; present with `target_directed` or `secondary_fire_directed` |", "| `directTargetVector` | world-space (x, z) | the aiming-stick vector; present with `targetDirected` or `secondaryFireDirected` |"),
("**R-SIM-439** (MUST) The team command state holds these fields.", "**R-SIM-439** (MUST) The team command state, a public aggregate, holds these fields."),
# RA-200 e + O23
("with right the camera's control vector and front = up × (−right),", "with right the camera's control vector ([../06_controls_camera_and_commands/camera.md](../06_controls_camera_and_commands/camera.md) R-CTRL-451, R-CTRL-487) and front = up × (−right),"),
("The `camera_tracking` flag is part of the state the sender sends,", "The `cameraTracking` flag is part of the state the sender sends,"),
# RA-200 a
("| selects them on the team (R-SIM-405); then, even while the team's selection is blocked, the team's Power slots select the Power bound to the named group or actor, if there is one |",
 "| selects them on the team (R-SIM-405); then, even while the team's selection is blocked, the team's Power slots select the Power bound to the named group or actor, if there is one; when no Power tracks the named group or actor, the team's current Power becomes none ([" + sel + "](" + sel + ") R-CTRL-202) |"),
# RA-258 command row
("| `PlayerCommand::SetAchievementMask` | team, mask | sets the lobby slot's achievement mask | — | periodically, once the participant holds a team (DEC-243 item 9) |\n",
 "| `PlayerCommand::SetAchievementMask` | team, mask | sets the lobby slot's achievement mask | — | periodically, once the participant holds a team (DEC-243 item 9) |\n| `PlayerCommand::SetSlotOpen` | team slot, open flag | before the match starts, opens or closes the slot under the rules of R-SIM-461 | — | the host's open-and-close control beside a slot row ([../09_front_end_and_match_flow/screen_flow_and_match_setup.md](../09_front_end_and_match_flow/screen_flow_and_match_setup.md) R-META-090); a match-setup change under DEC-243 |\n"),
# RA-200 c
("If no Worker remains under control, because they all died or the group was released after the order was given, the order does nothing: no route is created and no Worker receives an order (DEC-555).",
 "As the last part of its step the order sets the team's selection to none, which releases the group ([" + sel + "](" + sel + ") R-CTRL-160 step 5). If no Worker remains under control, because they all died or the group was released after the order was given, the order does nothing: no route is created and no Worker receives an order (DEC-555)."),
# tuning note
("| `ref.sim.max_teams` | 6 | team slots | `kMaxTeams`; also the number of actor-grid layers | R-SIM-392 | DEC-200 |", "| `ref.sim.max_teams` | 6 | team slots | the constant `MAX_TEAMS`; also the number of actor-grid layers | R-SIM-392 | DEC-200 |"),
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
