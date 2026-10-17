# Decision register: open questions

> Status: Reference decisions · Range: DEC-300 to DEC-449 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the decisions that arise from the open questions of the reference library: behaviours whose design intent is not recorded, figures that are estimated rather than established, edge cases whose reachability is unknown, and inferences that a design choice depends on. Each **Open** entry states the faithful reference behaviour as its default, which is what the reference documents specify now, and gives options, a recommendation and the point by which it is decided. The **Resolved** entry records reference facts that are now settled and are stated as rules. Each **Dropped** entry records a group of questions that concern only content the library does not specify, with the reason. Questions that another register already decides are not repeated here: the headline verdicts, simulation-integrity rules, scope verdicts, fix-list decisions and design-critical uncertainties are in `decisions/verdicts_and_fixes.md` (DEC-001 to DEC-117); the networking model, process architecture, tick rate, numeric model, lobby, sessions and services, terrain runtime model and persistence are in `decisions/deferred/` (DEC-200 to DEC-251); engine-technology choices are ADRs under `adr/`. Entries here cite those decisions by number.

## Index

<!-- index:start -->
<!-- index:end -->

## 1 Match rules and modes

### DEC-300 Recovery of teams wiped out in sudden death
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: When sudden death eliminates a team, its units are removed but its Spawn Nodes stay in its possession, so the team can in principle produce Workers again and return to play before the tie is broken. How often a wiped team recovers in practice is not established.
- Options: A — keep the rule: a wiped team keeps its Spawn Nodes and can recover; B — a team wiped out in sudden death also loses its Spawn Nodes, which turn neutral, so it cannot return.
- Recommendation: A, the reference rule. Change to B only if playtests show that recoveries prolong sudden death without changing its outcome.
- Decide by: playtest

### DEC-301 Solar Array knock-out in Siege
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG)
- Specified behaviour: In Siege, when a Solar Array is captured from the team that owned it at match start, its ports are removed, so no Worker can operate it for the rest of the match. The rule is specified; its design intent is not recorded.
- Options: A — keep the knock-out: capturing a defender's Solar Array takes it out of play; B — the Solar Array keeps its ports after capture and can be recaptured like any other structure.
- Recommendation: A. It works as a deliberate attacker objective with a permanent effect, which suits the one-sided structure of Siege; confirm in playtest that defenders can still hold out once their arrays fall.
- Decide by: playtest

## 2 Units, crates and weapons

### DEC-302 Transport unload order over sea
- Status: Open
- Affects: reference/02_units/transport.md (R-UNIT), reference/06_controls_camera_and_commands.md (R-CTRL)
- Specified behaviour: A Transport cannot unload its passengers while it is over sea. An unload order given there is not registered, and the Transport falls back to its idle state. This inference is design-critical and has not been confirmed in play.
- Options: A — keep: an unload order over sea is ignored and the Transport goes idle; B — the Transport moves to the nearest shore point and unloads there; C — the order is rejected with player feedback, and the Transport keeps its previous state.
- Recommendation: C. It keeps the rule that a Transport never unloads at sea and removes the silent loss of the Transport's previous orders.
- Decide by: playtest

### DEC-303 Missile Barrage arrival spread
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE)
- Specified behaviour: The launch of each barrage missile is compensated for its distance to the target by a term of distance ÷ 100. With a mean flight speed of about 47 wu/s this does not equalise arrival times: missiles from nearer launch points strike first. The arrival times are derived by calculation on flat terrain (DEC-111); this inference is design-critical.
- Options: A — keep the staggered arrival; B — equalise arrival, so every missile of one barrage strikes at the same tick; C — an explicit, authored stagger with a tuning key.
- Recommendation: A. The stagger reads as a rolling barrage, and the outcome's damage balance is tuned under it. Confirm the spread on real terrain in the first prototype, and move to C only if the spread varies too much between maps.
- Decide by: playtest

### DEC-304 Spawn Surge request count
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/05_population_flow.md (R-ECON)
- Specified behaviour: Spawn Surge gives each Spawn Node of the opening team 50 or 51 extra production requests. Which of the two applies depends on whether the Supply crate opens before or after that Spawn Node's update within the same tick.
- Options: A — keep the order-dependent count; B — a fixed count of 50 requests per Spawn Node; C — a fixed count of 51.
- Recommendation: B. The count no longer depends on the update order, and the difference from the reference is at most one Worker per Spawn Node.
- Decide by: design phase

