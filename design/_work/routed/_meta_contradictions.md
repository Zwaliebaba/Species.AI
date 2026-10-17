# Collected "contradictions" sections from all drafts

<!-- from P01_app_core -->
## Contradictions / open questions

1. `docs/Network.txt` says the server ticks at 25 FPS; the code ticks at 10 Hz (0.1 s) [C] `code/network/server.cpp:Server::AdvanceIfNecessary` — see discrepancies.
2. `App::App` comment says main-thread affinity is "Even processors" but mask `0xAA` selects CPUs 1,3,5,7 (zero-based odd) [C]. Irrelevant for a port, noted for accuracy.
3. `Location::ChristmasModEnabled` is commented "0 = unavailable, 1 = enabled, 2 = disabled" but returns a bool; callers compare `== 1` [C].
4. `Unit::GetOffset` is documented "Takes into account formation AND obstructions" but ignores obstructions [C].
5. `Location::GetMonsterTeamId` contains (commented-out) logic that would pick the first free slot; the live code hard-codes 4 [C].
6. `GameTimer::Reset` does not reset `m_requestedSpeedLength`/`m_accurateTime`; a slow-down crate active at game end would carry its remaining duration into the next match's first ticks (then revert to speed 1.0). [I] from reading; harmless in practice because the next match starts at speed 0 and the grace-period request overrides the target.
7. The in-match "game over → press ToggleReady to return to the menu" path in `LocationInput::Advance` exists only under `TESTBED_ENABLED`; in FULL the return to menu must be driven by UI elsewhere [C].

<!-- from P02_game_modes -->
## Contradictions / open questions

1. **BlitzMaster condition** `m_startTime <= m_timeRemaining + 300` mixes a game-time stamp with remaining mode-seconds; the commented-out predecessor used `GetNetworkTime() - m_startTime < 300`. Live behaviour: essentially every Blitzkrieg win qualifies [C code; I intent].
2. **Achievement thresholds vs comments:** Carnage comment "more than 3000", code `>= 2000`; Genocide comment "100,000", code `>= 1000000`; `m_totalKills` starts at −1 [C].
3. **Assault multi-round** (rotate positions, best defence time wins, later rounds timed by the best time so far) is fully implemented but unreachable: `m_assaultSingleRound` defaults true and no UI sets it [C/I].
4. **Double trunk-port advance** on some Blitzkrieg maps (D3) — likely unintended.
5. **`s_numMultiwiniaZones` accumulation** (D4) — bug and desync hazard.
6. **Domination co-op winner loop** iterates `i < totalTeams` instead of all slots (may skip teams in slots ≥ totalTeams) [C].
7. KOTH zone ties are awarded to the lower team index (no contested state) [C].
8. `GetGroupColour(member, -1)` constructs `RGBAColour(0.6f, 0.6f, 0.6f, 1.0f)` — whether that is 0.6 bytes (≈ black) or 60% grey depends on the RGBAColour float ctor (not read) [U-lite].
9. Exit prompt reads "Press ESC to Exit" but polls `ControlToggleReady` [C strings/code; binding P10].
10. `multiwinia_blitzkrieg_unoccupied` and `_underattack` texts both read "Under Threat" — an unoccupied home flag is reported as under threat [C].
11. FuelGenerator renders pump speed with maxSurges 15 but advances with 20 [C].
12. AI-mode network range check accepts 3 [C].
13. Rocket in `StateFlight` can still be damaged into `StateExploding` by non-Darwinian damage (attract-attackers stops at Flight) — then it would "refuel" mid-air [I].
14. Pulse bomb capture check continues after detonation → possible second `RecordDefenseTime` [I].
15. `IsEliminated` for KOTH/CTS (no spawn points & < 50 Darwinians) never removes a team; it only enables retribution (§4.9) and loses CTS ties [C].

<!-- from P03_entity_core -->
## Contradictions / open questions
1. `TypeOfficer` (and `TypeNuke`, `TypeMeteor`, `TypeSpaceShip`, `TypeDropShip`) are "blocked" yet created at runtime; "blocked" only filters level-file placement [C].
2. `Darwinian::StateWatchingGodDish` is never assigned anywhere; `StateFollowingShaman` is unreachable (`SearchForShamans` call commented out) [C].
3. The MP "idle Darwinians go to useful buildings/officers" feature in `AdvanceIdle` is dead because `Advance` resets `m_idleTimer = 60` every tick in MP [C] — likely an unintended disable.
4. `Darwinian::SearchForThreats` ant-nest attack and `Officer::RunAI` final target both pass an **array index** where a building unique id is expected [C] (bugs; behaviour depends on whether a building happens to have that id).
5. `Darwinian::FireDamage` increments the attacker's kill counter on every call after ignition [C].
6. `Officer::Advance` runs `AdvanceInWater` twice per tick (drown ×2 speed) [C].
7. `Entity::AdvanceInAir` discards the `PushFromObstructions` result; `WorldObject::BounceOffLandscape` midpoint is a no-op [C].
8. `ObstructionGrid::CalculateBuildingArea` fence "diagonal gap" guard is defeated by variable shadowing (always dilates) [C].
9. `EntityGridCache` hits ignore the requested range; `AreNeighboursPresent` tests cell capacity, not occupancy; `GetNum*` counts are not range-filtered [C] — all "fast" approximations that change gameplay thresholds (grenade decision, scared check).
10. `SearchForPorts` draws a random building then overrides it with the nearest [C].
11. Laser fences set enemy Darwinians on fire (`SetFire`) rather than killing them outright; other entities die instantly (`ChangeHealth(−9999)`) [C].
12. In MP `Officer::SetNextMode/SetPreviousMode` are no-ops; officer mode changes come from `SetOrders`/`SetFormation` only [C].
13. `IsWalkable` is direction-dependent but the routing graph caches it symmetrically [C].

