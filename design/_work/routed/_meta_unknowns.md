# Collected "unknowns" sections from all drafts

<!-- from P01_app_core -->
## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Whether anything throttles the frame loop (vsync / sleep in `Renderer::Render` or `WindowManager::Flip`) | P12/P13 read of renderer flip path |
| U2 | Which `m_clientId` CPU teams are given (host client vs -1) | P02/P15: callers of `Multiwinia::InitialiseTeam` |
| U3 | Whether route ids diverge across clients and matter (D15) | P03 Darwinian route following + X3 check of sync hash contents |
| U4 | Whether `ClientToServer::m_outOfSyncClients` changes at the same sequence id on all clients (D4) | P15 |
| U5 | Whether any shipped Multiwinia map contains a `FenceSwitch` (D16) | X4 level catalogue |
| U6 | Whether every Multiwinia mission path runs `LevelFile::GenerateAutomaticObjectives` (else `LoadLevel` would dereference a NULL `GlobalLocation` for id 999) | P11 `LevelFile` constructor |
| U7 | How the client leaves a finished match in the FULL build (who sets `m_requestedLocationId = -1`) | P17b/P18 game-over / in-game menu windows |
| U8 | Exact semantics of `NET_DARWINIA_TERMINATE` when `RequestTerminateEntity` targets an officer vs a task | P15/P09 |

<!-- from P02_game_modes -->
## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Exact rules for Darwinians occupying building ports (how ControlStation occupants are acquired/lost) | P06 `code/worldobject/building.cpp` port code |
| U2 | Damage per Darwinian shot / explosion on `EscapeRocket` (hence time-to-explode) | P03 `Darwinian::AdvanceAttackingBuilding`, P04 explosion damage |
| U3 | Composition of `Team::m_others` (what counts toward trunk-port population caps) | P01 `team.cpp` |
| U4 | `TaskManager::Capacity()` for eliminated teams (string says "Two powerups max") | P09 `taskmanager.cpp` |
| U5 | `RGBAColour(float…)` semantics (contradiction 8) | `code/lib/rgb_colour.*` |
| U6 | Whether the retail Steam/XBL build had a real achievement layer replacing the stubs | not in tree; external |
| U7 | Units of level `PlayTime` — treated as minutes because it is copied into `TimeLimit` and printed as `0:%02d` | consistent [I]; confirm with P18 `MapData` |
| U8 | Port-marker → port-count mapping (5 and 3 ports assumed from `MarkerPort*` markers) | P06 `Building::SetShape`/port loading |

<!-- from P03_entity_core -->
## [U] Unknowns
| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Where `Darwinian::m_underPlayerControl` is cleared after an order (only teleport entry clears it in P03 files) | read `code/location_input.cpp`, `code/taskmanager.cpp` (P09) |
| U2 | Effective `m_stats` value for Meteor (350) and SpaceShip (500) speed after double→uchar conversion (UB; MSVC x87 behaviour) | disassemble/run the shipped binary, or check whether P05 subclasses read `m_stats[StatSpeed]` at all |
| U3 | Whether the wall-clock `m_front` in `AdvanceWatchingSpectacle` actually desyncs Rocket Riot (is the match still simulated after launch?) | P02 Rocket Riot end-of-game flow; sync-log test |
| U4 | Whether Harvesters collect enemy-team souls (no team test seen in `Harvester::SearchForSpirits`) | full read of `code/worldobject/harvester.cpp` (P04) |
| U5 | Crate capture rule (how `StateOpeningCrate` Darwinians contribute; team rules) | `code/worldobject/crate.cpp` (P08) |
| U6 | Exact port-capture contribution of `Building::OperatePort` per tick | `code/worldobject/building.cpp`, `spawnpoint.cpp` (P06) |
| U7 | Whether `GAMEOPTION_GAME_RESEARCH_LEVEL` can override the fixed MP research table (order of `SetGameResearch` vs `Location::Init`) | P15/P01 sequencing of lobby options vs location init |

