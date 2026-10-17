# Collected "discrepancies" sections from all drafts

<!-- from P01_app_core -->
## Docs-vs-code discrepancies

| Doc | Claim | Code | Ref |
|---|---|---|---|
| `docs/Network.txt` | "Server ticks at 25 FPS", client "runs heavy weight physics at 25 fps", server sleeps until next 1/25 s | server tick every 0.1 s (10 Hz) of real time, no sleep; client spreads each tick over 10 slices consumed at ~30 % of backlog per frame | `code/network/server.cpp:AdvanceIfNecessary`, `code/network/clienttoserver.cpp:GetNumSlicesToAdvance` |
| `docs/NetworkThreads.txt` | dedicated server/client **send** threads that suspend when idle | sends happen on the main thread: `AdvanceSender` (outbox drained under mutex) is called at the end of `ClientToServer::Advance` and inside `Server::Advance`; only receive/listen paths are threads [C] | `code/network/clienttoserver.cpp:ClientToServer::Advance`, `code/network/server.cpp:Server::Advance` |
| `docs/NetworkThreads.txt` | main thread runs "client heavy weight advance" only when `time >= nextClientTimeSlice`, light-weight advance "if there is time" | heavy-weight work is sliced (10 slices/tick) and interleaved with every rendered frame; light-weight prediction is the render-time extrapolation `pos + vel × g_predictionTime` | §2.5 |
| `docs/network overview.txt` | lobby over TCP, then UDP ports 4000/4001 | not verified here (P15) | – |
| `code/globals.h` comment | `MINIMUM_RENDER_PERIOD` "Render at least this frequently" | macro unused | grep |

<!-- from P02_game_modes -->
## Docs-vs-code discrepancies

| Doc statement (docs/multiwinia_manual.pdf, pp.10–19) | Code behaviour | Ref |
|---|---|---|
| Assault: "Play then switches around and the attacker becomes the defender … winner is the player who successfully attacks in the quickest time" | Default single round, no side switch; winner = attackers if the bomb was destroyed else defenders. Rotation exists but is unreachable | `AdvanceAssault`, §2.5 |
| Assault: "sabotage the three control stations" | 3–4 linked stations per map | level files |
| Blitzkrieg: "When a player loses their home flag, they are eliminated" | Also requires the capturer to hold an unbroken, not-being-recaptured chain of its flags back to its own home flag | `DoesBlitzkriegRouteExist` |
| CTS: a shattered statue "re-appears randomly in one of the Statue Zones" | Respawn after a randomised ≥ 10-mode-second timer at the neutral zone farthest from existing statues (random only when none exist) | `AdvanceCaptureStatue` |
| Rocket Riot: explosion burns "all the fuel and all the Multiwinians inside, re-setting the rocket to its starting point" | MP: fuel burns down to 50% of its pre-explosion value; passengers are ejected (50% on fire, the rest survive); rocket returns to Refueling | `EscapeRocket::AdvanceExploding` |
| Handicap: "Domination, King of the Hill" | Also an Assault option | `data/multiwinia.txt` |
| Co-op "(4 players required) – Domination and King of the Hill" | Any map flagged `CoopMode`/`ForceCoop` (incl. 3P Assault maps, CTS 4P_2, TankBattle 4P) | level files |
| KOTH "scoring one point per second for every zone under their control" | Correct for ScoreMode 0; 3 more modes exist; a "second" is 1.1 game s | §4.2, D1 |
| Option descriptions (english.txt) — Crate drop "50% deviation" | Matches `1.5 − U(0,1)` | `Location::AdvanceCrates` |

<!-- from P03_entity_core -->
## Docs-vs-code discrepancies
| Doc | Claim | Code |
|---|---|---|
| `docs/spirit resource system.txt` | Spirits "gradually float away over the course of a minute"; garbage-collector units and factories that need spirits | MP: 6 s birth + 30–50 s floating + 90 s ascent; SP 6 + 90–150 + 180 s (`Spirit::Advance`). Only Harvesters (crate) and legacy Engineers/Incubators consume souls; no "garbage collector" unit [C] |
| `docs/multiwinia_manual.pdf` (Officers) | "Right click on officer: set surrounding Multiwinians into formation" | formation toggles when the order point is < 50 from the officer and no radar-dish entrance is within √5 (`Officer::IsFormationToggle`) — consistent; "surrounding" Darwinians join only if they are FOLLOW-searching within 150 and the formation (90 max) isn't full [C] |
| manual (tips) | "Multiwinians in Formation are very vulnerable from behind" | consistent: formation threat search is centred 0.66 × range **ahead** of the officer and members hold position (`SearchForThreats`) [C] |
| `docs/changes.txt` | "Un-occupied Battle Cannons will no longer scare Darwinians" | `EffectGunTurretTarget` threats are only considered in single-player and for other teams; in MP they are ignored entirely [C] |
| `docs/multiwinia-changes.txt` | "Fixed: Sync error involving Officers sending Darwinians into a radar dish" | current code path: `SearchForOfficers` copies `m_ordersBuildingId`; no residual issue identified [I] |

