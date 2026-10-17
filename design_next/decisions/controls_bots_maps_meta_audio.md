# Decision register: controls, bots, maps, meta systems and audio

> Status: Reference decisions · Range: DEC-550 to DEC-649 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file holds the decisions on controls and powers, bots, maps and level data, and audio design that the other register files do not already make. Each entry states the behaviour the reference documents specify now. A **Resolved** entry states the corrected rule that the documents apply; an **Open** entry states the faithful reference behaviour as the default, with a recommendation; a **Dropped** entry records content that the library does not specify. Most decisions in these areas are in [verdicts and fixes](verdicts_and_fixes.md): the subsystem verdicts for bots (DEC-022), controls (DEC-023), the camera (DEC-024), maps (DEC-025), the front end (DEC-026) and audio (DEC-027); the correction lists for bots (DEC-087), input (DEC-096) and audio (DEC-097); and the hook-only features (DEC-045 to DEC-051). Front-end, options and localisation decisions are DEC-026, DEC-044, DEC-067, DEC-099 and DEC-100, so this file holds no front-end entry of its own. Lobby, session, identity and client-storage topics are in [deferred/numerics_lobby_terrain_services.md](deferred/numerics_lobby_terrain_services.md) (DEC-243 to DEC-250); command authority and content identity are in [deferred/networking.md](deferred/networking.md) (DEC-202, DEC-204). The open questions of these areas are recorded in `open_questions.md`.

## Index

