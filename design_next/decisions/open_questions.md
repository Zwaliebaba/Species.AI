# Decision register: open questions

> Status: Reference decisions · Range: DEC-300 to DEC-449 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the decisions that arise from the open questions of the reference library: behaviours whose design intent is not recorded, figures that are estimated rather than established, edge cases whose reachability is unknown, and inferences that a design choice depends on. Each **Open** entry states the faithful reference behaviour as its default, which is what the reference documents specify now, and gives options, a recommendation and the point by which it is decided. The **Resolved** entry records reference facts that are now settled and are stated as rules. Each **Dropped** entry records a group of questions that concern only content the library does not specify, with the reason. Questions that another register already decides are not repeated here: the headline verdicts, simulation-integrity rules, scope verdicts, fix-list decisions and design-critical uncertainties are in `decisions/verdicts_and_fixes.md` (DEC-001 to DEC-117); the networking model, process architecture, tick rate, numeric model, lobby, sessions and services, terrain runtime model and persistence are in `decisions/deferred/` (DEC-200 to DEC-251); engine-technology choices are ADRs under `adr/`. Entries here cite those decisions by number.

## Index

<!-- index:start -->
| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-300](#dec-300-return-of-teams-wiped-out-in-sudden-death) | Return of teams wiped out in sudden death | Open | R-MODE |
| [DEC-301](#dec-301-solar-array-knock-out-in-siege) | Solar Array knock-out in Siege | Open | R-MODE, R-BLDG |
| [DEC-302](#dec-302-transport-unload-order-over-sea) | Transport unload order over sea | Open | R-UNIT, R-CTRL |
| [DEC-303](#dec-303-missile-barrage-arrival-spread) | Missile Barrage arrival spread | Open | R-CRATE |
| [DEC-304](#dec-304-spawn-surge-request-count) | Spawn Surge request count | Open | R-CRATE, R-ECON |
| [DEC-305](#dec-305-essence-grove-orbs-of-team-slots-not-in-play) | Essence Grove orbs of team slots not in play | Open | R-CRATE, R-UNIT, R-ECON |
| [DEC-306](#dec-306-spawn-network-transit-time-and-update-order) | Spawn-network transit time and update order | Open | R-ECON, R-BLDG, R-SIM |
| [DEC-307](#dec-307-essence-orbs-held-by-a-removed-collector) | Essence orbs held by a removed Collector | Open | R-UNIT, R-ECON |
| [DEC-308](#dec-308-orders-to-workers-that-operate-a-port) | Orders to Workers that operate a port | Open | R-CTRL, R-BLDG, R-ECON |
| [DEC-309](#dec-309-global-flag-on-source-nodes) | Global flag on Source Nodes | Open | R-BLDG, R-MAP, DAT |
| [DEC-310](#dec-310-camera-tracking-after-selecting-a-leader-or-a-transport) | Camera tracking after selecting a Leader or a Transport | Open | R-CTRL, CAM |
| [DEC-311](#dec-311-bot-controller-as-a-target) | Bot controller as a target | Open | R-BOT, R-UNIT, R-BLDG |
| [DEC-312](#dec-312-bot-decisions-and-node-recounts-within-a-tick) | Bot decisions and node recounts within a tick | Open | R-BOT, R-SIM |
| [DEC-313](#dec-313-dead-airborne-actors) | Dead airborne actors | Open | R-SIM, R-UNIT |
| [DEC-314](#dec-314-map-list-order-under-ties) | Map list order under ties | Open | R-META, R-MAP |
| [DEC-315](#dec-315-level-design-patterns-of-the-reference-maps) | Level-design patterns of the reference maps | Open | R-MAP, R-MODE |
| [DEC-316](#dec-316-front-end-text-blend-and-page-transition) | Front-end text blend and page transition | Open | UI, RND |
| [DEC-317](#dec-317-voice-set-language) | Voice set language | Open | R-AUD, R-META |
| [DEC-318](#dec-318-sound-triggers-tied-to-the-frame-rate) | Sound triggers tied to the frame rate | Open | R-AUD, AEN |
| [DEC-319](#dec-319-ambience-during-loading-views) | Ambience during loading views | Open | R-AUD, R-META |
| [DEC-320](#dec-320-match-end-pitch-drop) | Match-end pitch drop | Open | R-AUD, AEN |
| [DEC-321](#dec-321-attached-sounds-when-their-emitter-is-removed) | Attached sounds when their emitter is removed | Open | R-AUD, AEN |
| [DEC-322](#dec-322-start-value-of-smoothed-sound-parameters) | Start value of smoothed sound parameters | Open | R-AUD, AEN |
| [DEC-323](#dec-323-loop-keep-alive-polling) | Loop keep-alive polling | Open | R-AUD, AEN, R-MODE |
| [DEC-324](#dec-324-terrain-generator-parameter-semantics-and-biases) | Terrain generator parameter semantics and biases | Open | TER, R-MAP |
| [DEC-325](#dec-325-light-direction-and-the-camera-transform) | Light direction and the camera transform | Open | RND |
| [DEC-326](#dec-326-terrain-grid-overlay) | Terrain grid overlay | Open | RND, TER |
| [DEC-327](#dec-327-structure-render-detail-variants) | Structure render-detail variants | Open | RND, R-BLDG |
| [DEC-328](#dec-328-saturated-team-colours-on-effect-bubbles) | Saturated team colours on effect bubbles | Open | VFX, RND |
| [DEC-329](#dec-329-assets-of-objects-spawned-only-at-run-time) | Assets of objects spawned only at run time | Open | AST |
| [DEC-330](#dec-330-mesh-sockets-referenced-only-by-data) | Mesh sockets referenced only by data | Open | AST |
| [DEC-331](#dec-331-confirmed-facts-the-convert-tag-and-the-victory-celebration) | Confirmed facts: the convert tag and the victory celebration | Resolved | R-CRATE, R-BLDG, R-UNIT, R-MAP, R-AUD |
| [DEC-332](#dec-332-other-editions-packaged-archives-and-evidence-questions) | Other editions, packaged archives and evidence questions | Dropped | R-VIS, AST, BLD |
| [DEC-333](#dec-333-trial-edition-gating-and-credential-files) | Trial-edition gating and credential files | Dropped | R-META, PLT |
| [DEC-334](#dec-334-development-test-and-benchmark-maps) | Development, test and benchmark maps | Dropped | R-MAP, DBG |
| [DEC-335](#dec-335-online-service-listing-fields) | Online-service listing fields | Dropped | R-META |
| [DEC-336](#dec-336-second-graphics-back-end) | Second graphics back end | Dropped | RND, VFX |
| [DEC-337](#dec-337-graphics-driver-cursor-and-glyph-atlas-behaviour) | Graphics-driver, cursor and glyph-atlas behaviour | Dropped | RND, UI |
| [DEC-338](#dec-338-input-stack-implementation-details) | Input-stack implementation details | Dropped | INP |
| [DEC-339](#dec-339-audio-driver-software-mixer-and-effect-processing-details) | Audio driver, software mixer and effect-processing details | Dropped | AEN |
| [DEC-340](#dec-340-data-formats-archives-and-production-tools) | Data formats, archives and production tools | Dropped | DAT, BLD, AST |
| [DEC-341](#dec-341-runtime-library-compiler-and-debug-build-details) | Runtime-library, compiler and debug-build details | Dropped | DBG, PLT |
| [DEC-342](#dec-342-provenance-of-reference-art-and-audio) | Provenance of reference art and audio | Dropped | assets |
<!-- index:end -->

## 1 Match rules and modes

### DEC-300 Return of teams wiped out in sudden death
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: When sudden death eliminates a team, its units are removed but its Spawn Nodes stay in its possession, so the team can in principle produce Workers again and return to play before the tie is broken. How often a wiped team returns in practice is not established.
- Options: A — keep the rule: a wiped team keeps its Spawn Nodes and can return; B — a team wiped out in sudden death also loses its Spawn Nodes, which turn neutral, so it cannot return.
- Recommendation: A, the reference rule. Change to B only if playtests show that such returns prolong sudden death without changing its outcome.
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
- Specified behaviour: When the camera switches to tracking a Leader or a Transport, it keeps a stale orbit centre. This state is reachable only when the player selects a Leader or a Transport while the camera is already tracking a Strike team, which happens in gamepad play.
- Options: A — keep the reference behaviour; B — recompute the orbit centre whenever the tracked unit changes.
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

## 8 Terrain, rendering, effects and assets

### DEC-324 Terrain generator parameter semantics and biases
- Status: Open
- Affects: architecture/components/terrain.md (TER), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/08_maps_and_level_design/map_editor_hooks.md (R-MAP)
- Specified behaviour: The terrain generator's arithmetic is kept (DEC-025). Two of its properties are inferred from the formulas, not from recorded intent: the roughness parameter acts as an exponent, larger values giving smoother terrain, and the gain modifiers approximately normalise amplitude across the generator's shape parameters. Two are arithmetic facts whose deliberate use is unknown: generation method 2 never chooses two of its directions and almost never chooses two others, and the smooth flatten type divides a sum of five terms by six.
- Options: A — keep the arithmetic exactly, biases included; B — keep A, and also offer unbiased variants of generation method 2 and of the smooth flatten type in the map editor; C — replace the biased forms with unbiased ones.
- Recommendation: B. The biased forms keep the reference look for terrains that are meant to match it, and new maps can use the unbiased variants.
- Decide by: design phase

### DEC-325 Light direction and the camera transform
- Status: Open
- Affects: architecture/components/renderer/view_lighting_and_fog.md (RND)
- Specified behaviour: Map lights are set before the camera transform of each frame, so during fast camera turns the light direction lags the view by one frame. The size of the visible lag has not been measured.
- Options: A — reproduce the one-frame lag; B — lights and view use the transform of the same frame.
- Recommendation: B. The lag is not part of the look.
- Decide by: design phase

### DEC-326 Terrain grid overlay
- Status: Open
- Affects: architecture/components/renderer/world_renderers.md (RND), architecture/components/terrain.md (TER)
- Specified behaviour: The terrain grid overlay maps a texture of 2 × 2 sub-cells with three diagonals once onto each height-field cell, so its lines do not coincide with the edges of the terrain triangles. Its design intent is not recorded.
- Options: A — keep the stylised overlay as drawn; B — draw the overlay along the actual triangle edges.
- Recommendation: A. The overlay reads as a stylised grid, not as a wireframe; confirm the look visually.
- Decide by: design phase

### DEC-327 Structure render-detail variants
- Status: Open
- Affects: architecture/components/renderer/options_and_budgets.md (RND), architecture/components/renderer/world_renderers.md (RND), reference/03_buildings_and_world_objects/README.md (R-BLDG)
- Specified behaviour: The structure-detail setting switches branches in the drawing of structure ports, of the translucent layers of Barrier Emitters and of the translucent layers of spawn-network structures, at detail level 1 and at levels below 3. The exact visual change of each branch is not established.
- Options: A — reproduce each branch as the reference draws it; B — define the detail variants anew as part of the renderer's quality levels, with the same intent: fewer translucent layers at lower detail.
- Recommendation: B. All art is new, and the renderer sets its own budgets.
- Decide by: design phase

### DEC-328 Saturated team colours on effect bubbles
- Status: Open
- Affects: architecture/components/animation_and_vfx.md (VFX), architecture/components/renderer/visual_language.md (RND)
- Specified behaviour: The Shield bubble and the Visitor-faction bubble are drawn with team colours whose channels are saturated to fully on or fully off, because 8-bit channel values are passed where unit fractions are expected. Whether this look is intended is not recorded.
- Options: A — keep the saturated colours as the look of both bubbles; B — draw the bubbles in the team colour palette, with their saturation set by art direction.
- Recommendation: B, after a visual comparison of both looks.
- Decide by: design phase

### DEC-329 Assets of objects spawned only at run time
- Status: Open
- Affects: assets/asset_manifest.csv, assets/README.md, architecture/components/asset_pipeline_and_resources.md (AST)
- Specified behaviour: Fifty-one reference assets belong to objects that no reference map places. Some of them are spawned at run time by Supply crate outcomes or mode rules; which ones is not established. Until it is, every one of them is treated as needed.
- Options: A — recreate all 51 assets; B — recreate only the assets whose objects a spawn analysis of reference/03 and reference/04 confirms.
- Recommendation: B, with the spawn analysis made in the asset-manifest pass. Assets of content that the library does not specify are not recreated.
- Decide by: design phase

### DEC-330 Mesh sockets referenced only by data
- Status: Open
- Affects: architecture/components/asset_pipeline_and_resources.md (AST), assets/asset_manifest.csv
- Specified behaviour: Some reference meshes carry sockets that no rule names directly: island markers, Transit Gate route markers and float markers. Whether code reaches them through a name pattern is not established, so every socket kind that the reference meshes carry is treated as used.
- Options: A — every socket kind of the reference meshes is part of its asset class's socket schema (ADR-0021); B — only socket kinds with a confirmed consumer are part of the schemas.
- Recommendation: B. The asset pipeline component lists the consumer of each socket kind, and a kind without a consumer is left out.
- Decide by: design phase

## 9 Confirmed reference facts

### DEC-331 Confirmed facts: the convert tag and the victory celebration
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/02_units/worker.md (R-UNIT), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/10_audio_design.md (R-AUD)
- Specified behaviour: These behaviours are settled and are specified as rules. A Worker's convert tag, which turns gun-turret shots into convert lasers, is set only by the Convert power: placing the power tags the placing team's own Workers within the power effect range; the power lasts 30 s and ends early when no tagged Worker remains; a Worker clears its tag once the power has ended. A map with no primary objectives receives one objective that never completes, and the victory celebration of idle Workers starts only when every objective is complete, so it never starts on such a map.
- Options: none.
- Recommendation: Resolved: each fact is confirmed, and the owning reference document states it as a rule.

## 10 Content outside the reference

### DEC-332 Other editions, packaged archives and evidence questions
- Status: Dropped
- Affects: reference/00_reference_overview.md (R-VIS), architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/build_tooling_and_distribution.md (BLD)
- Specified behaviour: Not specified. The library does not record what other editions, regional and partner versions, patches, test releases or packaged archives of the reference contained: the contents of the extra and branding archives; whether the packaged sound archives match the sound definitions; which languages each edition carried; whether a packaged edition carried the map-list file that two achievement conditions depend on; the inclusion list of a partner edition; the map list of a test release; a regional text patch; the build that a test-release note on random-number tracking refers to; and how individual front-end pages are classified as reachable or not.
- Options: none.
- Recommendation: Dropped: Species sets no parity target against other editions (DEC-005) and authors all content new; the achievement conditions are redesigned (DEC-047), language completeness is DEC-099, and the live front-end pages are those of DEC-026.

### DEC-333 Trial-edition gating and credential files
- Status: Dropped
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), architecture/components/platform_layer.md (PLT)
- Specified behaviour: Not specified. The trial-edition restrictions and how they are enforced, the credential file that start-up reads and the one that key entry writes, the trial map list that an online service could send, a trial game-size cap, and whether a trial player can host are outside the reference.
- Options: none.
- Recommendation: Dropped: these concern only trial gating and credential keys, which Species replaces with platform entitlements (DEC-248).

### DEC-334 Development, test and benchmark maps
- Status: Dropped
- Affects: reference/08_maps_and_level_design/README.md (R-MAP), architecture/components/diagnostics_test_and_performance.md (DBG)
- Specified behaviour: Not specified. Maps that declare no mode, namely the sandbox, benchmark and stress maps, are never listed in the front end, so the campaign-only structures they contain are unreachable in a match. How the benchmark and stress builds started those maps, and the content of ten further map ids reserved for downloadable maps, are outside the reference.
- Options: none.
- Recommendation: Dropped: no match can select these maps, and their structures are campaign content (DEC-052). Species stress and benchmark runs are part of the diagnostics design (DEC-041, DEC-228).

### DEC-335 Online-service listing fields
- Status: Dropped
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: Not specified. Whether the reference's directory service could add a listing field that makes the password filter hide sessions, and whether its protocol could carry leaderboard or rating data, are outside the reference.
- Options: none.
- Recommendation: Dropped: the reference's own online services are not part of the library; session discovery is DEC-245 and DEC-249, and leaderboards are DEC-047.

### DEC-336 Second graphics back end
- Status: Dropped
- Affects: architecture/components/renderer/README.md (RND), architecture/components/animation_and_vfx.md (VFX)
- Specified behaviour: Not specified. Whether the reference's alternative graphics build compiles, its dynamic-water look, its shader register allocation, its automatic mip generation, its doubled projection depth range, its deform render-target handling, its effect on floating-point precision, and its debris and deform look are outside the reference.
- Options: none.
- Recommendation: Dropped: no match runs on that build; the Species renderer is specified by its component and ADRs (DEC-059, ADR-0001).

### DEC-337 Graphics-driver, cursor and glyph-atlas behaviour
- Status: Dropped
- Affects: architecture/components/renderer/README.md (RND), architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI)
- Specified behaviour: Not specified. The transparency of the cursor background on another operating system's drivers, the code-point range that one glyph-atlas page of the reference claims, and how period graphics drivers treat edge clamping and mip filtering of textures without mip levels are outside the reference.
- Options: none.
- Recommendation: Dropped: these are properties of the reference's drivers and font atlases. Species uses its own glyph cache (ADR-0034) and texture pipeline (ADR-0017, ADR-0018) on its own graphics back end.

### DEC-338 Input-stack implementation details
- Status: Dropped
- Affects: architecture/components/input.md (INP)
- Specified behaviour: Not specified. The controller dead-zone and trigger-threshold constants built into the reference, the overflow behaviour of its trigger-byte conversion, the character conversion of its binding descriptions, and whether an extra input update between its loops can swallow a button edge are outside the reference.
- Options: none.
- Recommendation: Dropped: these are details of the reference's input implementation. Species defines its dead zones and thresholds (ADR-0029), and DEC-096 requires that no press is ever lost.

### DEC-339 Audio driver, software mixer and effect-processing details
- Status: Dropped
- Affects: architecture/components/audio_engine.md (AEN)
- Specified behaviour: Not specified. The driver-level panning and distance roll-off of the reference's positional voices, its software mixer and that mixer's restart, the quality parameter and the assertion of its effect processing, the start latency that depends on each driver's buffer cursors, the sample-rate mismatch of its built-in effects, the case-insensitive look-up of samples in its archives, and whether its effect processing was ever enabled for players are outside the reference.
- Options: none.
- Recommendation: Dropped: the normative distance and panning laws are fixed in reference/10, and the Species audio engine is specified by its component and ADRs (ADR-0022 to ADR-0027).

### DEC-340 Data formats, archives and production tools
- Status: Dropped
- Affects: architecture/components/data_schemas.md (DAT), architecture/components/build_tooling_and_distribution.md (BLD), architecture/components/asset_pipeline_and_resources.md (AST)
- Specified behaviour: Not specified. The version and compression method of the reference's archive tool, the provenance of its crash-report tool, what its panel-animation tool writes, the forward-axis sign convention of its mesh exporter, the behaviour of its absent tools (updater, copy helper, profile locator, upload client, data-build and release scripts), the role of a memory-log sorting tool, and the name of the linker map file are outside the reference.
- Options: none.
- Recommendation: Dropped: Species defines its own pack files (ADR-0058), data schemas (ADR-0061), mesh and axis conventions (ADR-0016, ADR-0019, ADR-0021), crash reporting (ADR-0065) and tools; no reference data or tool output is imported.

### DEC-341 Runtime-library, compiler and debug-build details
- Status: Dropped
- Affects: architecture/components/diagnostics_test_and_performance.md (DBG), architecture/components/platform_layer.md (PLT)
- Specified behaviour: Not specified. How the reference's runtime library handles a rethrow without an active exception inside a release assertion, which system-information lines its benchmark capture records, whether its crash-time stack walk works in optimised builds, what it prints for malformed format strings in two development windows, whether its menu-driving test bed builds, and whether its compiler zero-initialises the camera-view record are outside the reference.
- Options: none.
- Recommendation: Dropped: these are properties of the reference's toolchain and debug builds. Species defines crash handling (ADR-0065), its test harness and soak test (DEC-041, ADR-0069, ADR-0070) and its own data types.

### DEC-342 Provenance of reference art and audio
- Status: Dropped
- Affects: assets/README.md, assets/art_and_audio_briefs.md
- Specified behaviour: Not specified. Whether the reference's samples were recorded in-house or licensed, whether an unused help background is referenced indirectly (it is not), and which tool made the map thumbnails are outside the reference.
- Options: none.
- Recommendation: Dropped: every Species asset is authored new, map thumbnails are generated by the engine, and all reference art and audio are treated as intellectual property of the reference.
