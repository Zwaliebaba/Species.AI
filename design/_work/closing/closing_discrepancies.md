# Collected "discrepancies" sections

<!-- from 02_units_and_entities.md -->
### From 02_units_and_entities.md

_Pending — being written._

<!-- from 03_buildings_and_world_objects.md -->
### From 03_buildings_and_world_objects.md

| Doc | Claim | Code reality | Ref |
|---|---|---|---|
| `docs/spirit resource system.txt` (Darwinia design note) | spirits of dead enemies are collected and used by factories to spawn units | Multiwinia spawn points use virtual master-emitted spirits; collected-spirit production exists only via Engineers/Harvesters delivering to spawn points and in legacy Factory/Incubator code | [C] `code/worldobject/spawnpoint.cpp`, `factory.cpp`, `incubator.cpp` |
| `docs/multiwiniachanges-beta.txt` (issue 756) | "Gun turret/Incubators emplacements shared by friendly teams don't fire/spawn" (bug report) | GunTurret ownership explicitly allows allied co-occupation (`IsFriend` check); the turret fires for the winning ally | [C] `GunTurret::RecalculateOwnership` |
| `docs/multiwinia-changes.txt` | "Virii no longer count towards your population limit" | consistent: population = `m_others.NumUsed() − Virii::s_viriiPopulation` | [C] `SpawnPoint::PopulationLocked` |
| `docs/game objects.txt` "Tree / forest" | "acts as an obstacle to anything in the air, but can be walked through" | ground units are pushed out of the trunk and crown spheres — trees block walking | [C] `Darwinian::PushFromObstructions`, `Entity::PushFromObstructions` |
| `docs/game objects.txt` "Tree / forest" | "? spawn seeds which sprout into new trees ?" (speculative) | implemented (`CreateAnotherTree`) but only crate-created trees and their offspring have a spawn budget; level trees never spread | [C] `Tree::CreateAnotherTree`, `Crate::GenerateTrees` |
| `docs/game objects.txt` "Laser Fence" | "can only be shut down by disconnecting / destroying power source" | in Multiwinia a fence keeps its level mode unless it is solar-linked (then it drops 5 s after the last friendly surge); fences are indestructible | [C] `LaserFence::Advance` |
| `docs/game objects.txt` "Wall" | "can be blown up with enough force" | matches: a single hit ≤ −300; in FULL only WallBuster paths reach it | [C] `Wall::Damage`, §7.2 |
| `docs/new building docs.txt` FenceSwitch | mentions "Pylon 1 / Pylon 2" values for passing on power surges | `FenceSwitch` has no pylon fields or surge handling (reads 2 fence links, switchValue, lockable, script, locked) | [C] `code/worldobject/switch.cpp:FenceSwitch::Read` |
| `contrib/systemiv/lib/math/random_number.h` comment | `sfrand`: "−range < result < range" | returns [−range/2, range/2] | [C] (P07) |
| `docs/multiwinia_manual.pdf` (Assault) | "sabotage the three control stations" | 3–4 linked stations per official map | [C] level files |
| `docs/multiwinia_manual.pdf` (Rocket Riot) | an explosion burns "all the fuel and all the Multiwinians inside, re-setting the rocket to its starting point" | MP: fuel burns down to 50% of its pre-explosion value; passengers are ejected (50% set on fire, the rest survive); the rocket returns to Refueling | [C] `EscapeRocket::AdvanceExploding` |
| `docs/multiwinia_manual.pdf` (CTS) | a shattered statue "re-appears randomly in one of the Statue Zones" | respawn after a randomised timer at the neutral zone farthest from existing statues (random only when none exist) | [C] `code/multiwinia.cpp` CTS block ([01](01_game_modes.md)) |
| source comment in `building.h` | "When you add an entry here remember to update building.cpp" | consistent (75 names for 75 values) | [C] |

<!-- from 04_crates_powerups_and_weapons.md -->
### From 04_crates_powerups_and_weapons.md

