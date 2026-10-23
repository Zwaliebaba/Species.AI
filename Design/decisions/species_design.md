# Species design decisions

> Status: Species design decisions · Range: DEC-1000 to DEC-1999 · Related: [decisions index](README.md), [owner decisions](owner_decisions.md), [world and sites](../gdd/01_world_and_regions/world_and_sites.md), [MVP scope](../implementation/mvp_scope.md), [conventions](../CONVENTIONS.md)

This file records the decisions of the Species design and of the MVP built from it that the design does not settle: questions that building the MVP raises about a Final requirement, numbered with four digits from one thousand (CONVENTIONS §3). Each entry states the behaviour that the code builds until the owner decides. An **Open** entry is the owner's to decide, and a **Resolved** entry records the owner's decision.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-1000](#dec-1000-the-place-of-the-ninth-home-resource-site-on-the-mvps-terrain) | The place of the ninth home resource site on the MVP's terrain | Resolved | WLD, MVP |
| [DEC-1001](#dec-1001-the-provisional-looks-of-the-settlement) | The provisional looks of the settlement | Open | MVP |
| [DEC-1002](#dec-1002-the-provisional-looks-of-the-territories) | The provisional looks of the territories | Open | MVP |
| [DEC-1003](#dec-1003-the-weight-and-width-of-the-text-in-bahnschrift) | The weight and width of the text in Bahnschrift | Open | UI, MVP |
| [DEC-1004](#dec-1004-the-publishers-folder-of-the-user-data-root) | The publisher's folder of the user-data root | Open | PLT, MVP |
| [DEC-1005](#dec-1005-the-provisional-layout-and-palette-of-the-settlement-hud) | The provisional layout and palette of the settlement HUD | Open | MVP, UI |
| [DEC-1006](#dec-1006-how-the-ai-settlements-claim-and-raise-their-expeditions) | How the AI settlements claim and raise their expeditions | Open | MVP |
| [DEC-1007](#dec-1007-renown-for-a-withdrawn-claim) | Renown for a withdrawn claim | Open | MVP, PIL |

## 1 World and sites

### DEC-1000 The place of the ninth home resource site on the MVP's terrain
- Status: Resolved
- Affects: gdd/01_world_and_regions/world_and_sites.md (WLD-042, WLD-082, WLD-085, `wld.resource.extra_bearing`), gdd/01_world_and_regions/terrain_time_and_views.md (WLD-163 check 7), implementation/mvp_scope.md (MVP-010); ADR-102, ADR-130
- Specified behaviour: The ninth home resource site of a site without a crossing stands at `wld.resource.extra_radius` (1,050 wu) from the site's centre, at a bearing of `wld.resource.extra_bearing` (+84°) from the outward diagonal instead of on it. Check 7 of WLD-163 counts no yield of a resource site that stands at or below the shore height or on ground steeper than walkable.
  - On the diagonal, the place lies 2,521 wu from the valley's centre towards the region's corner, where M1's island base tile is lowest (ADR-102). Over seeds 1 to 100 it is at or below `ref.unit.worker.shore_height` or steeper than `ref.sim.walk.min_normal_y` at 188 of the 400 sites, in 93 seeds.
  - The other ten resource sites of every site are dry and walkable on all of them.
  - At +84° and the same radius, every site of those seeds is walkable, has no sample at or below the shore height within 200 wu, and stands at least 315 wu from every other resource site and 237 wu outside each of its own valley border areas. Every seed therefore passes check 7 at its first attempt, as it passes the other checks.
  - The bearing lies between 80°, where one seed has water within 200 wu, and 86°, where the border areas come within 200 wu (WLD-082).
- Options:
  - A — as stated: the same radius at +84°, keeping the 11 units, every gathering distance and every seed's terrain;
  - B — the ninth on the diagonal as WLD-082 states, with the island's corners raised by the M1 terrain keys (`mvp.terrain.guide_rim`) until the place is dry and walkable on every seed; every seed's terrain changes, and M1's evidence on its seeds is measured again;
  - C — the MVP's sites hold 10 units, without the ninth home resource site, and MVP-010 and WLD-085 are amended;
  - D — WLD-082 as written, with check 7 re-rolling a region whose ninth site is wet; 93 of seeds 1 to 100 fail at their first attempt, so most seeds give another valley.
- Recommendation: A. It moves one value of the Final world design and nothing a steward can measure: every site keeps its 11 units at the same distances (WLD-042), and the valley of every seed that M1 measured stays as it was.
- Decide by: M2 task T10, before the growth scene tunes the economy on the 11 units.
- Resolution (2026-10-22, owner decision O87): option A. The ninth home resource site of a site without a crossing stands at `wld.resource.extra_radius` (1,050 wu) at `wld.resource.extra_bearing` (84°, now final) from the outward diagonal, as the specified behaviour states, and check 7 keeps counting no yield of a resource site in the sea or on a slope. Every site keeps its 11 units at the same distances, every seed's valley stays as M1 measured it, and M2 task T10's tuning, made on this place, stands (WLD-082 and MVP-010 amendments, [ADR-130](../ADR/ADR-130-settlements-structures-and-resource-sites-at-m2.md) amendment).

## 2 Presentation

### DEC-1001 The provisional looks of the settlement
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-082, MVP-083, and table 3's `mvp.structure.*`, `mvp.wellspring.*`, `mvp.orb.cloudy_size`, `mvp.orb.cloudy_alpha` and `mvp.worker.load_color`); ADR-140
- Specified behaviour: The client draws the settlement with the first values of table 3 (owner decision O89). MVP-083 states that a construction site rises with its progress, that an inactive structure is dark and that a completed structure flashes once, and MVP-082 that a carried load is visible, but not how far, how dark, how large or in what colour.
  - A construction site is sunk by `mvp.structure.rise_depth_factor` (1.0) × its mesh's height × the share of its work still to come, so it rises out of the ground as it is built.
  - An inactive structure's lit colour is multiplied by `mvp.structure.inactive_brightness` (0.35), and it shows no ownership lights.
  - A completed structure flashes once: a camera-facing `fx.starburst` of half-size `mvp.structure.flash_size` (40 wu) in its team's colour, its own half-size above the structure's place, fading over `mvp.structure.flash_seconds` (0.5 s) from the tick of its event.
  - The Wellspring glows: `mvp.wellspring.glow_blobs` (5) quads of `fx.glow_cloudy`, of half-size `mvp.wellspring.glow_size` (22 wu) and alpha `mvp.wellspring.glow_alpha` (0.45) in its team's colour, circle the centre of its crystal at `mvp.wellspring.glow_radius` (12 wu), one turn every `mvp.wellspring.glow_turn_seconds` (24 s).
  - An orb drifting home or waiting at its Wellspring is wrapped in a quad of `fx.glow_cloudy` of half-size `mvp.orb.cloudy_size` (16 wu) and alpha `mvp.orb.cloudy_alpha` (0.5) in its settlement's colour.
  - A Worker that carries a load shows `sprite.worker.carry_load` over its body in `mvp.worker.load_color` (0.6, 0.9, 1.0).
- Options:
  - A — the values as stated, which the owner retunes after M2 task T12's run, as the two lights were retuned after M1's runs (O75, O83);
  - B — values that the owner sets before the run.
- Recommendation: A. Each look is one key of the client's presentation record, read at start-up, so a retune changes no code and nothing the simulation computes.
- Decide by: the owner's run of M2 task T12; the likeness review of PIL-027 at M2's end judges the result.

### DEC-1002 The provisional looks of the territories
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-084, and table 3's `mvp.territory_line.*`, `mvp.border_mark.*` and `mvp.view_fog.*`); ADR-143, ADR-144
- Specified behaviour: The client draws the territories and the border areas with the first values of table 3 (owner decision O90). MVP-084 states that a territory is a glowing line of `fx.beam_long` in its Cladan's colour, that border areas are marked and that the land outside the steward's areas lies under black fog (WLD-244), but not how wide, how bright, how marked or how dark.
  - Each settlement's territory line runs halfway between the cells it holds and those it does not, `mvp.territory_line.lift` (2 wu) over the terrain, a camera-facing segment of `fx.beam_long` of half-width `mvp.territory_line.half_width` (3 wu) at alpha `mvp.territory_line.alpha` (0.8) in its Cladan's colour.
  - Each valley border area's halfway line and edge carry a spot of `fx.particle_dot` every `mvp.border_mark.spacing` (24 wu), of half-size `mvp.border_mark.spot_size` (4 wu), lifted by its size over the terrain, in `mvp.border_mark.color` (0.55, 0.55, 0.55) at alpha `mvp.border_mark.alpha` (0.6), outside the coast bands.
  - Holders are not marked, since none is replicated at M2 (WLD-063); the strategic view's marks (WLD-241) are not drawn in the settlement view.
  - Outside the steward's areas the terrain and the sea keep 1 − `mvp.view_fog.strength` (0.85) of their colour, and the fog's edge is a ramp `mvp.view_fog.edge_width` (64 wu) wide, centred on the areas' edge.
- Options:
  - A — the values as stated, which the owner retunes after M2 task T13's run, as DEC-1001's after T12's;
  - B — values that the owner sets before the run.
- Recommendation: A. Each look is one key of the client's presentation record, so a retune changes no code and nothing the simulation computes.
- Decide by: the owner's run of M2 task T13; the likeness review of PIL-027 at M2's end judges the result.

### DEC-1003 The weight and width of the text in Bahnschrift
- Status: Open
- Affects: architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI-195, UI-196, UI-197; `ui.font.spacing_*`, `ui.text.horizontal_scale_latin`); implementation/mvp_scope.md (MVP-100, MVP-102, MVP-103); ADR-147
- Specified behaviour: Since owner decision O92 the player's text is set in Bahnschrift, a font Windows installs. UI-196 and UI-197 were written for the reference's fonts: they give each role a spacing factor (body 0.91, title 1.03) and condense Latin text to 0.6 of its width. The client builds the following until the owner decides.
  - Every font role is set at Bahnschrift's normal weight.
  - The spacing factors and the horizontal scale are 1.0, so the text keeps Bahnschrift's own widths, which are already narrow.
  - The badge's count is `ui.text.default_size` (12 px) and the development overlay `dbg.overlay.text_size` (14 px), each × the DPI scale and never below the accessibility minimum of UI-171.
- Options:
  - A — as stated, with the HUD's weights and sizes set at M2 task T16 from the owner's view of T14's test page;
  - B — the reference's factors and condensation applied to Bahnschrift (ADR-034's base text);
  - C — Bahnschrift's own narrower widths (SemiCondensed or Condensed) for some roles, in place of a horizontal scale.
- Recommendation: A. Bahnschrift is drawn narrow already, and condensing it again to 0.6 would make small text hard to read. A width or weight per role is one family name or one key, so a later choice changes no code.
- Decide by: the owner's run of M2 task T14's DirectWrite step; the HUD of T16 uses the answer.

### DEC-1005 The provisional layout and palette of the settlement HUD
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-102, and table 3's `mvp.hud.*`); architecture/components/renderer/visual_language.md (RND-403); ADR-152
- Specified behaviour: MVP-102 lists what the HUD shows, and the MVP assets give its icons and panel strips, but no layout or palette exists; the owner approves the HUD layout (MVP assets §1) and chose a provisional palette for M2 (O95). The client builds the following until the owner decides.
  - **Where the panels go.**
    - The settlement panel, top left: the stores, the Workers, growth, the upkeep, the three priorities and the idle Workers by reason, each on a row with its icon, with a meter under the stores, the Workers and each priority.
    - The construction bar, bottom centre, with a site's square for each queued site above it.
    - The notification cards, top right, newest first.
    - The world's panel, bottom left, with the seed.
  - **Measures.** In the HUD space of RND-377, as table 3's `mvp.hud.*` measures state, times the UI scale.
  - **The palette** (`mvp.hud.palette.*`), drafted from the front end's colours (`ui.palette.fe_*`) for RND-403's roles: a blue accent panel, a grey neutral panel, white text, green positive and red negative captions, and blue meters.
- Options:
  - A — as stated, with the owner retuning the keys after M2 task T16's run;
  - B — a layout and palette the owner sets before the run.
- Recommendation: A. Every measure and colour is a key of the client's presentation record, so a retune changes no code, and the layout is reviewed for likeness at M2's end (PIL-027).
- Decide by: the owner's run of M2 task T16; the likeness review of PIL-027 judges the result.

## 3 Platform

### DEC-1004 The publisher's folder of the user-data root
- Status: Open
- Affects: architecture/components/platform_layer.md (PLT-016, PLT-017); ADR-071, ADR-150
- Specified behaviour: PLT-016 places the user-data root at `%LOCALAPPDATA%\<Studio>\Species\`, where `<Studio>` is the publisher's name compiled into the build. O49 deferred the publisher to before the first external build, so no name exists. Until the owner decides, the client builds the following.
  - The root is `%LOCALAPPDATA%\Species\`: the Local AppData known folder, then the product's folder, with no publisher's folder between them.
  - `--user-data <directory>` replaces it in every build, as PLT-025 states.
  - When a publisher is named, the root moves under its folder. The settings file is never migrated (R-META-409), so a player's settings and saved worlds stay in the old folder unless a one-time move is built with the change.
- Options:
  - A — as stated, with a one-time move of the old folder when the publisher is named;
  - B — a placeholder publisher's folder now, such as the owner's name, which the publisher's name replaces later, with the same move;
  - C — the publisher's folder named `Species` until O49 is decided, giving `%LOCALAPPDATA%\Species\Species\`.
- Recommendation: A. Only the owner's machines hold user data before an external build, and a folder named for nobody would need the same move as no folder.
- Decide by: O49's publisher decision, before the first external build.

## 4 Relations and conflict

### DEC-1006 How the AI settlements claim and raise their expeditions
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-041, MVP-055, MVP-057, and table 3's `mvp.ai.muster_minutes`); ADR-162
- Specified behaviour: MVP-057 gives each temperament its claims in a few words, and MVP-055 says when an AI settlement may attack but not how it gathers its expedition or how large it makes it. Until the owner decides, the AI settlements build the following.
  - **Claims**, one a decision at most, each only when its claim would not be refused:
    - Bold claims the central resource site from its first decision, and again whenever it has no claim on it and does not hold it.
    - Territorial claims each half of its border areas that a valley neighbour holds, as soon as the neighbour holds it.
    - Wary claims nothing until it has been attacked: until its Cladan holds an attack's grievance, or it has defended a battle. It then claims the halves held by a settlement that attacked it.
  - **The target**: among the settlements it may attack (MVP-055), the one its Cladan holds most anger against, the lower site on a tie. Bold raids when its anger allows; the others fight the claim battle when they hold a claim on what the target holds.
  - **The size**: it may attack when the Workers it can field reach `mvp.ai.attack_min_share` of the target's Workers it sees, at least one. It can field its Workers beyond `mvp.protection.worker_floor`, at most `mvp.expedition.size_max`, and it means to send all of them.
  - **The muster** (MVP-041): it promotes one Leader for each `ref.sim.team.formation_size_upgrade` Workers it means to send, at most three, from the Workers nearest its Wellspring, and forms them up there, so that the Workers on guard duty join them. It declares once all it means to send follow them, or after `mvp.ai.muster_minutes` (2) with at least the fewest it may send; otherwise it gives the attack up. It does not change its priorities to raise more guards.
  - **After a battle**: its Leaders outside a muster are demoted, and their Workers go back to their tasks.
- Options:
  - A — as stated;
  - B — as stated, with the expedition the fewest it may send rather than all it can field, so that the settlement's economy keeps more Workers;
  - C — as stated, with the guard share raised for the muster's length, so that the expedition draws more Workers than those already on guard duty.
- Recommendation: A for M3's headless phase. The valley run of M3 task T9 measures the battles' sizes and lengths against Playtest 1's lines, and the lever it changes then is this one or `muster_minutes`.
- Decide by: the valley run of M3 task T9, before Playtest 1.

### DEC-1007 Renown for a withdrawn claim
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-061); gdd/00_vision_pillars_and_core_loops.md (PIL-125); ADR-163
- Specified behaviour: MVP-061 gives `mvp.renown.grievance_eased` for each 20 weight of grievance eased by tribute or by a withdrawn claim. A claim costs nothing and gives its holder a grievance of `mvp.grievance.claim_weight` (30), so a claim made and withdrawn at once would earn renown for a grievance that its own claimant created a moment before, as often as it is repeated. Until the owner decides, the server builds the following.
  - A withdrawn claim eases, towards its claimant's renown, the weight its grievance holds at the withdrawal, but only once the claim has stood `mvp.claim.uncontested_minutes` (30) since it was made.
  - A claim withdrawn sooner still removes its grievance's cause (PIL-122), and earns no renown.
  - Tribute eases the weight it removes, whenever it is sent.
- Options:
  - A — as stated;
  - B — the scope's rule as written, every withdrawal crediting its grievance's weight, accepting that a steward can repeat it;
  - C — no renown for a withdrawn claim, only for tribute.
- Recommendation: A. It keeps the scope's reward for backing down from a real claim and closes the repetition, with no new key.
- Decide by: Playtest 1, whose renown standing (MVP-062) shows whether easing is worth what it costs.

### DEC-1008 The valley run's misses of Playtest 1's lines
- Status: Open
- Affects: implementation/mvp_scope.md (MVP-045, MVP-053, MVP-055, §6.1's table 5, and table 3's `mvp.battle.objective_hold_s`, `mvp.battle.rout_share`, `mvp.ai.attack_min_share` and `mvp.conflict.attack_cost_ru`); ADR-162; ADR-164; DEC-1006
- Specified behaviour: the rules and values stand as they are until the owner decides. The valley run of M3 task T9 runs seeds 1 to 8 for a run each, with the steward's settlement under its scripted steward and a Territorial settlement's decisions (`playtests/M3.md`, "The valley run"). It meets two of Playtest 1's six measurable lines: every conflict reached an end state within 7 hours of play (158 of 158 claims), and the strongest settlement ended at most 1.40 times the median. It misses four:
  - **The steward's battles in a run**: 11 to 22, against 4 to 10. Claims repeat with nothing to space them. A Territorial settlement claims each half its neighbour holds as soon as the neighbour holds it, so a half that changes hands is claimed back at once. One claimant claimed one area up to 7 times in a run, and 86 of the steward's 141 battles were claim battles.
  - **Battle length**: 13 % of 263 battles within 4 to 12 minutes, against 90 %. None ran above 20, since `pil.battle.max_minutes` ends a battle there. Of 150 claim battles, 80 ended within a minute, by the objective held for `mvp.battle.objective_hold_s` (30 s) or by a rout at `mvp.battle.rout_share`. Of 113 raids, 40 ran to the 20-minute limit.
  - **The figures in the steward's battles**: 249 typical and 79 at the least, against 300 to 450 and never below 200. They are counted as the most of both sides on the battlefield at once. An expedition is what its settlement can field when its muster ends (DEC-1006), and a claim battle at a border area draws only the defenders that reach the claimed site.
  - **The largest gap between the steward's growth steps in hours 0 to 5**: up to 76.6 minutes on four seeds, and 15.3 to 21.0 on the others, against 20. The first contact comes at 1.4 to 2.1 hours, and from then on the claims cost the steward attacks at `mvp.conflict.attack_cost_ru` (300 ru) and Workers.
- Options:
  - A — space the claims: no claim may be made on an area until a new key, `mvp.claim.cooldown_minutes`, has passed since the last claim on it ended. Raise `mvp.battle.objective_hold_s` so that a claim battle is fought before it is won. Retune the figures and the growth steps on M2's evidence (O98).
  - B — keep the rules and retune only the values (`objective_hold_s`, `rout_share`, `attack_min_share`, `attack_cost_ru`), accepting the battles that repeated claims bring.
  - C — narrow DEC-1006's Territorial rule: claim a held half only while its anger towards the holder is at least its threshold.
- Recommendation: A. The repeated claims are a rule's gap, not a value's, and no value of the present keys spaces them; the battle and growth values wait for M2's evidence, as O98 has it.
- Decide by: M3 task T9's tuning, after the owner's M2 runs and before Playtest 1.
