# Collected "unknowns" sections

<!-- from 02_units_and_entities.md -->
### From 02_units_and_entities.md

_Pending — being written._

<!-- from 03_buildings_and_world_objects.md -->
### From 03_buildings_and_world_objects.md

| ID | Question | Tag | What would resolve it |
|---|---|---|---|
| 03-U1 | Exact composition of `Team::m_others` (which entity types count against the PopulationCap used by SpawnPoint, TrunkPort and AntHill `PopulationLocked`) | [U] | read `code/team.cpp` (`Team::NewEntity` / `m_others` insertion) — owned by [05](05_economy_and_population.md) |
| 03-U2 | Whether every building is advanced exactly once per 0.1 s tick (all 10 slices of `NUM_SLICES_PER_FRAME` processed every tick) | [I] | `code/lib/slice_darray.h:GetNextSliceBounds` and the main-loop slice driver ([21](21_simulation_and_entity_model.md)) |
| 03-U3 | Whether the GameTypes-none dev maps (`mp_sandbox`, `mp_sandbox_all`) can be opened from the shipped UI, making ControlTower and Incubator reachable | [U] | map-list construction in the game menu ([09](09_meta_systems.md)) |
| 03-U4 | Multiplayer research level of `TypeLaser` (subversion-laser lifetime) | [U] | callers of `Multiwinia::SetGameResearch` and the MP research table |
| 03-U5 | Origin and lifetime of `Darwinian::m_subversionTaskId` (turns Standard turret shots into subversion lasers) | [U] | crate/task code ([04](04_crates_powerups_and_weapons.md)) |
| 03-U6 | How Darwinians react to `GunTurretTarget` (flee radius and trigger) | [U] | `Darwinian::SearchForThreats` ([02](02_units_and_entities.md)) |
| 03-U7 | Behaviour when a mandatory level-file token is missing (`GetNextToken` returning NULL/empty passed to `atoi`/`iv_atof`) | [U] | `TextReader::GetNextToken` ([29](29_data_formats_reference.md)) |
| 03-U8 | Whether any `GlobalBuilding` records exist during Multiwinia matches (so that `m_isGlobal` touch points are truly inert) | [I] | `GlobalWorld` setup on Multiwinia start ([20](20_architecture.md)) |
| 03-U9 | Whether the six SmokeMarkers of `mp_assault_3p_1` (invisible 13-unit obstacles) sit on walkable routes | [U] | plot marker positions against the map's AITargets/routes or run the map |
| 03-U10 | Whether the Randomiser can hand Darwinians to an unused team slot (`syncrand() % NumPlayers`) and what `SpawnEntities` does for an unused team | [U] | `Location::SpawnEntities` and team initialisation for unused slots |
| 03-U11 | Design intent of dividing PopulationCap by the level's `NumPlayers` rather than by playing teams | [U] | design decision; see [05](05_economy_and_population.md) |
| 03-U12 | Visual/collision impact of the unrotated tree crown sphere (hit centre ignores `m_front`) | [U] | reproduce the generator numerically for sample trees |
| 03-U13 | Effective rocket damage per Darwinian attack (grenades aimed 50 above the rocket base but damage measured to the base) | [I] | simulate `Location::ThrowWeapon` ballistics with grenade bounces and fuse 3 s against the rocket position |
| 03-U14 | Whether the per-tick `ChangeTeam` events of empty solar panels and of level templates are audible | [U] | `data/sounds.txt` bindings for `SolarPanel ChangeTeam` and generic building `ChangeTeam` ([10](10_audio_design.md)) |
| 03-U15 | Exact 10-vs-11-tick pattern of the `GetNetworkTime() > t + 1.0` recount timers | [I] | replay the double accumulation of `m_gameTime += 0.1` |
| 03-U16 | Design intent of the Assault SolarPanel knock-out (ports deleted when captured from the starting team) | [I] | design documentation or play-testing; code behaviour is [C] |
| 03-U17 | Whether shipped `isGlobal 1` MasterSpawnPoints (11 `mp_*` files, visible) differ in behaviour from `isGlobal 0` ones beyond rendering | [I] | `MasterSpawnPoint::Advance` shows the only global-specific branch is single-player; confirm no other reader of `m_isGlobal` for masters |

<!-- from 04_crates_powerups_and_weapons.md -->
### From 04_crates_powerups_and_weapons.md

