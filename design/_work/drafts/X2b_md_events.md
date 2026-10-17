| # | Source | Event | Data blocks (src name; pos/loop/... params) | Files | Code trigger site(s) [status] | Fired in FULL | Gameplay trigger |
|---|---|---|---|---|---|---|---|
| 1 | ENTITY LaserTroop | Attack | [Group 0lasers] Stat/Once md100.00 v=8.10 f=rnd 0.93..1.03 | 24 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 2 | ENTITY LaserTroop | Die | [Group darwiniandie] Stat/Once md100.00 v=8.50 f=rnd 0.83..1.09/once | 8 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 3 | ENTITY Engineer | Create | [Sample engineer_hover] Att/Loop md100.00 v=9.03 | 1 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 4 | ENTITY Engineer | BeginReprogramming | [Sample engineer_reprog_radar_loop] Att/Loop md100.00 v=9.00 | 1 | code/worldobject/engineer.cpp:Engineer::AdvanceToControlTower:L814 [FULL] ; code/worldobject/engineer.cpp:Engineer::AdvanceToResearchItem:L1127 [FULL] | yes |  |
| 5 | ENTITY Engineer | EndReprogramming | [Sample engineer_repog_radar_loop_end] Stat/Once md100.00 v=6.48 | 1 | code/worldobject/engineer.cpp:Engineer::Advance:L376 [FULL] ; code/worldobject/engineer.cpp:Engineer::Advance:L384 [FULL] ; code/worldobject/engineer.cpp:Engineer::Advance:L448 [FULL] ; code/worldobject/engineer.cpp:Engineer::Advance:L466 [commented-out] ; code/worldobject/engineer.cpp:Engineer::AdvanceReprogramming:L934 [FULL] | yes |  |
| 6 | ENTITY Engineer | LoseHealth | [Group engineerinjury] Stat/Once md200.00 v=10.00 f=rnd 1.58..1.89/once r=5.03 | 9 | code/worldobject/engineer.cpp:Engineer::ChangeHealth:L295 [FULL] ; code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 7 | ENTITY Engineer | Die | [Group engineerdie] Stat/Once md200.00 v=10.00 f=rnd 0.95..1.05/once | 5 | code/worldobject/engineer.cpp:Engineer::ChangeHealth:L302 [FULL] ; code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 8 | ENTITY Virii | Attack | [Group virusattack] Att/Once md200.00 v=9.03 f=rnd 0.91..1.12/once | 8 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] ; code/worldobject/virii.cpp:Virii::AdvanceAttacking:L447 [FULL] | yes |  |
| 9 | ENTITY Virii | LoseHealth | [Group viruspain] Stat/Once md200.00 v=10.00 f=rnd 0.81..1.01/once r=2.71 fx=Echo | 3 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 10 | ENTITY Virii | ChangeDirection | [Group newsamplegroup11] Att/Once md50.00 v=6.55 f=rnd 1.70..2.17/once | 29 | code/worldobject/virii.cpp:Virii::SearchForIdleDirection:L741 [FULL] ; code/worldobject/virii.cpp:Virii::SearchForIdleDirection:L765 [FULL] | yes |  |
| 11 | ENTITY Virii | Die | [Group newsamplegroup9] Att/Once md175.00 v=10.00 f=rnd 1.54..1.79/once r=10.06/once fx=Echo | 7 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 12 | ENTITY Squadie | Create | [Group newsamplegroup1] Att/Loop/MonoRandom md100.00 v=9.00/once r=5.23/once ld=Velocity 6.80..0.00->0.00..2.90/once fx=Echo | 20 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 13 | ENTITY Squadie | LoseHealth | [Group newsamplegroup5] Att/Once md100.00 v=9.00 f=0.91/once r=4.06/once fx=Echo | 4 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] ; code/worldobject/insertion_squad.cpp:Squadie::ChangeHealth:L625 [FULL] | yes |  |
| 14 | ENTITY Squadie | WeaponReturns | [Sample grenade_return3] Att/Once/MonoRandom md100.00 v=9.00/once | 1 | code/worldobject/insertion_squad.cpp:Squadie::Advance:L663 [FULL] | yes |  |
| 15 | ENTITY Squadie | ThrowAirStrike | [Sample airstrike_alarm] Att/Once md100.00 v=10.00 | 1 | code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L987 [FULL] | yes |  |
| 16 | ENTITY Squadie | FireLaser | [Group turretfire] Att/Once md100.00 v=8.77/once f=rnd 0.90..1.05/once | 6 | code/worldobject/insertion_squad.cpp:Squadie::Attack:L954 [FULL] | yes |  |
| 17 | ENTITY Squadie | Die | [Group squaddiedeath] Att/Once md100.00 v=8.97 f=rnd 0.76..1.05/once | 5 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] ; code/worldobject/insertion_squad.cpp:Squadie::ChangeHealth:L632 [FULL] | yes |  |
| 18 | ENTITY Squadie | EnterTeleport | [Group enterteleport] Stat/Once md100.00 v=9.00 | 6 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 19 | ENTITY Squadie | ExitTeleport | [Group exitteleport] Att/Once md100.00 v=9.10 | 6 | — | no-site |  |
| 20 | ENTITY Egg | Die | [Group eggsplat] Stat/Once md100.00 v=3.90 | 7 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 21 | ENTITY SporeGenerator | Create | [Group sporegengroan] Att/Loop md100.00 v=8.32 ld=HeightAboveGround 4.00..164.00->0.19..5.81 | 3 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 22 | ENTITY SporeGenerator | LoseHealth | [Group sporegenlosehealt] Att/Once md150.00 v=10.00 | 6 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 23 | ENTITY SporeGenerator | Die | [Group sporegeneratordie] Stat/Once md150.00 v=10.00/once f=rnd 0.86..1.10/once r=10.45 fx=Echo | 6 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 24 | ENTITY SporeGenerator | LayEgg | [Group spideregglay] Att/Once md100.00 v=9.03 | 10 | code/worldobject/sporegenerator.cpp:SporeGenerator::AdvanceEggLaying:L301 [FULL] | yes |  |
| 25 | ENTITY Tripod | Create | [Group tripodcreate] Att/Once md100.00 v=8.50 f=rnd 0.84..1.14/once | 3 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 26 | ENTITY Tripod | Attack | [Group tprdattac] Att/Once md100.00 v=8.50 f=rnd 0.98..1.15/once | 5 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 27 | ENTITY Tripod | LoseHealth | [Group trpodlosehealth] Att/Once md100.00 v=8.50 f=rnd 0.68..1.05/once | 7 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 28 | ENTITY Tripod | Die | [Group ttripoddie] Att/Once md100.00 v=8.50 f=rnd 0.81..1.02/once | 4 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 29 | ENTITY Centipede | Attack | [Group newsamplegroup25] Att/Once md100.00 v=10.00 f=rnd 0.88..1.08/once | 8 | code/worldobject/centipede.cpp:Centipede::Attack:L391 [FULL] ; code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 30 | ENTITY Centipede | Create | [Group newsamplegroup26] Att/LoopADSR md80.09 v=8.00 f=rnd 0.95..1.05/once | 4 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 31 | ENTITY Centipede | Die | [Group newsamplegroup27] Stat/Once md150.00 v=10.00 f=0.94 r=10.00 fx=Echo | 4 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 32 | ENTITY Centipede | LoseHealth | [Group newsamplegroup28] Att/Once md100.00 v=9.03 f=rnd 0.58..1.59 s0.56/once ld=Velocity 11.20..0.00->0.58..4.84/once | 5 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 33 | ENTITY SpaceInvader | DropGrenade | [Sample airstrike_drop] Att/Once md200.00 v=10.00 | 1 | code/worldobject/airstrike.cpp:SpaceInvader::Advance:L257 [FULL] | yes |  |
| 34 | ENTITY SpaceInvader | Create | [Sample airstrike_hover2] Att/Loop md200.00 v=10.00 f=Velocity 13.20..74.40->0.91..1.96 | 1 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 35 | ENTITY Spider | Create | [Group spidercreate] Att/Loop md100.00 v=8.93 f=rnd 0.77..1.21/once ld=Velocity 4.80..0.00->0.00..3.10 | 17 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 36 | ENTITY Spider | Die | [Group newsamplegroup13] Att/Once md200.00 v=10.00 f=Velocity 1.20..34.40->0.58..0.82 r=16.65 fx=DSound_Echo | 7 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 37 | ENTITY Spider | Attack | [Group spiderpounce] Att/Once md100.00 v=9.03 f=rnd 0.74..1.03/once | 6 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] ; code/worldobject/spider.cpp:Spider::AdvanceAttack:L493 [FULL] | yes |  |
| 38 | ENTITY Spider | Pounce | [Group spiderpounce] Att/Once md100.00 v=9.03 f=rnd 0.89..1.22/once | 6 | code/worldobject/spider.cpp:Spider::AdvanceAttack:L494 [FULL] | yes |  |
| 39 | ENTITY Spider | LayEgg | [Group spideregglay] Att/Once md100.00 v=9.03 | 10 | code/worldobject/spider.cpp:Spider::AdvanceEggLaying:L708 [FULL] | yes |  |
| 40 | ENTITY Spider | LoseHealth | [Group spiderdeath] Att/Once md100.00 v=9.00 | 2 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 41 | ENTITY Spider | FootFall | [Group spiderfeet] Att/Once md62.63 v=Velocity 0.00..27.20->4.71..8.06 | 13 | code/worldobject/spider.cpp:Spider::UpdateLegs:L298 [FULL] | yes |  |
| 42 | ENTITY Darwinian | Die | [Group darwiniandie] Stat/Once md100.00 v=10.00 r=10.00 \|\| [Group laserhitdarwinian] Stat/Once md100.00 v=10.00 | 20 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 43 | ENTITY Darwinian | LoseHealth | [Group darwinianlosehealth] Att/Once md100.00 v=8.00 f=1.06 r=10.00 fx=Echo \|\| [Group darwinianlosehealth] Att/Once md100.00 v=8.00 f=0.91 | 18 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 44 | ENTITY Darwinian | Create | [Group darwiniancreate] Att/Once/MonoNearest md100.00 v=8.50 f=rnd 0.58..1.59 s0.56/once r=1.16 ld=rnd 0.87..4.84/once fx=Echo | 15 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 45 | ENTITY Darwinian | GivenOrders | [Group officercreate] Att/Once md100.00 v=8.00 f=rnd 1.51..1.85/once | 6 | code/worldobject/darwinian.cpp:Darwinian::SearchForOfficers:L2441 [FULL] ; code/worldobject/darwinian.cpp:Darwinian::SearchForOfficers:L2464 [commented-out] ; code/worldobject/darwinian.cpp:Darwinian::GiveOrders:L2623 [commented-out] | yes |  |
| 46 | ENTITY Darwinian | SeenThreatRunAway | [Group darwinrunaway] Att/Once md100.00 v=9.00/once f=rnd 0.90..1.09/once \|\| [Group combatvoxretreat] Att/Once md100.00 v=9.00 | 36 | code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L2764 [FULL] ; code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L2793 [FULL] ; code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L3008 [FULL] | yes |  |
| 47 | ENTITY Darwinian | Attack | [Group 0lasers] Att/Once md85.90 v=9.00 \|\| [Group laserreload] Att/Once/MonoNearest md60.34 v=9.10 ld=5.03 | 29 | code/worldobject/darwinian.cpp:Darwinian::FireSubversion:L5004 [FULL] ; code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 48 | ENTITY Darwinian | VictoryJump | [Group victoryjum] Att/Once md10.12 v=8.00 f=rnd 0.96..1.21/once | 9 | code/worldobject/darwinian.cpp:Darwinian::BeginVictoryDance:L3243 [FULL] | yes |  |
| 49 | ENTITY Darwinian | OnFire | [Group darwinnianburncry] Stat/Once md101.16 v=9.35 ld=0.58 | 13 | code/worldobject/darwinian.cpp:Darwinian::AdvanceOnFire:L3591 [FULL] ; code/worldobject/darwinian.cpp:Darwinian::AdvanceSacraficing:L3668 [FULL] | yes |  |
| 50 | ENTITY Darwinian | Selected | [Group dselect] Att/Once md100.00 v=8.00 f=rnd 0.92..1.25/once | 10 | — | no-site |  |
| 51 | ENTITY Darwinian | GivenOrdersFromPlayer | [Group dselect] Att/Once md100.00 v=8.03 f=1.03 | 10 | code/worldobject/darwinian.cpp:Darwinian::AdvanceCarryingBuilding:L843 [FULL] | yes |  |
| 52 | ENTITY Darwinian | LiftBuilding | [Group carrystatue] Att/Once md100.00 v=8.00 f=rnd 0.93..1.21/once | 4 | code/worldobject/darwinian.cpp:Darwinian::AdvanceCarryingBuilding:L839 [FULL] | yes |  |
| 53 | ENTITY Darwinian | SeenThreatAttack | [Group combatvoxattack] Att/Once md100.00 v=9.00 | 37 | code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L3016 [FULL] | yes |  |
| 54 | ENTITY Darwinian | SeenThreatAttackHorn | [Group darwinseethreatattack] Att/Once md125.13 v=9.00 | 3 | code/worldobject/darwinian.cpp:Darwinian::SearchForThreats:L3026 [FULL] | yes |  |
| 55 | ENTITY Officer | Create | [Group officercreate] Att/Once md200.00 v=8.00 \|\| [Group officercreate] 2D/Once/MyTeam md100.00 v=8.58 | 6 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 56 | ENTITY Officer | LoseHealth | [Group darwinianlosehealth] Att/Once md100.03 v=9.71 | 18 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 57 | ENTITY Officer | Die | [Group darwiniandie] Att/Once md200.00 v=9.03 | 8 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 58 | ENTITY Officer | Attack | [Group darwinseethreatattack] Att/Once md200.00 v=9.03 | 3 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 59 | ENTITY Officer | EnterTeleport | [Group enterteleport] Att/Once md200.00 v=9.16 | 6 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 60 | ENTITY Officer | SetOrderGoto | [Group officerleftright] Att/Loop md100.00 v=8.03 \|\| [Group officercreate] Att/Once/MyTeam md173.77 v=9.13 | 23 | code/worldobject/officer.cpp:Officer::SetOrders:L1497 [FULL] ; code/worldobject/officer.cpp:Officer::SetNextMode:L1712 [FULL] ; code/worldobject/officer.cpp:Officer::SetPreviousMode:L1875 [FULL] | yes |  |
| 61 | ENTITY Officer | Selected | [Group officercreate] 2D/Once/MyTeam md100.00 v=8.06 | 6 | — | no-site |  |
| 62 | ENTITY Officer | SetOrderFormation | [Group officerleftright] Att/Loop md100.00 v=8.06 \|\| [Group officercreate] Att/Once/MyTeam md173.77 v=9.13 | 23 | code/worldobject/officer.cpp:Officer::SetFormation:L1578 [FULL] | yes |  |
| 63 | ENTITY ArmyAnt | Attack | [Group antatack] Att/Once md100.00 v=8.50 f=rnd 0.81..1.38/once | 6 | code/worldobject/armyant.cpp:ArmyAnt::AdvanceAttackEnemy:L329 [FULL] ; code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 64 | ENTITY ArmyAnt | Create | [Group antcreat] Att/Loop md61.12 v=rnd 5.52..8.00 s0.60 f=rnd 0.67..1.34 s0.45 ld=0.10 | 8 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 65 | ENTITY ArmyAnt | LoseHealth | [Group antlosehealth] Att/Once md100.00 v=8.03 f=rnd 0.85..1.23/once | 7 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 66 | ENTITY ArmyAnt | Die | [Group antdeath] Att/Once md100.00 v=8.06 f=rnd 0.70..1.28/once | 6 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 67 | ENTITY Armour | Create | [Sample tank_hover] Att/Loop md218.81 v=Velocity 0.00..2.40->9.55..10.00 f=Velocity 0.00..22.40->1.10..1.68 fx=Gargle \|\| [Group tankcreate] Att/Once md100.00 v=9.35 | 7 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 68 | ENTITY Armour | LoseHealth | [Group tankloseheath] Att/Once md141.42 v=10.00 f=rnd 0.88..1.05/once | 10 | code/worldobject/armour.cpp:Armour::ChangeHealth:L178 [FULL] ; code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 69 | ENTITY Armour | Die | [Group tankdeath] Stat/Once md222.43 v=10.00/once f=rnd 0.89..1.09/once | 9 | code/worldobject/armour.cpp:Armour::ChangeHealth:L214 [FULL] ; code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 70 | ENTITY Armour | SetOrders | [Group officerordergoto] Att/Once md162.69 v=9.03 | 4 | — | no-site |  |
| 71 | ENTITY SoulDestroyer | Create | [Group souldestroycreate] Att/Loop md100.00 v=9.71/once f=HeightAboveGround 0.00..300.00->1.01..1.65 a=0.19/once r=0.39/once \|\| [Group souldestpain] Att/Once md204.25 v=10.00 | 21 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 72 | ENTITY SoulDestroyer | LoseHealth | [Group souldestpain] Att/Once md200.00 v=10.00 | 10 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 73 | ENTITY SoulDestroyer | Die | [Group souldestdeath] Att/Once md200.00 v=10.00 f=rnd 0.78..1.05/once | 10 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 74 | ENTITY SoulDestroyer | Attack | [Group soulattack] Att/Once md200.00 v=10.00 f=rnd 0.78..1.01/once | 7 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] ; code/worldobject/souldestroyer.cpp:SoulDestroyer::Attack:L249 [FULL] | yes |  |
| 75 | ENTITY SoulDestroyer | EnemySighted | [Group soulenemysighted] Att/Loop md100.00 v=HeightAboveGround 812.00..0.00->0.00..8.68 f=HeightAboveGround 72.00..0.00->0.37..0.76 s0.69 a=0.58/once r=10.45 | 9 | code/worldobject/souldestroyer.cpp:SoulDestroyer::SearchForTargetEnemy:L364 [FULL] | yes |  |
| 76 | ENTITY TriffidEgg | Create | [Group triffidegglaunc] Att/Once md100.00 v=9.10 f=rnd 0.89..1.19/once | 5 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 77 | ENTITY TriffidEgg | Bounce | [Group triffideggbounce] Stat/Once md100.00 v=Velocity 0.00..7.60->0.16..9.84 f=Velocity 0.00..8.80->0.60..1.11/once | 5 | code/worldobject/triffid.cpp:TriffidEgg::Advance:L811 [FULL] | yes |  |
| 78 | ENTITY TriffidEgg | BurstOpen | [Group triffeggbursts] Stat/Once md100.00 v=9.55 f=rnd 0.91..0.99/once | 10 | code/worldobject/triffid.cpp:TriffidEgg::Advance:L834 [FULL] | yes |  |
| 79 | ENTITY TriffidEgg | Die | [Group souldestdeath] Stat/Once md100.00 v=10.00 | 10 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 80 | ENTITY Harvester | Create | [Sample ufo_engine2] Att/Loop md96.11 v=8.00 f=HeightAboveGround 0.00..984.00->0.32..2.47 \|\| [Sample ufo_engine1] Att/Loop md89.81 v=Velocity 3.60..1.60->8.06..8.90 \|\| [Sample rumble2_loop] Att/Loop md150.84 v=9.00 f=1.29 | 3 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 81 | ENTITY Harvester | DropSpirit | [Group incubatoraddspirit] Stat/Once md150.00 v=9.00 f=rnd 0.38..0.52/once r=15.10 fx=Gargle | 6 | code/worldobject/harvester.cpp:Harvester::AdvanceToSpawnPoint:L875 [FULL] | yes |  |
| 82 | ENTITY Nuke | Create | [Sample rocketthrustloop] Att/Loop md104.86 v=9.00 f=CameraDistance 100.00..60.00->0.41..1.00 | 1 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 83 | ENTITY Nuke | Explode | [Sample nukedetonatenear] Stat/Once md217.86 v=10.00 f=rnd 0.45..0.99/once \|\| [Sample nukedetonatefar] Stat/Once md338.76 v=9.77 f=rnd 0.45..0.74/once a=rnd 0.19..2.13/once | 2 | code/worldobject/nuke.cpp:Nuke::AdvanceToTarget:L225 [FULL] | yes |  |
| 84 | ENTITY Meteor | Land | [Sample meteorhitclose1] Stat/Once md200.00 v=10.00 f=1.03 \|\| [Sample meteorhitfar1] Stat/Once md450.00 v=CameraDistance 100.00..320.00->0.00..9.90 | 2 | code/worldobject/meteor.cpp:Meteor::Advance:L84 [FULL] | yes |  |
| 85 | ENTITY Meteor | Create | [Sample rocketthrust2loop] Att/Loop md97.96 v=10.00 f=HeightAboveGround 24.00..996.00->0.89..1.80 \|\| [Sample rumble2_loop] Att/Loop md18.52 v=9.00 | 2 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 86 | ENTITY Tank | Create | [Sample tankenginefar] Att/Loop md100.00 v=Velocity 0.00..2.40->8.39..10.00 f=Velocity 0.00..22.40->1.10..1.68 \|\| [Group tankcreate] Att/Once md100.00 v=9.35 | 7 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 87 | ENTITY Tank | LoseHealth | [Group tankloseheath] Att/Once md100.00 v=9.00 f=rnd 0.88..1.05/once | 10 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] ; code/worldobject/tank.cpp:Tank::ChangeHealth:L142 [FULL] | yes |  |
| 88 | ENTITY Tank | Die | [Group tankdeath] Stat/Once md100.00 v=10.00/once f=rnd 0.73..0.94/once | 9 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] ; code/worldobject/tank.cpp:Tank::ChangeHealth:L150 [FULL] | yes |  |
| 89 | ENTITY SpaceShip | Create | [Group theramins] Att/Loop md329.40 v=10.00 ld=rnd 0.00..5.81/once \|\| [Sample ufo_engine2] Att/Loop md208.04 v=10.00 f=HeightAboveGround 0.00..984.00->0.32..2.47 \|\| [Sample ufo_engine1] Att/Loop md144.99 v=Velocity 3.60..1.60->8.06..9.90 \|\| [Sample rumble2_loop] Att/Loop md300.84 v=10.00 f=1.29 | 12 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 90 | ENTITY SpaceShip | TractorBeam | [Sample tractorbeam2] Att/Loop md100.00 v=10.00 | 1 | code/worldobject/spaceship.cpp:SpaceShip::Advance:L188 [FULL] | yes |  |
| 91 | BUILDING RadarDish | BeginRotation | [Sample radar_start_turn] Att/Once md100.00 v=9.03 \|\| [Sample radar_motor_run] Att/Loop md100.00 v=8.97 | 2 | code/worldobject/radardish.cpp:RadarDish::Aim:L380 [FULL] | yes |  |
| 92 | BUILDING RadarDish | EndRotation | [Sample radar_stop_turn] Att/Once md100.00 v=9.00 | 1 | code/worldobject/radardish.cpp:RadarDish::Advance:L231 [FULL] | yes |  |
| 93 | BUILDING RadarDish | ReprogramComplete | [Sample radar_reprog_complete] Att/Once md200.00 v=8.97 r=17.61 | 1 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 94 | BUILDING RadarDish | ConnectionEstablished | [Group radarconnect] Att/Loop md50.00 v=8.16 | 4 | code/worldobject/radardish.cpp:RadarDish::Advance:L297 [FULL] | yes |  |
| 95 | BUILDING RadarDish | ConnectionLost | [Group radarlose] Att/Once md150.00 v=8.94 r=14.32 fx=Echo | 4 | code/worldobject/radardish.cpp:RadarDish::Advance:L289 [FULL] | yes |  |
| 96 | BUILDING LaserFence | Create | [Sample fence_hum] Att/Loop md20.00 v=10.00/once | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 97 | BUILDING LaserFence | Spark | [Group elec_cackle] Att/Once md100.00 v=7.00 f=rnd 0.84..1.40/once | 6 | code/worldobject/laserfence.cpp:LaserFence::Spark:L106 [FULL] | yes |  |
| 98 | BUILDING LaserFence | Damage | [Group elecfence@damage] Att/Once md100.00 v=10.00 f=rnd 0.94..1.09/once | 16 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 99 | BUILDING GunTurret | FireShell | [Group turretfire] Att/Once md91.92 v=10.00/once f=rnd 0.94..1.16/once | 6 | code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L577 [FULL] | yes |  |
| 100 | BUILDING GunTurret | Damage | [Group battlecannondamage] Att/Once md100.00 v=9.03/once f=rnd 0.75..0.93/once | 10 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 101 | BUILDING GunTurret | Create | [Group armourtransform] Stat/Once md100.00 v=10.00 | 6 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 102 | BUILDING GunTurret | FireFlame | [Group flameturretfire] Stat/Once md100.00 v=10.00 f=rnd 0.92..1.55/once | 10 | code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L578 [FULL] | yes |  |
| 103 | BUILDING GunTurret | RegisterKill | [Sample countdownbeep3] Att/Once md50.78 v=10.00/once f=1.20/once | 1 | code/worldobject/gunturret.cpp:GunTurret::RegisterKill:L209 [FULL] | yes |  |
| 104 | BUILDING Powerstation | Create | [Sample power_station_online] Att/Loop md100.00 v=10.00 | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 105 | BUILDING Tree | CreateNormal | [Group treeatmos] Att/LoopADSR md5.98 v=10.00 a=2.55/once s=rnd 5.81..9.68/once r=2.55 | 8 | code/worldobject/tree.cpp:Tree::Initialise:L101 [FULL] | yes |  |
| 106 | BUILDING Tree | CreateMagic | [Group treeatmos] Att/LoopADSR md5.98 v=10.00 a=2.55/once s=rnd 5.81..9.68/once r=2.55 | 8 | code/worldobject/tree.cpp:Tree::Initialise:L100 [FULL] | yes |  |
| 107 | BUILDING Tree | Burn | [Group treeonfire] Att/Loop md145.75 v=10.00 r=9.68 | 15 | code/worldobject/tree.cpp:Tree::Damage:L893 [FULL] | yes |  |
| 108 | BUILDING Tree | Create | [Sample cratereward_forest2] Att/Once md90.83 v=9.03 f=0.32 | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 109 | BUILDING Tree | CreateEvil | [Group gdspeechgroup] Att/Loop md19.12 v=10.00 ld=rnd 0.00..23.61/once | 17 | code/worldobject/tree.cpp:Tree::Initialise:L99 [FULL] | yes |  |
| 110 | BUILDING TrunkPort | ReprogramComplete | [Group portcreate] Att/Loop md100.00 v=7.00 | 5 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 111 | BUILDING ResearchItem | Create | [Group researchcreate] Att/Loop md100.00 v=7.00 | 9 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 112 | BUILDING ResearchItem | AquireResearch | [Group aquireresearch] Stat/Once md150.00 v=10.00 | 7 | code/worldobject/researchitem.cpp:ResearchItem::Advance:L110 [FULL] | yes |  |
| 113 | BUILDING Library | Create | [Sample ablaster] Att/Once md30.00 v=3.00 | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 114 | BUILDING Generator | ReprogramComplete | [Sample generator_online] Stat/Loop md100.00 v=9.84/once | 1 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 115 | BUILDING Generator | TriggerSurge | [Group generatorburst] Att/Once md100.00 v=7.00 f=rnd 0.86..1.21/once | 5 | code/worldobject/generator.cpp:PowerBuilding::TriggerSurge:L299 [FULL] | yes |  |
| 116 | BUILDING Pylon | TriggerSurge | [Group ylonelecsurge] Att/Once md50.00 v=7.10 f=rnd 0.94..1.55/once | 22 | code/worldobject/generator.cpp:PowerBuilding::TriggerSurge:L299 [FULL] | yes |  |
| 117 | BUILDING SolarPanel | TriggerSurge | [Group solarpanelenable] Att/Once md50.00 v=7.06/once f=rnd 0.89..1.24/once | 5 | code/worldobject/generator.cpp:PowerBuilding::TriggerSurge:L299 [FULL] | yes |  |
| 118 | BUILDING SolarPanel | Operate | [Sample solarcharge] Att/Loop md100.00 v=8.00 f=rnd 1.15..1.18 s0.24/once a=4.06 | 1 | code/worldobject/generator.cpp:SolarPanel::Advance:L753 [FULL] | yes |  |
| 119 | BUILDING TrackLink | CogTurn | [Group minecartcreak] Stat/Once md150.00 v=rnd 8.19..9.74/once | 43 | code/worldobject/mine.cpp:MineBuilding::Advance:L338 [FULL] | yes |  |
| 120 | BUILDING Refinery | CogTurn | [Group refineryrun] Att/Once md200.00 v=10.00/once | 8 | code/worldobject/mine.cpp:MineBuilding::Advance:L338 [FULL] | yes |  |
| 121 | BUILDING Refinery | Create | [Group mineambience] Att/Loop md500.00 v=9.00 f=0.62 | 6 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 122 | BUILDING Mine | CogTurn | [Group minerun] Att/Once md150.00 v=8.97 f=0.95/once fx=DSound_I3DL2Reverb | 7 | code/worldobject/mine.cpp:MineBuilding::Advance:L338 [FULL] | yes |  |
| 123 | BUILDING Yard | Create | [Sample yard] Att/Loop/MonoRandom md200.00 v=10.00 f=1.50 | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 124 | BUILDING Incubator | Reprogramming | [Sample engineer_reprog_radar_loop] Att/Once md200.00 v=8.06 | 1 | — | no-site |  |
| 125 | BUILDING Incubator | SpawnEntity | [Group incubatorspawn] Stat/Once md200.00 v=8.61 f=rnd 0.90..1.02/once r=7.16 | 6 | code/worldobject/incubator.cpp:Incubator::SpawnEntity:L245 [FULL] | yes |  |
| 126 | BUILDING Incubator | AddSpirit | [Group incubatoraddspirit] Att/Once md150.00 v=9.00 f=rnd 0.38..0.52/once r=15.10 fx=Gargle | 6 | code/worldobject/incubator.cpp:Incubator::AddSpirit:L265 [FULL] | yes |  |
| 127 | BUILDING AntHill | Damage | [Group 1anthilldamage] Att/Once md100.00 v=9.03 | 10 | code/worldobject/anthill.cpp:AntHill::Damage:L78 [FULL] ; code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 128 | BUILDING AntHill | Explode | [Group newsamplegroup3] Stat/Once md200.00 v=10.00 f=0.31 r=12.19 | 5 | code/worldobject/anthill.cpp:AntHill::Damage:L101 [FULL] ; code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 129 | BUILDING Triffid | Damage | [Group sporegenlosehealt] Stat/Once md100.00 v=10.00 f=1.67 r=12.00 | 6 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 130 | BUILDING Triffid | Burn | [Group triffidburn] Att/Loop md124.17 v=10.00 | 3 | code/worldobject/triffid.cpp:Triffid::Damage:L291 [FULL] | yes |  |
| 131 | BUILDING SpiritReceiver | Create | [Group spiritdescent] Att/LoopADSR md50.00 v=8.00 a=3.29 s=8.71 r=6.39 ld=3.87 | 25 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 132 | BUILDING SpiritProcessor | Create | [Group spiritdescent] Att/LoopADSR md100.00 v=8.00 a=3.29 s=7.74 r=8.32 ld=3.10 | 25 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 133 | BUILDING SpawnPoint | TurnOn | [Sample spawnpoint_playerturnson2] Att/Once md100.00 v=10.00 | 1 | code/worldobject/spawnpoint.cpp:SpawnPoint::RecalculateOwnership:L883 [FULL] | yes |  |
| 134 | BUILDING SpawnPoint | TurnOff | [Sample spawnpoint_playerturnsoff2] Att/Once md100.00 v=9.10 | 1 | code/worldobject/spawnpoint.cpp:SpawnPoint::RecalculateOwnership:L882 [FULL] | yes |  |
| 135 | BUILDING BlueprintStore | Create | [Group elec_cackle] Att/Loop md100.00 v=9.00 f=0.42 fx=DSound_Echo | 6 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 136 | BUILDING BlueprintConsole | Create | [Group relayhover] Att/Loop md100.00 v=10.00 f=0.30 | 4 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 137 | BUILDING BlueprintRelay | Create | [Group relayhover] Att/Loop md50.00 v=8.48/once | 4 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 138 | BUILDING Spam | Create | [Group samcreate] Att/Loop md100.00 v=9.03 | 8 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 139 | BUILDING Spam | Damage | [Group emaildamage] Att/Once md150.00 v=10.00 f=1.67 r=13.16 fx=DSound_Echo | 7 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 140 | BUILDING Spam | Attack | [Group spamattack] Stat/Once md100.00 v=10.00 f=rnd 1.00..1.28/once | 7 | code/worldobject/spam.cpp:Spam::SpawnInfection:L243 [FULL] | yes |  |
| 141 | BUILDING Spam | Explode | [Group spamboom] Stat/Once md200.00 v=10.00 f=rnd 0.92..1.21/once fx=DSound_Echo | 6 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] ; code/worldobject/spam.cpp:Spam::Damage:L94 [FULL] | yes |  |
| 142 | BUILDING Spam | CreateResearch | [Group researchcreate] Att/Loop md100.00 v=7.00 | 9 | code/worldobject/spam.cpp:Spam::SetAsResearch:L343 [FULL] | yes |  |
| 143 | BUILDING GodDish | ConnectToGod | [Group connectgod] Att/Loop md200.00 v=9.87 f=0.56 r=12.00 | 3 | code/worldobject/goddish.cpp:GodDish::Activate:L186 [FULL] | yes |  |
| 144 | BUILDING GodDish | DisconnectFromGod | [Group disconnectgod] Att/Once md250.00 v=10.00 | 3 | code/worldobject/goddish.cpp:GodDish::DeActivate:L199 [FULL] | yes |  |
| 145 | BUILDING GodDish | Create | [Group goddishcreate] Att/Loop md200.00 v=10.00 f=1.11 | 5 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 146 | BUILDING FuelGenerator | PumpUp | [Sample fuelpumpexhale1] Att/Once md202.34 v=8.45 f=1.03 \|\| [Group bigmetalgroan] Att/Once md100.00 v=8.87 f=1.02 | 6 | code/worldobject/rocket.cpp:FuelGenerator::Advance:L367 [FULL] | yes |  |
| 147 | BUILDING FuelGenerator | PumpDown | [Sample fuelpumpup1] Att/Once md100.00 v=8.77 f=0.98 \|\| [Group kickverb] Att/Once md195.22 v=10.00 f=0.99 | 4 | code/worldobject/rocket.cpp:FuelGenerator::Advance:L371 [FULL] | yes |  |
| 148 | BUILDING FuelPipe | PumpFuel | [Sample fuelpipe2_loop] Att/Loop md100.00 v=10.00 f=0.57 a=5.23 | 1 | code/worldobject/rocket.cpp:FuelPipe::Advance:L458 [FULL] | yes |  |
| 149 | BUILDING FuelStation | LoadPassenger | [Group loadpassenger] Stat/Once md75.41 v=9.06 r=4.65 | 5 | code/worldobject/rocket.cpp:FuelStation::BoardRocket:L618 [FULL] | yes |  |
| 150 | BUILDING EscapeRocket | Happy | [Group shiphappy] Att/Loop md100.00 v=6.94 f=rnd 0.60..0.99/once ld=4.65 \|\| [Sample shipheartbeat1] Att/Loop md200.10 v=10.00 f=rnd 0.72..0.82 s0.59/once | 6 | — | no-site |  |
| 151 | BUILDING EscapeRocket | Malfunction | [Sample shipmalfunction1] Att/Loop md163.95 v=9.55 | 1 | — | no-site |  |
| 152 | BUILDING EscapeRocket | Explode | [Sample engineexplode] Att/Once md318.60 v=10.00 r=13.35 \|\| [Group newsamplegroup27] Att/Once md190.11 v=10.00 f=0.62 | 5 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] ; code/worldobject/rocket.cpp:EscapeRocket::Damage:L1556 [FULL] | yes |  |
| 153 | BUILDING EscapeRocket | Damage | [Group shipscream] Stat/Once md200.00 v=10.00 f=1.05 | 2 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 154 | BUILDING EscapeRocket | EngineBurn | [Sample engineburn] Att/Loop md150.25 v=10.00 \|\| [Sample ignitionzap1] Att/Once md100.00 v=7.00 f=0.50 r=6.39 | 2 | code/worldobject/rocket.cpp:EscapeRocket::SetupSounds:L972 [FULL] | yes |  |
| 155 | BUILDING EscapeRocket | Unhappy | [Group shiphungry] Att/Loop/MonoNearest md200.00 v=9.35 ld=rnd 4.06..16.06/once | 7 | — | no-site |  |
| 156 | BUILDING EscapeRocket | Flight | [Group choir1] Att/Loop md10000.00 v=10.00 r=12.00 ld=rnd 0.00..3.48/once | 6 | — | no-site |  |
| 157 | BUILDING MultiwiniaZone | CaptureStatue | [Sample player_capture_statue] 2D/Once md200.00 v=10.00 \|\| [Sample playercapturebugle5] 2D/Once/MyTeam md200.00 v=9.29 \|\| [Sample enemycapturebugle1] 2D/Once/Enemy md200.00 v=10.00 | 3 | code/worldobject/multiwiniazone.cpp:MultiwiniaZone::Advance_CaptureTheStatue:L415 [FULL] | yes |  |
| 158 | BUILDING MultiwiniaZone | ChangeTeam | [Sample koh_zone_captured] Att/Once/MyTeam md300.00 v=10.00 \|\| [Sample koh_zone_lost] Att/Once/Enemy md300.00 v=10.00 | 2 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 159 | BUILDING MultiwiniaZone | Create | [Sample koh_zoneactive] Att/Loop md47.91 v=9.00 | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 160 | BUILDING ControlStation | Create | [Group relayhover] Att/Loop md70.89 v=8.84/once | 4 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 161 | BUILDING Crate | Land | [Sample crateland1] Att/Once md150.00 v=9.00 f=rnd 0.89..1.42/once | 1 | code/worldobject/crate.cpp:Crate::AdvanceFloating:L119 [FULL] | yes |  |
| 162 | BUILDING Crate | BeginCapture | [Sample crate_capturemyteam] Att/Once/MyTeam md100.00 v=10.00 \|\| [Sample engineer_reprog_radar_loop] Att/Loop md64.40 v=6.94 \|\| [Sample crate_captureotherteam] Att/Once/Enemy md100.00 v=10.00 | 3 | code/worldobject/crate.cpp:Crate::AdvanceOnGround:L693 [FULL] | yes |  |
| 163 | BUILDING Crate | LostCapture | [Sample crate_lostcapture] Att/Once md100.00 v=10.00 | 1 | code/worldobject/crate.cpp:Crate::AdvanceOnGround:L689 [FULL] | yes |  |
| 164 | BUILDING Crate | Capturing_0pc | [Sample cratecapture_10a] Att/LoopADSR md150.00 v=9.00 a=1.16 r=11.81 | 1 | code/worldobject/crate.cpp:Crate::AdvanceOnGround:L694 [FULL] | yes |  |
| 165 | BUILDING Crate | Capturing_20pc | [Sample cratecapture_20a] Att/LoopADSR md150.00 v=9.00 a=1.16 r=12.00 | 1 | — | no-site |  |
| 166 | BUILDING Crate | Capturing_40pc | [Sample cratecapture_30a] Att/LoopADSR md157.70 v=9.48 a=1.16 r=12.39 | 1 | — | no-site |  |
| 167 | BUILDING Crate | Capturing_60pc | [Sample cratecapture_40a] Att/LoopADSR md200.03 v=9.52 a=0.97 r=12.19 | 1 | — | no-site |  |
| 168 | BUILDING Crate | Capturing_80pc | [Sample cratecapture_50a] Att/LoopADSR md218.95 v=10.00 a=1.16 r=12.58 | 1 | — | no-site |  |
| 169 | BUILDING Crate | Captured | [Sample cratecapturedaccent] Stat/Once md200.00 v=10.00 | 1 | code/worldobject/crate.cpp:Crate::AdvanceOnGround:L648 [FULL] | yes |  |
| 170 | BUILDING Crate | Falling | [Group cratefalling] Att/LoopADSR md100.00 v=8.58 a=4.26 s=rnd 0.97..12.58/once r=4.65 fx=DSound_Echo | 7 | code/worldobject/crate.cpp:Crate::Initialise:L79 [FULL] | yes |  |
| 171 | BUILDING Statue | ChangeTeam | [Sample player_capture_statue] 2D/Once/MyTeam md100.00 v=7.97 \|\| [Sample enemy_capture_statue] 2D/Once/Enemy md100.00 v=7.97 | 2 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 172 | BUILDING Statue | Drop | [Sample statuedrop3] Att/Once md250.14 v=10.00 | 1 | code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance:L250 [FULL] | yes |  |
| 173 | BUILDING Statue | Create | [Sample statuespawn1] Stat/Once md249.31 v=10.00 | 1 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 174 | BUILDING PulseBomb | Create | [Group choir1] Stat/Loop md100.00 v=9.32 f=0.50 | 6 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 175 | OTHER Laser | InjuredEntity | [Group combatvoxinjured] Att/Once md100.00 v=9.55 | 36 | code/worldobject/weapons.cpp:Laser::Advance:L772 [FULL] | yes |  |
| 176 | OTHER Grenade | Explode | [Group newsamplegroup3] Stat/Once md200.00 v=10.00 r=10.00 fx=DSound_WavesReverb | 5 | — | no-site |  |
| 177 | OTHER Grenade | Create | [Group newsamplegroup4] Att/Once md100.00 v=9.55 f=rnd 0.74..1.54 s0.84 | 5 | — | no-site |  |
| 178 | OTHER Grenade | Flash | [Sample grenade_beep] Att/Once md100.00 v=7.90 | 1 | — | no-site |  |
| 179 | OTHER Grenade | ExplodeController | [Sample hypno_loop_run] Att/Loop md100.00 v=10.00 | 1 | code/worldobject/weapons.cpp:ControllerGrenade::Advance:L354 [FULL] | yes |  |
| 180 | OTHER Grenade | Bounce | [Group newsamplegroup6] Att/Once md74.60 v=Velocity 0.00..10.00->6.97..9.00/once f=rnd 0.96..1.56 s0.66/once | 6 | — | no-site |  |
| 181 | OTHER Rocket | Explode | [Group newsamplegroup7] Stat/Once md200.00 v=10.00 f=rnd 0.80..1.20 | 4 | code/worldobject/weapons.cpp:Rocket::Explode:L539 [FULL] | yes |  |
| 182 | OTHER Rocket | Create | [Sample rocket_fire4_run] Att/Loop md100.00 v=10.00 \|\| [Sample rocketlauncher] Att/Once md150.00 v=10.00 f=0.94/once | 2 | code/worldobject/weapons.cpp:Rocket::Initialise:L422 [FULL] | yes |  |
| 183 | OTHER AirStrikeBomb | Explode | [Group airstrikeexplode] Stat/Once md200.00 v=9.23 | 5 | — | no-site |  |
| 184 | OTHER AirStrikeBomb | Flash | [Sample grenade_beep] Att/Once md100.00 v=8.00 | 1 | — | no-site |  |
| 185 | OTHER Spirit | PickedUp | [Sample sonar] Att/Once md100.00 v=9.00 | 1 | code/worldobject/spirit.cpp:Spirit::CollectorArrives:L286 [FULL] | yes |  |
| 186 | OTHER Sepulveda | Appear | [Sample sepulveda_appear] Ed/Once md100.00 v=8.00/once | 1 | code/sepulveda.cpp:Sepulveda::Advance:L199 [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] | no-live-site |  |
| 187 | OTHER Sepulveda | Disappear | [Sample sepulveda_disappear] Ed/Once md100.00 v=7.97/once r=7.94 | 1 | code/sepulveda.cpp:Sepulveda::Advance:L207 [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] | no-live-site |  |
| 188 | OTHER Sepulveda | TextAppear | [Sample [???]] Att/Once md100.00 v=7.00 | 0 | code/sepulveda.cpp:Sepulveda::RenderTextBoxCutsceneMode:L699 [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] ; code/sepulveda.cpp:Sepulveda::RenderTextBoxTaskManagerMode:L864 [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] ; code/sepulveda.cpp:Sepulveda::RenderTextBox:L1124 [not-compiled(#if USE_SEPULVEDA_HELP_TUTORIAL)] | no-live-site |  |
| 189 | OTHER Gesture | GestureBegin | [Sample filtersweep] 2D/Once md100.00 v=6.97 r=0.39 | 1 | code/location_input.cpp:LocationInput::AdvanceTeamControl:L1137 [commented-out] ; code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L175 [FULL] ; code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::RunScreenZone:L678 [FULL] ; code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::RunScreenZone:L712 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1264 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1327 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L817 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L883 [FULL] | yes |  |
| 190 | OTHER Gesture | GestureSuccess | [Sample gesturesuccess] 2D/Once md100.00 v=10.00 | 1 | code/location_input.cpp:LocationInput::AdvanceTeamControl:L1138 [commented-out] ; code/taskmanager.cpp:Task::TargetSquad:L211 [FULL] ; code/taskmanager.cpp:Task::TargetEngineer:L224 [FULL] ; code/taskmanager.cpp:Task::TargetShaman:L234 [FULL] ; code/taskmanager.cpp:Task::TargetArmour:L252 [FULL] ; code/taskmanager.cpp:Task::TargetHarvester:L272 [FULL] ; code/taskmanager.cpp:Task::TargetOfficer:L420 [FULL] ; code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L209 [FULL] ; code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::RunScreenZone:L713 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1328 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L884 [FULL] | yes |  |
| 191 | OTHER Gesture | GestureFail | [Sample gesturefail] 2D/Once md100.00 v=10.00 | 1 | code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L222 [FULL] | yes |  |
| 192 | OTHER Ambience | EnterLocation | [Group newsamplegroup8] 2D/LoopADSR md100.00 v=Ypos 736.00..232.00->0.10..9.58 a=10.00 s=10.84 r=10.00 \|\| [Sample white_noise] 2D/Loop md500.00 v=Ypos 0.00..992.00->3.03..7.03 fx=Echo \|\| [Group highaltitude] 2D/LoopADSR md100.00 v=Ypos 0.00..988.00->0.06..5.97 f=rnd 0.75..1.35 s0.66/once a=10.84 s=rnd 1.94..16.45/once r=10.45 | 11 | code/main.cpp:?:L424 [FULL] | yes |  |
| 193 | OTHER Ambience | EnterGlobalWorld | [Group worldviewdry] 2D/Loop md100.00 v=6.84 r=8.32 ld=rnd 0.00..2.71/once fx=Echo \|\| [Group worldviewwet] 2D/Loop md100.00 v=6.10 ld=rnd 0.19..2.52/once \|\| [Group worldamb] 2D/LoopADSR md100.00 v=6.00 f=0.37 a=9.68 s=0.97 r=11.03 ld=0.39 fx=Echo \|\| [Sample altitude1] 2D/Loop md100.00 v=5.23 f=0.70 a=6.19 r=11.23 \|\| [Sample rumble1] 2D/Loop md100.00 v=7.74 f=0.60 a=5.42 r=6.00 | 22 | code/main.cpp:?:L705 [FULL] | yes |  |
| 194 | OTHER Music | Cutscene1 | [Sample tresk_-_excuses] 2D/Loop md100.00 v=10.00 r=20.13 | 1 | — | no-site |  |
| 195 | OTHER Music | Cutscene2 | [Sample tresk_-_faces-of-a-fashion] 2D/Loop md100.00 v=10.00 r=20.13 | 1 | — | no-site |  |
| 196 | OTHER Music | Cutscene3 | [Sample trash80_-_pain-fade-down] 2D/Loop md100.00 v=10.00 a=5.23 r=20.13 | 1 | — | no-site |  |
| 197 | OTHER Music | Cutscene4 | [Sample tresk_-_impact-of-silence] 2D/Loop md100.00 v=10.00 a=2.90 r=20.13 | 1 | — | no-site |  |
| 198 | OTHER Music | LoaderRaytrace | [Sample dma_sc-visitors_from_dreams] 2D/Loop md100.00 v=9.00 r=10.06 | 1 | code/loaders/raytrace_loader.cpp:RayTraceLoader::Run:L675 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 199 | OTHER Music | LoaderFodder | [Sample fodder] 2D/Once md100.00 v=9.03 | 1 | code/loaders/fodder_loader.cpp:FodderLoader::Run:L34 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 200 | OTHER Music | LoaderSoul | [Sample tresk_-weeklies-_schroeders-failure] 2D/Loop md100.00 v=10.00 r=10.06 | 1 | code/loaders/soul_loader.cpp:SoulLoader::Run:L207 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 201 | OTHER Music | EndSequence | [Sample tresk_-_faces-of-a-fashion] 2D/Once md100.00 v=10.00 a=5.23 r=3.29 | 1 | — | no-site |  |
| 202 | OTHER Music | Credits | [Sample trash80_-_pain-fade-down] 2D/Once md100.00 v=10.00 a=2.71 r=10.06 | 1 | code/loaders/credits_loader.cpp:CreditsLoader::Run:L519 [FULL] | yes |  |
| 203 | OTHER Music | StartSequence | [Sample dma_sc-visitors_from_dreams] 2D/Once md100.00 v=9.00 r=10.06 | 1 | code/startsequence.cpp:StartSequence::Advance:L76 [FULL] | yes |  |
| 204 | OTHER Music | Cutscene5 | [Sample tresk_-_excuses] 2D/Once md100.00 v=10.00 a=3.10 r=15.48 | 1 | — | no-site |  |
| 205 | OTHER Music | LoaderSpeccy | [Sample speccyloader] 2D/Once md100.00 v=9.00 | 1 | code/loaders/speccy_loader.cpp:SpeccyLoader::Run:L347 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 206 | OTHER Music | LoaderAmiga | [Sample pang] 2D/Once md100.00 v=9.00 r=10.06 | 1 | code/loaders/amiga_loader.cpp:AmigaLoader::Run:L221 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 207 | OTHER Music | LoaderMatrix | [Sample matrix] 2D/Once md100.00 v=9.03 r=7.55 | 1 | code/loaders/matrix_loader.cpp:MatrixLoader::Run:L75 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 208 | OTHER Music | LoaderGameOfLife | [Sample twoataris] 2D/Once md100.00 v=9.00 r=10.26 | 1 | code/loaders/gameoflife_loader.cpp:GameOfLifeLoader::Run:L320 [not-compiled(#if USE_LOADERS)] | no-live-site |  |
| 209 | OTHER Music | Demo2Intro | [Sample trash80_-_pain-fade-down] 2D/Once md100.00 v=9.03 | 1 | — | no-site |  |
| 210 | OTHER Music | Demo2Mid | [Sample tresk_-_faces-of-a-fashion] 2D/Once md100.00 v=9.00 r=20.71 | 1 | — | no-site |  |
| 211 | OTHER Interface | Show | [Sample taskin] 2D/Once md100.00 v=8.48 f=1.05 | 1 | code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::Advance:L245 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::Advance:L225 [commented-out] | yes |  |
| 212 | OTHER Interface | Hide | [Sample taskout] 2D/Once md100.00 v=8.52 f=0.72 | 1 | code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::HideTaskManager:L206 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::HideTaskManager:L191 [commented-out] | yes |  |
| 213 | OTHER Interface | Slide | [Sample menuscroll2] 2D/Once md100.00 v=9.48 | 1 | code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenEdges:L560 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1361 [FULL] | yes |  |
| 214 | OTHER Interface | DeleteTask | [Sample deselect1] 2D/Once md100.00 v=9.13 | 1 | code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1315 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L871 [FULL] | yes |  |
| 215 | OTHER Interface | MouseOverIcon | [Group mouseovericon] 2D/Once md100.00 v=7.00 | 2 | code/shaman_interface.cpp:ShamanInterface::AdvanceMouseInIcon:L79 [FULL] ; code/shaman_interface.cpp:ShamanInterface::AdvanceSelectionInput:L108 [FULL] ; code/taskmanager_interface.cpp:TaskManagerInterface::AdvanceTab:L274 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L983 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L994 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L1011 [FULL] ; code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::AdvanceScreenZones:L1102 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L555 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L566 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L583 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::AdvanceScreenZones:L671 [FULL] | yes |  |
| 216 | OTHER CratePowerup | Squad | [Sample cratereward_squaddies4] 2D/Once md200.35 v=10.00 f=1.21 | 1 | — | no-site |  |
| 217 | OTHER CratePowerup | Harvester | [Sample cratereward_harvester2] 2D/Once md100.00 v=9.13 | 1 | — | no-site |  |
| 218 | OTHER CratePowerup | Armour | [Sample cratereward_armour1] 2D/Once md100.00 v=9.06 | 1 | — | no-site |  |
| 219 | OTHER CratePowerup | Darwinians | [Sample craterewrad_darwinians2] 2D/Once md100.00 v=10.00 | 1 | — | no-site |  |
| 220 | OTHER CratePowerup | AntNest | [Sample cratereward_antsnest1] 2D/Once md100.00 v=10.00 f=0.75 | 1 | — | no-site |  |
| 221 | OTHER CratePowerup | Eggs | [Sample cratereward_egg4] 2D/Once md199.97 v=9.55 | 1 | — | no-site |  |
| 222 | OTHER CratePowerup | Airstrike | [Sample cratereward_airstrike1] 2D/Once md200.35 v=10.00 | 1 | — | no-site |  |
| 223 | OTHER CratePowerup | MagicalForest | [Sample cratereward_forest1] 2D/Once md200.00 v=9.00 f=0.51 | 1 | — | no-site |  |
| 224 | OTHER CratePowerup | DarkForest | [Sample cratereward_darkforest1] 2D/Once md200.00 v=9.52 | 1 | — | no-site |  |
| 225 | OTHER CratePowerup | CrateMania | [Sample cratereward_cratemania3] 2D/Once md200.00 v=9.55 f=0.81 | 1 | — | no-site |  |
| 226 | OTHER CratePowerup | GunTurret | [Sample cratereward_gunturret1] 2D/Once md200.00 v=9.16 | 1 | — | no-site |  |
| 227 | OTHER CratePowerup | Meteor | [Sample cratereward_meteor4] 2D/Once md200.00 v=9.39 | 1 | — | no-site |  |
| 228 | OTHER CratePowerup | HotFeet | [Sample cratereward_hotfeet1] 2D/Once md100.00 v=9.06 | 1 | — | no-site |  |
| 229 | OTHER CratePowerup | Nuke | [Sample cratereward_nuke] 2D/Once md100.00 v=9.06 | 1 | — | no-site |  |
| 230 | OTHER CratePowerup | Plague | [Sample cratereward_plague] 2D/Once md100.00 v=9.06 | 1 | — | no-site |  |
| 231 | OTHER CratePowerup | Randomiser | [Sample cratereward_randomiser] 2D/Once md100.00 v=9.00 | 1 | — | no-site |  |
| 232 | OTHER CratePowerup | PersonalShield | [Sample cratereward_shield] 2D/Once md100.00 v=10.00 | 1 | — | no-site |  |
| 233 | OTHER CratePowerup | Subversion | [Sample cratereward_subversion2] 2D/Once md200.00 v=8.94 | 1 | — | no-site |  |
| 234 | OTHER CratePowerup | SpawnMania | [Sample cratereward_spawnmania] 2D/Once md100.00 v=8.97 | 1 | — | no-site |  |
| 235 | OTHER CratePowerup | Bitzkreig | [Sample cratereward_bitzkrieg] 2D/Once md100.00 v=9.00 | 1 | — | no-site |  |
| 236 | OTHER CratePowerup | FormationBooster | [Sample cratereward_formation2] 2D/Once md100.00 v=9.00 | 1 | — | no-site |  |
| 237 | OTHER CratePowerup | Rocketwinians | [Sample cratereward_rocketwinians] 2D/Once md100.00 v=9.10 | 1 | — | no-site |  |
| 238 | OTHER CratePowerup | FireExtinguisher | [Sample cratereward_extinguisher] 2D/Once md100.00 v=9.00 | 1 | — | no-site |  |
| 239 | OTHER CratePowerup | Infection | [Sample cratereward_infection] 2D/Once md100.00 v=9.00 | 1 | — | no-site |  |
| 240 | OTHER CratePowerup | Rage | [Sample cratereward_rage4] 2D/Once md100.00 v=9.06 | 1 | — | no-site |  |
| 241 | OTHER CratePowerup | Tank | [Sample cratereward_tank] 2D/Once md100.00 v=8.97 | 1 | — | no-site |  |
| 242 | OTHER CratePowerup | FlameTurret | [Sample cratereward_gunturret1] 2D/Once md100.00 v=9.03 | 1 | — | no-site |  |
| 243 | OTHER CratePowerup | Triffids | [Sample cratereward_triffid] 2D/Once md100.00 v=9.10 | 1 | — | no-site |  |
| 244 | OTHER CratePowerup | SpaceShip | [Sample cratereward_futurewinians] 2D/Once md100.00 v=10.00 | 1 | — | no-site |  |
| 245 | OTHER MultiwiniaInterface | CountdownTick | [Sample countdownbeep3] 2D/Once md100.00 v=7.03 | 1 | code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2385 [FULL] ; code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2399 [FULL] ; code/multiwinia.cpp:Multiwinia::Render:L3660 [FULL] | yes |  |
| 246 | OTHER MultiwiniaInterface | CountdownZeroTick | [Sample countdownbeep3] 2D/Once md100.00 v=7.10 f=2.00 | 1 | code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2412 [FULL] ; code/multiwinia.cpp:Multiwinia::Render:L3664 [FULL] | yes |  |
| 247 | OTHER MultiwiniaInterface | SelectionCircle | [Sample filtersweep] 2D/Once md100.00 v=7.00 r=0.58 \|\| [Sample droneloop] 2D/Loop md100.00 v=8.84 a=1.35 r=0.97 | 2 | code/location_input.cpp:LocationInput::AdvanceTeamControl:L707 [FULL] | yes |  |
| 248 | OTHER MultiwiniaInterface | LobbyAmbience | [Group newsamplegroup8] 2D/LoopADSR md100.00 v=8.00 a=9.29 s=10.65 r=10.84 \|\| [Sample shipheartbeat1] 2D/Loop md200.10 v=8.50 f=0.50 \|\| [Group multilobbyambiences] 2D/LoopADSR md100.00 v=9.00 f=rnd 0.67..1.01/once s=10.65 r=10.26 | 14 | code/UI/GameMenuWindow.cpp:GameMenuWindow::Update:L507 [FULL] ; code/app.cpp:App::CheckSounds:L526 [commented-out] | yes |  |
| 249 | OTHER MultiwiniaInterface | MenuRollOver | [Group mouseovericon] 2D/Once md100.00 v=5.03 | 2 | code/UI/FancyColourButton.h:FancyColourButton::if:L231 [FULL] ; code/UI/LevelSelectButton.h:LevelSelectButton::Render:L328 [FULL] ; code/game_menu.cpp:GameMenuButton::Render:L361 [FULL] ; code/game_menu.cpp:GameMenuInputField::Render:L1339 [FULL] ; code/game_menu.cpp:GameMenuDropDown::Render:L1722 [FULL] ; code/game_menu.cpp:GameMenuCheckBox::Render:L1942 [FULL] ; code/interface/leaderboards_window.cpp:?:L146 [FULL] | yes |  |
| 250 | OTHER MultiwiniaInterface | MenuSelect | [Sample menuscroll2] 2D/Once md100.00 v=9.00 | 1 | code/UI/LevelSelectButton.h:LevelSelectButton::MouseUp:L25 [FULL] ; code/UI/MultiwiniaEditorButton.h:MultiwiniaEditorButton::MouseUp:L23 [FULL] ; code/game_menu.cpp:GameMenuButton::MouseUp:L222 [FULL] ; code/interface/helpandoptions_windows.cpp:?:L33 [FULL] ; code/interface/helpandoptions_windows.cpp:?:L50 [FULL] ; code/interface/helpandoptions_windows.cpp:?:L69 [FULL] ; code/interface/helpandoptions_windows.cpp:HelpAndOptionsWindow::Create:L258 [FULL] ; code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L368 [FULL] ; code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L389 [FULL] ; code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L410 [FULL] ; code/interface/mainmenus.cpp:GameOptionsWindow::CreateMenuControl:L430 [FULL] ; code/interface/mainmenus.cpp:ConfirmExitWindow::Create:L745 [FULL] ; code/interface/multiwinia_window.cpp:?:L153 [FULL] ; code/interface/multiwinia_window.cpp:?:L171 [FULL] | yes |  |
| 251 | OTHER MultiwiniaInterface | MenuCancel | [Sample menuscroll1] 2D/Once md100.00 v=8.00 | 1 | code/game_menu.cpp:GameMenuButton::MouseUp:L226 [FULL] ; code/game_menu.cpp:BackPageButton::MouseUp:L377 [FULL] ; code/interface/mainmenus.h:MenuCloseButton::MouseUp:L114 [FULL] | yes |  |
| 252 | OTHER MultiwiniaInterface | UnitHighlight | [Group unithighlight] Stat/Once md100.00 v=7.00 f=0.90 | 2 | code/gamecursor.cpp:GameCursor::GetHighlightedObject:L316 [FULL] | yes |  |
| 253 | OTHER MultiwiniaInterface | SelectUnit | [Sample mouseclick1] 2D/Once md100.00 v=6.97 f=0.33 | 1 | code/location_input.cpp:LocationInput::SelectObjectUnderMouse:L313 [FULL] | yes |  |
| 254 | OTHER MultiwiniaInterface | MouseError | [Sample beep_harsh_malfunction] 2D/Once md100.00 v=8.55 f=0.62 | 1 | code/location_input.cpp:LocationInput::SelectObjectUnderMouse:L338 [FULL] ; code/location_input.cpp:LocationInput::AdvanceTeamControl:L832 [FULL] | yes |  |
| 255 | OTHER MultiwiniaInterface | ToggleUnitState | [Sample mouseclick1] 2D/Once md100.00 v=9.03 f=0.58 | 1 | code/location_input.cpp:LocationInput::AdvanceTeamControl:L512 [FULL] ; code/location_input.cpp:LocationInput::AdvanceTeamControl:L526 [FULL] | yes |  |
| 256 | OTHER MultiwiniaInterface | ChatMessage | [Sample chat] 2D/Once md100.00 v=8.00 | 1 | code/UI/GameMenuWindow.cpp:GameMenuWindow::NewChatMessage:L1374 [FULL] ; code/location.cpp:Location::NewChatMessage:L4965 [FULL] | yes |  |
| 257 | OTHER Interface | SetOrderGoto | — (no data: silent) | 0 | code/location_input.cpp:LocationInput::AdvanceTeamControl:L690 [FULL] | yes |  |
| 258 | OTHER Ambience | ExitLocation | — (no data: silent) | 0 | code/main.cpp:?:L609 [FULL] | yes |  |
| 259 | OTHER MultiwiniaInterface | GameOver | — (no data: silent) | 0 | code/multiwinia.cpp:Multiwinia::DeclareWinner:L4233 [FULL] | yes |  |
| 260 | OTHER MultiwiniaInterface | YouWon | — (no data: silent) | 0 | code/multiwinia.cpp:Multiwinia::DeclareWinner:L4237 [FULL] | yes |  |
| 261 | OTHER MultiwiniaInterface | YouLost | — (no data: silent) | 0 | code/multiwinia.cpp:Multiwinia::DeclareWinner:L4241 [FULL] | yes |  |
| 262 | OTHER Interface | ShowLogo | — (no data: silent) | 0 | code/script.cpp:Script::RunCommand_ShowDarwinLogo:L551 [FULL] | yes |  |
| 263 | OTHER Gesture | GestureEnd | — (no data: silent) | 0 | code/taskmanager_interface_gestures.cpp:TaskManagerInterfaceGestures::AdvanceGestures:L199 [FULL] | yes |  |
| 264 | OTHER Interface | SelectTask | — (no data: silent) | 0 | code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::RunScreenZone:L1293 [FULL] ; code/taskmanager_interface_multiwinia.cpp:TaskManagerInterfaceMultiwinia::RunScreenZone:L848 [FULL] | yes |  |
| 265 | OTHER Interface | TaskManagerDeselectTask | — (no data: silent) | 0 | code/team.cpp:Team::SelectUnit:L244 [FULL] | yes |  |
| 266 | OTHER Interface | TaskManagerSelectTask | — (no data: silent) | 0 | code/team.cpp:Team::SelectUnit:L248 [FULL] | yes |  |
| 267 | ENTITY SpaceInvader | LoseHealth | — (no data: silent) | 0 | code/worldobject/airstrike.cpp:SpaceInvader::ChangeHealth:L304 [FULL] ; code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 268 | ENTITY SpaceInvader | Die | — (no data: silent) | 0 | code/worldobject/airstrike.cpp:SpaceInvader::ChangeHealth:L311 [FULL] ; code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 269 | ENTITY Armour | LoadDarwinian | — (no data: silent) | 0 | code/worldobject/armour.cpp:Armour::AddPassenger:L1646 [FULL] | yes |  |
| 270 | ENTITY Armour | UnloadDarwinian | — (no data: silent) | 0 | code/worldobject/armour.cpp:Armour::RemovePassenger:L1655 [FULL] | yes |  |
| 271 | BUILDING RadarDish | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 272 | BUILDING TrunkPort | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 273 | BUILDING Generator | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 274 | BUILDING Pylon | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 275 | BUILDING SolarPanel | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 276 | BUILDING TrackLink | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 277 | BUILDING Mine | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 278 | BUILDING Incubator | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 279 | BUILDING AntHill | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 280 | BUILDING Triffid | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 281 | BUILDING SpawnPoint | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 282 | BUILDING FuelGenerator | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 283 | BUILDING FuelPipe | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 284 | BUILDING FuelStation | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 285 | BUILDING EscapeRocket | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 286 | BUILDING Crate | Create | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Initialise:L157 [FULL] | yes |  |
| 287 | BUILDING LaserFence | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 288 | BUILDING GunTurret | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 289 | BUILDING Powerstation | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 290 | BUILDING Tree | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 291 | BUILDING ResearchItem | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 292 | BUILDING Library | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 293 | BUILDING Pylon | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 294 | BUILDING SolarPanel | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 295 | BUILDING TrackLink | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 296 | BUILDING Refinery | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 297 | BUILDING Mine | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 298 | BUILDING Yard | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 299 | BUILDING Incubator | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 300 | BUILDING AntHill | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 301 | BUILDING Triffid | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 302 | BUILDING SpiritReceiver | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 303 | BUILDING SpiritProcessor | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 304 | BUILDING SpawnPoint | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 305 | BUILDING BlueprintStore | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 306 | BUILDING BlueprintConsole | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 307 | BUILDING BlueprintRelay | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 308 | BUILDING Spam | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 309 | BUILDING GodDish | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 310 | BUILDING FuelGenerator | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 311 | BUILDING FuelPipe | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 312 | BUILDING FuelStation | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 313 | BUILDING EscapeRocket | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 314 | BUILDING MultiwiniaZone | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 315 | BUILDING ControlStation | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 316 | BUILDING Crate | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 317 | BUILDING Statue | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 318 | BUILDING PulseBomb | ReprogramComplete | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::ReprogramComplete:L272 [FULL] | yes |  |
| 319 | BUILDING RadarDish | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 320 | BUILDING LaserFence | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 321 | BUILDING GunTurret | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 322 | BUILDING Powerstation | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 323 | BUILDING Tree | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 324 | BUILDING TrunkPort | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 325 | BUILDING ResearchItem | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 326 | BUILDING Library | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 327 | BUILDING Generator | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 328 | BUILDING Pylon | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 329 | BUILDING SolarPanel | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 330 | BUILDING TrackLink | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 331 | BUILDING Refinery | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 332 | BUILDING Mine | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 333 | BUILDING Yard | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 334 | BUILDING Incubator | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 335 | BUILDING AntHill | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 336 | BUILDING Triffid | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 337 | BUILDING SpiritReceiver | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 338 | BUILDING SpiritProcessor | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 339 | BUILDING SpawnPoint | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 340 | BUILDING BlueprintStore | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 341 | BUILDING BlueprintConsole | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 342 | BUILDING BlueprintRelay | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 343 | BUILDING Spam | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 344 | BUILDING GodDish | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 345 | BUILDING FuelGenerator | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 346 | BUILDING FuelPipe | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 347 | BUILDING FuelStation | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 348 | BUILDING EscapeRocket | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 349 | BUILDING ControlStation | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 350 | BUILDING Crate | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 351 | BUILDING PulseBomb | ChangeTeam | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::SetTeamId:L293 [FULL] | yes |  |
| 352 | BUILDING RadarDish | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 353 | BUILDING Powerstation | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 354 | BUILDING Tree | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 355 | BUILDING TrunkPort | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 356 | BUILDING ResearchItem | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 357 | BUILDING Library | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 358 | BUILDING Generator | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 359 | BUILDING Pylon | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 360 | BUILDING SolarPanel | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 361 | BUILDING TrackLink | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 362 | BUILDING Refinery | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 363 | BUILDING Mine | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 364 | BUILDING Yard | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 365 | BUILDING Incubator | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 366 | BUILDING SpiritReceiver | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 367 | BUILDING SpiritProcessor | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 368 | BUILDING SpawnPoint | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 369 | BUILDING BlueprintStore | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 370 | BUILDING BlueprintConsole | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 371 | BUILDING BlueprintRelay | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 372 | BUILDING GodDish | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 373 | BUILDING FuelGenerator | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 374 | BUILDING FuelPipe | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 375 | BUILDING FuelStation | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 376 | BUILDING MultiwiniaZone | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 377 | BUILDING ControlStation | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 378 | BUILDING Crate | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 379 | BUILDING Statue | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 380 | BUILDING PulseBomb | Damage | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Damage:L582 [FULL] | yes |  |
| 381 | BUILDING RadarDish | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 382 | BUILDING LaserFence | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 383 | BUILDING GunTurret | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 384 | BUILDING Powerstation | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 385 | BUILDING Tree | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 386 | BUILDING TrunkPort | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 387 | BUILDING ResearchItem | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 388 | BUILDING Library | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 389 | BUILDING Generator | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 390 | BUILDING Pylon | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 391 | BUILDING SolarPanel | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 392 | BUILDING TrackLink | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 393 | BUILDING Refinery | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 394 | BUILDING Mine | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 395 | BUILDING Yard | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 396 | BUILDING Incubator | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 397 | BUILDING Triffid | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 398 | BUILDING SpiritReceiver | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 399 | BUILDING SpiritProcessor | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 400 | BUILDING SpawnPoint | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 401 | BUILDING BlueprintStore | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 402 | BUILDING BlueprintConsole | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 403 | BUILDING BlueprintRelay | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 404 | BUILDING GodDish | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 405 | BUILDING FuelGenerator | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 406 | BUILDING FuelPipe | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 407 | BUILDING FuelStation | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 408 | BUILDING MultiwiniaZone | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 409 | BUILDING ControlStation | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 410 | BUILDING Crate | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 411 | BUILDING Statue | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 412 | BUILDING PulseBomb | Explode | — (no data: silent) | 0 | code/worldobject/building.cpp:Building::Destroy:L597 [FULL] | yes |  |
| 413 | BUILDING Statue | Lift | — (no data: silent) | 0 | code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance:L240 [FULL] | yes |  |
| 414 | BUILDING Statue | Carry | — (no data: silent) | 0 | code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance:L241 [FULL] | yes |  |
| 415 | ENTITY Centipede | Panic | — (no data: silent) | 0 | code/worldobject/centipede.cpp:Centipede::Panic:L189 [FULL] | yes |  |
| 416 | ENTITY Centipede | Grow | — (no data: silent) | 0 | code/worldobject/centipede.cpp:Centipede::EatSpirits:L520 [FULL] | yes |  |
| 417 | ENTITY Centipede | EnemySighted | — (no data: silent) | 0 | code/worldobject/centipede.cpp:Centipede::SearchForTargetEnemy:L582 [FULL] | yes |  |
| 418 | BUILDING Crate | Expired | — (no data: silent) | 0 | code/worldobject/crate.cpp:Crate::AdvanceOnGround:L711 [FULL] | yes |  |
| 419 | ENTITY Darwinian | TakenControl | — (no data: silent) | 0 | code/worldobject/darwinian.cpp:Darwinian::TakeControl:L3531 [FULL] | yes |  |
| 420 | ENTITY Darwinian | EscapedControl | — (no data: silent) | 0 | code/worldobject/darwinian.cpp:Darwinian::LoseControl:L3549 [FULL] | yes |  |
| 421 | ENTITY Engineer | DropSpirit | — (no data: silent) | 0 | code/worldobject/engineer.cpp:Engineer::AdvanceToIncubator:L767 [FULL] | yes |  |
| 422 | ENTITY Engineer | ReprogrammingComplete | — (no data: silent) | 0 | code/worldobject/engineer.cpp:Engineer::AdvanceResearching:L879 [FULL] ; code/worldobject/engineer.cpp:Engineer::AdvanceReprogramming:L946 [FULL] | yes |  |
| 423 | ENTITY LaserTroop | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 424 | ENTITY Egg | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 425 | ENTITY TriffidEgg | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 426 | ENTITY Harvester | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] ; code/worldobject/harvester.cpp:Harvester::ChangeHealth:L165 [FULL] | yes |  |
| 427 | ENTITY Nuke | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 428 | ENTITY Meteor | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 429 | ENTITY SpaceShip | LoseHealth | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L182 [FULL] | yes |  |
| 430 | ENTITY Harvester | Die | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] ; code/worldobject/harvester.cpp:Harvester::ChangeHealth:L172 [FULL] | yes |  |
| 431 | ENTITY Nuke | Die | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 432 | ENTITY Meteor | Die | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 433 | ENTITY SpaceShip | Die | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::ChangeHealth:L189 [FULL] | yes |  |
| 434 | ENTITY Engineer | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 435 | ENTITY Squadie | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] ; code/worldobject/insertion_squad.cpp:Squadie::Attack:L953 [FULL] | yes |  |
| 436 | ENTITY Egg | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 437 | ENTITY SporeGenerator | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 438 | ENTITY SpaceInvader | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 439 | ENTITY Armour | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 440 | ENTITY TriffidEgg | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 441 | ENTITY Harvester | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 442 | ENTITY Nuke | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 443 | ENTITY Meteor | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 444 | ENTITY Tank | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 445 | ENTITY SpaceShip | Attack | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Attack:L225 [FULL] | yes |  |
| 446 | ENTITY LaserTroop | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 447 | ENTITY Engineer | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 448 | ENTITY Virii | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 449 | ENTITY Egg | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 450 | ENTITY SporeGenerator | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 451 | ENTITY Tripod | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 452 | ENTITY Centipede | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 453 | ENTITY SpaceInvader | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 454 | ENTITY Spider | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 455 | ENTITY Darwinian | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 456 | ENTITY ArmyAnt | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 457 | ENTITY Armour | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 458 | ENTITY SoulDestroyer | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 459 | ENTITY TriffidEgg | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 460 | ENTITY Harvester | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 461 | ENTITY Nuke | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 462 | ENTITY Meteor | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 463 | ENTITY Tank | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 464 | ENTITY SpaceShip | EnterTeleport | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::EnterTeleports:L278 [FULL] ; code/worldobject/entity.cpp:Entity::EnterTeleports:L295 [FULL] | yes |  |
| 465 | ENTITY LaserTroop | Create | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 466 | ENTITY Virii | Create | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 467 | ENTITY Egg | Create | — (no data: silent) | 0 | code/worldobject/entity.cpp:Entity::Begin:L379 [FULL] | yes |  |
| 468 | BUILDING Generator | Enable | — (no data: silent) | 0 | code/worldobject/generator.cpp:Generator::ReprogramComplete:L378 [FULL] | yes |  |
| 469 | BUILDING GunTurret | TargetSighted | — (no data: silent) | 0 | code/worldobject/gunturret.cpp:GunTurret::SearchForTargets:L327 [FULL] | yes |  |
| 470 | BUILDING GunTurret | FireGrenade | — (no data: silent) | 0 | code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L579 [FULL] | yes |  |
| 471 | BUILDING GunTurret | FireRocket | — (no data: silent) | 0 | code/worldobject/gunturret.cpp:GunTurret::PrimaryFire:L580 [FULL] | yes |  |
| 472 | ENTITY Squadie | ThrowGrenade | — (no data: silent) | 0 | code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L978 [FULL] | yes |  |
| 473 | ENTITY Squadie | ThrowController | — (no data: silent) | 0 | code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L993 [FULL] | yes |  |
| 474 | ENTITY Squadie | FireRocket | — (no data: silent) | 0 | code/worldobject/insertion_squad.cpp:Squadie::FireSecondaryWeapon:L999 [FULL] | yes |  |
| 475 | BUILDING LaserFence | Electrocute | — (no data: silent) | 0 | code/worldobject/laserfence.cpp:LaserFence::Electrocute:L568 [FULL] | yes |  |
| 476 | ENTITY Officer | SetOrderFollow | — (no data: silent) | 0 | code/worldobject/officer.cpp:Officer::SetOrders:L1400 [FULL] ; code/worldobject/officer.cpp:Officer::SetNextMode:L1715 [FULL] ; code/worldobject/officer.cpp:Officer::SetPreviousMode:L1878 [FULL] | yes |  |
| 477 | ENTITY Officer | SetOrderNone | — (no data: silent) | 0 | code/worldobject/officer.cpp:Officer::SetNextMode:L1711 [FULL] ; code/worldobject/officer.cpp:Officer::SetPreviousMode:L1874 [FULL] | yes |  |
| 478 | ENTITY Officer | SetOrderAbsorb | — (no data: silent) | 0 | code/worldobject/officer.cpp:Officer::SetNextMode:L1714 [FULL] ; code/worldobject/officer.cpp:Officer::SetPreviousMode:L1877 [FULL] | yes |  |
| 479 | ENTITY SoulDestroyer | Panic | — (no data: silent) | 0 | code/worldobject/souldestroyer.cpp:SoulDestroyer::Panic:L304 [FULL] | yes |  |
| 480 | ENTITY Spider | PounceLand | — (no data: silent) | 0 | code/worldobject/spider.cpp:Spider::AdvancePouncing:L525 [FULL] | yes |  |
| 481 | OTHER Spirit | Create | — (no data: silent) | 0 | code/worldobject/spirit.cpp:Spirit::Begin:L65 [commented-out] | no-live-site |  |
| 482 | OTHER Spirit | BeginAscent | — (no data: silent) | 0 | code/worldobject/spirit.cpp:Spirit::Advance:L123 [commented-out] | no-live-site |  |
| 483 | OTHER Spirit | Dropped | — (no data: silent) | 0 | code/worldobject/spirit.cpp:Spirit::CollectorDrops:L298 [commented-out] | no-live-site |  |
| 484 | OTHER Spirit | PlacedInEgg | — (no data: silent) | 0 | code/worldobject/spirit.cpp:Spirit::InEgg:L313 [commented-out] | no-live-site |  |
| 485 | OTHER Spirit | EggDestroyed | — (no data: silent) | 0 | code/worldobject/spirit.cpp:Spirit::EggDestroyed:L324 [commented-out] | no-live-site |  |
| 486 | BUILDING SpiritReceiver | TriggerSpirit | — (no data: silent) | 0 | code/worldobject/spiritreceiver.cpp:ReceiverBuilding::TriggerSpirit:L235 [FULL] | yes |  |
| 487 | BUILDING SpiritProcessor | TriggerSpirit | — (no data: silent) | 0 | code/worldobject/spiritreceiver.cpp:ReceiverBuilding::TriggerSpirit:L235 [FULL] | yes |  |
| 488 | BUILDING Triffid | LaunchEgg | — (no data: silent) | 0 | code/worldobject/triffid.cpp:Triffid::Launch:L340 [FULL] | yes |  |
| 489 | BUILDING TrunkPort | PowerUp | — (no data: silent) | 0 | code/worldobject/trunkport.cpp:TrunkPort::Advance:L116 [FULL] | yes |  |
| 490 | OTHER Laser | Create | — (no data: silent) | 0 | code/worldobject/weapons.cpp:Laser::Initialise:L599 [FULL] | yes |  |
| 491 | OTHER Laser | HitBuilding | — (no data: silent) | 0 | code/worldobject/weapons.cpp:Laser::Advance:L678 [FULL] | yes |  |
| 492 | OTHER Laser | HitEntity | — (no data: silent) | 0 | code/worldobject/weapons.cpp:Laser::Advance:L730 [commented-out] | no-live-site |  |
| 493 | OTHER Laser | KilledEntity | — (no data: silent) | 0 | code/worldobject/weapons.cpp:Laser::Advance:L765 [FULL] ; code/worldobject/weapons.cpp:TurretShell::Advance:L1415 [FULL] | yes |  |
