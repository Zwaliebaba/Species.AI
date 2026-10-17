# P04 helper: write coverage/P04_entities_military.csv
import csv, sys
out = sys.argv[1]
S0="0. Common conventions used in this partition"
S1="1. Projectile & effect classes"
S2="2. Airstrike and Napalm Strike"
S3="3. Nuke"
S4="4. Insertion Squad"
S5="5. Armour (troop transport)"
S6="6. Tank"
S7="7. Harvester"
S8="8. LaserTrooper"
S9="9. Shaman and Shaman Interface"
S10="10. Engineer"
S11="11. Crate-delivered military effects"
S12="12. Team colouring summary"
S13="13. Sound events and visual assets"
SX3="X3. Determinism notes"
S14="14. Consolidated constants table"
APP="Appendix — dead/legacy code"
rows = [
 ("code/shaman_interface.cpp","full","",[S9,S13,APP]),
 ("code/shaman_interface.h","full","",[S9]),
 ("code/worldobject/airstrike.cpp","full","",[S2,S11,S12,S13,S14]),
 ("code/worldobject/airstrike.h","full","",[S2]),
 ("code/worldobject/armour.cpp","full","",[S5,S11,S12,S13,SX3,S14,APP]),
 ("code/worldobject/armour.h","full","",[S5,S14]),
 ("code/worldobject/engineer.cpp","full","",[S10,S11,S12,S13,SX3,S14,APP]),
 ("code/worldobject/engineer.h","full","",[S10,S7,S14]),
 ("code/worldobject/harvester.cpp","full","",[S7,S11,S12,S13,SX3,S14]),
 ("code/worldobject/harvester.h","full","",[S7]),
 ("code/worldobject/insertion_squad.cpp","full","",[S4,S11,S13,SX3,S14,APP]),
 ("code/worldobject/insertion_squad.h","full","",[S4,S14]),
 ("code/worldobject/lasertrooper.cpp","full","",[S8,S12,APP]),
 ("code/worldobject/lasertrooper.h","full","",[S8]),
 ("code/worldobject/nuke.cpp","full","",[S3,S11,S12,S13,S14,APP]),
 ("code/worldobject/nuke.h","full","",[S3]),
 ("code/worldobject/shaman.cpp","full","",[S9,S12,S14,APP]),
 ("code/worldobject/shaman.h","full","",[S9]),
 ("code/worldobject/tank.cpp","full","",[S6,S12,S14,APP]),
 ("code/worldobject/tank.h","full","",[S6,S14]),
 ("code/worldobject/weapons.cpp","full","",[S1,S2,S11,S12,S13,SX3,S14,APP]),
 ("code/worldobject/weapons.h","full","",[S1]),
 ("data/stats.txt","full","",[S0,S2,S3,S4,S5,S6,S7,S8,S9,S10]),
 ("code/location.cpp","skimmed","read Bang, CreateShockwave, ThrowWeapon, FireRocket, FireTurretShell, LaunchFireball, FireLaser, Init research overrides, instant-unit spawning and team-control dispatch only (other partition)",[S0,S1,S4,S5,S6]),
 ("code/taskmanager.cpp","skimmed","Task::Start/Target*/Advance/Stop and RunTask weapon cases only (other partition)",[S2,S3,S4,S5,S7,S10]),
 ("code/worldobject/crate.cpp","skimmed","reward dispatch, IsValid, RunAirStrike, RunNuke only (P08)",[S2,S3,S11]),
 ("code/worldobject/entity.cpp","skimmed","EntityBlueprint, SetType, ChangeHealth, Attack, AdvanceDead/InAir/InWater, Advance, EntityIsBlocked",[S0]),
 ("code/worldobject/entity.h","skimmed","type/stat/damage enums",[S0]),
 ("code/worldobject/darwinian.cpp","skimmed","armour boarding/inside/search, rocket/grenade throw, FireSubversion only",[S5,S1]),
 ("code/worldobject/spirit.cpp","skimmed","Spirit::Advance state handling for harvest/collect",[S7]),
 ("code/worldobject/spawnpoint.cpp","skimmed","SpawnPoint::SpawnDarwinian only",[S7,S10]),
 ("code/unit.cpp","skimmed","GetFormationOffset, Attack accumulator, LaserTroop target logic",[S2,S8]),
 ("code/team.cpp","skimmed","Team::NewUnit only",[S2]),
 ("code/gametimer.cpp","skimmed","GetServerAdvancePeriod",[S0]),
 ("code/gametimer.h","full","",[S0]),
 ("contrib/systemiv/lib/math/random_number.cpp","skimmed","AppRandom, frand/sfrand, _syncfrand/_syncsfrand",[S0,SX3]),
 ("contrib/systemiv/lib/math/random_number.h","full","",[S0,SX3]),
 ("code/lib/vector3.cpp","skimmed","FromRand wrappers, FastRotateAround, RotateAround",[S0,S3]),
 ("code/lib/vector3.h","skimmed","FRAND/SFRAND macros, cross product",[S0,S3]),
 ("code/worldobject/worldobject.cpp","skimmed","constructor defaults, BounceOffLandscape, COEF_OF_RESTITUTION",[S1]),
 ("code/worldobject/worldobject.h","skimmed","Effect type enum",[S1]),
 ("code/multiwinia.cpp","skimmed","AdvanceTankBattle, AdvanceTrunkPorts armour block, game-type dispatch",[S5,S6]),
 ("code/location_input.cpp","skimmed","armour/squad input branches, quick-run units, toggle-entity",[S4,S5]),
 ("code/network/clienttoserver.cpp","skimmed","RequestArmourOrders, ORDERGOTO, TOGGLEENTITY, NEXT/PREVWEAPON, SHAMANSUMMON, RUNPROGRAM handlers",[S5,S9]),
 ("code/taskmanager_interface_multiwinia.cpp","skimmed","keyboard shortcuts, SelectWeapon zone, shaman hijack",[S4,S9]),
 ("code/level_file.cpp","skimmed","ParseInstantUnits blocked-entity filter",[S0,S2]),
 ("code/global_world.cpp","skimmed","GlobalResearch::CurrentLevel",[S0]),
 ("code/app.cpp","skimmed","IsSinglePlayer, Multiplayer, difficulty default",[S0]),
 ("docs/multiwinia_manual.pdf","skimmed","unit sections only (pdftotext)",["Docs-vs-code discrepancies"]),
 ("docs/multiwinia-changes.txt","skimmed","grep for unit names",["Docs-vs-code discrepancies"]),
 ("docs/multiwiniachanges-beta.txt","skimmed","grep for unit names",["Docs-vs-code discrepancies"]),
]
with open(out,'w',newline='') as f:
    w=csv.writer(f)
    w.writerow(["path","depth","reason","sections"])
    for p,d,r,sec in rows:
        w.writerow([p,d,r,";".join(sec)])
print(len(rows))
