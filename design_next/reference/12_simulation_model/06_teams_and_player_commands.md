# Teams and player commands

> Status: Reference · Prefix: R-SIM · Related: [README.md](README.md), [01_tick_timing_and_main_loop.md](01_tick_timing_and_main_loop.md), [03_entity_model_and_identity.md](03_entity_model_and_identity.md), [04_containers_and_update_order.md](04_containers_and_update_order.md), [05_space_routing_and_physics.md](05_space_routing_and_physics.md), [07_randomness_and_integrity.md](07_randomness_and_integrity.md), [../06_controls_camera_and_commands.md](../06_controls_camera_and_commands.md)

## 1 Purpose and player experience

A reference match has six team slots: up to four players, each a human or a Bot, plus the Rogue faction and the Visitor faction, which are hazards driven by bots. Each team has a colour from a fixed palette of eight player colours, a start position on the map, and, in co-op matches, a partner that shares its colour family. Every object in the world belongs to a team or to no team, and friendship between teams decides who fights whom.

The player acts on the world only through **commands**: discrete commands (select, order, promote, place, cancel, toggle) and a continuous control state that each participant sends once per tick for its own team (DEC-008, DEC-023). Every participant executes the same commands in the same order, so every participant's world stays the same. This document specifies:

- team slots, team ids and team kinds; the lobby slot and the match team; ownership of every object family; friendship; colours, start positions and alliances; actor groups and their formations; bots as team slots;
- the continuous control state (`TeamCommandState`), the per-tick execution order of commands, and the complete Species player command catalogue (`PlayerCommand`), with the parameters and the effect of every command.

Who may send which command, and how commands travel, is not part of the reference library (DEC-200, DEC-202). The input actions that issue each command are in [../06_controls_camera_and_commands.md](../06_controls_camera_and_commands.md).

## 2 Rules

### 2.1 General

**R-SIM-390** (MUST) The values of §4 are the reference tuning baseline (DEC-004). The rules of this document are the specification of record for teams, ownership and player commands (DEC-005), and every reference quirk it keeps is an explicit rule (DEC-006).

**R-SIM-391** (MUST) No rule of this document stores a non-finite value in simulation state (DEC-653). In particular: a team's selected-Worker centre exists only while at least one Worker is selected, and with none selected the team has no centre (DEC-653 item 1); a crowd order with no Worker under control computes no route and no route start (DEC-653 item 2, R-SIM-455); an actor-group formation offset uses a row length of at least 1 (DEC-653 item 3, R-SIM-430); and an actor group whose members reported no position in a tick keeps its centre and velocity (R-SIM-427).

### 2.2 Team slots, ids and kinds

**R-SIM-392** (MUST) Every match has exactly `ref.sim.max_teams` (6) team slots, and all six team objects exist in every match; a slot that nobody plays has the kind `TeamKind::Unused`.

**R-SIM-393** (MUST) Team ids have these roles:

| Team id | Role |
|---|---|
| 0 to `ref.sim.team.player_slots` − 1 (0 to 3) | player positions: a human on a local or remote participant, a Bot or a spectator. The map's start positions 0 to (player count − 1) are assigned to team ids at load (R-SIM-417). |
| `ref.sim.team.rogue_slot` (4) | the **Rogue faction**, owner of hostile creatures: colour id `ref.sim.team.rogue_colour_id` (8); a bot team that no participant owns |
| `ref.sim.team.visitor_slot` (5) | the **Visitor faction**: colour id `ref.sim.team.visitor_colour_id` (9); a bot team that no participant owns |
| the no-team value | no team: unowned structures, weather effects, Supply crates and every other object without a team. It is never registered in the actor grid and is nobody's friend (R-SIM-412). One no-team value serves every field (DEC-464). |

An effect that harms everyone, such as a fireball, carries the no-team value.

**R-SIM-394** (MUST) When the map does not allow the Rogue faction, the Rogue faction's team id is "none" and no rule creates objects for it. The Rogue and Visitor factions are initialised at match-world initialisation ([04_containers_and_update_order.md](04_containers_and_update_order.md)), and also on demand when actors are spawned for them. Each receives a bot controller like every bot team (DEC-558). No Essence orb is ever created for the Rogue faction.

