# Decision register: verdicts and fixes (continued)

> Status: Reference decisions · Range: DEC-064 to DEC-119 · Related: [decisions index](README.md), [verdicts and fixes, DEC-001 to DEC-063](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

## Index

The [register index](README.md) lists every entry of this file with its title, status and link. The sections of this file, which continue the numbering of [verdicts_and_fixes.md](verdicts_and_fixes.md):

| Section | Entries | Count |
|---|---|---|
| [7 Reference defects that Species fixes](#7-reference-defects-that-species-fixes) | DEC-064 to DEC-105 | 42 |
| [8 Design-critical uncertainties and values to confirm by measurement](#8-design-critical-uncertainties-and-values-to-confirm-by-measurement) | DEC-106 to DEC-117 | 12 |
| [9 Further integrity rules](#9-further-integrity-rules) | DEC-118, DEC-119 | 2 |

## 7 Reference defects that Species fixes

### DEC-064 One fixed recount period
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/README.md (R-BLDG-003), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/zone_control.md (R-MODE), reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/01_skirmish_rules_and_modes/relic_run.md (R-MODE)
- Specified behaviour: The recounts of Control Zones, Flag posts and Relics, which are re-armed from the match clock, run on one fixed integer period: 11 ticks for a recount written with a 1-second period, and 21 ticks for one written with a 2-second period. Timers that count down from a fixed reset, such as the 20-tick recounts of bot navigation nodes and bot objective markers, keep their own tick counts and are not covered.
- Options: A — one fixed period; B — the reference behaviour, in which the period depends on the re-arm time: at normal speed it is 10 ticks for re-arm times in [0.4, 3.0), [15.5, 63.2) and [255.4, 1023.0) s and from 4,095.5 s on, and 11 ticks otherwise (20 or 21 ticks for a 2-second period).
- Recommendation: A. The irregular cadence has no design intent; the 11-tick period matches the mode second (DEC-002).

### DEC-065 Transit Gates advance once per tick, and map values override lobby defaults
- Status: Resolved
- Affects: reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: Transit Gates advance exactly once per tick on every map. Where a map sets a value, the map value overrides the lobby default; a typed override keeps 0 as a value, where the reference silently drops a per-map override whose value is 0.
- Options: A — the corrected rule; B — the reference behaviour, in which Transit Gates advance twice per tick on three Flag Chain maps, doubling their waves, and lobby values override map values; with the routine run twice per tick every countdown step takes 5.5 ticks on average: waves 330 ticks apart with a first gap of 324, and 40 Workers per gate every 33.0 s (about 72.7 per minute).
- Recommendation: A. Maps whose balance relies on doubled waves are tuned through their reinforcement values instead: on the three Flag Chain maps that set them, a wave is 100 Workers with up to 3 Transports, not the lobby's 40 and 2.
- See also: DEC-120 (how a map value of 0, an absent value and "unlimited" are written).

### DEC-066 Multi-round Siege
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-029)
- Specified behaviour: Siege is a single-round match. Multi-round Siege, in which the sides rotate between rounds and the best defence time wins, is fully specified in the rules, but no match setup can select it. The round reset runs at a tick boundary (DEC-261) and leaves the object serial counter unchanged.
- Options: A — drop multi-round Siege; B — expose it in the lobby, with a round reset that also resets the object serial counter.
- Recommendation: A, the stated default, unless playtests show demand. If B is chosen, the round reset is never applied outside a tick.
- Decide by: design phase

### DEC-067 Map defaults are applied once, by the host
- Status: Resolved
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: When a map is selected, the host applies the map's default play time and its other defaults to the match setup once, authoritatively; no client applies them on its own.
- Options: A — the corrected rule; B — the reference behaviour, in which the map's play time and the default time limit race in the lobby, so the result depends on message order.
- Recommendation: A.

### DEC-068 The Flag Chain time-up draw uses valid team indices
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE)
- Specified behaviour: The Flag Chain time-up draw check uses only valid team indices. An entity with no team holds an empty team (no sentinel value), which is never used as a roster index.
- Options: A — validate indices; B — the reference behaviour, in which the draw check reads a roster entry for "no team", outside the roster.
- Recommendation: A. A read outside the roster is a crash defect.

### DEC-069 The co-op game-over line shows the declared result
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: In a co-op match the game-over line shows the declared result, draws included.
- Options: A — the corrected rule; B — the reference behaviour, in which the line names the alliance with the higher score total, which can differ from the declared winner.
- Recommendation: A.

### DEC-070 The Spawn Control co-op winner check covers every slot
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/spawn_control.md (R-MODE)
- Specified behaviour: The Spawn Control co-op winner check iterates over every team slot.
- Options: A — the corrected rule; B — the reference behaviour, in which the check iterates only over as many slots as there are teams in the match, so a team in a higher slot is missed.
- Recommendation: A.

### DEC-071 Flag Chain base-under-attack announcement interval
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/10_audio_design/ (R-AUD)
- Specified behaviour: The Flag Chain announcement that a team's base is under attack repeats at most about every 210 s.
- Options: A — keep about 210 s; B — a throttle of 20 s, counted in ticks.
- Recommendation: B, the stated default: 20 s is the intended interval.
- Decide by: playtest

### DEC-072 The final countdown sound plays at time-up
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/10_audio_design/ (R-AUD)
- Specified behaviour: The countdown sound for the last tick plays at time-up.
- Options: A — the corrected rule; B — the reference behaviour, in which the sound for the zero tick never plays.
- Recommendation: A.

### DEC-073 Typed match options with explicit per-mode defaults
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Every match option is typed and has an explicit default for each mode; an option that a mode does not set takes that typed default. The Comeback Bonus is on by default in every mode where it applies: an eliminated team receives a comeback power every 450 ticks (45 s) and holds at most two. It runs in Siege and Launch Race too but never grants there, because no team counts as eliminated in those modes. In the reference it is a check box on the host's advanced-options page for Spawn Control, Zone Control, Relic Run and Flag Chain, and an absent value counts as on. The typed defaults reproduce every effect of an absent option: no time limit; sudden death off; reinforcement interval 60 mode seconds; reinforcement count and Transport cap from the map, with no Transports when the map sets none; no reinforcement turrets; turret cap 2; no crate drops; random crate placement; Catch-up multiplier 1; and no starting powers.
- Options: A — typed options with explicit defaults; B — the reference behaviour, in which an absent option reads as −1, a Boolean test treats −1 as true, and the Comeback Bonus is therefore on in every mode.
- Recommendation: A. The mechanism is corrected and the Comeback Bonus default is kept explicitly per mode, so play does not change where the bonus applies.

### DEC-074 Rank under ties
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: A team's announced rank is the number of teams with a strictly better score, plus 1.
- Options: A — the corrected rule; B — the reference behaviour, in which the rank announcement uses the team's own list index when scores tie.
- Recommendation: A.

### DEC-075 The Core Device result is fixed at detonation
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: When the Core Device detonates, the Siege result is fixed, and Command Console capture is no longer evaluated.
- Options: A — the corrected rule; B — the reference behaviour, in which Command Console capture checks continue after detonation.
- Recommendation: A.

### DEC-076 Launch Vessel Flight is invulnerable and terminal
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: Once a Launch Vessel enters Flight it cannot be damaged and cannot return to refuelling; Flight ends only in the launch result.
- Options: A — Flight is invulnerable and terminal; B — Flight is terminal, and damage during Flight destroys the vessel; C — the reference behaviour, in which the vessel can be damaged during Flight and can refuel in mid-air.
- Recommendation: A. A launch is the climax of Launch Race, and an invulnerable, terminal Flight makes its outcome unambiguous.

### DEC-077 The bot-mode option accepts only defined values
- Status: Resolved
- Affects: reference/07_bots/ (R-BOT), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: The range check of the bot-mode option accepts only the defined values 0 to 2 and rejects the value 3.
- Options: A — the corrected check; B — the reference behaviour, in which the check accepts 3.
- Recommendation: A.

### DEC-078 No-team colour, under-attack status and the neutral Home post
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), architecture/components/renderer/visual_language.md (RND)
- Specified behaviour: The no-team colour is an opaque grey. A structure's under-attack status is set while it is attacked. A neutral Home post never reports itself under threat.
- Options: A — the corrected rules; B — the reference behaviour, in which the no-team colour is black with alpha 1/255, the under-attack status is never set and a neutral Home post reports itself under threat.
- Recommendation: A.

### DEC-079 Per-team population cap
- Status: Resolved
- Owner decision: O14 (2026-10-17): every player slot is occupied; empty slots are filled with bots by default, and the host may close slots before the match.
- Affects: reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/01_skirmish_rules_and_modes/siege.md (R-MODE-369), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: Every open player slot of a map is occupied: at match start, empty open slots are filled with bots, and a participant who leaves is taken over by a bot. Before the match, the host may close slots. The per-team cap is the map's population cap divided by the number of **open** slots (the teams playing), so it never changes during a match. Alliances are not pooled. The bodies counted are listed explicitly: the team's bot controller, Transports, Leaders, Crawler eggs and fading corpses count; Crawlers do not. In Siege each defending team has the defender cap, and the attackers share (map cap − defenders × defender cap) ÷ attackers by integer division; a Siege map with no defender cap uses map cap ÷ open slots for both sides. In a Siege match whose map sets a defender cap and an unlimited population cap, a defending team is locked at the defender cap and an attacking team is decided by the Population Limiter fallback (reference/05 R-ECON-052 rule 2); no reference map has this combination. A map that sets no population cap states "no cap" explicitly (DEC-120).
- Options: A — the reference rule; B — a cap from the open slots, per team and never pooled within an alliance, with an explicit list of what counts as a body.
- Recommendation: B, the stated default. It keeps army size proportional to the teams present and makes co-op caps explicit; confirm by playtest across player counts and on co-op maps.

### DEC-080 Collector collection of any team's Essence orbs
- Status: Resolved
- Owner decision: O15 (2026-10-17): option B.
- Affects: reference/02_units/collector.md (R-UNIT), reference/05_population_flow.md (R-ECON)
- Specified behaviour: A Collector collects Essence orbs of any team and delivers them to its own team. The delivery is subject to the receiving team's population cap (DEC-079) and to the activation rules of the receiving Spawn Node: orbs that would exceed the cap are not converted. A Collector at its Spawn Node delivers an orb only on a tick on which that node may produce, that is, its activation delay is 0 and it is not population-locked (R-ECON-052); on any other tick it keeps its orbs and waits at the node, so no orb is lost and none becomes a Worker above the cap. A node that has no team still spawns nothing and the orb is still removed. Whether a waiting Collector releases its orbs instead is decided by playtest.
- Options: A — the reference rule; B — keep any-team collection and apply the population cap on delivery; C — own-team orbs only.
- Recommendation: B, the stated default. Any-team collection is a designed comeback lever, and the cap on delivery bounds it.

### DEC-081 Catch-up bonus curve
- Status: Resolved (superseded by DEC-124)
- Owner decision: O16 (2026-10-17): option B.
- Affects: reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/ (R-MODE)
- Specified behaviour: The Catch-up bonus is a proportional curve of a team's share deficit, with the Rogue and Visitor factions excluded. DEC-124 states the curve.
- Options: A — the reference step function; B — proportional catch-up.
- Recommendation: B, by owner decision; see DEC-124.

### DEC-082 Rogue and Visitor factions never win and never block elimination
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/ (R-MODE: the README and all six mode files, among them R-MODE-307 of Relic Run)
- Specified behaviour: The Rogue faction (team slot 4) and the Visitor faction (team slot 5) are neutral-hostile in every mode: they never win a match and never block an elimination win. Hostile Supply crate outcomes stay owned by the Rogue faction, and the Visitor faction still arrives with the Abductor.
- Options: A — one rule for both factions in every mode; B — the reference behaviour, in which they block the last-team-standing win of Spawn Control and can win Zone Control and Flag Chain on score at time-up.
- Recommendation: A. The reference behaviour differs from mode to mode; one rule keeps both factions as hazards rather than competitors.

### DEC-083 Clickable power bar and wide power ids
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), architecture/components/ui_framework/pages_and_screen_recipes.md (UI)
- Specified behaviour: The power bar is clickable and shows empty slots, and it stays selectable by keys and gamepad. Power ids in commands are at least 16 bits wide. The six-slot limit is kept (DEC-033).
- Options: A — the corrected rule; B — the reference behaviour, in which the bar is selectable only by keys, tab and bracket keys and shoulder buttons, and power ids are one byte.
- Recommendation: A.