<!-- from P04_entities_military -->
## Docs-vs-code discrepancies
| Doc | Claim | Code | Ref |
|---|---|---|---|
| `contrib/systemiv/lib/math/random_number.h` comments | `sfrand`/`_syncsfrand`: "-range < result < range" | actual range (−range/2, +range/2) | [C] `random_number.cpp:sfrand/_syncsfrand` |
| `data/stats.txt` header | RATE = "secs to reload" | fractional rates truncate to whole seconds per entity (`unsigned char`); speeds scaled by difficulty | [C] `entity.h`, `EntityBlueprint::GetStat` |
| `docs/multiwinia_manual.pdf` (Armour) | "transport up to 100 Multiwinians … can go across water"; "Armours are vulnerable to attack" | capacity 100 ✔, hovers at max(terrain,0) ✔; **immune to lasers**, shells ÷3 (manual silent) | [C] `Armour::Capacity`, `ChangeHealth` |
| manual (Squads) | "deadly units, but only as long as you control them"; grenade = hold right + left click | autonomous fire is probabilistic `min(1,4/d)`/tick; `secondaryFire = primaryFireTarget && secondaryFireTarget` ✔ | [C] `Squadie::RunAI`, `code/location.cpp` |
| `docs/multiwiniachanges-beta.txt` | "Darwinians surviving armour explosions (Issue 710)" listed as an issue | passengers are now ejected in `StateOnFire` | [C] `Darwinian::AdvanceInsideArmour` |
| `docs/multiwinia-changes.txt` | "Napalm Strikes will now set fire to trees much more reliably" | trees/triffids within 100 take flat −200 with no falloff | [C] `FlameGrenade::Advance` |
| Code comment in `Location::FireLaser` | "subversion lasers are half speed" | speed is the caller's: Subversion Darwinians fire at 75 (half the generic 150) ✔, but the Subversion gun turret fires at shell velocity / 4 | [C] `code/worldobject/darwinian.cpp:Darwinian::FireSubversion`, `code/worldobject/gunturret.cpp` |

<!-- from P05_entities_creatures -->
## Docs-vs-code discrepancies
- `docs/multiwinia_todo.txt` (L74–76) suggests creatures such as Triffids should belong to "a generic non-player team… reserve team 0". Code uses a dedicated **Monster team = 4** (and Futurewinian team = 5), created lazily by `Location::InitMonsterTeam`. [C] `code/location.cpp:Location::GetMonsterTeamId`.
- `docs/sound changes.txt` lists CratePowerup events including `PersonalShield` and `Infection`; code names the Infection reward `CrateRewardSpam` whose sound event name is produced by `Crate::GetName` (P08 to reconcile names). The `Meteor: Land` event in the same doc matches `Meteor::Advance`. [C]
- `docs/bad guys.txt` is a brainstorm (egg fields, mother wasps, flocking); the implemented egg cycle is SporeGenerator/Spider → Egg → (spirit) → Virii, and Triffid → TriffidEgg → creature. No other docs describe these creatures. [C]

<!-- from P06_buildings_core -->
## Docs-vs-code discrepancies

| Doc | Claim | Code reality | Ref |
|---|---|---|---|
| `docs/spirit resource system.txt` (Darwinia design note) | spirits of dead enemies are collected (garbage collectors) and used by factories to spawn units | Multiwinia spawn points use *virtual* master-emitted spirits; collected-spirit production exists only in legacy Factory/Incubator code | [C] `spawnpoint.cpp`, `factory.cpp`, `incubator.cpp` |
| `docs/multiwiniachanges-beta.txt` issue 756 | "Gun turret/Incubators emplacements shared by friendly teams don't fire/spawns" (bug report) | GunTurret ownership now explicitly allows friendly (co-op) co-occupation (`IsFriend` check); turret fires for the winning ally | [C] `GunTurret::RecalculateOwnership` |
| `docs/multiwinia-changes.txt` | "Virii no longer count towards your population limit" | consistent: `currentPop` subtracts `Virii::s_viriiPopulation` | [C] `SpawnPoint::PopulationLocked` |
| Source comment in `building.h` ("When you add an entry here remember to update building.cpp") | name table must match enum | consistent (75 names for 75 values) | [C] |

