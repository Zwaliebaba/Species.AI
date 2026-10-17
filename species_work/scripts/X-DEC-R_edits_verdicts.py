"""X-DEC-R: amendments to design_next/decisions/verdicts_and_fixes.md (data only).

Each edit is (tag, old, new). Imported by X-DEC-R_apply.py.
"""
F = 'design_next/decisions/verdicts_and_fixes.md'

EDITS = [
# ---- B: DEC-002 extra tick counts; upper bound of the Relay transit (X-DEC-4a, X-DEC-3 #9)
('B DEC-002',
 'Relay transit 10h + 1 for h hops (11 for the first hop, 10 for each further hop). Seconds appear only as a derived display value next to the tick count.',
 'Relay transit at most 10h + 1 for h hops (11 for the first hop, 10 for each further hop; the same-tick rule that sets the exact count is DEC-455). Further reference tick counts: burst pause 101–150 (mean 125.5); first production timer 20–40; first ownership evaluation 1–20, later evaluations 20–30; reinforcement wave every 660, with a first gap of 649; Spawn Control elimination 330; first Comeback Bonus grant 51, for a team already eliminated on the first tick after the grace period; Spawn Surge 50 or 51 (DEC-304); Essence orb floating 300–500 and death ascent 901; Collector and Builder idle retarget 11; bot Collector decision 11; Worker idle retarget 11–20; corpse fade 25 on the ground and 50 in the air; Population Limiter recount 90–110; Hive orb-to-ant conversion 10–11; Crawler egg hatch 151; Collector delivery one Essence orb per tick. The three mode-second timers start at 0, so the first mode second ends on the first tick after the grace period. Seconds appear only as a derived display value next to the tick count.'),

# ---- B: DEC-007 render writers and presentation-only leg state (X-DEC-4a, X-DEC-5a #5, X-DEC-3 #15)
('B DEC-007',
 'Tutorial and help logic changes the world only through sequenced simulation events (DEC-103).\n- Options: A — rules emit presentation events',
 'Tutorial and help logic changes the world only through sequenced simulation events (DEC-103). Spider leg and foot state, with its wall-clock swing timing, is presentation only, and no simulation rule reads it. The reference render paths that write fields of simulated objects are listed in DEC-222 item 2.\n- Options: A — rules emit presentation events'),

# ---- B: DEC-008 placeholder reference replaced by the deciding entry (X-DEC-2a cross-fix)
('X DEC-008',
 'is a networking-model question (see DEC-2xx, networking).',
 'is a networking-model question (DEC-202).'),

# ---- B: DEC-010 instant-unit copies (X-DEC-5a #7)
('B DEC-010',
 'Species specifies actor speeds at difficulty level 0 and has no process-global difficulty.',
 'Species specifies actor speeds at difficulty level 0 and has no process-global difficulty. Instant-unit copies of the centipedes of team slot 1 exist only at a load difficulty factor above 1, which never occurs in a match, so no map rule specifies them.'),

# ---- A1: DEC-011 offset table aligned with DEC-652; B: Source Node id, teleport table, slot-5 count (X-DEC-4a, X-DEC-5a #8)
('A1 DEC-011',
 'The actor-group offset table, refilled from the simulation generator on demand, is cleared at every match start and end.',
 'The actor-group offset table is cleared at every match start and end, and regenerated at a fixed step of match initialisation (DEC-652). The Source Node registry id belongs to the match as well: in the reference it survives between matches and stays stale until the new map\'s Source Node first advances, and on a map without a Source Node a stale id fails its kind check. The teleport transit table is per-match state (DEC-223 item 2). The Spawn Node count of team slot 5 matters because a Spawn Node skips its 300-tick activation delay when it is its team\'s only Spawn Node.'),

# ---- B: DEC-012 tree crown centre and mesh validator checks (X-DEC-3 #5, X-DEC-5a #11)
('B DEC-012',
 'Hit-test geometry is a separate collision asset that the content build validates.\n- Options: A — collision volumes',
 'Hit-test geometry is a separate collision asset that the content build validates. The reference tree crown hit centre ignores the tree\'s rotation, so the baked collision data states each centre explicitly. The mesh checks of the content build are in ADR-0059; the Relic mesh is chosen by a simulation draw among 8.\n- Options: A — collision volumes'),

# ---- B: DEC-013 cosmetic draw sites and the audio generator (X-DEC-5a #6, X-DEC-5b #2)
('B DEC-013',
 'The draw order of the reference generator is not reproduced.\n- Options: A — per-match seed',
 'The draw order of the reference generator is not reproduced. The cosmetic draws that move from the simulation stream to the presentation generator are listed in DEC-222 item 7, and the audio engine has its own presentation-generator instance (ADR-0022).\n- Options: A — per-match seed'),

# ---- A11 / B: DEC-014 Collector and Builder loops (X-DEC-4a)
('A11 DEC-014',
 'and the bot colour choice, which picks from the list of free colours instead of re-rolling.',
 'and the bot colour choice, which picks from the list of free colours instead of re-rolling. The Collector release loop and the Builder drop loop remove a carried entry that has become invalid, so each ends after one pass over its entries; in the reference they end only when every carried entry is valid, so an invalid entry makes them spin. The spawn-network link walk is bounded by DEC-456.'),

# ---- B: DEC-015 duplicate keys (X-DEC-4a)
('B DEC-015',
 'A missing mandatory field, an unterminated block, a stray blank line or a missing asset is a build error with a diagnostic.',
 'A missing mandatory field, an unterminated block, a stray blank line or a missing asset is a build error with a diagnostic, and so is a key listed twice, such as a second population-cap line in a map, where the reference takes the last line (DEC-801).'),

# ---- B: DEC-016 further ordered ties (X-DEC-3 #1, X-DEC-4a)
('B DEC-016',
 'bots consider structures in stable structure-index order. Simulation containers iterate',
 'bots consider structures in stable structure-index order; the port occupant is the first operating Worker in grid result order, whose stable key is cell x, then cell z, then team 0 to 5, then cell slot; the single-round Siege winner is the first slot of the winning side; the Relic Run co-op winner is the last slot of the group; the Launch Vessel lookup takes the last matching structure; and Command Console ownership goes to the first qualifying team in slot order and changes only to a team with strictly more operators. Simulation containers iterate'),

# ---- B: DEC-019 stat-table rows (X-DEC-5c #4)
('B DEC-019',
 'Actor kinds are typed schema entries, not row positions in a stat table.',
 'Actor kinds are typed schema entries, not row positions in a stat table; a missing record or an unknown kind is a build error, where a reference stat table with fewer rows than kinds leaves the missing kinds\' stats at 0 and one with more rows ends the program.'),

# ---- B: DEC-021 starting-powers draw (X-DEC-4b #3)
('B DEC-021',
 'and each team may start with 0 to 3 powers.',
 'and each team may start with 0 to 3 powers. The starting-powers draw excludes Allegiance Scrambler, Supply Storm, Reinforcements, Frenzy, Spawn Surge, Sickness, Rocket Troops, Abductor, Swarm, Spitter Plants, Time Rush, Time Dilation, one grove outcome that reference/04 names, the outcome of DEC-057 and invalid values, and every starting power is locked for 30 s with a visible countdown.'),

# ---- B: DEC-024 orientation law of scripted moves (X-DEC-5c #5)
('B DEC-024',
 'Scripted camera moves land exactly on their target view, with no overshoot.',
 'Scripted camera moves land exactly on their target view, with no overshoot, and interpolate orientation spherically; the reference blends orientation through a midpoint rotated by half the sine of the angle, an approximation of half the angle, with quadratic ease-in and ease-out, and forces the up vector to world up every frame. During scripted moves the field of view eases with a 10 s time constant.'),

# ---- B: DEC-025 bot-navigation map inputs (X-DEC-4b #8); deferred-file citation by id
('B DEC-025',
 'A map declares typed mode objects, per-map rule overrides, lights, camera views and instant units.',
 'A map declares typed mode objects, per-map rule overrides, lights, camera views and instant units, and the bot-navigation inputs of the reference maps: node priority modifiers, links placed by hand and cliff checks.'),
('X DEC-025',
 'Whether terrain is generated at load or baked is a design-phase topic (decisions/deferred/numerics_lobby_terrain_services.md).',
 'Whether terrain is generated at load or baked is a design-phase topic (DEC-246).'),
('X DEC-026',
 'which participant validates it is a networking-model question (decisions/deferred/networking.md).',
 'which participant validates it is the design-phase question of lobby authority (DEC-243).'),

# ---- B: DEC-028 per-call storage limit, unchecked outside-cell ids, stale carried entries (X-DEC-4a, X-DEC-5a #4)
('B DEC-028',
 'Scratch storage is per call, never a shared static buffer.',
 'Scratch storage is per call, never a shared static buffer, and has no fixed limit; the reference carryable-structure collision list is a static array of 32 entries. Outside-cell ids are checked like every handle: in the reference an actor entering a teleport structure and Strike team direct control dereference them unchecked, and with generational handles a stale id is a failed look-up, not a crash. A Collector or Builder drops a carried entry that has become invalid (DEC-014).'),
('X DEC-028',
 'How work is scheduled within a tick is a design-phase topic (decisions/deferred/process_tick_and_roadmap.md).',
 'How work is scheduled within a tick is a design-phase topic (DEC-224).'),

# ---- B: DEC-033 turret-cap option, research (X-DEC-4a, X-DEC-4b #3)
('B DEC-033',
 'A power granted while every slot is full is lost.',
 'A power granted while every slot is full is lost. In matches, Power-slot capacity ignores research. Where a mode has the turret-cap option, a placed turret\'s power keeps its Power slot while the turret exists and counts towards the cap; without the option the power ends at placement.'),

# ---- B: DEC-035 deferred-file citation by id
('X DEC-035',
 'Reconnection that reclaims the slot depends on the networking model (decisions/deferred/networking.md).',
 'Reconnection that reclaims the slot depends on the networking model (DEC-205, DEC-244).'),

# ---- B: DEC-037 explainer depictions are briefs (X-DEC-3 #13)
('B DEC-037',
 'All hint text and explainer art are new.',
 'All hint text and explainer art are new. What each reference explainer animation depicts is inferred from its track data, so descriptions of them are briefs for the new art, not facts.'),

# ---- B: DEC-041 research cheat (X-DEC-3 #12)
('B DEC-041',
 'Benchmark upload is replaced by consented telemetry (DEC-060).',
 'Benchmark upload is replaced by consented telemetry (DEC-060). The cheat that grants all research has no effect in a match, because match start fixes the research levels (DEC-115), so the rebuilt cheat set sets the fixed levels or omits it. Which cheats change simulation state is listed in DEC-202 item 8.'),

# ---- B: DEC-044 mouse inversion and subtitles (X-DEC-4b #5)
('B DEC-044',
 '- Options: A — add these options to the Species front end; B — keep the reference option set.',
 '- Options: A — add these options, together with mouse inversion and subtitles, to the Species front end; B — keep the reference option set.'),

# ---- B: DEC-045 editor lessons and one header parser (X-DEC-3 #10, X-DEC-4b #6)
('B DEC-045',
 'Map data is a versioned, schema-validated map asset with a separately readable header.',
 'Map data is a versioned, schema-validated map asset with a separately readable header, which one parser reads for the menus and for play; the reference has two parsers with different case sensitivity. Lessons from the reference editor: a load, save and load round trip changes terrain and map identity through writer quantisation; a plain-text map opened under the compressed-map extension always fails; and a compressed map with trailing bytes after its stream is read by an unchecked decompression call. These are the reasons for the lossless writers of DEC-800 and the fuzz-tested loaders of DEC-015.'),

# ---- B: DEC-047 deferred-file citation by id
('X DEC-047',
 'The platform service is a deferred topic (decisions/deferred/numerics_lobby_terrain_services.md).',
 'The platform service is a deferred topic (DEC-248).'),

# ---- B: DEC-052 turret aiming markers (X-DEC-3 #6)
('B DEC-052',
 'and the Advisor narrator are outside the reference.',
 'and the Advisor narrator are outside the reference. In a match, Workers have no threat reaction to turret aiming markers; that threat type is evaluated only outside matches.'),

# ---- B: DEC-065 zero-valued map overrides (X-DEC-4b #7)
('B DEC-065',
 'Where a map sets a value, the map value overrides the lobby default.',
 'Where a map sets a value, the map value overrides the lobby default; a typed override keeps 0 as a value, where the reference silently drops a per-map override whose value is 0.'),

# ---- B: DEC-066 round reset and the object id seed (X-DEC-5a #9)
('B DEC-066',
 'is fully specified in the rules, but no match setup can select it.',
 'is fully specified in the rules, but no match setup can select it. The reference round reset leaves the object id seed unchanged and runs after each process has advanced a variable number of update slices from the round-end flag, which consume simulation draws.'),
('B DEC-066 option',
 'B — expose it in the lobby, with the round reset performed inside a tick as a sequenced simulation event.',
 'B — expose it in the lobby, with the round reset performed at a tick boundary as a sequenced simulation event that also resets the object id seed.'),

# ---- B: DEC-073 absent-option effects and the Comeback Bonus option (X-DEC-4a, X-DEC-3 #3)
('B DEC-073',
 'an eliminated team receives a comeback power every 450 ticks (45 s) and holds at most two.',
 'an eliminated team receives a comeback power every 450 ticks (45 s) and holds at most two. It runs in Siege and Launch Race too but never grants there, because no team counts as eliminated in those modes. In the reference it is a check box on the host\'s advanced-options page for Spawn Control, Zone Control, Relic Run and Flag Chain, and an absent value counts as on. The typed defaults reproduce every effect of an absent option: no time limit; sudden death off; reinforcement interval 60 mode seconds; reinforcement count and Transport cap from the map, with no Transports when the map sets none; no reinforcement turrets; turret cap 2; no crate drops; random crate placement; Catch-up multiplier 1; and no starting powers.'),

# ---- B: DEC-079 Siege caps and bot-filled slots (X-DEC-4a)
('B DEC-079',
 'Two reference co-op maps set no population cap.',
 'Two reference co-op maps set no population cap. In Siege each defending team has the defender cap, and the attackers share (map cap − defenders × defender cap) ÷ attackers, by integer division; two Siege maps set no defender cap and use map cap ÷ player count for both sides. Because empty slots below the player count are filled with bots at match start, the cap equals map cap ÷ teams in play at start, and only later departures leave shares unused.'),

# ---- B: DEC-081 multiplier details (X-DEC-4a)
('B DEC-081',
 'its (own − 1) ÷ total term is always 0, and it has no effect in Siege.',
 'its (own − 1) ÷ total term is always 0, and it has no effect in Siege. The multiplier is 1 while any Spawn Node on the map is neutral and when the map disables it, and 0 for the no-team; allies count as other teams, and the average uses integer division. It can help only a team holding exactly one Spawn Node, and only on maps with at least 7 (two players), 9 (three) or 11 (four) Spawn Nodes. The burst size int(20 × H) runs from 20 at a setting of 0 % to 38 at 90 %. Only Spawn Control and Zone Control can use it: the option is absent in Relic Run, Launch Race and Flag Chain, and Siege maps have no Spawn Nodes.'),

# ---- B: DEC-084 Rogue faction deaths and Hive sorties (X-DEC-4a)
('B DEC-084',
 'A destroyed Hive releases no Essence orbs, and a Hive\'s ants do not count against the Rogue faction\'s cap.',
 'A destroyed Hive releases no Essence orbs, and a Hive\'s ants do not count against the Rogue faction\'s cap. No death of a Rogue faction actor leaves an Essence orb; a Hive\'s sortie check counts only the Rogue faction\'s other bodies (its bot controller, crate creatures and eggs), and a Hive releases one ant per 0.2 s.'),

# ---- A8 / B: DEC-087 every named bot correction (X-DEC-4b #1, X-DEC-5a #2, X-DEC-3 #4)
('A8 DEC-087',
 'The corrections include: correct ids where wrong ids are passed; no NaN priorities; no stall of Leader promotion while the bot team has a selection; an initial navigation graph that accounts for Barrier Emitters; turret placement around the whole base rather than in one quadrant; and a bot controller placed inside the map rather than at its corner. The unbounded bot loops are bounded (DEC-014).',
 'The corrections include: correct ids where wrong ids are passed, including the current power id instead of a stale one; no NaN priorities; no stall of Leader promotion while the bot team has a selection; Leader powers left without a Leader expire instead of accumulating in Power slots; an initial navigation graph that accounts for Barrier Emitters, because the obstacle grid is built before the bot target links; crossings stored independently of a Barrier Emitter\'s state, so a toggle needs no graph rebuild; a bot turret placed at its most threatened qualifying frontier node, not the last one in structure order; turret placement around the whole base rather than in one quadrant; a power-placement centroid that divides the Worker sum by the Worker count, not by the count of all friendly actors; a Relic un-stick centroid that no longer divides by the count of all collisions; a random walk onto a sea node that no longer leaves the army in place; a recall rate that does not scale with array capacity; an active Siege bot Transport behaviour, which never runs in the reference; defending formations turned towards the threat; a Siege defence objective that does not fall back at once (map validation binds its marker, DEC-561); and a bot controller placed inside the map rather than at its corner. The unbounded bot loops are bounded (DEC-014).'),
('A8 DEC-087 rec',
 'Recommendation: A. Bot strength is then calibrated by playtest (DEC-112).',
 'Recommendation: A. Bot strength is then calibrated by playtest in every mode, Siege included (DEC-112).'),

# ---- A12: DEC-088 obstacle-grid band around Barrier Emitter lines (H7)
('A12 DEC-088',
 'Barrier Emitter collision geometry that matches its visible geometry;',
 'Barrier Emitter collision geometry that matches its visible geometry, so the obstacle-grid band that marks each Barrier Emitter line one cell wider on every side never blocks movement beyond the visible line and serves at most to select collision candidates;'),

# ---- B: DEC-092 further per-tick rules (X-DEC-4a)
('B DEC-092',
 'the gravity of Crawler eggs and spiders, and the empty-Hive counter.',
 'the gravity of Crawler eggs and spiders, the empty-Hive counter, the volcano vent eruption, tree drops, the Spitter Plant shrink, Collector delivery (one Essence orb per tick), Spawn Surge requests (one per tick), the Launch Vessel passenger release, the Relay overshoot and the game-speed hold counters. Under DEC-039 every one of them scales the same way.'),

# ---- B: DEC-094 query consequences (X-DEC-5a #1); stable occupant key (X-DEC-3 #1)
('B DEC-094',
 'where the occupant is the first operating Worker in grid result order.',
 'where the occupant is the first operating Worker in grid result order (stable key in DEC-016). Count queries count whole cells of the square that bounds the range, so they over-count by up to one cell beyond it; a cell that has ever held an actor answers a presence check as occupied for the rest of the match; and a cache hit returns the cached list unfiltered, including actors outside the requested range.'),

# ---- B: DEC-096 one click, one command (X-DEC-4b #2)
('B DEC-096',
 'and one click produces one selection command. The composable binding graph',
 'and one click produces one selection command (DEC-119). The input corrections that this entry does not name are DEC-554. The composable binding graph'),

# ---- A9: DEC-097 cross-cites to the audio open questions
('A9 DEC-097',
 'and a missing sample is a diagnostic, not a fatal assertion (DEC-015).',
 'and a missing sample is a diagnostic, not a fatal assertion (DEC-015). The behaviour of attached sounds when their emitter is removed and the start value of smoothed parameters are open questions (DEC-321, DEC-322), and loop keep-alive is DEC-323.'),

# ---- B: DEC-098 first-requester outcomes (X-DEC-3 #8, X-DEC-5a #10)
('B DEC-098',
 'Two requests for different processing of one image get two cache entries.',
 'Two requests for different processing of one image get two cache entries. In the reference the start-up preload loads the shared cloud and laser textures with mipmaps first, which is why the first requester fixes their processing; two outcomes are visible: the colour marker image becomes a white alpha mask when the colour-circle marker pass draws before the recent-Power pass in the first frame, and the no-mip draws of the laser and cloud textures can sample an incomplete texture after a cache flush. Case variants of one name would be separate entries; Species ids are lower case, so one entry results.'),

# ---- B: DEC-099 miss results and start-up language order (X-DEC-5b #8); ADR-0063 (X-ADR-b)
('B DEC-099',
 'A string that a language lacks falls back, per key, to the base language. A language is offered only when it is complete.',
 'A string that a language lacks falls back, per key, to the base language. A language is offered only when it is complete; the content build reports the key coverage of every language (ADR-0063). The start-up language falls back from the language preference to the operating-system locale\'s language, then to the base language, then to the first language file present. In the reference a missing key shows the key text, a missing table or a null key shows a fixed error text, the miss result lives in one shared buffer that the next miss overwrites (DEC-102), and the previous string table stays alive for 5 main-loop iterations after a language change.'),

# ---- B: DEC-100 disagreeing defaults (X-DEC-4b #4, X-DEC-5c #6); ADR-0061 (X-ADR-b)
('B DEC-100',
 'so installed and development builds use the same defaults.',
 'so installed and development builds use the same defaults (ADR-0061). The table holds one value where reference call sites disagree, and the effective reference value is the one the runtime reader uses: screen refresh 75 or 60; colour depth 32 or 16; water detail none or 1 (with the key missing, a match shows flat water while the options page shows high); the pixel-shader, structure-detail and help-enabled keys, no default or 1; sound buffer 512 or 2000; mix frequency 22,050 or 44,100; sound channels 32 or 16 (with the key missing, the Apply button shows as active); vertical sync on in code and off in the defaults file. Colour depth, buffer size and mix frequency have no Species equivalent, and the network keys that nothing reads are listed in DEC-250.'),

# ---- A2: DEC-101 keeps the open design choice; the integrity part is DEC-118
('A2 DEC-101',
 '- Specified behaviour: A seasonal theme (seasonal hats on Workers and ice ramps) is switched on by each client\'s own date check, and the flag is relayed through the command stream into the simulation.\n- Options: A — the reference behaviour; B — the host decides the theme in the match setup, and it is sequenced like any other match option; C — drop the seasonal theme.\n- Recommendation: B, the stated default, with new seasonal art.',
 '- Specified behaviour: The reference has a seasonal theme (seasonal hats on Workers and ice ramps) that each client switches on from its own date check, from 24 to 26 December local time, and whose flag is relayed through the command stream into the simulation. Whether the theme is active is fixed once per match and identical for every participant (DEC-118); this entry decides whether Species keeps a seasonal theme and who chooses it. Until it is decided, the host\'s match setup applies the reference date rule once, from the host\'s local date.\n- Options: A — the host decides the theme in the match setup, and it is sequenced like any other match option; B — drop the seasonal theme.\n- Recommendation: A, the stated default, with new seasonal art; under either option the seasonal state follows DEC-118.'),

# ---- A3 / B: DEC-102 window-core quirks Q1-Q10 (X-DEC-5b notes 2, facts 6 and 7)
('A3 DEC-102',
 '- Specified behaviour: The Species UI framework avoids by construction all 33 framework defects that the UI framework component lists. They include: Escape cancels and never commits; gamepad Back is handled as input, not polled during rendering; message text containing a percent sign displays literally; and no fixed-size array limits a list.',
 '- Specified behaviour: The Species UI framework avoids by construction the 10 window-core quirks and all 33 framework defects that the UI framework component lists. The window-core corrections include: a press and its release act on the same widget, with pointer capture; a held button repeats on explicit timers (ADR-0032); a relayout keeps view-model state (ADR-0033); and the maximise path never accesses a missing object, a crash defect. The reference framework supports tooltips, but nothing registers one, so no tooltip appears; Species specifies none, and a tooltip feature is front-end design for the design phase. The framework corrections include: Escape cancels and never commits; gamepad Back is handled as input, not polled during rendering; message text containing a percent sign displays literally; no fixed-size array limits a list; a disabled widget never acts, so a host button stays inactive while hosting is not permitted (DEC-243); and list rows are removed by identity, so shrinking a list never removes the wrong row, such as the Back entry.'),

# ---- B: DEC-111 Collector throughput, rocket moves (X-DEC-3 #2)
('B DEC-111',
 'and Launch Vessel survival (DEC-108). The formulas are the specification.',
 'and Launch Vessel survival (DEC-108). The Collector throughput of about 2.5 Workers per second for a 500 wu round trip is such an estimate, from a movement law with turn rate 9.5 × dt, velocity filter 0.92 / 0.08 and arrival radius 5 wu; the rocket\'s outcome of 8 or 9 moves also depends on its effect slot (DEC-455). The formulas are the specification.'),

# ---- A8: DEC-112 calibration covers Siege and bot defence (X-DEC-4b #1)
('A8 DEC-112',
 'a stale power id; the Siege defence fall-back; and NaN priorities.\n- Options: A — keep the reference difficulty parameters after the corrections; B — re-calibrate the parameters by playtest.\n- Recommendation: B. Apply the corrections first, then calibrate each difficulty by playtest rather than by parity.',
 'a stale power id; the Siege defence fall-back; and NaN priorities. Two corrections change Siege and bot defence most: the Siege bot Transport behaviour runs only after its correction, and defending formations turn towards the threat, with a threat search centred about 59 wu ahead along the facing and a radius of 90 wu, so defenders miss enemies more than about 31 wu behind them.\n- Options: A — keep the reference difficulty parameters after the corrections; B — re-calibrate the parameters by playtest, per mode.\n- Recommendation: B. Apply the corrections first, then calibrate each difficulty by playtest rather than by parity, in every mode; Siege attack and Siege defence are calibrated explicitly, because the corrections above change them most.'),

# ---- B: DEC-117 empty Solar Array team change (X-DEC-4a)
('B DEC-117',
 'an empty Solar Array\'s per-tick team change is silent, because a team-change sound is bound only for Control Zones and Relics;',
 'an empty Solar Array\'s per-tick team change is silent, because a team-change sound is bound only for Control Zones and Relics, and with one no-team value (DEC-464) the change itself disappears, because it comes from comparing two different no-team values;'),
('B DEC-018',
 'Essence orbs travel along Relays in 10h + 1 ticks for h hops and are discarded',
 'Essence orbs travel along Relays in at most 10h + 1 ticks for h hops (DEC-455) and are discarded'),
('X DEC-106',
 'The world tick rate and per-tick budget are deferred topics (decisions/deferred/process_tick_and_roadmap.md).',
 'The world tick rate and per-tick budget are deferred topics (DEC-227, DEC-228).'),
]

