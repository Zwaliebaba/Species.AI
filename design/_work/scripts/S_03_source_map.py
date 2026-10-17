"""S03: build the Source-map table for 03_buildings_and_world_objects.md.
Usage: python3 -I S_03_source_map.py <manifest.csv> <doc.md> <levels_enum.txt>
Code/doc rows come from the CODE table below; data rows are generated: every data/levels file in the
manifest, and every data/shapes|textures|icons|sprites file whose base name is cited in the document.
Every emitted path is checked against the manifest (case-insensitive, manifest spelling used).
"""
import sys, csv, re, os

CODE = [
 ('code/worldobject/building.h', 'Building class, type enum (75 values), BuildingPort'),
 ('code/worldobject/building.cpp', 'Building base behaviour, ports, damage, Read/Write, factory, name table, BuildingIsBlocked'),
 ('code/worldobject/spawnpoint.h', 'SpawnBuilding, SpawnBuildingLink, SpawnBuildingSpirit, SpawnLink, MasterSpawnPoint, SpawnPoint, SpawnPopulationLock declarations'),
 ('code/worldobject/spawnpoint.cpp', 'spawn network behaviour, capture, activation, population lock'),
 ('code/worldobject/gunturret.h', 'GunTurret constants, turret types, GunTurretTarget'),
 ('code/worldobject/gunturret.cpp', 'GunTurret capture, targeting, firing, damage; GunTurretTarget'),
 ('code/worldobject/teleport.h', 'Teleport base declaration'),
 ('code/worldobject/teleport.cpp', 'Teleport transit and unit re-assembly'),
 ('code/worldobject/radardish.h', 'RadarDish constants'),
 ('code/worldobject/radardish.cpp', 'RadarDish aiming, connection, transport'),
 ('code/worldobject/controlstation.h', 'ControlStation declaration'),
 ('code/worldobject/controlstation.cpp', 'ControlStation capture and bomb linking'),
 ('code/worldobject/jumppad.h', 'JumpPad declaration'),
 ('code/worldobject/jumppad.cpp', 'JumpPad launch behaviour'),
 ('code/worldobject/generator.h', 'PowerBuilding family declarations'),
 ('code/worldobject/generator.cpp', 'PowerBuilding, SolarPanel, Pylon, PylonEnd, PowerSplitter, Generator, PylonStart'),
 ('code/worldobject/powerstation.h', 'Powerstation declaration'),
 ('code/worldobject/powerstation.cpp', 'Powerstation (legacy)'),
 ('code/worldobject/laserfence.h', 'LaserFence modes and constants'),
 ('code/worldobject/laserfence.cpp', 'LaserFence behaviour, power, hit tests, rendering'),
 ('code/worldobject/wall.h', 'Wall declaration'),
 ('code/worldobject/wall.cpp', 'Wall constants, damage, hit tests'),
 ('code/worldobject/tree.h', 'Tree members'),
 ('code/worldobject/tree.cpp', 'Tree generation, fire, propagation, evil trees'),
 ('code/worldobject/staticshape.h', 'StaticShape declaration'),
 ('code/worldobject/staticshape.cpp', 'StaticShape behaviour'),
 ('code/worldobject/restrictionzone.h', 'RestrictionZone declaration'),
 ('code/worldobject/restrictionzone.cpp', 'RestrictionZone registry and IsRestricted'),
 ('code/worldobject/smokemarker.h', 'SmokeMarker declaration (float signature bug)'),
 ('code/worldobject/smokemarker.cpp', 'SmokeMarker particles'),
 ('code/worldobject/eruptionmarker.h', 'EruptionMarker declaration'),
 ('code/worldobject/eruptionmarker.cpp', 'EruptionMarker eruptions'),
 ('code/worldobject/multiwiniazone.h', 'MultiwiniaZone members'),
 ('code/worldobject/multiwiniazone.cpp', 'KOTH / CTS / Blitzkrieg zone behaviour'),
 ('code/worldobject/flag.h', 'Flag declaration'),
 ('code/worldobject/flag.cpp', 'Flag cloth rendering'),
 ('code/worldobject/carryablebuilding.h', 'CarryableBuilding declaration'),
 ('code/worldobject/carryablebuilding.cpp', 'carrying, ownership, movement'),
 ('code/worldobject/statue.h', 'Statue declaration'),
 ('code/worldobject/statue.cpp', 'Statue behaviour'),
 ('code/worldobject/wallbuster.h', 'WallBuster declaration'),
 ('code/worldobject/wallbuster.cpp', 'WallBuster breach and scuttle'),
 ('code/worldobject/trunkport.h', 'TrunkPort declaration'),
 ('code/worldobject/trunkport.cpp', 'TrunkPort population lock and rendering'),
 ('code/worldobject/pulsebomb.h', 'PulseBomb declaration'),
 ('code/worldobject/pulsebomb.cpp', 'PulseBomb countdown, capture, detonation'),
 ('code/worldobject/rocket.h', 'FuelBuilding family and EscapeRocket states'),
 ('code/worldobject/rocket.cpp', 'fuel chain and EscapeRocket state machine'),
 ('code/worldobject/anthill.h', 'AntHill declaration'),
 ('code/worldobject/anthill.cpp', 'AntHill behaviour'),
 ('code/worldobject/triffid.h', 'Triffid and TriffidEgg declarations, spawn enum'),
 ('code/worldobject/triffid.cpp', 'Triffid behaviour and egg spawning'),
 ('code/worldobject/portal.h', 'Portal states'),
 ('code/worldobject/portal.cpp', 'Portal (The Beast) behaviour'),
 ('code/worldobject/randomiser.h', 'Randomiser declaration'),
 ('code/worldobject/randomiser.cpp', 'Randomiser effect'),
 ('code/worldobject/ai.h', 'AITarget, AISpawnPoint declarations'),
 ('code/worldobject/ai.cpp', 'AITarget graph and AISpawnPoint'),
 ('code/worldobject/aiobjective.h', 'AIObjective, AIObjectiveMarker declarations'),
 ('code/worldobject/aiobjective.cpp', 'Assault objective chain'),
 ('code/worldobject/controltower.h', 'ControlTower declaration'),
 ('code/worldobject/controltower.cpp', 'ControlTower reprogramming'),
 ('code/worldobject/incubator.h', 'Incubator constants'),
 ('code/worldobject/incubator.cpp', 'Incubator spirit conversion'),
 ('code/worldobject/clonelab.h', 'CloneLab declaration'),
 ('code/worldobject/clonelab.cpp', 'CloneLab capture'),
 ('code/worldobject/constructionyard.h', 'ConstructionYard, DisplayScreen declarations'),
 ('code/worldobject/constructionyard.cpp', 'ConstructionYard, DisplayScreen (legacy)'),
 ('code/worldobject/spiritreceiver.h', 'receiver network and UnprocessedSpirit declarations'),
 ('code/worldobject/spiritreceiver.cpp', 'receiver network (legacy)'),
 ('code/worldobject/spiritstore.h', 'SpiritStore declaration'),
 ('code/worldobject/spiritstore.cpp', 'SpiritStore (legacy)'),
 ('code/worldobject/factory.h', 'Factory declaration'),
 ('code/worldobject/factory.cpp', 'Factory (legacy)'),
 ('code/worldobject/blueprintstore.h', 'Blueprint family declarations'),
 ('code/worldobject/blueprintstore.cpp', 'Blueprint family (legacy)'),
 ('code/worldobject/bridge.h', 'Bridge constants'),
 ('code/worldobject/bridge.cpp', 'Bridge (legacy)'),
 ('code/worldobject/cave.h', 'Cave declaration'),
 ('code/worldobject/cave.cpp', 'Cave (legacy)'),
 ('code/worldobject/chess.h', 'ChessBase, ChessPiece declarations'),
 ('code/worldobject/chess.cpp', 'chess prototype (legacy)'),
 ('code/worldobject/feedingtube.h', 'FeedingTube declaration'),
 ('code/worldobject/feedingtube.cpp', 'FeedingTube (legacy)'),
 ('code/worldobject/generichub.h', 'DynamicBase/Hub/Node declarations'),
 ('code/worldobject/generichub.cpp', 'dynamic objective network (legacy)'),
 ('code/worldobject/goddish.h', 'GodDish declaration'),
 ('code/worldobject/goddish.cpp', 'GodDish (legacy)'),
 ('code/worldobject/spam.h', 'Spam declaration'),
 ('code/worldobject/spam.cpp', 'Spam building (legacy)'),
 ('code/worldobject/library.h', 'Library declaration'),
 ('code/worldobject/library.cpp', 'Library (legacy)'),
 ('code/worldobject/researchitem.h', 'ResearchItem declaration'),
 ('code/worldobject/researchitem.cpp', 'ResearchItem (legacy)'),
 ('code/worldobject/mine.h', 'MineBuilding family and MineCart declarations'),
 ('code/worldobject/mine.cpp', 'mine conveyor (legacy)'),
 ('code/worldobject/safearea.h', 'SafeArea declaration'),
 ('code/worldobject/safearea.cpp', 'SafeArea (legacy)'),
 ('code/worldobject/scripttrigger.h', 'ScriptTrigger declaration'),
 ('code/worldobject/scripttrigger.cpp', 'ScriptTrigger (legacy)'),
 ('code/worldobject/switch.h', 'FenceSwitch declaration'),
 ('code/worldobject/switch.cpp', 'FenceSwitch (legacy)'),
 ('code/worldobject/upgrade_port.h', 'UpgradePort, PrimaryUpgradePort declarations'),
 ('code/worldobject/upgrade_port.cpp', 'UpgradePort family (legacy)'),
 ('code/worldobject/crate.h', 'INCLUDE_THEBEAST gating, crate rewards that create buildings'),
 ('code/worldobject/crate.cpp', 'runtime creators: RunGunTurret, RunAntNest, RunTriffids, RunTheBeast, RunRandomiser, GenerateTrees'),
 ('code/worldobject/darwinian.h', 'search ranges for ports and carryables'),
 ('code/worldobject/darwinian.cpp', 'Darwinian side of ports, carrying, building attack, fence/wall push-out'),
 ('code/worldobject/entity.cpp', 'Entity::EnterTeleports, PushFromObstructions (fence lethality)'),
 ('code/worldobject/engineer.cpp', 'Engineer bridge, tower reprogramming, spirit delivery'),
 ('code/worldobject/weapons.cpp', 'projectile damage to buildings'),
 ('code/worldobject/armyant.cpp', 'ant delivery to AntHill'),
 ('code/worldobject/armour.cpp', 'Armour conversion to GunTurret, fence route check'),
 ('code/worldobject/lightning.cpp', 'tree ignition'),
 ('code/worldobject/nuke.cpp', 'explosion damage to buildings'),
 ('code/worldobject/meteor.cpp', 'explosion damage to buildings'),
 ('code/worldobject/lander.cpp', 'explosion damage to buildings'),
 ('code/worldobject/worldobject.h', 'WorldObjectId (unsigned char team)'),
 ('code/location.cpp', 'building instantiation, AdvanceBuildings, Bang, IsFriend, GetTeamFromPosition, InitialiseTeam, CreateRandomCrate'),
 ('code/level_file.cpp', 'ParseBuildings: name lookup, global asserts, difficulty scaling'),
 ('code/obstruction_grid.cpp', 'building rasterisation (CalculateAll, CalculateBuildingArea)'),
 ('code/multiwinia.cpp', 'mode rules reading buildings; GetGameOption; s_numMultiwiniaZones; AdvanceTrunkPorts'),
 ('code/gametimer.h', 'SERVER_ADVANCE_PERIOD, GetNetworkTime'),
 ('code/gametimer.cpp', 'game clock and tick period'),
 ('code/globals.h', 'NUM_TEAMS, NUM_SLICES_PER_FRAME'),
 ('code/team.cpp', 'Team::Eliminate (trunk ports), SelectUnit (turrets)'),
 ('code/taskmanager.cpp', 'turret tasks, IsValidTargetArea (RestrictionZone)'),
 ('code/taskmanager.h', 'POWERUP_EFFECT_RANGE'),
 ('code/location_input.cpp', 'selection of turrets, dishes, FenceSwitch::Switch call'),
 ('code/network/clienttoserver.cpp', 'NET_DARWINIA_AIMBUILDING and TOGGLELASERFENCE handling'),
 ('code/lib/resource.cpp', 'shape loading, CreateDisplayListAsync (tree hit volume)'),
 ('code/lib/universal_include.h', 'BLOCK_OLD_DARWINIA_OBJECTS, INCLUDE_CRATES_ADVANCED, game-mode macros'),
 ('code/particle_system.cpp', 'particle types used by markers, trees, rockets'),
 ('code/explosion.cpp', 'shape-shattering explosions for buildings'),
 ('code/script.cpp', 'GodDish/Spam script commands (legacy)'),
 ('code/global_world.cpp', 'GlobalBuilding records, GenerateBuildingId'),
 ('code/interface/buildings_window.cpp', 'editor building palette honours the block list [B:EDITOR]'),
 ('code/interface/tree_window.cpp', 'editor ranges for tree parameters [B:EDITOR]'),
 ('data/levels/level_enum.txt', 'official map list (census definition)'),
 ('data/multiwinia.txt', 'game-option blueprints read by GetGameOption (TurretFrequency etc.)'),
 ('data/sounds.txt', 'sound bindings for building events'),
 ('data/stats.txt', 'entity stats referenced by building outputs'),
 ('docs/game objects.txt', 'design notes on trees, fences, walls (docs-vs-code)'),
 ('docs/new building docs.txt', 'design notes on FenceSwitch and dynamic hubs (docs-vs-code)'),
 ('docs/spirit resource system.txt', 'design note on spirits (docs-vs-code)'),
]

