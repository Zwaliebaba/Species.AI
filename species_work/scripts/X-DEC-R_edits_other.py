"""X-DEC-R: amendments to the other register files, the deferred files and the ADRs (data only).

EDITS maps a path (relative to the repository) to a list of (tag, old, new). Imported by X-DEC-R_apply.py.
"""
OQ = 'design_next/decisions/open_questions.md'
UB = 'design_next/decisions/units_buildings_crates_economy.md'
CB = 'design_next/decisions/controls_bots_maps_meta_audio.md'
SI = 'design_next/decisions/simulation_terrain_render_assets.md'
VA = 'design_next/decisions/vfx_audio_ui.md'
NW = 'design_next/decisions/deferred/networking.md'
NU = 'design_next/decisions/deferred/numerics_lobby_terrain_services.md'
PT = 'design_next/decisions/deferred/process_tick_and_roadmap.md'
AR = 'design_next/adr/README.md'
A71 = 'design_next/adr/0071-install-layout-runtime-files-and-user-data.md'
A63 = 'design_next/adr/0063-localisation.md'
A75 = 'design_next/adr/0075-application-window-and-dpi-awareness.md'
A35 = 'design_next/adr/0035-procedural-animation-runtime.md'
A33 = 'design_next/adr/0033-ui-layout-scaling-and-dpi.md'
A59 = 'design_next/adr/0059-content-build-and-validators.md'
A22 = 'design_next/adr/0022-audio-engine-on-xaudio2.md'

DEC306_OLD = '''### DEC-306 Spawn-network transit time and update order
- Status: Open
- Affects: reference/05_population_flow.md (R-ECON), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM)
- Specified behaviour: A transit orb that passes along h Relays reaches its Spawn Node after between 9h + 1 and 10h + 1 ticks. A hand-off to a structure that comes later in the structure update order advances again in the same tick, so the exact transit time on a map depends on the order in which its structures load. DEC-002 states the upper bound, 10h + 1.
- Options: A — keep the order-dependent transit time, with the structure order fixed as the stable structure-index order (DEC-016); B — a fixed transit time of 10h + 1 ticks, independent of the update order; C — a fixed transit time of 9h + 1 ticks.
- Recommendation: B. It matches the figure the population flow is described with (DEC-002, DEC-018), removes the dependence on load order, and changes timing by at most one tick per hop.
- Decide by: design phase
'''
DEC306_NEW = '''### DEC-306 Spawn-network transit time and update order
- Status: Dropped
- Superseded by DEC-455.
- DEC-455 decides the same-tick advance of every object handed over during a tick, the spawn-network transit of 9h + 1 to 10h + 1 ticks included, and its options give this entry's order-dependent, 10h + 1 and 9h + 1 transit times.
'''

DEC323_OLD = '''### DEC-323 Loop keep-alive polling
- Status: Open
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN), reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE)
- Specified behaviour: Game code keeps looping sounds alive by polling how many instances of a sound are playing for an emitter, and starts the loop again when it finds none. The poll cannot see an instance's emitter until an audio update has resolved it, and it never sees non-positional or stationary instances, so a loop can be started twice. The Launch Vessel loops of Launch Race are affected; the question is design-critical for that mode's sound.
- Options: A — the reference polling; B — the audio engine records the emitter when a sound is triggered, and loops are started and stopped by explicit events, so a loop never runs twice for one emitter.
- Recommendation: B. It removes duplicate loops by construction and fits the event model of DEC-007.
- Decide by: design phase
'''
DEC323_NEW = '''### DEC-323 Loop keep-alive polling
- Status: Resolved
- Affects: reference/10_audio_design.md (R-AUD), architecture/components/audio_engine.md (AEN), reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE)
- Specified behaviour: The audio engine records a sound's emitter when the sound is triggered, so a query made before the next audio update, or for a non-positional or stationary sound, finds the instance. Looping sounds are started and stopped by explicit events, so a loop never runs twice for one emitter; the Launch Vessel loops of Launch Race follow this rule. Instance counts are defined in every voice state (DEC-752), and the other audio corrections are DEC-097.
- Options: A — the corrected rule; B — the reference behaviour: game code keeps looping sounds alive by polling how many instances of a sound are playing for an emitter, and starts the loop again when it finds none; the poll cannot see an instance's emitter until an audio update has resolved it and never sees non-positional or stationary instances, so a loop can be started twice; and a stop request with no emitter matches every instance whose emitter was never set, that is, every non-positional and stationary sound.
- Recommendation: A. Recording the emitter at trigger time removes duplicate loops by construction and fits the event model of DEC-007.
'''