### DEC-305 Essence Grove orbs of team slots not in play
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/essence_orbs.md (R-UNIT), reference/05_population_flow.md (R-ECON)
- Specified behaviour: The Essence Grove creates Essence orbs whose team is drawn uniformly from team slots 0 to 3, whatever the map's player count. On 2-player and 3-player maps some of these orbs therefore belong to team slots that are not in play. No rule treats them specially, and what becomes of them (collection, expiry, any effect on scoring) is not established. The Allegiance Scrambler has no such case: before a match starts, the host fills every empty slot below the map's player count with a bot team, so its draw over the map's player count always names a team in play.
- Options: A — the reference behaviour; B — draw each orb's team from the teams in play.
- Recommendation: B. Orbs that belong to no playing team have no use in the match.
- Decide by: design phase

## 3 Population flow and structures

### DEC-306 Spawn-network transit time and update order
- Status: Open
- Affects: reference/05_population_flow.md (R-ECON), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM)
- Specified behaviour: A transit orb that passes along h Relays reaches its Spawn Node after between 9h + 1 and 10h + 1 ticks. A hand-off to a structure that comes later in the structure update order advances again in the same tick, so the exact transit time on a map depends on the order in which its structures load. DEC-002 states the upper bound, 10h + 1.
- Options: A — keep the order-dependent transit time, with the structure order fixed as the stable structure-index order (DEC-016); B — a fixed transit time of 10h + 1 ticks, independent of the update order; C — a fixed transit time of 9h + 1 ticks.
- Recommendation: B. It matches the figure the population flow is described with (DEC-002, DEC-018), removes the dependence on load order, and changes timing by at most one tick per hop.
- Decide by: design phase

### DEC-307 Essence orbs held by a removed Collector
- Status: Open
- Affects: reference/02_units/collector.md (R-UNIT), reference/02_units/essence_orbs.md (R-UNIT), reference/05_population_flow.md (R-ECON)
- Specified behaviour: A Collector holds Essence orbs in the harvested and collected states. When a Collector is removed by a path that does not release its orbs, such as the termination of its power or a stop of all of a team's powers, the orbs stay in those states. The harvested state has no timer and no owner check, so such an orb can stay in place indefinitely and keep its slot. Elimination damage cannot remove a Collector, because a Collector is invulnerable unless it is made vulnerable. Whether such orbs occur in play is not established.
- Options: A — the reference behaviour; B — every removal path of a Collector releases its orbs to the free state, each with its remaining lifetime.
- Recommendation: B. A stranded orb has no play value and occupies a slot until the match ends.
- Decide by: design phase

### DEC-308 Orders to Workers that operate a port
- Status: Open
- Affects: reference/06_controls_camera_and_commands.md (R-CTRL), reference/03_buildings_and_world_objects/README.md (R-BLDG), reference/05_population_flow.md (R-ECON)
- Specified behaviour: Selection and crowd orders apply as reference/06 specifies, with no special case for a Worker that is operating a port. Whether a player can thereby pull an operating Worker off its port, and so un-man a structure on purpose, is not established.
- Options: A — orders reach operating Workers, so a player can un-man a structure deliberately; B — operating Workers ignore crowd orders until they leave their port on their own.
- Recommendation: Establish first what the reference rules produce. If un-manning is possible, keep A as a deliberate tactic, because it lets players choose which structures their Workers hold; choose B only if playtests show frequent accidental un-manning.
- Decide by: playtest

### DEC-309 Global flag on Source Nodes
- Status: Open
- Affects: reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas.md (DAT)
- Specified behaviour: A map definition can mark a Source Node as global, and several reference maps do. In a match the flag changes no rule, because the only behaviour that reads it belongs to campaign content (DEC-052); it affects only how the Source Node is drawn. That no other match rule reads the flag is inferred, not confirmed.
- Options: A — keep the flag as a presentation-only attribute of the map definition; B — remove the flag from Species map data, and express any visual variant of a Source Node as an explicit asset choice.
- Recommendation: B. Every map is authored new, and a flag that suggests a world-level link but only changes drawing invites misuse.
- Decide by: design phase

## 4 Controls, camera and bots