### DEC-084 Hive Essence orbs and Hive ants
- Status: Open
- Affects: reference/05_population_flow.md (R-ECON), reference/02_units/creatures.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG)
- Specified behaviour: A destroyed Hive releases no Essence orbs, and a Hive's ants do not count against the Rogue faction's cap. No death of a Rogue faction actor leaves an Essence orb; a Hive's sortie check counts only the Rogue faction's other bodies (its bot controller, crate creatures and eggs), and a Hive releases one ant per 0.2 s.
- Options: A — keep, documented as a rule: the Rogue faction is outside the essence economy; B — Hives release orbs and their ants count against the cap.
- Recommendation: A, the stated default.
- Decide by: playtest

### DEC-085 Builder Essence-orb delivery to friendly Spawn Nodes
- Status: Resolved
- Affects: reference/02_units/builder.md (R-UNIT-447, R-UNIT-454, R-UNIT-457), reference/05_population_flow.md (R-ECON-082), reference/00_reference_overview.md (R-VIS)
- Specified behaviour: A Builder collects Essence orbs of any team in Birth or Floating, up to its capacity, and delivers them to the nearest Spawn Node, by plain distance, whose owner is a friend of its team: one orb per tick, each becoming one Worker of the node's current owner, with no population-cap or activation check (DEC-453). Carried orbs never expire; a Builder that dies drops them to Floating.
- Options: A — the delivery rule as specified; B — remove the Builder's orb collection.
- Recommendation: A. The collected orbs have a destination on every map with Spawn Nodes, and the Builder outcome is valid only on such maps, so the behaviour has a clear economic effect and needs no change.