<!-- from P07_buildings_misc -->
## Docs-vs-code discrepancies
| Doc | Claim | Code reality |
|---|---|---|
| `docs/game objects.txt` "Tree / forest" | "Acts as an obstacle to anything in the air, but can be walked through" | Ground units are pushed out of the 10-unit trunk sphere and crown sphere (`Darwinian::PushFromObstructions` generic branch, `Entity::PushFromObstructions`) — trees block walking [C]. |
| `docs/game objects.txt` "Tree / forest" | "? Spawn seeds which sprout into new trees ?" (speculative) | Implemented as `m_spawnsRemaining`/`CreateAnotherTree`, but only crate-created trees (and offspring) have a non-zero budget; level trees never spread [C]. |
| `docs/game objects.txt` "Laser Fence" | "Can only be shut down by disconnecting / destroying power source" | In MW a fence's mode is fixed by the level unless it is solar-linked (powered fences drop 5 s after the last surge); fences are indestructible [C]. |
| `docs/game objects.txt` "Wall" | "Can be blown up with enough force" | Matches: single hit ≤ −300 [C]. |
| `docs/new building docs.txt` FenceSwitch | Mentions "Pylon 1 / Pylon 2" values for passing on power surges | `FenceSwitch` has no pylon fields or surge handling in this tree (`switch.cpp` reads 2 fence links, switchValue, lockable, script, locked) [C]. |
| `contrib/systemiv/lib/math/random_number.h` comment | `sfrand`: "-range < result < range" | Returns [−range/2, range/2] [C]. |

<!-- from P08_crates_ai -->
## Docs-vs-code discrepancies
- `docs/integrating virtual players into network system.txt` suggests a client may host "an AI running locally"; in code the AI is a simulation entity executed identically on all peers, with no input messages [C] `code/team.cpp`, `code/location.cpp:Location::InitialiseTeam`.
- `docs/multiwinia_todo.txt` lists "Creating a CPU player should automatically spawn an AI entity" as outstanding; implemented in `Location::InitialiseTeam` [C].
- `docs/multiwiniachanges-beta.txt` mentions crate/powerup placement permitted near trunk ports in Assault (Issue 754) and "Impossible powerups/crate placement when Initial Powerups is higher than 0" (Issue 300) — consistent with `IsValidTargetArea` restrictions and the 30 s `m_startTimer` lock [I].
- In-code comments vs behaviour: see Contradictions #2/#3.

<!-- from P09_control_taskmanager -->
## Docs-vs-code discrepancies

- `docs/betachanges.txt` lists fixes for "Gestures Mode", "Icons task manager used when use Xbox 360 controller" and Tab in Icons mode — these concern the Darwinia(+) SP interfaces; code confirms both SP variants still exist but are unreachable in Multiwinia (FULLGAME has no Prologue/Campaign entry) [C]. No Multiwinia-specific task-manager documentation found in `docs/` (grep for "task manager", "gesture").
- `docs/game objects.txt` suggests power-ups "triggered by a gesture of some kind"; in Multiwinia power-ups are placed by click, never by gesture [C] `TaskManagerInterface::CreateTaskManager`.

<!-- from P10_input_camera -->
## Docs-vs-code discrepancies

| Doc | Claim | Code reality |
|---|---|---|
| `docs/input_preferences_syntax.txt` | Grammar lists no `repeat`, `~` icon lines, `alias`, `value`, `->` pipe, `mouse any`, `key any` | `repeat` exists for XInput BOOL controls; `~` sets icons; `alias`/`value` drivers exist; pipe exists but is unregistered; `any` parses but never fires [C] |
| same | `XInput <xinput_analog> <analog_cond> [with sensitivity <1-200>]` | sensitivity is also accepted after BOOL conditions on analog controls (e.g. `leftthumbstick pressed with sensitivity 50`) and is rejected on directional stick buttons [C] `XInputDriver::writeExtraSpecInfo` |
| same | "The user input preferences will override the FIRST binding" | true, and the keyboard-layout file does the same; repeated lines in one override file overwrite each other [C] |
| same | unknown/invalid specs implied to fail | unknown condition words are silently accepted as `down` [C] §1.3 |
| `docs/DarwiniaInput.rtf` | Action words include `moved` | the word is `move`; `moved` would hit the fallback and act as `down` [C] |
| same | `lib/input/transform.h` holds `Movement2D` | it is in `lib/input/movement2d.h` [C] |
| same | `g_controlBindings->ControlEvent(ControlBindings::ControlDeselect)` (old API) | current API is `g_inputManager->controlEvent(ControlUnitDeselect)`; no `ControlDeselect` exists [C] |
| `code/camera.h` comment | "Remember to update the static string table at the end of camera.cpp" | no mode-name table exists [C] |
| `code/camera.cpp` comment on `DistanceToBlockage` / `DirectDistanceToBlockage` | "If there is no blockage FLT_MAX is returned" | returns `_maxDist`; FLT_MAX only when there is no location [C] |
| `code/lib/input/keynames.cpp` comment | `"QUOTE", //192` | QUOTE is index 222; MINUS_PAD/Unknown swapped vs `keydefs.h` [C] |
| `code/lib/input/control_types.inc` comments | `CameraRotate` "rotate the cam in entity track mode"; `WeaponCycle*` "cycle weapons" | no consumer in the FULL build [C] |
| `code/lib/input/control_bindings.cpp` comment | ControlBindings::replacePrimaryBinding | commented out; replace is a flag on `bind` [C] |