### DEC-310 Camera tracking after selecting a Leader or a Transport
- Status: Open
- Affects: reference/06_controls_camera_and_commands.md (R-CTRL), architecture/components/camera_and_sequences.md (CAM)
- Specified behaviour: When the camera tracks a Leader or a Transport, it orbits a centre that is not updated as the tracked unit moves. This state is reachable only when the player selects a Leader or a Transport while the camera is already tracking a Strike team, which happens in gamepad play.
- Options: A — keep the reference behaviour; B — recompute the orbit centre whenever the tracked unit changes and as it moves.
- Recommendation: B. A stale orbit centre has no play value, and the fix does not change any other camera rule (DEC-024).
- Decide by: design phase

### DEC-311 Bot controller as a target
- Status: Open
- Affects: reference/07_bots.md (R-BOT), reference/02_units/worker.md (R-UNIT), reference/02_units/creatures_and_hazards.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG)
- Specified behaviour: Each bot team has a bot controller, an immortal actor that sits in the actor grid; in the reference it stands at (20, 0, 20), and DEC-087 places it inside the map instead. The Worker threat search does not exclude it, so enemy Workers near it can lock onto a target they cannot kill. Whether the target searches of turrets, ants and other creatures exclude it is not established.
- Options: A — the reference behaviour; B — no target search ever selects a bot controller, and it still counts towards its team's population (DEC-079); C — the bot controller is not a spatial actor at all.
- Recommendation: B for the reference skirmish rules, because it keeps the population count unchanged. C is the cleaner model for the world game, where settlement AI and the deputy need no body in the world.
- Decide by: design phase

### DEC-312 Bot decisions and node recounts within a tick
- Status: Open
- Affects: reference/07_bots.md (R-BOT), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM)
- Specified behaviour: The bot controller advances in the update slice that its position in the actor pool gives it, assumed to be slice 0. The order within one tick between its decisions and the recount of bot navigation nodes is not established; it changes a bot's reaction time by at most one tick.
- Options: A — leave the order to the bot controller's pool position; B — state the order explicitly: bot decisions run after the bot navigation node recount of the same tick.
- Recommendation: B, stated in the update order of reference/12, so that bot behaviour does not depend on pool layout (DEC-016).
- Decide by: design phase

## 5 Simulation

### DEC-313 Dead airborne actors
- Status: Open
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/02_units/README.md (R-UNIT)
- Specified behaviour: An actor that dies while airborne skips the landing test and sinks through the terrain while it fades. Exact actor-grid queries skip dead actors, but the cell-based approximate counts include them (DEC-094), so a falling corpse still counts where approximate counts are used.
- Options: A — keep both behaviours; B — every count excludes dead actors, and the corpse still sinks while it fades; C — as B, and a dead airborne actor falls to the terrain before it fades.
- Recommendation: B. Excluding dead actors from counts follows the recommendation of DEC-094; whether the corpse sinks or lands is a presentation choice with no effect on play, so the reference look is kept.
- Decide by: design phase

## 6 Maps and front end

### DEC-314 Map list order under ties
- Status: Open
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), reference/08_maps_and_level_design/README.md (R-MAP)
- Specified behaviour: The front-end map lists are sorted. Entries that tie on the sort key keep an order that depends on the order of the pack index, through an off-by-one in the ordered insertion.
- Options: A — the reference behaviour; B — ties are broken by a stable secondary key, the map id.
- Recommendation: B. The list order then depends only on map data.
- Decide by: design phase

### DEC-315 Level-design patterns of the reference maps
- Status: Open
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/01_skirmish_rules_and_modes/zone_control.md (R-MODE), reference/01_skirmish_rules_and_modes/relic_run.md (R-MODE)
- Specified behaviour: Reference maps use these patterns, whose design intent is not recorded: concentric Control Zones in Zone Control; duplicate Relic Spawn Zones in Relic Run; Zone Control maps whose Control Zones start owned by a team; key and fill pairs of map lights; and twin terrains, where two maps are built on one terrain. The map schema and the rules accept each of them.
- Options: A — every pattern is a supported level-design tool: the map schema and the per-mode validation rules accept each, and reference/08 documents it for map authors; B — validation rejects the patterns whose effect is unclear: concentric Control Zones, pre-owned Control Zones and duplicate Relic Spawn Zones.
- Recommendation: A. Confirm each pattern's effect on play when the first Species maps that use it are playtested.
- Decide by: design phase

