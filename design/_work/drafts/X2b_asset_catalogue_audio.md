# X2b_asset_catalogue_audio — Audio asset catalogue: every sample, every binding, every trigger
Scope: every file in `data/sounds/` (954 `.ogg`) and `data/mwsounds/` (378 `.ogg`) = 1332 files; `data/sounds.txt`; `tools/sounds.txt`; `tools/menu_sounds.txt`; `tools/unused_samples.sh`; `tools/unused_sample_groups.sh`. Supporting (read to resolve packaging/reachability, outside scope): `tools/build_multiwinia_data.bat`, `tools/listfiles.pl`, `tools/multiwinia_exclusions.txt`, `tools/patch.full.list`, `code/sound/sample_cache.cpp`, `code/sound/soundsystem.cpp`, `code/sound/sound_instance.cpp`, `code/lib/resource.cpp`, `contrib/systemiv/lib/tosser/hash_table.cpp`, `contrib/systemiv/lib/filesys/filesys_utils.cpp`, `code/app.cpp`, and every file containing a `Trigger*Event` / `StopAllSounds` call. Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Engine behaviour (voice allocation, ADSR, parameters, DSP, music slot mechanics, options screen) is specified in `drafts/P16_sound.md` (§27.x / §10.1–10.11) and is **not** repeated here; this draft is the per-asset / per-event layer on top of it and corrects P16 where noted.

Generated artefacts (all reproducible, `python3 -I _work/scripts/X2b_catalogue.py <SRC> <OUT/_work> _work/scripts/X2b_event_notes.py`):

| File | Rows | Content |
|---|---|---|
| `_work/drafts/X2b_samples.csv` | 1332 | one row per audio file: path, folder, name, channels, rate, duration_s, bytes, nominal_kbps, encoder vendor, Vorbis comment tags, referenced_by (events), via_groups, orphan_groups, status (referenced/unused), event_liveness (live/dead-data), archive, unused_samples.sh verdict, decoded PCM bytes |
| `_work/drafts/X2b_sound_events.csv` | 471 | one row per object-type × event: 256 pairs bound in `data/sounds.txt` + 215 pairs fired by code with no data; blueprint summary of every EVENT block, resolved files, every code trigger site with build/reachability status, fired-in-FULL verdict, type reachability, ListSoundEvents membership, gameplay trigger |
| `_work/drafts/X2b_callsites.csv` | 285 | every `TriggerEntityEvent`/`TriggerBuildingEvent`/`TriggerOtherEvent` call in `code/` (excluding the sound system itself): function, arguments, commented-out flag, `#if` evaluation under the baseline defines, vcproj membership |
| `_work/drafts/X2b_summary.txt` | — | statistics and cross-checks quoted below |
| `_work/scripts/X2b_event_notes.py` | — | hand-verified facts (reachability, dynamic names, descriptions) consumed by the script; parsed with `ast.literal_eval`, never imported |

---

## [→ 10_audio_design] 10.A Method and verification of P16

1. **Ogg headers** [C]: each file is walked page by page (`OggS` capture, segment table). Packet 1 is the Vorbis identification header (`\x01vorbis`: version, channels, rate, bitrate max/nominal/min, block sizes); packet 2 the comment header (vendor string + `TAG=value` list); duration = granule position of the last page with granule ≠ −1, divided by the rate (this equals `ov_pcm_total` for these files: every file has one logical stream, no resync gaps, Vorbis version 0) [C] `_work/drafts/X2b_summary.txt`.
2. **Blueprint parse** [C]: an independent re-implementation of `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints`, `ParseSoundEvent`, `ParseSampleGroup` (tokens split on blanks/tabs, keywords case-insensitive, `SOUNDNAME`/`SAMPLE` values lower-cased and stripped from the last `.` as `contrib/systemiv/lib/filesys/filesys_utils.cpp:RemoveExtension` does). Cross-checked block-by-block against `drafts/P16_sound_events.csv`: 296 = 296 EVENT blocks, **0 mismatches** in source, event, SOUNDNAME, LOOPTYPE, POSITIONTYPE [C].
3. **Code sites** [C]: every `Trigger(Entity|Building|Other)Event(` in `code/**/*.{cpp,h}` (285 sites), with block/line-comment stripping, a baseline preprocessor evaluator (`#if/#ifdef/#ifndef/#elif/#else` over the FULL define set of build_matrix §3–4), vcproj membership from `manifest.csv`, enclosing `Class::Method`. Base-class sites (`Entity::Begin`, `Entity::ChangeHealth`, `Building::Initialise`, `Building::Damage`, `Building::SetTeamId`, …) are attributed to each concrete type unless a subclass overrides the method **without** chaining to a base implementation (body search for `X::Method(`); `Entity::Attack` and `Entity::EnterTeleports` are restricted to the types whose code actually calls them (verified callers: `code/worldobject/darwinian.cpp:Darwinian::AdvanceCombat`, `code/unit.cpp:Unit::Attack`, `code/worldobject/lasertrooper.cpp`, `insertion_squad.cpp`, `officer.cpp`, `shaman.cpp`). Runtime-built names are expanded by hand (§10.C notes). Reachability of compiled-in sites (e.g. Darwinia-only interfaces) is hand-verified and recorded in `X2b_event_notes.py:SITE_STATUS`.
4. **P16 verification**: P16's counts are confirmed (68 sources = 21 ENTITY + 35 BUILDING + 12 OTHER; 296 blocks = 108/96/92; 1332 mono files, 1330 at 44100 Hz + `data/sounds/fodder.ogg` and `data/mwsounds/chat.ogg` at 22050 Hz; largest decoded track `tresk_-_excuses` 30 389 760 bytes ≈ 29.0 MiB) with two corrections: (a) there are **131** `SAMPLEGROUP` definitions with **1126** `SAMPLE` lines — `officercreate` is defined twice with identical content (`data/sounds.txt` lines 3858 and 4101), so 130 distinct groups / 1120 distinct entries as P16 states; (b) P16 §10.11's "data no code fires" list misses `CratePowerup FlameTurret`, `BUILDING GunTurret Damage`, `ENTITY Egg Die`, `ENTITY Officer Attack`, `BUILDING Incubator Reprogramming`, `BUILDING Generator ReprogramComplete` (§10.C).

## [→ 10_audio_design] 10.B How a sample name becomes a file (resolution, precedence, archives)