<!-- from P11_terrain_world -->
## Docs-vs-code discrepancies

| Doc | Says | Code | 
|---|---|---|
| `docs/landscape lightmap.txt` | Landscape has `GenerateLightMap/ApplyLightmap/DeleteLightMap`: ray-cast shadows from a point light at (700,200,100), shade 255/50, optional 5×5 smoothing, texture | No such functions; terrain is only GL vertex-lit; the sole "lightmap" is the water shore mask (`Water::GenerateLightMap`). Only `LIGHTMAP_SCALEFACTOR` survives |
| `docs/OpenGLState.txt` | Fog start 800, end 2000, colour = world background | `Location::SetupFog`: start = end/3, end = max(4000, size+1000) |
| `docs/sphere world.txt`, `docs/top level map.txt` | Top-level sphere map is the hub between islands | Present in code but unreachable in Multiwinia (`GlobalWorldGameLoop` exits immediately); menu shows only clouds |
| `code/water.h` header comment | sea plane twice the land size, depth darkens and shrinks waves | Matches; plus deep water is not drawn at all (flat tiles and dynamic verts culled where depth ≥ 1 / > 0.999) |
| `code/level_file.cpp:ParseLights` comment | mission-file lights take precedence over map lights | Mission files cannot contain `Lights` (asserted and skipped) |

<!-- from P12_render_core -->
## Docs-vs-code discrepancies
| Doc | Doc says | Code does | Ref |
|---|---|---|---|
| `docs/OpenGLState.txt` | fog start 800, end 2000 | start `fogEnd/3`, end `max(4000, worldSize+1000)` | `Location::SetupFog` |
| `docs/OpenGLState.txt` | lights from `g_app->m_world->m_lights` (ambient/diffuse/position/specular per light) | `Location::m_lights` direction + colour, ambient forced black, LIGHT1 zeroed first, only 2 lights used | `Location::SetupLights` |
| `docs/OpenGLState.txt` | mag/min filter undefined | every textured pass sets its filters explicitly; upload default LINEAR / LINEAR_MIPMAP_LINEAR | passes in 24.2 |
| `docs/HardwareComplianceIssues.txt` | ARB multitexture unsupported on a Linux test machine | flat water requires `gglActiveTextureARB` multitexture with no fallback in the paths read | `code/water.cpp:Water::RenderFlatWater` (P11/P13) |
| comment `render_utils.h` | split quads needed for Radeon fog artefacts | functions have no callers | `code/lib/render_utils.cpp` |
| comment in `ApplyPixelEffect` | pass 4 "Additive smooth" | uses NEAREST (blocky) | `Renderer::ApplyPixelEffect` |
| method name `ApplyDilateFilter` | dilate | 8-neighbour mean (box blur without centre) | `code/lib/bitmap.cpp` |
| comment `ParticleType::m_spin` | spin rad/s; fade-in/out times | never read | `code/particle_system.cpp` |
| signature `deform.h` | `AddPunch(Vector3 pos, float range)` takes a range | the `range` argument is ignored (`m_range = 1`) | `DeformEffect::AddPunch` |

<!-- from P13_gfx_backend_math -->
## Docs-vs-code discrepancies

