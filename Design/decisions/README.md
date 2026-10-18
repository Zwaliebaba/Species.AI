# Decision register

> Status: Decision register · Related: [conventions](../CONVENTIONS.md) §3 and §7, [deferred decisions](deferred/README.md), [vision brief](../../vision/species-vision-brief.md)

Entries live in the files listed below; add a decision to the file of its area, in the format of [CONVENTIONS](../CONVENTIONS.md) §7, and add its row to the index of that file and to the index below. **Open** decisions state the behaviour the documents specify now (the reference default), so implementation never blocks on them. **Resolved** decisions state the fixed rule. **Dropped** entries record out-of-scope items and what Species does instead. Decisions marked "Decide by: design phase" are answered by the Species design phase.

Totals: **302** decisions: 148 Open, 126 Resolved, 28 Dropped.

## Numbering

- A DEC id is stable: it is never renumbered or reused, and a decision that no longer applies becomes Dropped and keeps its id, so that every citation stays valid.
- A new decision of the reference library takes the next free id of the register file that owns its topic, within that file's block in the table below.
- The block DEC-900 to DEC-949 is reserved: no register file numbers into it.
- The Species design phase numbers its decisions with four-digit ids, starting at 1000 (CONVENTIONS §3).

## Register files

| File | Scope | Block | Entries |
|---|---|---|---|
| [verdicts_and_fixes.md](verdicts_and_fixes.md) | Headline verdicts, simulation-integrity rules, subsystem scope verdicts, rules kept as the reference states them, reference defects that Species fixes, design-critical uncertainties | DEC-001–119 | 119 |
| [verdicts_and_fixes_2.md](verdicts_and_fixes_2.md) | Verdicts and owner decisions (continued) | DEC-120–199 | 5 |
| [deferred/networking.md](deferred/networking.md) | Deferred: networking | DEC-200–219 | 9 |
| [deferred/process_tick_and_roadmap.md](deferred/process_tick_and_roadmap.md) | Deferred: process, tick rate and roadmap | DEC-220–239 | 10 |
| [deferred/numerics_lobby_terrain_services.md](deferred/numerics_lobby_terrain_services.md) | Deferred: numerics, lobby, terrain runtime, services, persistence | DEC-240–259 | 12 |
| [simulation_gap_rules.md](simulation_gap_rules.md) | Gap rules: simulation | DEC-260–299 | 8 |
| [open_questions.md](open_questions.md) | Open questions | DEC-300–449 | 43 |
| [units_buildings_crates_economy.md](units_buildings_crates_economy.md) | Units, buildings, crates and population | DEC-450–549 | 17 |
| [controls_bots_maps_meta_audio.md](controls_bots_maps_meta_audio.md) | Controls, bots, maps, front end and audio | DEC-550–649 | 21 |
| [simulation_terrain_render_assets.md](simulation_terrain_render_assets.md) | Simulation, terrain, rendering and assets | DEC-650–749 | 5 |
| [vfx_audio_ui.md](vfx_audio_ui.md) | Animation and VFX, audio engine, UI | DEC-750–799 | 3 |
| [data_sequences_platform_debug.md](data_sequences_platform_debug.md) | Data, sequences, platform and diagnostics | DEC-800–849 | 5 |
| [gameplay_gap_rules.md](gameplay_gap_rules.md) | Gap rules: gameplay | DEC-850–899 | 22 |
| — | Reserved: no register file numbers into this block | DEC-900–949 | 0 |
| [engine_gap_rules.md](engine_gap_rules.md) | Gap rules: engine | DEC-950–999 | 23 |

## Index