| Id | Question | Status | What would resolve it |
|---|---|---|---|
| 04-U1 | (resolved by coordinator) Does any shipped map lack sea inside the `Crate::RunNuke` sampling rectangle `[0, X−100) × [0, Z−100)`? **No.** Re-generating every `mp_*` heightmap with the P11 terrain port (`_work/scripts/C_nuke_sea_check.py`, 25-unit sampling grid; results in `_work/drafts/C_nuke_sea_check.csv`) gives a sea fraction between 26.2 % (`data/levels/mp_assault_3p_3.txt`) and the maximum for every map, so the expected number of re-rolls is ≤ 4 and the loop terminates on all shipped maps. The hazard remains for user/edited maps without water. | [I] (port is not bit-exact, but the margin is large) | Bit-exact height dump from a running build would make it [C]; the new game should bound the loop or pre-compute a sea-cell list. |
| 04-U2 | Weighted crate drops accept a position iff the terrain at z ∈ {0, 1} (the `pos.z > 20.0` bug) is non-zero. Which maps accept on the first try (sea-bed ≠ 0) and which always fall back to random (edge height exactly 0)? Weighted drops may also land in water and expire unreachable. | [I] mechanism, [U] per map | Heightmap scan of the z = 0/1 rows per level; instrument `Location::CreateRandomCrate`. |
| 04-U3 | Edge cases at the (0,0) corner: random-mode crates start from the template position (0,0,0) and the Space Ship from (0,0,0); if that corner were land (≥ 20 resp. ≥ 10) and unrestricted, the crate / ship would spawn there at y = 0. | [I] | Heightmap value at (0,0) per level (expected sea). |
| 04-U4 | Design intent of: bad outcomes ignoring `InvalidCrates` and the AntNest limit; Eggs creatures belonging to the task owner while every other monster outcome is Monster-team; Meteor and Plague hitting the user's own units; `InitMonsterTeam(capturer)` no-op. | [U] intent ([C] behaviour) | Designer notes or playtest of the shipped build; decide per item for the new game. |
| 04-U5 | `Randomiser` assigns `syncrand() % levelFile.m_numPlayers` — on a 4-slot map played by 2–3 teams it can create Darwinians on a team id that is unused in this game. What do such Darwinians do (AI, scoring, crate capture)? Same question for magical-forest spirits of unused team ids. | [U] | Trace `Team`/`Location` handling of entities on `TeamTypeUnused` teams; playtest a Randomiser on a 4P map with 2 players. |
| 04-U6 | Monster (4) and Futurewinian (5) Darwinians can capture crates (§1.7). Their teams receive the task rewards — does any AI entity drive those teams' task managers? | [U] | [07](07_ai.md) (AI instantiation for CPU-typed monster/Futurewinian teams, `Location::InitialiseTeam`). |
| 04-U7 | Airstrike and nuke timings on real terrain (simulations assume flat ground). | [I] | Instrument `AirstrikeUnit::Advance` and `Nuke::AdvanceToTarget` in a build on shipped maps. |
| 04-U8 | Real-time durations of Slow Down / Speed Up and the meteor slow-motion assume 10 Hz real-time server ticks; whether `GameTimer::Reset` clears a pending speed request between matches (the routed meta notes suggest `m_requestedSpeedLength` is not reset). | [I] | Read `code/gametimer.cpp:GameTimer::Reset` together with the server tick scheduler ([20](20_architecture.md)); measure in a build. |
| 04-U9 | Are the `GameTypes none` levels (`mp_sandbox`, `mp_sandbox_all`, `mp_benchmark_1`, `mp_assault_stress`) ever selectable in the FULL lobby (their crate rows in §3.5 would then apply with mode-defined options)? | [I] not selectable | Lobby map-list filter ([09](09_meta_systems.md), [08](08_levels_and_level_design.md)). |
| 04-U10 | Can crates drop during the pre-game grace period? `Location::Advance` is not gated by the grace period. | [I] yes | Confirm the grace-period gating of `Location::Advance` in `code/network/clienttoserver.cpp` / [01](01_game_modes.md). |
| 04-U11 | Grenade, rocket and shockwave effective ranges/hit counts (§6.3, §6.6, §6.11) are derived from re-implemented arithmetic on flat terrain. | [I] | Instrumented build; or adopt the formulas as the new game's spec. |
| 04-U12 | (resolved) Tick counts of `x −= 0.1` timers: computed by S04 with IEEE-754 doubles — 1.0 → 11 ticks, 2.5 → 25 (24 active), 5.0 → 51, 15.0 → 151, 45.0 → 450, 60.0 → 600; float `Task::m_lifeTimer` 30 → 300, 60 → 601. Any new implementation should use integer tick counters and decide the intended values. | [C] arithmetic | — |

<!-- from 05_economy_and_population.md -->
### From 05_economy_and_population.md