**R-SIM-395** (MUST) A team has one of these kinds (`TeamKind`):

| Kind | Meaning |
|---|---|
| `TeamKind::Unused` | an empty slot; the team advance and the actor look-up skip it |
| `TeamKind::LocalPlayer` | the human playing on this participant |
| `TeamKind::RemotePlayer` | a human playing on another participant; while one exists, the match cannot be paused (R-SIM-458) |
| `TeamKind::Bot` | a bot-controlled team, which receives a bot controller (R-SIM-434); the Rogue and Visitor factions have this kind |
| `TeamKind::Spectator` | a spectator slot |

**R-SIM-396** (MUST) Whether a human team is a local-player or a remote-player team depends on the participant that looks at it. The simulation treats both kinds alike; the distinction exists only for each participant's own input, camera and presentation (DEC-008).

### 2.3 Lobby slots and match teams

**R-SIM-397** (MUST) Each slot has two records: a persistent **lobby slot** (`LobbySlot`), which lives from the lobby through the match to the results, and a per-match **team** (`MatchTeam`). The team's slot id, owning participant, kind and colour are the lobby slot's own fields, shared and not copied: a change made through either record is visible through both.

**R-SIM-398** (MUST) A lobby slot holds:

| Field | Initial value | Meaning |
|---|---|---|
| slot id | none | the team slot |
| owner | none | the participant that owns the slot; none for the Rogue and Visitor factions |
| kind | `TeamKind::Unused` | R-SIM-395 |
| colour id | none | index into the colour table (R-SIM-415): 0 to 7 for players, 8 for the Rogue faction, 9 for the Visitor faction |
| colour | — | the RGBA colour used to draw the team |
| co-op group position | 0 | 1 or 2: which member of a co-op pair, which picks the shade (R-SIM-421) |
| score, previous score | 0 | the mode score ([../01_skirmish_rules_and_modes/README.md](../01_skirmish_rules_and_modes/README.md)) |
| Launch Vessel | none | the team's Launch Race vessel |
| disconnected | true | |
| ready | false | the lobby ready flag (R-SIM-459) |
| name | empty | the player's name |
| bot takeover time | none | the game time at which a bot took over a departed player's team (R-SIM-436) |
| achievement mask | 0 | the achievement hook mask ([../09_front_end_and_match_flow/options_localisation_and_hooks.md](../09_front_end_and_match_flow/options_localisation_and_hooks.md)) |

**R-SIM-399** (MUST) A match team holds:

| Field | Initial value | Meaning |
|---|---|---|
| slot id, owner, kind, colour | the lobby slot's fields | shared with the lobby slot (R-SIM-397) |
| lobby slot | — | a reference back to its lobby slot |
| actor groups | empty | the team's actor groups (R-SIM-424) |
| ungrouped actors | empty | every actor of the team that is not in an actor group, updated in `ref.sim.update_slices` slices ([04_containers_and_update_order.md](04_containers_and_update_order.md)) |
| special units | empty | handles of the team's Leaders, Transports, Collectors and Builders, for quick look-up |
| Power slots | created when the team is initialised | the team's Power slots ([../04_supply_crates_powers_and_weapons/crates_and_powers.md](../04_supply_crates_powers_and_weapons/crates_and_powers.md)) |
| in-match ready | false | readiness in the ready-up phase (R-SIM-459) |
| current selection | none, none, none | the selected actor group, actor and structure (R-SIM-401) |
| current pointer position | — | the pointer position of the team's last applied control state (R-SIM-442) |
| Rogue faction flag, Visitor faction flag | false | marks the two faction teams |
| selected Worker count and centre | 0, none | latched in update slice 0 from the previous tick's accumulation (R-SIM-402) |
| selected Worker accumulators | 0 | filled by the team's selected Workers during the tick |
| team kills | 0 | the number of Workers this team has killed, added by explosions and laser bolts |
| block selection change | false | while true, selection changes are ignored (R-SIM-405) |
| upgrades | formation size `ref.sim.team.formation_size_upgrade` (90); rockets `ref.sim.team.rocket_upgrade` (0) | per-team upgrades: the Leader formation cap ([../02_units/leader_and_formations.md](../02_units/leader_and_formations.md)) and the Rocket Troops upgrade ([../04_supply_crates_powers_and_weapons/crates_and_powers.md](../04_supply_crates_powers_and_weapons/crates_and_powers.md)) |