EDITS = {
OQ: [
    ('X OQ scope Resolved',
     'The **Resolved** entry records reference facts that are now settled and are stated as rules. Each **Dropped** entry records a group of questions that concern only content the library does not specify, with the reason.',
     'A **Resolved** entry states a settled reference fact or a corrected rule as a rule. Each **Dropped** entry records a group of questions that concern only content the library does not specify, with the reason, or an entry superseded by the entry it names.'),
    ('X OQ scope range',
     '`decisions/verdicts_and_fixes.md` (DEC-001 to DEC-117);',
     '`decisions/verdicts_and_fixes.md` (DEC-001 to DEC-119);'),
    ('A5 DEC-306', DEC306_OLD, DEC306_NEW),
    ('A5 DEC-306 index',
     '| Spawn-network transit time and update order | Open | R-ECON, R-BLDG, R-SIM |',
     '| Spawn-network transit time and update order | Dropped | R-ECON, R-BLDG, R-SIM |'),
    ('A5 DEC-304',
     'Which of the two applies depends on whether the Supply crate opens before or after that Spawn Node\'s update within the same tick.',
     'Which of the two applies depends on whether the Supply crate opens before or after that Spawn Node\'s update within the same tick. It is one instance of the same-tick rule of DEC-455, which does not fix the count; this entry does.'),
    ('B DEC-323', DEC323_OLD, DEC323_NEW),
    ('B DEC-323 index',
     '| Loop keep-alive polling | Open | R-AUD, AEN, R-MODE |',
     '| Loop keep-alive polling | Resolved | R-AUD, AEN, R-MODE |'),
    ('A9 DEC-321',
     'Which behaviour is intended is not recorded; the question is design-critical for the sound of deaths and removals.',
     'Which behaviour is intended is not recorded; the question is design-critical for the sound of deaths and removals. It is not among the audio corrections of DEC-097.'),
    ('A9 DEC-322',
     'so authored random pitch ranges are compressed towards 1.0 at the start of a note: a range of 0.75 to 1.35 becomes about 0.88 to 1.16.',
     'so authored random pitch ranges are compressed towards 1.0 at the start of a note: a range of 0.75 to 1.35 becomes about 0.88 to 1.16. It is not among the audio corrections of DEC-097.'),
    ('X DEC-320',
     'which sounds like a tape slowing down. Its audible character has not been checked.',
     'which sounds like a tape slowing down. DEC-569 keeps the drop as a deliberate effect of the world buses; its audible character has not been checked, and this entry confirms it.'),
],
UB: [
    ('A5 DEC-455 spec',
     'and the order of Transit Gate waves against the cap follows structure order. DEC-002 already fixes the Relay transit at 10h + 1 ticks.',
     'and the order of Transit Gate waves against the cap follows structure order. The exact spawn-network transit on a map therefore depends on the order in which its structures load; DEC-002 states its upper bound, 10h + 1 ticks. The fixed Spawn Surge count is DEC-304, the rocket\'s move count is a derived value of DEC-111, and the draws that initialise a new object happen in its spawn step (DEC-652).'),
    ('A5 DEC-455 option A',
     '- Options: A — reproduce the reference slot allocation, so that every same-tick advance matches;',
     '- Options: A — reproduce the reference slot allocation and structure order, so that every same-tick advance matches, with the structure order fixed as the stable structure-index order (DEC-016);'),
],
CB: [
    ('A7 DEC-555',
     'If none remain, because they all died or the group was released after the click, the order does nothing: no route is created and no Worker receives an order.',
     'If none remain, because they all died or the group was released after the click, the order does nothing: no route is created and no Worker receives an order. This is the crowd-order case of DEC-653, under which simulation state never holds a non-finite value.'),
    ('B DEC-561',
     '  3. Siege: one Core Device owned by a defending start position, at least three Command Consoles linked to it, and a Core Device timer equal to the match time limit;',
     '  3. Siege: one Core Device owned by a defending start position, at least three Command Consoles linked to it, a Core Device timer equal to the match time limit, and every defence bot objective marker that has a scan range bound to a structure (an unbound marker defaults to team 0, and its defence objective falls back at once, DEC-087);'),
    ('A6 DEC-567',
     'Where Species uses effects, they are applied per bus (DEC-027).',
     'Where Species uses effects, they are applied per bus (DEC-027), and switching them off leaves no effect processing on any voice or bus (DEC-752). A DSP parameter that an effect binding omits is 0.0 in the reference, not its declared default; the schema default of each DSP parameter is therefore chosen deliberately, 0.0 for parity or the declared value, and it is audible only once effects are enabled.'),
    ('A7 DEC-569',
     'The pitch drop at match end is kept as a deliberate effect: as the game speed falls, the world sounds fall to 0.1× pitch.',
     'The pitch drop at match end is kept as a deliberate effect: as the game speed falls, the world sounds fall to 0.1× pitch, and a listening test confirms its character (DEC-320). The smoothed game-speed value starts every match at normal speed and is discarded at match end (DEC-751).'),
],
SI: [
    ('A5 DEC-652',
     'These rules extend the draw discipline of DEC-013; the draw order of the reference generator is not reproduced, and every distribution is unchanged.',
     'These rules extend the draw discipline of DEC-013; the draw order of the reference generator is not reproduced, and every distribution is unchanged. When a newly created object first advances is DEC-455.'),
    ('A7 A10 DEC-653',
     'What such an order then does is the controls decision on orders with no controlled Workers (`decisions/open_questions.md`), under the handler rule of DEC-202 item 7.',
     'Such an order then does nothing (DEC-555), under the handler rule of DEC-202 item 7.'),
],
VA: [
    ('X vfx table',
     '| Seasonal theme | DEC-101 |',
     '| Seasonal theme; the seasonal state is decided once per match | DEC-101, DEC-118 |'),
],
NW: [
    ('A4 DEC-202 item 5',
     'The simulation never issues player commands as a side effect.',
     'The simulation never issues player commands as a side effect (DEC-119).'),
    ('A10 DEC-202 item 7',
     'Every command handler defines its result when its target no longer exists.',
     'Every command handler defines its result when its target no longer exists; a crowd order for an empty group does nothing (DEC-555).'),
    ('B DEC-202 item 8',
     'In Species a cheat is a command that only an authorised operator may send, validated by the server.',
     'In Species a cheat is a command that only an authorised operator may send, validated by the server. Reference cheats that change simulation state: kill all enemies; spawn Workers for each of the six team slots; spawn a Transport, Crawlers or Essence orbs (the last draws 20 values from the simulation generator); spawn structure ports; spawn a Supply crate; and allow arbitrary placement, which changes only the local placement check that every participant repeats when the placement command executes, so in Species it is a session rule that the server applies, not a client flag. Cheats with no simulation effect: the profiler, sound editor, sound statistics and network statistics windows, and a render-resource reset, after which trees regenerate from the same per-tree seed.'),
    ('X networking numeric model',
     'a pinned numeric model (`numerics_lobby_terrain_services.md`)',
     'a pinned numeric model (DEC-240)'),
    ('X networking lobby',
     'Who may change lobby and match-setup state is decided in `numerics_lobby_terrain_services.md`.',
     'Who may change lobby and match-setup state is decided in DEC-243.'),
    ('X networking persistence',
     'Persistence and world-save migration across patches are decided in `numerics_lobby_terrain_services.md`.',
     'Persistence and world-save migration across patches are decided in DEC-250 and DEC-251.'),
    ('X networking identity',
     'Identity, entitlement, encryption keys and discovery are decided in `numerics_lobby_terrain_services.md`.',
     'Identity, entitlement, encryption keys and discovery are decided in DEC-248 and DEC-249.'),
],
NU: [
    ('X DEC-240 item 3',
     'and in every worker that runs simulation code, and no third-party code may run inside a tick.',
     'and in every worker that runs simulation code, and no third-party code may run inside a tick; gameplay is computed only by the CPU simulation (DEC-654).'),
    ('X DEC-241 item 3',
     'Integer tick timers resolve this, and the 11-tick mode second stays an explicit constant (`decisions/verdicts_and_fixes.md`).',
     'Integer tick timers resolve this, and the 11-tick mode second stays an explicit constant (DEC-002).'),
    ('B DEC-241 item 5',
     'turret target wobble uses rounded trigonometry.',
     'turret target wobble uses rounded trigonometry, and the same rounded trigonometry drives the Supply crate sway and the dust vortex in simulation paths.'),
    ('X DEC-243 item 4',
     'and the bot-difficulty check accepts one value past the last level.',
     'and the bot-difficulty check accepts one value past the last level (corrected by DEC-077).'),
    ('B DEC-245 item 7',
     'There is no sort column for the player to choose.',
     'There is no sort column for the player to choose; sortable columns and a ping column are recommended.'),
    ('B DEC-246 item 7',
     'All three are observable in the generated terrain.',
     'All three are observable in the generated terrain; on the reference maps the edge wrap affects only heights below sea level, so play never sees land at the edge, and the truncating index conversion also underlies the Abductor wander-area restriction (DEC-113).'),
    ('A13 DEC-250 path',
     'writes them to the `<STUDIO>\\Species\\` profile folder.',
     'writes them to the `%LOCALAPPDATA%\\<Studio>\\Species\\` profile folder (ADR-0071).'),
    ('B DEC-250 item 1',
     'and the graphics and other options listed in reference/09.',
     'and the graphics and other options listed in reference/09. The defaults also write a server address, a server port and a network-bypass flag that nothing reads; the live ports are the server and client port settings.'),
],
PT: [
    ('B DEC-222 item 2',
     'some renderers write simulated fields;',
     'some renderers write simulated fields, which DEC-007 forbids: the Supply crate orientation and bubble timer, once per rendered frame; a thrown weapon\'s orientation, tumble and flash count, and its flash sound; the shell-casing facing that its advance reads; the Barrier Emitter link, which its renderer marks as absent when the linked id is missing or names no Barrier Emitter, for emitters in view only (Species validates links at build, DEC-015, and the simulation treats a missing link as absent); the Beam Dish range and receiver, which its renderer saves and restores around link drawing; a Solar Array\'s orientation from the terrain normal, equal to the value its initialisation sets; and a turret\'s facing, in the editor only. The world cursor\'s crosshair assigns, instead of comparing, the structure kind of the local team\'s selected structure, which is dormant because only the Beam Dish and turrets can be selected;'),
    ('B DEC-222 item 7',
     'particles, debris and other cosmetic effects never change a simulation outcome.',
     'particles, debris and other cosmetic effects never change a simulation outcome. These cosmetic draws move from the simulation stream to the presentation generator (DEC-013): the ant leg shape (every tick per ant), snowflake and Wisp wobble (four draws per object per tick), meteor fire size (five per tick), Spitter Plant fire size, turret shell casings (four per shot) and overheat smoke, Trooper casings, the Worker abduction spark and the volcano vent idle draw. Spider leg and foot state, with its wall-clock swing timing, is presentation only (DEC-007).'),
    ('B DEC-225 item 10',
     '  9. The window message pump belongs to the platform layer and runs once per frame, not inside the input update.\n',
     '  9. The window message pump belongs to the platform layer and runs once per frame, not inside the input update.\n  10. Frame budget: the reference renders as fast as it can, with no sleep in the match loop; clamps the presentation frame delta to 0.25 s; advances audio a second time after rendering when the frame rate is below 15; writes a memory report to the log every 60 s; pins its main thread to four alternate cores; and marks 50, 100 and 200 ms per frame on its profiler graph. Of these, only the 0.25 s presentation clamp has a Species counterpart to state (ADR-0012).\n'),
],
AR: [
    ('A13 README path',
     'that writes user data under `%LOCALAPPDATA%\\Species` and never beside the executable;',
     'that writes user data under `%LOCALAPPDATA%\\<Studio>\\Species\\`, where `<Studio>` is the publisher-name parameter, and never beside the executable;'),
    ('A13 README 2.3',
     '| 2.3 | Dedicated-server compilers (Linux) | **Clang 23 (LLVM 23.1.3) + libc++** primary, **GCC 16 (16.2.0)** secondary; shared code uses the intersection with MSVC | medium | [ADR-0052](0052-cpp23-language-standard.md), [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) |',
     '| 2.3 | Dedicated-server compilers (Linux) | **Clang 23 (LLVM 23.1.3) + libc++** primary, **GCC 16 (16.2.0)** secondary; shared code uses the intersection with MSVC | medium | Design phase ([Hosting](#deferred-to-the-design-phase)); [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) and [ADR-0052](0052-cpp23-language-standard.md) keep shared code within the portable subset |'),
    ('X README WACK',
     '- Whether the Agility SDK passes MSIX and WACK certification (only a 2021 statement was found).',
     '- Whether a package that contains the Agility SDK passes Store (MSIX) certification (only a 2021 statement was found). A re-check on 2026-10-17 (https://learn.microsoft.com/windows/msix/package/packaging-uwp-apps#validate-your-app-package) found that the Windows App Certification Kit is deprecated and no longer maintained, and that certification runs automatically when a package is submitted to Partner Center; [ADR-0072](0072-distribution-channels-and-package-formats.md) therefore requires a test submission first.'),
    ('X README deferred header',
     '| Topic | Problem the design phase must solve |\n|---|---|\n',
     '| Topic | Problem the design phase must solve | Register entries |\n|---|---|---|\n'),
    ('X README deferred networking',
     'and the bandwidth for hundreds of crowd units in view. |',
     'and the bandwidth for hundreds of crowd units in view. | DEC-200 to DEC-208 |'),
    ('X README deferred numeric',
     'and how positions keep their precision far from the origin. |',
     'and how positions keep their precision far from the origin. | DEC-240 to DEC-242 |'),
    ('X README deferred threading',
     'and kept separate from presentation, both in-process and in a dedicated server. |',
     'and kept separate from presentation, both in-process and in a dedicated server. | DEC-220 to DEC-226 |'),
    ('X README deferred tick',
     'starting from the reference tick rate in `reference/12_simulation_model/`. |',
     'starting from the reference tick rate in `reference/12_simulation_model/`. | DEC-227, DEC-228 |'),
    ('X README deferred persistence',
     'versioned and migrated across patches, with backup and rollback. |',
     'versioned and migrated across patches, with backup and rollback. | DEC-250, DEC-251 |'),
    ('X README deferred sessions',
     'lobby, discovery, slot assignment and host authority. |',
     'lobby, discovery, slot assignment and host authority. | DEC-243 to DEC-245 |'),
    ('X README deferred services',
     'and the services a later companion app would use. |',
     'and the services a later companion app would use. | DEC-248, DEC-249 |'),
    ('X README deferred hosting',
     'moderation, server crash reporting and artefact signing. |',
     'moderation, server crash reporting and artefact signing. | DEC-220 |'),
    ('X README deferred terrain',
     'and how terrain content identity is checked. |',
     'and how terrain content identity is checked. | DEC-246, DEC-247 |'),
],
A71: [
    ('A13 ADR-0071 path',
     '`%LOCALAPPDATA%\\<STUDIO>\\Species\\` with `settings\\`',
     '`%LOCALAPPDATA%\\<Studio>\\Species\\` with `settings\\`'),
    ('A13 ADR-0071 note',
     '* `<STUDIO>` is the studio-name placeholder of CONVENTIONS §8.11.',
     '* `<Studio>` is the publisher-name parameter, the studio-name placeholder of CONVENTIONS §8.11; the same form names the user-data folder in the [ADR index](README.md#technology-baseline) and in the decision register.'),
],
A63: [
    ('X ADR-0063 IME',
     'the client supports input method editors through the window-message path chosen by the input ADR.',
     'the client supports input method editors through the window-message path chosen by the input ADR ([ADR-0028](0028-input-device-back-ends.md)); composition, caret, selection and clipboard follow [ADR-0032](0032-ui-focus-graph-page-stack-and-input.md).'),
    ('X ADR-0063 text',
     '(text rendering is specified by the UI framework ADRs).',
     '(text rendering is specified by [ADR-0034](0034-text-rendering-and-glyph-cache.md)).'),
],
A75: [
    ('X ADR-0075 input',
     'and forwards input messages to the input layer and size, DPI and focus events to the renderer and UI.',
     'and forwards input messages to the input layer ([ADR-0028](0028-input-device-back-ends.md)) and size, DPI and focus events to the renderer and UI ([ADR-0033](0033-ui-layout-scaling-and-dpi.md)).'),
],
A35: [
    ('B ADR-0035 legs',
     '- Bad, because procedural motion is code, so new creature motion needs engineering work rather than authored clips.',
     '- Neutral, because legs look different from the reference, in which the shin keeps its length and is turned outward by θ = −acos(min(1, 0.4·L ÷ thigh length)), the ratio clamped to 1, and the thigh is drawn from knee to hip at whatever length results, so it stretches and compresses; in Species both segments keep their lengths.\n- Bad, because procedural motion is code, so new creature motion needs engineering work rather than authored clips.'),
],
A33: [
    ('B ADR-0033 explainer',
     '- The UI framework component (`architecture/components/ui_framework/`) specifies the grid, the anchors and the overflow rules.',
     '- The UI framework component (`architecture/components/ui_framework/`) specifies the grid, the anchors and the overflow rules.\n- Explainer-animation panels (ADR-0035) follow the relayout rule: they are laid out again on a resize, DPI change or language change, whereas the reference lays a panel out once, when it is created.'),
],
A59: [
    ('B ADR-0059 mesh checks',
     '  * a separate collision asset for every collidable mesh;\n',
     '  * a separate collision asset for every collidable mesh; duplicate node names, and a hit-test mesh whose position count differs from its vertex count, are errors, while a socket-only mesh with no node is accepted (DEC-012; one reference Relic mesh has 1,182 positions for 1,082 vertices, and the Brood Drifter mesh 43 for 42);\n  * keys listed twice and repeated fields, which common parsers of the authoring syntax accept silently (DEC-801);\n'),
],
A22: [
    ('B ADR-0022 generator',
     'it uses the presentation random generator (`FxRng`).',
     'it uses the presentation random generator (`FxRng`), through its own instance, shared with no other system; its draws choose sample-group members, random emitters and ranged-random parameters (DEC-013).'),
],
}
