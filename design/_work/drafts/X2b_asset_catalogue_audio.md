# X2b_asset_catalogue_audio — Audio asset catalogue: every sample, every binding, every trigger
Scope: every file in `data/sounds/` (954 `.ogg`) and `data/mwsounds/` (378 `.ogg`) = 1332 files; `data/sounds.txt`; `tools/sounds.txt`; `tools/menu_sounds.txt`; `tools/unused_samples.sh`; `tools/unused_sample_groups.sh`. Supporting (read to resolve packaging/reachability, outside scope): `tools/build_multiwinia_data.bat`, `tools/listfiles.pl`, `tools/multiwinia_exclusions.txt`, `tools/patch.full.list`, `code/sound/sample_cache.cpp`, `code/sound/soundsystem.cpp`, `code/sound/sound_instance.cpp`, `code/lib/resource.cpp`, `contrib/systemiv/lib/tosser/hash_table.cpp`, `contrib/systemiv/lib/filesys/filesys_utils.cpp`, `code/app.cpp`, and every file containing a `Trigger*Event` / `StopAllSounds` call. Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Engine behaviour (voice allocation, ADSR, parameters, DSP, music slot mechanics, options screen) is specified in `drafts/P16_sound.md` (§27.x / §10.1–10.11) and is **not** repeated here; this draft is the per-asset / per-event layer on top of it and corrects P16 where noted.

Generated artefacts (all reproducible: `python3 -I _work/scripts/X2b_catalogue.py <SRC> <OUT/_work> _work/scripts/X2b_event_notes.py <scratch>` then `python3 -I _work/scripts/X2b_assemble.py <this draft> EVENTS=<scratch>/X2b_md_events.md CODEONLY=<scratch>/X2b_md_events_codeonly.md SAMPLES=<scratch>/X2b_md_samples.md` to refresh the three embedded tables):

| File | Rows | Content |
|---|---|---|
| `_work/drafts/X2b_samples.csv` | 1332 | one row per audio file: path, folder, name, channels, rate, duration_s, bytes, nominal_kbps, encoder vendor, Vorbis comment tags, referenced_by (events), via_groups, orphan_groups, status (referenced/unused), event_liveness (live/dead-data), archive, unused_samples.sh verdict, decoded PCM bytes |
| `_work/drafts/X2b_sound_events.csv` | 469 | one row per object-type × event: 256 pairs bound in `data/sounds.txt` + 213 pairs fired by code with no data; blueprint summary of every EVENT block, resolved files, every code trigger site with build/reachability status, fired-in-FULL verdict, type reachability, ListSoundEvents membership, gameplay trigger |
| `_work/drafts/X2b_callsites.csv` | 285 | every `TriggerEntityEvent`/`TriggerBuildingEvent`/`TriggerOtherEvent` call in `code/` (excluding the sound system itself): function, arguments, commented-out flag, `#if` evaluation under the baseline defines, vcproj membership |
| `_work/drafts/X2b_summary.txt` | — | statistics and cross-checks quoted below |
| `_work/scripts/X2b_event_notes.py` | — | hand-verified facts (reachability, dynamic names, descriptions) consumed by the script; parsed with `ast.literal_eval`, never imported |

---

## [→ 10_audio_design] 10.A Method and verification of P16

1. **Ogg headers** [C]: each file is walked page by page (`OggS` capture, segment table). Packet 1 is the Vorbis identification header (`\x01vorbis`: version, channels, rate, bitrate max/nominal/min, block sizes); packet 2 the comment header (vendor string + `TAG=value` list); duration = granule position of the last page with granule ≠ −1, divided by the rate (this equals `ov_pcm_total` for these files: every file has one logical stream, no resync gaps, Vorbis version 0) [C] `_work/drafts/X2b_summary.txt`.
2. **Blueprint parse** [C]: an independent re-implementation of `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints`, `ParseSoundEvent`, `ParseSampleGroup` (tokens split on blanks/tabs, keywords case-insensitive, `SOUNDNAME`/`SAMPLE` values lower-cased and stripped from the last `.` as `contrib/systemiv/lib/filesys/filesys_utils.cpp:RemoveExtension` does). Cross-checked block-by-block against `drafts/P16_sound_events.csv`: 296 = 296 EVENT blocks, **0 mismatches** in source, event, SOUNDNAME, LOOPTYPE, POSITIONTYPE [C].
3. **Code sites** [C]: every `Trigger(Entity|Building|Other)Event(` in `code/**/*.{cpp,h}` (285 sites), with block/line-comment stripping, a baseline preprocessor evaluator (`#if/#ifdef/#ifndef/#elif/#else` over the FULL define set of build_matrix §3–4), vcproj membership from `manifest.csv`, enclosing `Class::Method`. Base-class sites (`Entity::Begin`, `Entity::ChangeHealth`, `Entity::Attack`, `Entity::EnterTeleports`, `Building::Initialise`, `Building::Damage`, `Building::SetTeamId`, `Building::Destroy`, `Building::ReprogramComplete`, `PowerBuilding::TriggerSurge`, `MineBuilding::Advance`, `CarryableBuilding::Advance`, `ReceiverBuilding::TriggerSpirit`) are attributed to each concrete type unless a subclass overrides the method **without** chaining to a base implementation (body search for `X::Method(`); `Entity::Attack` and `Entity::EnterTeleports` are restricted to the types whose code actually calls them (verified callers: `code/worldobject/darwinian.cpp:Darwinian::AdvanceCombat`, `code/unit.cpp:Unit::Attack`, `code/worldobject/lasertrooper.cpp`, `insertion_squad.cpp`, `officer.cpp`, `shaman.cpp`). Runtime-built names are expanded by hand (§10.C notes). Reachability of compiled-in sites (e.g. Darwinia-only interfaces) is hand-verified and recorded in `X2b_event_notes.py:SITE_STATUS`.
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

**Formats** [C]: 1332/1332 Ogg Vorbis I, mono, 44100 Hz except the two 22050 Hz files above. Encoders (comment-header vendor): `Xiph.Org libVorbis I 20040920` 863 files (all in `data/sounds`, nominal 65 kbps except `fodder.ogg` 33 kbps), `Xiph.Org libVorbis I 20020717` 466 (128 kbps: 369 of the 378 `data/mwsounds` files + 89 older `data/sounds` files, 96 kbps 8), `Xiph.Org libVorbis I 20030909` 2, `AO; aoTuV b3 [20041120]` 1 (`data/sounds/pang.ogg`, a loader tune). Vorbis comments: 466 files (exactly the `libVorbis I 20020717` set) carry the bare comment `Sony Ogg Vorbis 1.0 Final` (an authoring-tool marker [I]); 27 of them (`darwin_burn_death*`, `darwin_on_fire*`, `fuel_load_passenger*`) carry `DATE=2005-09-05/06` and an `ENGINEER=` personal name (value redacted in `X2b_samples.csv`); no artist/title tags anywhere [C] `X2b_samples.csv:tags`.

### Packaging (`tools/build_multiwinia_data.bat`, PC default platform) [C]
| Archive | Built from | Files (from this tree) | Note |
|---|---|---|---|
| `menusounds.dat` | `tools/menu_sounds.txt` (18 paths, prefixed `data/`), `rar -m2` | 18 (`multilobbyamb1..10`, `shipheartbeat1`, `mouse2`, `mouse3`, `menuscroll1`, `menuscroll2`, `alien_lands3`, `evil`, `rumble1`) = exactly the samples of `MultiwiniaInterface LobbyAmbience`, `MenuRollOver`, `MenuSelect`, `MenuCancel` | parsed in the App constructor, i.e. before the background sound load |
| `sounds.dat` | `tools/sounds.txt` (1297 paths: 935 `sounds/`, 362 `mwsounds/`), `rar -m2` | 1297; all exist on disk | includes 191 of the 192 unused samples |
| `sounds2.dat` | every `.ogg` of `perl tools/listfiles.pl data completeExclusions` (`completeExclusions` = `tools/sounds.txt` + `tools/menu_sounds.txt` + `tools/multiwinia_exclusions.txt`) | 6: `data/mwsounds/beep_harsh_malfunction.ogg`, `chat.ogg`, `mouseclick1.ogg`, `rollover1a.ogg`, `rollover1b.ogg`, `rollover1c.ogg` (late Multiwinia UI additions never added to `tools/sounds.txt`) | |
| `main.dat` | the same list minus every path containing `sounds/`, then only `*.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim` | 0 audio; carries `data/sounds.txt` and `data/effects.txt` | |
| not shipped | `tools/multiwinia_exclusions.txt`: `sounds/tresk*`, `sounds/trash80*`, `sounds/dma_sc*`, `sounds/twoataris*`, `sounds/pang*`, `sounds/speccyloader.ogg`, `sounds/matrix.ogg`, `sounds/fodder.ogg` | 11 music files (1702.0 s, 11.5 MiB) | see §10.F |

Thus `tools/sounds.txt` and `tools/menu_sounds.txt` are simultaneously **archive manifests** and **exclusion lists** (they keep those files out of `main.dat`/`sounds2.dat`). `tools/patch.full.list` (a Darwinia patch list naming `sounds.txt`, `sounds/pang.ogg` (twice), `sounds/TaskIn.ogg`, `TaskOut.ogg`, `Mouse.ogg`, `Mouse1.ogg`–`Mouse6.ogg`, `SolarCharge.ogg`, `MenuScroll1.ogg`, `MenuScroll2.ogg`, `Deselect1.ogg`, `rocket_boom1.ogg`–`rocket_boom4.ogg` plus icons, two Darwinia level files and two textures) is referenced by no Multiwinia build script [C] (grep of `tools/`).

Design consequences: (1) a blueprint that names a sample absent from every archive is a **crash**, not silence (steps 2/4) — the only such references are the 11 excluded music files, whose events are unreachable (§10.F); (2) `menusounds.dat` is loaded early but every `Trigger*Event` returns while `App::m_soundsLoaded` is false, and that flag is set only after `sounds.dat` + `sounds2.dat` are parsed (`soundsystem.cpp:NotReadyForGameSoundsYet`), so the early front-end archive buys nothing audible in this code version [I] (likely a remnant of an earlier "play lobby ambience during load" path — the commented-out `TriggerOtherEvent(…"LobbyAmbience"…)` in `code/app.cpp:App::CheckSounds`). A new engine should keep "UI/front-end bank loaded first, game bank streamed later" but actually allow UI events from the first bank.

## [→ 10_audio_design] 10.C Complete event table — every object type × event bound in `data/sounds.txt`

256 (source, event) pairs from 296 EVENT blocks. Column legend: *Data blocks* — one line per EVENT block: `[Sample|Group name]` (SOURCETYPE), position (`2D`, `Stat` = 3D stationary, `Att` = 3D attached, `Ed` = in-editor) / loop (`Once`, `Loop`, `LoopADSR`) / instance (`MonoRandom`, `MonoNearest`; polyphonic omitted) / team filter (`MyTeam`, `Enemy`; all omitted); `md` MINDISTANCE (wu); `v` VOLUME (0–10 log scale), `f` FREQUENCY multiplier, `a/s/r/ld` ATTACK/SUSTAIN/RELEASE/LOOPDELAY (s); parameter forms: number = fixed, `rnd lo..hi` = ranged random, `Link inLo..inHi->outLo..outHi` = linked, `sN` = smoothing, `/once` = UpdateOncePerLoop; `fx` = DSP chain (off in shipped config, P16 §10.7). *Files* = distinct files the event can play. *Fired in FULL*: `yes` = a live, reachable call site exists in the baseline and the object type exists in Multiwinia; `site-live/type-LEGACY|DEBUG` = the code path is live but the object type never appears in a Multiwinia match; `no(…)` = all sites are LEGACY / not-compiled / commented-out / absent. Site status tags: `[LEGACY(reason)]`, `[not-compiled(#if X)]`, `[commented-out]`, `[DEAD(reason)]`, no tag = live in FULL. All rows [C] from `data/sounds.txt` + the cited code unless the trigger text says [I].