<!-- index:start -->
| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-550](#dec-550-power-cancellation-in-the-tutorials) | Power cancellation in the tutorials | Open | R-TUT, R-CRATE, R-CTRL |
| [DEC-551](#dec-551-units-that-outlive-their-power-slot) | Units that outlive their Power slot | Open | R-CRATE, R-CTRL, R-UNIT |
| [DEC-552](#dec-552-power-kinds-without-targeting-behaviour) | Power kinds without targeting behaviour | Dropped | R-CRATE, R-CTRL |
| [DEC-553](#dec-553-default-bindings-that-rely-on-lenient-parsing) | Default bindings that rely on lenient parsing | Resolved | INP, R-CTRL |
| [DEC-554](#dec-554-input-devices-key-names-and-binding-drivers) | Input devices, key names and binding drivers | Resolved | INP, R-CTRL |
| [DEC-555](#dec-555-a-crowd-order-for-an-empty-group-does-nothing) | A crowd order for an empty group does nothing | Resolved | R-CTRL, R-SIM |
| [DEC-556](#dec-556-power-slot-order) | Power slot order | Open | R-CTRL, R-CRATE, UI |
| [DEC-557](#dec-557-bot-troop-movement-between-navigation-nodes) | Bot troop movement between navigation nodes | Open | R-BOT, R-SIM |
| [DEC-558](#dec-558-bot-controller-of-the-rogue-and-visitor-factions) | Bot controller of the Rogue and Visitor factions | Open | R-BOT, R-MODE |
| [DEC-559](#dec-559-bot-module-structure) | Bot module structure | Open | R-BOT |
| [DEC-560](#dec-560-start-positions-are-typed-map-records) | Start positions are typed map records | Resolved | R-MAP, DAT, CAM, R-MODE |
| [DEC-561](#dec-561-per-mode-map-validation) | Per-mode map validation | Resolved | R-MAP, R-BLDG, DAT, BLD |
| [DEC-562](#dec-562-map-lighting-and-weather-presets) | Map lighting and weather presets | Open | R-MAP, RND, VFX, DAT |
| [DEC-563](#dec-563-map-thumbnails-and-the-map-index-are-content-build-outputs) | Map thumbnails and the map index are content-build outputs | Resolved | R-MAP, R-META, AST, BLD |
| [DEC-564](#dec-564-laser-fire-is-audible-and-selection-sounds-stay-local) | Laser fire is audible, and selection sounds stay local | Resolved | R-AUD, R-CTRL |
| [DEC-565](#dec-565-sounds-for-the-other-silent-gameplay-events) | Sounds for the other silent gameplay events | Open | R-AUD |
| [DEC-566](#dec-566-listener-position-and-distance-limits) | Listener position and distance limits | Open | R-AUD, AEN |
| [DEC-567](#dec-567-dry-mix-or-applied-effects) | Dry mix or applied effects | Open | R-AUD, AEN |
| [DEC-568](#dec-568-sample-group-choice-and-linked-parameters) | Sample-group choice and linked parameters | Resolved | R-AUD, AEN |
| [DEC-569](#dec-569-game-speed-pitch-bend) | Game-speed pitch bend | Resolved | R-AUD, AEN |
| [DEC-570](#dec-570-buses-priority-classes-and-voice-budgets) | Buses, priority classes and voice budgets | Resolved | R-AUD, R-META, AEN |
<!-- index:end -->

## 1 Controls and powers

### DEC-550 Power cancellation in the tutorials
- Status: Open
- Affects: reference/11_tutorial_and_help.md (R-TUT), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: During the first tutorial, and during the second tutorial until its bot is allowed to spawn, no power can be terminated. A one-shot power that ends by terminating itself once its effect fires (Missile Barrage, Swarm, Essence Grove, Blight Grove, Hive, Brood and meteor shower) therefore stays Waiting after firing and can be fired again. The airstrike power is unaffected by the block: it stops on the tick it is targeted, as outside the tutorials (R-CRATE-017, R-CRATE-021).
- Options: A — the reference rule: the tutorial block covers every termination, so one-shot powers can be fired repeatedly; B — the block covers only the player's cancel command, and a one-shot power always ends when its effect fires; C — no termination block in the tutorials.
- Recommendation: B. Repeated firing is a side effect of the block, not a lesson, and it hands the tutorial player powers that no step granted. Where a lesson needs a power again, the tutorial step graph grants it explicitly as a sequenced simulation event (DEC-103). Tutorials are designed anew (DEC-051), so the choice belongs with that design.
- Decide by: design phase

### DEC-551 Units that outlive their Power slot
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/06_controls_camera_and_commands/ (R-CTRL), reference/02_units/strike_team.md (R-UNIT), reference/02_units/transport.md (R-UNIT)
- Specified behaviour: Cancelling a power frees its Power slot even when the unit or structure that the power placed refuses to stop, so a turret placed by a power, or in a tutorial a Transport, can stay in the world with no slot; outside a tutorial a cancelled Transport is destroyed (R-UNIT-370, R-CRATE-020). A Strike team that passes through a Beam Dish is registered again as a new power: it receives a new power id and the last slot position, so the key that selects it changes. If that registration fails because the team's slot count has dropped to two on elimination (DEC-033), the Strike team carries on with no power: it has no life limit, occupies no slot and cannot be cancelled from the power bar. A Strike team's life limit counts down only while it is above zero, and Strike teams that a map places as instant units have no life limit.
- Options: A — the reference rules; B — a placed unit's life is bound to its slot: a cancel that cannot stop the unit fails and the slot stays occupied, and a Beam Dish transit keeps the Strike team's power id, slot position and remaining life; C — a cancel always removes the placed unit.
- Recommendation: B. Units without a slot bypass the six-slot limit (DEC-033), and an eliminated team can keep a Strike team with no life limit. Keeping the power id across a transit also keeps key selection stable (DEC-556). Strike teams placed as instant units keep their unlimited life, which is a map rule.
- Decide by: design phase

### DEC-552 Power kinds without targeting behaviour
- Status: Dropped
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: Not specified. Two power kinds have an empty targeting behaviour, so a power of either kind would stay active for ever: the duel-vehicle power (DEC-057) and a second power kind that no match can grant.
- Options: none.
- Recommendation: Dropped: no match can grant either power kind.

### DEC-553 Default bindings that rely on lenient parsing
- Status: Resolved
- Affects: architecture/components/input.md (INP), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: The default binding set states each binding with its effective meaning, and the binding parser rejects a condition word it does not know (DEC-096). Three default bindings are press bindings on the gamepad triggers, which fire once when the trigger goes down: the left and right bindings of the leaderboard page (a hook page, DEC-047) and the secondary fire-at-target binding on the left trigger. The default set leaves out the lines that bind nothing: lines for a device that no input driver accepts (on the deselect, cycle-right and toggle-ready controls), a line that assigns a glyph to a control that does not exist, and a line that declares an input type with no assignment. The gesture bindings, among them one bound through a misspelt condition word and the power-cancel key chord, are not specified (DEC-055). The UI pointer-down and UI pointer-held bindings are aliases: the show-cursor control combined with the gamepad's primary face button going down, and held, respectively.
- Options: A — state the effective bindings explicitly and reject unknown words; B — the reference behaviour, in which an unknown condition word (an auto-repeat word on a trigger, a misspelling, or a Boolean word on a one-axis control) reads as a press, and four default bindings work only because of it.
- Recommendation: A. The bindings a player uses stay exactly as they are, and a mistyped binding becomes a load error with a diagnostic instead of a silent press binding.

### DEC-554 Input devices, key names and binding drivers
- Status: Resolved
- Affects: architecture/components/input.md (INP), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: These input corrections apply in addition to those of DEC-096:
  1. analogue trigger values are read over their full range without overflow, so a binding may use trigger magnitude; no default binding does;
  2. one table maps every key code to one canonical key name, and parsing, display and saving all use it; the numeric-pad minus key and the meta key resolve to their own key codes;
  3. every binding driver validates its words and indices at load: no driver reads past a table, leaves a field uninitialised or throws while parsing, and an alias chain that refers back to itself is rejected;
  4. auto-repeat timing is held per device and per binding, and querying a binding never changes it;
  5. each local player has an assigned gamepad, and a gamepad drives only the player it is assigned to; a gamepad is assigned by its first button press (DEC-953);
  6. a keyboard-layout change at run time reloads the layout-specific bindings, and restoring the default bindings re-applies the layout layer.
- Options: A — apply the corrections; B — the reference behaviour, in which the scaled trigger value overflows its byte for raw values above 40, so only zero and non-zero are reliable; the key-name table and the key-code constants disagree, so the numeric-pad minus name binds the separator key, and the meta key lies outside the lookup range; five dormant drivers carry an off-by-one key index, an uninitialised input type, a table over-read on an unknown word, an exception while parsing and unguarded alias recursion; one shared auto-repeat table serves every gamepad and every binding of a control, and queries change it; any connected gamepad drives the single local player, the first whose input matches a binding winning; and a layout change at run time is only logged.
- Recommendation: A. The table over-read and the unguarded recursion are crash defects; the other defects make bindings unreliable in ways that a player can neither see nor fix.

### DEC-555 A crowd order for an empty group does nothing
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM)
- Specified behaviour: A crowd order executes against the Workers that the sending team controls at the tick of execution. If none remain, because they all died or the group was released after the click, the order does nothing: no route is created and no Worker receives an order. This is the crowd-order case of DEC-653, under which simulation state never holds a non-finite value.
- Options: A — the corrected rule; B — the reference behaviour, in which the route starts at the mean position of the controlled Workers, computed by dividing by their number, so an empty group gives an undefined (not-a-number) start position.
- Recommendation: A. An undefined position must never enter simulation state. The general rule that every command handler defines its result when its target no longer exists is a constraint of DEC-202.

### DEC-556 Power slot order
- Status: Open
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), architecture/components/ui_framework/pages_and_screen_recipes.md (UI)
- Specified behaviour: The power bar orders a team's powers by the slot-ordering rules of reference/06, and the number keys select a power by its position in that order, so a power that is registered again moves to the last position and the key that selects it changes (DEC-551). The bar is clickable and shows its empty slots (DEC-083).
- Options: A — the reference ordering; B — stable slot positions: a power keeps the slot it was granted in until it ends, and each number key always selects the same slot.
- Recommendation: B. With a clickable bar that shows empty slots, the slot a player sees is the natural identity of a power, and a key that keeps pointing at the same slot avoids firing the wrong power in a fight. Keep A only if a playtest shows that players depend on the compact order for keyboard play.
- Decide by: playtest

## 2 Bots

### DEC-557 Bot troop movement between navigation nodes
- Status: Open
- Affects: reference/07_bots/ (R-BOT), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM)
- Specified behaviour: A bot sends troops from a bot navigation node to a linked node, and they move there in a straight line. Automatic links join nodes at most 600 wu apart that pass a walkability test when the graph is built; a map may also place links by hand.
- Options: A — the reference straight-line movement along links; B — each dispatched group moves along the navigation graph with the same path-finding as player orders, or by flow fields.
- Recommendation: B for the world game, where the deputy and the settlement AI move crowds over large terrain and straight lines between distant nodes fail around obstacles; A is adequate on skirmish maps of reference size, whose automatic links are tested for walkability. Path-finding at world scale belongs to the big-world technology challenge of the vision (§6 item 6).
- Decide by: design phase

### DEC-558 Bot controller of the Rogue and Visitor factions
- Status: Open
- Affects: reference/07_bots/ (R-BOT), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: The Rogue faction and the Visitor faction each run the full bot controller, like any bot team: it dispatches their Workers between navigation nodes, uses Leaders and drives their Power slots (DEC-022). Neither faction can win a match or block an elimination win (DEC-082).
- Options: A — the full bot controller, as specified; B — a reduced hazard controller: the faction attacks what is near it and uses its powers, but dispatches no troops across the map and promotes no Leaders; C — no controller: the faction's units act only on their own autonomy.
- Recommendation: A for the skirmish rules, because a hostile crowd that is dispatched like a bot team is part of the threat of the hostile Supply crate outcomes. The world game states the behaviour of neutral-hostile groups explicitly as part of its relations model, where they become world events rather than match hazards.
- Decide by: design phase

### DEC-559 Bot module structure
- Status: Open
- Affects: reference/07_bots/ (R-BOT)
- Specified behaviour: reference/07 specifies bot behaviour by function: the navigation graph with its links, head counts and node ownership; the per-mode priority fields with their external pins; the dispatcher; Leader use; one handler per power; the per-mode strategies, including the Siege objective chain; and the policies of the Transport, Strike team, Leader, Builder and Collector. It prescribes no module structure. Bots run inside the simulation step, time everything in ticks and draw only from the simulation generator (DEC-002, DEC-013, DEC-022).
- Options: A — five modules: a navigation graph (nodes, links, head counts, ownership); a priority module (the per-mode fields and their pins); a director per bot team (the dispatcher, Leader use, the power handlers and one strategy object per mode); unit policies (Transport, Strike team, Leader, Builder, Collector); and the Siege objective chain; B — one controller object per bot team.
- Recommendation: A. Each module can be tested on its own against the rules of reference/07, and the graph, priority and unit-policy modules can serve the deputy and the settlement AI of the world game, which add their own director that plays under fog of war (vision §5).
- Decide by: design phase

## 3 Maps and level data

### DEC-560 Start positions are typed map records
- Status: Resolved
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT), architecture/components/camera_and_sequences.md (CAM), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: A map definition holds one start-position record for each player position. The record carries the position's camera view, its Siege side and its co-op group. Structures, units and camera views name the start position they belong to, and at load each start position is remapped to a match team (DEC-025), so one map serves 2 to 4 players.
- Options: A — typed start-position records; B — the reference conventions, in which a start position exists only implicitly: camera views are found through a naming convention per player slot, and side and group assignments are held in parallel lists.
- Recommendation: A. The remapping rule is unchanged, and typed records let the content build validate every start position (DEC-561).

### DEC-561 Per-mode map validation
- Status: Resolved
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/08_maps_and_level_design/map_editor_hooks.md (R-MAP), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), architecture/components/data_schemas/ (DAT), architecture/components/build_tooling_and_distribution.md (BLD)
- Specified behaviour: Mode objects are typed records with validated links, and Flag post adjacency is stored as one two-way relation. The content build, and any map tool when it saves, validates a map for every mode it offers and rejects it with a diagnostic when a rule fails:
  1. Relic Run: exactly one Relic Base per start position, and at least one Relic Spawn Zone;
  2. Flag Chain: one Home post per start position, and a connected Flag post graph;
  3. Siege: one Core Device owned by a defending start position, at least three Command Consoles linked to it, a Core Device timer equal to the match time limit, and every defence bot objective marker that has a scan range bound to a structure (an unbound marker defaults to team 0, and its defence objective falls back at once, DEC-087);
  4. Launch Race: for each start position, one Launch Vessel, three fuel depots and one Transit Gate for reinforcements.
- Options: A — validate at build and save time; B — the reference behaviour, in which these conventions hold only by authoring discipline, no tool checks them, and Flag post adjacency is not held as a two-way relation.
- Recommendation: A. These are the per-mode conventions of the reference maps; checking them stops a broken map from reaching players under either editor option of DEC-045. Schema validation of content in general is DEC-015.

### DEC-562 Map lighting and weather presets
- Status: Open
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/renderer/view_lighting_and_fog.md (RND), architecture/components/animation_and_vfx/ (VFX), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Each map lists its lights with raw colour multipliers; every reference map uses exactly two directional lights, and a map may override the key light. A map switches its weather effects one by one: snow, lightning strikes, dust vortices and Essence-orb rain.
- Options: A — the reference per-map values; B — named presets: a lighting preset (two directional lights and an optional key-light override) and a weather preset, each a named set of the same parameters, which a map selects and may override value by value.
- Recommendation: B. Presets keep the two-light look consistent across newly authored maps (DEC-063) and make the visual language easy to hold (vision O9). Weather that affects play, lightning strikes and dust vortices, stays an explicit setting of the map rather than a side effect of a look preset.
- Decide by: design phase

### DEC-563 Map thumbnails and the map index are content-build outputs
- Status: Resolved
- Affects: reference/08_maps_and_level_design/README.md (R-MAP), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/build_tooling_and_distribution.md (BLD)
- Specified behaviour: The content build renders each map's thumbnail with the engine, from a top-down view of the map at an aspect ratio of 4:3. It also generates the map index, the list of map ids that the Completionist achievement hook counts (DEC-047), and packages it with the maps it lists. Packaged data holds no asset that nothing references.
- Options: A — build outputs; B — the reference behaviour, in which thumbnails are 400 × 300 images made outside the engine, the content build leaves the map index out of the packaged data, so the Completionist achievement has no list of maps to count, and the packaged data holds assets that nothing references.
- Recommendation: A. A generated thumbnail always matches the map it shows, and a generated index cannot drift from the maps it lists.

## 4 Audio design

### DEC-564 Laser fire is audible, and selection sounds stay local
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: The laser-bolt events are bound: a laser bolt is heard when it is fired, when it hits a structure and when it kills. When the interface sounds for selecting and deselecting a power are bound, they play only for the local player's own selections, never for another team's. The other silent moments of the reference are decided elsewhere: the match-end stings (DEC-049), the Supply crate capture and decay tones and the Strike team grenade throw (DEC-097), the final countdown sound (DEC-072), and the Supply crate expiry and the remaining silent events (DEC-565).
- Options: A — bind the laser-bolt events, with the selection sounds scoped to the local player; B — the reference behaviour, in which the laser-bolt events fire with no binding and are silent, and the selection sounds fire for every team's selections, silent only because they have no binding.
- Recommendation: A. Laser fire is the Workers' main attack (DEC-030) and should never be silent. The reference sound set holds unbound recordings for laser charging, structure hits, actor hits, ground hits and ricochets, so these moments are meant to be heard; Species authors new sounds for them. A selection sound that plays for another team's selection would tell the player nothing about their own actions.

### DEC-565 Sounds for the other silent gameplay events
- Status: Open
- Affects: reference/10_audio_design/ (R-AUD)
- Specified behaviour: These gameplay events fire with no binding and are silent: a Supply crate expiring unopened; a turret acquiring a target, and a rocket turret firing; an actor electrocuted by a Barrier Emitter; a Relic being lifted and carried, whose carry sound is stopped when the Relic is dropped; Workers boarding and leaving a Transport; a Leader changing its order mode (follow and none); the creature cues of a spider landing a pounce, a centipede growing, panicking and sighting an enemy, a Reaper Serpent panicking and a Spitter Plant launching a Seed Pod; a Transit Gate coming online; selecting a Transport, a Builder or a Collector; selecting a power, and the player's waypoint order; a Builder delivering an Essence orb; a Collector, a Bomber, a barrage missile or a meteor losing health or being destroyed; and leaving the match. The reference sound set holds unbound recordings of carrying and straining, of Leader acknowledgements and of marching, attention and formation calls.
- Options: A — keep these events silent; B — author and bind a sound for every one of them; C — bind a reviewed subset first, ranked by how much the player needs to hear the event to act on it, and extend it as the mix budget allows.
- Recommendation: C, growing to B within the voice budgets of DEC-570. Bind first the events a player acts on: a Supply crate expiring unopened, a turret acquiring a target, a Barrier Emitter electrocution, a Relic lifted and carried, Workers boarding and leaving a Transport, Leader order changes, unit and power selection, the waypoint order and a Transit Gate coming online; then the creature cues and the damage sounds. The authored event list is reviewed event by event, so that no event stays silent by accident (DEC-027).
- Decide by: design phase

### DEC-566 Listener position and distance limits
- Status: Open
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: The listener is the camera: panning and distance attenuation both use the camera position. Attenuation follows the inverse-distance law (minimum distance ÷ distance) with no maximum distance. There is no Doppler effect and no occlusion. Voice limits come from priority classes and virtualisation (DEC-027), and the audibility of crowd sounds from DEC-109.
- Options: A — the reference listener and law; B — pan from the camera position, but attenuate from a focus point on the ground under the camera, blended with the camera position by zoom, so that a high camera still hears the battle it looks at; keep the inverse-distance law and add a per-event maximum distance and culling radius; no Doppler effect and no occlusion, with an optional low-pass filter by distance.
- Recommendation: B, tuned together with DEC-109 by a listening test at several camera heights. The inverse-distance law keeps the authoring model, and the focus point keeps the battle audible at strategic zoom, a view that the strategic zoom level of the world game makes common (vision O9).
- Decide by: playtest

### DEC-567 Dry mix or applied effects
- Status: Open
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: No audio effect runs, so the reference is heard dry. Sound definitions name effects in 22 places, but no effect is attached unless the preference file is edited by hand, and the long releases authored for echo tails play out as silence while they keep a voice busy. Where Species uses effects, they are applied per bus (DEC-027), and switching them off leaves no effect processing on any voice or bus (DEC-752). A DSP parameter that an effect binding omits takes its declared default, the schema default of every effect parameter (AEN-071, DAT-012); the reference reads an omitted parameter as 0.0, which lies outside the declared range of 21 of the 63 parameters. The choice is audible only once effects are enabled.
- Options: A — dry: the effect slots of the buses stay empty; B — a reverb send and a limiter on the master bus; C — option B plus per-event effects where a sound is designed for one.
- Recommendation: B, adopted only after a listening test confirms that it keeps the character of the dry reference mix (vision pillar 5). The master limiter is safe in every case, because it only prevents clipping in dense fights. Sounds are authored for the chosen effect model, so no release tail is authored for an effect that does not run.
- Decide by: design phase

### DEC-568 Sample-group choice and linked parameters
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: A sample group chooses its sample at random with authored weights and never plays the same sample twice in a row; pitch variation adds further variety. Linked parameters keep the four reference links (velocity, height above ground, camera distance and vertical position) and add four more: the age of the emitting object, the number of teams nearby, the game time remaining and the unit count.
- Options: A — the adapted rules; B — the reference behaviour, in which a group's random choice can repeat the previous sample and only the four reference links exist.
- Recommendation: A. No immediate repeat removes the most audible artefact of random choice, weights let a designer favour the best takes, and the new links let the mix follow the state of the match.

### DEC-569 Game-speed pitch bend
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine.md (AEN)
- Specified behaviour: Game speed bends the pitch of world sounds by the square root of the game speed, smoothed over time rather than per call (DEC-097), as an effect of the world buses. UI sounds and announcer stings are not bent. The pitch drop at match end is kept as a deliberate effect: as the game speed falls, the world sounds fall to 0.1× pitch, and a listening test confirms its character (DEC-320). The smoothed game-speed value starts every match at normal speed and is discarded at match end (DEC-751).
- Options: A — the adapted rule; B — the reference behaviour, in which the bend comes from a smoother that advances once per call, so its speed depends on the number of playing voices (DEC-097), and the bend is not limited to world sounds.
- Recommendation: A. A speed change is a world event (Time Dilation, Time Rush and the slow-motion phase of the meteor shower, DEC-039), so it belongs on the world buses, while UI feedback and stings stay readable at every speed.

### DEC-570 Buses, priority classes and voice budgets
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/audio_engine.md (AEN)
- Specified behaviour: The mix has six buses under the master bus, each with its own volume slider for the player (DEC-044), sharing 64 real voices:

  | Bus | Contents | Default level | Real voices | Priority class |
  |---|---|---|---|---|
  | UI | interface sounds | 0 dB | 4 | highest; never stolen |
  | Announcer and stings | Supply crate rewards, zone and Relic stings, match flow | 0 dB; ducks the world bus by 6 dB for 1 s | 4 | high |
  | Voice | unit barks | −3 dB | 16; at most 3 per event | medium |
  | World | weapons, impacts, structures, creatures | −3 dB | 32; at most 4 to 6 per event | by loudness at the listener (DEC-566) |
  | Ambience | ambience beds | −10 dB | 4 | low, but never stolen by world sounds |
  | Music | the music system (DEC-049) | −6 dB | 2, for cross-fades | separate |

  A request steals a voice only when it outranks the victim by at least 3 dB; a request that loses is virtualised, not dropped. Triggers carry a time stamp, so start latency is not quantised to the tick, and any number of voices may start in one frame. Per-event cool-downs are data and replace fixed rate limits of 0.2 s, 0.5 s and 4 s.
- Options: A — this bus layout and these budgets as the Species baseline; B — the reference behaviour: one master volume; a priority based on perceived volume with no steal test; at most one voice start per 50 ms audio tick, in which the lowest-ranked voice is replaced by the best queued request whatever its loudness, so a burst of 30 explosions takes 1.5 s to start and most one-shots expire unheard; interface sounds and stings always win and can evict world combat voices; and only a higher volume, a larger minimum distance or closeness to the camera protects an important world sound, so Strike team laser fire is easily lost.
- Recommendation: A. The buses implement the replacement of the priority model and the mix (DEC-027, DEC-097); the figures are starting values, retuned by listening tests in dense battles, where UI sounds and stings must never be lost and no kind of one-shot sound may disappear.