Kill totals and kill milestones reach the presentation as events (DEC-007).

**R-SIM-400** (MUST) A team's list of ungrouped actors holds every actor of the team that was spawned without an actor group, its bot controller included: Workers, Leaders, Transports, Builders, Collectors and every creature that is not in a group. Strike teams, Bombers, centipedes and Hive ants live inside actor groups. Each special unit registers in the special-unit list when it starts and unregisters when it is removed, so the list never holds a removed actor (DEC-117).

**R-SIM-401** (MUST) A team's current selection changes only through the execution of a command (R-SIM-450) and through the effects that executing a command has inside the same simulation step; no other simulation rule and no presentation or input code writes it (DEC-008, DEC-119).

**R-SIM-402** (MUST) Selected Workers add their position to their team's selected-Worker accumulators as they advance. In update slice 0 the team latches the count and the centre (sum ÷ count) from the previous tick's accumulation and resets the accumulators. With a count of 0 the team has no centre, and the user-interface elements that read it (the route preview, the selection arrows and the Power icon position) show nothing (DEC-653 item 1).

**R-SIM-403** (MUST) Two kinds of per-team state belong to the input layer of the local participant, not to the simulation: the selection-circle centre and radius, and the time of the team's last crowd order, which enforces the crowd-order interval (DEC-032). The input layer writes them for its own team only ([../06_controls_camera_and_commands.md](../06_controls_camera_and_commands.md)), and no simulation rule reads them.

**R-SIM-404** (MUST) A team provides these operations: select (R-SIM-405); register and unregister a special unit (R-SIM-400); ray hit, which returns the first actor hit among its actor groups and then among its ungrouped actors; resolve the current selection to a group, an actor or a structure, or nothing when the handle no longer resolves; create an actor group ([04_containers_and_update_order.md](04_containers_and_update_order.md)); create an actor, which goes to the ungrouped actors when it has no group and is otherwise delegated to its group; count its actors of one kind, or of every kind, over its groups and its ungrouped actors; the team name (R-SIM-406); the Visitor test (R-SIM-406); and elimination (R-SIM-407).

**R-SIM-405** (MUST) Selecting (group g, actor a, structure s) on a team:

1. is ignored entirely while the team's block-selection-change flag is set;
2. sets the team's current selection to (g, a, s);
3. when the selected actor is a Leader, clears the team's current Power;
4. when any of the team's Workers were selected, clears the selected state of all of them;
5. when the selected structure is a turret, sets the turret's fire timer to `ref.sim.team.turret_select_delay` (6 ticks, the 0.5 s countdown of DEC-002);
6. emits presentation events for the team's own participant: the cursor arrows grow (×2.0, [../06_controls_camera_and_commands.md](../06_controls_camera_and_commands.md)), the selection and Power-slot sounds play, and selecting a turret puts the camera into turret-aim mode.

**R-SIM-406** (MUST) A team's displayed name is its lobby name; a bot team's name is prefixed with the bot label `bot.label`, and after game over the departed label `match.team_left` replaces it. A team without a lobby name is named `faction.rogue.name` for the Rogue faction, `faction.visitor.name` for the Visitor faction, and otherwise by its colour id 0 to 7 with the team-colour names `team.colour.green`, `team.colour.red`, `team.colour.yellow`, `team.colour.blue`, `team.colour.orange`, `team.colour.purple`, `team.colour.cyan` and `team.colour.pink` (with the bot label for a bot team); any other colour id shows `ui.team.invalid_name`. A team is the Visitor faction when the map's Visitor-faction option names this team's position, or when its Visitor faction flag is set.

**R-SIM-407** (MUST) Eliminating a team changes the health of every member of its actor groups and of every ungrouped actor by −`ref.sim.team.eliminate_damage_factor` × that actor's health stat (2 × health), stops all of its Powers, and sets every Transit Gate it owns to the no-team value.