| Source | Says | Code / data does | Ref |
|---|---|---|---|
| `code/worldobject/crate.h` enum comments | SpawnMania "spawn constantly for 15 seconds" | 5.0 s | `Crate::RunSpawnMania` |
| same | Bitzkreig "all Darwinians will always throw grenades for 10 seconds" | 15 s global rage (`HasRage`), lasers not grenades | `Crate::RunBitzkreig`, `Darwinian::HasRage` |
| same | Forest "spawns 1–3 trees" | 1–2 trees | `Crate::GenerateTrees` |
| same | Eggs "spawns between 1 and 3 eggs" | 2–4 eggs | `Crate::RunEggs` |
| same | Rage "for 60 seconds" | 30 s task | `Task::Start` |
| same | Booster "immune to laser fire" | immune to every resistible damage type (explosives pop it) | `Darwinian::ChangeHealth` |
| same | Spam "spawns a Spam object" | 100 SpamInfections, no Spam building | `Crate::RunInfection` |
| same | Tank "a bitchin tank for 30 seconds" | commented out | `Crate::CaptureCrate` |
| `Crate::RunUncapturedCrate` (name) and `docs/sound changes.txt` "Expired (nobody captured the crate in time)" | suggest a bad effect for uncaptured crates | the function is the 10 % bad outcome of a **captured** crate; an expired crate does nothing (the call is commented out) and its `Expired` sound has no data block | `Crate::AdvanceOnGround`; `data/sounds.txt` |
| `docs/sound changes.txt` CratePowerup list | events Squad … Bitzkreig | data adds FormationBooster, FireExtinguisher, Tank (never fired) and RocketTurret/FlameTurret/NapalmStrike/SlowDown/SpeedUp/Engineer/TheBeast are never fired as such (turrets fire `GunTurret`, napalm fires `Airstrike`, speed crates and Engineer fire none) | `Crate::TriggerSoundEvent` call sites; `data/sounds.txt` |
| `data/language/english.txt` `crate_tooltip_squad` | "follow your every command for 60 seconds" | consistent (60 s, then the squad dies) | `Task::Start`, `Task::Stop` |
| `docs/multiwinia_manual.pdf` (Retribution Mode) | eliminated players periodically receive a power-up | consistent — live in code (contradicting the P08 draft, see §1.2) | `Multiwinia::AdvanceRetribution` |
| `docs/multiwinia_manual.pdf` (Crate Drop Mode) | Domination, KOTH, CTS only | consistent | `data/multiwinia.txt` |
| `docs/multiwinia-changes.txt` | "Napalm Strikes will now set fire to trees much more reliably" | trees/triffids within 100 take a flat −200 (no falloff) | `FlameGrenade::Advance` |
| `docs/multiwiniachanges-beta.txt` Issue 754 | "Permit crate/power-ups placement near trunk ports" (Assault) | only gun turrets are restricted (400 from enemy trunk ports, 200 from enemy spawn points); other power-ups are unrestricted there | `TaskManager::IsValidTargetArea` |

<!-- from 05_economy_and_population.md -->
### From 05_economy_and_population.md

| Doc / comment | Statement | Code behaviour | Ref |
|---|---|---|---|
| `docs/spirit resource system.txt` | a spirit "gradually floats away over the course of a minute" | MP: Birth 6.1 s + Floating 30–50 s (collectable) + ascent 90.1 s ≈ 126–146 s | `code/worldobject/spirit.cpp:Spirit::Advance` |
| `docs/spirit resource system.txt` | "When your creatures die they die for good and can not be respawned" | Harvesters recycle souls of every team, including your own | `code/worldobject/harvester.cpp:Harvester::AdvanceHarvesting` |
| `docs/spirit resource system.txt` | garbage collectors grab "2 or 3 at a time", "if they die they will drop their spirits"; factories need spirits | Harvester capacity 100 and sucks every soul within 75; it dies only by sucking up a grenade, which re-births its souls; spawn points use virtual master spirits; spirit factories are legacy/blocked | `harvester.cpp`, `spawnpoint.cpp`, `code/worldobject/factory.cpp` |
| `docs/multiwiniachanges-beta.txt` Issue 366 | "Magical forests produce red souls only if red around" | tree souls pick a uniform random team 0–3 regardless of surroundings (may be an unused slot) | `code/worldobject/tree.cpp:Tree::Advance` |
| `docs/multiwiniachanges-beta.txt` Issue 304 | "Engineer gets stuck trying to offload souls" | consistent: in MP an engineer can only unload at an incubator; no shipped official map has one, so engineers keep souls | `code/worldobject/engineer.cpp:Engineer::AdvanceIdle` |
| `docs/multiwiniachanges-beta.txt` Issue 612 and the `AdvanceSkirmish` comment ("ScoreMode 0: Final Population, 1: Population Per Second") | Domination scores population | ScoreMode 0 → score = spawn points owned now; 1 → cumulative spawn points per mode-second; population is never scored | `code/multiwinia.cpp:Multiwinia::AdvanceSkirmish` |
| `docs/multiwinia_manual.pdf` (via P02) | Handicap applies to "Domination, King of the Hill" | also an Assault option (inert there: no Assault map has spawn points) | `data/multiwinia.txt` |
| `docs/multiwinia-changes.txt` | "Virii no longer count towards your population limit, but have their own limit" | consistent (subtracted; 250 per team) | `SpawnPoint::PopulationLocked`, `code/worldobject/virii.h` |
| `code/level_file.h` comment on `LevelOptionDefenderReinforcementType` | "replaces the TrunkPortArmour option (which must be active for this option to have effect)" | applied whenever `AdvanceTrunkPorts` runs (`trunkportreinforcements` alone suffices); it suppresses port armour for that side | `Multiwinia::AdvanceTrunkPorts` |
| `code/level_file.h` comment on `LevelOptionNoGunTurrets` | "disables GunTurret tasks from TrunkPorts on RocketRiot" | disables all turret-wave tasks (incl. Blitzkrieg AirStrike upgrades) in any mode (used on `mp_blitzkrieg_2P_4`) | `Multiwinia::AdvanceTrunkPorts` |
| `code/level_file.h` comment on `LevelOptionTrunkPortReinforcements` | "activates the RocketRiot style trunk port reinforcements on a level" | in Rocket Riot/Blitzkrieg its count is ignored (game option wins) but it adds a second call per tick | `Multiwinia::Advance` |
| `SpawnPoint::Advance` comment | instant activation for "our first (or last) spawn point" | triggers whenever the team owns exactly one point at that moment | `code/worldobject/spawnpoint.cpp:SpawnPoint::Advance` |
| UI "seconds" (time remaining, reinforcement countdown) driven by `m_timer = 1.0` / `m_reinforcementTimer = 1.0` resets | one game second per decrement | one decrement per 11 ticks = 1.1 s, so displayed seconds and wave timers run 10 % slow relative to game time | `code/multiwinia.cpp:Multiwinia::AdvanceSkirmish`, `Multiwinia::AdvanceTrunkPorts`; §8.2 |
| `Multiwinia::Render` comment `// Rocket Riot` on the reinforcement countdown | countdown is a Rocket Riot feature | drawn on every map where trunk-port waves run (Assault, Blitzkrieg, `mp_koth_3P_4`, `mp_cts_3p_3`) | `code/multiwinia.cpp:Multiwinia::Render` |