### DEC-316 Front-end text blend and page transition
- Status: Open
- Affects: architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI), architecture/components/ui_framework/pages_and_screen_recipes.md (UI), architecture/components/renderer/visual_language.md (RND)
- Specified behaviour: Ink text is drawn with a blend of source alpha and one minus source colour over the menu highlight gradient and the fill boxes; only the blend arithmetic is established, not the on-screen look. Every page change plays a 1.0 s slide transition of a captured frame: with the transition's progress value f, the curve value is c = 3.6 × f, the slide offset is c⁶ × the screen width and the alpha is 1 − 0.2 × c. Which frame the transition captures, the previous frame or undefined back-buffer content, depends on the graphics driver.
- Options: A — reproduce the blend and the transition, with an explicit capture of the last presented frame; B — design the text style and the page transition anew.
- Recommendation: A. The transition curve is fully specified and is part of the front-end feel; capturing the last presented frame removes the driver dependence. Confirm the ink-text look visually in the Species working colour space (ADR-0013).
- Decide by: design phase

## 7 Audio

### DEC-317 Voice set language
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), assets/art_and_audio_briefs.md
- Specified behaviour: Voice events fire for unit acknowledgements, attack and retreat orders, Supply crate outcomes, the game-over announcement, and the win and loss announcements. Whether the reference voice content consists of intelligible words, which would need localisation, is not established. Every Species voice line is authored new.
- Options: A — worded voice lines, localised through the string tables; B — non-verbal vocalisations only; C — non-verbal crowd voices with worded, localised announcer lines.
- Recommendation: C. Crowd acknowledgements repeat constantly and need no translation, while the announcements carry information that players must understand.
- Decide by: design phase

### DEC-318 Sound triggers tied to the frame rate
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: The flash sounds of grenades and airstrike bombs and the tick sounds of the match clock are triggered from drawing code, so how often they play can depend on the frame rate, and they can be skipped while the HUD is hidden.
- Options: A — the reference behaviour; B — these sounds are presentation events issued at the tick where the flash or the clock change happens (DEC-007), independent of drawing.
- Recommendation: B. What players hear then no longer depends on their frame rate or HUD setting.
- Decide by: design phase

### DEC-319 Ambience during loading views
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: The front-end sounds are loaded before the first loading view, but no lobby ambience plays during a loading view, because its trigger is inactive.
- Options: A — silent loading views; B — an ambience bed during loading views.
- Recommendation: B with new ambience if loading views stay long enough to be noticed; otherwise A. The choice follows the loading design of DEC-226.
- Decide by: design phase

### DEC-320 Match-end pitch drop
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: When a match ends, the pitch of game sounds follows the square root of a smoothed game speed as the speed falls to 0.1×, which sounds like a tape slowing down. Its audible character has not been checked.
- Options: A — keep the effect; B — replace it with the designed match-end stings alone (DEC-049).
- Recommendation: A, combined with the new match-end stings and confirmed by a listening test.
- Decide by: playtest

### DEC-321 Attached sounds when their emitter is removed
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: A sound attached to an actor or a structure stops without its release on the first tick after its emitter leaves the match world. A graceful path, in which the sound becomes stationary at its last position and ends with its release, exists but almost never runs. Which behaviour is intended is not recorded; the question is design-critical for the sound of deaths and removals.
- Options: A — the hard cut; B — the graceful path for every removal: the sound stays at the emitter's last position and plays its release.
- Recommendation: B, confirmed by a listening test.
- Decide by: playtest

### DEC-322 Start value of smoothed sound parameters
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: A sound parameter that is smoothed once per audio update starts from its definition's construction value (pitch 1.0) and is evaluated only two or three times before playback starts, so authored random pitch ranges are compressed towards 1.0 at the start of a note: a range of 0.75 to 1.35 becomes about 0.88 to 1.16.
- Options: A — keep the compression; B — a smoothed parameter starts at its first evaluated value, so the authored range is heard as authored.
- Recommendation: B, confirmed by a listening test against the intended ranges of the new sound set.
- Decide by: playtest

### DEC-323 Loop keep-alive polling
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN), reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE)
- Specified behaviour: Game code keeps looping sounds alive by polling how many instances of a sound are playing for an emitter, and starts the loop again when it finds none. The poll cannot see an instance's emitter until an audio update has resolved it, and it never sees non-positional or stationary instances, so a loop can be started twice. The Launch Vessel loops of Launch Race are affected; the question is design-critical for that mode's sound.
- Options: A — the reference polling; B — the audio engine records the emitter when a sound is triggered, and loops are started and stopped by explicit events, so a loop never runs twice for one emitter.
- Recommendation: B. It removes duplicate loops by construction and fits the event model of DEC-007.
- Decide by: design phase