### DEC-086 Separate demote and scrap actions for Leaders
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/02_units/leader_and_formations.md (R-UNIT)
- Specified behaviour: A player can demote a selected Leader back to a Worker and, as a separate action, scrap one of its own units. The cancel key does not kill a Leader.
- Options: A — the corrected rule; B — the reference behaviour, in which the cancel key kills a selected Leader and players cannot demote.
- Recommendation: A.

### DEC-087 Bot corrections
- Status: Resolved
- Affects: reference/07_bots/ (R-BOT)
- Specified behaviour: Bots behave as reference/07 specifies, with every correction it lists applied. The corrections include: correct ids where wrong ids are passed, including the current power id instead of a stale one; no NaN priorities; no stall of Leader promotion while the bot team has a selection; Leader powers left without a Leader expire instead of accumulating in Power slots; an initial navigation graph that accounts for Barrier Emitters, because the obstacle grid is built before the bot target links; crossings stored independently of a Barrier Emitter's state, so a toggle needs no graph rebuild; a bot turret placed at its most threatened qualifying frontier node, not the last one in structure order; turret placement around the whole base rather than in one quadrant; a power-placement centroid that divides the Worker sum by the Worker count, not by the count of all friendly actors; a Relic un-stick centroid that no longer divides by the count of all collisions; a random walk onto a sea node that no longer leaves the army in place; a recall rate that does not scale with array capacity; a filled Siege bot Transport objective list, so that the mobile-bomb branch of the Siege Transport behaviour runs; defending formations turned towards the threat; a Siege defence objective that does not fall back at once (map validation binds its marker, DEC-561); and a bot controller placed inside the map rather than at its corner. The position of the bot controller and the expiry of Leader powers are DEC-862, and the unbounded bot loops are bounded (DEC-014, DEC-857).
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A. Bot strength is then calibrated by playtest in every mode, Siege included (DEC-112).