<!-- from 06_player_control_and_input.md -->
### From 06_player_control_and_input.md

| Doc / comment | Claim | Code reality |
|---|---|---|
| `docs/input_preferences_syntax.txt` | grammar lists no `repeat`, `~` icon lines, `alias`, `value`, `->` pipe, `mouse any`, `key any` | `repeat` exists for XInput BOOL controls; `~` sets icons; `alias`/`value` drivers exist; pipe exists but is unregistered; `any` parses but never fires [C] §11.6 |
| same | `XInput <xinput_analog> <analog_cond> [with sensitivity <1-200>]` | sensitivity is also accepted after BOOL conditions on analogue controls and rejected on directional stick buttons [C] `XInputDriver::writeExtraSpecInfo` |
| same | "The user input preferences will override the FIRST binding" | true, and the keyboard-layout file does the same; repeated lines in one override file overwrite each other [C] §11.10 |
| same | unknown/invalid specs implied to fail | unknown condition words are silently accepted as `down` [C] §11.3 |
| `docs/DarwiniaInput.rtf` | action words include `moved` | the word is `move`; `moved` hits the fallback and acts as `down` [C] |
| same | `lib/input/transform.h` holds `Movement2D` | it is in `code/lib/input/movement2d.h` [C] |
| same | `g_controlBindings->ControlEvent(ControlBindings::ControlDeselect)` (old API) | current API is `g_inputManager->controlEvent(ControlUnitDeselect)`; no `ControlDeselect` exists [C] |
| `docs/betachanges.txt` | fixes for "Gestures Mode", "Icons task manager used when use Xbox 360 controller", Tab in Icons mode | these concern the Darwinia(+) interfaces, which exist but are unreachable in Multiwinia (§4.1) [C] |
| `docs/game objects.txt` | power-ups "triggered by a gesture of some kind" | Multiwinia power-ups are placed by click, never by gesture [C] `TaskManagerInterface::CreateTaskManager` |
| `docs/todo.txt` | key-binding window issues | the Controls options window is unreachable in the shipped build [C] `OptionsMenuWindow::Create` |
| DebugMenu captions "(F2)…(F9)" | debug hot-keys | no such bindings; F1–F8 are legacy task-manager keys, F9 = network stats [C] `data/input/input_preferences.txt` ([32](32_debug_test_and_performance.md)) |
| `code/location_input.cpp` comments "Player pressed CTRL-C, so demote this officer" / "so terminate this turret" | demote / terminate | the bound key is plain C; the officer is killed (`ChangeHealth(−999)`), the turret is only deselected (the damage call is commented out) [C] |
| `code/camera.h` comment | "Remember to update the static string table at the end of camera.cpp" | no mode-name table exists [C] |
| `code/camera.cpp` comment on `DistanceToBlockage` / `DirectDistanceToBlockage` | "If there is no blockage FLT_MAX is returned" | returns `_maxDist`; FLT_MAX only when there is no location [C] |
| `code/lib/input/keynames.cpp` comment | `"QUOTE", //192` | QUOTE is index 222; MINUS_PAD/Unknown swapped vs `keydefs.h` [C] |
| `code/lib/input/control_types.inc` comments | `CameraRotate` "rotate the cam in entity track mode"; `WeaponCycle*` "cycle weapons" | no consumer in the FULL build [C] |
| `code/lib/input/control_bindings.cpp` comment | `ControlBindings::replacePrimaryBinding` | commented out; replace is a flag on `bind` [C] |
| `docs/multiwinia_manual.pdf` (Officers) | "Right click on officer: set surrounding Multiwinians into formation" | consistent: RMB on an idle officer → `TOGGLEENTITY` → formation (§5.3); Darwinians join only if FOLLOW-searching within 150 u and the formation (90) is not full [C] |