| Doc statement | Code reality | Ref |
|---|---|---|
| `docs/OpenGLState.txt`: fog `GL_FOG_END 2000`, `GL_FOG_START 800` | `Location::SetupFog`: `fogEnd = max(4000, worldSizeX+1000, worldSizeZ+1000)`, `start = fogEnd/3`; `CheckOpenGLState` asserts `FOG_END ≥ 4000` | `code/location.cpp:Location::SetupFog`, `code/renderer.cpp:Renderer::CheckOpenGLState` |
| fog colour = `g_app->m_world->m_backgroundColour` | `g_app->m_backgroundColour` with alpha 0; and the D3D path ignores fog colour entirely (fades to black) | `Location::SetupFog`, `glFogfv` |
| lights from `g_app->m_world->m_lights[x]` | `m_location->SetupLights()` or `m_globalWorld->SetupLights()`; `m_world` is a Darwinia-era name | `Renderer::SetOpenGLState` |
| `glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE)` | `glTexEnvi(…MODULATE)` plus `glTexEnviv(GL_TEXTURE_ENV_COLOR, {0,0,0,0})` | `Renderer::SetOpenGLState` |
| lists `GL_TEXTURE_MAG_FILTER` twice (MIN filter missing) | neither filter is part of the default state | `SetOpenGLState` |
| no mention of `GL_NORMALIZE`, `GL_LIGHT_MODEL_AMBIENT`, `GL_SCISSOR_TEST`, lights 2–7 | code disables NORMALIZE, sets light-model ambient {0,0,0,0}, disables scissor and all 8 lights | `SetOpenGLState` |
| trailing `glMaterialfv(GL_FRONT, GL_AMBIENT/DIFFUSE/SHININESS/SPECULAR…)` and `allegro_gl_set_texture_format(GL_INTENSITY8)` | no Allegro anywhere in `code/` [C grep]; materials set by `Renderer::SetObjectLighting` (specular 0, diffuse 1, shininess 127, ambient 0); under D3D `GL_AMBIENT` material is ignored and materials never reach shaders | `Renderer::SetObjectLighting`, `glMaterialfv` |
| (implied) the documented state is the API contract | under D3D, shade model, line/point smooth, fog colour/density, light-model ambient and material state are not emulated, so the contract holds only for the GL baseline | emulator tables |
| `docs/HardwareComplianceIssues.txt`: "Multitexturing ARB not supported" (Linux) | no Win32 fallback: multitexture function pointers are called unguarded; Linux build is dead | `ogl_extensions.cpp`, consumers |
| `docs/HardwareComplianceIssues.txt`: "Mouse pointer background not transparent" | no code in this partition addresses it (see U9) | — |
| Code comment `vector3.h`: `==`/`!=` "Uses FLT_EPSILON" | uses `NearlyEquals` (1e-6 absolute) | `Vector3::Compare` |
| Code comment `math_utils.h`: `Round` "may round up or down, depending on the current CPU rounding mode" | macro is `(int)(a)` = truncation (the rounding-mode-dependent asm is commented out) | `math_utils.h` |
| `random_number.h` comment: `_syncsfrand` "-range < result < range" | returns `(syncfrand(1) − 0.5)·range` ∈ [−range/2, range/2) | `random_number.cpp:_syncsfrand` |
| `Matrix33/34::DecomposeToYDR()` comment "x = dive, y = yaw, z = roll" | returns (yaw, dive, roll) | matrix files |
| `window_manager_win32.cpp` comment: secondary GL context "for loading thread" | never made current; no loading-thread GL use | `code/lib/window_manager_win32.cpp:WindowManagerWin32::EnableOpenGL` |

<!-- from P14_platform_lib -->
## Docs-vs-code discrepancies