| Id | Title | Status | File |
|---|---|---|---|
| DEC-001 | [Reference core loop: crowd-and-territory economy](verdicts_and_fixes.md#dec-001-reference-core-loop-crowd-and-territory-economy) | Open | verdicts_and_fixes.md |
| DEC-002 | [Integer tick timers and the 11-tick mode second](verdicts_and_fixes.md#dec-002-integer-tick-timers-and-the-11-tick-mode-second) | Resolved | verdicts_and_fixes.md |
| DEC-003 | [Mode-second length after playtest](verdicts_and_fixes.md#dec-003-mode-second-length-after-playtest) | Open | verdicts_and_fixes.md |
| DEC-004 | [Reference tuning values adopted as the Species baseline](verdicts_and_fixes.md#dec-004-reference-tuning-values-adopted-as-the-species-baseline) | Resolved | verdicts_and_fixes.md |
| DEC-005 | [The reference library is the specification of record](verdicts_and_fixes.md#dec-005-the-reference-library-is-the-specification-of-record) | Resolved | verdicts_and_fixes.md |
| DEC-006 | [Reference quirks are kept as explicit, named rules](verdicts_and_fixes.md#dec-006-reference-quirks-are-kept-as-explicit-named-rules) | Resolved | verdicts_and_fixes.md |
| DEC-007 | [Presentation never writes simulation state; rules emit presentation events](verdicts_and_fixes.md#dec-007-presentation-never-writes-simulation-state-rules-emit-presentation-events) | Resolved | verdicts_and_fixes.md |
| DEC-008 | [Player intent reaches the simulation only as commands](verdicts_and_fixes.md#dec-008-player-intent-reaches-the-simulation-only-as-commands) | Resolved | verdicts_and_fixes.md |
| DEC-009 | [Simulation time comes only from the tick counter](verdicts_and_fixes.md#dec-009-simulation-time-comes-only-from-the-tick-counter) | Resolved | verdicts_and_fixes.md |
| DEC-010 | [Process-global settings never feed the simulation](verdicts_and_fixes.md#dec-010-process-global-settings-never-feed-the-simulation) | Resolved | verdicts_and_fixes.md |
| DEC-011 | [Per-match state is owned and reset at every match start](verdicts_and_fixes.md#dec-011-per-match-state-is-owned-and-reset-at-every-match-start) | Resolved | verdicts_and_fixes.md |
| DEC-012 | [Collision volumes are map and asset data](verdicts_and_fixes.md#dec-012-collision-volumes-are-map-and-asset-data) | Resolved | verdicts_and_fixes.md |
| DEC-013 | [Random-stream discipline: per-match seed and a separate presentation generator](verdicts_and_fixes.md#dec-013-random-stream-discipline-per-match-seed-and-a-separate-presentation-generator) | Resolved | verdicts_and_fixes.md |
| DEC-014 | [Bounded algorithms with a deterministic fallback](verdicts_and_fixes.md#dec-014-bounded-algorithms-with-a-deterministic-fallback) | Resolved | verdicts_and_fixes.md |
| DEC-015 | [Content is validated at build time, and no load path is fatal](verdicts_and_fixes.md#dec-015-content-is-validated-at-build-time-and-no-load-path-is-fatal) | Resolved | verdicts_and_fixes.md |
| DEC-016 | [Tie-breaks are explicit ordered rules](verdicts_and_fixes.md#dec-016-tie-breaks-are-explicit-ordered-rules) | Resolved | verdicts_and_fixes.md |
| DEC-017 | [Match rules and modes: one rules skeleton, typed rulesets](verdicts_and_fixes.md#dec-017-match-rules-and-modes-one-rules-skeleton-typed-rulesets) | Resolved | verdicts_and_fixes.md |
| DEC-018 | [Population flow: keep the model and its numbers](verdicts_and_fixes.md#dec-018-population-flow-keep-the-model-and-its-numbers) | Resolved | verdicts_and_fixes.md |
| DEC-019 | [Units and actors: keep the roster, adapt the data representation](verdicts_and_fixes.md#dec-019-units-and-actors-keep-the-roster-adapt-the-data-representation) | Resolved | verdicts_and_fixes.md |
| DEC-020 | [Structures and world objects: keep the function, apply the corrections](verdicts_and_fixes.md#dec-020-structures-and-world-objects-keep-the-function-apply-the-corrections) | Resolved | verdicts_and_fixes.md |
| DEC-021 | [Supply crates, powers and weapons: keep the outcome set, bound the selection](verdicts_and_fixes.md#dec-021-supply-crates-powers-and-weapons-keep-the-outcome-set-bound-the-selection) | Resolved | verdicts_and_fixes.md |
| DEC-022 | [Bots: keep the design, apply the corrections](verdicts_and_fixes.md#dec-022-bots-keep-the-design-apply-the-corrections) | Resolved | verdicts_and_fixes.md |
| DEC-023 | [Controls and commands: keep the command model and control scheme](verdicts_and_fixes.md#dec-023-controls-and-commands-keep-the-command-model-and-control-scheme) | Resolved | verdicts_and_fixes.md |
| DEC-024 | [Camera: keep the free camera, clamp it and use one presentation clock](verdicts_and_fixes.md#dec-024-camera-keep-the-free-camera-clamp-it-and-use-one-presentation-clock) | Resolved | verdicts_and_fixes.md |
| DEC-025 | [Maps and level design: keep the capability set and the generator look](verdicts_and_fixes.md#dec-025-maps-and-level-design-keep-the-capability-set-and-the-generator-look) | Resolved | verdicts_and_fixes.md |
| DEC-026 | [Front end and match flow: keep the flow and its page catalogue](verdicts_and_fixes.md#dec-026-front-end-and-match-flow-keep-the-flow-and-its-page-catalogue) | Resolved | verdicts_and_fixes.md |
| DEC-027 | [Audio design: keep the binding model, replace priorities and the mix](verdicts_and_fixes.md#dec-027-audio-design-keep-the-binding-model-replace-priorities-and-the-mix) | Resolved | verdicts_and_fixes.md |
| DEC-028 | [Simulation model: keep the object model, replace weak mechanisms](verdicts_and_fixes.md#dec-028-simulation-model-keep-the-object-model-replace-weak-mechanisms) | Resolved | verdicts_and_fixes.md |
| DEC-029 | [Visual language is kept](verdicts_and_fixes.md#dec-029-visual-language-is-kept) | Resolved | verdicts_and_fixes.md |
| DEC-030 | [Worker autonomy and armament](verdicts_and_fixes.md#dec-030-worker-autonomy-and-armament) | Resolved | verdicts_and_fixes.md |
| DEC-031 | [Leader signposts and formations](verdicts_and_fixes.md#dec-031-leader-signposts-and-formations) | Resolved | verdicts_and_fixes.md |
| DEC-032 | [Selection circle and crowd-order interval](verdicts_and_fixes.md#dec-032-selection-circle-and-crowd-order-interval) | Resolved | verdicts_and_fixes.md |
| DEC-033 | [Six Power slots, and a grant is lost when they are full](verdicts_and_fixes.md#dec-033-six-power-slots-and-a-grant-is-lost-when-they-are-full) | Resolved | verdicts_and_fixes.md |
| DEC-034 | [Co-op alliances](verdicts_and_fixes.md#dec-034-co-op-alliances) | Resolved | verdicts_and_fixes.md |
| DEC-035 | [A leaving player is replaced by a bot](verdicts_and_fixes.md#dec-035-a-leaving-player-is-replaced-by-a-bot) | Resolved | verdicts_and_fixes.md |
| DEC-036 | [Ready-up barrier, countdown and grace period](verdicts_and_fixes.md#dec-036-ready-up-barrier-countdown-and-grace-period) | Resolved | verdicts_and_fixes.md |
| DEC-037 | [Onboarding helpers](verdicts_and_fixes.md#dec-037-onboarding-helpers) | Resolved | verdicts_and_fixes.md |
| DEC-038 | [Pause and the speed ramp](verdicts_and_fixes.md#dec-038-pause-and-the-speed-ramp) | Resolved | verdicts_and_fixes.md |
| DEC-039 | [Speed crates are tick scheduling](verdicts_and_fixes.md#dec-039-speed-crates-are-tick-scheduling) | Resolved | verdicts_and_fixes.md |
| DEC-040 | [Inverted ink-on-paper style](verdicts_and_fixes.md#dec-040-inverted-ink-on-paper-style) | Open | verdicts_and_fixes.md |
| DEC-041 | [Development and diagnostics tooling](verdicts_and_fixes.md#dec-041-development-and-diagnostics-tooling) | Resolved | verdicts_and_fixes.md |
| DEC-042 | [Volumes are authored in decibels](verdicts_and_fixes.md#dec-042-volumes-are-authored-in-decibels) | Resolved | verdicts_and_fixes.md |
| DEC-043 | [Wall destructibility](verdicts_and_fixes.md#dec-043-wall-destructibility) | Open | verdicts_and_fixes.md |
| DEC-044 | [Commonly expected options](verdicts_and_fixes.md#dec-044-commonly-expected-options) | Open | verdicts_and_fixes.md |
| DEC-045 | [Map editor](verdicts_and_fixes.md#dec-045-map-editor) | Open | verdicts_and_fixes.md |
| DEC-046 | [Replays](verdicts_and_fixes.md#dec-046-replays) | Open | verdicts_and_fixes.md |
| DEC-047 | [Achievements, statistics and leaderboards](verdicts_and_fixes.md#dec-047-achievements-statistics-and-leaderboards) | Open | verdicts_and_fixes.md |
| DEC-048 | [Post-match results screen](verdicts_and_fixes.md#dec-048-post-match-results-screen) | Open | verdicts_and_fixes.md |
| DEC-049 | [Music](verdicts_and_fixes.md#dec-049-music) | Open | verdicts_and_fixes.md |
| DEC-050 | [Front-end features designed new](verdicts_and_fixes.md#dec-050-front-end-features-designed-new) | Open | verdicts_and_fixes.md |
| DEC-051 | [Tutorials and presentation timelines](verdicts_and_fixes.md#dec-051-tutorials-and-presentation-timelines) | Open | verdicts_and_fixes.md |
| DEC-052 | [Single-player campaign and narrator](verdicts_and_fixes.md#dec-052-single-player-campaign-and-narrator) | Dropped | verdicts_and_fixes.md |
| DEC-053 | [Script interpreter and cut-scene scripts](verdicts_and_fixes.md#dec-053-script-interpreter-and-cut-scene-scripts) | Dropped | verdicts_and_fixes.md |
| DEC-054 | [Loader mini-programs, intro start sequence and attract mode](verdicts_and_fixes.md#dec-054-loader-mini-programs-intro-start-sequence-and-attract-mode) | Dropped | verdicts_and_fixes.md |
| DEC-055 | [Gesture input and the icon power bar](verdicts_and_fixes.md#dec-055-gesture-input-and-the-icon-power-bar) | Dropped | verdicts_and_fixes.md |
| DEC-056 | [Alternative control scheme](verdicts_and_fixes.md#dec-056-alternative-control-scheme) | Dropped | verdicts_and_fixes.md |
| DEC-057 | [Rift Horror outcome and inactive crate rewards](verdicts_and_fixes.md#dec-057-rift-horror-outcome-and-inactive-crate-rewards) | Dropped | verdicts_and_fixes.md |
| DEC-058 | [Vehicle Duel mode](verdicts_and_fixes.md#dec-058-vehicle-duel-mode) | Dropped | verdicts_and_fixes.md |
| DEC-059 | [Pixelation, deform and reflection effects](verdicts_and_fixes.md#dec-059-pixelation-deform-and-reflection-effects) | Dropped | verdicts_and_fixes.md |
| DEC-060 | [Benchmark recorder, upload and in-game video capture](verdicts_and_fixes.md#dec-060-benchmark-recorder-upload-and-in-game-video-capture) | Dropped | verdicts_and_fixes.md |
| DEC-061 | [User profiles and mods](verdicts_and_fixes.md#dec-061-user-profiles-and-mods) | Dropped | verdicts_and_fixes.md |
| DEC-062 | [Actor kinds that no match creates](verdicts_and_fixes.md#dec-062-actor-kinds-that-no-match-creates) | Dropped | verdicts_and_fixes.md |
| DEC-063 | [Reference map shapes and map identities](verdicts_and_fixes.md#dec-063-reference-map-shapes-and-map-identities) | Dropped | verdicts_and_fixes.md |
| DEC-064 | [One fixed recount period](verdicts_and_fixes.md#dec-064-one-fixed-recount-period) | Resolved | verdicts_and_fixes.md |
| DEC-065 | [Transit Gates advance once per tick, and map values override lobby defaults](verdicts_and_fixes.md#dec-065-transit-gates-advance-once-per-tick-and-map-values-override-lobby-defaults) | Resolved | verdicts_and_fixes.md |
| DEC-066 | [Multi-round Siege](verdicts_and_fixes.md#dec-066-multi-round-siege) | Open | verdicts_and_fixes.md |
| DEC-067 | [Map defaults are applied once, by the host](verdicts_and_fixes.md#dec-067-map-defaults-are-applied-once-by-the-host) | Resolved | verdicts_and_fixes.md |
| DEC-068 | [The Flag Chain time-up draw uses valid team indices](verdicts_and_fixes.md#dec-068-the-flag-chain-time-up-draw-uses-valid-team-indices) | Resolved | verdicts_and_fixes.md |
| DEC-069 | [The co-op game-over line shows the declared result](verdicts_and_fixes.md#dec-069-the-co-op-game-over-line-shows-the-declared-result) | Resolved | verdicts_and_fixes.md |
| DEC-070 | [The Spawn Control co-op winner check covers every slot](verdicts_and_fixes.md#dec-070-the-spawn-control-co-op-winner-check-covers-every-slot) | Resolved | verdicts_and_fixes.md |
| DEC-071 | [Flag Chain base-under-attack announcement interval](verdicts_and_fixes.md#dec-071-flag-chain-base-under-attack-announcement-interval) | Open | verdicts_and_fixes.md |
| DEC-072 | [The final countdown sound plays at time-up](verdicts_and_fixes.md#dec-072-the-final-countdown-sound-plays-at-time-up) | Resolved | verdicts_and_fixes.md |
| DEC-073 | [Typed match options with explicit per-mode defaults](verdicts_and_fixes.md#dec-073-typed-match-options-with-explicit-per-mode-defaults) | Resolved | verdicts_and_fixes.md |
| DEC-074 | [Rank under ties](verdicts_and_fixes.md#dec-074-rank-under-ties) | Resolved | verdicts_and_fixes.md |
| DEC-075 | [The Core Device result is fixed at detonation](verdicts_and_fixes.md#dec-075-the-core-device-result-is-fixed-at-detonation) | Resolved | verdicts_and_fixes.md |
| DEC-076 | [Launch Vessel Flight is invulnerable and terminal](verdicts_and_fixes.md#dec-076-launch-vessel-flight-is-invulnerable-and-terminal) | Resolved | verdicts_and_fixes.md |
| DEC-077 | [The bot-mode option accepts only defined values](verdicts_and_fixes.md#dec-077-the-bot-mode-option-accepts-only-defined-values) | Resolved | verdicts_and_fixes.md |
| DEC-078 | [No-team colour, under-attack status and the neutral Home post](verdicts_and_fixes.md#dec-078-no-team-colour-under-attack-status-and-the-neutral-home-post) | Resolved | verdicts_and_fixes.md |
| DEC-079 | [Per-team population cap](verdicts_and_fixes.md#dec-079-per-team-population-cap) | Resolved | verdicts_and_fixes.md |
| DEC-080 | [Collector collection of any team's Essence orbs](verdicts_and_fixes.md#dec-080-collector-collection-of-any-teams-essence-orbs) | Resolved | verdicts_and_fixes.md |
| DEC-081 | [Catch-up bonus curve](verdicts_and_fixes.md#dec-081-catch-up-bonus-curve) | Resolved | verdicts_and_fixes.md |
| DEC-082 | [Rogue and Visitor factions never win and never block elimination](verdicts_and_fixes.md#dec-082-rogue-and-visitor-factions-never-win-and-never-block-elimination) | Resolved | verdicts_and_fixes.md |
| DEC-083 | [Clickable power bar and wide power ids](verdicts_and_fixes.md#dec-083-clickable-power-bar-and-wide-power-ids) | Resolved | verdicts_and_fixes.md |
| DEC-084 | [Hive Essence orbs and Hive ants](verdicts_and_fixes.md#dec-084-hive-essence-orbs-and-hive-ants) | Open | verdicts_and_fixes.md |
| DEC-085 | [Builder Essence-orb delivery to friendly Spawn Nodes](verdicts_and_fixes.md#dec-085-builder-essence-orb-delivery-to-friendly-spawn-nodes) | Resolved | verdicts_and_fixes.md |
| DEC-086 | [Separate demote and scrap actions for Leaders](verdicts_and_fixes.md#dec-086-separate-demote-and-scrap-actions-for-leaders) | Resolved | verdicts_and_fixes.md |
| DEC-087 | [Bot corrections](verdicts_and_fixes.md#dec-087-bot-corrections) | Resolved | verdicts_and_fixes.md |
| DEC-088 | [Structure corrections](verdicts_and_fixes.md#dec-088-structure-corrections) | Resolved | verdicts_and_fixes.md |
| DEC-089 | [Smoke-column obstacle](verdicts_and_fixes.md#dec-089-smoke-column-obstacle) | Open | verdicts_and_fixes.md |
| DEC-090 | [Floating-point actor stats and the Abductor speed](verdicts_and_fixes.md#dec-090-floating-point-actor-stats-and-the-abductor-speed) | Resolved | verdicts_and_fixes.md |
| DEC-091 | [Dormant Crawler egg lifetime](verdicts_and_fixes.md#dec-091-dormant-crawler-egg-lifetime) | Open | verdicts_and_fixes.md |
| DEC-092 | [Per-tick rules scale with game speed](verdicts_and_fixes.md#dec-092-per-tick-rules-scale-with-game-speed) | Resolved | verdicts_and_fixes.md |
| DEC-093 | [Explosion damage falloff](verdicts_and_fixes.md#dec-093-explosion-damage-falloff) | Resolved | verdicts_and_fixes.md |
| DEC-094 | [Exact or approximate actor-grid queries](verdicts_and_fixes.md#dec-094-exact-or-approximate-actor-grid-queries) | Open | verdicts_and_fixes.md |
| DEC-095 | [Obstacle grid and route upkeep; route previews kept separate](verdicts_and_fixes.md#dec-095-obstacle-grid-and-route-upkeep-route-previews-kept-separate) | Resolved | verdicts_and_fixes.md |
| DEC-096 | [Input corrections](verdicts_and_fixes.md#dec-096-input-corrections) | Resolved | verdicts_and_fixes.md |
| DEC-097 | [Audio corrections](verdicts_and_fixes.md#dec-097-audio-corrections) | Resolved | verdicts_and_fixes.md |
| DEC-098 | [Texture cache keyed by asset and variant](verdicts_and_fixes.md#dec-098-texture-cache-keyed-by-asset-and-variant) | Resolved | verdicts_and_fixes.md |
| DEC-099 | [Localisation fallback and language completeness](verdicts_and_fixes.md#dec-099-localisation-fallback-and-language-completeness) | Resolved | verdicts_and_fixes.md |
| DEC-100 | [One defaults table for preferences](verdicts_and_fixes.md#dec-100-one-defaults-table-for-preferences) | Resolved | verdicts_and_fixes.md |
| DEC-101 | [Seasonal theme](verdicts_and_fixes.md#dec-101-seasonal-theme) | Open | verdicts_and_fixes.md |
| DEC-102 | [UI framework corrections by construction](verdicts_and_fixes.md#dec-102-ui-framework-corrections-by-construction) | Resolved | verdicts_and_fixes.md |
| DEC-103 | [Tutorials are step graphs with simulation-sequenced effects](verdicts_and_fixes.md#dec-103-tutorials-are-step-graphs-with-simulation-sequenced-effects) | Resolved | verdicts_and_fixes.md |
| DEC-104 | [Formation volley rhythm in integer arithmetic](verdicts_and_fixes.md#dec-104-formation-volley-rhythm-in-integer-arithmetic) | Resolved | verdicts_and_fixes.md |
| DEC-105 | [Worker grenade team and the Hive and Spitter Plant attack target](verdicts_and_fixes.md#dec-105-worker-grenade-team-and-the-hive-and-spitter-plant-attack-target) | Resolved | verdicts_and_fixes.md |
| DEC-106 | [Performance envelope at the population cap](verdicts_and_fixes.md#dec-106-performance-envelope-at-the-population-cap) | Open | verdicts_and_fixes.md |
| DEC-107 | [Hostile Supply crate outcomes: ownership and filtering](verdicts_and_fixes.md#dec-107-hostile-supply-crate-outcomes-ownership-and-filtering) | Resolved | verdicts_and_fixes.md |
| DEC-108 | [Launch Vessel survival under attack](verdicts_and_fixes.md#dec-108-launch-vessel-survival-under-attack) | Open | verdicts_and_fixes.md |
| DEC-109 | [Crowd-sound audibility radius](verdicts_and_fixes.md#dec-109-crowd-sound-audibility-radius) | Open | verdicts_and_fixes.md |
| DEC-110 | [Second tutorial start condition](verdicts_and_fixes.md#dec-110-second-tutorial-start-condition) | Open | verdicts_and_fixes.md |
| DEC-111 | [Values derived from flat-terrain calculation](verdicts_and_fixes.md#dec-111-values-derived-from-flat-terrain-calculation) | Open | verdicts_and_fixes.md |
| DEC-112 | [Bot strength calibration](verdicts_and_fixes.md#dec-112-bot-strength-calibration) | Open | verdicts_and_fixes.md |
| DEC-113 | [Abductor wander-target area](verdicts_and_fixes.md#dec-113-abductor-wander-target-area) | Open | verdicts_and_fixes.md |
| DEC-114 | [Position of score-weighted crate drops](verdicts_and_fixes.md#dec-114-position-of-score-weighted-crate-drops) | Open | verdicts_and_fixes.md |
| DEC-115 | [Fixed match research levels](verdicts_and_fixes.md#dec-115-fixed-match-research-levels) | Open | verdicts_and_fixes.md |
| DEC-116 | [Supply crates during the grace period](verdicts_and_fixes.md#dec-116-supply-crates-during-the-grace-period) | Open | verdicts_and_fixes.md |
| DEC-117 | [Reference rules: Strike team grenades, silent Solar Array changes, structure advance and team unit lists](verdicts_and_fixes.md#dec-117-reference-rules-strike-team-grenades-silent-solar-array-changes-structure-advance-and-team-unit-lists) | Resolved | verdicts_and_fixes.md |
| DEC-118 | [The seasonal state is decided once per match](verdicts_and_fixes.md#dec-118-the-seasonal-state-is-decided-once-per-match) | Resolved | verdicts_and_fixes.md |
| DEC-119 | [Simulation code never issues player commands](verdicts_and_fixes.md#dec-119-simulation-code-never-issues-player-commands) | Resolved | verdicts_and_fixes.md |
| DEC-120 | [Map values: absent, zero and unlimited are distinct](verdicts_and_fixes_2.md#dec-120-map-values-absent-zero-and-unlimited-are-distinct) | Resolved | verdicts_and_fixes_2.md |
| DEC-121 | [Ownership of Brood creatures](verdicts_and_fixes_2.md#dec-121-ownership-of-brood-creatures) | Open | verdicts_and_fixes_2.md |
| DEC-122 | [Bot and deputy actions outside the player command catalogue](verdicts_and_fixes_2.md#dec-122-bot-and-deputy-actions-outside-the-player-command-catalogue) | Open | verdicts_and_fixes_2.md |
| DEC-123 | [Tutorial for the first playable](verdicts_and_fixes_2.md#dec-123-tutorial-for-the-first-playable) | Open | verdicts_and_fixes_2.md |
| DEC-124 | [Proportional Catch-up bonus curve](verdicts_and_fixes_2.md#dec-124-proportional-catch-up-bonus-curve) | Resolved | verdicts_and_fixes_2.md |
| DEC-200 | [Replication and authority model](deferred/networking.md#dec-200-replication-and-authority-model) | Open | deferred/networking.md |
| DEC-201 | [Replication bandwidth and population budget](deferred/networking.md#dec-201-replication-bandwidth-and-population-budget) | Open | deferred/networking.md |
| DEC-202 | [Command validation and authority](deferred/networking.md#dec-202-command-validation-and-authority) | Open | deferred/networking.md |
| DEC-203 | [Input timing, coalescing and latency](deferred/networking.md#dec-203-input-timing-coalescing-and-latency) | Open | deferred/networking.md |
| DEC-204 | [Content and version identity](deferred/networking.md#dec-204-content-and-version-identity) | Open | deferred/networking.md |
| DEC-205 | [Join, reconnect, leave and session continuity](deferred/networking.md#dec-205-join-reconnect-leave-and-session-continuity) | Open | deferred/networking.md |
| DEC-206 | [Divergence detection and integrity checking](deferred/networking.md#dec-206-divergence-detection-and-integrity-checking) | Open | deferred/networking.md |
| DEC-207 | [Transport, message format and robustness](deferred/networking.md#dec-207-transport-message-format-and-robustness) | Open | deferred/networking.md |
| DEC-208 | [Network diagnostics and test tooling](deferred/networking.md#dec-208-network-diagnostics-and-test-tooling) | Open | deferred/networking.md |
| DEC-220 | [Process roles and the headless server build](deferred/process_tick_and_roadmap.md#dec-220-process-roles-and-the-headless-server-build) | Open | deferred/process_tick_and_roadmap.md |
| DEC-221 | [Composition root, service lifetimes and the application state machine](deferred/process_tick_and_roadmap.md#dec-221-composition-root-service-lifetimes-and-the-application-state-machine) | Open | deferred/process_tick_and_roadmap.md |
| DEC-222 | [Simulation and presentation boundary](deferred/process_tick_and_roadmap.md#dec-222-simulation-and-presentation-boundary) | Open | deferred/process_tick_and_roadmap.md |
| DEC-223 | [World, region and match state ownership and reset](deferred/process_tick_and_roadmap.md#dec-223-world-region-and-match-state-ownership-and-reset) | Open | deferred/process_tick_and_roadmap.md |
| DEC-224 | [Threading and job model](deferred/process_tick_and_roadmap.md#dec-224-threading-and-job-model) | Open | deferred/process_tick_and_roadmap.md |
| DEC-225 | [Frame loop, simulation pacing and catch-up](deferred/process_tick_and_roadmap.md#dec-225-frame-loop-simulation-pacing-and-catch-up) | Open | deferred/process_tick_and_roadmap.md |
| DEC-226 | [Loading, streaming and asynchronous work](deferred/process_tick_and_roadmap.md#dec-226-loading-streaming-and-asynchronous-work) | Open | deferred/process_tick_and_roadmap.md |
| DEC-227 | [Simulation tick rate](deferred/process_tick_and_roadmap.md#dec-227-simulation-tick-rate) | Open | deferred/process_tick_and_roadmap.md |
| DEC-228 | [Per-tick CPU budget, population caps and stress populations](deferred/process_tick_and_roadmap.md#dec-228-per-tick-cpu-budget-population-caps-and-stress-populations) | Open | deferred/process_tick_and_roadmap.md |
| DEC-229 | [Build order, milestones and work breakdown](deferred/process_tick_and_roadmap.md#dec-229-build-order-milestones-and-work-breakdown) | Open | deferred/process_tick_and_roadmap.md |
| DEC-240 | [Simulation numeric model and determinism scope](deferred/numerics_lobby_terrain_services.md#dec-240-simulation-numeric-model-and-determinism-scope) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-241 | [Numeric semantics that reference rules depend on](deferred/numerics_lobby_terrain_services.md#dec-241-numeric-semantics-that-reference-rules-depend-on) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-242 | [Simulation random generator algorithm and stream derivation](deferred/numerics_lobby_terrain_services.md#dec-242-simulation-random-generator-algorithm-and-stream-derivation) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-243 | [Lobby authority and the match-setup state](deferred/numerics_lobby_terrain_services.md#dec-243-lobby-authority-and-the-match-setup-state) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-244 | [Session join, admission, reconnection and spectators](deferred/numerics_lobby_terrain_services.md#dec-244-session-join-admission-reconnection-and-spectators) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-245 | [Session discovery, the advertised session record and the session browser](deferred/numerics_lobby_terrain_services.md#dec-245-session-discovery-the-advertised-session-record-and-the-session-browser) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-246 | [Terrain runtime model and terrain content identity](deferred/numerics_lobby_terrain_services.md#dec-246-terrain-runtime-model-and-terrain-content-identity) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-247 | [Terrain scale and large-world terrain inputs](deferred/numerics_lobby_terrain_services.md#dec-247-terrain-scale-and-large-world-terrain-inputs) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-248 | [Player identity and entitlement](deferred/numerics_lobby_terrain_services.md#dec-248-player-identity-and-entitlement) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-249 | [Directory, rendezvous, relay and matchmaking services](deferred/numerics_lobby_terrain_services.md#dec-249-directory-rendezvous-relay-and-matchmaking-services) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-250 | [Persisted data inventory and client-local storage](deferred/numerics_lobby_terrain_services.md#dec-250-persisted-data-inventory-and-client-local-storage) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-251 | [World-state persistence, schema migration and rollback](deferred/numerics_lobby_terrain_services.md#dec-251-world-state-persistence-schema-migration-and-rollback) | Open | deferred/numerics_lobby_terrain_services.md |
| DEC-260 | [Order within an actor-grid cell and container iteration order](simulation_gap_rules.md#dec-260-order-within-an-actor-grid-cell-and-container-iteration-order) | Resolved | simulation_gap_rules.md |
| DEC-261 | [Multi-round Siege rebuilds the match world at a tick boundary](simulation_gap_rules.md#dec-261-multi-round-siege-rebuilds-the-match-world-at-a-tick-boundary) | Resolved | simulation_gap_rules.md |
| DEC-262 | [Generations of laser-bolt and Essence-orb handles, and the team selection](simulation_gap_rules.md#dec-262-generations-of-laser-bolt-and-essence-orb-handles-and-the-team-selection) | Resolved | simulation_gap_rules.md |
| DEC-263 | [Route and actor-group edge cases](simulation_gap_rules.md#dec-263-route-and-actor-group-edge-cases) | Open | simulation_gap_rules.md |
| DEC-264 | [What runs while game speed is 0](simulation_gap_rules.md#dec-264-what-runs-while-game-speed-is-0) | Open | simulation_gap_rules.md |
| DEC-265 | [Real duration of the speed-crate holds](simulation_gap_rules.md#dec-265-real-duration-of-the-speed-crate-holds) | Open | simulation_gap_rules.md |
| DEC-266 | [Decorative objects in the simulated pools](simulation_gap_rules.md#dec-266-decorative-objects-in-the-simulated-pools) | Open | simulation_gap_rules.md |
| DEC-267 | [Barrier Emitter line in the obstacle grid](simulation_gap_rules.md#dec-267-barrier-emitter-line-in-the-obstacle-grid) | Open | simulation_gap_rules.md |
| DEC-300 | [Return of teams wiped out in sudden death](open_questions.md#dec-300-return-of-teams-wiped-out-in-sudden-death) | Open | open_questions.md |
| DEC-301 | [Solar Array knock-out in Siege](open_questions.md#dec-301-solar-array-knock-out-in-siege) | Open | open_questions.md |
| DEC-302 | [Transport unload order over sea](open_questions.md#dec-302-transport-unload-order-over-sea) | Open | open_questions.md |
| DEC-303 | [Missile Barrage arrival spread](open_questions.md#dec-303-missile-barrage-arrival-spread) | Open | open_questions.md |
| DEC-304 | [Spawn Surge request count](open_questions.md#dec-304-spawn-surge-request-count) | Open | open_questions.md |
| DEC-305 | [Essence Grove orbs of team slots not in play](open_questions.md#dec-305-essence-grove-orbs-of-team-slots-not-in-play) | Open | open_questions.md |
| DEC-306 | [Spawn-network transit time and update order](open_questions.md#dec-306-spawn-network-transit-time-and-update-order) | Dropped | open_questions.md |
| DEC-307 | [Essence orbs held by a removed Collector](open_questions.md#dec-307-essence-orbs-held-by-a-removed-collector) | Open | open_questions.md |
| DEC-308 | [Orders to Workers that operate a port](open_questions.md#dec-308-orders-to-workers-that-operate-a-port) | Open | open_questions.md |
| DEC-309 | [Global flag on Source Nodes](open_questions.md#dec-309-global-flag-on-source-nodes) | Open | open_questions.md |
| DEC-310 | [Camera tracking after selecting a Leader or a Transport](open_questions.md#dec-310-camera-tracking-after-selecting-a-leader-or-a-transport) | Open | open_questions.md |
| DEC-311 | [Bot controller as a target](open_questions.md#dec-311-bot-controller-as-a-target) | Open | open_questions.md |
| DEC-312 | [Bot decisions and node recounts within a tick](open_questions.md#dec-312-bot-decisions-and-node-recounts-within-a-tick) | Open | open_questions.md |
| DEC-313 | [Dead airborne actors](open_questions.md#dec-313-dead-airborne-actors) | Open | open_questions.md |
| DEC-314 | [Map list order under ties](open_questions.md#dec-314-map-list-order-under-ties) | Open | open_questions.md |
| DEC-315 | [Level-design patterns of the reference maps](open_questions.md#dec-315-level-design-patterns-of-the-reference-maps) | Open | open_questions.md |
| DEC-316 | [Front-end text blend and page transition](open_questions.md#dec-316-front-end-text-blend-and-page-transition) | Open | open_questions.md |
| DEC-317 | [Voice set language](open_questions.md#dec-317-voice-set-language) | Open | open_questions.md |
| DEC-318 | [Sound triggers tied to the frame rate](open_questions.md#dec-318-sound-triggers-tied-to-the-frame-rate) | Open | open_questions.md |
| DEC-319 | [Ambience during loading views](open_questions.md#dec-319-ambience-during-loading-views) | Open | open_questions.md |
| DEC-320 | [Match-end pitch drop](open_questions.md#dec-320-match-end-pitch-drop) | Open | open_questions.md |
| DEC-321 | [Attached sounds when their emitter is removed](open_questions.md#dec-321-attached-sounds-when-their-emitter-is-removed) | Open | open_questions.md |
| DEC-322 | [Start value of smoothed sound parameters](open_questions.md#dec-322-start-value-of-smoothed-sound-parameters) | Open | open_questions.md |
| DEC-323 | [Loop keep-alive polling](open_questions.md#dec-323-loop-keep-alive-polling) | Resolved | open_questions.md |
| DEC-324 | [Terrain generator parameter semantics and biases](open_questions.md#dec-324-terrain-generator-parameter-semantics-and-biases) | Open | open_questions.md |
| DEC-325 | [Light direction and the camera transform](open_questions.md#dec-325-light-direction-and-the-camera-transform) | Open | open_questions.md |
| DEC-326 | [Terrain grid overlay](open_questions.md#dec-326-terrain-grid-overlay) | Open | open_questions.md |
| DEC-327 | [Structure render-detail variants](open_questions.md#dec-327-structure-render-detail-variants) | Open | open_questions.md |
| DEC-328 | [Saturated team colours on effect bubbles](open_questions.md#dec-328-saturated-team-colours-on-effect-bubbles) | Open | open_questions.md |
| DEC-329 | [Assets of objects spawned only at run time](open_questions.md#dec-329-assets-of-objects-spawned-only-at-run-time) | Open | open_questions.md |
| DEC-330 | [Mesh sockets referenced only by data](open_questions.md#dec-330-mesh-sockets-referenced-only-by-data) | Open | open_questions.md |
| DEC-331 | [Reference rules: the convert tag and the victory celebration](open_questions.md#dec-331-reference-rules-the-convert-tag-and-the-victory-celebration) | Resolved | open_questions.md |
| DEC-332 | [Content that no reference match uses](open_questions.md#dec-332-content-that-no-reference-match-uses) | Dropped | open_questions.md |
| DEC-333 | [Entitlement and edition gating](open_questions.md#dec-333-entitlement-and-edition-gating) | Dropped | open_questions.md |
| DEC-334 | [Development, test and benchmark maps](open_questions.md#dec-334-development-test-and-benchmark-maps) | Dropped | open_questions.md |
| DEC-335 | [Online-service listing fields](open_questions.md#dec-335-online-service-listing-fields) | Dropped | open_questions.md |
| DEC-336 | [Other renderers](open_questions.md#dec-336-other-renderers) | Dropped | open_questions.md |
| DEC-337 | [Graphics-driver, cursor and glyph-atlas behaviour](open_questions.md#dec-337-graphics-driver-cursor-and-glyph-atlas-behaviour) | Dropped | open_questions.md |
| DEC-338 | [Input-stack implementation details](open_questions.md#dec-338-input-stack-implementation-details) | Dropped | open_questions.md |
| DEC-339 | [Audio device details](open_questions.md#dec-339-audio-device-details) | Dropped | open_questions.md |
| DEC-340 | [Data formats, archives and production tools](open_questions.md#dec-340-data-formats-archives-and-production-tools) | Dropped | open_questions.md |
| DEC-341 | [Toolchain and debug-build details](open_questions.md#dec-341-toolchain-and-debug-build-details) | Dropped | open_questions.md |
| DEC-342 | [Reference art and audio](open_questions.md#dec-342-reference-art-and-audio) | Dropped | open_questions.md |
| DEC-450 | [Start positions and the colours of bot and unused slots](units_buildings_crates_economy.md#dec-450-start-positions-and-the-colours-of-bot-and-unused-slots) | Open | units_buildings_crates_economy.md |
| DEC-451 | [Co-op friendship of slots outside the two alliances](units_buildings_crates_economy.md#dec-451-co-op-friendship-of-slots-outside-the-two-alliances) | Open | units_buildings_crates_economy.md |
| DEC-452 | [Comeback Bonus power pool and the map's excluded outcomes](units_buildings_crates_economy.md#dec-452-comeback-bonus-power-pool-and-the-maps-excluded-outcomes) | Resolved | units_buildings_crates_economy.md |
| DEC-453 | [Population-cap enforcement points and uncapped sources](units_buildings_crates_economy.md#dec-453-population-cap-enforcement-points-and-uncapped-sources) | Open | units_buildings_crates_economy.md |
| DEC-454 | [Population Limiter as a fallback cap](units_buildings_crates_economy.md#dec-454-population-limiter-as-a-fallback-cap) | Open | units_buildings_crates_economy.md |
| DEC-455 | [Same-tick advance of objects created or handed over during a tick](units_buildings_crates_economy.md#dec-455-same-tick-advance-of-objects-created-or-handed-over-during-a-tick) | Open | units_buildings_crates_economy.md |
| DEC-456 | [Spawn networks are trees, and an Essence orb is on one edge at a time](units_buildings_crates_economy.md#dec-456-spawn-networks-are-trees-and-an-essence-orb-is-on-one-edge-at-a-time) | Resolved | units_buildings_crates_economy.md |
| DEC-457 | [Structure links are type-checked](units_buildings_crates_economy.md#dec-457-structure-links-are-type-checked) | Resolved | units_buildings_crates_economy.md |
| DEC-458 | [Linking a Command Console to the Core Device rebuilds its ports](units_buildings_crates_economy.md#dec-458-linking-a-command-console-to-the-core-device-rebuilds-its-ports) | Resolved | units_buildings_crates_economy.md |
| DEC-459 | [Core Device with no linked Command Console](units_buildings_crates_economy.md#dec-459-core-device-with-no-linked-command-console) | Resolved | units_buildings_crates_economy.md |
| DEC-460 | [A burning Hive releases ants only while it holds any](units_buildings_crates_economy.md#dec-460-a-burning-hive-releases-ants-only-while-it-holds-any) | Resolved | units_buildings_crates_economy.md |
| DEC-461 | [Each turret type fires from the sockets of its own mesh](units_buildings_crates_economy.md#dec-461-each-turret-type-fires-from-the-sockets-of-its-own-mesh) | Resolved | units_buildings_crates_economy.md |
| DEC-462 | [Turret facing from map data](units_buildings_crates_economy.md#dec-462-turret-facing-from-map-data) | Open | units_buildings_crates_economy.md |
| DEC-463 | [One surge limit for the fuel pump](units_buildings_crates_economy.md#dec-463-one-surge-limit-for-the-fuel-pump) | Resolved | units_buildings_crates_economy.md |
| DEC-464 | [The Siege Solar Array knock-out uses one no-team value](units_buildings_crates_economy.md#dec-464-the-siege-solar-array-knock-out-uses-one-no-team-value) | Resolved | units_buildings_crates_economy.md |
| DEC-465 | [Score-weighted Supply crate drop when every score is zero](units_buildings_crates_economy.md#dec-465-score-weighted-supply-crate-drop-when-every-score-is-zero) | Resolved | units_buildings_crates_economy.md |
| DEC-466 | [Defects in structures and fields that no match uses](units_buildings_crates_economy.md#dec-466-defects-in-structures-and-fields-that-no-match-uses) | Dropped | units_buildings_crates_economy.md |
| DEC-550 | [Power cancellation in the tutorials](controls_bots_maps_meta_audio.md#dec-550-power-cancellation-in-the-tutorials) | Open | controls_bots_maps_meta_audio.md |
| DEC-551 | [Units that outlive their Power slot](controls_bots_maps_meta_audio.md#dec-551-units-that-outlive-their-power-slot) | Open | controls_bots_maps_meta_audio.md |
| DEC-552 | [Power kinds without targeting behaviour](controls_bots_maps_meta_audio.md#dec-552-power-kinds-without-targeting-behaviour) | Dropped | controls_bots_maps_meta_audio.md |
| DEC-553 | [Default bindings that rely on lenient parsing](controls_bots_maps_meta_audio.md#dec-553-default-bindings-that-rely-on-lenient-parsing) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-554 | [Input devices, key names and binding drivers](controls_bots_maps_meta_audio.md#dec-554-input-devices-key-names-and-binding-drivers) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-555 | [A crowd order for an empty group does nothing](controls_bots_maps_meta_audio.md#dec-555-a-crowd-order-for-an-empty-group-does-nothing) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-556 | [Power slot order](controls_bots_maps_meta_audio.md#dec-556-power-slot-order) | Open | controls_bots_maps_meta_audio.md |
| DEC-557 | [Bot troop movement between navigation nodes](controls_bots_maps_meta_audio.md#dec-557-bot-troop-movement-between-navigation-nodes) | Open | controls_bots_maps_meta_audio.md |
| DEC-558 | [Bot controller of the Rogue and Visitor factions](controls_bots_maps_meta_audio.md#dec-558-bot-controller-of-the-rogue-and-visitor-factions) | Open | controls_bots_maps_meta_audio.md |
| DEC-559 | [Bot module structure](controls_bots_maps_meta_audio.md#dec-559-bot-module-structure) | Open | controls_bots_maps_meta_audio.md |
| DEC-560 | [Start positions are typed map records](controls_bots_maps_meta_audio.md#dec-560-start-positions-are-typed-map-records) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-561 | [Per-mode map validation](controls_bots_maps_meta_audio.md#dec-561-per-mode-map-validation) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-562 | [Map lighting and weather presets](controls_bots_maps_meta_audio.md#dec-562-map-lighting-and-weather-presets) | Open | controls_bots_maps_meta_audio.md |
| DEC-563 | [Map thumbnails and the map index are content-build outputs](controls_bots_maps_meta_audio.md#dec-563-map-thumbnails-and-the-map-index-are-content-build-outputs) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-564 | [Laser fire is audible, and selection sounds stay local](controls_bots_maps_meta_audio.md#dec-564-laser-fire-is-audible-and-selection-sounds-stay-local) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-565 | [Sounds for the other silent gameplay events](controls_bots_maps_meta_audio.md#dec-565-sounds-for-the-other-silent-gameplay-events) | Open | controls_bots_maps_meta_audio.md |
| DEC-566 | [Listener position and distance limits](controls_bots_maps_meta_audio.md#dec-566-listener-position-and-distance-limits) | Open | controls_bots_maps_meta_audio.md |
| DEC-567 | [Dry mix or applied effects](controls_bots_maps_meta_audio.md#dec-567-dry-mix-or-applied-effects) | Open | controls_bots_maps_meta_audio.md |
| DEC-568 | [Sample-group choice and linked parameters](controls_bots_maps_meta_audio.md#dec-568-sample-group-choice-and-linked-parameters) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-569 | [Game-speed pitch bend](controls_bots_maps_meta_audio.md#dec-569-game-speed-pitch-bend) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-570 | [Buses, priority classes and voice budgets](controls_bots_maps_meta_audio.md#dec-570-buses-priority-classes-and-voice-budgets) | Resolved | controls_bots_maps_meta_audio.md |
| DEC-650 | [Actor-grid registration follows the recorded footprint](simulation_terrain_render_assets.md#dec-650-actor-grid-registration-follows-the-recorded-footprint) | Resolved | simulation_terrain_render_assets.md |
| DEC-651 | [Lifetime of simulation routes](simulation_terrain_render_assets.md#dec-651-lifetime-of-simulation-routes) | Open | simulation_terrain_render_assets.md |
| DEC-652 | [Random draws at initialisation happen once, in explicit simulation steps](simulation_terrain_render_assets.md#dec-652-random-draws-at-initialisation-happen-once-in-explicit-simulation-steps) | Resolved | simulation_terrain_render_assets.md |
| DEC-653 | [Simulation state never holds a non-finite value](simulation_terrain_render_assets.md#dec-653-simulation-state-never-holds-a-non-finite-value) | Resolved | simulation_terrain_render_assets.md |
| DEC-654 | [Gameplay is computed only by the CPU simulation](simulation_terrain_render_assets.md#dec-654-gameplay-is-computed-only-by-the-cpu-simulation) | Resolved | simulation_terrain_render_assets.md |
| DEC-750 | [Explainer-animation loop and visibility rules](vfx_audio_ui.md#dec-750-explainer-animation-loop-and-visibility-rules) | Open | vfx_audio_ui.md |
| DEC-751 | [Game-speed pitch state starts every match at normal speed](vfx_audio_ui.md#dec-751-game-speed-pitch-state-starts-every-match-at-normal-speed) | Resolved | vfx_audio_ui.md |
| DEC-752 | [Audio queries and reused voices carry no stale state](vfx_audio_ui.md#dec-752-audio-queries-and-reused-voices-carry-no-stale-state) | Resolved | vfx_audio_ui.md |
| DEC-800 | [Content and settings writers are lossless, typed and atomic](data_sequences_platform_debug.md#dec-800-content-and-settings-writers-are-lossless-typed-and-atomic) | Resolved | data_sequences_platform_debug.md |
| DEC-801 | [Duplicate keys, repeated fields and over-long values are build errors](data_sequences_platform_debug.md#dec-801-duplicate-keys-repeated-fields-and-over-long-values-are-build-errors) | Resolved | data_sequences_platform_debug.md |
| DEC-802 | [Maps that match setup does not list](data_sequences_platform_debug.md#dec-802-maps-that-match-setup-does-not-list) | Open | data_sequences_platform_debug.md |
| DEC-803 | [Camera animations without content](data_sequences_platform_debug.md#dec-803-camera-animations-without-content) | Open | data_sequences_platform_debug.md |
| DEC-804 | [Soak-test sequence and timing rules](data_sequences_platform_debug.md#dec-804-soak-test-sequence-and-timing-rules) | Resolved | data_sequences_platform_debug.md |
| DEC-850 | [Match-rule gap rules](gameplay_gap_rules.md#dec-850-match-rule-gap-rules) | Open | gameplay_gap_rules.md |
| DEC-851 | [Population-flow rule details](gameplay_gap_rules.md#dec-851-population-flow-rule-details) | Open | gameplay_gap_rules.md |
| DEC-852 | [Game-speed divisors in Worker rules](gameplay_gap_rules.md#dec-852-game-speed-divisors-in-worker-rules) | Open | gameplay_gap_rules.md |
| DEC-853 | [Worker and Trooper rule details](gameplay_gap_rules.md#dec-853-worker-and-trooper-rule-details) | Open | gameplay_gap_rules.md |
| DEC-854 | [Transport, Collector and Strike team rule details](gameplay_gap_rules.md#dec-854-transport-collector-and-strike-team-rule-details) | Open | gameplay_gap_rules.md |
| DEC-855 | [Creature and hazard rule quirks](gameplay_gap_rules.md#dec-855-creature-and-hazard-rule-quirks) | Open | gameplay_gap_rules.md |
| DEC-856 | [Creature presentation quirks](gameplay_gap_rules.md#dec-856-creature-presentation-quirks) | Open | gameplay_gap_rules.md |
| DEC-857 | [Attempt caps of the bounded searches](gameplay_gap_rules.md#dec-857-attempt-caps-of-the-bounded-searches) | Resolved | gameplay_gap_rules.md |
| DEC-858 | [Empty outcome lists](gameplay_gap_rules.md#dec-858-empty-outcome-lists) | Open | gameplay_gap_rules.md |
| DEC-859 | [Tick counts chosen for reference durations](gameplay_gap_rules.md#dec-859-tick-counts-chosen-for-reference-durations) | Open | gameplay_gap_rules.md |
| DEC-860 | [Structure rule details](gameplay_gap_rules.md#dec-860-structure-rule-details) | Open | gameplay_gap_rules.md |
| DEC-861 | [Bot rule quirks](gameplay_gap_rules.md#dec-861-bot-rule-quirks) | Open | gameplay_gap_rules.md |
| DEC-862 | [Bot rule details](gameplay_gap_rules.md#dec-862-bot-rule-details) | Open | gameplay_gap_rules.md |
| DEC-863 | [Control details of the corrected command set](gameplay_gap_rules.md#dec-863-control-details-of-the-corrected-command-set) | Resolved | gameplay_gap_rules.md |
| DEC-864 | [Pointer fallback and camera smoothing](gameplay_gap_rules.md#dec-864-pointer-fallback-and-camera-smoothing) | Open | gameplay_gap_rules.md |
| DEC-865 | [Map data without a reference case](gameplay_gap_rules.md#dec-865-map-data-without-a-reference-case) | Open | gameplay_gap_rules.md |
| DEC-866 | [Population-cap values of a map](gameplay_gap_rules.md#dec-866-population-cap-values-of-a-map) | Open | gameplay_gap_rules.md |
| DEC-867 | [Closed player slots](gameplay_gap_rules.md#dec-867-closed-player-slots) | Open | gameplay_gap_rules.md |
| DEC-868 | [Front-end reference quirks](gameplay_gap_rules.md#dec-868-front-end-reference-quirks) | Open | gameplay_gap_rules.md |
| DEC-869 | [Reference sound options without a Species counterpart](gameplay_gap_rules.md#dec-869-reference-sound-options-without-a-species-counterpart) | Dropped | gameplay_gap_rules.md |
| DEC-870 | [Tutorial director details](gameplay_gap_rules.md#dec-870-tutorial-director-details) | Open | gameplay_gap_rules.md |
| DEC-871 | [Essence orb break operation](gameplay_gap_rules.md#dec-871-essence-orb-break-operation) | Dropped | gameplay_gap_rules.md |
| DEC-950 | [Keyboard operation of the menus](engine_gap_rules.md#dec-950-keyboard-operation-of-the-menus) | Resolved | engine_gap_rules.md |
| DEC-951 | [Repeat and increment timing of the UI and of input](engine_gap_rules.md#dec-951-repeat-and-increment-timing-of-the-ui-and-of-input) | Resolved | engine_gap_rules.md |
| DEC-952 | [UI framework reference behaviours kept as defaults](engine_gap_rules.md#dec-952-ui-framework-reference-behaviours-kept-as-defaults) | Open | engine_gap_rules.md |
| DEC-953 | [Input back-end rules](engine_gap_rules.md#dec-953-input-back-end-rules) | Resolved | engine_gap_rules.md |
| DEC-954 | [Input reference behaviours kept as defaults](engine_gap_rules.md#dec-954-input-reference-behaviours-kept-as-defaults) | Open | engine_gap_rules.md |
| DEC-955 | [Presentation clocks while the world is held, and camera shake](engine_gap_rules.md#dec-955-presentation-clocks-while-the-world-is-held-and-camera-shake) | Resolved | engine_gap_rules.md |
| DEC-956 | [Pick-ray pixel convention](engine_gap_rules.md#dec-956-pick-ray-pixel-convention) | Open | engine_gap_rules.md |
| DEC-957 | [Every draw sets its complete pipeline state](engine_gap_rules.md#dec-957-every-draw-sets-its-complete-pipeline-state) | Resolved | engine_gap_rules.md |
| DEC-958 | [Renderer reference looks kept as defaults](engine_gap_rules.md#dec-958-renderer-reference-looks-kept-as-defaults) | Open | engine_gap_rules.md |
| DEC-959 | [Renderer defaults for the burn layer, pulse clocks, egg shadow, route spots and countdown screens](engine_gap_rules.md#dec-959-renderer-defaults-for-the-burn-layer-pulse-clocks-egg-shadow-route-spots-and-countdown-screens) | Open | engine_gap_rules.md |
| DEC-960 | [Effects advance in fixed steps](engine_gap_rules.md#dec-960-effects-advance-in-fixed-steps) | Resolved | engine_gap_rules.md |
| DEC-961 | [Explainer panels lay out again on every screen change](engine_gap_rules.md#dec-961-explainer-panels-lay-out-again-on-every-screen-change) | Resolved | engine_gap_rules.md |
| DEC-962 | [Species effect defaults for emitters, shard edges and the eaten-orb display](engine_gap_rules.md#dec-962-species-effect-defaults-for-emitters-shard-edges-and-the-eaten-orb-display) | Open | engine_gap_rules.md |
| DEC-963 | [Animation and VFX reference looks kept as defaults](engine_gap_rules.md#dec-963-animation-and-vfx-reference-looks-kept-as-defaults) | Open | engine_gap_rules.md |
| DEC-964 | [Accessibility limits for camera shake and flashes](engine_gap_rules.md#dec-964-accessibility-limits-for-camera-shake-and-flashes) | Open | engine_gap_rules.md |
| DEC-965 | [Audio engine reference behaviours kept as defaults](engine_gap_rules.md#dec-965-audio-engine-reference-behaviours-kept-as-defaults) | Open | engine_gap_rules.md |
| DEC-966 | [Terrain query edge cases](engine_gap_rules.md#dec-966-terrain-query-edge-cases) | Open | engine_gap_rules.md |
| DEC-967 | [Reproducible terrain colours and no regeneration of the height field](engine_gap_rules.md#dec-967-reproducible-terrain-colours-and-no-regeneration-of-the-height-field) | Resolved | engine_gap_rules.md |
| DEC-968 | [Content-build validation of terrain, map and record data](engine_gap_rules.md#dec-968-content-build-validation-of-terrain-map-and-record-data) | Resolved | engine_gap_rules.md |
| DEC-969 | [Co-op group lists in ascending position order](engine_gap_rules.md#dec-969-co-op-group-lists-in-ascending-position-order) | Open | engine_gap_rules.md |
| DEC-970 | [System summary and crash-context records](engine_gap_rules.md#dec-970-system-summary-and-crash-context-records) | Resolved | engine_gap_rules.md |
| DEC-971 | [Reference test machine and minimum-specification hardware](engine_gap_rules.md#dec-971-reference-test-machine-and-minimum-specification-hardware) | Open | engine_gap_rules.md |
| DEC-972 | [What the cheats-used marker suppresses](engine_gap_rules.md#dec-972-what-the-cheats-used-marker-suppresses) | Open | engine_gap_rules.md |
