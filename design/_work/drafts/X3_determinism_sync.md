# X3_determinism_sync — Simulation determinism model (RNG, floats, ordering, sim-vs-cosmetic, sync checks)

Scope: cross-cutting over the whole live codebase (`code/**`, `contrib/systemiv/lib/**`, `targets/vs2005/*`). Build baseline: see `build_matrix.md` (`Release|Win32` + `TARGET_PC_FULLGAME`, shipped as `multiwinia.exe`). P15 documents the wire protocol; this draft documents the determinism contract that protocol relies on.

Generated tables: `drafts/X3_syncrand_sites.csv` (every synced-RNG call site, produced by `scripts/X3_rng_sites.py`), `drafts/X3_unsynced_rng_sites.csv` (every unsynced RNG call site with context classification, same script).


## [→ 22_determinism_and_networking] 1. Determinism model at a glance

| Question | Answer | Evidence |
|---|---|---|
| Model | **Deterministic lockstep over a server-sequenced command stream.** Every peer (host included) runs the complete simulation; only commands, `TeamControls` (ALIVE) and lobby options travel; the server never simulates. One UPDATE letter = one *frame* = 0.1 s **real** time = `0.1 × gameSpeed` **game** time, executed as 10 *slices*. | [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`; `code/network/server.cpp:Server::AdvanceIfNecessary`, `Server::SendLetter`; P15 §1–2 |
| What must be bit-identical on every peer | (1) the ordered command stream, (2) the synced RNG state (MT19937, one seed per server session), (3) every simulated field touched by `Location::Advance`, `ParticleSystem`-independent (§5), (4) static data: level file, `data/stats.txt`, shapes used for hit tests, terrain seeds, (5) per-process statics that sim code reads (§7) | [C]/[I] this draft |
| Numeric environment it relies on | Win32 **x87** code (no `/arch:SSE2`), x87 precision control forced to **53-bit mantissa** per thread, `/fp:precise` in the shipped config, every transcendental and `sqrt` result rounded to **IEEE single** by `iv_trunc_low_nibble` | §3 |
| Ordering it relies on | Strictly serial, single-threaded, index-ordered iteration of slot arrays (`FastDArray`/`SliceDArray` with LIFO free lists), fixed slice schedule; no hash-map or pointer ordering anywhere in the sim | §4 |
| How divergence is detected | Per frame a 1-byte value is sent to the server. **Baseline build: the byte is the next synced-RNG draw** (`255 × syncfrand(1)`), so only divergence of the RNG stream position/state is detected; debug/beta builds hash a world snapshot (SHA-1). | [C] `code/network/iframe.cpp:IFrame::Generate`; §6 |
| What happens on desync | Server broadcasts NETSYNCERROR (sequenced); every client pauses the simulation permanently (`App::GamePaused`), shows a red banner, writes `syncerror-*.txt`. No resync, no recovery. | [C] §6; P15 §8 |
| Seed | `SHA1(threadId XOR lowbits(GetHighResTime()), startTime)` word 0, chosen by the server at `Server::Initialise`, delivered in CLIENTID, applied with `syncrandseed` before history replay | [C] `code/network/server.cpp:GenSyncRandSeed`, `clienttoserver.cpp:ProcessServerLetters` |

```mermaid
sequenceDiagram
    participant S as Server (host process, 10 Hz real time)
    participant C as Every client (incl. host client)
    S->>C: UPDATE seq=N {commands since last tick}
    Note over C: GetNextLetter (strict seq order)
    C->>C: ProcessServerUpdates(N) — apply commands (may draw syncrand)
    C->>C: g_sliceNum=0; GameTimer::Tick() (+0.1×speed, speed blend)
    C->>C: GenerateSyncValue(): baseline = 255×syncfrand(1)
    C->>S: SYNCHRONISE {seq=N, byte}
    loop slices 0..9 (spread over render frames by GetNumSlicesToAdvance)
        C->>C: if !GamePaused: Location::Advance(slice); ParticleSystem::Advance(slice)
        C->>C: slice 8: Multiwinia::Advance() (runs even when paused)
    end
    S->>S: HandleSyncMessage: all bytes for seq N in? compare → NETSYNCERROR(seq N) if odd one out
```

## [→ 22_determinism_and_networking] 2. Random number sources

### 2.1 Inventory of every generator in the live code

| # | Generator (call form) | Algorithm and state | Seeding | Output (as implemented) | Synced? | Thread(s) that call it | Ref |
|---|---|---|---|---|---|---|---|
| G1 | `syncrand()` → `_syncrand()` | **MT19937** (Matsumoto–Nishimura 2002 `init_genrand`, standard tempering 11/7/15/18, masks `0x9d2c5680`/`0xefc60000`, `MATRIX_A 0x9908b0df`); state = file-static `unsigned long mt[624]`, `int mti` (624+1 = unseeded), `mag01[2]`, `s_randseed`, `s_callCount` | `syncrandseed(s)`: `s == (unsigned long)-1` → 5489; else `s`. If `_syncrand` runs before any seed it calls `syncrandseed()` whose default argument is **0** (not 5489) | 32-bit unsigned `[0, 2^32−1]` | **yes** | main thread (frames) and the loading worker thread (`Location::Init`) — never concurrently [I] | [C] `contrib/systemiv/lib/math/random_number.cpp:syncrandseed`, `_syncrand` |
| G2 | `syncfrand(r)` → `_syncfrand(r)` | `(_syncrand() * r) / 4294967296.0` in double | G1 | `[0, r)` for r > 0 | yes | as G1 | [C] `_syncfrand` |
| G3 | `syncsfrand(r)` → `_syncsfrand(r)` | `(_syncfrand(1.0) − 0.5) * r` | G1 | **`[−r/2, r/2)`** (the header comment says `−range < result < range`; wrong) | yes | as G1 | [C] `_syncsfrand`, `random_number.h` |
| G4 | `FRAND(x)`, `SFRAND(x)` inside `Vector3(…)` / `Vector3::Set(…)` | Marker structs `RandomFloat{v}` / `SymmetricRandomFloat{v}`; the templated `Vector3` constructor/`Set` calls `FromRand` on x, then y, then z, which calls `_syncfrand(v)` / `_syncsfrand(v)`. Purpose (source comment, paraphrased): C++ leaves argument evaluation order unspecified, so draws written directly as constructor arguments would be consumed in compiler-dependent order | G1 | as G2/G3 | yes | as G1 | [C] `code/lib/vector3.h` (`RandomFloat`, template ctor), `code/lib/vector3.cpp:FromRand` |
| G5 | `RandomNormalNumber(mean, range)`, `RandomApplyVariance(num, var)` | Irwin–Hall: sum of 12 `syncfrand(1.0)`, `(s − 6)·range/3 + mean`, clamped to `mean ± range` | G1 | clamped normal | yes | — | [C] `random_number.cpp`; **no callers** in the live code [C grep] |
| G6 | `AppRandom()`, `frand(r)`, `sfrand(r)`, `AppSeedRandom(s)` | **MSVC `rand()` clone LCG**: `holdrand = holdrand·214013 + 2531011` (`static long`, initial 1), returns `(holdrand >> 16) & 0x7fff`; `frand(r) = r·AppRandom()/32767`, `sfrand(r) = (0.5 − AppRandom()/32767)·r` | `AppSeedRandom(1)` in `Location::Init` (after `LoadLevel`, before landscape), in `GlobalInternet` ctor (start-up); reseeded with `m_seed` by every tree geometry pass; `landscape_renderer` reseeds per vertex with `x | (y + AppRandom())`; never reseeded otherwise | `AppRandom ∈ [0, 32767]`; `frand ∈ [0, r]` **inclusive**; `sfrand ∈ [−r/2, r/2]` (header comments claim `[0,r)` and `(−r, r)`) | **no** | main thread; loading worker thread (tree hit-sphere pass, landscape colours); **MetaServer WAN-registration thread** (`sfrand(3.0)`); sound code on the main thread | [C] `random_number.cpp:AppRandom/frand/sfrand`; `code/location.cpp:Location::Init`; `code/worldobject/tree.cpp:Tree::Generate*`; `contrib/systemiv/lib/metaserver/metaserver.cpp:ServerRegisterWANThread` |
| G7 | CRT `rand()` | MSVC CRT LCG, per-thread state under the static multithreaded CRT (`RuntimeLibrary="0"`) [I: CRT behaviour] | `srand` is called only in dead code (`TESTBED_ENABLED` block of `GameMenuWindow.cpp`, `egames_loader.cpp`) → default seed 1 per thread | `[0, 32767]` | no | main thread | [C] `code/worldobject/tree.cpp:Tree::Advance` (leaf particles), `darwinian.cpp:Darwinian::AdvanceInfection` (plague particles), `randomiser.cpp:Randomiser::Render`, `multiwiniahelp.cpp:MultiwiniaHelp::AttachTo3d`, `portal.cpp` [B:LEGACY] |
| G8 | `LandscapeTile::LandscapeAppRandom/Landscapefrand/Landscapesfrand` | Same LCG constants as G6 but **per-tile private** `m_holdrand` | `LandscapeAppSeedRandom(m_randomSeed)` from the level file before each tile's generation | as G6 | deterministic by construction (sim-relevant: heightmap) | loading worker thread | [C] `code/landscape.cpp`; P11 §23 |
| G9 | Orphan `::syncrand()` in `code/lib/math_utils.cpp` | A second MT19937 copy (`init_genrand(5489)` on first use, file-static state) | none exported | — | — | **never called**: every `syncrand()` in code is rewritten to `_syncrand` by the `random_number.h` macro, `math_utils.cpp` does not include that header and `math_utils.h` does not declare the function [C grep]; dead duplicate [I] | [C] `code/lib/math_utils.cpp:syncrand` |
| G10 | Server session seed | `MakeRandom() = GetCurrentThreadId() XOR (low 32 bits of the double GetHighResTime())`; `GenSyncRandSeed = (int) SHA1(LE32(random) ‖ LE64(startTime))[0]` | per `Server::Initialise` | int | it *is* the seed | main thread | [C] `code/network/server.cpp:MakeRandom`, `GenSyncRandSeed`, `Server::Initialise` |

Debug-only forms [B:DEBUG] (`TRACK_SYNC_RAND`, defined only by `TARGET_DEBUG`; commented out for `TARGET_BETATEST_GROUP_ALL`): `syncrand()` → `DebugSyncRand(__FILE__, __LINE__)`, `syncfrand(x)` → `DebugSyncFrand(file, line, 0.0f, x)`, `syncsfrand(x)` → `DebugSyncFrand(file, line, -x/2.0, x/2.0)` computing `min + (r·range)·(1/2^32)`. Two consequences [C code, I consequence]: (a) the tracked `syncsfrand` is **not bit-identical** to the untracked one (`−x/2 + r·x·2^−32` vs `(r·2^−32 − 0.5)·x` round differently), so tracked and untracked builds cannot play together even before `INCLUDE_THEBEAST` changes `Crate::NumCrateRewards`; (b) the macro does not parenthesise `x` — harmless today because no call passes a top-level `+`/`−` expression [C: scripted check over all 148 `syncsfrand` arguments]. `code/worldobject/snow.cpp` and `code/worldobject/spirit.cpp` `#undef TRACK_SYNC_RAND` before including `random_number.h`, so their draws are not logged even in debug builds (and use the untracked formula) [C].

### 2.2 Seeding and lifetime of the synced stream

1. **One stream per server session = one match.** `App::ShutdownCurrentGame` calls `ClientToServer::ClientLeave` and deletes `m_server`; the next game creates a new `Server` with a new seed [C] `code/app.cpp:App::ShutdownCurrentGame`, `Server::Initialise`.
2. The seed travels in the **unsequenced** CLIENTID reply; on receipt every client calls `syncrandseed(randomSeed)`, resets `g_lastProcessedSequenceId = −1`, `Multiwinia::Reset()`, option versions, and then replays the server history from sequence 0 [C] `ClientToServer::ProcessServerLetters` (CLIENTID branch). This is the only `syncrandseed` call in `code/` [C grep].
3. In the lobby no synced draw happens: `IFrame::Generate` returns before drawing when there is no `Location`, `Location::Advance`/`Multiwinia::Advance` are not called without a location [C] `iframe.cpp:IFrame::Generate`, `ProcessAllServerLetters` (`if( loc )`). No synced call exists in UI, render, sound or input code [C: scripted census, §2.4 — 0 of 655 baseline sites are in `code/UI`, `code/interface` (except the `CHEATMENU_ENABLED` window), `code/sound`, or in functions whose name contains `Render`/`Draw`].
4. START_GAME is processed inside an UPDATE letter; `m_loadingLocation` stops further letter processing until `Location::Init` (run on the loading-screen **worker thread** by `DoEnterLocation`) has finished; Init's synced draws (unused-team colours `syncrand() % MULTIWINIA_NUM_TEAMCOLOURS / 2` retry loop, building/AI initialisation, starting power-ups) therefore occur at the same logical point on every peer [C] `code/main.cpp:EnterLocation/DoEnterLocation`, `code/location.cpp:Location::Init`.
5. Late joiners re-simulate the whole match from sequence 0 (catch-up: 20 slices per call while > 20 frames behind, 0.3 s budget per render frame) [C] `ClientToServer::AdvanceAndCatchUp`, `GetNumSlicesToAdvance`.

### 2.3 RNG discipline rules the code follows (and where it breaks them)

| Rule | Status | Evidence |
|---|---|---|
| R1 Every gameplay decision draws from G1–G4 | Followed in all inspected sim code (crate outcomes, AI choices, spawn timers, damage rolls, conversions, scatter) | [C] census §2.4; P04/P05/P07/P08 notes |
| R2 Fixed draw order inside an expression | Enforced by `FRAND/SFRAND` for `Vector3`; **one violation**: `SpiritStore::AddSpirit` builds `Vector3(syncsfrand(a), syncsfrand(b), syncsfrand(c))` (three raw draws as constructor arguments → x/y/z permutation is compiler-dependent) — [B:LEGACY] (SpiritStore only used by the blocked Factory) | [C] `code/worldobject/spiritstore.cpp:SpiritStore::AddSpirit`; scripted multi-draw-per-statement search found no other case |
| R3 Draw *count* must not depend on local state | Followed except where local state leaks (§5.4). Cosmetic effects inside `Advance` sometimes draw from the synced stream (e.g. `ArmyAnt::Advance` leg shape `syncrand()%3` every tick, `Snow`/`Zombie` wobble, `Meteor` fire size, `Triffid` fire size, `GunTurret` muzzle sparks, `Squadie::Attack` brass, `Missile` trail size) — harmless but they must be preserved draw-for-draw if bit-compatibility were ever wanted | [C] CSV column `cosmetic_hint`; P04 X3.2, P05 table |
| R4 Variable-length rejection loops | Deterministic given identical inputs, several unbounded (P08 list: `CaptureCrate`, `RunNuke`, `RunSpaceShip`, AI area loops, `Location::Init` colour loop) | [C] P08 §17; `Location::Init` |
| R5 Short-circuit evaluation changes stream length | e.g. `syncfrand(1.0) < 0.9` only evaluated when `m_rewardId == −1` in crate capture | [C] P08 §17 |
| R6 Unsynced generators never feed the sim | Followed, with one sanctioned exception: tree geometry (G6 reseeded from `Tree::m_seed`) produces the **simulation** hit sphere `m_hitcheckCentre/m_hitcheckRadius` used for fire spread (§2.6) | [C] `tree.cpp:Tree::Generate`, `RenderBranch`, `Tree::Advance` |

### 2.4 Census of synced call sites (generated)

Script: `OUT/_work/scripts/X3_rng_sites.py` (comment-stripping, evaluates every `#if/#ifdef/#elif/#else` against the baseline macro set, attributes each call to its enclosing function) → `OUT/_work/drafts/X3_syncrand_sites.csv` (columns `file,line,function,generator,argument,context,decides,cosmetic_hint,liveness,build,compiled,kind,preproc_conditions,statement`). `decides` is a heuristic (assignment target, `condition`, or the callee the value is passed to); `cosmetic_hint` flags draws whose value feeds `CreateParticle` or a sound trigger within the next 8 lines; `liveness` marks files whose classes are Darwinia-only in Multiwinia (from P05–P07 and `BuildingIsBlocked`).

| Measure | Value |
|---|---|
| Baseline (`[B:FULL]`) call sites | **655** (`syncfrand` 268, `syncsfrand` 148, `syncrand` 110, `SFRAND` 80, `FRAND` 49) in 56 files |
| … in files with only live classes / mixed live+legacy files / legacy-only files | 410 / 179 / 66 |
| … by enclosing-function class | `*Advance*`/tick 248, constructors/`Initialise`/`Read`/`Create*`/`Spawn*` 129, other sim functions (`Damage`, `Explode`, AI `Run*`, crate `Run*`, …) 278, render/UI/sound **0** |
| Non-baseline sites | 4: `code/interface/cheat_window.cpp` ×2 [B:DEBUG], debug branch of `IFrame::Generate` [B:DEBUG], `code/loaders/gameoflife_loader.cpp` [B:DEAD] |
| Draws flagged `cosmetic_hint` (live/mixed files) | 35 (e.g. `Armour::Advance` exhaust, `GunTurret::Advance` sparks, `Squadie::Attack` brass, `Spirit::BreakSpirit`, `Missile::Advance`, `Meteor::Advance`, `Triffid::Advance`, `EscapeRocket::AdvanceExploding`) — heuristic, includes a few false positives (e.g. `Triffid::Launch` egg timer is gameplay) |

Heaviest consumers (baseline sites): `darwinian.cpp` 84, `crate.cpp` 76, `location.cpp` 61, `ai.cpp` 56, `triffid.cpp` 36, `weapons.cpp` 23, `armour.cpp` 18, `gunturret.cpp` 18, `souldestroyer.cpp` 16, `spiritreceiver.cpp` 16 [B:LEGACY], `multiwinia.cpp`/`rocket.cpp`/`spam.cpp`/`spirit.cpp`/`tree.cpp` 14 each, `anthill.cpp` 13, `insertion_squad.cpp`/`sporegenerator.cpp` 12 [C] CSV. What each draw decides is listed per site in the CSV; the gameplay meaning of each is specified by the owning partition (P02 modes, P03–P05 entities, P06–P07 buildings, P08 crates/AI).

Per-frame fixed draws: exactly **one** per executed UPDATE frame for the sync value (`IFrame::Generate`), in every build [C].

### 2.5 Non-synced randomness inside simulation code — every live site, with verdict

Generated list: `OUT/_work/drafts/X3_unsynced_rng_sites.csv` (same script; 328 baseline call sites in compiled files: `sfrand` 144, `frand` 128, `AppRandom` 40, `rand` 9, `AppSeedRandom` 7; by context: `*Advance*` 132, other 99, render 53, init 19, loaders 11 [B:DEAD content], particle/explosion system 7, UI 4, sound 3). Every non-render site in simulation files was read; verdicts:

| Site(s) | What the unsynced value drives | Verdict | Ref |
|---|---|---|---|
| `Tree::GenerateBranches/GenerateLeaves/Generate` (`AppSeedRandom(m_seed)` then `frand` per branch) | branch geometry **and the simulation hit sphere** (leaf centroid, max leaf distance ×0.8) | deterministic **only** while nothing else touches G6 between the reseed and the end of the pass — see §2.6 race | [C] `code/worldobject/tree.cpp` |
| `Tree::Advance` fire particles (`frand`, `sfrand`), leaf drop (`rand() % (51 − leafDropRate)`) | particles only | cosmetic | [C] |
| `Entity::Advance`, `AntHill::Advance`, `Centipede::Advance`, `Triffid::Advance` — `m_renderDamaged = frand(0.75)·(…) > healthFraction` (with wall-clock `g_gameTime`) | render flicker flag stored in the sim object | cosmetic (never read by sim) [I] | [C] `code/worldobject/entity.cpp:Entity::Advance` |
| `Darwinian::AdvanceOnFire`, `AdvanceSacraficing` [B:LEGACY], `AdvanceHotFeet`, `AdvanceInfection` (`rand()%2`), `AdvanceOpeningCrate`, `SetBooster`, `GiveHotFeet`, `GiveMindControl`, `GiveRage`, `TakeControl`, `LoseControl` | particle counts/velocities; `AppRandom()%30` / `%100` gates the "OnFire" sound | cosmetic | [C] `darwinian.cpp` |
| `Darwinian::SearchForThreats` `frand(200) < 1.0` | "SeenThreatAttackHorn" sound, rate-limited by a function-static game-time stamp shared by all Darwinians | cosmetic | [C] |
| `Location::Bang` (core/debris particle counts, sizes), `Location::PromoteDarwinian/DemoteOfficer`, `Task::Promote/Demote` (flash particles) | particles | cosmetic | [C] `code/location.cpp`, `code/taskmanager.cpp` |
| `Crate::RunExtinguisher` (200 fire particles, rejection loop on `sfrand`) | particles | cosmetic (loop length local) | [C] `code/worldobject/crate.cpp` |
| `Laser::Advance`, `TurretShell::Advance`, `Fireball::Advance`, `FlameGrenade::Advance`, `ControllerGrenade::Advance`, `Rocket::Explode` (firework colours/cores), `Lightning::Advance`, `Meteor::Advance` (positions), `Harvester::AdvanceEngines`, `SpaceShip::AdvanceEngines`, `EscapeRocket::Advance`, `FuelGenerator::Advance`, `FuelStation::BoardRocket`, `EruptionMarker::Advance`, `SmokeMarker::Advance`, `LaserFence::Spark`, `Incubator::SpawnEntity` [content LEGACY], `ControlTower::Advance` [content LEGACY], `Engineer::AdvanceResearching` [B:LEGACY], `Tentacle::*` [B:LEGACY], `SpamInfection::AdvanceAttacking*`, `Shaman::SacraficeDarwinians` [B:LEGACY] | particles | cosmetic | [C] `code/worldobject/weapons.cpp` et al. |
| `SoulDestroyer::SoulDestroyer` (`m_spiritPosition[]`), `SoulDestroyer::Attack` (`AppRandom() % SOULDESTROYER_MAXSPIRITS` slot, value = `GetHighResTime()`) | the trail of captured-spirit sprites (render) | cosmetic; the source comment says it does not need to be sync-safe [C] | [C] `code/worldobject/souldestroyer.cpp` |
| `Explosion`/`Tumbler` (`frand`, `sfrand`, `AppRandom`), `Particle::Initialise/Advance` | shatter debris and particles; advanced with wall-clock `g_advanceTime` (explosions) or inside the sim slice (particles) | cosmetic; nothing in the sim reads particles or explosions (`CreateParticle` returns `void`) | [C] `code/explosion.cpp`, `code/particle_system.cpp` |
| `Camera::Advance` shake (`sfrand`) | local camera | local | [C] `code/camera.cpp` |
| `LevelFile::ParseInstantUnits` centipede copies `sfrand(60)` | instant-unit positions — **sim state, and G6 is unseeded at that point** (`LoadLevel` runs before `AppSeedRandom(1)`) | unreachable in Multiwinia: copies are made only for team 1 centipedes when `loadDifficultyFactor > 1`, i.e. `m_difficultyLevel > 0`, which the MW build never sets — but see §7 (difficulty) [C code, I reachability] | [C] `code/level_file.cpp:LevelFile::ParseInstantUnits`, `code/app.cpp:App::UpdateDifficultyFromPreferences` |
| `ResearchItem::ResearchItem`, `Spam::Spam` (`m_front.RotateAroundY(frand(2π))`) | building orientation (hashed in debug IFrames) | [B:LEGACY] (both blocked) — would desync if reachable | [C] |
| `GlobalWorld::TransferSpirits`, `SphereWorld::AddLocation`, `GlobalInternet` | Darwinia global world | [B:LEGACY] | [C] |
| `MultiwiniaHelp::AttachTo3d` (`rand() % numPlayers`) | which help tab to show | local UI | [C] |
| `Randomiser::Render` (`rand() % NUM_TEAMS`) | particle colour in a render function | cosmetic (render calling `CreateParticle` — particle count depends on frame rate) | [C] `code/worldobject/randomiser.cpp:Randomiser::Render` |
| `SoundInstance::OpenStream`, `GetAttachedObject`, `SoundParameter::Recalculate` | sample choice, emitter choice, randomised parameters | cosmetic (P16) | [C] `code/sound/*` |
| `EclRegisterWindow` (`AppRandom` window placement), `NetworkQuality::Write` [B:DEAD], loaders [B:DEAD] | UI / test | local | [C] |

### 2.6 The shared non-synced LCG is a sim input for trees — and it is not thread-safe

[C] code facts: `holdrand` is a single unsynchronised global; `Tree::Generate` (called by `Tree::SetDetail(1)` from `Building::SetDetail` paths during level load on the **worker thread**, by `CreateAnotherTree` at runtime, and by `Tree::RenderAlphas` whenever the display lists were flushed) first compiles the leaf display list (`GenerateLeaves` — accumulates `m_hitcheckCentre`/`m_numLeafs`), then reseeds and runs a radius pass on the calling thread; `Resource::CreateDisplayListAsync` runs the compile synchronously on the main thread or, from another thread, queues it to the loading screen and blocks until done. Concurrent G6 users exist: the loading-screen loop calls `SoundSystem::Advance` (sample choice `AppRandom`) on the main thread while the worker thread runs the radius pass; the MetaServer WAN registration thread calls `sfrand(3.0)` each period on the host. [I] consequence: a G6 draw from another thread between the reseed and the end of a tree's pass changes that tree's crown sphere on that peer only → fire-spread (`Tree::Advance` neighbour test `theirRadius + ourRadius >= distance`) diverges → desync detectable only once it changes synced draws. The beta release notes list "synchronization errors related to trees and height maps" as fixed [C] `docs/multiwiniareleasenotes-beta.txt`, consistent with this area being fragile. **Design rule for the new game**: geometry that feeds the simulation must use its own local generator object, never a process-global one.

## [→ 22_determinism_and_networking] 3. Floating-point model

### 3.1 Numeric types used by the simulation

| Item | Fact | Ref |
|---|---|---|
| Geometry | `Vector2`, `Vector3`, `Matrix33`, `Matrix34`, `Plane` store **double** | [C] `code/lib/vector3.h`; P13 |
| Sim object fields | overwhelmingly double (301 `double m_…` members across `code/worldobject/*.h`) | [C] grep |
| Sim fields declared **float** | `Task::m_lifeTimer`, `Task::m_startTimer` (task timers), `Tree::m_corruptCheckTimer`, `SpaceShip::m_abductionTimer`, `EscapeRocket::m_fuelBeforeExplosion`, `GunTurret::m_firedLastFrame`, `RRSolarPanelInfo::m_range` (AI Rocket-Riot helper struct, `ai.h`), `Team::m_accurateCPUTakeOverTime`, `EntityGridCacheElement::{m_timeIndex, m_worldX, m_worldZ, m_rangeSqd, m_halfRangeSqd}` (sim-relevant query cache) — each assignment rounds a double to binary32 | [C] `code/taskmanager.h`, `code/worldobject/tree.h`, `spaceship.h`, `rocket.h`, `gunturret.h`, `ai.h`, `code/team.h`, `code/entity_grid_cache.h` |
| float locals in sim decisions | `Task::FindDarwinian` (nearest distance), `TaskManager::IsValidTargetArea`, `Multiwinia::AdvanceAssault` (`bestTime`), `GunTurret::Advance` (`range`), `SpaceShip::AdvanceAbducting` (`distance`), `Teleport::EnterTeleport` | [C] scripted scan (`OUT/_work/scripts/X3_local_state_sites.py`, pattern `float-decl`) |
| float literals | e.g. `Location::Bang` `numCores = _range * _damage * 0.01f` (float literal promoted) | [C] `code/location.cpp:Location::Bang` |
| Accumulated game time | `GameTimer::m_gameTime += 0.1 × m_gameSpeed` in double each frame (0.1 not representable) → P02 D1: "1-second" countdowns of `t -= 0.1; if (t <= 0)` take **11** frames | [C] `code/gametimer.cpp:GameTimer::Tick`; P02 §8 D1 |

### 3.2 The `iv_*` wrappers and `iv_trunc_low_nibble`

All of `iv_sin, iv_cos, iv_tan, iv_sqrt, iv_pow, iv_exp, iv_log, iv_asin, iv_acos, iv_atan` are macros `iv_trunc_low_nibble(<crt>(double(x)))`; `iv_abs` is an exact inline; `iv_atof(s) = iv_trunc_low_nibble(atof(s))`, `iv_wtof` likewise [C] `code/lib/universal_include.h`, `contrib/systemiv/lib/math/math_stubs.cpp`. Call-site lines in `code/` + `contrib/systemiv/lib`: `iv_sin` 296, `iv_cos` 159, `iv_sqrt` 69, `iv_pow` 19, `iv_acos` 7, `iv_asin` 5, `iv_exp` 3, `iv_log` 3, `iv_atan` 2, `iv_tan` 1, `iv_abs` 84, `iv_atof` 122 [C grep].

> **Verbatim excerpt** — `contrib/systemiv/lib/math/math_stubs.cpp:iv_trunc_low_nibble` (Introversion licence, ≤15 lines)
```cpp
double iv_trunc_low_nibble(double x)
{
	volatile float f = (float)x;
	return f;
}
```

- **What it does** [C]: rounds the double to IEEE binary32 (24-bit significand) under the current rounding mode (never changed → round-to-nearest-even [I]) and widens back exactly; the `volatile` forces a real 32-bit memory store so the x87 cannot keep the 53/64-bit register value. Despite the name it discards **29** low significand bits, not a nibble. Compiled for every non-Xbox target (`#ifndef _XBOX`).
- **Why** [I]: the file is credited to the Mac porting partner (header: Ambrosia Software, 2007-10-25) and the precision-control helper notes that PowerPC exponent size differs; the wrappers make results of CRT transcendental functions — which differ by an ulp or so between Microsoft's x87 CRT, Apple's libm and SSE implementations — collapse to the same binary32 value, enabling PC↔Mac lockstep. Results differ after rounding only if the two libraries' doubles straddle a binary32 rounding midpoint; for a 1-ulp disagreement that is ≈2^−29 per call [I arithmetic], i.e. rare but not impossible over 10^8–10^9 calls per match.
- **Side effect on the model** [C]: every `Mag()`, `Normalise()`, `SetLength()`, `RotateAround()`, `FastRotateAround()` and angle uses a single-precision-rounded sqrt/sin/cos, so distances and unit vectors carry ~7 significant digits even though positions are doubles. `sqrt` itself is correctly rounded on IEEE hardware, so the rounding of `iv_sqrt` is a precision loss, not a portability need [I].
- `iv_strtod` is declared `extern "C"` in `math_stubs.cpp` but defined nowhere in the tree [C grep] (presumably a portable `strtod` planned for the Mac).
- Parsing is mixed: 122 `iv_atof` vs 44 raw `atof` sites, e.g. `MultiwiniaZone::Read` (`m_size`), `AITarget::Read` (`m_priorityModifier`), `LevelFile` landscape `m_maxHeight` keep full double precision while most building fields are float-rounded [C grep]. Deterministic on one CRT; cross-CRT `strtod` rounding differences are a residual risk [U: whether MSVC 2005 `atof` is correctly rounded for all shipped literals].

### 3.3 Remaining raw CRT math

Raw `sinf` 53, `cosf` 24, `sqrtf` 10, `expf` 4, `atan2` 6, `acosf` 1, `fmodf` 31, `fmod` 13, `pow` 11, `sin` 5, `sqrt` 5 call lines in `code/` [C grep]. In simulation code the only uses are: `Darwinian::AdvanceCombat` — formation volley timing `int(fmodf(gameTime×10 + formationIndex, formationSize/4.0)) == 0` (and `/16.0` with Rage): converts the accumulated **double game time to float** and truncates; exact fmod but extremely sensitive to the accumulated value (a peer whose game time differed in the last bit could fire on a different tick); `formationSize < 4` → modulus < 1 → fires every frame [C] `code/worldobject/darwinian.cpp:Darwinian::AdvanceCombat`. `Darwinian::AdvanceBeingAbducted` `fmodf(uniqueId, 10)` (exact). `TurretEmptyShell::Advance` `fmod` (cosmetic object). `Location::GetEntityId` `pow` (mouse picking, local). `Matrix33/34::DecomposeToYDR` (`atan2`) has no caller outside `code/lib` [C grep]. All other raw calls are in render/UI/camera/water/explosion code.

### 3.4 FPU control word, compiler and instruction set

| Item | Baseline `Release|Win32` (shipped) | `ReleaseDirect3D|Win32` | Ref |
|---|---|---|---|
| FP model | no `FloatingPointModel` attribute → VS2005 default `/fp:precise` [I: default] | `FloatingPointModel="2"` = **`/fp:fast`** | [C] `targets/vs2005/Darwinia.vcproj` (only one `FloatingPointModel` attribute in the project; no per-file overrides) |
| Instruction set | no `EnableEnhancedInstructionSet` → x87 [I: VS2005 x86 default] | same | [C] vcproj |
| Optimisation | `/O2`, `/Ob2` | `/O2`, `/Ob2`, `/Oy`, `/GS-` | [C] vcproj |
| OpenMP | attribute absent → off [I default] | `OpenMP="false"` | [C] vcproj |
| Precision control | `StandardiseCPUPrecision()` = `_control87(_PC_53, _MCW_PC)` (53-bit significand; exponent range stays x87-extended — the source comment acknowledges the PowerPC difference) called first thing in `Initialise` (main thread) and at the start of every `WorkQueue` thread (`WorkQueue::RunThread`, comment: precision flags are per-thread) — covers the loading worker (where `Location::Init` runs), the sounds loader, the client listen thread, the sound preload queue and the D3D work queue | same, but `IDirect3D9::CreateDevice` is called **without `D3DCREATE_FPU_PRESERVE`** → D3D9 switches the calling (main/sim) thread to 24-bit precision [I: documented D3D9 behaviour] | [C] `code/main.cpp:StandardiseCPUPrecision`, `Initialise`; `code/lib/work_queue.cpp:WorkQueue::RunThread`; `code/lib/opengl_directx.cpp:Direct3DInit` |
| Threads not standardised | `Server` listen thread, MetaServer/matchmaker/authentication threads (`NetStartThread` directly) — they do no simulation math [I] | same | [C] `code/network/server.cpp:Server::Initialise`, `metaserver.cpp` |
| Rounding mode / exceptions | never changed (no `fesetround`, `_controlfp` rounding, `_RC_*`, `_clearfp`, `fldcw` outside the Mac `#elif`) | same | [C] grep |

Consequences [I]: the shipped GL build and the D3D build run different arithmetic (24-bit vs 53-bit x87 precision, `/fp:precise` vs `/fp:fast`) and must never be mixed in one match; only `multiwinia.exe` (GL) shipped (`build_matrix.md`). On x87 with `_PC_53`, `+ − × ÷` produce the same results as SSE2 doubles except where a value overflows/underflows the double exponent range while held in a register; `/fp:precise` makes assignments and casts round to their declared type, which is what makes the many `float` locals reproducible across optimisation levels.

### 3.5 Deliberate ordering/rounding guards in the code

| Guard | Where | Purpose [I unless C] |
|---|---|---|
| `volatile double lenSqrd` | `Vector3::Normalise` (comment: "volatile for sync") [C] | force a 64-bit store so register spills cannot change the comparison `lenSqrd > 1e-10` and the reciprocal |
| `volatile` temporaries for offspring height (`syncsfrand(0.2)`, `h×0.75`, `1 + r`, product) | `Tree::CreateAnotherTree` (`tree.cpp` ~L525) [C] | fix evaluation order and rounding of a synced value (P07) |
| `volatile double randomFactor = 1.0; += syncfrand(0.3)` (comment: ordering of operations important for error control) | `Spider::AdvancePouncing` squash [C] | same |
| `volatile double force` (comment: sync safety) | `TriffidEgg::Advance` tumble force [C] `triffid.cpp` ~L786 | same |
| `FRAND/SFRAND` marker classes | `Vector3` [C] | fixed draw order |
| Reciprocal multiply in `Vector3::operator/` and `/=` | [C] `vector3.h` | speed; part of the de-facto numeric spec (P13 R5) |
| `Vector3::operator==` = `NearlyEquals` per component (`|a−b| < 1e-6` absolute; the header comment says FLT_EPSILON) | [C] `vector3.h`, `vector3.cpp` | tolerance comparisons are deterministic but not transitive |
| `Round(a)` ≡ `(int)(a)` truncation (comment warns not to use an asm rounding version for repeatable results) | [C] `code/lib/math_utils.h` | MSVC `/fp:precise` `(int)` truncates independent of the rounding mode [I] |

### 3.6 Known NaN/∞ producers in simulation paths

| Site | Condition | Ref |
|---|---|---|
| `Team::Advance` slice 0: `m_selectedDarwinianCentre = m_newSelectedDarwinianCentre / (double)m_numDarwiniansSelected` | no selected Darwinians (the common case) → 0/0 = NaN stored every frame | [C] `code/team.cpp:Team::Advance` |
| `Darwinian::AdvanceOnFire`, `ThrowableWeapon::Advance`, `Darwinian::AdvanceCombat`: `x / g_gameTimer.GetGameSpeed()` | speed 0 → ∞ (not reached: sim is skipped while paused) | [C] |
| AI priority / weighted crate drops 0/0 | P08 §17 | [C] P08 |
| `iv_acos(dot)` with `|dot| > 1` from rounding (`GunTurret::Advance` aim angle) | NaN → all comparisons false | [C] `code/worldobject/gunturret.cpp` |
| Degenerate math (`HorizontalAndNormalise`, `Vector2::SetLength`, matrix inverse) | P13 R7 | [C] P13 |

NaN propagation is deterministic on one platform/compiler; payload and sign handling differ across instruction sets [I].

## [→ 22_determinism_and_networking] 4. Timestep, frame structure and update ordering

### 4.1 Time constants

| Quantity | Value | Kind | Ref |
|---|---|---|---|
| Server tick (one UPDATE letter) | **0.1 s real time**; schedule `m_nextServerAdvanceTime += 0.1`, at most one tick per call, no skipping (`SkipToTime` has no caller) | literal | [C] `code/network/server.cpp:Server::Initialise`, `AdvanceIfNecessary`; P15 §2.1 |
| Game time per frame `SERVER_ADVANCE_PERIOD` | `0.1 × m_gameSpeed` game seconds (macro → `GameTimer::GetServerAdvancePeriod()`) | derived | [C] `code/gametimer.h`, `gametimer.cpp` |
| Slices per frame | `NUM_SLICES_PER_FRAME = 10` | `#define` | [C] `code/globals.h` |
| Client input (ALIVE) period | `IAMALIVE_PERIOD = 0.1` s real time | `#define` | [C] `code/globals.h`, `code/main.cpp:LocationGameLoop` |
| Game speed values | 0 (MP match start; pause; victory, then locked), 1.0 (grace end, immediate), slow-down crate `0.2 + syncfrand(0.2)` for 15 s, speed-up crate `1.3 + syncfrand(0.7)` for 15 s, meteor slow-mo 0.4 for 4 s | data from sim | [C] `code/gametimer.cpp:GameTimer::Reset`; `code/worldobject/crate.cpp:Crate::RunSlowDownTime/RunSpeedUpTime`; `meteor.cpp:Meteor::Advance`; `code/multiwinia.cpp:Multiwinia::DeclareWinner/AdvanceGracePeriod` |
| Speed blending per frame | `speed += (target − speed) × rate` (rate 0.05 crates, 0.1 meteor, 0.2 pause, 1.0 grace), snap when within 0.01; a duration counts down **0.1 per frame regardless of speed** then the target becomes 1.0; `LockSpeedChange` blocks further requests | sim | [C] `GameTimer::Tick`, `RequestSpeedChange` |
| Physics step | variable: every object integrates once per frame with `dt = SERVER_ADVANCE_PERIOD` ∈ {0, 0.02 … 0.2} s — the network frame rate stays 10 Hz, game speed scales dt, not tick count | sim | [C] `Location::Advance` callers; e.g. `Spider::AdvancePouncing` `m_pos += m_vel * SERVER_ADVANCE_PERIOD` |
| Sim pacing on clients | `GetNumSlicesToAdvance`: 20 when catching up, else `int(0.3 × pendingSlices)` clamped to [0, 20] (the intended time-based branch for ≤ 10 pending slices is dead: it assigns a shadowing local) | local only | [C] `code/network/clienttoserver.cpp:GetNumSlicesToAdvance` |
| Catch-up | loop while > 20 frames behind and < 0.3 s spent this render frame | local only | [C] `ClientToServer::AdvanceAndCatchUp` |

### 4.2 Execution order of one frame (single entry point)

The only caller of `Location::Advance` and `Multiwinia::Advance` is `ClientToServer::ProcessAllServerLetters` on the main thread [C grep]. For frame N:

1. `GetNextLetter` returns letter N (strict sequence order; unsequenced letters run immediately at the inbox head) [C].
2. `ProcessServerLetters` (CLIENTHELLO/GOODBYE/CLIENTID/TEAMASSIGN/NETSYNCERROR/NETSYNCFIXED/SPECTATORASSIGN/FILESEND …) else `ProcessServerUpdates`: the letter's sub-directories are applied **in the order the server popped them from its FIFO inbox** (ALIVE → `Location::UpdateTeam`, SELECTUNIT, RUNPROGRAM, TARGETPROGRAM, TERMINATE, ORDERGOTO, PROMOTE, TOGGLEENTITY, SELECTDGS, ORDERDGS, PAUSE, game options …) [C] `ClientToServer::ProcessServerUpdates`; message catalogue in P15 §5.
3. If it was an UPDATE: `g_sliceNum = 0`, `GameTimer::Tick()`, `GenerateSyncValue()`, `SendSyncronisation()` [C].
4. Slices 0…9 (possibly spread over several render frames):

| Slice | `Location::Advance(slice)` work, in this order | Ref |
|---|---|---|
| every | **Teams** 0→5 (skip `TeamTypeUnused`): [slice 1 only: `TaskManager::Advance`, kill-milestone messages for the local team] → for each unit (index order) `Unit::AdvanceEntities(slice)` (unit's slice range) → [slice 0 only: `Unit::Advance` for every unit, remove dead] → [slice 0 only: selected-Darwinian count/centroid roll-over] → `m_others` slice range: `Entity::Advance`, then entity-grid `RemoveObject`/`UpdateObject` | [C] `code/team.cpp:Team::Advance`, `code/unit.cpp:Unit::AdvanceEntities` |
| every | **Weapons**: `m_lasers` slice range (`Laser::Advance`, remove), then `m_effects` slice range (`WorldObject::Advance`, delete) | [C] `Location::AdvanceWeapons` |
| every | **Buildings** slice range (`Building::Advance`; on removal: delete, `RecalculateAITargets` if AITarget/Wall, and after the loop `ObstructionGrid::CalculateAll`) | [C] `Location::AdvanceBuildings` |
| every | **Spirits** slice range | [C] `Location::AdvanceSpirits` |
| 3 | **Clouds** (`m_offset += (0.03, 0, −0.01) × period` — cosmetic but inside the step) | [C] `Location::AdvanceClouds`, `code/clouds.cpp:Clouds::Advance` |
| every | `MissionComplete()` evaluation (Darwinia objectives; MW maps use `NeverTrue`) | [C] `Location::Advance`; P11 |
| 3 | Weather spawners: snow (level effect or Christmas mode — both may run, sharing `m_christmasTimer`), soul rain, dust storm, lightning | [C] `Location::Advance`; P05 |
| 7 | `EntityGridCache::Advance` — evict cached enemy queries older than 1.0 game s | [C] `code/entity_grid_cache.cpp` |
| 8 (MP) | `AdvanceCrates`; `m_spawnManiaTimer`, `m_bitzkreigTimer` −= period | [C] `Location::Advance` |
| after each slice | `ParticleSystem::Advance(slice)` — advances **all** particles at slice 0 only (cosmetic) | [C] `code/particle_system.cpp:ParticleSystem::Advance` |
| 8 (MP) | `Multiwinia::Advance()` — mode rules; runs **even while `GamePaused()`** (grace countdown, un-pause) | [C] `ProcessAllServerLetters` |

Steps 1–5 of `Location::Advance` are written as `#pragma omp parallel for schedule(dynamic)` over `step = 0..4` (with `#undef for` to defeat the `for`-scoping macro). OpenMP is not enabled in any configuration, so they execute serially in the order above [C config, I default]. They share the synced RNG, the entity grid and building lists — enabling OpenMP would make the simulation nondeterministic.

### 4.3 Container iteration and slot allocation (what makes the order reproducible)

| Mechanism | Rule | Ref |
|---|---|---|
| `FastDArray<T>` (base of `SliceDArray`) | slots iterated 0…`Size()−1` skipping empty slots; `PutData` takes the head of a **LIFO free list** (most recently freed slot first); when full it grows by the step size (`m_buildings` 10, `m_spirits`/`m_lasers`/`m_effects` 100; `m_spirits` presized 100) or doubles when step = −1; `RemoveData` pushes the slot on the free list | [C] `contrib/systemiv/lib/tosser/fast_darray.cpp:PutData/RemoveData/Grow`; `code/location.cpp:Location::Location` |
| `SliceDArray::GetNextSliceBounds(slice)` | slice 0 starts at 0; slice k < 9 covers `[lastUpdated+1, lastUpdated+1+int(Size()/10.0f)]`; slice 9 ends at `Size()−1`; `Size()` is the **capacity**, read at slice time, so growth during earlier slices shifts later bounds. Every slot is advanced exactly once per frame, except: an object created into a slot above the current `lastUpdated` is advanced in the same frame; one created into a reused lower slot waits for the next frame | [C] `code/lib/slice_darray.cpp` |
| Render-side use | renderers call `GetLastUpdated()`: slots ≤ lastUpdated are extrapolated with `predictionTime`, later slots with `predictionTime + SERVER_ADVANCE_PERIOD` | [C] `code/team.cpp:Team::RenderDarwinians` |
| Entity-grid queries | results ordered by cell x ascending, then z ascending, then team 0…5, then cell slot order; duplicates (multi-cell objects) removed | [C] `code/entity_grid.cpp:EntityGrid::GetNeighbours` |
| Entity-grid cache | `GetEnemies` returns a cached result list when a same-team query centre lies within half the cached range (float-stored keys); entries die after 1.0 game s (slice 7). Cached results are therefore stale by up to 1 s — the cache is simulation state and its contents depend on query order | [C] `code/entity_grid_cache.cpp:EntityGridCache::GetEnemies/FindCacheHit/Advance` |
| Hash maps / sorting / pointer order | none in simulation code: `qsort` only for the alpha-pass building depth sort (render), `std::map<Entity*,…>` only in the `TRACK_SYNC_RAND` watch list | [C grep over `code/worldobject`, `location`, `team`, `unit`, `taskmanager`, `multiwinia`, `entity_grid`, `routing_system`, `obstruction_grid`, `level_file`, `global_world`] |
| Function-static scratch vectors | `s_neighbours` in several files, `Location::s_neighbours` with an in-use flag (`Location::Bang` falls back to a local vector when re-entered) | [C] |

### 4.4 Object identity and id allocation

| Id | Allocation | Determinism notes | Ref |
|---|---|---|---|
| `WorldObjectId` = {`unsigned char teamId`, `int unitId`, `int index`, `int uniqueId`}; special unit ids `UNIT_BUILDINGS −100`, `UNIT_SPIRITS −101`, `UNIT_LASERS −102`, `UNIT_EFFECTS −103` | — | `index` is the slot in `Team::m_others` / `Unit::m_entities`; network commands address entities **by slot index** (TERMINATE, PROMOTE, TOGGLEENTITY, SELECTUNIT entityId), so slot allocation history must be identical on all peers | [C] `code/worldobject/worldobject.h`; `ClientToServer::ProcessServerUpdates` |
| `uniqueId` for entities and effects | global `WorldObjectId::s_nextUniqueId`, pre-incremented (first id 1) by `GenerateUniqueId` in `Entity::CreateEntity`, `Location::ThrowWeapon/FireRocket/FireTurretShell/LaunchFireball/FireLaser/CreateShockwave`, weather spawners, zombies, spam infections, turret shells/targets, tank missiles/mines | reset only by `ResetUniqueIdSeed` in `DoEnterLocation` (**not** in Assault `Multiwinia::ResetLocation`) — deterministic either way because every peer executes the same allocations | [C] `code/worldobject/worldobject.cpp`, `code/main.cpp:DoEnterLocation` |
| Building `uniqueId` | level-file ids; runtime buildings (crates, trees, AI targets of turrets/statues/radar dishes) from `GlobalWorld::GenerateBuildingId()` = `s_nextUniqueBuildingId++`, reset to 0 in `Location::Init` and set to (highest level id + 1) after `InitBuildings` | building ids travel in AIMBUILDING, SELECTUNIT, TOGGLELASERFENCE; `Location::GetBuilding(id)` is a linear scan returning the first match. **Local callers exist**: `MultiwiniaHelp::AttachTutorialTo3d/AttachTutorial2To3d` create crates with `GenerateBuildingId` from the render-frame loop (single-player tutorial only, P19) | [C] `code/global_world.cpp:GenerateBuildingId`; `code/location.cpp:InitBuildings`; `code/multiwiniahelp.cpp` |
| Route ids | slots of `RoutingSystem::m_routes`, shared by simulated routes and **local** mouse-preview routes from `LocationInput` | handles only, never transmitted (P03) — but they perturb the slot order on each peer differently | [C] P03 |
| Spirit index | slot in `Location::m_spirits` (`FastDArray<Spirit>` by value) | as FastDArray | [C] `Location::SpawnSpirit` |
| Task ids | per-team counter, 1 byte on the wire (P09) | — | [C] P09 |

### 4.5 Threads and the simulation

The simulation runs on exactly one thread at a time: the main thread for frames, the loading-screen worker thread for `Location::Init` (initial load; the Assault `ResetLocation` re-init runs on the main thread). While the worker loads, the main thread renders the loading screen, advances sound, sends KEEPALIVE syncs and ticks the server, and executes display-list jobs queued by the worker — none of which touch the synced RNG [C] `code/loading_screen.cpp:LoadingScreen::Render`, `code/main.cpp:EnterLocation`. Hand-over between the threads is via the `WorkQueue` mutex/events [I: memory visibility guaranteed by the Win32 synchronisation calls]. The only cross-thread hazard to sim state is the shared non-synced LCG (§2.6).

## [→ 21_simulation_and_entity_model] 5. Simulated vs cosmetic state

### 5.1 Classification of every major subsystem

"Simulated" = must be bit-identical on all peers; advanced only inside `ProcessAllServerLetters`. "Cosmetic" = local, may differ freely, must never be read by simulated code. "Sim-resident cosmetic" = stored in a simulated object but only written/read by presentation (a re-implementation should move it to a view model).

| Subsystem / state | Class | Evidence |
|---|---|---|
| Entities (`Team::m_units[].m_entities`, `Team::m_others`) — positions, velocities, fronts, health, states, timers, orders | **simulated** | [C] `Team::Advance`, `Unit::AdvanceEntities`; hashed by the debug IFrame |
| Buildings (`Location::m_buildings`) incl. ownership, capture, ports, spawn timers | **simulated** | [C] `Location::AdvanceBuildings` |
| Lasers (`m_lasers`), effects (`m_effects`: grenades, rockets, shells, airstrike markers/bombs, shockwaves, fireballs, pulse wave, spam infections, zombies, snow, dust balls, lightning, muzzle flashes…) | **simulated** (some — snow, zombies, muzzle flashes — are purely decorative but live in the simulated list and draw synced RNG) | [C] `Location::AdvanceWeapons`; `code/worldobject/worldobject.h` effect enum |
| Spirits (`m_spirits`) | **simulated** (not hashed even by the debug IFrame) | [C] `Location::AdvanceSpirits`, `IFrame::Generate` |
| Crates (spawn, fall, capture, reward) | **simulated** | P08; [C] `Location::AdvanceCrates` |
| Crate orientation `m_front/m_up`, `m_centrePos`, bubble timer | **sim-resident cosmetic**: `Crate::Render` rotates `m_front/m_up` by wall-clock `g_advanceTime`; the crate's `m_centrePos` is recomputed in sim from that orientation but only read by its renderer; crates' hit tests are disabled (`DoesSphereHit/ShapeHit/RayHit` return false) and the debug IFrame writes zero vectors for them | [C] `code/worldobject/crate.cpp:Crate::Render`, `Crate::DoesSphereHit`; `iframe.cpp` |
| AI (one `Entity::TypeAI` per CPU team) | **simulated** on every peer | [C] `Location::InitialiseTeam`; P08, P15 §11 |
| Task manager tasks | **simulated**; `m_recentTasks`, `m_mostRecentTaskIdTimer`, `TaskSource::m_timer` use wall clock — sim-resident cosmetic | P09 D1 |
| Game timer `m_gameTime`, `m_gameSpeed`, speed requests | **simulated**; `m_lastClientAdvance`, `m_accurateTime`, `m_estimatedServerStartTime`, prediction | local | [C] `code/gametimer.cpp` |
| Multiwinia rules state (scores, timers, eliminations, statue/rocket/zone state) | **simulated**; messages, achievements, help notifications keyed on `m_myTeamId` | local side effects (P02 D5) | [C] `code/multiwinia.cpp` |
| Lobby/game options, team names/colours, ready flags | **synchronised** through the sequenced stream (P15 §5.3) | [C] `ProcessServerUpdates` |
| Terrain heightmap, normal map, obstruction grid, routing node tables | **simulated static data** (deterministic generation from the level file; private LCG) | [C] `code/landscape.cpp`; P11 |
| Entity grid, entity-grid cache | **simulated derived structures** (query results and staleness matter) | [C] §4.3 |
| Trees: fire, spread, spirit drops, corruption, offspring | **simulated**; crown hit sphere is simulated but computed by the display-list generator (§2.6); colours/sway | cosmetic | [C] `tree.cpp`; P07 |
| Clouds offset | cosmetic, advanced inside the step (slice 3) without RNG | [C] `Clouds::Advance` |
| Weather spawn (snow, soul rain, dust storm, lightning) | **simulated** (dust-ball vortex and lightning strikes affect units; snow is decoration on the synced stream) | [C] `Location::AdvanceChristmas/AdvanceDustWind/AdvanceLightning`; P05 |
| Particles (`ParticleSystem`) | **cosmetic**; advanced inside the slice loop but only from unsynced RNG; `CreateParticle` returns `void`, nothing reads particles | [C] `code/particle_system.cpp` |
| Explosions (shape shatter, `g_explosionManager`) | cosmetic; wall-clock lifetimes; advanced per render frame | [C] `code/explosion.cpp`, `code/main.cpp:LocationGameLoop` |
| Sound | cosmetic (never writes sim state; P16) | P16 determinism notes |
| Camera, `LocationInput`, `GameCursor`, `TaskManagerInterface`, markers, `MultiwiniaHelp`, chat, HUD | local; see leaks §5.4 | [C] `code/main.cpp:LocationGameLoop` render-frame calls |
| Water (`Water::Advance` per render frame), landscape colours, lights, fog, deform/pixel effects | cosmetic | [C] `LocationGameLoop`; P11, P12 |
| Entity legs (`EntityLeg`: Spider, Tripod) | sim-resident cosmetic for Spider (foot swing timed with wall-clock `g_gameTime`; only leg rendering, foot-fall sounds and `m_nextLegMoveTime` depend on it; body height is computed from terrain, not feet); Tripod uses wall clock for gameplay [B:LEGACY] | [C] `code/worldobject/entity_leg.cpp:EntityLeg::Advance`, `spider.cpp:Spider::Advance/UpdateLegs`; P03 |
| `m_renderDamaged` flags (Entity, AntHill, Centipede, Triffid) | sim-resident cosmetic (unsynced `frand` + wall clock inside `Advance`) | [C] |
| Soul Destroyer captured-spirit trail (`m_spirits` of the creature) | sim-resident cosmetic (wall-clock birth times, `AppRandom` slot) | [C] `SoulDestroyer::Attack` |
| Achievements, statistics, profile | local | P02, P18 |

### 5.2 Render-time interpolation

`GameTimer::GetPredictionTime() = min(now − m_lastClientAdvance, 1.0) × m_gameSpeed`, 0 while paused; `m_lastClientAdvance` is stamped (wall clock) when a frame's commands are applied [C] `code/gametimer.cpp`. Renderers receive `predictionTime` and extrapolate locally (`predictedPos = m_pos + m_vel × t`), adding one `SERVER_ADVANCE_PERIOD` for slots the current frame has not yet advanced (§4.3). Prediction is a pure function argument — it is never stored into simulated state [C: scripted scan of `*Render*` functions in `code/worldobject` and core sim files for member writes, results in §5.3].

### 5.3 Render functions that write fields of simulated objects (complete list from the scan)

Scan: every assignment to an `m_…` member inside a function whose name contains `Render`, in `code/worldobject/*.cpp`, `location.cpp`, `team.cpp`, `unit.cpp`, `taskmanager.cpp`, `multiwinia.cpp`, `routing_system.cpp` (94 lines) [C].

| Writer | Field(s) | Read by simulation? | Verdict |
|---|---|---|---|
| `Tree::RenderAlphas` → `Generate()` when display lists are missing | `m_hitcheckCentre`, `m_hitcheckRadius` (via `RenderBranch`) | **yes** (fire spread) | recomputes the same values from `m_seed` (idempotent) unless the G6 race of §2.6 hits; colours `m_*ColourArray`, `m_renderCorruptShadow` cosmetic [C] |
| `LaserFence::RenderAlphas` | `m_nextLaserFenceId = −1` if the linked id is missing or not a LaserFence | **yes** (`LaserFence::Advance` branches on `== −1`) and the call is camera-culled (`IsInView`) | latent desync: shipped data has **0 dangling links** among 703 fences in 57 `mp_*` maps [C] `scripts/X3_fence_links.py`; fences are never destroyed (no `Damage`/destroy path) [C] |
| `RadarDish::RenderAlphas` | `m_range`, `m_receiverId` | yes | saved and restored around the link drawing (single-threaded) → safe [C] |
| `GunTurret::Render` | `m_turretFront = m_front` | yes (aim) | only when `g_app->m_editing` [B:EDITOR] [C]; `m_actualTargetPos` render-only |
| `Crate::Render`, `Crate::RenderBubble` | `m_front`, `m_up`, `m_bubbleTimer` | no (see 5.1) | sim-resident cosmetic |
| `ThrowableWeapon::Render` | `m_up`, `m_front` (tumble per render frame), `m_numFlashes` (flash sound counter) | no (only the constructor and renderer use them) [C grep] | sim-resident cosmetic |
| `TurretEmptyShell::Render`, `Missile::Render` (`m_fire`), `Armour::Render` (`m_smooth*`), `Darwinian::Render` (`m_colourTimer`, `m_shadowBuildingId`), `FuelGenerator::Render` (`m_pumpMovement`), `Flag::Render`, `MultiwiniaZone::RenderBlitzkrieg` (creates `m_blitzkriegFlag`), `ConstructionYard::RenderAlphas` [B:LEGACY], `LaserTrooper::Render` (`m_justFired`) | presentation fields | no [I: names and grep of readers] | cosmetic |
| `SolarPanel::Render`, `DynamicNode::Render` [B:LEGACY], `SpiritReceiver::Render` [B:LEGACY] | `m_up`, `m_front` from the terrain normal | yes (shape hit transform) | idempotent: `SolarPanel::Initialise` sets the same values [C] |
| `BlueprintRelay::Render` | `m_pos.y` | — | [B:LEGACY] |
| `Location::RenderBuildingAlphas` (sort buffer), `RenderCurrentMessage`, `RoutingSystem::Render` (`m_walkableRoute`), `Multiwinia::RenderRocketStatusPanel` | UI state | no | cosmetic |
| `GameCursor::RenderCrosshair` (`code/gamecursor.cpp`) | **`building->m_type = Building::TypeRadarDish`** (`=` typo for `==`) for the local team's selected non-turret building | yes (type!) | P09 D4 confirmed [C]; dormant because only RadarDish/GunTurret can be selected in Multiwinia (P09) |

### 5.4 Local-only state leaking into simulation (verified list)

| # | Leak | Reachable in MW? | Severity | Ref |
|---|---|---|---|---|
| L1 | `InsertionSquad::DirectControl`, executed on every peer from `Location::UpdateTeam` for the team's selected squad, returns early only if **neither** the sender's `m_cameraEntityTracking` **nor the local camera's `m_camLookAround`** is set. `m_camLookAround` = local control `CameraFreeMovement` held (default F2, gamepad right-stick click) and refreshed every frame in free-movement mode. On a peer whose player holds it, the squad receives `SetWayPoint(pointMan->m_pos)` whenever it is moving (or `SetWayPoint(point + Dx/Dy)` when the sender holds the left button) and an extra `Attack(...)` when the sender fires → that squad's state diverges on that peer only | **yes** (squad crate reward; any player holding the free-look control while an opponent's selected squad moves) [C code; I frequency] | **High** | [C] `code/worldobject/insertion_squad.cpp:InsertionSquad::DirectControl`, `code/location.cpp:Location::UpdateTeam`, `code/camera.cpp:Camera::SwitchEntityTracking`, `data/input/input_preferences.txt` (`CameraFreeMovement`) |
| L2 | `GameCursor::RenderCrosshair` type-assignment typo | dormant | medium (latent) | §5.3; P09 D4 |
| L3 | `TaskManagerInterfaceMultiwinia::Advance` sets `tm->m_mostRecentTaskId = −1` on the owning client only, after a wall-clock delay; sim reads it right after successful `RunTask` calls | safe today (reads follow a successful `RunTask` in the same tick) | medium (latent) | [C] `code/taskmanager_interface_multiwinia.cpp` (L~287); P09 D2 |
| L4 | `TaskManagerInterfaceMultiwinia::RunScreenZone("DeleteTask")` calls `TaskManager::TerminateTask` directly (no network); same in the Icons/Gestures interfaces | dormant in MP (P09: zone not drawn / control unbound) | medium (latent) | [C] `taskmanager_interface_multiwinia.cpp` (L~865); P09 D3 |
| L5 | `MultiwiniaHelp::AttachTutorialTo3d/AttachTutorial2To3d` (render-frame loop, wall-clock timers) insert crates into `m_buildings` with `GenerateBuildingId`, change game options | single-player tutorial only | low (but breaks replay determinism) | [C] `code/multiwiniahelp.cpp`; P19 §2.6 |
| L6 | `LocationInput` allocates/frees preview routes in the simulated `RoutingSystem::m_routes` | yes; ids never transmitted | low | P03 |
| L7 | `Location::UpdateTeam` / `Team::SelectUnit` / `Location::IssueDarwinianOrders` etc. call markers, cursor, sounds, camera requests guarded by `m_myTeamId` | yes; presentation only | low (but an event queue is the right design) | [C]; P09 D5 |
| L8 | `Officer::CalculateBoundingSphere` scales by local camera distance and pref `DarwinianScaleFactor` | used only by `Location::GetEntityId` mouse picking (local) | none | [C] `code/worldobject/officer.cpp`, `code/location.cpp:Location::GetEntityId` |
| L9 | `Darwinian::AdvanceWatchingSpectacle` faces a wall-clock-wobbled target | `EscapeRocket::IsSpectacle` returns false in Multiplayer; GodDish blocked | none in MW [B:LEGACY] | [C] `darwinian.cpp`, `rocket.cpp:EscapeRocket::IsSpectacle` |
| L10 | `Tripod::*` mode timers and turn rate use wall clock (`g_gameTime`, `g_advanceTime`) | Tripod blocked | none in MW [B:LEGACY] | [C] `code/worldobject/tripod.cpp` |
| L11 | Armour direct `SetOrders` from local input | only in the `!Multiplayer()` branch | none in MW [B:LEGACY] | [C] `code/location_input.cpp` (L~1054) |
| L12 | `Multiwinia::AdvanceAssault` compares wall clock `GetHighResTime()` with game-time `m_startTime` | only feeds the local "defended all" achievement flag | none | [C] `code/multiwinia.cpp` (L~1752) |

## [→ 22_determinism_and_networking] 6. Sync checking, desync reaction and forensics

P15 §8 documents the wire messages and server algorithm; this section states exactly what is protected and what is not.

### 6.1 What the per-frame sync byte covers

| Build | Sync byte | Covers | Does **not** cover | Ref |
|---|---|---|---|---|
| Baseline FULL (`!TARGET_DEBUG && !TARGET_BETATEST_GROUP`) | `(unsigned char)(255 × syncfrand(1))` → 0…254 | only the **number of synced draws consumed so far** (and the seed): MT19937 outputs are a fixed sequence, so peers that made the same number of draws report the same byte whatever they used the values for | every world field; a divergence is noticed only once it changes the count of synced draws (e.g. a different number of units in range, a branch that draws) | [C] `code/network/iframe.cpp:IFrame::Generate` |
| `TARGET_DEBUG`, `TARGET_BETATEST_GROUP*` | low byte of SHA-1 word 0 over the snapshot | seq id, `GetNetworkTime()`, per used team: every unit's `m_centrePos` and every unit entity's `m_pos/m_vel/m_front/uniqueId`, every `m_others` entity's `m_pos/m_vel/m_front/uniqueId`; every laser `m_pos/m_vel`; every effect `m_pos/m_vel`; every building `m_pos/m_vel/m_radius/uniqueId` and `m_front/m_up/m_centrePos` (zero vectors for crates); slot validity flags and array sizes; a `syncfrand(255)` sample | health and stats, entity/building **states and timers**, building **team ownership**, spirits, task managers, AI state, scores and mode state, game speed, entity-grid cache, routes | [C] `IFrame::Generate`, `ReadIFrameData` |

Cadence: one byte per executed UPDATE frame (10 Hz), plus 1 Hz KEEPALIVE repeats of the last byte while a level loads; lobby frames send 0 [C] `ClientToServer::GenerateSyncValue`, `KeepAlive`, `SendSyncronisation`. Each value consumes one synced draw in both build kinds, keeping the streams of same-kind builds aligned [C].

Detection statistics [I arithmetic]: if two peers' draw counts differ, their next outputs are effectively independent, so the bytes collide with probability 1/255 per frame; the chance of escaping detection for k consecutive frames is 255^−k (≈ 0.4 % for one frame, 1.5·10^−5 for two). The real latency is dominated by how long a state divergence takes to alter the draw count — unbounded in principle.

### 6.2 Decision and reaction (summary of P15 §8.3–8.4, verified)

- Server: stores `ServerToClient::m_sync[seq]`; when all connected clients have reported `seq` and values disagree, flags each unflagged client for which no other unflagged (or later-flagged) client has the same byte, and broadcasts sequenced NETSYNCERROR(client index, seq). Two-player games flag both players. Flags are never cleared (the "sync fixed" path is commented out) [C] `code/network/server.cpp:Server::HandleSyncMessage`, `NotifyNetSyncError`.
- Clients: `SetSyncState(id, false)` → `m_outOfSyncClients` non-empty → `App::GamePaused()` true → `Location::Advance`, particles and `GameTimer` accumulation stop at the **same sequence id on every peer** (NETSYNCERROR is sequenced) while `Multiwinia::Advance` keeps running; red "multiwinia_error_syncerror" banner [C] `ClientToServer::ProcessServerLetters`, `code/app.cpp:App::GamePaused`, `code/renderer.cpp`. CLIENTGOODBYE of a flagged client clears its flag; if none remain the match resumes [C].
- On the first notification: `SendIFrame(seq)` (inert in the baseline: frames carry no data and `SequenceId()` stays 0), `[B:DEBUG]` `DumpSyncRandLog`, and `Location::DumpWorldToSyncLog` appends `Begin Final Object Positions` plus one `LogState()` line per unit entity, other entity, effect (except snow) and building to `<profile>syncerror-<server-start YYYY-MM-DD--HH-MM-SS>-client<id>-team<team>.txt` [C] `ClientToServer::GetSyncFileName`, `code/location.cpp:Location::DumpWorldToSyncLog`.

### 6.3 Forensic tooling

| Tool | Build | Behaviour | Ref |
|---|---|---|---|
| Sync-rand call log | [B:DEBUG] (`TRACK_SYNC_RAND`) | every synced draw appends `"%8d | <value> | <file> line <n>"` (call counter since the last seed; value printed by `HashDouble`); the seed call is logged; frame markers (`Seq ID`, letter summaries via `WriteSummary`/`DecodeCmd` for the first 1000 sequence ids, slice numbers, "Advance Location", "Advance Multiwinia"); `ENTITY/UNIT ENTITY/EFFECT/Building BEFORE/AFTER` state lines around every advance; ring buffer of **100 000** lines; `DumpSyncRandLog(file)` writes `SyncRandSeed <n> (as signed int: <n>)` then the lines | [C] `contrib/systemiv/lib/math/random_number.cpp`; `code/network/clienttoserver.cpp:ProcessAllServerLetters`; `code/team.cpp`, `code/unit.cpp`, `code/location.cpp` |
| Entity watch list | [B:DEBUG] | `TrackEntity` (body commented out) / `WatchEntities` log `Watch Changed From/To` when a tracked entity's `LogState` changes | [C] `code/location.cpp` (end of file), `entity.cpp:Entity::CreateEntity` |
| `HashDouble` | all | prints the IEEE-754 bit pattern of a double as 16 hex digits, **least-significant nibble first** (exact comparison of logged values) | [C] `contrib/systemiv/lib/debug_utils.cpp:HashDouble` |
| `LogState()` formats | all | `WOBJ Type[t] Id[u] pos[x,y,z], vel[Σ]`; `ENT Type[name] Id pos vel[Σ] front[Σ]`; Darwinian adds `state`, `waypoint[x,y,z]`, `boxId`; Officer adds `state`, `waypoint[Σ]`; Building adds `front[Σ] centre[Σ] radius`; Crate `pos vel radius`; Tree adds `height`, `hitcheckCentre[Σ]`, `hitcheckRadius` (Σ = sum of components, hashed) | [C] `worldobject.cpp`, `entity.cpp`, `darwinian.cpp`, `officer.cpp`, `building.cpp`, `crate.cpp`, `tree.cpp` `::LogState` |
| Cost note | all | `Location::AdvanceBuildings` and `Unit::AdvanceEntities` call `LogState()` (two `sprintf`s with `HashDouble`) around **every** advance even in the baseline, where `SyncRandLogText` is an empty function | [C] |
| IFrame exchange and diff | [B:DEBUG]/beta | snapshots of the last **50** frames; on desync uploaded via the FTP manager, relayed by the server, diffed index-by-index (`Local only`, `Remote only`, `[POS][VEL][FRONT][UP][CENTRE][RADIUS]`, `[CENTREPOS]`, `[n ENTITY DIFFS]`, `Network Time Different`, `Sync Rand Different`), written to `syncdiff-*.txt` and drawn as in-world arrows (`Team::RenderSyncErrors`) | [C] `iframe.cpp:CalculateDiff`, `code/multiwinia.cpp:Multiwinia::SaveIFrame`; P15 §8.4 |
| Network stats window | [B:FULL] (`NETWORK_STATS_ENABLED`) | per client "out of sync" when the server has flagged it | [C] `code/interface/network_window.cpp` |
| Command-history record/replay | all (pref `RecordDemo`) | `1`: host writes the server history to `serverHistory.dat` at `App::ShutdownCurrentGame`; `2`: `Server::LoadHistory("ServerHistory.dat")` at `DoEnterLocation`; both values also switch `UpdateAdvanceTime` to a fixed fake clock (`DemoFrameRate`, default 25) for AVI capture. The sync seed is **not** in the history (it travels in the unsequenced CLIENTID) and the count is written as a raw host-endian `int` → [I] replays only reproduce a match by accident; developer tool | [C] `code/app.cpp:App::ShutdownCurrentGame`, `code/main.cpp:DoEnterLocation`, `UpdateAdvanceTime`, `code/network/server.cpp:SaveHistory/LoadHistory` |

Historical evidence of desync classes (release notes, supporting only): trees and height maps, laser fences, statues (Capture the Statue), the Tornado/dust-storm effect, Officers sending Darwinians into radar dishes [C] `docs/multiwiniareleasenotes-beta.txt`, `docs/multiwiniachanges-beta.txt`, `docs/multiwinia-changes.txt`.

## [→ 22_determinism_and_networking] 7. Session-level preconditions and cross-match state

### 7.1 Inputs that must match but are never verified

| Input | Verification in code | Consequence | Ref |
|---|---|---|---|
| Data files (`data/stats.txt`, level files, shapes used for hit tests, `data/game.txt` options) | none: no CRC/hash exchange; the map is chosen by **filename string** (`GAMEOPTION_MAPNAME`); `mapcrc.h` ids serve only map lists/permissions | modified or mismatched data desyncs silently | [C] `ClientToServer::ProcessServerUpdates` (SETGAMEOPTIONSTRING), `code/level_file.cpp:LevelFile::CalculeCRC` callers (`code/UI/MapData.cpp`, `code/main.cpp:DoEnterLocation`) |
| Executable version | only the **client-side** server browser filters on `NET_METASERVER_GAMEVERSION == APP_VERSION`; the server accepts any CLIENT_JOIN (P15 §5) | mixed builds (e.g. GL vs D3D, debug vs release, PC vs Mac with the same version string) can join each other | [C] `code/UI/GameMenuWindow.cpp` (`protocolMatch`), P15 |
| `g_app->m_difficultyLevel` | none | read by simulation in MP: `EntityBlueprint::GetStat` multiplies **every entity's speed** (except SpaceInvader) by `1 + d/10`; `SporeGenerator` egg chance and timer; level parsing factors. It is fixed at 0 by `App::UpdateDifficultyFromPreferences`, **but** `PrefsOtherWindow`'s Apply button (Options → Other, live in MW) sets it to `pref Difficulty − 1` when applied at the main menu, and no shipped `mp_*` map pins `CreatedAsDifficulty` (0 of 57) → a player with a hand-edited `Difficulty` pref who applies Other Options desyncs every later match | [C] `code/worldobject/entity.cpp:EntityBlueprint::GetStat`, `sporegenerator.cpp`, `code/interface/prefs_other_window.cpp:ApplyOtherButton::MouseUp`, `PrefsOtherWindow::PrefsOtherWindow`, `code/location.cpp:Location::Init`, `data/levels/mp_*.txt` |
| Preferences read inside sim code | `RenderBuildingDetail` (tree fire particle count only), `RenderTreeDetail` (leaf alpha only; `Tree::SetDetail` asserts detail 1 because other values change the hit sphere — source comment), `RenderEntityDetail` (read by `Virii::RecordHistoryPosition` but unused — the tail length is the constant 175, the detail term is commented out), `DarwinianScaleFactor` (render/picking), `AutomaticCamera` (`Task::TargetSquad` camera only), `SoundMemoryUsage` (sound) | cosmetic as implemented [C] | [C] `tree.cpp:Tree::Advance/Generate/SetDetail`, `virii.cpp:Virii::RecordHistoryPosition`, `taskmanager.cpp:Task::TargetSquad` |
| Local state gating sequenced commands | CHRISTMASMODE and TOGGLE_READY are interpreted differently depending on the local `g_app->m_atMainMenu` | consistent as long as every peer is in the same phase when the command executes (true for lobby and in-game replays) [I] | [C] `ProcessServerUpdates` |
| FPU state | per-thread `_PC_53` (§3.4) | D3D9 device creation silently changes it on the D3D build | §3.4 |

### 7.2 Static/global simulation state and its reset points

Reset at every level load (`Location::Init`): `GlobalWorld::s_nextUniqueBuildingId`, `Crate::s_crates`, `Crate::s_spaceShipUsed`, `AIObjective::s_objectivesInitialised`, `Armour::s_armourObjectives`, `Engineer::s_buildingIds`, `Engineer::s_researchIds`, `SpawnPopulationLock::Reset()`, `SoulDestroyer::s_numSoulDestroyers`, `DustBall::s_vortexTimer`, `DustBall::s_vortexPos`, `Unit::s_offsets` (freed; lazily refilled with 100 `syncfrand(1.0)` on the first `Unit::GetFormationOffset` call — so P04's "never reset between games" is superseded), per team `AI::s_ai[i]`, `SpawnPoint::s_numSpawnPoints[i]`, `PulseBomb::s_pulseBombs[i]`, `MultiwiniaZone::s_blitzkriegBaseZone[i]`, `AppSeedRandom(1)`; `DoEnterLocation`: `WorldObjectId::ResetUniqueIdSeed()`; `~Location`: `Entity::s_entityPopulation = 0` [C] `code/location.cpp:Location::Init`, `Location::~Location`, `code/main.cpp:DoEnterLocation`, `code/unit.cpp:Unit::GetFormationOffset`.

**Not reset** (survive into the next match of the same process):

| Static | Read by sim? | Hazard | Ref |
|---|---|---|---|
| `Armour::SetDirectOrders` function-static `lastOrdersSet` (game time of the last toggle by **any** armour of **any** team) | yes: NEXTWEAPON/PREVWEAPON on a selected armour toggles loading only if `now > lastOrdersSet + 0.3` | game time restarts at 0 each match, so a peer that toggled late in a previous match ignores every toggle until the new match's clock passes that time, while a fresh peer applies them → **desync** for peers with different session histories; within a match one team's toggle blocks every other team's for 0.3 s | [C] `code/worldobject/armour.cpp:Armour::SetDirectOrders`; `ClientToServer::ProcessServerUpdates` (NEXTWEAPON/PREVWEAPON); `code/location_input.cpp` (sends NEXTWEAPON) |
| File-static `s_numMultiwiniaZones` | yes (`AdvanceTrunkPorts` reward choice) | P02 D4 desync hazard | [C] `code/multiwinia.cpp`; P02 |
| `Officer::SetNextMode/SetPreviousMode` statics | return early in Multiplayer | none in MW | [C] `officer.cpp` |
| `Location::UpdateTeam` static `unitMoved` (shared by all teams) | only gates the local cursor marker | none | [C] |
| `Darwinian::SearchForThreats` static `lastAttackHorn`, `Multiwinia::AdvanceGracePeriod` statics | sound only | none | [C] |
| `ReceiverBuilding::GetSpiritProcessor` static id cache | [B:LEGACY] | — | [C] `spiritreceiver.cpp` |
| Synced RNG state | re-seeded per session (§2.2) | none | [C] |

### 7.3 Latent: Assault multi-round level reset is applied at a peer-dependent point

`Multiwinia::AdvanceAssault` (slice 8) sets `m_resetLevel = true` at round end; the reload itself (`Multiwinia::ResetLocation`: delete + `new Location` + `Init`, which draws synced RNG, `g_gameTimer.Reset()`) runs from `LocationGameLoop` **after** `AdvanceAndCatchUp` returns. Between the flag and the reset each peer keeps advancing the old location for however many slices its pacing allows (`GetNumSlicesToAdvance`, catch-up loop), consuming synced draws, so the RNG position at the new `Location::Init` differs per peer [C code; I consequence]. Unreachable in the shipped game because `m_assaultSingleRound` is always true (P02 contradiction 3) [C via P02]. A re-implementation must apply such resets as a sequenced/simulated event at a fixed tick. [C] `code/multiwinia.cpp:Multiwinia::AdvanceAssault`, `ResetLocation`; `code/main.cpp:LocationGameLoop`.

## [→ 22_determinism_and_networking] 8. Determinism risk register for the new game (Windows, C++23, DirectX 12, multiplayer)

Each row: what the original does, why it is a hazard for a modern lockstep (or replay-capable) implementation, and the mitigation to design in. Severity is for the new game.

| ID | Hazard | Original behaviour (ref) | Severity | Mitigation |
|---|---|---|---|---|
| D1 | Floating-point results differ across compilers, optimisation flags, CPUs | x87 + `_PC_53` + `/fp:precise`, transcendental/sqrt rounded to binary32 (§3) — x64 has no precision control (`_MCW_PC` unsupported); MSVC x64 CRT `sin/cos/exp/log/pow` select SIMD/FMA code paths at run time by CPU [I: external CRT behaviour]; FMA contraction and `/fp:fast` reorder arithmetic | **High** | Pick one strategy up front: (a) **fixed-point / integer simulation** (Q16.16 or Q32.32 positions, integer ticks, table- or CORDIC-based trig, integer sqrt) — portable and replay-safe; or (b) strict floats: one compiler and one binary for all players, `/fp:strict` (or `/fp:precise` + `/fp:contract-`), no `/arch` auto-vectorisation of sim code, own deterministic `sin/cos/atan2/exp` implementations (polynomials), `sqrt` only (IEEE exact), and CI that replays recorded matches on Intel/AMD/ARM64 machines and compares per-tick hashes |
| D2 | Floating-point control state changed behind the sim's back | D3D9 without `D3DCREATE_FPU_PRESERVE` changes x87 precision; per-thread FPU state needed explicit setup (§3.4) | Medium | On x64, assert/restore MXCSR (rounding = nearest, FTZ/DAZ off or always on — but fixed) at the start of every sim tick and in every sim worker thread; never call third-party code from inside a sim tick |
| D3 | One global synced RNG shared by all systems; cosmetic draws on it; draw order hidden in expressions | MT19937 global (G1–G4), 655 call sites, cosmetic consumers (§2.3 R3), `FRAND/SFRAND` workaround, one unordered case | High | `SimRng` object inside the match state (e.g. PCG32/xoshiro128++), passed explicitly; separate client-local `FxRng`; derive sub-streams `hash(seed, systemId, entityId, tick)` so adding a cosmetic or new gameplay draw in one system does not shift all others; one draw per statement (lint rule); never draw inside function-call argument lists |
| D4 | Non-sim RNG feeding sim data; thread-unsafe global RNG | Tree crown sphere from the global LCG while sound/metaserver threads also use it (§2.6) | Medium | Gameplay-relevant procedural data generated by the sim with sim-owned generators or baked into level data; presentation RNGs are per-thread objects |
| D5 | Weak desync detection | Baseline byte = next RNG output → detects only draw-count divergence; debug hash omits health, states, ownership, spirits, timers (§6.1) | High | 64-bit hash of the canonical serialised sim state (or an incremental hash maintained on mutation) every tick or every N ticks, compared by the host; on mismatch dump the state of both peers at that tick (structured diff) |
| D6 | No recovery from desync; late join replays the full history | Permanent freeze (§6.2); joiners re-simulate from sequence 0 (§2.2) | Medium | Serializable sim state → host snapshot + tail of commands for late join, reconnection and desync repair; replay files = config + seed + command stream (+ periodic snapshots) |
| D7 | Local client state read by simulation code | L1 squad direct control reads the local camera (reachable desync), L2 crosshair typo writes a building type, L3/L4 UI writes task state, L5 tutorial inserts buildings (§5.4) | **High** | Hard module boundary: the sim library cannot include camera/input/UI/sound/renderer headers (C++20 modules or separate static library with an include allow-list); all player intent enters as commands; sim emits presentation events into a queue consumed by the client |
| D8 | Render code mutating sim objects | crate orientation, tree regeneration, laser-fence link reset, throwable tumble (§5.3) | Medium | Renderer receives `const` snapshots / read-only views; interpolation and per-object visual state live in client-side view components keyed by entity id |
| D9 | Statics surviving between matches | `Armour::SetDirectOrders` debounce, `s_numMultiwiniaZones` (§7.2) | Medium | All mutable sim state inside a `Match` object constructed per match; forbid `static` mutable variables in sim code (static analysis) |
| D10 | Unverified data/version/settings | no data hash, server accepts any version, local `Difficulty` pref reaches sim (§7.1) | High | Lobby exchanges build id + hash of every sim-relevant asset (maps, unit stats, shapes/collision, rules); gameplay settings exist only as lobby options inside the command stream; refuse mismatches |
| D11 | Variable timestep through game speed; double time accumulation | dt = 0.1 × speed; `m_gameTime += 0.1·speed`; "1-second" timers take 11 ticks; formation volleys from `fmodf(time×10)` (§3.1, §3.3) | Medium | Fixed simulation dt; game speed changes the number of ticks per real second (or is a cosmetic slow-mo only); timers as integer tick counts; any "per second" rule expressed in ticks |
| D12 | Iteration order coupled to slot reuse; commands addressing slots | LIFO free lists, slice bounds from capacity; TERMINATE/PROMOTE/TOGGLEENTITY by slot index (§4.3–4.4) | Medium | Stable monotonically increasing entity ids (never reused within a match); iterate in id order or in a documented stable order; commands carry ids; no `unordered_map` iteration and no pointer comparisons in sim |
| D13 | Parallelising the sim | Original is single-threaded; an OpenMP pragma over shared-state steps would break determinism (§4.2) | Medium | Deterministic job design: phases that read the previous state and write per-entity outputs, fixed-order reductions, per-entity RNG sub-streams; no shared mutable caches (or caches whose content is a pure function of the tick) |
| D14 | Derived caches change results | `EntityGridCache` returns up-to-1 s-stale query results depending on query order (§4.3) | Low | Either make caches transparent (results identical to uncached) or specify their semantics as part of the rules |
| D15 | Mode transitions outside the tick | Assault `ResetLocation` from the render loop (§7.3) | Medium | Every state transition (round reset, victory, eliminations) executes inside a sim tick at a defined slice |
| D16 | Sim generating network traffic | TOGGLEENTITY handler makes every client send SELECTUNIT (P15) | Low | Sim never sends; it may emit "suggested command" events handled only by the owning client |
| D17 | NaN/∞ inside sim state | 0/0 selection centroid every frame, AI 0/0, acos domain (§3.6) | Low | Debug assertion "no NaN/Inf in sim state" after each tick; guarded divisions |
| D18 | Debug instrumentation altering arithmetic | tracked `syncsfrand` formula differs from untracked; TheBeast changes the crate enum in debug (§2.1) | Low | Tracing must observe, not recompute; debug and release must be able to play together (or the lobby must refuse) |
| D19 | GPU or middleware in the sim | — (original is CPU-only; GPU used only for rendering) | Medium | Keep all gameplay on the CPU sim; no DX12 compute, no third-party physics in gameplay unless certified deterministic |
| D20 | Wall-clock time read by sim code | Spider legs, Tripod, `m_renderDamaged`, task-pop-up timers (§5.1, §5.4) | Low | Sim API exposes only the tick counter; wall-clock is a client-only service |

## [→ 21_simulation_and_entity_model] 9. Determinism rules a re-implementation must state in the entity model

1. Simulated objects live in match-owned containers with stable ids and a documented iteration order (§4.3, D12).
2. Every simulated object advances exactly once per tick; creation during a tick is visible to later systems in the same tick only by explicit rule (the original's slice/capacity behaviour is incidental, §4.3).
3. Simulated objects hold no presentation fields; presentation state is keyed by entity id in the client (§5.3).
4. Hit volumes used by gameplay are data or sim-computed, never by-products of rendering (§2.6, §5.3).
5. The only time source is the tick counter (D20); the only randomness is the sim RNG (D3).

## Cross-partition notes

- **P15 (network)**: this draft relies on P15 for message layouts, server sequencing, reliability and session lifecycle; it adds (a) baseline sync byte = draw-count detector (§6.1), (b) NETSYNCERROR being sequenced resolves P01 D4 — every peer pauses at the same sequence id [C] `Server::SendLetter`, (c) the seed is per server session = per match (§2.2), (d) `RecordDemo` replay lacks the seed (§6.3).
- **P01 (app core)**: frame/slice order and `StandardiseCPUPrecision` confirmed; `Release|Win32` has no `OpenMP` attribute at all (P01 quotes the D3D config's `OpenMP="false"`; both mean off).
- **P02 (modes)**: D1 (11-tick seconds) and D4 (`s_numMultiwiniaZones`) are integrated as D11/D9; the Assault reset-timing hazard (§7.3) belongs to the multi-round path P02 found unreachable.
- **P03 (entities/routing)**: route-id sharing with local previews (L6) confirmed; Spider legs use wall clock but do not feed Spider movement (§5.1) — Spider is live in MW (P05), so the "irrelevant, Tripod blocked" note should mention Spider is safe for a different reason.
- **P04 (military)**: L1 (`InsertionSquad::DirectControl` camera leak) — P04 listed it; this draft establishes the trigger (local `CameraFreeMovement` control held, default F2 / right-stick click) and the effect (waypoint reset and extra `Attack` on that peer only) → reachable desync. P04's claim that `Unit::s_offsets` is never reset is superseded (reset in `Location::Init`/`Empty`). P04's RNG table calls `_syncfrand` an "LCG" — it is MT19937.
- **P07 (misc buildings)**: Portal is not reachable through The Beast in the baseline (`INCLUDE_THEBEAST` is defined only under `TARGET_DEBUG`); laser-fence render reset verified harmless for shipped data (0 dangling links).
- **P08 (crates/AI)**: variable-length draw loops and short-circuit draw counts are carried into R4/R5 (§2.3).
- **P09 (task manager)**: D2/D3/D4 verified [C] (L3, L4, L2).
- **P11 (terrain)**: terrain uses a private per-tile LCG (G8); landscape colours reseed the global LCG (G6).
- **P13 (math)**: numeric environment table consistent; `DecomposeToYDR` (raw `atan2`) has no simulation caller.
- **P16 (sound)**: audio uses G6 on the main thread — it is one of the concurrent users in the tree race (§2.6).
- **P17b (options)**: `PrefsOtherWindow` Apply writes `g_app->m_difficultyLevel` from the `Difficulty` pref (§7.1) — a live desync vector despite the Difficulty control being hidden.
- **P19 (tutorial)**: tutorial director mutates sim state from the render-frame loop (L5).
- **For 21**: §4.3–4.4 (containers, ids, slices), §5 (state classification, render writes), §9 (rules).
- **For 40/41**: risk register §8.

## Contradictions / open questions

1. `build_matrix.md` §4 lists `INCLUDE_THEBEAST` as "on" (defined in `crate.h`); the definition is inside `#ifdef TARGET_DEBUG`, so it is **off** in the baseline and `Crate::CrateRewardTheBeast` does not exist there [C] `code/worldobject/crate.h`. P07's "Portal [B:FULL] via The Beast" inherits the error; P05 has it right.
2. P04 §0.2 describes `syncfrand` as an LCG; it is MT19937 [C] `random_number.cpp`.
3. P04 X3.6 says `Unit::s_offsets` is never reset between games; `Location::Init` and `Location::Empty` delete it [C].
4. Header comments vs implementation for every float generator range (`frand` inclusive upper bound, `sfrand`/`syncsfrand` are ±r/2) — documented as code (§2.1).
5. Whether the D3D configuration was ever used for multiplayer is open; it would not interoperate with the GL build (§3.4).

## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Whether the VS2005 x87 CRT `sin/cos/pow/acos` produce identical doubles on all x86 CPUs of the era (e.g. differing `fsin` argument reduction between vendors), i.e. how much work `iv_trunc_low_nibble` actually did on PC-only games | Disassemble the CRT used by the shipped binary or run a cross-CPU comparison |
| U2 | Real-world frequency of L1 (holding `CameraFreeMovement` while an opponent's squad moves) | Play-test with two clients or a replay harness that logs per-tick state |
| U3 | Whether the tree/global-LCG race (§2.6) ever fires in practice (sound triggers during level load, WAN thread wake-ups) | Instrument `AppRandom` with a thread-id check during `Tree::Generate` |
| U4 | Whether VS2005 `atof` is correctly rounded for every literal in the shipped data (matters only for non-MSVC peers) | Compare `atof` against a correctly-rounded parser over all numeric tokens in `data/` |
| U5 | Whether PC and Mac clients actually played together (the precision helper and `iv_*` wrappers imply the intent) | Mac target files / release notes for a cross-platform lobby |
| U6 | Whether `EntityGridCache` staleness (≤ 1 s, half-range reuse) was a deliberate gameplay property | Designer intent; compare behaviour with cache disabled |
| U7 | VS2005 defaults assumed for absent attributes (`/fp:precise`, x87 code generation, OpenMP off) | Build log of the shipped binary (`cl` command line) |

## Docs-vs-code discrepancies

| Doc | Says | Code | Ref |
|---|---|---|---|
| `docs/Network.txt` | client sends a "ResyncMe" message when it detects a gap | no resync message; gaps are filled by the server's resend window, and a desync is never repaired | [C] `ClientToServer::ReceiveLetter`, `Server::UpdateClients`, §6.2 |
| `docs/multiwinia_todo.txt` | "(DONE) … fix determinism; ensure floating point accuracy over multiple platforms" | implemented as `_PC_53` precision control plus binary32 rounding of transcendental results | §3 |
| `docs/multiwiniareleasenotes-beta.txt` (1.9.20) | beta builds enable debugging code to track rand sync errors | in this tree `TRACK_SYNC_RAND` is commented out for `TARGET_BETATEST_GROUP_ALL`; beta targets instead hash the full IFrame snapshot | [C] `code/lib/universal_include.h`, `iframe.cpp` |
| `contrib/systemiv/lib/math/random_number.h` comments | `frand`: `0 ≤ r < range`; `sfrand`, `_syncsfrand`: `−range < r < range` | `frand` ∈ [0, range]; `sfrand` ∈ [−range/2, range/2]; `syncsfrand` ∈ [−range/2, range/2) | [C] `random_number.cpp` |
| `code/lib/vector3.h` comment | `operator==` uses FLT_EPSILON | absolute tolerance 1e-6 (`NearlyEquals`) | [C] |

## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" in string ids `multiwinia_error_syncerror`, `multiwinia_error_lostconnection` | name/text | `code/renderer.cpp` | `net_error_desync`, `net_error_connection_lost` |
| "Darwinian" in debug state lines (`DARWINIAN Id[…]`) and entity type names | name | `code/worldobject/darwinian.cpp:Darwinian::LogState` | `UNIT` / "Worker" |
| Sync error report filenames `syncerror-…`, `syncdiff-…` | text (generic, not IP) | `ClientToServer::GetSyncFileName` | may be reused |
| MT19937 reference implementation (BSD-style licence, Matsumoto & Nishimura) | third-party code | `random_number.cpp`, `code/lib/math_utils.cpp` | use the C++ standard `std::mt19937` or PCG/xoshiro (own implementation) — attribution if the reference code is copied |
| SHA-1 from the bundled unrar sources used for seeds and IFrame hashes | third-party code (unrar licence) | `code/network/iframe.cpp`, `code/network/server.cpp:GenSyncRandSeed` (`contrib/unrar/sha1`) | any public-domain hash (xxHash, BLAKE3) |
| Ambrosia Software credit in `math_stubs` | third-party name | `contrib/systemiv/lib/math/math_stubs.cpp` | — (do not copy) |

## Appendix — dead/legacy code

| Item | Status | Ref |
|---|---|---|
| Orphan MT19937 `::syncrand()` (no seed function, never called) | [B:DEAD in effect] | `code/lib/math_utils.cpp` |
| Mac `StandardiseCPUPrecision` (`fnstenv`/`fldenv`, clear bit 8) | [B:DEAD] (`TARGET_OS_MACOSX && __i386__`) | `code/main.cpp` |
| `iv_strtod` declaration | no definition | `math_stubs.cpp` |
| `RandomNormalNumber`, `RandomApplyVariance` | no callers | `random_number.cpp` |
| `TRACK_SYNC_RAND` logging, entity watch list, debug IFrame snapshot/diff, `CHECK_IFRAME_SERIALISATION` (commented) | [B:DEBUG] | §6.3 |
| `INCLUDE_THEBEAST` crate outcome (`Portal`/`Tentacle` chain) | [B:DEBUG] | `crate.h` |
| `NetworkQuality` packet-loss simulator (`AppRandom`) | [B:DEAD] (`NETWORKQUALITY_TESTING` commented out) | `code/network/network_quality.cpp` |
| Testbed `srand(time)`/`rand()` fake server lists | [B:DEAD] (`TESTBED_ENABLED`) | `code/UI/GameMenuWindow.cpp` |
| Loader RNG (`AppSeedRandom(time(NULL))`, credits, egames `srand(0)`) | [B:DEAD] (`USE_LOADERS` off) | `code/loaders/*` |
| `SpiritStore::AddSpirit` unordered triple draw | [B:LEGACY] | `spiritstore.cpp` |
| Spectacle watching (wall-clock facing), Tripod wall-clock timers, `ResearchItem`/`Spam` random orientation, Armour direct local `SetOrders`, `GlobalWorld::TransferSpirits` | [B:LEGACY] | §2.5, §5.4 |
| `#pragma omp parallel for` in `Location::Advance` | inert (OpenMP off) | `code/location.cpp` |
| Time-based branch of `GetNumSlicesToAdvance` (shadowed local) | dead statement | `clienttoserver.cpp` |
| `Server::SkipToTime` | no caller | `server.cpp` |
