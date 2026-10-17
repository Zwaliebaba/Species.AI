# Verdicts, keep-list and fix-list (continued)

> Continuation of [verdicts_and_fixes.md](verdicts_and_fixes.md) for DEC-120 onwards (the first file holds DEC-001 to DEC-119 and is at the 120 KB limit). Same format; the generated index is [README.md](README.md).

| Id | Title | Status | Affects |
|---|---|---|---|
| DEC-120 | Map values: absent, zero and unlimited are distinct | Resolved | reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT), reference/05_population_flow.md (R-ECON) |
| DEC-121 | Ownership of Brood creatures | Open | reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/creatures.md (R-UNIT) |
| DEC-122 | Bot and deputy actions outside the player command catalogue | Open | reference/07_bots/README.md (R-BOT), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM) |
| DEC-123 | Tutorial for the first playable | Open | reference/11_tutorial_and_help.md (R-TUT), reference/01_skirmish_rules_and_modes/README.md (R-MODE) |

### DEC-120 Map values: absent, zero and unlimited are distinct
- Status: Resolved
- Owner decision: O19 (2026-10-17): explicit values.
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT), reference/05_population_flow.md (R-ECON)
- Specified behaviour: In the typed map schema, every per-map limit (for example the Transport cap and the population cap) has three distinct forms. An **absent** value means the lobby default applies. **0** means none is allowed. **Unlimited** is its own explicit value, never encoded as 0 or as a negative number. Map values override lobby defaults (DEC-065).
- Options: A — explicit values; B — 0 means unlimited, as the reference reads caps.
- Recommendation: A. A map designer can express "none" and "unlimited" without magic numbers, and the schema validator rejects ambiguous values.

### DEC-121 Ownership of Brood creatures
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE-198), reference/02_units/creatures.md (R-UNIT-512)
- Specified behaviour: The creatures of the Brood outcome belong to the team that placed the power.
- Options: A — they belong to the placing team; B — they belong to the Rogue faction.
- Recommendation: decide in playtest: A makes Brood a weapon, B makes it a pure hazard, consistent with the other hostile outcomes (DEC-107).
- Decide by: playtest

### DEC-122 Bot and deputy actions outside the player command catalogue
- Status: Open
- Affects: reference/07_bots/README.md (R-BOT-014, R-BOT-015), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM-435), reference/02_units/worker.md (R-UNIT-111), reference/02_units/leader_and_formations.md (R-UNIT), reference/02_units/transport.md (R-UNIT-402), reference/02_units/strike_team.md (R-UNIT-359), reference/02_units/collector.md (R-UNIT-442), reference/02_units/builder.md (R-UNIT-464), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG-294)
- Specified behaviour: A bot is an authorised command source of its team (DEC-119). It issues the three actions that the player command catalogue expresses, targeting a Power, stopping a Power and aiming a Beam Dish, as catalogue commands that execute at once inside its own step (R-BOT-014). Every other reference bot action has no player command and is a bot-only operation: it applies the named simulation operation directly, inside the bot's step, after checking that the object still exists and belongs to the bot's team (R-BOT-015). The bot-only actions are: a waypoint order for one Worker; Leader orders, waypoints and formations; a Transport's arrival point, loading, unloading and self-destruct target; Strike team moves and attacks; Collector and Builder waypoints; Breach Charge routes; and starting a promotion Power.
- Options: A — keep the bot-only operations, each with its existence-and-team check; B — extend the player command catalogue with a command for each bot-only action, so that players, bots and the deputy act through one validated command set; C — a separate, validated command set for AI controllers, which bots and the deputy share; D — restrict bots to the existing catalogue and drop the behaviours that need other actions.
- Recommendation: B or C, chosen together with command authority (DEC-202) and the AI design of the world game. The deputy defends a settlement whose owner is away with the same simulation as a live battle (vision O2), so every AI action must pass the same validation as a player command, and a server interface for deputy orders (vision O6) needs commands that it can name. This entry is an input to the design-phase steps on AI settlements and the AI runtime (B7 and D7 of the design-phase prompt).
- Decide by: design phase

### DEC-123 Tutorial for the first playable
- Status: Open
- Affects: reference/11_tutorial_and_help.md (R-TUT-001), reference/01_skirmish_rules_and_modes/README.md (R-MODE-083), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP-158), reference/00_reference_overview.md (R-VIS-174), reference/07_bots/05_mode_strategies_and_tutorials.md (R-BOT-150), assets/GameAssets.md
- Specified behaviour: The two reference tutorials are Zone Control matches on their own maps (DEC-051, R-MODE-083, R-MAP-158). The first playable is Spawn Control with two players (owner decision O20), and the tutorial maps and assets are scheduled for the launch, so the first playable has no tutorial.
- Options: A — no tutorial in the first playable; the two reference tutorials follow at launch; B — re-author the first tutorial on a two-player Spawn Control map for the first playable: its gathering, capture, Leader, Supply crate and airstrike lessons carry over, and its two Control Zone lessons and its scores tab are replaced.
- Recommendation: A, because the first playable is the combat balance test bed (vision O8) rather than an onboarding test; B once onboarding is playtested. The owner is asked at the design-phase steps on skirmish and on the tutorial (B9 and C5 of the design-phase prompt).
- Decide by: design phase