<!-- from 07_ai.md -->
### From 07_ai.md

| Document / comment | Claim | Code | Ref |
|---|---|---|---|
| `docs/integrating virtual players into network system.txt` | a client may play "with an AI running locally" | the AI is a simulation entity run identically on every peer; CPU teams produce no network commands | `code/location.cpp:Location::InitialiseTeam`, `code/team.cpp:Team::Advance` |
| `docs/multiwinia_todo.txt` (Outstanding issues) | "Creating a CPU player should automatically spawn an AI entity" is outstanding | implemented: `Location::InitialiseTeam` and `Multiwinia::RemoveTeams` spawn one | `code/location.cpp`, `code/multiwinia.cpp` |
| `code/worldobject/aiobjective.h` header comment | several unlinked start objectives → "one will be selected randomly" | all stay active; each AI picks one (random, or a defender waits for the first one attacked) and deactivates its other own start objectives | `AI::AdvanceAssaultAI` |
| `docs/multiwiniachanges-beta.txt` v1.9.7 | "Crash AI::RunForest (Issue 328)" fixed | `RunForest` still dereferences `GetEntity(specials[i])` without a NULL check (07-U11) | `AI::RunForest` |
| `contrib/systemiv/lib/math/random_number.h` | `_syncsfrand`: "−range < result < range" | returns `(U[0,1) − 0.5) × range`, i.e. ±range/2 | `contrib/systemiv/lib/math/random_number.cpp:_syncsfrand` |
| `code/worldobject/ai.h` comment on `AITarget::m_areaControl` | pass / always-update scheme for clustered targets | field never read or written | `code/worldobject/ai.cpp` |
| `code/worldobject/armour.cpp` comment "bit of a cheat" | self-destruct at enemy spawn points is a shortcut | live AI-only behaviour (Standard armour) | `Armour::RunStandardAI` |
| `docs/multiwiniachanges-beta.txt` v1.9.7 "AI places two turrets inside one another (Issue 350)", "KOTH AI abandons hills (Issue 352)", "Multiple officers at the same point (Issue 314)" | fixed | consistent with code: `PushFromTurrets` + turret `DoesSphereHit(10)` validity; KOTH garrison 50/10 and hill override; `SearchForOfficers` checks (no discrepancy — recorded for traceability) | `code/worldobject/ai.cpp`, `code/taskmanager.cpp` |

<!-- from 20_architecture.md -->
### From 20_architecture.md

<!-- SD -->

<!-- from 23_terrain_and_landscape.md -->
### From 23_terrain_and_landscape.md