### DEC-088 Structure corrections
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/ (R-BLDG: every file of the folder)
- Specified behaviour: Structures behave as reference/03 specifies, with every correction it lists applied. The corrections include: port occupancy bound to the (structure, port) pair; Barrier Emitter collision geometry that matches its visible geometry, so the obstacle-grid band that marks each Barrier Emitter line one cell wider on every side never blocks movement beyond the visible line and serves at most to select collision candidates; per-tick rates that scale with game speed (DEC-092); no team-change sounds from map template structures at load; turret line of sight sampled from the turret towards the target; Worker grenades against structures thrown for the Worker's own team (DEC-105); and tree hit volumes taken from asset data (DEC-012). The smoke-column obstacle is DEC-089.
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A.

### DEC-089 Smoke-column obstacle
- Status: Open
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: A smoke-column marker is an invisible obstacle 13 wu in size that blocks movement.
- Options: A — the reference behaviour; B — markers do not collide unless a map explicitly marks the obstacle as wanted.
- Recommendation: B, the stated default.
- Decide by: design phase

### DEC-090 Floating-point actor stats and the Abductor speed
- Status: Resolved
- Affects: reference/02_units/README.md (R-UNIT), reference/02_units/hazards_and_projectiles.md (R-UNIT), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Actor stats are floating-point values, and speed multipliers (×2, ×1.5, ×0.75, ÷2) apply exactly, without truncation. The Abductor's speed is an explicit tuning value whose default is its effective reference speed of 244.
- Options: A — floating-point stats and an explicit Abductor speed; B — the reference behaviour, in which stats are 8-bit values, so boosted speeds truncate and the Abductor's listed speed of 500 wraps to 244.
- Recommendation: A. Boosted units become fractionally faster than in the reference, and the Abductor keeps its effective speed until a playtest re-tunes it.

### DEC-091 Dormant Crawler egg lifetime
- Status: Open
- Affects: reference/02_units/creatures.md (R-UNIT)
- Specified behaviour: A dormant Crawler egg never decays; its 120 s lifetime limit has no effect.
- Options: A — the reference behaviour; B — apply the 120 s lifetime, counted in ticks.
- Recommendation: B, the stated default.
- Decide by: playtest

### DEC-092 Per-tick rules scale with game speed
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/creatures.md (R-UNIT), reference/03_buildings_and_world_objects/README.md (R-BLDG), reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM)
- Specified behaviour: Every per-frame or per-tick probability, growth and rate is a per-second rate scheduled in ticks, so the speed crates change it like every other rule. This covers the lightning roll of 1 % per tick, the Blight Grove growth of 2 % per tick, the Supply crate capture cap of 0.05 per tick, the Solar Array energy surge, the gravity of Seed Pods and spiders, the empty-Hive counter, the volcano vent eruption, tree drops, the Spitter Plant shrink, Collector delivery (one Essence orb per tick), Spawn Surge requests (one per tick), the Launch Vessel passenger release, the Relay overshoot and the game-speed hold counters. Under DEC-039 every one of them scales the same way.
- Options: A — per-second rates scheduled in ticks; B — the reference behaviour, in which these rules ignore game speed.
- Recommendation: A.

### DEC-093 Explosion damage falloff
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM)
- Specified behaviour: The damage factor at distance d from an explosion of range r is ((2r − d) ÷ r) × 2. This is the tuned curve and is documented as the rule.
- Options: A — keep the curve; B — the apparent intent, (2r − d) ÷ 2r.
- Recommendation: A, unless explosions are re-tuned as a whole: the curve is stronger than the apparent intent, and all explosive balance rests on it.