### Name normalisation and lookup [C]
| Step | Rule | Ref |
|---|---|---|
| 1 | `SOUNDNAME x` / `SAMPLE x`: `x` lower-cased (`StrToLower`) and cut at its **last** `.` (`RemoveExtension`). No shipped name carries an extension. | `code/sound/soundsystem.cpp:ParseSoundEvent`, `ParseSampleGroup` |
| 2 | `SOURCETYPE 1` → `SOUNDNAME` is a group name: linear scan of `m_sampleGroups`, `stricmp`, **first definition wins**; not found → `AppReleaseAssert("Failed to find Sample Group")` (fatal). | `soundsystem.cpp:GetSampleGroup`; `sound_instance.cpp:SoundInstance::OpenStream` |
| 3 | Group member chosen `AppRandom() % n` (n halved/quartered by pref `SoundMemoryUsage` 2/3, see P16 §27.6) | `sound_instance.cpp:OpenStream` |
| 4 | `CachedSample("<name>")` asks the resource layer for `sounds/<name>`; if that fails, `mwsounds/<name>`; if both fail → `AppReleaseAssert("Failed to open sound stream decoder")` (**fatal**). | `code/sound/sample_cache.cpp:CachedSample::CachedSample` |
| 5 | `Resource::GetSoundStreamDecoder` appends **`.ogg`** unconditionally → the WAV branch of `SoundStreamDecoder` can never be reached from a blueprint. | `code/lib/resource.cpp:Resource::GetSoundStreamDecoder` |
| 6 | `Resource::GetBinaryReader("sounds/x.ogg")`: (USE_DARWINIA_MOD_SYSTEM mod folders — off) → loose file `data/sounds/x.ogg` on disk (`DoesFileExist` = `fopen`, case-insensitive on Windows; `FindCaseInsensitive` elsewhere) → archive entry `data/sounds/x.ogg` in `m_resourceFiles`. | `resource.cpp:GetBinaryReader`; `contrib/systemiv/lib/filesys/filesys_utils.cpp:DoesFileExist` |
| 7 | Archive keys are `strlwr`-ed at load and `m_resourceFiles` is a `HashTable` whose hash masks the case bit (`& 0xDF`) and compares with `stricmp` → lookups are case-insensitive. 359 of the 1332 file names contain upper-case letters (e.g. `data/mwsounds/CrateFall1.ogg`) and all resolve. | `resource.cpp:Resource::ParseArchive`; `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::HashFunc`, `GetIndex` |

**Precedence** [C]: loose file > archive; `data/sounds/` > `data/mwsounds/`; among archives the **later** `ParseArchive` replaces an earlier entry with the same key ("Subsequent archives may override existing resources"). Load order: `init.dat` (App ctor), `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat` (App ctor, main thread); `sounds.dat`, `sounds2.dat` (`App::LoadSounds`, sound work queue) [C] `code/app.cpp:App::App`, `App::LoadSounds`. In the shipped tree no stem exists in both folders (0 case-insensitive collisions), no name contains a space, and no file appears in two archives [C] `X2b_summary.txt`.

**Language-specific audio: none** [C]. `data/language/` holds only the 9 language `.txt` tables; no `.ogg`/`.wav` exists anywhere in the tree outside `data/sounds/` and `data/mwsounds/` (the only other `.wav` is `contrib/systemiv/contrib/SDL-1.2.11/test/sample.wav`, SDL test data); `tools/build_language_dat.bat` and `tools/init.list` contain no audio. The sound system has no locale switch.

**Formats** [C]: 1332/1332 Ogg Vorbis I, mono, 44100 Hz except the two 22050 Hz files above. Encoders (comment-header vendor): `Xiph.Org libVorbis I 20040920` 863 files (all at nominal 65 kbps except 1 — the Darwinia-era `data/sounds` set), `Xiph.Org libVorbis I 20020717` 466 (128 kbps: 369 of the 378 `data/mwsounds` files + 89 older `data/sounds` files, 96 kbps 8), `Xiph.Org libVorbis I 20030909` 2, `AO; aoTuV b3 [20041120]` 1 (a music track). Only 33 files carry tags, all `DATE=2005-09-05/06` (`darwin_burn_death*`, `darwin_on_fire*`, `fuel_load_passenger*`) — no artist/title tags anywhere [C] `X2b_samples.csv:tags`.

### Packaging (`tools/build_multiwinia_data.bat`, PC default platform) [C]
| Archive | Built from | Files (from this tree) | Note |
|---|---|---|---|
| `menusounds.dat` | `tools/menu_sounds.txt` (18 paths, prefixed `data/`), `rar -m2` | 18 (`multilobbyamb1..10`, `shipheartbeat1`, `mouse2`, `mouse3`, `menuscroll1`, `menuscroll2`, `alien_lands3`, `evil`, `rumble1`) = exactly the samples of `MultiwiniaInterface LobbyAmbience`, `MenuRollOver`, `MenuSelect`, `MenuCancel` | parsed in the App constructor, i.e. before the background sound load |
| `sounds.dat` | `tools/sounds.txt` (1297 paths: 935 `sounds/`, 362 `mwsounds/`), `rar -m2` | 1297; all exist on disk | includes 191 of the 192 unused samples |
| `sounds2.dat` | every `.ogg` of `perl tools/listfiles.pl data completeExclusions` (`completeExclusions` = `tools/sounds.txt` + `tools/menu_sounds.txt` + `tools/multiwinia_exclusions.txt`) | 6: `data/mwsounds/beep_harsh_malfunction.ogg`, `chat.ogg`, `mouseclick1.ogg`, `rollover1a.ogg`, `rollover1b.ogg`, `rollover1c.ogg` (late Multiwinia UI additions never added to `tools/sounds.txt`) | |
| `main.dat` | the same list minus every path containing `sounds/`, then only `*.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim` | 0 audio; carries `data/sounds.txt` and `data/effects.txt` | |
| not shipped | `tools/multiwinia_exclusions.txt`: `sounds/tresk*`, `sounds/trash80*`, `sounds/dma_sc*`, `sounds/twoataris*`, `sounds/pang*`, `sounds/speccyloader.ogg`, `sounds/matrix.ogg`, `sounds/fodder.ogg` | 11 music files (1702.0 s, 11.5 MiB) | see §10.F |

Thus `tools/sounds.txt` and `tools/menu_sounds.txt` are simultaneously **archive manifests** and **exclusion lists** (they keep those files out of `main.dat`/`sounds2.dat`). `tools/patch.full.list` (a Darwinia patch list naming `sounds/pang.ogg`, `TaskIn/TaskOut/Mouse*.ogg`, `rocket_boom1..4.ogg`, …) is referenced by no Multiwinia build script [C] (grep of `tools/`).

Design consequences: (1) a blueprint that names a sample absent from every archive is a **crash**, not silence (steps 2/4) — the only such references are the 11 excluded music files, whose events are unreachable (§10.F); (2) `menusounds.dat` is loaded early but every `Trigger*Event` returns while `App::m_soundsLoaded` is false, and that flag is set only after `sounds.dat` + `sounds2.dat` are parsed (`soundsystem.cpp:NotReadyForGameSoundsYet`), so the early front-end archive buys nothing audible in this code version [I] (likely a remnant of an earlier "play lobby ambience during load" path — the commented-out `TriggerOtherEvent(…"LobbyAmbience"…)` in `code/app.cpp:App::CheckSounds`). A new engine should keep "UI/front-end bank loaded first, game bank streamed later" but actually allow UI events from the first bank.