| ID | Item | Status | What would resolve it |
|---|---|---|---|
| 05-U1 | Does the 5.0-radius neighbour query in `Building::EvaluatePorts` return only Darwinians within 5.0 of the port, or anything in the grid cell (P03 reports grid shortcuts ignoring the requested range)? Affects which Darwinian "occupies" a port and therefore capture. | [U] | Read `code/entity_grid.cpp:EntityGrid::GetNeighbours` (owned by [21](21_simulation_and_entity_model.md)) or instrument a run. |
| 05-U2 | Exact wall latency of the spawn network per map: depends on how many link hand-offs go to a building later in `m_buildings` (same-tick increment) — range 9h + 1 … 10h + 1 ticks. | [I] | Replay `SpawnBuilding::Advance` with each map's building order (ids in load order). |
| 05-U3 | Fate of spirits in `StateHarvested` / `StateCollected` when a Harvester disappears by a path other than its own grenade (task termination, `Team::StopAllTasks`); Harvested has no timer and no owner check, so such spirits may persist forever and occupy slots. Note `Team::Eliminate`'s `ChangeHealth` cannot kill a harvester (invulnerable unless `m_vunerable`). | [U] | Read `code/taskmanager.cpp` task stop/removal for TypeHarvester and the entity removal path; test. |
| 05-U4 | Do Futurewinian (team 5) Darwinians operate spawn-point ports and capture points? If yes, E1 (never-reset `s_numSpawnPoints[6]`) is reachable in shipped play. | [I] (they are ordinary Darwinians, so probably yes) | Runtime test with the space-ship crate on a spawn-point map; read team-5 AI (07). |
| 05-U5 | Design intent of the handicap (binary, diluted by the two special teams, `(mine−1)/total` always 0). Shipped behaviour is [C]; intent unknown. | [U] | Introversion design notes / patch history. |
| 05-U6 | Intent of the per-team cap divisor (level NumPlayers, alliances not pooled) and of the two uncapped co-op maps (`mp_domination_4P_2`, `mp_koth_4P_3`). | [U] | Design notes; play-testing data. |
| 05-U7 | Whether the doubled reinforcement rate on `mp_blitzkrieg_2P_3`, `2p_1`, `2p_2` was noticed and balanced around. | [I] unintended | Release notes / balance discussions. |
| 05-U8 | Engineer can target a spirit that is `Harvested`/`Collected`/`InEgg` (`Engineer::AdvanceToSpirit` rejects only Death/Attached) → one slot referenced by two collectors; combined with the `while (m_spirits.Size() > 0)` drop loops this could hang (E8). Reachability unknown. | [I] | Targeted test: engineer and harvester on the same soul field. |
| 05-U9 | Whether player selection/orders can pull an operating Darwinian off a port (would let a player "un-man" a point deliberately). | [I] | [06](06_player_control_and_input.md) selection rules / test. |
| 05-U10 | `Virii::s_viriiPopulation` returns to 0 between games because `Team::~Team` deletes `m_others` and `~Virii` decrements — holds only if every constructed Virii had `Begin()` called (always true via `SpawnEntities`). | [I] (resolves meta U3 of the creatures agent) | Confirm no `new Virii` outside `Entity::NewEntity` + `SpawnEntities`. |
| 05-U11 | Spawn Mania exact request count per point (50 or 51) depends on whether the crate is captured in a building slot before or after the point in the same tick. | [I] | Instrumented run. |
| 05-U12 | Harvester throughput figure (≈ 2.5 Darwinians/s for a 500-unit round trip) is a back-of-envelope estimate; real movement uses turning and acceleration filters. | [I] | Simulate `Harvester::AdvanceToTargetPos` on a map. |

Resolved in this document (previously open): meta U3/U6 composition of `m_others` (§4.1, [C]); meta U4 harvesters collect enemy souls (§5.3, [C]); P06 "each building once per tick?" (§8.1, [C] — yes); P06 "GetNetworkTime vs GetGameTime" — `GetNetworkTime` is a macro for `g_gameTimer.GetGameTime` [C] `code/gametimer.h`; per-team cap vs teams in play (§4.2: empty slots are CPU-filled, disconnected humans become CPU with a new AI entity [C] `code/multiwinia.cpp:Multiwinia::RemoveTeams`).

Contradictions resolved by re-reading code: (1) retribution called vs never called (§6.9); (2) destroyed ant hills releasing monster spirits (§5.1); (3) spore generators creating spirits regardless of team (§5.1); (4) handicap "other used teams" omitting the two special teams (§3.2); (5) spawn-link latency "exactly 1.0 s" vs 11/10 ticks (§3.4); (6) double-run step "6 ticks" vs 5.5 ticks / 330-tick waves (§6.4); (7) P03 labelling Engineer spirit collection and egg fertilisation "legacy" — both are live in [B:FULL] via crates (§5.3); (8) P05/P03 "harvester enemy souls [I]" → [C].

<!-- from 06_player_control_and_input.md -->
### From 06_player_control_and_input.md