| Document / comment | Says | Code does | Ref |
|---|---|---|---|
| `docs/landscape lightmap.txt` | Landscape has `GenerateLightMap`, `ApplyLightmap`, `DeleteLightMap`: ray-cast shadows from a point light at (700, 200, 100), shade 255/50, optional 5 × 5 smoothing, a lightmap texture | none of these functions or members exist; terrain is only GL vertex-lit by two level lights; the only "lightmap" is the water shore mask; only `LIGHTMAP_SCALEFACTOR` survives | §12 |
| `docs/OpenGLState.txt` | fog start 800, end 2000 | fog derived from world size: end = max(4000, size + 1000), start = end/3 | `code/location.cpp:Location::SetupFog`; [24](24_rendering.md) |
| `code/landscape.h` comment on `m_guideGridPower` | "Log to the base 2 of the resolution of the guide grid" | resolution = 2^p − 1, so p = log₂(resolution + 1) | `code/landscape.cpp:LandscapeTile::GuideGridSetPower` |
| `contrib/systemiv/lib/math/random_number.h` comment on `sfrand` | "−range < result < range" | result is in [−range/2, +range/2] (same for `Landscapesfrand`) | `contrib/systemiv/lib/math/random_number.cpp:sfrand` |
| `code/lib/math_utils.h` comment on `Round` | may round up or down depending on the CPU rounding mode | the active macro is `(int)(a)`: always truncates toward zero (the `fistp` version is commented out) | `code/lib/math_utils.h:Round` |
| `code/landscape.h` declaration `GenerateMidpoints(int _x1, int _x2, int _z1, int _z2)` | parameter order x1, x2, z1, z2 | definition and call use (x1, z1, x2, z2); names only, behaviour consistent | `code/landscape.cpp:LandscapeTile::GenerateMidpoints` |
| Editor label `editor_noiseperiod` for `m_fractalDimension`; file header label "frac dim" | a period / a fractal dimension | the value is the exponent of the per-level amplitude law; larger means smoother (§5.4) | `code/interface/landscape_window.cpp:LandscapeTileEditWindow::Create` |
| `code/water.h` header comment | sea plane twice the land size; depth modulates brightness, wave amplitude and colour | matches; additionally deep water is not drawn at all (flat tiles skipped where all depths ≥ 1, wave vertices where depth > 0.999) | §13 |
| Phase-2 P12 render-pass draft (not a docs file; recorded for traceability) | flat-water texture unit 1 is the "landscape lightmap" | it is the water shore mask `"water_lightmap"` | §12 |

<!-- from 24_rendering.md -->
### From 24_rendering.md

| Doc / comment | Says | Code does | Ref |
|---|---|---|---|
| `docs/OpenGLState.txt` | fog start 800, end 2000 | start `fogEnd/3`, end `max(4000, worldSizeX+1000, worldSizeZ+1000)`; `CheckOpenGLState` asserts end ≥ 4000 | `code/location.cpp:Location::SetupFog`, `code/renderer.cpp:Renderer::CheckOpenGLState` |
| `docs/OpenGLState.txt` | fog colour `g_app->m_world->m_backgroundColour` | `g_app->m_backgroundColour` (constant black, alpha 0); the D3D path ignores fog colour entirely | `Location::SetupFog`, `code/lib/opengl_directx.cpp:glFogfv` |
| `docs/OpenGLState.txt` | lights from `g_app->m_world->m_lights[x]` with ambient/diffuse/position/specular | `Location::SetupLights` (or `GlobalWorld::SetupLights`): direction + colour, ambient (0,0,0,1), LIGHT1 zeroed first, only LIGHT0/1 ever enabled | `code/location.cpp:Location::SetupLights` |
| `docs/OpenGLState.txt` | `glTexEnvf(GL_TEXTURE_ENV_MODE, GL_MODULATE)` | `glTexEnvi(…MODULATE)` plus `glTexEnviv(GL_TEXTURE_ENV_COLOR, {0,0,0,0})` | `Renderer::SetOpenGLState` |
| `docs/OpenGLState.txt` | lists `GL_TEXTURE_MAG_FILTER` twice "undefined" (MIN missing) | neither filter is part of the default state; every textured pass sets its own; uploads default LINEAR / LINEAR_MIPMAP_LINEAR | §3.2 |
| `docs/OpenGLState.txt` | no mention of `GL_NORMALIZE`, `GL_LIGHT_MODEL_AMBIENT`, `GL_SCISSOR_TEST`, lights 2–7 | code disables NORMALIZE, sets light-model ambient (0,0,0,0), disables scissor and all 8 lights | `Renderer::SetOpenGLState` |
| `docs/OpenGLState.txt` | trailing `glMaterialfv(GL_FRONT, …)` and `allegro_gl_set_texture_format(GL_INTENSITY8)` | no Allegro anywhere in `code/`; materials set by `Renderer::SetObjectLighting` (specular 0, diffuse 1, ambient 0, shininess 127) | `Renderer::SetObjectLighting` |
| `docs/OpenGLState.txt` (implied contract) | the documented state is the API contract | under [B:D3D] shade model, line/point smooth, fog colour/density, light-model ambient and material state are not emulated | §1.5.4 |
| `docs/HardwareComplianceIssues.txt` | "Multitexturing ARB not supported" on a Linux test machine | no Win32 fallback: `gglActiveTextureARB` / `gglMultiTexCoord2fARB` are called unguarded (flat water, fences, radar dish) | `code/lib/ogl_extensions.cpp`, `code/water.cpp:Water::RenderFlatWater` |
| `docs/HardwareComplianceIssues.txt` | "Mouse pointer background not transparent" | no explicit fix; Win32 cursors are drawn additively by the game (24-U9) | `code/gamecursor.cpp:MouseCursor::Render` |
| `docs/landscape lightmap.txt` | `Landscape::GenerateLightMap/ApplyLightmap/DeleteLightMap`: ray-cast shadows from a point light at (700,200,100), shade 255/50, optional 5×5 filter | none of these exist; terrain is only GL vertex-lit; the only "lightmap" is the water shore mask; orphan `LIGHTMAP_SCALEFACTOR` remains | `code/landscape.h`, `code/water.cpp:Water::GenerateLightMap` |
| `docs/sphere world.txt`, `docs/top level map.txt` | the sphere world is the hub between islands | present and built at start-up but unreachable in Multiwinia; the menu shows only clouds | `code/main.cpp:GlobalWorldGameLoop`, `code/game_menu.cpp:GameMenu::Render` |
| `code/water.h` header comment | sea plane twice the land size; depth darkens and shrinks waves | matches; additionally deep water is not drawn at all | `code/water.cpp` |
| comment in `code/lib/render_utils.h` | split quads are needed for Radeon fog artefacts | `RenderSplitUpQuad*` have no callers (clouds subdivide inline) | `code/lib/render_utils.cpp`, `code/clouds.cpp:Clouds::RenderQuad` |
| comment in `Renderer::ApplyPixelEffect` | pass 4 "Additive smooth" | uses NEAREST (blocky) | `code/renderer.cpp:Renderer::ApplyPixelEffect` |
| comment on `ParticleType::m_spin` | spin in rad/s; fade-in/out times | `m_spin`, `m_fadeInTime`, `m_fadeOutTime` never read; render fade hard-coded | `code/particle_system.cpp` |
| signature `DeformEffect::AddPunch(Vector3 pos, float range)` | takes a range | argument ignored (`m_range = 1`) | `code/deform.cpp:DeformEffect::AddPunch` |
| comment in `WindowManagerWin32::EnableOpenGL` | secondary GL context "for loading thread" | never made current; no loading-thread GL use | `code/lib/window_manager_win32.cpp` |
| comment in `App::UseChristmasMode` | "Also allow user to disable if he wishes" | the `ChristmasEnabled` preference is not consulted | `code/app.cpp:App::UseChristmasMode`, `code/interface/prefs_other_window.cpp` |
| graphics menu (`dialog_landscapedetail`, `dialog_pixeleffect` strings) | landscape detail and pixel effect are user options | both controls are commented out; `RenderLandscapeDetail` has no effect | `code/interface/prefs_graphics_window.cpp` |
| `Renderer::Initialise` reads `WaitVerticalRetrace` | vsync is configurable | first two `CreateWin` attempts pass literal `true`; the GL window manager ignores the argument entirely | `code/renderer.cpp:Renderer::Initialise`, `code/lib/window_manager_win32.cpp:CreateWin` |