### DEC-094 Exact or approximate actor-grid queries
- Status: Open
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/02_units/worker.md (R-UNIT), reference/07_bots/ (R-BOT), reference/01_skirmish_rules_and_modes/zone_control.md (R-MODE)
- Specified behaviour: Neighbour queries go through an actor grid cache whose results can be up to about 1.1 s old; a query with half the range of a cached query reuses the cached result; presence checks use cell capacity rather than exact membership. Port occupancy is exact: a strict dx² + dz² < 25 test (radius 5 wu), where the occupant is the first operating Worker in grid result order (stable key in DEC-016). Count queries count whole cells of the square that bounds the range, so they over-count by up to one cell beyond it; a cell that has ever held an actor answers a presence check as occupied for the rest of the match; and a cache hit returns the cached list unfiltered, including actors outside the requested range.
- Options: A — the reference cached and approximate queries, specified as explicit rules; B — exact queries, or an explicit per-tick memo with identical results.
- Recommendation: B, the stated default. Run an A/B playtest of the same scenarios with the cache on and off, because exact queries change targeting, grenade decisions, zone contests and bot under-attack tests; if the softness is wanted, specify it as an explicit rule.
- Decide by: playtest

### DEC-095 Obstacle grid and route upkeep; route previews kept separate
- Status: Resolved
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: The obstacle grid is updated incrementally, and its outside cell is cleared like every other cell. When a structure is built or destroyed, the affected route-graph edges are rebuilt. Route previews that the UI draws are computed in a separate store and never take entries from the simulation's route storage.
- Options: A — the corrected rules; B — the reference behaviour, in which the outside cell is never cleared, route tables are never rebuilt after load, and preview routes share the simulation's free list.
- Recommendation: A. Shared storage lets presentation change simulation results (DEC-007).

### DEC-096 Input corrections
- Status: Resolved
- Affects: architecture/components/input/ (INP), reference/06_controls_camera_and_commands/ (R-CTRL), architecture/components/camera_and_sequences.md (CAM)
- Specified behaviour: Input behaves as reference/06 specifies, with every correction it lists applied. The corrections include: a press shorter than a frame is never lost; an unknown condition word in a binding definition is rejected at load, not read as a held key; the camera is clamped horizontally; the Strike team camera distance is a floating-point value; and one click produces one selection command (DEC-119). The input corrections that this entry does not name are DEC-554. The composable binding graph of drivers, combination operators and aliases is kept.
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A.

### DEC-097 Audio corrections
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine/ (AEN)
- Specified behaviour: Audio behaves as reference/10 specifies, with all 24 corrections it lists applied. The corrections include: the Supply crate decay tone names a real sound; the Core Device loop stops; the victory and defeat stings are bound to the match-end events (R-AUD-307, DEC-049); crowd sounds are audible according to an authored priority class rather than an accidental cull radius (DEC-109); voice stealing compares priorities; a change of ownership to no team plays only the lost sting, not both the captured and the lost sting; and a missing sample is a diagnostic, not a fatal assertion (DEC-015). The behaviour of attached sounds when their emitter is removed and the start value of smoothed parameters are open questions (DEC-321, DEC-322), and loop keep-alive is DEC-323.
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A.

### DEC-098 Texture cache keyed by asset and variant
- Status: Resolved
- Affects: architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/renderer/README.md (RND)
- Specified behaviour: Textures are cached by the pair (asset, variant), where the variant names the mip and mask processing, and an import manifest declares each variant. Two requests for different processing of one image get two cache entries. Species asset ids are lower case, so one image has one name and one entry per variant.
- Options: A — the corrected cache; B — the reference behaviour, in which the cache is keyed by name, so the first request fixes the processing for every later one: the colour marker image can show as a white alpha mask, and the laser and cloud textures drawn without mipmaps can sample an incomplete texture after the cache is flushed.
- Recommendation: A.

### DEC-099 Localisation fallback and language completeness
- Status: Resolved
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI)
- Specified behaviour: A string that a language lacks falls back, per key, to the base language (R-META-464). A language is offered only when its table is complete (R-META-463); the content build reports the key coverage of every language (ADR-063), and the development language test lists every missing id (R-META-473). The start-up language falls back from the language preference to the operating-system locale's language, then to the base language, then to the first language file present.
- Options: A — the corrected rule; B — the reference behaviour, in which a missing string shows its key and a language is offered whatever its coverage.
- Recommendation: A.

### DEC-100 One defaults table for preferences
- Status: Resolved
- Affects: architecture/components/platform_layer.md (PLT), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Preference defaults come from one table, in code or in a schema, that is available before any pack file is mounted, so installed and development builds use the same defaults (ADR-061). Each setting has one default, which every reader of the setting, the options pages included, uses; the settings are those of the settings table of the data schemas (DAT-193), each with a Species effect: among them the render size, the window mode, vertical sync, the bus volumes, the language and the bindings. Colour depth, refresh rate, shader switches and the audio device's buffers, mixing rate and voice count are not settings (DEC-869). The network keys that nothing reads are listed in DEC-250.
- Options: A — one defaults table; B — the reference behaviour, in which the default-preferences file is read before the main pack is mounted, so packaged installs fall back to code defaults, and the readers of one setting can disagree on its default.
- Recommendation: A.