| # | Item | Kind | What would resolve it |
|---|---|---|---|
| 06-U1 | Exact XInput dead-zone / trigger-threshold values compiled into the shipped exe (`XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE`, `…_RIGHT_THUMB_DEADZONE`, `XINPUT_GAMEPAD_TRIGGER_THRESHOLD`; believed 7849 / 8689 / 30) — headers are not in the tree | [U] | the DirectX SDK version referenced by `targets/vs2005/Darwinia.vcproj` include paths, or disassembly of the shipped binary |
| 06-U2 | Result of `BYTE = floor(trigger·6.3)` for raw > 40 under MSVC 2005 (wrap, saturate or undefined) — affects trigger magnitudes (X14) | [U] | compile the expression with VS2005 `/fp:precise` or read the generated code |
| 06-U3 | Whether `UnicodeString` → `std::string` assignment in `W32InputDriver::getInputDescription` yields ASCII (assumed by X15) | [U] | read the conversion operators in `contrib/systemiv/lib/unicode/unicode_string.h` |
| 06-U4 | Whether route ids diverge between peers because route-preview lines share the routing free list (X3), and whether that matters (route ids hashed or compared?) | [U] design-critical | two-client instrumented run comparing route ids after identical orders; check whether route ids enter the sync value ([22](22_determinism_and_networking.md)) |
| 06-U5 | Whether any camera-state reader inside `code/worldobject/*` (frustum/distance LOD in 73 files) influences simulated state; one sim-side use found in the Darwinia-only `ScriptTrigger` | [U] | determinism audit of the listed files ([22](22_determinism_and_networking.md)) |
| 06-U6 | Whether the extra `InputManager::Advance` at the bottom of `AppMain`'s top-level loop can swallow an edge between sub-loops | [U] | instrumented run logging edges across a menu → location transition |
| 06-U7 | Practical frequency of the X1 desync (a player holding F2/R3 look-around while another team's squad is processed without tracking) | [I] design-critical | two-client test: one player holds F2 while another's squad moves/fires; compare sync values |
| 06-U8 | InsertionSquad secondary weapon is always the grenade in Multiwinia (no reachable `SetWeaponType` caller) | [I] | runtime confirmation; full call-graph search for `SetWeaponType` ([02](02_units_and_entities.md)) |
| 06-U9 | Whether an `ORDERDGS` can execute with zero controlled Darwinians (NaN route start, X23) in normal play | [I] | instrumented run: select, let the group die, right-click within the same 0.1 s tick |
| 06-U10 | Entity-track of an officer/armour (stale orbit centre) is reachable only by selecting one while already tracking a squad | [I] | gamepad play test |
| 06-U11 | §12 "Use in FULL" classification of front-end windows (leaderboards, profiles, file dialogs counted as legacy) | [I] | window-reachability graph in [09](09_meta_systems.md) / [28](28_ui_framework.md) |
| 06-U12 | Return value of `Resource::GetTexture` for the missing `icons/compass_empty.bmp` (X30) and its on-screen effect | [U] | read `code/lib/resource.cpp:Resource::GetTexture` ([25](25_textures_and_assets.md)) |

Resolved during synthesis (no longer open): P01-U5 (no shipped map contains a FenceSwitch, §1.6), P01-U8 (`TERMINATE` with PROGRAM 255 kills the entity via `ChangeHealth(−999)`, §1.4), P03-U1 (`m_underPlayerControl` cleared by `Team::SelectUnit`, §2.3), P04-U3 (`m_currentMousePos` travels in `ALIVE`, §6.2), P04-U6 (keyboard squad controls: hold LMB move, hold RMB lasers, RMB + LMB grenade, §6.3), P09-U1 (selection-circle fields are UI-only, §2.1), P09-U2 (particles never feed back into simulation, §3.6), P09-U3 (`RenderWeaponMarker`, `Task::Demote`, `GetNextTaskId` have no callers, §3.6–§3.7, §8.10), P09-U4 (`RestrictionZone::IsRestricted` semantics, §3.6), P10-U6 (keyboard entity tracking only via a hand-edited `AutomaticCamera = 2`, §9.6).

Contradictions resolved by re-reading the code (11): (1) control scheme selectable vs forced (P01/P10 vs `PrefsManager::Load`) → forced to RTS; (2) rebinding reachable (P10) vs unreachable (P17b) → unreachable; (3) "game over → ToggleReady returns to menu" live (P09) vs testbed-only (P01) → `TESTBED_ENABLED` only; (4) officer terminate "demote" (P01, code comment) vs kill (P09) → `ChangeHealth(−999)` kill; (5) armour order flags (P01 wording) → plain RMB = move (`SetWayPoint`), Shift = move-and-deploy (`SetOrders`), FORCEONLYMOVE = !deploy; (6) armour orders marker registered by the local team in `code/location.cpp` (P08) → that call is commented out, only the network handler registers; (7) stick output range ±39/±19 (P10) → asymmetric [−40, 39] / [−20, 19] because of `floor`; (8) `m_cursorHighlight` size 30 (P09) → default 20, set by callers; (9) camera census "88 level files" (P10) → 87 top-level files (+30 under `data/levels/old/`); (10) `NEXTWEAPON`/`PREVWEAPON` listed with senders (P15) → `RequestNextWeapon` has an empty body and no `RequestPreviousWeapon` exists, never sent; (11) pad bindings for officer orders (P09 "special = X toggles") → A = walk, X = GOTO because of the gamepad inversion.

<!-- from 07_ai.md -->
### From 07_ai.md

| Id | Item | Status | What would resolve it |
|---|---|---|---|
| 07-U1 | How often the selection gate in `Task::TargetOfficer` stalls AI officer creation and how many orphan Officer tasks accumulate (B13, B24) — and therefore how often CPU teams lose crate rewards to full slots | [I] mechanism confirmed in code, impact unmeasured | instrumented runs on every shipped map logging each CPU team's task list, `m_currentEntityId/UnitId/BuildingId` and promotion attempts |
| 07-U2 | Whether the stale `m_mostRecentTaskId` path (B12) ever fires a pending power-up (Meteor, Nuke, Air Strike) at the AI's own Darwinians | [I] | same instrumentation, forcing full task slots with a pending power-up |
| 07-U3 | Whether the retail Multiwinia build's AI matches this source release (patch level of `f909630` vs the last shipped patch) | [U] | compare against retail binary behaviour/replays, or version strings |
| 07-U4 | Whether every shipped map has at least one sea sample (height ≤ 0) inside [0, worldSize − 100)², otherwise `Crate::RunNuke` hangs (B1) | [U] | heightmap scan of every `mp_*` landscape definition ([23](23_terrain_and_landscape.md)) |
| 07-U5 | Practical effect of the fence/wall-blind initial graph (B9) on the seven maps with fences and no turrets before the first rebuild | [I] | offline comparison of the initial and rebuilt link sets per map; observe opening minute |
| 07-U6 | Whether an officer's `m_targetFront` controls where its formation fires, i.e. whether B15 (defenders facing away) changes combat outcomes | [U] | read formation firing in `code/worldobject/officer.cpp` / `code/worldobject/darwinian.cpp` ([02](02_units_and_entities.md)) |
| 07-U7 | Whether hostile units, turrets or ants can select the immortal AI entity at (20, 0, 20) as a target (B22) and get stuck on it | [U] | inspect target filters of `EntityGrid::GetBestEnemy` callers and creature AIs for entity-type checks |
| 07-U8 | How often isolated own nodes occur (NaN priorities, B14) in shipped graphs | [I] | offline graph build per map with the §4.2 rules |
| 07-U9 | Whether the host can switch `RetributionMode` off in the lobby (P18 lists it as a "−1 → checkbox" option) — governs how often eliminated CPU teams get retribution powers | [I] default on | read the Advanced Options page builder ([09](09_meta_systems.md)) |
| 07-U10 | Whether any shipped Assault defence marker is unbound, has `scanRange > 0` and sits where no defender stands, so the B17 team-0 default makes its objective fall back at once | [U] | offline marker/bound-building analysis of the 8 Assault maps plus a run per map |
| 07-U11 | Whether `Team::m_specials` can hold ids of dead entities (would make `AI::RunForest` crash, B18, contrary to the changelog's "fixed") | [U] | read `Team::m_specials` maintenance ([21](21_simulation_and_entity_model.md)) |
| 07-U12 | The AI entity's slice (index 0 assumed) and the one-tick ordering between AI decisions and node recounts | [I] | read `Team::NewEntity` and `SliceDArray::GetNextSliceBounds`; only affects one-tick latency |

<!-- from 20_architecture.md -->
### From 20_architecture.md

<!-- SU -->

<!-- from 23_terrain_and_landscape.md -->
### From 23_terrain_and_landscape.md

Unknowns:

- **23-U1** [U] — Bit-exactness of the Python port (and hence of this specification) against `multiwinia.exe`. Evidence so far is indirect (§17.5). *Resolve*: instrument or patch a build of the original to dump `Landscape::m_heightMap` after `Landscape::Init` for several maps (at least `mp_koth_2P_1`, `mp_assault_2p_3`, `mp_rocketriot_2p_3`, `mp_skirmish_3P_1`, `mp_blitzkrieg_2p_1`) and compare with `S_23_terrain_oracle.py dump` / the digests of §18.
- **23-U2** [U] — Accuracy of the MSVC 2005 static CRT's `pow`/`exp`/`log` (x87 and SSE2 code paths), in particular whether `pow(x, 2.0)` is exact for the 17 exact-tie arguments of `mp_assault_2p_3` tile 5. *Resolve*: a test program linked against the VS2005 `/MT` CRT, run on SSE2 and non-SSE2 CPUs, evaluating the logged arguments (`S_23_terrain_oracle.py` can be extended to print them).
- **23-U3** [U] — Whether the main thread's sound-system `AppRandom` draws during level loading actually interleave with the colour chain of §11.6 (cosmetic: terrain colours varying between runs/peers). *Resolve*: log `AppRandom` calls with thread ids during a level load.
- **23-U4** [U] — MSVC 2005's conversion of doubles ≤ −1 to `unsigned short` (assumed: truncate to 32-bit int and keep the low 16 bits). Irrelevant to the shipped maps (§7.4, §17.4) but needed to predict user-made maps. *Resolve*: disassemble `SurfaceMap2D<double>::GetValue` and `Landscape::MergeTileIntoLandscape` in the shipped executable.
- **23-U5** [U] — Terrain equality between the PC and the maintained Mac build (different compiler and CRT). *Resolve*: dump heights from a Mac build and compare digests.

Design-critical inferences:

- **23-I1** — x87 code at 53-bit precision under `/fp:precise` evaluates the generator exactly like IEEE double without FMA (§3, §17.3); the port depends on it.
- **23-I2** — No map-content check between peers before a match (§17.1); a modified local map desynchronises silently. Confirm against the start-game handshake in [22](22_determinism_and_networking.md).
- **23-I3** — `fd` acts as a roughness exponent (larger = smoother) and the gain modifiers approximately normalise amplitude across `fd` and `ls` (§5.4); intent is inferred from the formulas only.
- **23-I4** — Generation method 2's directional bias (D/S never chosen, C/N almost never) and the smooth flatten type's 5-terms-over-6 divisor are arithmetic facts [C]; whether designers relied on them deliberately is unknown. A new editor should offer unbiased variants and keep these only for importing legacy parameters.
- **23-I5** — World-edge wrap artefacts (§9.3) only affect underwater heights in shipped maps; gameplay never sees phantom land at the edges.

<!-- from 24_rendering.md -->
### From 24_rendering.md

| # | Item | Tag | What would resolve it |
|---|---|---|---|
| 24-U1 | Does `ReleaseDirect3D` compile? B1 (`water.cpp:119`) and B2 (`WindowManagerDirectX::CreateWin` signature) are ill-formed C++; B3 (`IDirect3DDevice9::Resolve`, `D3DRESOLVE_RENDERTARGET0`) and B4 (`D3DRS_LINEWIDTH`) depend on the SDK headers | [I] | build the configuration with the DirectX SDK named in `targets/vs2005/Direct3D.vsprops`; inspect `d3d9.h`/`d3d9types.h` |
| 24-U2 | What D3D players saw on dynamic water in Multiwinia (the `water-meshwaves` shader samples a reflection target that is never rendered because the pre-pass is single-player-only) | [U] | run a D3D build in a match and capture the frame |
| 24-U3 | Register allocation of FixedFunc/deform/water uniforms without `register()` and of samplers `map0`/`map1` | [U] | disassemble with `fxc /Fc` for the profile returned by `D3DXGetVertexShaderProfile` |
| 24-U4 | Whether D3D9 exposes > 1 mip level for `D3DUSAGE_AUTOGENMIPMAP` managed textures (does `MipMapTask` do any work?) | [I] | runtime `GetLevelCount()` on target hardware |
| 24-U5 | Whether `gluPerspective`'s `_43 *= 2` (effective near/far doubled under D3D) was intended | [U] | compare GL and D3D builds near the near plane; author intent unavailable |
| 24-U6 | Real per-frame cost, draw-call count and fill rate of the shipped build (no budgets are encoded) | [U] | profile `multiwinia.exe` with the built-in profiler and a GPU capture tool on a 4-player map |
| 24-U7 | Actual swap interval of the GL build (no `wglSwapIntervalEXT` call; driver default) | [U] | runtime observation on target drivers |
| 24-U8 | Which texture flags win for textures requested with different flags (texture cache keyed by name only): `textures/clouds.bmp` (blobby: no mip; flat: default mip), `textures/laser.bmp` (sky: mip; lasers: no mip) — first requester fixes mip-mapping and masking for the session | [I] | log `Resource::GetTextureWithFlags` call order at start-up (owner [25](25_textures_and_assets.md)) |
| 24-U9 | Whether the `docs/HardwareComplianceIssues.txt` "mouse pointer background not transparent" issue still applies (Linux only; Win32 cursors are additive so black is invisible) | [I] | test on a driver without multitexture/alpha support |
| 24-U10 | Visible difference of flat-shaded colours between GL (last provoking vertex) and D3D (Gouraud for unconverted triangles, first vertex for D3D flat shading) | [I] | side-by-side capture of explosions and shapes in both builds |
| 24-U11 | Hangul atlas `square.bb00.bfff.bmp` claims U+BB00–BFFF; U+BC00–BFFF may sample outside the atlas depending on the previous character | [I] | render a Korean string containing U+BC00–BFFF characters |
| 24-U12 | [B:D3D] `DeformEffect::Stop` never restores render target 0 before the full-screen composite — composite would go into the deform texture | [I] | moot unless the D3D build is revived; trace with PIX |
| 24-U13 | [B:D3D] x87 precision after `CreateDevice` without `D3DCREATE_FPU_PRESERVE` (24-bit) and its effect on lockstep between GL and D3D clients | [I] | `_control87(0,0)` readback after `Direct3DInit` (owner [22](22_determinism_and_networking.md)) |
| 24-U14 | Lights specified before the camera matrix each frame — magnitude of the one-frame lag in light direction during fast camera turns | [I] | capture a fast camera rotation; compare with lights set after `SetupMatricesFor3D` |
| 24-U15 | Design intent of `textures/triangleOutline.bmp` (2×2 sub-cells, three diagonals) mapped once per height-map cell versus the actual triangle edges of the strip | [I] | visual check in a running build (owner [23](23_terrain_and_landscape.md)) |

Resolved during synthesis (no longer open): special-lighting levels (`mp_koth_2P_3`, `mp_koth_2P_3b`, `mp_koth_3P_1`, §5.5); `g_predictionTime` semantics (§2.4); background/fog colour (constant black, §3.2); landscape D3D path does not restore the emulator vertex declaration — `Water::RenderFlatWater` works around it by resetting `s_pCustomVertexDecl` [C] (§4.2); visual content of `StartSequence` is moot (never instantiated, §3.2); `sprites/viriifull.bmp` is generated at run time (§4.9).

<!-- from 27_audio_engine.md -->
### From 27_audio_engine.md

| Id | Item | Class | What would resolve it |
|---|---|---|---|
| 27-U1 | Exact panning / HRTF and distance roll-off applied by DirectSound for `DS3DALG_DEFAULT` software buffers on shipping drivers (rolloff and max distance are left at defaults: 1.0 and 1e9 [I]) | [U] | Measure the shipped game (sweep a test source around the listener) or confirm with DirectSound documentation per driver; a new engine can use equal-power pan + `minDistance/d` and add a max distance |
| 27-U2 | Is the music channel reachable at all in Multiwinia? Every `TypeMusic` trigger is Darwinia script content ([B:LEGACY]) or `USE_LOADERS` code ([B:DEAD]); if so the music system, music buffer and music callback are idle in every match and in the front end | [I], design-critical | Confirm in [30](30_scripting_tutorial_and_cutscenes.md) that no Multiwinia path runs `RunScript` on a `TriggerSound` script; runtime trace of `SoundSystem::m_music` |
| 27-U3 | Selecting the software mixer (hand-edited `SoundLibrary`) dereferences the uninitialised `SoundLibrary3d::m_profiler` in `GetChannelData` / `ApplyDspFX` / `Callback` (`PROFILER_ENABLED` is off) → probable crash; the Sound Profile window reads the same pointer | [I] | Run the baseline binary with `SoundLibrary = software` |
| 27-U4 | After a software-mixer library restart the new waveOut device gets no primed buffers (function-static counter in `SoundLib2dBuf`), so it may stay silent | [I] | Runtime test (Apply in the options screen with the software library) |
| 27-U5 | Attached (`Type3DAttachedToObject`) sounds are hard-cut, without release, on the first tick after their emitter leaves the location, although `Update3DPosition` contains a graceful "become stationary + final release" path that is almost never reached. Which behaviour was intended? | [I], design-critical | Designer intent / listening test; recommendation: graceful release |
| 27-U6 | `DSound_I3DL2Reverb` quality is passed as a type-punned float and probably always becomes 0 | [I] | Run with `SoundDSP = 1` and query `IDirectSoundFXI3DL2Reverb8::GetQuality`; moot while DSP is off |
| 27-U7 | `EnableDspFX` reads `results[i]` with the full-list index while `SetFX` filled only the compacted DirectX entries; a sound mixing an in-house type before a DirectX type may trip the release-assert | [I] | Run with `SoundDSP = 1` on a block with mixed effects; moot while DSP is off |
| 27-U8 | `MonophonicNearest` emitter selection is seeded with 99999.9 on **squared** distance, so a fresh instance with no emitter within ≈ 316.2 wu of the camera is culled on its first tick: `Darwinian Create`, `Darwinian Attack` and `EscapeRocket Unhappy` would be inaudible at typical RTS camera distances | [C] mechanism, [I] audible effect, design-critical | Play-test at several camera heights and log culls; decide the intended audible radius for crowd sounds |
| 27-U9 | Start latency on the live path depends on the DirectSound play-to-write cursor gap of each driver (`ResetChannel` writes from the write cursor) | [U] | Measure `GetCurrentPosition` play/write gap on representative hardware |
| 27-U10 | Smoothed `UpdateOncePerLoop` parameters start from the constructor value (FREQUENCY 1.0) and get only 2–3 evaluations before playback, compressing authored random pitch ranges toward 1.0 (e.g. 0.75–1.35 becomes ≈ 0.88–1.16 at note start). Intended? | [I] | Listening comparison against the authoring intent; designers' notes |
| 27-U11 | In-house DSP time constants use `SoundMixFreq` (default 22050) while channel data runs at the source rate (44.1 kHz): Echo/Reverb times halved, Gargle rate doubled | [I] | Run with `SoundDSP = 1` and measure an echo; moot while DSP is off |
| 27-U12 | Case-insensitive resolution of lower-case sample stems against mixed-case names inside the `.dat` archives | [I] | Verify archive name matching in `code/lib/resource.cpp` ([31](31_platform_build_and_tools.md)) |
| 27-U13 | `SoundLibrary2dWin32::m_fillsRequested` is incremented on the driver thread and decremented on the main thread without synchronisation | [I] | Code review / stress test of the software path; atomic counter in any re-implementation |
| 27-U14 | Loop keep-alive polling by game code (`NumInstances(m_id, …)` in `code/worldobject/rocket.cpp`) cannot see an instance's emitter until a sound tick has run `GetAttachedObject`, and never sees 2D/stationary instances, so loops may be re-triggered in duplicate (§4) | [I], design-critical for Rocket Riot audio | Runtime trace of `m_sounds` during a Rocket Riot match; a new engine should record the emitter id at trigger time |

<!-- from 28_ui_framework.md -->
### From 28_ui_framework.md

| # | Item | Kind | What would resolve it |
|---|---|---|---|
| 28-U1 | Exact on-screen look of "ink" text (`SRC_ALPHA, ONE_MINUS_SRC_COLOR`) over the Darwinia highlight gradient and the fill boxes | [I] (blend arithmetic only) | render a captured frame on the target build (or run the GL path in a reference renderer) |
| 28-U2 | Content of the frame captured by `InitialiseMenuTransition` (`glCopyTexImage2D` from the read buffer during the update phase, i.e. after the previous `Flip`): previous frame or undefined back-buffer contents depending on the driver's swap behaviour | [U] | test on several GL drivers / read `WindowManagerWin32::Flip` pixel-format flags (swap copy vs exchange) |
| 28-U3 | Whether the shipped SKUs include the partial `japanese`/`korean`/`portuguese` language files (161–221 keys each); if they do, choosing them shows raw phrase ids for most of the UI | [I] design-critical for localisation QA | inspect the retail `language.dat` per SKU (`tools/build_language_dat.bat` + `tools/*language.list`, [31](31_platform_build_and_tools.md)) |
| 28-U4 | Whether committed IME text reaches `InputField` as `WM_CHAR` units (no `WM_IME_*` handling exists) | [I] | run the game with a Japanese/Korean IME |
| 28-U5 | Whether the server rejects lobby option requests from non-host clients (the UI only hides controls, §4.12) | [U] (shared with 09/22) | `code/network/server.cpp` handlers for `NET_DARWINIA_SETGAMEOPTION*` |
| 28-U6 | Consequence of F21 (server-row navigation entries removed at the wrong index) in real browsing sessions | [I] | instrument `UpdateJoinGamePage` or step through with a shrinking list |
| 28-U7 | Whether `EclUpdate` skipping a window when another window is removed during the same iteration (§2.9) ever drops a frame of navigation in practice | [I] | trace `EclUpdate` with drop-down windows that self-remove in `DropDownWindow::Update` |
| 28-U8 | Whether any window in a shipped flow is opened while the Eclipse screen size is stale (screen size is only refreshed in `AdjustWindowPositions`) | [I] | audit every screen-mode change path (`RestartWindowManagerAndRenderer`, `UnlockFullGameButton`, `SetWindowed`) |
| 28-U9 | Whether a demo-key player whose demo limitations forbid hosting can still host: `App::HostGamePermitted()` is consulted only to set the no-op `m_disabled` flag on the Host button (F17), so the UI does not block it | [I] design-critical (demo gating) | check `App::StartMultiPlayerServer` / `ClientToServer::GetDemoLimitations` and the metaserver side ([22](22_determinism_and_networking.md), [09](09_meta_systems.md)) |

<!-- from 31_platform_build_and_tools.md -->
### From 31_platform_build_and_tools.md

| Id | Question | Tag | What would resolve it |
|---|---|---|---|
| 31-U1 | Does `ReleaseDirect3D` compile at this commit, or was it built from another branch/with uncommitted fixes? Four breakages found (§7.9) | [I] strong "no" | build `ReleaseDirect3D` with VS2008 + DirectX SDK March 2008 |
| 31-U2 | Are `D3DRS_LINEWIDTH` and `IDirect3DDevice9::Resolve`/`D3DRESOLVE_RENDERTARGET0` absent from every PC DirectX 9 SDK referenced by `Common.vsprops`? | [I] | inspect `d3d9.h`/`d3d9types.h` of the SDK versions listed in §7.2 |
| 31-U3 | Exact x87 control word on the main thread after D3D9 `CreateDevice`/`Reset` in `[B:D3D]` (expected 24-bit precision) | [I] | read `_control87(0,0)` after device creation at run time |
| 31-U4 | Behaviour of `throw;` with no active exception under the VS2008 static CRT inside `AppMain`'s `__try` (SEH 0xE06D7363 reaching `RecordExceptionInfo`, or `terminate()` → `abort()` bypassing it) | [U] | trigger an `AppReleaseAssert` in a Release build |
| 31-U5 | Which authentication-key file the retail build read (`authkey.dev` here; installers and the support INI know only `authkey`), and whether retail used `metaserver-mwdev` or a production host | [U] | inspect a retail executable/install |
| 31-U6 | Behaviour of the absent tools: `autopatch.exe`, `copy_assist.exe`, `ProfileLocator.exe`, `ncftpput.exe`, `tools/build_ambx_dat.bat`, `tools/convert_bmp_to_tex.cmd`, `tools/macosx/add-appcast-url.rb`, `tools/macosx/add-svn-revision.rb`, `tools/PasswordGenerator/main.cpp`; in particular whether the auto-patcher writes `userprefs.txt`, `game-history.txt`, `stdout.txt` and the `backup_*`/`revert_*` folders | [U] | obtain the release-branch `tools/` |
| 31-U7 | Contents and producers of `ambx.dat`, `branding.dat`, `patch.dat` | [U] | retail media / patch archives |
| 31-U8 | Was `default_preferences.txt` ever shipped loose (or by a patch) so that first-run defaults applied in packaged installs (§8.9)? | [U] | retail install tree |
| 31-U9 | Exact role of `tools/SlashSort.exe` | [I] | run it on a `TRACK_MEMORY_LEAKS` log |
| 31-U10 | Which string the Russian build's `subst.pl … retail\... retail.ru` patch targeted (absent from this tree) | [U] | diff a Russian retail exe against an English one |
| 31-U11 | Is the debug menu (hence the Grabber window and profiler) reachable from the shipped FULL build's input bindings? | [U] | see [32](32_debug_test_and_performance.md)/[06](06_player_control_and_input.md) binding analysis |
| 31-U12 | Name of the map file the baseline `Release` link produces (linker default) — `tools/traceblackbox.bat` expects `darwinia.map` | [I] | link the Release configuration |
| 31-U13 | Is QPC monotonic across the main (0xAA) and worker (0x55) affinity sets on the 2008 target hardware? Cross-core skew would surface directly in `GetHighResTime` (§4.1) | [I] | measure on period hardware; irrelevant for the new engine |
| 31-U14 | Was the shipped Windows binary built from this commit? (`DARWINIA_VERSION "dev"`, development metaserver host, versionless `.rc`) — design-critical because every [B:FULL] claim assumes it | [I] probably a development snapshot close to release | compare behaviour/strings with a retail `multiwinia.exe` |
| 31-U15 | Did the Mac 1.3.1 port play lockstep games with Windows clients bit-exactly (its `-mfpmath=387`/`-mno-fused-madd` flags suggest the intent)? | [U] | cross-platform network test with period binaries |

Design-critical inferences (not unknowns, but [I]): GL and D3D clients in one game are a desync risk (§7.10); no map-content check between peers (§10.3); worker-thread crashes bypass the crash reporter (§6.2); a hand-typed authentication key is not reloaded at the next start (§5.3); first-run defaults ignore `default_preferences.txt` in packaged installs (§8.9).