<!-- from 27_audio_engine.md -->
### From 27_audio_engine.md

| Doc / comment | Says | Code / data does | Ref |
|---|---|---|---|
| `docs/adsr.gif` | Classic ADSR: Attack → **Decay** → Sustain **level** → Release on key-off | No decay stage (`StateDecay` never entered); SUSTAIN is a hold **duration**; peak = sustain level = VOLUME | `code/sound/sound_instance.cpp:SoundInstance::UpdateChannelVolume` |
| `docs/ThoughtsOnSoundSystem.txt` | 2D buffer fixed at 2000 samples "probably needs to be adjustable"; wants a linear/log toggle on the parameter graph; "Sort out SetChannelVolume / SetChannel3dMode for software mixer" | Buffer is pref `SoundBufferSize` (512 generated / 2000 fallback); the graph draws output on a √ scale but the toggle button is commented out; both software-mixer functions are implemented | `code/sound/sound_library_2d_win32.cpp`; `code/interface/soundparameter_window.cpp:SoundParameterGraph::GetPosition`; `code/sound/sound_library_3d_software.cpp` |
| `docs/ImplementationNotes/SoundSystemSamplePath.jpg` | Ogg **and WAV** files reach the sample cache from both the file system and RAR archives | `Resource::GetSoundStreamDecoder` only appends `.ogg`; WAV decoding is unreachable | `code/lib/resource.cpp:Resource::GetSoundStreamDecoder` |
| `docs/ImplementationNotes/SoundSystemData.jpg` | Sound params include min **and max** distance, pan (non-3D), loop points; properties include angular velocity, health, time; source is a WAV file | No max distance, pan or loop points; link inputs are only Nothing, HeightAboveGround, X/Y/Z position, Velocity, CameraDistance; assets are Ogg | `code/sound/sound_parameter.h`, `code/sound/sound_instance.h` |
| `docs/ImplementationNotes/SoundSystemData2.jpg` | `SoundFilterBlueprint` array indexed 0..12 (13 filters); class names `SoundFilterBlueprint` / `SoundFilterParamBlueprint`, `SoundChannel` | 14 filters (`NUM_FILTERS`, 0..13); classes `DspBlueprint` / `DspParameterBlueprint`; channel table is `SoundInstanceId[]` | `code/sound/sound_library_3d.h`, `code/sound/soundsystem.h` |
| `docs/ImplementationNotes/Sounds_DiskToSpeaker.gif` | One `SoundSystem::SoundLibraryCallback()`; instance holds `SoundFilterHandle *m_filters` | Two callbacks (`SoundLibraryMainCallback`, `SoundLibraryMusicCallback`); filters are `DspHandle`s; otherwise the CachedSample → handle → instance → SoundSystem → DirectSound chain matches | `code/sound/soundsystem.h` |
| `docs/ImplementationNotes/SoundEffectBlueprint2.jpg` | Linked parameter graph flat outside the input range, linear between | Consistent | `code/sound/sound_parameter.cpp:SoundParameter::Recalculate` |
| `docs/MinDistAndVolume.jpg` | Flat to min distance, then fall-off | Consistent (inverse distance) | `code/sound/sound_instance.cpp:SoundInstance::CalculatePerceivedVolume` |
| "sound changes.txt" in `docs/` (Requested section) | Microphone positioning; "drop all volumes to highlight a single sound event" | Not implemented (listener = camera; no ducking or buses) | `code/sound/soundsystem.cpp:SoundSystem::Advance` |
| `code/sound/sample_cache.cpp` comment | "Largest song takes 13 MB" | Largest shipped track decodes to 29.0 MiB (`data/sounds/tresk_-_excuses.ogg`, 344.6 s mono 44.1 kHz) and its whole buffer is allocated on open | `code/sound/sample_cache.cpp:CachedSampleManager::CleanUp` |
| `code/sound/sound_instance.h` comment | VOLUME 0.0 = "silence" | 0.0 = −50 dB on DirectSound (−43.4 dB software); only master 0 + volume 0 approaches the −100 dB floor | `code/sound/sound_library_3d_dsound.cpp:SoundLibrary3dDirectSound::SetChannelVolume` |
| `code/sound/sound_library_3d.h` header comment | "Currently two implementations" | Three plus a null device: DirectSound, software, XAudio (dead on PC), NoSound | `code/sound/` |
| `code/sound/soundsystem.h` comment on `StopAllSounds` | "Full event name required" | Substring match (`strstr`) | `code/sound/soundsystem.cpp:SoundSystem::StopAllSounds` |
| `code/sound/sound_library_3d_software.cpp` comment on `m_forceVolumeJump` | "prevents the volume ramping code from being used when a new sound first starts" | Ramping happens **only** when the flag is set (new sound) | `SoundLibrary3dSoftware::Callback` |
| `contrib/systemiv/lib/math/random_number.h` comment | `frand`: 0 ≤ result < range | Can return exactly `range` (`AppRandom` max 32767 / 32767) | `contrib/systemiv/lib/math/random_number.cpp:frand` |
| `code/interface/debugmenu.cpp` captions | "Sound Stats (F7)", "Sound Editor (F8)", "Sound Profile (F9)" (button text "Sound Profile (F8)") | Hot-keys commented out; buttons never created (`modsEnabled` false) | `code/user_input.cpp:UserInput::Advance` |