### DEC-101 Seasonal theme
- Status: Open
- Affects: architecture/components/animation_and_vfx/ (VFX), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM)
- Specified behaviour: The reference has a seasonal theme (seasonal hats on Workers and ice ramps) that each client switches on from its own date check, from 24 to 26 December local time, and whose flag is relayed through the command stream into the simulation. Whether the theme is active is fixed once per match and identical for every participant (DEC-118); this entry decides whether Species keeps a seasonal theme and who chooses it. Until it is decided, the host's match setup applies the reference date rule once, from the host's local date.
- Options: A — the host decides the theme in the match setup, and it is sequenced like any other match option; B — drop the seasonal theme.
- Recommendation: A, the stated default, with new seasonal art; under either option the seasonal state follows DEC-118.
- Decide by: design phase

### DEC-102 UI framework corrections by construction
- Status: Resolved
- Affects: architecture/components/ui_framework/ (UI: README.md, core_and_widgets.md, layout_rendering_input_and_localisation.md, pages_and_screen_recipes.md)
- Specified behaviour: The Species UI framework avoids by construction every defect of the window core and of the framework that the UI framework component lists, each held by a test of its defect suite DT-01 to DT-33 (UI-014, UI-490). The window-core corrections include: a press and its release act on the same widget, with pointer capture; a held button repeats on explicit timers (ADR-032); a relayout keeps view-model state (ADR-033); and the maximise path never accesses a missing object, a crash defect. The reference framework supports tooltips, but nothing registers one, so no tooltip appears; Species specifies none, and a tooltip feature is front-end design for the design phase. The framework corrections include: Escape cancels and never commits; gamepad Back is handled as input, not polled during rendering; a percent sign is an ordinary character of message text and text fields; a colour or side row has one availability rule for drawing and pressing; no fixed-size array limits a list; a disabled widget never acts, so a host button stays inactive while hosting is not permitted (DEC-243); and list rows are removed by identity, so shrinking a list never removes the wrong row, such as the Back entry.
- Options: A — a new framework that avoids every listed defect; B — reproduce the reference framework.
- Recommendation: A.

### DEC-103 Tutorials are step graphs with simulation-sequenced effects
- Status: Resolved
- Affects: reference/11_tutorial_and_help.md (R-TUT), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM)
- Specified behaviour: Tutorial steps form a graph keyed by object tags, never by fixed team or structure ids. Tutorial state resets at every run. Every world-changing tutorial effect, such as inserting a Supply crate or changing an option, is a sequenced simulation event at a tick, never an action of the render loop or of a wall-clock timer. The director is evaluated once per tick of match time on the tutorial player's participant, issues each world change as a tutorial event, and tests the tutorial player's team, never team 0 (R-TUT-061, R-TUT-062, R-ECON-052).
- Options: A — the corrected design; B — the reference behaviour, in which the second tutorial checks ownership against team 0, tutorial state persists between runs, and the tutorial director changes the world from the render loop on wall-clock timers.
- Recommendation: A.

### DEC-104 Formation volley rhythm in integer arithmetic
- Status: Resolved
- Affects: reference/02_units/leader_and_formations.md (R-UNIT)
- Specified behaviour: A formation member's volley phase is (tick count + member index) modulo (formation size ÷ 4), with the firing condition of reference/02. The phase is evaluated exactly in integer arithmetic, for example as (4 × (tick count + member index)) modulo formation size, so formations whose size is not a multiple of 4 keep the reference rhythm.
- Options: A — integer arithmetic with the reference rhythm; B — the reference single-precision floating-point evaluation.
- Recommendation: A.

### DEC-105 Worker grenade team and the Hive and Spitter Plant attack target
- Status: Resolved
- Affects: reference/02_units/worker.md (R-UNIT), reference/02_units/creatures.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG)
- Specified behaviour: Grenades that Workers throw at structures belong to the thrower's team. Attacks by Hive ants and Spitter Plants on a structure pass the id of that structure.
- Options: A — the corrected rules; B — the reference behaviour, in which these grenades are thrown as team 1 and these attacks pass an obstacle index in place of a structure id.
- Recommendation: A.

## 8 Design-critical uncertainties and values to confirm by measurement

### DEC-106 Performance envelope at the population cap
- Status: Open
- Affects: reference/12_simulation_model/README.md (R-SIM), reference/05_population_flow.md (R-ECON), architecture/components/diagnostics_test_and_performance/ (DBG), architecture/components/renderer/options_and_budgets.md (RND), architecture/components/animation_and_vfx/ (VFX)
- Specified behaviour: The reference population caps allow 1,400 to 2,000 simulated bodies per map, plus Essence orbs and projectiles. No frame time, draw-call count, particle count or per-system cost is known for this population, and neither is the population at which the frame rate falls below 30 or 15 frames per second. The Worker task search is the largest expected simulation cost.
- Options: A — keep the reference caps and effect counts; B — lower them to fit a measured budget.
- Recommendation: A provisionally. Measure the first prototype with a synthetic 2,000-unit match on the Species renderer, set per-system budgets, and lower caps only if a budget is missed. The world tick rate and per-tick budget are deferred topics (DEC-227, DEC-228).
- Decide by: prototype