## [→ 10_audio_design] 10.C Complete event table — every object type × event bound in `data/sounds.txt`

256 (source, event) pairs from 296 EVENT blocks. Column legend: *Data blocks* — one line per EVENT block: `[Sample|Group name]` (SOURCETYPE), position (`2D`, `Stat` = 3D stationary, `Att` = 3D attached, `Ed` = in-editor) / loop (`Once`, `Loop`, `LoopADSR`) / instance (`MonoRandom`, `MonoNearest`; polyphonic omitted) / team filter (`MyTeam`, `Enemy`; all omitted); `md` MINDISTANCE (wu); `v` VOLUME (0–10 log scale), `f` FREQUENCY multiplier, `a/s/r/ld` ATTACK/SUSTAIN/RELEASE/LOOPDELAY (s); parameter forms: number = fixed, `rnd lo..hi` = ranged random, `Link inLo..inHi->outLo..outHi` = linked, `sN` = smoothing, `/once` = UpdateOncePerLoop; `fx` = DSP chain (off in shipped config, P16 §10.7). *Files* = distinct files the event can play. *Fired in FULL*: `yes` = a live, reachable call site exists in the baseline and the object type exists in Multiwinia; `site-live/type-LEGACY|DEBUG` = the code path is live but the object type never appears in a Multiwinia match; `no(…)` = all sites are LEGACY / not-compiled / commented-out / absent. Site status tags: `[LEGACY(reason)]`, `[not-compiled(#if X)]`, `[commented-out]`, `[DEAD(reason)]`, no tag = live in FULL. All rows [C] from `data/sounds.txt` + the cited code unless the trigger text says [I].