def main():
    man, doc, enumf = sys.argv[1], sys.argv[2], sys.argv[3]
    paths = {}
    with open(man, newline='', encoding='utf-8') as f:
        for r in csv.DictReader(f):
            paths[r['path'].lower()] = r['path']
    text = open(doc, encoding='utf-8').read()
    official = set(l.split()[0].lower() for l in open(enumf, encoding='latin-1') if l.split())
    rows = []
    missing = []
    for p, role in CODE:
        if p.lower() in paths: rows.append((paths[p.lower()], role))
        else: missing.append(p)
    for lp in sorted(v for k, v in paths.items() if k.startswith('data/levels/') and k.endswith('.txt')):
        base = os.path.basename(lp).lower()
        if lp.lower() == 'data/levels/level_enum.txt': continue
        if '/old/' in lp.lower(): role = 'prototype map (old): building lines (census only, [B:LEGACY])'
        elif base.startswith('mp_') and base in official: role = 'official map: Buildings block (placement, census)'
        elif base.startswith('mp_'): role = 'dev map: Buildings block (census)'
        else: role = 'Darwinia campaign map/mission: building lines (census only, [B:LEGACY])'
        rows.append((lp, role))
    cited = set(m.lower() for m in re.findall(r'([A-Za-z0-9_\-]+\.(?:shp|bmp))', text))
    carry = re.findall(r'`(carry[a-z]+)`', text)
    cited |= set(c.lower() + '.shp' for c in carry)
    for k, v in sorted(paths.items()):
        if any(k.startswith(d) for d in ('data/shapes/', 'data/textures/', 'data/icons/', 'data/sprites/')):
            if os.path.basename(k) in cited:
                kind = 'shape' if k.endswith('.shp') else 'texture'
                rows.append((v, f'{kind} used by a building class (render/collision/markers)'))
    print('| File | Role |')
    print('|---|---|')
    for p, role in rows:
        print(f'| `{p}` | {role} |')
    if missing:
        print('<!-- missing from manifest: ' + ', '.join(missing) + ' -->')
    sys.stderr.write(f'rows={len(rows)} missing={missing}\n')

if __name__ == '__main__':
    main()