<!-- from P04_entities_military -->
## [U] Unknowns
1. [U] Whether `squad.shp` / `spaceinvader.shp` / `harvester.shp` / `engineer.shp` carry any team colouring at render time (no recolour code in these classes). Resolve: inspect shape files' colour data and the generic entity render path in `code/location.cpp`/`code/renderer.cpp`, or run the game.
2. [U] Exact airstrike/nuke timings on real terrain (simulations assume flat ground; bomber flight ignores terrain entirely, nuke arc depends on target height). Resolve: instrument `AirstrikeUnit::Advance` / `Nuke::AdvanceToTarget` in a running build.
3. [U] Whether `team->m_currentMousePos` used by `Squadie::Advance` is network-synchronised for remote teams (facing only). Resolve: read `TeamControls` serialisation in `code/network/` (P-net).
4. [U] Behaviour of a `Spirit` left in `StateHarvested` when its Harvester is destroyed via `Task::Stop` (no fallback in `Spirit::Advance`). Resolve: test, or read remaining `Spirit` code paths (P05).
5. [U] Whether any shipped Multiwinia map is the `mp_sandbox*` map that contains Control Towers/Incubators (Engineer reprogramming relevance). Resolve: P08 map list / `PERMITTED_MAPS` / menu filters.
6. [U] Final in-game key bindings for squad grenade/armour toggling (manual: right-click = lasers, hold right + left-click = grenade, right-click armour = toggle). Resolve: P06 input binding files `data/` input prefs.

<!-- from P05_entities_creatures -->
## [U] Unknowns
| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Exact runtime value of the SpaceShip byte speed stat (UB conversion) | Run the shipped binary or disassemble `Entity::SetType` |
| U2 | Designer intent for Eggs-crate creatures belonging to the task owner (contradiction 2) | Playtest / design notes |
| U3 | Whether `Virii::s_viriiPopulation` returns to 0 between games (no explicit reset; relies on destructors) | Verify `Location` teardown deletes every Virii |
| U4 | Behaviour of Futurewinian-team Darwinians after drop (CPU team with no spawn points/AI entity?) | Read `Location::InitFuturewinianTeam` callers and AI creation for team 5 (07_ai) |
| U5 | `Location::Bang` damage falloff formula `(_range*2 - d)/_range*2` (operator precedence) — confirmed in code; whether intended | Design intent; documented as-is |

<!-- from P06_buildings_core -->
## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | MP research levels (`GlobalResearch::TypeLaser`, `TypeRocket`, `TypeArmour`) → subversion-laser lifetime; (TurretShell and turret rockets override research in MP) | `Multiwinia::SetGameResearch` callers / MP research table (modes agent) |
| U2 | Whether `GetNetworkTime()` and `g_gameTimer.GetGameTime()` are the same synced clock | `code/gametimer.cpp` (timer agent) |
| U3 | How Darwinians react to `EffectGunTurretTarget` (flee radius, trigger conditions) | `darwinian.cpp` threat search (units agent) |
| U4 | Whether GameTypes-none dev maps (`mp_sandbox*`) can be opened from the shipped UI (affects ControlTower/Incubator reachability) | `GameMenu::AddMap` (meta agent) |
| U5 | `NUM_SLICES_PER_FRAME` and confirmation that each building advances exactly once per 0.1-s tick | core-loop agent (`main.cpp`/`clienttoserver.cpp`) |
| U6 | Exact composition of `Team::m_others` (which entity types count against the population cap) | `team.cpp:NewEntity` (units/team agent) |
| U7 | Where `m_numLiftersThisFrame` is incremented and the `m_minLifters/m_maxLifters/m_carryRadius/m_speedScale` of Statue/WallBuster | `statue.cpp`, `wallbuster.cpp`, Darwinian carry code |
| U8 | Origin/meaning of `Darwinian::m_subversionTaskId` (turns Standard turret shots into subversion lasers) | crate/task agent (`Crate::RunSubversion`) |
| U9 | `LaserFence::ProvidePower` and `FuelGenerator::ProvideSurge` effect per surge | laser fence / rocket agents |
| U10 | `Location::GetTeamFromPosition` mapping rules (level team positions → lobby teams) | location/lobby agent |