<!-- from 28_ui_framework.md -->
### From 28_ui_framework.md

| Doc | Says | Code | Ref |
|---|---|---|---|
| `docs/multiwiniachanges-beta.txt` (Issue 650, listed as fixed) | "Game Menu: X360 B button in menus doesn't always go back" | Structural cases remain where B does nothing: pages without a `BackPageButton` (Main, AuthKeyEntry), any frame in which the `BackPageButton` is not rendered (drop-down list open, window unfocused), and `MessageDialog` (no back button) [C mechanism; I that these were the reported cases] | `code/game_menu.cpp:BackPageButton::Render`, `code/interface/message_dialog.cpp` |
| `docs/multiwiniachanges-beta.txt` (Issue 558, listed as fixed) | "Menu navigation in the pre-game with controller" | consistent: the lobby builds `m_buttonOrder` and starts the cursor on Play | `GameMenuWindow::SetupMultiplayerPage` |
| `docs/detailed schedule.txt` "Translation System → Missed out" (Sound Stats/Parameter/Editor/Profile windows, Profile Window, Pokey Window, Network Window, Input Logger, Grabber Window, Gesture Window, Debug Menu, Cheat Window, Camera Anim Window) | these windows are untranslated | consistent: literal English captions (§10.3) | P17a strings table |
| `docs/RussianFontRequest.txt` | only speccy had Cyrillic glyphs; they were copied to square and editor as a stop-gap | consistent: the three Cyrillic atlas pairs are byte-identical | `data/textures/unicode/*.0410.042f.bmp`, `*.0430.044f.bmp` |
| `docs/sound changes.txt` | "Sound editor will now use a scrollbar if there are too many events to fit" | consistent (`SoundEditorWindow` creates a `ScrollBar`), but the sound editor is [B:DEBUG] only | `code/interface/soundeditor_window.cpp` |