### DEC-107 Hostile Supply crate outcomes: ownership and filtering
- Status: Resolved
- Owner decision: O18 (2026-10-17): hazards stay team-blind, every outcome respects the map's exclusion list, and Brood ownership is decided in playtest (DEC-121).
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/hazards_and_projectiles.md (R-UNIT)
- Specified behaviour: The meteor shower, Sickness and Missile Barrage outcomes affect every team (team-blind). Every Supply crate outcome, hostile ones included, respects the map's list of excluded outcomes, and so does the Comeback Bonus draw (DEC-452). The ownership of Brood creatures is decided separately (DEC-121). The step that sets up the Rogue faction for the opening team has no effect and is not specified.
- Options: A — keep each behaviour as specified; B — decide per outcome: hostile outcomes respect the map's exclusion list, Brood creatures belong to the Rogue faction, and each outcome's team filter is explicit.
- Recommendation: A for the hazards that ignore teams, since they are the risk of the 10 % hostile pool (DEC-021); B for the exclusion list, so that map designers control every outcome. Decide Brood ownership in playtest.

### DEC-108 Launch Vessel survival under attack
- Status: Open
- Affects: reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE)
- Specified behaviour: Grenades thrown at a Launch Vessel aim 50 wu above its base; damage is measured to the base and scaled by 0.2; the vessel's damage decays at 0.33 per second. How long a besieged vessel survives is derived from these rules, not specified as a value.
- Options: A — keep the rules as specified and tune them by playtest; B — specify vessel survival directly.
- Recommendation: A. Simulate the race between attack and refuelling offline from the specified ballistics first, then confirm by playtest.
- Decide by: playtest

### DEC-109 Crowd-sound audibility radius
- Status: Open
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine/ (AEN)
- Specified behaviour: Crowd sounds that play only from their nearest emitter (Worker creation, Worker attack and the Launch Vessel's distress sound) are audible only when an emitter is within about 316 wu of the camera, which can silence them at strategic camera heights. Their audibility is set by an authored priority class (DEC-097), and the default radius of that class is the reference value of about 316 wu.
- Options: A — keep about 316 wu; B — a larger authored radius, or a rule that depends on camera altitude.
- Recommendation: B, after a playtest at several camera heights establishes what players should hear from a 1,000-unit battle.
- Decide by: playtest

### DEC-110 Second tutorial start condition
- Status: Open
- Affects: reference/11_tutorial_and_help.md (R-TUT)
- Specified behaviour: The lessons of the second tutorial begin only when idle player Workers wander within port range of its Command Console, 224.6 wu from a designated player Spawn Node, and the bot captures a designated Spawn Node first, so the lessons may not appear in order.
- Options: A — reproduce the condition; B — the tutorial step graph sets up these states explicitly (DEC-103).
- Recommendation: B.
- Decide by: design phase

### DEC-111 Values derived from flat-terrain calculation
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/02_units/README.md (R-UNIT), reference/02_units/hazards_and_projectiles.md (R-UNIT), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: These values follow from the rules on flat terrain and are confirmed by measurement in the prototype: weapon ranges, the timing of airstrikes and of the Missile Barrage, and Launch Vessel survival (DEC-108). The Collector throughput of about 2.5 Workers per second for a 500 wu round trip is such a value, from a movement law with turn rate 9.5 × dt, velocity filter 0.92 / 0.08 and arrival radius 5 wu; the rocket's outcome of 8 or 9 moves also depends on its effect slot (DEC-455). The formulas are the specification.
- Options: A — adopt the formulas and tune by playtest; B — derive new values before use.
- Recommendation: A, validating the most important values in the first prototype.
- Decide by: playtest

### DEC-112 Bot strength calibration
- Status: Open
- Affects: reference/07_bots/ (R-BOT)
- Specified behaviour: Bot difficulty is set by decision period and randomness (DEC-022). At every difficulty, reference bot strength is shaped by behaviours that DEC-087 corrects: a stall in Leader promotion that fills Power slots with orphaned Leader powers, so the bot loses Supply crate rewards; a stale power id; the Siege defence fall-back; and NaN priorities. Two corrections change Siege and bot defence most: the mobile-bomb branch of the Siege bot Transport behaviour runs only after its correction, and defending formations turn towards the threat, with a threat search centred about 59 wu ahead along the facing and a radius of 90 wu, so defenders miss enemies more than about 31 wu behind them.
- Options: A — keep the reference difficulty parameters after the corrections; B — re-calibrate the parameters by playtest, per mode.
- Recommendation: B. Apply the corrections first, then calibrate each difficulty by playtest rather than by parity, in every mode; Siege attack and Siege defence are calibrated explicitly, because the corrections above change them most.
- Decide by: playtest

### DEC-113 Abductor wander-target area
- Status: Open
- Affects: reference/02_units/hazards_and_projectiles.md (R-UNIT)
- Specified behaviour: An Abductor accepts wander targets only in the quarter of the map where x is in [0, X/2) and z is in [0, Z/2), with X and Z the map size, because its target offsets cover [−r/2, r/2] and every negative coordinate reads as a sea cell.
- Options: A — the reference behaviour; B — sample targets over the whole land area with a bounded candidate list (DEC-014).
- Recommendation: B. The restriction has no visible purpose and concentrates Abductors in one corner of the map.
- Decide by: playtest

### DEC-114 Position of score-weighted crate drops
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE)
- Specified behaviour: A score-weighted Supply crate drop takes its first candidate point whatever the terrain there, so it can land in water, or up to 100 wu outside the map when the structure is near an edge (R-CRATE-106 item 7).
- Options: A — the reference behaviour; B — validate the candidate against land of at least `ref.crate.drop.min_land_height` inside the map, for up to `ref.crate.drop.weighted_attempts` (1000) candidates, with a deterministic fallback (DEC-014).
- Recommendation: B.
- Decide by: playtest

