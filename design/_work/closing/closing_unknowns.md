# Collected "unknowns" sections

<!-- from 00_overview.md -->
### From 00_overview.md

Only the unknowns that change what the game *is* at overview level; each links the document that owns it.

| ID | Question | Tag | What would resolve it | Owner |
|---|---|---|---|---|
| 00-U1 | Did the retail (Steam / Xbox Live) build have a real achievement layer replacing the PC stubs, and were achievements persisted? | [U] | the retail branch or binary; not in this tree | [01](01_game_modes.md#open-questions-and-unknowns) 01-U4, [09](09_meta_systems.md#open-questions-and-unknowns) 09-U1 |
| 00-U2 | Is the 11-tick "second" intended pacing or an accident that the balance was then tuned around? | [I] accident; balance tuned under it | design decision for the new game | [01](01_game_modes.md#open-questions-and-unknowns) 01-U2 |
| 00-U3 | What was the handicap meant to do (binary, diluted by the two special teams)? | [U] | design notes or patch history | [05](05_economy_and_population.md#open-questions-and-unknowns) 05-U5 |
| 00-U4 | Intended per-team cap divisor (level `NumPlayers`, alliances not pooled) and why two co-op maps are uncapped | [U] | design notes; play-testing data | [05](05_economy_and_population.md#open-questions-and-unknowns) 05-U6 |
| 00-U5 | Intended role of the monster and Futurewinian teams in win checks (inconsistent across modes) | [I] | design intent not recorded | [01](01_game_modes.md#open-questions-and-unknowns) 01-U10 |
| 00-U6 | Was multi-round Assault (the manual's "switches around") ever enabled in a shipped patch? | [U] | later source or patch notes | [01](01_game_modes.md#open-questions-and-unknowns) 01-U11 |
| 00-U7 | How the external metaserver / matchmaker behaved (listing, demo restrictions, NAT relay) — its server side is not in the tree | [U] | metaserver source or packet capture | [09](09_meta_systems.md#open-questions-and-unknowns) 09-U11, [22 §12](22_determinism_and_networking.md#12-lobby-and-server-discovery-protocol) |
| 00-U8 | What the retail `patch.dat` and `branding.dat` archives contained (they are loaded but no build list exists; they could override shipped data or supply assets the tree lacks) | [U] | retail install contents | [31](31_platform_build_and_tools.md), [25](25_textures_and_assets.md) |
| 00-U9 | Whether `default_preferences.txt` is applied on first run of a packaged install (it lives in `main.dat`, read before archives load) | [I] | first launch of a packaged build | [09](09_meta_systems.md#open-questions-and-unknowns) 09-U4 |
| 00-U10 | Was lobby or match music ever planned beyond the ambience beds? | [U] | designer interview; moot for the new game | [10](10_audio_design.md#open-questions-and-unknowns) 10-U12 |

<!-- from 01_game_modes.md -->
### From 01_game_modes.md

| Id | Question | Status / current best reading | What would resolve it |
|---|---|---|---|
| 01-U1 | Does the lobby keep the map's PlayTime or the mode default TimeLimit (§3.5, B8)? | [I] usually PlayTime: the default re-request normally carries the same option version and is discarded; default wins only if the server echo arrives between the two frames | instrumented host session logging `GameOptionVersion` for `GAME_PARAM[TimeLimit]` around a map click |
| 01-U2 | Is the 11-tick "second" (B1) intended pacing or an accident? | [I] accident of `t −= 0.1; if (t <= 0)` in double; but every shipped balance value was tuned under it | design decision for the new game (keep 1.1 s or re-tune) |
| 01-U3 | How long does a besieged rocket survive (attackers throw grenades, damage ×0.2, decay 0.33/s)? | [U] needs grenade damage on buildings | combine [04](04_crates_powerups_and_weapons.md) grenade damage with [02 §4.4](02_units_and_entities.md#4-darwinian) AttackingBuilding rate; or measure |
| 01-U4 | Did the retail (Steam) build have a real achievement layer replacing the stubs? | [U] not in this tree | retail binary / Steamworks integration |
| 01-U5 | Effect of the out-of-bounds `m_teams[255]` read on a Blitzkrieg draw (B11) | [I] reads unrelated `Multiwinia` memory; result only feeds a stub | debugger on a Blitzkrieg draw with SuddenDeath off |
| 01-U6 | Can attackers flip a detonation into a win by completing station capture during the pulse wave (B19)? | [I] code allows it; the wave kills non-friendly Darwinians, so rare | in-game test on a small Assault map |
| 01-U7 | Can a rocket in Flight be destroyed by air strikes or nukes (B20)? | [I] code allows it | in-game test |
| 01-U8 | Exact distribution of 10- vs 11-tick recount periods for zones, flags and statues | **Resolved** [C arithmetic]: the period depends on the re-arm time, not on a fixed ratio (10 ticks for τ ∈ [0.4, 3.0), [15.5, 63.2), [255.4, 1023.0), ≥ 4095.5 s at speed 1.0; 11 otherwise), see Conventions and [03 §1.2](03_buildings_and_world_objects.md#12-time-base) (03-U15) | — (exact replay of `code/gametimer.cpp:GameTimer::Tick`) |
| 01-U9 | Whether the Domination co-op loop bound (B13) can end a game early on shipped co-op maps | [I] only if a hostile team sits in a slot ≥ totalTeams | enumerate slot layouts for `mp_domination_4P_1/4P_2` |
| 01-U10 | Intended behaviour of special teams (monster, Futurewinian) in win checks — they block Domination's "last team standing" and can win KOTH and Blitzkrieg time-outs | [I] inconsistent by mode (§5) | design intent not recorded; play-test frequency of Futurewinian arrivals |
| 01-U11 | Was multi-round Assault (manual: "play then switches around") ever enabled in a shipped patch? | [U] no control in this tree; `m_assaultSingleRound` fixed true | later source or patch notes |
| 01-U12 | Do sudden-death losers (wiped by `Team::Eliminate`, spawn points kept) realistically recover before the tie breaks? | [I] possible in principle | play-test |

Resolved during synthesis (no longer open): P02-U1 port occupancy → [03 §3.6](03_buildings_and_world_objects.md#3-building-base-class); P02-U4 capacity of an eliminated team = 2 ([06 §3.5](06_player_control_and_input.md#3-task-manager)); P02-U5 `RGBAColour` float constructor → bytes 0,0,0,1 (B22); P02-U7 PlayTime units = minutes (copied into TimeLimit); P18-U1 server does not check option senders (B16); P08 "retribution never called" → live (§2.9).

<!-- from 02_units_and_entities.md -->
### From 02_units_and_entities.md

Every [U] item and every design-critical [I] item in this document.

| # | Item | Tag | What would resolve it |
|---|---|---|---|
| 02-U1 | Runtime value of the `unsigned char` speed stat for SpaceShip (blueprint 500) and Meteor (350): C++ undefined behaviour; MSVC x86 probably yields 244 / 94. SpaceShip movement uses the byte (Meteor reads `GetStat` and is unaffected) | [U] (new-game decision taken: use **244**, §2.4) | disassemble `Entity::SetType` in the shipped `multiwinia.exe`, or log `m_stats[StatSpeed]` in a running build; if the shipped value differs, replace 244 |
| 02-U2 | Eggs-crate creatures belong to the task owner (they fight for the player), unlike Triffids/Ant Nest/Infection (Monster team) — intentional? | [U] intent | design notes or playtest of the shipped build |
| 02-U3 | (resolved by coordinator after 22) Does the wall-clock `m_front` written in `Darwinian::AdvanceWatchingSpectacle` (EscapeRocket launch in Rocket Riot) desync matches? **No:** `code/worldobject/rocket.cpp:EscapeRocket::IsSpectacle` returns false whenever `App::Multiplayer()` is true — always the case in Multiwinia — so the spectacle path is never entered; see [22 L9](22_determinism_and_networking.md). | [C] | — |
| 02-U4 | Meteor-shower and nuke damage are not team-filtered (the owner's units are hit) — intentional? | [U] intent | playtest / design notes |
| 02-U5 | SpaceShip wander targets confined to the low-x/low-z quarter (SFRAND range + heightmap index wrap) | [I] | instrument `SpaceShip::FindNewTarget` or read `SurfaceMap2D::GetValue` out-of-range handling in detail |
| 02-U6 | Squads always use grenades in Multiwinia — no reachable setter of `InsertionSquad::m_weaponType` was found | [I] | exhaustive search of input/task-manager code paths in the FULL build, or playtest |
| 02-U7 | Whether the lobby option `GAMEOPTION_GAME_RESEARCH_LEVEL` (`Multiwinia::SetGameResearch`) can override the fixed MP research table set in `Location::Init` | [U] | trace the order of `SetGameResearch` vs `Location::Init` at match start ([01](01_game_modes.md), [22](22_determinism_and_networking.md)) |
| 02-U8 | Local-only routes (the formation preview route in `Officer::SetFormation` for the local team) are allocated from the same routing-system id space that Darwinians and officers resolve by id on every peer; if ids diverge, route-following may desync | [I] | read `RoutingSystem::GenerateRoute` slot allocation and test a formation order in a 2-peer match ([21](21_simulation_and_entity_model.md)) |
| 02-U9 | A spirit left in `StateHarvested` when its Harvester disappears (`Task::Stop` path does not release spirits) has no fallback in `Spirit::Advance` — it would hang in place forever | [I] | test, or confirm `Harvester` removal paths call `ReleaseSpirits` |
| 02-U10 | `Virii::s_viriiPopulation[team]` is never reset explicitly; correctness between matches relies on every Virii destructor running at `Location` teardown | [U] | verify `Location::Empty` deletes every team's entities before the next match |
| 02-U11 | Airstrike and nuke timings on real terrain (the simulations assume flat ground; bombers ignore terrain, nuke arcs depend on target height) | [U] | instrument `AirstrikeUnit::Advance` and `Nuke::AdvanceToTarget` in a running build |
| 02-U12 | Unbounded search loops: `Crate::RunNuke` (needs sea), `SpaceShip::FindNewTarget` and `Crate::RunSpaceShip` (need terrain ≥ 30 / ≥ 10) would hang on maps without such terrain. `RunNuke` is resolved for the shipped maps: every `mp_*` map has ≥ 26 % sea in its sampling rectangle ([04](04_crates_powerups_and_weapons.md#open-questions-and-unknowns) 04-U1, [I]) | [U] for the SpaceShip loops | heightmap scan of every shipped `mp_*` map ([23](23_terrain_and_landscape.md)) |
| 02-U13 | The AI entity sits in the entity grid at (20, ·, 20) and `Darwinian::SearchForThreats` does not exclude `TypeAI`; enemy Darwinians near that corner may lock onto an invulnerable target | [I] | test on a map with spawn points near (20, 20), or add the exclusion |
| 02-U14 | Harvester waiting forever at a spawn point when its head spirit index is invalid; Engineer drop loop spinning on an invalid index | [I] | confirm collected/attached spirits can never be removed by another system |
| 02-U15 | Armour cannot unload over water and flips to Idle when told to (registration skipped → MP auto-Idle) | [I] design-critical | test an unload order over sea |
| 02-U16 | Nuke launch compensation (`/100`) does not equalise arrival times (mean flight ≈ 47 u/s) — nearer subs hit first | [I] design-critical | run `_work/scripts/P04_nuke_sim.py` against real maps or instrument a build |
| 02-U17 | Whether any shipped map contains Control Towers, Research Items or Incubators (Engineer reprogramming relevance); only `mp_sandbox*` among MP-named maps do | [U] | confirm whether the sandbox maps are listed by the menu / shipped archives ([08](08_levels_and_level_design.md)) |
| 02-U18 | Spider leg swing uses the wall clock; assumed cosmetic because foot state does not feed body motion or synced RNG | [I] | sync-log test with a spider on screen at different frame rates |

<!-- from 03_buildings_and_world_objects.md -->
### From 03_buildings_and_world_objects.md

| ID | Question | Tag | What would resolve it |
|---|---|---|---|
| 03-U1 | Exact composition of `Team::m_others` (which entity types count against the PopulationCap used by SpawnPoint, TrunkPort and AntHill `PopulationLocked`) | **Resolved** [C] | `Team::NewEntity` puts every entity spawned with unit id −1 into `m_others` (Darwinians, officers, armour, engineers, harvesters, Virii, the CPU team's `AI` entity, …); entities created inside a `Unit` (squads, airstrike bombers, centipedes, hill ants, tentacles) are not in it. The population test subtracts `Virii::s_viriiPopulation`. `code/team.cpp:Team::NewEntity`; per-type list in [05 §4.1](05_economy_and_population.md#41-what-is-counted) |
| 03-U2 | Whether every building is advanced exactly once per 0.1 s tick (all 10 slices of `NUM_SLICES_PER_FRAME` processed every tick) | **Resolved** [C] | yes: a new update is taken only after slices 0–9 have run, and the slice bounds are contiguous (§1.2). `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`, `code/lib/slice_darray.cpp:SliceDArray::GetNextSliceBounds`; also [21 §5.4](21_simulation_and_entity_model.md#54-slice-partitioning) |
| 03-U3 | Whether the GameTypes-none dev maps (`mp_sandbox`, `mp_sandbox_all`) can be opened from the shipped UI, making ControlTower and Incubator reachable | [U] | map-list construction in the game menu ([09](09_meta_systems.md)) |
| 03-U4 | Multiplayer research level of `TypeLaser` (subversion-laser lifetime) | **Resolved** [C] | `Location::Init` sets `TypeLaser` = 2 in every multiplayer level → subversion laser lifetime 1.2 s (§5.1). `code/location.cpp:Location::Init`; call order in [41](41_risks_and_open_questions.md) (02-U7) |
| 03-U5 | Origin and lifetime of `Darwinian::m_subversionTaskId` (turns Standard turret shots into subversion lasers) | **Resolved** [C] | set by the Subversion power-up (`Crate::RunSubversion` → `Darwinian::GiveMindControl`) for own Darwinians within `POWERUP_EFFECT_RANGE`; cleared by `Darwinian::Advance` when the task is gone (§5.1, [04](04_crates_powerups_and_weapons.md)) |
| 03-U6 | How Darwinians react to `GunTurretTarget` (flee radius and trigger) | **Resolved** [C] | not at all in Multiwinia: `Darwinian::SearchForThreats` tests `EffectGunTurretTarget` only when `!Multiplayer()` (§5.1) |
| 03-U7 | Behaviour when a mandatory level-file token is missing (`GetNextToken` returning NULL/empty passed to `atoi`/`iv_atof`) | **Resolved** [C] NULL, [I] crash | `GetNextToken` returns NULL and the `Read` functions pass it unchecked to `atoi`/`atof` → crash at load (§3.10). `contrib/systemiv/lib/filesys/text_stream_readers.cpp:TextReader::GetNextToken`; agrees with [29 §4.1](29_data_formats_reference.md) |
| 03-U8 | Whether any `GlobalBuilding` records exist during Multiwinia matches (so that `m_isGlobal` touch points are truly inert) | [I] | `GlobalWorld` setup on Multiwinia start ([20](20_architecture.md)) |
| 03-U9 | Whether the six SmokeMarkers of `mp_assault_3p_1` (invisible 13-unit obstacles) sit on walkable routes | [U] | plot marker positions against the map's AITargets/routes or run the map |
| 03-U10 | Whether the Randomiser can hand Darwinians to an unused team slot (`syncrand() % NumPlayers`) and what `SpawnEntities` does for an unused team | [U] | `Location::SpawnEntities` and team initialisation for unused slots |
| 03-U11 | Design intent of dividing PopulationCap by the level's `NumPlayers` rather than by playing teams | [U] | design decision; see [05](05_economy_and_population.md) |
| 03-U12 | Visual/collision impact of the unrotated tree crown sphere (hit centre ignores `m_front`) | [U] | reproduce the generator numerically for sample trees |
| 03-U13 | Effective rocket damage per Darwinian attack (grenades aimed 50 above the rocket base but damage measured to the base) | [I] | simulate `Location::ThrowWeapon` ballistics with grenade bounces and fuse 3 s against the rocket position |
| 03-U14 | Whether the per-tick `ChangeTeam` events of empty solar panels and of level templates are audible | [U] | `data/sounds.txt` bindings for `SolarPanel ChangeTeam` and generic building `ChangeTeam` ([10](10_audio_design.md)) |
| 03-U15 | Exact 10-vs-11-tick pattern of the `GetNetworkTime() > t + 1.0` recount timers | **Resolved** [C arithmetic] | exact replay of `m_gameTime += 0.1` from 0 at speed 1.0: 10 ticks for re-arm times in [0.4, 3.0), [15.5, 63.2), [255.4, 1023.0) s and ≥ 4095.5 s, 11 ticks otherwise; 20/21 ticks for `+ 2.0` (§1.2). `code/gametimer.cpp:GameTimer::Tick` |
| 03-U16 | Design intent of the Assault SolarPanel knock-out (ports deleted when captured from the starting team) | [I] | design documentation or play-testing; code behaviour is [C] |
| 03-U17 | Whether shipped `isGlobal 1` MasterSpawnPoints (11 `mp_*` files, visible) differ in behaviour from `isGlobal 0` ones beyond rendering | [I] | `MasterSpawnPoint::Advance` shows the only global-specific branch is single-player; confirm no other reader of `m_isGlobal` for masters |

<!-- from 04_crates_powerups_and_weapons.md -->
### From 04_crates_powerups_and_weapons.md

| Id | Question | Status | What would resolve it |
|---|---|---|---|
| 04-U1 | (resolved by coordinator) Does any shipped map lack sea inside the `Crate::RunNuke` sampling rectangle `[0, X−100) × [0, Z−100)`? **No.** Re-generating every `mp_*` heightmap from its level file (`Landscape`, `LandscapeTiles` and `LandFlattenAreas` blocks, pipeline of [23](23_terrain_and_landscape.md)) and sampling the rectangle on a 25-unit grid gives a sea fraction of at least 26.2 % on every map (minimum `data/levels/mp_assault_3p_3.txt`; maximum 91.6 %, `data/levels/mp_assault_2p_3.txt`), so the expected number of re-rolls is ≤ 4 and the loop terminates on all shipped maps. The per-map sea and border statistics are specified in [23 §9.4](23_terrain_and_landscape.md#94-sea-level) (provenance: generated by `_work/scripts/C_nuke_sea_check.py`, re-run after the flatten-type-3 fix). The hazard remains for user/edited maps without water. | [I] (port is not bit-exact, but the margin is large) | Bit-exact height dump from a running build would make it [C]; the new game should bound the loop or pre-compute a sea-cell list. |
| 04-U2 | (resolved) Weighted crate drops accept a position iff the terrain at z ∈ {0, 1} (the `pos.z > 20.0` bug) is non-zero. Which maps accept on the first try and which always fall back to random? **Every shipped map accepts on the first try.** Re-generating each `mp_*` heightmap from its level file (pipeline of [23](23_terrain_and_landscape.md)) gives z = 0/1 heights between −30 and −6.93 for x ∈ [−100, worldX + 100] on all 57 maps, never 0: the grid is initialised to `OutsideHeight` and tiles only raise cells (`Landscape::MergeTileIntoLandscape`) (computed by `_work/scripts/S_41_crate_edge_check.py`). Weighted drops therefore land at the chosen building ± 100 wu regardless of terrain and may land in water and expire unreachable (§1.4). | [I] (re-generation not bit-exact; margin ≥ 6.9 wu) [C] mechanism `code/location.cpp:Location::CreateRandomCrate`, `code/landscape.cpp:Landscape::GenerateHeightMap`, `Landscape::MergeTileIntoLandscape` | A bit-exact height dump from a running build would make it [C]; the new game must replicate the bug literally or test the real terrain. |
| 04-U3 | Edge cases at the (0,0) corner: random-mode crates start from the template position (0,0,0) and the Space Ship from (0,0,0); if that corner were land (≥ 20 resp. ≥ 10) and unrestricted, the crate / ship would spawn there at y = 0. | [I] | Heightmap value at (0,0) per level (expected sea). |
| 04-U4 | Design intent of: bad outcomes ignoring `InvalidCrates` and the AntNest limit; Eggs creatures belonging to the task owner while every other monster outcome is Monster-team; Meteor and Plague hitting the user's own units; `InitMonsterTeam(capturer)` no-op. | [U] intent ([C] behaviour) | Designer notes or playtest of the shipped build; decide per item for the new game. |
| 04-U5 | `Randomiser` assigns `syncrand() % levelFile.m_numPlayers` — on a 4-slot map played by 2–3 teams it can create Darwinians on a team id that is unused in this game. What do such Darwinians do (AI, scoring, crate capture)? Same question for magical-forest spirits of unused team ids. | [U] | Trace `Team`/`Location` handling of entities on `TeamTypeUnused` teams; playtest a Randomiser on a 4P map with 2 players. |
| 04-U6 | (resolved) Monster (4) and Futurewinian (5) Darwinians can capture crates (§1.7). Their teams receive the task rewards — does any AI entity drive those teams' task managers? **Yes.** In multiplayer `Location::Init` initialises both teams as CPU teams (`InitMonsterTeam` / `InitFuturewinianTeam` → `Multiwinia::InitialiseTeam`), which calls `Location::InitialiseTeam` because the teams are already initialised; that spawns one `AI` entity per CPU team, and its `AI::AdvanceCurrentTasks` runs (on the AI main timer) every startable task of its own team's task manager ([02 §24](02_units_and_entities.md#24-ai)) | [C] `code/location.cpp:Location::InitMonsterTeam`, `Location::InitFuturewinianTeam`, `Location::InitialiseTeam`, `code/multiwinia.cpp:Multiwinia::InitialiseTeam`, `code/worldobject/ai.cpp:AI::AdvanceCurrentTasks` | — |
| 04-U7 | Airstrike and nuke timings on real terrain (simulations assume flat ground). | [I] | Instrument `AirstrikeUnit::Advance` and `Nuke::AdvanceToTarget` in a build on shipped maps. |
| 04-U8 | Real-time durations of Slow Down / Speed Up and the meteor slow-motion assume 10 Hz real-time server ticks; whether `GameTimer::Reset` clears a pending speed request between matches (the routed meta notes suggest `m_requestedSpeedLength` is not reset). | [I] | Read `code/gametimer.cpp:GameTimer::Reset` together with the server tick scheduler ([20](20_architecture.md)); measure in a build. |
| 04-U9 | Are the `GameTypes none` levels (`mp_sandbox`, `mp_sandbox_all`, `mp_benchmark_1`, `mp_assault_stress`) ever selectable in the FULL lobby (their crate rows in §3.5 would then apply with mode-defined options)? | [I] not selectable | Lobby map-list filter ([09](09_meta_systems.md), [08](08_levels_and_level_design.md)). |
| 04-U10 | Can crates drop during the pre-game grace period? `Location::Advance` is not gated by the grace period. | [I] yes | Confirm the grace-period gating of `Location::Advance` in `code/network/clienttoserver.cpp` / [01](01_game_modes.md). |
| 04-U11 | Grenade, rocket and shockwave effective ranges/hit counts (§6.3, §6.6, §6.11) are derived from re-implemented arithmetic on flat terrain; the rocket's 8-or-9-move outcome additionally depends on its effect slot (§6.6, §7). | [I] | Instrumented build; or adopt the formulas as the new game's spec. |
| 04-U12 | (resolved) Tick counts of `x −= 0.1` timers: computed by S04 with IEEE-754 doubles — 1.0 → 11 ticks, 2.5 → 25 (24 active), 5.0 → 51, 15.0 → 151, 45.0 → 450, 60.0 → 600; float `Task::m_lifeTimer` 30 → 300, 60 → 601. Any new implementation should use integer tick counters and decide the intended values. | [C] arithmetic | — |

<!-- from 05_economy_and_population.md -->
### From 05_economy_and_population.md

| ID | Item | Status | What would resolve it |
|---|---|---|---|
| 05-U1 | Does the 5.0-radius neighbour query in `Building::EvaluatePorts` return only Darwinians within 5.0 of the port, or anything in the grid cell? Affects which Darwinian "occupies" a port and therefore capture. | **Resolved [C]**: exact strict test `dx² + dz² < 25`; the occupant is the first operating Darwinian in grid result order (cell x → cell z → team 0–5 → cell slot), §2.1 | — (closed by `code/entity_grid.cpp:EntityGrid::GetNeighbours`) |
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
| 05-U12 | Harvester throughput figure (≈ 2.5 Darwinians/s for a 500-unit round trip) is a back-of-envelope estimate; real movement uses turning and acceleration filters. | [I] | Simulate `Harvester::AdvanceToTargetPos` on a map with the movement law of [02 §9.6](02_units_and_entities.md#96-harvester--movement-terrain-and-obstacles) (turn 9.5·dt, velocity filter 0.92 / 0.08, arrival 5). |

Resolved in this document (previously open): 05-U1 port-occupancy query radius and tie order (§2.1, [C]); meta U3/U6 composition of `m_others` (§4.1, [C]); meta U4 harvesters collect enemy souls (§5.3, [C]); P06 "each building once per tick?" (§8.1, [C] — yes); P06 "GetNetworkTime vs GetGameTime" — `GetNetworkTime` is a macro for `g_gameTimer.GetGameTime` [C] `code/gametimer.h`; per-team cap vs teams in play (§4.2: empty slots are CPU-filled, disconnected humans become CPU with a new AI entity [C] `code/multiwinia.cpp:Multiwinia::RemoveTeams`).

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
| 06-U12 | **Closed.** Return value of `Resource::GetTexture` for the missing `icons/compass_empty.bmp` (X30): it never returns — `Resource::GetTextureWithFlags` calls `AppReleaseAssert(false, "Failed to load texture %s")` (fatal in release: message box, `blackbox.txt`, `throw`), and the `return -1` after it is unreachable; the file is never requested in practice (X30) | resolved [C] | `code/lib/resource.cpp:Resource::GetTextureWithFlags`, `contrib/systemiv/lib/debug_utils.cpp:AppReleaseAssertFailed` |

Resolved during synthesis (no longer open): P01-U5 (no shipped map contains a FenceSwitch, §1.6), P01-U8 (`TERMINATE` with PROGRAM 255 kills the entity via `ChangeHealth(−999)`, §1.4), P03-U1 (`m_underPlayerControl` cleared by `Team::SelectUnit`, §2.3), P04-U3 (`m_currentMousePos` travels in `ALIVE`, §6.2), P04-U6 (keyboard squad controls: hold LMB move, hold RMB lasers, RMB + LMB grenade, §6.3), P09-U1 (selection-circle fields are UI-only, §2.1), P09-U2 (particles never feed back into simulation, §3.6), P09-U3 (`RenderWeaponMarker`, `Task::Demote`, `GetNextTaskId` have no callers, §3.6–§3.7, §8.10), P09-U4 (`RestrictionZone::IsRestricted` semantics, §3.6), P10-U6 (keyboard entity tracking only via a hand-edited `AutomaticCamera = 2`, §9.6).

Contradictions resolved by re-reading the code (11): (1) control scheme selectable vs forced (P01/P10 vs `PrefsManager::Load`) → forced to RTS; (2) rebinding reachable (P10) vs unreachable (P17b) → unreachable; (3) "game over → ToggleReady returns to menu" live (P09) vs testbed-only (P01) → `TESTBED_ENABLED` only; (4) officer terminate "demote" (P01, code comment) vs kill (P09) → `ChangeHealth(−999)` kill; (5) armour order flags (P01 wording) → plain RMB = move (`SetWayPoint`), Shift = move-and-deploy (`SetOrders`), FORCEONLYMOVE = !deploy; (6) armour orders marker registered by the local team in `code/location.cpp` (P08) → that call is commented out, only the network handler registers; (7) stick output range ±39/±19 (P10) → asymmetric [−40, 39] / [−20, 19] because of `floor`; (8) `m_cursorHighlight` size 30 (P09) → default 20, set by callers; (9) camera census "88 level files" (P10) → 87 top-level files (+30 under `data/levels/old/`); (10) `NEXTWEAPON`/`PREVWEAPON` listed with live senders (P15) → both senders exist with full bodies, but the only call (`RequestNextWeapon`) is commented out and `RequestPreviousWeapon` has no caller, so neither is ever sent; (11) pad bindings for officer orders (P09 "special = X toggles") → A = walk, X = GOTO because of the gamepad inversion.

<!-- from 07_ai.md -->
### From 07_ai.md

| Id | Item | Status | What would resolve it |
|---|---|---|---|
| 07-U1 | How often the selection gate in `Task::TargetOfficer` stalls AI officer creation and how many orphan Officer tasks accumulate (B13, B24) — and therefore how often CPU teams lose crate rewards to full slots | [I] mechanism and the complete list of CPU selecting/deselecting calls confirmed in code (§7.4); impact unmeasured | instrumented runs on every shipped map logging each CPU team's task list, `m_currentEntityId/UnitId/BuildingId` and promotion attempts |
| 07-U2 | Whether the stale `m_mostRecentTaskId` path (B12) ever fires a pending power-up (Meteor, Nuke, Air Strike) at the AI's own Darwinians | [I] | same instrumentation, forcing full task slots with a pending power-up |
| 07-U3 | Whether the retail Multiwinia build's AI matches this source release (patch level of `f909630` vs the last shipped patch) | [U] | compare against retail binary behaviour/replays, or version strings |
| 07-U4 | Whether every shipped map has at least one sea sample (height ≤ 0) inside [0, worldSize − 100)², otherwise `Crate::RunNuke` hangs (B1) | [U] | heightmap scan of every `mp_*` landscape definition ([23](23_terrain_and_landscape.md)) |
| 07-U5 | Practical effect of the fence/wall-blind initial graph (B9) on the seven maps with fences and no turrets before the first rebuild | [I] | offline comparison of the initial and rebuilt link sets per map; observe opening minute |
| 07-U6 | Whether an officer's `m_targetFront` controls where its formation fires, i.e. whether B15 (defenders facing away) changes combat outcomes | **Resolved** [C] | yes: it turns `m_front`, which places the formation slots and offsets each member's threat search 59 wu ahead (§7.2). `code/worldobject/officer.cpp:Officer::Advance`, `Officer::GetFormationPositionFromIndex`, `code/worldobject/darwinian.cpp:Darwinian::SearchForThreats`, `Darwinian::AdvanceCombat` |
| 07-U7 | Whether hostile units, turrets or ants can select the immortal AI entity at (20, 0, 20) as a target (B22) and get stuck on it | [U] | inspect target filters of `EntityGrid::GetBestEnemy` callers and creature AIs for entity-type checks |
| 07-U8 | How often isolated own nodes occur (NaN priorities, B14) in shipped graphs | [I] | offline graph build per map with the §4.2 rules |
| 07-U9 | Whether the host can switch `RetributionMode` off in the lobby (P18 lists it as a "−1 → checkbox" option) — governs how often eliminated CPU teams get retribution powers | [I] default on | read the Advanced Options page builder ([09](09_meta_systems.md)) |
| 07-U10 | Whether any shipped Assault defence marker is unbound, has `scanRange > 0` and sits where no defender stands, so the B17 team-0 default makes its objective fall back at once | [U] | offline marker/bound-building analysis of the 8 Assault maps plus a run per map |
| 07-U11 | Whether `Team::m_specials` can hold ids of dead entities (would make `AI::RunForest` crash, B18, contrary to the changelog's "fixed") | [U] | read `Team::m_specials` maintenance ([21](21_simulation_and_entity_model.md)) |
| 07-U12 | The AI entity's slice (index 0 assumed) and the one-tick ordering between AI decisions and node recounts | [I] | read `Team::NewEntity` and `SliceDArray::GetNextSliceBounds`; only affects one-tick latency |

<!-- from 08_levels_and_level_design.md -->
### From 08_levels_and_level_design.md

| Id | Question | Status | What would resolve it |
|---|---|---|---|
| 08-U1 | Did the retail product ship `levels/level_enum.txt` (e.g. in a patch archive)? Without it no level is recorded for the "Master of Multiwinia" achievement | [U] | list a retail `main.dat` / `patch.dat` |
| 08-U2 | How `mp_benchmark_1` and `mp_assault_stress` were started in the hardware-compat / stress builds despite `GameTypes none` | [U] | data of those builds, or a launcher/test-bed path not in this tree |
| 08-U3 | Contents of the inclusion list `multiwinia_inclusions_ambx.txt` (referenced by `tools/build_multiwinia_data.bat` for platform `platambx`; the file is absent from `tools/`) | [U] | the missing file |
| 08-U4 | Exact tie order inside each menu list (depends on archive index order through `OrderedInsert`'s off-by-one) | [I] | run `CreateMapList` against a real `main.dat` |
| 08-U5 | Content of the 10 PDLC maps named in `level_enum.txt` (ids 50–59) | [U] | PDLC data (not in the tree) |
| 08-U6 | Whether the released metaserver sent a demo map list different from the 5-map default | [U] | metaserver data / retail logs |
| 08-U7 | Tool used to make the 400×300 thumbnails (no in-engine writer) | [U] | art-pipeline notes |
| 08-U8 | Effect of a load → save → load round trip on terrain and CRC (writer quantisation, §8.7) | [I] | run `scripts/S_23_terrain_oracle.py` on a re-saved map |
| 08-U9 | Outcome of opening a plain-text map named `.mwm` (certain failure in `MapFile::MapFile`; crash vs. garbage level) | [I] mechanism [C] | run an editor build |
| 08-U10 | Whether a `.mwm` read with 4 trailing bytes past the zlib stream and an unchecked `uncompress` ever mis-loads | [I] | fuzz `MapFile::MapFile` with editor-written files |
| 08-U11 | Group-A beta build: 13 included maps vs 3 whitelisted, with only CTS compiled → would list 1 map | [I] | the group-A build or its release notes |
| 08-U12 | Design intent of concentric KOTH zones, duplicate CTS statue zones, KOTH team-owned zones starting owned, fill/key light pairs, twin terrains | [I] | designer notes; play-test in the original |
| 08-U13 | `CalculeCRC` buffer is 100 000 bytes: a > 100 000-byte level overflows by one byte (largest shipped 46 085) | [I] | n/a for shipped data; avoid in the new design |

<!-- from 09_meta_systems.md -->
### From 09_meta_systems.md

Every [U] and every design-critical [I] in this document. Resolved during synthesis (not counted): server-side enforcement of host-only options (§4.4, resolved — none); `PageDarwinia` reachability (§1.2.1, debug only); in-match exit path (§2.1, Esc menu only); duplicate English help key (§8.1); persisted player-name truncation (§3.8); tutorial map CRCs — the P11/X4 CRC re-computation reproduces every `MAPID_*` value in `code/mapcrc.h`, so the tutorial buttons are active in a full install ([08](08_levels_and_level_design.md)).

| Id | Question | Tag | What would resolve it |
|---|---|---|---|
| 09-U1 | Did the retail Steam / Xbox Live build have a real achievement layer replacing the `App::EarnedAchievement` / `GiveAchievement` stubs, and were achievements persisted (`achievements.txt`)? | [U] | the retail branch or a retail binary; not in this tree |
| 09-U2 | Which authentication key file shipped: start-up reads `<profile>authkey.dev`, typed keys are written to `<profile>authkey` (support INI and installers reference `authkey`). If the retail build matched this tree, hand-typed keys were lost on restart | [U] | retail binary / release branch, or a retail install's profile folder |
| 09-U3 | Final `TimeLimit` after picking a map with a non-zero `PlayTime`: timing-dependent (map PlayTime if the lobby is built before the echo — the usual case — else the blueprint default) | [I] | instrumented run logging `GAMEOPTION_GAME_PARAM` versions on host and clients |
| 09-U4 | Whether a retail install also shipped a loose `data/default_preferences.txt` (the code shows the copy inside `main.dat` cannot be read when the first-run defaults are built, because only `init.dat` is parsed then) — decides first-run `NetworkServerPort`, `WaitVerticalRetrace`, `DarwinianScaleFactor` | [U] | first launch of a packaged build with an empty profile folder; inspect the written `preferences.txt` |
| 09-U5 | Could the external metaserver inject key `"dy"` into listings (which would make the "show passworded games" filter hide those servers)? | [U] | metaserver source or packet capture |
| 09-U6 | Exact visual of the 1.0 s menu transition (slide vs fade, curve) used by every page change | [U] | [24](24_rendering.md)/[28](28_ui_framework.md) `Renderer::RenderMenuTransition`, or a run |
| 09-U7 | Whether the metaserver protocol could carry leaderboard / TrueSkill data; none of the defined fields is used for it (`PLAYERSCORE` unused) | [U] | metaserver source |
| 09-U8 | Whether `data/mwhelp/background.bmp` is referenced indirectly (e.g. by a data file or skin); no code reference found | [U] | full-text search of `data/` and `contrib/eclipse`, or remove and run |
| 09-U9 | Whether `GL_BLEND` is enabled when `CreditsLoader::FlipBuffers` draws its fading clear quad (decides whether the first 7 s smear) | [U] | GL state trace at `Script::RunCommand_RunCredits` / outro camera (legacy path) |
| 09-U10 | Exact visual change of the `RenderBuildingDetail == 1` / `< 3` branches in `Building::RenderPorts`, `LaserFence::RenderAlphas`, `MineBuilding::*`, `SpawnBuilding::RenderAlphas` | [U] | read those functions in full ([03](03_buildings_and_world_objects.md) owners) |
| 09-U11 | How MetaServer / MatchMaker behave server-side: which fields they add to LISTRESULT (`PROTOCOLMATCH`, `DEMORESTRICTED`, `DEMOSERVER`), how they decide "demo-restricted" (the release-notes LAN rule), how REQUEST_CONNECT is relayed | [U] | metaserver source (not in this tree) or packet captures |
| 09-U12 | The real key-check algorithm (`Authentication_SimpleKeyCheck`, `Authentication_GenerateKey` are stubbed in this release) | [U] | original `authentication.cpp`; irrelevant for a new game using platform entitlements |
| 09-U13 | Manual `ServerN` browser entries are effectively unjoinable (no version / protocol / map CRC ⇒ "incompatible") | [I] | run with a `Server1` pref pointing at a LAN host |
| 09-U14 | With Japanese, Korean or Portuguese selected about 90 % of the UI shows raw phrase ids; whether any SKU shipped those files | [I] | retail SKU manifests (`language.dat` contents of each release) |
| 09-U15 | Tutorial 2 relies on radar dish 4 becoming player-owned and on CPU team 2 taking spawn 83 before the player; if not, steps are skipped or no tab appears | [I]/[U] | runtime observation ([30 §2.4](30_scripting_tutorial_and_cutscenes.md)) |
| 09-U16 | `LoadingScreen::m_rendering` is uninitialised until the first `Render()`; whether any early `IsRendering()` call reads it (affects resource locking and sound suppression) | [U] | trace calls before the first `Render()` in `App::App` |
| 09-U17 | Demo players: the client enforces only mode/map/host/multiplayer predicates; `m_maxGameSize` is never read — whether the external metaserver or a retail server enforced a demo game-size cap | [U] | metaserver source / retail server behaviour |

<!-- from 10_audio_design.md -->
### From 10_audio_design.md

| Id | Item | Class | What would resolve it |
|---|---|---|---|
| 10-U1 | Audible content of the voice files — intelligible words (needing replacement and possibly localisation) or gibberish: `YesSir*`, `Attack*`, `Retreat*`, `CrateReward_*`, `GDSpeech*`, `GameOver*`, `YouWon`, `YouLost2` | [U] | Audition the files |
| 10-U2 | Whether the retail `sounds.dat` / `sounds2.dat` / `menusounds.dat` match this tree's manifests (archives are not in the repository) | [U] | List a retail install's archives |
| 10-U3 | Provenance of the non-music samples (in-house recordings vs licensed libraries) | [U] | Ask the rights holder; affects only the legal note (all treated as IP) |
| 10-U4 | Stationary loops never stopped: does the Assault pulse-bomb choir really keep playing after the bomb is destroyed and after the match? | [I], design-critical for Assault | Play an Assault match to completion; trace `SoundSystem::m_sounds` |
| 10-U5 | Audible effect of the 316 wu `MonophonicNearest` radius on Darwinian spawn chatter, the reload click and `EscapeRocket Unhappy` at normal camera heights | [I], design-critical (crowd design) | Play-test at several camera heights, log culls (27-U8) |
| 10-U6 | Whether the hard-cut of attached sounds at emitter removal was intended (a graceful release path exists but is almost never reached) | [I], design-critical | Designer intent / listening test (27-U5) |
| 10-U7 | Exact DirectSound 3D panning / roll-off of `DS3DALG_DEFAULT` on shipping drivers. Fidelity question only: the normative replacement laws are fixed in §8.2–§8.3 | [U] (not blocking) | Measure the shipped game (27-U1) |
| 10-U8 | Do idle Darwinians perform victory jumps (`VictoryJump`) on the 10 `mp_*` maps without `PrimaryObjectives` (`mp_assault_2p_3`, `mp_assault_3p_1`, `mp_assault_3p_3`, `mp_assault_4P_2`, `mp_blitzkrieg_4P_2`, `mp_domination_2P_1`, `mp_domination_4P_1`, `mp_koth_2P_3`, `mp_koth_2P_3b`, `mp_skirmish_3P_1`)? | [I] | Run one of those maps; check how the level loader leaves the objective list |
| 10-U9 | Exact text produced by the `Capturing_%pc` bug under MSVC 2005 | [I] (moot: it never matches) | Run and log the event name |
| 10-U10 | Audible effect of team 255 passing every friend/foe filter (both KotH/Blitzkrieg stings on neutralisation; zone stings at Blitzkrieg start when bases are set neutral, possibly dropped by the loading-screen gate) | [I] | Play a Blitzkrieg match and listen at the first neutralisation |
| 10-U11 | Duplicate Rocket Riot loops from keep-alive polling before the first sound tick | [I], design-critical for Rocket Riot audio | Trace `m_sounds` during a Rocket Riot match (27-U14) |
| 10-U12 | Was lobby music ever planned beyond the ambience beds (`docs/multiwinia audio.docx`: "Lobby — Sounds + music ?")? | [U] | Designer interview; moot for the new game (§14.5) |
| 10-U13 | Whether the compression of smoothed random pitch ranges on the first note (e.g. 0.75–1.35 → ≈ 0.88–1.16) was intended | [I] | Listening comparison (27-U10) |
| 10-U14 | Render-rate dependence of `Grenade Flash` / `AirStrikeBomb Flash` and the in-game timer ticks (hidden HUD, low frame rate) | [I] | Run with the HUD hidden / at low frame rate |
| 10-U15 | Whether the early `menusounds.dat` load was meant to allow lobby ambience during loading (commented-out trigger in `App::CheckSounds`) | [I] | Designer intent; irrelevant to the new game beyond §2.6 |
| 10-U16 | Audible character of the game-over "tape stop" (√ of a smoothed game speed down to 0.1× pitch) | [I] | Record the end of a match |

<!-- from 20_architecture.md -->
### From 20_architecture.md

| # | Item | Status | What would resolve it |
|---|---|---|---|
| 20-U1 | Real frame rate of the shipped GL build: the code sets no swap interval, so vsync depends on driver settings; the host's frame rate bounds the server tick rate (§10.2, §10.5) | [U] | measure a retail build on reference hardware with driver vsync on/off |
| 20-U2 | Behaviour when the host renders below 10 fps: one server tick per frame ⇒ the whole session slows; clients cannot run ahead | [I] (from one-tick-per-call) | timing test with an artificially slowed host |
| 20-U3 | Pacing results of §10.4/§10.9 assume zero network latency and perfectly periodic letters | [I] (model) | instrumented run logging slices per frame on host and remote clients |
| 20-U4 | `Multiwinia::ResetLocation` (multi-round Assault) rebuilds the world after `AdvanceAndCatchUp` returns, so peers may have run different numbers of slices after the round-end flag; also no `ResetUniqueIdSeed` | [I] design-critical (if the mode were enabled) | confirm with [01](01_game_modes.md) that no UI sets multiple rounds; otherwise a two-client test |
| 20-U5 | `SpawnPoint::s_numSpawnPoints[6]` (team 5) is never reset between matches (§5.3); impact depends on whether team 5 (Futurewinian) can own spawn points | [C] gap / [I] impact | [05](05_economy_and_population.md)/X4 level data: does any map give team 5 spawn points or let it capture them |
| 20-U6 | `GameTimer` residue across matches can drive the game speed to −∞ and NaN (§10.7 item 6); reachability requires leaving a match during a crate hold and a quick restart | [I] design-critical for the port (reset everything) | reproduce in the original build: leave mid-hold, restart within the hold, wait > 110 s before readying |
| 20-U7 | `m_fillsRequested` race between the waveOut callback thread and the main thread (H5) | [I] | audio stress test / code inspection of `TopupBuffer` buffer accounting ([27](27_audio_engine.md)) |
| 20-U8 | Server listener running during `~Server` (H7) | [I] | review of `NetSocketListener::StopListening` blocking semantics in `contrib/systemiv/lib/netlib/net_socket_listener.cpp` |
| 20-U9 | OpenMP in `Release\|Win32`: no `OpenMP` attribute → Visual Studio default (off) | [I] (default), [C] attribute absent | build log of the baseline configuration (`/openmp` absent) |
| 20-U10 | `WorkQueue::RunThread` calling-convention mismatch (H1) is harmless in practice | [I] | disassembly of the shipped binary or a debug run |
| 20-U11 | Whether DirectSound/winmm internal threads touch game memory beyond `WaveOutProc` | [I] | API documentation review; no game callbacks other than `WaveOutProc` are registered [C] |
| 20-U12 | `g_fileSystem` (SystemIV `FileSystem`) is never instantiated | [I] | grep for `new FileSystem` across compiled sources ([31](31_platform_build_and_tools.md)) |
| 20-U13 | `Teleport::m_teleportMap` has no per-match clear | [I] | read `code/worldobject/teleport.cpp` fully ([03](03_buildings_and_world_objects.md)) |
| 20-U14 | Texture lookups via `Resource::GetTextureWithFlags` are unlocked outside loading screens; safe only if no worker touches `Resource` then (thumbnail preload runs under a loading screen) | [I] | audit every `Resource` call reachable from the SndL thread after boot |

Resolved in this document (previously open in drafts or `_meta_unknowns`): P01 U1 (frame throttling, §10.5), P01 U7 (who ends a match, §8.2), `_meta_unknowns` U2 (`GetNetworkTime` vs `GetGameTime`, §10.1), U5 and "slice cadence" (once per tick, §10.3), "real-time vs game-time cadence of `Tick`" (§10.7), "semantics of `g_predictionTime`" (§10.6), "is `g_app->m_location` assigned before `Location::Init`" (§8.4), the thread count (§11.2), the sound preloader's existence on PC (§11.2), the thread on which the boot archives are parsed (§6.1), the OpenMP attribute (§10.3).

<!-- from 21_simulation_and_entity_model.md -->
### From 21_simulation_and_entity_model.md

| # | Item | Tag | What would resolve it |
|---|---|---|---|
| 21-U1 | Whether the shared non-synced LCG race during `Tree::Generate` (reseed with `AppSeedRandom(m_seed)`, then another thread — sound on the loading-screen loop, the metaserver registration thread — draws from the same generator) ever changes a tree's crown hit sphere on one peer (§10.3) | [U] | instrument `AppRandom` with a thread-id check during `Tree::Generate`; long multi-client soak test |
| 21-U2 | Whether the `EntityGridCache` staleness (≤ ~1.1 s, half-range reuse, requested range ignored) was a deliberate gameplay property or only an optimisation (§6.3) | [U] | designer intent; compare Darwinian combat behaviour with the cache disabled |
| 21-U3 | Whether any shipped map has a building whose bounding sphere reaches within half a cell beyond the far map edge, which would feed the never-cleared obstruction-grid outside cell and could make `Entity::EnterTeleports` dereference a deleted building (§6.4) | [U] | compute building bounding spheres (shape radius) against the world size for the 57 `mp_*` maps, or run a debug build with the assert enabled |
| 21-U4 | Route ids differ between peers (preview routes share `m_routes`, lowest-free-slot re-use); the analysis that this is harmless rests on ids never being transmitted, hashed (the debug IFrame does not hash routes) or compared across entities (§7.6) | [I] design-critical | audit every read of `m_routeId`, `m_orderRouteId`, `Unit::m_routeId` for equality tests or ordering; a replay comparing two clients' route ids |
| 21-U5 | Real-world frequency of the `InsertionSquad::DirectControl` free-look leak L1 (§10.4) | [U] | two-client play test or a replay harness logging per-tick squad state |
| 21-U6 | Spirit handles are slot indices without a generation check; Harvester (`m_spirits`), Engineer (`m_spirits`) and Darwinian (`m_spiritId`) hold them across ticks. Whether a held spirit can be removed (lifetime expiry, another collector) so that the index silently designates a newer spirit (§3.5 rule 3) | [U] | read the Spirit attached/harvested states ([02 §6](02_units_and_entities.md#6-spirit)) for removal paths while held; test with a debug check in `GetSpirit(int)` |
| 21-U7 | `Team::m_currentEntityId` is a raw slot index; after the selected entity dies, the selection can designate the entity that re-uses the slot. Whether any command path (SELECTUNIT, TERMINATE, PROMOTE, TOGGLEENTITY) then acts on the wrong entity (§3.5 rule 7) | [I] | trace the command handlers in `ClientToServer::ProcessServerUpdates` and `Team::GetMyEntity` validation ([22](22_determinism_and_networking.md)) |
| 21-U8 | Whether the unbounded building push loop in `Entity::PushFromObstructions` (zero-length push vector when the entity stands exactly on a building's base point) is reachable (§8.5) | [I] | bound the loop and log in a test build |
| 21-U9 | Whether designers tuned thresholds (grenade decision, zone contest, AI under-attack tests) against the approximate count/presence queries; i.e. whether exact queries would change balance (§6.2) | [U] | designer intent; A/B simulation with exact counts |
| 21-U10 | Whether any by-value pool pointer (`Spirit*`, `Laser*`) is held across a creation into the same pool (growth copies the array, §4.4) | [I] | audit `GetPointer` results kept in locals across `SpawnSpirit`/`FireLaser` calls; none found in the P03 files |
| 21-U11 | Dead airborne entities skip the landing test and fall through the terrain while fading (§8.3); whether any grid user counts them (most queries skip `m_dead`, the approximate counts do not) | [I] | audit grid users for `m_dead` checks; counts are cell-based and include dead entities [C] for `GetNumNeighbours` |

<!-- from 22_determinism_and_networking.md -->
### From 22_determinism_and_networking.md

| # | Question | Tag | What would resolve it |
|---|---|---|---|
| 22-U1 | MetaServer/MatchMaker server-side behaviour: which fields it adds to LISTRESULT (PROTOCOLMATCH, DEMORESTRICTED, DEMOSERVER), how it relays REQUEST_CONNECT, how it validates keys, the hash-token policy | [U] | the metaserver source (not in this tree) or packet captures of the live service |
| 22-U2 | The real key algorithm (`Authentication_SimpleKeyCheck`, `Authentication_GenerateKey` are stubbed in this release) | [U] | the original `authentication.cpp`; irrelevant for a new game (use platform entitlements) |
| 22-U3 | Whether a single UPDATE letter > 1500 bytes ever occurred in shipped play (stall, §3) and whether Windows `recvfrom` drops it (`WSAEMSGSIZE`) as inferred | [I] design-critical | measure letter sizes with 6 players + option bursts; test `recvfrom` with an oversize datagram |
| 22-U4 | Intent of the dead time-based branch in `GetNumSlicesToAdvance` (time-based vs backlog-based pacing) | [U] intent | developer history; irrelevant for a new design |
| 22-U5 | Measured steady-state backlog/latency over a real network (§2.4 is a zero-latency re-simulation of the pacing controller) | [I] | instrumented two-machine run |
| 22-U6 | Whether joining a match in progress was intended for release (it works but replays from frame 0) | [U] | release notes or a UI that greys in-progress games (`GameServerButton` does not) |
| 22-U7 | Whether the VS2005 x87 CRT `sin/cos/pow/acos` produce identical doubles on all x86 CPUs of the era (how much work `iv_trunc_low_nibble` did on PC-only games) | [U] | disassemble the CRT of the shipped binary or run a cross-CPU comparison |
| 22-U8 | Real-world frequency of L1 (holding `CameraFreeMovement` while another team's selected squad moves) | [U] | two-client play-test or a replay harness logging per-tick state |
| 22-U9 | Whether the tree / global-LCG race (§14.7) ever fires (sound triggers during level load, WAN-thread wake-ups) | [U] | instrument `AppRandom` with a thread-id check during `Tree::Generate` |
| 22-U10 | VS2005 defaults assumed for absent project attributes (`/fp:precise`, x87 code generation, OpenMP off) | [I] | the shipped binary's `cl` command line / build log |
| 22-U11 | Whether VS2005 `atof` is correctly rounded for every literal in the shipped data (matters only for non-MSVC peers) | [U] | compare `atof` with a correctly rounded parser over all numeric tokens in `data/` |
| 22-U12 | Cross-CRT equality of `pow/exp/log` results after binary32 rounding for terrain generation | [U] | run the terrain oracle (`_work/scripts/S_23_terrain_oracle.py`) against another libm |
| 22-U13 | Whether PC and Mac clients actually played together (the precision helper and `iv_*` wrappers imply the intent) | [U] | Mac target files / release notes for a cross-platform lobby |
| 22-U14 | Whether `EntityGridCache` staleness (≤ 1 s, half-range reuse) was a deliberate gameplay property | [U] | designer intent; compare behaviour with the cache disabled |
| 22-U15 | Whether the D3D configuration was ever used for multiplayer (it would not interoperate with the GL build) | [U] | release history; the D3D build probably does not compile at this commit ([24](24_rendering.md)) |
| 22-U16 | Which key file name shipped (`authkey.dev` read vs `authkey` written) | [U] | a retail install or the release branch ([09](09_meta_systems.md), [31](31_platform_build_and_tools.md)) |
| 22-U17 | Whether team slot 5 ever owns spawn points in a shipped mode (stale `s_numSpawnPoints[6]`, §14.12) | [I] | census of team assignments per mode/map ([01](01_game_modes.md), [05](05_economy_and_population.md)) |
| 22-U18 | Whether a rejoin into a full match is refused (the departed client's reserved `ServerTeam` counts toward the limit) | [I] | two-client test |
| 22-U19 | What all clients experience when the host quits mid-match (server deleted by `ShutdownCurrentGame`; inferred: lost-connection banner, then nothing) | [I] design-critical | two-client test |
| 22-U20 | Whether `RecordDemo` replays ever reproduced a match (seed not stored; loaded after the replaying session's START_GAME) | [I] | run with `RecordDemo` 1 then 2 |
| 22-U21 | Release-time size of the frame-level command stream for 6-player games and its effect on late join | [I] | measurement |

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

<!-- from 25_textures_and_assets.md -->
### From 25_textures_and_assets.md

| # | Question | Tag | What would resolve it |
|---|---|---|---|
| 25-U1 | Contents of `patch.dat` and `branding.dat` (e.g. whether `carryspecialshape.shp` or replacement art was delivered there); no builder exists in `tools/` | [U] | list the archives of a retail install (`code/lib/resource.cpp:Resource::ParseArchive` semantics) |
| 25-U2 | Which language list each retail SKU's `language.dat` was built with (decides which font atlases shipped; currency atlases ship in none) | [U] | release logs or archive listings of each SKU |
| 25-U3 | Which of the 51 FULL assets whose consumers have no placement in live maps are instantiated at run time (crate or mode spawning) | [I] | cross-reference [03](03_buildings_and_world_objects.md) and [04](04_crates_powerups_and_weapons.md) spawn analyses, or instrument a play session |
| 25-U4 | Whether period GL drivers treated `GL_CLAMP` as clamp-to-edge and accepted non-mip textures left with `MIN = LINEAR_MIPMAP_LINEAR` (affects edge darkening and the no-mip `laser.bmp` draws after a flush) | [U] | run on period hardware or a strict GL implementation |
| 25-U5 | Whether an accurate shape hit test is ever run on `carryweights.shp` or `sporegenerator.shp` (heap overrun, §4.5) | [I] | trace `RayHit`/`SphereHit` callers with `accurate = true` for statues and spore generators |
| 25-U6 | Visible result of the `marker_colour.bmp` processing conflict in the first frame (which pass requests it first after each flush) | [I] | capture the first match frame after a mode change |
| 25-U7 | Exact `tools/Rar.exe` version / RAR method used for retail `.dat` files | [U] | run `Rar.exe` on Windows or inspect a retail archive header |
| 25-U8 | Whether the retail sound archives match this tree's manifests | [U] | list a retail install's `sounds.dat`/`sounds2.dat`/`menusounds.dat` |
| 25-U9 | Audible content of voice samples (`data/mwsounds/YesSir*`, `Attack*`, `CrateReward_*`, `GameOver*`, `YouWon`, `YouLost2`) — words or gibberish | [U] | audition the files |
| 25-U10 | Whether data-only marker names (`MarkerIsland*`, `MarkerTrunk*to*`, `MarkerFloat*`) are looked up through a formatted name not caught by the literal scan | [I] | full call-graph search of `LookupMarker` arguments |

<!-- from 26_animation_and_vfx.md -->
### From 26_animation_and_vfx.md

| Id | Item | Kind | What would resolve it |
|---|---|---|---|
| 26-U1 | What each `.anim` file visually depicts (§3.6 "What it shows" column is inferred from track data) | [I] | run the shipped build and capture the grace-period panel for each mode |
| 26-U2 | Whether Christmas mode can be enabled on some peers and not others (date checked per client; command applied only by receivers at the main menu), causing a snow-RNG desync | [U] | test a lobby with mismatched clocks on 24 Dec; inspect server relay of `NET_DARWINIA_CHRISTMASMODE` in [22](22_determinism_and_networking.md) |
| 26-U3 | Real particle and explosion counts and render cost in heavy matches (no caps exist) | [U] | profile the shipped build (`g_profiler` "Render Particles", "Advance Explosions") on a 4-player Domination map |
| 26-U4 | `animator.exe` behaviour and any fields it writes beyond `AnimatedPanel::Write` | [U] | run the tool on Windows |
| 26-U5 | Whether anything reads Spider foot positions in the simulation (only rendering, sounds and gait planning found) | [I] (§12.3 #3) | exhaustive grep of `m_legs`/`m_foot` readers in a full-project index; none found in `code/worldobject/spider.cpp` |
| 26-U6 | Exact on-screen look of the D3D build's debris (unlit, MODULATE instead of DECAL) and deform effects | [U] | build `ReleaseDirect3D` (may not compile, [24 §1.2](24_rendering.md#12-is-the-d3d-configuration-buildable-at-this-commit-i)) and capture |
| 26-U7 | Precise semantics of `g_predictionTime` used by every render extrapolation | [U] (owned by 24-U3) | trace `code/gametimer.h` and `code/main.cpp` in the architecture owner's document [20](20_architecture.md) |
| 26-U8 | Whether the "binary saturated" team colours of the booster and Futurewinian bubbles (byte values passed to `glColor4f`) were intended | [I] (§4.4) | compare with shipped screenshots |
| 26-U9 | Whether `SoundDSP` was ever enabled for players (effects.txt inert otherwise) | [I] (§9.4) | check retail `preferences.txt` defaults or patch notes |

Design-critical inferred items: §12.3 #1 (tree hit volume from render), #4–#6 (cosmetic RNG coupling), §5.1 wall-clock leg timing.

<!-- from 27_audio_engine.md -->
### From 27_audio_engine.md

| Id | Item | Class | What would resolve it |
|---|---|---|---|
| 27-U1 | Exact panning / HRTF and distance roll-off applied by DirectSound for `DS3DALG_DEFAULT` software buffers on shipping drivers (rolloff and max distance are left at defaults: 1.0 and 1e9 [I]). Fidelity question only: the normative replacement laws are fixed in [10 §8.2–§8.3](10_audio_design.md#82-distance-attenuation-model-minimum-distance--volume-curve) | [U] (not blocking) | Measure the shipped game (sweep a test source around the listener) or confirm with DirectSound documentation per driver; a new engine can use equal-power pan + `minDistance/d` and add a max distance |
| 27-U2 | **Resolved [C] (code reachability):** the music channel is unreachable in Multiwinia. Every `TypeMusic` trigger is Darwinia script content (no Multiwinia path opens a script, [30 §2.5](30_scripting_tutorial_and_cutscenes.md#25-start-paths-triggers)), `CreditsLoader` (reached only through the script ops `GameOver` / `RunCredits`, [B:LEGACY]), `StartSequence` (only from `RunBootloader` under `USE_LOADERS`, [B:LEGACY]) or the seven `USE_LOADERS` loaders ([B:DEAD]) (§8). The music system, music buffer and music callback are idle in every match and in the front end | [C] (closed) | A runtime trace of `SoundSystem::m_music` would only confirm it |
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
| 28-U3 | Whether the shipped SKUs include the partial `japanese`/`korean`/`portuguese` language files (160–220 loaded phrases each); if they do, choosing them shows raw phrase ids for most of the UI | [I] design-critical for localisation QA | inspect the retail `language.dat` per SKU (`tools/build_language_dat.bat` + `tools/*language.list`, [31](31_platform_build_and_tools.md)) |
| 28-U4 | Whether committed IME text reaches `InputField` as `WM_CHAR` units (no `WM_IME_*` handling exists) | [I] | run the game with a Japanese/Korean IME |
| 28-U5 | Whether the server rejects lobby option requests from non-host clients (the UI only hides controls, §4.12) | [U] (shared with 09/22) | `code/network/server.cpp` handlers for `NET_DARWINIA_SETGAMEOPTION*` |
| 28-U6 | Consequence of F21 (server-row navigation entries removed at the wrong index) in real browsing sessions | [I] | instrument `UpdateJoinGamePage` or step through with a shrinking list |
| 28-U7 | Whether `EclUpdate` skipping a window when another window is removed during the same iteration (§2.9) ever drops a frame of navigation in practice | [I] | trace `EclUpdate` with drop-down windows that self-remove in `DropDownWindow::Update` |
| 28-U8 | Whether any window in a shipped flow is opened while the Eclipse screen size is stale (screen size is only refreshed in `AdjustWindowPositions`) | [I] | audit every screen-mode change path (`RestartWindowManagerAndRenderer`, `UnlockFullGameButton`, `SetWindowed`) |
| 28-U9 | Whether a demo-key player whose demo limitations forbid hosting can still host: `App::HostGamePermitted()` is consulted only to set the no-op `m_disabled` flag on the Host button (F17), so the UI does not block it | [I] design-critical (demo gating) | check `App::StartMultiPlayerServer` / `ClientToServer::GetDemoLimitations` and the metaserver side ([22](22_determinism_and_networking.md), [09](09_meta_systems.md)) |

<!-- from 29_data_formats_reference.md -->
### From 29_data_formats_reference.md

| Id | Question | Tag | What would resolve it |
|---|---|---|---|
| 29-U1 | Did any retail archive (e.g. `patch.dat`) ship `levels/level_enum.txt`, without which "Master of Multiwinia"/"Explorer" are never awarded? (§4.2) | [U] | list a retail `main.dat` / `patch.dat` |
| 29-U2 | Was `data/default_preferences.txt` ever installed loose, so that first-run defaults (`WaitVerticalRetrace = 0`, `NetworkClientPort = 4001`) applied? (§5.1) | [U] | check `tools/init.list` against a retail `init.dat`, or a clean retail install |
| 29-U3 | Did retail editions ship complete Japanese/Korean/Portuguese language files (the tree's are ~10 % complete with no English fallback)? (§6.1) | [U] | inspect a retail `language.dat` and the `tools/*language.list` used for each SKU |
| 29-U4 | Are data-only marker names (`MarkerIsland*`, `MarkerTrunk*to*`, `MarkerFloat*`, …) looked up through a `sprintf` pattern the literal grep missed? (§8.1) | [U] | full call-graph search of `LookupMarker` arguments |
| 29-U5 | Exact `tools/Rar.exe` version and whether retail `.dat` files used RAR 2.9 or 3.x method numbering (`animator.dat` shows unpack version 29) (§10.1) | [U] | run `Rar.exe` without arguments, or read a retail `.dat` header |
| 29-U6 | Exact failure mode of a NULL token in release (`atoi(NULL)` access violation vs CRT invalid-parameter termination) (§1.4) | [U] (immaterial for a re-implementation) | run the original with a truncated file |
| 29-U7 | Was the shipped `MultiwiniaSupport.exe` built from `contrib/XCrashReport` unchanged? (§11.14) | [U] | compare resources/strings of the shipped binary |
| 29-U8 | What `animator.exe` writes beyond `AnimatedPanel::Write` (no source in tree) (§8.6) | [U] | run the tool and diff its output |
| 29-U9 | Contents of `patch.dat` and `branding.dat` (could supply missing icons such as `icons/compass_empty.bmp`) (§8.7, §10.1) | [U] | inspect the archives of a retail install |
| 29-U10 | Whether `new CameraMount()` zero-initialises the POD under MSVC 2005 (decides whether a 63+ character mount name is terminated) (§4.1.11) | [I] | compile a test with the original toolchain |
| 29-U11 | Hang on an unterminated `[` in a phrase (`inOffset += len + 1` with `len = −2`) — derived from arithmetic, not observed (§6.1) | [I] design-critical for localisation tooling | add an unterminated `[` to a test language file and run |
| 29-U12 | Letters larger than the 1 500-byte receive buffer are dropped and resent forever (§3.5) | [I] design-critical for the network format | send an oversized letter in a LAN test |
| 29-U13 | Marker `front` sign convention differs from fragments in the exporter (marker along +Y, fragment along −Y of the Max node) (§8.2) | [I] | export a test scene with the plug-in |
| 29-U14 | A locally modified map desyncs instead of being rejected because no map CRC is exchanged before a match (§4.6) | [I] design-critical | two-client test with one edited map |
| 29-U15 | Whether replaying `serverHistory.dat` reproduces a match (no version header; seq-id equality quirk) (§11.6) | [I] | record and replay a LAN match with `RecordDemo` 1/2 |

---

<!-- from 30_scripting_tutorial_and_cutscenes.md -->
### From 30_scripting_tutorial_and_cutscenes.md

| Id | Item | Class | Why it matters | What would resolve it |
|---|---|---|---|---|
| 30-U1 | Whether newly spawned, uninstructed player Darwinians wander into port range of ControlStation 51 (224.6 wu from spawn 3) so that radar dish 4 becomes player-owned and Tutorial 2 can start showing tabs (§3.9 a) | [I] design-critical | If they do not, Tutorial 2 shows nothing at the start and the player must discover the station unaided | Runtime observation of Tutorial 2, or a simulation of Darwinian idle wandering (`Darwinian::SearchForRandomPosition`, `SearchForPorts`) from spawn 3 |
| 30-U2 | Whether CPU team 2 reliably captures spawn 83 before the player in Tutorial 2 (§3.9 b) | [I] design-critical | Decides whether the formation-attack and break-formation lessons are ever shown | Repeated runtime observation of Tutorial 2 |
| 30-U3 | Whether `RecordDemo = 1` / `2` reproduces a Multiwinia match (history includes the lobby; team/client layout, sync seed and live client commands must match) (§7.1) | [U] | Determines whether a replay feature can be built on the existing history format unchanged | Record a local match with `RecordDemo 1`, replay with `RecordDemo 2`, compare per-frame sync bytes and final scores |
| 30-U4 | Behaviour when a script omits a mandatory 2nd–4th argument: `GetNextToken()` returns NULL and is passed to `iv_atof`/`atoi`/`stricmp` (§2.2) | [I] (likely access violation) | Robustness of authored scripts | Run a script with a truncated `CamMove` line under the original build |
| 30-U5 | The local player is team 0 in both tutorials (Tutorial 2 hard-codes team 0) (§3.6) | [I] strong (the local client requests its team before any CPU team; the server assigns the first free slot) | A different assignment would make Tutorial 2 test the wrong team | Trace `Server` team-slot assignment ([22](22_determinism_and_networking.md)) during a tutorial start |
| 30-U6 | **Resolved [C]:** `GlobalWorldGameLoop` (where `Script::Advance` is also called) sets `m_atMainMenu = true` itself before its loop and breaks on the first check, so its body never runs in any mode (§2.1) | [C] (closed) | The second tick site is inert | `code/main.cpp:GlobalWorldGameLoop` (L708, L711) |
| 30-U7 | Duration of a late-join catch-up replay (≈ 10 min for a 20-min match at +20 frames/s) (§7.2) | [I] | Join-in-progress UX | [22](22_determinism_and_networking.md) measurement |
| 30-U8 | Whether `GL_BLEND` is enabled when the credits loader (script op `RunCredits`, outro camera) draws its fading clear quad | [U] | Visual only, legacy path | GL state trace at `Script::RunCommand_RunCredits` ([09](09_meta_systems.md)) |

Resolved during synthesis (not counted): P19 U1 (music event names are matched with `stricmp`), U2 (map CRCs reproduce), U3 (dish 4 is captured through ControlStation 51 — mechanism; the residual question is 30-U1), U4 (`_kbd`/`_xin` resolution, see [29](29_data_formats_reference.md)).

<!-- from 31_platform_build_and_tools.md -->
### From 31_platform_build_and_tools.md

| Id | Question | Tag | What would resolve it |
|---|---|---|---|
| 31-U1 | Does `ReleaseDirect3D` compile at this commit, or was it built from another branch/with uncommitted fixes? Four breakages found (§7.9) | [I] strong "no" | build `ReleaseDirect3D` with VS2008 + DirectX SDK March 2008 |
| 31-U2 | Are `D3DRS_LINEWIDTH` and `IDirect3DDevice9::Resolve`/`D3DRESOLVE_RENDERTARGET0` absent from every PC DirectX 9 SDK referenced by `Common.vsprops`? | [I] | inspect `d3d9.h`/`d3d9types.h` of the SDK versions listed in §7.2 |
| 31-U3 | Exact x87 control word on the main thread after D3D9 `CreateDevice`/`Reset` in `[B:D3D]` (expected 24-bit precision) | [I] | read `_control87(0,0)` after device creation at run time |
| 31-U4 | Behaviour of `throw;` with no active exception under the VS2008 static CRT inside `AppMain`'s `__try` (SEH 0xE06D7363 reaching `RecordExceptionInfo`, or `terminate()` → `abort()` bypassing it) | [U] | trigger an `AppReleaseAssert` in a Release build |
| 31-U5 | Which authentication-key file the retail build read (`authkey.dev` here; installers and the support INI know only `authkey`), and whether retail used `metaserver-mwdev` or a production host | [U] | inspect a retail executable/install |
| 31-U6 | Behaviour of the absent tools: `autopatch.exe`, `copy_assist.exe`, `ProfileLocator.exe`, `ncftpput.exe` and, under `tools/`, `build_ambx_dat.bat`, `convert_bmp_to_tex.cmd`, `macosx/add-appcast-url.rb`, `macosx/add-svn-revision.rb`, `PasswordGenerator/main.cpp`; in particular whether the auto-patcher writes `userprefs.txt`, `game-history.txt`, `stdout.txt` and the `backup_*`/`revert_*` folders | [U] | obtain the release-branch `tools/` |
| 31-U7 | Contents and producers of `ambx.dat`, `branding.dat`, `patch.dat` | [U] | retail media / patch archives |
| 31-U8 | Was `default_preferences.txt` ever shipped loose (or by a patch) so that first-run defaults applied in packaged installs (§8.9)? | [U] | retail install tree |
| 31-U9 | Exact role of `tools/SlashSort.exe` | [I] | run it on a `TRACK_MEMORY_LEAKS` log |
| 31-U10 | Which string the Russian build's `subst.pl … retail\... retail.ru` patch targeted (absent from this tree) | [U] | diff a Russian retail exe against an English one |
| 31-U11 | ~~Is the debug menu (hence the Grabber window and profiler) reachable from the shipped FULL build's input bindings?~~ **Resolved [C]:** no. `DebugMenu` is never constructed in any build (its only opener, the F1 call in `code/user_input.cpp:UserInput::AdvanceMenus`, is commented out, and there is no `new DebugMenu`); `DebugKeyBindings::GrabberButton` has no caller at all, so the Grabber/AVI path is [B:DEAD] (§3.12); the profiler window is reachable only from the cheat window in `TARGET_DEBUG` builds (`code/interface/cheat_window.cpp` calls `DebugKeyBindings::ProfileButton`; [32 §3.2](32_debug_test_and_performance.md#32-debugkeybindings--the-18-static-entry-points-c-codeinterfacedebugmenuhdebugkeybindings-codeinterfacedebugmenucpp)) | closed | — |
| 31-U12 | Name of the map file the baseline `Release` link produces (linker default) — `tools/traceblackbox.bat` expects `darwinia.map` | [I] | link the Release configuration |
| 31-U13 | Is QPC monotonic across the main (0xAA) and worker (0x55) affinity sets on the 2008 target hardware? Cross-core skew would surface directly in `GetHighResTime` (§4.1) | [I] | measure on period hardware; irrelevant for the new engine |
| 31-U14 | Was the shipped Windows binary built from this commit? (`DARWINIA_VERSION "dev"`, development metaserver host, versionless `.rc`) — design-critical because every [B:FULL] claim assumes it | [I] probably a development snapshot close to release | compare behaviour/strings with a retail `multiwinia.exe` |
| 31-U15 | Did the Mac 1.3.1 port play lockstep games with Windows clients bit-exactly (its `-mfpmath=387`/`-mno-fused-madd` flags suggest the intent)? | [U] | cross-platform network test with period binaries |

Design-critical inferences (not unknowns, but [I]): GL and D3D clients in one game are a desync risk (§7.10); no map-content check between peers (§10.3); worker-thread crashes bypass the crash reporter (§6.2); a hand-typed authentication key is not reloaded at the next start (§5.3); first-run defaults ignore `default_preferences.txt` in packaged installs (§8.9).

<!-- from 32_debug_test_and_performance.md -->
### From 32_debug_test_and_performance.md

| Id | Question | Kind | What would resolve it |
|---|---|---|---|
| 32-U1 | What `throw;` with no active exception does in `AppReleaseAssertFailed` under the shipped CRT: caught by the `AppMain` SEH filter (ERRORLOG + minidump + support tool after the black box) or `terminate()` → `abort()` bypassing it | [U] | trigger a release assert in a `Release|Win32` build and observe which files appear |
| 32-U2 | Real per-frame and per-system cost, draw-call counts and the population at which the shipped build drops below 30 / 15 fps on the EULA minimum hardware — no measurement exists in the tree | [U] | run a DEBUG build with the Profiler window (glFinish off) or an external profiler on `mp_benchmark_1` and a 2000-population map |
| 32-U3 | Whether the hardware-compatibility (and assault-stress) SKUs were built with `BENCHMARK_AND_FTP`, and how they started `mp_benchmark_1` / `mp_assault_stress` although `GameTypes none` makes `GameMenu::AddMap` discard them (the hwcompat packaging copies `ncftpput.exe`) | [U] / design-critical [I] that hwcompat was the benchmark SKU | build logs or the `universal_include.h` revision used for those SKUs; their data archives |
| 32-U4 | Whether the dxdiag filter ever captured `GFX` / `SND` lines (the reader stops at the first blank line) | [I] only `SYS` | run `dxdiag /t` on a period machine and feed it to `BenchMark::BuildDxDiag` |
| 32-U5 | Whether the black-box EBP walk yields a usable trace in `Release|Win32` (`/O2` implies frame-pointer omission on x86 unless overridden) | [I] unreliable | inspect the compiled prologues or the `cl` command line of the shipped exe |
| 32-U6 | What the MSVC 2005 CRT prints for the malformed formats `"%2.1 Kb/sec"`, `"%2.1 secs"` (network window) and `"%4.1"`, `"%4.0"` (profiler rows) | [U] | run the window |
| 32-U7 | Whether the testbed compiles and runs at this commit (never-defined macro; `!GetTestBedMode() == TESTBED_ON` precedence slip in `code/UI/GameServerButton.h`; out-of-range reads noted in §6.2) | [U] | build with `TESTBED_ENABLED` and run a server and a client |
| 32-U8 | Which build the beta release note "debugging code … to track rand sync errors" refers to (`TRACK_SYNC_RAND` is commented out for the beta targets here; beta groups do have snapshot IFrames) | [U] | beta-build configuration history |
| 32-I1 | Network-safety classification of the 23 cheats (§4.3), in particular "Allow Arbitrary Placement" desyncing on the first otherwise-invalid placement and "Clear Resources" being simulation-safe | design-critical [I] | a two-client DEBUG session using each cheat while watching the network window |
| 32-I2 | The catch-up spiral of §12.2 (a slow client spends up to 0.3 s per frame simulating, lowering fps further) and the 0-slice dead band for 1–3 pending slices | design-critical [I] (code [C], effect inferred) | instrument `GetNumSlicesToAdvance` on a throttled client |
| 32-I3 | "Give all research" being a no-op in Multiwinia | [I] | run it in a DEBUG match and compare research levels |