<!-- from P07_buildings_misc -->
## [U] Unknowns
1. [U] Exact visual impact of the Tree hit-sphere/rotation mismatch (needs running the game or reproducing the generator numerically for a sample tree) — design-irrelevant for a new game but relevant for bit-exact replays.
2. [U] Whether `mp_assault_3p_1` smoke markers sit on walkable routes (resolve by plotting the six marker positions against the map's routes/AITargets).
3. [U] Whether `Location::Bang` is ever called with damage ≥ 300 at a wall's position in MW (nuke/airstrike values live in P08/P04) — determines whether walls can be destroyed by anything other than the WallBuster.
4. [U] Texture contents/dimensions of `textures/tree.bmp`, `laserfence.bmp`, `laserfence2.bmp` (P25 asset catalogue).

<!-- from P08_crates_ai -->
## [U] Unknowns
1. [U] Exact `Location::Advance` slice cadence (10 slices per tick assumed from `NUM_SLICES_PER_FRAME`) — resolve in P01/P03 (`code/location.cpp`, server advance loop). Affects only the "once per tick" statements for `AdvanceCrates` and `Team::Advance` ordering.
2. [U] Whether `g_app->m_location` is assigned before `Location::Init` completes (needed for monster/Futurewinian AI spawn via `Multiwinia::InitialiseTeam`) — resolve by reading `code/multiwinia.cpp:Multiwinia::ResetLocation`/`App` location creation (P01/P02).
3. [U] `TaskManager::IsValidTargetArea/GetTargetArea` placement rules (what areas the AI may target: own territory radius, restriction zones) — owned by P09.
4. [U] Real-time vs game-time behaviour of `GameTimer::Tick` cadence while game speed ≠ 1 (determines real duration of SlowDown/SpeedUp holds) — P01/P15.
5. [U] Whether any shipped map lacks sea (would hang `RunNuke`) — needs a heightmap scan of every level (`data/levels/map_*`/landscape definitions; P11).
6. [U] Effect-entity behaviours (Randomiser radius/duration, SpaceShip abduction, AirStrike bomber counts, Nuke damage) — P04/P05.

<!-- from P09_control_taskmanager -->
## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Whether `Team::m_selectionCircleCentre/Radius` (written locally by UI) are read by simulation code | grep/read of P01/P03 Darwinian selection handling (`NET_DARWINIA_SELECTDARWINIANS` handler) |
| U2 | Whether particles created from sim code (`Task::Promote`) can affect simulation | P12 particle-system review |
| U3 | Callers of `GameCursor::RenderWeaponMarker`, `Task::Demote`, `TaskManager::GetNextTaskId` in live code (none found by grep in `code/`) | full-tree reference search by X-agents |
| U4 | Exact semantics of `RestrictionZone::IsRestricted(pos, true)` used by turret/anywhere placement | P07/P11 restriction-zone documentation |
| U5 | Visual appearance of icon/cursor BMPs (only file names known) | asset review (art IP — replace anyway) |

<!-- from P10_input_camera -->
## [U] Unknowns

1. [U] Exact XInput SDK dead-zone/threshold constants used at build time (`XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE`, `…_RIGHT_THUMB_DEADZONE`, `XINPUT_GAMEPAD_TRIGGER_THRESHOLD`) — headers not in the tree; resolve from the DirectX SDK version referenced by the vcproj (believed 7849 / 8689 / 30).
2. [U] Behaviour of `BYTE = floor(trigger·6.3)` overflow under MSVC 2005 (wrap vs saturate) — affects analogue trigger magnitudes (no shipped binding uses trigger magnitude, only zero/non-zero, so impact is limited).
3. [U] Whether `UnicodeString` → `std::string` assignment in `W32InputDriver::getInputDescription` produces ASCII text (assumed in the rebinding-failure finding §1.6.5); resolve by reading `contrib/systemiv/lib/unicode/unicode_string.h` conversion operators.
4. [U] Whether any live camera-distance consumer (determinism note 5) influences simulated state — for the determinism agent.
5. [U] `InputManager::Advance` is also called at the bottom of the top-level `while(true)` loop in `AppMain` after each sub-loop returns [C] `code/main.cpp`; whether this extra advance can swallow an edge between loops is unverified.
6. [U] Real-world reachability of `ModeEntityTrack` for keyboard users: requires `AutomaticCamera = 2`, which no shipped UI widget sets; only by editing `preferences.txt`.

<!-- from P11_terrain_world -->
## [U] Unknowns

1. **Bit-exactness of the reconstructed terrain** versus `multiwinia.exe` — resolve by dumping `Landscape::m_heightMap` from a running build for a few maps and diffing against `scripts/P11_terrain_ref.py --dump`.
2. MSVC 2005 x86 behaviour of `(unsigned short)` conversion of doubles ≤ −1 (affects `GetValue` on far-negative coordinates) — resolve by disassembly or a test program built with the original toolchain.
3. Whether MSVC 2005's `pow/exp/log` round identically (after float truncation) to a modern CRT for every tile parameter in the shipped data — resolve by running the reference generator with both CRTs.
4. Whether the loading-screen thread consumes the global `AppRandom` while `GetLandscapeColour` runs (would make terrain colours vary between runs) — resolve by reviewing `code/loading_screen.cpp` (P20).
5. Which texture flags win for `textures/clouds.bmp` (requested both with and without mip-maps; the resource cache is keyed by name) — resolve in `Resource::GetTextureWithFlags` (P12/X?).
6. Units of `PlayTime` (assumed minutes) — resolve in P02/P18 consumers.
7. Design intent of `triangleOutline.bmp` (2×2 sub-cells, three diagonals) mapped once per height-map cell — visual check in a running build.
8. Whether `ReleaseDirect3D` was ever built from this tree (three compile blockers now known) — attempt a build.

<!-- from P12_render_core -->
## [U] Unknowns
1. [U] Whether `ReleaseDirect3D` builds at all (resolve: compile `code/deform.cpp` against the DirectX 9 SDK; check for an `IDirect3DDevice9` wrapper declaring `Resolve`).
2. [U] What D3D players actually saw on dynamic water in Multiwinia (reflection RT contents never written; resolve: run the D3D build and capture).
3. [U] Exact semantics of `g_predictionTime` (macro in `code/gametimer.h`, outside scope; resolve with P-architecture).
4. [U] Whether shipped data contains any RLE or top-down BMP (decoder would fail silently in release; resolve: scan all `data/**/*.bmp` headers with `OUT/_work/scripts/P12_bmpinfo.py` — the 25 owner's catalogue).
5. [U] Visual content of `StartSequence::Render`, `MultiwiniaHelp::Render`, `MouseCursor` passes (P-UI).
6. [U] Real per-frame cost / draw-call budget (no budgets are encoded beyond the caps listed; resolve: profile the shipped build with `g_profiler`).
7. [U] Whether any shipped level sets `LevelEffectSpecialLighting` (resolve with the level-file agent). `RenderSpecialLighting` defaults to 0 and `data/default_preferences.txt` has no Render* entries [C], so in practice special lighting comes from Rocket Riot or the level effect.

<!-- from P13_gfx_backend_math -->
## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Does `ReleaseDirect3D` compile (abstract `WindowManagerDirectX`, `D3DRS_LINEWIDTH`)? | Build with DirectX SDK March 2008 headers |
| U2 | Is `D3DRS_LINEWIDTH` present in any PC D3D9 SDK header? | Inspect `d3d9types.h` of the SDK in the vcproj include path |
| U3 | Register allocation of non-`register()` uniforms in FixedFunc shaders (and sampler registers s0/s1) | Disassemble compiled shaders (`fxc /Fc`) for the profile returned by `D3DXGetVertexShaderProfile` |
| U4 | Whether D3D9 exposes >1 mip level for `D3DUSAGE_AUTOGENMIPMAP` managed textures here (does `MipMapTask` do any work?) | Runtime check of `GetLevelCount()` on target hardware/driver |
| U5 | Whether `_43 *= 2` was intended (effective depth range [2zn, 2zf]) | Compare D3D vs GL builds at the near plane; author intent unavailable |
| U6 | Actual swap interval of the baseline GL build (driver default) | Runtime observation; no code sets it |
| U7 | Behaviour of `AVIStreamWrite` with `biSizeImage = 0` (moot: `AddFrame()` is never called) | Runtime test under a debug build |
| U8 | Exact x87 control-word change performed by D3D9 `CreateDevice` without `FPU_PRESERVE` on the shipped driver/runtime | `_control87(0,0)` readback after `Direct3DInit` |
| U9 | Whether `docs/HardwareComplianceIssues.txt` "mouse pointer background not transparent" was fixed in cursor code (P12 `targetcursor`) | P12 review of cursor texture alpha handling |
| U10 | Whether the landscape D3D path restores the emulator's vertex declaration after drawing | P11 reading of `LandscapeRenderer::Render*` end |

<!-- from P14_platform_lib -->
## [U] Unknowns

1. [U] Behaviour of `throw;` without an active exception under the VS2008 static CRT inside the `__try` of `AppMain` (expected: SEH 0xE06D7363 caught by `RecordExceptionInfo`, or `terminate()` → `abort()` bypassing it). Resolve by running an assert in a Release build.
2. [U] Exact x87 control word after D3D9 device creation in `[B:D3D]` (expected 24-bit precision). Resolve by reading the control word after `CreateDevice` at run time.
3. [U] Role of `tools/SlashSort.exe` (no script references it; only a usage banner is visible).
4. [U] Contents/role of the absent `ambx.dat`, `branding.dat`, `patch.dat` builders and of `tools/autopatcher`, `tools/profilelocatorwin32` (referenced by packaging scripts, absent from the tree).
5. [U] Which authentication key file name shipped (contradiction 2).
6. [U] Whether `MultiwiniaSupport.exe` uploads automatically or only on user consent, and its protocol (binary only; strings mention `crashreports.introversion.co.uk`).
7. [U] Whether shipped `default_preferences.txt` was ever placed loose next to the exe (would change first-run defaults, §31.5.3).

<!-- from P15_network -->
## [U] Unknowns
1. **MetaServer/MatchMaker server-side behaviour** (which fields it adds to LISTRESULT such as PROTOCOLMATCH/DEMORESTRICTED/DEMOSERVER, how it relays REQUEST_CONNECT, how it validates keys, hash token policy). Resolve: the metaserver source (not in this tree) or packet captures.
2. Real key algorithm (`Authentication_SimpleKeyCheck`, `GenerateKey` stubbed in this release). Resolve: original authentication.cpp. Irrelevant for a new game (replace with platform auth).
3. The exact host-only gating of lobby option controls (UI side). Resolve: P18 (`GameMenuWindow` page setup).
4. Steady-state backlog/latency numbers in §2.4 are analytic; measured values would need instrumentation.
5. Whether `DEBUG_RENDER_ENABLED` sync-diff markers are visible to players in baseline (they only appear after an IFrame diff, which never happens in baseline) — effectively never.

<!-- from P16_sound -->
## [U] Unknowns

1. [U] Exact DirectSound 3D panning/HRTF of `DS3DALG_DEFAULT` on the shipping drivers — resolve by measuring the shipped game; a new engine can use a simple equal-power pan + inverse-distance curve.
2. [U] Whether any Multiwinia path executes Darwinia scripts containing `TriggerSound` (music) — resolve with P19's reachability analysis.
3. [U] Whether `menusounds.dat` contains assets not in `data/mwsounds` (the archive build is in `tools/build_multiwinia_data.bat`, owned by P20/X) — resolve by inspecting the packaging script/archive manifests.
4. [U] Runtime behaviour after a library restart with the software mixer (waveOut buffers not re-primed) — needs a run; not reachable from the shipped UI.
5. [U] Intended meaning of `docs/multiwinia audio.docx` "Lobby: Sounds + music ?" — whether lobby music was ever planned beyond the ambience beds.

<!-- from P17a_ui_framework_editor -->
## [U] Unknowns

| # | Unknown | Resolution |
|---|---|---|
| U1 | Format/reading path of `.mwm` written by "Publish" vs plain text from `NewMapWindow` | P11: `code/mapfile.cpp:MapFile::Save`, `MapFile::MapFile` |
| U2 | `m_gameTypes` single value vs bitmask | P11/P02: readers of `LevelFile::m_gameTypes` |
| U3 | What `CrateCreateButton` `m_reward = −1` does (random reward?) | P08: `Crate` reward selection when `m_rewardId == −1` |
| U4 | Exact visual effect of the `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR` "ink" text over the gradient (verify by rendering) | run the game / P12 |
| U5 | Whether `RGBAColour` default (instant unit with team −1 in editor) is white or black | `code/lib/rgb_colour.cpp` (P13) |

<!-- from P17b_menus_options -->
## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Exact visual change for the `buildingDetail == 1` / `< 3` branches in `Building::RenderPorts`, `LaserFence::RenderAlphas`, `MineBuilding::*`, `SpawnBuilding::RenderAlphas` (only the gating was read) | read those functions in full (P06/P07 own them) |
| U2 | (resolved) `InputScroller::CheckInput` repeats while the mouse is held on the glyph every `INTEGER_INCREMENT_PERIOD` = 0.1 s, change ×5 while `ControlScrollSpeedup` is held, ints clamped by `NetworkInt::Bound` [C] `code/interface/input_field.cpp:InputScroller::CheckInput` | — |
| U3 | (resolved) `DropDownMenu::RegisterInt` wraps the member in a `LocalInt` **reference** and immediately calls `SelectOption(*value)`, so `ScreenResDropDownMenu` populates the refresh list at creation; selections write straight into the window member [C] `code/interface/drop_down_menu.cpp:DropDownMenu::RegisterInt`, `code/network/networkvalue.h:LocalType` | — |
| U4 | (resolved) `LList::operator[]` → `GetData` → `GetItem` returns NULL for an invalid index, so `m_languages[-1]` is NULL and the following `stricmp(NULL, …)` is undefined behaviour (crash) [C] `contrib/systemiv/lib/tosser/llist.cpp:LList::GetItem` | — |
| U5 | Whether the metaserver protocol (P15) carries any leaderboard/TrueSkill data despite the UI stub | P15 review of metaserver message types |
| U6 | Duplicate-key behaviour of `LangTable` (`multiwinia_helppage_3_2`) | P14/P19 `code/lib/language_table.cpp` |

<!-- from P18_ui_gamemenu -->
## [U] Unknowns
- U1: Does the server reject game-option requests from non-host clients? → read P15 `code/network/server.cpp` handlers for `NET_DARWINIA_SETGAMEOPTION*`.
- U2: Which key file (`authkey` or `authkey.dev`) is loaded at start-up, and with what precedence? → P15 `Authentication_GetKey` / app initialisation.
- U3: Could the metaserver inject key `"dy"` into listings (which would make the password filter hide those servers)? → metaserver protocol docs/P15.
- U4: Meaning of the three boolean arguments of `MetaServer_GetServerList` (LAN/WAN/… selection) → P15 `contrib/systemiv/lib/metaserver/metaserver.cpp`.
- (U5 resolved during drafting: a missing phrase id renders as the id itself [C] `code/lib/language_table.cpp:LangTable::LookupPhrase`.)
- U6: Exact visual of the menu transition (slide vs fade, curve) → P12 `Renderer::RenderMenuTransition`.
- U7: Are option requests applied strictly in arrival order (affects contradiction 1)? → P15 server letter processing.
- U8: Semantic difference of Eclipse `m_disabled` vs Darwinia `m_inactive` for click handling → P17a.
- U9: Visual appearance of art assets referenced (mode pictures, thumbnails, icons) → data partition (art not inspected by P18).

<!-- from P19_help_tutorial_script -->
## [U] Unknowns
| # | Unknown | Would be resolved by |
|---|---|---|
| U1 | Case sensitivity of sound-event lookup (`cutscene1` in scripts vs `Cutscene1` in `sounds.txt`); moot in FULL | sound-system partition (`code/sound/soundsystem.cpp`) |
| U2 | That `MAPID_MP_KOTH_2P_1`/`_3P_1` CRCs correspond to the current `mp_koth_2P_1.txt`/`mp_koth_3P_1.txt` (if the file was edited after the CRC was generated the tutorial button would be inactive) | run the map-CRC tool (`tools/`) on the two files |
| U3 | How RadarDish 4 becomes player-owned at the start of Tutorial 2 (no tab is shown until then) | RadarDish ownership rules (P-buildings, `code/worldobject/radardish.cpp`) |
| U4 | Exact runtime resolution of `_kbd`/`_xin` phrase suffixes (affects which help strings appear) | `code/lib/language_table.cpp` (other partition) |
| U5 | Whether `data/mwhelp/background.bmp` is referenced indirectly (e.g. by a data file or eclipse skin) | full-text search of `data/` and `contrib/eclipse` (none found in `code/`) |
| U6 | Visual layout of `AnimatedPanelRenderer` panels and the content of `data/animations/*.anim` | animation/UI partition |
| U7 | Whether CPU team 2 reliably captures spawn 83 in T2 before the player (inferred: AITarget 92 at (1309.43, 1645.08) is 47 units from spawn 83 and gets team-2 priority 1.0 while `!AICanSpawn`; team 2 home = spawn 1 (545.70, 2528.04), team 1 home = spawn 2) — if the player takes 83 first, the FormationAttack/BreakFormation steps are skipped | runtime observation |

<!-- from P20_sequences_loaders_bench -->
## [U] Unknowns
0. (Resolved while drafting — not counted) `TextFileReader::ReadLine` → `TextReader::CleanLine` strips `\n`/`\r` and truncates at `#` (comment) [C] `contrib/systemiv/lib/filesys/text_stream_readers.cpp`; on an empty line `GetRestOfLine()` returns NULL. Consequences: `BenchMark::BuildDxDiag` stops at the first blank line of `dxdiag.txt` — with dxdiag's usual layout (blank line between sections) [I] only `SYS` fields are captured, never `GFX`/`SND`; `CreditsLoader::GenerateBetaTesterList` would crash on a blank line (`strlen(NULL)`) — the shipped `betatesters.txt` has none [C]; a `#` in a name truncates it.
1. [U] Units of `m_receiveRate/m_sendRate` recorded by the benchmark (bytes/s vs bits/s) — P15.
2. [U] Exact appearance of `textures/program_darwinia.bmp`, `speccy_screen.bmp`, `dmacrew.bmp` (only dimensions checked: 256×256 each) — not needed for mechanics; art is IP anyway.
3. [U] Whether `GL_BLEND` is enabled when `CreditsLoader::FlipBuffers` draws its fading clear quad (state inherited from the caller) — determines whether the first 7 s smear. Resolve by tracing GL state at `Script::RunCommand_RunCredits` / outro camera.
4. [U] Exact reachability of `PageDarwinia` (and therefore `CampaignButton`) in FULL — P18 to confirm; affects whether §1.6's bug is player-visible.

<!-- from X1b_asset_formats -->
## [U] Unknowns
1. Exact version of `tools/Rar.exe` (strings are packed; no version resource found) and whether retail `.dat` files were produced with RAR 2.9 or 3.x method numbering — resolve by running `Rar.exe` without arguments on Windows or by inspecting a retail `.dat` header (`animator.dat` shows unpack version 29).
2. Whether the Shaman interface (which loads the RLE8 `textures/shaman_background.bmp`) is reachable in Multiwinia play — resolve with P05/P09 reachability analysis or a run.
3. Whether data-only marker names (`MarkerIsland*`, `MarkerTrunk*to*`, `MarkerFloat*`, …) are looked up via a `sprintf` pattern not caught by the literal grep — resolve with a full call-graph search of `LookupMarker` arguments.
4. `animator.exe` behaviour (key editing, any fields it writes beyond `AnimatedPanel::Write`) — no source in tree; resolve by running the tool.
5. `.psp` / `.grd` internal block contents beyond the header (irrelevant to the game).
6. Content equality of duplicated images (`animations/flag*.bmp` vs `icons/flag*.bmp`, `animations/darwinian.bmp` vs `sprites/darwinian.bmp`) — resolve by hashing.

<!-- from X2b_asset_catalogue_audio -->
## [U] Unknowns

1. [U] Audible content of voice files (intelligible words vs Darwinian gibberish) — audition `data/mwsounds/YesSir*`, `Attack*`, `Retreat*`, `CrateReward_*`, `GDSpeech*`, `GameOver*`, `YouWon`, `YouLost2`.
2. [U] Whether the retail `sounds.dat`/`sounds2.dat`/`menusounds.dat` match this tree's manifests (archives are not in the repository) — list a retail install's archives (`code/lib/resource.cpp` parsing, P14 `scripts/P14_rar_list.py`).
3. [U] Provenance of non-music samples (in-house recordings vs licensed libraries) — affects only the legal note; all are treated as Introversion IP below.
4. [U] Behaviour of objective-less maps (Contradiction 3) and stationary loops (Contradiction 1) at runtime — play-test.

<!-- from X4_level_catalogue -->
## [U] Unknowns
| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Did the shipped retail data contain `levels/level_enum.txt` (e.g. via a patch archive)? | inspect a retail `main.dat` / `patch.dat` |
| U2 | How `mp_benchmark_1` and `mp_assault_stress` were started in the hwcompat / stress builds despite `GameTypes none` | data of those builds, or a launcher/testbed path not in this tree |
| U3 | Contents of `tools/multiwinia_inclusions_ambx.txt` (referenced by `build_multiwinia_data.bat`, absent) | the missing file |
| U4 | Exact tie order inside each menu list (depends on archive index order through `OrderedInsert`) | run `CreateMapList` against a real `main.dat` |
| U5 | Content of the 10 PDLC maps named in `level_enum.txt` | PDLC data (not in tree) |
| U6 | Whether the released game's metaserver sent a demo map list different from the 5-map default | metaserver data / retail logs |
| U7 | Tool used to make the 400×300 thumbnails (no in-engine map-thumbnail writer exists) | art pipeline notes |