<!-- from 31_platform_build_and_tools.md -->
### From 31_platform_build_and_tools.md

| Doc / artefact | Claim | Code reality |
|---|---|---|
| `README.TXT` | "There are four possible build configurations" — then lists six, including "PC OpenGL (Release) produces darwinia_gl.exe" and "PC OpenGL (ReleaseDirect3D) produces darwinia.exe" | the vcproj has four configurations; the solution adds `PC OpenGL (Debug)`/`(Release)` solution configurations mapped to `Debug`/`Release`; no "PC OpenGL (ReleaseDirect3D)" exists [C] `targets/vs2005/Darwinia.sln` |
| `README.TXT` | "Debug and Release build Darwinia…" | the product built is Multiwinia (`APP_NAME`) |
| `tools/readme-linux.txt` | preferences in `~/.darwinia/demo` or `~/.darwinia/full` | Multiwinia Linux path is `~/.multiwinia/<DARWINIA_GAMETYPE>/` [C] `code/app.cpp:App::GetProfileDirectory` |
| Installers' `[UninstallDelete]`, `MultiwiniaSupport.ini` | user files live in `{app}` | written to `%LOCALAPPDATA%\Introversion\Multiwinia\` on Windows 2000+ unless `--NoProfile` [C] |
| Installers, `MultiwiniaSupport.ini` | files `userprefs.txt`, `game-history.txt`, `stdout.txt`; key file `authkey` | no code in `code/` or `contrib/systemiv/lib` writes the first three; the game reads `authkey.dev` [C grep] |
| Support-tool success message | "This reference is also recorded in errorlog.txt in your Multiwinia directory" | `ERRORLOG.TXT` is written to the profile directory, not the install directory [C] |
| `docs/vista installer check list.txt` | `TARGET_VISTA`, `ReleaseD3D`, `DarwiniaSupport.ini/.exe` | `TARGET_OS_VISTA`, `ReleaseDirect3D`, `MultiwiniaSupport.*` |
| `docs/RussianFontRequest.txt` | only Speccy has Cyrillic glyphs | `data/textures/unicode/` has Cyrillic pages for `editor`, `speccy` and `square`; but the all-languages `tools/language.list` packs only the `speccy` ones (§12.2) |
| `tools/language/README.txt` | translation inputs must be UCS-2 BE | `data/language/japanese.txt` and `russian.txt` are UTF-16 LE; the game accepts both, `report.pl` only BE |
| `tools/FindUnusedResources.sh` | must run from a folder named `Darwinia` | repository folder is `multiwinia` |
| `docs/cpu cycles.txt` | (requested by the specification as performance notes) | a gameplay design note (task manager/gestures); no performance data |
| `docs/Network.txt` | server ticks at 25 FPS | 10 Hz in code ([22](22_determinism_and_networking.md)) |
| `targets/vs2005/GDF/*.gdf.xml` (compiled into the exe) | "Darwinia" 1.5.4.0, `darwinia.exe`, saved games `Darwinia\users` | the product is Multiwinia / `multiwinia.exe`; Game Explorer metadata is stale |
| `targets/vs2005/version.rc` vs installers vs `DARWINIA_VERSION` | 6.0.0.0 vs 1.0.x–1.9.x vs "dev" | no single version source (§9.4) |
| `code/lib/window_manager_win32.cpp` comment | secondary GL context "for loading thread" | never made current; worker GL work is marshalled to the main thread |
| `code/app.cpp` comments | affinity `0x02|0x8|0x20|0x80` "Even processors", `0x01|0x4|0x10|0x40` "Processor Number % 3 == 2" / "Odd processors" | the masks select CPUs 1,3,5,7 and 0,2,4,6 (0-based) respectively |
| `tools/build_multiwinia_data.bat` echoes | sounds2 step prints "Making sounds.dat"; `ambx` platform prints "…hwcompat.txt" | builds `sounds2.dat`; appends the beta-test exclusion list |
| `targets/vs2005/Common.vsprops` | include path `contrib/steam/` | directory absent from the tree |
| `_work/build_matrix.md` §3 (earlier phase note) | target table complete | omitted `LAN/WAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE`, `TARGET_BETATEST_GROUP`; corrected in §7.6 |