| Doc | Claim | Code reality |
|---|---|---|
| `README.TXT` | 6 configurations incl. "PC OpenGL (Release) produces darwinia_gl.exe" and "PC OpenGL (ReleaseDirect3D) produces darwinia.exe" | The vcproj has 4 configs; the sln adds `PC OpenGL (Debug)`/`(Release)` solution configs mapped to `Debug`/`Release`; no "PC OpenGL (ReleaseDirect3D)" [C] `targets/vs2005/Darwinia.sln` |
| `README.TXT` | "Debug and Release build Darwinia to use the OpenGL engine" | correct, but the product is Multiwinia (`APP_NAME`) |
| `tools/readme-linux.txt` | prefs in `~/.darwinia/demo` or `~/.darwinia/full` | Multiwinia Linux path is `~/.multiwinia/<DARWINIA_GAMETYPE>/` [C] `code/app.cpp:GetProfileDirectory` |
| Installers `[UninstallDelete]`, `MultiwiniaSupport.ini` | user files in `{app}` | written to `%LOCALAPPDATA%\Introversion\Multiwinia\` on Win2000+ unless `--NoProfile` [C] `code/app.cpp:GetProfileDirectory` |
| `MultiwiniaSupport.ini`, installers | files `userprefs.txt`, `game-history.txt`, `stdout.txt` | no code in `code/` or `contrib/systemiv/lib` writes these names [C grep] (likely autopatcher/old versions) |
| `docs/vista installer check list.txt` | `TARGET_VISTA`, `ReleaseD3D`, `DarwiniaSupport.ini/.exe` | `TARGET_OS_VISTA`, `ReleaseDirect3D`, `MultiwiniaSupport.*` |
| `docs/RussianFontRequest.txt` | only Speccy has Cyrillic | Cyrillic pages exist for editor, speccy, square in `data/textures/unicode/` |
| `tools/language/README.txt` | inputs must be UCS-2 BE | `data/language/japanese.txt` and `russian.txt` are UTF-16 LE; the game accepts both, report.pl only BE |
| `contrib/systemiv/lib/math/random_number.h` comments | `AppRandom` < APP_RAND_MAX; `sfrand`, `syncsfrand` in (−range, range) | `AppRandom` ≤ 32767 inclusive; `sfrand`/`syncsfrand` ∈ [−range/2, range/2] (§22.R.1) |
| `contrib/systemiv/lib/filesys/text_stream_readers.cpp` comment | "Returns 0 on EOF, 1 otherwise" | `TextDataReader` returns false on the last line when it lacks a newline |
| `tools/FindUnusedResources.sh` | must run from `Darwinia` folder | repo folder is `multiwinia` |
| `docs/cpu cycles.txt` | (requested as performance notes) | it is a gameplay design document (task manager/gestures), no performance data |
| `docs/Network.txt` | server ticks at 25 FPS | not verified in P14 (see P15: `code/gametimer.h`) |
| `targets/vs2005/GDF/*.gdf.xml` (compiled into exe) | "Darwinia" 1.5.4.0, `darwinia.exe`, saved games `Darwinia\users` | product is Multiwinia `multiwinia.exe`; Game Explorer metadata stale |

<!-- from P15_network -->
## Docs-vs-code discrepancies
| Doc | Doc says | Code does |
|---|---|---|
| `docs/Network.txt` | Server ticks at **25 FPS** | 10 Hz (0.1 s) [C] `Server::AdvanceIfNecessary` |
| `docs/Network.txt` | each chunk also sends copies of the previous two frames | at most one piggy-backed copy (oldest unacked) + up to 3 letters per tick [C] `UpdateClients` |
| `docs/Network.txt` | watchdog: client dead after 1.0 s silence | 20.0 s [C] |
| `docs/Network.txt` | client sends a "ResyncMe" message on gaps; I'm-alive only when idle | no resync message — cumulative ack `LASTSEQID` on every letter; ALIVE always every 0.1 s in game [C] |
| `docs/Network.txt` | heavy physics at 25 fps on a separate thread | 10 frames/s × 10 slices on the main thread [C] |
| `docs/NetworkThreads.txt` | separate client/server send threads | sends happen synchronously on the main thread [C] |
| `docs/NetworkThreads.txt` | "light weight advance" extrapolates positions from last received velocity | there is no state reception; rendering interpolates locally via `g_predictionTime` (P12) [C for no state; I for rendering] |
| `docs/network overview.txt` | lobby over **TCP**, broadcast "Are you a server?" query, LOAD MAP + confirm handshake, then UDP 4000/4001 | everything is UDP in one channel; servers broadcast REGISTER every 5 s (clients never query); START_GAME is just a sequenced command — no confirm; clients catch up after loading [C] |
| `docs/network overview.txt` | ports 4000 server / 4001 client | matches defaults [C] |
| `docs/integrating virtual players…` | `ServerToClient` index = team id; proposes separate team and client arrays | implemented as proposed (`m_teams` + `m_clients`) [C] |

<!-- from P16_sound -->
## Docs-vs-code discrepancies

| Doc | Doc says | Code/data does | Ref |
|---|---|---|---|
| `docs/adsr.gif` | Classic ADSR: Attack → **Decay** → Sustain **level** → Release on key-off | No decay stage (`StateDecay` never entered); SUSTAIN is a hold **duration**; peak = sustain level = VOLUME | `sound_instance.cpp:UpdateChannelVolume` |
| `docs/ThoughtsOnSoundSystem.txt` | 2D buffer fixed at 2000 samples "probably needs to be adjustable"; wants linear/log toggle on the parameter graph; "Sort out SetChannelVolume / SetChannel3dMode for software mixer" | Buffer is pref `SoundBufferSize`; graph has a sqrt ("log") mode but the toggle button is commented out; both software-mixer functions implemented | `sound_library_2d_win32.cpp`; `soundparameter_window.cpp:SoundParameterGraph`; `sound_library_3d_software.cpp` |
| `docs/sound changes.txt` (28 Aug 2007) | New CRATE event `Expired` | Code fires it; data has no block → silent | `data/sounds.txt` Crate block |
| `docs/sound changes.txt` (19 Sep 2007) | CARRYABLE BUILDING events `Lift`, `Carry (loop)`, `Drop` | Code fires Lift/Carry/Drop; data has only `Statue Drop` | same |
| `docs/sound changes.txt` (28 Aug 2007) | CratePowerup event list includes Infection etc. | Data adds FireExtinguisher/FormationBooster/Tank (never fired) and lacks Engineer, RocketTurret, NapalmStrike, SlowDown, SpeedUp, TheBeast | §10.11 |
| `docs/sound changes.txt` (Requested) | Microphone position, "drop all volumes to highlight a single sound" | Not implemented | — |
| `docs/multiwinia audio.docx` | Game Over / You've won / You've lost sounds; Age-of-object parameter link | Events fired but no data; link not implemented | §10.10, §10.5 |
| `code/sound/sample_cache.cpp` comment | "Largest song takes 13 MB" | Largest shipped track decodes to 29.0 MiB (`tresk_-_excuses.ogg`, 344.6 s mono 44.1 kHz); whole PCM buffer is allocated on open | header scan of `data/sounds/*.ogg` |
| `code/sound/sound_instance.h` comment | VOLUME 0.0 = "silence" | 0.0 = −50 dB on DirectSound (−43 dB software); only master 0 + vol 0 reaches the −100 dB floor | `SetChannelVolume` |
| `code/sound/sound_library_3d.h` comment | "Currently two implementations" | Three (+ null): DirectSound, software, XAudio | `code/sound/` |
| `contrib/systemiv/lib/math/random_number.h` comment | `frand`: 0 ≤ result < range | can return exactly `range` (AppRandom max 32767 / 32767) | `random_number.cpp:frand` |
| `docs/MinDistAndVolume.jpg` | Flat to min distance, then fall-off | Consistent (inverse distance) | §10.6 |

<!-- from P17a_ui_framework_editor -->
## Docs-vs-code discrepancies

| Doc | Says | Code | Ref |
|---|---|---|---|
| `docs/changes.txt` "Editor changes" | moving a land tile moves its buildings; option "in the land tiles menu" | behaviour confirmed (`MoveBuildingsInTile`), but the toggle is in the Landscape window, not the per-tile window | `LocationEditor::AdvanceModeLandTile`, `LandscapeEditWindow::Create` |
| `docs/detailed schedule.txt` "Translation System" → "Missed out:" Sound Stats Window, Sound Parameter Window, Sound Editor Window, Sound Profile Window, Profile Window, Pokey Window, Network Window, Input Logger, Grabber Window, Gesture Window, Debug Menu, Cheat Window, Camera Anim Window | these windows are untranslated | consistent: they use literal English captions | §D3–D7, §E4.10 |
| `docs/donelist.txt` "made it possible to add new gestures from editor" | gesture editor exists | present but compiled out (`USE_SEPULVEDA_HELP_TUTORIAL`) | §D7 |
| `docs/new building docs.txt` (FenceSwitch "Linking from the editor occurs in the following order: first set of fences, second set") | link order matters | editor link tool calls `Building::SetBuildingLink(id)` per click; ordering semantics live in the building (P07) | `AdvanceModeBuilding` |
| DebugMenu captions "(F2)…(F9)" | hot-keys | no such bindings; F-keys are task-manager keys in `data/input/input_preferences.txt` | §D3 |

<!-- from P17b_menus_options -->
## Docs-vs-code discrepancies

| Doc | Says | Code |
|---|---|---|
| `docs/multiwinia-changes.txt` | "Port forwarding configuration in Help and Options \| Settings \| Other Options." | Matches [C] (`PrefsOtherWindow`) — no discrepancy |
| `docs/multiwiniachanges-beta.txt` | "Game Menu: Leaderboards and Achievements appear in the in-game menu (Issue 412)" (listed as fixed) | The current in-game `LocationWindow` has neither; the only Leaderboards/Achievements buttons are in the unreachable Darwinia `MainMenuWindow` |
| `docs/todo.txt` | "Water reflection linked to pixel effect option" | True only under D3D (`Water::Render` combines `RenderWaterEffect` and `RenderPixelShader`); the pixel-effect control itself is commented out |
| `docs/todo.txt` | "Leaderboards — DONE (?) : Show ranked game leaderboard(s)?" | No leaderboard backend; UI unreachable stub |
| `docs/todo.txt` | keybindings-window issues | The Control Options window is unreachable in the shipped build |

<!-- from P18_ui_gamemenu -->
## Docs-vs-code discrepancies
- `docs/multiwinia-changes.txt` (v1.1.0) "Server filter settings are now saved": code saves the four visible checkboxes (plus the dead custom-maps flag) but **not** the game-type filter drop-down [C] `SaveFiltersButton::MouseUp`.
- Same file: "Server hosts may now password their servers": hosting works, but the browser's "show passworded" filter does not detect passworded servers (contradiction 2).
- `docs/multiwiniachanges-beta.txt` issue 264 text "Pressing back on the map selection screen takes you into the game lobby. Pressing back on that takes you back to game mode": in code MapSelect Back → GameSelect; lobby Back → GameSelect (host), MapSelect (single player), NewOrJoin (client) [C].
- `docs/multiwiniachanges-beta.txt` issue 412 "Leaderboards and Achievements appear in the in-game menu": the front-end Achievements/Leaderboard pages exist in `GameMenuWindow` but are unreachable/empty; the in-game buttons (P17b) are inactive or handler-less [C].
- `docs/multiwinia-changes.txt` v1.2.0 demo LAN rule ("join any LAN game on ANY map if hosted by a full-game player") is not implemented in P18 code; the client only reacts to the `demo` (demo-restricted) flag supplied by server/metaserver [I].

<!-- from P19_help_tutorial_script -->
## Docs-vs-code discrepancies
- `docs/help system.txt` describes a Sepulveda context-help design with F1 to request help, "fixed messages" (BuildingCaptured, BuildingOnline, NewGestureAvailable, objective complete…) and "failure help" (SquadCreationOutOfBounds…), and action items GestureBasics, ThrowAirstrike, FireRocket, Persuadatron, EngineerBuildBridge, EngineerHarvestSpirits, EngineerCaptureBuilding, AimRadarDish, PopulateWithDarwinians. Code: key is `SepulvedaHelp = key h`, the enum is the 20 items of §6, no fixed-message or failure-help classes exist, and the whole system is compiled out of Multiwinia. [C] `code/helpsystem.h`, `data/input/input_preferences.txt` L255.
- `docs/multiwinia-changes.txt` ("Power-ups in the Advanced Tutorial will now terminate correctly upon use") is consistent with code only after the T2 End step (`TerminateTask` is blocked while `!m_tutorial2AICanSpawn`). [C+I]
- `docs/multiwiniachanges-beta.txt` / `multiwiniareleasenotes-beta.txt` mention a "quick tutorial" and an "Advanced Concepts tutorial" — match Tutorial 1/2. No discrepancy.

<!-- from P20_sequences_loaders_bench -->
## Docs-vs-code discrepancies
| Doc | Claim | Code | Ref |
|---|---|---|---|
| `docs/multiwiniareleasenotes-beta.txt` L35 | Beta records benchmarks and uploads them via FTP "when you exit"; files in a `benchmarks` profile folder | Feature compiled out in every target of this tree (`//#define BENCHMARK_AND_FTP`); when enabled, upload happens at the **end of each match** (after `LocationGameLoop`), not at exit; folder name matches | `code/lib/universal_include.h`, `code/main.cpp:EnterLocation`, `code/benchmark.cpp` |
| `docs/changes.txt` L82–84 | "Show First Time Intro" option makes the intro play "the next time you start Darwinia" | Intro only runs from `RunBootloader`, i.e. when the Campaign is launched (and only with `USE_LOADERS`); the option row is not built in the Multiwinia options UI | `code/main.cpp:RunBootloader`, `code/interface/prefs_other_window.cpp` |
| `docs/detailed schedule.txt` L19 | "(OPTIONAL) Intro bootloaders" planned | Implemented but compiled out (`USE_LOADERS`) | `code/lib/universal_include.h` |
| `docs/todo.txt` L19, `docs/betachanges.txt` L46/L57, `docs/buglist.txt` L21 | Bootloader issues (GoL-glow frame rate, Spectrum text distortion, ray-tracer performance, widescreen stretching) | Loaders not compiled in Multiwinia; widescreen stretching is consistent with fixed 320×256 / 800×600 canvases in Speccy/Amiga/Credits (`IsOffscreen` 800×600) | loader sources |
| `contrib/systemiv/lib/math/random_number.h` comment | `sfrand(range)`: −range < result < range | Implementation returns `(0.5 − U)·range`, i.e. ±range/2 (affects Soul/DemoEnd spawn spreads quoted above) | `contrib/systemiv/lib/math/random_number.cpp:sfrand` |