Runtime-built event names (expanded in the table) [C]:
- `Crate Capturing_<n>pc`: `sprintf("Capturing_%dpc", postIntCaptured*20)` with `postIntCaptured = int(5·captureTimer/CRATE_CAPTURE_TIME)` ∈ {1..5} while the owner's majority holds → `Capturing_20pc` … `Capturing_80pc` (data) and `Capturing_100pc` (no data). The **decay** branch formats `"Capturing_%pc"` — `%p` is the pointer conversion, so the event becomes e.g. `Capturing_00000003c` and never matches: progress tones are silent while a crate's capture timer is falling (bug, don't copy) `code/worldobject/crate.cpp:Crate::AdvanceOnGround`.
- `EscapeRocket <state sound>`: `EscapeRocket::SetupSounds` picks `Refueling` (no data), `Unhappy`, `Malfunction`, `Happy` or `Flight` from state/damage/fuel and keeps exactly one instance alive by polling `NumInstances(m_id, "EscapeRocket <name>")` every sim tick (stopping all five otherwise) `code/worldobject/rocket.cpp`.
- `Grenade|AirStrikeBomb <Create|Bounce|Flash|Explode>`: `ThrowableWeapon::TriggerSoundEvent` routes `EffectThrowableGrenade`/`AirstrikeMarker`/`ControllerGrenade` to `OTHER Grenade` and `EffectThrowableAirstrikeBomb`/`EffectFlameGrenade` to `OTHER AirStrikeBomb` `code/worldobject/weapons.cpp`. Bomber bombs are created without `Initialise()` (no Create) `code/worldobject/airstrike.cpp:SpaceInvader::Advance`.
- `CratePowerup <Crate::GetName(type)>`: `Crate::TriggerSoundEvent(type, pos, team)` → `TriggerOtherEvent(pos, name, TypeCratePowerup, team)`; callers listed per row; `RunGunTurret` always passes `CrateRewardGunTurret`, so the `FlameTurret` block is dead; `Engineer`, `RocketTurret`, `NapalmStrike` (uses `Airstrike`), `SlowDown`, `SpeedUp` names are never passed (Engineer plays `Gesture GestureSuccess` instead) `code/worldobject/crate.cpp`, `code/taskmanager.cpp`.
- `Music <name>`: `Script::RunCommand_TriggerSound(name)` — script names found in `data/scripts/*.txt` are `cutscene1`–`cutscene5`, `endsequence`, `Demo2Intro`, `Demo2Mid` (case-insensitive match).

<!-- X2B:EVENTS:BEGIN -->
<!-- X2B:EVENTS:END -->

Totals: 182 of 256 pairs are live in FULL; 30 are on live code paths but their object type never exists in Multiwinia (Darwinia buildings/LaserTroop/Tripod); 3 are Tank (debug-only); 41 are never fired (17 with LEGACY-only sites, 3 LEGACY + commented-out, 3 commented-out only, 10 not compiled, 7 with no call site at all, 1 DEAD) [C] `X2b_sound_events.csv`.

## [→ 10_audio_design] 10.D Events fired by code that have no data (silent in the shipped game)

These are the events the game asks for and the data does not answer (215 object-type × event pairs, aggregated by group + event; types listed are those that have a blueprint in `sounds.txt` — types with no blueprint at all are silent for every event: ENTITY `Lander`, `AI`, `Shaman`, `Tentacle`, `DropShip`; every BUILDING type not listed in §10.C, e.g. `Wall`, `StaticShape`, `PowerSplitter`, `PylonEnd`, `SpawnPointMaster`, `SpawnLink`, `SpawnPopulationLock`, `AITarget`, `AIObjective`, `AIObjectiveMarker`, `RestrictionZone`, `JumpPad`, `WallBuster`, `EruptionMarker`, `SmokeMarker`, `ControlTower`, `Factory`, `Cave`, `Bridge`, …(full list: building names absent from §10.C in `code/worldobject/building.cpp:Building::GetTypeName`)). Notable gameplay-relevant gaps for the new game: **match end** (`GameOver`, `YouWon`, `YouLost` — samples `data/mwsounds/GameOver1.ogg`, `GameOver2.ogg`, `YouWon.ogg`, `YouLost2.ogg` exist but are unbound), crate `Expired`, `Capturing_100pc`, every laser `Create`/`HitBuilding`/`KilledEntity`, squad `ThrowGrenade` (the only reachable squad secondary), turret `TargetSighted`/`FireRocket`, `LaserFence Electrocute`, statue `Lift`/`Carry` (the code even stops `"Statue Carry"` on drop), `Armour LoadDarwinian`/`UnloadDarwinian`, Officer `SetOrderFollow`/`SetOrderAbsorb`/`SetOrderNone`, `Spider PounceLand`, `Centipede Grow`/`Panic`/`EnemySighted`, `SoulDestroyer Panic`, `Triffid LaunchEgg`, `TrunkPort PowerUp`, task-manager `SelectTask`/`TaskManagerSelectTask`/`TaskManagerDeselectTask`, `Interface SetOrderGoto`, `Ambience ExitLocation`.

<!-- X2B:CODEONLY:BEGIN -->
<!-- X2B:CODEONLY:END -->

## [→ 10_audio_design] 10.E Stop / keep-alive sites (how looping sounds end)

Every `StopAllSounds(objectId, partialName)` call outside `code/sound/` [C] (substring match on `"<Source> <Event>"`, P16 §10.1). A looping event without a stop site ends only when its emitter disappears (attached) or never (2D/stationary loops).

| Partial name stopped | Emitter | Where / when | Ref |
|---|---|---|---|
| `MultiwiniaInterface SelectionCircle` | none | selection circle released | `code/location_input.cpp:LocationInput::AdvanceTeamControl` L739 |
| `MultiwiniaInterface LobbyAmbience` | none | `MainMenuLoop` exits | `code/main.cpp` L1477 |
| `Ambience EnterLocation` | none | `LocationGameLoop` exits (then `Ambience ExitLocation` fired, no data) | `code/main.cpp` L608 |
| `Ambience EnterGlobalWorld` | none | world loop exit / script [B:LEGACY] | `code/main.cpp` L768, `code/script.cpp` L446 |
| `Music`, `Music <event>` | none | cutscene escape / script StopSound [B:LEGACY]; each loader stops its own track [B:DEAD]; `CreditsLoader` stops `Music Cutscene3` | `code/script.cpp` L371/L654, `code/loaders/*`, `code/startsequence.cpp` L94 |
| `Tree Burn` | tree | fire out / tree destroyed | `code/worldobject/tree.cpp` L189, L261 |
| `RadarDish BeginRotation` | dish | before re-aim; on alignment | `code/worldobject/radardish.cpp` L177, L377 |
| `RadarDish ConnectionEstablished` | dish | link lost | `radardish.cpp` L288 |
| `SolarPanel Operate` | panel | occupancy ≤ 60 % while operating | `code/worldobject/generator.cpp` L759 |
| `SpaceShip TractorBeam` | ship | abduction ends | `code/worldobject/spaceship.cpp` L120 |
| `Officer SetOrderNone/Goto/Follow/Absorb/Formation` | officer | `Officer::CancelOrderSounds` before every new order sound | `code/worldobject/officer.cpp` L1315–1319 |
| `Crate Falling` | crate | lands | `code/worldobject/crate.cpp` L118 |
| `Crate Capturing_` (all progress tones) | crate | `Crate::StopCaptureSounds` before each new progress tone / on capture / loss | `crate.cpp` L572 |
| `Crate BeginCapture` | crate | captured or lost | `crate.cpp` L647, L688 |
| `Engineer BeginReprogramming` | engineer | reprogramming ends [B:LEGACY] | `code/worldobject/engineer.cpp` (7 sites) |
| `GodDish ConnectToGod` | dish | deactivated [B:LEGACY] | `code/worldobject/goddish.cpp` L198 |
| `Spam Create` | spam | becomes research [B:LEGACY] | `code/worldobject/spam.cpp` L324 |
| `Rocket Create` | rocket | explodes | `code/worldobject/weapons.cpp` L538 |
| `Darwinian SeenThreat` | darwinian | threat cleared / state change | `code/worldobject/darwinian.cpp` L1019, L1043, L3048 |
| `Darwinian TakenControl` | darwinian | released (no data for either) | `darwinian.cpp` L3548 |
| `FuelPipe PumpFuel` | pipe | level ≤ 0.2 | `code/worldobject/rocket.cpp` L462 |
| `EscapeRocket Refueling/Happy/Unhappy/Malfunction/Flight` | rocket | wrong state sound playing | `rocket.cpp` L944–948 |
| `EscapeRocket EngineBurn` | rocket | engines off | `rocket.cpp` L979 |
| `Statue Carry` | statue | dropped (no data) | `code/worldobject/carryablebuilding.cpp` L251 |
| all sounds of the tank | tank | tank dies [B:DEBUG] | `code/worldobject/tank.cpp` L149 |

Looping blocks (LOOPTYPE 1/2) with no stop site — they end only when their attached emitter disappears (hard cut, P16 §27.4) [C]: `Engineer Create`, `Squadie Create`, `SporeGenerator Create`, `Centipede Create`, `SpaceInvader Create`, `Spider Create`, `ArmyAnt Create`, `Armour Create`, `SoulDestroyer Create`, `SoulDestroyer EnemySighted`, `Harvester Create` (3 blocks), `Nuke Create`, `Meteor Create` (2), `Tank Create`, `SpaceShip Create` (4), `LaserFence Create`, `Tree CreateNormal`, `Tree CreateMagic`, `Tree CreateEvil`, `Triffid Burn`, `MultiwiniaZone Create`, `ControlStation Create`, `Grenade ExplodeController`, `Rocket Create` (stopped by explode), and the legacy `Powerstation`, `TrunkPort ReprogramComplete`, `ResearchItem`, `Refinery`, `Yard`, `SpiritReceiver`, `SpiritProcessor`, `Blueprint*`, `Spam`, `GodDish Create` loops. **Stationary loops are never stopped**: `BUILDING PulseBomb Create` (group of `choir1..6`, POSITIONTYPE 1, Loop) and the legacy `Generator ReprogramComplete` have POSITIONTYPE 1, so `SoundInstance::m_objId` is never set (P16 §10.11), no `StopAllSounds(m_id, …)` can reach them and the orphan cull only applies to attached sounds → the Assault pulse-bomb choir keeps looping (at its fixed world position, attenuated by distance) after the bomb is destroyed and even after the match ends [I from `code/sound/soundsystem.cpp:SoundSystem::Advance`, `StopAllSounds`; not observed]. A new engine must release all location sounds at location exit and allow stopping stationary instances by emitter.

## [→ 10_audio_design] 10.F Music — confirmed never heard in Multiwinia

P16 §10.9 inferred that the music channel is silent in Multiwinia. **Confirmed** by three independent lines of evidence:

1. **No reachable trigger** [C]: the 17 `OTHER Music` blocks are fired only by (a) the seven `code/loaders/*_loader.cpp::Run` methods and `StartSequence::Advance` — compiled only under `USE_LOADERS` (`Loader::CreateLoader` cases, `RunBootloader`), off in every target [B:DEAD]; (b) `CreditsLoader::Run` (compiled, but created only from `Camera::AdvanceSphereWorldOutroMode` and `Script::RunCommand_RunCredits` — Darwinia world-map outro and scripts) [B:LEGACY]; (c) `Script::RunCommand_TriggerSound` — no Multiwinia path loads a script (P19 §1.5) [B:LEGACY].
2. **Not packaged** [C]: all 11 music files are excluded from the Multiwinia PC data build by `tools/multiwinia_exclusions.txt` (`sounds/tresk*`, `sounds/trash80*`, `sounds/dma_sc*`, `sounds/twoataris*`, `sounds/pang*`, `sounds/speccyloader.ogg`, `sounds/matrix.ogg`, `sounds/fodder.ogg`) and are listed in neither `tools/sounds.txt` nor `tools/menu_sounds.txt` → present in no shipped archive. `data/scripts/*` is excluded too.
3. **Would crash if reached** [I]: a music trigger promotes `m_requestedMusic` → `OpenStream` → `CachedSample` construction → `AppReleaseAssert("Failed to open sound stream decoder")` for an unshipped file (§10.B step 4). Any shipped reachable music path would therefore have been found in testing as a crash.

Music tracks (all `data/sounds/`, 44.1 kHz mono unless noted) and the blueprints that name them [C]:

| File | Duration s | Size KiB | Encoder kbps | Events (OTHER Music …) | Original context | Tag |
|---|---|---|---|---|---|---|
| `data/sounds/tresk_-_excuses.ogg` | 344.555 | 2014 | 33 (aoTuV) | Cutscene1 (Loop), Cutscene5 (Once, attack 3.10 s) | Darwinia cutscene scripts `music1.txt`, `containment_intro.txt`, `music5.txt`, `biosphere_intro.txt` | [B:LEGACY], not shipped |
| `data/sounds/tresk_-_faces-of-a-fashion.ogg` | 278.648 | see csv | 65 | Cutscene2 (Loop), EndSequence (Once, attack 5.23 s), Demo2Mid (Once) | `music2.txt`, `containment_end.txt`, `gameover.txt`, `launchpad_purpose.txt` | [B:LEGACY], not shipped |
| `data/sounds/trash80_-_pain-fade-down.ogg` | 213.943 | see csv | 65 | Cutscene3 (Loop, attack 5.23 s), Credits (Once, attack 2.71 s), Demo2Intro (Once) | `music3.txt`, `yard_intro.txt`, `launchpad_intro.txt`, credits loader | [B:LEGACY], not shipped |
| `data/sounds/tresk_-_impact-of-silence.ogg` | 172.130 | see csv | 65 | Cutscene4 (Loop, attack 2.90 s) | `music4.txt`, `yard_end.txt` | [B:LEGACY], not shipped |
| `data/sounds/tresk_-weeklies-_schroeders-failure.ogg` | 103.579 | see csv | 65 | LoaderSoul (Loop) | soul loader | [B:DEAD], not shipped |
| `data/sounds/dma_sc-visitors_from_dreams.ogg` | 325.251 | see csv | 65 | LoaderRaytrace (Loop), StartSequence (Once) | raytrace loader, first-run boot sequence | [B:DEAD], not shipped |
| `data/sounds/twoataris.ogg` | 87.484 | see csv | 65 | LoaderGameOfLife (Once) | game-of-life loader | [B:DEAD], not shipped |
| `data/sounds/pang.ogg` | 87.121 | see csv | 65 | LoaderAmiga (Once) | Amiga loader | [B:DEAD], not shipped |
| `data/sounds/speccyloader.ogg` | 58.979 | see csv | 65 | LoaderSpeccy (Once) | ZX Spectrum loader | [B:DEAD], not shipped |
| `data/sounds/matrix.ogg` | 24.000 | see csv | 65 | LoaderMatrix (Once) | matrix loader | [B:DEAD], not shipped |
| `data/sounds/fodder.ogg` | 6.315 | see csv | 60/80 (22050 Hz) | LoaderFodder (Once) | Cannon-Fodder homage loader | [B:DEAD], not shipped |

(Exact sizes per file are in the §25.B catalogue; total 1702.0 s, 12 070 545 bytes.)

**What the player actually hears instead of music** [C]: in the front end, `MultiwiniaInterface LobbyAmbience` — three 2D beds started once by `GameMenuWindow::Update` and stopped when `MainMenuLoop` exits: (a) group `newsamplegroup8` = `alien_lands3` (30.5 s), `rumble1` (22.1 s), `evil` (53.9 s), LoopADSR, vol 8.0, attack 9.29 s, sustain 10.65 s, release 10.84 s; (b) `shipheartbeat1` (1.4 s) Loop at pitch 0.5, vol 8.5; (c) group `multilobbyambiences` = `data/mwsounds/MultiLobbyAmb1..10.ogg` (19.4–36.0 s each, 287 s total), LoopADSR, vol 9.0, pitch random 0.67–1.01 per note, sustain 10.65 s, release 10.26 s — a generative pad bed (each note ≈ 20.9 s audible, the next random pad starting at each release). In a match, `Ambience EnterLocation` (P16 §10.9): `newsamplegroup8` faded out with camera height (Ypos 232 → 736 : vol 9.58 → 0.10), `white_noise` (29.0 s, Ypos 0 → 992 : 3.03 → 7.03), and group `highaltitude` = `data/mwsounds/High1..7.ogg` (15.3–39.6 s, 179.6 s total; Ypos 0 → 988 : 0.06 → 5.97). There is **no in-match music, no victory/defeat music, no menu music track** in the shipped game. Design note for the new game: music support exists in the engine (single streamed slot, P16 §27.10) but has no live content or triggers — design a music system from scratch (it is a missing feature, not a feature to port).

## [→ 10_audio_design] 10.G Menu, HUD and UI sounds (complete, with files)

All 2D (head-relative, never pitch-bent by game speed) unless noted; all [B:FULL] live unless tagged [C].

| Event | Trigger | File(s) (duration s) | Vol / pitch |
|---|---|---|---|
| `MultiwiniaInterface MenuRollOver` | mouse enters a menu control: `GameMenuButton::Render`, `GameMenuInputField::Render`, `GameMenuDropDown::Render`, `GameMenuCheckBox::Render` (`code/game_menu.cpp`), `FancyColourButton` (`code/UI/FancyColourButton.h`), `LevelSelectButton::Render` (`code/UI/LevelSelectButton.h`), leaderboards (`code/interface/leaderboards_window.cpp`) | group `mouseovericon`: `data/sounds/mouse2.ogg` (0.056), `mouse3.ogg` (0.050) | 5.03 |
| `MultiwiniaInterface MenuSelect` | button activated: `GameMenuButton::MouseUp`, `LevelSelectButton::MouseUp`, `MultiwiniaEditorButton::MouseUp`, `GameOptionsWindow::CreateMenuControl` buttons, `ConfirmExitWindow`, `HelpAndOptionsWindow`, `multiwinia_window.cpp` buttons | `data/sounds/menuscroll2.ogg` (1.862) | 9.00 |
| `MultiwiniaInterface MenuCancel` | back/close: `GameMenuButton::MouseUp` (cancel), `BackPageButton::MouseUp`, `MenuCloseButton::MouseUp` (`code/interface/mainmenus.h`) | `data/sounds/menuscroll1.ogg` (2.171) | 8.00 |
| `MultiwiniaInterface LobbyAmbience` | §10.F | 14 files | 8.0 / 8.5 / 9.0 |
| `MultiwiniaInterface ChatMessage` | chat line in lobby (`GameMenuWindow::NewChatMessage`, not for `PLAYERJOIN`) or in game (`Location::NewChatMessage`) | `data/mwsounds/chat.ogg` (0.216, 22050 Hz) | 8.00 |
| `MultiwiniaInterface CountdownTick` | grace period (first tick when all players ready, then each whole second of `GRACE_TIME` = 3.0 s), in-game clock at every whole minute and each of the last 10 s (render-driven) | `data/mwsounds/CountdownBeep3.ogg` (0.094) | 7.03 |
| `MultiwiniaInterface CountdownZeroTick` | grace period over (game starts; game speed requested back to 1.0) and timed game reaches 0 | `CountdownBeep3.ogg` at pitch 2.00 | 7.10 |
| `MultiwiniaInterface SelectionCircle` | selection circle starts (not during grace period) | `data/mwsounds/FilterSweep.ogg` (3.538, release 0.58 s) + `DroneLoop.ogg` (5.957, Loop, attack 1.35 s, release 0.97 s) stopped on circle end | 7.00 / 8.84 |
| `MultiwiniaInterface SelectUnit` | unit selected by clicking (max 1 per 0.5 s) | `data/mwsounds/mouseclick1.ogg` (0.107) | 6.97 / pitch 0.33 |
| `MultiwiniaInterface ToggleUnitState` | click on own selected Armour (deploy/conversion toggle) or Officer (mode cycle) | `mouseclick1.ogg` | 9.03 / pitch 0.58 |
| `MultiwiniaInterface MouseError` | invalid click (nothing selectable while something selected; right-click while placing a task) | `data/mwsounds/beep_harsh_malfunction.ogg` (0.840) | 8.55 / pitch 0.62 |
| `MultiwiniaInterface UnitHighlight` | cursor highlight moves to another object (max 1 per 0.2 s); **3D stationary at the object**, team 255 | group `unithighlight`: `data/mwsounds/rollover1a.ogg` (0.018), `rollover1c.ogg` (0.018) (`rollover1b.ogg` unused) | 7.00 / pitch 0.90 |
| `Interface MouseOverIcon` | task-manager highlight moves (`TaskManagerInterfaceMultiwinia::AdvanceScreenZones`, `TaskManagerInterface::AdvanceTab`) | group `mouseovericon` (`mouse2`, `mouse3`) | 7.00 |
| `Interface DeleteTask` | task terminated from the task manager | `data/sounds/deselect1.ogg` (2.508) | 9.13 |
| `Gesture GestureBegin` | task started from the Multiwinia task manager (`RunScreenZone` RunProgram / SelectWeapon) | `data/mwsounds/FilterSweep.ogg` | 6.97, release 0.39 |
| `Gesture GestureSuccess` | officer promotion, engineer deployed, weapon selected | `data/sounds/gesturesuccess.ogg` (2.215) | 10.00 |
| `Interface Show` / `Hide` / `Slide` | Darwinia task-manager open/close/scroll — Show/Hide commented out in the Multiwinia interface [B:LEGACY] | `taskin.ogg` (3.126) / `taskout.ogg` (0.972) / `menuscroll2.ogg` | 8.48 / 8.52 / 9.48 |
| `Gesture GestureFail` | Darwinia gesture UI only [B:LEGACY] | `gesturefail.ogg` (2.215) | 10.00 |
| silent UI events (no data) | `Interface SelectTask`, `TaskManagerSelectTask`, `TaskManagerDeselectTask`, `SetOrderGoto`, `ShowLogo`; `Gesture GestureEnd`; `MultiwiniaInterface GameOver`, `YouWon`, `YouLost` | — | — |

Game-state stings that are not UI-source events but serve as HUD feedback [C] (§10.C rows): `Crate BeginCapture` (my-team / enemy stings), `Crate LostCapture`, `Crate Captured`, `Statue ChangeTeam` (my-team / enemy, 2D), `MultiwiniaZone CaptureStatue` (2D + team bugles), `MultiwiniaZone ChangeTeam` (KotH captured/lost, team-filtered), `SpawnPoint TurnOn`/`TurnOff`, and the 25 live `CratePowerup` reward stings (2D).

## [→ 10_audio_design] 10.H Voice / speech content

There is no dialogue system, no subtitles/voice sync and no localisation of audio [C] (§10.B). Voiced content consists of short non-lexical or one-word barks bound to events; the file names suggest the content (e.g. `YesSir`, `Aye`, `Attention`, `Retreat`, `Attack`), but the recordings were not auditioned [U: listen to resolve whether any line is intelligible speech that would need localisation or replacement].

| Voice set | Files | Bound to (live unless tagged) |
|---|---|---|
| Darwinian spawn chatter | `data/sounds/darwin_create*.ogg` (15 in group `darwiniancreate`) | `Darwinian Create` (one shared MonophonicNearest voice) |
| Darwinian acknowledgement | `data/mwsounds/DSelect1..10.ogg` (group `dselect`) | `Darwinian Selected`, `Darwinian GivenOrdersFromPlayer` |
| Darwinian battle cries | `data/mwsounds/Attack*.ogg` (37, group `combatvoxattack`); `darwinian_threat_attack*.ogg` (3, `darwinseethreatattack`) | `Darwinian SeenThreatAttack`; `SeenThreatAttackHorn` (and dead `Officer Attack`) |
| Darwinian retreat | `data/mwsounds/Retreat*.ogg` (26, `combatvoxretreat`) + `data/sounds/darwinian_threat_run*.ogg` (10, `darwinrunaway`) | `Darwinian SeenThreatRunAway` (two layered blocks) |
| Darwinian pain | `darwin_losehealth*.ogg` (18, `darwinianlosehealth`); `data/mwsounds/Hit1..36.ogg` (`combatvoxinjured`) | `Darwinian LoseHealth`, `Officer LoseHealth`; `Laser InjuredEntity` |
| Darwinian death | `darwin_death*.ogg` (8, `darwiniandie`); `LaserHitDarwinian1..12.ogg` (`laserhitdarwinian`); `darwin_burn_death1..6.ogg` + `silence1..7.ogg` (`darwinnianburncry`) | `Darwinian Die` (2 layers), `Officer Die`, legacy `LaserTroop Die`; `Darwinian OnFire` (the 7 silence files make roughly half the picks silent) |
| Darwinian effort / joy | `Carrying3a..3d.ogg` (`carrystatue`); `darwin_jump*.ogg` (9, `victoryjum`) | `Darwinian LiftBuilding`; `Darwinian VictoryJump` |
| Officer voice | `officer_create4/5/6/9/10/11.ogg` (`officercreate`); `officer_order*.ogg` (17, `officerleftright`) | `Officer Create`, `Officer Selected`, `Darwinian GivenOrders`, owner confirmations of `SetOrderGoto`/`SetOrderFormation`; order loops |
| Squad voice | `squaddie_chatter1..21.ogg` (20, `newsamplegroup1`), `squad_lose_health*.ogg`, `squad_death*.ogg` | `Squadie Create` (shared MonophonicRandom loop), `LoseHealth`, `Die` |
| Evil-tree speech | `data/mwsounds/GDSpeech*.ogg` (17, `gdspeechgroup`) | `Tree CreateEvil` (Dark Forest crate) |
| Announcer stings (crate rewards, captures) | `data/mwsounds/CrateReward_*.ogg`, `Crate_CaptureMyTeam.ogg`, `Crate_CaptureOtherTeam.ogg`, `Crate_LostCapture.ogg`, `Player_Capture_Statue.ogg`, `Enemy_Capture_Statue.ogg`, `KOH_Zone_Captured.ogg`, `KOH_Zone_Lost.ogg`, `SpawnPoint_PlayerTurnsOn2.ogg`, `SpawnPoint_PlayerTurnsOff2.ogg` | `CratePowerup *` (25 live, 4 dead: FireExtinguisher, FormationBooster, Tank, FlameTurret), crate/statue/zone/spawn-point events |
| **Unbound** voice takes | `YesSir1..12` (orphan group `officerselected`), `Aye1..9` (`aye`), `Attention1..4` (`officerabsorb`), `Formation1..2` (`officersetorderformation`), `Marching1..5` (`officersetorderfollow`), `DStrain1..17`, `GameOver1..2`, `YouWon`, `YouLost2`, `CrateReward_OtherTeam`, 25 alternate `CrateReward_*`/`CrateRewrad_*`/`CreateReward_*` takes | none — intended officer order acknowledgements and match-end announcements never wired (the officer order events `SetOrderFollow`/`SetOrderAbsorb` and `GameOver`/`YouWon`/`YouLost` are fired by code but have no data) |

## [→ 10_audio_design] 10.I Statistics

All figures [C] `X2b_summary.txt` / `X2b_samples.csv`.

| Set | Files | Duration | Bytes on disk | Decoded PCM (16-bit mono) |
|---|---|---|---|---|
| All audio | 1332 | 5636.7 s (93.9 min) | 57 467 611 (54.8 MiB) | 473.9 MiB |
| `data/sounds/` | 954 | 4165.7 s | 34 323 117 | — |
| `data/mwsounds/` | 378 | 1471.0 s | 23 144 494 | — |
| referenced by some EVENT block | 1140 | 5234.5 s | 51 688 528 | — |
| of which **live** (some referencing event fired in FULL) | 898 | 2633.1 s (43.9 min) | 32 721 754 (31.2 MiB) | 221.5 MiB |
| of which dead-data (only unreachable / never-fired events) | 242 | 2601.4 s | 18 966 774 | 218.6 MiB |
| unused (no blueprint reference) | 192 (73 `sounds`, 119 `mwsounds`) | 402.2 s | 5 779 083 | 33.8 MiB |
| `menusounds.dat` | 18 | 384.0 s | 4 950 327 | — |
| `sounds.dat` | 1297 | 3933.5 s | 45 360 003 | — |
| `sounds2.dat` | 6 | 1.2 s | 37 063 | — |
| not shipped (music) | 11 | 1702.0 s | 12 070 545 | — |

Format: 1332 Ogg Vorbis I mono; 1330 × 44 100 Hz, 2 × 22 050 Hz. Nominal bitrate: 65 kbps 862 files (all in `data/sounds`), 128 kbps 458, 96 kbps 8, 33/40/60/80 kbps 1 each. Duration distribution: ≤ 0.25 s 117, ≤ 0.5 s 152, ≤ 1 s 236, ≤ 2 s 215, ≤ 5 s 414, ≤ 10 s 131, ≤ 30 s 48, ≤ 60 s 10, > 60 s 9; median 1.686 s, mean 4.232 s; shortest `data/mwsounds/rollover1a.ogg` / `rollover1c.ogg` 0.018 s, longest `tresk_-_excuses.ogg` 344.555 s. Blueprint structure: 68 sources, 296 EVENT blocks, 256 source×event pairs (182 live), 131 SAMPLEGROUP definitions (130 names, 12 never referenced), 1126 SAMPLE lines; largest groups: `TrackLink CogTurn` 43 files (legacy), `Darwinian SeenThreatAttack` 37, `Laser InjuredEntity` 36, `Darwinian SeenThreatRunAway` 36 (2 groups), `Virii ChangeDirection` 29, `Darwinian Attack` 29.

Memory implication [I]: the live set decodes to ≈ 221.5 MiB of PCM against the 60 MiB LRU sample cache (P16 §27.7) — the cache relies on only a fraction of samples being touched per match; a new engine budget should assume streaming or ADPCM/compressed-in-memory for the long beds (`High*`, `MultiLobbyAmb*`, `evil`, `alien_lands3`, `white_noise`).

## [→ 10_audio_design] 10.J Unused samples, dead data and the `tools/` checking scripts

**Unused samples (192)** — on disk and shipped in `sounds.dat` (191) or `sounds2.dat` (`rollover1b.ogg`), but named by no EVENT block directly or through a referenced group [C] `X2b_samples.csv:status=unused`:

- `data/sounds/` (73): `darwin_burn_death.ogg`, `darwin_burn_death7.ogg`, `darwin_on_fire.ogg`, `darwin_on_fire1.ogg`–`darwin_on_fire13.ogg` (13, orphan group `darwinnianonfire`), `engneignite1.ogg`, `flag_wave.ogg`, `flag_wave2.ogg`–`flag_wave12.ogg` (11), `fuelpipe1_loop.ogg`, `fuelpipe3_loop.ogg`, `fuelpump1.ogg`, `fuelpumpdown1.ogg`, `gesturestart.ogg`, `ident1.ogg`, `laser_hit_building.ogg`, `laser_hit_building2.ogg`–`laser_hit_building10.ogg` (9; orphan group `laserhitbuild`), `laser_hitent1.ogg`–`laser_hitent8.ogg` (orphan `laserhitentity`), `laser_hitgnd.ogg`, `laser_hitgnd2.ogg`–`laser_hitgnd9.ogg` (8; orphan `laserhitgnd`, `laser_hitgnd9` also in `laserhitentity`), `mouse.ogg`, `mouse1.ogg`, `mouse4.ogg`, `mouse5.ogg`, `mouse6.ogg`, `squadlasers3.ogg`–`squadlasers8.ogg` (6; orphan `laserricochet`).
- `data/mwsounds/` (119): `Attention1.ogg`–`Attention4.ogg` (orphan `officerabsorb`), `Aye1.ogg`–`Aye9.ogg` (orphan `aye`), `Bleep3.ogg`, `carry1.ogg`–`carry4.ogg`, `CountdownBeep1.ogg`, `CountdownEndBeep.ogg`, `CrateReward_Armour2.ogg`, `CrateReward_CrateMania1.ogg`, `CrateReward_CrateMania2.ogg`, `CrateReward_Egg1.ogg`, `CrateReward_Egg2.ogg`, `CrateReward_Egg3.ogg`, `CrateReward_Extinguisher2.ogg`, `CrateReward_Extinguisher3.ogg`, `CrateReward_Formation1.ogg`, `CrateReward_Harvester1.ogg`, `CrateReward_Harvester3.ogg`, `CrateReward_Meteor2.ogg`, `CrateReward_Meteor3.ogg`, `CrateReward_OtherTeam.ogg`, `CrateReward_Rage1.ogg`, `CrateReward_Rage2.ogg`, `CrateReward_Rage3.ogg`, `CrateReward_SpawnMania2.ogg`, `CrateReward_Squaddies.ogg`, `CrateReward_Squaddies2.ogg`, `CrateReward_Squaddies3.ogg`, `CrateRewrad_Darwinians1.ogg`, `CreateReward_AntsNest1.ogg`, `CreateReward_Harvester1.ogg`, `CreateReward_Harvester2.ogg`, `CreateReward_Harvester3.ogg`, `DroneLoop2.ogg`, `DroneLoop3.ogg`, `Drop_Statue.ogg`, `Drop_Statue2.ogg`, `DStrain1.ogg`–`DStrain17.ogg` (17), `FlameTurretFinal.ogg`, `FlameTurretLoop1.ogg`, `Formation1.ogg`, `Formation2.ogg` (orphan `officersetorderformation`), `GameOver1.ogg`, `GameOver2.ogg`, `Laser_Cock1.ogg`–`Laser_Cock10.ogg` (orphan `lasercock`), `LaserHitDarwinian13.ogg`–`LaserHitDarwinian19.ogg` (7), `Marching1.ogg`–`Marching5.ogg` (orphan `officersetorderfollow`), `PinkNoiseLoop.ogg`, `rollover1b.ogg`, `SpawnPoint_PlayerTurnsOff.ogg`, `SpawnPoint_PlayerTurnsOn.ogg`, `SpawnPointActivate5.ogg`, `SpawnPointDeactivate4.ogg`, `StatueDrop.ogg`, `StatueDrop2.ogg`, `StatueDrop4.ogg`, `TankEngineNear.ogg`, `YesSir1.ogg`–`YesSir12.ogg` (orphan `officerselected`), `YouLost2.ogg`, `YouWon.ogg`.

**Unreferenced sample groups (12)** [C]: `aye`, `darwinnianonfire`, `lasercock`, `laserhitbuild`, `laserhitentity`, `laserhitgnd`, `laserricochet`, `offciersetordergoto` (typo; its members are used elsewhere), `officerabsorb`, `officerselected`, `officersetorderfollow`, `officersetorderformation`.

**Dead data — referenced only by events that never fire in FULL (242 files, 2601.4 s)** [C], by event: `TrackLink CogTurn` 42 (89.3 s); `SpiritProcessor`/`SpiritReceiver Create` 25 (77.0 s); `Ambience EnterGlobalWorld` 19 (216.9 s); `GunTurret Damage` 10 (26.9 s); `ResearchItem Create`/`Spam CreateResearch` 9; `Refinery CogTurn` 8; `Spam Create` 8; `ResearchItem AquireResearch` 7; `Egg Die` 7; `Mine CogTurn` 7; `Spam Attack` 7; `Spam Damage` 7; `Tripod LoseHealth` 7; `Refinery Create` 6; `Incubator SpawnEntity` 6; `Spam Explode` 6; `Generator TriggerSurge` 5; `GodDish Create` 5; `Tripod Attack` 5; `TrunkPort ReprogramComplete` 5; `Tripod Die` 4; `GodDish DisconnectFromGod` 3; `GodDish ConnectToGod` 3; `Tripod Create` 3; and one file each for `Library Create`, `Squadie ThrowAirStrike`, `Engineer EndReprogramming`, `Generator ReprogramComplete`, `Gesture GestureFail`, `Grenade ExplodeController`, `Powerstation Create`, `RadarDish ReprogramComplete`, `Sepulveda Appear`, `Sepulveda Disappear`, `Interface Show`, `Interface Hide`, `Yard Create`, `CratePowerup FireExtinguisher`, `CratePowerup FormationBooster`, `CratePowerup Tank`, `Tank Create`, and the 11 music tracks.

**`tools/unused_samples.sh`** [C]: a Darwinia-era bash helper that must be run from `data/`; it collects every value of lines matching `soundname ` and `sample ` (case-insensitive grep, case-sensitive `sed` strip of `SOUNDNAME`/`SAMPLE`), lower-cases them, lists `data/sounds/` only (`ls -l | awk '{print $9}'`, `.ogg` stripped) and prints `diff --side-by-side` lines containing `<` or `|`. Limits: (1) ignores `data/mwsounds/` entirely (378 files); (2) counts members of **unreferenced** groups as used; (3) `sounds.txt` has CRLF line ends, so on a POSIX shell every extracted name keeps a trailing CR and nothing matches [I]; (4) group names (`SOUNDNAME newsamplegroupN`) are mixed into the list (harmless `>` lines). Emulated correctly (CR stripped) it reports **27** files — the 73 `data/sounds` unused files minus the 46 that sit in orphan groups: `darwin_burn_death`, `darwin_burn_death7`, `darwin_on_fire`, `engneignite1`, `flag_wave`, `flag_wave2`–`flag_wave12` (11), `fuelpipe1_loop`, `fuelpipe3_loop`, `fuelpump1`, `fuelpumpdown1`, `gesturestart`, `ident1`, `mouse`, `mouse1`, `mouse4`, `mouse5`, `mouse6` (column `unused_samples_sh=listed` in `X2b_samples.csv`).
**`tools/unused_sample_groups.sh`** [C]: lists `SAMPLEGROUP` names that never appear as a `SOUNDNAME` value — emulated result = exactly the 12 unreferenced groups above (it cannot see that some of their members are referenced via other groups).

<!-- X2B:NARRATIVE3 -->