# New entries (A2, A4), appended as a new section after DEC-117.
NEW_SECTION_ANCHOR = '- Recommendation: Resolved: each fact is confirmed, and the owning reference document states it as a rule.\n'
NEW_SECTION = '''
## 9 Further integrity rules

### DEC-118 The seasonal state is decided once per match
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/animation_and_vfx.md (VFX)
- Specified behaviour: Whether the seasonal theme is active is match state. It is decided once, in the match setup before match start, recorded with the setup and identical for every participant; no client decides it from its own date or clock, and nothing changes it during the match. Seasonal effects such as snow draw from the presentation generator (DEC-013). Whether Species has a seasonal theme, and who chooses it, is DEC-101.
- Options: A — one seasonal state per match, fixed in the match setup and identical for all participants; B — the reference behaviour, in which each client switches the theme on from its own date check and relays the flag through the command stream into the simulation.
- Recommendation: A. A flag that each participant derives from its own clock makes a match depend on local time and local state, which DEC-009 and DEC-010 exclude; a state fixed in the match setup is part of the recorded match and replays identically.

### DEC-119 Simulation code never issues player commands
- Status: Resolved
- Affects: reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), reference/06_controls_camera_and_commands.md (R-CTRL), architecture/components/input.md (INP)
- Specified behaviour: Player commands come only from a player's input or from an authorised command source, such as a bot or a deputy, and every command passes the same validation path (DEC-202). Executing a command or advancing the simulation never issues a further player command as a side effect: a rule that needs a follow-up change applies it inside the same simulation step. One Leader toggle therefore yields one command and one selection change on every participant.
- Options: A — the simulation never issues player commands, and follow-up changes happen inside the step; B — the reference behaviour, in which executing one Leader toggle command makes every participant issue a further selection command, so one click becomes as many identical commands as there are participants.
- Recommendation: A. A command that the simulation issues is input that no player gave: it multiplies with the participant count and is harmless only while the sequencer keeps one selection per sender and tick. Keeping the simulation a function of the commands it receives is the rule that DEC-008 sets for player intent.
'''

NEW_INDEX_ANCHOR = None  # set by the apply script from the last index row
NEW_INDEX_ROWS = [
    ('DEC-118', 'The seasonal state is decided once per match', 'Resolved', 'R-SIM, R-META, VFX'),
    ('DEC-119', 'Simulation code never issues player commands', 'Resolved', 'R-SIM, R-CTRL, INP'),
]