<!-- from P04_entities_military -->
## Contradictions / open questions
1. `Entity::EntityIsBlocked` lists `TypeNuke`, `TypeSpaceInvader`, `TypeShaman`, `TypeOfficer`, `TypeLaserTroop` as "old Darwinia objects", yet Nuke, SpaceInvader and Officer are core Multiwinia units; the block only affects level-file `InstantUnits` parsing (and the editor's instant-unit window) [C]. Do not interpret it as "not in Multiwinia".
2. Squad secondary weapon: four weapon types are fully implemented, but no reachable Multiwinia path changes `m_weaponType` from Grenade (§4.5) [I]. If the new game wants squad weapon choice it must add UI.
3. `data/stats.txt` RATE for InsertionSquadie (0.5 s) truncates to 0 in `Entity::m_stats` (unsigned char); only the squad-level accumulator uses the 0.5 (via `EntityBlueprint::GetStat`), autonomous fire has no reload at all [C]. The intended per-squaddie reload is ambiguous.
4. `Location::Bang` falloff formula has an operator-precedence bug producing a flat full-damage core to 1.5R (§0.7) [C]; designers balanced against the buggy behaviour.
5. Nuke launch-time compensation (`/100`) does not match the actual mean flight speed (~47 u/s) — simultaneous impact was presumably intended [I].
6. Harvester/SpaceInvader damage explosions scale against Engineer max health (copy-paste) [C] — visual only.
7. Tank Battle respawn re-runs a Tank task whose `TargetTank` is commented out → respawn would spawn nothing even in a debug build [I].
8. `Grenade::m_collected` freeze is not honoured by `FlameGrenade::Advance`; harmless today because Harvesters only capture `EffectThrowableGrenade` [C].
9. Armour exits (unloading) are blocked over water (terrain ≤ 0 at the entrance) — passengers stay inside indefinitely while over sea [C]; intended.
10. Squaddies spawn no spirit on death (custom `ChangeHealth`) whereas Darwinians do [C] — confirm intent (Harvester economy).

<!-- from P05_entities_creatures -->
## Contradictions / open questions
1. **`INCLUDE_THEBEAST`**: `code/worldobject/crate.h` wraps the `#define` in `#ifdef TARGET_DEBUG`, so in `TARGET_PC_FULLGAME` it is **off** and `CrateRewardTheBeast` does not exist (Portal/Tentacle unreachable). `build_matrix.md` §4 originally said "on"; it has since been corrected to off/[B:DEBUG] — consistent with this draft. [C] `code/worldobject/crate.h`.
2. **Eggs crate owner**: `Crate::RunEggs` spawns TriffidEggs on the *task owner's* team, so hatched Virii/Centipedes/Spiders/SporeGenerators fight for the player who used the task, while the uncaptured variants (Triffids/Infection/AntNest) are Monster-team. Intentional design? [C] behaviour; intent [U] — resolve by playtesting the shipped build.
3. **Meteor shower friendly fire**: `Location::Bang` is not team-filtered, so the user's own units in the 420 m square are hit. [C]; intent [U].
4. **SpaceShip wander bias** to the low-x/low-z quarter (SFRAND + heightmap wrap). [I] — resolve by instrumenting `SpaceShip::FindNewTarget` in a build.
5. **`Triffid::GetSpawnName` table order** (`SpawnDarwinians`, `SpawnAnts`, `SpawnSoulDestroyer`, `SpawnSporeGenerator` at indices 6–9) does not match the enum (`SpawnAnts`=6, `SpawnSporeGenerator`=7, `SpawnSoulDestroyer`=8, `SpawnDarwinians`=9); only affects names/translations (`spawnname_*`). [C] `code/worldobject/triffid.cpp:Triffid::GetSpawnName`.
6. **SoulDestroyer hit FX** run the death tail-explosion on every hit (`if( fractionDead )` always true). [C]
7. **Virii trail scatter** (`Virii::AdvanceDead()`) is dead code due to a signature mismatch. [C]
8. **Spider** `StompFoot` (foot damage) never called; `NORMAL_SPEED`/`ATTACK_SPEED`/`ATTACK_PREAMBLE`/blueprint speed unused. [C]
9. **DropShip `ScaleRadius(0.2)`** mutates the shared `spaceship.shp` used by SpaceShip. [C] call; effect on SpaceShip picking/culling [I].
10. **SpaceShip speed stat** relies on out-of-range double→`unsigned char` conversion (500). [I] value 244 under MSVC.
11. **Centipede radius** fixed at `Begin` (10 × size at that moment) while size later converges; contact range therefore depends on spawn order. [C] code; gameplay impact [I].

<!-- from P06_buildings_core -->
## Contradictions / open questions

1. **Handicap arithmetic** uses integer division twice (`average = others/n`; `factor = 1 − (mine−1)/total` → always 1 in practice). The apparent intent (scale by share) is not what ships. Document the shipped behaviour (§05.5). [C].
2. **Per-team cap divides by the level's `NumPlayers`**, not by teams in play: a 4-player map played by 2 teams still gives each team PopulationCap/4. Intentional? [U] (game-design intent; check any `NumPlayers` override in lobby code).
3. **Co-op maps without caps** (`mp_domination_4P_2`, `mp_koth_4P_3`: `PopulationCap -1`, no locks) → unbounded spawn-point growth. [C] data; [I] no other limiter.
4. `mp_cts_4p_3` contains two `PopulationCap` lines (−1 then 2000); the parser keeps the last. [C].
5. `m_spawnCounter` / `m_spawnTimer` are not reset on ownership change (a captured point may fire a partial burst after activation). [C].
6. `s_numSpawnPoints[NUM_TEAMS]` (team 5) not reset in `Location::Init`. [C].
7. `Building::EvaluatePorts` accepts any operating Darwinian within 5 units, without checking it is operating *this* building/port — adjacent buildings could share an occupant. [C] code; [I] effect.
8. `SpawnBuilding::TriggerSpirit` can place one spirit pointer on several links (latent double-delivery / double-free). Shipped graphs avoid it. [C].
9. GunTurret sound list names `RegisteredKill` but the code triggers `RegisterKill`. [C].
10. `Powerstation::Advance` dereferences its link without a null check (legacy). [C].
11. Data oddity for other agents: `mp_assault_3p_1` declares `NumPlayers 2`. [C].
12. JumpPad ignores `m_destroyed` and port evaluation (no base `Advance`). Harmless today. [C].
13. SolarPanel captured from its starting team in Assault loses its ports permanently (the design reads as "knock out"); in other modes panels are freely recapturable. [C]; intent [I].

<!-- from P07_buildings_misc -->
## Contradictions / open questions
- `BuildingIsBlocked` filters only **name-based** creation. Whether any live MW code path creates a blocked type by id: `Crate` creates `Portal` (blocked by name) — P06/P08 to confirm reachability; `Engineer` (→Bridge) is itself expected to be a blocked entity (P03 to confirm via `Entity::EntityIsBlocked`).
- `CloneLab` is not blocked and fully functional yet appears in no shipped map; maps could still be authored with it (e.g. by modders/editor builds). Treated as compiled-in but data-unused.
- SmokeMarker's `float` signatures make it a 13-unit invisible obstacle (by C++ overriding rules); whether this materially affects play on `mp_assault_3p_1` depends on marker placement relative to paths — not verified in-game.
- `LaserFence::DoesRayHit` uses only this post's height for both ends of the collision quad, while rendering uses each post's own height — with differing `scale` values on a link the visible curtain and the lethal surface differ.

<!-- from P08_crates_ai -->
## Contradictions / open questions
1. build_matrix.md §4 says `INCLUDE_THEBEAST` is on in the baseline; `code/worldobject/crate.h` defines it only under `TARGET_DEBUG` → **off in FULL**, `NumCrateRewards = 31` [C]. Coordinator should correct the matrix.
2. Enum comments vs code: SpawnMania comment "15 seconds" vs 5.0 s; Bitzkreig comment "always throw grenades for 10 seconds" vs code = 15 s global rage (`HasRage`); Forest comment "1–3 trees" vs 1–2; Eggs comment "1–3 eggs" vs 2–4; Rage comment "60 seconds" vs task life 30 s; Booster comment "immune to laser fire" vs effect defined in `Darwinian::SetBooster` (P04) [C].
3. `aiobjective.h` header comment says multiple unlinked start objectives → "one will be selected randomly"; code keeps all unlinked objectives active and lets *each AI* pick (`syncrand`) or (defender) wait for the first attacked one, then deactivates its other own objectives [C].
4. Bad-pool outcomes ignore `InvalidCrates` and the AntNest/CrateMania limits — design intent unclear (bug or deliberate) [C behaviour].
5. `RunUncapturedCrate` calls `InitMonsterTeam(capturingTeam)` (no-op) — likely intended `InitMonsterTeam(monsterTeam)` [I].
6. Several AI target-id bugs: `RunSpam` acceptable target = AI's own id; `RunPlague` and `Officer::RunAI` attack target use array index as building id; `RunGunTurret` neutral case returns the own target id; `RunPowerup` centroid divides a Darwinian-only sum by all-friend count. A re-implementation should fix them; behaviour as shipped is "partly random" [C code, I effect].
7. Weighted crate drop height check bug (`pos.z > 20.0` inside `GetValue`) [C].

<!-- from P09_control_taskmanager -->
## Contradictions / open questions

1. `GameCursor::RenderSelectionArrows` and `BoostSelectionArrows` are wired from sim code but never drawn in MP — dead feedback path in the shipped game [C].
2. The MP task bar draws only occupied slots although `Capacity()` is computed; the Icons/Gestures variants draw empty slot shadows for every capacity slot [C]. Design intent for MP slot visibility is unclear [I].
3. `TaskManagerInterfaceMultiwinia` contains a full overlay implementation (delete icons, pad zone navigation, scroll zones 1/3) that can never become visible in MP [C] — remnants of the SP Icons interface.
4. `Task::m_route` and the Controller task creation are commented out; `TypeController` survives only as a squad weapon and as a fall-through branch in `Advance` [C].

<!-- from P10_input_camera -->
## Contradictions / open questions

1. Is the `InsertionSquad::DirectControl` / `m_camLookAround` coupling exercised in practice? It only matters when a player holds `CameraFreeMovement` (F2 / R3) while another team's squad is processed without entity-tracking — likely rare but a genuine desync path [C] code; impact [I].
2. Squad camera zoom-out (`CameraSquadZoomOut`) is effectively broken by int truncation at normal frame rates [C]; was this noticed by players? If the new game keeps a squad follow-camera it should store distance as float.
3. `CameraRotateLeft/Right`, `CameraRotate`, `CameraDeZoom`, `WeaponCycleLeft/Right` are bound (and `control_help_camerarotate*` strings exist) but no live code reads them — the gamepad has no camera rotation in the shipped build [C].
4. The `Value` driver's uninitialised `spec.type` and the `Prefs` driver's off-by-one key index are latent bugs with no shipped trigger [C].
5. Officer/armour entity-tracking uses a stale orbit centre (§3.6) [C]; is EntityTrack ever entered for non-squad entities in practice (via `Task::Start` with `AutomaticCamera = 2` for an officer task)? [I] yes, reachable on gamepad.

<!-- from P11_terrain_world -->
## Contradictions / open questions

1. `LevelFile::ParseMultiwiniaOptions` keeps only the first `GameTypes` token while `GameMenu::AddMap` lists a map under every token; for a multi-mode map the running game's `m_gameTypes` may disagree with the mode chosen in the menu. No shipped map lists more than one mode (survey shows one token per file) [C]/[I].
2. `GlobalWorld::GetLocationIdFromMapFilename` is broken (`*mapName += 4`), so `GenerateAutomaticObjectives` can never create trunk-port secondary objectives — a Darwinia regression that is invisible in Multiwinia [C].
3. Under [B:D3D] in a Multiwinia match the water shader samples a reflection target that is never rendered (pre-pass gated by `!Multiplayer()`) [I] (agrees with P12).
4. `RenderMainSlow` enables blending in negative-renderer mode and never disables it; the overlay pass happens to disable it afterwards [C].
5. `Clouds::RenderFace` fade-out logic ramps alpha up again after 28 s [C] — likely a bug.
6. `LevelFile::WriteMultiwiniaOptions` missing `break` after `futurewinianteam` [C]; `WriteRunningPrograms` and `GlobalWorld::SaveLocations` use `%8.2`/`%-8.2` without a conversion character [C].
7. `ParseLeaderboardStats` reads `%f` into a double [C].
8. Landscape VBO is uploaded with the array's capacity (`Size()`), not its used count [C] — harmless but wasteful.
9. The strip builder adds no degenerate join at row ends; land touching the map edge would produce a stray wrap-around triangle [I].

<!-- from P12_render_core -->
## Contradictions / open questions
1. **D3D deform does not compile on PC as written** [I]: `DeformEffect::Stop` calls `g_pd3dDevice->Resolve(D3DRESOLVE_RENDERTARGET0, …)`, an Xbox 360 API absent from `IDirect3DDevice9`. Either the `ReleaseDirect3D` configuration was not buildable at this commit or a shim existed outside the tree.
2. **Deform render target never restored** [I]: after building the deform table into `m_deformTexture`, `Stop()` never calls `SetRenderTarget(0, m_oldRenderTarget)` before the full-screen composite (and leaks the `GetRenderTarget` reference). On PC this would draw the composite into the deform texture.
3. **Pixel effect / deform / water reflection never run in Multiwinia** [C]: gated by `!g_app->Multiplayer()`; `Multiplayer()` is true for every Multiwinia game. The graphics-menu checkbox for `RenderPixelShader` is also commented out. Under D3D the dynamic water still samples the never-refreshed reflection RT.
4. **Flat-shading colour differs GL vs D3D** [I]: OpenGL's provoking vertex is the last vertex, D3D9's the first.
5. **Texture cache ignores flags** [C]: `GetTexture("x", false)` after `GetTexture("x")` silently returns the mipmapped texture (and vice versa); masking/alpha conversion likewise follows the first caller.
6. **Hangul atlas misnamed** `square.bb00.bfff.bmp` [C] → possible wrong glyphs U+BC00–BFFF [I].
7. **Lights specified before the camera matrix** each frame [C]/[I] (see 24.1).
8. **BMP 4/8-bit row padding ignored** [C].
9. **`WaitVerticalRetrace` not honoured** on the first `CreateWin` attempt (literal `true`) [C].
10. **Max-texture-size downscale discarded** when power-of-two rescaling is also required [C].
11. Explosion debris has **no terrain collision** and uses wall-clock time (pauses only via `GamePaused`) [C].
12. `ShapeFragment::m_positionsInWS` sized by vertex count, filled by position count [C].
13. `DecodeSurrogatePair` OR-vs-ADD bug and 16-bit storage [C].
14. `Renderer::m_pixelEffectGrid` declared `[y][x]` but indexed `[x][y]` everywhere [C] (consistent use, harmless).

<!-- from P13_gfx_backend_math -->
## Contradictions / open questions

1. `Matrix33/34::DecomposeToYDR()` comment says `x = dive, y = yaw`, but the code returns `(yaw, dive, roll)` in (x, y, z) [C]. Callers outside this partition must be checked by their owners.
2. `Matrix33 * Vector3` (world→local) and `Matrix34 * Vector3` (local→world) use opposite conventions; `Matrix34::InverseMultiplyVector` and `Matrix34::Transpose` are mathematically not inverses (both unused) [C].
3. `Plane(normal, point)` uses the opposite sign of `d` to `Plane(a,b,c)` (unused) [C].
4. `Renderer::Initialise` passes literal `true` for `waitVRT` on the first two attempts although it reads `WaitVerticalRetrace` — intent unclear (bug or deliberate "always vsync") [C/U].
5. `SetOpenGLState` sets `glShadeModel(GL_FLAT)` but the D3D emulator ignores shade model; the baseline look relies on flat shading (P12's visual-style section should note that D3D renders Gouraud except for strips) [C/I].
6. `gluPerspective`'s `_43 *= 2` ("Preserve OpenGL compatibility") doubles effective near/far distances [I]; deliberate?
7. Whether `ReleaseDirect3D` builds at all at this commit (see 24 build-viability) [I].

<!-- from P14_platform_lib -->
## Contradictions / open questions

1. `build_matrix.md` §6 says `targets/macosx/*` "still reference Darwinia and do not build Multiwinia". Only `targets/macosx/Darwinia/` is Darwinia; `targets/macosx/Multiwinia/Multiwinia.xcodeproj` is a maintained Multiwinia 1.3.1 Mac project (Ambrosia port) [C] `targets/macosx/Multiwinia/Info.plist`, `MultiwiniaCocoaBridge.cpp`.
2. Authentication key file name: read `authkey.dev` (`code/app.cpp:InitMetaServer`, `code/UI/GameMenuWindow.cpp` L4989, `code/UI/PlayDemoAuthkeyButton.h`) vs written `authkey` (`code/UI/AuthInputField.h` L149); support INI and installers reference `authkey`. Which one shipped [U] — resolve by checking a retail install or the release branch.
3. Beta group A: `tools/multiwinia_inclusions_beta_test_group_a.txt` (13 maps) ≠ `TARGET_BETATEST_GROUP_A` `PERMITTED_MAPS` (3 CTS maps) in `code/lib/universal_include.h`; it equals the commented list under `TARGET_BETATEST_GROUP_ALL` [C].
4. Exe version metadata: `version.rc` says 6.0.0.0 while installers ship v1.0.3/1.0.4/1.2.3/1.9.x and the Game Explorer GDF says Darwinia 1.5.4.0 [C].
5. `build_multiwinia_data.bat` names `sounds2.dat` step "Making sounds.dat" (cosmetic) and calls absent `build_ambx_dat`; `InstallerScript.iss` and the beta installer require `build\ambx.dat`, which no script in the tree produces [C].
6. Is the debug menu (and so the profiler window) reachable in the shipped FULL exe? Depends on input bindings (P10/P17a) [U].

<!-- from P15_network -->
## Contradictions / open questions
1. NETSYNCERROR's CLIENTID is the server's `m_clients` array index while every other message uses client ids; `RegisterNewTeam` also indexes `m_clients[_clientId]` to look up the demo flag. Equivalent only while no client has left. Intended: client id [I].
2. Server `REQUEST_SPECTATOR` handling `return`s from `Server::Advance` in single player, skipping that tick's UPDATE (frame gap is not created — the sequence id simply isn't consumed — but the letter object leaks) [C].
3. Comment in `ClientToServer::Advance` says team requests are sent "once a second"; the timer gate actually fires every 0.1 s [C].
4. `GetNumSlicesToAdvance` contains a dead time-based branch (shadowed variable); unknown whether the intended pacing was time-based [U: intent only].
5. `HandleDisconnectedClient` and `RemoveClient` append to `m_outbox` without `m_outboxMutex` (benign: only the main thread touches the outbox) [C].
6. Late joining in progress is technically possible but impractically slow; whether it was intended for release is unclear [U: release notes or UI that greys in-progress games would resolve; `GameServerButton` does not].
7. In-game the server does not remove a departed client's `ServerTeam`; whether the CPU-converted slot can be re-occupied by a rejoiner depends on `RegisterNewTeam` counting it (it does count it), so a rejoin into a full match is refused [I].

<!-- from P16_sound -->
## Contradictions / open questions

1. Channel-count default: prefs generator writes 32 and the sound system falls back to 32, but `PrefsSoundWindow::ReadValues` and the Apply-button comparison fall back to 16 — only visible when the key is missing from `preferences.txt` (the Apply button then shows as active) [C].
2. `SoundMixFreq` fallback: 22050 in the sound system and options window, 44100 in `SoundLibrary2dWin32` [C].
3. `SoundBufferSize`: generated default 512 vs code fallback 2000 [C].
4. Volume curve differs between DirectSound (5 dB per vol unit) and the software mixer (4.34 dB per unit) — which is "the design"? Proposal: DirectSound's (shipped) [I].
5. The software-mixer volume-ramp flag is inverted relative to its comment; a new mixer should ramp gain every block (de-click) except on a fresh voice [C/I].
6. Is music reachable at all in Multiwinia? All `TypeMusic` triggers are `USE_LOADERS` code or Darwinia scripts [I] — P19/P20 to confirm.
7. Attached sounds hard-cut when their emitter dies (§27.4) although `Update3DPosition` contains a graceful "convert to stationary and release" path that is effectively dead — intended behaviour was probably the graceful one [I].
8. DirectSound head-relative 2D sounds: `SetChannelPosition` skips updates closer than 5 % of the listener distance, measured in world space, so a 2D sound (position 0,0,0 in head space) inherits a stale world position when the channel's previous position was near the world origin relative to the camera distance [I from code; not observed].

<!-- from P17a_ui_framework_editor -->
## Contradictions / open questions

1. `docs/changes.txt` ("Editor changes") says the move-buildings-with-tile option is "in the land tiles menu"; code puts `editor_movebuildings` in `LandscapeEditWindow` (the main landscape window), not `LandscapeTileEditWindow` [C] — wording ambiguity, logged below.
2. `DarwiniaWindow` computes a translated per-window title but `Render` always prints "MULTIWINIA" — intentional rebrand shortcut or bug? [I: rebrand shortcut when Darwinia windows were reused for Multiwinia].
3. Reinforcement-mode drop-down values collide (airstrike 0 = gun turret 0; napalm 1 = rocket turret 1) — presumably interpreted per reinforcement type [I]; P02 should confirm how `LevelOptionAttack/DefendReinforcementMode` is consumed.
4. `LevelFile::m_gameTypes` is a single int edited with a single-choice drop-down although named plural; P11 should confirm whether it is a bitmask elsewhere [U].
5. Whether `MapFile` can open a `.mwm` created by `NewMapWindow` (which writes `LevelFile` text) [U] (see E1.3).

<!-- from P17b_menus_options -->
## Contradictions / open questions

1. `HelpAndOptionsWindow` and `OptionsMenuWindow` exist both at the front end and in game, but the Other-Options apply rebuilds windows assuming one of two contexts (`m_atMainMenu` → `GameMenu::CreateMenu()`, else `LocationWindow`) — after a language change in game the user lands on the Location menu, not back in Other Options (unlike Screen Options, which re-opens itself). Intentional? [I] probably not; harmless.
2. `PrefsOtherWindow` Apply-button dirty check ignores `ControlHelpEnabled`, `AutomaticCamera`, `LargeMenus`, `Difficulty`, `BootLoader`; since none has a visible control this is consistent with the shipped UI, but it shows the Darwinia window had more rows (labels still rendered in the unreachable second half of `PrefsOtherWindow::Render`: Help System, Control Pad Help, Boot Loaders, Language, Difficulty, Large Menus, Automatic Camera, version string).
3. `PrefsGraphicsWindow::Render` returns immediately after `GameOptionsWindow::Render`, leaving dead code that would draw labels and an FPS readout (`"%d FPS"`); same in `PrefsOtherWindow::Render`.
4. `ResetProfileButton` deletes `users/<name>/` relative to the working directory while profiles are saved under `GetProfileDirectory()` (`%LOCALAPPDATA%\Introversion\Multiwinia\`) — would delete the wrong (usually non-existent) folder. Legacy only.
5. `TextButton(UnicodeString)` constructor calls `TextButton(NULL)` as a statement (creates a temporary) — `m_string`, `m_rightAligned`, `m_shadow` are left uninitialised for unicode-caption TextButtons [C] `code/interface/helpandoptions_windows.cpp:TextButton::TextButton`. Only the `char*` constructor is used in scope.
6. `english.txt` defines `multiwinia_helppage_3_2` twice; which one the `LangTable` keeps is P19/P14 territory [U].

<!-- from P18_ui_gamemenu -->
## Contradictions / open questions
1. **Map PlayTime is overwritten** (§8.4): the map click requests `TimeLimit = PlayTime`, then the host's lobby setup resets every parameter to defaults [C both calls; I outcome].
2. **"Show passworded games" filter is ineffective**: it tests key `"dy"` (`NET_DARWINIA_SERVER_PASSWORD`), servers advertise `"dI"` (`NET_METASERVER_HASPASSWORD`); `"dy"` is also `NET_METASERVER_GAMETIME`, which nobody sets [C keys; I effect].
3. **Manual server entries are unjoinable**: synthetic entries lack version/protocol and map CRC, so clicking reports "incompatible" [I].
4. **Server-row bookkeeping**: rows are inserted into `m_buttonOrder` at `Size()−1` but removed at `m_serverButtonOrderStartIndex + row` (start index 0 = Back button's slot), so shrinking the list removes the wrong order entries (possibly the Back button) [C mechanics; I consequence].
5. **`SpecialScroll` target** is fetched by focus index from the registration list (§12.4) [C].
6. Colour button **names** for ids 5/6 swapped (cyan/purple) relative to colours and captions [C].
7. Typed keys are saved to `<profile>authkey`, pasted and demo keys to `<profile>authkey.dev` [C].
8. Kick buttons exist only for slots 1..n−1; a remote player occupying slot 0 cannot be kicked [C/I].
9. Scroll bar created with window size 10 for a 13-row list [C].
10. Phrase `Spectator` used by gamer tags does not exist in `english.txt`; drop-down captions use raw parameter names as phrase ids; Update button caption hard-coded "Update" [C].
11. Leaving a multiplayer lobby as host always destroys the server (`ShutdownCurrentGame`) and returns to GameSelect — there is no way to change map/mode without dropping clients [C].
12. No timeouts: Connecting page has none in FULL; a lobby waiting for a team id stays blank indefinitely [C/I].
13. Constructor auth gate uses only the local algorithmic check; metaserver-rejected keys are only surfaced via the status box [C].
14. `m_clickGameType` is not initialised in the constructor (commented out) though read in `SetupMapSelectPage` — always written by `GameTypeButton` before MapSelect is first reached [C/I].
15. The single-player lobby's map-description widget is named `chat_button` (same name as the MP chat log) [C].

<!-- from P19_help_tutorial_script -->
## Contradictions / open questions
1. The brief listed `tutorial.cpp`, `tutorial_demo1.cpp`, `tutorial_demo2.cpp` as "the Multiwinia tutorial(s)"; the code shows these are the Darwinia demo tutorials and are not compiled. The Multiwinia tutorials are in `multiwiniahelp.cpp`. [C]
2. `data/mwhelp/*.txt` was expected; the directory contains only 51 BMPs. [C]
3. `MultiwiniaHelp` computes 18 objective tabs every frame (and keeps "accomplished" notifications from 6 call sites) but never draws them (renderer under a never-defined macro). Either a cut feature or a deliberate disable; a new team should decide whether to revive "objective cards". [C fact / I intent]
4. `Script::RunCommand_Wait` uses `GetHighResTime()` but `Advance` compares with `g_gameTime` (equal except while paused or demo-recording, where `g_gameTime` freezes / uses fake time) — waits keep running while paused. [C+I]
5. `WaitSay` and `WaitPlayerNotBusy` silently stop blocking in FULL because their checks are compiled out; any script relying on them would race. [C]
6. `MultiwiniaHelp::Reset` does not reset the T1 score-help latches (second run of T1 loses the Scores tab). [C]
7. T1 rule B can request two different tabs in one frame; the second is rejected by the 0.5 s limiter (first spawn in iteration order wins). [C]

<!-- from P20_sequences_loaders_bench -->
## Contradictions / open questions
1. `LoadingScreen::m_rendering` is never initialised in the constructor; `IsRendering()` before the first `Render()` returns an indeterminate value (affects `Resource` locking and `SoundSystem` event suppression during early start-up) [C]. Probably harmless because the first `Render()` happens inside `App::App`, but unverified.
2. `CreditsLoader::Run` starts `Credits` but stops `Music Cutscene3` [C]; `MatrixLoader::Run` returns at 23 s without stopping its music [C]. Intent unclear — likely bugs.
3. Raytrace help prints motion blur with a missing `swprintf` argument [C] (undefined behaviour).
4. `GameOfLifeLoader` speed keys are overridden every frame (no effect) [C]; it seeds from `syncrand` (network RNG) [C].
5. `GetRandomLoaderIndex` reseeds `AppRandom` from wall-clock time — affects every later non-synced random consumer in the session [C].
6. `AnimatedPanel::DeleteKeyframe` removes without freeing; `BeginRender/EndRender` declared, undefined [C].
7. `animatedpanel.cpp` includes `lib/render/renderer.h`, which resolves (via the `..\..\contrib\SystemIV` include path) to the uncompiled SystemIV renderer header [I] — nothing from it appears to be used; a re-implementation can ignore it.
8. `RequestBootloaderSequence` leaves `g_RenderingFirstTimeBL = true` forever in FULL builds → fade-outs suppressed after a Campaign launch (§1.6) [C].

<!-- from X1b_asset_formats -->
## Contradictions / open questions
1. P14 states archive keys must be requested in lower case; code shows case-insensitive look-up (above) — resolved in favour of code [C].
2. Original spec §5 places `effects.txt` under 26_animation_and_vfx; the file is audio DSP configuration → route to 10/27 [C].
3. `ShapeExporter` source vs shipped `.shp`: the in-tree exporter writes triangle lists and no `right:` lines, yet 39 shipped files are strip-encoded and 3 contain `right:` → files predate the current exporter [I].
4. Marker `front` sign convention differs from fragments in the exporter (§29.B4) — interpretation [I]; a test export would confirm.

<!-- from X2a_asset_catalogue_visual -->
## Contradictions / open questions
- `textures/laser.bmp` and `clouds.bmp` cache order: P12 inferred `RenderBlobby` loads clouds first (non-mip); code shows the start-up preload loads both with default flags first [C]. P12's conclusion for laser ("saved because RenderSky requests it mip-mapped earlier") is right in outcome but for a different reason.
- `icons/compass_empty.bmp` is requested by live code (`MarkerSystem::RenderCompass` when `_screenDX == 0 && _screenDY == 0`, and unconditionally by `MarkerSystem::RenderCompassEmpty`) but the file does not exist in `data/`; `GetTexture` would raise the fatal "Failed to load texture" assert. `RenderCompassEmpty` has no caller [C grep], so only the zero-direction case is exposed.
- `data/textures/shaman_background.bmp` is RLE8-compressed; the loader does not support RLE. Harmless only because ShamanInterface is unreachable in Multiwinia.
- `data/icons/marker_colour.bmp` is processed differently depending on first requester (§25.2.4) — a latent visual inconsistency.
- `data/shapes/carrycube.shp` declares fragment `Box04` twice (debug assert in `Shape::Load`; in release both attach, and `LookupFragment("Box04")` returns the first).
- `data/thumbnails/mp_cts_3p_3.bmp` and the level `mp_cts_3p_3.txt` are both excluded from retail packing although the level is otherwise a normal CTS map — the map is effectively cut from the shipped game [C] `tools/multiwinia_exclusions.txt`.

<!-- from X2b_asset_catalogue_audio -->
## Contradictions / open questions

1. **Stationary loops never stop** (`PulseBomb Create`, legacy `Generator ReprogramComplete`) — §10.E [I]; resolve by running an Assault match to completion.
2. **`menusounds.dat` early load has no effect** because all triggers wait for `m_soundsLoaded` (set after `sounds.dat`/`sounds2.dat`) — §10.B [I].
3. **Victory jumps in objective-less maps**: `Darwinian::BeginVictoryDance` treats an empty `m_primaryObjectives` list as victory; 47 of the 57 `data/levels/mp_*.txt` define `PrimaryObjectives` (45 with `NeverTrue`, 2 `mp_sandbox*` with `BuildingOnline`), 10 define none (`mp_assault_2p_3`, `mp_assault_3p_1`, `mp_assault_3p_3`, `mp_assault_4P_2`, `mp_blitzkrieg_4P_2`, `mp_domination_2P_1`, `mp_domination_4P_1`, `mp_koth_2P_3`, `mp_koth_2P_3b`, `mp_skirmish_3P_1`) → on those, idle team-0 Darwinians above y = 10 should randomly jump with `VictoryJump` [I: depends on the level loader leaving the list empty; P11/P02 to confirm].
4. **Crate decay sound bug** (`"Capturing_%pc"`) — §10.C [C].
5. **Base-class attribution is by method name**: overloads with different signatures (e.g. `Statue::Destroy()` vs `Building::Destroy(double)`) are treated as overrides; this can only mis-classify rows that are silent anyway (no data) [I].
6. **Render-driven triggers**: `Grenade|AirStrikeBomb Flash` (`ThrowableWeapon::Render`) and the in-game clock `CountdownTick` (`Multiwinia::Render`) fire from rendering, so they depend on the object/HUD being rendered [I]; a new engine should drive them from simulation/UI state.
7. **Officer/Squad acknowledgement voices recorded but unbound** (§10.H) — design intent (docs) vs shipped data; the new game should decide explicitly.
8. **P16 §27.10 music release**: P16 states all music blocks have RELEASE 0; the data has 3.29–20.71 s releases on 14 of 17 blocks (§10.F) [C]. Moot for Multiwinia (music unreachable) but relevant to anyone reusing the music design.

<!-- from X4_level_catalogue -->
## Contradictions / open questions
1. `level_enum.txt` is excluded from every Multiwinia data build but is the only source of achievement level ids (§13) → Master of Multiwinia unreachable with script-built data [C]/[I].
2. Beta group A: `PERMITTED_MAPS {CTS_2P_1, CTS_3P_1, CTS_4P_3}` vs `multiwinia_inclusions_beta_test_group_a.txt` shipping 13 maps of which only `mp_cts_3p_1` is in that whitelist (the inclusion list equals the commented-out `GROUP_ALL` list); with only `INCLUDEGAMEMODE_CTS` compiled, a group-A build would show 1 CTS map [I].
3. `TARGET_PC_HARDWARECOMPAT` and `TARGET_ASSAULT_STRESSTEST` whitelist `MP_BENCHMARK_1` / `MP_ASSAULT_STRESS`, whose files say `GameTypes none` → `AddMap` discards them, so they cannot be picked in the menu [C]; how those builds launched them is [U].
4. `LevelFile::ParseMultiwiniaOptions` keeps only the first `GameTypes` token whereas `GameMenu::AddMap` honours all; harmless for shipped data (every file has ≤1 type) and `LevelFile::m_gameTypes` is only read by the editor options window [C].
5. `mp_assault_3p_1` is named "3p" but declares `NumPlayers 2` (its `old/` precursor had 3) [C].
6. `mp_blitzkrieg_2p_1` and `mp_skirmish_2p_4` define a `player2` camera mount on 2-player maps; `mp_assault_3p_3`, `mp_tankbattle_3P`, `mp_tankbattle_4P` have no `player<p>` mounts (camera stays at the post-load target) [C].
7. `WriteMultiwiniaOptions` fall-through after `futurewinianteam` (§12) [C].
8. `GlobalWorld::GetLocationIdFromMapFilename` pointer bug (`*mapName += 4`) (§11) [C].
9. `OrderedInsert` never compares against the last list element, so map-list base order is only approximately alphabetical (§1.4) [C].
10. Two thumbnails without a loadable level ship in retail data (§10) [C].