### DEC-115 Fixed match research levels
- Status: Open
- Affects: reference/02_units/README.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: Match start sets the research levels; the match setup carries no research settings (R-META-069, R-MODE-082). Every research level is set to 3, then Strike team to 1, laser, grenade, rocket and airstrike to 2, and Worker to 4. At laser level 2 a Convert Turret's laser lasts 1.2 s.
- Options: A — keep the fixed levels, with no research settings in the match setup; B — add research settings to the match setup and let them apply.
- Recommendation: A. The fixed levels are the tuned combat model (DEC-030).
- Decide by: design phase

### DEC-116 Supply crates during the grace period
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-012), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM-218), reference/00_reference_overview.md (R-VIS-052)
- Specified behaviour: The Supply crate drop rule has no grace-period gate. While the world is held for the start of a match it still runs, once per command frame, so its countdown runs while the teams ready up and during the grace countdown, and a Supply crate can drop before the grace period is over; such a crate stays at its start height until the first world tick. A pause stops the drop rule (R-CRATE-102, R-MODE-044, R-VIS-052, R-SIM-550).
- Options: A — the reference: the drop rule runs during the start hold; B — a grace-period gate: the drop rule runs only on world ticks, so no Supply crate exists when the match starts.
- Recommendation: B. Every other per-tick rule waits for the first world tick (DEC-039, DEC-092), and under A a long ready-up lets crates accumulate up to the map's cap before anyone can move.
- Decide by: playtest

### DEC-117 Reference rules: Strike team grenades, silent Solar Array changes, structure advance and team unit lists
- Status: Resolved
- Affects: reference/02_units/strike_team.md (R-UNIT), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/05_population_flow.md (R-ECON), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM)
- Specified behaviour: These behaviours are specified as rules: a Strike team always throws grenades in a match, because weapon cycling is not available in matches and no map sets another weapon; an empty Solar Array's per-tick team change is silent, because a team-change sound is bound only for Control Zones and Relics, and with one no-team value (DEC-464) the change itself disappears, because it comes from comparing two different no-team values; every structure advances exactly once per tick; a team's list of ungrouped actors holds every actor spawned without an actor group, its bot controller included, while Strike teams, Bombers, centipedes and Hive ants live inside actor groups; and a team's list of special units never holds a removed actor, because each special unit registers when it starts and unregisters when it is removed.
- Options: none.
- Recommendation: Resolved: the owning reference document states each of them as a rule.

## 9 Further integrity rules

### DEC-118 The seasonal state is decided once per match
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/animation_and_vfx/ (VFX)
- Specified behaviour: Whether the seasonal theme is active is match state. It is decided once, in the match setup before match start, recorded with the setup and identical for every participant; no client decides it from its own date or clock, and nothing changes it during the match. Seasonal effects such as snow draw from the presentation generator (DEC-013). Whether Species has a seasonal theme, and who chooses it, is DEC-101.
- Options: A — one seasonal state per match, fixed in the match setup and identical for all participants; B — the reference behaviour, in which each client switches the theme on from its own date check and relays the flag through the command stream into the simulation.
- Recommendation: A. A flag that each participant derives from its own clock makes a match depend on local time and local state, which DEC-009 and DEC-010 exclude; a state fixed in the match setup is part of the recorded match and replays identically.

### DEC-119 Simulation code never issues player commands
- Status: Resolved
- Affects: reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), reference/06_controls_camera_and_commands/ (R-CTRL), architecture/components/input/ (INP)
- Specified behaviour: Player commands come only from a player's input or from an authorised command source, such as a bot or a deputy, and every command passes the same validation path (DEC-202). Executing a command or advancing the simulation never issues a further player command as a side effect: a rule that needs a follow-up change applies it inside the same simulation step. One Leader toggle therefore yields one command and one selection change on every participant.
- Options: A — the simulation never issues player commands, and follow-up changes happen inside the step; B — the reference behaviour, in which executing one Leader toggle command makes every participant issue a further selection command, so one click becomes as many identical commands as there are participants.
- Recommendation: A. A command that the simulation issues is input that no player gave: it multiplies with the participant count and is harmless only while the sequencer keeps one selection per sender and tick. Keeping the simulation a function of the commands it receives is the rule that DEC-008 sets for player intent.