Runtime-built event names (expanded in the table) [C]:
- `Crate Capturing_<n>pc`: `sprintf("Capturing_%dpc", postIntCaptured*20)` with `postIntCaptured = int(5·captureTimer/CRATE_CAPTURE_TIME)` ∈ {1..5} while the owner's majority holds → `Capturing_20pc`, `Capturing_40pc`, `Capturing_60pc`, `Capturing_80pc` (data) and `Capturing_100pc` (no data). The **decay** branch (owner no longer the majority, timer falling) formats `"Capturing_%pc"` — `%p` is the pointer conversion, so the event becomes a hex string such as `Capturing_00000003c` under MSVC [I: exact digits] and never matches: progress tones are silent while a crate's capture timer is falling (bug, don't copy) `code/worldobject/crate.cpp:Crate::AdvanceOnGround`.
- `EscapeRocket <state sound>`: `EscapeRocket::SetupSounds` picks `Refueling` (no data), `Unhappy`, `Malfunction`, `Happy` or `Flight` from state/damage/fuel and keeps exactly one instance alive by polling `NumInstances(m_id, "EscapeRocket <name>")` every sim tick (stopping all five otherwise) `code/worldobject/rocket.cpp`.
- `Grenade|AirStrikeBomb <Create|Bounce|Flash|Explode>`: `ThrowableWeapon::TriggerSoundEvent` routes `EffectThrowableGrenade`/`AirstrikeMarker`/`ControllerGrenade` to `OTHER Grenade` and `EffectThrowableAirstrikeBomb`/`EffectFlameGrenade` to `OTHER AirStrikeBomb` `code/worldobject/weapons.cpp`. Bomber bombs are created without `Initialise()` (no Create) `code/worldobject/airstrike.cpp:SpaceInvader::Advance`.
- `CratePowerup <Crate::GetName(type)>`: `Crate::TriggerSoundEvent(type, pos, team)` → `TriggerOtherEvent(pos, name, TypeCratePowerup, team)`; callers listed per row; `RunGunTurret` always passes `CrateRewardGunTurret`, so the `FlameTurret` block is dead; `Engineer`, `RocketTurret`, `NapalmStrike` (uses `Airstrike`), `SlowDown`, `SpeedUp` names are never passed (Engineer plays `Gesture GestureSuccess` instead) `code/worldobject/crate.cpp`, `code/taskmanager.cpp`.
- `Music <name>`: `Script::RunCommand_TriggerSound(name)` — script names found in `data/scripts/*.txt` are `cutscene1`–`cutscene5`, `endsequence`, `Demo2Intro`, `Demo2Mid` (case-insensitive match).

<!-- X2B:EVENTS:BEGIN -->
| # | Source | Event | Data blocks: [Sample\|Group name] pos/loop/inst/team md=min-distance v=volume f=pitch a/s/r/ld=ADSR,loop-delay fx | Files | Fired in FULL | Gameplay trigger | Code trigger site(s) |
|---|---|---|---|---|---|---|---|
| 1 | ENTITY LaserTroop | Attack | [Group 0lasers] Stat/Once md100.00 v=8.10 f=rnd 0.93..1.03 | 24 | site-live/type-LEGACY | Laser trooper fires (Entity::Attack via Unit::Attack). Type unreachable in MW. | `code/worldobject/entity.cpp:Entity::Attack:L225` |
| 2 | ENTITY LaserTroop | Die | [Group darwiniandie] Stat/Once md100.00 v=8.50 f=rnd 0.83..1.09/once | 8 | site-live/type-LEGACY | Laser trooper killed (Entity::ChangeHealth, health reaches 0). Type unreachable in MW. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 3 | ENTITY Engineer | Create | [Sample engineer_hover] Att/Loop md100.00 v=9.03 | 1 | yes | Engineer spawned (Entity::Begin); looping hover hum that follows it until it is removed. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 4 | ENTITY Engineer | BeginReprogramming | [Sample engineer_reprog_radar_loop] Att/Loop md100.00 v=9.00 | 1 | no(LEGACY) | Engineer arrives at a ControlTower / ResearchItem and starts reprogramming (loop, stopped by StopAllSounds "Engineer BeginReprogramming"). Unreachable in MW (no towers/research items). | `code/worldobject/engineer.cpp:Engineer::AdvanceToControlTower:L814` [LEGACY(needs ControlTower)]<br>`code/worldobject/engineer.cpp:Engineer::AdvanceToResearchItem:L1127` [LEGACY(needs ResearchItem)] |
| 5 | ENTITY Engineer | EndReprogramming | [Sample engineer_repog_radar_loop_end] Stat/Once md100.00 v=6.48 | 1 | no(LEGACY,commented-out) | Engineer leaves/finishes/loses its ControlTower (Engineer::Advance, AdvanceReprogramming). Unreachable in MW. | `code/worldobject/engineer.cpp:Engineer::Advance:L376` [LEGACY(needs ControlTower)]<br>`code/worldobject/engineer.cpp:Engineer::Advance:L384` [LEGACY(needs ControlTower)]<br>`code/worldobject/engineer.cpp:Engineer::Advance:L448` [LEGACY(needs ControlTower)]<br>`code/worldobject/engineer.cpp:Engineer::Advance:L466` [commented-out]<br>`code/worldobject/engineer.cpp:Engineer::AdvanceReprogramming:L934` [LEGACY(needs ControlTower)] |
| 6 | ENTITY Engineer | LoseHealth | [Group engineerinjury] Stat/Once md200.00 v=10.00 f=rnd 1.58..1.89/once r=5.03 | 9 | yes | Engineer takes non-laser damage (Engineer::ChangeHealth ignores DamageTypeLaser). | `code/worldobject/engineer.cpp:Engineer::ChangeHealth:L295` |
| 7 | ENTITY Engineer | Die | [Group engineerdie] Stat/Once md200.00 v=10.00 f=rnd 0.95..1.05/once | 5 | yes | Engineer health drops below 0 (Engineer::ChangeHealth). | `code/worldobject/engineer.cpp:Engineer::ChangeHealth:L302` |
| 8 | ENTITY Virii | Attack | [Group virusattack] Att/Once md200.00 v=9.03 f=rnd 0.91..1.12/once | 8 | yes | A Virii reaches its target entity and bites it for 20 damage (Virii::AdvanceAttacking). | `code/worldobject/virii.cpp:Virii::AdvanceAttacking:L447` |
| 9 | ENTITY Virii | LoseHealth | [Group viruspain] Stat/Once md200.00 v=10.00 f=rnd 0.81..1.01/once r=2.71 fx=Echo | 3 | yes | Virii damaged (Entity::ChangeHealth, amount < 0). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 10 | ENTITY Virii | ChangeDirection | [Group newsamplegroup11] Att/Once md50.00 v=6.55 f=rnd 1.70..2.17/once | 29 | yes | Idle Virii picks a new wander direction (Virii::SearchForIdleDirection) - frequent ambient chirp per worm. | `code/worldobject/virii.cpp:Virii::SearchForIdleDirection:L741`<br>`code/worldobject/virii.cpp:Virii::SearchForIdleDirection:L765` |
| 11 | ENTITY Virii | Die | [Group newsamplegroup9] Att/Once md175.00 v=10.00 f=rnd 1.54..1.79/once r=10.06/once fx=Echo | 7 | yes | Virii killed (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 12 | ENTITY Squadie | Create | [Group newsamplegroup1] Att/Loop/MonoRandom md100.00 v=9.00/once r=5.23/once ld=Velocity 6.80..0.00->0.00..2.90/once fx=Echo | 20 | yes | Squad member spawned (Entity::Begin via Squadie::Begin): one shared looping chatter voice for all squadies (MonophonicRandom). | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 13 | ENTITY Squadie | LoseHealth | [Group newsamplegroup5] Att/Once md100.00 v=9.00 f=0.91/once r=4.06/once fx=Echo | 4 | yes | Squadie damaged (Squadie::ChangeHealth). | `code/worldobject/insertion_squad.cpp:Squadie::ChangeHealth:L625` |
| 14 | ENTITY Squadie | WeaponReturns | [Sample grenade_return3] Att/Once/MonoRandom md100.00 v=9.00/once | 1 | yes | Secondary-weapon (grenade) reload timer expires (Squadie::Advance, m_secondaryTimer <= 0); one shared voice (MonophonicRandom). | `code/worldobject/insertion_squad.cpp:Squadie::Advance:L663` |
| 15 | ENTITY Squadie | ThrowAirStrike | [Sample airstrike_alarm] Att/Once md100.00 v=10.00 | 1 | no(LEGACY) | Squadie throws an air-strike marker as secondary weapon (Squadie::FireSecondaryWeapon). Unreachable in MW (secondary is always Grenade; ThrowGrenade has no data). | `code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L987` [LEGACY(squad secondary is always Grenade in MW)] |
| 16 | ENTITY Squadie | FireLaser | [Group turretfire] Att/Once md100.00 v=8.77/once f=rnd 0.90..1.05/once | 6 | yes | Every squadie laser shot (Squadie::Attack, alongside the silent "Attack"). | `code/worldobject/insertion_squad.cpp:Squadie::Attack:L954` |
| 17 | ENTITY Squadie | Die | [Group squaddiedeath] Att/Once md100.00 v=8.97 f=rnd 0.76..1.05/once | 5 | yes | Squadie killed (Squadie::ChangeHealth). | `code/worldobject/insertion_squad.cpp:Squadie::ChangeHealth:L632` |
| 18 | ENTITY Squadie | EnterTeleport | [Group enterteleport] Stat/Once md100.00 v=9.00 | 6 | yes | Squadie enters a RadarDish/Bridge teleport (Entity::EnterTeleports from Squadie::Advance). | `code/worldobject/entity.cpp:Entity::EnterTeleports:L278`<br>`code/worldobject/entity.cpp:Entity::EnterTeleports:L295` |
| 19 | ENTITY Squadie | ExitTeleport | [Group exitteleport] Att/Once md100.00 v=9.10 | 6 | yes | Squadie arrives at the far RadarDish (RadarDish::UpdateEntityInTransit fires on any exiting entity). | `code/worldobject/radardish.cpp:RadarDish::UpdateEntityInTransit:L705` |
| 20 | ENTITY Egg | Die | [Group eggsplat] Stat/Once md100.00 v=3.90 | 7 | no(no-site) | No code fires Egg "Die" (Egg::ChangeHealth overrides without a sound) - dead data. | — |
| 21 | ENTITY SporeGenerator | Create | [Group sporegengroan] Att/Loop md100.00 v=8.32 ld=HeightAboveGround 4.00..164.00->0.19..5.81 | 3 | yes | Spore generator spawned (Entity::Begin); looping body hum. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 22 | ENTITY SporeGenerator | LoseHealth | [Group sporegenlosehealt] Att/Once md150.00 v=10.00 | 6 | yes | Spore generator damaged (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 23 | ENTITY SporeGenerator | Die | [Group sporegeneratordie] Stat/Once md150.00 v=10.00/once f=rnd 0.86..1.10/once r=10.45 fx=Echo | 6 | yes | Spore generator killed (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 24 | ENTITY SporeGenerator | LayEgg | [Group spideregglay] Att/Once md100.00 v=9.03 | 10 | yes | Spore generator lays a Virii Egg (SporeGenerator::AdvanceEggLaying). | `code/worldobject/sporegenerator.cpp:SporeGenerator::AdvanceEggLaying:L301` |
| 25 | ENTITY Tripod | Create | [Group tripodcreate] Att/Once md100.00 v=8.50 f=rnd 0.84..1.14/once | 3 | site-live/type-LEGACY | Tripod spawned. Type unreachable in MW. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 26 | ENTITY Tripod | Attack | [Group tprdattac] Att/Once md100.00 v=8.50 f=rnd 0.98..1.15/once | 5 | no(no-site) | Mapped to Entity::Attack but Tripod fires its own lasers without calling it - effectively silent; type unreachable. | — |
| 27 | ENTITY Tripod | LoseHealth | [Group trpodlosehealth] Att/Once md100.00 v=8.50 f=rnd 0.68..1.05/once | 7 | site-live/type-LEGACY | Tripod damaged. Type unreachable. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 28 | ENTITY Tripod | Die | [Group ttripoddie] Att/Once md100.00 v=8.50 f=rnd 0.81..1.02/once | 4 | site-live/type-LEGACY | Tripod killed. Type unreachable. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 29 | ENTITY Centipede | Attack | [Group newsamplegroup25] Att/Once md100.00 v=10.00 f=rnd 0.88..1.08/once | 8 | yes | A centipede segment touches an enemy within its radius (Centipede::Attack, per enemy hit each tick). | `code/worldobject/centipede.cpp:Centipede::Attack:L391` |
| 30 | ENTITY Centipede | Create | [Group newsamplegroup26] Att/LoopADSR md80.09 v=8.00 f=rnd 0.95..1.05/once | 4 | yes | Each centipede segment spawned (Entity::Begin, incl. growth segments); LoopADSR crawl texture per segment. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 31 | ENTITY Centipede | Die | [Group newsamplegroup27] Stat/Once md150.00 v=10.00 f=0.94 r=10.00 fx=Echo | 4 | yes | Centipede segment killed (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 32 | ENTITY Centipede | LoseHealth | [Group newsamplegroup28] Att/Once md100.00 v=9.03 f=rnd 0.58..1.59 s0.56/once ld=Velocity 11.20..0.00->0.58..4.84/once | 5 | yes | Centipede segment damaged. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 33 | ENTITY SpaceInvader | DropGrenade | [Sample airstrike_drop] Att/Once md200.00 v=10.00 | 1 | yes | Air-strike bomber releases a bomb when within 90 wu of the target (SpaceInvader::Advance). | `code/worldobject/airstrike.cpp:SpaceInvader::Advance:L257` |
| 34 | ENTITY SpaceInvader | Create | [Sample airstrike_hover2] Att/Loop md200.00 v=10.00 f=Velocity 13.20..74.40->0.91..1.96 | 1 | yes | Air-strike bomber spawned (Entity::Begin); looping engine. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 35 | ENTITY Spider | Create | [Group spidercreate] Att/Loop md100.00 v=8.93 f=rnd 0.77..1.21/once ld=Velocity 4.80..0.00->0.00..3.10 | 17 | yes | Spider spawned (Entity::Begin); looping body sound. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 36 | ENTITY Spider | Die | [Group newsamplegroup13] Att/Once md200.00 v=10.00 f=Velocity 1.20..34.40->0.58..0.82 r=16.65 fx=DSound_Echo | 7 | yes | Spider killed (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 37 | ENTITY Spider | Attack | [Group spiderpounce] Att/Once md100.00 v=9.03 f=rnd 0.74..1.03/once | 6 | yes | Spider starts a pounce at an enemy (Spider::AdvanceAttack; fired together with Pounce). | `code/worldobject/spider.cpp:Spider::AdvanceAttack:L493` |
| 38 | ENTITY Spider | Pounce | [Group spiderpounce] Att/Once md100.00 v=9.03 f=rnd 0.89..1.22/once | 6 | yes | Spider leaps (Spider::AdvanceAttack). | `code/worldobject/spider.cpp:Spider::AdvanceAttack:L494` |
| 39 | ENTITY Spider | LayEgg | [Group spideregglay] Att/Once md100.00 v=9.03 | 10 | yes | Spider lays a Virii Egg next to a spirit (Spider::AdvanceEggLaying). | `code/worldobject/spider.cpp:Spider::AdvanceEggLaying:L708` |
| 40 | ENTITY Spider | LoseHealth | [Group spiderdeath] Att/Once md100.00 v=9.00 | 2 | yes | Spider damaged (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 41 | ENTITY Spider | FootFall | [Group spiderfeet] Att/Once md62.63 v=Velocity 0.00..27.20->4.71..8.06 | 13 | yes | Each time one of the 8 legs plants its foot (Spider::UpdateLegs). | `code/worldobject/spider.cpp:Spider::UpdateLegs:L298` |
| 42 | ENTITY Darwinian | Die | [Group darwiniandie] Stat/Once md100.00 v=10.00 r=10.00 \|\| [Group laserhitdarwinian] Stat/Once md100.00 v=10.00 | 20 | yes | Darwinian killed (Entity::ChangeHealth, health <= 0); two layered blocks (body + voice). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 43 | ENTITY Darwinian | LoseHealth | [Group darwinianlosehealth] Att/Once md100.00 v=8.00 f=1.06 r=10.00 fx=Echo \|\| [Group darwinianlosehealth] Att/Once md100.00 v=8.00 f=0.91 | 18 | yes | Darwinian damaged (Entity::ChangeHealth); two layered blocks. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 44 | ENTITY Darwinian | Create | [Group darwiniancreate] Att/Once/MonoNearest md100.00 v=8.50 f=rnd 0.58..1.59 s0.56/once r=1.16 ld=rnd 0.87..4.84/once fx=Echo | 15 | yes | Darwinian spawned (Darwinian::Begin -> Entity::Begin): one shared voice for all Darwinians, emitted from the one nearest the camera (MonophonicNearest). | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 45 | ENTITY Darwinian | GivenOrders | [Group officercreate] Att/Once md100.00 v=8.00 f=rnd 1.51..1.85/once | 6 | yes | Darwinian picks up an Officer's goto order and starts following it (Darwinian::SearchForOfficers). | `code/worldobject/darwinian.cpp:Darwinian::SearchForOfficers:L2441`<br>`code/worldobject/darwinian.cpp:Darwinian::SearchForOfficers:L2464` [commented-out]<br>`code/worldobject/darwinian.cpp:Darwinian::GiveOrders:L2623` [commented-out] |
| 46 | ENTITY Darwinian | SeenThreatRunAway | [Group darwinrunaway] Att/Once md100.00 v=9.00/once f=rnd 0.90..1.09/once \|\| [Group combatvoxretreat] Att/Once md100.00 v=9.00 | 36 | yes | Darwinian becomes scared: grenade/air-strike marker nearby, burning tree within 2x burn range, or scared by enemies (Darwinian::SearchForThreats; once per new threat). | `code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L2764`<br>`code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L2793`<br>`code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L3008` |
| 47 | ENTITY Darwinian | Attack | [Group 0lasers] Att/Once md85.90 v=9.00 \|\| [Group laserreload] Att/Once/MonoNearest md60.34 v=9.10 ld=5.03 | 29 | yes | Darwinian fires a laser (Entity::Attack from Darwinian::AdvanceCombat) or a subversion/futurewinian bolt (Darwinian::FireSubversion); 2 blocks, one MonophonicNearest. | `code/worldobject/darwinian.cpp:Darwinian::FireSubversion:L5004`<br>`code/worldobject/entity.cpp:Entity::Attack:L225` |
| 48 | ENTITY Darwinian | VictoryJump | [Group victoryjum] Att/Once md10.12 v=8.00 f=rnd 0.96..1.21/once | 9 | yes | Team-0 idle Darwinian above y=10 jumps when all level primary objectives are true (Darwinian::BeginVictoryDance, 1-in-5 per retarget). MW maps use NeverTrue objectives; 10 maps define none, where the empty list evaluates as victory [I]. | `code/worldobject/darwinian.cpp:Darwinian::BeginVictoryDance:L3243` |
| 49 | ENTITY Darwinian | OnFire | [Group darwinnianburncry] Stat/Once md101.16 v=9.35 ld=0.58 | 13 | yes | Burning Darwinian screams: 1-in-30 chance per sim tick while on fire (Darwinian::AdvanceOnFire). | `code/worldobject/darwinian.cpp:Darwinian::AdvanceOnFire:L3591`<br>`code/worldobject/darwinian.cpp:Darwinian::AdvanceSacraficing:L3668` [LEGACY(sacrifice = Darwinia)] |
| 50 | ENTITY Darwinian | Selected | [Group dselect] Att/Once md100.00 v=8.00 f=rnd 0.92..1.25/once | 10 | yes | Local player selects Darwinians (Location::SelectDarwinians, myTeam only) or a Darwinian entity (Team::SelectUnit). | `code/location.cpp:Location::SelectDarwinians:L3615`<br>`code/team.cpp:Team::SelectUnit:L215` |
| 51 | ENTITY Darwinian | GivenOrdersFromPlayer | [Group dselect] Att/Once md100.00 v=8.03 f=1.03 | 10 | yes | Player issues a direct move order to selected Darwinians (Location::IssueDarwinianOrders) and, while carrying a statue/building, alternating with LiftBuilding (Darwinian::AdvanceCarryingBuilding). | `code/location.cpp:Location::IssueDarwinianOrders:L3733`<br>`code/worldobject/darwinian.cpp:Darwinian::AdvanceCarryingBuilding:L843` |
| 52 | ENTITY Darwinian | LiftBuilding | [Group carrystatue] Att/Once md100.00 v=8.00 f=rnd 0.93..1.21/once | 4 | yes | Lifting chant while carrying a CarryableBuilding (statue): every soundFrequency = 15 + 20*(1 - lifters/maxLifters) sequence ids (Darwinian::AdvanceCarryingBuilding). | `code/worldobject/darwinian.cpp:Darwinian::AdvanceCarryingBuilding:L839` |
| 53 | ENTITY Darwinian | SeenThreatAttack | [Group combatvoxattack] Att/Once md100.00 v=9.00 | 37 | yes | Darwinian sees an enemy and is not scared -> battle cry, once per threat (Darwinian::SearchForThreats). | `code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L3016` |
| 54 | ENTITY Darwinian | SeenThreatAttackHorn | [Group darwinseethreatattack] Att/Once md125.13 v=9.00 | 3 | yes | Random war horn: frand(200) < 1, not already shouting, and >= 4 s game time since the last horn of any Darwinian (Darwinian::SearchForThreats). | `code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L3026` |
| 55 | ENTITY Officer | Create | [Group officercreate] Att/Once md200.00 v=8.00 \|\| [Group officercreate] 2D/Once/MyTeam md100.00 v=8.58 | 6 | yes | Officer created by promotion (Officer::Begin -> Entity::Begin): 2D voice for the owner (MyTeamOnly) + attached sound for everybody. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 56 | ENTITY Officer | LoseHealth | [Group darwinianlosehealth] Att/Once md100.03 v=9.71 | 18 | yes | Officer damaged (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 57 | ENTITY Officer | Die | [Group darwiniandie] Att/Once md200.00 v=9.03 | 8 | yes | Officer killed (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 58 | ENTITY Officer | Attack | [Group darwinseethreatattack] Att/Once md200.00 v=9.03 | 3 | no(no-site) | Officers never call Entity::Attack - effectively silent (dead data). | — |
| 59 | ENTITY Officer | EnterTeleport | [Group enterteleport] Att/Once md200.00 v=9.16 | 6 | yes | Officer walks into a RadarDish/Bridge (Entity::EnterTeleports from Officer). | `code/worldobject/entity.cpp:Entity::EnterTeleports:L278`<br>`code/worldobject/entity.cpp:Entity::EnterTeleports:L295` |
| 60 | ENTITY Officer | SetOrderGoto | [Group officerleftright] Att/Loop md100.00 v=8.03 \|\| [Group officercreate] Att/Once/MyTeam md173.77 v=9.13 | 23 | yes | Officer given a goto order (Officer::SetOrders) or cycled to Goto mode (SetNextMode/SetPreviousMode via network toggle): looping order hum + owner-only confirmation. | `code/worldobject/officer.cpp:Officer::SetOrders:L1497`<br>`code/worldobject/officer.cpp:Officer::SetNextMode:L1712`<br>`code/worldobject/officer.cpp:Officer::SetPreviousMode:L1875` |
| 61 | ENTITY Officer | Selected | [Group officercreate] 2D/Once/MyTeam md100.00 v=8.06 | 6 | yes | Local player selects an Officer (Team::SelectUnit, owner only - 2D/MyTeamOnly). | `code/team.cpp:Team::SelectUnit:L215` |
| 62 | ENTITY Officer | SetOrderFormation | [Group officerleftright] Att/Loop md100.00 v=8.06 \|\| [Group officercreate] Att/Once/MyTeam md173.77 v=9.13 | 23 | yes | Officer switched to formation mode (Officer::SetFormation): looping hum + owner-only confirmation. | `code/worldobject/officer.cpp:Officer::SetFormation:L1578` |
| 63 | ENTITY ArmyAnt | Attack | [Group antatack] Att/Once md100.00 v=8.50 f=rnd 0.81..1.38/once | 6 | yes | Army ant reaches an enemy and bites it (ArmyAnt::AdvanceAttackEnemy). | `code/worldobject/armyant.cpp:ArmyAnt::AdvanceAttackEnemy:L329` |
| 64 | ENTITY ArmyAnt | Create | [Group antcreat] Att/Loop md61.12 v=rnd 5.52..8.00 s0.60 f=rnd 0.67..1.34 s0.45 ld=0.10 | 8 | yes | Army ant spawned (Entity::Begin); looping skitter. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 65 | ENTITY ArmyAnt | LoseHealth | [Group antlosehealth] Att/Once md100.00 v=8.03 f=rnd 0.85..1.23/once | 7 | yes | Army ant damaged. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 66 | ENTITY ArmyAnt | Die | [Group antdeath] Att/Once md100.00 v=8.06 f=rnd 0.70..1.28/once | 6 | yes | Army ant killed. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 67 | ENTITY Armour | Create | [Sample tank_hover] Att/Loop md218.81 v=Velocity 0.00..2.40->9.55..10.00 f=Velocity 0.00..22.40->1.10..1.68 fx=Gargle \|\| [Group tankcreate] Att/Once md100.00 v=9.35 | 7 | yes | Armour spawned (Entity::Begin): start-up one-shot + looping engine, volume 9.55 -> 10 over speed 0 -> 2.4 and pitch 1.10 -> 1.68 over speed 0 -> 22.4 wu/s. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 68 | ENTITY Armour | LoseHealth | [Group tankloseheath] Att/Once md141.42 v=10.00 f=rnd 0.88..1.05/once | 10 | yes | Armour damaged (Armour::ChangeHealth). | `code/worldobject/armour.cpp:Armour::ChangeHealth:L178` |
| 69 | ENTITY Armour | Die | [Group tankdeath] Stat/Once md222.43 v=10.00/once f=rnd 0.89..1.09/once | 9 | yes | Armour destroyed (Armour::ChangeHealth, new health 0). | `code/worldobject/armour.cpp:Armour::ChangeHealth:L214` |
| 70 | ENTITY Armour | SetOrders | [Group officerordergoto] Att/Once md162.69 v=9.03 | 4 | yes | Player orders an Armour to move/deploy (LocationInput::AdvanceTeamControl). | `code/location.cpp:Location::UpdateTeam:L3940` [commented-out]<br>`code/location_input.cpp:LocationInput::AdvanceTeamControl:L1032` |
| 71 | ENTITY SoulDestroyer | Create | [Group souldestroycreate] Att/Loop md100.00 v=9.71/once f=HeightAboveGround 0.00..300.00->1.01..1.65 a=0.19/once r=0.39/once \|\| [Group souldestpain] Att/Once md204.25 v=10.00 | 21 | yes | Soul destroyer spawned: one-shot + looping body sound. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 72 | ENTITY SoulDestroyer | LoseHealth | [Group souldestpain] Att/Once md200.00 v=10.00 | 10 | yes | Soul destroyer damaged. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L182` |
| 73 | ENTITY SoulDestroyer | Die | [Group souldestdeath] Att/Once md200.00 v=10.00 f=rnd 0.78..1.05/once | 10 | yes | Soul destroyer killed. | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 74 | ENTITY SoulDestroyer | Attack | [Group soulattack] Att/Once md200.00 v=10.00 f=rnd 0.78..1.01/once | 7 | yes | Soul destroyer passes through an enemy within damage range (SoulDestroyer::Attack, per victim). | `code/worldobject/souldestroyer.cpp:SoulDestroyer::Attack:L249` |
| 75 | ENTITY SoulDestroyer | EnemySighted | [Group soulenemysighted] Att/Loop md100.00 v=HeightAboveGround 812.00..0.00->0.00..8.68 f=HeightAboveGround 72.00..0.00->0.37..0.76 s0.69 a=0.58/once r=10.45 | 9 | yes | Soul destroyer acquires a target (SoulDestroyer::SearchForTargetEnemy); looping. | `code/worldobject/souldestroyer.cpp:SoulDestroyer::SearchForTargetEnemy:L364` |
| 76 | ENTITY TriffidEgg | Create | [Group triffidegglaunc] Att/Once md100.00 v=9.10 f=rnd 0.89..1.19/once | 5 | yes | Triffid egg launched/spawned (Entity::Begin). | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 77 | ENTITY TriffidEgg | Bounce | [Group triffideggbounce] Stat/Once md100.00 v=Velocity 0.00..7.60->0.16..9.84 f=Velocity 0.00..8.80->0.60..1.11/once | 5 | yes | Egg hits the ground with force > 0.1 (TriffidEgg::Advance). | `code/worldobject/triffid.cpp:TriffidEgg::Advance:L811` |
| 78 | ENTITY TriffidEgg | BurstOpen | [Group triffeggbursts] Stat/Once md100.00 v=9.55 f=rnd 0.91..0.99/once | 10 | yes | Egg timer expires and it spawns its creatures (TriffidEgg::Advance). | `code/worldobject/triffid.cpp:TriffidEgg::Advance:L834` |
| 79 | ENTITY TriffidEgg | Die | [Group souldestdeath] Stat/Once md100.00 v=10.00 | 10 | yes | Egg destroyed by damage (Entity::ChangeHealth). | `code/worldobject/entity.cpp:Entity::ChangeHealth:L189` |
| 80 | ENTITY Harvester | Create | [Sample ufo_engine2] Att/Loop md96.11 v=8.00 f=HeightAboveGround 0.00..984.00->0.32..2.47 \|\| [Sample ufo_engine1] Att/Loop md89.81 v=Velocity 3.60..1.60->8.06..8.90 \|\| [Sample rumble2_loop] Att/Loop md150.84 v=9.00 f=1.29 | 3 | yes | Harvester spawned: three layered loops - engine pitched 0.32 -> 2.47 by height above ground 0 -> 984, engine whose volume follows speed, low rumble at pitch 1.29. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 81 | ENTITY Harvester | DropSpirit | [Group incubatoraddspirit] Stat/Once md150.00 v=9.00 f=rnd 0.38..0.52/once r=15.10 fx=Gargle | 6 | yes | Harvester delivers a spirit to a spawn point (Harvester::AdvanceToSpawnPoint). | `code/worldobject/harvester.cpp:Harvester::AdvanceToSpawnPoint:L875` |
| 82 | ENTITY Nuke | Create | [Sample rocketthrustloop] Att/Loop md104.86 v=9.00 f=CameraDistance 100.00..60.00->0.41..1.00 | 1 | yes | Nuke spawned; looping flight roar. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 83 | ENTITY Nuke | Explode | [Sample nukedetonatenear] Stat/Once md217.86 v=10.00 f=rnd 0.45..0.99/once \|\| [Sample nukedetonatefar] Stat/Once md338.76 v=9.77 f=rnd 0.45..0.74/once a=rnd 0.19..2.13/once | 2 | yes | Nuke reaches its target and detonates (Nuke::AdvanceToTarget): stationary near (md 217.86) and far (md 338.76) detonation layers. | `code/worldobject/nuke.cpp:Nuke::AdvanceToTarget:L225` |
| 84 | ENTITY Meteor | Land | [Sample meteorhitclose1] Stat/Once md200.00 v=10.00 f=1.03 \|\| [Sample meteorhitfar1] Stat/Once md450.00 v=CameraDistance 100.00..320.00->0.00..9.90 | 2 | yes | Meteor impacts its target (Meteor::Advance; game speed was already ramped to 0.4 within 200 wu of the target): near-impact layer + distant-boom layer whose volume rises 0 -> 9.9 as camera distance goes 100 -> 320 wu (near/far crossfade by distance). | `code/worldobject/meteor.cpp:Meteor::Advance:L84` |
| 85 | ENTITY Meteor | Create | [Sample rocketthrust2loop] Att/Loop md97.96 v=10.00 f=HeightAboveGround 24.00..996.00->0.89..1.80 \|\| [Sample rumble2_loop] Att/Loop md18.52 v=9.00 | 2 | yes | Meteor spawned; two looping fall sounds. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 86 | ENTITY Tank | Create | [Sample tankenginefar] Att/Loop md100.00 v=Velocity 0.00..2.40->8.39..10.00 f=Velocity 0.00..22.40->1.10..1.68 \|\| [Group tankcreate] Att/Once md100.00 v=9.35 | 7 | site-live/type-DEBUG | Tank spawned (debug-only). | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 87 | ENTITY Tank | LoseHealth | [Group tankloseheath] Att/Once md100.00 v=9.00 f=rnd 0.88..1.05/once | 10 | site-live/type-DEBUG | Tank damaged (debug-only). | `code/worldobject/tank.cpp:Tank::ChangeHealth:L142` |
| 88 | ENTITY Tank | Die | [Group tankdeath] Stat/Once md100.00 v=10.00/once f=rnd 0.73..0.94/once | 9 | site-live/type-DEBUG | Tank destroyed (debug-only). | `code/worldobject/tank.cpp:Tank::ChangeHealth:L150` |
| 89 | ENTITY SpaceShip | Create | [Group theramins] Att/Loop md329.40 v=10.00 ld=rnd 0.00..5.81/once \|\| [Sample ufo_engine2] Att/Loop md208.04 v=10.00 f=HeightAboveGround 0.00..984.00->0.32..2.47 \|\| [Sample ufo_engine1] Att/Loop md144.99 v=Velocity 3.60..1.60->8.06..9.90 \|\| [Sample rumble2_loop] Att/Loop md300.84 v=10.00 f=1.29 | 12 | yes | Space ship spawned: four layered loops (theremin group, height-pitched engine, speed-volume engine, rumble). | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 90 | ENTITY SpaceShip | TractorBeam | [Sample tractorbeam2] Att/Loop md100.00 v=10.00 | 1 | yes | Ship reaches its target and begins abducting Darwinians (SpaceShip::Advance, ShipStateIdle -> ShipStateAbducting); looping. | `code/worldobject/spaceship.cpp:SpaceShip::Advance:L188` |
| 91 | BUILDING RadarDish | BeginRotation | [Sample radar_start_turn] Att/Once md100.00 v=9.03 \|\| [Sample radar_motor_run] Att/Loop md100.00 v=8.97 | 2 | yes | Dish is aimed (RadarDish::Aim from player order / AI): stops any running rotation sound and restarts; one-shot + loop. | `code/worldobject/radardish.cpp:RadarDish::Aim:L380` |
| 92 | BUILDING RadarDish | EndRotation | [Sample radar_stop_turn] Att/Once md100.00 v=9.00 | 1 | yes | Dish finishes turning (horizontally aligned, angular velocity < 0.05) (RadarDish::Advance). | `code/worldobject/radardish.cpp:RadarDish::Advance:L231` |
| 93 | BUILDING RadarDish | ReprogramComplete | [Sample radar_reprog_complete] Att/Once md200.00 v=8.97 r=17.61 | 1 | no(LEGACY) | Building::ReprogramComplete - only via ControlTower/script; unreachable in MW. | `code/worldobject/building.cpp:Building::ReprogramComplete:L272` [LEGACY(Building::ReprogramComplete only via ControlTower::Reprogram / script)] |
| 94 | BUILDING RadarDish | ConnectionEstablished | [Group radarconnect] Att/Loop md50.00 v=8.16 | 4 | yes | Dish locks onto a receiving dish (RadarDish::Advance); looping link hum. | `code/worldobject/radardish.cpp:RadarDish::Advance:L297` |
| 95 | BUILDING RadarDish | ConnectionLost | [Group radarlose] Att/Once md150.00 v=8.94 r=14.32 fx=Echo | 4 | yes | Link broken (dish re-aimed / receiver lost) (RadarDish::Advance). | `code/worldobject/radardish.cpp:RadarDish::Advance:L289` |
| 96 | BUILDING LaserFence | Create | [Sample fence_hum] Att/Loop md20.00 v=10.00/once | 1 | yes | Fence initialised (Building::Initialise); looping hum. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 97 | BUILDING LaserFence | Spark | [Group elec_cackle] Att/Once md100.00 v=7.00 f=rnd 0.84..1.40/once | 6 | yes | Every 8-12 s while not disabled (LaserFence::Advance -> Spark). | `code/worldobject/laserfence.cpp:LaserFence::Spark:L106` |
| 98 | BUILDING LaserFence | Damage | [Group elecfence@damage] Att/Once md100.00 v=10.00 f=rnd 0.94..1.09/once | 16 | yes | Fence damaged by an explosion (Building::Damage via Location::Bang). | `code/worldobject/building.cpp:Building::Damage:L582` |
| 99 | BUILDING GunTurret | FireShell | [Group turretfire] Att/Once md91.92 v=10.00/once f=rnd 0.94..1.16/once | 6 | yes | Standard turret fires (GunTurret::PrimaryFire). | `code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L577` |
| 100 | BUILDING GunTurret | Damage | [Group battlecannondamage] Att/Once md100.00 v=9.03/once f=rnd 0.75..0.93/once | 10 | no(no-site) | GunTurret::Damage overrides Building::Damage without a sound - dead data. | — |
| 101 | BUILDING GunTurret | Create | [Group armourtransform] Stat/Once md100.00 v=10.00 | 6 | yes | Turret initialised (Building::Initialise). | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 102 | BUILDING GunTurret | FireFlame | [Group flameturretfire] Stat/Once md100.00 v=10.00 f=rnd 0.92..1.55/once | 10 | yes | Flame turret fires (GunTurret::PrimaryFire). | `code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L578` |
| 103 | BUILDING GunTurret | RegisterKill | [Sample countdownbeep3] Att/Once md50.78 v=10.00/once f=1.20/once | 1 | yes | Turret credited with a kill (GunTurret::RegisterKill). | `code/worldobject/gunturret.cpp:GunTurret::RegisterKill:L209` |
| 104 | BUILDING Powerstation | Create | [Sample power_station_online] Att/Loop md100.00 v=10.00 | 1 | site-live/type-LEGACY | Building::Initialise. Type unreachable in MW. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 105 | BUILDING Tree | CreateNormal | [Group treeatmos] Att/LoopADSR md5.98 v=10.00 a=2.55/once s=rnd 5.81..9.68/once r=2.55 | 8 | yes | Ordinary tree created (Tree::Initialise); LoopADSR rustle. | `code/worldobject/tree.cpp:Tree::Initialise:L101` |
| 106 | BUILDING Tree | CreateMagic | [Group treeatmos] Att/LoopADSR md5.98 v=10.00 a=2.55/once s=rnd 5.81..9.68/once r=2.55 | 8 | yes | Tree with spirit drop rate > 0 (MagicalForest) created (Tree::Initialise); LoopADSR shimmer. | `code/worldobject/tree.cpp:Tree::Initialise:L100` |
| 107 | BUILDING Tree | Burn | [Group treeonfire] Att/Loop md145.75 v=10.00 r=9.68 | 15 | yes | Tree catches fire (Tree::Damage, once until it stops burning); looping. | `code/worldobject/tree.cpp:Tree::Damage:L893` |
| 108 | BUILDING Tree | Create | [Sample cratereward_forest2] Att/Once md90.83 v=9.03 f=0.32 | 1 | yes | Every tree (Building::Initialise) in addition to the Create* variant. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 109 | BUILDING Tree | CreateEvil | [Group gdspeechgroup] Att/Loop md19.12 v=10.00 ld=rnd 0.00..23.61/once | 17 | yes | Evil tree (DarkForest) created (Tree::Initialise); looping. | `code/worldobject/tree.cpp:Tree::Initialise:L99` |
| 110 | BUILDING TrunkPort | ReprogramComplete | [Group portcreate] Att/Loop md100.00 v=7.00 | 5 | no(LEGACY) | TrunkPort::ReprogramComplete - only via ControlTower/script; unreachable in MW. | `code/worldobject/building.cpp:Building::ReprogramComplete:L272` [LEGACY(Building::ReprogramComplete only via ControlTower::Reprogram / script)] |
| 111 | BUILDING ResearchItem | Create | [Group researchcreate] Att/Loop md100.00 v=7.00 | 9 | site-live/type-LEGACY | Type unreachable in MW. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 112 | BUILDING ResearchItem | AquireResearch | [Group aquireresearch] Stat/Once md150.00 v=10.00 | 7 | site-live/type-LEGACY | Research item fully reprogrammed (ResearchItem::Advance). Unreachable. | `code/worldobject/researchitem.cpp:ResearchItem::Advance:L110` |
| 113 | BUILDING Library | Create | [Sample ablaster] Att/Once md30.00 v=3.00 | 1 | site-live/type-LEGACY | Type unreachable in MW. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 114 | BUILDING Generator | ReprogramComplete | [Sample generator_online] Stat/Loop md100.00 v=9.84/once | 1 | no(no-site) | Generator::ReprogramComplete overrides without this event (fires "Enable") - dead data. | — |
| 115 | BUILDING Generator | TriggerSurge | [Group generatorburst] Att/Once md100.00 v=7.00 f=rnd 0.86..1.21/once | 5 | site-live/type-LEGACY | Power surge passes through (PowerBuilding::TriggerSurge). Type unreachable. | `code/worldobject/generator.cpp:PowerBuilding::TriggerSurge:L299` |
| 116 | BUILDING Pylon | TriggerSurge | [Group ylonelecsurge] Att/Once md50.00 v=7.10 f=rnd 0.94..1.55/once | 22 | yes | A power surge reaches this pylon (PowerBuilding::TriggerSurge, propagated SolarPanel -> Pylon chain -> PylonEnd). | `code/worldobject/generator.cpp:PowerBuilding::TriggerSurge:L299` |
| 117 | BUILDING SolarPanel | TriggerSurge | [Group solarpanelenable] Att/Once md50.00 v=7.06/once f=rnd 0.89..1.24/once | 5 | yes | Solar panel emits a surge: chance fractionOccupied/20 per tick (SolarPanel::Advance -> TriggerSurge). | `code/worldobject/generator.cpp:PowerBuilding::TriggerSurge:L299` |
| 118 | BUILDING SolarPanel | Operate | [Sample solarcharge] Att/Loop md100.00 v=8.00 f=rnd 1.15..1.18 s0.24/once a=4.06 | 1 | yes | Occupancy rises above 60 % while not yet operating (SolarPanel::Advance); looping. | `code/worldobject/generator.cpp:SolarPanel::Advance:L753` |
| 119 | BUILDING TrackLink | CogTurn | [Group minecartcreak] Stat/Once md150.00 v=rnd 8.19..9.74/once | 43 | site-live/type-LEGACY | Mine-cart line speed rises above 0.1 (MineBuilding::Advance). Unreachable. | `code/worldobject/mine.cpp:MineBuilding::Advance:L338` |
| 120 | BUILDING Refinery | CogTurn | [Group refineryrun] Att/Once md200.00 v=10.00/once | 8 | site-live/type-LEGACY | As TrackLink. Unreachable. | `code/worldobject/mine.cpp:MineBuilding::Advance:L338` |
| 121 | BUILDING Refinery | Create | [Group mineambience] Att/Loop md500.00 v=9.00 f=0.62 | 6 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 122 | BUILDING Mine | CogTurn | [Group minerun] Att/Once md150.00 v=8.97 f=0.95/once fx=DSound_I3DL2Reverb | 7 | site-live/type-LEGACY | As TrackLink. Unreachable. | `code/worldobject/mine.cpp:MineBuilding::Advance:L338` |
| 123 | BUILDING Yard | Create | [Sample yard] Att/Loop/MonoRandom md200.00 v=10.00 f=1.50 | 1 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 124 | BUILDING Incubator | Reprogramming | [Sample engineer_reprog_radar_loop] Att/Once md200.00 v=8.06 | 1 | no(no-site) | No code fires it - dead data. | — |
| 125 | BUILDING Incubator | SpawnEntity | [Group incubatorspawn] Stat/Once md200.00 v=8.61 f=rnd 0.90..1.02/once r=7.16 | 6 | site-live/type-LEGACY | Incubator hatches a Darwinian (Incubator::SpawnEntity). Dev maps only. | `code/worldobject/incubator.cpp:Incubator::SpawnEntity:L245` |
| 126 | BUILDING Incubator | AddSpirit | [Group incubatoraddspirit] Att/Once md150.00 v=9.00 f=rnd 0.38..0.52/once r=15.10 fx=Gargle | 6 | site-live/type-LEGACY | Spirit added to the incubator (Incubator::AddSpirit). Dev maps only. | `code/worldobject/incubator.cpp:Incubator::AddSpirit:L265` |
| 127 | BUILDING AntHill | Damage | [Group 1anthilldamage] Att/Once md100.00 v=9.03 | 10 | yes | Ant hill damaged (AntHill::Damage on each 20-point health band, plus Building::Damage). | `code/worldobject/anthill.cpp:AntHill::Damage:L78`<br>`code/worldobject/building.cpp:Building::Damage:L582` |
| 128 | BUILDING AntHill | Explode | [Group newsamplegroup3] Stat/Once md200.00 v=10.00 f=0.31 r=12.19 | 5 | yes | Ant hill destroyed (AntHill::Damage, health reaches 0; spirits released). | `code/worldobject/anthill.cpp:AntHill::Damage:L101`<br>`code/worldobject/building.cpp:Building::Destroy:L597` [LEGACY(Building::Destroy only via Script::RunCommand_DestroyBuilding)] |
| 129 | BUILDING Triffid | Damage | [Group sporegenlosehealt] Stat/Once md100.00 v=10.00 f=1.67 r=12.00 | 6 | yes | Triffid damaged (Building::Damage via Triffid::Damage). | `code/worldobject/building.cpp:Building::Damage:L582` |
| 130 | BUILDING Triffid | Burn | [Group triffidburn] Att/Loop md124.17 v=10.00 | 3 | yes | Triffid killed (Triffid::Damage, damage crosses 0); looping burn. | `code/worldobject/triffid.cpp:Triffid::Damage:L291` |
| 131 | BUILDING SpiritReceiver | Create | [Group spiritdescent] Att/LoopADSR md50.00 v=8.00 a=3.29 s=8.71 r=6.39 ld=3.87 | 25 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 132 | BUILDING SpiritProcessor | Create | [Group spiritdescent] Att/LoopADSR md100.00 v=8.00 a=3.29 s=7.74 r=8.32 ld=3.10 | 25 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 133 | BUILDING SpawnPoint | TurnOn | [Sample spawnpoint_playerturnson2] Att/Once md100.00 v=10.00 | 1 | yes | Spawn point captured by a team (SpawnPoint::RecalculateOwnership, team changed to non-neutral). | `code/worldobject/spawnpoint.cpp:SpawnPoint::RecalculateOwnership:L883` |
| 134 | BUILDING SpawnPoint | TurnOff | [Sample spawnpoint_playerturnsoff2] Att/Once md100.00 v=9.10 | 1 | yes | Spawn point becomes neutral (winningTeam 255). | `code/worldobject/spawnpoint.cpp:SpawnPoint::RecalculateOwnership:L882` |
| 135 | BUILDING BlueprintStore | Create | [Group elec_cackle] Att/Loop md100.00 v=9.00 f=0.42 fx=DSound_Echo | 6 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 136 | BUILDING BlueprintConsole | Create | [Group relayhover] Att/Loop md100.00 v=10.00 f=0.30 | 4 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 137 | BUILDING BlueprintRelay | Create | [Group relayhover] Att/Loop md50.00 v=8.48/once | 4 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 138 | BUILDING Spam | Create | [Group samcreate] Att/Loop md100.00 v=9.03 | 8 | site-live/type-LEGACY | Unreachable (Spam building is Darwinia-only). | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 139 | BUILDING Spam | Damage | [Group emaildamage] Att/Once md150.00 v=10.00 f=1.67 r=13.16 fx=DSound_Echo | 7 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Damage:L582` |
| 140 | BUILDING Spam | Attack | [Group spamattack] Stat/Once md100.00 v=10.00 f=rnd 1.00..1.28/once | 7 | site-live/type-LEGACY | Spam spawns infection (Spam::SpawnInfection). Unreachable. | `code/worldobject/spam.cpp:Spam::SpawnInfection:L243` |
| 141 | BUILDING Spam | Explode | [Group spamboom] Stat/Once md200.00 v=10.00 f=rnd 0.92..1.21/once fx=DSound_Echo | 6 | site-live/type-LEGACY | Spam destroyed (Spam::Damage). Unreachable. | `code/worldobject/building.cpp:Building::Destroy:L597` [LEGACY(Building::Destroy only via Script::RunCommand_DestroyBuilding)]<br>`code/worldobject/spam.cpp:Spam::Damage:L94` |
| 142 | BUILDING Spam | CreateResearch | [Group researchcreate] Att/Loop md100.00 v=7.00 | 9 | site-live/type-LEGACY | Spam::SetAsResearch. Unreachable. | `code/worldobject/spam.cpp:Spam::SetAsResearch:L343` |
| 143 | BUILDING GodDish | ConnectToGod | [Group connectgod] Att/Loop md200.00 v=9.87 f=0.56 r=12.00 | 3 | site-live/type-LEGACY | GodDish::Activate. Unreachable. | `code/worldobject/goddish.cpp:GodDish::Activate:L186` |
| 144 | BUILDING GodDish | DisconnectFromGod | [Group disconnectgod] Att/Once md250.00 v=10.00 | 3 | site-live/type-LEGACY | GodDish::DeActivate. Unreachable. | `code/worldobject/goddish.cpp:GodDish::DeActivate:L199` |
| 145 | BUILDING GodDish | Create | [Group goddishcreate] Att/Loop md200.00 v=10.00 f=1.11 | 5 | site-live/type-LEGACY | Unreachable. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 146 | BUILDING FuelGenerator | PumpUp | [Sample fuelpumpexhale1] Att/Once md202.34 v=8.45 f=1.03 \|\| [Group bigmetalgroan] Att/Once md100.00 v=8.87 f=1.02 | 6 | yes | Pump-head position crosses the 0.1 threshold (FuelGenerator::Advance compares previous/new m_previousPumpPos); two layered blocks. | `code/worldobject/rocket.cpp:FuelGenerator::Advance:L367` |
| 147 | BUILDING FuelGenerator | PumpDown | [Sample fuelpumpup1] Att/Once md100.00 v=8.77 f=0.98 \|\| [Group kickverb] Att/Once md195.22 v=10.00 f=0.99 | 4 | yes | Pump-head position crosses the 0.9 threshold (FuelGenerator::Advance); two layered blocks. | `code/worldobject/rocket.cpp:FuelGenerator::Advance:L371` |
| 148 | BUILDING FuelPipe | PumpFuel | [Sample fuelpipe2_loop] Att/Loop md100.00 v=10.00 f=0.57 a=5.23 | 1 | yes | Kept alive while pipe fuel level > 0.2 (FuelPipe::Advance polls NumInstances each tick), stopped when <= 0.2; looping. | `code/worldobject/rocket.cpp:FuelPipe::Advance:L458` |
| 149 | BUILDING FuelStation | LoadPassenger | [Group loadpassenger] Stat/Once md75.41 v=9.06 r=4.65 | 5 | yes | A Darwinian boards the escape rocket (FuelStation::BoardRocket). | `code/worldobject/rocket.cpp:FuelStation::BoardRocket:L618` |
| 150 | BUILDING EscapeRocket | Happy | [Group shiphappy] Att/Loop md100.00 v=6.94 f=rnd 0.60..0.99/once ld=4.65 \|\| [Sample shipheartbeat1] Att/Loop md200.10 v=10.00 f=rnd 0.72..0.82 s0.59/once | 6 | yes | State Loading/Ignition/Ready/Countdown with damage < 75 (EscapeRocket::SetupSounds keeps exactly one instance alive); 2 looping blocks. | `code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L957` |
| 151 | BUILDING EscapeRocket | Malfunction | [Sample shipmalfunction1] Att/Loop md163.95 v=9.55 | 1 | yes | Damage >= 75 in Refueling..Countdown, or Exploding with fuel left (EscapeRocket::SetupSounds); looping. | `code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L957` |
| 152 | BUILDING EscapeRocket | Explode | [Sample engineexplode] Att/Once md318.60 v=10.00 r=13.35 \|\| [Group newsamplegroup27] Att/Once md190.11 v=10.00 f=0.62 | 5 | yes | Rocket damage exceeds 100 -> StateExploding (EscapeRocket::Damage); 2 blocks. | `code/worldobject/building.cpp:Building::Destroy:L597` [LEGACY(Building::Destroy only via Script::RunCommand_DestroyBuilding)]<br>`code/worldobject/rocket.cpp:EscapeRocket::Damage:L1556` |
| 153 | BUILDING EscapeRocket | Damage | [Group shipscream] Stat/Once md200.00 v=10.00 f=1.05 | 2 | yes | Rocket damaged (Building::Damage from EscapeRocket::Damage). | `code/worldobject/building.cpp:Building::Damage:L582` |
| 154 | BUILDING EscapeRocket | EngineBurn | [Sample engineburn] Att/Loop md150.25 v=10.00 \|\| [Sample ignitionzap1] Att/Once md100.00 v=7.00 f=0.50 r=6.39 | 2 | yes | Engines on in Ready/Countdown/Flight (EscapeRocket::SetupSounds keeps it alive, stops otherwise); one-shot + loop. | `code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L972` |
| 155 | BUILDING EscapeRocket | Unhappy | [Group shiphungry] Att/Loop/MonoNearest md200.00 v=9.35 ld=rnd 4.06..16.06/once | 7 | yes | Refueling with fuel level <= 0.2, or Exploding with no fuel (EscapeRocket::SetupSounds); MonophonicNearest loop. | `code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L957` |
| 156 | BUILDING EscapeRocket | Flight | [Group choir1] Att/Loop md10000.00 v=10.00 r=12.00 ld=rnd 0.00..3.48/once | 6 | yes | Rocket launched (StateFlight); looping. | `code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L957` |
| 157 | BUILDING MultiwiniaZone | CaptureStatue | [Sample player_capture_statue] 2D/Once md200.00 v=10.00 \|\| [Sample playercapturebugle5] 2D/Once/MyTeam md200.00 v=9.29 \|\| [Sample enemycapturebugle1] 2D/Once/Enemy md200.00 v=10.00 | 3 | yes | A statue is delivered into this CTS zone (MultiwiniaZone::Advance_CaptureTheStatue): 2D sting for all + team-specific sting. | `code/worldobject/multiwiniazone.cpp:MultiwiniaZone::Advance_CaptureTheStatue:L415` |
| 158 | BUILDING MultiwiniaZone | ChangeTeam | [Sample koh_zone_captured] Att/Once/MyTeam md300.00 v=10.00 \|\| [Sample koh_zone_lost] Att/Once/Enemy md300.00 v=10.00 | 2 | yes | Zone owner changes (Building::SetTeamId, KotH): different sounds for my team / enemy. | `code/worldobject/building.cpp:Building::SetTeamId:L293` |
| 159 | BUILDING MultiwiniaZone | Create | [Sample koh_zoneactive] Att/Loop md47.91 v=9.00 | 1 | yes | Zone initialised; looping hum. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 160 | BUILDING ControlStation | Create | [Group relayhover] Att/Loop md70.89 v=8.84/once | 4 | yes | Assault control station initialised; looping hum. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 161 | BUILDING Crate | Land | [Sample crateland1] Att/Once md150.00 v=9.00 f=rnd 0.89..1.42/once | 1 | yes | Crate touches ground (Crate::AdvanceFloating; stops "Crate Falling"). | `code/worldobject/crate.cpp:Crate::AdvanceFloating:L119` |
| 162 | BUILDING Crate | BeginCapture | [Sample crate_capturemyteam] Att/Once/MyTeam md100.00 v=10.00 \|\| [Sample engineer_reprog_radar_loop] Att/Loop md64.40 v=6.94 \|\| [Sample crate_captureotherteam] Att/Once/Enemy md100.00 v=10.00 | 3 | yes | Neutral crate gains a majority team (Crate::AdvanceOnGround after timer reaches 0): looping capture bed + my-team / enemy stings. | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L693` |
| 163 | BUILDING Crate | LostCapture | [Sample crate_lostcapture] Att/Once md100.00 v=10.00 | 1 | yes | Capture abandoned, crate returns to neutral (Crate::AdvanceOnGround). | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L689` |
| 164 | BUILDING Crate | Capturing_0pc | [Sample cratecapture_10a] Att/LoopADSR md150.00 v=9.00 a=1.16 r=11.81 | 1 | yes | Capture starts (fired with BeginCapture); LoopADSR progress tone. | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L694` |
| 165 | BUILDING Crate | Capturing_20pc | [Sample cratecapture_20a] Att/LoopADSR md150.00 v=9.00 a=1.16 r=12.00 | 1 | yes | Capture progress crosses 20 % (sprintf "Capturing_%dpc" in Crate::AdvanceOnGround); previous tone stopped by StopCaptureSounds. | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L623` |
| 166 | BUILDING Crate | Capturing_40pc | [Sample cratecapture_30a] Att/LoopADSR md157.70 v=9.48 a=1.16 r=12.39 | 1 | yes | Progress crosses 40 %. | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L623` |
| 167 | BUILDING Crate | Capturing_60pc | [Sample cratecapture_40a] Att/LoopADSR md200.03 v=9.52 a=0.97 r=12.19 | 1 | yes | Progress crosses 60 %. | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L623` |
| 168 | BUILDING Crate | Capturing_80pc | [Sample cratecapture_50a] Att/LoopADSR md218.95 v=10.00 a=1.16 r=12.58 | 1 | yes | Progress crosses 80 % (100 % fires "Capturing_100pc" which has no data). | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L623` |
| 169 | BUILDING Crate | Captured | [Sample cratecapturedaccent] Stat/Once md200.00 v=10.00 | 1 | yes | Capture timer reaches CRATE_CAPTURE_TIME and the reward runs (Crate::AdvanceOnGround). | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L648` |
| 170 | BUILDING Crate | Falling | [Group cratefalling] Att/LoopADSR md100.00 v=8.58 a=4.26 s=rnd 0.97..12.58/once r=4.65 fx=DSound_Echo | 7 | yes | Crate created in the sky (Crate::Initialise); LoopADSR whistle until Land. | `code/worldobject/crate.cpp:Crate::Initialise:L79` |
| 171 | BUILDING Statue | ChangeTeam | [Sample player_capture_statue] 2D/Once/MyTeam md100.00 v=7.97 \|\| [Sample enemy_capture_statue] 2D/Once/Enemy md100.00 v=7.97 | 2 | yes | Statue owner changes (Building::SetTeamId): my-team / enemy 2D stings. | `code/worldobject/building.cpp:Building::SetTeamId:L293` |
| 172 | BUILDING Statue | Drop | [Sample statuedrop3] Att/Once md250.14 v=10.00 | 1 | yes | Statue put down (CarryableBuilding::Advance, lifted -> not lifted). | `code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance:L250` |
| 173 | BUILDING Statue | Create | [Sample statuespawn1] Stat/Once md249.31 v=10.00 | 1 | yes | Statue spawned (Building::Initialise). | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 174 | BUILDING PulseBomb | Create | [Group choir1] Stat/Loop md100.00 v=9.32 f=0.50 | 6 | yes | Assault pulse bomb initialised; looping tick. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 175 | OTHER Laser | InjuredEntity | [Group combatvoxinjured] Att/Once md100.00 v=9.55 | 36 | yes | A laser wounds (not kills) a Darwinian; plays at the victim position with the shooter's team (Laser::Advance). | `code/worldobject/weapons.cpp:Laser::Advance:L772` |
| 176 | OTHER Grenade | Explode | [Group newsamplegroup3] Stat/Once md200.00 v=10.00 r=10.00 fx=DSound_WavesReverb | 5 | yes | Grenade detonates (Grenade::Advance via ThrowableWeapon::TriggerSoundEvent). | `code/worldobject/weapons.cpp:Grenade::Advance:L254 -> TriggerSoundEvent` |
| 177 | OTHER Grenade | Create | [Group newsamplegroup4] Att/Once md100.00 v=9.55 f=rnd 0.74..1.54 s0.84 | 5 | yes | Grenade or air-strike marker thrown (ThrowableWeapon::Initialise from Location::ThrowWeapon). | `code/worldobject/weapons.cpp:ThrowableWeapon::Initialise:L73 -> TriggerSoundEvent (Grenade/AirStrikeMarker via Location::ThrowWeapon)` |
| 178 | OTHER Grenade | Flash | [Sample grenade_beep] Att/Once md100.00 v=7.90 | 1 | yes | Once per whole second since the throw, during rendering (ThrowableWeapon::Render) - render-rate dependent. | `code/worldobject/weapons.cpp:ThrowableWeapon::Render:L159 -> TriggerSoundEvent (render-driven)` |
| 179 | OTHER Grenade | ExplodeController | [Sample hypno_loop_run] Att/Loop md100.00 v=10.00 | 1 | no(LEGACY) | Controller grenade activates 3 s after the throw (ControllerGrenade::Advance). Unreachable in MW. | `code/worldobject/weapons.cpp:ControllerGrenade::Advance:L354` [LEGACY(controller grenade unreachable in MW)] |
| 180 | OTHER Grenade | Bounce | [Group newsamplegroup6] Att/Once md74.60 v=Velocity 0.00..10.00->6.97..9.00/once f=rnd 0.96..1.56 s0.66/once | 6 | yes | Grenade/marker bounces off the landscape (ThrowableWeapon::Advance); volume follows speed. | `code/worldobject/weapons.cpp:ThrowableWeapon::Advance:L122 -> TriggerSoundEvent` |
| 181 | OTHER Rocket | Explode | [Group newsamplegroup7] Stat/Once md200.00 v=10.00 f=rnd 0.80..1.20 | 4 | yes | Rocket explodes (Rocket::Explode; stops "Rocket Create"). | `code/worldobject/weapons.cpp:Rocket::Explode:L539` |
| 182 | OTHER Rocket | Create | [Sample rocket_fire4_run] Att/Loop md100.00 v=10.00 \|\| [Sample rocketlauncher] Att/Once md150.00 v=10.00 f=0.94/once | 2 | yes | Rocket launched (Rocket::Initialise from Location::FireRocket): launch one-shot + looping motor (stopped by Rocket::Explode). | `code/worldobject/weapons.cpp:Rocket::Initialise:L422` |
| 183 | OTHER AirStrikeBomb | Explode | [Group airstrikeexplode] Stat/Once md200.00 v=9.23 | 5 | yes | Bomber bomb / napalm bomb explodes (Grenade::Advance / FlameGrenade::Advance). | `code/worldobject/weapons.cpp:Grenade::Advance:L254 / FlameGrenade::Advance:L2016 -> TriggerSoundEvent` |
| 184 | OTHER AirStrikeBomb | Flash | [Sample grenade_beep] Att/Once md100.00 v=8.00 | 1 | yes | Bomb render flash once per second (ThrowableWeapon::Render). | `code/worldobject/weapons.cpp:ThrowableWeapon::Render:L159 -> TriggerSoundEvent` |
| 185 | OTHER Spirit | PickedUp | [Sample sonar] Att/Once md100.00 v=9.00 | 1 | yes | Engineer/Harvester picks up a spirit (Spirit::CollectorArrives). | `code/worldobject/spirit.cpp:Spirit::CollectorArrives:L286` |
| 186 | OTHER Sepulveda | Appear | [Sample sepulveda_appear] Ed/Once md100.00 v=8.00/once | 1 | no(not-compiled) | Sepulveda help avatar appears. Dead (USE_SEPULVEDA_HELP_TUTORIAL). | `code/sepulveda.cpp:Sepulveda::Advance:L199` [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] |
| 187 | OTHER Sepulveda | Disappear | [Sample sepulveda_disappear] Ed/Once md100.00 v=7.97/once r=7.94 | 1 | no(not-compiled) | Dead (USE_SEPULVEDA_HELP_TUTORIAL). | `code/sepulveda.cpp:Sepulveda::Advance:L207` [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] |
| 188 | OTHER Sepulveda | TextAppear | [Sample [???]] Att/Once md100.00 v=7.00 | 0 | no(not-compiled) | Dead; also has SOUNDNAME "[???]" so it could never start. | `code/sepulveda.cpp:Sepulveda::RenderTextBoxCutsceneMode:L699` [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)]<br>`code/sepulveda.cpp:Sepulveda::RenderTextBoxTaskManagerMode:L864` [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)]<br>`code/sepulveda.cpp:Sepulveda::RenderTextBox:L1124` [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] |
| 189 | OTHER Gesture | GestureBegin | [Sample filtersweep] 2D/Once md100.00 v=6.97 r=0.39 | 1 | yes | Player starts a task from the Multiwinia task manager (TaskManagerInterfaceMultiwinia::RunScreenZone RunProgram / SelectWeapon). | `code/location_input.cpp:LocationInput::AdvanceTeamControl:L1137` [commented-out]<br>`code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L175` [LEGACY(TaskManagerInterfaceGestures: single-player only)]<br>`code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::RunScreenZone:L678` [LEGACY(TaskManagerInterfaceGestures)]<br>`code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::RunScreenZone:L712` [LEGACY(TaskManagerInterfaceGestures)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1264` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1327` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L817`<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L883` |
| 190 | OTHER Gesture | GestureSuccess | [Sample gesturesuccess] 2D/Once md100.00 v=10.00 | 1 | yes | Officer promotion succeeds (Task::TargetOfficer), Engineer deployed (Task::TargetEngineer), weapon selected in the task manager. | `code/location_input.cpp:LocationInput::AdvanceTeamControl:L1138` [commented-out]<br>`code/taskmanager.cpp:Task::TargetSquad:L211` [LEGACY(else-branch of g_app->Multiplayer())]<br>`code/taskmanager.cpp:Task::TargetEngineer:L224`<br>`code/taskmanager.cpp:Task::TargetShaman:L234` [LEGACY(Shaman task)]<br>`code/taskmanager.cpp:Task::TargetArmour:L252` [LEGACY(else-branch of g_app->Multiplayer())]<br>`code/taskmanager.cpp:Task::TargetHarvester:L272` [LEGACY(else-branch of g_app->Multiplayer())]<br>`code/taskmanager.cpp:Task::TargetOfficer:L420`<br>`code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L209` [LEGACY(TaskManagerInterfaceGestures)]<br>`code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::RunScreenZone:L713` [LEGACY(TaskManagerInterfaceGestures)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1328` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L884` |
| 191 | OTHER Gesture | GestureFail | [Sample gesturefail] 2D/Once md100.00 v=10.00 | 1 | no(LEGACY) | Darwinia gesture recognition failed. Unreachable in MW. | `code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L222` [LEGACY(TaskManagerInterfaceGestures)] |
| 192 | OTHER Ambience | EnterLocation | [Group newsamplegroup8] 2D/LoopADSR md100.00 v=Ypos 736.00..232.00->0.10..9.58 a=10.00 s=10.84 r=10.00 \|\| [Sample white_noise] 2D/Loop md500.00 v=Ypos 0.00..992.00->3.03..7.03 fx=Echo \|\| [Group highaltitude] 2D/LoopADSR md100.00 v=Ypos 0.00..988.00->0.06..5.97 f=rnd 0.75..1.35 s0.66/once a=10.84 s=rnd 1.94..16.45/once r=10.45 | 11 | yes | Start of every match (LocationGameLoop); stopped when the loop exits. 3 layered 2D beds driven by camera altitude. | `code/main.cpp:LocationGameLoop:L424` |
| 193 | OTHER Ambience | EnterGlobalWorld | [Group worldviewdry] 2D/Loop md100.00 v=6.84 r=8.32 ld=rnd 0.00..2.71/once fx=Echo \|\| [Group worldviewwet] 2D/Loop md100.00 v=6.10 ld=rnd 0.19..2.52/once \|\| [Group worldamb] 2D/LoopADSR md100.00 v=6.00 f=0.37 a=9.68 s=0.97 r=11.03 ld=0.39 fx=Echo \|\| [Sample altitude1] 2D/Loop md100.00 v=5.23 f=0.70 a=6.19 r=11.23 \|\| [Sample rumble1] 2D/Loop md100.00 v=7.74 f=0.60 a=5.42 r=6.00 | 22 | no(LEGACY) | Darwinia world-map loop. Unreachable in MW. | `code/main.cpp:GlobalWorldGameLoop:L705` [LEGACY(GlobalWorldGameLoop = Darwinia world map)] |
| 194 | OTHER Music | Cutscene1 | [Sample tresk_-_excuses] 2D/Loop md100.00 v=10.00 r=20.13 | 1 | no(LEGACY) | Script TriggerSound cutscene1 (Darwinia). Unreachable; track not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 195 | OTHER Music | Cutscene2 | [Sample tresk_-_faces-of-a-fashion] 2D/Loop md100.00 v=10.00 r=20.13 | 1 | no(LEGACY) | Script TriggerSound cutscene2. Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 196 | OTHER Music | Cutscene3 | [Sample trash80_-_pain-fade-down] 2D/Loop md100.00 v=10.00 a=5.23 r=20.13 | 1 | no(LEGACY) | Script TriggerSound cutscene3. Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 197 | OTHER Music | Cutscene4 | [Sample tresk_-_impact-of-silence] 2D/Loop md100.00 v=10.00 a=2.90 r=20.13 | 1 | no(LEGACY) | Script TriggerSound cutscene4. Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 198 | OTHER Music | LoaderRaytrace | [Sample dma_sc-visitors_from_dreams] 2D/Loop md100.00 v=9.00 r=10.06 | 1 | no(not-compiled) | Raytrace loader (USE_LOADERS off). Dead; not shipped. | `code/loaders/raytrace_loader.cpp:RayTraceLoader::Run:L675` [not-compiled(#if USE_LOADERS)] |
| 199 | OTHER Music | LoaderFodder | [Sample fodder] 2D/Once md100.00 v=9.03 | 1 | no(not-compiled) | Fodder loader (USE_LOADERS off). Dead; not shipped. | `code/loaders/fodder_loader.cpp:FodderLoader::Run:L34` [not-compiled(#if USE_LOADERS)] |
| 200 | OTHER Music | LoaderSoul | [Sample tresk_-weeklies-_schroeders-failure] 2D/Loop md100.00 v=10.00 r=10.06 | 1 | no(not-compiled) | Soul loader (USE_LOADERS off). Dead; not shipped. | `code/loaders/soul_loader.cpp:SoulLoader::Run:L207` [not-compiled(#if USE_LOADERS)] |
| 201 | OTHER Music | EndSequence | [Sample tresk_-_faces-of-a-fashion] 2D/Once md100.00 v=10.00 a=5.23 r=3.29 | 1 | no(LEGACY) | Script gameover.txt (Darwinia). Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 202 | OTHER Music | Credits | [Sample trash80_-_pain-fade-down] 2D/Once md100.00 v=10.00 a=2.71 r=10.06 | 1 | no(LEGACY) | Credits loader via Darwinia outro camera / script. Unreachable; not shipped. | `code/loaders/credits_loader.cpp:CreditsLoader::Run:L519` [LEGACY(Camera::AdvanceSphereWorldOutroMode / Script::RunCommand_RunCredits only)] |
| 203 | OTHER Music | StartSequence | [Sample dma_sc-visitors_from_dreams] 2D/Once md100.00 v=9.00 r=10.06 | 1 | no(DEAD) | First-run boot sequence (USE_LOADERS). Dead; not shipped. | `code/startsequence.cpp:StartSequence::Advance:L76` [DEAD(StartSequence only built in RunBootloader under USE_LOADERS)] |
| 204 | OTHER Music | Cutscene5 | [Sample tresk_-_excuses] 2D/Once md100.00 v=10.00 a=3.10 r=15.48 | 1 | no(LEGACY) | Script TriggerSound cutscene5. Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 205 | OTHER Music | LoaderSpeccy | [Sample speccyloader] 2D/Once md100.00 v=9.00 | 1 | no(not-compiled) | USE_LOADERS off. Dead; not shipped. | `code/loaders/speccy_loader.cpp:SpeccyLoader::Run:L347` [not-compiled(#if USE_LOADERS)] |
| 206 | OTHER Music | LoaderAmiga | [Sample pang] 2D/Once md100.00 v=9.00 r=10.06 | 1 | no(not-compiled) | USE_LOADERS off. Dead; not shipped. | `code/loaders/amiga_loader.cpp:AmigaLoader::Run:L221` [not-compiled(#if USE_LOADERS)] |
| 207 | OTHER Music | LoaderMatrix | [Sample matrix] 2D/Once md100.00 v=9.03 r=7.55 | 1 | no(not-compiled) | USE_LOADERS off. Dead; not shipped. | `code/loaders/matrix_loader.cpp:MatrixLoader::Run:L75` [not-compiled(#if USE_LOADERS)] |
| 208 | OTHER Music | LoaderGameOfLife | [Sample twoataris] 2D/Once md100.00 v=9.00 r=10.26 | 1 | no(not-compiled) | USE_LOADERS off. Dead; not shipped. | `code/loaders/gameoflife_loader.cpp:GameOfLifeLoader::Run:L320` [not-compiled(#if USE_LOADERS)] |
| 209 | OTHER Music | Demo2Intro | [Sample trash80_-_pain-fade-down] 2D/Once md100.00 v=9.03 | 1 | no(LEGACY) | Darwinia demo 2 script. Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 210 | OTHER Music | Demo2Mid | [Sample tresk_-_faces-of-a-fashion] 2D/Once md100.00 v=9.00 r=20.71 | 1 | no(LEGACY) | Darwinia demo 2 script. Unreachable; not shipped. | `code/script.cpp:Script::RunCommand_TriggerSound:L362` [LEGACY(no Multiwinia path runs a script; data/scripts excluded from PC build)] |
| 211 | OTHER Interface | Show | [Sample taskin] 2D/Once md100.00 v=8.48 f=1.05 | 1 | no(LEGACY,commented-out) | Darwinia task-manager open (TaskManagerInterfaceIcons); commented out in the Multiwinia interface. | `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::Advance:L245` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::Advance:L225` [commented-out] |
| 212 | OTHER Interface | Hide | [Sample taskout] 2D/Once md100.00 v=8.52 f=0.72 | 1 | no(LEGACY,commented-out) | Darwinia task-manager close; commented out in the Multiwinia interface. | `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::HideTaskManager:L206` [LEGACY(TaskManagerInterfaceIcons: never created when Multiplayer())]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::HideTaskManager:L191` [commented-out] |
| 213 | OTHER Interface | Slide | [Sample menuscroll2] 2D/Once md100.00 v=9.48 | 1 | no(LEGACY) | Darwinia icon interface scroll. Unreachable in MW. | `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenEdges:L560` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1361` [LEGACY(TaskManagerInterfaceIcons)] |
| 214 | OTHER Interface | DeleteTask | [Sample deselect1] 2D/Once md100.00 v=9.13 | 1 | yes | Player terminates a task (TaskManagerInterfaceMultiwinia::RunScreenZone "DeleteTask"). | `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1315` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L871` |
| 215 | OTHER Interface | MouseOverIcon | [Group mouseovericon] 2D/Once md100.00 v=7.00 | 2 | yes | Task-manager highlight moves to another screen zone / task tab (TaskManagerInterfaceMultiwinia::AdvanceScreenZones, TaskManagerInterface::AdvanceTab). | `code/shaman_interface.cpp:ShamanInterface::AdvanceMouseInIcon:L79` [LEGACY(Shaman interface)]<br>`code/shaman_interface.cpp:ShamanInterface::AdvanceSelectionInput:L108` [LEGACY(Shaman interface)]<br>`code/taskmanager_interface.cpp:TaskManagerInterface::AdvanceTab:L274`<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L983` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L994` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L1011` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L1102` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L555`<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L566`<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L583`<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L671` |
| 216 | OTHER CratePowerup | Squad | [Sample cratereward_squaddies4] 2D/Once md200.35 v=10.00 f=1.21 | 1 | yes | Squad crate reward deployed (Task::TargetSquad, Multiplayer branch) - announcer voice. | `code/taskmanager.cpp:Task::TargetSquad:L207 -> Crate::TriggerSoundEvent` |
| 217 | OTHER CratePowerup | Harvester | [Sample cratereward_harvester2] 2D/Once md100.00 v=9.13 | 1 | yes | Harvester deployed (Task::TargetHarvester). | `code/taskmanager.cpp:Task::TargetHarvester:L268 -> Crate::TriggerSoundEvent` |
| 218 | OTHER CratePowerup | Armour | [Sample cratereward_armour1] 2D/Once md100.00 v=9.06 | 1 | yes | Armour deployed (Task::TargetArmour). | `code/taskmanager.cpp:Task::TargetArmour:L248 -> Crate::TriggerSoundEvent` |
| 219 | OTHER CratePowerup | Darwinians | [Sample craterewrad_darwinians2] 2D/Once md100.00 v=10.00 | 1 | yes | Darwinians reward (Crate::RunDarwinians). | `code/worldobject/crate.cpp:Crate::RunDarwinians:L1976` |
| 220 | OTHER CratePowerup | AntNest | [Sample cratereward_antsnest1] 2D/Once md100.00 v=10.00 f=0.75 | 1 | yes | Ant nest reward (Crate::RunAntNest). | `code/worldobject/crate.cpp:Crate::RunAntNest:L1807` |
| 221 | OTHER CratePowerup | Eggs | [Sample cratereward_egg4] 2D/Once md199.97 v=9.55 | 1 | yes | Eggs reward (Crate::RunEggs). | `code/worldobject/crate.cpp:Crate::RunEggs:L1895` |
| 222 | OTHER CratePowerup | Airstrike | [Sample cratereward_airstrike1] 2D/Once md200.35 v=10.00 | 1 | yes | Air strike or napalm strike (Crate::RunAirStrike). | `code/worldobject/crate.cpp:Crate::RunAirStrike:L1488 (also NapalmStrike) -> Crate::TriggerSoundEvent` |
| 223 | OTHER CratePowerup | MagicalForest | [Sample cratereward_forest1] 2D/Once md200.00 v=9.00 f=0.51 | 1 | yes | Magical forest (Crate::RunMagicalForest). | `code/worldobject/crate.cpp:Crate::RunMagicalForest:L1716` |
| 224 | OTHER CratePowerup | DarkForest | [Sample cratereward_darkforest1] 2D/Once md200.00 v=9.52 | 1 | yes | Dark forest (Crate::RunDarkForest). | `code/worldobject/crate.cpp:Crate::RunDarkForest:L1747` |
| 225 | OTHER CratePowerup | CrateMania | [Sample cratereward_cratemania3] 2D/Once md200.00 v=9.55 f=0.81 | 1 | yes | Crate mania (Crate::RunCrateMania, team 255). | `code/worldobject/crate.cpp:Crate::RunCrateMania:L2102 (team 255)` |
| 226 | OTHER CratePowerup | GunTurret | [Sample cratereward_gunturret1] 2D/Once md200.00 v=9.16 | 1 | yes | Any turret reward: gun/rocket/flame (Crate::RunGunTurret). | `code/worldobject/crate.cpp:Crate::RunGunTurret:L1672 (GunTurret, RocketTurret and FlameTurret rewards)` |
| 227 | OTHER CratePowerup | Meteor | [Sample cratereward_meteor4] 2D/Once md200.00 v=9.39 | 1 | yes | Meteor shower (Crate::RunMeteorShower). | `code/worldobject/crate.cpp:Crate::RunMeteorShower:L1942` |
| 228 | OTHER CratePowerup | HotFeet | [Sample cratereward_hotfeet1] 2D/Once md100.00 v=9.06 | 1 | yes | Hot feet (Crate::RunHotFeet). | `code/worldobject/crate.cpp:Crate::RunHotFeet:L1640` |
| 229 | OTHER CratePowerup | Nuke | [Sample cratereward_nuke] 2D/Once md100.00 v=9.06 | 1 | yes | Nuke (Crate::RunNuke). | `code/worldobject/crate.cpp:Crate::RunNuke:L1546` |
| 230 | OTHER CratePowerup | Plague | [Sample cratereward_plague] 2D/Once md100.00 v=9.06 | 1 | yes | Plague (Crate::RunPlague). | `code/worldobject/crate.cpp:Crate::RunPlague:L1830` |
| 231 | OTHER CratePowerup | Randomiser | [Sample cratereward_randomiser] 2D/Once md100.00 v=9.00 | 1 | yes | Randomiser (Crate::RunRandomiser). | `code/worldobject/crate.cpp:Crate::RunRandomiser:L1992` |
| 232 | OTHER CratePowerup | PersonalShield | [Sample cratereward_shield] 2D/Once md100.00 v=10.00 | 1 | yes | Personal shield / booster (Crate::RunBooster). | `code/worldobject/crate.cpp:Crate::RunBooster:L1609` |
| 233 | OTHER CratePowerup | Subversion | [Sample cratereward_subversion2] 2D/Once md200.00 v=8.94 | 1 | yes | Subversion (Crate::RunSubversion). | `code/worldobject/crate.cpp:Crate::RunSubversion:L1578` |
| 234 | OTHER CratePowerup | SpawnMania | [Sample cratereward_spawnmania] 2D/Once md100.00 v=8.97 | 1 | yes | Spawn mania (Crate::RunSpawnMania, team 255). | `code/worldobject/crate.cpp:Crate::RunSpawnMania:L2064 (team 255)` |
| 235 | OTHER CratePowerup | Bitzkreig | [Sample cratereward_bitzkrieg] 2D/Once md100.00 v=9.00 | 1 | yes | Blitzkrieg reward (Crate::RunBitzkreig, team 255). | `code/worldobject/crate.cpp:Crate::RunBitzkreig:L2080 (team 255)` |
| 236 | OTHER CratePowerup | FormationBooster | [Sample cratereward_formation2] 2D/Once md100.00 v=9.00 | 1 | no(commented-out) | Never fired (reward code commented out) - dead data. | `code/worldobject/crate.cpp:Crate::RunFormtationIncrease:L2120 (inside /* */; GetName case commented out)` [commented-out] |
| 237 | OTHER CratePowerup | Rocketwinians | [Sample cratereward_rocketwinians] 2D/Once md100.00 v=9.10 | 1 | yes | Rocket Darwinians (Crate::RunRocketDarwinians, team 255). | `code/worldobject/crate.cpp:Crate::RunRocketDarwinians:L2134 (team 255)` |
| 238 | OTHER CratePowerup | FireExtinguisher | [Sample cratereward_extinguisher] 2D/Once md100.00 v=9.00 | 1 | no(commented-out) | Never fired (commented out) - dead data. | `code/taskmanager.cpp:Task::TargetExtinguisher:L585 (inside /* */; GetName case commented out)` [commented-out] |
| 239 | OTHER CratePowerup | Infection | [Sample cratereward_infection] 2D/Once md100.00 v=9.00 | 1 | yes | Infection (Crate::RunInfection). | `code/worldobject/crate.cpp:Crate::RunInfection:L1698` |
| 240 | OTHER CratePowerup | Rage | [Sample cratereward_rage4] 2D/Once md100.00 v=9.06 | 1 | yes | Rage (Crate::RunRage, team 255). | `code/worldobject/crate.cpp:Crate::RunRage:L2165 (team 255)` |
| 241 | OTHER CratePowerup | Tank | [Sample cratereward_tank] 2D/Once md100.00 v=8.97 | 1 | no(commented-out) | Never fired (commented out) - dead data. | `code/taskmanager.cpp:Task::TargetTank:L595 (inside /* */; GetName case commented out)` [commented-out] |
| 242 | OTHER CratePowerup | FlameTurret | [Sample cratereward_gunturret1] 2D/Once md100.00 v=9.03 | 1 | no(no-site) | Never fired: RunGunTurret always fires "GunTurret" - dead data. | — |
| 243 | OTHER CratePowerup | Triffids | [Sample cratereward_triffid] 2D/Once md100.00 v=9.10 | 1 | yes | Triffids (Crate::RunTriffids, team 255). | `code/worldobject/crate.cpp:Crate::RunTriffids:L2236 (team 255)` |
| 244 | OTHER CratePowerup | SpaceShip | [Sample cratereward_futurewinians] 2D/Once md100.00 v=10.00 | 1 | yes | Space ship (Crate::RunSpaceShip, team 255). | `code/worldobject/crate.cpp:Crate::RunSpaceShip:L2184 (team 255)` |
| 245 | OTHER MultiwiniaInterface | CountdownTick | [Sample countdownbeep3] 2D/Once md100.00 v=7.03 | 1 | yes | Pre-game countdown: first tick when all players ready, then each whole second of the grace period (Multiwinia::AdvanceGracePeriod); in-game timer at every whole minute and each of the last 10 s (Multiwinia::Render - render-driven). | `code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2385`<br>`code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2399`<br>`code/multiwinia.cpp:Multiwinia::Render:L3660` |
| 246 | OTHER MultiwiniaInterface | CountdownZeroTick | [Sample countdownbeep3] 2D/Once md100.00 v=7.10 f=2.00 | 1 | yes | Grace period reaches 0 (game starts) and timed game reaches 0 (Multiwinia::Render). | `code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2412`<br>`code/multiwinia.cpp:Multiwinia::Render:L3664` |
| 247 | OTHER MultiwiniaInterface | SelectionCircle | [Sample filtersweep] 2D/Once md100.00 v=7.00 r=0.58 \|\| [Sample droneloop] 2D/Loop md100.00 v=8.84 a=1.35 r=0.97 | 2 | yes | Player begins growing a selection circle (LocationInput::AdvanceTeamControl, not in grace period): sweep + looping drone, stopped by StopAllSounds "MultiwiniaInterface SelectionCircle" when the circle ends (location_input.cpp L739). | `code/location_input.cpp:LocationInput::AdvanceTeamControl:L707` |
| 248 | OTHER MultiwiniaInterface | LobbyAmbience | [Group newsamplegroup8] 2D/LoopADSR md100.00 v=8.00 a=9.29 s=10.65 r=10.84 \|\| [Sample shipheartbeat1] 2D/Loop md200.10 v=8.50 f=0.50 \|\| [Group multilobbyambiences] 2D/LoopADSR md100.00 v=9.00 f=rnd 0.67..1.01/once s=10.65 r=10.26 | 14 | yes | Front-end ambience, started once by GameMenuWindow::Update when sounds are ready; stopped when MainMenuLoop exits. | `code/UI/GameMenuWindow.cpp:GameMenuWindow::Update:L507`<br>`code/app.cpp:App::CheckSounds:L526` [commented-out] |
| 249 | OTHER MultiwiniaInterface | MenuRollOver | [Group mouseovericon] 2D/Once md100.00 v=5.03 | 2 | yes | Mouse moves onto a menu button/control (GameMenuButton/InputField/DropDown/CheckBox::Render, FancyColourButton, LevelSelectButton, leaderboards). | `code/UI/FancyColourButton.h:FancyColourButton::Render:L231`<br>`code/UI/LevelSelectButton.h:LevelSelectButton::Render:L328`<br>`code/game_menu.cpp:GameMenuButton::Render:L361`<br>`code/game_menu.cpp:GameMenuInputField::Render:L1339`<br>`code/game_menu.cpp:GameMenuDropDown::Render:L1722`<br>`code/game_menu.cpp:GameMenuCheckBox::Render:L1942`<br>`code/interface/leaderboards_window.cpp:?:L146` |
| 250 | OTHER MultiwiniaInterface | MenuSelect | [Sample menuscroll2] 2D/Once md100.00 v=9.00 | 1 | yes | Menu button activated (GameMenuButton::MouseUp and option windows). | `code/UI/LevelSelectButton.h:LevelSelectButton::MouseUp:L25`<br>`code/UI/MultiwiniaEditorButton.h:MultiwiniaEditorButton::MouseUp:L23`<br>`code/game_menu.cpp:GameMenuButton::MouseUp:L222`<br>`code/interface/helpandoptions_windows.cpp:?:L33`<br>`code/interface/helpandoptions_windows.cpp:?:L50`<br>`code/interface/helpandoptions_windows.cpp:?:L69`<br>`code/interface/helpandoptions_windows.cpp:HelpAndOptionsWindow::Create:L258`<br>`code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L368`<br>`code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L389`<br>`code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L410`<br>`code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L430`<br>`code/interface/mainmenus.cpp:ConfirmExitWindow::Create:L745`<br>`code/interface/multiwinia_window.cpp:?:L153`<br>`code/interface/multiwinia_window.cpp:?:L171` |
| 251 | OTHER MultiwiniaInterface | MenuCancel | [Sample menuscroll1] 2D/Once md100.00 v=8.00 | 1 | yes | Back/close button (BackPageButton, MenuCloseButton, GameMenuButton cancel). | `code/game_menu.cpp:GameMenuButton::MouseUp:L226`<br>`code/game_menu.cpp:BackPageButton::MouseUp:L377`<br>`code/interface/mainmenus.h:MenuCloseButton::MouseUp:L114` |
| 252 | OTHER MultiwiniaInterface | UnitHighlight | [Group unithighlight] Stat/Once md100.00 v=7.00 f=0.90 | 2 | yes | Cursor highlight moves to a new object, max every 0.2 s, 3D at the object (GameCursor::GetHighlightedObject). | `code/gamecursor.cpp:GameCursor::GetHighlightedObject:L316` |
| 253 | OTHER MultiwiniaInterface | SelectUnit | [Sample mouseclick1] 2D/Once md100.00 v=6.97 f=0.33 | 1 | yes | Unit selected by clicking, max every 0.5 s (LocationInput::SelectObjectUnderMouse). | `code/location_input.cpp:LocationInput::SelectObjectUnderMouse:L313` |
| 254 | OTHER MultiwiniaInterface | MouseError | [Sample beep_harsh_malfunction] 2D/Once md100.00 v=8.55 f=0.62 | 1 | yes | Invalid click: click on nothing while something is selected, or right-click while placing a task (LocationInput). | `code/location_input.cpp:LocationInput::SelectObjectUnderMouse:L338`<br>`code/location_input.cpp:LocationInput::AdvanceTeamControl:L832` |
| 255 | OTHER MultiwiniaInterface | ToggleUnitState | [Sample mouseclick1] 2D/Once md100.00 v=9.03 f=0.58 | 1 | yes | Click on own selected Armour (toggle deploy/conversion) or Officer (cycle mode) (LocationInput::AdvanceTeamControl). | `code/location_input.cpp:LocationInput::AdvanceTeamControl:L512`<br>`code/location_input.cpp:LocationInput::AdvanceTeamControl:L526` |
| 256 | OTHER MultiwiniaInterface | ChatMessage | [Sample chat] 2D/Once md100.00 v=8.00 | 1 | yes | Chat line received in lobby (GameMenuWindow::NewChatMessage, not for PLAYERJOIN) or in game (Location::NewChatMessage). | `code/UI/GameMenuWindow.cpp:GameMenuWindow::NewChatMessage:L1374`<br>`code/location.cpp:Location::NewChatMessage:L4965` |
<!-- X2B:EVENTS:END -->

Totals: 182 of 256 pairs are live in FULL; 30 are on live code paths but their object type never exists in Multiwinia (Darwinia buildings/LaserTroop/Tripod); 3 are Tank (debug-only); 41 are never fired (17 with LEGACY-only sites, 3 LEGACY + commented-out, 3 commented-out only, 10 not compiled, 7 with no call site at all, 1 DEAD) [C] `X2b_sound_events.csv`.

## [→ 10_audio_design] 10.D Events fired by code that have no data (silent in the shipped game)

These are the events the game asks for and the data does not answer (213 object-type × event pairs, aggregated by group + event; types listed are those that have a blueprint in `sounds.txt` — types with no blueprint at all are silent for every event: ENTITY `Lander`, `AI`, `Shaman`, `Tentacle`, `DropShip`; BUILDING `Factory`, `Cave`, `ControlTower`, `Bridge`, `Wall`, `PylonStart`, `PylonEnd`, `PowerSplitter`, `TrackJunction`, `TrackStart`, `TrackEnd`, `DisplayScreen`, `UpgradePort`, `PrimaryUpgrade`, `SafeArea`, `ReceiverLink`, `SpiritSpawner`, `SpawnPopulationLock`, `SpawnPointMaster`, `SpawnLink`, `AITarget`, `AISpawnPoint`, `ScriptTrigger`, `StaticShape`, `FenceSwitch`, `DynamicHub`, `DynamicNode`, `FeedingTube`, `ChessPiece`, `ChessBase`, `CloneLab`, `Portal`, `WallBuster`, `RestrictionZone`, `JumpPad`, `AIObjective`, `AIObjectiveMarker`, `EruptionMarker`, `SmokeMarker` (39 of the 74 building type names in `code/worldobject/building.cpp:Building::GetTypeName`, not counting `Invalid`; the other 35 have blueprints). Notable gameplay-relevant gaps for the new game: **match end** (`GameOver`, `YouWon`, `YouLost` — samples `data/mwsounds/GameOver1.ogg`, `GameOver2.ogg`, `YouWon.ogg`, `YouLost2.ogg` exist but are unbound), crate `Expired`, `Capturing_100pc`, every laser `Create`/`HitBuilding`/`KilledEntity`, squad `ThrowGrenade` (the only reachable squad secondary), turret `TargetSighted`/`FireRocket`, `LaserFence Electrocute`, statue `Lift`/`Carry` (the code even stops `"Statue Carry"` on drop), `Armour LoadDarwinian`/`UnloadDarwinian`, Officer `SetOrderFollow`/`SetOrderAbsorb`/`SetOrderNone`, `Spider PounceLand`, `Centipede Grow`/`Panic`/`EnemySighted`, `SoulDestroyer Panic`, `Triffid LaunchEgg`, `TrunkPort PowerUp`, task-manager `SelectTask`/`TaskManagerSelectTask`/`TaskManagerDeselectTask`, `Interface SetOrderGoto`, `Ambience ExitLocation`.

<!-- X2B:CODEONLY:BEGIN -->
| # | Group | Event (fired, no data -> silent) | Object types (with a blueprint in sounds.txt) | Fired in FULL | Meaning | Code site(s) |
|---|---|---|---|---|---|---|
| 1 | OTHER | SetOrderGoto | Interface | yes | Player sends selected Darwinians somewhere (LocationInput::AdvanceTeamControl) - no data (the Darwinians' own GivenOrdersFromPlayer voice plays). | `code/location_input.cpp:LocationInput::AdvanceTeamControl:L690` |
| 2 | OTHER | ExitLocation | Ambience | yes | Fired after the match loop exits (LocationGameLoop) - no data. | `code/main.cpp:LocationGameLoop:L609` |
| 3 | OTHER | GameOver | MultiwiniaInterface | yes | Multiwinia::DeclareWinner - no data: the match end is silent. | `code/multiwinia.cpp:Multiwinia::DeclareWinner:L4233` |
| 4 | OTHER | YouWon | MultiwiniaInterface | yes | Multiwinia::DeclareWinner for the local winner - no data. | `code/multiwinia.cpp:Multiwinia::DeclareWinner:L4237` |
| 5 | OTHER | YouLost | MultiwiniaInterface | yes | Multiwinia::DeclareWinner for losers - no data. | `code/multiwinia.cpp:Multiwinia::DeclareWinner:L4241` |
| 6 | OTHER | ShowLogo | Interface | no(LEGACY) | Script ShowDarwinLogo - legacy, no data. | `code/script.cpp:Script::RunCommand_ShowDarwinLogo:L551` [LEGACY(script command)] |
| 7 | OTHER | GestureEnd | Gesture | no(LEGACY) | Darwinia gesture end - legacy, no data. | `code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L199` [LEGACY(TaskManagerInterfaceGestures)] |
| 8 | OTHER | SelectTask | Interface | yes | Task selected in the task manager - no data. | `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1293` [LEGACY(TaskManagerInterfaceIcons)]<br>`code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L848` |
| 9 | ENTITY | Selected | Armour, Engineer, Harvester | yes | Team::SelectUnit of an entity in the team's m_others list by the local player (click-select, LocationInput::SelectObjectUnderMouse) - no data for this type: silent. | `code/team.cpp:Team::SelectUnit:L215` |
| 10 | OTHER | TaskManagerDeselectTask | Interface | yes | Team::SelectUnit(-1,-1,-1) - no data. | `code/team.cpp:Team::SelectUnit:L244` |
| 11 | OTHER | TaskManagerSelectTask | Interface | yes | Team::SelectUnit of something - no data. | `code/team.cpp:Team::SelectUnit:L248` |
| 12 | ENTITY | LoseHealth | SpaceInvader, LaserTroop, TriffidEgg, Nuke, Meteor, Harvester | site-live/type-LEGACY/yes | ChangeHealth damage - no data for this type: silent. | `code/worldobject/airstrike.cpp:SpaceInvader::ChangeHealth:L304`<br>`code/worldobject/entity.cpp:Entity::ChangeHealth:L182`<br>`code/worldobject/harvester.cpp:Harvester::ChangeHealth:L165` |
| 13 | ENTITY | Die | SpaceInvader, Nuke, Meteor, Harvester | yes | ChangeHealth death - no data for this type: silent. | `code/worldobject/airstrike.cpp:SpaceInvader::ChangeHealth:L311`<br>`code/worldobject/entity.cpp:Entity::ChangeHealth:L189`<br>`code/worldobject/harvester.cpp:Harvester::ChangeHealth:L172` |
| 14 | ENTITY | LoadDarwinian | Armour | yes | Darwinian boards an Armour (Armour::AddPassenger) - no data. | `code/worldobject/armour.cpp:Armour::AddPassenger:L1646` |
| 15 | ENTITY | UnloadDarwinian | Armour | yes | Darwinian leaves an Armour (Armour::RemovePassenger) - no data. | `code/worldobject/armour.cpp:Armour::RemovePassenger:L1655` |
| 16 | BUILDING | Create | RadarDish, TrunkPort, Generator, Pylon, SolarPanel, TrackLink, Mine, Incubator, AntHill, Triffid, SpawnPoint, FuelGenerator, FuelPipe, FuelStation, EscapeRocket, Crate | site-live/type-LEGACY/yes | Building::Initialise - no data for this type: silent. | `code/worldobject/building.cpp:Building::Initialise:L157` |
| 17 | BUILDING | ReprogramComplete | LaserFence, GunTurret, Powerstation, Tree, ResearchItem, Library, Pylon, SolarPanel, TrackLink, Refinery, Mine, Yard, Incubator, AntHill, Triffid, SpiritReceiver, SpiritProcessor, SpawnPoint, BlueprintStore, BlueprintConsole, BlueprintRelay, Spam, GodDish, FuelGenerator, FuelPipe, FuelStation, EscapeRocket, MultiwiniaZone, ControlStation, Crate, Statue, PulseBomb | no(LEGACY) | Building::ReprogramComplete - legacy; silent. | `code/worldobject/building.cpp:Building::ReprogramComplete:L272` [LEGACY(Building::ReprogramComplete only via ControlTower::Reprogram / script)] |
| 18 | BUILDING | ChangeTeam | RadarDish, LaserFence, GunTurret, Powerstation, Tree, TrunkPort, ResearchItem, Library, Generator, Pylon, SolarPanel, TrackLink, Refinery, Mine, Yard, Incubator, AntHill, Triffid, SpiritReceiver, SpiritProcessor, SpawnPoint, BlueprintStore, BlueprintConsole, BlueprintRelay, Spam, GodDish, FuelGenerator, FuelPipe, FuelStation, EscapeRocket, ControlStation, Crate, PulseBomb | site-live/type-LEGACY/yes | Building::SetTeamId on owner change - no data for this type: silent. | `code/worldobject/building.cpp:Building::SetTeamId:L293` |
| 19 | BUILDING | Damage | RadarDish, Powerstation, Tree, TrunkPort, ResearchItem, Library, Generator, Pylon, SolarPanel, TrackLink, Refinery, Mine, Yard, Incubator, SpiritReceiver, SpiritProcessor, SpawnPoint, BlueprintStore, BlueprintConsole, BlueprintRelay, GodDish, FuelGenerator, FuelPipe, FuelStation, MultiwiniaZone, ControlStation, Crate, Statue, PulseBomb | site-live/type-LEGACY/yes | Building::Damage (explosions, fire, lasers) - no data for this type: silent. | `code/worldobject/building.cpp:Building::Damage:L582` |
| 20 | BUILDING | Explode | RadarDish, LaserFence, GunTurret, Powerstation, Tree, TrunkPort, ResearchItem, Library, Generator, Pylon, SolarPanel, TrackLink, Refinery, Mine, Yard, Incubator, Triffid, SpiritReceiver, SpiritProcessor, SpawnPoint, BlueprintStore, BlueprintConsole, BlueprintRelay, GodDish, FuelGenerator, FuelPipe, FuelStation, MultiwiniaZone, ControlStation, Crate | no(LEGACY) | Building::Destroy - script-only (legacy); silent. | `code/worldobject/building.cpp:Building::Destroy:L597` [LEGACY(Building::Destroy only via Script::RunCommand_DestroyBuilding)] |
| 21 | BUILDING | Lift | Statue | yes | CarryableBuilding lifted - no data: silent. | `code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance:L240` |
| 22 | BUILDING | Carry | Statue | yes | CarryableBuilding lifted (with Lift) - no data: silent. | `code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance:L241` |
| 23 | ENTITY | Panic | Centipede, SoulDestroyer | yes | Centipede/SoulDestroyer starts panicking - no data. | `code/worldobject/centipede.cpp:Centipede::Panic:L189`<br>`code/worldobject/souldestroyer.cpp:SoulDestroyer::Panic:L304` |
| 24 | ENTITY | Grow | Centipede | yes | Centipede grows a segment (Centipede::EatSpirits) - no data. | `code/worldobject/centipede.cpp:Centipede::EatSpirits:L520` |
| 25 | ENTITY | EnemySighted | Centipede | yes | Centipede acquires a target - no data: silent. | `code/worldobject/centipede.cpp:Centipede::SearchForTargetEnemy:L582` |
| 26 | BUILDING | Capturing_100pc | Crate | yes | Crate capture reaches 100 % - no data: silent. | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L623` |
| 27 | BUILDING | Capturing_%pc(bug) | Crate | yes | Decay branch builds "Capturing_%pc" with %p (pointer format) -> e.g. "Capturing_00000003c": never matches data, so falling progress is silent (bug). | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L677` |
| 28 | BUILDING | Expired | Crate | yes | Uncaptured crate decays after 60 s (Crate::AdvanceOnGround) - no data: silent (docs requested it). | `code/worldobject/crate.cpp:Crate::AdvanceOnGround:L711` |
| 29 | ENTITY | TakenControl | Darwinian | yes | Darwinian taken over by a controller (Darwinian::TakeControl) - no data. | `code/worldobject/darwinian.cpp:Darwinian::TakeControl:L3531` |
| 30 | ENTITY | EscapedControl | Darwinian | yes | Darwinian released from controller (Darwinian::LoseControl) - no data. | `code/worldobject/darwinian.cpp:Darwinian::LoseControl:L3549` |
| 31 | ENTITY | DropSpirit | Engineer | yes | Engineer delivers a spirit to a spawn point (Engineer::AdvanceToIncubator) - no data: silent. | `code/worldobject/engineer.cpp:Engineer::AdvanceToIncubator:L767` |
| 32 | ENTITY | ReprogrammingComplete | Engineer | no(LEGACY) | Engineer finishes reprogramming - legacy, no data. | `code/worldobject/engineer.cpp:Engineer::AdvanceResearching:L879` [LEGACY(needs ResearchItem)]<br>`code/worldobject/engineer.cpp:Engineer::AdvanceReprogramming:L946` [LEGACY(needs ControlTower)] |
| 33 | ENTITY | EnterTeleport | LaserTroop, Darwinian | site-live/type-LEGACY/yes | Entity::EnterTeleports - no data for this type: silent. | `code/worldobject/entity.cpp:Entity::EnterTeleports:L278`<br>`code/worldobject/entity.cpp:Entity::EnterTeleports:L295` |
| 34 | ENTITY | Create | LaserTroop, Virii, Egg | site-live/type-LEGACY/yes | Entity::Begin - no data for this type: silent. | `code/worldobject/entity.cpp:Entity::Begin:L379` |
| 35 | BUILDING | Enable | Generator | site-live/type-LEGACY | Generator/DynamicHub reprogrammed - legacy; silent. | `code/worldobject/generator.cpp:Generator::ReprogramComplete:L378` |
| 36 | BUILDING | TargetSighted | GunTurret | yes | GunTurret acquires a new target (GunTurret::SearchForTargets) - no data: silent. | `code/worldobject/gunturret.cpp:GunTurret::SearchForTargets:L327` |
| 37 | BUILDING | FireGrenade | GunTurret | no(UNREACHABLE) | Mortar turret fire - no data, no mortar turret in MW. | `code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L579` [UNREACHABLE(no Mortar turret is ever created in MW)] |
| 38 | BUILDING | FireRocket | GunTurret | yes | Rocket turret fire (RocketTurret crate) - no data: silent (the Rocket projectile has its own Create sound). | `code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L580` |
| 39 | ENTITY | Attack | Squadie | yes | Attack event from the type's own attack code (Squadie::Attack fires it with FireLaser) - no data for this type: silent. | `code/worldobject/insertion_squad.cpp:Squadie::Attack:L953` |
| 40 | ENTITY | ThrowGrenade | Squadie | yes | Squad throws its grenade (the only MW secondary) - no data: silent (the grenade itself has Create/Bounce/Explode). | `code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L978` |
| 41 | ENTITY | ThrowController | Squadie | no(LEGACY) | Squad controller secondary - legacy, no data. | `code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L993` [LEGACY(squad secondary is always Grenade in MW)] |
| 42 | ENTITY | FireRocket | Squadie | no(LEGACY) | Squad rocket secondary - legacy, no data. | `code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L999` [LEGACY(squad secondary is always Grenade in MW)] |
| 43 | BUILDING | Electrocute | LaserFence | yes | Entity touches an active fence (LaserFence::Electrocute) - no data: silent. | `code/worldobject/laserfence.cpp:LaserFence::Electrocute:L568` |
| 44 | ENTITY | SetOrderFollow | Officer | yes | Officer switched to follow mode - no data. | `code/worldobject/officer.cpp:Officer::SetOrders:L1400`<br>`code/worldobject/officer.cpp:Officer::SetNextMode:L1715`<br>`code/worldobject/officer.cpp:Officer::SetPreviousMode:L1878` |
| 45 | ENTITY | SetOrderNone | Officer | yes | Officer orders cleared - no data. | `code/worldobject/officer.cpp:Officer::SetNextMode:L1711`<br>`code/worldobject/officer.cpp:Officer::SetPreviousMode:L1874` |
| 46 | ENTITY | SetOrderAbsorb | Officer | yes | Officer switched to absorb mode - no data. | `code/worldobject/officer.cpp:Officer::SetNextMode:L1714`<br>`code/worldobject/officer.cpp:Officer::SetPreviousMode:L1877` |
| 47 | ENTITY | ExitTeleport | Darwinian, Officer | yes | RadarDish::UpdateEntityInTransit for an entity arriving at the far dish (entities that use Entity::EnterTeleports in MW: Squadie, Officer, Darwinian) - no data for this type: silent. | `code/worldobject/radardish.cpp:RadarDish::UpdateEntityInTransit:L705` |
| 48 | BUILDING | Refueling | EscapeRocket | yes | EscapeRocket refuelling bed (SetupSounds) - no data: silent. | `code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L957` |
| 49 | ENTITY | PounceLand | Spider | yes | Spider lands after a pounce - no data. | `code/worldobject/spider.cpp:Spider::AdvancePouncing:L525` |
| 50 | OTHER | Create | Spirit, Laser | no(commented-out)/yes | Spirit::Begin (commented out) / Laser::Initialise (every laser bolt) - no data: silent. | `code/worldobject/spirit.cpp:Spirit::Begin:L65` [commented-out]<br>`code/worldobject/weapons.cpp:Laser::Initialise:L599` |
| 51 | OTHER | BeginAscent | Spirit | no(commented-out) | Commented out in Spirit::Advance. | `code/worldobject/spirit.cpp:Spirit::Advance:L123` [commented-out] |
| 52 | OTHER | Dropped | Spirit | no(commented-out) | Commented out in Spirit::CollectorDrops. | `code/worldobject/spirit.cpp:Spirit::CollectorDrops:L298` [commented-out] |
| 53 | OTHER | PlacedInEgg | Spirit | no(commented-out) | Commented out in Spirit::InEgg. | `code/worldobject/spirit.cpp:Spirit::InEgg:L313` [commented-out] |
| 54 | OTHER | EggDestroyed | Spirit | no(commented-out) | Commented out in Spirit::EggDestroyed. | `code/worldobject/spirit.cpp:Spirit::EggDestroyed:L324` [commented-out] |
| 55 | BUILDING | TriggerSpirit | SpiritReceiver, SpiritProcessor | site-live/type-LEGACY | ReceiverBuilding spirit pulse - legacy; silent. | `code/worldobject/spiritreceiver.cpp:ReceiverBuilding::TriggerSpirit:L235` |
| 56 | BUILDING | LaunchEgg | Triffid | yes | Triffid launches an egg (Triffid::Launch) - no data: silent. | `code/worldobject/triffid.cpp:Triffid::Launch:L340` |
| 57 | BUILDING | PowerUp | TrunkPort | yes | TrunkPort goes online (TrunkPort::Advance) - no data: silent. | `code/worldobject/trunkport.cpp:TrunkPort::Advance:L116` |
| 58 | OTHER | HitBuilding | Laser | yes | Laser hits a building (Laser::Advance) - no data. | `code/worldobject/weapons.cpp:Laser::Advance:L678` |
| 59 | OTHER | HitEntity | Laser | no(commented-out) | Commented out. | `code/worldobject/weapons.cpp:Laser::Advance:L730` [commented-out] |
| 60 | OTHER | KilledEntity | Laser | yes | Laser/turret shell kills an entity (Laser::Advance, TurretShell::Advance) - no data. | `code/worldobject/weapons.cpp:Laser::Advance:L765`<br>`code/worldobject/weapons.cpp:TurretShell::Advance:L1415` |
| 61 | OTHER | Bounce | AirStrikeBomb | yes | Air-strike bomb bounces (ThrowableWeapon::Advance) - no AirStrikeBomb Bounce data: silent. | `code/worldobject/weapons.cpp:ThrowableWeapon::Advance:L122 -> TriggerSoundEvent (bomb Grenade with m_type=EffectThrowableAirstrikeBomb)` |
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
| `data/sounds/tresk_-_excuses.ogg` | 344.555 | 2510 | 65 | Cutscene1 (Loop), Cutscene5 (Once, attack 3.10 s) | Darwinia cutscene scripts `music1.txt`, `containment_intro.txt`, `music5.txt`, `biosphere_intro.txt` | [B:LEGACY], not shipped |
| `data/sounds/tresk_-_faces-of-a-fashion.ogg` | 278.648 | 2066 | 65 | Cutscene2 (Loop), EndSequence (Once, attack 5.23 s), Demo2Mid (Once) | `music2.txt`, `containment_end.txt`, `gameover.txt`, `launchpad_purpose.txt` | [B:LEGACY], not shipped |
| `data/sounds/trash80_-_pain-fade-down.ogg` | 213.943 | 1533 | 65 | Cutscene3 (Loop, attack 5.23 s), Credits (Once, attack 2.71 s), Demo2Intro (Once) | `music3.txt`, `yard_intro.txt`, `launchpad_intro.txt`, credits loader | [B:LEGACY], not shipped |
| `data/sounds/tresk_-_impact-of-silence.ogg` | 172.130 | 982 | 65 | Cutscene4 (Loop, attack 2.90 s) | `music4.txt`, `yard_end.txt` | [B:LEGACY], not shipped |
| `data/sounds/tresk_-weeklies-_schroeders-failure.ogg` | 103.579 | 485 | 65 | LoaderSoul (Loop) | soul loader | [B:DEAD], not shipped |
| `data/sounds/dma_sc-visitors_from_dreams.ogg` | 325.251 | 2285 | 65 | LoaderRaytrace (Loop), StartSequence (Once) | raytrace loader, first-run boot sequence | [B:DEAD], not shipped |
| `data/sounds/twoataris.ogg` | 87.484 | 848 | 80 (libVorbis 20030909) | LoaderGameOfLife (Once) | game-of-life loader | [B:DEAD], not shipped |
| `data/sounds/pang.ogg` | 87.121 | 626 | 60 (aoTuV b3) | LoaderAmiga (Once) | Amiga loader | [B:DEAD], not shipped |
| `data/sounds/speccyloader.ogg` | 58.979 | 235 | 65 | LoaderSpeccy (Once) | ZX Spectrum loader | [B:DEAD], not shipped |
| `data/sounds/matrix.ogg` | 24.000 | 188 | 65 | LoaderMatrix (Once) | matrix loader | [B:DEAD], not shipped |
| `data/sounds/fodder.ogg` | 6.315 | 31 | 33 (22050 Hz) | LoaderFodder (Once) | fodder loader (retro-game homage [I]) | [B:DEAD], not shipped |

(Total 1702.0 s, 12 070 545 bytes; all 44100 Hz mono except `fodder.ogg` 22050 Hz; encoder libVorbis I 20040920 unless noted.)

Music envelope data [C] `data/sounds.txt` `OTHER Music`: VOLUME 9.00–10.00; ATTACK on Cutscene3 5.23 s, Cutscene4 2.90 s, Cutscene5 3.10 s, EndSequence 5.23 s, Credits 2.71 s; **RELEASE** on 14 of the 17 blocks — Cutscene1/2/3/4 20.13 s, Demo2Mid 20.71 s, Cutscene5 15.48 s, LoaderGameOfLife 10.26 s, LoaderRaytrace/LoaderSoul/Credits/StartSequence/LoaderAmiga 10.06 s, LoaderMatrix 7.55 s, EndSequence 3.29 s; none on LoaderFodder, LoaderSpeccy, Demo2Intro. Because a music change calls `BeginRelease(true)` on the old track and the new one starts only when the old reports finished (P16 §27.10), the authored design is a 3.3–20.7 s fade-out followed by the new track's attack — this corrects P16 §27.10's "all shipped music entries have RELEASE 0 → immediate cut".

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

## [→ 10_audio_design] 10.K Live content per sound source (scope of a replacement asset set)

Distinct files that can actually play in a FULL Multiwinia match, per source (events fired in FULL only) [C] `X2b_sound_events.csv` ∩ `X2b_samples.csv`. 898 files in total.

| Source | Live events | Files | Duration s |
|---|---|---|---|
| ENTITY Engineer | 3 | 15 | 31.3 |
| ENTITY Virii | 4 | 47 | 11.0 |
| ENTITY Squadie | 7 | 48 | 40.3 |
| ENTITY SporeGenerator | 4 | 25 | 77.2 |
| ENTITY Centipede | 4 | 21 | 48.8 |
| ENTITY SpaceInvader | 2 | 2 | 7.9 |
| ENTITY Spider | 7 | 53 | 89.2 |
| ENTITY Darwinian | 13 | 200 | 173.3 |
| ENTITY Officer | 7 | 55 | 71.4 |
| ENTITY ArmyAnt | 4 | 27 | 6.7 |
| ENTITY Armour | 4 | 30 | 129.4 |
| ENTITY SoulDestroyer | 5 | 47 | 99.1 |
| ENTITY TriffidEgg | 4 | 30 | 64.1 |
| ENTITY Harvester | 2 | 9 | 17.7 |
| ENTITY Nuke | 2 | 3 | 20.5 |
| ENTITY Meteor | 2 | 4 | 17.7 |
| ENTITY SpaceShip | 2 | 13 | 171.9 |
| BUILDING RadarDish | 4 | 11 | 16.1 |
| BUILDING LaserFence | 3 | 23 | 31.0 |
| BUILDING GunTurret | 4 | 23 | 50.3 |
| BUILDING Tree | 5 | 41 | 180.4 |
| BUILDING Pylon | 1 | 22 | 8.1 |
| BUILDING SolarPanel | 2 | 6 | 6.5 |
| BUILDING AntHill | 2 | 15 | 54.8 |
| BUILDING Triffid | 2 | 9 | 74.4 |
| BUILDING SpawnPoint | 2 | 2 | 17.3 |
| BUILDING FuelGenerator | 2 | 10 | 33.3 |
| BUILDING FuelPipe | 1 | 1 | 4.2 |
| BUILDING FuelStation | 1 | 5 | 3.6 |
| BUILDING EscapeRocket | 7 | 29 | 132.7 |
| BUILDING MultiwiniaZone | 3 | 6 | 18.8 |
| BUILDING ControlStation | 1 | 4 | 30.7 |
| BUILDING Crate | 10 | 18 | 188.4 |
| BUILDING Statue | 3 | 4 | 11.2 |
| BUILDING PulseBomb | 1 | 6 | 36.1 |
| OTHER Laser | 1 | 36 | 16.0 |
| OTHER Grenade | 4 | 17 | 39.8 |
| OTHER Rocket | 2 | 6 | 22.4 |
| OTHER AirStrikeBomb | 2 | 6 | 30.7 |
| OTHER Spirit | 1 | 1 | 2.0 |
| OTHER Gesture | 2 | 2 | 5.8 |
| OTHER Ambience | 1 | 11 | 315.0 |
| OTHER Interface | 2 | 3 | 2.6 |
| OTHER CratePowerup | 25 | 25 | 121.9 |
| OTHER MultiwiniaInterface | 12 | 26 | 394.8 |

(Files shared between sources are counted once per source; column sums exceed 898.)

## [→ 25_textures_and_assets] 25.A Audio assets in the asset pipeline

| Asset class | Location | Format (verified) | Loader / code site | Consumer | Packed in |
|---|---|---|---|---|---|
| Sound samples | `data/sounds/*.ogg` (954), `data/mwsounds/*.ogg` (378) | Ogg Vorbis I, 1 channel, 44100 Hz (2 files 22050 Hz), 33–128 kbps; mono is mandatory (`CachedSample::Read` release-asserts "not mono") | `code/sound/sample_cache.cpp:CachedSample::CachedSample` → `code/lib/resource.cpp:Resource::GetSoundStreamDecoder` → `code/sound/sound_stream_decoder.cpp:SoundStreamDecoder` (libvorbisfile) | `SoundInstance` via `CachedSampleManager` (lazy decode, 60 MiB LRU) | `menusounds.dat` (18), `sounds.dat` (1297), `sounds2.dat` (6); 11 not shipped |
| Sound bindings | `data/sounds.txt` (162 804 bytes, CRLF) | text, grammar P16 §10.3 / X1b | `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints` via `Resource::GetTextReader("sounds.txt")` | `SoundSystem` blueprints | `main.dat` (`*.txt`) |
| DSP descriptors | `data/effects.txt` | text | `SoundSystem::LoadEffects` | DSP (off by default) | `main.dat` |
| Packaging manifests | `tools/sounds.txt` (1297 lines), `tools/menu_sounds.txt` (18 lines), `tools/multiwinia_exclusions.txt` | text, CRLF, paths relative to `data/` | `tools/build_multiwinia_data.bat` (`rar a -m2 -s -idp`) | build only | — |
| Checking tools | `tools/unused_samples.sh`, `tools/unused_sample_groups.sh` | bash | run by hand from `data/` | content QA | — |
| Editor billboard | `data/sprites/sound.bmp` | bitmap (X2a owns) | `code/interface/soundeditor_window.cpp:SoundEditorWindow::Render` [B:DEBUG] | sound editor | `main.dat` |

Naming rules a new pipeline must preserve or replace: names are case-insensitive, extension-less, unique across both folders, space-free (the editor rejects spaces, `soundsystem.cpp:IsSoundSourceOK` in `_DEBUG` only); there is no metadata sidecar (no loop points, no per-file gain) — looping, gain, pitch and envelopes live entirely in `sounds.txt`.

## [→ 25_textures_and_assets] 25.B Per-directory audio catalogue (every file)

Every one of the 1332 files, in directory order (case-insensitive sort). Duration from the last Ogg granule; size in KiB (1024 bytes); full per-file metadata (channels, rate, bytes, kbps, encoder, decoded size, complete event list) in `_work/drafts/X2b_samples.csv` [C].

<!-- X2B:SAMPLES:BEGIN -->
### `data/sounds/` (954 files)

Columns: duration s; size KiB; archive (`menusounds.dat` / `sounds.dat` / `sounds2.dat` / NOT-SHIPPED(exclusion pattern)); status (live = referenced by an event fired in FULL, dead = referenced only by unreachable/never-fired events, unused = no blueprint reference); referenced by (sample group -> events).

| Path | s | KiB | Archive | Status | Referenced by |
|---|---|---|---|---|---|
| `data/sounds/ablaster.ogg` | 0.384 | 6 | sounds.dat | dead | Library Create |
| `data/sounds/airstrike_alarm.ogg` | 0.946 | 10 | sounds.dat | dead | Squadie ThrowAirStrike |
| `data/sounds/airstrike_boom1.ogg` | 5.875 | 49 | sounds.dat | live | `airstrikeexplode` -> AirStrikeBomb Explode |
| `data/sounds/airstrike_boom2.ogg` | 5.898 | 48 | sounds.dat | live | `airstrikeexplode` -> AirStrikeBomb Explode |
| `data/sounds/airstrike_boom3.ogg` | 5.991 | 49 | sounds.dat | live | `airstrikeexplode` -> AirStrikeBomb Explode |
| `data/sounds/airstrike_boom4.ogg` | 5.921 | 49 | sounds.dat | live | `airstrikeexplode` -> AirStrikeBomb Explode |
| `data/sounds/airstrike_boom5.ogg` | 6.014 | 49 | sounds.dat | live | `airstrikeexplode` -> AirStrikeBomb Explode |
| `data/sounds/airstrike_drop.ogg` | 4.000 | 32 | sounds.dat | live | SpaceInvader DropGrenade |
| `data/sounds/airstrike_hover2.ogg` | 3.878 | 22 | sounds.dat | live | SpaceInvader Create |
| `data/sounds/alien_lands1.ogg` | 17.740 | 137 | sounds.dat | dead | `worldamb` -> Ambience EnterGlobalWorld |
| `data/sounds/alien_lands3.ogg` | 30.465 | 204 | menusounds.dat | live | `newsamplegroup8`, `worldamb` -> Ambience EnterGlobalWorld; Ambience EnterLocation; MultiwiniaInterface LobbyAmbience |
| `data/sounds/altitude1.ogg` | 91.304 | 587 | sounds.dat | dead | Ambience EnterGlobalWorld |
| `data/sounds/ant_attack.ogg` | 0.145 | 4 | sounds.dat | live | `antatack` -> ArmyAnt Attack |
| `data/sounds/ant_attack2.ogg` | 0.123 | 4 | sounds.dat | live | `antatack` -> ArmyAnt Attack |
| `data/sounds/ant_attack3.ogg` | 0.089 | 4 | sounds.dat | live | `antatack` -> ArmyAnt Attack |
| `data/sounds/ant_attack4.ogg` | 0.124 | 4 | sounds.dat | live | `antatack` -> ArmyAnt Attack |
| `data/sounds/ant_attack5.ogg` | 0.166 | 5 | sounds.dat | live | `antatack` -> ArmyAnt Attack |
| `data/sounds/ant_attack6.ogg` | 0.149 | 4 | sounds.dat | live | `antatack` -> ArmyAnt Attack |
| `data/sounds/ant_death.ogg` | 0.618 | 8 | sounds.dat | live | `antdeath` -> ArmyAnt Die |
| `data/sounds/ant_death2.ogg` | 0.618 | 9 | sounds.dat | live | `antdeath` -> ArmyAnt Die |
| `data/sounds/ant_death3.ogg` | 0.511 | 7 | sounds.dat | live | `antdeath` -> ArmyAnt Die |
| `data/sounds/ant_death4.ogg` | 0.804 | 10 | sounds.dat | live | `antdeath` -> ArmyAnt Die |
| `data/sounds/ant_death5.ogg` | 0.446 | 7 | sounds.dat | live | `antdeath` -> ArmyAnt Die |
| `data/sounds/ant_death6.ogg` | 0.517 | 7 | sounds.dat | live | `antdeath` -> ArmyAnt Die |
| `data/sounds/ant_lose_health.ogg` | 0.199 | 5 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/ant_lose_health2.ogg` | 0.170 | 4 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/ant_lose_health3.ogg` | 0.188 | 5 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/ant_lose_health4.ogg` | 0.188 | 5 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/ant_lose_health5.ogg` | 0.253 | 5 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/ant_lose_health6.ogg` | 0.341 | 6 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/ant_lose_health7.ogg` | 0.269 | 5 | sounds.dat | live | `antlosehealth` -> ArmyAnt LoseHealth |
| `data/sounds/anthill_damage.ogg` | 3.274 | 27 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage10.ogg` | 3.930 | 30 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage2.ogg` | 3.483 | 27 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage3.ogg` | 3.158 | 26 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage4.ogg` | 2.775 | 23 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage5.ogg` | 3.524 | 28 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage6.ogg` | 4.481 | 34 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage7.ogg` | 4.098 | 31 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage8.ogg` | 3.511 | 28 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/anthill_damage9.ogg` | 3.059 | 25 | sounds.dat | live | `1anthilldamage` -> AntHill Damage |
| `data/sounds/ants_create.ogg` | 0.099 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create2.ogg` | 0.122 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create3.ogg` | 0.128 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create4.ogg` | 0.078 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create5.ogg` | 0.102 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create6.ogg` | 0.082 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create7.ogg` | 0.081 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/ants_create8.ogg` | 0.117 | 4 | sounds.dat | live | `antcreat` -> ArmyAnt Create |
| `data/sounds/aquire_research.ogg` | 2.356 | 20 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/aquire_research2.ogg` | 2.336 | 20 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/aquire_research3.ogg` | 1.949 | 17 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/aquire_research4.ogg` | 2.020 | 17 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/aquire_research5.ogg` | 2.108 | 19 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/aquire_research6.ogg` | 2.020 | 18 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/aquire_research7.ogg` | 2.297 | 20 | sounds.dat | dead | `aquireresearch` -> ResearchItem AquireResearch |
| `data/sounds/armour_to_turret.ogg` | 6.827 | 52 | sounds.dat | live | `armourtransform` -> GunTurret Create |
| `data/sounds/armour_to_turret2.ogg` | 6.525 | 49 | sounds.dat | live | `armourtransform` -> GunTurret Create |
| `data/sounds/armour_to_turret3.ogg` | 6.548 | 49 | sounds.dat | live | `armourtransform` -> GunTurret Create |
| `data/sounds/armour_to_turret4.ogg` | 6.803 | 52 | sounds.dat | live | `armourtransform` -> GunTurret Create |
| `data/sounds/armour_to_turret5.ogg` | 5.132 | 39 | sounds.dat | live | `armourtransform` -> GunTurret Create |
| `data/sounds/armour_to_turret6.ogg` | 5.875 | 46 | sounds.dat | live | `armourtransform` -> GunTurret Create |
| `data/sounds/battle_cannon_damage.ogg` | 2.182 | 21 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage10.ogg` | 2.618 | 21 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage2.ogg` | 2.839 | 23 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage3.ogg` | 3.541 | 28 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage4.ogg` | 2.763 | 23 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage5.ogg` | 2.746 | 23 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage6.ogg` | 2.728 | 22 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage7.ogg` | 2.531 | 21 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage8.ogg` | 2.270 | 20 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/battle_cannon_damage9.ogg` | 2.670 | 22 | sounds.dat | dead | `battlecannondamage` -> GunTurret Damage |
| `data/sounds/bigmetal1.ogg` | 3.613 | 46 | sounds.dat | live | `bigmetalgroan` -> FuelGenerator PumpUp |
| `data/sounds/bigmetal2.ogg` | 3.680 | 37 | sounds.dat | live | `bigmetalgroan` -> FuelGenerator PumpUp |
| `data/sounds/bigmetal3.ogg` | 2.189 | 26 | sounds.dat | live | `bigmetalgroan` -> FuelGenerator PumpUp |
| `data/sounds/bigmetal4.ogg` | 5.081 | 68 | sounds.dat | live | `bigmetalgroan` -> FuelGenerator PumpUp |
| `data/sounds/bigmetal5.ogg` | 2.742 | 36 | sounds.dat | live | `bigmetalgroan` -> FuelGenerator PumpUp |
| `data/sounds/blueprint_sateilte.ogg` | 7.892 | 64 | sounds.dat | live | `relayhover` -> BlueprintConsole Create; BlueprintRelay Create; ControlStation Create |
| `data/sounds/blueprint_sateilte2.ogg` | 7.885 | 64 | sounds.dat | live | `relayhover` -> BlueprintConsole Create; BlueprintRelay Create; ControlStation Create |
| `data/sounds/blueprint_sateilte3.ogg` | 7.817 | 62 | sounds.dat | live | `relayhover` -> BlueprintConsole Create; BlueprintRelay Create; ControlStation Create |
| `data/sounds/blueprint_sateilte4.ogg` | 7.151 | 58 | sounds.dat | live | `relayhover` -> BlueprintConsole Create; BlueprintRelay Create; ControlStation Create |
| `data/sounds/centipede_attack.ogg` | 0.595 | 8 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack2.ogg` | 1.120 | 12 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack3.ogg` | 1.370 | 14 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack4.ogg` | 1.144 | 12 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack5.ogg` | 0.998 | 10 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack6.ogg` | 1.344 | 14 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack7.ogg` | 1.184 | 12 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_attack8.ogg` | 1.064 | 12 | sounds.dat | live | `newsamplegroup25` -> Centipede Attack |
| `data/sounds/centipede_create.ogg` | 3.386 | 48 | sounds.dat | live | `newsamplegroup26` -> Centipede Create |
| `data/sounds/centipede_create2.ogg` | 3.412 | 48 | sounds.dat | live | `newsamplegroup26` -> Centipede Create |
| `data/sounds/centipede_create3.ogg` | 2.901 | 26 | sounds.dat | live | `newsamplegroup26` -> Centipede Create |
| `data/sounds/centipede_create4.ogg` | 4.271 | 43 | sounds.dat | live | `newsamplegroup26` -> Centipede Create |
| `data/sounds/centipede_death.ogg` | 5.248 | 43 | sounds.dat | live | `newsamplegroup27` -> EscapeRocket Explode; Centipede Die |
| `data/sounds/centipede_death2.ogg` | 5.410 | 46 | sounds.dat | live | `newsamplegroup27` -> EscapeRocket Explode; Centipede Die |
| `data/sounds/centipede_death3.ogg` | 4.946 | 41 | sounds.dat | live | `newsamplegroup27` -> EscapeRocket Explode; Centipede Die |
| `data/sounds/centipede_death4.ogg` | 4.272 | 35 | sounds.dat | live | `newsamplegroup27` -> EscapeRocket Explode; Centipede Die |
| `data/sounds/centipede_losehealth.ogg` | 1.370 | 13 | sounds.dat | live | `newsamplegroup28` -> Centipede LoseHealth |
| `data/sounds/centipede_losehealth2.ogg` | 1.026 | 11 | sounds.dat | live | `newsamplegroup28` -> Centipede LoseHealth |
| `data/sounds/centipede_losehealth3.ogg` | 1.132 | 12 | sounds.dat | live | `newsamplegroup28` -> Centipede LoseHealth |
| `data/sounds/centipede_losehealth4.ogg` | 1.663 | 16 | sounds.dat | live | `newsamplegroup28` -> Centipede LoseHealth |
| `data/sounds/centipede_losehealth5.ogg` | 0.946 | 10 | sounds.dat | live | `newsamplegroup28` -> Centipede LoseHealth |
| `data/sounds/choir1.ogg` | 6.001 | 93 | sounds.dat | live | `choir1` -> EscapeRocket Flight; PulseBomb Create |
| `data/sounds/choir2.ogg` | 5.802 | 90 | sounds.dat | live | `choir1` -> EscapeRocket Flight; PulseBomb Create |
| `data/sounds/choir3.ogg` | 5.875 | 89 | sounds.dat | live | `choir1` -> EscapeRocket Flight; PulseBomb Create |
| `data/sounds/choir4.ogg` | 6.249 | 96 | sounds.dat | live | `choir1` -> EscapeRocket Flight; PulseBomb Create |
| `data/sounds/choir5.ogg` | 6.191 | 95 | sounds.dat | live | `choir1` -> EscapeRocket Flight; PulseBomb Create |
| `data/sounds/choir6.ogg` | 5.939 | 92 | sounds.dat | live | `choir1` -> EscapeRocket Flight; PulseBomb Create |
| `data/sounds/chug1.ogg` | 5.654 | 38 | sounds.dat | dead | `mineambience` -> Refinery Create |
| `data/sounds/chug2.ogg` | 5.933 | 41 | sounds.dat | dead | `mineambience` -> Refinery Create |
| `data/sounds/chug3.ogg` | 9.242 | 61 | sounds.dat | dead | `mineambience` -> Refinery Create |
| `data/sounds/chug4.ogg` | 8.081 | 53 | sounds.dat | dead | `mineambience` -> Refinery Create |
| `data/sounds/chug5.ogg` | 9.138 | 61 | sounds.dat | dead | `mineambience` -> Refinery Create |
| `data/sounds/chug6.ogg` | 7.105 | 47 | sounds.dat | dead | `mineambience` -> Refinery Create |
| `data/sounds/darwin_burn_death.ogg` | 3.216 | 49 | sounds.dat | unused |  |
| `data/sounds/darwin_burn_death1.ogg` | 3.216 | 50 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/darwin_burn_death2.ogg` | 3.692 | 57 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/darwin_burn_death3.ogg` | 3.587 | 56 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/darwin_burn_death4.ogg` | 3.855 | 58 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/darwin_burn_death5.ogg` | 4.063 | 66 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/darwin_burn_death6.ogg` | 3.019 | 48 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/darwin_burn_death7.ogg` | 3.866 | 62 | sounds.dat | unused |  |
| `data/sounds/darwin_create.ogg` | 0.167 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create10.ogg` | 0.136 | 4 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create11.ogg` | 0.086 | 4 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create12.ogg` | 0.110 | 4 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create13.ogg` | 0.083 | 4 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create16.ogg` | 0.254 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create18.ogg` | 0.485 | 7 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create2.ogg` | 0.243 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create3.ogg` | 0.080 | 4 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create4.ogg` | 0.161 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create5.ogg` | 0.254 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create6.ogg` | 0.286 | 6 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create7.ogg` | 0.183 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create8.ogg` | 0.246 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_create9.ogg` | 0.201 | 5 | sounds.dat | live | `darwiniancreate` -> Darwinian Create |
| `data/sounds/darwin_death.ogg` | 0.910 | 10 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death2.ogg` | 0.477 | 7 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death3.ogg` | 0.663 | 8 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death4.ogg` | 0.701 | 8 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death5.ogg` | 0.599 | 8 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death6.ogg` | 0.710 | 8 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death7.ogg` | 0.595 | 8 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_death9.ogg` | 1.225 | 12 | sounds.dat | live | `darwiniandie` -> Darwinian Die; LaserTroop Die; Officer Die |
| `data/sounds/darwin_jump.ogg` | 3.110 | 29 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump2.ogg` | 2.469 | 24 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump3.ogg` | 2.446 | 24 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump4.ogg` | 2.891 | 27 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump5.ogg` | 2.699 | 26 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump6.ogg` | 2.565 | 25 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump7.ogg` | 2.568 | 25 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump8.ogg` | 2.912 | 26 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_jump9.ogg` | 2.878 | 26 | sounds.dat | live | `victoryjum` -> Darwinian VictoryJump |
| `data/sounds/darwin_losehealth.ogg` | 0.427 | 7 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth10.ogg` | 0.528 | 7 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth11.ogg` | 0.273 | 5 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth12.ogg` | 0.349 | 6 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth13.ogg` | 0.751 | 9 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth14.ogg` | 0.700 | 8 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth15.ogg` | 0.544 | 7 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth16.ogg` | 0.586 | 8 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth17.ogg` | 0.215 | 5 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth18.ogg` | 0.833 | 10 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth2.ogg` | 0.580 | 7 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth3.ogg` | 0.731 | 9 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth4.ogg` | 0.251 | 5 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth5.ogg` | 0.252 | 5 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth6.ogg` | 0.441 | 7 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth7.ogg` | 0.223 | 5 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth8.ogg` | 0.224 | 5 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_losehealth9.ogg` | 0.430 | 6 | sounds.dat | live | `darwinianlosehealth` -> Darwinian LoseHealth; Officer LoseHealth |
| `data/sounds/darwin_on_fire.ogg` | 0.911 | 18 | sounds.dat | unused |  |
| `data/sounds/darwin_on_fire1.ogg` | 0.911 | 18 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire10.ogg` | 1.480 | 27 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire11.ogg` | 1.120 | 21 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire12.ogg` | 1.097 | 20 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire13.ogg` | 1.103 | 21 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire2.ogg` | 1.016 | 19 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire3.ogg` | 1.167 | 21 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire4.ogg` | 0.891 | 17 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire5.ogg` | 1.144 | 23 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire6.ogg` | 0.787 | 16 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire7.ogg` | 0.977 | 20 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire8.ogg` | 1.116 | 21 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwin_on_fire9.ogg` | 0.996 | 20 | sounds.dat | unused | orphan group `darwinnianonfire` |
| `data/sounds/darwinian_threat_attack.ogg` | 4.365 | 34 | sounds.dat | live | `darwinseethreatattack` -> Darwinian SeenThreatAttackHorn; Officer Attack |
| `data/sounds/darwinian_threat_attack2.ogg` | 3.071 | 26 | sounds.dat | live | `darwinseethreatattack` -> Darwinian SeenThreatAttackHorn; Officer Attack |
| `data/sounds/darwinian_threat_attack3.ogg` | 3.553 | 25 | sounds.dat | live | `darwinseethreatattack` -> Darwinian SeenThreatAttackHorn; Officer Attack |
| `data/sounds/darwinian_threat_run.ogg` | 0.499 | 7 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run10.ogg` | 0.894 | 10 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run2.ogg` | 0.411 | 6 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run3.ogg` | 0.528 | 7 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run4.ogg` | 0.816 | 9 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run5.ogg` | 0.993 | 11 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run6.ogg` | 1.019 | 11 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run7.ogg` | 0.650 | 8 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run8.ogg` | 1.004 | 11 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/darwinian_threat_run9.ogg` | 0.697 | 8 | sounds.dat | live | `darwinrunaway` -> Darwinian SeenThreatRunAway |
| `data/sounds/deselect1.ogg` | 2.508 | 38 | sounds.dat | live | Interface DeleteTask |
| `data/sounds/dma_sc-visitors_from_dreams.ogg` | 325.251 | 2285 | NOT-SHIPPED(sounds/dma_sc*) | dead | Music LoaderRaytrace; Music StartSequence |
| `data/sounds/eggsplat.ogg` | 2.485 | 23 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/eggsplat2.ogg` | 2.194 | 21 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/eggsplat3.ogg` | 1.962 | 19 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/eggsplat4.ogg` | 2.241 | 21 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/eggsplat5.ogg` | 2.305 | 22 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/eggsplat6.ogg` | 2.136 | 20 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/eggsplat7.ogg` | 2.489 | 24 | sounds.dat | dead | `eggsplat` -> Egg Die |
| `data/sounds/elec_cackle.ogg` | 0.878 | 10 | sounds.dat | live | `elec_cackle` -> BlueprintStore Create; LaserFence Spark |
| `data/sounds/elec_cackle2.ogg` | 0.895 | 11 | sounds.dat | live | `elec_cackle` -> BlueprintStore Create; LaserFence Spark |
| `data/sounds/elec_cackle3.ogg` | 0.882 | 11 | sounds.dat | live | `elec_cackle` -> BlueprintStore Create; LaserFence Spark |
| `data/sounds/elec_cackle4.ogg` | 1.010 | 12 | sounds.dat | live | `elec_cackle` -> BlueprintStore Create; LaserFence Spark |
| `data/sounds/elec_cackle5.ogg` | 1.963 | 20 | sounds.dat | live | `elec_cackle` -> BlueprintStore Create; LaserFence Spark |
| `data/sounds/elec_cackle6.ogg` | 1.329 | 18 | sounds.dat | live | `elec_cackle` -> BlueprintStore Create; LaserFence Spark |
| `data/sounds/engineburn.ogg` | 3.275 | 53 | sounds.dat | live | EscapeRocket EngineBurn |
| `data/sounds/engineer_die.ogg` | 2.891 | 25 | sounds.dat | live | `engineerdie` -> Engineer Die |
| `data/sounds/engineer_die2.ogg` | 3.140 | 30 | sounds.dat | live | `engineerdie` -> Engineer Die |
| `data/sounds/engineer_die3.ogg` | 3.483 | 28 | sounds.dat | live | `engineerdie` -> Engineer Die |
| `data/sounds/engineer_die4.ogg` | 2.351 | 21 | sounds.dat | live | `engineerdie` -> Engineer Die |
| `data/sounds/engineer_die5.ogg` | 2.914 | 27 | sounds.dat | live | `engineerdie` -> Engineer Die |
| `data/sounds/engineer_hover.ogg` | 0.777 | 7 | sounds.dat | live | Engineer Create |
| `data/sounds/engineer_losehealth.ogg` | 2.723 | 17 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth2.ogg` | 1.520 | 11 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth3.ogg` | 1.652 | 14 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth4.ogg` | 1.686 | 13 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth5.ogg` | 2.174 | 17 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth6.ogg` | 1.361 | 11 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth7.ogg` | 1.449 | 12 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth8.ogg` | 1.733 | 13 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_losehealth9.ogg` | 1.471 | 12 | sounds.dat | live | `engineerinjury` -> Engineer LoseHealth |
| `data/sounds/engineer_repog_radar_loop_end.ogg` | 1.132 | 12 | sounds.dat | dead | Engineer EndReprogramming |
| `data/sounds/engineer_reprog_radar_loop.ogg` | 7.987 | 67 | sounds.dat | live | Crate BeginCapture; Incubator Reprogramming; Engineer BeginReprogramming |
| `data/sounds/engineexplode.ogg` | 5.253 | 82 | sounds.dat | live | EscapeRocket Explode |
| `data/sounds/engneignite1.ogg` | 6.408 | 101 | sounds.dat | unused |  |
| `data/sounds/enter_teleport.ogg` | 0.678 | 8 | sounds.dat | live | `enterteleport` -> Officer EnterTeleport; Squadie EnterTeleport |
| `data/sounds/enter_teleport2.ogg` | 0.677 | 8 | sounds.dat | live | `enterteleport` -> Officer EnterTeleport; Squadie EnterTeleport |
| `data/sounds/enter_teleport3.ogg` | 0.676 | 8 | sounds.dat | live | `enterteleport` -> Officer EnterTeleport; Squadie EnterTeleport |
| `data/sounds/enter_teleport4.ogg` | 0.682 | 8 | sounds.dat | live | `enterteleport` -> Officer EnterTeleport; Squadie EnterTeleport |
| `data/sounds/enter_teleport5.ogg` | 0.679 | 8 | sounds.dat | live | `enterteleport` -> Officer EnterTeleport; Squadie EnterTeleport |
| `data/sounds/enter_teleport6.ogg` | 0.628 | 8 | sounds.dat | live | `enterteleport` -> Officer EnterTeleport; Squadie EnterTeleport |
| `data/sounds/evil.ogg` | 53.870 | 313 | menusounds.dat | live | `newsamplegroup8`, `worldamb` -> Ambience EnterGlobalWorld; Ambience EnterLocation; MultiwiniaInterface LobbyAmbience |
| `data/sounds/exit_teleport.ogg` | 0.678 | 8 | sounds.dat | live | `exitteleport` -> Squadie ExitTeleport |
| `data/sounds/exit_teleport2.ogg` | 0.628 | 8 | sounds.dat | live | `exitteleport` -> Squadie ExitTeleport |
| `data/sounds/exit_teleport3.ogg` | 0.679 | 8 | sounds.dat | live | `exitteleport` -> Squadie ExitTeleport |
| `data/sounds/exit_teleport4.ogg` | 0.682 | 8 | sounds.dat | live | `exitteleport` -> Squadie ExitTeleport |
| `data/sounds/exit_teleport5.ogg` | 0.676 | 8 | sounds.dat | live | `exitteleport` -> Squadie ExitTeleport |
| `data/sounds/exit_teleport6.ogg` | 0.677 | 8 | sounds.dat | live | `exitteleport` -> Squadie ExitTeleport |
| `data/sounds/fence_damage.ogg` | 1.023 | 12 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage10.ogg` | 1.063 | 12 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage11.ogg` | 1.623 | 15 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage12.ogg` | 1.899 | 21 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage13.ogg` | 1.041 | 16 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage14.ogg` | 1.535 | 16 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage15.ogg` | 1.271 | 16 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage16.ogg` | 1.621 | 16 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage2.ogg` | 0.788 | 9 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage3.ogg` | 1.510 | 16 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage4.ogg` | 1.995 | 22 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage5.ogg` | 1.697 | 21 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage6.ogg` | 1.282 | 12 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage7.ogg` | 2.455 | 21 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage8.ogg` | 1.868 | 18 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_damage9.ogg` | 1.329 | 13 | sounds.dat | live | `elecfence@damage` -> LaserFence Damage |
| `data/sounds/fence_hum.ogg` | 0.020 | 3 | sounds.dat | live | LaserFence Create |
| `data/sounds/flag_wave.ogg` | 5.306 | 39 | sounds.dat | unused |  |
| `data/sounds/flag_wave10.ogg` | 5.309 | 39 | sounds.dat | unused |  |
| `data/sounds/flag_wave11.ogg` | 5.338 | 43 | sounds.dat | unused |  |
| `data/sounds/flag_wave12.ogg` | 5.327 | 41 | sounds.dat | unused |  |
| `data/sounds/flag_wave2.ogg` | 5.317 | 42 | sounds.dat | unused |  |
| `data/sounds/flag_wave3.ogg` | 5.295 | 40 | sounds.dat | unused |  |
| `data/sounds/flag_wave4.ogg` | 5.338 | 43 | sounds.dat | unused |  |
| `data/sounds/flag_wave5.ogg` | 5.505 | 41 | sounds.dat | unused |  |
| `data/sounds/flag_wave6.ogg` | 5.322 | 39 | sounds.dat | unused |  |
| `data/sounds/flag_wave7.ogg` | 5.342 | 38 | sounds.dat | unused |  |
| `data/sounds/flag_wave8.ogg` | 5.082 | 41 | sounds.dat | unused |  |
| `data/sounds/flag_wave9.ogg` | 5.326 | 41 | sounds.dat | unused |  |
| `data/sounds/fodder.ogg` | 6.315 | 31 | NOT-SHIPPED(sounds/fodder.ogg) | dead | Music LoaderFodder |
| `data/sounds/fuel_load_passenger.ogg` | 0.772 | 16 | sounds.dat | live | `loadpassenger` -> FuelStation LoadPassenger |
| `data/sounds/fuel_load_passenger2.ogg` | 0.830 | 17 | sounds.dat | live | `loadpassenger` -> FuelStation LoadPassenger |
| `data/sounds/fuel_load_passenger3.ogg` | 0.715 | 15 | sounds.dat | live | `loadpassenger` -> FuelStation LoadPassenger |
| `data/sounds/fuel_load_passenger4.ogg` | 0.768 | 16 | sounds.dat | live | `loadpassenger` -> FuelStation LoadPassenger |
| `data/sounds/fuel_load_passenger5.ogg` | 0.546 | 12 | sounds.dat | live | `loadpassenger` -> FuelStation LoadPassenger |
| `data/sounds/fuelpipe1_loop.ogg` | 6.768 | 102 | sounds.dat | unused |  |
| `data/sounds/fuelpipe2_loop.ogg` | 4.226 | 66 | sounds.dat | live | FuelPipe PumpFuel |
| `data/sounds/fuelpipe3_loop.ogg` | 4.403 | 68 | sounds.dat | unused |  |
| `data/sounds/fuelpump1.ogg` | 3.743 | 45 | sounds.dat | unused |  |
| `data/sounds/fuelpumpdown1.ogg` | 2.685 | 34 | sounds.dat | unused |  |
| `data/sounds/fuelpumpexhale1.ogg` | 5.590 | 87 | sounds.dat | live | FuelGenerator PumpUp |
| `data/sounds/fuelpumpup1.ogg` | 2.891 | 34 | sounds.dat | live | FuelGenerator PumpDown |
| `data/sounds/generator_burst.ogg` | 0.856 | 10 | sounds.dat | dead | `generatorburst` -> Generator TriggerSurge |
| `data/sounds/generator_burst2.ogg` | 0.833 | 10 | sounds.dat | dead | `generatorburst` -> Generator TriggerSurge |
| `data/sounds/generator_burst3.ogg` | 0.740 | 9 | sounds.dat | dead | `generatorburst` -> Generator TriggerSurge |
| `data/sounds/generator_burst4.ogg` | 1.457 | 16 | sounds.dat | dead | `generatorburst` -> Generator TriggerSurge |
| `data/sounds/generator_burst5.ogg` | 0.996 | 12 | sounds.dat | dead | `generatorburst` -> Generator TriggerSurge |
| `data/sounds/generator_online.ogg` | 27.683 | 152 | sounds.dat | dead | Generator ReprogramComplete |
| `data/sounds/gesturefail.ogg` | 2.215 | 15 | sounds.dat | dead | Gesture GestureFail |
| `data/sounds/gesturestart.ogg` | 5.754 | 44 | sounds.dat | unused |  |
| `data/sounds/gesturesuccess.ogg` | 2.215 | 16 | sounds.dat | live | Gesture GestureSuccess |
| `data/sounds/god_dish_disconnect.ogg` | 6.536 | 52 | sounds.dat | dead | `disconnectgod` -> GodDish DisconnectFromGod |
| `data/sounds/god_dish_disconnect2.ogg` | 7.000 | 55 | sounds.dat | dead | `disconnectgod` -> GodDish DisconnectFromGod |
| `data/sounds/god_dish_disconnect3.ogg` | 5.793 | 46 | sounds.dat | dead | `disconnectgod` -> GodDish DisconnectFromGod |
| `data/sounds/god_dishconnect.ogg` | 8.074 | 56 | sounds.dat | dead | `connectgod` -> GodDish ConnectToGod |
| `data/sounds/god_dishconnect2.ogg` | 7.888 | 55 | sounds.dat | dead | `connectgod` -> GodDish ConnectToGod |
| `data/sounds/god_dishconnect3.ogg` | 8.038 | 56 | sounds.dat | dead | `connectgod` -> GodDish ConnectToGod |
| `data/sounds/goddish_create.ogg` | 3.706 | 27 | sounds.dat | dead | `goddishcreate` -> GodDish Create |
| `data/sounds/goddish_create2.ogg` | 3.708 | 28 | sounds.dat | dead | `goddishcreate` -> GodDish Create |
| `data/sounds/goddish_create3.ogg` | 3.708 | 28 | sounds.dat | dead | `goddishcreate` -> GodDish Create |
| `data/sounds/goddish_create4.ogg` | 3.708 | 27 | sounds.dat | dead | `goddishcreate` -> GodDish Create |
| `data/sounds/goddish_create5.ogg` | 3.708 | 27 | sounds.dat | dead | `goddishcreate` -> GodDish Create |
| `data/sounds/gren_boom1.ogg` | 3.704 | 36 | sounds.dat | live | `newsamplegroup3` -> AntHill Explode; Grenade Explode |
| `data/sounds/gren_boom2.ogg` | 4.075 | 38 | sounds.dat | live | `newsamplegroup3` -> AntHill Explode; Grenade Explode |
| `data/sounds/gren_boom3.ogg` | 4.040 | 38 | sounds.dat | live | `newsamplegroup3` -> AntHill Explode; Grenade Explode |
| `data/sounds/gren_boom4.ogg` | 3.820 | 35 | sounds.dat | live | `newsamplegroup3` -> AntHill Explode; Grenade Explode |
| `data/sounds/gren_boom5.ogg` | 3.831 | 35 | sounds.dat | live | `newsamplegroup3` -> AntHill Explode; Grenade Explode |
| `data/sounds/gren_bounce1.ogg` | 0.372 | 6 | sounds.dat | live | `newsamplegroup6` -> Grenade Bounce |
| `data/sounds/gren_bounce2.ogg` | 0.430 | 6 | sounds.dat | live | `newsamplegroup6` -> Grenade Bounce |
| `data/sounds/gren_bounce3.ogg` | 0.594 | 7 | sounds.dat | live | `newsamplegroup6` -> Grenade Bounce |
| `data/sounds/gren_bounce4.ogg` | 0.475 | 6 | sounds.dat | live | `newsamplegroup6` -> Grenade Bounce |
| `data/sounds/gren_bounce5.ogg` | 0.728 | 8 | sounds.dat | live | `newsamplegroup6` -> Grenade Bounce |
| `data/sounds/gren_bounce6.ogg` | 0.445 | 6 | sounds.dat | live | `newsamplegroup6` -> Grenade Bounce |
| `data/sounds/gren_fire1.ogg` | 3.250 | 25 | sounds.dat | live | `newsamplegroup4` -> Grenade Create |
| `data/sounds/gren_fire2.ogg` | 3.251 | 27 | sounds.dat | live | `newsamplegroup4` -> Grenade Create |
| `data/sounds/gren_fire3.ogg` | 3.251 | 28 | sounds.dat | live | `newsamplegroup4` -> Grenade Create |
| `data/sounds/gren_fire4.ogg` | 3.248 | 28 | sounds.dat | live | `newsamplegroup4` -> Grenade Create |
| `data/sounds/gren_fire5.ogg` | 3.251 | 28 | sounds.dat | live | `newsamplegroup4` -> Grenade Create |
| `data/sounds/grenade_beep.ogg` | 1.004 | 10 | sounds.dat | live | AirStrikeBomb Flash; Grenade Flash |
| `data/sounds/grenade_return3.ogg` | 0.517 | 8 | sounds.dat | live | Squadie WeaponReturns |
| `data/sounds/gun_turret_fire.ogg` | 1.022 | 11 | sounds.dat | live | `turretfire` -> GunTurret FireShell; Squadie FireLaser |
| `data/sounds/gun_turret_fire2.ogg` | 1.004 | 11 | sounds.dat | live | `turretfire` -> GunTurret FireShell; Squadie FireLaser |
| `data/sounds/gun_turret_fire3.ogg` | 1.045 | 12 | sounds.dat | live | `turretfire` -> GunTurret FireShell; Squadie FireLaser |
| `data/sounds/gun_turret_fire4.ogg` | 0.997 | 11 | sounds.dat | live | `turretfire` -> GunTurret FireShell; Squadie FireLaser |
| `data/sounds/gun_turret_fire5.ogg` | 0.994 | 11 | sounds.dat | live | `turretfire` -> GunTurret FireShell; Squadie FireLaser |
| `data/sounds/gun_turret_fire6.ogg` | 0.926 | 10 | sounds.dat | live | `turretfire` -> GunTurret FireShell; Squadie FireLaser |
| `data/sounds/hypno_loop_run.ogg` | 8.568 | 57 | sounds.dat | dead | Grenade ExplodeController |
| `data/sounds/ident1.ogg` | 10.151 | 137 | sounds.dat | unused |  |
| `data/sounds/ignitionzap1.ogg` | 0.615 | 13 | sounds.dat | live | EscapeRocket EngineBurn |
| `data/sounds/incubator_addspirit.ogg` | 1.120 | 11 | sounds.dat | live | `incubatoraddspirit` -> Incubator AddSpirit; Harvester DropSpirit |
| `data/sounds/incubator_addspirit2.ogg` | 1.384 | 13 | sounds.dat | live | `incubatoraddspirit` -> Incubator AddSpirit; Harvester DropSpirit |
| `data/sounds/incubator_addspirit3.ogg` | 1.408 | 13 | sounds.dat | live | `incubatoraddspirit` -> Incubator AddSpirit; Harvester DropSpirit |
| `data/sounds/incubator_addspirit4.ogg` | 1.541 | 14 | sounds.dat | live | `incubatoraddspirit` -> Incubator AddSpirit; Harvester DropSpirit |
| `data/sounds/incubator_addspirit5.ogg` | 1.588 | 14 | sounds.dat | live | `incubatoraddspirit` -> Incubator AddSpirit; Harvester DropSpirit |
| `data/sounds/incubator_addspirit6.ogg` | 1.277 | 12 | sounds.dat | live | `incubatoraddspirit` -> Incubator AddSpirit; Harvester DropSpirit |
| `data/sounds/incubator_birth.ogg` | 1.390 | 13 | sounds.dat | dead | `incubatorspawn` -> Incubator SpawnEntity |
| `data/sounds/incubator_birth2.ogg` | 1.382 | 13 | sounds.dat | dead | `incubatorspawn` -> Incubator SpawnEntity |
| `data/sounds/incubator_birth3.ogg` | 1.434 | 13 | sounds.dat | dead | `incubatorspawn` -> Incubator SpawnEntity |
| `data/sounds/incubator_birth4.ogg` | 1.480 | 14 | sounds.dat | dead | `incubatorspawn` -> Incubator SpawnEntity |
| `data/sounds/incubator_birth5.ogg` | 1.466 | 14 | sounds.dat | dead | `incubatorspawn` -> Incubator SpawnEntity |
| `data/sounds/incubator_birth6.ogg` | 1.440 | 13 | sounds.dat | dead | `incubatorspawn` -> Incubator SpawnEntity |
| `data/sounds/kickverb1.ogg` | 2.508 | 39 | sounds.dat | live | `kickverb` -> FuelGenerator PumpDown |
| `data/sounds/kickverb2.ogg` | 2.968 | 42 | sounds.dat | live | `kickverb` -> FuelGenerator PumpDown |
| `data/sounds/kickverb3.ogg` | 2.038 | 32 | sounds.dat | live | `kickverb` -> FuelGenerator PumpDown |
| `data/sounds/laser1.ogg` | 0.604 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser10.ogg` | 0.871 | 10 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser11.ogg` | 0.759 | 9 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser12.ogg` | 0.554 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser13.ogg` | 0.795 | 10 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser14.ogg` | 0.563 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser15.ogg` | 0.559 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser16.ogg` | 0.576 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser17.ogg` | 0.536 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser18.ogg` | 0.795 | 9 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser19.ogg` | 0.747 | 9 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser2.ogg` | 0.837 | 9 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser20.ogg` | 0.814 | 10 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser21.ogg` | 1.003 | 11 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser22.ogg` | 0.505 | 7 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser23.ogg` | 0.871 | 10 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser24.ogg` | 0.501 | 7 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser3.ogg` | 0.580 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser4.ogg` | 0.557 | 8 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser5.ogg` | 0.501 | 7 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser6.ogg` | 0.508 | 7 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser7.ogg` | 0.798 | 10 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser8.ogg` | 0.782 | 10 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser9.ogg` | 0.662 | 9 | sounds.dat | live | `0lasers` -> Darwinian Attack; LaserTroop Attack |
| `data/sounds/laser_hit_building.ogg` | 1.071 | 12 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building10.ogg` | 0.591 | 8 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building2.ogg` | 0.852 | 10 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building3.ogg` | 1.001 | 11 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building4.ogg` | 0.714 | 9 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building5.ogg` | 0.800 | 9 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building6.ogg` | 0.393 | 6 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building7.ogg` | 0.625 | 8 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building8.ogg` | 1.096 | 12 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hit_building9.ogg` | 0.440 | 6 | sounds.dat | unused | orphan group `laserhitbuild` |
| `data/sounds/laser_hitent1.ogg` | 0.506 | 7 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent2.ogg` | 0.506 | 7 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent3.ogg` | 0.520 | 7 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent4.ogg` | 0.746 | 9 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent5.ogg` | 0.261 | 5 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent6.ogg` | 0.229 | 5 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent7.ogg` | 0.524 | 7 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitent8.ogg` | 0.222 | 5 | sounds.dat | unused | orphan group `laserhitentity` |
| `data/sounds/laser_hitgnd.ogg` | 0.380 | 6 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd2.ogg` | 0.779 | 9 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd3.ogg` | 1.395 | 14 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd4.ogg` | 0.744 | 9 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd5.ogg` | 0.721 | 8 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd6.ogg` | 0.762 | 9 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd7.ogg` | 0.762 | 9 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd8.ogg` | 0.689 | 8 | sounds.dat | unused | orphan group `laserhitgnd` |
| `data/sounds/laser_hitgnd9.ogg` | 1.084 | 11 | sounds.dat | unused | orphan group `laserhitentity;laserhitgnd` |
| `data/sounds/matrix.ogg` | 24.000 | 188 | NOT-SHIPPED(sounds/matrix.ogg) | dead | Music LoaderMatrix |
| `data/sounds/menuscroll1.ogg` | 2.171 | 32 | menusounds.dat | live | MultiwiniaInterface MenuCancel |
| `data/sounds/menuscroll2.ogg` | 1.862 | 27 | menusounds.dat | live | Interface Slide; MultiwiniaInterface MenuSelect |
| `data/sounds/mine_cart_creak.ogg` | 2.548 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak10.ogg` | 2.084 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak11.ogg` | 1.980 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak12.ogg` | 2.101 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak13.ogg` | 2.026 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak14.ogg` | 2.247 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak15.ogg` | 1.956 | 17 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak16.ogg` | 2.177 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak17.ogg` | 1.503 | 14 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak18.ogg` | 1.956 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak19.ogg` | 2.078 | 20 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak2.ogg` | 2.676 | 23 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak20.ogg` | 1.974 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak21.ogg` | 2.020 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak22.ogg` | 1.921 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak23.ogg` | 2.183 | 20 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak24.ogg` | 1.921 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak25.ogg` | 1.631 | 15 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak26.ogg` | 1.753 | 16 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak27.ogg` | 2.202 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak28.ogg` | 2.249 | 20 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak29.ogg` | 2.486 | 24 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak3.ogg` | 2.130 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak30.ogg` | 2.525 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak31.ogg` | 2.490 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak32.ogg` | 2.223 | 20 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak33.ogg` | 2.072 | 20 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak34.ogg` | 2.084 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak35.ogg` | 1.794 | 17 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak36.ogg` | 1.811 | 17 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak37.ogg` | 2.206 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak38.ogg` | 2.334 | 22 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak39.ogg` | 2.009 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak4.ogg` | 2.315 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak40.ogg` | 2.334 | 22 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak41.ogg` | 2.067 | 19 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak42.ogg` | 2.450 | 22 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak5.ogg` | 2.421 | 20 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak6.ogg` | 1.939 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak7.ogg` | 2.154 | 18 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak8.ogg` | 2.368 | 21 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cart_creak9.ogg` | 1.939 | 17 | sounds.dat | dead | `minecartcreak` -> TrackLink CogTurn |
| `data/sounds/mine_cog.ogg` | 2.409 | 23 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mine_cog2.ogg` | 2.406 | 24 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mine_cog3.ogg` | 2.415 | 24 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mine_cog4.ogg` | 2.409 | 24 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mine_cog5.ogg` | 2.415 | 23 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mine_cog6.ogg` | 2.426 | 24 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mine_cog7.ogg` | 2.368 | 22 | sounds.dat | dead | `minerun` -> Mine CogTurn |
| `data/sounds/mouse.ogg` | 0.085 | 5 | sounds.dat | unused |  |
| `data/sounds/mouse1.ogg` | 0.056 | 5 | sounds.dat | unused |  |
| `data/sounds/mouse2.ogg` | 0.056 | 5 | menusounds.dat | live | `mouseovericon` -> Interface MouseOverIcon; MultiwiniaInterface MenuRollOver |
| `data/sounds/mouse3.ogg` | 0.050 | 5 | menusounds.dat | live | `mouseovericon` -> Interface MouseOverIcon; MultiwiniaInterface MenuRollOver |
| `data/sounds/mouse4.ogg` | 0.070 | 5 | sounds.dat | unused |  |
| `data/sounds/mouse5.ogg` | 0.055 | 5 | sounds.dat | unused |  |
| `data/sounds/mouse6.ogg` | 0.110 | 5 | sounds.dat | unused |  |
| `data/sounds/ocean1.ogg` | 23.499 | 163 | sounds.dat | dead | `worldamb` -> Ambience EnterGlobalWorld |
| `data/sounds/officer_create10.ogg` | 0.265 | 6 | sounds.dat | live | `officercreate` -> Darwinian GivenOrders; Officer Create; Officer Selected; Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_create11.ogg` | 0.339 | 6 | sounds.dat | live | `officercreate` -> Darwinian GivenOrders; Officer Create; Officer Selected; Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_create4.ogg` | 0.529 | 7 | sounds.dat | live | `officercreate` -> Darwinian GivenOrders; Officer Create; Officer Selected; Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_create5.ogg` | 0.310 | 6 | sounds.dat | live | `officercreate` -> Darwinian GivenOrders; Officer Create; Officer Selected; Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_create6.ogg` | 0.412 | 6 | sounds.dat | live | `officercreate` -> Darwinian GivenOrders; Officer Create; Officer Selected; Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_create9.ogg` | 0.313 | 6 | sounds.dat | live | `officercreate` -> Darwinian GivenOrders; Officer Create; Officer Selected; Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order.ogg` | 3.001 | 13 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order10.ogg` | 3.000 | 11 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order11.ogg` | 3.001 | 10 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order12.ogg` | 3.001 | 9 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order13.ogg` | 3.001 | 13 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order14.ogg` | 3.000 | 11 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order15.ogg` | 3.001 | 13 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order16.ogg` | 3.001 | 11 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order17.ogg` | 3.001 | 10 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order2.ogg` | 3.001 | 14 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order3.ogg` | 3.001 | 17 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order4.ogg` | 3.000 | 19 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order5.ogg` | 3.001 | 16 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order6.ogg` | 3.001 | 12 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order7.ogg` | 3.000 | 11 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order8.ogg` | 3.000 | 12 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order9.ogg` | 3.001 | 12 | sounds.dat | live | `officerleftright` -> Officer SetOrderFormation; Officer SetOrderGoto |
| `data/sounds/officer_order_goto.ogg` | 4.876 | 26 | sounds.dat | live | `officerordergoto` -> Armour SetOrders |
| `data/sounds/officer_order_goto2.ogg` | 4.667 | 24 | sounds.dat | live | `officerordergoto` -> Armour SetOrders |
| `data/sounds/officer_order_goto3.ogg` | 4.540 | 24 | sounds.dat | live | `officerordergoto` -> Armour SetOrders |
| `data/sounds/officer_order_goto4.ogg` | 4.859 | 24 | sounds.dat | live | `officerordergoto` -> Armour SetOrders |
| `data/sounds/pang.ogg` | 87.121 | 626 | NOT-SHIPPED(sounds/pang*) | dead | Music LoaderAmiga |
| `data/sounds/power_station_online.ogg` | 22.375 | 113 | sounds.dat | dead | Powerstation Create |
| `data/sounds/pylon_surge.ogg` | 0.377 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge10.ogg` | 0.337 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge11.ogg` | 0.342 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge12.ogg` | 0.331 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge13.ogg` | 0.366 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge14.ogg` | 0.315 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge15.ogg` | 0.276 | 5 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge16.ogg` | 0.405 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge17.ogg` | 0.508 | 7 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge18.ogg` | 0.363 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge19.ogg` | 0.491 | 7 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge2.ogg` | 0.296 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge20.ogg` | 0.435 | 7 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge21.ogg` | 0.569 | 9 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge22.ogg` | 0.337 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge3.ogg` | 0.369 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge4.ogg` | 0.367 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge5.ogg` | 0.289 | 5 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge6.ogg` | 0.321 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge7.ogg` | 0.371 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge8.ogg` | 0.366 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/pylon_surge9.ogg` | 0.313 | 6 | sounds.dat | live | `ylonelecsurge` -> Pylon TriggerSurge |
| `data/sounds/radar_lose_connection.ogg` | 1.250 | 12 | sounds.dat | live | `radarlose` -> RadarDish ConnectionLost |
| `data/sounds/radar_lose_connection2.ogg` | 1.794 | 16 | sounds.dat | live | `radarlose` -> RadarDish ConnectionLost |
| `data/sounds/radar_lose_connection3.ogg` | 1.174 | 12 | sounds.dat | live | `radarlose` -> RadarDish ConnectionLost |
| `data/sounds/radar_lose_connection4.ogg` | 1.778 | 15 | sounds.dat | live | `radarlose` -> RadarDish ConnectionLost |
| `data/sounds/radar_motor_run.ogg` | 0.675 | 8 | sounds.dat | live | RadarDish BeginRotation |
| `data/sounds/radar_reprog_complete.ogg` | 0.635 | 7 | sounds.dat | dead | RadarDish ReprogramComplete |
| `data/sounds/radar_start_turn.ogg` | 1.503 | 14 | sounds.dat | live | RadarDish BeginRotation |
| `data/sounds/radar_stop_turn.ogg` | 3.219 | 26 | sounds.dat | live | RadarDish EndRotation |
| `data/sounds/radar_transmission.ogg` | 1.038 | 13 | sounds.dat | live | `radarconnect` -> RadarDish ConnectionEstablished |
| `data/sounds/radar_transmission2.ogg` | 1.752 | 21 | sounds.dat | live | `radarconnect` -> RadarDish ConnectionEstablished |
| `data/sounds/radar_transmission3.ogg` | 0.660 | 9 | sounds.dat | live | `radarconnect` -> RadarDish ConnectionEstablished |
| `data/sounds/radar_transmission4.ogg` | 1.254 | 15 | sounds.dat | live | `radarconnect` -> RadarDish ConnectionEstablished |
| `data/sounds/redvirus_death.ogg` | 0.488 | 6 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/redvirus_death3.ogg` | 0.703 | 8 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/redvirus_losehealth.ogg` | 0.803 | 9 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/redvirus_losehealth2.ogg` | 0.303 | 5 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/redvirus_losehealth3.ogg` | 0.280 | 5 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/redvirus_losehealth4.ogg` | 0.418 | 6 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/redvirus_losehealth7.ogg` | 0.245 | 5 | sounds.dat | live | `newsamplegroup9` -> Virii Die |
| `data/sounds/refinery_cog.ogg` | 2.409 | 19 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog2.ogg` | 2.412 | 18 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog3.ogg` | 2.417 | 19 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog4.ogg` | 2.417 | 19 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog5.ogg` | 2.413 | 19 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog6.ogg` | 2.418 | 18 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog7.ogg` | 2.420 | 18 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/refinery_cog8.ogg` | 2.369 | 17 | sounds.dat | dead | `refineryrun` -> Refinery CogTurn |
| `data/sounds/research_create.ogg` | 3.216 | 26 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create10.ogg` | 2.624 | 22 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create3.ogg` | 3.035 | 25 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create4.ogg` | 3.340 | 27 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create5.ogg` | 3.394 | 28 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create6.ogg` | 2.584 | 22 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create7.ogg` | 2.785 | 23 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create8.ogg` | 2.540 | 22 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/research_create9.ogg` | 2.375 | 20 | sounds.dat | dead | `researchcreate` -> ResearchItem Create; Spam CreateResearch |
| `data/sounds/rocket_boom1.ogg` | 5.689 | 54 | sounds.dat | live | `newsamplegroup7` -> Rocket Explode |
| `data/sounds/rocket_boom2.ogg` | 5.492 | 54 | sounds.dat | live | `newsamplegroup7` -> Rocket Explode |
| `data/sounds/rocket_boom3.ogg` | 4.330 | 42 | sounds.dat | live | `newsamplegroup7` -> Rocket Explode |
| `data/sounds/rocket_boom4.ogg` | 5.550 | 53 | sounds.dat | live | `newsamplegroup7` -> Rocket Explode |
| `data/sounds/rocket_fire4_run.ogg` | 0.464 | 6 | sounds.dat | live | Rocket Create |
| `data/sounds/rocketlauncher.ogg` | 0.830 | 8 | sounds.dat | live | Rocket Create |
| `data/sounds/rumble1.ogg` | 22.105 | 98 | menusounds.dat | live | `newsamplegroup8` -> Ambience EnterGlobalWorld; Ambience EnterLocation; MultiwiniaInterface LobbyAmbience |
| `data/sounds/sepulveda_appear.ogg` | 0.559 | 7 | sounds.dat | dead | Sepulveda Appear |
| `data/sounds/sepulveda_disappear.ogg` | 0.560 | 6 | sounds.dat | dead | Sepulveda Disappear |
| `data/sounds/shipheartbeat1.ogg` | 1.436 | 20 | menusounds.dat | live | EscapeRocket Happy; MultiwiniaInterface LobbyAmbience |
| `data/sounds/shiphungry1.ogg` | 5.063 | 79 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shiphungry2.ogg` | 6.517 | 99 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shiphungry3.ogg` | 5.815 | 87 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shiphungry4.ogg` | 5.820 | 88 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shiphungry5.ogg` | 7.215 | 110 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shiphungry6.ogg` | 5.437 | 84 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shiphungry7.ogg` | 4.497 | 70 | sounds.dat | live | `shiphungry` -> EscapeRocket Unhappy |
| `data/sounds/shipmalfunction1.ogg` | 1.219 | 20 | sounds.dat | live | EscapeRocket Malfunction |
| `data/sounds/shippurr1.ogg` | 3.599 | 50 | sounds.dat | live | `shiphappy` -> EscapeRocket Happy |
| `data/sounds/shippurr2.ogg` | 3.413 | 47 | sounds.dat | live | `shiphappy` -> EscapeRocket Happy |
| `data/sounds/shippurr3.ogg` | 3.251 | 46 | sounds.dat | live | `shiphappy` -> EscapeRocket Happy |
| `data/sounds/shippurr4.ogg` | 3.529 | 50 | sounds.dat | live | `shiphappy` -> EscapeRocket Happy |
| `data/sounds/shippurr5.ogg` | 3.135 | 44 | sounds.dat | live | `shiphappy` -> EscapeRocket Happy |
| `data/sounds/shipscream1.ogg` | 3.781 | 55 | sounds.dat | live | `shipscream` -> EscapeRocket Damage |
| `data/sounds/shipscream2.ogg` | 3.931 | 59 | sounds.dat | live | `shipscream` -> EscapeRocket Damage |
| `data/sounds/silence1.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/silence2.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/silence3.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/silence4.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/silence5.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/silence6.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/silence7.ogg` | 0.250 | 4 | sounds.dat | live | `darwinnianburncry` -> Darwinian OnFire |
| `data/sounds/solar_panel.ogg` | 0.169 | 5 | sounds.dat | live | `solarpanelenable` -> SolarPanel TriggerSurge |
| `data/sounds/solar_panel2.ogg` | 1.384 | 15 | sounds.dat | live | `solarpanelenable` -> SolarPanel TriggerSurge |
| `data/sounds/solar_panel3.ogg` | 0.444 | 7 | sounds.dat | live | `solarpanelenable` -> SolarPanel TriggerSurge |
| `data/sounds/solar_panel4.ogg` | 0.446 | 7 | sounds.dat | live | `solarpanelenable` -> SolarPanel TriggerSurge |
| `data/sounds/solar_panel5.ogg` | 0.303 | 6 | sounds.dat | live | `solarpanelenable` -> SolarPanel TriggerSurge |
| `data/sounds/solarcharge.ogg` | 3.713 | 39 | sounds.dat | live | SolarPanel Operate |
| `data/sounds/sonar.ogg` | 1.970 | 18 | sounds.dat | live | Spirit PickedUp |
| `data/sounds/soul_attack.ogg` | 1.736 | 16 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_attack2.ogg` | 2.020 | 17 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_attack3.ogg` | 1.646 | 15 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_attack4.ogg` | 1.440 | 13 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_attack5.ogg` | 1.654 | 14 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_attack6.ogg` | 1.724 | 15 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_attack7.ogg` | 2.113 | 18 | sounds.dat | live | `soulattack` -> SoulDestroyer Attack |
| `data/sounds/soul_dest_create.ogg` | 0.807 | 9 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create10.ogg` | 1.945 | 17 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create11.ogg` | 2.006 | 18 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create2.ogg` | 0.995 | 11 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create3.ogg` | 0.834 | 10 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create4.ogg` | 1.101 | 11 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create5.ogg` | 1.182 | 12 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create6.ogg` | 1.370 | 13 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create7.ogg` | 1.526 | 14 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create8.ogg` | 1.272 | 13 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_dest_create9.ogg` | 1.381 | 13 | sounds.dat | live | `souldestroycreate` -> SoulDestroyer Create |
| `data/sounds/soul_enemy_sighted1.ogg` | 1.228 | 12 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted2.ogg` | 1.648 | 15 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted3.ogg` | 2.156 | 19 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted4.ogg` | 3.041 | 25 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted5.ogg` | 2.925 | 26 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted6.ogg` | 2.438 | 21 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted7.ogg` | 1.392 | 14 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted8.ogg` | 1.985 | 18 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/soul_enemy_sighted9.ogg` | 1.902 | 18 | sounds.dat | live | `soulenemysighted` -> SoulDestroyer EnemySighted |
| `data/sounds/souldeath.ogg` | 2.862 | 24 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath10.ogg` | 2.560 | 22 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath2.ogg` | 2.287 | 20 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath3.ogg` | 3.199 | 25 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath4.ogg` | 3.071 | 25 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath5.ogg` | 3.419 | 27 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath6.ogg` | 2.450 | 21 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath7.ogg` | 2.461 | 21 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath8.ogg` | 3.024 | 27 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/souldeath9.ogg` | 3.228 | 26 | sounds.dat | live | `souldestdeath` -> SoulDestroyer Die; TriffidEgg Die |
| `data/sounds/soulpain.ogg` | 2.873 | 19 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain10.ogg` | 2.548 | 20 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain2.ogg` | 1.608 | 14 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain3.ogg` | 2.583 | 20 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain4.ogg` | 3.199 | 25 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain5.ogg` | 3.349 | 22 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain6.ogg` | 2.171 | 17 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain7.ogg` | 2.264 | 16 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain8.ogg` | 1.689 | 15 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/soulpain9.ogg` | 2.769 | 22 | sounds.dat | live | `souldestpain` -> SoulDestroyer Create; SoulDestroyer LoseHealth |
| `data/sounds/spam_attack.ogg` | 6.200 | 50 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_attack2.ogg` | 6.372 | 51 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_attack3.ogg` | 6.618 | 52 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_attack4.ogg` | 8.104 | 64 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_attack5.ogg` | 6.409 | 51 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_attack6.ogg` | 7.814 | 61 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_attack7.ogg` | 6.467 | 51 | sounds.dat | dead | `spamattack` -> Spam Attack |
| `data/sounds/spam_create.ogg` | 3.692 | 32 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create2.ogg` | 2.520 | 23 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create3.ogg` | 3.864 | 33 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create4.ogg` | 2.851 | 25 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create5.ogg` | 2.720 | 26 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create6.ogg` | 2.893 | 29 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create7.ogg` | 1.977 | 19 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_create8.ogg` | 2.434 | 22 | sounds.dat | dead | `samcreate` -> Spam Create |
| `data/sounds/spam_damage.ogg` | 3.947 | 33 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_damage2.ogg` | 3.500 | 29 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_damage3.ogg` | 3.796 | 30 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_damage4.ogg` | 3.721 | 32 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_damage5.ogg` | 3.982 | 33 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_damage6.ogg` | 4.296 | 36 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_damage7.ogg` | 4.296 | 35 | sounds.dat | dead | `emaildamage` -> Spam Damage |
| `data/sounds/spam_death.ogg` | 4.574 | 37 | sounds.dat | dead | `spamboom` -> Spam Explode |
| `data/sounds/spam_death2.ogg` | 4.400 | 35 | sounds.dat | dead | `spamboom` -> Spam Explode |
| `data/sounds/spam_death3.ogg` | 4.690 | 37 | sounds.dat | dead | `spamboom` -> Spam Explode |
| `data/sounds/spam_death4.ogg` | 4.644 | 37 | sounds.dat | dead | `spamboom` -> Spam Explode |
| `data/sounds/spam_death5.ogg` | 4.574 | 37 | sounds.dat | dead | `spamboom` -> Spam Explode |
| `data/sounds/spam_death6.ogg` | 4.702 | 39 | sounds.dat | dead | `spamboom` -> Spam Explode |
| `data/sounds/speccyloader.ogg` | 58.979 | 235 | NOT-SHIPPED(sounds/speccyloader.ogg) | dead | Music LoaderSpeccy |
| `data/sounds/spider_create.ogg` | 2.253 | 22 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create1e.ogg` | 1.460 | 16 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create2.ogg` | 2.071 | 21 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create2e.ogg` | 1.695 | 17 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create3.ogg` | 1.788 | 18 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create4.ogg` | 1.782 | 19 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create4e.ogg` | 0.625 | 8 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create5.ogg` | 1.346 | 14 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create5e.ogg` | 0.622 | 8 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create6.ogg` | 2.054 | 21 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create6e.ogg` | 1.637 | 17 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create7.ogg` | 0.900 | 11 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create7e.ogg` | 1.025 | 12 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create8.ogg` | 1.724 | 17 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create8e.ogg` | 0.505 | 7 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create9.ogg` | 1.735 | 17 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_create9e.ogg` | 0.992 | 11 | sounds.dat | live | `spidercreate` -> Spider Create |
| `data/sounds/spider_death.ogg` | 3.146 | 27 | sounds.dat | live | `newsamplegroup13`, `spiderdeath` -> Spider Die; Spider LoseHealth |
| `data/sounds/spider_death2.ogg` | 2.815 | 23 | sounds.dat | live | `newsamplegroup13`, `spiderdeath` -> Spider Die; Spider LoseHealth |
| `data/sounds/spider_death3.ogg` | 3.140 | 27 | sounds.dat | live | `newsamplegroup13` -> Spider Die |
| `data/sounds/spider_death4.ogg` | 3.134 | 27 | sounds.dat | live | `newsamplegroup13` -> Spider Die |
| `data/sounds/spider_death5.ogg` | 3.134 | 27 | sounds.dat | live | `newsamplegroup13` -> Spider Die |
| `data/sounds/spider_death6.ogg` | 3.164 | 27 | sounds.dat | live | `newsamplegroup13` -> Spider Die |
| `data/sounds/spider_death7.ogg` | 3.146 | 27 | sounds.dat | live | `newsamplegroup13` -> Spider Die |
| `data/sounds/spider_egg_lay.ogg` | 1.199 | 13 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay10.ogg` | 2.641 | 25 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay2.ogg` | 1.843 | 19 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay3.ogg` | 1.669 | 17 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay4.ogg` | 1.910 | 20 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay5.ogg` | 1.518 | 15 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay6.ogg` | 1.843 | 18 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay7.ogg` | 1.544 | 16 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay8.ogg` | 1.950 | 18 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_egg_lay9.ogg` | 1.686 | 19 | sounds.dat | live | `spideregglay` -> Spider LayEgg; SporeGenerator LayEgg |
| `data/sounds/spider_foot.ogg` | 0.235 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot10.ogg` | 0.140 | 4 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot11.ogg` | 0.241 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot12.ogg` | 0.208 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot13.ogg` | 0.363 | 6 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot2.ogg` | 0.211 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot3.ogg` | 0.180 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot4.ogg` | 0.150 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot5.ogg` | 0.248 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot6.ogg` | 0.168 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot7.ogg` | 0.186 | 5 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot8.ogg` | 0.127 | 4 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_foot9.ogg` | 0.112 | 4 | sounds.dat | live | `spiderfeet` -> Spider FootFall |
| `data/sounds/spider_pounce.ogg` | 3.511 | 28 | sounds.dat | live | `spiderpounce` -> Spider Attack; Spider Pounce |
| `data/sounds/spider_pounce2.ogg` | 4.390 | 35 | sounds.dat | live | `spiderpounce` -> Spider Attack; Spider Pounce |
| `data/sounds/spider_pounce3.ogg` | 3.955 | 33 | sounds.dat | live | `spiderpounce` -> Spider Attack; Spider Pounce |
| `data/sounds/spider_pounce4.ogg` | 3.419 | 29 | sounds.dat | live | `spiderpounce` -> Spider Attack; Spider Pounce |
| `data/sounds/spider_pounce5.ogg` | 4.162 | 34 | sounds.dat | live | `spiderpounce` -> Spider Attack; Spider Pounce |
| `data/sounds/spider_pounce6.ogg` | 3.460 | 29 | sounds.dat | live | `spiderpounce` -> Spider Attack; Spider Pounce |
| `data/sounds/spirit_descent.ogg` | 1.679 | 11 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent10.ogg` | 2.302 | 8 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent11.ogg` | 10.865 | 24 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent12.ogg` | 1.494 | 9 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent13.ogg` | 3.169 | 12 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent14.ogg` | 4.067 | 15 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent15.ogg` | 1.632 | 7 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent16.ogg` | 1.494 | 6 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent17.ogg` | 1.506 | 6 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent18.ogg` | 2.284 | 7 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent19.ogg` | 2.280 | 7 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent2.ogg` | 3.268 | 14 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent20.ogg` | 2.269 | 7 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent21.ogg` | 2.428 | 8 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent22.ogg` | 0.864 | 5 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent23.ogg` | 4.665 | 14 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent24.ogg` | 4.945 | 14 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent25.ogg` | 4.748 | 14 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent3.ogg` | 1.329 | 6 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent4.ogg` | 2.081 | 8 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent5.ogg` | 4.084 | 12 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent6.ogg` | 4.444 | 14 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent7.ogg` | 3.473 | 11 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent8.ogg` | 3.174 | 10 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spirit_descent9.ogg` | 2.414 | 9 | sounds.dat | dead | `spiritdescent` -> SpiritProcessor Create; SpiritReceiver Create |
| `data/sounds/spore_gen_death.ogg` | 4.017 | 34 | sounds.dat | live | `sporegeneratordie` -> SporeGenerator Die |
| `data/sounds/spore_gen_death2.ogg` | 5.120 | 39 | sounds.dat | live | `sporegeneratordie` -> SporeGenerator Die |
| `data/sounds/spore_gen_death3.ogg` | 4.015 | 32 | sounds.dat | live | `sporegeneratordie` -> SporeGenerator Die |
| `data/sounds/spore_gen_death4.ogg` | 3.932 | 32 | sounds.dat | live | `sporegeneratordie` -> SporeGenerator Die |
| `data/sounds/spore_gen_death5.ogg` | 3.402 | 26 | sounds.dat | live | `sporegeneratordie` -> SporeGenerator Die |
| `data/sounds/spore_gen_death6.ogg` | 2.496 | 21 | sounds.dat | live | `sporegeneratordie` -> SporeGenerator Die |
| `data/sounds/spore_gen_groan2.ogg` | 5.565 | 44 | sounds.dat | live | `sporegengroan` -> SporeGenerator Create |
| `data/sounds/spore_gen_groan3.ogg` | 5.631 | 48 | sounds.dat | live | `sporegengroan` -> SporeGenerator Create |
| `data/sounds/spore_gen_groan4.ogg` | 4.679 | 39 | sounds.dat | live | `minecartcreak`, `sporegengroan` -> TrackLink CogTurn; SporeGenerator Create |
| `data/sounds/spore_gen_pain.ogg` | 3.878 | 33 | sounds.dat | live | `sporegenlosehealt` -> Triffid Damage; SporeGenerator LoseHealth |
| `data/sounds/spore_gen_pain2.ogg` | 4.696 | 36 | sounds.dat | live | `sporegenlosehealt` -> Triffid Damage; SporeGenerator LoseHealth |
| `data/sounds/spore_gen_pain3.ogg` | 3.954 | 31 | sounds.dat | live | `sporegenlosehealt` -> Triffid Damage; SporeGenerator LoseHealth |
| `data/sounds/spore_gen_pain4.ogg` | 3.262 | 27 | sounds.dat | live | `sporegenlosehealt` -> Triffid Damage; SporeGenerator LoseHealth |
| `data/sounds/spore_gen_pain5.ogg` | 2.409 | 21 | sounds.dat | live | `sporegenlosehealt` -> Triffid Damage; SporeGenerator LoseHealth |
| `data/sounds/spore_gen_pain6.ogg` | 2.363 | 19 | sounds.dat | live | `sporegenlosehealt` -> Triffid Damage; SporeGenerator LoseHealth |
| `data/sounds/squad_death.ogg` | 2.363 | 20 | sounds.dat | live | `squaddiedeath` -> Squadie Die |
| `data/sounds/squad_death2.ogg` | 1.945 | 17 | sounds.dat | live | `squaddiedeath` -> Squadie Die |
| `data/sounds/squad_death3.ogg` | 1.945 | 18 | sounds.dat | live | `squaddiedeath` -> Squadie Die |
| `data/sounds/squad_death4.ogg` | 2.247 | 21 | sounds.dat | live | `squaddiedeath` -> Squadie Die |
| `data/sounds/squad_death5.ogg` | 1.846 | 17 | sounds.dat | live | `squaddiedeath` -> Squadie Die |
| `data/sounds/squad_lose_health1.ogg` | 0.772 | 9 | sounds.dat | live | `newsamplegroup5` -> Squadie LoseHealth |
| `data/sounds/squad_lose_health3.ogg` | 0.310 | 6 | sounds.dat | live | `newsamplegroup5` -> Squadie LoseHealth |
| `data/sounds/squad_lose_health4.ogg` | 0.463 | 7 | sounds.dat | live | `newsamplegroup5` -> Squadie LoseHealth |
| `data/sounds/squad_lose_health6.ogg` | 0.827 | 10 | sounds.dat | live | `newsamplegroup5` -> Squadie LoseHealth |
| `data/sounds/squaddie_chatter1.ogg` | 0.404 | 6 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter10.ogg` | 0.300 | 6 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter11.ogg` | 0.807 | 10 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter12.ogg` | 0.456 | 7 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter13.ogg` | 0.538 | 8 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter14.ogg` | 0.881 | 10 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter15.ogg` | 0.805 | 9 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter16.ogg` | 0.597 | 8 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter18.ogg` | 0.855 | 10 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter19.ogg` | 0.607 | 8 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter2.ogg` | 0.952 | 11 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter20.ogg` | 0.676 | 9 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter21.ogg` | 0.583 | 8 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter3.ogg` | 0.359 | 6 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter4.ogg` | 0.791 | 10 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter5.ogg` | 0.499 | 7 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter6.ogg` | 0.916 | 11 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter7.ogg` | 0.862 | 10 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter8.ogg` | 0.653 | 9 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squaddie_chatter9.ogg` | 0.486 | 7 | sounds.dat | live | `newsamplegroup1` -> Squadie Create |
| `data/sounds/squadlasers3.ogg` | 1.080 | 11 | sounds.dat | unused | orphan group `laserricochet` |
| `data/sounds/squadlasers4.ogg` | 1.010 | 11 | sounds.dat | unused | orphan group `laserricochet` |
| `data/sounds/squadlasers5.ogg` | 1.030 | 11 | sounds.dat | unused | orphan group `laserricochet` |
| `data/sounds/squadlasers6.ogg` | 1.016 | 11 | sounds.dat | unused | orphan group `laserricochet` |
| `data/sounds/squadlasers7.ogg` | 0.972 | 10 | sounds.dat | unused | orphan group `laserricochet` |
| `data/sounds/squadlasers8.ogg` | 1.059 | 11 | sounds.dat | unused | orphan group `laserricochet` |
| `data/sounds/tank_create.ogg` | 3.286 | 28 | sounds.dat | live | `tankcreate` -> Armour Create; Tank Create |
| `data/sounds/tank_create2.ogg` | 3.009 | 26 | sounds.dat | live | `tankcreate` -> Armour Create; Tank Create |
| `data/sounds/tank_create3.ogg` | 3.008 | 26 | sounds.dat | live | `tankcreate` -> Armour Create; Tank Create |
| `data/sounds/tank_create4.ogg` | 3.152 | 27 | sounds.dat | live | `tankcreate` -> Armour Create; Tank Create |
| `data/sounds/tank_create5.ogg` | 3.274 | 28 | sounds.dat | live | `tankcreate` -> Armour Create; Tank Create |
| `data/sounds/tank_create6.ogg` | 3.030 | 26 | sounds.dat | live | `tankcreate` -> Armour Create; Tank Create |
| `data/sounds/tank_damage.ogg` | 3.640 | 29 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage10.ogg` | 3.495 | 28 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage2.ogg` | 4.005 | 33 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage3.ogg` | 3.675 | 30 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage4.ogg` | 3.500 | 29 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage5.ogg` | 3.135 | 27 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage6.ogg` | 3.111 | 26 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage7.ogg` | 3.355 | 27 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage8.ogg` | 3.216 | 27 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_damage9.ogg` | 5.334 | 40 | sounds.dat | live | `tankloseheath` -> Armour LoseHealth; Tank LoseHealth |
| `data/sounds/tank_death10.ogg` | 5.039 | 37 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death2.ogg` | 4.139 | 32 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death3.ogg` | 3.733 | 30 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death4.ogg` | 3.547 | 28 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death5.ogg` | 4.087 | 33 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death6.ogg` | 4.023 | 33 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death7.ogg` | 3.541 | 28 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death8.ogg` | 4.017 | 33 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_death9.ogg` | 5.335 | 42 | sounds.dat | live | `tankdeath` -> Armour Die; Tank Die |
| `data/sounds/tank_hover.ogg` | 17.748 | 67 | sounds.dat | live | Armour Create |
| `data/sounds/taskin.ogg` | 3.126 | 43 | sounds.dat | dead | Interface Show |
| `data/sounds/taskout.ogg` | 0.972 | 16 | sounds.dat | dead | Interface Hide |
| `data/sounds/trash80_-_pain-fade-down.ogg` | 213.943 | 1533 | NOT-SHIPPED(sounds/trash80*) | dead | Music Credits; Music Cutscene3; Music Demo2Intro |
| `data/sounds/tree_atmos1.ogg` | 8.000 | 63 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos2.ogg` | 8.001 | 64 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos3.ogg` | 8.000 | 59 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos4.ogg` | 8.001 | 64 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos5.ogg` | 8.001 | 58 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos6.ogg` | 8.000 | 64 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos7.ogg` | 8.000 | 63 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_atmos8.ogg` | 8.000 | 61 | sounds.dat | live | `treeatmos` -> Tree CreateMagic; Tree CreateNormal |
| `data/sounds/tree_fire.ogg` | 1.491 | 17 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire10.ogg` | 1.366 | 17 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire11.ogg` | 0.436 | 7 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire12.ogg` | 0.567 | 9 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire13.ogg` | 1.153 | 14 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire14.ogg` | 1.735 | 20 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire2.ogg` | 0.929 | 12 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire3.ogg` | 1.181 | 14 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire4.ogg` | 0.673 | 9 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire5.ogg` | 0.819 | 11 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire6.ogg` | 0.928 | 12 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire7.ogg` | 0.497 | 8 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire8.ogg` | 1.154 | 14 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_fire9.ogg` | 1.735 | 20 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tree_on_fire.ogg` | 13.117 | 129 | sounds.dat | live | `treeonfire` -> Tree Burn |
| `data/sounds/tresk_-_excuses.ogg` | 344.555 | 2510 | NOT-SHIPPED(sounds/tresk*) | dead | Music Cutscene1; Music Cutscene5 |
| `data/sounds/tresk_-_faces-of-a-fashion.ogg` | 278.648 | 2066 | NOT-SHIPPED(sounds/tresk*) | dead | Music Cutscene2; Music Demo2Mid; Music EndSequence |
| `data/sounds/tresk_-_impact-of-silence.ogg` | 172.130 | 982 | NOT-SHIPPED(sounds/tresk*) | dead | Music Cutscene4 |
| `data/sounds/tresk_-weeklies-_schroeders-failure.ogg` | 103.579 | 485 | NOT-SHIPPED(sounds/tresk*) | dead | Music LoaderSoul |
| `data/sounds/triffid_burn.ogg` | 17.992 | 148 | sounds.dat | live | `triffidburn` -> Triffid Burn |
| `data/sounds/triffid_burn2.ogg` | 17.893 | 145 | sounds.dat | live | `triffidburn` -> Triffid Burn |
| `data/sounds/triffid_burn3.ogg` | 17.969 | 144 | sounds.dat | live | `triffidburn` -> Triffid Burn |
| `data/sounds/triffid_egg_bounce.ogg` | 1.271 | 12 | sounds.dat | live | `triffideggbounce` -> TriffidEgg Bounce |
| `data/sounds/triffid_egg_bounce2.ogg` | 1.263 | 12 | sounds.dat | live | `triffideggbounce` -> TriffidEgg Bounce |
| `data/sounds/triffid_egg_bounce3.ogg` | 1.283 | 12 | sounds.dat | live | `triffideggbounce` -> TriffidEgg Bounce |
| `data/sounds/triffid_egg_bounce4.ogg` | 1.248 | 12 | sounds.dat | live | `triffideggbounce` -> TriffidEgg Bounce |
| `data/sounds/triffid_egg_bounce5.ogg` | 1.271 | 12 | sounds.dat | live | `triffideggbounce` -> TriffidEgg Bounce |
| `data/sounds/triffid_egg_burst.ogg` | 2.014 | 18 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst10.ogg` | 2.450 | 22 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst2.ogg` | 2.194 | 21 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst3.ogg` | 2.461 | 22 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst4.ogg` | 2.496 | 23 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst5.ogg` | 2.119 | 20 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst6.ogg` | 2.278 | 21 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst7.ogg` | 2.281 | 21 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst8.ogg` | 2.577 | 23 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_burst9.ogg` | 2.177 | 20 | sounds.dat | live | `triffeggbursts` -> TriffidEgg BurstOpen |
| `data/sounds/triffid_egg_launch.ogg` | 1.538 | 14 | sounds.dat | live | `triffidegglaunc` -> TriffidEgg Create |
| `data/sounds/triffid_egg_launch2.ogg` | 1.254 | 12 | sounds.dat | live | `triffidegglaunc` -> TriffidEgg Create |
| `data/sounds/triffid_egg_launch3.ogg` | 1.239 | 12 | sounds.dat | live | `triffidegglaunc` -> TriffidEgg Create |
| `data/sounds/triffid_egg_launch4.ogg` | 1.158 | 12 | sounds.dat | live | `triffidegglaunc` -> TriffidEgg Create |
| `data/sounds/triffid_egg_launch5.ogg` | 0.990 | 10 | sounds.dat | live | `triffidegglaunc` -> TriffidEgg Create |
| `data/sounds/tripod_attack1.ogg` | 2.908 | 26 | sounds.dat | dead | `tprdattac` -> Tripod Attack |
| `data/sounds/tripod_attack2.ogg` | 2.287 | 21 | sounds.dat | dead | `tprdattac` -> Tripod Attack |
| `data/sounds/tripod_attack3.ogg` | 1.916 | 18 | sounds.dat | dead | `tprdattac` -> Tripod Attack |
| `data/sounds/tripod_attack4.ogg` | 2.206 | 20 | sounds.dat | dead | `tprdattac` -> Tripod Attack |
| `data/sounds/tripod_attack5.ogg` | 2.560 | 23 | sounds.dat | dead | `tprdattac` -> Tripod Attack |
| `data/sounds/tripod_create.ogg` | 1.361 | 14 | sounds.dat | dead | `tripodcreate` -> Tripod Create |
| `data/sounds/tripod_create2.ogg` | 1.791 | 17 | sounds.dat | dead | `tripodcreate` -> Tripod Create |
| `data/sounds/tripod_create3.ogg` | 1.588 | 15 | sounds.dat | dead | `tripodcreate` -> Tripod Create |
| `data/sounds/tripod_death1.ogg` | 2.914 | 26 | sounds.dat | dead | `ttripoddie` -> Tripod Die |
| `data/sounds/tripod_death2.ogg` | 3.495 | 31 | sounds.dat | dead | `ttripoddie` -> Tripod Die |
| `data/sounds/tripod_death3.ogg` | 3.019 | 29 | sounds.dat | dead | `ttripoddie` -> Tripod Die |
| `data/sounds/tripod_death4.ogg` | 2.746 | 25 | sounds.dat | dead | `ttripoddie` -> Tripod Die |
| `data/sounds/tripod_lose_health.ogg` | 1.533 | 16 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/tripod_lose_health2.ogg` | 1.416 | 14 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/tripod_lose_health3.ogg` | 2.003 | 19 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/tripod_lose_health4.ogg` | 1.718 | 15 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/tripod_lose_health5.ogg` | 2.119 | 19 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/tripod_lose_health6.ogg` | 1.447 | 13 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/tripod_lose_health7.ogg` | 1.149 | 12 | sounds.dat | dead | `trpodlosehealth` -> Tripod LoseHealth |
| `data/sounds/trunk_create.ogg` | 3.730 | 27 | sounds.dat | dead | `portcreate` -> TrunkPort ReprogramComplete |
| `data/sounds/trunk_create2.ogg` | 3.722 | 28 | sounds.dat | dead | `portcreate` -> TrunkPort ReprogramComplete |
| `data/sounds/trunk_create3.ogg` | 3.729 | 27 | sounds.dat | dead | `portcreate` -> TrunkPort ReprogramComplete |
| `data/sounds/trunk_create4.ogg` | 3.675 | 27 | sounds.dat | dead | `portcreate` -> TrunkPort ReprogramComplete |
| `data/sounds/trunk_create5.ogg` | 3.743 | 27 | sounds.dat | dead | `portcreate` -> TrunkPort ReprogramComplete |
| `data/sounds/twoataris.ogg` | 87.484 | 848 | NOT-SHIPPED(sounds/twoataris*) | dead | Music LoaderGameOfLife |
| `data/sounds/virus_attack.ogg` | 0.392 | 7 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack2.ogg` | 0.473 | 8 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack3.ogg` | 0.469 | 8 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack4.ogg` | 0.594 | 9 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack5.ogg` | 0.715 | 10 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack6.ogg` | 0.416 | 8 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack7.ogg` | 0.445 | 8 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_attack8.ogg` | 0.459 | 8 | sounds.dat | live | `virusattack` -> Virii Attack |
| `data/sounds/virus_cd10.ogg` | 0.087 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd11.ogg` | 0.082 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd12.ogg` | 0.072 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd13.ogg` | 0.078 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd14.ogg` | 0.081 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd15.ogg` | 0.078 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd16.ogg` | 0.079 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd17.ogg` | 0.074 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd18.ogg` | 0.080 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd19.ogg` | 0.077 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd2.ogg` | 0.097 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd20.ogg` | 0.078 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd21.ogg` | 0.071 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd22.ogg` | 0.063 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd23.ogg` | 0.066 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd24.ogg` | 0.081 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd25.ogg` | 0.075 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd26.ogg` | 0.076 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd27.ogg` | 0.068 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd28.ogg` | 0.079 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd29.ogg` | 0.084 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd3.ogg` | 0.083 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd30.ogg` | 0.097 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd4.ogg` | 0.094 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd5.ogg` | 0.087 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd6.ogg` | 0.084 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd7.ogg` | 0.096 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd8.ogg` | 0.085 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_cd9.ogg` | 0.074 | 4 | sounds.dat | live | `newsamplegroup11` -> Virii ChangeDirection |
| `data/sounds/virus_lose_health.ogg` | 0.289 | 6 | sounds.dat | live | `viruspain` -> Virii LoseHealth |
| `data/sounds/virus_lose_health2.ogg` | 0.467 | 6 | sounds.dat | live | `viruspain` -> Virii LoseHealth |
| `data/sounds/virus_lose_health3.ogg` | 0.721 | 9 | sounds.dat | live | `viruspain` -> Virii LoseHealth |
| `data/sounds/white_noise.ogg` | 28.979 | 134 | sounds.dat | live | Ambience EnterLocation |
| `data/sounds/world1.ogg` | 8.983 | 55 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world2.ogg` | 5.816 | 31 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world3.ogg` | 4.644 | 26 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world4.ogg` | 5.712 | 34 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world5.ogg` | 5.631 | 42 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world6.ogg` | 6.269 | 41 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world7.ogg` | 5.596 | 40 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/world8.ogg` | 6.484 | 45 | sounds.dat | dead | `worldviewwet` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry1.ogg` | 3.736 | 29 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry2.ogg` | 9.262 | 60 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry3.ogg` | 3.621 | 29 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry4.ogg` | 3.682 | 28 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry5.ogg` | 5.028 | 36 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry6.ogg` | 4.471 | 32 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry7.ogg` | 3.513 | 27 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/worlddry8.ogg` | 1.910 | 18 | sounds.dat | dead | `worldviewdry` -> Ambience EnterGlobalWorld |
| `data/sounds/yard.ogg` | 4.807 | 36 | sounds.dat | dead | Yard Create |

### `data/mwsounds/` (378 files)

Columns: duration s; size KiB; archive (`menusounds.dat` / `sounds.dat` / `sounds2.dat` / NOT-SHIPPED(exclusion pattern)); status (live = referenced by an event fired in FULL, dead = referenced only by unreachable/never-fired events, unused = no blueprint reference); referenced by (sample group -> events).

| Path | s | KiB | Archive | Status | Referenced by |
|---|---|---|---|---|---|
| `data/mwsounds/Attack1.ogg` | 0.748 | 16 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack10.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack11.ogg` | 0.600 | 14 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack12.ogg` | 0.398 | 10 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack13.ogg` | 1.190 | 23 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack14.ogg` | 0.900 | 18 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack15.ogg` | 0.632 | 14 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack16.ogg` | 0.664 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack17.ogg` | 0.727 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack18.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack19.ogg` | 1.100 | 22 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack2.ogg` | 0.661 | 14 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack20.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack21.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack23.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack3.ogg` | 0.801 | 16 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack32.ogg` | 0.600 | 14 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack33.ogg` | 0.566 | 13 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack34.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack35.ogg` | 0.697 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack36.ogg` | 0.701 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack37.ogg` | 0.460 | 11 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack38.ogg` | 1.416 | 26 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack39.ogg` | 1.416 | 27 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack4.ogg` | 1.352 | 26 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack40.ogg` | 1.142 | 22 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack41.ogg` | 0.546 | 12 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack42.ogg` | 1.024 | 20 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack43.ogg` | 0.710 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack44.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack45.ogg` | 0.800 | 16 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack46.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack5.ogg` | 0.901 | 18 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack6.ogg` | 0.580 | 13 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack7.ogg` | 0.459 | 11 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack8.ogg` | 1.001 | 20 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attack9.ogg` | 0.600 | 13 | sounds.dat | live | `CombatVoxAttack` -> Darwinian SeenThreatAttack |
| `data/mwsounds/Attention1.ogg` | 1.152 | 19 | sounds.dat | unused | orphan group `officerabsorb` |
| `data/mwsounds/Attention2.ogg` | 0.912 | 19 | sounds.dat | unused | orphan group `officerabsorb` |
| `data/mwsounds/Attention3.ogg` | 0.818 | 15 | sounds.dat | unused | orphan group `officerabsorb` |
| `data/mwsounds/Attention4.ogg` | 0.648 | 14 | sounds.dat | unused | orphan group `officerabsorb` |
| `data/mwsounds/Aye1.ogg` | 0.167 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye2.ogg` | 0.117 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye3.ogg` | 0.147 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye4.ogg` | 0.154 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye5.ogg` | 0.087 | 5 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye6.ogg` | 0.133 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye7.ogg` | 0.160 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye8.ogg` | 0.188 | 7 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/Aye9.ogg` | 0.149 | 6 | sounds.dat | unused | orphan group `aye` |
| `data/mwsounds/beep_harsh_malfunction.ogg` | 0.840 | 14 | sounds2.dat | live | MultiwiniaInterface MouseError |
| `data/mwsounds/Bleep3.ogg` | 3.046 | 33 | sounds.dat | unused |  |
| `data/mwsounds/carry1.ogg` | 2.321 | 39 | sounds.dat | unused |  |
| `data/mwsounds/carry2.ogg` | 2.361 | 39 | sounds.dat | unused |  |
| `data/mwsounds/carry3.ogg` | 2.399 | 41 | sounds.dat | unused |  |
| `data/mwsounds/carry4.ogg` | 2.423 | 42 | sounds.dat | unused |  |
| `data/mwsounds/Carrying3a.ogg` | 3.006 | 21 | sounds.dat | live | `CarryStatue` -> Darwinian LiftBuilding |
| `data/mwsounds/Carrying3b.ogg` | 3.007 | 27 | sounds.dat | live | `CarryStatue` -> Darwinian LiftBuilding |
| `data/mwsounds/Carrying3c.ogg` | 3.007 | 29 | sounds.dat | live | `CarryStatue` -> Darwinian LiftBuilding |
| `data/mwsounds/Carrying3d.ogg` | 3.007 | 20 | sounds.dat | live | `CarryStatue` -> Darwinian LiftBuilding |
| `data/mwsounds/chat.ogg` | 0.216 | 4 | sounds2.dat | live | MultiwiniaInterface ChatMessage |
| `data/mwsounds/CountdownBeep1.ogg` | 1.991 | 27 | sounds.dat | unused |  |
| `data/mwsounds/CountdownBeep3.ogg` | 0.094 | 5 | sounds.dat | live | GunTurret RegisterKill; MultiwiniaInterface CountdownTick; MultiwiniaInterface CountdownZeroTick |
| `data/mwsounds/CountdownEndBeep.ogg` | 4.807 | 69 | sounds.dat | unused |  |
| `data/mwsounds/Crate_CaptureMyTeam.ogg` | 4.567 | 69 | sounds.dat | live | Crate BeginCapture |
| `data/mwsounds/Crate_CaptureOtherTeam.ogg` | 5.299 | 80 | sounds.dat | live | Crate BeginCapture |
| `data/mwsounds/Crate_LostCapture.ogg` | 2.525 | 31 | sounds.dat | live | Crate LostCapture |
| `data/mwsounds/CrateCapture_10a.ogg` | 16.475 | 251 | sounds.dat | live | Crate Capturing_0pc |
| `data/mwsounds/CrateCapture_20a.ogg` | 11.936 | 188 | sounds.dat | live | Crate Capturing_20pc |
| `data/mwsounds/CrateCapture_30a.ogg` | 15.406 | 237 | sounds.dat | live | Crate Capturing_40pc |
| `data/mwsounds/CrateCapture_40a.ogg` | 12.358 | 189 | sounds.dat | live | Crate Capturing_60pc |
| `data/mwsounds/CrateCapture_50a.ogg` | 14.769 | 226 | sounds.dat | live | Crate Capturing_80pc |
| `data/mwsounds/CrateCapturedAccent.ogg` | 3.565 | 47 | sounds.dat | live | Crate Captured |
| `data/mwsounds/CrateFall1.ogg` | 12.707 | 192 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateFall2.ogg` | 12.746 | 193 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateFall3.ogg` | 12.725 | 179 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateFall4.ogg` | 12.472 | 186 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateFall5.ogg` | 12.783 | 193 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateFall6.ogg` | 12.774 | 192 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateFall7.ogg` | 13.260 | 196 | sounds.dat | live | `cratefalling` -> Crate Falling |
| `data/mwsounds/CrateLand1.ogg` | 4.025 | 64 | sounds.dat | live | Crate Land |
| `data/mwsounds/CrateReward_Airstrike1.ogg` | 4.834 | 73 | sounds.dat | live | CratePowerup Airstrike |
| `data/mwsounds/CrateReward_AntsNest1.ogg` | 5.213 | 78 | sounds.dat | live | CratePowerup AntNest |
| `data/mwsounds/CrateReward_Armour1.ogg` | 3.511 | 48 | sounds.dat | live | CratePowerup Armour |
| `data/mwsounds/CrateReward_Armour2.ogg` | 3.511 | 48 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Bitzkrieg.ogg` | 5.726 | 90 | sounds.dat | live | CratePowerup Bitzkreig |
| `data/mwsounds/CrateReward_CrateMania1.ogg` | 1.142 | 19 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_CrateMania2.ogg` | 4.162 | 62 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_CrateMania3.ogg` | 4.162 | 63 | sounds.dat | live | CratePowerup CrateMania |
| `data/mwsounds/CrateReward_DarkForest1.ogg` | 5.219 | 83 | sounds.dat | live | CratePowerup DarkForest |
| `data/mwsounds/CrateReward_Egg1.ogg` | 3.747 | 60 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Egg2.ogg` | 4.209 | 63 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Egg3.ogg` | 4.910 | 73 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Egg4.ogg` | 3.063 | 37 | sounds.dat | live | CratePowerup Eggs |
| `data/mwsounds/CrateReward_Extinguisher.ogg` | 8.127 | 128 | sounds.dat | dead | CratePowerup FireExtinguisher |
| `data/mwsounds/CrateReward_Extinguisher2.ogg` | 3.808 | 57 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Extinguisher3.ogg` | 4.040 | 64 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Forest1.ogg` | 4.001 | 59 | sounds.dat | live | CratePowerup MagicalForest |
| `data/mwsounds/CrateReward_Forest2.ogg` | 12.055 | 189 | sounds.dat | live | Tree Create |
| `data/mwsounds/CrateReward_Formation1.ogg` | 1.122 | 20 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Formation2.ogg` | 3.235 | 46 | sounds.dat | dead | CratePowerup FormationBooster |
| `data/mwsounds/CrateReward_Futurewinians.ogg` | 6.284 | 89 | sounds.dat | live | CratePowerup SpaceShip |
| `data/mwsounds/CrateReward_GunTurret1.ogg` | 5.017 | 78 | sounds.dat | live | CratePowerup FlameTurret; CratePowerup GunTurret |
| `data/mwsounds/CrateReward_Harvester1.ogg` | 2.000 | 29 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Harvester2.ogg` | 5.953 | 88 | sounds.dat | live | CratePowerup Harvester |
| `data/mwsounds/CrateReward_Harvester3.ogg` | 4.986 | 76 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_HotFeet1.ogg` | 2.912 | 34 | sounds.dat | live | CratePowerup HotFeet |
| `data/mwsounds/Cratereward_Infection.ogg` | 4.000 | 59 | sounds.dat | live | CratePowerup Infection |
| `data/mwsounds/CrateReward_Meteor2.ogg` | 6.874 | 108 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Meteor3.ogg` | 7.651 | 119 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Meteor4.ogg` | 4.575 | 70 | sounds.dat | live | CratePowerup Meteor |
| `data/mwsounds/CrateReward_Nuke.ogg` | 8.125 | 122 | sounds.dat | live | CratePowerup Nuke |
| `data/mwsounds/CrateReward_OtherTeam.ogg` | 2.077 | 28 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Plague.ogg` | 6.875 | 111 | sounds.dat | live | CratePowerup Plague |
| `data/mwsounds/CrateReward_Rage1.ogg` | 3.309 | 56 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Rage2.ogg` | 3.231 | 56 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Rage3.ogg` | 1.616 | 30 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Rage4.ogg` | 4.313 | 66 | sounds.dat | live | CratePowerup Rage |
| `data/mwsounds/CrateReward_Randomiser.ogg` | 3.241 | 56 | sounds.dat | live | CratePowerup Randomiser |
| `data/mwsounds/CrateReward_Rocketwinians.ogg` | 3.628 | 49 | sounds.dat | live | CratePowerup Rocketwinians |
| `data/mwsounds/CrateReward_Shield.ogg` | 3.088 | 43 | sounds.dat | live | CratePowerup PersonalShield |
| `data/mwsounds/CrateReward_SpawnMania.ogg` | 8.078 | 120 | sounds.dat | live | CratePowerup SpawnMania |
| `data/mwsounds/CrateReward_SpawnMania2.ogg` | 8.078 | 121 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Squaddies.ogg` | 2.084 | 34 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Squaddies2.ogg` | 3.403 | 47 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Squaddies3.ogg` | 3.000 | 42 | sounds.dat | unused |  |
| `data/mwsounds/CrateReward_Squaddies4.ogg` | 2.031 | 32 | sounds.dat | live | CratePowerup Squad |
| `data/mwsounds/CrateReward_Subversion2.ogg` | 8.188 | 117 | sounds.dat | live | CratePowerup Subversion |
| `data/mwsounds/CrateReward_Tank.ogg` | 2.260 | 32 | sounds.dat | dead | CratePowerup Tank |
| `data/mwsounds/CrateReward_Triffid.ogg` | 3.269 | 45 | sounds.dat | live | CratePowerup Triffids |
| `data/mwsounds/CrateRewrad_Darwinians1.ogg` | 3.077 | 34 | sounds.dat | unused |  |
| `data/mwsounds/CrateRewrad_Darwinians2.ogg` | 6.620 | 71 | sounds.dat | live | CratePowerup Darwinians |
| `data/mwsounds/CreateReward_AntsNest1.ogg` | 5.213 | 78 | sounds.dat | unused |  |
| `data/mwsounds/CreateReward_Harvester1.ogg` | 2.000 | 29 | sounds.dat | unused |  |
| `data/mwsounds/CreateReward_Harvester2.ogg` | 5.953 | 88 | sounds.dat | unused |  |
| `data/mwsounds/CreateReward_Harvester3.ogg` | 4.986 | 76 | sounds.dat | unused |  |
| `data/mwsounds/DroneLoop.ogg` | 5.957 | 85 | sounds.dat | live | MultiwiniaInterface SelectionCircle |
| `data/mwsounds/DroneLoop2.ogg` | 3.619 | 41 | sounds.dat | unused |  |
| `data/mwsounds/DroneLoop3.ogg` | 2.175 | 22 | sounds.dat | unused |  |
| `data/mwsounds/Drop_Statue.ogg` | 2.508 | 36 | sounds.dat | unused |  |
| `data/mwsounds/Drop_Statue2.ogg` | 3.054 | 36 | sounds.dat | unused |  |
| `data/mwsounds/DSelect1.ogg` | 0.182 | 7 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect10.ogg` | 0.292 | 8 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect2.ogg` | 0.203 | 7 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect3.ogg` | 0.163 | 6 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect4.ogg` | 0.344 | 9 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect5.ogg` | 0.354 | 10 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect6.ogg` | 0.365 | 10 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect7.ogg` | 0.221 | 7 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect8.ogg` | 0.187 | 7 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DSelect9.ogg` | 0.146 | 6 | sounds.dat | live | `DSelect` -> Darwinian GivenOrdersFromPlayer; Darwinian Selected |
| `data/mwsounds/DStrain1.ogg` | 1.081 | 19 | sounds.dat | unused |  |
| `data/mwsounds/DStrain10.ogg` | 0.536 | 11 | sounds.dat | unused |  |
| `data/mwsounds/DStrain11.ogg` | 1.225 | 23 | sounds.dat | unused |  |
| `data/mwsounds/DStrain12.ogg` | 0.941 | 20 | sounds.dat | unused |  |
| `data/mwsounds/DStrain13.ogg` | 1.193 | 23 | sounds.dat | unused |  |
| `data/mwsounds/DStrain14.ogg` | 1.387 | 25 | sounds.dat | unused |  |
| `data/mwsounds/DStrain15.ogg` | 1.340 | 25 | sounds.dat | unused |  |
| `data/mwsounds/DStrain16.ogg` | 0.733 | 15 | sounds.dat | unused |  |
| `data/mwsounds/DStrain17.ogg` | 0.721 | 15 | sounds.dat | unused |  |
| `data/mwsounds/DStrain2.ogg` | 1.478 | 27 | sounds.dat | unused |  |
| `data/mwsounds/DStrain3.ogg` | 0.948 | 19 | sounds.dat | unused |  |
| `data/mwsounds/DStrain4.ogg` | 0.881 | 18 | sounds.dat | unused |  |
| `data/mwsounds/DStrain5.ogg` | 1.172 | 22 | sounds.dat | unused |  |
| `data/mwsounds/DStrain6.ogg` | 1.050 | 21 | sounds.dat | unused |  |
| `data/mwsounds/DStrain7.ogg` | 1.242 | 23 | sounds.dat | unused |  |
| `data/mwsounds/DStrain8.ogg` | 0.743 | 16 | sounds.dat | unused |  |
| `data/mwsounds/DStrain9.ogg` | 1.657 | 29 | sounds.dat | unused |  |
| `data/mwsounds/Enemy_Capture_Statue.ogg` | 2.815 | 42 | sounds.dat | live | Statue ChangeTeam |
| `data/mwsounds/EnemyCaptureBugle1.ogg` | 2.985 | 38 | sounds.dat | live | MultiwiniaZone CaptureStatue |
| `data/mwsounds/FilterSweep.ogg` | 3.538 | 52 | sounds.dat | live | Gesture GestureBegin; MultiwiniaInterface SelectionCircle |
| `data/mwsounds/FlameTurret1.ogg` | 0.810 | 16 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret10.ogg` | 0.993 | 19 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret2.ogg` | 0.770 | 16 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret3.ogg` | 0.636 | 13 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret4.ogg` | 0.589 | 12 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret5.ogg` | 0.477 | 11 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret6.ogg` | 0.675 | 14 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret7.ogg` | 0.471 | 11 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret8.ogg` | 0.497 | 12 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurret9.ogg` | 0.541 | 12 | sounds.dat | live | `flameturretfire` -> GunTurret FireFlame |
| `data/mwsounds/FlameTurretFinal.ogg` | 2.204 | 31 | sounds.dat | unused |  |
| `data/mwsounds/FlameTurretLoop1.ogg` | 7.213 | 113 | sounds.dat | unused |  |
| `data/mwsounds/Formation1.ogg` | 0.434 | 10 | sounds.dat | unused | orphan group `officersetorderformation` |
| `data/mwsounds/Formation2.ogg` | 0.669 | 14 | sounds.dat | unused | orphan group `officersetorderformation` |
| `data/mwsounds/GameOver1.ogg` | 4.368 | 68 | sounds.dat | unused |  |
| `data/mwsounds/GameOver2.ogg` | 4.389 | 51 | sounds.dat | unused |  |
| `data/mwsounds/GDSpeech10.ogg` | 2.167 | 35 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech11.ogg` | 5.070 | 82 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech12.ogg` | 4.754 | 77 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech13.ogg` | 2.990 | 47 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech14.ogg` | 4.319 | 68 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech15.ogg` | 3.493 | 55 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech16.ogg` | 4.183 | 68 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech17.ogg` | 5.400 | 87 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech18.ogg` | 4.600 | 73 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech2.ogg` | 4.471 | 72 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech3.ogg` | 4.050 | 66 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech4.ogg` | 4.543 | 73 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech5.ogg` | 3.840 | 59 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech6.ogg` | 5.270 | 81 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech7.ogg` | 5.990 | 95 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech8.ogg` | 6.690 | 107 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/GDSpeech9.ogg` | 4.744 | 75 | sounds.dat | live | `gdspeechgroup` -> Tree CreateEvil |
| `data/mwsounds/High1.ogg` | 15.315 | 235 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/High2.ogg` | 16.230 | 263 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/High3.ogg` | 37.511 | 597 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/High4.ogg` | 15.511 | 244 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/High5.ogg` | 39.563 | 619 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/High6.ogg` | 35.229 | 548 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/High7.ogg` | 20.248 | 311 | sounds.dat | live | `highaltitude` -> Ambience EnterLocation |
| `data/mwsounds/Hit1.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit10.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit11.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit12.ogg` | 0.311 | 9 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit13.ogg` | 0.344 | 9 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit14.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit15.ogg` | 0.281 | 9 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit16.ogg` | 0.381 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit17.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit18.ogg` | 0.344 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit19.ogg` | 0.374 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit2.ogg` | 0.416 | 11 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit20.ogg` | 0.500 | 12 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit21.ogg` | 0.284 | 8 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit22.ogg` | 0.800 | 16 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit23.ogg` | 0.384 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit24.ogg` | 0.511 | 12 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit25.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit26.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit27.ogg` | 0.600 | 14 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit28.ogg` | 0.234 | 8 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit29.ogg` | 0.453 | 11 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit3.ogg` | 0.579 | 13 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit30.ogg` | 0.361 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit31.ogg` | 0.302 | 9 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit32.ogg` | 0.500 | 12 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit33.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit34.ogg` | 0.700 | 15 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit35.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit36.ogg` | 0.500 | 12 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit4.ogg` | 0.393 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit5.ogg` | 0.487 | 12 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit6.ogg` | 0.500 | 12 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit7.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit8.ogg` | 0.200 | 7 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/Hit9.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxInjured` -> Laser InjuredEntity |
| `data/mwsounds/KOH_Zone_Captured.ogg` | 1.951 | 26 | sounds.dat | live | MultiwiniaZone ChangeTeam |
| `data/mwsounds/KOH_Zone_Lost.ogg` | 2.786 | 37 | sounds.dat | live | MultiwiniaZone ChangeTeam |
| `data/mwsounds/KOH_ZoneActive.ogg` | 5.350 | 86 | sounds.dat | live | MultiwiniaZone Create |
| `data/mwsounds/Laser_Cock1.ogg` | 0.617 | 14 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock10.ogg` | 0.471 | 11 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock2.ogg` | 0.401 | 11 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock3.ogg` | 0.647 | 13 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock4.ogg` | 0.672 | 15 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock5.ogg` | 0.230 | 8 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock6.ogg` | 0.423 | 10 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock7.ogg` | 0.659 | 15 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock8.ogg` | 0.380 | 10 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Cock9.ogg` | 0.478 | 12 | sounds.dat | unused | orphan group `lasercock` |
| `data/mwsounds/Laser_Reload1.ogg` | 0.726 | 15 | sounds.dat | live | `laserreload` -> Darwinian Attack |
| `data/mwsounds/Laser_Reload2.ogg` | 0.900 | 17 | sounds.dat | live | `laserreload` -> Darwinian Attack |
| `data/mwsounds/Laser_Reload3.ogg` | 0.894 | 17 | sounds.dat | live | `laserreload` -> Darwinian Attack |
| `data/mwsounds/Laser_Reload4.ogg` | 1.068 | 16 | sounds.dat | live | `laserreload` -> Darwinian Attack |
| `data/mwsounds/Laser_Reload5.ogg` | 1.403 | 19 | sounds.dat | live | `laserreload` -> Darwinian Attack |
| `data/mwsounds/LaserHitDarwinian1.ogg` | 0.255 | 8 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian10.ogg` | 0.627 | 14 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian11.ogg` | 0.737 | 16 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian12.ogg` | 0.612 | 14 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian13.ogg` | 0.652 | 14 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian14.ogg` | 0.647 | 14 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian15.ogg` | 0.380 | 10 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian16.ogg` | 0.325 | 9 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian17.ogg` | 0.314 | 9 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian18.ogg` | 0.369 | 10 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian19.ogg` | 0.269 | 8 | sounds.dat | unused |  |
| `data/mwsounds/LaserHitDarwinian2.ogg` | 0.512 | 12 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian3.ogg` | 0.740 | 16 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian4.ogg` | 1.124 | 22 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian5.ogg` | 0.412 | 11 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian6.ogg` | 0.787 | 16 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian7.ogg` | 0.853 | 18 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian8.ogg` | 0.559 | 13 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/LaserHitDarwinian9.ogg` | 0.500 | 12 | sounds.dat | live | `laserhitdarwinian` -> Darwinian Die |
| `data/mwsounds/Marching1.ogg` | 1.600 | 18 | sounds.dat | unused | orphan group `officersetorderfollow` |
| `data/mwsounds/Marching2.ogg` | 1.600 | 18 | sounds.dat | unused | orphan group `officersetorderfollow` |
| `data/mwsounds/Marching3.ogg` | 1.600 | 15 | sounds.dat | unused | orphan group `officersetorderfollow` |
| `data/mwsounds/Marching4.ogg` | 1.600 | 20 | sounds.dat | unused | orphan group `officersetorderfollow` |
| `data/mwsounds/Marching5.ogg` | 1.600 | 17 | sounds.dat | unused | orphan group `officersetorderfollow` |
| `data/mwsounds/MeteorHitClose1.ogg` | 7.246 | 119 | sounds.dat | live | Meteor Land |
| `data/mwsounds/MeteorHitFar1.ogg` | 5.793 | 76 | sounds.dat | live | Meteor Land |
| `data/mwsounds/mouseclick1.ogg` | 0.107 | 5 | sounds2.dat | live | MultiwiniaInterface SelectUnit; MultiwiniaInterface ToggleUnitState |
| `data/mwsounds/MultiLobbyAmb1.ogg` | 31.113 | 479 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb10.ogg` | 28.365 | 420 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb2.ogg` | 24.142 | 373 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb3.ogg` | 32.643 | 493 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb4.ogg` | 36.021 | 535 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb5.ogg` | 21.658 | 337 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb6.ogg` | 19.420 | 294 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb7.ogg` | 21.002 | 314 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb8.ogg` | 24.948 | 381 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/MultiLobbyAmb9.ogg` | 32.704 | 506 | menusounds.dat | live | `MultiLobbyAmbiences` -> MultiwiniaInterface LobbyAmbience |
| `data/mwsounds/NukeDetonateFar.ogg` | 10.077 | 106 | sounds.dat | live | Nuke Explode |
| `data/mwsounds/NukeDetonateNear.ogg` | 7.246 | 83 | sounds.dat | live | Nuke Explode |
| `data/mwsounds/PinkNoiseLoop.ogg` | 5.000 | 79 | sounds.dat | unused |  |
| `data/mwsounds/Player_Capture_Statue.ogg` | 1.161 | 21 | sounds.dat | live | MultiwiniaZone CaptureStatue; Statue ChangeTeam |
| `data/mwsounds/PlayerCaptureBugle5.ogg` | 4.554 | 71 | sounds.dat | live | MultiwiniaZone CaptureStatue |
| `data/mwsounds/Retreat1.ogg` | 0.353 | 10 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat10.ogg` | 0.539 | 12 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat11.ogg` | 0.409 | 10 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat12.ogg` | 0.537 | 12 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat13.ogg` | 0.900 | 18 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat14.ogg` | 0.600 | 13 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat15.ogg` | 0.561 | 13 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat16.ogg` | 0.527 | 12 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat17.ogg` | 0.468 | 11 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat18.ogg` | 1.070 | 21 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat19.ogg` | 0.885 | 18 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat2.ogg` | 0.300 | 9 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat20.ogg` | 0.328 | 9 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat21.ogg` | 0.540 | 13 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat22.ogg` | 1.682 | 30 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat23.ogg` | 0.633 | 14 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat24.ogg` | 0.500 | 11 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat25.ogg` | 0.360 | 10 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat26.ogg` | 0.455 | 11 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat3.ogg` | 0.837 | 17 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat4.ogg` | 0.587 | 14 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat5.ogg` | 0.400 | 10 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat6.ogg` | 0.801 | 17 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat7.ogg` | 0.366 | 10 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat8.ogg` | 0.364 | 10 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/Retreat9.ogg` | 0.600 | 14 | sounds.dat | live | `CombatVoxRetreat` -> Darwinian SeenThreatRunAway |
| `data/mwsounds/RocketThrust2Loop.ogg` | 3.223 | 54 | sounds.dat | live | Meteor Create |
| `data/mwsounds/RocketThrustLoop.ogg` | 3.226 | 48 | sounds.dat | live | Nuke Create |
| `data/mwsounds/rollover1a.ogg` | 0.018 | 4 | sounds2.dat | live | `UnitHighlight` -> MultiwiniaInterface UnitHighlight |
| `data/mwsounds/rollover1b.ogg` | 0.024 | 4 | sounds2.dat | unused |  |
| `data/mwsounds/rollover1c.ogg` | 0.018 | 4 | sounds2.dat | live | `UnitHighlight` -> MultiwiniaInterface UnitHighlight |
| `data/mwsounds/Rumble2_Loop.ogg` | 1.404 | 12 | sounds.dat | live | Harvester Create; Meteor Create; SpaceShip Create |
| `data/mwsounds/SpawnPoint_PlayerTurnsOff.ogg` | 2.908 | 43 | sounds.dat | unused |  |
| `data/mwsounds/SpawnPoint_PlayerTurnsOff2.ogg` | 7.883 | 110 | sounds.dat | live | SpawnPoint TurnOff |
| `data/mwsounds/SpawnPoint_PlayerTurnsOn.ogg` | 4.726 | 69 | sounds.dat | unused |  |
| `data/mwsounds/SpawnPoint_PlayerTurnsOn2.ogg` | 9.391 | 123 | sounds.dat | live | SpawnPoint TurnOn |
| `data/mwsounds/SpawnPointActivate5.ogg` | 14.000 | 198 | sounds.dat | unused |  |
| `data/mwsounds/SpawnPointDeactivate4.ogg` | 4.924 | 80 | sounds.dat | unused |  |
| `data/mwsounds/StatueDrop.ogg` | 3.250 | 42 | sounds.dat | unused |  |
| `data/mwsounds/StatueDrop2.ogg` | 2.061 | 28 | sounds.dat | unused |  |
| `data/mwsounds/StatueDrop3.ogg` | 5.906 | 89 | sounds.dat | live | Statue Drop |
| `data/mwsounds/StatueDrop4.ogg` | 5.906 | 90 | sounds.dat | unused |  |
| `data/mwsounds/StatueSpawn1.ogg` | 1.277 | 21 | sounds.dat | live | Statue Create |
| `data/mwsounds/TankEngineFar.ogg` | 3.518 | 46 | sounds.dat | dead | Tank Create |
| `data/mwsounds/TankEngineNear.ogg` | 2.001 | 31 | sounds.dat | unused |  |
| `data/mwsounds/theramin1.ogg` | 16.000 | 219 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin2.ogg` | 19.598 | 257 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin3.ogg` | 18.112 | 224 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin4.ogg` | 16.235 | 212 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin5.ogg` | 15.145 | 225 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin6.ogg` | 16.057 | 233 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin7.ogg` | 23.429 | 321 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin8.ogg` | 16.872 | 238 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/theramin9.ogg` | 17.067 | 256 | sounds.dat | live | `theramins` -> SpaceShip Create |
| `data/mwsounds/TractorBeam2.ogg` | 3.994 | 62 | sounds.dat | live | SpaceShip TractorBeam |
| `data/mwsounds/UFO_Engine1.ogg` | 6.628 | 72 | sounds.dat | live | Harvester Create; SpaceShip Create |
| `data/mwsounds/UFO_Engine2.ogg` | 1.387 | 12 | sounds.dat | live | Harvester Create; SpaceShip Create |
| `data/mwsounds/YesSir1.ogg` | 0.300 | 8 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir10.ogg` | 0.314 | 9 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir11.ogg` | 0.309 | 9 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir12.ogg` | 0.295 | 8 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir2.ogg` | 0.204 | 7 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir3.ogg` | 0.255 | 8 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir4.ogg` | 0.293 | 9 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir5.ogg` | 0.314 | 9 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir6.ogg` | 0.284 | 9 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir7.ogg` | 0.270 | 8 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir8.ogg` | 0.270 | 8 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YesSir9.ogg` | 0.321 | 9 | sounds.dat | unused | orphan group `officerselected` |
| `data/mwsounds/YouLost2.ogg` | 7.148 | 86 | sounds.dat | unused |  |
| `data/mwsounds/YouWon.ogg` | 7.045 | 82 | sounds.dat | unused |  |
<!-- X2B:SAMPLES:END -->

## Cross-partition notes

- **P16 (sound engine)**: verified (§10.A); correction: music RELEASE values are not 0 (§10.F). Additions for P16 §10.10/§10.11: `Interface Show/Hide/Slide` and `Gesture GestureFail` are unreachable in Multiwinia (`TaskManagerInterfaceIcons`/`TaskManagerInterfaceGestures` are created only when `!App::Multiplayer()`, `code/taskmanager_interface.cpp:TaskManagerInterface::CreateTaskManager`; Show/Hide are commented out in `code/taskmanager_interface_multiwinia.cpp`); `Gesture GestureSuccess` is live through `Task::TargetEngineer`, `Task::TargetOfficer` and the Multiwinia task manager; dead data also includes `CratePowerup FlameTurret`, `GunTurret Damage`, `Egg Die`, `Officer Attack`, `Incubator Reprogramming`, `Generator ReprogramComplete`.
- **P04/P05/P06/P07 (entities/buildings)**: `GunTurret::Damage` has no sound (data `GunTurret Damage` dead); `Egg::ChangeHealth` has no sound; Officer never calls `Entity::Attack`; `Squadie ThrowGrenade` (the only reachable squad secondary) has no data while unreachable `ThrowAirStrike` has; `TrunkPort PowerUp`, `LaserFence Electrocute`, `Triffid LaunchEgg`, `Spider PounceLand`, `Centipede Grow/Panic/EnemySighted`, `Armour Load/UnloadDarwinian` are fired but silent. `Darwinian::BeginVictoryDance` can fire on the 10 MW maps without `PrimaryObjectives` (§Contradictions 3).
- **P08 (crates)**: `Crate::TriggerSoundEvent` callers and the `%p` decay-sound bug in `Crate::AdvanceOnGround` (§10.C); `RunGunTurret` reports every turret type as `GunTurret`; `Expired` and `Capturing_100pc` silent.
- **P02 / P18 (modes, game menu)**: match end (`Multiwinia::DeclareWinner`) fires `GameOver`/`YouWon`/`YouLost` with no data although samples exist; countdown ticks are partly render-driven (`Multiwinia::Render`).
- **P19 / P20 (scripts, loaders)**: music reachability confirmed dead/legacy (§10.F).
- **X1b (formats)**: `sounds.txt` has CRLF line ends and one duplicate `SAMPLEGROUP` (`officercreate`, first definition wins); `SAMPLE` lines carry no extension; unknown fields are ignored.
- **29/31 (packaging / build)**: archive membership rules (§10.B); `tools/patch.full.list` is Darwinia-only; `sounds2.dat` exists only because six late UI samples were never added to `tools/sounds.txt`.
- **X2a (texture catalogue)**: `data/sprites/sound.bmp` is the only image used by the audio tooling.

## Contradictions / open questions

1. **Stationary loops never stop** (`PulseBomb Create`, legacy `Generator ReprogramComplete`) — §10.E [I]; resolve by running an Assault match to completion.
2. **`menusounds.dat` early load has no effect** because all triggers wait for `m_soundsLoaded` (set after `sounds.dat`/`sounds2.dat`) — §10.B [I].
3. **Victory jumps in objective-less maps**: `Darwinian::BeginVictoryDance` treats an empty `m_primaryObjectives` list as victory; 47 of the 57 `data/levels/mp_*.txt` define `PrimaryObjectives` (45 with `NeverTrue`, 2 `mp_sandbox*` with `BuildingOnline`), 10 define none (`mp_assault_2p_3`, `mp_assault_3p_1`, `mp_assault_3p_3`, `mp_assault_4P_2`, `mp_blitzkrieg_4P_2`, `mp_domination_2P_1`, `mp_domination_4P_1`, `mp_koth_2P_3`, `mp_koth_2P_3b`, `mp_skirmish_3P_1`) → on those, idle team-0 Darwinians above y = 10 should randomly jump with `VictoryJump` [I: depends on the level loader leaving the list empty; P11/P02 to confirm].
4. **Crate decay sound bug** (`"Capturing_%pc"`) — §10.C [C].
5. **Base-class attribution is by method name**: overloads with different signatures (e.g. `Statue::Destroy()` vs `Building::Destroy(double)`) are treated as overrides; this can only mis-classify rows that are silent anyway (no data) [I].
6. **Render-driven triggers**: `Grenade|AirStrikeBomb Flash` (`ThrowableWeapon::Render`) and the in-game clock `CountdownTick` (`Multiwinia::Render`) fire from rendering, so they depend on the object/HUD being rendered [I]; a new engine should drive them from simulation/UI state.
7. **Officer/Squad acknowledgement voices recorded but unbound** (§10.H) — design intent (docs) vs shipped data; the new game should decide explicitly.
8. **P16 §27.10 music release**: P16 states all music blocks have RELEASE 0; the data has 3.29–20.71 s releases on 14 of 17 blocks (§10.F) [C]. Moot for Multiwinia (music unreachable) but relevant to anyone reusing the music design.

## [U] Unknowns

1. [U] Audible content of voice files (intelligible words vs Darwinian gibberish) — audition `data/mwsounds/YesSir*`, `Attack*`, `Retreat*`, `CrateReward_*`, `GDSpeech*`, `GameOver*`, `YouWon`, `YouLost2`.
2. [U] Whether the retail `sounds.dat`/`sounds2.dat`/`menusounds.dat` match this tree's manifests (archives are not in the repository) — list a retail install's archives (`code/lib/resource.cpp` parsing, P14 `scripts/P14_rar_list.py`).
3. [U] Provenance of non-music samples (in-house recordings vs licensed libraries) — affects only the legal note; all are treated as Introversion IP below.
4. [U] Behaviour of objective-less maps (Contradiction 3) and stationary loops (Contradiction 1) at runtime — play-test.

## Docs-vs-code discrepancies

| Doc | Doc says | Code/data does | Ref |
|---|---|---|---|
| `docs/multiwinia audio.docx` (P16) | Game Over / You've won / You've lost sounds | Events fired by `Multiwinia::DeclareWinner`; samples `GameOver1/2`, `YouWon`, `YouLost2` exist in `data/mwsounds/` but no EVENT block binds them | `data/sounds.txt` `OTHER MultiwiniaInterface` |
| `docs/sound changes.txt` (P16) | Crate `Expired`; carryable `Lift`/`Carry`/`Drop` | `Expired`, `Lift`, `Carry` fired but unbound; only `Statue Drop` bound (sample `StatueDrop3`; `StatueDrop`, `StatueDrop2`, `StatueDrop4`, `Drop_Statue`, `Drop_Statue2`, `carry1..4` unused) | §10.D, §10.J |
| `code/sound/sample_cache.cpp:CachedSampleManager::CleanUp` comment | "Largest song takes 13 MB" | the largest track decodes to 30 389 760 bytes, but no music is shipped; the largest **shipped** files are `data/sounds/altitude1.ogg` (91.3 s ≈ 8.05 MB decoded, dead data) and, live, `data/sounds/evil.ogg` (53.9 s ≈ 4.75 MB) | §10.I |
| `tools/unused_samples.sh`, `tools/unused_sample_groups.sh` | "run this script from the Darwinia data directory" | they ignore `data/mwsounds/` and orphan-group membership; CRLF breaks the comparison on POSIX shells | §10.J |
| P16 §10.2 | "130 SAMPLEGROUPs referencing 1120 sample entries" | 131 definitions / 1126 lines; 130 distinct names / 1120 distinct entries (duplicate `officercreate`) | §10.A |

## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| All 1332 sound samples (898 live) — Introversion recordings/designs, plus third-party music | audio | `data/sounds/`, `data/mwsounds/` (catalogue §25.B) | Commission a new set using the per-source counts of §10.K as the brief (≈ 900 live variants; 1–3 variants per one-shot is usually enough for prototypes) |
| Third-party music: `tresk_-_excuses`, `tresk_-_faces-of-a-fashion`, `tresk_-_impact-of-silence`, `tresk_-weeklies-_schroeders-failure` (artist "tresk"), `trash80_-_pain-fade-down` (artist "trash80"), `dma_sc-visitors_from_dreams` ("dma_sc") | audio (third-party, not shipped with Multiwinia) | `data/sounds/` | Do not reuse; commission `music_*` |
| Retro-homage loader tunes `fodder`, `speccyloader`, `pang`, `matrix`, `twoataris` | audio (possibly derived from third-party game music) | `data/sounds/` | Do not reuse |
| Lobby pad set `MultiLobbyAmb1..10`, altitude bed `High1..7`, `evil`, `alien_lands3`, `rumble1`, `white_noise`, `shipheartbeat1` | audio (signature ambience) | `data/mwsounds/`, `data/sounds/` | `amb_lobby_pad_01..`, `amb_altitude_01..`, `amb_ground_01..` |
| Voice/bark sets (`darwin_*`, `DSelect*`, `Attack*`, `Retreat*`, `Hit*`, `LaserHitDarwinian*`, `officer_*`, `squaddie_chatter*`, `GDSpeech*`, `YesSir*`, `Aye*`, `Attention*`, `Marching*`, `Formation*`, `DStrain*`) | audio (character voices) | `data/sounds/`, `data/mwsounds/` | New character voices; names `vo_unit_select_01`, `vo_unit_attack_01`, `vo_commander_ack_01` |
| Announcer stings `CrateReward_*` (incl. `CrateReward_Futurewinians`, `CrateReward_Rocketwinians`), `GameOver*`, `YouWon`, `YouLost2`, `Player_Capture_Statue`, `Enemy_Capture_Statue`, `KOH_Zone_*`, `Crate_Capture*` | audio + embedded IP names (Futurewinians, Rocketwinians, KOH, Statue) | `data/mwsounds/` | `vo_reward_<function>`, `vo_match_win`, `vo_match_lose`, `sfx_zone_captured` |
| File and group names embedding IP names: `darwin_*`, `darwinian_threat_*`, `darwiniandie`, `darwiniancreate`, `darwinnianonfire`, `darwinnianburncry`, `CrateRewrad_Darwinians*`, `sepulveda_appear`, `sepulveda_disappear`, `koh_zone*`, `KOH_ZoneActive`, `gdspeechgroup`, `souldestroyer`/`soul_dest_*`, `redvirus_*`, `multilobbyambiences`, `MultiLobbyAmb*` | name | `data/sounds.txt` SAMPLEGROUPs, file names | Functional names (`unit_die`, `unit_spawn`, `zone_active`) |
| Source/event names (IP-flavoured): `Darwinian`, `Officer`, `Squadie`, `SoulDestroyer`, `Virii`, `Sepulveda`, `MultiwiniaInterface`, `MultiwiniaZone`, `CratePowerup` reward names `Bitzkreig`, `Rocketwinians`, `Darwinians`, `Infection`, `Plague` | name | `data/sounds.txt`, `code/sound/soundsystem.cpp:GetSoundSourceName`, `code/worldobject/crate.cpp:Crate::GetName` | `Unit`, `Commander`, `Trooper`, `MatchUI`, `CaptureZone`, `Reward_<function>` |
| Encoder vendor strings (`Xiph.Org libVorbis I …`, `aoTuV b3`) | metadata (open source, BSD) | Ogg comment headers | none needed |

Placeholder plan [I]: (1) keep the binding model (source × event → blueprint → sample/group), (2) generate silent or synthesised placeholder `.ogg`/`.wav` files named by function for the 182 live events (one file per event, more variants later), (3) bind the currently silent but gameplay-relevant events listed in §10.D (match end, crate expiry, squad grenade throw, laser create/hit, turret target/rocket fire), (4) add a real music system (none exists in live content), (5) give every looping event an explicit stop rule and release all location sounds on exit.

## Appendix — dead/legacy code

- [B:DEAD] Music triggers in `code/loaders/{speccy,amiga,matrix,raytrace,gameoflife,fodder,soul}_loader.cpp` and `code/startsequence.cpp` (`USE_LOADERS`); `Sepulveda Appear/Disappear/TextAppear` in `code/sepulveda.cpp` (`USE_SEPULVEDA_HELP_TUTORIAL`); `App::CheckSounds` lobby-ambience trigger (commented out); `Spirit Create/BeginAscent/Dropped/PlacedInEgg/EggDestroyed`, `Laser HitEntity` (commented out); `CratePowerup FormationBooster/FireExtinguisher/Tank` callers (commented out).
- [B:LEGACY] `TaskManagerInterfaceIcons`/`TaskManagerInterfaceGestures` UI sounds, `ShamanInterface` mouse-over, script music/`ShowLogo`, `Ambience EnterGlobalWorld`, `CreditsLoader`, ControlTower/ResearchItem engineer reprogramming, `Building::ReprogramComplete`/`Building::Destroy` paths, all Darwinia-only building blueprints (`Powerstation`, `ResearchItem`, `Library`, `Generator`, `TrackLink`, `Refinery`, `Mine`, `Yard`, `Incubator`, `SpiritReceiver`, `SpiritProcessor`, `BlueprintStore/Console/Relay`, `Spam`, `GodDish`) and entity blueprints `LaserTroop`, `Tripod`; squad `ThrowAirStrike/ThrowController/FireRocket`; controller grenade `ExplodeController`.
- [B:DEBUG] `Tank` blueprints (Tank Battle mode); sound editor tooling (P16 §27.13).

